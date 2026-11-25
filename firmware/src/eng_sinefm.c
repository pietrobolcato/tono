/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Charles Vestal
 * AMB+ fixed-point adaptation of fm1-amb's sine carrier, bell index and
 * bass harmonic mixture. Voice allocation, pitch, amplitude and sends belong
 * to the ordinary track. All eight operator controls work on every patch. */
static const char *const SF_RAT[] = {"1", "2", "3", "3.5", "4.77"};
static const uint16_t sf_ratio[] = {256, 512, 768, 896, 1221};
static void sf_start(track_t *t, voice_t *v)
{
    (void)t;
    v->ph[0] = v->ph[1] = 0;
    v->ph[2] = 0x40000000u;
    v->s[0] = 32767;
    v->s[1] = 0;
    v->s[2] = 0;
    v->s[3] = 128;
}
static void sf_render(track_t *t, voice_t *v, int32_t *out, uint32_t n, const vmod_t *m)
{
    const int16_t *p = t->p;
    uint32_t ph = v->ph[0], mod = v->ph[1], sub = v->ph[2];
    uint32_t inc = cents_inc(m->pitch16, p[P_E4], m->fine);
    uint32_t mi = (inc >> 8) * sf_ratio[clamp(p[P_E0] + v->s[2], 0, 4)];
    int env = v->s[0], decay = 4 + (127 - p[P_E3]) * (127 - p[P_E3]) / 35;
    int index = clamp(p[P_E1] + (m->shape - (64 << 8)) / 256 + m->cutoff / 512, 0, 127) * 120 +
                mulq15(p[P_E2] < 0 ? m->envq15 : env, (p[P_E2] < 0 ? -p[P_E2] : p[P_E2]) * 180);
    index = index * v->s[3] / 128;
    int harm = p[P_E5] * 200, low = p[P_E6] * 230, feedback = p[P_E7] * 60;
    int last = v->s[1];
    int initial=m->amp0, delta=m->amp1-initial, ramp=delta<0?CTL-1:0;
    for (unsigned i = 0; i < n; i++) {
        int wave = index ? sine_i(mod + (uint32_t)(last * feedback) * 4u) : 0;
        last = sine_i(ph + (uint32_t)wave * (uint32_t)index * 4u);
        int sample = last;
        if (harm) sample += mulq15(sine_i(ph * 2u), harm);
        if (low) sample += mulq15(sine_i(sub), low);
        int bounded=sample > -60000 ? sample : -60000;
        bounded=bounded < 60000 ? bounded : 60000;
        out[i] += mulq15(mulq15(bounded,initial+(ramp>>CTL_LOG2)),VOICE_FS);
        ramp+=delta;
        ph += inc;
        mod += mi;
        sub += inc >> 1;
    }
    v->ph[0] = ph;
    v->ph[1] = mod;
    v->ph[2] = sub;
    v->s[1] = last;
    v->s[0] = env - mulq15(env, decay * (int)n / CTL);
}
static const preset_t SF_PATCHES[] = {
    {"GARDEN RHODES", {0, 22, -18, 114, -3, 20, 0, 0}, {97, 90, 126, 111}, 0, TAG(PAD), FX(0, 12, 20, 65), 0, 0},
    {"GARDEN BELL", {2, 0, 39, 68, 0, 0, 0, 0}, {22, 109, 0, 102}, 0, TAG(BELL), FX(0, 0, 85, 70), 0, 0},
    {"GARDEN PEDAL", {0, 0, 0, 80, 0, 26, 85, 0}, {90, 90, 125, 90}, 0, TAG(BASS), FX(0, 0, 0, 8), 0, 0},
    /* Original Tono voices. Positive contours soften after the strike;
     * negative contours let the upper harmonics bloom with a held note. */
    {"SUNROOM KEYS", {1, 8, 47, 72, 0, 19, 0, 0}, {22, 84, 110, 101}, 0, TAG(KEYS), FX(0, 0, 15, 49), 0, 0},
    {"LINEN PAD", {0, 6, -32, 108, -1, 32, 5, 0}, {91, 105, 119, 116}, 0, TAG(PAD), FX(0, 0, 19, 69), 0, 0},
    {"SKYLIGHT", {1, 9, -36, 118, 0, 46, 0, 2}, {102, 108, 120, 116}, 0, TAG(PAD), FX(0, 0, 11, 75), 0, 0},
    {"WILLOW TINE", {1, 2, 52, 53, 0, 16, 0, 0}, {3, 73, 0, 91}, 0, TAG(BELL), FX(0, 0, 24, 58), 0, 0},
    {"DEW BELL", {4, 0, 32, 84, 2, 7, 0, 0}, {13, 97, 0, 103}, 0, TAG(BELL), FX(0, 0, 44, 82), 0, 0},
    {"LINDEN BASS", {0, 4, 23, 56, 0, 52, 9, 0}, {27, 84, 103, 88}, 0, TAG(BASS), FX(0, 0, 0, 19), 0, 0}
};
static const engine_t ENG_SINEFM = {.name = "SINE FM",
                                    .page_title = {"OPERATOR", "COLOR"},
                                    .edit = {{"RATIO", F_ENUM, 0, 4, 0, SF_RAT, 0},
                                             {"INDEX", F_PCT, 0, 127, 22, 0, 0},
                                             {"CONTOUR", F_INT, -63, 127, 20, 0, 0},
                                             {"DECAY", F_PCT, 0, 127, 68, 0, 0},
                                             {"TUNE", F_INT, -50, 50, 0, 0, "ct"},
                                             {"OCTAVE", F_PCT, 0, 127, 0, 0, 0},
                                             {"SUB", F_PCT, 0, 127, 0, 0, 0},
                                             {"FB", F_PCT, 0, 127, 0, 0, 0}},
                                    .presets = SF_PATCHES,
                                    .npresets = NELEM(SF_PATCHES),
                                    .poly = 4,
                                    .knob = {P_E0, P_E1, P_E2, P_E3},
                                    .note_on = sf_start,
                                    .render = sf_render};
