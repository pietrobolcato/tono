/* A22 - Studio DSP toolkit. Header-only, single-precision float, no libm. */
#pragma once
#include "a22_math.h"

#define a22_SIN_N 2048
#include "a22_sine.h"
static inline a22_f32 a22_sine_entry(int i) {
    i &= 2047;
    int j = i & 1023;
    if (j > 512) j = 1024 - j;
    return i > 1024 ? -a22_quarter[j] : a22_quarter[j];
}

/* sin(2*pi*t) from table, t in turns, any finite value (|t| < 2^20). The turn's whole part is not taken off first:
 * scaling by a22_SIN_N is exact and the mask drops the whole turns, with one float-to-int conversion where there were
 * two. For t >= 0 that is the same to the bit as taking it off; for t < 0 it is a little more exact (t + 1 rounded
 * away some of a small negative t's bits). */
static inline a22_f32 a22_tsin(a22_f32 t)
{
    a22_f32 x = t * (a22_f32)a22_SIN_N;
    int32_t i = a22_ifloor(x);
    x -= (a22_f32)i;
    i &= a22_SIN_N - 1;
    a22_f32 a = a22_sine_entry(i);
    return a + (a22_sine_entry(i + 1) - a) * x;
}

static inline a22_f32 a22_tcos(a22_f32 t) { return a22_tsin(t + 0.25f); }

/* MIDI note to Hz (fractional notes fine) */
static inline a22_f32 a22_mtof(a22_f32 n) { return 440.0f * a22_exp2fx((n - 69.0f) * (1.0f / 12.0f)); }

/* polynomial band-limited step (PolyBLEP) */
static inline a22_f32 a22_blep(a22_f32 t, a22_f32 dt)
{
    if (t < dt) {
        t /= dt;
        return t + t - t * t - 1.0f;
    }
    if (t > 1.0f - dt) {
        t = (t - 1.0f) / dt;
        return t * t + t + t + 1.0f;
    }
    return 0.0f;
}

static inline a22_f32 a22_saw_bl(a22_f32 ph, a22_f32 dt) { return 2.0f * ph - 1.0f - a22_blep(ph, dt); }

static inline a22_f32 a22_pulse_bl(a22_f32 ph, a22_f32 dt, a22_f32 duty)
{
    a22_f32 p2 = ph + 1.0f - duty;
    if (p2 >= 1.0f) p2 -= 1.0f;
    return (ph < duty ? 1.0f : -1.0f) + a22_blep(ph, dt) - a22_blep(p2, dt) - (2.0f * duty - 1.0f);
}

static inline a22_f32 a22_tri_wave(a22_f32 ph) { return 4.0f * a22_absf(ph - 0.5f) - 1.0f; }

/* asymmetric warm tape / tube saturation (adds musical 2nd + 3rd harmonics) */
static inline a22_f32 a22_tape_sat(a22_f32 x)
{
    a22_f32 y = a22_softclip(x + 0.08f * x * x);
    return y - 0.075f;
}

/* smooth sine wavefolder */
static inline a22_f32 a22_fold_sin(a22_f32 x) { return a22_tsin(x * 0.25f); }

/* equal-power stereo pan law, pan in [-1, 1] */
static inline void a22_pan_gains(a22_f32 pan, a22_f32 *gl, a22_f32 *gr)
{
    a22_f32 t = a22_clampf(pan, -1.0f, 1.0f) * 0.125f + 0.125f;   /* 0 .. 0.25 turns */
    *gl = a22_tcos(t);
    *gr = a22_tsin(t);
}

/* ZDF Trapezoidal State-Variable Filter (12 dB/oct LP/BP/HP/Notch) */
typedef struct a22_Svf { a22_f32 g, k, a1, a2, a3; } a22_Svf;
static inline void a22_svf_set(a22_Svf *f, a22_f32 fc, a22_f32 q, a22_f32 isr)
{
    a22_f32 x = fc * isr;
    if (x > 0.42f) x = 0.42f;
    if (x < 0.0002f) x = 0.0002f;
    f->g = a22_tanfx(a22_H1_PI * x);
    f->k = 1.0f / a22_maxf(0.5f, q);
    f->a1 = 1.0f / (1.0f + f->g * (f->g + f->k));
    f->a2 = f->g * f->a1;
    f->a3 = f->g * f->a2;
}
#define a22_SVF_TICK(f, s, x, lp, bp) do { \
        a22_f32 v3_ = (x) - (s)[1]; \
        a22_f32 v1_ = (f).a1 * (s)[0] + (f).a2 * v3_; \
        a22_f32 v2_ = (s)[1] + (f).a2 * (s)[0] + (f).a3 * v3_; \
        (s)[0] = 2.0f * v1_ - (s)[0]; \
        (s)[1] = 2.0f * v2_ - (s)[1]; \
        (lp) = v2_; (bp) = v1_; \
    } while (0)

/* 24 dB/oct Zero-Delay-Feedback Non-Linear Analog a22_Ladder Filter (Moog/Digitone style) */
typedef struct a22_Ladder { a22_f32 g, k, comp; } a22_Ladder;
static inline void a22_ladder_set(a22_Ladder *f, a22_f32 fc, a22_f32 reso, a22_f32 isr)
{
    a22_f32 x = a22_clampf(fc * isr, 0.00025f, 0.40f);
    a22_f32 t = a22_tanfx(a22_H1_PI * x);
    f->g = t / (1.0f + t);
    f->k = a22_clampf(reso, 0.0f, 3.85f);
    f->comp = 0.48f * f->k;
}
static inline a22_f32 a22_ladder_tick(const a22_Ladder *f, a22_f32 *s, a22_f32 x)
{
    a22_f32 G = f->g, G2 = G * G, G3 = G2 * G, G4 = G2 * G2;
    a22_f32 S = G3 * s[0] + G2 * s[1] + G * s[2] + s[3];
    a22_f32 u = (x * (1.0f + f->comp) - f->k * S) / (1.0f + f->k * G4);
    a22_f32 v, y;
    u = a22_softclip(u);
    v = (u - s[0]) * G; y = v + s[0]; s[0] = y + v;
    v = (y - s[1]) * G; y = v + s[1]; s[1] = y + v;
    v = (y - s[2]) * G; y = v + s[2]; s[2] = y + v;
    v = (y - s[3]) * G; y = v + s[3]; s[3] = y + v;
    return y;
}

/* white noise from a 32-bit xorshift state */
static inline a22_f32 a22_noise_s(uint32_t *s) { return (a22_f32)(int32_t)a22_rng_next(s) * (1.0f / 2147483648.0f); }

/* one-pole smoothing coefficient for a time constant in seconds */
static inline a22_f32 a22_onepole_k(a22_f32 seconds, a22_f32 rate)
{
    if (seconds <= 0.0f) return 1.0f;
    return 1.0f - a22_exp2fx(-1.442695f / (seconds * rate));
}
