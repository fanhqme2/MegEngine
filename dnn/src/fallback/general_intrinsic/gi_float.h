#pragma once

#include "gi_common.h"

GI_FORCEINLINE
GI_INT32_t GiReinterpretAsInt32(GI_FLOAT32_t In) {
#if defined(GI_NEON_INTRINSICS)
    return vreinterpretq_s32_f32(In);
#elif defined(GI_SSE2_INTRINSICS)
    return _mm_castps_si128(In);
#else
    return (GI_INT32_t)In;
#endif
}

GI_FORCEINLINE
GI_UINT32_t GiReinterpretAsUint32(GI_FLOAT32_t In) {
#if defined(GI_NEON_INTRINSICS)
    return vreinterpretq_u32_f32(In);
#elif defined(GI_SSE2_INTRINSICS)
    return _mm_castps_si128(In);
#else
    return (GI_UINT32_t)In;
#endif
}

GI_FORCEINLINE
GI_FLOAT32_t GiReintInt32ToFloat32(GI_INT32_t Vector) {
#if defined(GI_NEON_INTRINSICS)
    return vreinterpretq_f32_s32(Vector);
#elif defined(GI_SSE2_INTRINSICS)
    return _mm_castsi128_ps(Vector);
#else
    return (GI_FLOAT32_t)Vector;
#endif
}

GI_FORCEINLINE
GI_FLOAT32_t GiReintUint32ToFloat32(GI_UINT32_t Vector) {
#if defined(GI_NEON_INTRINSICS)
    return vreinterpretq_f32_u32(Vector);
#elif defined(GI_SSE2_INTRINSICS)
    return _mm_castsi128_ps(Vector);
#else
    return (GI_FLOAT32_t)Vector;
#endif
}

GI_FORCEINLINE
GI_INT32_t GiRoundAsInt32(GI_FLOAT32_t Vector) {
#if defined(GI_NEON_INTRINSICS)
#if __ARM_ARCH >= 8
    return vcvtaq_s32_f32(Vector);
#else
    float32x4_t vinc0 = vbslq_f32(vcgeq_f32(Vector, vfzero), vfhalf, vfneg_half);
    return vcvtq_s32_f32(vaddq_f32(Vector, vinc0));
#endif
#elif defined(GI_SSE42_INTRINSICS)
    __m128 vinc0 = _mm_blendv_ps(vfneg_half, vfhalf, _mm_cmpge_ps(Vector, vfzero));
    return _mm_cvttps_epi32(_mm_add_ps(Vector, vinc0));
#else
    GI_INT32_t ret;
    GI_INT32_NAIVE_t tmp_ret;
    GI_FLOAT32_NAIVE_t s0;
    memcpy(&s0, &Vector, sizeof(GI_FLOAT32_NAIVE_t));
    for (size_t i = 0; i < GI_SIMD_LEN_BYTE / sizeof(float); i++) {
        tmp_ret[i] = (int32_t)round(s0[i]);
    }
    memcpy(&ret, &tmp_ret, sizeof(GI_INT32_t));
    return ret;
#endif
}

GI_FORCEINLINE
GI_INT32_t GiCastToInt32(GI_FLOAT32_t Vector) {
#if defined(GI_NEON_INTRINSICS)
    return vcvtq_s32_f32(Vector);
#elif defined(GI_SSE2_INTRINSICS)
    return _mm_cvttps_epi32(Vector);
#else
    GI_INT32_t ret;
    for (size_t i = 0; i < GI_SIMD_LEN_BYTE / sizeof(float); i++) {
        ret[i] = (int32_t)(Vector[i]);
    }
    return ret;
#endif
}

GI_FORCEINLINE
GI_FLOAT32_t GiCastToFloat32(GI_INT32_t Vector) {
#if defined(GI_NEON_INTRINSICS)
    return vcvtq_f32_s32(Vector);
#elif defined(GI_SSE2_INTRINSICS)
    return _mm_cvtepi32_ps(Vector);
#else
    GI_FLOAT32_t ret;
    for (size_t i = 0; i < GI_SIMD_LEN_BYTE / sizeof(int32_t); i++) {
        ret[i] = (float)Vector[i];
    }
    return ret;
#endif
}

GI_FORCEINLINE
GI_FLOAT32_t GiLoadBroadcastFloat32(const float* Value) {
#if defined(GI_NEON_INTRINSICS)
    return vld1q_dup_f32(Value);
#elif defined(GI_SSE2_INTRINSICS)
    return _mm_load_ps1(Value);
#else
    GI_FLOAT32_t ret;
    for (size_t i = 0; i < GI_SIMD_LEN_BYTE / sizeof(float); i++) {
        ret[i] = *Value;
    }
    return ret;
#endif
}

GI_FORCEINLINE
GI_FLOAT32_t GiZeroFloat32(void) {
#if defined(GI_NEON_INTRINSICS)
    return vdupq_n_f32(0.0f);
#elif defined(GI_SSE2_INTRINSICS)
    return _mm_setzero_ps();
#else
    return GiBroadcastFloat32(0.0f);
#endif
}

GI_FORCEINLINE
GI_FLOAT32_t GiLoadFloat32(const float* Buffer) {
#if defined(GI_NEON_INTRINSICS)
    return vld1q_f32(Buffer);
#elif defined(GI_SSE2_INTRINSICS)
    if ((((uintptr_t)(Buffer)) & 15) == 0)
        return _mm_load_ps(Buffer);
    else
        return _mm_loadu_ps(Buffer);
#else
    GI_FLOAT32_t ret;
    for (size_t i = 0; i < GI_SIMD_LEN_BYTE / sizeof(float); i++) {
        ret[i] = Buffer[i];
    }
    return ret;
#endif
}

GI_FORCEINLINE
GI_FLOAT32_V2_t GiLoadFloat32V2(const float* Buffer) {
#if defined(GI_NEON_INTRINSICS)
    return vld1q_f32_x2(Buffer);
#else
    GI_FLOAT32_V2_t v;
    v.val[0] = GiLoadFloat32(Buffer);
    v.val[1] = GiLoadFloat32(Buffer + GI_SIMD_LEN_BYTE / sizeof(float));

    return v;
#endif
}

GI_FORCEINLINE
GI_FLOAT32_t GiLoadFloat32LowHalf(const float* Buffer) {
#if defined(GI_NEON_INTRINSICS)
    return vcombine_f32(vld1_f32(Buffer), vdup_n_f32(0.f));
#elif defined(GI_SSE2_INTRINSICS)
    typedef __m64_128 float32x2_t;
    float32x2_t low, high;
    low.m64_f32[0] = Buffer[0];
    low.m64_f32[1] = Buffer[1];
    high.m64_f32[0] = 0;
    high.m64_f32[1] = 0;
    __m128i res = _mm_unpacklo_epi64(_pM128i(low), _pM128i(high));
    return _M128(res);
#else
    GI_FLOAT32_t ret;
    memset(&ret, 0, sizeof(GI_FLOAT32_t));
    for (size_t i = 0; i < GI_SIMD_LEN_BYTE / sizeof(float) / 2; i++) {
        ret[i] = Buffer[i];
    }
    return ret;
#endif
}

GI_FORCEINLINE
GI_FLOAT32_t GiMlaqFloat32(GI_FLOAT32_t a, GI_FLOAT32_t b, GI_FLOAT32_t c) {
#if defined(GI_NEON_INTRINSICS)
#if defined(__ARM_FEATURE_FMA)
    return vfmaq_f32(a, b, c);
#else
    return vmlaq_f32(a, b, c);
#endif
#elif defined(GI_SSE2_INTRINSICS)
    // fma is coming soon, but right now:
    __m128 res;
    res = _mm_mul_ps(c, b);
    return _mm_add_ps(a, res);
#else
    GI_FLOAT32_t ret;
    for (size_t i = 0; i < GI_SIMD_LEN_BYTE / sizeof(float); i++) {
        ret[i] = a[i] + (b[i] * c[i]);
    }
    return ret;
#endif
}

GI_FORCEINLINE GI_FLOAT32_V2_t GiUzpqFloat32(GI_FLOAT32_t a, GI_FLOAT32_t b) {
#if defined(GI_NEON_INTRINSICS)
    return vuzpq_f32(a, b);
#elif defined(GI_SSE2_INTRINSICS)
    GI_FLOAT32_V2_t v32x4;
    v32x4.val[0] = _mm_shuffle_ps(a, b, _MM_SHUFFLE(2, 0, 2, 0));
    v32x4.val[1] = _mm_shuffle_ps(a, b, _MM_SHUFFLE(3, 1, 3, 1));
    return v32x4;
#else
    GI_FLOAT32_V2_t ret;
    ret.val[0][0] = a[0];
    ret.val[0][1] = a[2];
    ret.val[0][2] = b[0];
    ret.val[0][3] = b[2];
    ret.val[1][0] = a[1];
    ret.val[1][1] = a[3];
    ret.val[1][2] = b[1];
    ret.val[1][3] = b[3];
    return ret;
#endif
}

GI_FORCEINLINE float32x2_t GiDupFloat32(float a) {
#if defined(GI_NEON_INTRINSICS)
    return vdup_n_f32(a);
#elif defined(GI_SSE2_INTRINSICS)
    float32x2_t res;
    res.m64_f32[0] = a;
    res.m64_f32[1] = a;
    return res;
#else
    float32x2_t res;
    res[0] = a;
    res[1] = a;
    return res;
#endif
}

GI_FORCEINLINE float32x2_t GiLdFloat32(float const* ptr) {
#if defined(GI_NEON_INTRINSICS)
    return vld1_f32(ptr);
#elif defined(GI_SSE2_INTRINSICS)
    float32x2_t res;
    res.m64_f32[0] = *(ptr);
    res.m64_f32[1] = *(ptr + 1);
    return res;
#else
    float32x2_t res;
    res[0] = *(ptr);
    res[1] = *(ptr + 1);
    return res;
#endif
}

GI_FORCEINLINE float32x2_t GiAddDFloat32(float32x2_t a, float32x2_t b) {
#if defined(GI_NEON_INTRINSICS)
    return vadd_f32(a, b);
#elif defined(GI_SSE2_INTRINSICS)
    __m128 res;
    __m64_128 res64;
    res = _mm_add_ps(_pM128(a), _pM128(b));  // SSE, use only low 64 bits
    _M64f(res64, res);
    return res64;
#else
    float32x2_t res;
    res[0] = a[0] + b[0];
    res[1] = a[1] + b[1];
    return res;
#endif
}

#if defined(GI_NEON_INTRINSICS)
#define GiGetLaneFloat32(v, lane) vget_lane_f32(v, lane)
#else
GI_FORCEINLINE float __gi_vget_lane_f32(float32x2_t v, const int lane) {
#if defined(GI_SSE2_INTRINSICS)
    return _sse_vget_lane_f32(v, lane);
#else
    return v[lane];
#endif
}
#define GiGetLaneFloat32(v, lane) __gi_vget_lane_f32(v, lane)
#endif

#if defined(GI_NEON_INTRINSICS)
#define GiSetLaneFloat32(value, vec, lane) vset_lane_f32(value, vec, lane)
#else
GI_FORCEINLINE float32x2_t
__gi_vset_lane_f32(float32_t value, float32x2_t vec, int lane) {
#if defined(GI_SSE2_INTRINSICS)
    float32x2_t res;
    res = vec;
    res.m64_f32[lane] = value;
    return res;
#else
    float32x2_t res;
    res = vec;
    res[lane] = value;
    return res;
#endif
}
#define GiSetLaneFloat32(value, vec, lane) __gi_vset_lane_f32(value, vec, lane)
#endif

GI_FORCEINLINE void GiSt1Float32(float* ptr, float32x2_t val) {
#if defined(GI_NEON_INTRINSICS)
    return vst1_f32(ptr, val);
#elif defined(GI_SSE2_INTRINSICS)
    *(ptr) = val.m64_f32[0];
    *(ptr + 1) = val.m64_f32[1];
    return;
#else
    *(ptr) = val[0];
    *(ptr + 1) = val[1];
    return;
#endif
}

GI_FORCEINLINE GI_FLOAT32_V2_t GiLd2qFloat32(const float* Buffer) {
#if defined(GI_NEON_INTRINSICS)
    return vld2q_f32(Buffer);
#elif defined(GI_SSE2_INTRINSICS)
    GI_FLOAT32_V2_t v;
    v.val[0] = GiLoadFloat32(Buffer);
    v.val[1] = GiLoadFloat32((Buffer + 4));
    v = GiUzpqFloat32(v.val[0], v.val[1]);
    return v;
#else
    GI_FLOAT32_V2_t ret;
    ret.val[0][0] = Buffer[0];
    ret.val[0][1] = Buffer[2];
    ret.val[0][2] = Buffer[4];
    ret.val[0][3] = Buffer[6];
    ret.val[1][0] = Buffer[1];
    ret.val[1][1] = Buffer[3];
    ret.val[1][2] = Buffer[5];
    ret.val[1][3] = Buffer[7];
    return ret;
#endif
}

#if defined(GI_NEON_INTRINSICS)
#define GiExtqFloat32(a, b, n) vextq_f32(a, b, n)
#elif defined(GI_SSE2_INTRINSICS)
#define GiExtqFloat32(a, b, n) _M128(_sse_vextq_s32(_M128i(a), _M128i(b), n));
#else
GI_FORCEINLINE GI_FLOAT32_t
__naive_gi_vextq_f32(GI_FLOAT32_t a, GI_FLOAT32_t b, const int n) {
    GI_FLOAT32_t ret;
    int t_count = GI_SIMD_LEN_BYTE / sizeof(float);
    int a_count = t_count - n;
    for (int i = 0; i < a_count; i++) {
        ret[i] = a[i + n];
    }
    for (int i = 0; i < n; i++) {
        ret[i + a_count] = b[i];
    }
    return ret;
}
#define GiExtqFloat32(a, b, n) __naive_gi_vextq_f32(a, b, n)
#endif

GI_FORCEINLINE
GI_FLOAT32_t GiMultiplySubFloat32(
        GI_FLOAT32_t VectorSum, GI_FLOAT32_t Vector1, GI_FLOAT32_t Vector2) {
#if defined(GI_NEON_INTRINSICS)
    return vmlsq_f32(VectorSum, Vector1, Vector2);
#elif defined(GI_SSE2_INTRINSICS)
    return _mm_sub_ps(VectorSum, _mm_mul_ps(Vector1, Vector2));
#else
    GI_FLOAT32_t ret;
    for (size_t i = 0; i < GI_SIMD_LEN_BYTE / sizeof(float); i++) {
        ret[i] = VectorSum[i] - Vector1[i] * Vector2[i];
    }

    return ret;
#endif
}

#if defined(GI_SSE2_INTRINSICS)
GI_FORCEINLINE GI_FLOAT32_t
_MM_INSERT_PS(GI_FLOAT32_t vec, GI_FLOAT32_t p, const int LANE) {
    _GI_ALIGN_16 uint32_t mask[4] = {0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff};
    __m128 tmp, vec_masked, p_masked;
    mask[LANE >> 4] = 0x0;
    vec_masked = _mm_and_ps(*(__m128*)mask, vec);
    p_masked = _mm_andnot_ps(*(__m128*)mask, p);
    tmp = _mm_or_ps(vec_masked, p_masked);
    return tmp;
}

GI_FORCEINLINE float32x2_t sse_vget_high_f32(GI_FLOAT32_t a) {
    __m128i res;
    __m64_128 res64;
    res = _mm_unpackhi_epi64(_M128i(a), _M128i(a));
    return64(res);
}

GI_FORCEINLINE float32x2_t sse_vget_low_f32(GI_FLOAT32_t a) {
    float32x2_t res64;
    _M64f(res64, a);
    return res64;
}

GI_FORCEINLINE GI_FLOAT32_t
sse_vmlaq_lane_f32(GI_FLOAT32_t a, GI_FLOAT32_t b, float32x2_t v, int l) {
    float32_t vlane;
    GI_FLOAT32_t c;
    vlane = _sse_vget_lane_f32(v, l);
    c = _mm_set1_ps(vlane);
    return GiMlaqFloat32(a, b, c);
}

GI_FORCEINLINE int _MM_EXTRACT_PS(__m128 vec, const int LANE) {
    _GI_ALIGN_16 int32_t tmp[4];
    _mm_store_si128((__m128i*)tmp, _M128i(vec));
    return tmp[LANE];
}

GI_FORCEINLINE float32_t sse_vgetq_lane_f32(GI_FLOAT32_t vec, int lane) {
    float32_t floatVal;
    char* const floatVal_c = (char*)&floatVal;
    *((int32_t*)floatVal_c) = _MM_EXTRACT_PS(vec, lane);
    return floatVal;
}

GI_FORCEINLINE GI_FLOAT32_t
sse_vmlsq_lane_f32(GI_FLOAT32_t a, GI_FLOAT32_t b, float32x2_t v, int l) {
    float32_t vlane;
    GI_FLOAT32_t c;
    vlane = (float)GiGetLaneFloat32(v, l);
    c = GiBroadcastFloat32(vlane);
    return GiMultiplySubFloat32(a, b, c);
}

#endif

#if defined(GI_NEON_INTRINSICS)
#define GiLd1qLaneFloat32(Buffer, src, n) vld1q_lane_f32(Buffer, src, n)
#else
GI_FORCEINLINE GI_FLOAT32_t
__gi_vld1q_lane_f32(const float* Buffer, GI_FLOAT32_t src, const int n) {
#if defined(GI_SSE2_INTRINSICS)
    GI_FLOAT32_t p;
    p = _mm_set1_ps(*(Buffer));
    return _MM_INSERT_PS(src, p, _INSERTPS_NDX(0, n));
#else
    GI_FLOAT32_t ret;
    memcpy(&ret, &src, sizeof(GI_FLOAT32_t));
    ret[n] = *Buffer;
    return ret;
#endif
}
#define GiLd1qLaneFloat32(Buffer, src, n) __gi_vld1q_lane_f32(Buffer, src, n)
#endif

#if defined(GI_NEON_INTRINSICS)
#define GiSetqLaneFloat32(value, vec, lane) vsetq_lane_f32(value, vec, lane)
#else
GI_FORCEINLINE GI_FLOAT32_t
__gi_vsetq_lane_f32(float value, GI_FLOAT32_t vec, const int lane) {
    float val = value;
    return GiLd1qLaneFloat32(&val, vec, lane);
}
#define GiSetqLaneFloat32(value, vec, lane) __gi_vsetq_lane_f32(value, vec, lane)
#endif

#if defined(GI_NEON_INTRINSICS)
#define GiMlaqLaneFloat32HighHalf(a, b, v, lane) \
    vmlaq_lane_f32(a, b, vget_high_f32(v), lane)
#elif defined(GI_SSE2_INTRINSICS)
#define GiMlaqLaneFloat32HighHalf(a, b, v, lane) \
    sse_vmlaq_lane_f32(a, b, sse_vget_high_f32(v), lane)
#else
GI_FORCEINLINE GI_FLOAT32_t __naive_gi_vmlaq_lane_f32_high_half(
        GI_FLOAT32_t a, GI_FLOAT32_t b, GI_FLOAT32_t v, const int lane) {
    GI_FLOAT32_t ret;
    for (size_t i = 0; i < GI_SIMD_LEN_BYTE / sizeof(float); i++) {
        ret[i] = a[i] + (b[i] * v[lane + 2]);
    }
    return ret;
}
#define GiMlaqLaneFloat32HighHalf(a, b, v, lane) \
    __naive_gi_vmlaq_lane_f32_high_half(a, b, v, lane)
#endif

#if defined(GI_NEON_INTRINSICS)
#define GiVmlaqLaneFloat32LowHalf(a, b, v, lane) \
    vmlaq_lane_f32(a, b, vget_low_f32(v), lane)
#elif defined(GI_SSE2_INTRINSICS)
#define GiVmlaqLaneFloat32LowHalf(a, b, v, lane) \
    sse_vmlaq_lane_f32(a, b, sse_vget_low_f32(v), lane)
#else
GI_FORCEINLINE GI_FLOAT32_t __naive_gi_vmlaq_lane_f32_low_half(
        GI_FLOAT32_t a, GI_FLOAT32_t b, GI_FLOAT32_t v, const int lane) {
    GI_FLOAT32_t ret;
    for (size_t i = 0; i < GI_SIMD_LEN_BYTE / sizeof(float); i++) {
        ret[i] = a[i] + (b[i] * v[lane]);
    }
    return ret;
}
#define GiVmlaqLaneFloat32LowHalf(a, b, v, lane) \
    __naive_gi_vmlaq_lane_f32_low_half(a, b, v, lane)
#endif

GI_FORCEINLINE
void GiStoreFloat32(float* Buffer, GI_FLOAT32_t Vector) {
#if defined(GI_NEON_INTRINSICS)
    vst1q_f32(Buffer, Vector);
#elif defined(GI_SSE2_INTRINSICS)
    _mm_storeu_ps(Buffer, Vector);
#else
    for (size_t i = 0; i < GI_SIMD_LEN_BYTE / sizeof(float); i++) {
        Buffer[i] = Vector[i];
    }
#endif
}

GI_FORCEINLINE
void GiStoreFloat32V2(float* Buffer, GI_FLOAT32_V2_t Vector) {
#if defined(GI_NEON_INTRINSICS)
    vst1q_f32_x2(Buffer, Vector);
#else
    GiStoreFloat32(Buffer, Vector.val[0]);
    GiStoreFloat32(Buffer + GI_SIMD_LEN_BYTE / sizeof(float), Vector.val[1]);
#endif
}

#if defined(GI_NEON_INTRINSICS)
#define GISTORELANEFLOAT32(i)                                                         \
    GI_FORCEINLINE void GiStoreLane##i##Float32(float* Buffer, GI_FLOAT32_t Vector) { \
        vst1q_lane_f32(Buffer, Vector, i);                                            \
    }

#elif defined(GI_SSE2_INTRINSICS)

#define GISTORELANEFLOAT32(i)                                                          \
    GI_FORCEINLINE void GiStoreLane##i##Float32(float* Buffer, GI_FLOAT32_t Vector) {  \
        _mm_store_ss(Buffer, _mm_shuffle_ps(Vector, Vector, _MM_SHUFFLE(i, i, i, i))); \
    }
#else
#define GISTORELANEFLOAT32(i)                                                         \
    GI_FORCEINLINE void GiStoreLane##i##Float32(float* Buffer, GI_FLOAT32_t Vector) { \
        *Buffer = Vector[i];                                                          \
    }
#endif

GISTORELANEFLOAT32(0)
GISTORELANEFLOAT32(1)
GISTORELANEFLOAT32(2)
GISTORELANEFLOAT32(3)

#undef GISTORELANEFLOAT32

#if defined(GI_NEON_INTRINSICS)
#define GIEXTRACTLANEFLOAT32(i)                                           \
    GI_FORCEINLINE float GiExtractLane##i##Float32(GI_FLOAT32_t Vector) { \
        return vgetq_lane_f32(Vector, i);                                 \
    }
#elif defined(GI_SSE2_INTRINSICS)

#define GIEXTRACTLANEFLOAT32(i)                                                        \
    GI_FORCEINLINE float GiExtractLane##i##Float32(GI_FLOAT32_t Vector) {              \
        return _mm_cvtss_f32(_mm_shuffle_ps(Vector, Vector, _MM_SHUFFLE(i, i, i, i))); \
    }
#else
#define GIEXTRACTLANEFLOAT32(i)                                           \
    GI_FORCEINLINE float GiExtractLane##i##Float32(GI_FLOAT32_t Vector) { \
        return Vector[i];                                                 \
    }
#endif

GIEXTRACTLANEFLOAT32(0)
GIEXTRACTLANEFLOAT32(1)
GIEXTRACTLANEFLOAT32(2)
GIEXTRACTLANEFLOAT32(3)
#undef GIEXTRACTLANEFLOAT32

GI_FORCEINLINE
GI_FLOAT32_V2_t GiZipqFloat32(GI_FLOAT32_t Vector1, GI_FLOAT32_t Vector2) {
#if defined(GI_NEON_INTRINSICS)
    return vzipq_f32(Vector1, Vector2);
#elif defined(GI_SSE2_INTRINSICS)
    GI_FLOAT32_V2_t f32x4;
    f32x4.val[0] = _mm_unpacklo_ps(Vector1, Vector2);
    f32x4.val[1] = _mm_unpackhi_ps(Vector1, Vector2);
    return f32x4;
#else
    GI_FLOAT32_V2_t ret;
    ret.val[0][0] = Vector1[0];
    ret.val[0][1] = Vector2[0];
    ret.val[0][2] = Vector1[1];
    ret.val[0][3] = Vector2[1];
    ret.val[1][0] = Vector1[2];
    ret.val[1][1] = Vector2[2];
    ret.val[1][2] = Vector1[3];
    ret.val[1][3] = Vector2[3];
    return ret;
#endif
}

GI_FORCEINLINE
void GiStoreZipFloat32V2(float* Buffer, GI_FLOAT32_V2_t Vector) {
#if defined(GI_NEON_INTRINSICS)
    vst2q_f32(Buffer, Vector);
#else
    GI_FLOAT32_V2_t tmp;
    tmp = GiZipqFloat32(Vector.val[0], Vector.val[1]);
    GiStoreFloat32(Buffer, tmp.val[0]);
    GiStoreFloat32(Buffer + GI_SIMD_LEN_BYTE / sizeof(float), tmp.val[1]);
#endif
}

GI_FORCEINLINE
GI_FLOAT32_t GiInterleaveLowFloat32(GI_FLOAT32_t Vector1, GI_FLOAT32_t Vector2) {
#if defined(GI_NEON64_INTRINSICS)
    return vzip1q_f32(Vector1, Vector2);
#elif defined(GI_NEON32_INTRINSICS)
    float32x4x2_t zipped = vzipq_f32(Vector1, Vector2);
    return zipped.val[0];
#elif defined(GI_SSE2_INTRINSICS)
    return _mm_unpacklo_ps(Vector1, Vector2);
#else
    GI_FLOAT32_t ret;
    for (size_t i = 0; i < GI_SIMD_LEN_BYTE / 2 / sizeof(float); i++) {
        ret[2 * i] = Vector1[i];
        ret[2 * i + 1] = Vector2[i];
    }
    return ret;
#endif
}

GI_FORCEINLINE
GI_FLOAT32_t GiInterleaveHighFloat32(GI_FLOAT32_t Vector1, GI_FLOAT32_t Vector2) {
#if defined(GI_NEON64_INTRINSICS)
    return vzip2q_f32(Vector1, Vector2);
#elif defined(GI_NEON32_INTRINSICS)
    float32x4x2_t zipped = vzipq_f32(Vector1, Vector2);
    return zipped.val[1];
#elif defined(GI_SSE2_INTRINSICS)
    return _mm_unpackhi_ps(Vector1, Vector2);
#else
    GI_FLOAT32_t ret;
    for (size_t i = 0; i < GI_SIMD_LEN_BYTE / 2 / sizeof(float); i++) {
        ret[2 * i] = Vector1[GI_SIMD_LEN_BYTE / 2 / sizeof(float) + i];
        ret[2 * i + 1] = Vector2[GI_SIMD_LEN_BYTE / 2 / sizeof(float) + i];
    }
    return ret;
#endif
}

GI_FORCEINLINE
GI_FLOAT32_t GiAddFloat32(GI_FLOAT32_t Vector1, GI_FLOAT32_t Vector2) {
#if defined(GI_NEON_INTRINSICS)
    return vaddq_f32(Vector1, Vector2);
#elif defined(GI_SSE2_INTRINSICS)
    return _mm_add_ps(Vector1, Vector2);
#else
    return Vector1 + Vector2;
#endif
}

GI_FORCEINLINE
GI_FLOAT32_t GiSubtractFloat32(GI_FLOAT32_t Vector1, GI_FLOAT32_t Vector2) {
#if defined(GI_NEON_INTRINSICS)
    return vsubq_f32(Vector1, Vector2);
#elif defined(GI_SSE2_INTRINSICS)
    return _mm_sub_ps(Vector1, Vector2);
#else
    return Vector1 - Vector2;
#endif
}

GI_FORCEINLINE
GI_FLOAT32_t GiMultiplyFloat32(GI_FLOAT32_t Vector1, GI_FLOAT32_t Vector2) {
#if defined(GI_NEON_INTRINSICS)
    return vmulq_f32(Vector1, Vector2);
#elif defined(GI_SSE2_INTRINSICS)
    return _mm_mul_ps(Vector1, Vector2);
#else
    return Vector1 * Vector2;
#endif
}

GI_FORCEINLINE
GI_FLOAT32_t GiMultiplyScalerFloat32(GI_FLOAT32_t Vector1, float Scaler) {
#if defined(GI_NEON_INTRINSICS)
    return vmulq_n_f32(Vector1, Scaler);
#elif defined(GI_SSE2_INTRINSICS)
    GI_FLOAT32_t Vector2 = _mm_set1_ps(Scaler);
    return _mm_mul_ps(Vector1, Vector2);
#else
    return Vector1 * Scaler;
#endif
}

GI_FORCEINLINE
GI_FLOAT32_t GiMultiplyAddFloat32(
        GI_FLOAT32_t VectorSum, GI_FLOAT32_t Vector1, GI_FLOAT32_t Vector2) {
#if defined(GI_NEON_INTRINSICS)
    return v_fma_ps_f32(VectorSum, Vector1, Vector2);
#elif defined(GI_FMA3_INTRINSICS)
    return _mm_fmadd_ps(Vector1, Vector2, VectorSum);
#elif defined(GI_SSE2_INTRINSICS)
    return _mm_add_ps(_mm_mul_ps(Vector1, Vector2), VectorSum);
#else
    return Vector1 * Vector2 + VectorSum;
#endif
}

GI_FORCEINLINE
GI_FLOAT32_t GiMultiplyAddScalarFloat32(
        GI_FLOAT32_t VectorSum, GI_FLOAT32_t Vector, float Scalar) {
#if defined(GI_NEON_INTRINSICS)
    return v_fma_n_f32(VectorSum, Vector, Scalar);
#elif defined(GI_SSE2_INTRINSICS)
    return GiMultiplyAddFloat32(VectorSum, GiBroadcastFloat32(Scalar), Vector);
#else
    return VectorSum + Vector * Scalar;
#endif
}

#if defined(GI_NEON_INTRINSICS)
#define GIMULTIPLYADDLANFLOAT32(i)                                                \
    GI_FORCEINLINE GI_FLOAT32_t GiMultiplyAddLan##i##Float32(                     \
            GI_FLOAT32_t VectorSum, GI_FLOAT32_t Vector1, GI_FLOAT32_t Vector2) { \
        return v_fma_lane_f32(VectorSum, Vector1, vget_low_f32(Vector2), i);      \
    }
GIMULTIPLYADDLANFLOAT32(0)
GIMULTIPLYADDLANFLOAT32(1)
#undef GIMULTIPLYADDLANFLOAT32
#define GIMULTIPLYADDLANFLOAT32(i)                                                \
    GI_FORCEINLINE GI_FLOAT32_t GiMultiplyAddLan##i##Float32(                     \
            GI_FLOAT32_t VectorSum, GI_FLOAT32_t Vector1, GI_FLOAT32_t Vector2) { \
        return v_fma_lane_f32(VectorSum, Vector1, vget_high_f32(Vector2), i - 2); \
    }
GIMULTIPLYADDLANFLOAT32(2)
GIMULTIPLYADDLANFLOAT32(3)
#undef GIMULTIPLYADDLANFLOAT32
#else

#define GIMULTIPLYADDLANFLOAT32(i)                                                \
    GI_FORCEINLINE GI_FLOAT32_t GiMultiplyAddLan##i##Float32(                     \
            GI_FLOAT32_t VectorSum, GI_FLOAT32_t Vector1, GI_FLOAT32_t Vector2) { \
        return GiMultiplyAddScalarFloat32(                                        \
                VectorSum, Vector1, GiExtractLane##i##Float32(Vector2));          \
    }
GIMULTIPLYADDLANFLOAT32(0)
GIMULTIPLYADDLANFLOAT32(1)
GIMULTIPLYADDLANFLOAT32(2)
GIMULTIPLYADDLANFLOAT32(3)
#undef GIMULTIPLYADDLANFLOAT32
#endif

GI_FORCEINLINE
GI_FLOAT32_t GiDivideFloat32(GI_FLOAT32_t Vector1, GI_FLOAT32_t Vector2) {
#if defined(GI_NEON64_INTRINSICS)
    return vdivq_f32(Vector1, Vector2);
#elif defined(GI_NEON32_INTRINSICS)
    float32x4_t recp = vrecpeq_f32(Vector2);
    recp = vmulq_f32(vrecpsq_f32(Vector2, recp), recp);
    return vmulq_f32(Vector1, recp);
#elif defined(GI_SSE2_INTRINSICS)
    return _mm_div_ps(Vector1, Vector2);
#else
    return Vector1 / Vector2;
#endif
}

GI_FORCEINLINE
GI_FLOAT32_t GiRecpeSFloat32(GI_FLOAT32_t Vector1, GI_FLOAT32_t Vector2) {
#if defined(GI_NEON64_INTRINSICS)
    return vrecpsq_f32(Vector1, Vector2);
#elif defined(GI_SSE2_INTRINSICS)
    GI_FLOAT32_t two = _mm_set1_ps(2.0f);
    return _mm_sub_ps(two, _mm_mul_ps(Vector1, Vector2));
#else
    return (2.0f - Vector1 * Vector2);
#endif
}

GI_FORCEINLINE
GI_FLOAT32_t GiRecpeFloat32(GI_FLOAT32_t Vector) {
#if defined(GI_NEON32_INTRINSICS)
    return vrecpeq_f32(Vector);
#elif defined(GI_SSE2_INTRINSICS)
    GI_FLOAT32_t ones = _mm_set1_ps(1.0f);
    return _mm_div_ps(ones, Vector);
#else
    //! FIXME: neon or sse always have low accuracy than 1/x
    return 1 / Vector;
#endif
}

GI_FORCEINLINE
GI_FLOAT32_t GiNegFloat32(GI_FLOAT32_t Vector) {
#if defined(GI_NEON32_INTRINSICS)
    return vnegq_f32(Vector);
#elif defined(GI_SSE2_INTRINSICS)
    GI_FLOAT32_t zero = _mm_set1_ps(0.0f);
    return _mm_sub_ps(zero, Vector);
#else
    return -Vector;
#endif
}

GI_FORCEINLINE
GI_UINT32_t GiGreaterThanFloat32(GI_FLOAT32_t Vector1, GI_FLOAT32_t Vector2) {
#if defined(GI_NEON_INTRINSICS)
    return vcgtq_f32(Vector1, Vector2);
#elif defined(GI_SSE2_INTRINSICS)
    return _mm_castps_si128(_mm_cmpgt_ps(Vector1, Vector2));
#else
    GI_UINT32_t ret;
    for (size_t i = 0; i < GI_SIMD_LEN_BYTE / sizeof(float); i++) {
        ret[i] = Vector1[i] > Vector2[i] ? 0xFFFFFFFF : 0;
    }
    return ret;
#endif
}

GI_FORCEINLINE
GI_UINT32_t GiLessThanEqFloat32(GI_FLOAT32_t Vector1, GI_FLOAT32_t Vector2) {
#if defined(GI_NEON_INTRINSICS)
    return vcleq_f32(Vector1, Vector2);
#elif defined(GI_SSE2_INTRINSICS)
    return _mm_castps_si128(_mm_cmple_ps(Vector1, Vector2));
#else
    GI_UINT32_t ret;
    for (size_t i = 0; i < GI_SIMD_LEN_BYTE / sizeof(float); i++) {
        ret[i] = Vector1[i] <= Vector2[i] ? 0xFFFFFFFF : 0;
    }
    return ret;
#endif
}

GI_FORCEINLINE
GI_UINT32_t GiLessThanFloat32(GI_FLOAT32_t Vector1, GI_FLOAT32_t Vector2) {
#if defined(GI_NEON_INTRINSICS)
    return vcltq_f32(Vector1, Vector2);
#elif defined(GI_SSE2_INTRINSICS)
    return _mm_castps_si128(_mm_cmplt_ps(Vector1, Vector2));
#else
    GI_UINT32_t ret;
    for (size_t i = 0; i < GI_SIMD_LEN_BYTE / sizeof(float); i++) {
        ret[i] = Vector1[i] < Vector2[i] ? 0xFFFFFFFF : 0;
    }
    return ret;
#endif
}

GI_FORCEINLINE
GI_FLOAT32_t GiAndFloat32(GI_FLOAT32_t Vector1, GI_FLOAT32_t Vector2) {
#if defined(GI_SSE2_INTRINSICS)
    return _mm_and_ps(Vector1, Vector2);
#else
    return GiReintInt32ToFloat32(
            GiAndInt32(GiReinterpretAsInt32(Vector1), GiReinterpretAsInt32(Vector2)));
#endif
}

GI_FORCEINLINE
GI_FLOAT32_t GiOrFloat32(GI_FLOAT32_t Vector1, GI_FLOAT32_t Vector2) {
#if defined(GI_SSE2_INTRINSICS)
    return _mm_or_ps(Vector1, Vector2);
#else
    return GiReintInt32ToFloat32(
            GiOrInt32(GiReinterpretAsInt32(Vector1), GiReinterpretAsInt32(Vector2)));
#endif
}

GI_FORCEINLINE
GI_FLOAT32_t GiAndNotFloat32(GI_FLOAT32_t VectorNot, GI_FLOAT32_t Vector) {
#if defined(GI_SSE2_INTRINSICS)
    return _mm_andnot_ps(VectorNot, Vector);
#else
    return GiReintInt32ToFloat32(GiAndNotInt32(
            GiReinterpretAsInt32(VectorNot), GiReinterpretAsInt32(Vector)));
#endif
}

GI_FORCEINLINE
GI_FLOAT32_t GiXorFloat32(GI_FLOAT32_t Vector1, GI_FLOAT32_t Vector2) {
#if defined(GI_SSE2_INTRINSICS)
    return _mm_xor_ps(Vector1, Vector2);
#else
    return GiReintInt32ToFloat32(
            GiXorInt32(GiReinterpretAsInt32(Vector1), GiReinterpretAsInt32(Vector2)));
#endif
}

GI_FORCEINLINE
GI_FLOAT32_t GiBlendFloat32(
        GI_FLOAT32_t Vector1, GI_FLOAT32_t Vector2, GI_FLOAT32_t Selection) {
    return GiOrFloat32(
            GiAndFloat32(Vector1, Selection), GiAndNotFloat32(Selection, Vector2));
}

#define MIN_NAN(a, b) (isnan(a) || (a) < (b)) ? (a) : (b);
#define MAX_NAN(a, b) (isnan(a) || (a) > (b)) ? (a) : (b);

GI_FORCEINLINE
GI_FLOAT32_t GiBSLFloat32(
        GI_UINT32_t Selection, GI_FLOAT32_t Vector1, GI_FLOAT32_t Vector2) {
#if defined(GI_NEON_INTRINSICS)
    return vbslq_f32(Selection, Vector1, Vector2);
#else
    return GiBlendFloat32(Vector1, Vector2, GiReintUint32ToFloat32(Selection));
#endif
}

GI_FORCEINLINE
GI_FLOAT32_t GiMaximumFloat32(GI_FLOAT32_t Vector1, GI_FLOAT32_t Vector2) {
#if defined(GI_NEON_INTRINSICS)
    return vmaxq_f32(Vector1, Vector2);
#elif defined(GI_NEON32_INTRINSICS)
    return _mm_max_ps(Vector1, Vector2);
#else
    GI_FLOAT32_t max;
    for (size_t i = 0; i < GI_SIMD_LEN_BYTE / sizeof(float); i++) {
        max[i] = Max(Vector1[i], Vector2[i]);
    }
    return max;
#endif
}

GI_FORCEINLINE
GI_FLOAT32_t GiMinimumFloat32(GI_FLOAT32_t Vector1, GI_FLOAT32_t Vector2) {
#if defined(GI_NEON_INTRINSICS)
    return vminq_f32(Vector1, Vector2);
#elif defined(GI_NEON32_INTRINSICS)
    return _mm_min_ps(Vector1, Vector2);
#else
    GI_FLOAT32_t min;
    for (size_t i = 0; i < GI_SIMD_LEN_BYTE / sizeof(float); i++) {
        min[i] = Min(Vector1[i], Vector2[i]);
    }
    return min;
#endif
}

GI_FORCEINLINE
GI_FLOAT32_t GiMaxNanFloat32(GI_FLOAT32_t Vector1, GI_FLOAT32_t Vector2) {
#if defined(GI_NEON_INTRINSICS)
    return vmaxq_f32(Vector1, Vector2);
#else
    //! _mm_max_ps does not fellow the IEEE standard when input is NAN, so
    //! implement by C code
    GI_FLOAT32_t max;
    for (size_t i = 0; i < GI_SIMD_LEN_BYTE / sizeof(float); i++) {
        max[i] = MAX_NAN(Vector1[i], Vector2[i]);
    }
    return max;
#endif
}

GI_FORCEINLINE
GI_FLOAT32_t GiMinNanFloat32(GI_FLOAT32_t Vector1, GI_FLOAT32_t Vector2) {
#if defined(GI_NEON_INTRINSICS)
    return vminq_f32(Vector1, Vector2);
#else
    //! _mm_min_ps does not fellow the IEEE standard when input is NAN, so
    //! implement by C code
    GI_FLOAT32_t min;
    for (size_t i = 0; i < GI_SIMD_LEN_BYTE / sizeof(float); i++) {
        min[i] = MIN_NAN(Vector1[i], Vector2[i]);
    }
    return min;
#endif
}

GI_FORCEINLINE
GI_FLOAT32_t GiClampFloat32(GI_FLOAT32_t Value, float LowerRange, float UpperRange) {
    Value = GiMaximumFloat32(GiBroadcastFloat32(LowerRange), Value);
    Value = GiMinimumFloat32(GiBroadcastFloat32(UpperRange), Value);
    return Value;
}

GI_FORCEINLINE
float GiReduceAddFloat32(GI_FLOAT32_t Vector) {
#if defined(GI_NEON64_INTRINSICS)
    Vector = vpaddq_f32(Vector, Vector);
    Vector = vpaddq_f32(Vector, Vector);
    return vgetq_lane_f32(Vector, 0);
#elif defined(GI_NEON32_INTRINSICS)
    float32x2_t VectorLow = vget_low_f32(Vector);
    float32x2_t VectorHigh = vget_high_f32(Vector);
    VectorLow = vpadd_f32(VectorLow, VectorHigh);
    VectorLow = vpadd_f32(VectorLow, VectorHigh);
    return vget_lane_f32(VectorLow, 0);
#elif defined(GI_SSE2_INTRINSICS)
    Vector = GiAddFloat32(
            Vector, _mm_shuffle_ps(Vector, Vector, _MM_SHUFFLE(2, 3, 2, 3)));
    Vector = GiAddFloat32(
            Vector, _mm_shuffle_ps(Vector, Vector, _MM_SHUFFLE(1, 1, 1, 1)));
    return GiExtractLane0Float32(Vector);
#else
    float ret = 0;
    for (size_t i = 0; i < GI_SIMD_LEN_BYTE / sizeof(float); i++) {
        ret += Vector[i];
    }
    return ret;
#endif
}

GI_FORCEINLINE
float GiReduceMultiplyFloat32(GI_FLOAT32_t Vector) {
#if defined(GI_NEON64_INTRINSICS)
    float32x2_t low = vget_low_f32(Vector);
    float32x2_t high = vget_high_f32(Vector);
    float32x2_t res = vmul_f32(low, high);
    return vget_lane_f32(res, 0) * vget_lane_f32(res, 1);
#elif defined(GI_SSE2_INTRINSICS)
    Vector = GiMultiplyFloat32(
            Vector, _mm_shuffle_ps(Vector, Vector, _MM_SHUFFLE(2, 3, 2, 3)));
    Vector = GiMultiplyFloat32(
            Vector, _mm_shuffle_ps(Vector, Vector, _MM_SHUFFLE(1, 1, 1, 1)));
    return GiExtractLane0Float32(Vector);
#else
    float ret = 1;
    for (size_t i = 0; i < GI_SIMD_LEN_BYTE / sizeof(float); i++) {
        ret *= Vector[i];
    }
    return ret;
#endif
}

#define Max(a, b) (a) > (b) ? (a) : (b)
#define Min(a, b) (a) < (b) ? (a) : (b)

GI_FORCEINLINE
float GiReduceMaxNanFloat32(GI_FLOAT32_t Vector) {
#if defined(GI_NEON64_INTRINSICS)
    return vmaxvq_f32(Vector);
#elif defined(GI_NEON32_INTRINSICS)
    float32x2_t VectorLow = vget_low_f32(Vector);
    float32x2_t VectorHigh = vget_high_f32(Vector);
    VectorLow = vpmax_f32(VectorLow, VectorHigh);
    VectorLow = vpmax_f32(VectorLow, VectorHigh);
    return vget_lane_f32(VectorLow, 0);
#elif defined(GI_SSE2_INTRINSICS)
    Vector = GiMaxNanFloat32(
            Vector, _mm_shuffle_ps(Vector, Vector, _MM_SHUFFLE(2, 3, 2, 3)));
    Vector = GiMaxNanFloat32(
            Vector, _mm_shuffle_ps(Vector, Vector, _MM_SHUFFLE(1, 1, 1, 1)));
    return GiExtractLane0Float32(Vector);
#else
    float ret = Vector[0];
    for (size_t i = 1; i < GI_SIMD_LEN_BYTE / sizeof(float); i++) {
        ret = MAX_NAN(ret, Vector[i]);
    }
    return ret;
#endif
}

GI_FORCEINLINE
float GiReduceMinNanFloat32(GI_FLOAT32_t Vector) {
#if defined(GI_NEON64_INTRINSICS)
    return vminvq_f32(Vector);
#elif defined(GI_NEON32_INTRINSICS)
    float32x2_t VectorLow = vget_low_f32(Vector);
    float32x2_t VectorHigh = vget_high_f32(Vector);
    VectorLow = vpmin_f32(VectorLow, VectorHigh);
    VectorLow = vpmin_f32(VectorLow, VectorHigh);
    return vget_lane_f32(VectorLow, 0);
#elif defined(GI_SSE2_INTRINSICS)
    Vector = GiMinNanFloat32(
            Vector, _mm_shuffle_ps(Vector, Vector, _MM_SHUFFLE(2, 3, 2, 3)));
    Vector = GiMinNanFloat32(
            Vector, _mm_shuffle_ps(Vector, Vector, _MM_SHUFFLE(1, 1, 1, 1)));
    return GiExtractLane0Float32(Vector);
#else
    float ret = Vector[0];
    for (size_t i = 1; i < GI_SIMD_LEN_BYTE / sizeof(float); i++) {
        ret = MIN_NAN(ret, Vector[i]);
    }
    return ret;
#endif
}

GI_FORCEINLINE
GI_FLOAT32_t GiAbsFloat32(GI_FLOAT32_t Vector1) {
#if defined(GI_NEON64_INTRINSICS)
    return vabsq_f32(Vector1);
#elif defined(GI_SSE2_INTRINSICS)
    union {
        unsigned int int_val;
        float float_val;
    } value;
    value.int_val = 0x7fffffff;
    return _mm_and_ps(Vector1, _mm_set_ps1(value.float_val));
#else
    GI_FLOAT32_t ret;
    for (size_t i = 0; i < GI_SIMD_LEN_BYTE / sizeof(float); i++) {
        ret[i] = Vector1[i] > 0 ? Vector1[i] : -Vector1[i];
    }
    return ret;
#endif
}

#if defined(GI_SSE2_INTRINSICS)
typedef __m128i int8x16_t;
typedef __m64_128 int8x8_t;
GI_FORCEINLINE int8x16_t vcombine_s8(int8x8_t low, int8x8_t high) {
    return _mm_unpacklo_epi64(_pM128i(low), _pM128i(high));
}

typedef __m64_128 int64x1_t;
GI_FORCEINLINE int64x1_t vget_low_s64(GI_INT64_t a) {
    int64x1_t res64;
    return64(a);
}
GI_FORCEINLINE int64x1_t vget_high_s64(GI_INT64_t a) {
    int64x1_t res64;
    __m128i res;
    res = _mm_unpackhi_epi64(a, a);
    return64(res);
}
#endif

GI_FORCEINLINE GI_INT64_t GiZip1qS64(GI_INT64_t __p0, GI_INT64_t __p1) {
#if defined(GI_NEON_INTRINSICS)
    return vzip1q_s64(__p0, __p1);
#elif defined(GI_SSE2_INTRINSICS)
#define vcombine_s64 vcombine_s8
    return vcombine_s64(vget_low_s64(__p0), vget_low_s64(__p1));
#else
    GI_INT64_t ret;
    ret[0] = __p0[0];
    ret[1] = __p1[0];
    return ret;
#endif
}

GI_FORCEINLINE GI_INT64_t GiZip2qS64(GI_INT64_t __p0, GI_INT64_t __p1) {
#if defined(GI_NEON_INTRINSICS)
    return vzip2q_s64(__p0, __p1);
#elif defined(GI_SSE2_INTRINSICS)
#define vcombine_s64 vcombine_s8
    return vcombine_s64(vget_high_s64(__p0), vget_high_s64(__p1));
#else
    GI_INT64_t ret;
    ret[0] = __p0[1];
    ret[1] = __p1[1];
    return ret;
#endif
}

GI_FORCEINLINE GI_FLOAT32_t GiReinterpretqS64ToFloat32(GI_INT64_t a) {
#if defined(GI_NEON_INTRINSICS)
    return vreinterpretq_f32_s64(a);
#elif defined(GI_SSE2_INTRINSICS)
    return _M128(a);
#else
    GI_FLOAT32_t ret;
    memcpy(&ret, &a, sizeof(GI_FLOAT32_t));
    return ret;
#endif
}

GI_FORCEINLINE GI_INT64_t GiReinterpretqFloat32ToS64(GI_FLOAT32_t a) {
#if defined(GI_NEON_INTRINSICS)
    return vreinterpretq_s64_f32(a);
#elif defined(GI_SSE2_INTRINSICS)
    return _M128i(a);
#else
    GI_INT64_t ret;
    memcpy(&ret, &a, sizeof(GI_INT64_t));
    return ret;
#endif
}

#if defined(GI_NEON_INTRINSICS)
#define GiSimdFmaLane(a, b, c, d) vfmaq_laneq_f32(a, b, c, d)
#else
GI_FORCEINLINE GI_FLOAT32_t
___gi_vmlaq_lane_f32(GI_FLOAT32_t a, GI_FLOAT32_t b, float32x2_t v, int l) {
    float vlane;
    GI_FLOAT32_t c;
    vlane = (float)GiGetLaneFloat32(v, l);
    c = GiBroadcastFloat32(vlane);
    return GiMlaqFloat32(a, b, c);
}
GI_FORCEINLINE float32x2_t ___gi_vget_low_f32(GI_FLOAT32_t a) {
#if defined(GI_SSE2_INTRINSICS)
    float32x2_t res64;
    _M64f(res64, a);
    return res64;
#else
    float32x2_t ret;
    ret[0] = a[0];
    ret[1] = a[1];
    return ret;
#endif
}
GI_FORCEINLINE float32x2_t ___gi_vget_high_f32(GI_FLOAT32_t a) {
#if defined(GI_SSE2_INTRINSICS)
    __m128i res;
    __m64_128 res64;
    res = _mm_unpackhi_epi64(_M128i(a), _M128i(a));
    return64(res);
#else
    float32x2_t ret;
    ret[0] = a[2];
    ret[1] = a[3];
    return ret;
#endif
}
GI_FORCEINLINE GI_FLOAT32_t
___gi_vfmaq_laneq_f32(GI_FLOAT32_t a, GI_FLOAT32_t b, GI_FLOAT32_t v, int l) {
    if (l < 2) {
        return ___gi_vmlaq_lane_f32(a, b, ___gi_vget_low_f32(v), l);
    } else {
        return ___gi_vmlaq_lane_f32(a, b, ___gi_vget_high_f32(v), l - 2);
    }
}
#define GiSimdFmaLane(a, b, c, d) ___gi_vfmaq_laneq_f32(a, b, c, d)
#endif

#if defined(GI_NEON_INTRINSICS)
#if MEGDNN_AARCH64
#define GiMlaqLowLaneFloat32(__a, __b, __v, __lane) \
    vmlaq_laneq_f32(__a, __b, __v, __lane)

#define GiMlaqHighLaneFloat32(__a, __b, __v, __lane) \
    vmlaq_laneq_f32(__a, __b, __v, __lane)

#else
#define GiMlaqLowLaneFloat32(__a, __b, __v, __lane)               \
    __extension__({                                               \
        float32x2_t c = vget_low_f32(__v);                        \
        GI_FLOAT32_t __ret = vmlaq_lane_f32(__a, __b, c, __lane); \
        __ret;                                                    \
    })

#define GiMlaqHighLaneFloat32(__a, __b, __v, __lane)                    \
    __extension__({                                                     \
        float32x2_t c = vget_high_f32(__v);                             \
        GI_FLOAT32_t __ret = vmlaq_lane_f32(__a, __b, c, (__lane - 2)); \
        __ret;                                                          \
    })

#endif

#elif defined(GI_SSE2_INTRINSICS)
#define GiMlaqLowLaneFloat32(__a, __b, __v, __lane)                   \
    __extension__({                                                   \
        float32x2_t c = sse_vget_low_f32(__v);                        \
        GI_FLOAT32_t __ret = sse_vmlaq_lane_f32(__a, __b, c, __lane); \
        __ret;                                                        \
    })

#define GiMlaqHighLaneFloat32(__a, __b, __v, __lane)                        \
    __extension__({                                                         \
        float32x2_t c = sse_vget_high_f32(__v);                             \
        GI_FLOAT32_t __ret = sse_vmlaq_lane_f32(__a, __b, c, (__lane - 2)); \
        __ret;                                                              \
    })

#else
//! naive
#define GiMlaqLowLaneFloat32(__a, __b, __v, __lane)                     \
    __extension__({                                                     \
        GI_FLOAT32_t __ret;                                             \
        for (size_t i = 0; i < GI_SIMD_LEN_BYTE / sizeof(float); i++) { \
            __ret[i] = __a[i] + (__b[i] * __v[__lane]);                 \
        }                                                               \
        __ret;                                                          \
    })

#define GiMlaqHighLaneFloat32(__a, __b, __v, __lane)                    \
    __extension__({                                                     \
        GI_FLOAT32_t __ret;                                             \
        for (size_t i = 0; i < GI_SIMD_LEN_BYTE / sizeof(float); i++) { \
            __ret[i] = __a[i] + (__b[i] * __v[__lane]);                 \
        }                                                               \
        __ret;                                                          \
    })
#endif

#if defined(GI_NEON_INTRINSICS)
#define GiFmsqLaneQFloat32(a, b, v, lane) vfmsq_laneq_f32(a, b, v, lane)
#elif defined(GI_SSE2_INTRINSICS)
#define SSE_VFMSQ_LANEQ_F32(lane)                                   \
    GI_FORCEINLINE GI_FLOAT32_t sse_vfmsq_lane_##lane##_q_f32(      \
            GI_FLOAT32_t a, GI_FLOAT32_t b, GI_FLOAT32_t v) {       \
        return sse_vmlsq_lane_f32(a, b, sse_vget_low_f32(v), lane); \
    }
SSE_VFMSQ_LANEQ_F32(0)
SSE_VFMSQ_LANEQ_F32(1)
#undef SSE_VFMSQ_LANEQ_F32
#define SSE_VFMSQ_LANEQ_F32(lane)                                        \
    GI_FORCEINLINE GI_FLOAT32_t sse_vfmsq_lane_##lane##_q_f32(           \
            GI_FLOAT32_t a, GI_FLOAT32_t b, GI_FLOAT32_t v) {            \
        return sse_vmlsq_lane_f32(a, b, sse_vget_high_f32(v), lane - 2); \
    }
SSE_VFMSQ_LANEQ_F32(2)
SSE_VFMSQ_LANEQ_F32(3)
#undef SSE_VFMSQ_LANEQ_F32
#define GiFmsqLaneQFloat32(a, b, v, lane) sse_vfmsq_lane_##lane##_q_f32(a, b, v)
#else
//! naive
GI_FORCEINLINE GI_FLOAT32_t __naive_GiFmsqLaneQFloat32(
        GI_FLOAT32_t a, GI_FLOAT32_t b, GI_FLOAT32_t v, const int lane) {
    GI_FLOAT32_t ret;
    for (size_t i = 0; i < GI_SIMD_LEN_BYTE / sizeof(float); i++) {
        ret[i] = a[i] - (b[i] * v[lane]);
    }

    return ret;
}
#define GiFmsqLaneQFloat32(a, b, v, lane) __naive_GiFmsqLaneQFloat32(a, b, v, lane)
#endif

GI_FORCEINLINE GI_FLOAT32_t GiCombineFloat32(float32x2_t a, float32x2_t b) {
#if defined(GI_NEON_INTRINSICS)
    return vcombine_f32(a, b);
#elif defined(GI_SSE2_INTRINSICS)
    __m128i res;
    res = _mm_unpacklo_epi64(_pM128i(a), _pM128i(b));
    return _M128(res);
#else
    GI_FLOAT32_t res;
    res[0] = a[0];
    res[1] = a[1];
    res[2] = b[0];
    res[3] = b[1];
    return res;
#endif
}

GI_FORCEINLINE float32x2_t GiGetLowFloat32(GI_FLOAT32_t a) {
#if defined(GI_NEON_INTRINSICS)
    return vget_low_f32(a);
#else
    return ___gi_vget_low_f32(a);
#endif
}

GI_FORCEINLINE float32x2_t GiGetHighFloat32(GI_FLOAT32_t a) {
#if defined(GI_NEON_INTRINSICS)
    return vget_high_f32(a);
#else
    return ___gi_vget_high_f32(a);
#endif
}

GI_FORCEINLINE float32x2_t GiPaddFloat32(float32x2_t a, float32x2_t b) {
#if defined(GI_NEON_INTRINSICS)
    return vpadd_f32(a, b);
#elif defined(GI_SSE2_INTRINSICS)
    float32x2_t res;
    res.m64_f32[0] = a.m64_f32[0] + a.m64_f32[1];
    res.m64_f32[1] = b.m64_f32[0] + b.m64_f32[1];
    return res;
#else
    float32x2_t res;
    res[0] = a[0] + a[1];
    res[1] = b[0] + b[1];
    return res;
#endif
}

GI_FORCEINLINE float32x2_t GiPmaxFloat32(float32x2_t a, float32x2_t b) {
#if defined(GI_NEON_INTRINSICS)
    return vpmax_f32(a, b);
#elif defined(GI_SSE2_INTRINSICS)
    float32x2_t res;
    res.m64_f32[0] = MAX_NAN(a.m64_f32[0], a.m64_f32[1]);
    res.m64_f32[1] = MAX_NAN(b.m64_f32[0], b.m64_f32[1]);
    return res;
#else
    float32x2_t res;
    res[0] = MAX_NAN(a[0], a[1]);
    res[1] = MAX_NAN(b[0], b[1]);
    return res;
#endif
}

GI_FORCEINLINE
GI_FLOAT32_V3_t GiLoadUzipFloat32V3(const float* ptr) {
#if defined(GI_NEON_INTRINSICS)
    return vld3q_f32(ptr);
#elif defined(GI_SSE2_INTRINSICS)
    GI_FLOAT32_V3_t v;
    __m128 tmp0, tmp1, tmp2, tmp3;
    v.val[0] = GiLoadFloat32(ptr);
    v.val[1] = GiLoadFloat32((ptr + 4));
    v.val[2] = GiLoadFloat32((ptr + 8));

    tmp0 = _mm_castsi128_ps(_mm_shuffle_epi32(
            _mm_castps_si128(v.val[0]), 0 | (3 << 2) | (1 << 4) | (2 << 6)));
    tmp1 = _mm_castsi128_ps(
            _mm_shuffle_epi32(_mm_castps_si128(v.val[1]), _SWAP_HI_LOW32));
    tmp2 = _mm_castsi128_ps(_mm_shuffle_epi32(
            _mm_castps_si128(v.val[2]), 1 | (2 << 2) | (0 << 4) | (3 << 6)));
    tmp3 = _mm_unpacklo_ps(tmp1, tmp2);

    v.val[0] = _mm_movelh_ps(tmp0, tmp3);
    tmp0 = _mm_unpackhi_ps(tmp0, tmp1);
    v.val[1] =
            _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(tmp0), _SWAP_HI_LOW32));
    v.val[1] = _mm_movehl_ps(tmp3, v.val[1]);
    v.val[2] = _mm_movehl_ps(tmp2, tmp0);
    return v;
#else
    GI_FLOAT32_V3_t ret;
    for (size_t i = 0; i < 3; i++) {
        ret.val[i][0] = ptr[0 + i];
        ret.val[i][1] = ptr[3 + i];
        ret.val[i][2] = ptr[6 + i];
        ret.val[i][3] = ptr[9 + i];
    }

    return ret;
#endif
}

GI_FORCEINLINE
void GiStoreZipFloat32V3(float* ptr, GI_FLOAT32_V3_t val) {
#if defined(GI_NEON_INTRINSICS)
    vst3q_f32(ptr, val);
#elif defined(GI_SSE2_INTRINSICS)
    GI_FLOAT32_V3_t v;
    __m128 tmp0, tmp1, tmp2;
    tmp0 = _mm_unpacklo_ps(val.val[0], val.val[1]);
    tmp1 = _mm_unpackhi_ps(val.val[0], val.val[1]);
    tmp2 = _mm_unpacklo_ps(val.val[1], val.val[2]);
    v.val[1] = _mm_shuffle_ps(tmp2, tmp1, _MM_SHUFFLE(1, 0, 3, 2));
    v.val[2] = _mm_movehl_ps(val.val[2], tmp1);
    v.val[2] = _mm_shuffle_ps(v.val[2], v.val[2], _MM_SHUFFLE(3, 1, 0, 2));
    tmp1 = _mm_unpacklo_ps(tmp2, val.val[0]);
    v.val[0] = _mm_shuffle_ps(tmp0, tmp1, _MM_SHUFFLE(3, 2, 1, 0));

    GiStoreFloat32(ptr, v.val[0]);
    GiStoreFloat32((ptr + 4), v.val[1]);
    GiStoreFloat32((ptr + 8), v.val[2]);
#else
    for (size_t i = 0; i < GI_SIMD_LEN_BYTE / sizeof(float); i++) {
        *ptr++ = val.val[0][i];
        *ptr++ = val.val[1][i];
        *ptr++ = val.val[2][i];
    }
#endif
}
