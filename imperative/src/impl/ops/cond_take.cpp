#include "../dnn_op_helper.h"
#include "../op_trait.h"
#include "megbrain/imperative/ops/autogen.h"
#include "megbrain/opr/misc.h"

using namespace megdnn;

namespace mgb::imperative {

namespace {

cg::OperatorNodeBase* apply_on_var_node(const OpDef& def, const VarNodeArray& inputs) {
    auto&& op = def.cast_final_safe<CondTake>();
    auto&& graph = inputs[0]->owner_graph();

    opr::CondTake::Param param;
    param.val = 1;
    OperatorNodeConfig config{op.make_name()};
    cg::OperatorNodeBase* opr = graph->insert_opr(
            std::make_unique<opr::CondTake>(inputs[0], inputs[1], param, config));
    return opr;
}

SmallVector<TensorPtr> apply_on_physical_tensor(
        const OpDef& def, const SmallVector<TensorPtr>& inputs,
        SmallVector<LogicalTensorDesc>& output_descs, const bool& validated) {
    mgb_assert(inputs.size() == 2, "CondTake take 2 inputs, got %lu", inputs.size());

    auto&& inp = inputs[0];
    auto&& msk = inputs[1];
    SmallVector<TensorPtr> out;
    mgb_assert(
            inp->layout().eq_shape(msk->layout()),
            "input shape does not match mask shape");
    mgb_assert(
            msk->get_value().dtype().enumv() == DTypeEnum::Bool,
            "mask dtype must be bool");
    MegDNNDynOutMallocImpl<2> policy{inp->comp_node()};
    if (inp->layout().is_empty()) {
        // empty tensor
        policy.alloc_output(0, inp->layout().dtype, {0}, nullptr);
        policy.alloc_output(1, dtype::Int32(), {0}, nullptr);
    } else {
        DnnOprCaller<megdnn::CondTake> dnn_op(inp->comp_node());
        dnn_op.op->param().val = 1;

        TensorLayout m_layout(
                {dnn_op.op->get_workspace_in_bytes(inp->layout())}, dtype::Byte());

        auto dnn_workspace = dnn_op.create_workspace(m_layout);

        dnn_op.op->exec(
                inp->dev_tensor().as_megdnn(), msk->dev_tensor().as_megdnn(),
                dnn_workspace, &policy);
    }
    out.push_back(policy.at(0));
    out.push_back(policy.at(1));
    return out;
}

std::tuple<SmallVector<LogicalTensorDesc>, bool> infer_output_attrs_fallible(
        const OpDef& def, const SmallVector<LogicalTensorDesc>& inputs) {
    auto cn = inputs[0].comp_node;
    return {{{TensorLayout(inputs[0].layout.dtype), cn},
             {TensorLayout(dtype::Int32()), cn}},
            false};
}

OP_TRAIT_REG(CondTake, CondTake, opr::CondTake)
        .apply_on_var_node(apply_on_var_node)
        .apply_on_physical_tensor(apply_on_physical_tensor)
        .infer_output_attrs_fallible(infer_output_attrs_fallible)
        .fallback();

}  // namespace

}  // namespace mgb::imperative
