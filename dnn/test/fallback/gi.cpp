#include <vector>
#include "test/fallback/fixture.h"

#include "src/fallback/general_intrinsic/gi_float.h"
#include "src/fallback/general_intrinsic/gi_int.h"

namespace megdnn {
namespace test {

#define SIMD_LEN    GI_SIMD_LEN_BYTE / sizeof(float)
#define SIMD_LEN_16 GI_SIMD_LEN_BYTE / sizeof(int16_t)
#define SIMD_LEN_8  GI_SIMD_LEN_BYTE / sizeof(int8_t)
template <typename T>
static void init(
        T* dst, const std::vector<T>& value, const size_t simd_len = SIMD_LEN) {
    for (size_t i = 0; i < simd_len; i++) {
        dst[i] = value[i];
    }
}

template <typename T>
static void assert_eq(T* a, const std::vector<T>& b, const size_t simd_len = SIMD_LEN) {
    for (size_t i = 0; i < simd_len; i++) {
        ASSERT_EQ(a[i], b[i]);
    }
}

template <typename T>
static void assert_eq_and_nan(
        T* a, const std::vector<T>& b, const size_t simd_len = SIMD_LEN) {
    for (size_t i = 0; i < simd_len; i++) {
        if (isnan(a[i]) && isnan(b[i])) {
            continue;
        }
        ASSERT_EQ(a[i], b[i]);
    }
}

static void assert_lt(
        float* a, const std::vector<float>& b, const float eps,
        const size_t simd_len = SIMD_LEN) {
    for (size_t i = 0; i < simd_len; i++) {
        ASSERT_LT(std::abs(a[i] - b[i]), eps);
    }
}

TEST_F(FALLBACK, GiGetSimdType) {
    auto t = GiGetSimdType();
    auto should_type = GI_UNKNOWN;
#if defined(GI_AVX_INTRINSICS) || defined(GI_AVX2_INTRINSICS) || \
        defined(GI_FMA_INTRINSICS)
    should_type = GI_AVX;
#elif defined(GI_NEON_INTRINSICS)
    should_type = GI_NEON;
#elif defined(GI_SSE2_INTRINSICS) || defined(GI_SSE42_INTRINSICS)

#if defined(GI_SSE42_INTRINSICS)
    should_type = GI_SSE42;
#elif defined(GI_SSE2_INTRINSICS)
    should_type = GI_SSE2;
#else
    should_type = GI_UNKNOWN;
#error "code issue happened!!"
#endif

#else
    should_type = GI_NAIVE;
#endif

    printf("test GiGetSimdType: %d, should_type: %d\n", t, should_type);

    ASSERT_EQ(t, should_type);
}

TEST_F(FALLBACK, GiAndInt32) {
    GI_INT32_t src0, src1, ret;
    std::vector<int32_t> s0{1, 2, 3, 4};
    s0.resize(SIMD_LEN);
    std::vector<int32_t> s1{5, 6, 7, 8};
    s1.resize(SIMD_LEN);
    init((int32_t*)&src0, s0);
    init((int32_t*)&src1, s1);

    ret = GiAndInt32(src0, src1);

    std::vector<int32_t> naive;
    for (size_t i = 0; i < SIMD_LEN; i++) {
        naive.push_back(s0[i] & s1[i]);
    }

    assert_eq((int32_t*)&ret, naive);
}

TEST_F(FALLBACK, GiOrInt32) {
    GI_INT32_t src0, src1, ret;
    std::vector<int32_t> s0{1, 2, 3, 4};
    s0.resize(SIMD_LEN);
    std::vector<int32_t> s1{5, 6, 7, 8};
    s1.resize(SIMD_LEN);
    init((int32_t*)&src0, s0);
    init((int32_t*)&src1, s1);

    ret = GiOrInt32(src0, src1);

    std::vector<int32_t> naive;
    for (size_t i = 0; i < SIMD_LEN; i++) {
        naive.push_back(s0[i] | s1[i]);
    }

    assert_eq((int*)&ret, naive);
}

TEST_F(FALLBACK, GiAndNotInt32) {
    GI_INT32_t src0, src1, ret;
    std::vector<int32_t> s0{1, 2, 3, 4};
    s0.resize(SIMD_LEN);
    std::vector<int32_t> s1{5, 6, 7, 8};
    s1.resize(SIMD_LEN);
    init((int32_t*)&src0, s0);
    init((int32_t*)&src1, s1);

    ret = GiAndNotInt32(src0, src1);

    std::vector<int32_t> naive;
    for (size_t i = 0; i < SIMD_LEN; i++) {
        naive.push_back(~s0[i] & s1[i]);
    }

    assert_eq((int32_t*)&ret, naive);
}

TEST_F(FALLBACK, GiXorInt32) {
    GI_INT32_t src0, src1, ret;
    std::vector<int32_t> s0{1, 2, 3, 4};
    s0.resize(SIMD_LEN);
    std::vector<int32_t> s1{5, 6, 7, 8};
    s1.resize(SIMD_LEN);
    init((int32_t*)&src0, s0);
    init((int32_t*)&src1, s1);

    ret = GiXorInt32(src0, src1);

    std::vector<int32_t> naive;
    for (size_t i = 0; i < SIMD_LEN; i++) {
        naive.push_back(s0[i] ^ s1[i]);
    }

    assert_eq((int32_t*)&ret, naive);
}

TEST_F(FALLBACK, GiBroadcastFloat32) {
    GI_FLOAT32_t ret;
    float b = 2022.0420;

    ret = GiBroadcastFloat32(b);

    std::vector<float> naive;
    for (size_t i = 0; i < SIMD_LEN; i++) {
        naive.push_back(b);
    }

    assert_eq((float*)&ret, naive);
}

TEST_F(FALLBACK, GiBroadcastInt32) {
    GI_INT32_t ret;
    int32_t b = 20220420;

    ret = GiBroadcastInt32(b);

    std::vector<int32_t> naive;
    for (size_t i = 0; i < SIMD_LEN; i++) {
        naive.push_back(b);
    }

    assert_eq((int32_t*)&ret, naive);
}

TEST_F(FALLBACK, GiReinterpretAsInt32) {
    GI_INT32_t ret;
    GI_FLOAT32_t src0;
    std::vector<float> s0{1.0f, 2.2f, 3.4f, 4.5f};
    s0.resize(SIMD_LEN);
    init((float*)&src0, s0);

    ret = GiReinterpretAsInt32(src0);

    std::vector<int32_t> naive;
    for (size_t i = 0; i < SIMD_LEN; i++) {
        int32_t tmp;
        memcpy(&tmp, &s0[i], sizeof(int32_t));
        naive.push_back(tmp);
    }

    assert_eq((int32_t*)&ret, naive);
}

TEST_F(FALLBACK, GiReinterpretAsUint32) {
    GI_UINT32_t ret;
    GI_FLOAT32_t src0;
    std::vector<float> s0{1.0f, 2.2f, 3.4f, 4.5f};
    s0.resize(SIMD_LEN);
    init((float*)&src0, s0);

    ret = GiReinterpretAsUint32(src0);

    std::vector<uint32_t> naive;
    for (size_t i = 0; i < SIMD_LEN; i++) {
        uint32_t tmp;
        memcpy(&tmp, &s0[i], sizeof(uint32_t));
        naive.push_back(tmp);
    }

    assert_eq((uint32_t*)&ret, naive);
}

TEST_F(FALLBACK, GiReintInt32ToFloat32) {
    GI_FLOAT32_t ret;
    GI_INT32_t src0;
    std::vector<int32_t> s0{1, 2, 3, 4};
    s0.resize(SIMD_LEN);
    init((int32_t*)&src0, s0);

    ret = GiReintInt32ToFloat32(src0);

    std::vector<float> naive;
    for (size_t i = 0; i < SIMD_LEN; i++) {
        float tmp;
        memcpy(&tmp, &s0[i], sizeof(float));
        naive.push_back(tmp);
    }

    assert_eq((float*)&ret, naive);
}

TEST_F(FALLBACK, GiReintUint32ToFloat32) {
    GI_FLOAT32_t ret;
    GI_UINT32_t src0;
    std::vector<uint32_t> s0{1, 2, 3, 4};
    s0.resize(SIMD_LEN);
    init((uint32_t*)&src0, s0);

    ret = GiReintUint32ToFloat32(src0);

    std::vector<float> naive;
    for (size_t i = 0; i < SIMD_LEN; i++) {
        float tmp;
        memcpy(&tmp, &s0[i], sizeof(float));
        naive.push_back(tmp);
    }

    assert_eq((float*)&ret, naive);
}

TEST_F(FALLBACK, GiRoundAsInt32) {
    GI_FLOAT32_t src0;
    GI_INT32_t ret;
    std::vector<float> s0{1.1f, 2.2f, 3.5f, 4.9f};
    s0.resize(SIMD_LEN);
    init((float*)&src0, s0);

    ret = GiRoundAsInt32(src0);

    std::vector<int32_t> naive;
    for (size_t i = 0; i < SIMD_LEN; i++) {
        naive.push_back((int32_t)round(s0[i]));
    }

    assert_eq((int*)&ret, naive);
}

TEST_F(FALLBACK, GiCastToInt32) {
    GI_FLOAT32_t src0;
    GI_INT32_t ret;
    std::vector<float> s0{1.1f, 2.2f, 3.5f, 4.9f};
    s0.resize(SIMD_LEN);
    init((float*)&src0, s0);

    ret = GiCastToInt32(src0);

    std::vector<int32_t> naive;
    for (size_t i = 0; i < SIMD_LEN; i++) {
        naive.push_back((int32_t)(s0[i]));
    }

    assert_eq((int*)&ret, naive);
}

TEST_F(FALLBACK, GiCastToFloat32) {
    GI_INT32_t src0;
    GI_FLOAT32_t ret;
    std::vector<int32_t> s0{100, 200, 300, 400};
    s0.resize(SIMD_LEN);
    init((int32_t*)&src0, s0);

    ret = GiCastToFloat32(src0);

    std::vector<float> naive;
    for (size_t i = 0; i < SIMD_LEN; i++) {
        naive.push_back((float)s0[i]);
    }

    assert_eq((float*)&ret, naive);
}

TEST_F(FALLBACK, GiLoadBroadcastFloat32) {
    GI_FLOAT32_t ret;
    float p = 2022.0420;

    ret = GiLoadBroadcastFloat32(&p);

    std::vector<float> naive;
    for (size_t i = 0; i < SIMD_LEN; i++) {
        naive.push_back(p);
    }

    assert_eq((float*)&ret, naive);
}

TEST_F(FALLBACK, GiZeroFloat32) {
    GI_FLOAT32_t ret;
    memset(&ret, 'f', sizeof(GI_FLOAT32_t));
    float p = 0;

    ret = GiZeroFloat32();

    std::vector<float> naive;
    for (size_t i = 0; i < SIMD_LEN; i++) {
        naive.push_back(p);
    }

    assert_eq((float*)&ret, naive);
}

TEST_F(FALLBACK, GiLoadFloat32) {
    GI_FLOAT32_t ret;
    std::vector<float> s0{2.3f, 4.7f, -1.4f, 1223.6f};
    s0.resize(SIMD_LEN);

    ret = GiLoadFloat32(s0.data());

    std::vector<float> naive;
    for (size_t i = 0; i < SIMD_LEN; i++) {
        naive.push_back(s0[i]);
    }

    assert_eq((float*)&ret, naive);
}

TEST_F(FALLBACK, GiLoadFloat32V2) {
    GI_FLOAT32_V2_t ret;
    std::vector<float> s0{2.3f, 4.7f, -1.4f, 1223.6f, 1.1f, 4.0f, 99.7f, 1234.9f};
    s0.resize(SIMD_LEN * 2);

    ret = GiLoadFloat32V2(s0.data());

    std::vector<float> naive;
    for (size_t i = 0; i < SIMD_LEN * 2; i++) {
        naive.push_back(s0[i]);
    }

    assert_eq((float*)&ret, naive, SIMD_LEN * 2);
}

TEST_F(FALLBACK, GiLoadFloat32LowHalf) {
    GI_FLOAT32_t ret;
    std::vector<float> s0{2.3f, 4.7f, -1.4f, 1223.6f};
    s0.resize(SIMD_LEN);

    ret = GiLoadFloat32LowHalf(s0.data());

    std::vector<float> naive;
    for (size_t i = 0; i < SIMD_LEN; i++) {
        if (i < SIMD_LEN / 2) {
            naive.push_back(s0[i]);
        } else {
            naive.push_back(0);
        }
    }

    assert_eq((float*)&ret, naive);
}

TEST_F(FALLBACK, GiMlaqFloat32) {
    GI_FLOAT32_t src0, src1, src2, ret;
    std::vector<float> s0{1.1f, 2.2f, 3.5f, 4.9f};
    std::vector<float> s1{2312.1f, 345.244f, 3.59f, -12.8f};
    std::vector<float> s2{1.2f, -3.1f, 9.0f, 11.2f};
    s0.resize(SIMD_LEN);
    s1.resize(SIMD_LEN);
    s2.resize(SIMD_LEN);
    init((float*)&src0, s0);
    init((float*)&src1, s1);
    init((float*)&src2, s2);

    ret = GiMlaqFloat32(src0, src1, src2);

    std::vector<float> naive;
    for (size_t i = 0; i < SIMD_LEN; i++) {
        naive.push_back(s0[i] + (s1[i] * s2[i]));
    }

    assert_eq((float*)&ret, naive);
}

TEST_F(FALLBACK, GiUzpqFloat32) {
    GI_FLOAT32_t src0, src1;
    GI_FLOAT32_V2_t ret;
    std::vector<float> s0{1.1f, 2.2f, 3.5f, 4.9f};
    std::vector<float> s1{2312.1f, 345.244f, 3.59f, -12.8f};
    s0.resize(SIMD_LEN);
    s1.resize(SIMD_LEN);
    init((float*)&src0, s0);
    init((float*)&src1, s1);

    ret = GiUzpqFloat32(src0, src1);

    std::vector<float> naive0;
    std::vector<float> naive1;
    naive0.push_back(s0[0]);
    naive0.push_back(s0[2]);
    naive0.push_back(s1[0]);
    naive0.push_back(s1[2]);
    naive1.push_back(s0[1]);
    naive1.push_back(s0[3]);
    naive1.push_back(s1[1]);
    naive1.push_back(s1[3]);

    assert_eq((float*)&ret, naive0);
    assert_eq((float*)&ret + SIMD_LEN, naive1);
}

TEST_F(FALLBACK, GiDupFloat32) {
    float32x2_t ret;
    float t = 3.1415;

    ret = GiDupFloat32(t);

    auto r = (float*)&ret;
    ASSERT_EQ(*r, t);
    ASSERT_EQ(*(r + 1), t);
}

TEST_F(FALLBACK, GiLdFloat32) {
    float32x2_t ret;
    std::vector<float> s0{1.1f, -3.1415f};

    ret = GiLdFloat32(s0.data());

    auto r = (float*)&ret;
    ASSERT_EQ(*r, s0[0]);
    ASSERT_EQ(*(r + 1), s0[1]);
}

TEST_F(FALLBACK, GiAddDFloat32) {
    float32x2_t src0, src1, ret;
    std::vector<float> s0{1.1f, -3.1415f};
    std::vector<float> s1{2.3f, 3.14777f};
    memcpy(&src0, s0.data(), sizeof(float32x2_t));
    memcpy(&src1, s1.data(), sizeof(float32x2_t));

    ret = GiAddDFloat32(src0, src1);

    auto r = (float*)&ret;

    auto naive0 = s0[0] + s1[0];
    auto naive1 = s0[1] + s1[1];
    ASSERT_EQ(*r, naive0);
    ASSERT_EQ(*(r + 1), naive1);
}

TEST_F(FALLBACK, GiGetLaneFloat32) {
    float32x2_t src0;
    std::vector<float> s0{1.1f, -3.1415f};
    memcpy(&src0, s0.data(), sizeof(float32x2_t));

    auto ret = GiGetLaneFloat32(src0, 0);
    ASSERT_EQ(ret, s0[0]);

    ret = GiGetLaneFloat32(src0, 1);
    ASSERT_EQ(ret, s0[1]);
}

TEST_F(FALLBACK, GiSetLaneFloat32) {
    float32x2_t src0, ret;
    std::vector<float> s0{2.1f, -3.1415f};
    memcpy(&src0, s0.data(), sizeof(float32x2_t));
    float p = 2022.0420;

    auto r = (float*)&ret;
    ret = GiSetLaneFloat32(p, src0, 0);
    ASSERT_EQ(*r, p);
    ASSERT_EQ(*(r + 1), s0[1]);

    ret = GiSetLaneFloat32(p, src0, 1);
    ASSERT_EQ(*r, s0[0]);
    ASSERT_EQ(*(r + 1), p);
}

TEST_F(FALLBACK, GiSt1Float32) {
    float32x2_t src0;
    std::vector<float> s0{2.1f, -3.1415f};
    memcpy(&src0, s0.data(), sizeof(float32x2_t));

    std::vector<float> ret{0, 0};
    GiSt1Float32(ret.data(), src0);
    ASSERT_EQ(ret[0], s0[0]);
    ASSERT_EQ(ret[1], s0[1]);
}

TEST_F(FALLBACK, GiLd2qFloat32) {
    GI_FLOAT32_V2_t ret;
    std::vector<float> s0{1.1f, 2.2f, 3.5f, 4.9f, 2312.1f, 345.244f, 3.59f, -12.8f};

    ret = GiLd2qFloat32(s0.data());

    std::vector<float> naive0;
    std::vector<float> naive1;
    naive0.push_back(s0[0]);
    naive0.push_back(s0[2]);
    naive0.push_back(s0[4]);
    naive0.push_back(s0[6]);
    naive1.push_back(s0[1]);
    naive1.push_back(s0[3]);
    naive1.push_back(s0[5]);
    naive1.push_back(s0[7]);

    assert_eq((float*)&ret, naive0);
    assert_eq((float*)&ret + SIMD_LEN, naive1);
}

TEST_F(FALLBACK, GiExtqFloat32) {
    GI_FLOAT32_t src0, src1, ret;
    std::vector<float> s0{1.1f, 2.2f, 3.5f, 4.9f};
    std::vector<float> s1{-9.1f, 34234.6f, 9.0f, 34.1f};
    s0.resize(SIMD_LEN);
    s1.resize(SIMD_LEN);
    init((float*)&src0, s0);
    init((float*)&src1, s1);
    std::vector<float> naive = {0, 0, 0, 0};

    auto compare = [&](const size_t n) {
        size_t t_count = SIMD_LEN;
        size_t a_count = t_count - n;
        for (size_t i = 0; i < a_count; i++) {
            naive[i] = s0[i + n];
        }
        for (size_t i = 0; i < n; i++) {
            naive[i + a_count] = s1[i];
        }
        assert_eq((float*)&ret, naive);
    };

#define CB(n)                           \
    ret = GiExtqFloat32(src0, src1, n); \
    compare(n);

    CB(0)
    CB(1)
    CB(2)
    CB(3)
#undef CB
}

TEST_F(FALLBACK, GiMultiplySubFloat32) {
    GI_FLOAT32_t src0, src1, src2, ret;
    std::vector<float> s0{1.1f, 2.2f, 3.5f, 4.9f};
    std::vector<float> s1{-9.1f, 34234.6f, 9.0f, 34.1f};
    std::vector<float> s2{0.4f, 9.9f, 4.3f, 6.2f};
    s0.resize(SIMD_LEN);
    s1.resize(SIMD_LEN);
    s2.resize(SIMD_LEN);
    init((float*)&src0, s0);
    init((float*)&src1, s1);
    init((float*)&src2, s2);

    ret = GiMultiplySubFloat32(src0, src1, src2);
    std::vector<float> naive;
    for (size_t i = 0; i < SIMD_LEN; i++) {
        naive.push_back(s0[i] - (s1[i] * s2[i]));
    }

    assert_eq((float*)&ret, naive);
}

TEST_F(FALLBACK, GiLd1qLaneFloat32) {
    GI_FLOAT32_t src0, ret;
    std::vector<float> s0{1.1f, 2.2f, 3.5f, 4.9f};
    s0.resize(SIMD_LEN);
    init((float*)&src0, s0);
    std::vector<float> naive = {0, 0, 0, 0};

    float buffer = 3.14159;

    auto compare = [&](const size_t n) {
        memcpy(naive.data(), s0.data(), sizeof(GI_FLOAT32_t));
        naive[n] = buffer;
        assert_eq((float*)&ret, naive);
    };

#define CB(n)                                  \
    ret = GiLd1qLaneFloat32(&buffer, src0, n); \
    compare(n);

    CB(0)
    CB(1)
    CB(2)
    CB(3)
#undef CB
}

TEST_F(FALLBACK, GiSetqLaneFloat32) {
    GI_FLOAT32_t src0, ret;
    std::vector<float> s0{2.1f, 6.2f, -9.5f, 2.9f};
    s0.resize(SIMD_LEN);
    init((float*)&src0, s0);
    std::vector<float> naive = {0, 0, 0, 0};

    float buffer = 6.14159;

    auto compare = [&](const size_t n) {
        memcpy(naive.data(), s0.data(), sizeof(GI_FLOAT32_t));
        naive[n] = buffer;
        assert_eq((float*)&ret, naive);
    };

#define CB(n)                                 \
    ret = GiSetqLaneFloat32(buffer, src0, n); \
    compare(n);

    CB(0)
    CB(1)
    CB(2)
    CB(3)
#undef CB
}

TEST_F(FALLBACK, GiMlaqLaneFloat32HighHalf) {
    GI_FLOAT32_t src0, src1, src2, ret;
    std::vector<float> s0{1.1f, 2.2f, 3.5f, 4.9f};
    std::vector<float> s1{-9.1f, 34234.6f, 9.0f, 34.1f};
    std::vector<float> s2{0.4f, 9.9f, 4.3f, 6.2f};
    s0.resize(SIMD_LEN);
    s1.resize(SIMD_LEN);
    s2.resize(SIMD_LEN);
    init((float*)&src0, s0);
    init((float*)&src1, s1);
    init((float*)&src2, s2);
    std::vector<float> naive = {0, 0, 0, 0};

    auto compare = [&](const size_t n) {
        for (size_t i = 0; i < GI_SIMD_LEN_BYTE / sizeof(float); i++) {
            naive[i] = s0[i] + (s1[i] * s2[n + 2]);
        }
        assert_eq((float*)&ret, naive);
    };

#define CB(n)                                             \
    ret = GiMlaqLaneFloat32HighHalf(src0, src1, src2, n); \
    compare(n);

    CB(0)
    CB(1)
#undef CB
}

TEST_F(FALLBACK, GiVmlaqLaneFloat32LowHalf) {
    GI_FLOAT32_t src0, src1, src2, ret;
    std::vector<float> s0{1.1f, 2.2f, 3.5f, 4.9f};
    std::vector<float> s1{-9.1f, 34234.6f, 9.0f, 34.1f};
    std::vector<float> s2{0.4f, 9.9f, 4.3f, 6.2f};
    s0.resize(SIMD_LEN);
    s1.resize(SIMD_LEN);
    s2.resize(SIMD_LEN);
    init((float*)&src0, s0);
    init((float*)&src1, s1);
    init((float*)&src2, s2);
    std::vector<float> naive = {0, 0, 0, 0};

    auto compare = [&](const size_t n) {
        for (size_t i = 0; i < GI_SIMD_LEN_BYTE / sizeof(float); i++) {
            naive[i] = s0[i] + (s1[i] * s2[n]);
        }
        assert_eq((float*)&ret, naive);
    };

#define CB(n)                                             \
    ret = GiVmlaqLaneFloat32LowHalf(src0, src1, src2, n); \
    compare(n);

    CB(0)
    CB(1)
#undef CB
}

TEST_F(FALLBACK, GiStoreFloat32) {
    GI_FLOAT32_t src0;
    std::vector<float> s0{1.1f, 2.2f, 3.5f, 4.9f};
    s0.resize(SIMD_LEN);
    init((float*)&src0, s0);
    std::vector<float> ret{0};
    ret.resize(SIMD_LEN);

    GiStoreFloat32(ret.data(), src0);
    assert_eq(ret.data(), s0);
}

TEST_F(FALLBACK, GiStoreFloat32V2) {
    GI_FLOAT32_V2_t src0;
    std::vector<float> s0{1.1f, 2.2f, 3.5f, 4.9f, -1.1f, -2.2f, -3.5f, -4.9};
    s0.resize(SIMD_LEN * 2);
    init((float*)&src0, s0, SIMD_LEN * 2);
    std::vector<float> ret{0};
    ret.resize(SIMD_LEN * 2);

    GiStoreFloat32V2(ret.data(), src0);
    assert_eq(ret.data(), s0, SIMD_LEN * 2);
}

TEST_F(FALLBACK, GiStoreLaneXXFloat32) {
    GI_FLOAT32_t src0;
    std::vector<float> s0{1.1f, 2.2f, 3.5f, 4.9f};
    s0.resize(SIMD_LEN);
    init((float*)&src0, s0);
    float ret{0};

#define CB(n)                            \
    GiStoreLane##n##Float32(&ret, src0); \
    ASSERT_EQ(ret, s0[n]);

    CB(0)
    CB(1)
    CB(2)
    CB(3)
#undef CB
}

TEST_F(FALLBACK, GiExtractLaneXXFloat32) {
    GI_FLOAT32_t src0;
    std::vector<float> s0{1.1f, 2.2f, 3.5f, 4.9f};
    s0.resize(SIMD_LEN);
    init((float*)&src0, s0);
    float ret{0};

#define CB(n)                              \
    ret = GiExtractLane##n##Float32(src0); \
    ASSERT_EQ(ret, s0[n]);

    CB(0)
    CB(1)
    CB(2)
    CB(3)
#undef CB
}

TEST_F(FALLBACK, GiZipqFloat32) {
    GI_FLOAT32_t src0, src1;
    GI_FLOAT32_V2_t ret;
    std::vector<float> s0{1.1f, 2.2f, 3.5f, 4.9f};
    std::vector<float> s1{2312.1f, 345.244f, 3.59f, -12.8f};
    s0.resize(SIMD_LEN);
    s1.resize(SIMD_LEN);
    init((float*)&src0, s0);
    init((float*)&src1, s1);

    ret = GiZipqFloat32(src0, src1);

    std::vector<float> naive0;
    std::vector<float> naive1;
    naive0.push_back(s0[0]);
    naive0.push_back(s1[0]);
    naive0.push_back(s0[1]);
    naive0.push_back(s1[1]);
    naive1.push_back(s0[2]);
    naive1.push_back(s1[2]);
    naive1.push_back(s0[3]);
    naive1.push_back(s1[3]);

    assert_eq((float*)&ret, naive0);
    assert_eq((float*)&ret + SIMD_LEN, naive1);
}

TEST_F(FALLBACK, GiInterleaveLowFloat32) {
    GI_FLOAT32_t src0, src1, ret;
    std::vector<float> s0{1.1f, 2.2f, 3.5f, 4.9f};
    std::vector<float> s1{2312.1f, 345.244f, 3.59f, -12.8f};
    s0.resize(SIMD_LEN);
    s1.resize(SIMD_LEN);
    init((float*)&src0, s0);
    init((float*)&src1, s1);

    ret = GiInterleaveLowFloat32(src0, src1);

    std::vector<float> naive;
    naive.resize(SIMD_LEN);

    for (size_t i = 0; i < SIMD_LEN / 2; i++) {
        naive[2 * i] = s0[i];
        naive[2 * i + 1] = s1[i];
    }
    assert_eq((float*)&ret, naive);
}

TEST_F(FALLBACK, GiInterleaveHighFloat32) {
    GI_FLOAT32_t src0, src1, ret;
    std::vector<float> s0{1.1f, 2.2f, 3.5f, 4.9f};
    std::vector<float> s1{2312.1f, 345.244f, 3.59f, -12.8f};
    s0.resize(SIMD_LEN);
    s1.resize(SIMD_LEN);
    init((float*)&src0, s0);
    init((float*)&src1, s1);

    ret = GiInterleaveHighFloat32(src0, src1);

    std::vector<float> naive;
    naive.resize(SIMD_LEN);

    for (size_t i = 0; i < SIMD_LEN / 2; i++) {
        naive[2 * i] = s0[i + SIMD_LEN / 2];
        naive[2 * i + 1] = s1[i + SIMD_LEN / 2];
    }
    assert_eq((float*)&ret, naive);
}

TEST_F(FALLBACK, GiAddFloat32) {
    GI_FLOAT32_t src0, src1, ret;
    std::vector<float> s0{1.1f, 2.2f, 3.5f, 4.9f};
    std::vector<float> s1{2312.1f, 345.244f, 3.59f, -12.8f};
    s0.resize(SIMD_LEN);
    s1.resize(SIMD_LEN);
    init((float*)&src0, s0);
    init((float*)&src1, s1);

    ret = GiAddFloat32(src0, src1);

    std::vector<float> naive;

    for (size_t i = 0; i < SIMD_LEN; i++) {
        naive.push_back(s0[i] + s1[i]);
    }

    assert_eq((float*)&ret, naive);
}

TEST_F(FALLBACK, GiSubtractFloat32) {
    GI_FLOAT32_t src0, src1, ret;
    std::vector<float> s0{1.1f, 2.2f, 3.5f, 4.9f};
    std::vector<float> s1{2312.1f, 345.244f, 3.59f, -12.8f};
    s0.resize(SIMD_LEN);
    s1.resize(SIMD_LEN);
    init((float*)&src0, s0);
    init((float*)&src1, s1);

    ret = GiSubtractFloat32(src0, src1);

    std::vector<float> naive;

    for (size_t i = 0; i < SIMD_LEN; i++) {
        naive.push_back(s0[i] - s1[i]);
    }

    assert_eq((float*)&ret, naive);
}

TEST_F(FALLBACK, GiMultiplyFloat32) {
    GI_FLOAT32_t src0, src1, ret;
    std::vector<float> s0{1.1f, 2.2f, 3.5f, 4.9f};
    std::vector<float> s1{2312.1f, 345.244f, 3.59f, -12.8f};
    s0.resize(SIMD_LEN);
    s1.resize(SIMD_LEN);
    init((float*)&src0, s0);
    init((float*)&src1, s1);

    ret = GiMultiplyFloat32(src0, src1);

    std::vector<float> naive;

    for (size_t i = 0; i < SIMD_LEN; i++) {
        naive.push_back(s0[i] * s1[i]);
    }

    assert_eq((float*)&ret, naive);
}

TEST_F(FALLBACK, GiMultiplyScalerFloat32) {
    GI_FLOAT32_t src0, ret;
    std::vector<float> s0{1.1f, 2.2f, 3.5f, 4.9f};
    s0.resize(SIMD_LEN);
    init((float*)&src0, s0);

    float scalar = 3.1415;

    ret = GiMultiplyScalerFloat32(src0, scalar);

    std::vector<float> naive;

    for (size_t i = 0; i < SIMD_LEN; i++) {
        naive.push_back(s0[i] * scalar);
    }

    assert_eq((float*)&ret, naive);
}

TEST_F(FALLBACK, GiMultiplyAddFloat32) {
    GI_FLOAT32_t src0, src1, src2, ret;
    std::vector<float> s0{1.1f, 2.2f, 3.5f, 4.9f};
    std::vector<float> s1{2312.1f, 345.244f, 3.59f, -12.8f};
    std::vector<float> s2{12.1f, 35.244f, 23.59f, -112.8f};
    s0.resize(SIMD_LEN);
    s1.resize(SIMD_LEN);
    s2.resize(SIMD_LEN);
    init((float*)&src0, s0);
    init((float*)&src1, s1);
    init((float*)&src2, s2);

    ret = GiMultiplyAddFloat32(src0, src1, src2);

    std::vector<float> naive;

    for (size_t i = 0; i < SIMD_LEN; i++) {
        naive.push_back(s1[i] * s2[i] + s0[i]);
    }

    assert_lt((float*)&ret, naive, 1e-3);
}

TEST_F(FALLBACK, GiMultiplyAddScalarFloat32) {
    GI_FLOAT32_t src0, src1, ret;
    std::vector<float> s0{1.1f, 2.2f, 3.5f, 4.9f};
    std::vector<float> s1{2312.1f, 345.244f, 3.59f, -12.8f};
    s0.resize(SIMD_LEN);
    s1.resize(SIMD_LEN);
    init((float*)&src0, s0);
    init((float*)&src1, s1);

    float scalar = 3.1415;

    ret = GiMultiplyAddScalarFloat32(src0, src1, scalar);

    std::vector<float> naive;

    for (size_t i = 0; i < SIMD_LEN; i++) {
        naive.push_back(s1[i] * scalar + s0[i]);
    }

    assert_eq((float*)&ret, naive);
}

TEST_F(FALLBACK, GiMultiplyAddLanXXFloat32) {
    GI_FLOAT32_t src0, src1, src2, ret;
    std::vector<float> s0{1.1f, 2.2f, 3.5f, 4.9f};
    std::vector<float> s1{2312.1f, 345.244f, 3.59f, -12.8f};
    std::vector<float> s2{12.1f, 35.244f, 23.59f, -112.8f};
    s0.resize(SIMD_LEN);
    s1.resize(SIMD_LEN);
    s2.resize(SIMD_LEN);
    init((float*)&src0, s0);
    init((float*)&src1, s1);
    init((float*)&src2, s2);

    std::vector<float> naive = {0, 0, 0, 0};

    auto compare = [&](const size_t n) {
        for (size_t i = 0; i < GI_SIMD_LEN_BYTE / sizeof(float); i++) {
            naive[i] = s0[i] + (s1[i] * s2[n]);
        }
        assert_eq((float*)&ret, naive);
    };

#define CB(n)                                             \
    ret = GiMultiplyAddLan##n##Float32(src0, src1, src2); \
    compare(n);

    CB(0)
    CB(1)
    CB(2)
    CB(3)
#undef CB
}

TEST_F(FALLBACK, GiDivideFloat32) {
    GI_FLOAT32_t src0, src1, ret;
    std::vector<float> s0{1.1f, 2.2f, 3.5f, 4.9f};
    std::vector<float> s1{2312.1f, 345.244f, 3.59f, -12.8f};
    s0.resize(SIMD_LEN);
    s1.resize(SIMD_LEN);
    init((float*)&src0, s0);
    init((float*)&src1, s1);

    ret = GiDivideFloat32(src0, src1);

    std::vector<float> naive;

    for (size_t i = 0; i < SIMD_LEN; i++) {
        naive.push_back(s0[i] / s1[i]);
    }

    assert_lt((float*)&ret, naive, 1e-3);
}

TEST_F(FALLBACK, GiRecpeSFloat32) {
    GI_FLOAT32_t src0, src1, ret;
    std::vector<float> s0{1.1f, 2.2f, 3.5f, 4.9f};
    std::vector<float> s1{2312.1f, 345.244f, 3.59f, -12.8f};
    s0.resize(SIMD_LEN);
    s1.resize(SIMD_LEN);
    init((float*)&src0, s0);
    init((float*)&src1, s1);

    ret = GiRecpeSFloat32(src0, src1);

    std::vector<float> naive;

    for (size_t i = 0; i < SIMD_LEN; i++) {
        naive.push_back(2.0f - s0[i] * s1[i]);
    }

    assert_eq((float*)&ret, naive);
}

TEST_F(FALLBACK, GiRecpeFloat32) {
    GI_FLOAT32_t src0, ret;
    std::vector<float> s0{100.1f, 2.2f, 3.5f, 4.9f};
    s0.resize(SIMD_LEN);
    init((float*)&src0, s0);

    ret = GiRecpeFloat32(src0);

    std::vector<float> naive;

    for (size_t i = 0; i < SIMD_LEN; i++) {
        naive.push_back(1.0f / s0[i]);
    }

    assert_lt((float*)&ret, naive, 1e-3);
}

TEST_F(FALLBACK, GiNegFloat32) {
    GI_FLOAT32_t src0, ret;
    std::vector<float> s0{-1.1f, 2.2f, 3.5f, 4.9f};
    s0.resize(SIMD_LEN);
    init((float*)&src0, s0);

    ret = GiNegFloat32(src0);

    std::vector<float> naive;

    for (size_t i = 0; i < SIMD_LEN; i++) {
        naive.push_back(-s0[i]);
    }

    assert_eq((float*)&ret, naive);
}

TEST_F(FALLBACK, GiGreaterThanFloat32) {
    GI_FLOAT32_t src0, src1;
    GI_UINT32_t ret;
    std::vector<float> s0{1.1f, 2.2f, 3.5f, 4.9f};
    std::vector<float> s1{2312.1f, 0.1f, 3.59f, -12.8f};
    s0.resize(SIMD_LEN);
    s1.resize(SIMD_LEN);
    init((float*)&src0, s0);
    init((float*)&src1, s1);

    ret = GiGreaterThanFloat32(src0, src1);

    std::vector<int32_t> naive;

    for (size_t i = 0; i < SIMD_LEN; i++) {
        naive.push_back(s0[i] > s1[i] ? 0xFFFFFFFF : 0);
    }

    assert_eq((int32_t*)&ret, naive);
}

TEST_F(FALLBACK, GiLessThanEqFloat32) {
    GI_FLOAT32_t src0, src1;
    GI_UINT32_t ret;
    std::vector<float> s0{1.1f, 2.2f, 3.5f, 4.9f};
    std::vector<float> s1{2312.1f, 0.1f, 3.59f, -12.8f};
    s0.resize(SIMD_LEN);
    s1.resize(SIMD_LEN);
    init((float*)&src0, s0);
    init((float*)&src1, s1);

    ret = GiLessThanEqFloat32(src0, src1);

    std::vector<int32_t> naive;

    for (size_t i = 0; i < SIMD_LEN; i++) {
        naive.push_back(s0[i] <= s1[i] ? 0xFFFFFFFF : 0);
    }

    assert_eq((int32_t*)&ret, naive);
}

TEST_F(FALLBACK, GiLessThanFloat32) {
    GI_FLOAT32_t src0, src1;
    GI_UINT32_t ret;
    std::vector<float> s0{1.1f, 2.2f, 3.5f, 4.9f};
    std::vector<float> s1{1.1f, 0.1f, 3.59f, -12.8f};
    s0.resize(SIMD_LEN);
    s1.resize(SIMD_LEN);
    init((float*)&src0, s0);
    init((float*)&src1, s1);

    ret = GiLessThanFloat32(src0, src1);

    std::vector<int32_t> naive;

    for (size_t i = 0; i < SIMD_LEN; i++) {
        naive.push_back(s0[i] < s1[i] ? 0xFFFFFFFF : 0);
    }

    assert_eq((int32_t*)&ret, naive);
}

TEST_F(FALLBACK, GiAndFloat32) {
    GI_FLOAT32_t src0, src1, ret;
    std::vector<float> s0{1.1f, 2.2f, 3.5f, 4.9f};
    std::vector<float> s1{2312.1f, 345.244f, 3.59f, -12.8f};
    s0.resize(SIMD_LEN);
    s1.resize(SIMD_LEN);
    init((float*)&src0, s0);
    init((float*)&src1, s1);

    ret = GiAndFloat32(src0, src1);

    std::vector<float> naive;

    for (size_t i = 0; i < SIMD_LEN; i++) {
        int32_t tmp0, tmp1, tmp;
        float tmp2;
        memcpy(&tmp0, &s0[i], sizeof(int32_t));
        memcpy(&tmp1, &s1[i], sizeof(int32_t));
        tmp = tmp0 & tmp1;
        memcpy(&tmp2, &tmp, sizeof(float));
        naive.push_back(tmp2);
    }

    assert_eq((float*)&ret, naive);
}

TEST_F(FALLBACK, GiOrFloat32) {
    GI_FLOAT32_t src0, src1, ret;
    std::vector<float> s0{2, 2, 3, 4};
    std::vector<float> s1{6, 6, 7, 8};
    s0.resize(SIMD_LEN);
    s1.resize(SIMD_LEN);
    init((float*)&src0, s0);
    init((float*)&src1, s1);

    ret = GiOrFloat32(src0, src1);

    std::vector<float> naive;

    for (size_t i = 0; i < SIMD_LEN; i++) {
        int32_t tmp0, tmp1, tmp;
        float tmp2;
        memcpy(&tmp0, &s0[i], sizeof(int32_t));
        memcpy(&tmp1, &s1[i], sizeof(int32_t));
        tmp = tmp0 | tmp1;
        memcpy(&tmp2, &tmp, sizeof(float));
        naive.push_back(tmp2);
    }

    assert_eq((float*)&ret, naive);
}

TEST_F(FALLBACK, GiAndNotFloat32) {
    GI_FLOAT32_t src0, src1, ret;
    std::vector<float> s0{1.1f, 2.2f, 3.5f, 4.9f};
    std::vector<float> s1{2312.1f, 345.244f, 3.59f, -12.8f};
    s0.resize(SIMD_LEN);
    s1.resize(SIMD_LEN);
    init((float*)&src0, s0);
    init((float*)&src1, s1);

    ret = GiAndNotFloat32(src0, src1);

    std::vector<float> naive;

    for (size_t i = 0; i < SIMD_LEN; i++) {
        int32_t tmp0, tmp1, tmp;
        float tmp2;
        memcpy(&tmp0, &s0[i], sizeof(int32_t));
        memcpy(&tmp1, &s1[i], sizeof(int32_t));
        tmp = ~tmp0 & tmp1;
        memcpy(&tmp2, &tmp, sizeof(float));
        naive.push_back(tmp2);
    }

    assert_eq((float*)&ret, naive);
}

TEST_F(FALLBACK, GiXorFloat32) {
    GI_FLOAT32_t src0, src1, ret;
    std::vector<float> s0{1.1f, 2.2f, 3.5f, 4.9f};
    std::vector<float> s1{2312.1f, 345.244f, 3.59f, -12.8f};
    s0.resize(SIMD_LEN);
    s1.resize(SIMD_LEN);
    init((float*)&src0, s0);
    init((float*)&src1, s1);

    ret = GiXorFloat32(src0, src1);

    std::vector<float> naive;

    for (size_t i = 0; i < SIMD_LEN; i++) {
        int32_t tmp0, tmp1, tmp;
        float tmp2;
        memcpy(&tmp0, &s0[i], sizeof(int32_t));
        memcpy(&tmp1, &s1[i], sizeof(int32_t));
        tmp = tmp0 ^ tmp1;
        memcpy(&tmp2, &tmp, sizeof(float));
        naive.push_back(tmp2);
    }

    assert_eq((float*)&ret, naive);
}

TEST_F(FALLBACK, GiBSLFloat32) {
    GI_FLOAT32_t src0, src1, ret, na;
    GI_UINT32_t mask;
    std::vector<float> s0{1.1f, 2.2f, 4.5f, 4.9f};
    std::vector<float> s1{2312.1f, 345.244f, 3.59f, -12.8f};
    std::vector<std::vector<uint32_t>> s2s = {
            {1, 2, 3, 0},       {0u, 0u, 0u, 0u},    {~0u, 0u, 0u, 0u},
            {~0u, ~0u, 0u, 0u}, {~0u, ~0u, ~0u, 0u}, {~0u, ~0u, ~0u, ~0u}};
    s0.resize(SIMD_LEN);
    s1.resize(SIMD_LEN);
    init((float*)&src0, s0);
    init((float*)&src1, s1);

    for (auto& s2 : s2s) {
        init((uint32_t*)&mask, s2);

        ret = GiBSLFloat32(mask, src0, src1);
        na = GiBlendFloat32(src0, src1, GiReintUint32ToFloat32(mask));

        std::vector<float> naive;
        naive.resize(SIMD_LEN);
        memcpy(naive.data(), &na, sizeof(GI_FLOAT32_t));

        assert_eq_and_nan((float*)&ret, naive);
    }
}

TEST_F(FALLBACK, GiMaximumFloat32) {
    GI_FLOAT32_t src0, src1, ret;
    std::vector<float> s0{1.1f, 2.2f, 4.5f, 4.9f};
    std::vector<float> s1{2312.1f, 345.244f, 3.59f, -12.8f};
    s0.resize(SIMD_LEN);
    s1.resize(SIMD_LEN);
    init((float*)&src0, s0);
    init((float*)&src1, s1);

    ret = GiMaximumFloat32(src0, src1);

    std::vector<float> naive;
    for (size_t i = 0; i < SIMD_LEN; i++) {
        naive.push_back(Max(s0[i], s1[i]));
    }

    assert_eq((float*)&ret, naive);
}

TEST_F(FALLBACK, GiMinimumFloat32) {
    GI_FLOAT32_t src0, src1, ret;
    std::vector<float> s0{1.1f, 2.2f, 4.5f, 4.9f};
    std::vector<float> s1{2312.1f, 345.244f, 3.59f, -12.8f};
    s0.resize(SIMD_LEN);
    s1.resize(SIMD_LEN);
    init((float*)&src0, s0);
    init((float*)&src1, s1);

    ret = GiMinimumFloat32(src0, src1);

    std::vector<float> naive;
    for (size_t i = 0; i < SIMD_LEN; i++) {
        naive.push_back(Min(s0[i], s1[i]));
    }

    assert_eq((float*)&ret, naive);
}

TEST_F(FALLBACK, GiMaxNanFloat32) {
    GI_FLOAT32_t src0, src1, ret;
    std::vector<float> s0{1.1f, 2.2f, 4.5f, NAN};
    std::vector<float> s1{2312.1f, 345.244f, NAN, -12.8f};
    s0.resize(SIMD_LEN);
    s1.resize(SIMD_LEN);
    init((float*)&src0, s0);
    init((float*)&src1, s1);

    ret = GiMaxNanFloat32(src0, src1);

    std::vector<float> naive;
    for (size_t i = 0; i < SIMD_LEN; i++) {
        auto t = MAX_NAN(s0[i], s1[i]);
        naive.push_back(t);
    }

    assert_eq_and_nan((float*)&ret, naive);
}

TEST_F(FALLBACK, GiMinNanFloat32) {
    GI_FLOAT32_t src0, src1, ret;
    std::vector<float> s0{NAN, 2.2f, NAN, 4.9f};
    std::vector<float> s1{2312.1f, 345.244f, 3.59f, -12.8f};
    s0.resize(SIMD_LEN);
    s1.resize(SIMD_LEN);
    init((float*)&src0, s0);
    init((float*)&src1, s1);

    ret = GiMinNanFloat32(src0, src1);

    std::vector<float> naive;
    for (size_t i = 0; i < SIMD_LEN; i++) {
        auto t = MIN_NAN(s0[i], s1[i]);
        naive.push_back(t);
    }

    assert_eq_and_nan((float*)&ret, naive);
}

TEST_F(FALLBACK, GiClampFloat32) {
    GI_FLOAT32_t src0, src1, ret, na;
    std::vector<float> s0{1.1f, 2.2f, 4.5f, 4.9f};
    std::vector<float> s1{1.1f, 2.2f, 4.5f, 4.9f};
    s0.resize(SIMD_LEN);
    s1.resize(SIMD_LEN);
    init((float*)&src0, s0);
    init((float*)&src1, s1);
    float LowerRange = 3.1415;
    float UpperRange = 4.876;

    auto naive_c = [](GI_FLOAT32_t Value, float LowerRange,
                      float UpperRange) -> GI_FLOAT32_t {
        Value = GiMaximumFloat32(GiBroadcastFloat32(LowerRange), Value);
        Value = GiMinimumFloat32(GiBroadcastFloat32(UpperRange), Value);
        return Value;
    };
    ret = GiClampFloat32(src0, LowerRange, UpperRange);
    na = naive_c(src1, LowerRange, UpperRange);

    std::vector<float> naive;
    naive.resize(SIMD_LEN);
    memcpy(naive.data(), &na, sizeof(GI_FLOAT32_t));

    assert_eq((float*)&ret, naive);
}

TEST_F(FALLBACK, GiReduceAddFloat32) {
    GI_FLOAT32_t src0;
    float ret{0};
    std::vector<float> s0{1.1f, 2.2f, 4.5f, -4.9f};
    s0.resize(SIMD_LEN);
    init((float*)&src0, s0);

    ret = GiReduceAddFloat32(src0);

    float naive{0};
    for (size_t i = 0; i < SIMD_LEN; i++) {
        naive += s0[i];
    }

    ASSERT_LT(std::abs(ret - naive), 1e-3);
}

TEST_F(FALLBACK, GiReduceMultiplyFloat32) {
    GI_FLOAT32_t src0;
    float ret{0};
    std::vector<float> s0{1.1f, 2.2f, 4.5f, -4.9f};
    s0.resize(SIMD_LEN);
    init((float*)&src0, s0);

    ret = GiReduceMultiplyFloat32(src0);

    float naive{1};
    for (size_t i = 0; i < SIMD_LEN; i++) {
        naive *= s0[i];
    }

    ASSERT_LT(std::abs(ret - naive), 1e-3);
}

TEST_F(FALLBACK, GiReduceMaxNanFloat32) {
    GI_FLOAT32_t src0;
    float ret{0};
    std::vector<float> s0{1.1f, 2.2f, 4.9f, -4.9f};
    s0.resize(SIMD_LEN);
    init((float*)&src0, s0);

    ret = GiReduceMaxNanFloat32(src0);

    float naive = s0[0];
    for (size_t i = 0; i < SIMD_LEN; i++) {
        naive = MAX_NAN(naive, s0[i]);
    }

    ASSERT_EQ(ret, naive);
    ret = 0;
    s0 = {1.1f, 2.2f, 4.9f, NAN};
    init((float*)&src0, s0);

    ret = GiReduceMaxNanFloat32(src0);
    ASSERT_TRUE(isnan(ret));
}

TEST_F(FALLBACK, GiReduceMinNanFloat32) {
    GI_FLOAT32_t src0;
    float ret{0};
    std::vector<float> s0{1.1f, 2.2f, 4.5f, -4.9f};
    s0.resize(SIMD_LEN);
    init((float*)&src0, s0);

    ret = GiReduceMinNanFloat32(src0);

    float naive = s0[0];
    for (size_t i = 0; i < SIMD_LEN; i++) {
        naive = MIN_NAN(naive, s0[i]);
    }

    ASSERT_EQ(ret, naive);
    ret = 0;
    s0 = {-1.1f, 2.2f, 4.9f, NAN};
    init((float*)&src0, s0);

    ret = GiReduceMaxNanFloat32(src0);
    ASSERT_TRUE(isnan(ret));
}

TEST_F(FALLBACK, GiAbsFloat32) {
    GI_FLOAT32_t src0, ret;
    std::vector<float> s0{2312.1f, 345.244f, 3.59f, -12.8f};
    s0.resize(SIMD_LEN);
    init((float*)&src0, s0);

    ret = GiAbsFloat32(src0);

    std::vector<float> naive;
    for (size_t i = 0; i < SIMD_LEN; i++) {
        naive.push_back(s0[i] > 0 ? s0[i] : -s0[i]);
    }

    assert_eq((float*)&ret, naive);
}

TEST_F(FALLBACK, GiZip1qS64) {
    GI_INT64_t src0, src1, ret;
    std::vector<int64_t> s0{234242423424245, 42342342422323};
    std::vector<int64_t> s1{23424245, -4234234242232};
    s0.resize(SIMD_LEN / 2);
    s1.resize(SIMD_LEN / 2);
    memcpy(&src0, s0.data(), sizeof(GI_INT64_t));
    memcpy(&src1, s1.data(), sizeof(GI_INT64_t));

    ret = GiZip1qS64(src0, src1);

    std::vector<int64_t> naive;
    naive.push_back(s0[0]);
    naive.push_back(s1[0]);
    auto p = (int64_t*)&ret;
    ASSERT_EQ(naive[0], p[0]);
    ASSERT_EQ(naive[1], p[1]);
}

TEST_F(FALLBACK, GiZip2qS64) {
    GI_INT64_t src0, src1, ret;
    std::vector<int64_t> s0{234242423424245, 42342342422323};
    std::vector<int64_t> s1{23424245, -4234234242232};
    s0.resize(SIMD_LEN / 2);
    s1.resize(SIMD_LEN / 2);
    memcpy(&src0, s0.data(), sizeof(GI_INT64_t));
    memcpy(&src1, s1.data(), sizeof(GI_INT64_t));

    ret = GiZip2qS64(src0, src1);

    std::vector<int64_t> naive;
    naive.push_back(s0[1]);
    naive.push_back(s1[1]);
    auto p = (int64_t*)&ret;
    ASSERT_EQ(naive[0], p[0]);
    ASSERT_EQ(naive[1], p[1]);
}

TEST_F(FALLBACK, GiReinterpretqS64ToFloat32) {
    GI_INT64_t src0;
    GI_FLOAT32_t ret;
    std::vector<int64_t> s0{234242423424245, 42342342422323};
    s0.resize(SIMD_LEN / 2);
    memcpy(&src0, s0.data(), sizeof(GI_INT64_t));

    ret = GiReinterpretqS64ToFloat32(src0);

    std::vector<float> naive;
    naive.resize(SIMD_LEN);
    memcpy(naive.data(), s0.data(), sizeof(GI_FLOAT32_t));

    assert_eq((float*)&ret, naive);
}

TEST_F(FALLBACK, GiReinterpretqFloat32ToS64) {
    GI_FLOAT32_t src0;
    GI_INT64_t ret;
    std::vector<float> s0{2312.1f, 345.244f, 3.59f, -12.8f};
    s0.resize(SIMD_LEN);
    init((float*)&src0, s0);

    ret = GiReinterpretqFloat32ToS64(src0);

    std::vector<float> naive;
    naive.resize(SIMD_LEN);
    memcpy(naive.data(), s0.data(), sizeof(GI_INT64_t));

    assert_eq((float*)&ret, naive);
}

TEST_F(FALLBACK, GiSimdFmaLane) {
    GI_FLOAT32_t src0, src1, src2, ret;
    std::vector<float> s0{1.1f, 2.2f, 3.5f, 4.9f};
    std::vector<float> s1{2312.1f, 345.244f, 3.59f, -12.8f};
    std::vector<float> s2{12.1f, 2.2f, 89.0f, -112.8f};
    s0.resize(SIMD_LEN);
    s1.resize(SIMD_LEN);
    s2.resize(SIMD_LEN);
    init((float*)&src0, s0);
    init((float*)&src1, s1);
    init((float*)&src2, s2);

    std::vector<float> naive = {0, 0, 0, 0};

    auto compare = [&](const size_t n) {
        for (size_t i = 0; i < GI_SIMD_LEN_BYTE / sizeof(float); i++) {
            naive[i] = s0[i] + (s1[i] * s2[n]);
        }
        assert_eq((float*)&ret, naive);
    };

#define CB(n)                                 \
    ret = GiSimdFmaLane(src0, src1, src2, n); \
    compare(n);

    CB(0)
    CB(1)
    CB(2)
    CB(3)
#undef CB
}

TEST_F(FALLBACK, GiMlaqLowLaneFloat32) {
    GI_FLOAT32_t src0, src1, src2, ret;
    std::vector<float> s0{1.1f, 2.2f, 3.5f, 4.9f};
    std::vector<float> s1{2312.1f, 345.244f, 3.59f, -12.8f};
    std::vector<float> s2{12.1f, 2.2f, 89.0f, -112.8f};
    s0.resize(SIMD_LEN);
    s1.resize(SIMD_LEN);
    s2.resize(SIMD_LEN);
    init((float*)&src0, s0);
    init((float*)&src1, s1);
    init((float*)&src2, s2);

    std::vector<float> naive = {0, 0, 0, 0};

    auto compare = [&](const size_t n) {
        for (size_t i = 0; i < GI_SIMD_LEN_BYTE / sizeof(float); i++) {
            naive[i] = s0[i] + (s1[i] * s2[n]);
        }
        assert_eq((float*)&ret, naive);
    };

#define CB(n)                                        \
    ret = GiMlaqLowLaneFloat32(src0, src1, src2, n); \
    compare(n);

    CB(0)
    CB(1)
#undef CB
}

TEST_F(FALLBACK, GiMlaqHighLaneFloat32) {
    GI_FLOAT32_t src0, src1, src2, ret;
    std::vector<float> s0{1.1f, 2.2f, 3.5f, 4.9f};
    std::vector<float> s1{2312.1f, 345.244f, 3.59f, -12.8f};
    std::vector<float> s2{12.1f, 2.2f, 89.0f, -112.8f};
    s0.resize(SIMD_LEN);
    s1.resize(SIMD_LEN);
    s2.resize(SIMD_LEN);
    init((float*)&src0, s0);
    init((float*)&src1, s1);
    init((float*)&src2, s2);

    std::vector<float> naive = {0, 0, 0, 0};

    auto compare = [&](const size_t n) {
        for (size_t i = 0; i < GI_SIMD_LEN_BYTE / sizeof(float); i++) {
            naive[i] = s0[i] + (s1[i] * s2[n]);
        }
        assert_eq((float*)&ret, naive);
    };

#define CB(n)                                         \
    ret = GiMlaqHighLaneFloat32(src0, src1, src2, n); \
    compare(n);

    CB(2)
    CB(3)
#undef CB
}

TEST_F(FALLBACK, GiFmsqLaneQFloat32) {
    GI_FLOAT32_t src0, src1, src2, ret;
    std::vector<float> s0{1.1f, 2.2f, 3.5f, 4.9f};
    std::vector<float> s1{2312.1f, 345.244f, 3.59f, -12.8f};
    std::vector<float> s2{12.1f, 2.2f, 89.0f, -112.8f};
    s0.resize(SIMD_LEN);
    s1.resize(SIMD_LEN);
    s2.resize(SIMD_LEN);
    init((float*)&src0, s0);
    init((float*)&src1, s1);
    init((float*)&src2, s2);

    std::vector<float> naive = {0, 0, 0, 0};

    auto compare = [&](const size_t n) {
        for (size_t i = 0; i < GI_SIMD_LEN_BYTE / sizeof(float); i++) {
            naive[i] = s0[i] - (s1[i] * s2[n]);
        }
        assert_eq((float*)&ret, naive);
    };

#define CB(n)                                      \
    ret = GiFmsqLaneQFloat32(src0, src1, src2, n); \
    compare(n);

    CB(0)
    CB(1)
    CB(2)
    CB(3)
#undef CB
}

TEST_F(FALLBACK, GiBroadcastUint32) {
    int32_t src0 = 20220422;
    GI_UINT32_t ret;

    ret = GiBroadcastUint32(src0);

    std::vector<uint32_t> naive;
    for (size_t i = 0; i < SIMD_LEN; i++) {
        naive.push_back(src0);
    }

    assert_eq((uint32_t*)&ret, naive);
}

TEST_F(FALLBACK, GiLoadInt32) {
    std::vector<int32_t> s0{1, 2, -200, 999};
    GI_INT32_t ret;

    ret = GiLoadInt32(s0.data());

    std::vector<uint32_t> naive;
    for (size_t i = 0; i < SIMD_LEN; i++) {
        naive.push_back(s0[i]);
    }

    assert_eq((uint32_t*)&ret, naive);
}

TEST_F(FALLBACK, GiLoadInt16) {
    std::vector<int16_t> s0{1, 2, -200, 32767, -32768, 45, 3, 0};
    GI_INT16_t ret;

    ret = GiLoadInt16(s0.data());

    auto p = (int16_t*)&ret;
    for (size_t i = 0; i < SIMD_LEN_16; i++) {
        ASSERT_EQ(p[i], s0[i]);
    }
}

TEST_F(FALLBACK, GiLoadInt8) {
    std::vector<int8_t> s0{9,  2, -128, 127, 2, 45, 3, 0,
                           11, 2, -128, 127, 2, 55, 3, -1};
    GI_INT8_t ret;

    ret = GiLoadInt8(s0.data());

    auto p = (int8_t*)&ret;
    for (size_t i = 0; i < SIMD_LEN_8; i++) {
        ASSERT_EQ(p[i], s0[i]);
    }
}

TEST_F(FALLBACK, GiStoreInt32) {
    GI_INT32_t src0;
    std::vector<int32_t> s0{1, 2, -200, 999};
    s0.resize(SIMD_LEN);
    init((int32_t*)&src0, s0);

    std::vector<int32_t> ret;
    ret.resize(SIMD_LEN);
    GiStoreInt32(ret.data(), src0);

    assert_eq<int32_t>(ret.data(), s0);
}

TEST_F(FALLBACK, GiStoreLaneXXInt32) {
    GI_INT32_t src0;
    std::vector<int32_t> s0{1, 2, -200, 999};
    s0.resize(SIMD_LEN);
    init((int32_t*)&src0, s0);

    int32_t ret = 8888;

#define CB(n)                          \
    GiStoreLane##n##Int32(&ret, src0); \
    ASSERT_EQ(s0[n], ret);

    CB(0)
    CB(1)
    CB(2)
    CB(3)
}

TEST_F(FALLBACK, GiReinterInt32ToInt8) {
    GI_INT32_t src0;
    GI_INT8_t ret, naive;
    std::vector<int32_t> s0{65536, 2, -200, 999};
    s0.resize(SIMD_LEN);
    init((int32_t*)&src0, s0);

    ret = GiReinterInt32ToInt8(src0);
    naive = (GI_INT8_t)src0;

    ASSERT_FALSE(memcmp(&ret, &naive, sizeof(GI_INT8_t)));
}

TEST_F(FALLBACK, GiStoreInt16) {
    GI_INT16_t src0;
    std::vector<int16_t> s0{32767, 2, -200, -32768, 1, 2, 3, 4};
    s0.resize(SIMD_LEN_16);
    init((int16_t*)&src0, s0, SIMD_LEN_16);

    std::vector<int16_t> ret;
    ret.resize(SIMD_LEN_16);
    GiStoreInt16(ret.data(), src0);

    assert_eq<int16_t>(ret.data(), s0, SIMD_LEN_16);
}

TEST_F(FALLBACK, GiStoreInt8) {
    GI_INT8_t src0;
    std::vector<int8_t> s0{127, 2, 56, -128, 1, 2, 3, 4, 127, 2, 56, -128, 1, 2, 3, 4};
    s0.resize(SIMD_LEN_8);
    init((int8_t*)&src0, s0, SIMD_LEN_8);

    std::vector<int8_t> ret;
    ret.resize(SIMD_LEN_8);
    GiStoreInt8(ret.data(), src0);

    assert_eq<int8_t>(ret.data(), s0, SIMD_LEN_8);
}

TEST_F(FALLBACK, GiStoreLowInt8) {
    GI_INT8_t src0;
    std::vector<int8_t> s0{127, 2, 56, -128, 1, 2, 3, 4, 127, 2, 56, -128, 1, 2, 3, 4};
    s0.resize(SIMD_LEN_8);
    init((int8_t*)&src0, s0, SIMD_LEN_8);

    std::vector<int8_t> ret;
    ret.resize(SIMD_LEN_8 / 2);
    GiStoreLowInt8(ret.data(), src0);

    assert_eq<int8_t>(ret.data(), s0, SIMD_LEN_8 / 2);
}

TEST_F(FALLBACK, GiStoreHihgInt8) {
    GI_INT8_t src0;
    std::vector<int8_t> s0{127, 2, 56, -128, 1, 2, 3, 4, 127, 2, 56, -128, 1, 2, 3, 4};
    s0.resize(SIMD_LEN_8);
    init((int8_t*)&src0, s0, SIMD_LEN_8);

    std::vector<int8_t> ret;
    ret.resize(SIMD_LEN_8 / 2);
    GiStoreHihgInt8(ret.data(), src0);

    std::vector<int8_t> naive;
    for (size_t i = 0; i < SIMD_LEN_8 / 2; i++) {
        naive.push_back(s0[SIMD_LEN_8 / 2 + i]);
    }

    assert_eq<int8_t>(ret.data(), naive, SIMD_LEN_8 / 2);
}

TEST_F(FALLBACK, GiNegInt32) {
    GI_INT32_t src0, ret;
    std::vector<int32_t> s0{
            std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::min(),
            -3, 4};
    s0.resize(SIMD_LEN);
    init((int32_t*)&src0, s0);

    ret = GiNegInt32(src0);

    std::vector<int32_t> naive;
    for (size_t i = 0; i < SIMD_LEN; i++) {
        naive.push_back(-s0[i]);
    }

    assert_eq((int32_t*)&ret, naive);
}

TEST_F(FALLBACK, GiNegInt8) {
    GI_INT8_t src0, ret;
    std::vector<int8_t> s0{
            std::numeric_limits<int8_t>::max(),
            std::numeric_limits<int8_t>::min(),
            56,
            -128,
            1,
            2,
            3,
            4,
            127,
            2,
            56,
            -128,
            1,
            2,
            3,
            4};
    s0.resize(SIMD_LEN_8);
    init((int8_t*)&src0, s0, SIMD_LEN_8);

    ret = GiNegInt8(src0);

    std::vector<int8_t> naive;
    for (size_t i = 0; i < SIMD_LEN_8; i++) {
        naive.push_back(-s0[i]);
    }

    assert_eq<int8_t>((int8_t*)&ret, naive, SIMD_LEN_8);
}

TEST_F(FALLBACK, GiTestAndSetUint32) {
    GI_UINT32_t src0, src1, ret;
    std::vector<uint32_t> s0{
            8, 2, std::numeric_limits<uint32_t>::max(),
            std::numeric_limits<uint32_t>::min()};
    std::vector<uint32_t> s1{
            8, 4, std::numeric_limits<uint32_t>::max(),
            std::numeric_limits<uint32_t>::max()};
    s0.resize(SIMD_LEN);
    s1.resize(SIMD_LEN);
    init((uint32_t*)&src0, s0);
    init((uint32_t*)&src1, s1);

    ret = GiTestAndSetUint32(src0, src1);

    std::vector<uint32_t> naive;
    for (size_t i = 0; i < SIMD_LEN; i++) {
        naive.push_back(s0[i] & s1[i] ? 0xFFFFFFFF : 0);
    }

    assert_eq<uint32_t>((uint32_t*)&ret, naive);
}

TEST_F(FALLBACK, GiAddInt32) {
    GI_INT32_t src0, src1, ret;
    std::vector<int32_t> s0{127, 2, std::numeric_limits<int32_t>::max(), 9999};
    std::vector<int32_t> s1{1, 2, std::numeric_limits<int32_t>::max(), -9};
    s0.resize(SIMD_LEN);
    s1.resize(SIMD_LEN);
    init((int32_t*)&src0, s0);
    init((int32_t*)&src1, s1);

    ret = GiAddInt32(src0, src1);

    std::vector<int32_t> naive;
    for (size_t i = 0; i < SIMD_LEN; i++) {
        naive.push_back(s0[i] + s1[i]);
    }

    assert_eq((int32_t*)&ret, naive);
}

TEST_F(FALLBACK, GiAddUint32) {
    GI_UINT32_t src0, src1, ret;
    std::vector<uint32_t> s0{127, 2, std::numeric_limits<uint32_t>::max(), 9999};
    std::vector<uint32_t> s1{1, 2, std::numeric_limits<uint32_t>::max(), 9};
    s0.resize(SIMD_LEN);
    s1.resize(SIMD_LEN);
    init((uint32_t*)&src0, s0);
    init((uint32_t*)&src1, s1);

    ret = GiAddUint32(src0, src1);

    std::vector<uint32_t> naive;
    for (size_t i = 0; i < SIMD_LEN; i++) {
        naive.push_back(s0[i] + s1[i]);
    }

    assert_eq((uint32_t*)&ret, naive);
}

TEST_F(FALLBACK, GiAddInt16) {
    GI_INT16_t src0, src1, ret;
    std::vector<int16_t> s0{-127, 2, std::numeric_limits<int16_t>::max(), 9999, 1, 2,
                            3,    4};
    std::vector<int16_t> s1{1,
                            2,
                            std::numeric_limits<int16_t>::max(),
                            std::numeric_limits<int16_t>::min(),
                            -1,
                            23,
                            -3,
                            -5};
    s0.resize(SIMD_LEN_16);
    s1.resize(SIMD_LEN_16);
    init((int16_t*)&src0, s0, SIMD_LEN_16);
    init((int16_t*)&src1, s1, SIMD_LEN_16);

    ret = GiAddInt16(src0, src1);

    std::vector<int16_t> naive;
    for (size_t i = 0; i < SIMD_LEN_16; i++) {
        naive.push_back(s0[i] + s1[i]);
    }

    assert_eq<int16_t>((int16_t*)&ret, naive, SIMD_LEN_16);
}

TEST_F(FALLBACK, GiAddInt8) {
    GI_INT8_t src0, src1, ret;
    std::vector<int8_t> s0{
            std::numeric_limits<int8_t>::max(),
            std::numeric_limits<int8_t>::min(),
            56,
            -128,
            1,
            2,
            3,
            4,
            127,
            2,
            56,
            -128,
            1,
            2,
            3,
            4};
    std::vector<int8_t> s1{
            3,
            std::numeric_limits<int8_t>::max(),
            std::numeric_limits<int8_t>::min(),
            56,
            -128,
            1,
            2,
            3,
            4,
            127,
            2,
            56,
            -128,
            1,
            2,
            4};
    s0.resize(SIMD_LEN_8);
    s1.resize(SIMD_LEN_8);
    init((int8_t*)&src0, s0, SIMD_LEN_8);
    init((int8_t*)&src1, s1, SIMD_LEN_8);

    ret = GiAddInt8(src0, src1);

    std::vector<int8_t> naive;
    for (size_t i = 0; i < SIMD_LEN_8; i++) {
        naive.push_back(s0[i] + s1[i]);
    }

    assert_eq<int8_t>((int8_t*)&ret, naive, SIMD_LEN_8);
}

TEST_F(FALLBACK, GiSubtractInt32) {
    GI_INT32_t src0, src1, ret;
    std::vector<int32_t> s0{127, 2, std::numeric_limits<int32_t>::max(), 9999};
    std::vector<int32_t> s1{1, 2, std::numeric_limits<int32_t>::max(), -9};
    s0.resize(SIMD_LEN);
    s1.resize(SIMD_LEN);
    init((int32_t*)&src0, s0);
    init((int32_t*)&src1, s1);

    ret = GiSubtractInt32(src0, src1);

    std::vector<int32_t> naive;
    for (size_t i = 0; i < SIMD_LEN; i++) {
        naive.push_back(s0[i] - s1[i]);
    }

    assert_eq((int32_t*)&ret, naive);
}

TEST_F(FALLBACK, GiSubtractUint32) {
    GI_UINT32_t src0, src1, ret;
    std::vector<uint32_t> s0{127, 2, std::numeric_limits<uint32_t>::max(), 9999};
    std::vector<uint32_t> s1{1, 2, std::numeric_limits<uint32_t>::max(), 9};
    s0.resize(SIMD_LEN);
    s1.resize(SIMD_LEN);
    init((uint32_t*)&src0, s0);
    init((uint32_t*)&src1, s1);

    ret = GiSubtractUint32(src0, src1);

    std::vector<uint32_t> naive;
    for (size_t i = 0; i < SIMD_LEN; i++) {
        naive.push_back(s0[i] - s1[i]);
    }

    assert_eq((uint32_t*)&ret, naive);
}

TEST_F(FALLBACK, GiSubtractInt8) {
    GI_INT8_t src0, src1, ret;
    std::vector<int8_t> s0{
            std::numeric_limits<int8_t>::max(),
            std::numeric_limits<int8_t>::min(),
            56,
            -128,
            1,
            2,
            3,
            4,
            127,
            2,
            56,
            -128,
            1,
            2,
            3,
            4};
    std::vector<int8_t> s1{
            3,
            std::numeric_limits<int8_t>::max(),
            std::numeric_limits<int8_t>::min(),
            56,
            -128,
            1,
            2,
            3,
            4,
            127,
            2,
            56,
            -128,
            1,
            2,
            4};
    s0.resize(SIMD_LEN_8);
    s1.resize(SIMD_LEN_8);
    init((int8_t*)&src0, s0, SIMD_LEN_8);
    init((int8_t*)&src1, s1, SIMD_LEN_8);

    ret = GiSubtractInt8(src0, src1);

    std::vector<int8_t> naive;
    for (size_t i = 0; i < SIMD_LEN_8; i++) {
        naive.push_back(s0[i] - s1[i]);
    }

    assert_eq<int8_t>((int8_t*)&ret, naive, SIMD_LEN_8);
}

TEST_F(FALLBACK, GiMultiplyInt32) {
    GI_INT32_t src0, src1, ret;
    std::vector<int32_t> s0{127, 2, 202204, 99};
    std::vector<int32_t> s1{1, 2, -4, -9};
    s0.resize(SIMD_LEN);
    s1.resize(SIMD_LEN);
    init((int32_t*)&src0, s0);
    init((int32_t*)&src1, s1);

    ret = GiMultiplyInt32(src0, src1);

    std::vector<int32_t> naive;
    for (size_t i = 0; i < SIMD_LEN; i++) {
        naive.push_back(s0[i] * s1[i]);
    }

    assert_eq((int32_t*)&ret, naive);
}

TEST_F(FALLBACK, GiMultiplyInt8) {
    GI_INT8_t src0, src1, ret;
    std::vector<int8_t> s0{
            std::numeric_limits<int8_t>::max(),
            std::numeric_limits<int8_t>::min(),
            56,
            -128,
            1,
            2,
            3,
            4,
            127,
            2,
            56,
            -128,
            1,
            2,
            3,
            4};
    std::vector<int8_t> s1{
            3,
            std::numeric_limits<int8_t>::max(),
            std::numeric_limits<int8_t>::min(),
            56,
            -128,
            1,
            2,
            3,
            4,
            127,
            2,
            56,
            -128,
            1,
            2,
            4};
    s0.resize(SIMD_LEN_8);
    s1.resize(SIMD_LEN_8);
    init((int8_t*)&src0, s0, SIMD_LEN_8);
    init((int8_t*)&src1, s1, SIMD_LEN_8);

    ret = GiMultiplyInt8(src0, src1);

    std::vector<int8_t> naive;
    for (size_t i = 0; i < SIMD_LEN_8; i++) {
        naive.push_back(s0[i] * s1[i]);
    }

    assert_eq<int8_t>((int8_t*)&ret, naive, SIMD_LEN_8);
}

TEST_F(FALLBACK, GiMultiplyAddInt32) {
    GI_INT32_t src0, src1, src2, ret;
    std::vector<int32_t> s0{127, 2, 67, 9999};
    std::vector<int32_t> s1{1, 2, 90, -9};
    std::vector<int32_t> s2{-1, 12, 4, -9};
    s0.resize(SIMD_LEN);
    s1.resize(SIMD_LEN);
    s2.resize(SIMD_LEN);
    init((int32_t*)&src0, s0);
    init((int32_t*)&src1, s1);
    init((int32_t*)&src2, s2);

    ret = GiMultiplyAddInt32(src0, src1, src2);

    std::vector<int32_t> naive;
    for (size_t i = 0; i < SIMD_LEN; i++) {
        naive.push_back(s0[i] + s1[i] * s2[i]);
    }

    assert_eq((int32_t*)&ret, naive);
}

TEST_F(FALLBACK, GiMultiplyAddInt8) {
    GI_INT8_t src0, src1, src2, ret;
    std::vector<int8_t> s0{
            std::numeric_limits<int8_t>::max(),
            std::numeric_limits<int8_t>::min(),
            56,
            -128,
            1,
            2,
            3,
            4,
            127,
            2,
            56,
            -128,
            1,
            2,
            3,
            4};
    std::vector<int8_t> s1{
            3,
            std::numeric_limits<int8_t>::max(),
            std::numeric_limits<int8_t>::min(),
            56,
            -128,
            1,
            2,
            3,
            4,
            127,
            2,
            56,
            -128,
            1,
            2,
            4};
    std::vector<int8_t> s2{
            std::numeric_limits<int8_t>::min(),
            56,
            -128,
            1,
            2,
            3,
            4,
            127,
            2,
            56,
            -128,
            1,
            2,
            5,
            8,
            4};
    s0.resize(SIMD_LEN_8);
    s1.resize(SIMD_LEN_8);
    s2.resize(SIMD_LEN_8);
    init((int8_t*)&src0, s0, SIMD_LEN_8);
    init((int8_t*)&src1, s1, SIMD_LEN_8);
    init((int8_t*)&src2, s2, SIMD_LEN_8);

    ret = GiMultiplyAddInt8(src0, src1, src2);

    std::vector<int8_t> naive;
    for (size_t i = 0; i < SIMD_LEN_8; i++) {
        naive.push_back(s0[i] + s1[i] * s2[i]);
    }

    assert_eq<int8_t>((int8_t*)&ret, naive, SIMD_LEN_8);
}

TEST_F(FALLBACK, GiAndInt8) {
    GI_INT8_t src0, src1, ret;
    std::vector<int8_t> s0{
            std::numeric_limits<int8_t>::max(),
            std::numeric_limits<int8_t>::min(),
            56,
            -128,
            1,
            2,
            3,
            4,
            127,
            2,
            56,
            -128,
            1,
            2,
            3,
            4};
    std::vector<int8_t> s1{
            3,
            std::numeric_limits<int8_t>::max(),
            std::numeric_limits<int8_t>::min(),
            56,
            -128,
            1,
            2,
            3,
            4,
            127,
            2,
            56,
            -128,
            1,
            2,
            4};
    s0.resize(SIMD_LEN_8);
    s1.resize(SIMD_LEN_8);
    init((int8_t*)&src0, s0, SIMD_LEN_8);
    init((int8_t*)&src1, s1, SIMD_LEN_8);

    ret = GiAndInt8(src0, src1);

    std::vector<int8_t> naive;
    for (size_t i = 0; i < SIMD_LEN_8; i++) {
        naive.push_back(s0[i] & s1[i]);
    }

    assert_eq<int8_t>((int8_t*)&ret, naive, SIMD_LEN_8);
}

TEST_F(FALLBACK, GiEOrUint32) {
    GI_UINT32_t src0, src1, ret;
    std::vector<uint32_t> s0{127, 2, std::numeric_limits<uint32_t>::max(), 9999};
    std::vector<uint32_t> s1{1, 2, std::numeric_limits<uint32_t>::max(), 9};
    s0.resize(SIMD_LEN);
    s1.resize(SIMD_LEN);
    init((uint32_t*)&src0, s0);
    init((uint32_t*)&src1, s1);

    ret = GiEOrUint32(src0, src1);

    std::vector<uint32_t> naive;
    for (size_t i = 0; i < SIMD_LEN; i++) {
        naive.push_back(s0[i] ^ s1[i]);
    }

    assert_eq((uint32_t*)&ret, naive);
}

TEST_F(FALLBACK, GiOrInt8) {
    GI_INT8_t src0, src1, ret;
    std::vector<int8_t> s0{
            std::numeric_limits<int8_t>::max(),
            std::numeric_limits<int8_t>::min(),
            56,
            -128,
            1,
            2,
            3,
            4,
            127,
            2,
            56,
            -128,
            1,
            2,
            3,
            4};
    std::vector<int8_t> s1{
            3,
            std::numeric_limits<int8_t>::max(),
            std::numeric_limits<int8_t>::min(),
            56,
            -128,
            1,
            2,
            3,
            4,
            127,
            2,
            56,
            -128,
            1,
            2,
            4};
    s0.resize(SIMD_LEN_8);
    s1.resize(SIMD_LEN_8);
    init((int8_t*)&src0, s0, SIMD_LEN_8);
    init((int8_t*)&src1, s1, SIMD_LEN_8);

    ret = GiOrInt8(src0, src1);

    std::vector<int8_t> naive;
    for (size_t i = 0; i < SIMD_LEN_8; i++) {
        naive.push_back(s0[i] | s1[i]);
    }

    assert_eq<int8_t>((int8_t*)&ret, naive, SIMD_LEN_8);
}

TEST_F(FALLBACK, GiAndNotInt8) {
    GI_INT8_t src0, src1, ret;
    std::vector<int8_t> s0{
            std::numeric_limits<int8_t>::max(),
            std::numeric_limits<int8_t>::min(),
            56,
            -128,
            1,
            2,
            3,
            4,
            127,
            2,
            56,
            -128,
            1,
            2,
            3,
            4};
    std::vector<int8_t> s1{
            3,
            std::numeric_limits<int8_t>::max(),
            std::numeric_limits<int8_t>::min(),
            56,
            -128,
            1,
            2,
            3,
            4,
            127,
            2,
            56,
            -128,
            1,
            2,
            4};
    s0.resize(SIMD_LEN_8);
    s1.resize(SIMD_LEN_8);
    init((int8_t*)&src0, s0, SIMD_LEN_8);
    init((int8_t*)&src1, s1, SIMD_LEN_8);

    ret = GiAndNotInt8(src0, src1);

    std::vector<int8_t> naive;
    for (size_t i = 0; i < SIMD_LEN_8; i++) {
        naive.push_back((~s0[i]) & s1[i]);
    }

    assert_eq<int8_t>((int8_t*)&ret, naive, SIMD_LEN_8);
}

TEST_F(FALLBACK, GiXorInt8) {
    GI_INT8_t src0, src1, ret;
    std::vector<int8_t> s0{
            std::numeric_limits<int8_t>::max(),
            std::numeric_limits<int8_t>::min(),
            56,
            -128,
            1,
            2,
            3,
            4,
            127,
            2,
            56,
            -128,
            1,
            2,
            3,
            4};
    std::vector<int8_t> s1{
            3,
            std::numeric_limits<int8_t>::max(),
            std::numeric_limits<int8_t>::min(),
            56,
            -128,
            1,
            2,
            3,
            4,
            127,
            2,
            56,
            -128,
            1,
            2,
            4};
    s0.resize(SIMD_LEN_8);
    s1.resize(SIMD_LEN_8);
    init((int8_t*)&src0, s0, SIMD_LEN_8);
    init((int8_t*)&src1, s1, SIMD_LEN_8);

    ret = GiXorInt8(src0, src1);

    std::vector<int8_t> naive;
    for (size_t i = 0; i < SIMD_LEN_8; i++) {
        naive.push_back((s0[i]) ^ s1[i]);
    }

    assert_eq<int8_t>((int8_t*)&ret, naive, SIMD_LEN_8);
}

TEST_F(FALLBACK, GiShiftRight23Int32) {
    GI_INT32_t src0, ret;
    std::vector<int32_t> s0{1, 2, 3, -4};
    s0.resize(SIMD_LEN);
    init((int32_t*)&src0, s0);

    ret = GiShiftRight23Int32(src0);

    std::vector<int32_t> naive;
    for (size_t i = 0; i < SIMD_LEN; i++) {
        naive.push_back(s0[i] >> 23);
    }

    assert_eq((int32_t*)&ret, naive);
}

TEST_F(FALLBACK, GiBlendInt32) {
    GI_INT32_t src0, src1, src2, ret, na;
    std::vector<int32_t> s0{1, 2, 3, -4};
    std::vector<int32_t> s1{12, 22, 32, -43};
    std::vector<int32_t> s2{-1, 21, 34, 4};
    s0.resize(SIMD_LEN);
    s1.resize(SIMD_LEN);
    s2.resize(SIMD_LEN);
    init((int32_t*)&src0, s0);
    init((int32_t*)&src1, s1);
    init((int32_t*)&src2, s2);

    ret = GiBlendInt32(src0, src1, src2);

    na = GiOrInt32(GiAndInt32(src1, src2), GiAndNotInt32(src2, src0));

    std::vector<int32_t> naive;
    auto p = (int32_t*)&na;
    for (size_t i = 0; i < SIMD_LEN; i++) {
        naive.push_back(p[i]);
    }

    assert_eq((int32_t*)&ret, naive);
}

TEST_F(FALLBACK, GiBlendInt8) {
    GI_INT8_t src0, src1, src2, ret, na;
    std::vector<int8_t> s0{
            std::numeric_limits<int8_t>::max(),
            std::numeric_limits<int8_t>::min(),
            56,
            -128,
            1,
            2,
            3,
            4,
            127,
            2,
            56,
            -128,
            1,
            2,
            3,
            4};
    std::vector<int8_t> s1{
            3,
            std::numeric_limits<int8_t>::max(),
            std::numeric_limits<int8_t>::min(),
            56,
            -128,
            1,
            2,
            3,
            4,
            127,
            2,
            56,
            -128,
            1,
            2,
            4};
    std::vector<int8_t> s2{
            std::numeric_limits<int8_t>::min(),
            56,
            -128,
            1,
            2,
            3,
            4,
            127,
            2,
            56,
            -128,
            1,
            2,
            5,
            8,
            4};
    s0.resize(SIMD_LEN_8);
    s1.resize(SIMD_LEN_8);
    s2.resize(SIMD_LEN_8);
    init((int8_t*)&src0, s0, SIMD_LEN_8);
    init((int8_t*)&src1, s1, SIMD_LEN_8);
    init((int8_t*)&src2, s2, SIMD_LEN_8);

    ret = GiBlendInt8(src0, src1, src2);
    na = GiOrInt8(GiAndInt8(src1, src2), GiAndNotInt8(src2, src0));

    std::vector<int8_t> naive;
    auto p = (int8_t*)&na;
    for (size_t i = 0; i < SIMD_LEN_8; i++) {
        naive.push_back(p[i]);
    }

    assert_eq<int8_t>((int8_t*)&ret, naive, SIMD_LEN_8);
}

TEST_F(FALLBACK, GiAbsInt32) {
    GI_INT32_t src0, ret;
    std::vector<int32_t> s0{-1, 2, -3, 4};
    s0.resize(SIMD_LEN);
    init((int32_t*)&src0, s0);

    ret = GiAbsInt32(src0);

    std::vector<int32_t> naive;
    for (size_t i = 0; i < SIMD_LEN; i++) {
        naive.push_back(s0[i] > 0 ? s0[i] : -s0[i]);
    }

    assert_eq((int32_t*)&ret, naive);
}

TEST_F(FALLBACK, GiAbsInt16) {
    GI_INT16_t src0, ret;
    std::vector<int16_t> s0{-127, 2, std::numeric_limits<int16_t>::max(), 9999, 1, 2,
                            3,    4};
    s0.resize(SIMD_LEN_16);
    init((int16_t*)&src0, s0, SIMD_LEN_16);

    ret = GiAbsInt16(src0);

    std::vector<int16_t> naive;
    for (size_t i = 0; i < SIMD_LEN_16; i++) {
        naive.push_back(s0[i] > 0 ? s0[i] : -s0[i]);
    }

    assert_eq<int16_t>((int16_t*)&ret, naive, SIMD_LEN_16);
}

TEST_F(FALLBACK, GiAbsInt8) {
    GI_INT8_t src0, ret;
    std::vector<int8_t> s0{
            std::numeric_limits<int8_t>::max(),
            std::numeric_limits<int8_t>::min(),
            56,
            -128,
            1,
            2,
            3,
            4,
            127,
            2,
            56,
            -128,
            1,
            2,
            3,
            4};
    s0.resize(SIMD_LEN_8);
    init((int8_t*)&src0, s0, SIMD_LEN_8);

    ret = GiAbsInt8(src0);

    std::vector<int8_t> naive;
    for (size_t i = 0; i < SIMD_LEN_8; i++) {
        naive.push_back(s0[i] > 0 ? s0[i] : -s0[i]);
    }

    assert_eq<int8_t>((int8_t*)&ret, naive, SIMD_LEN_8);
}

TEST_F(FALLBACK, GiMaximumInt32) {
    GI_INT32_t src0, src1, src2, ret, na;
    std::vector<int32_t> s0{1, -2, 3, 4};
    s0.resize(SIMD_LEN);
    std::vector<int32_t> s1{5, 6, 7, -8};
    s1.resize(SIMD_LEN);
    init((int32_t*)&src0, s0);
    init((int32_t*)&src1, s1);

    std::vector<int32_t> s2;
    for (size_t i = 0; i < SIMD_LEN; i++) {
        s2.push_back(s0[i] > s1[i] ? 0xFFFFFFFF : 0);
    }
    s2.resize(SIMD_LEN);
    init((int32_t*)&src2, s2);

    ret = GiMaximumInt32(src0, src1);

    na = GiBlendInt32(src1, src0, src2);
    std::vector<int32_t> naive;
    auto p = (int32_t*)&na;
    for (size_t i = 0; i < SIMD_LEN; i++) {
        naive.push_back(p[i]);
    }

    assert_eq((int32_t*)&ret, naive);
}

TEST_F(FALLBACK, GiMinimumInt32) {
    GI_INT32_t src0, src1, src2, ret, na;
    std::vector<int32_t> s0{1, -2, 3, 4};
    s0.resize(SIMD_LEN);
    std::vector<int32_t> s1{5, 6, 7, -8};
    s1.resize(SIMD_LEN);
    init((int32_t*)&src0, s0);
    init((int32_t*)&src1, s1);

    std::vector<int32_t> s2;
    for (size_t i = 0; i < SIMD_LEN; i++) {
        s2.push_back(s1[i] > s0[i] ? 0xFFFFFFFF : 0);
    }
    s2.resize(SIMD_LEN);
    init((int32_t*)&src2, s2);

    ret = GiMinimumInt32(src0, src1);

    na = GiBlendInt32(src1, src0, src2);
    std::vector<int32_t> naive;
    auto p = (int32_t*)&na;
    for (size_t i = 0; i < SIMD_LEN; i++) {
        naive.push_back(p[i]);
    }

    assert_eq((int32_t*)&ret, naive);
}

TEST_F(FALLBACK, GiBlendInt8x16) {
    GI_INT8_t src0, src1, src2, ret, na;
    std::vector<int8_t> s0{
            std::numeric_limits<int8_t>::max(),
            std::numeric_limits<int8_t>::min(),
            56,
            -128,
            1,
            2,
            3,
            4,
            127,
            2,
            56,
            -128,
            1,
            2,
            3,
            4};
    std::vector<int8_t> s1{
            3,
            std::numeric_limits<int8_t>::max(),
            std::numeric_limits<int8_t>::min(),
            56,
            -128,
            1,
            2,
            3,
            4,
            127,
            2,
            56,
            -128,
            1,
            2,
            4};
    std::vector<int8_t> s2{
            std::numeric_limits<int8_t>::min(),
            56,
            -128,
            1,
            2,
            3,
            4,
            127,
            2,
            56,
            -128,
            1,
            2,
            5,
            8,
            4};
    s0.resize(SIMD_LEN_8);
    s1.resize(SIMD_LEN_8);
    s2.resize(SIMD_LEN_8);
    init((int8_t*)&src0, s0, SIMD_LEN_8);
    init((int8_t*)&src1, s1, SIMD_LEN_8);
    init((int8_t*)&src2, s2, SIMD_LEN_8);

    ret = GiBlendInt8x16(src0, src1, src2);
    na = GiOrInt8(GiAndInt8(src1, src2), GiAndNotInt8(src2, src0));

    std::vector<int8_t> naive;
    auto p = (int8_t*)&na;
    for (size_t i = 0; i < SIMD_LEN_8; i++) {
        naive.push_back(p[i]);
    }

    assert_eq<int8_t>((int8_t*)&ret, naive, SIMD_LEN_8);
}

TEST_F(FALLBACK, GiMaximumInt8) {
    GI_INT8_t src0, src1, src2, ret, na;
    std::vector<int8_t> s0{
            std::numeric_limits<int8_t>::max(),
            std::numeric_limits<int8_t>::min(),
            56,
            -128,
            1,
            2,
            3,
            4,
            127,
            2,
            56,
            -128,
            1,
            2,
            3,
            4};
    std::vector<int8_t> s1{
            3,
            std::numeric_limits<int8_t>::max(),
            std::numeric_limits<int8_t>::min(),
            56,
            -128,
            1,
            2,
            3,
            4,
            127,
            2,
            56,
            -128,
            1,
            2,
            4};
    s0.resize(SIMD_LEN_8);
    s1.resize(SIMD_LEN_8);
    init((int8_t*)&src0, s0, SIMD_LEN_8);
    init((int8_t*)&src1, s1, SIMD_LEN_8);

    std::vector<int8_t> s2;
    for (size_t i = 0; i < SIMD_LEN_8; i++) {
        s2.push_back(s1[i] < s0[i] ? 0xFF : 0);
    }
    s2.resize(SIMD_LEN_8);
    init((int8_t*)&src2, s2, SIMD_LEN_8);
    ret = GiMaximumInt8(src0, src1);

    na = GiBlendInt8(src1, src0, src2);

    std::vector<int8_t> naive;
    auto p = (int8_t*)&na;
    for (size_t i = 0; i < SIMD_LEN_8; i++) {
        naive.push_back(p[i]);
    }

    assert_eq<int8_t>((int8_t*)&ret, naive, SIMD_LEN_8);
}

TEST_F(FALLBACK, GiMinimumInt8) {
    GI_INT8_t src0, src1, src2, ret, na;
    std::vector<int8_t> s0{
            std::numeric_limits<int8_t>::max(),
            std::numeric_limits<int8_t>::min(),
            56,
            -128,
            1,
            2,
            3,
            4,
            127,
            2,
            56,
            -128,
            1,
            2,
            3,
            4};
    std::vector<int8_t> s1{
            3,
            std::numeric_limits<int8_t>::max(),
            std::numeric_limits<int8_t>::min(),
            56,
            -128,
            1,
            2,
            3,
            4,
            127,
            2,
            56,
            -128,
            1,
            2,
            4};
    s0.resize(SIMD_LEN_8);
    s1.resize(SIMD_LEN_8);
    init((int8_t*)&src0, s0, SIMD_LEN_8);
    init((int8_t*)&src1, s1, SIMD_LEN_8);

    std::vector<int8_t> s2;
    for (size_t i = 0; i < SIMD_LEN_8; i++) {
        s2.push_back(s1[i] > s0[i] ? 0xFF : 0);
    }
    s2.resize(SIMD_LEN_8);
    init((int8_t*)&src2, s2, SIMD_LEN_8);
    ret = GiMinimumInt8(src0, src1);

    na = GiBlendInt8(src1, src0, src2);

    std::vector<int8_t> naive;
    auto p = (int8_t*)&na;
    for (size_t i = 0; i < SIMD_LEN_8; i++) {
        naive.push_back(p[i]);
    }

    assert_eq<int8_t>((int8_t*)&ret, naive, SIMD_LEN_8);
}

TEST_F(FALLBACK, GiMoveHighLongInt8) {
    GI_INT8_t src0;
    GI_INT16_t ret;

    std::vector<int8_t> s0{
            std::numeric_limits<int8_t>::max(),
            std::numeric_limits<int8_t>::min(),
            56,
            -128,
            1,
            2,
            3,
            4,
            127,
            2,
            56,
            -128,
            std::numeric_limits<int8_t>::max(),
            std::numeric_limits<int8_t>::min(),
            3,
            4};

    s0.resize(SIMD_LEN_8);
    init((int8_t*)&src0, s0, SIMD_LEN_8);

    ret = GiMoveHighLongInt8(src0);

    std::vector<int16_t> naive;
    for (size_t i = 0; i < SIMD_LEN_8 / 2; i++) {
        naive.push_back(s0[i + SIMD_LEN_8 / 2]);
    }

    assert_eq<int16_t>((int16_t*)&ret, naive, SIMD_LEN_16);
}

TEST_F(FALLBACK, GiMoveLowLongInt8) {
    GI_INT8_t src0;
    GI_INT16_t ret;

    std::vector<int8_t> s0{
            std::numeric_limits<int8_t>::max(),
            std::numeric_limits<int8_t>::min(),
            56,
            -128,
            1,
            2,
            3,
            4,
            127,
            2,
            56,
            -128,
            std::numeric_limits<int8_t>::max(),
            std::numeric_limits<int8_t>::min(),
            3,
            4};

    s0.resize(SIMD_LEN_8);
    init((int8_t*)&src0, s0, SIMD_LEN_8);

    ret = GiMoveLowLongInt8(src0);

    std::vector<int16_t> naive;
    for (size_t i = 0; i < SIMD_LEN_8 / 2; i++) {
        naive.push_back(s0[i]);
    }

    assert_eq<int16_t>((int16_t*)&ret, naive, SIMD_LEN_16);
}

TEST_F(FALLBACK, GiMoveHighLongInt16) {
    GI_INT16_t src0;
    GI_INT32_t ret;
    std::vector<int16_t> s0{-127, 2, std::numeric_limits<int16_t>::max(), 9999, 1, 2,
                            3,    4};
    s0.resize(SIMD_LEN_16);
    init((int16_t*)&src0, s0, SIMD_LEN_16);

    ret = GiMoveHighLongInt16(src0);

    std::vector<int32_t> naive;
    for (size_t i = 0; i < SIMD_LEN_16 / 2; i++) {
        naive.push_back(s0[i + SIMD_LEN_16 / 2]);
    }

    assert_eq<int32_t>((int32_t*)&ret, naive, SIMD_LEN);
}

TEST_F(FALLBACK, GiMoveLowLongInt16) {
    GI_INT16_t src0;
    GI_INT32_t ret;
    std::vector<int16_t> s0{-127, 2, std::numeric_limits<int16_t>::max(), 9999, 1, 2,
                            3,    4};
    s0.resize(SIMD_LEN_16);
    init((int16_t*)&src0, s0, SIMD_LEN_16);

    ret = GiMoveLowLongInt16(src0);

    std::vector<int32_t> naive;
    for (size_t i = 0; i < SIMD_LEN_16 / 2; i++) {
        naive.push_back(s0[i]);
    }

    assert_eq<int32_t>((int32_t*)&ret, naive, SIMD_LEN);
}

TEST_F(FALLBACK, GiReduceAddInt8) {
    GI_INT8_t src0;
    int32_t ret{0};
    std::vector<int8_t> s0{127, 2, 56, -128, 1, 2, 3, 4, 127, 2, 56, -128, 1, 2, 3, 4};
    s0.resize(SIMD_LEN_8);
    init((int8_t*)&src0, s0, SIMD_LEN_8);

    ret = GiReduceAddInt8(src0);

    int32_t naive{0};
    for (auto i : s0) {
        naive += i;
    }

    ASSERT_EQ(ret, naive);
}

TEST_F(FALLBACK, GiReduceMaxInt8) {
    GI_INT8_t src0;
    int8_t ret{0};
    std::vector<int8_t> s0{127, 2, 56, -128, 1, 2, 3, 4, 127, 2, 56, -128, 1, 2, 3, 4};
    s0.resize(SIMD_LEN_8);
    init((int8_t*)&src0, s0, SIMD_LEN_8);

    ret = GiReduceMaxInt8(src0);

    int8_t naive{s0[0]};
    for (size_t i = 0; i < SIMD_LEN_8; i++) {
        naive = Max(naive, s0[i]);
    }

    ASSERT_EQ(ret, naive);
}

TEST_F(FALLBACK, GiReduceMinInt8) {
    GI_INT8_t src0;
    int8_t ret{0};
    std::vector<int8_t> s0{127, 2, 56, -128, 1, 2, 3, 4, 127, 2, 56, -128, 1, 2, 3, 4};
    s0.resize(SIMD_LEN_8);
    init((int8_t*)&src0, s0, SIMD_LEN_8);

    ret = GiReduceMinInt8(src0);

    int8_t naive{s0[0]};
    for (size_t i = 0; i < SIMD_LEN_8; i++) {
        naive = Min(naive, s0[i]);
    }

    ASSERT_EQ(ret, naive);
}

TEST_F(FALLBACK, GiCvtFromFloat32ToInt8) {
    GI_INT8_t ret;
    GI_FLOAT32_t src0;
    std::vector<float> s0{
            1.0f, -2.2f, std::numeric_limits<float>::max(),
            std::numeric_limits<float>::min()};
    s0.resize(SIMD_LEN);
    init((float*)&src0, s0);

    ret = GiCvtFromFloat32ToInt8(src0);

    std::vector<int8_t> naive;
    naive.resize(SIMD_LEN_8);

    for (size_t i = 0; i < SIMD_LEN; i++) {
        int8_t data = Saturate(round(s0[i]), -128, 127);
        naive[i] = data;
        naive[SIMD_LEN + i] = data;
        naive[2 * SIMD_LEN + i] = data;
        naive[3 * SIMD_LEN + i] = data;
    }

    assert_eq<int8_t>((int8_t*)&ret, naive, SIMD_LEN_8);
}

TEST_F(FALLBACK, GiCvtFromFloat32V2ToInt8) {
    GI_INT8_t ret;
    GI_FLOAT32_V2_t src0;
    std::vector<float> s0{
            1.0f,
            -2.2f,
            std::numeric_limits<float>::max(),
            std::numeric_limits<float>::min(),
            1.1f,
            2.2f,
            -9.0f,
            899999.0f};
    s0.resize(SIMD_LEN * 2);
    init((float*)&src0, s0, SIMD_LEN * 2);

    ret = GiCvtFromFloat32V2ToInt8(src0);

    std::vector<int8_t> naive;

    for (size_t i = 0; i < SIMD_LEN * 2; i++) {
        naive.push_back(Saturate(round(s0[i]), -128, 127));
    }

    for (size_t i = 0; i < SIMD_LEN * 2; i++) {
        naive.push_back(Saturate(round(s0[i]), -128, 127));
    }

    assert_eq<int8_t>((int8_t*)&ret, naive, SIMD_LEN_8);
}

TEST_F(FALLBACK, GiCvtFromFloat32V4ToInt8) {
    GI_INT8_t ret;
    GI_FLOAT32_V4_t src0;
    std::vector<float> s0{
            std::numeric_limits<float>::max(),
            std::numeric_limits<float>::min(),
            1.0f,
            -2.2f,
            3.1f,
            4.2f,
            -5.0f,
            6.0f,
            7.0f,
            8.0f,
            -9.9f,
            10.9f,
            -11.9f,
            12.9f,
            13.9f,
            -14.9f};
    s0.resize(SIMD_LEN * 4);
    init((float*)&src0, s0, SIMD_LEN * 4);

    ret = GiCvtFromFloat32V4ToInt8(src0);

    std::vector<int8_t> naive;

    for (size_t i = 0; i < SIMD_LEN * 4; i++) {
        naive.push_back(Saturate(round(s0[i]), -128, 127));
    }

    assert_eq<int8_t>((int8_t*)&ret, naive, SIMD_LEN_8);
}

TEST_F(FALLBACK, GiCombineFloat32) {
    float32x2_t src0, src1;
    GI_FLOAT32_t ret;
    std::vector<float> s0{1.1f, -3.1415f};
    std::vector<float> s1{2.3f, 3.14777f};
    memcpy(&src0, s0.data(), sizeof(float32x2_t));
    memcpy(&src1, s1.data(), sizeof(float32x2_t));

    ret = GiCombineFloat32(src0, src1);

    std::vector<float> naive;
    naive.push_back(s0[0]);
    naive.push_back(s0[1]);
    naive.push_back(s1[0]);
    naive.push_back(s1[1]);

    assert_eq<float>((float*)&ret, naive);
}

TEST_F(FALLBACK, GiGetLowFloat32) {
    float32x2_t ret;
    GI_FLOAT32_t src0;
    std::vector<float> s0{1.0f, 2.2f, 3.4f, 4.5f};
    s0.resize(SIMD_LEN);
    init((float*)&src0, s0);

    ret = GiGetLowFloat32(src0);
    auto r = (float*)&ret;

    ASSERT_EQ(*r, s0[0]);
    ASSERT_EQ(*(r + 1), s0[1]);
}

TEST_F(FALLBACK, GiGetHighFloat32) {
    float32x2_t ret;
    GI_FLOAT32_t src0;
    std::vector<float> s0{1.0f, 2.2f, 3.4f, 4.5f};
    s0.resize(SIMD_LEN);
    init((float*)&src0, s0);

    ret = GiGetHighFloat32(src0);
    auto r = (float*)&ret;

    ASSERT_EQ(*r, s0[2]);
    ASSERT_EQ(*(r + 1), s0[3]);
}

TEST_F(FALLBACK, GiPaddFloat32) {
    float32x2_t src0, src1, ret;
    std::vector<float> s0{1.1f, -3.1415f};
    std::vector<float> s1{2.3f, 3.14777f};
    memcpy(&src0, s0.data(), sizeof(float32x2_t));
    memcpy(&src1, s1.data(), sizeof(float32x2_t));

    ret = GiPaddFloat32(src0, src1);

    std::vector<float> naive;
    naive.push_back(s0[0] + s0[1]);
    naive.push_back(s1[0] + s1[1]);

    auto r = (float*)&ret;
    ASSERT_LT(std::abs(naive[0] - r[0]), 1e-3);
    ASSERT_LT(std::abs(naive[1] - r[1]), 1e-3);
}

TEST_F(FALLBACK, GiPmaxFloat32) {
    float32x2_t src0, src1, ret;
    std::vector<float> s0{1.1f, -3.1415f};
    std::vector<float> s1{2.3f, 3.14777f};
    memcpy(&src0, s0.data(), sizeof(float32x2_t));
    memcpy(&src1, s1.data(), sizeof(float32x2_t));

    ret = GiPmaxFloat32(src0, src1);

    std::vector<float> naive;
    auto t0 = MAX_NAN(s0[0], s0[1]);
    auto t1 = MAX_NAN(s1[0], s1[1]);
    naive.push_back(t0);
    naive.push_back(t1);

    auto r = (float*)&ret;
    ASSERT_LT(std::abs(naive[0] - r[0]), 1e-3);
    ASSERT_LT(std::abs(naive[1] - r[1]), 1e-3);
}

TEST_F(FALLBACK, GiStoreZipFloat32V2) {
    GI_FLOAT32_V2_t src0;
    std::vector<float> s0{1.1f, 2.2f, 3.5f, 4.9f, 2312.1f, 345.244f, 3.59f, -12.8f};
    s0.resize(SIMD_LEN * 2);
    init((float*)&src0, s0, SIMD_LEN * 2);
    std::vector<float> ret;
    ret.resize(SIMD_LEN * 2);
    std::vector<float> ret_cmp;
    ret_cmp.resize(SIMD_LEN * 2);

    GiStoreZipFloat32V2(ret.data(), src0);

    GI_FLOAT32_V2_t tmp;
    tmp = GiZipqFloat32(src0.val[0], src0.val[1]);
    GiStoreFloat32(ret_cmp.data(), tmp.val[0]);
    GiStoreFloat32(ret_cmp.data() + SIMD_LEN, tmp.val[1]);

    assert_eq(ret.data(), ret_cmp, SIMD_LEN * 2);
}

TEST_F(FALLBACK, GiLoadUzipFloat32V3) {
    GI_FLOAT32_V3_t ret;
    std::vector<float> s0{1.1f,  2.2f,   3.5f, 4.9f, 2312.1f, 345.244f,
                          3.59f, -12.8f, 2.2f, 6.0f, 90.0f,   89.3f};
    s0.resize(SIMD_LEN * 3);

    ret = GiLoadUzipFloat32V3(s0.data());
    std::vector<float> naive;
    for (size_t i = 0; i < 3; i++) {
        naive.push_back(s0[0 + i]);
        naive.push_back(s0[3 + i]);
        naive.push_back(s0[6 + i]);
        naive.push_back(s0[9 + i]);
    }

    assert_eq((float*)&ret, naive);
}

TEST_F(FALLBACK, GiStoreZipFloat32V3) {
    GI_FLOAT32_V3_t src0;
    std::vector<float> s0{1.1f,  2.2f,   3.5f,  4.9f,   2312.1f, 345.244f,
                          3.59f, -12.8f, 3.59f, -12.8f, 2.2f,    6.0};
    s0.resize(SIMD_LEN * 3);
    init((float*)&src0, s0, SIMD_LEN * 3);
    std::vector<float> ret;
    ret.resize(SIMD_LEN * 3);

    GiStoreZipFloat32V3(ret.data(), src0);

    std::vector<float> ret_cmp;
    for (size_t i = 0; i < SIMD_LEN; i++) {
        ret_cmp.push_back(s0[0 + i]);
        ret_cmp.push_back(s0[4 + i]);
        ret_cmp.push_back(s0[8 + i]);
    }

    assert_eq(ret.data(), ret_cmp, SIMD_LEN * 3);
}

}  // namespace test
}  // namespace megdnn

// vim: syntax=cpp.doxygen
