/* NEON implementation of sin, cos, exp and log

   Inspired by Intel Approximate Math library, and based on the
   corresponding algorithms of the cephes math library
*/

/* Copyright (C) 2011  Julien Pommier

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  (this is the zlib license)
*/

#include "./neon_mathfun.h"

namespace megdnn {
namespace arm_common {

#define c_inv_mant_mask ~0x7f800000u
#define c_cephes_SQRTHF 0.707106781186547524
#define c_cephes_log_p0 7.0376836292E-2
#define c_cephes_log_p1 -1.1514610310E-1
#define c_cephes_log_p2 1.1676998740E-1
#define c_cephes_log_p3 -1.2420140846E-1
#define c_cephes_log_p4 +1.4249322787E-1
#define c_cephes_log_p5 -1.6668057665E-1
#define c_cephes_log_p6 +2.0000714765E-1
#define c_cephes_log_p7 -2.4999993993E-1
#define c_cephes_log_p8 +3.3333331174E-1
#define c_cephes_log_q1 -2.12194440e-4
#define c_cephes_log_q2 0.693359375

/**
 * natural logarithm computed for 4 simultaneous float return NaN for x <= 0
 */
v4sf log_ps_f32(v4sf x) {
    v4sf one = vdupq_n_f32(1);

    x = vmaxq_f32(x, vdupq_n_f32(0)); /* force flush to zero on denormal values */
    v4su invalid_mask = vcleq_f32(x, vdupq_n_f32(0));

    v4si ux = vreinterpretq_s32_f32(x);

    v4si emm0 = vshrq_n_s32(ux, 23);

    /* keep only the fractional part */
    ux = vandq_s32(ux, vdupq_n_s32(c_inv_mant_mask));
    ux = vorrq_s32(ux, vreinterpretq_s32_f32(vdupq_n_f32(0.5f)));
    x = vreinterpretq_f32_s32(ux);

    emm0 = vsubq_s32(emm0, vdupq_n_s32(0x7f));
    v4sf e = vcvtq_f32_s32(emm0);

    e = vaddq_f32(e, one);

    /* part2:
     *     if( x < SQRTHF ) {
     *       e -= 1;
     *       x = x + x - 1.0;
     *     } else { x = x - 1.0; }
     */
    v4su mask = vcltq_f32(x, vdupq_n_f32(c_cephes_SQRTHF));
    v4sf tmp = vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(x), mask));
    x = vsubq_f32(x, one);
    e = vsubq_f32(
            e, vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(one), mask)));
    x = vaddq_f32(x, tmp);

    v4sf z = vmulq_f32(x, x);

    v4sf y = vdupq_n_f32(c_cephes_log_p0);
    y = fma_ps_f32(vdupq_n_f32(c_cephes_log_p1), y, x);
    y = fma_ps_f32(vdupq_n_f32(c_cephes_log_p2), y, x);
    y = fma_ps_f32(vdupq_n_f32(c_cephes_log_p3), y, x);
    y = fma_ps_f32(vdupq_n_f32(c_cephes_log_p4), y, x);
    y = fma_ps_f32(vdupq_n_f32(c_cephes_log_p5), y, x);
    y = fma_ps_f32(vdupq_n_f32(c_cephes_log_p6), y, x);
    y = fma_ps_f32(vdupq_n_f32(c_cephes_log_p7), y, x);
    y = fma_ps_f32(vdupq_n_f32(c_cephes_log_p8), y, x);
    y = vmulq_f32(y, x);

    y = vmulq_f32(y, z);

    y = fma_ps_f32(y, e, vdupq_n_f32(c_cephes_log_q1));

    y = vmlsq_f32(y, z, vdupq_n_f32(0.5f));

    x = vaddq_f32(x, y);
    x = fma_ps_f32(x, e, vdupq_n_f32(c_cephes_log_q2));
    x = vreinterpretq_f32_u32(vorrq_u32(
            vreinterpretq_u32_f32(x), invalid_mask));  // negative arg will be NAN
    return x;
}

#define c_exp_hi 88.3762626647949f
#define c_exp_lo -88.3762626647949f

#define c_cephes_LOG2EF 1.44269504088896341
#define c_cephes_exp_C1 0.693359375
#define c_cephes_exp_C2 -2.12194440e-4

#define c_cephes_exp_p0 1.9875691500E-4
#define c_cephes_exp_p1 1.3981999507E-3
#define c_cephes_exp_p2 8.3334519073E-3
#define c_cephes_exp_p3 4.1665795894E-2
#define c_cephes_exp_p4 1.6666665459E-1
#define c_cephes_exp_p5 5.0000001201E-1

/* exp() computed for 4 float at once */
v4sf exp_ps_f32(v4sf x) {
    v4sf tmp, fx;

    v4sf one = vdupq_n_f32(1);
    x = vminq_f32(x, vdupq_n_f32(c_exp_hi));
    x = vmaxq_f32(x, vdupq_n_f32(c_exp_lo));

    /* express exp(x) as exp(g + n*log(2)) */
    fx = fma_ps_f32(vdupq_n_f32(0.5f), x, vdupq_n_f32(c_cephes_LOG2EF));

    /* perform a floorf */
    tmp = vcvtq_f32_s32(vcvtq_s32_f32(fx));

    /* if greater, subtract 1 */
    v4su mask = vcgtq_f32(tmp, fx);
    mask = vandq_u32(mask, vreinterpretq_u32_f32(one));

    fx = vsubq_f32(tmp, vreinterpretq_f32_u32(mask));

    tmp = vmulq_f32(fx, vdupq_n_f32(c_cephes_exp_C1));
    v4sf z = vmulq_f32(fx, vdupq_n_f32(c_cephes_exp_C2));
    x = vsubq_f32(x, tmp);
    x = vsubq_f32(x, z);

    z = vmulq_f32(x, x);

    v4sf y = vdupq_n_f32(c_cephes_exp_p0);
    y = fma_ps_f32(vdupq_n_f32(c_cephes_exp_p1), y, x);
    y = fma_ps_f32(vdupq_n_f32(c_cephes_exp_p2), y, x);
    y = fma_ps_f32(vdupq_n_f32(c_cephes_exp_p3), y, x);
    y = fma_ps_f32(vdupq_n_f32(c_cephes_exp_p4), y, x);
    y = fma_ps_f32(vdupq_n_f32(c_cephes_exp_p5), y, x);

    y = fma_ps_f32(x, y, z);
    y = vaddq_f32(y, one);

    /* build 2^n */
    v4si mm;
    mm = vcvtq_s32_f32(fx);
    mm = vaddq_s32(mm, vdupq_n_s32(0x7f));
    mm = vshlq_n_s32(mm, 23);
    v4sf pow2n = vreinterpretq_f32_s32(mm);

    y = vmulq_f32(y, pow2n);
    return y;
}

#if __ARM_FEATURE_FP16_VECTOR_ARITHMETIC
float16x8_t exp_ps_f16(float16x8_t x) {
    float32x4_t low = vcvt_f32_f16(vget_low_f16(x));
    float32x4_t high = vcvt_f32_f16(vget_high_f16(x));
    low = exp_ps_f32(low);
    high = exp_ps_f32(high);

    return vcombine_f16(vcvt_f16_f32(low), vcvt_f16_f32(high));
}
#endif

#define c_minus_cephes_DP1 -0.78515625
#define c_minus_cephes_DP2 -2.4187564849853515625e-4
#define c_minus_cephes_DP3 -3.77489497744594108e-8
#define c_sincof_p0        -1.9515295891E-4
#define c_sincof_p1        8.3321608736E-3
#define c_sincof_p2        -1.6666654611E-1
#define c_coscof_p0        2.443315711809948E-005
#define c_coscof_p1        -1.388731625493765E-003
#define c_coscof_p2        4.166664568298827E-002
#define c_cephes_FOPI      1.27323954473516  // 4 / M_PI

/* evaluation of 4 sines & cosines at once.

   The code is the exact rewriting of the cephes sinf function.
   Precision is excellent as long as x < 8192 (I did not bother to
   take into account the special handling they have for greater values
   -- it does not return garbage for arguments over 8192, though, but
   the extra precision is missing).

   Note that it is such that sinf((float)M_PI) = 8.74e-8, which is the
   surprising but correct result.

   Note also that when you compute sin(x), cos(x) is available at
   almost no extra price so both sin_ps_f32 and cos_ps_f32 make use of
   sincos_ps_f32..
  */
void sincos_ps_f32(v4sf x, v4sf* ysin, v4sf* ycos) {
    // any x
    v4sf y;

    v4su emm2;

    v4su sign_mask_sin, sign_mask_cos;
    sign_mask_sin = vcltq_f32(x, vdupq_n_f32(0));
    x = vabsq_f32(x);

    /* scale by 4/Pi */
    y = vmulq_f32(x, vdupq_n_f32(c_cephes_FOPI));

    /* store the integer part of y in mm0 */
    emm2 = vcvtq_u32_f32(y);
    /* j=(j+1) & (~1) (see the cephes sources) */
    emm2 = vaddq_u32(emm2, vdupq_n_u32(1));
    emm2 = vandq_u32(emm2, vdupq_n_u32(~1));
    y = vcvtq_f32_u32(emm2);

    /* get the polynom selection mask
     *     there is one polynom for 0 <= x <= Pi/4
     *     and another one for Pi/4<x<=Pi/2
     *
     *     Both branches will be computed.
     */
    v4su poly_mask = vtstq_u32(emm2, vdupq_n_u32(2));

    /* The magic pass: "Extended precision modular arithmetic"
     *     x = ((x - y * DP1) - y * DP2) - y * DP3; */
    x = fma_ps_f32(x, y, vdupq_n_f32(c_minus_cephes_DP1));
    x = fma_ps_f32(x, y, vdupq_n_f32(c_minus_cephes_DP2));
    x = fma_ps_f32(x, y, vdupq_n_f32(c_minus_cephes_DP3));

    sign_mask_sin = veorq_u32(sign_mask_sin, vtstq_u32(emm2, vdupq_n_u32(4)));
    sign_mask_cos = vtstq_u32(vsubq_u32(emm2, vdupq_n_u32(2)), vdupq_n_u32(4));

    /* Evaluate the first polynom  (0 <= x <= Pi/4) in y1,
     *     and the second polynom      (Pi/4 <= x <= 0) in y2 */
    v4sf z = vmulq_f32(x, x);
    v4sf y1, y2;

    y1 = fma_ps_f32(vdupq_n_f32(c_coscof_p1), z, vdupq_n_f32(c_coscof_p0));
    y2 = fma_ps_f32(vdupq_n_f32(c_sincof_p1), z, vdupq_n_f32(c_sincof_p0));
    y1 = fma_ps_f32(vdupq_n_f32(c_coscof_p2), y1, z);
    y2 = fma_ps_f32(vdupq_n_f32(c_sincof_p2), y2, z);
    y1 = vmulq_f32(y1, z);
    y2 = vmulq_f32(y2, z);
    y1 = vmulq_f32(y1, z);
    y1 = vmlsq_f32(y1, z, vdupq_n_f32(0.5f));
    y2 = fma_ps_f32(x, y2, x);
    y1 = vaddq_f32(y1, vdupq_n_f32(1));

    /* select the correct result from the two polynoms */
    v4sf ys = vbslq_f32(poly_mask, y1, y2);
    v4sf yc = vbslq_f32(poly_mask, y2, y1);
    *ysin = vbslq_f32(sign_mask_sin, vnegq_f32(ys), ys);
    *ycos = vbslq_f32(sign_mask_cos, yc, vnegq_f32(yc));
}

v4sf sin_ps_f32(v4sf x) {
    v4sf ysin, ycos;
    sincos_ps_f32(x, &ysin, &ycos);
    return ysin;
}

v4sf cos_ps_f32(v4sf x) {
    v4sf ysin, ycos;
    sincos_ps_f32(x, &ysin, &ycos);
    return ycos;
}

v4sf tan_ps_f32(v4sf x) {
    v4sf ysin, ycos;
    sincos_ps_f32(x, &ysin, &ycos);
    return ysin / ycos;
}

#undef c_exp_hi
#undef c_exp_lo
#undef c_cephes_LOG2EF
#undef c_cephes_exp_C1
#undef c_cephes_exp_C2
#undef c_cephes_exp_p0
#undef c_cephes_exp_p1
#undef c_cephes_exp_p2
#undef c_cephes_exp_p3
#undef c_cephes_exp_p4
#undef c_cephes_exp_p5

#undef c_minus_cephes_DP1
#undef c_minus_cephes_DP2
#undef c_minus_cephes_DP3
#undef c_sincof_p0
#undef c_sincof_p1
#undef c_sincof_p2
#undef c_coscof_p0
#undef c_coscof_p1
#undef c_coscof_p2
#undef c_cephes_FOPI

#undef c_inv_mant_mask
#undef c_cephes_SQRTHF
#undef c_cephes_log_p0
#undef c_cephes_log_p1
#undef c_cephes_log_p2
#undef c_cephes_log_p3
#undef c_cephes_log_p4
#undef c_cephes_log_p5
#undef c_cephes_log_p6
#undef c_cephes_log_p7
#undef c_cephes_log_p8
#undef c_cephes_log_q1
#undef c_cephes_log_q2

static const struct {
    float lower_range;
    float upper_range;
    float alpha_9;
    float alpha_7;
    float alpha_5;
    float alpha_3;
    float alpha_1;
    float beta_10;
    float beta_8;
    float beta_6;
    float beta_4;
    float beta_2;
    float beta_0;
    float one_half;
} sigmoid_constants = {
        -18.0f,
        18.0f,
        4.37031012579801e-11f,
        1.15627324459942e-07f,
        6.08574864600143e-05f,
        8.51377133304701e-03f,
        2.48287947061529e-01f,
        6.10247389755681e-13f,
        5.76102136993427e-09f,
        6.29106785017040e-06f,
        1.70198817374094e-03f,
        1.16817656904453e-01f,
        9.93151921023180e-01f,
        0.5f,
};

v4sf sigmoid_ps_f32(v4sf src) {
    auto val = vmaxq_f32(vdupq_n_f32(sigmoid_constants.lower_range), src);
    val = vminq_f32(vdupq_n_f32(sigmoid_constants.upper_range), val);
    auto squared = vmulq_f32(val, val);
    auto p = fma_ps_f32(
            vdupq_n_f32(sigmoid_constants.alpha_7), squared,
            vdupq_n_f32(sigmoid_constants.alpha_9));
    p = fma_ps_f32(vdupq_n_f32(sigmoid_constants.alpha_5), p, squared);
    p = fma_ps_f32(vdupq_n_f32(sigmoid_constants.alpha_3), p, squared);
    p = fma_ps_f32(vdupq_n_f32(sigmoid_constants.alpha_1), p, squared);
    p = vmulq_f32(p, val);
    auto q = fma_ps_f32(
            vdupq_n_f32(sigmoid_constants.beta_8), squared,
            vdupq_n_f32(sigmoid_constants.beta_10));
    q = fma_ps_f32(vdupq_n_f32(sigmoid_constants.beta_6), q, squared);
    q = fma_ps_f32(vdupq_n_f32(sigmoid_constants.beta_4), q, squared);
    q = fma_ps_f32(vdupq_n_f32(sigmoid_constants.beta_2), q, squared);
    q = fma_ps_f32(vdupq_n_f32(sigmoid_constants.beta_0), q, squared);
    return vaddq_f32(div_ps_f32(p, q), vdupq_n_f32(sigmoid_constants.one_half));
}

#if __ARM_FEATURE_FP16_VECTOR_ARITHMETIC
float16x8_t sigmoid_ps_f16(float16x8_t x) {
    float32x4_t low = vcvt_f32_f16(vget_low_f16(x));
    float32x4_t high = vcvt_f32_f16(vget_high_f16(x));
    low = sigmoid_ps_f32(low);
    high = sigmoid_ps_f32(high);
    return vcombine_f16(vcvt_f16_f32(low), vcvt_f16_f32(high));
}
#endif

}  // namespace arm_common
}  // namespace megdnn

// vim: syntax=cpp.doxygen
