#pragma once
#include "megdnn/oprs.h"

namespace megdnn {
namespace cuda {

class ParamPackConcatImpl final : public ParamPackConcat {
public:
    using ParamPackConcat::ParamPackConcat;
    void exec(
            _megdnn_tensor_in srcs, _megdnn_tensor_in table, _megdnn_tensor_out dst,
            _megdnn_workspace workspace) override;

    size_t get_workspace_in_bytes(
            const TensorShapeArray& srcs, const TensorShape& table,
            const TensorShape& dst) override;

private:
    template <typename T>
    void exec_internal(
            _megdnn_tensor_in srcs, _megdnn_tensor_in table, _megdnn_tensor_out dst,
            _megdnn_workspace workspace);
};

}  // namespace cuda
}  // namespace megdnn
