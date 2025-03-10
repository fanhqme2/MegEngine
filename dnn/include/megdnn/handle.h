#pragma once

#include "megcore.h"
#include "megdnn/basic_types.h"
#include "megdnn/config/config.h"

#include <functional>
#include <memory>

#include "megdnn/internal/visibility_prologue.h"
namespace megdnn {

class OperatorBase;

class Handle {
public:
    enum class HandleType {
        NAIVE = 0,
        FALLBACK = 1,
        X86 = 2,
        ARM_COMMON = 3,
        ARMV7 = 4,
        AARCH64 = 5,
        CUDA = 6,
        ROCM = 11,
        ATLAS = 13,
        CAMBRICON = 12,
    };

    //! Device vendor
    enum class HandleVendorType : uint32_t {
        NOT_SPEC = 0,
        MALI = 1,
        ADRENO = 2,
        CUDA = 3,
        INTEL = 4,
        POWERVR = 5,
        AMD = 6,
    };

protected:
    Handle(megcoreComputingHandle_t computing_handle, HandleType type);

public:
    /**
     * \brief Create a MegDNN handle from a MegCore Computing handle.
     *
     * \param[in] computing_handle MegCore computing handle. Please note
     *      that computing_handle would not be released when this Handle is
     *      destructed
     * \param[in] debug_level
     *   Applicable for CPU computing handle.
     *    0 means taking the fastest possible code path; it may contains
     *      platform-specific instructions such as SSE for x86_64 or NEON for
     *      armv7v7.
     *    1 means taking the fastest possible code path without
     *      platform-specific instructions in C++ code. Note that the compiled
     *      binary file still contains platform-specific codes.
     *    2 means taking the naive code path. Performance is severely
     *      hampered, but it is less error-prone since the internal
     *      implementation is rather straightforward.
     *
     *    **Debug level 1 and 2 should not be used in productions.**
     */
    MGE_WIN_DECLSPEC_FUC static std::unique_ptr<Handle> make(
            megcoreComputingHandle_t computing_handle, int debug_level = 0);

#if MEGDNN_WITH_CUDA
    MGE_WIN_DECLSPEC_FUC static std::unique_ptr<Handle> make_cuda_handle(
            megcoreComputingHandle_t computing_handle);
    template <typename opr>
    MGE_WIN_DECLSPEC_FUC std::unique_ptr<opr> create_cuda_operator();
#endif
#if MEGDNN_WITH_ROCM
    MGE_WIN_DECLSPEC_FUC static std::unique_ptr<Handle> make_rocm_handle(
            megcoreComputingHandle_t computing_handle);
    template <typename opr>
    MGE_WIN_DECLSPEC_FUC std::unique_ptr<opr> create_rocm_operator();
#endif

    virtual ~Handle();

    /*!
     * \brief Get the underlying megcore computing handle.
     */
    megcoreComputingHandle_t megcore_computing_handle() const {
        return m_computing_handle;
    }

    /*!
     * \brief set a callback function to be invoked when this handle is
     *      destructed, so associated resources can be released (e.g.
     *      computing handle)
     *
     * This function can be called at most once.
     */
    MGE_WIN_DECLSPEC_FUC void set_destructor(const thin_function<void()>& d);

    /*!
     * \brief set a callback to be invoked when an operator is destructed
     * \param[in,out] cb the callback function; it would be set to the
     *      previous callback function
     */
    void set_opr_destruct_callback(thin_function<void(OperatorBase*)>& cb) {
        cb.swap(m_on_opr_destructed);
    }

    MGE_WIN_DECLSPEC_FUC void on_opr_destructed(OperatorBase* opr);

    /**
     * \brief Create operator of Opr type.
     */
    template <typename Opr>
    MGE_WIN_DECLSPEC_FUC std::unique_ptr<Opr> create_operator();

    /*
     * =============================================================
     * Users should call functions below to query memory requirement.
     * =============================================================
     */

    /**
     * \brief The internal data pointer of TensorND should be aligned to
     *        alignment_requirement() in bytes.
     */
    MGE_WIN_DECLSPEC_FUC virtual size_t alignment_requirement() const;

    //! get alignment in bytes for rows of image 2D tensor format
    MGE_WIN_DECLSPEC_FUC virtual size_t image2d_pitch_alignment() const;

    //! get vendor type
    MGE_WIN_DECLSPEC_FUC virtual HandleVendorType vendor_type() const;

    HandleType type() const { return m_handle_type; }

    /**
     * \brief Check is the layout satisfy cross device copy constraint.
     *        1. The handle of the src and the dst is the same kind
     *        2. The dst is continguous.
     */
    MGE_WIN_DECLSPEC_FUC virtual bool check_cross_dev_copy_constraint(
            const TensorLayout& src);

private:
    static constexpr uint32_t ALIVE_MAGIC = 0x8595e9d2u;
    volatile uint32_t m_alive_magic = ALIVE_MAGIC;
    megcoreComputingHandle_t m_computing_handle;
    const HandleType m_handle_type;
    thin_function<void()> m_destructor;
    thin_function<void(OperatorBase*)> m_on_opr_destructed;

    Handle() = delete;
    Handle(const Handle& rhs) = delete;
    Handle& operator=(const Handle& rhs) = delete;
};

}  // namespace megdnn

#include "megdnn/internal/visibility_epilogue.h"

// vim: syntax=cpp.doxygen
