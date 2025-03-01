#include "src/naive/warp_perspective/opr_impl.h"
#include "src/naive/warp_perspective/warp_perspective_cv.h"

#include <cstring>
#include <type_traits>
#include "midout.h"
#include "src/common/cv/helper.h"
#include "src/common/utils.h"
#include "src/common/warp_common.h"
#include "src/naive/handle.h"

MIDOUT_DECL(megdnn_naive_warpperspective)

using namespace megdnn;
using namespace naive;

template <typename ctype, typename mtype>
void WarpPerspectiveForwardImpl::kern_naive(
        const KernParam<ctype, mtype>& kern_param, size_t task_id) {
    MEGDNN_MARK_USED_VAR(kern_param);
    MIDOUT_BEGIN(megdnn_naive_warpperspective, ctype, mtype, midout_iv(0)) {
        UNPACK_WARP_PERSPECTIVE_FWD_KERN_PARAM(kern_param);
        MEGDNN_MARK_USED_VAR(N_MAT);
        //! strides of C, H, W on src and dst
        size_t sstrd[3], dstrd[3];
        auto set_sstrd = [&](size_t s0, size_t s1, size_t s2) {
            sstrd[0] = s0;
            sstrd[1] = s1;
            sstrd[2] = s2;
        };
        auto set_dstrd = [&](size_t s0, size_t s1, size_t s2) {
            dstrd[0] = s0;
            dstrd[1] = s1;
            dstrd[2] = s2;
        };
        switch (kern_param.format) {
            case Format::NCHW:
            case Format::NCHW4:
                set_sstrd(IH * IW, IW, 1);
                set_dstrd(OH * OW, OW, 1);
                break;
            case Format::NHWC:
                set_sstrd(1, IW * C, C);
                set_dstrd(1, OW * C, C);
                break;
            default:
                megdnn_throw("bad format");
        }

        bool is_nchw4 = kern_param.format == Format::NCHW4;
        auto visit_src = [&sptr, sstrd, is_nchw4](size_t c, int h, int w) -> float {
            if (!is_nchw4)
                return sptr[sstrd[0] * c + sstrd[1] * h + sstrd[2] * w];
            else
                return sptr
                        [((sstrd[0] * (c >> 2) + sstrd[1] * h + sstrd[2] * w) << 2) +
                         (c & 0b11)];
        };
        auto visit_src_bd = [&sptr, sstrd, border_val, is_nchw4](
                                    size_t c, int h, int w) -> float {
            if (h != -1 && w != -1) {
                if (!is_nchw4) {
                    return sptr[sstrd[0] * c + sstrd[1] * h + sstrd[2] * w];
                } else {
                    return sptr
                            [((sstrd[0] * (c >> 2) + sstrd[1] * h + sstrd[2] * w)
                              << 2) +
                             (c & 0b11)];
                }
            } else
                return border_val;
        };
        auto visit_dst = [&dptr, dstrd, is_nchw4](size_t c, int h, int w) -> ctype& {
            if (!is_nchw4)
                return dptr[dstrd[0] * c + dstrd[1] * h + dstrd[2] * w];
            else
                return dptr
                        [((dstrd[0] * (c >> 2) + dstrd[1] * h + dstrd[2] * w) << 2) +
                         (c & 0b11)];
        };

        rounding::RoundingConverter<ctype> output_converter;
        auto orig_sptr = sptr;
        size_t n = task_id / OH;
        size_t oh = task_id % OH;
        mptr = mptr + n * 3 * 3;
        dptr = dptr + n * C * OH * OW;
        if (midx_ptr) {
            size_t idx = midx_ptr[n];
            megdnn_assert(
                    idx < N_SRC, "mat_idx out of bound: mat_idx[%zu]=%zu src_batch=%zu",
                    n, idx, N_SRC);
            sptr = orig_sptr + idx * (C * IH * IW);
        } else if (n) {
            sptr += n * C * IH * IW;
        }
        rep(ow, OW) {
            float numeratorw = mptr[0] * ow + mptr[1] * oh + mptr[2];
            float numeratorh = mptr[3] * ow + mptr[4] * oh + mptr[5];
            float denominator = mptr[6] * ow + mptr[7] * oh + mptr[8];
            float alphaw = numeratorw / denominator;
            float alphah = numeratorh / denominator;

            int iw0 = get_real_coord(std::floor(alphaw) + 0, IW);
            int iw1 = get_real_coord(std::floor(alphaw) + 1, IW);
            int ih0 = get_real_coord(std::floor(alphah) + 0, IH);
            int ih1 = get_real_coord(std::floor(alphah) + 1, IH);

            alphaw -= floor(alphaw);
            alphah -= floor(alphah);
            if (bmode != BorderMode::CONSTANT) {
                rep(c, C) {
                    visit_dst(c, oh, ow) = output_converter(
                            visit_src(c, ih0, iw0) * (1.0f - alphaw) * (1.0f - alphah) +
                            visit_src(c, ih0, iw1) * alphaw * (1.0f - alphah) +
                            visit_src(c, ih1, iw0) * (1.0f - alphaw) * alphah +
                            visit_src(c, ih1, iw1) * alphaw * alphah);
                }
            } else {
                rep(c, C) {
                    auto val = visit_src_bd(c, ih0, iw0) * (1.0f - alphaw) *
                                       (1.0f - alphah) +
                               visit_src_bd(c, ih0, iw1) * alphaw * (1.0f - alphah) +
                               visit_src_bd(c, ih1, iw0) * (1.0f - alphaw) * alphah +
                               visit_src_bd(c, ih1, iw1) * alphaw * alphah;
                    visit_dst(c, oh, ow) =
                            output_converter(std::isfinite(val) ? val : border_val);
                }
            }
        }
    }
    MIDOUT_END();
}

#define INST(ctype, mtype)                                              \
    template void WarpPerspectiveForwardImpl::kern_naive<ctype, mtype>( \
            const KernParam<ctype, mtype>&, size_t);

INST(float, float);

#if !MEGDNN_DISABLE_FLOAT16
INST(dt_float16, float);
INST(dt_float16, dt_float16);
INST(dt_bfloat16, float);
INST(dt_bfloat16, dt_bfloat16);
#endif

INST(int8_t, float);
INST(uint8_t, float);

#undef INST

template <typename ctype, typename mtype>
void WarpPerspectiveForwardImpl::kern_naive_nhwcd4(
        const KernParam<ctype, mtype>& kern_param, size_t task_id) {
    MIDOUT_BEGIN(megdnn_naive_warpperspective, ctype, mtype, midout_iv(1)) {
        auto get_index = [](size_t h, size_t w, size_t c, size_t W,
                            size_t C) -> size_t {
            size_t idx = h * (C / 4) * W * 4 + (c / 4) * W * 4 + w * 4 + (c % 4);
            return idx;
        };
        rounding::RoundingConverter<ctype> output_converter;
        UNPACK_WARP_PERSPECTIVE_FWD_KERN_PARAM(kern_param);
        MEGDNN_MARK_USED_VAR(N_MAT);
        size_t n = task_id / OH;
        size_t oh = task_id % OH;
        auto orig_sptr = sptr;
        mptr = mptr + n * 3 * 3;
        dptr = dptr + n * OH * (C / 4) * OW * 4;
        if (midx_ptr) {
            size_t idx = midx_ptr[n];
            megdnn_assert(
                    idx < N_SRC, "mat_idx out of bound: mat_idx[%zu]=%zu src_batch=%zu",
                    n, idx, N_SRC);
            sptr = orig_sptr + idx * IH * (C / 4) * IW * 4;
        } else if (n) {
            sptr += n * IH * (C / 4) * IW * 4;
        }
        rep(ow, OW) {
            float numeratorw = mptr[0] * ow + mptr[1] * oh + mptr[2];
            float numeratorh = mptr[3] * ow + mptr[4] * oh + mptr[5];
            float denominator = mptr[6] * ow + mptr[7] * oh + mptr[8];
            float alphaw = numeratorw / denominator;
            float alphah = numeratorh / denominator;

            int iw0 = get_real_coord(std::floor(alphaw) + 0, IW);
            int iw1 = get_real_coord(std::floor(alphaw) + 1, IW);
            int ih0 = get_real_coord(std::floor(alphah) + 0, IH);
            int ih1 = get_real_coord(std::floor(alphah) + 1, IH);

            alphaw -= floor(alphaw);
            alphah -= floor(alphah);
            if (bmode != BorderMode::CONSTANT) {
                rep(c, C) {
                    dptr[get_index(oh, ow, c, OW, C)] = output_converter(
                            sptr[get_index(ih0, iw0, c, IW, C)] * (1.0f - alphaw) *
                                    (1.0f - alphah) +
                            sptr[get_index(ih0, iw1, c, IW, C)] * alphaw *
                                    (1.0f - alphah) +
                            sptr[get_index(ih1, iw0, c, IW, C)] * (1.0f - alphaw) *
                                    alphah +
                            sptr[get_index(ih1, iw1, c, IW, C)] * alphaw * alphah);
                }
            } else {
                rep(c, C) {
                    const float b = border_val;
                    auto val = (ih0 != -1 && iw0 != -1
                                        ? sptr[get_index(ih0, iw0, c, IW, C)]
                                        : b) *
                                       (1.0f - alphaw) * (1.0f - alphah) +
                               (ih0 != -1 && iw1 != -1
                                        ? sptr[get_index(ih0, iw1, c, IW, C)]
                                        : b) *
                                       alphaw * (1.0f - alphah) +
                               (ih1 != -1 && iw0 != -1
                                        ? sptr[get_index(ih1, iw0, c, IW, C)]
                                        : b) *
                                       (1.0f - alphaw) * alphah +
                               (ih1 != -1 && iw1 != -1
                                        ? sptr[get_index(ih1, iw1, c, IW, C)]
                                        : b) *
                                       alphaw * alphah;
                    dptr[get_index(oh, ow, c, OW, C)] =
                            output_converter(std::isfinite(val) ? val : b);
                }
            }
        }
    }
    MIDOUT_END();
}

template <typename ctype, typename mtype>
void WarpPerspectiveForwardImpl::kern_naive_int4(
        const KernParam<ctype, mtype>& kern_param, size_t task_id) {
    MEGDNN_MARK_USED_VAR(kern_param);
    MIDOUT_BEGIN(megdnn_naive_warpperspective, ctype, mtype, midout_iv(0)) {
        UNPACK_WARP_PERSPECTIVE_FWD_KERN_PARAM(kern_param);
        MEGDNN_MARK_USED_VAR(N_MAT);
        uint32_t c_shift, c_mask, iw_shift = 0, ow_shift = 0;
        constexpr bool signedness = std::is_same<ctype, dt_qint4>::value;
        switch (param().format) {
            case Format::NCHW:
                c_shift = 0;
                c_mask = 0;
                iw_shift = IW % 2;
                ow_shift = OW % 2;
                break;
            case Format::NCHW64:
                c_shift = 6;
                c_mask = 0x3F;
                break;
            case Format::NHWC:
                megdnn_assert(C % 2 == 0);
                c_shift = 0;
                c_mask = 0;
                break;
            default:
                megdnn_throw("bad format");
                break;
        }
        //! strides of C, H, W on src and dst
        std::vector<size_t> sstrd = {
                IH * ((IW + iw_shift) << c_shift), (IW + iw_shift) << c_shift, 1};
        std::vector<size_t> dstrd = {
                OH * ((OW + ow_shift) << c_shift), (OW + ow_shift) << c_shift, 1};
        if (param().format == Format::NHWC) {
            sstrd = {1, IW * C, C};
            dstrd = {1, OW * C, C};
        }
        static constexpr uint8_t mask = (uint8_t)((1 << 4) - 1);
        auto visit_src = [&sptr, sstrd, c_shift, c_mask](
                                 size_t c, int h, int w) -> float {
            size_t index = (c >> c_shift) * sstrd[0] + h * sstrd[1] +
                           (w << c_shift) * sstrd[2] + (c & c_mask);
            uint8_t result = (sptr[index / 2].as_storage() >> (4 * (index % 2))) & 0xF;
            if (signedness) {
                return result & uint8_t(1 << 3) ? result | ~mask : result;
            } else {
                megdnn_assert((std::is_same<ctype, dt_quint4>::value));
                return result;
            }
        };
        auto visit_src_bd = [&sptr, sstrd, border_val, c_shift, c_mask](
                                    size_t c, int h, int w) -> float {
            if (h != -1 && w != -1) {
                size_t index = (c >> c_shift) * sstrd[0] + h * sstrd[1] +
                               (w << c_shift) * sstrd[2] + (c & c_mask);
                uint8_t result =
                        (sptr[index / 2].as_storage() >> (4 * (index % 2))) & 0xF;
                if (signedness) {
                    return result & uint8_t(1 << 3) ? result | ~mask : result;
                } else {
                    megdnn_assert((std::is_same<ctype, dt_quint4>::value));
                    return result;
                    ;
                }
            } else
                return border_val;
        };
        auto set_visit_dst = [&dptr, dstrd, c_shift, c_mask](
                                     size_t c, int h, int w, ctype v) {
            size_t index = (c >> c_shift) * dstrd[0] + h * dstrd[1] +
                           (w << c_shift) * dstrd[2] + (c & c_mask);
            dptr[index / 2] =
                    (dptr[index / 2].as_storage() & (0xF0 >> (4 * (index % 2)))) |
                    (v.as_storage() << (4 * (index % 2)));
        };

        rounding::RoundingConverter<ctype> output_converter;
        auto orig_sptr = sptr;
        size_t n = task_id / OH;
        size_t oh = task_id % OH;
        mptr = mptr + n * 3 * 3;
        dptr = dptr + n * C * OH * OW / 2;
        if (midx_ptr) {
            size_t idx = midx_ptr[n];
            megdnn_assert(
                    idx < N_SRC, "mat_idx out of bound: mat_idx[%zu]=%zu src_batch=%zu",
                    n, idx, N_SRC);
            sptr = orig_sptr + idx * (C * IH * IW) / 2;
        } else if (n) {
            sptr += n * C * IH * IW / 2;
        }
        rep(ow, OW) {
            float numeratorw = mptr[0] * ow + mptr[1] * oh + mptr[2];
            float numeratorh = mptr[3] * ow + mptr[4] * oh + mptr[5];
            float denominator = mptr[6] * ow + mptr[7] * oh + mptr[8];
            float alphaw = numeratorw / denominator;
            float alphah = numeratorh / denominator;

            int iw0 = get_real_coord(std::floor(alphaw) + 0, IW);
            int iw1 = get_real_coord(std::floor(alphaw) + 1, IW);
            int ih0 = get_real_coord(std::floor(alphah) + 0, IH);
            int ih1 = get_real_coord(std::floor(alphah) + 1, IH);
            alphaw -= floor(alphaw);
            alphah -= floor(alphah);
            if (bmode != BorderMode::CONSTANT) {
                rep(c, C) {
                    auto val =
                            visit_src(c, ih0, iw0) * (1.0f - alphaw) * (1.0f - alphah) +
                            visit_src(c, ih0, iw1) * alphaw * (1.0f - alphah) +
                            visit_src(c, ih1, iw0) * (1.0f - alphaw) * alphah +
                            visit_src(c, ih1, iw1) * alphaw * alphah;
                    set_visit_dst(c, oh, ow, output_converter(val));
                }
            } else {
                rep(c, C) {
                    auto val = visit_src_bd(c, ih0, iw0) * (1.0f - alphaw) *
                                       (1.0f - alphah) +
                               visit_src_bd(c, ih0, iw1) * alphaw * (1.0f - alphah) +
                               visit_src_bd(c, ih1, iw0) * (1.0f - alphaw) * alphah +
                               visit_src_bd(c, ih1, iw1) * alphaw * alphah;
                    set_visit_dst(
                            c, oh, ow,
                            output_converter(std::isfinite(val) ? val : border_val));
                }
            }
        }
    }
    MIDOUT_END();
}

template <typename ctype, typename dst_ctype, typename mtype>
void WarpPerspectiveForwardImpl::kern_naive_dimshuffle_typecvt(
        const KernParam<ctype, mtype>& kern_param, size_t task_id) {
    MEGDNN_MARK_USED_VAR(kern_param);
    MIDOUT_BEGIN(megdnn_naive_warpperspective, ctype, mtype, midout_iv(2)) {
        UNPACK_WARP_PERSPECTIVE_FWD_KERN_PARAM(kern_param);
        MEGDNN_MARK_USED_VAR(N_MAT);
        //! strides of C, H, W on src and dst
        size_t sstrd[3], dstrd[3];
        auto set_sstrd = [&](size_t s0, size_t s1, size_t s2) {
            sstrd[0] = s0;
            sstrd[1] = s1;
            sstrd[2] = s2;
        };
        auto set_dstrd = [&](size_t s0, size_t s1, size_t s2) {
            dstrd[0] = s0;
            dstrd[1] = s1;
            dstrd[2] = s2;
        };
        switch (kern_param.format) {
            case Format::NCHW:
            case Format::NCHW_NCHW4_IC_SMALL:
                set_sstrd(IH * IW, IW, 1);
                set_dstrd(OH * OW, OW, 1);
                break;
            case Format::NHWC_NCHW:
            case Format::NHWC_NCHW4_IC_SMALL:
                set_sstrd(1, IW * C, C);
                set_dstrd(OH * OW, OW, 1);
                break;
            default:
                megdnn_throw("bad format");
        }

        uint8_t zero_point = 0;
        float scale = 1.f;

        bool is_dst_float = kern_param.dst_dtype.enumv() == DTypeEnum::Float32;
        if (kern_param.src_dtype.enumv() == DTypeTrait<dtype::Quantized8Asymm>::enumv) {
            auto dtype_param =
                    kern_param.src_dtype.template param<dtype::Quantized8Asymm>();
            zero_point = dtype_param.zero_point;
            scale = dtype_param.scale;
        } else if (kern_param.src_dtype.enumv() == DTypeEnum::Uint8) {
            zero_point =
                    (kern_param.dst_dtype.enumv() == DTypeEnum::QuantizedS8) ? 128 : 0;
            scale = 1.f;
        }

        dst_ctype* dst_ptr = reinterpret_cast<dst_ctype*>(dptr);

        bool is_dst_nchw4 = (kern_param.format == Format::NCHW_NCHW4_IC_SMALL) ||
                            (kern_param.format == Format::NHWC_NCHW4_IC_SMALL);
        auto visit_src = [&sptr, sstrd](size_t c, int h, int w) -> float {
            return sptr[sstrd[0] * c + sstrd[1] * h + sstrd[2] * w];
        };
        auto visit_src_bd = [&sptr, sstrd, border_val](
                                    size_t c, int h, int w) -> float {
            if (h != -1 && w != -1) {
                return sptr[sstrd[0] * c + sstrd[1] * h + sstrd[2] * w];
            } else
                return border_val;
        };
        auto visit_dst = [&dst_ptr, dstrd, is_dst_nchw4](
                                 size_t c, int h, int w) -> dst_ctype& {
            if (!is_dst_nchw4)
                return dst_ptr[dstrd[0] * c + dstrd[1] * h + dstrd[2] * w];
            else
                return dst_ptr
                        [((dstrd[0] * (c >> 2) + dstrd[1] * h + dstrd[2] * w) << 2) +
                         (c & 0b11)];
        };

        rounding::RoundingConverter<dst_ctype> output_converter;
        auto orig_sptr = sptr;
        size_t n = task_id / OH;
        size_t oh = task_id % OH;
        mptr = mptr + n * 3 * 3;
        dst_ptr = is_dst_nchw4 ? (dst_ptr + n * OH * OW * 4)
                               : (dst_ptr + n * C * OH * OW);
        if (midx_ptr) {
            size_t idx = midx_ptr[n];
            megdnn_assert(
                    idx < N_SRC, "mat_idx out of bound: mat_idx[%zu]=%zu src_batch=%zu",
                    n, idx, N_SRC);
            sptr = orig_sptr + idx * (C * IH * IW);
        } else if (n) {
            sptr += n * C * IH * IW;
        }
        rep(ow, OW) {
            float numeratorw = mptr[0] * ow + mptr[1] * oh + mptr[2];
            float numeratorh = mptr[3] * ow + mptr[4] * oh + mptr[5];
            float denominator = mptr[6] * ow + mptr[7] * oh + mptr[8];
            float alphaw = numeratorw / denominator;
            float alphah = numeratorh / denominator;

            int iw0 = get_real_coord(std::floor(alphaw) + 0, IW);
            int iw1 = get_real_coord(std::floor(alphaw) + 1, IW);
            int ih0 = get_real_coord(std::floor(alphah) + 0, IH);
            int ih1 = get_real_coord(std::floor(alphah) + 1, IH);

            alphaw -= floor(alphaw);
            alphah -= floor(alphah);
            if (bmode != BorderMode::CONSTANT) {
                rep(c, C) {
                    auto val =
                            visit_src(c, ih0, iw0) * (1.0f - alphaw) * (1.0f - alphah) +
                            visit_src(c, ih0, iw1) * alphaw * (1.0f - alphah) +
                            visit_src(c, ih1, iw0) * (1.0f - alphaw) * alphah +
                            visit_src(c, ih1, iw1) * alphaw * alphah;
                    val = is_dst_float ? (val - zero_point) * scale : val - zero_point;
                    visit_dst(c, oh, ow) = output_converter(val);
                }
            } else {
                rep(c, C) {
                    auto val = visit_src_bd(c, ih0, iw0) * (1.0f - alphaw) *
                                       (1.0f - alphah) +
                               visit_src_bd(c, ih0, iw1) * alphaw * (1.0f - alphah) +
                               visit_src_bd(c, ih1, iw0) * (1.0f - alphaw) * alphah +
                               visit_src_bd(c, ih1, iw1) * alphaw * alphah;
                    val = std::isfinite(val) ? val : border_val;
                    val = is_dst_float ? (val - zero_point) * scale : val - zero_point;
                    visit_dst(c, oh, ow) = output_converter(val);
                }
            }
            if (is_dst_nchw4) {
                for (auto c = C; c < 4; ++c) {
                    visit_dst(c, oh, ow) = 0;
                }
            }
        }
    }
    MIDOUT_END();
}

#define INST(ctype, drc_ctype, mtype)                                        \
    template void WarpPerspectiveForwardImpl::kern_naive_dimshuffle_typecvt< \
            ctype, drc_ctype, mtype>(const KernParam<ctype, mtype>&, size_t);

INST(uint8_t, int8_t, float);
INST(uint8_t, float, float);

#undef INST

void WarpPerspectiveForwardImpl::exec(
        _megdnn_tensor_in src, _megdnn_tensor_in mat, _megdnn_tensor_in mat_idx,
        _megdnn_tensor_out dst, _megdnn_workspace workspace) {
    check_exec_allow_nhwc_mat_idx(
            src.layout, mat.layout, mat_idx.layout, dst.layout, workspace.size);
    size_t batch = dst.layout[0];

#define KERN_NAIVE(ct, mct)                                                            \
    auto kparam = KernParam<ct, mct>::from_tensors(                                    \
            param().format, param().bmode, param().border_val, src, mat, mat_idx, dst, \
            workspace);                                                                \
    auto run = [kparam, this](size_t index, size_t) { kern_naive(kparam, index); };    \
    MEGDNN_DISPATCH_MULTI_THREAD_CPU_KERN_OPR(run, kparam.oh* batch);

#define KERN_INT4(ct, mct)                                                             \
    auto kparam = KernParam<ct, mct>::from_tensors(                                    \
            param().format, param().bmode, param().border_val, src, mat, mat_idx, dst, \
            workspace);                                                                \
    auto run = [kparam, this](size_t index, size_t) {                                  \
        kern_naive_int4(kparam, index);                                                \
    };                                                                                 \
    MEGDNN_DISPATCH_MULTI_THREAD_CPU_KERN_OPR(run, kparam.oh* batch);

#define KERN_CD4(ct, mct)                                                              \
    auto kparam = KernParam<ct, mct>::from_tensors(                                    \
            param().format, param().bmode, param().border_val, src, mat, mat_idx, dst, \
            workspace);                                                                \
    auto run = [kparam, this](size_t index, size_t) {                                  \
        kern_naive_nhwcd4(kparam, index);                                              \
    };                                                                                 \
    MEGDNN_DISPATCH_MULTI_THREAD_CPU_KERN_OPR(run, kparam.oh* batch);

#define DISPATCH_ST(dt, ct, mct, kern)                       \
    if (src.layout.dtype.enumv() == DTypeTrait<dt>::enumv) { \
        kern(ct, mct);                                       \
        return;                                              \
    }

#define DISPATCH_ST_MT(dt, ct, kern)                                         \
    if (src.layout.dtype.enumv() == DTypeTrait<dt>::enumv) {                 \
        if (mat.layout.dtype.enumv() == DTypeTrait<dtype::Float32>::enumv) { \
            kern(ct, float);                                                 \
            return;                                                          \
        } else {                                                             \
            kern(ct, ct);                                                    \
            return;                                                          \
        }                                                                    \
    }

    if (param().format == Format::NHWCD4) {
        DISPATCH_ST(dtype::Float32, float, float, KERN_CD4);
        DISPATCH_ST(dtype::Quantized8Asymm, uint8_t, float, KERN_CD4);
        DISPATCH_ST(dtype::QuantizedS8, int8_t, float, KERN_CD4);

        DNN_INC_FLOAT16(DISPATCH_ST_MT(dtype::Float16, dt_float16, KERN_CD4));
        DNN_INC_FLOAT16(DISPATCH_ST_MT(dtype::BFloat16, dt_bfloat16, KERN_CD4));
        megdnn_throw(ssprintf(
                             "Unsupported input DType in "
                             "WarpPerspective: %s",
                             src.layout.dtype.name())
                             .c_str());
    }
#undef KERN_CD4

    if (src.layout.dtype.enumv() == DTypeTrait<dtype::QuantizedS4>::enumv) {
        DISPATCH_ST(dtype::QuantizedS4, dt_qint4, float, KERN_INT4);
        megdnn_assert(
                0, "Unsupported input DType in WarpPerspective: %s",
                src.layout.dtype.name());
    } else if (src.layout.dtype.enumv() == DTypeTrait<dtype::Quantized4Asymm>::enumv) {
        DISPATCH_ST(dtype::Quantized4Asymm, dt_quint4, float, KERN_INT4);
        megdnn_assert(
                0, "Unsupported input DType in WarpPerspective: %s",
                src.layout.dtype.name());
    }
#undef KERN_INT4

    bool is_fusion_dtype = src.layout.dtype.enumv() != dst.layout.dtype.enumv();
    bool is_u8_or_qu8_in =
            src.layout.dtype.enumv() == DTypeTrait<dtype::Uint8>::enumv ||
            src.layout.dtype.enumv() == DTypeTrait<dtype::Quantized8Asymm>::enumv;

    if (is_fusion_dtype && is_u8_or_qu8_in &&
        ((param().format == Format::NCHW_NCHW4_IC_SMALL) ||
         (param().format == Format::NHWC_NCHW4_IC_SMALL) ||
         (param().format == Format::NHWC_NCHW) || (param().format == Format::NCHW))) {
        megdnn_assert(
                src.layout.dtype.enumv() == DTypeTrait<dtype::Quantized8Asymm>::enumv ||
                        src.layout.dtype.enumv() == DTypeTrait<dtype::Uint8>::enumv,
                "Unsupported input DType in WarpPerspective: %s",
                src.layout.dtype.name());

        float scale = 1.f;
        if (src.layout.dtype.enumv() == DTypeTrait<dtype::Quantized8Asymm>::enumv) {
            scale = src.layout.dtype.param<dtype::Quantized8Asymm>().scale;
        }

        auto kparam = KernParam<uint8_t, float>::from_tensors(
                param().format, param().bmode, param().border_val, src, mat, mat_idx,
                dst, workspace);

        if (dst.layout.dtype.enumv() == DTypeTrait<dtype::Float32>::enumv) {
            auto run = [kparam, this](size_t index, size_t) {
                kern_naive_dimshuffle_typecvt<uint8_t, float, float>(kparam, index);
            };
            MEGDNN_DISPATCH_MULTI_THREAD_CPU_KERN_OPR(run, kparam.oh * batch);
            return;
        } else if (
                dst.layout.dtype.enumv() == DTypeTrait<dtype::QuantizedS8>::enumv &&
                dst.layout.dtype.param<dtype::QuantizedS8>().scale == scale) {
            auto run = [kparam, this](size_t index, size_t) {
                kern_naive_dimshuffle_typecvt<uint8_t, int8_t, float>(kparam, index);
            };
            MEGDNN_DISPATCH_MULTI_THREAD_CPU_KERN_OPR(run, kparam.oh * batch);
            return;
        } else {
            megdnn_assert(
                    0, "Unsupported DType in WarpPerspective Dimshuffle Typecvt: %s",
                    src.layout.dtype.name());
        }
    }

    if (warp::is_cv_available(
                src.layout, mat.layout, dst.layout, param().imode, param().format)) {
        MIDOUT_BEGIN(megdnn_naive_warpperspective, void) {
            warp_perspective_cv_exec(
                    src, mat, mat_idx, dst, param().border_val, param().bmode,
                    param().imode, handle());
        }
        MIDOUT_END();
    } else {
        megdnn_assert(warp::is_dnn_available(
                src.layout, mat.layout, dst.layout, param().imode, param().format));
        /*!
         * We currently use floating point for all WarpPerspective
         * computation, so even if the input ctype is one of the integer
         * type, mtype should always be float32.
         *
         * \warning It's different with \c WarpAffine, with mtype be float16
         * if input type is float16.
         */

        DISPATCH_ST(dtype::Float32, float, float, KERN_NAIVE);
        DISPATCH_ST(dtype::Int8, int8_t, float, KERN_NAIVE);
        DISPATCH_ST(dtype::QuantizedS8, int8_t, float, KERN_NAIVE);
        DISPATCH_ST(dtype::Uint8, uint8_t, float, KERN_NAIVE);
        DISPATCH_ST(dtype::Quantized8Asymm, uint8_t, float, KERN_NAIVE);

        DNN_INC_FLOAT16(DISPATCH_ST_MT(dtype::Float16, dt_float16, KERN_NAIVE));
        DNN_INC_FLOAT16(DISPATCH_ST_MT(dtype::BFloat16, dt_bfloat16, KERN_NAIVE));
        megdnn_throw(ssprintf(
                             "Unsupported input DType in "
                             "WarpPerspective: %s",
                             src.layout.dtype.name())
                             .c_str());
    }
#undef KERN_NAIVE
#undef DISPATCH_ST_MT
#undef DISPATCH_ST
}

template <typename ctype, typename mtype>
void WarpPerspectiveBackwardDataImpl::kern_naive(
        const KernParam<ctype, mtype>& kern_param) {
    const int N = kern_param.n_mat, C = kern_param.c, IH = kern_param.ih,
              IW = kern_param.iw;
    const int OH = kern_param.oh, OW = kern_param.ow;
    auto hptr = static_cast<ctype*>(kern_param.diff_ptr.get_ptr());
    auto mptr = static_cast<mtype*>(kern_param.mat_ptr.get_ptr());
    auto sptr = static_cast<ctype*>(kern_param.grad_ptr.get_ptr());
    auto midx_ptr = static_cast<int*>(kern_param.midx_ptr.get_ptr());
    if (midx_ptr) {
        std::memset(sptr, 0, sizeof(ctype) * kern_param.n_src * C * IH * IW);
    } else {
        std::memset(sptr, 0, sizeof(ctype) * N * C * IH * IW);
    }
    rep(n, N) {
        if (midx_ptr) {
            sptr = static_cast<ctype*>(kern_param.grad_ptr.get_ptr()) +
                   midx_ptr[n] * C * IH * IW;
        } else {
            sptr = static_cast<ctype*>(kern_param.grad_ptr.get_ptr()) + n * C * IH * IW;
        }
        rep(oh, OH) rep(ow, OW) {
            float numeratorw = mptr[0] * ow + mptr[1] * oh + mptr[2];
            float numeratorh = mptr[3] * ow + mptr[4] * oh + mptr[5];
            float denominator = mptr[6] * ow + mptr[7] * oh + mptr[8];
            float alphaw = numeratorw / denominator;
            float alphah = numeratorh / denominator;

            int iw0 = get_real_coord(std::floor(alphaw) + 0, IW);
            int iw1 = get_real_coord(std::floor(alphaw) + 1, IW);
            int ih0 = get_real_coord(std::floor(alphah) + 0, IH);
            int ih1 = get_real_coord(std::floor(alphah) + 1, IH);

            alphaw -= floor(alphaw);
            alphah -= floor(alphah);
            rep(c, C) {
                float hidden = hptr[c * OH * OW + oh * OW + ow];
                if (iw0 != -1 && ih0 != -1) {
                    sptr[c * IH * IW + ih0 * IW + iw0] +=
                            (1.0f - alphaw) * (1.0f - alphah) * hidden;
                }
                if (iw0 != -1 && ih1 != -1) {
                    sptr[c * IH * IW + ih1 * IW + iw0] +=
                            (1.0f - alphaw) * alphah * hidden;
                }
                if (iw1 != -1 && ih0 != -1) {
                    sptr[c * IH * IW + ih0 * IW + iw1] +=
                            alphaw * (1.0f - alphah) * hidden;
                }
                if (iw1 != -1 && ih1 != -1) {
                    sptr[c * IH * IW + ih1 * IW + iw1] += alphaw * alphah * hidden;
                }
            }
        }
        hptr += C * OH * OW;
        mptr += 3 * 3;
    }
}

void WarpPerspectiveBackwardDataImpl::exec(
        _megdnn_tensor_in mat, _megdnn_tensor_in mat_idx, _megdnn_tensor_in diff,
        _megdnn_tensor_out grad, _megdnn_workspace workspace) {
    check_exec(mat.layout, mat_idx.layout, diff.layout, grad.layout, workspace.size);
    megdnn_assert(
            param().format == param::WarpPerspective::Format::NCHW,
            "invalid warp_perspective format");
#define DISPATCH_ST_MT(dt, ct)                                                       \
    if (diff.layout.dtype.enumv() == DTypeTrait<dt>::enumv) {                        \
        if (mat.layout.dtype.enumv() == DTypeTrait<dtype::Float32>::enumv) {         \
            auto kparam =                                                            \
                    KernParam<ct, float>::from_tensors(mat, mat_idx, diff, grad);    \
            MEGDNN_DISPATCH_CPU_KERN_OPR(kern_naive(kparam));                        \
            return;                                                                  \
        } else {                                                                     \
            auto kparam = KernParam<ct, ct>::from_tensors(mat, mat_idx, diff, grad); \
            MEGDNN_DISPATCH_CPU_KERN_OPR(kern_naive(kparam));                        \
            return;                                                                  \
        }                                                                            \
    }
    DISPATCH_ST_MT(dtype::Float32, dt_float32);
    DNN_INC_FLOAT16(DISPATCH_ST_MT(dtype::BFloat16, dt_bfloat16));
    megdnn_throw(ssprintf(
                         "Unsupported input DType in "
                         "WarpPerspective: %s",
                         diff.layout.dtype.name())
                         .c_str());
#undef DISPATCH_ST_MT
}

template <typename ctype, typename mtype>
void WarpPerspectiveBackwardMatImpl::kern_naive(
        const KernParam<ctype, mtype>& kern_param) {
    const int N = kern_param.n_mat, C = kern_param.c, IH = kern_param.ih,
              IW = kern_param.iw;
    const int OH = kern_param.oh, OW = kern_param.ow;

    auto hptr = static_cast<ctype*>(kern_param.diff_ptr.get_ptr());
    auto sptr = static_cast<ctype*>(kern_param.src_ptr.get_ptr());
    auto res = static_cast<mtype*>(kern_param.grad_ptr.get_ptr());
    auto mptr = static_cast<mtype*>(kern_param.mat_ptr.get_ptr());
    auto midx_ptr = static_cast<int*>(kern_param.midx_ptr.get_ptr());
    auto border_val = kern_param.border_val;
    std::memset(res, 0, sizeof(float) * N * 3 * 3);
    rep(n, N) {
        if (midx_ptr) {
            sptr = static_cast<ctype*>(kern_param.src_ptr.get_ptr()) +
                   midx_ptr[n] * C * IH * IW;
        } else {
            sptr = static_cast<ctype*>(kern_param.src_ptr.get_ptr()) + n * C * IH * IW;
        }
        rep(oh, OH) rep(ow, OW) {
            float numeratorw = mptr[0] * ow + mptr[1] * oh + mptr[2];
            float numeratorh = mptr[3] * ow + mptr[4] * oh + mptr[5];
            float denominator = mptr[6] * ow + mptr[7] * oh + mptr[8];
            float denominator2 = denominator * denominator;
            float alphaw = numeratorw / denominator;
            float alphah = numeratorh / denominator;

            int iw0 = get_real_coord(std::floor(alphaw) + 0, IW);
            int iw1 = get_real_coord(std::floor(alphaw) + 1, IW);
            int ih0 = get_real_coord(std::floor(alphah) + 0, IH);
            int ih1 = get_real_coord(std::floor(alphah) + 1, IH);

            alphaw -= floor(alphaw);
            alphah -= floor(alphah);
            rep(c, C) {
                float b = border_val;
                float hidden = hptr[c * OH * OW + oh * OW + ow];
                float dalphaw = 0;
                dalphaw -=
                        ((ih0 != -1 && iw0 != -1) ? sptr[c * IH * IW + ih0 * IW + iw0]
                                                  : b) *
                        (1.0f - alphah);
                dalphaw +=
                        ((ih0 != -1 && iw1 != -1) ? sptr[c * IH * IW + ih0 * IW + iw1]
                                                  : b) *
                        (1.0f - alphah);
                dalphaw -=
                        ((ih1 != -1 && iw0 != -1) ? sptr[c * IH * IW + ih1 * IW + iw0]
                                                  : b) *
                        alphah;
                dalphaw +=
                        ((ih1 != -1 && iw1 != -1) ? sptr[c * IH * IW + ih1 * IW + iw1]
                                                  : b) *
                        alphah;
                float dalphah = 0;
                dalphah -=
                        ((ih0 != -1 && iw0 != -1) ? sptr[c * IH * IW + ih0 * IW + iw0]
                                                  : b) *
                        (1.0f - alphaw);
                dalphah -=
                        ((ih0 != -1 && iw1 != -1) ? sptr[c * IH * IW + ih0 * IW + iw1]
                                                  : b) *
                        alphaw;
                dalphah +=
                        ((ih1 != -1 && iw0 != -1) ? sptr[c * IH * IW + ih1 * IW + iw0]
                                                  : b) *
                        (1.0f - alphaw);
                dalphah +=
                        ((ih1 != -1 && iw1 != -1) ? sptr[c * IH * IW + ih1 * IW + iw1]
                                                  : b) *
                        alphaw;
                // printf("dalphaw=%f dalphah=%f\n", dalphaw, dalphaw);
                float dw[9], dh[9];
                // dw[i] = d(iw)/d(mat[i])
                dw[0] = ow / denominator;
                dw[1] = oh / denominator;
                dw[2] = 1.0f / denominator;
                dw[3] = 0.0f;
                dw[4] = 0.0f;
                dw[5] = 0.0f;
                float ddenominatorw = -numeratorw / denominator2;
                dw[6] = ow * ddenominatorw;
                dw[7] = oh * ddenominatorw;
                dw[8] = 1.0f * ddenominatorw;
                // dh[i] = d(ih)/d(mat[i])
                dh[0] = 0.0f;
                dh[1] = 0.0f;
                dh[2] = 0.0f;
                dh[3] = ow / denominator;
                dh[4] = oh / denominator;
                dh[5] = 1.0f / denominator;
                float ddenominatorh = -numeratorh / denominator2;
                dh[6] = ow * ddenominatorh;
                dh[7] = oh * ddenominatorh;
                dh[8] = 1.0f * ddenominatorh;
                rep(i, 9) {
                    float delta = hidden * dalphaw * dw[i] + hidden * dalphah * dh[i];
                    if (std::isfinite(delta))
                        res[i] += delta;
                }
            }
        }
        hptr += C * OH * OW;
        mptr += 3 * 3;
        res += 3 * 3;
    }
}

void WarpPerspectiveBackwardMatImpl::exec(
        _megdnn_tensor_in src, _megdnn_tensor_in mat, _megdnn_tensor_in mat_idx,
        _megdnn_tensor_in diff, _megdnn_tensor_out grad, _megdnn_workspace workspace) {
    check_exec(
            src.layout, mat.layout, mat_idx.layout, diff.layout, grad.layout,
            workspace.size);
#define DISPATCH_ST_MT(dt, ct)                                               \
    if (src.layout.dtype.enumv() == DTypeTrait<dt>::enumv) {                 \
        if (mat.layout.dtype.enumv() == DTypeTrait<dtype::Float32>::enumv) { \
            auto kparam = KernParam<ct, float>::from_tensors(                \
                    param().border_val, src, mat, mat_idx, diff, grad);      \
            MEGDNN_DISPATCH_CPU_KERN_OPR(kern_naive(kparam));                \
            return;                                                          \
        } else {                                                             \
            auto kparam = KernParam<ct, ct>::from_tensors(                   \
                    param().border_val, src, mat, mat_idx, diff, grad);      \
            MEGDNN_DISPATCH_CPU_KERN_OPR(kern_naive(kparam));                \
            return;                                                          \
        }                                                                    \
    }
    DISPATCH_ST_MT(dtype::Float32, dt_float32);
    DNN_INC_FLOAT16(DISPATCH_ST_MT(dtype::BFloat16, dt_bfloat16));
    megdnn_throw(ssprintf(
                         "Unsupported input DType in "
                         "WarpPerspective: %s",
                         diff.layout.dtype.name())
                         .c_str());
#undef DISPATCH_ST_MT
}

// vim: syntax=cpp.doxygen
