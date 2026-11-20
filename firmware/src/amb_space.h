/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Charles Vestal
 * Adapted from fm1-amb da0047dc, app/dsp/amb.c. Dattorro's figure-eight
 * plate, input diffusers and stereo observation taps, at half rate in the
 * existing shared reverb pool. Delay times are specified in seconds. */
#define PL(n) (((n) * 7409 + 5000) / 10000)
#define PL_LINES(X)                                                                                                  \
    X(PRE, 662, 0)                                                                                                   \
    X(I1, PL(142), PRE) X(I2, PL(107), I1) X(I3, PL(379), I2) X(I4, PL(277), I3) X(AL, PL(672) + PL(16) + 2, I4)     \
        X(DL, PL(4453), AL) X(BL, PL(1800), DL) X(EL, PL(3720), BL) X(AR, PL(908) + PL(16) + 2, EL)                  \
            X(DR, PL(4217), AR) X(BR, PL(2656), DR) X(ER, PL(3163), BR)
#define PL_ENUM(id, n, prev) PL_##id,
enum { PL_LINES(PL_ENUM) PL_COUNT };
#define PL_SIZE(id, n, prev) n,
static const uint16_t pl_len[] = {PL_LINES(PL_SIZE)};
#define PL_END(id, n, prev) PL_END_##id = PL_END_##prev + n + 1,
enum { PL_END_0 = 0, PL_LINES(PL_END) };
_Static_assert(PL_END_BR <= CL_LEN && 4096u <= NELEM(rev_comb) && PL_END_ER - PL_END_BR <= 4096u, "plate fits shared reverb line");
#define PL_BASE(id, n, prev) PL_END_##prev,
static const uint16_t pl_base[] = {PL_LINES(PL_BASE)};
static struct {
    uint32_t cursor;
    uint32_t phase;
    int32_t bw, damp[2], left, right, input;
    unsigned half;
} plate;
static inline int pl_read(unsigned id, int lag)
{
    /* A shared descending cursor moves all delay lines one tick together.
     * Each virtual line owns length + 1 addresses relative to that cursor,
     * including its oldest still-readable sample. ER uses the second ring. */
    unsigned at = plate.cursor + (unsigned)lag;
    return id == PL_ER ? rev_comb[at & 4095u] : rev_x.cl[(at + pl_base[id]) & (CL_LEN - 1u)];
}
static inline void pl_write(unsigned id, int x)
{
    int16_t value = (int16_t)clamp(x, -32767, 32767);
    if (id == PL_ER) rev_comb[plate.cursor & 4095u] = value;
    else rev_x.cl[(plate.cursor + pl_base[id]) & (CL_LEN - 1u)] = value;
}
static inline int pl_pass(unsigned id, int x, int gain, int delayed)
{
    int y = x - mulq15(gain, delayed);
    pl_write(id, y);
    return delayed + mulq15(gain, y);
}
static inline int pl_mod(unsigned id, int lag, int lfo)
{
    int shift = PL(16) * lfo;
    lag += shift >> 15;
    int a = pl_read(id, lag);
    return a + mulq15(pl_read(id, lag + 1) - a, shift & 32767);
}
static __attribute__((noinline)) void pl_render(const int32_t *in, int32_t *out, unsigned frames)
{
    int decay = 19661 + GFX(GF_RSIZE) * 96, damping = clamp(32600 - GFX(GF_RDAMP) * 150, 5000, 32600);
    int diffusion = clamp(decay + 4915, 8192, 16384);
    int width = 128 + song.g[G_WIDTH];
    int excursion = 8192 + song.g[G_RMOD] * 192;
    for (unsigned i = 0; i < frames; i++) {
        if (plate.half) {
            int fbL = pl_read(PL_ER, PL(3163)), fbR = pl_read(PL_EL, PL(3720));
            int x = pl_read(PL_PRE, pl_len[PL_PRE]);
            pl_write(PL_PRE, (plate.input + in[i]) / 8);
            plate.bw += mulq15(clamp(x - plate.bw, -65535, 65535), 27525);
            x = plate.bw;
            for (unsigned d = PL_I1; d <= PL_I4; d++)
                x = pl_pass(d, x, d <= PL_I2 ? 24576 : 20480, pl_read(d, pl_len[d]));
            plate.phase += 136714u;
            for (unsigned side = 0; side < 2; side++) {
                unsigned ap = side ? PL_AR : PL_AL;
                int z = pl_mod(ap, side ? PL(908) : PL(672), mulq15(sine_i(plate.phase + side * 0x40000000u), excursion));
                int a = pl_pass(ap, x + mulq15(decay, side ? fbR : fbL), -22938, z);
                int b = pl_read(ap + 1, pl_len[ap + 1]);
                pl_write(ap + 1, a);
                plate.damp[side] += mulq15(clamp(b - plate.damp[side], -65535, 65535), damping);
                b = pl_pass(ap + 2, mulq15(plate.damp[side], decay), diffusion, pl_read(ap + 2, pl_len[ap + 2]));
                pl_write(ap + 3, b);
            }
            /* Observation taps see the newly written tick. */
            plate.cursor--;
            plate.left = pl_read(PL_DR, PL(266)) + pl_read(PL_DR, PL(2974)) - pl_read(PL_BR, PL(1913)) +
                         pl_read(PL_ER, PL(1996)) - pl_read(PL_DL, PL(1990)) - pl_read(PL_BL, PL(187)) -
                         pl_read(PL_EL, PL(1066));
            plate.right = pl_read(PL_DL, PL(353)) + pl_read(PL_DL, PL(3627)) - pl_read(PL_BL, PL(1228)) +
                          pl_read(PL_EL, PL(2673)) - pl_read(PL_DR, PL(2111)) - pl_read(PL_BR, PL(335)) -
                          pl_read(PL_ER, PL(121));
        } else
            plate.input = in[i];
        plate.half ^= 1;
        out[i] += (plate.left + plate.right) * 3 / 4;
        int side = (plate.left - plate.right) * 3 * width / 512;
        fx_sl[i] += side;
        fx_sr[i] -= side;
    }
}
/* The quarter-rate tape uses the existing 64 KiB delay line as 8-bit
 * companded samples. Its quietest code step is eight int16 units. The short
 * machine has its own half-rate line so either head or both can be patched. */
#define TAPE_LENGTH 62000u
_Static_assert(sizeof dly_u >= TAPE_LENGTH, "two-head tape fits delay pool");
static int16_t plex_line[3550] __attribute__((section(".pool")));
static struct {
    unsigned kind, shortpos, shortfill, w, fill, phase, mod;
    int shortinput, lp, rp, acc, fb, hp, h1, h2, prev1, prev2;
    uint32_t length, head_clock, head_fraction, head_target;
} tape_fx;
static void tape_reset(unsigned kind)
{
    memset(&tape_fx,0,sizeof tape_fx);
    tape_fx.kind=kind; /* fill counters invalidate the short and long rings immediately */
}
static uint8_t tape_pack(int v)
{
    unsigned a = (v < 0 ? -v : v) + 132u;
    if (a > 32767)
        a = 32767;
    unsigned exponent = a < 256 ? 0u : 24u - (unsigned)__builtin_clz(a);
    return (v < 0 ? 128 : 0) | (exponent << 4) | ((a >> (exponent + 3)) & 15);
}
static int tape_unpack(uint8_t b)
{
    int x = (((b & 15) * 8 + 132) << ((b >> 4) & 7)) - 132;
    return b & 128 ? -x : x;
}
static int tape_read_head(unsigned distance)
{
    unsigned d = distance >> 8;
    if (d + 1 >= tape_fx.fill)
        return 0;
    unsigned at = tape_fx.w >= d ? tape_fx.w - d : tape_fx.w + TAPE_LENGTH - d;
    const uint8_t *buf = (const uint8_t *)&dly_u;
    int a = tape_unpack(buf[at]), b = tape_unpack(buf[at ? at - 1 : TAPE_LENGTH - 1]);
    return a + ((b - a) * (int)(distance & 255) >> 8);
}
static __attribute__((noinline)) void tape_run(unsigned kind, const int32_t *in, int32_t *left, int32_t *right,
                                               unsigned n, unsigned length, int feedback)
{
    unsigned target = clamp(length / 4, 3, TAPE_LENGTH - 8) << 8;
    if (!tape_fx.length) {
        tape_fx.length = target;
        tape_fx.head_fraction = tape_fx.head_target = 49152;
    }
    unsigned shortlen = clamp((int)(3550u * (length / 16u) / (div_samples(7) / 16u)), 2, 3550);
    unsigned short_right = shortlen * 27u / 71u;
    int motion = clamp(song.g[G_DMOD], 0, 127);
    for (unsigned i = 0; i < n; i++) {
        if (!(tape_fx.phase & 1))
            tape_fx.shortinput = in[i];
        if ((tape_fx.phase & 1) && kind != 3) {
            unsigned at = tape_fx.shortpos, other = at >= short_right ? at - short_right : at + 3550 - short_right;
            int a = tape_fx.shortfill >= shortlen ? plex_line[at >= shortlen ? at - shortlen : at + 3550 - shortlen] : 0,
                b = tape_fx.shortfill >= short_right ? plex_line[other] : 0;
            tape_fx.lp += (a - tape_fx.lp) * 19 / 32;
            tape_fx.rp += (b - tape_fx.rp) * 19 / 32;
            plex_line[at] = clamp((in[i] + tape_fx.shortinput) / 4 + mulq15(tape_fx.lp, feedback), -32767, 32767);
            tape_fx.shortpos = at + 1 == 3550 ? 0 : at + 1;
            if (tape_fx.shortfill < 3550)
                tape_fx.shortfill++;
        }
        if (kind != 2) {
            tape_fx.acc += in[i] / 2 + (kind == 4 ? tape_fx.lp * 3 / 10 : 0);
            if (tape_fx.phase == 3) {
                tape_fx.length += (int32_t)(target - tape_fx.length) / 1024;
                tape_fx.mod += 34137u;
                if (++tape_fx.head_clock % (TAPE_LENGTH * 6u) == 0 && motion) {
                    static const uint16_t positions[] = {49152, 43691, 40960, 32768, 24576};
                    tape_fx.head_target = positions[(tape_fx.mod >> 8) % 5];
                }
                if (!(tape_fx.head_clock & 127u) && tape_fx.head_target != tape_fx.head_fraction)
                    tape_fx.head_fraction += tape_fx.head_target > tape_fx.head_fraction ? 1u : (uint32_t)-1;
                int wow = mulq15(sine_i(tape_fx.mod), motion * 6);
                unsigned d = clamp((int)tape_fx.length + ((int)(tape_fx.length >> 12) * wow >> 5), 512,
                                   (TAPE_LENGTH - 4) * 256);
                tape_fx.prev1 = tape_fx.h1;
                tape_fx.prev2 = tape_fx.h2;
                tape_fx.h1 = tape_read_head(d);
                tape_fx.h2 = tape_read_head((d >> 8) * (tape_fx.head_fraction >> 8) +
                                            ((d & 255u) * (tape_fx.head_fraction >> 8) >> 8));
                tape_fx.fb += (tape_fx.h1 + (tape_fx.h2 - tape_fx.h1) / 8 - tape_fx.fb) *
                              (35 + clamp(GFX(GF_DCOL), 0, 127) / 3) / 100;
                tape_fx.hp += (tape_fx.fb - tape_fx.hp) * 3 / 100;
                tape_fx.w = tape_fx.w + 1 == TAPE_LENGTH ? 0 : tape_fx.w + 1;
                ((uint8_t *)&dly_u)[tape_fx.w] =
                    tape_pack(clamp(tape_fx.acc / 4 + mulq15(clamp(tape_fx.fb - tape_fx.hp, -65535, 65535), feedback),
                                    -32767, 32767));
                tape_fx.acc = 0;
                if (tape_fx.fill < TAPE_LENGTH)
                    tape_fx.fill++;
            }
        }
        unsigned ph = (tape_fx.phase + 1) & 3;
        int a = tape_fx.prev1 + (tape_fx.h1 - tape_fx.prev1) * (int)ph / 4,
            b = tape_fx.prev2 + (tape_fx.h2 - tape_fx.prev2) * (int)ph / 4;
        left[i] = (kind != 3 ? tape_fx.lp / 2 : 0) + (kind != 2 ? (4 * a + b) / 5 : 0);
        right[i] = (kind != 3 ? tape_fx.rp / 2 : 0) + (kind != 2 ? (a + 4 * b) / 5 : 0);
        tape_fx.phase = ph;
        int peak = left[i] < 0 ? -left[i] : left[i];
        if (peak > fxg_dpk)
            fxg_dpk = peak;
    }
    fxg_hold = length / CTL + 2u;
}
