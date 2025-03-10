#include "./comp_node.h"
#include "megbrain/comp_node_env.h"
#include "megbrain/utils/thread.h"

#include <string>

using namespace mgb;

#if MGB_CUDA

#include "megbrain/comp_node/alloc.h"

#include <cctype>
#include <cstdio>

#include <thread>

#include <cuda.h>
#include <cuda_runtime.h>

#ifdef __unix__
#include <sys/wait.h>
#include <unistd.h>
#endif

using CudaCompNodeImpl = CudaCompNode::CompNodeImpl;

namespace {
size_t get_min_system_memory(size_t available) {
    if (available < (1u << 31)) {
        // 225MiB
        return 225 * 1024 * 1024;
    } else {
        // max(300 MiB, 0.05 * available)
        return std::max<size_t>(300 * 1024 * 1024, available / 20);
    }
}
using CudaHostFunc = megdnn::thin_function<void()>;
void CUDART_CB cuda_host_func_caller(void* ud) {
    mgb_assert(ud);
    CudaHostFunc* func_ptr = reinterpret_cast<CudaHostFunc*>(ud);
    MGB_TRY { (*func_ptr)(); }
    MGB_FINALLY(delete func_ptr;);
}
}  // anonymous namespace

namespace mgb {
namespace mem_alloc {
class CudaRawAllocator final : public RawAllocator {
public:
    void* alloc(size_t size) override {
        void* addr;
        cudaError_t cuda_error = cudaMalloc(&addr, size);
        if (cuda_error == cudaSuccess) {
            mgb_assert(addr);
            return addr;
        }
        auto msg = mgb_ssprintf_log(
                "cudaMalloc failed while requesting %zd bytes (%.3fMiB)"
                " of memory; error: %s",
                size, size / (1024.0 * 1024), cudaGetErrorString(cuda_error));
        msg.append(CudaError::get_cuda_extra_info());
        if (cuda_error == cudaErrorMemoryAllocation) {
            mgb_log_error("%s", msg.c_str());
            // clear cuda error
            cudaGetLastError();
            mgb_assert(cudaGetLastError() == cudaSuccess);
            return nullptr;
        }
        mgb_throw_raw(MemAllocError{msg});
    }

    void free(void* ptr) override {
        cudaError_t cuda_error = cudaFree(ptr);
        if (cuda_error == cudaSuccess)
            return;
        auto msg = ssprintf(
                "cudaFree failed for %p: %s", ptr, cudaGetErrorString(cuda_error));
        msg.append(CudaError::get_cuda_extra_info());
        mgb_throw_raw(MemAllocError{msg});
    }

    void get_mem_info(size_t& free, size_t& tot) override {
        cudaError_t cuda_error = cudaMemGetInfo(&free, &tot);
        if (cuda_error == cudaSuccess)
            return;
        auto msg = ssprintf("cudaMemGetInfo failed %s", cudaGetErrorString(cuda_error));
        msg.append(CudaError::get_cuda_extra_info());
        mgb_throw_raw(MegBrainError{msg});
    }
};

class CudaHostAllocator : public RawAllocator {
public:
    void* alloc(size_t size) override {
        void* addr;
        cudaError_t cuda_error = cudaHostAlloc(&addr, size, cudaHostAllocDefault);
        if (cuda_error == cudaSuccess) {
            mgb_assert(addr);
            return addr;
        }
        auto msg = mgb_ssprintf_log(
                "cudaHostAlloc failed while requesting %zd bytes (%.3fMiB)"
                " of pinned host memory; error: %s",
                size, size / (1024.0 * 1024), cudaGetErrorString(cuda_error));
        msg.append(CudaError::get_cuda_extra_info());
        if (cuda_error == cudaErrorMemoryAllocation) {
            mgb_log_error("%s", msg.c_str());
            // clear cuda error
            cudaGetLastError();
            mgb_assert(cudaGetLastError() == cudaSuccess);
            return nullptr;
        }
        mgb_throw_raw(MemAllocError{msg});
    }

    void free(void* ptr) override {
        cudaError_t cuda_error = cudaFreeHost(ptr);
        if (cuda_error == cudaSuccess)
            return;
        auto msg = ssprintf(
                "cudaFreeHost failed for %p: %s", ptr, cudaGetErrorString(cuda_error));
        msg.append(CudaError::get_cuda_extra_info());
        mgb_throw_raw(MemAllocError{msg});
    }

    void get_mem_info(size_t& free, size_t& tot) override {
        free = 0;
        tot = 0;
    }
};

class CudaDeviceRuntimePolicy : public DeviceRuntimePolicy {
public:
    CompNode::DeviceType device_type() override { return CompNode::DeviceType::CUDA; }
    void set_device(int device) override { MGB_CUDA_CHECK(cudaSetDevice(device)); }
    void device_synchronize(int device) override {
        MGB_CUDA_CHECK(cudaSetDevice(device));
        MGB_CUDA_CHECK(cudaDeviceSynchronize());
    }
};

/* ===================== DevMemAlloc  ===================== */
std::unique_ptr<DevMemAlloc> DevMemAlloc::make_cuda_alloc() {
    return std::make_unique<FwdDevMemAlloc>(std::make_shared<CudaRawAllocator>());
}
}  // namespace mem_alloc
}  // namespace mgb

/* ===================== CudaCompNodeImpl  ===================== */
class CudaCompNode::CompNodeImpl final : public CompNode::Impl {
    MGB_DYN_TYPE_OBJ_FINAL_DECL;

    friend class EventImpl;
    friend class CudaCompNode;

    struct DeviceInfo;
    struct StaticData;
    static StaticData* sd;
    static Spinlock sd_mtx;
#if !MGB_BUILD_SLIM_SERVING
    std::mutex m_update_mem;
#endif

    //! set to true when m_locator is assigned; set to false if async init
    //! failed
    bool m_initialized = false;
    Locator m_locator, m_locator_logical;
    mem_alloc::StreamMemAlloc* m_mem_alloc;
    DeviceInfo* m_device_info;

    std::unique_ptr<Event> m_sync_event;
    Spinlock m_sync_event_mtx;

    void activate() { m_env.cuda_env().activate(); }

    void init(const Locator& locator, const Locator& locator_logical);
    void fini();

    //! return whether global finalized, and print warning in such case
    static inline bool check_global_finalized();

    static CompNode::DeviceProperties get_device_prop(int dev);

    //! enable peer copy from dev0 to dev1
    static void enable_peer_access(int dev0, int dev1);

    static void static_free_device(ImplBase* self, void* ptr) {
        static_cast<CompNodeImpl*>(self)->free_device(ptr);
    }

    static void static_free_host(ImplBase* self, void* ptr) {
        static_cast<CompNodeImpl*>(self)->free_host(ptr);
    }

public:
    CompNodeImpl() : Impl(static_free_device, static_free_host) {}

    static constexpr int MAX_NR_COMP_NODE = 1024, MAX_NR_DEVICE = 64;

    void* alloc_device(size_t size) override;

    void free_device(void* ptr);

    void* alloc_host(size_t size) override;

    void free_host(void* ptr);

    void copy_to_host(void* host_ptr, const void* device_ptr, size_t size) override {
        activate();
        MGB_CUDA_CHECK(cudaMemcpyAsync(
                host_ptr, device_ptr, size, cudaMemcpyDeviceToHost,
                m_env.cuda_env().stream));
    }

    void copy_to_device(void* device_ptr, const void* host_ptr, size_t size) override {
        activate();
        MGB_CUDA_CHECK(cudaMemcpyAsync(
                device_ptr, host_ptr, size, cudaMemcpyHostToDevice,
                m_env.cuda_env().stream));
    }

    void peer_copy_to(
            Impl* dest_impl, void* dest, const void* src, size_t size) override;

    size_t get_mem_addr_alignment() override { return m_env.property().mem_alignment; }

    std::unique_ptr<Event> create_event(size_t flags) override;

    void sync() override;

    MemNode mem_node() override;

    std::pair<size_t, size_t> get_mem_status_bytes() override {
        // explicitly call cuda_env() to ensure async init is finished
        m_env.cuda_env().activate();
        size_t tot, free;
        MGB_CUDA_CHECK(cudaMemGetInfo(&free, &tot));
        free += m_mem_alloc->get_free_memory_dev().tot;
        return {tot, free};
    }

#if !MGB_BUILD_SLIM_SERVING
    std::pair<size_t, size_t> get_free_left_and_right(
            size_t begin_ptr, size_t end_ptr) override {
        return m_mem_alloc->get_free_left_and_right(begin_ptr, end_ptr);
    }

    size_t get_max_block_size_available() override {
        activate();
        return m_mem_alloc->get_max_block_size_available();
    }

    size_t get_free_mem() override {
        m_env.cuda_env().activate();
        size_t tot, free;
        MGB_CUDA_CHECK(cudaMemGetInfo(&free, &tot));
        return free;
    }
#endif

    Locator locator() override { return m_locator; }

    Locator locator_logical() override { return m_locator_logical; }

    void add_callback(CudaHostFunc&& cb) override {
#if CUDART_VERSION >= 10000
        activate();
        CudaHostFunc* func_ptr = new CudaHostFunc(std::move(cb));
        MGB_TRY {
            MGB_CUDA_CHECK(cudaLaunchHostFunc(
                    m_env.cuda_env().stream, cuda_host_func_caller,
                    static_cast<void*>(func_ptr)));
        }
        MGB_CATCH(..., {
            delete func_ptr;
            throw;
        });
#else
        MGB_MARK_USED_VAR(cb);
        MGB_MARK_USED_VAR(cuda_host_func_caller);
        mgb_throw(
                MegBrainError,
                "add_callback only support in cuda10.0 and later version");
#endif
    }

    uint64_t get_uid() override { return m_uid; }

#if !MGB_BUILD_SLIM_SERVING
    size_t get_used_memory() override;

    size_t get_max_used_memory() override;

    size_t get_reserved_memory() override;

    size_t get_max_reserved_memory() override;

    void reset_max_used_memory() override;
    void reset_max_reserved_memory() override;
#endif

private:
    uint64_t m_uid;
#if !MGB_BUILD_SLIM_SERVING
    std::unordered_map<void*, size_t> ptr2size;
#endif
};
MGB_DYN_TYPE_OBJ_FINAL_IMPL(CudaCompNode::CompNodeImpl);

struct CudaCompNodeImpl::DeviceInfo {
    int dev_num = -1;
    std::atomic_size_t m_used_mem{0};
    std::atomic_size_t m_max_used_mem{0};
    std::unique_ptr<mem_alloc::DevMemAlloc> mem_alloc;

    bool init_done() const { return mem_alloc.get(); }

    void init(const CompNodeEnv& env);

    void fini() { mem_alloc.reset(); }
};

struct CudaCompNodeImpl::StaticData {
    std::recursive_mutex mtx;

    mem_alloc::DevMemAlloc::PreAllocConfig prealloc_config;

    std::unique_ptr<mem_alloc::SimpleCachingAlloc> host_alloc;
    CudaCompNode::CompNodeImpl node[MAX_NR_COMP_NODE];
    DeviceInfo dev_info[MAX_NR_DEVICE];
    int nr_node = 0,          //!< number of loaded node[]
            nr_dev_used = 0;  //!< number of used dev_info[]

    StaticData()
            : host_alloc(mem_alloc::SimpleCachingAlloc::make(
                      std::make_unique<mem_alloc::CudaHostAllocator>())) {
        prealloc_config.max_overhead = 0;
        prealloc_config.alignment = 1;
        host_alloc->alignment(1);
    }

    ~StaticData() {
        for (int i = 0; i < nr_node; ++i)
            node[i].fini();
        for (int i = 0; i < nr_dev_used; ++i)
            dev_info[i].fini();
    }

    static size_t get_mem_reserve_size() {
        if (auto setting = MGB_GETENV("MGB_CUDA_RESERVE_MEMORY")) {
            if (!strncmp(setting, "b:", 2)) {
                return std::stoull(setting + 2);
            }
            size_t tot, free;
            MGB_CUDA_CHECK(cudaFree(0));
            MGB_CUDA_CHECK(cudaMemGetInfo(&free, &tot));
            return free - get_min_system_memory(free);
        } else {
            return 0;
        }
    }
};
CudaCompNodeImpl::StaticData* CudaCompNodeImpl::sd = nullptr;
Spinlock CudaCompNodeImpl::sd_mtx;

struct DevicePropRec {
    bool init = false;
    CompNode::DeviceProperties prop;
    Spinlock mtx_com;
};
DevicePropRec device_prop_rec[CudaCompNodeImpl::MAX_NR_DEVICE];

void CudaCompNodeImpl::init(const Locator& locator, const Locator& locator_logical) {
    m_locator = locator;
    m_locator_logical = locator_logical;
    m_initialized = true;

#if defined(__linux__) || defined(TARGET_OS_MAC)
    FILE* fp;
    fp = fopen("/dev/urandom", "r");
    mgb_assert(fread(&m_uid, sizeof(m_uid), 1, fp) == 1);
    fclose(fp);
#else
    m_uid = std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();
#endif

    auto on_succ = [this](cudaStream_t stream) {
        auto locator = m_locator;
        log_comp_node_created(locator, m_locator_logical);

        MGB_LOCK_GUARD(sd->mtx);
        DeviceInfo* dev_info = nullptr;
        for (int i = 0; i < sd->nr_dev_used; ++i) {
            if (sd->dev_info[i].dev_num == locator.device) {
                dev_info = &sd->dev_info[i];
                break;
            }
        }

        if (!dev_info) {
            dev_info = &sd->dev_info[sd->nr_dev_used];
            dev_info->init(m_env);
            // note: add nr_dev_used only after init succeeds
            ++sd->nr_dev_used;
        }
        m_device_info = dev_info;
        m_mem_alloc = dev_info->mem_alloc->add_stream(static_cast<void*>(stream));
    };

    auto on_error = [this](std::exception&) {
        MGB_LOCK_GUARD(sd->mtx);
        m_initialized = false;
    };

    m_env.init_cuda_async(
            locator.device, make_comp_node_from_impl(this), {on_succ, on_error});
}

void CudaCompNodeImpl::fini() {
    if (!m_initialized)
        return;

    m_sync_event.reset();
    m_env.fini();
    m_mem_alloc = nullptr;
    m_device_info = nullptr;
    m_initialized = false;
}

void* CudaCompNodeImpl::alloc_device(size_t size) {
    activate();
#if MGB_BUILD_SLIM_SERVING
    return m_mem_alloc->alloc(size);
#else
    void* ptr = m_mem_alloc->alloc(size);
    {
        MGB_LOCK_GUARD(m_update_mem);
        ptr2size[ptr] = size;
        m_device_info->m_used_mem += size;
        if (m_device_info->m_used_mem > m_device_info->m_max_used_mem) {
            m_device_info->m_max_used_mem = m_device_info->m_used_mem.load();
        }
    }
    return ptr;
#endif
}

void CudaCompNodeImpl::free_device(void* ptr) {
    if (check_global_finalized())
        return;

    activate();
#if !MGB_BUILD_SLIM_SERVING
    {
        MGB_LOCK_GUARD(m_update_mem);
        mgb_assert(ptr2size.find(ptr) != ptr2size.end(), "ptr %p not found!", ptr);
        m_device_info->m_used_mem -= ptr2size.at(ptr);
        ptr2size.erase(ptr);
    }
#endif
    m_mem_alloc->free(ptr);
}

#if !MGB_BUILD_SLIM_SERVING
size_t CudaCompNodeImpl::get_used_memory() {
    return m_device_info->m_used_mem.load();
}

size_t CudaCompNodeImpl::get_max_used_memory() {
    return m_device_info->m_max_used_mem.load();
}

void CudaCompNodeImpl::reset_max_used_memory() {
    m_device_info->m_max_used_mem = 0;
}

size_t CudaCompNodeImpl::get_reserved_memory() {
    return m_device_info->mem_alloc->get_used_memory();
}

size_t CudaCompNodeImpl::get_max_reserved_memory() {
    return m_device_info->mem_alloc->get_max_used_memory();
}

void CudaCompNodeImpl::reset_max_reserved_memory() {
    m_device_info->mem_alloc->reset_max_used_memory();
}
#endif

void* CudaCompNodeImpl::alloc_host(size_t size) {
    // need activate because it create cuda cuda context in current device
    activate();
    return sd->host_alloc->alloc(size);
}

void CudaCompNodeImpl::free_host(void* ptr) {
    if (check_global_finalized())
        return;
    sd->host_alloc->free(ptr);
}

void CudaCompNodeImpl::peer_copy_to(
        Impl* dest_impl, void* dest, const void* src, size_t size) {
    if (dest_impl->same_type<CudaCompNodeImpl>()) {
        auto&& dst_env = static_cast<CudaCompNodeImpl*>(dest_impl)->m_env.cuda_env();
        auto&& src_env = m_env.cuda_env();
        activate();
        if (dst_env.device == src_env.device) {
            MGB_CUDA_CHECK(cudaMemcpyAsync(
                    dest, src, size, cudaMemcpyDeviceToDevice, dst_env.stream));
        } else {
            enable_peer_access(src_env.device, dst_env.device);
            enable_peer_access(dst_env.device, src_env.device);
            MGB_CUDA_CHECK(cudaMemcpyPeerAsync(
                    dest, dst_env.device, src, src_env.device, size, dst_env.stream));
        }
        return;
    }
    mgb_assert(
            dest_impl->env().property().type == DeviceType::CPU,
            "cuda peer_copy_to only implemented for CPU");
    auto copy = [this, dest, src, size]() {
        auto stream = m_env.cuda_env().stream;
        m_env.cuda_env().activate();
        MGB_CUDA_CHECK(
                cudaMemcpyAsync(dest, src, size, cudaMemcpyDeviceToHost, stream));
        MGB_CUDA_CHECK(cudaStreamSynchronize(stream));
    };
    dest_impl->env().cpu_env().dispatch(copy);
}

MemNode CudaCompNodeImpl::mem_node() {
    // m_device_info would be null before async init finishes; so we just return
    // a prive pointer related to device number here
    return MemNode{sd->dev_info + m_locator.device};
}

void CudaCompNodeImpl::sync() {
    activate();

    // do not use MGB_CUDA_CHECK(cudaStreamSynchronize(m_env->stream)) since
    // other threads may be adding operations into the stream, and we only care
    // about previous operations in current thread. However docs of
    // cudaStreamSynchronize did not describe details of such condition, so we
    // use manual event implementation

    Event* event;
    {
        MGB_LOCK_GUARD(m_sync_event_mtx);
        if (!m_sync_event)
            m_sync_event = create_event(0);
        event = m_sync_event.get();
    }
    event->record();
    event->host_wait();
}

void CudaCompNodeImpl::enable_peer_access(int dev0, int dev1) {
    static bool already_enabled[MAX_NR_DEVICE][MAX_NR_DEVICE];
    if (already_enabled[dev0][dev1])
        return;

    static std::mutex global_lock;
    MGB_LOCK_GUARD(global_lock);
    if (already_enabled[dev0][dev1])
        return;

    int can;
    MGB_CUDA_CHECK(cudaDeviceCanAccessPeer(&can, dev0, dev1));
    if (can) {
        mgb_log("enable peer access from GPU %d to GPU %d", dev0, dev1);
        MGB_CUDA_CHECK(cudaSetDevice(dev0));
        auto err = cudaDeviceEnablePeerAccess(dev1, 0);
        if (err != cudaSuccess) {
            mgb_log_error(
                    "failed to enable peer access from %d to %d: %s(%d)", dev0, dev1,
                    cudaGetErrorString(err), static_cast<int>(err));
            cudaGetLastError();
        }
    }

    // check for cudaMemcpyPeer usable
    int v0 = 1, v1 = 2;

    int *dp0, *dp1;
    MGB_CUDA_CHECK(cudaSetDevice(dev0));
    MGB_CUDA_CHECK(cudaMalloc(&dp0, sizeof(int)));
    MGB_CUDA_CHECK(cudaSetDevice(dev1));
    MGB_CUDA_CHECK(cudaMalloc(&dp1, sizeof(int)));
    MGB_CUDA_CHECK(cudaMemcpy(dp0, &v0, sizeof(int), cudaMemcpyHostToDevice));
    MGB_CUDA_CHECK(cudaMemcpy(dp1, &v1, sizeof(int), cudaMemcpyHostToDevice));
    MGB_CUDA_CHECK(cudaMemcpyPeer(dp1, dev1, dp0, dev0, sizeof(int)));
    int get = 0;
    MGB_CUDA_CHECK(cudaMemcpy(&get, dp1, sizeof(int), cudaMemcpyDeviceToHost));

    mgb_throw_if(
            get != 1, CudaError,
            "P2P copy (%d => %d) check failed; consider disabling "
            "Access Control Services(ACS) for the PCI device",
            dev0, dev1);

    already_enabled[dev0][dev1] = true;
}

/* ===================== CudaCompNodeImpl::DeviceInfo  ===================== */

void CudaCompNodeImpl::DeviceInfo::init(const CompNodeEnv& env) {
    mgb_assert(!mem_alloc);
#if 0
    // forward cudaMalloc
    mem_alloc = mem_alloc::DevMemAlloc::make_cuda_alloc();
#else
    auto&& cuenv = env.cuda_env();
    cuenv.activate();
    dev_num = cuenv.device;
    auto reserve_size = StaticData::get_mem_reserve_size();
    mem_alloc = mem_alloc::DevMemAlloc::make(
            dev_num, reserve_size, std::make_shared<mem_alloc::CudaRawAllocator>(),
            std::make_shared<mem_alloc::CudaDeviceRuntimePolicy>());
    mem_alloc->prealloc_config(sd->prealloc_config);
    auto align = env.property().mem_alignment;
    mem_alloc->alignment(align);
    mgb_log_debug(
            "cuda: gpu%d: name=`%s' dyn_mem_reserve=%.2fMiB alignment=0x%zx", dev_num,
            cuenv.device_prop.name, reserve_size / 1024.0 / 1024, align);
#endif
}

bool CudaCompNodeImpl::check_global_finalized() {
    if (!sd) {
        static std::atomic_flag warn_printed = ATOMIC_FLAG_INIT;
        if (!warn_printed.test_and_set()) {
            mgb_log_debug("cuda comp node method called after global finalize");
        }
        return true;
    }
#if MGB_CUDA && defined(WIN32)
    //! FIXME: windows cuda driver shutdown before call atexit function even
    //! register atexit function after init cuda driver! as a workround
    //! recovery resource by OS temporarily, may need remove this after
    //! upgrade cuda runtime
    if (CudaCompNode::is_into_atexit) {
        mgb_log_debug(
                "windows cudaErrorCudartUnloading happened!!, resource "
                "recovery by OS!!");
        return true;
    }
    //! FIXME: megengine dynamic with VCRT, atexit fuctions table have
    //! some order issue, which will lead to cuda runtime uploading, this
    //! always happened at python3 unload dll(means python3 will exit),
    //! as a workround, recovery resource by OS temporarily, may need
    //! remove this after upgrade cuda runtime
    int dev = -1;
    if (cudaErrorCudartUnloading == cudaGetDevice(&dev)) {
        mgb_log_debug(
                "windows cudaErrorCudartUnloading happened!!, resource "
                "recovery by OS!!");
        return true;
    }

#endif
    return false;
}

/* ===================== EventImpl  ===================== */

class CudaCompNode::EventImpl final : public EventImplHelper {
    bool m_init_finished = false;
    CudaCompNodeImpl* const m_comp_node_impl;
    cudaEvent_t m_cuda_event;

    void do_record() override {
        m_comp_node_impl->activate();
        auto&& env = m_comp_node_impl->m_env.cuda_env();
        MGB_CUDA_CHECK(cudaEventRecord(m_cuda_event, env.stream));
    }

    bool do_finished() override {
        m_comp_node_impl->activate();
        cudaError_t err = cudaEventQuery(m_cuda_event);
        if (err == cudaSuccess)
            return true;
        if (err == cudaErrorNotReady)
            return false;
        mgb_throw(
                CudaError, "failed to query event: %d: %s", int(err),
                cudaGetErrorString(err));
    }

    void host_wait_cv() override { MGB_CUDA_CHECK(cudaEventSynchronize(m_cuda_event)); }

    double do_elapsed_time_until(EventImplHelper& end) override {
        m_comp_node_impl->activate();
        float ret = 0.0;
        MGB_CUDA_CHECK(cudaEventElapsedTime(
                &ret, m_cuda_event, static_cast<EventImpl&>(end).m_cuda_event));
        return static_cast<double>(ret) * 1e-3;
    }

    void do_device_wait_by(Impl* cn_impl) override;

public:
    EventImpl(CudaCompNodeImpl* comp_node_impl, size_t create_flags)
            : EventImplHelper(comp_node_impl, create_flags),
              m_comp_node_impl{comp_node_impl} {
        m_comp_node_impl->activate();
        size_t cuda_flags = cudaEventDisableTiming;
        if (create_flags & NEED_TIMER)
            cuda_flags = 0;
        MGB_CUDA_CHECK(cudaEventCreateWithFlags(&m_cuda_event, cuda_flags));
        m_init_finished = true;
    }

    ~EventImpl() {
        if (m_init_finished) {
            MGB_TRY { MGB_CUDA_CHECK(cudaEventDestroy(m_cuda_event)); }
            MGB_CATCH(MegBrainError & exc, {
                mgb_log_error("failed to destroy cuda event: %s", exc.what());
            })
        }
    }
};

std::unique_ptr<CompNode::Event> CudaCompNodeImpl::create_event(size_t flags) {
    return std::make_unique<EventImpl>(this, flags);
}

void CudaCompNode::EventImpl::do_device_wait_by(Impl* cn_impl) {
    if (cn_impl->dyn_typeinfo() == CudaCompNodeImpl::typeinfo()) {
        auto imp = static_cast<CudaCompNodeImpl*>(cn_impl);
        auto stream = imp->m_env.cuda_env().stream;
        imp->activate();
        MGB_CUDA_CHECK(cudaStreamWaitEvent(stream, m_cuda_event, 0));
        return;
    }
    if (cn_impl->env().property().type == DeviceType::CPU) {
        auto waiter = [this]() { MGB_CUDA_CHECK(cudaEventSynchronize(m_cuda_event)); };
        cn_impl->add_callback(std::move(waiter));
        return;
    }
    mgb_throw(MegBrainError, "unimplemented event device_wait_by config");
}

/* ===================== CudaCompNode static methods ===================== */

namespace {

#ifndef __unix__
template <typename Func, typename Val>
CUresult call_cuda_forksafe(Func func, Val* val, size_t len) {
    cuInit(0);
    return func();
}
#else
struct RAIICloseFD : NonCopyableObj {
    int m_fd = -1;

    RAIICloseFD(int fd) : m_fd(fd) {}
    ~RAIICloseFD() { close(); }
    void close() {
        if (m_fd != -1) {
            ::close(m_fd);
            m_fd = -1;
        }
    }
};
// an implementation that does not call cuInit
template <typename Func, typename Val>
CUresult call_cuda_forksafe(Func func, Val* val, size_t len) {
    int t_ndev;
    // use cuDeviceGetCount to detect cuda initialization to avoid abnormal behavior
    auto err = cuDeviceGetCount(&t_ndev);
    if (err != CUDA_ERROR_NOT_INITIALIZED)
        return func();
    // cuInit not called, call it in child process
    int fd[2];
    mgb_assert(pipe(fd) == 0, "pipe() failed");
    int fdr = fd[0], fdw = fd[1];
    RAIICloseFD fdr_guard(fdr);
    RAIICloseFD fdw_guard(fdw);
    auto cpid = fork();
    mgb_assert(cpid != -1, "fork() failed");
    if (cpid == 0) {
        fdr_guard.close();
        do {
            err = cuInit(0);
            if (err != CUDA_SUCCESS)
                break;
            err = func();
        } while (0);
        auto sz = write(fdw, &err, sizeof(err));
        if (sz == sizeof(err) && err == CUDA_SUCCESS) {
            sz = write(fdw, val, sizeof(*val) * len);
        }
        fdw_guard.close();
        std::quick_exit(0);
    }
    fdw_guard.close();
    auto sz = read(fdr, &err, sizeof(err));
    mgb_assert(sz == sizeof(err), "failed to read error code from child");
    if (err == CUDA_SUCCESS) {
        sz = read(fdr, val, sizeof(*val) * len);
        mgb_assert(
                static_cast<size_t>(sz) == sizeof(*val) * len,
                "failed to read value from child");
        return err;
    }
    // try again, maybe another thread called cuInit while we fork
    auto err2 = func();
    if (err2 == CUDA_SUCCESS)
        return err2;
    if (err2 == CUDA_ERROR_NOT_INITIALIZED)
        return err;
    return err2;
}
#endif

const char* cu_get_error_string(CUresult err) {
    const char* ret = nullptr;
    cuGetErrorString(err, &ret);
    if (!ret) {
        //! caused by cuda stub do not find driver
        ret = "invalid_stub_call";
    }
    return ret;
}

#define MGB_CALL_CUDA_FORKSAFE_NOASSERT(func, ptr, len, ...) \
    call_cuda_forksafe([&]() { return func(ptr, ##__VA_ARGS__); }, ptr, len)

#define MGB_CALL_CUDA_FORKSAFE(func, ptr, len, ...)                                \
    {                                                                              \
        auto err = MGB_CALL_CUDA_FORKSAFE_NOASSERT(func, ptr, len, ##__VA_ARGS__); \
        if (err != CUDA_SUCCESS) {                                                 \
            auto err_s = cu_get_error_string(err);                                 \
            mgb_log_error(#func " failed: %s (err %d)", err_s, int(err));          \
        }                                                                          \
    }
}  // namespace

bool CudaCompNode::available() {
    static int result = -1;
    static Spinlock mtx;
    MGB_LOCK_GUARD(mtx);
    if (result == -1) {
        int ndev = -1;
        auto err = MGB_CALL_CUDA_FORKSAFE_NOASSERT(cuDeviceGetCount, &ndev, 1);
        result = err == CUDA_SUCCESS && ndev > 0;
        auto err_s = cu_get_error_string(err);
        //! only show !CUDA_SUCCESS log when with valid stub call
        if (!result && (std::string(err_s) != "invalid_stub_call")) {
            mgb_log_warn(
                    "cuda unavailable: %s(%d) ndev=%d", err_s, static_cast<int>(err),
                    ndev);
        }
        if (err == CUDA_ERROR_NOT_INITIALIZED) {
            mgb_throw(std::runtime_error, "cuda initialization error.");
        }
    }
    return result;
}

void CudaCompNode::finalize() {
    if (CudaCompNodeImpl::sd) {
        sync_all();

        auto ptr = CudaCompNodeImpl::sd;
        CudaCompNodeImpl::sd = nullptr;
        ptr->~StaticData();
    }
}

#if MGB_CUDA && defined(WIN32)
//! FIXME: windows cuda driver shutdown before call atexit function even
//! register atexit function after init cuda driver! as a workround
//! recovery resource by OS temporarily, may need remove this after
//! upgrade cuda runtime
bool CudaCompNode::is_into_atexit = false;
#endif
CompNode::Impl* CudaCompNode::load_cuda(
        const Locator& locator, const Locator& locator_logical) {
    int nr_gpu = get_device_count();
#if MGB_CUDA && defined(WIN32)
    //! FIXME: windows cuda driver shutdown before call atexit function even
    //! register atexit function after init cuda driver! as a workround
    //! recovery resource by OS temporarily, may need remove this after
    //! upgrade cuda runtime
    if (!is_into_atexit) {
        auto err = atexit([] { is_into_atexit = true; });
        mgb_assert(!err, "failed to register atexit function");
    }
#endif
    mgb_assert(
            locator.device >= 0 && locator.device < nr_gpu,
            "request gpu%d out of valid range [0, %d)", locator.device, nr_gpu);

    auto&& sdptr = CudaCompNodeImpl::sd;
    {
        MGB_LOCK_GUARD(CudaCompNodeImpl::sd_mtx);
        if (!sdptr) {
            // use static storage so object can be safely accessed even after
            // global finalize
            using T = CudaCompNodeImpl::StaticData;
            static std::aligned_storage_t<sizeof(T), alignof(T)> storage;
            sdptr = new (&storage) T;
        }
    }
    auto&& sd = *sdptr;
    MGB_LOCK_GUARD(sd.mtx);

    CompNodeImpl* available_node = nullptr;
    for (int i = 0; i < sd.nr_node; ++i) {
        auto&& cur = sd.node[i];
        if (cur.m_initialized) {
            if (cur.m_locator == locator && cur.m_locator_logical == locator_logical) {
                return &cur;
            }
        } else {
            available_node = &cur;
        }
    }

    if (!available_node) {
        mgb_assert(
                sd.nr_node < CompNodeImpl::MAX_NR_COMP_NODE,
                "too many CompNode allocated");
        available_node = &sd.node[sd.nr_node++];
    }
    mgb_assert(locator.device < CompNodeImpl::MAX_NR_DEVICE, "device number too large");

    mgb_assert(!available_node->m_initialized);
    available_node->init(locator, locator_logical);

    return available_node;
}

void CudaCompNode::try_coalesce_all_free_memory() {
    // TODO: optimized implementation
    auto sd = CudaCompNodeImpl::sd;
    if (!sd)
        return;

    size_t size = 0;
    for (int i = 0; i < sd->nr_dev_used; ++i) {
        size += sd->dev_info[i].mem_alloc->gather_stream_free_blk_and_release_full();
    }
    if (size) {
        mgb_log_debug("%zu bytes freed by try_coalesce_all_free_memory()", size);
    }
}

void CudaCompNode::sync_all() {
    auto sd = CudaCompNodeImpl::sd;
    if (!sd)
        return;

    for (int i = 0;; ++i) {
        // ensure async init finished
        CompNodeEnv* env;
        {
            MGB_LOCK_GUARD(sd->mtx);
            if (i >= sd->nr_node) {
                break;
            }
            env = &sd->node[i].env();
        }
        env->cuda_env();
    }

    MGB_LOCK_GUARD(sd->mtx);
    for (int i = 0; i < sd->nr_dev_used; ++i) {
        MGB_CUDA_CHECK(cudaSetDevice(sd->dev_info[i].dev_num));
        MGB_CUDA_CHECK(cudaDeviceSynchronize());
    }
}

void CudaCompNode::foreach (thin_function<void(CompNode)> callback) {
    auto sd = CudaCompNodeImpl::sd;
    if (!sd)
        return;

    for (int i = 0;; ++i) {
        CompNode cur;
        {
            MGB_LOCK_GUARD(sd->mtx);
            if (i >= sd->nr_node)
                return;
            cur = make_comp_node_from_impl(&sd->node[i]);
        }
        callback(cur);
    }
}

size_t CudaCompNode::get_device_count(bool warn) {
    static int cnt = -1;
    static Spinlock mtx;
    MGB_LOCK_GUARD(mtx);
    if (cnt == -1) {
        auto err = MGB_CALL_CUDA_FORKSAFE_NOASSERT(cuDeviceGetCount, &cnt, 1);
        auto err_s = cu_get_error_string(err);
        if (err != CUDA_SUCCESS) {
            if (warn && (std::string(err_s) != "invalid_stub_call"))
                mgb_log_error("cuDeviceGetCount failed: %s (err %d)", err_s, int(err));
            cnt = 0;
        }
        mgb_assert(cnt >= 0);
    }
    return cnt;
}

void CudaCompNode::set_prealloc_config(
        size_t alignment, size_t min_req, size_t max_overhead, double growth_factor) {
    auto&& sdptr = CudaCompNodeImpl::sd;
    {
        MGB_LOCK_GUARD(CudaCompNodeImpl::sd_mtx);
        if (!sdptr) {
            using T = CudaCompNodeImpl::StaticData;
            static std::aligned_storage_t<sizeof(T), alignof(T)> storage;
            sdptr = new (&storage) T;
            sdptr->prealloc_config.alignment = alignment;
            sdptr->prealloc_config.min_req = min_req;
            sdptr->prealloc_config.growth_factor = growth_factor;
            sdptr->prealloc_config.max_overhead = max_overhead;
        } else {
            mgb_log_warn(
                    "invalid call to set_prealloc_config, will fallback to "
                    "default config; "
                    "prealloc_config should be specified before any CUDA "
                    "memory allocation");
        }
    }
}

CompNode::DeviceProperties CudaCompNode::get_device_prop(int dev) {
    int cnt = static_cast<int>(get_device_count());
    mgb_assert(
            dev >= 0 && dev < cnt, "request gpu %d out of valid range [0, %d)", dev,
            cnt);

    auto&& rec = device_prop_rec[dev];
    if (!rec.init) {
        MGB_LOCK_GUARD(rec.mtx_com);
        if (!rec.init) {
            MGB_CALL_CUDA_FORKSAFE(
                    cuDeviceGetAttribute, &rec.prop.major, 1,
                    CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MAJOR, dev);
            MGB_CALL_CUDA_FORKSAFE(
                    cuDeviceGetAttribute, &rec.prop.minor, 1,
                    CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MINOR, dev);
            MGB_CALL_CUDA_FORKSAFE(cuDeviceTotalMem, &rec.prop.total_memory, 1, dev);
            char pname[256] = {0};
            MGB_CALL_CUDA_FORKSAFE(cuDeviceGetName, pname, 255, 255, dev);
            rec.prop.name = pname;
            rec.init = true;
        }
    }

    return rec.prop;
}

#else

bool CudaCompNode::available() {
    return false;
}
void CudaCompNode::try_coalesce_all_free_memory() {}
void CudaCompNode::foreach (thin_function<void(CompNode)>) {}
void CudaCompNode::finalize() {}
size_t CudaCompNode::get_device_count(bool warn) {
    return 0;
}
CudaCompNode::Impl* CudaCompNode::load_cuda(const Locator&, const Locator&) {
    mgb_throw(MegBrainError, "cuda disabled at compile time");
}
void CudaCompNode::sync_all() {}

void CudaCompNode::set_prealloc_config(
        size_t alignment, size_t min_req, size_t max_overhead, double growth_factor) {}

CompNode::DeviceProperties CudaCompNode::get_device_prop(int dev) {
    return CompNode::DeviceProperties{};
}

#undef err

#endif  // MGB_CUDA

// vim: syntax=cpp.doxygen foldmethod=marker foldmarker=f{{{,f}}}
