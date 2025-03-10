#include "megbrain_build_config.h"

#if MGB_ENABLE_OPR_MM
#include "../op_trait.h"
#include "megbrain/imperative/proxy_graph_detail.h"
#include "megbrain/opr/io_remote.h"
#include "megbrain/opr/mm_handler.h"
#endif  // MGB_ENABLE_OPR_MM

#include "megbrain/imperative/ops/autogen.h"

namespace mgb {
namespace imperative {

#if MGB_ENABLE_OPR_MM
namespace {
cg::OperatorNodeBase* apply_on_var_node_remote_send(
        const OpDef& def, const VarNodeArray& inputs) {
    auto&& send = def.cast_final_safe<RemoteSend>();
    auto group_client = std::make_shared<opr::GroupClientProxy>(
            ssprintf("%s:%d", send.addr.data(), send.port));
    auto&& graph = inputs[0]->owner_graph();

    OperatorNodeConfig config{send.make_name()};
    cg::OperatorNodeBase* opr =
            graph->insert_opr(std::make_unique<mgb::opr::RemoteSend>(
                    send.key, inputs[0], group_client, true, send.backend, config));
    return opr;
}

cg::OperatorNodeBase* apply_on_var_node_remote_recv(
        const OpDef& def, const VarNodeArray& inputs) {
    auto&& recv = def.cast_final_safe<RemoteRecv>();
    OperatorNodeConfig config{recv.cn};
    config.name(recv.make_name());
    auto group_client = std::make_shared<opr::GroupClientProxy>(
            ssprintf("%s:%d", recv.addr.data(), recv.port));
    auto&& graph = inputs[0]->owner_graph();
    mgb_assert(!recv.shape.empty());
    TensorShape shape;
    for (auto&& dim : recv.shape) {
        shape[shape.ndim++] = dim;
    }
    return graph->insert_opr(std::make_unique<mgb::opr::RemoteRecv>(
            recv.key, inputs[0], *graph, group_client, config, shape, recv.dtype,
            recv.backend));
}

OP_TRAIT_REG(RemoteSend, RemoteSend, mgb::opr::RemoteSend)
        .apply_on_var_node(apply_on_var_node_remote_send)
        .fallback();

OP_TRAIT_REG(RemoteRecv, RemoteRecv, mgb::opr::RemoteRecv)
        .apply_on_var_node(apply_on_var_node_remote_recv)
        .fallback();
}  // anonymous namespace
#endif  // MGB_ENABLE_OPR_MM

}  // namespace imperative
}  // namespace mgb
