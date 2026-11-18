/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Charles Vestal
 * The pitched sine thump and four-metal-oscillator hats from fm1-amb,
 * da0047dc. Uses the common track voices and sends. */
static void sp_on(track_t *t, voice_t *v)
{
    v->s[7] = t->p[P_ROOT];
    v->ph[0] = v->ph[1] = v->ph[2] = 0;
    v->s[0] = v->s[1] = v->s[2] = 0;
    v->s[3] = 22222;
    v->s[4] = 1 << 23;
    v->s[5] = 0;
    v->s[6] = 32767;
}
static void sp_render(track_t *t, voice_t *v, int32_t *out, uint32_t n, const vmod_t *m)
{
    static const uint32_t metal[] = {29645884, 35995793, 50907584, 77913239};
    const int16_t *p = t->p;
    int lane = v->note == 42 || v->note == 44 ? DV_HATC
               : v->note == 46                ? DV_HATO
               : v->note == 38                ? DV_SNARE
               : v->note <= 36                ? DV_KICK
                                              : DV_TOM;
    int hat = lane == DV_HATC || lane == DV_HATO || lane == DV_SNARE;
    int level = clamp(p[P_LN0 + lane], 0, 127);
    int amp = v->s[4], att = v->s[5], drop = v->s[6];
    /* Block-rate exponential decay to -60 dB at the specified duration. */
    unsigned ms = lane == DV_HATC ? 60 : lane == DV_HATO ? 420 : 450;
    ms = ms * (32 + p[P_E3]) / 96;
    int decay = clamp((int)(n * 5127u / ms), 1, 32700);
    uint32_t ph[] = {v->ph[0], v->ph[1], v->ph[2], (uint32_t)v->s[0]}, noise = (uint32_t)v->s[3];
    int low = v->s[1], band = v->s[2], f = clamp(13500 + p[P_E2] * 228, 8000, 38000);
    uint32_t inc = pitch_inc(clamp((36 + v->s[7] + (p[P_E1] - 64) / 4 + (lane == DV_TOM ? 7 : 0)) * 16, 0, 2047));
    uint32_t metal_inc[4];
    for (unsigned j = 0; j < 4; j++)
        metal_inc[j] = (metal[j] >> 8) * (224 + p[P_E1] / 2);
    for (unsigned i = 0; i < n; i++) {
        int x;
        if (hat) {
            int sum = 0;
            for (unsigned j = 0; j < 4; j++) {
                ph[j] += metal_inc[j];
                sum += ph[j] >> 31 ? -4096 : 4096;
            }
            noise = noise * 1664525u + 1013904223u;
            int src = (sum * (32 + p[P_E0]) / 96) + ((int32_t)noise >> 16) * (p[P_E4] + 8) / 128;
            int hp = src - low - mulq15(band, 26214);
            band = clamp(band + mulq15(clamp(hp, -56000, 56000), f), -48000, 48000);
            low = clamp(low + mulq15(band, f), -48000, 48000);
            x = clamp(hp, -60000, 60000) * (lane == DV_HATO ? 9 : lane == DV_SNARE ? 30 : 13) / 100;
        } else {
            x = sine_i(ph[0]);
            ph[0] += inc + ((inc >> 10) * (unsigned)(drop >> 5) / 64) * (unsigned)(p[P_E5] + 8);
            drop -= ((drop * 39 + 16384) >> 15);
        }
        if (p[P_E6])
            x += mulq15(sine_i(ph[0] * 2), p[P_E6] * 45);
        if (p[P_E7])
            x = softclip(clamp(x + x * p[P_E7] / 64, -65535, 65535));
        att += (32767 - att) * (hat ? 30 : 2) / 100;
        out[i] += voice_amp(mulq15(x, att) * (amp >> 10) / 8192 * level / 127, m, i);
    }
    v->ph[0] = ph[0];
    v->ph[1] = ph[1];
    v->ph[2] = ph[2];
    v->s[0] = (int32_t)ph[3];
    v->s[1] = low;
    v->s[2] = band;
    v->s[3] = (int32_t)noise;
    v->s[4] = amp - (int)(((int64_t)amp * decay) >> 15);
    v->s[5] = att;
    v->s[6] = drop;
    if (v->s[4] < 4000)
        v->active = v->gate = 0;
}
static const preset_t SP_PATCH[] = {
    {"GARDEN PULSE", {64, 64, 64, 64, 68, 69, 0, 0}, {0, 100, 127, 100}, 0, TAG(DRUM), FX(0, 0, 8, 12), 0, 0}};
static const engine_t ENG_SOFTPULSE = {.name = "SOFT PULSE",
                                       .page_title = {"PULSE", "COLOR"},
                                       .edit = {{"METAL", F_PCT, 0, 127, 64, 0, 0},
                                                {"TUNE", F_PCT, 0, 127, 64, 0, 0},
                                                {"HAT CUT", F_PCT, 0, 127, 64, 0, 0},
                                                {"DECAY", F_PCT, 0, 127, 64, 0, 0},
                                                {"NOISE", F_PCT, 0, 127, 68, 0, 0},
                                                {"DROP", F_PCT, 0, 127, 69, 0, 0},
                                                {"OCTAVE", F_PCT, 0, 127, 0, 0, 0},
                                                {"DRIVE", F_PCT, 0, 127, 0, 0, 0}},
                                       .presets = SP_PATCH,
                                       .npresets = 1,
                                       .poly = 4,
                                       .oneshot = 1,
                                       .knob = {P_E0, P_E1, P_E2, P_E3},
                                       .note_on = sp_on,
                                       .render = sp_render};
