/**
 * \file dnn/src/fallback/elemwise_helper/kimpl/typecvt.h
 */
#pragma once

#include "src/fallback/elemwise_helper/kimpl/op_base.h"

namespace megdnn {
namespace fallback {

template <typename src_ctype, typename dst_ctype = src_ctype>
struct TypeCvtOp;

template <>
struct TypeCvtOp<dt_qint32, dt_qint8> : UnaryOpBase<dt_qint32, dt_qint8> {
    using UnaryOpBase::UnaryOpBase;
    constexpr static size_t SIMD_WIDTH = GI_SIMD_LEN_BYTE / sizeof(float);

    void operator()(const GI_INT32_V2_t& vsrc, dt_qint8* dst) const {
        GiStoreLowInt8(reinterpret_cast<int8_t*>(dst), operator()(vsrc));
    }
    void operator()(const GI_INT32_t& vsrc, dt_qint8* dst) const {
        GiStoreLane0Int32(
                reinterpret_cast<int32_t*>(dst), (GI_INT32_t)(operator()(vsrc)));
    }
    void operator()(const src_ctype& src, dst_ctype* dst) const {
        *dst = operator()(src);
    }
    dt_qint8 operator()(const dt_qint32& src) const {
        float fsrc = src.as_int32() * this->scale;
        return QConverter::convert<dt_qint8, float>(fsrc);
    }

    GI_INT8_t operator()(const GI_INT32_V2_t& vsrc) const {
        auto vitem0 = GiMultiplyFloat32(GiCastToFloat32(vsrc.val[0]), this->vscale);
        auto vitem1 = GiMultiplyFloat32(GiCastToFloat32(vsrc.val[1]), this->vscale);

        return QConverter::convert<GI_INT8_t, GI_FLOAT32_V2_t>({{vitem0, vitem1}});
    }
    GI_INT8_t operator()(const GI_INT32_t& src) const {
        auto vitem0 = GiMultiplyFloat32(GiCastToFloat32(src), this->vscale);
        return QConverter::convert<GI_INT8_t, GI_FLOAT32_t>(vitem0);
    }
    GI_INT8_t operator()(const GI_FLOAT32_t& src) const {
        auto vitem0 = GiMultiplyFloat32(src, this->vscale);
        return QConverter::convert<GI_INT8_t, GI_FLOAT32_t>(vitem0);
    }
};

}  // namespace fallback
}  // namespace megdnn

// vim: syntax=cpp.doxygen
