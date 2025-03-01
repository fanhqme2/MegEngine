#pragma once
#include <cuda_runtime_api.h>
#include "megcore_cdefs.h"
#include "src/common/cv/enums.h"
#include "src/cuda/utils.cuh"

namespace megdnn {
namespace cuda {
namespace warp_perspective {

// all these kernels use bilinear interpolation

template <typename ctype>
void forward_proxy(
        bool is_nhwc, const ctype* src, const float* mat, const int* mat_idx,
        ctype* dst, int N_SRC, int N_MAT, int C, int IH, int IW, int OH, int OW,
        ctype bval, BorderMode bmode, megcore::AsyncErrorInfo* error_info,
        void* error_tracker, cudaStream_t stream);

template <typename ctype, int pack_c>
void forward_proxy_nhwc_bit4(
        const ctype* src, const float* mat, const int* mat_idx, ctype* dst, int N_SRC,
        int N_MAT, int C, int IH, int IW, int OH, int OW, ctype bval, BorderMode bmode,
        megcore::AsyncErrorInfo* error_info, void* error_tracker, cudaStream_t stream);

template <typename ctype>
void forward_proxy_nchw4(
        const ctype* src, const float* mat, const int* mat_idx, ctype* dst, int N_SRC,
        int N_MAT, int C, int IH, int IW, int OH, int OW, ctype bval, BorderMode bmode,
        megcore::AsyncErrorInfo* error_info, void* error_tracker, cudaStream_t stream);

template <typename ctype>
void forward_proxy_nchw64(
        const ctype* src, const float* mat, const int* mat_idx, ctype* dst, int N_SRC,
        int N_MAT, int C, int IH, int IW, int OH, int OW, ctype bval, BorderMode bmode,
        megcore::AsyncErrorInfo* error_info, void* error_tracker, cudaStream_t stream);

template <typename src_dtype, typename src_ctype, typename dst_ctype>
void forward_proxy_quint8_dimshuffle_typecvt_nchw4(
        bool is_nhwc, const src_ctype* src, const float* mat, const int* mat_idx,
        dst_ctype* dst, int N_SRC, int N_MAT, int C, int IH, int IW, int OH, int OW,
        src_ctype bval, DTypeParamImpl<src_dtype> param, BorderMode bmode,
        megcore::AsyncErrorInfo* error_info, void* error_tracker, cudaStream_t stream);

template <typename src_dtype, typename src_ctype, typename dst_ctype>
void forward_proxy_quint8_dimshuffle_typecvt_nchw(
        bool is_nhwc, const src_ctype* src, const float* mat, const int* mat_idx,
        dst_ctype* dst, int N_SRC, int N_MAT, int C, int IH, int IW, int OH, int OW,
        src_ctype bval, DTypeParamImpl<src_dtype> param, BorderMode bmode,
        megcore::AsyncErrorInfo* error_info, void* error_tracker, cudaStream_t stream);

void backward_data_proxy(
        const float* mat, const int* midx, const float* diff, float* grad,
        float* workspace, int N, int N_SRC, int C, int IH, int IW, int OH, int OW,
        float bval, BorderMode bmode, cudaStream_t stream);
size_t get_backward_data_workspace_in_bytes(
        int N, int C, int IH, int IW, int OH, int OW, BorderMode bmode);

void backward_mat_proxy(
        const float* src, const float* mat, const int* midx, const float* diff,
        float* grad, int N, int C, int IH, int IW, int OH, int OW, float bval,
        BorderMode bmode, cudaStream_t stream);

}  // namespace warp_perspective
}  // namespace cuda
}  // namespace megdnn

// vim: syntax=cpp.doxygen
