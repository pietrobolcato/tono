/* h1 - small math kit. Single precision only: the FM-1's FPU has no doubles and
 * the firmware links no libm, so everything the core needs lives here. */
#pragma once
#include <stdint.h>

/* Share control-rate coefficient math across the sixteen instrument models. */
#ifdef TONO_A22_FM1
#define A22_COEFFICIENT __attribute__((noinline))
#else
#define A22_COEFFICIENT inline
#endif

typedef float a22_f32;

#define a22_H1_PI 3.14159265f
#define a22_H1_TAU 6.28318531f

static inline a22_f32 a22_clampf(a22_f32 x, a22_f32 lo, a22_f32 hi) { return x < lo ? lo : (x > hi ? hi : x); }
static inline a22_f32 a22_sat(a22_f32 x) { return x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x); }
static inline a22_f32 a22_lerpf(a22_f32 a, a22_f32 b, a22_f32 t) { return a + (b - a) * t; }
static inline a22_f32 a22_absf(a22_f32 x) { return x < 0.0f ? -x : x; }
static inline a22_f32 a22_minf(a22_f32 a, a22_f32 b) { return a < b ? a : b; }
static inline a22_f32 a22_maxf(a22_f32 a, a22_f32 b) { return a > b ? a : b; }
static inline int32_t a22_imin(int32_t a, int32_t b) { return a < b ? a : b; }
static inline int32_t a22_imax(int32_t a, int32_t b) { return a > b ? a : b; }
static inline int32_t a22_iclamp(int32_t x, int32_t lo, int32_t hi) { return x < lo ? lo : (x > hi ? hi : x); }

static inline int32_t a22_ifloor(a22_f32 x)
{
    int32_t i = (int32_t)x;
    return (a22_f32)i > x ? i - 1 : i;
}
static inline a22_f32 a22_fractf(a22_f32 x) { return x - (a22_f32)a22_ifloor(x); }

/* 1/sqrt(x), x > 0: the bit trick and two Newton steps (rel. error ~5e-6) */
static inline a22_f32 a22_rsqrtf(a22_f32 x)
{
    union { a22_f32 f; uint32_t u; } v;
    a22_f32 y;
    v.f = x;
    v.u = 0x5f3759dfu - (v.u >> 1);
    y = v.f;
    y = y * (1.5f - 0.5f * x * y * y);
    y = y * (1.5f - 0.5f * x * y * y);
    return y;
}
static inline a22_f32 a22_sqrtfx(a22_f32 x) { return x <= 1e-12f ? 0.0f : x * a22_rsqrtf(x); }

/* smooth curves */
static inline a22_f32 a22_smooth01(a22_f32 t) { t = a22_sat(t); return t * t * (3.0f - 2.0f * t); }
static inline a22_f32 a22_ease_out(a22_f32 t) { t = 1.0f - a22_sat(t); return 1.0f - t * t * t; }

/* sin(2*pi*t), t in turns, any value. A degree-9 odd polynomial on a quarter
 * turn: |error| < 4e-6. Used to build the lookup table and by the graphics. */
static inline a22_f32 a22_sin_turns(a22_f32 t)
{
    /* Keep each multiply and add rounded separately, including on hosts that
     * otherwise contract this boot polynomial into fused operations. */
    #pragma STDC FP_CONTRACT OFF
    a22_f32 x, x2;
    t -= (a22_f32)a22_ifloor(t);                       /* 0..1 */
    if (t > 0.5f) t -= 1.0f;                   /* -0.5..0.5 */
    if (t > 0.25f) t = 0.5f - t;               /* fold to -0.25..0.25 */
    else if (t < -0.25f) t = -0.5f - t;
    x = t * a22_H1_TAU;
    x2 = x * x;
    return x * (1.0f + x2 * (-0.16666667f + x2 * (0.0083333310f + x2 * (-0.00019840874f + x2 * 2.7525562e-06f))));
}
static inline a22_f32 a22_cos_turns(a22_f32 t) { return a22_sin_turns(t + 0.25f); }

/* 2^x: integer part by exponent bits, fraction by a degree-5 polynomial (rel. error < 2e-6).
 * Any x: below 2^-126 it is 0, above 2^126 it stays there, and not-a-number is 0. (It used to
 * trust x to be small: a decay written as a22_exp2fx(-age * 30) wrapped the exponent bits four
 * seconds into a held note and came back as an enormous number.) */
static A22_COEFFICIENT a22_f32 a22_exp2fx(a22_f32 x)
{
    union { a22_f32 f; int32_t i; } v;
    int32_t n;
    a22_f32 f, p;
    if (!(x > -126.0f)) return 0.0f;
    if (x > 126.0f) x = 126.0f;
    n = a22_ifloor(x + 0.5f);
    f = x - (a22_f32)n;
    p = 1.3333558e-3f;
    p = p * f + 9.6181291e-3f;
    p = p * f + 5.5504109e-2f;
    p = p * f + 2.4022651e-1f;
    p = p * f + 6.9314718e-1f;
    p = p * f + 1.0f;
    v.f = p;
    v.i += n * 8388608;                          /* (n << 23, for a negative n too) */
    return v.f;
}

/* tan(x) for 0 <= x < ~1.3 (filter prewarp): Pade [3/2] */
static inline a22_f32 a22_tanfx(a22_f32 x)
{
    a22_f32 x2 = x * x;
    return x * (15.0f - x2) / (15.0f - 6.0f * x2);
}

/* soft clip, monotone, |y| < 1 */
static inline a22_f32 a22_softclip(a22_f32 x)
{
    if (x > 3.0f) return 1.0f;
    if (x < -3.0f) return -1.0f;
    if (x != x) return 0.0f;                     /* not a number: silence, not whatever the cast makes of it */
    return x * (27.0f + x * x) / (27.0f + 9.0f * x * x);
}

/* xorshift32 */
static inline uint32_t a22_rng_next(uint32_t *s)
{
    uint32_t x = *s;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return *s = x;
}
static inline a22_f32 a22_rng_f(uint32_t *s) { return (a22_f32)(a22_rng_next(s) >> 8) * (1.0f / 16777216.0f); }   /* 0..1 */
static inline a22_f32 a22_rng_s(uint32_t *s) { return a22_rng_f(s) * 2.0f - 1.0f; }                            /* -1..1 */

