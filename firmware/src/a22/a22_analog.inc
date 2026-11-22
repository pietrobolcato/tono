/* SPDX-License-Identifier: MIT. Adapted from Jake Handy, A22. See LICENSE. */
/* A22 - ANALOG: Thru-Zero Linear FM (TZFM) Virtual Analog + 24 dB a22_Ladder.
 *
 * Band-limited PolyBLEP dual oscillators with PWM, true Thru-Zero Linear FM (phase runs
 * smoothly in reverse on negative instantaneous frequency -> stays in tune across all octaves!),
 * 5-osc stereo supersaw, hard sync, 303 acid, chord stacks, and a 24 dB/oct
 * Zero-Delay-Feedback non-linear saturated a22_Ladder Filter driven by Mod-Env & Velocity.
 *
 *   K1 (BLUE)   SHAPE   pulse width / saw-pulse blend / chord interval / sync sweep
 *   K2 (GREEN)  TZFM    thru-zero linear FM depth / unison detune spread
 *   K3 (WHITE)  CUTOFF  24 dB analog ladder filter cutoff (Mod-Env + Velocity tracked)
 *   K4 (ORANGE) RESO    ladder resonance + warm pre-filter analog overdrive */


static const a22_f32 a22_CHORD_SEMI[6][3] = {
    { 7.0f, 12.0f, 19.0f},   /* power / 5th */
    { 3.0f,  7.0f, 10.0f},   /* minor 7 */
    { 4.0f,  7.0f, 11.0f},   /* major 7 */
    { 5.0f,  7.0f, 12.0f},   /* sus4 */
    { 3.0f,  7.0f, 14.0f},   /* minor 9 */
    { 4.0f,  7.0f, 14.0f},   /* major 9 */
};



static void pulse_block(A22Track *track, const a22_Blk *b, int controls_changed)
{
    if (controls_changed) {
        track->analog.algo  = b->algo & 7;
        track->analog.shape = b->k[0];
        track->analog.tzfm  = b->k[1];
        track->analog.cut   = 75.0f * a22_exp2fx(b->k[2] * 7.6f);          /* 75 Hz .. 14.5 kHz */
        track->analog.reso  = b->k[3] * 3.55f;
        track->analog.drive = 1.0f + 2.4f * b->k[3] * b->k[3];
        a22_f32 det = 0.04f + 0.36f * track->analog.tzfm;
        track->analog.detune[0] = a22_exp2fx(det * (1.0f / 12.0f));
        track->analog.detune[1] = a22_exp2fx(-det * 0.9f * (1.0f / 12.0f));
        track->analog.detune[2] = a22_exp2fx(det * 1.85f * (1.0f / 12.0f));
        track->analog.detune[3] = a22_exp2fx(-det * 1.75f * (1.0f / 12.0f));
        track->analog.comp = 1.0f / (1.0f + 0.45f * (track->analog.drive - 1.0f));
    }
    if (b->time < track->pulse_t_was) track->pwm_ph = 0.0f;
    track->pulse_t_was = b->time;
    track->pwm_ph = a22_fractf(track->pwm_ph + 0.45f * b->step);
    track->analog.pwm_lfo = 0.08f * a22_tsin(track->pwm_ph);
}

static void pulse_note(a22_Voice *v, const a22_Blk *b)
{
    int i;
    (void)b;
    if (v->env < 0.001f) {
        for (i = 0; i < 6; i++) v->ph[i] = a22_fractf(v->r1 * (a22_f32)(i + 1) * 0.29f);
        for (i = 0; i < 12; i++) v->s[i] = 0.0f;
    }
    v->s[8] = 1.0f;   /* punch pitch scoop */
}

static void pulse_render(A22Track *track, a22_Voice *v, const a22_Blk *b, a22_f32 *out_l, a22_f32 *out_r, int n)
{
    int algo = track->analog.algo, i;
    a22_f32 pitch = v->pitch + b->bend;
    a22_f32 vel = a22_lerpf(1.0f, 0.35f + 0.85f * v->vel_f, b->vel_sens);
    a22_f32 f, d0, d1, d2, d3, d4, fc;
    a22_f32 pw = a22_clampf(0.5f - 0.42f * track->analog.shape + track->analog.pwm_lfo, 0.06f, 0.94f);
    a22_f32 p0 = v->ph[0], p1 = v->ph[1], p2 = v->ph[2], p3 = v->ph[3], p4 = v->ph[4];
    a22_f32 tz_amt = track->analog.tzfm * track->analog.tzfm * 3.2f * a22_lerpf(1.0f, 0.3f + 1.1f * v->menv, b->menv_amt);
    a22_f32 comp = track->analog.comp;
    a22_Ladder fl;

    if (algo == 3 || algo == 5) pitch -= 12.0f;        /* SUB / ACID sit an octave lower */
    if (algo == 6) pitch -= 0.45f * a22_exp2fx(-v->age * 16.0f);  /* BRASS pitch scoop */
    if (b->punch > 0.01f) pitch += b->punch * v->s[8] * 1.4f;

    f = a22_mtof(pitch);
    d0 = a22_minf(f * b->isr, 0.42f);
    d1 = a22_minf(d0 * track->analog.detune[0], 0.42f);
    d2 = a22_minf(d0 * track->analog.detune[1], 0.42f);
    d3 = a22_minf(d0 * track->analog.detune[2], 0.42f);
    d4 = a22_minf(d0 * track->analog.detune[3], 0.42f);

    if (algo == 1) { /* TZFM modulator ratio on Osc B */
        static const a22_f32 TRAT[5] = {0.5f, 1.0f, 1.5f, 2.0f, 3.0f};
        int ti = a22_iclamp((int)(track->analog.shape * 4.99f), 0, 4);
        d1 = a22_minf(d0 * TRAT[ti], 0.42f);
    } else if (algo == 4) { /* SYNC slave pitch swept by shape + mod env */
        a22_f32 sr_sync = 1.0f + (0.4f + 5.5f * track->analog.shape) * (0.25f + 0.95f * v->menv);
        d1 = a22_minf(d0 * sr_sync, 0.44f);
    } else if (algo == 7) { /* CHORD (stacked chord intervals) */
        int ci = a22_iclamp((int)(track->analog.shape * 5.99f), 0, 5);
        d1 = a22_minf(d0 * a22_exp2fx(a22_CHORD_SEMI[ci][0] * (1.0f / 12.0f)), 0.42f);
        d2 = a22_minf(d0 * a22_exp2fx(a22_CHORD_SEMI[ci][1] * (1.0f / 12.0f)), 0.42f);
        d3 = a22_minf(d0 * a22_exp2fx(a22_CHORD_SEMI[ci][2] * (1.0f / 12.0f)), 0.42f);
    }

    /* Mod-Env + Velocity + Key-Tracking into 24 dB ZDF a22_Ladder Filter */
    fc = track->analog.cut * a22_exp2fx(v->menv * b->menv_amt * (algo == 5 ? 5.2f : 3.6f)
                      + (v->vel_f - 0.5f) * b->vel_sens * 1.6f
                      + (pitch - 60.0f) * (1.0f / 36.0f));
    a22_ladder_set(&fl, a22_clampf(fc, 45.0f, 17000.0f), algo == 5 ? a22_minf(3.75f, track->analog.reso + 1.2f) : track->analog.reso, b->isr);

    /* SUB and ACID drive both channels with the same oscillator sample. A
     * live model change can leave different filter histories; in that case
     * keep filtering the two channels independently. */
    int shared_ladder = (algo == 3 || algo == 5) &&
        __builtin_memcmp(&v->s[0], &v->s[4], 4 * sizeof(v->s[0])) == 0;
    for (i = 0; i < n; i++) {
        a22_f32 xl = 0.0f, xr = 0.0f;
        switch (algo) {
        case 0: /* DUAL: stereo detuned PolyBLEP Saw + PWM Pulse */
        case 6: /* BRASS */ {
            a22_f32 s0 = a22_saw_bl(p0, d0), s1 = a22_saw_bl(p1, d1), sq = a22_pulse_bl(p2, d2, pw);
            a22_f32 mix = track->analog.shape;
            xl = a22_lerpf(s0, sq, mix * 0.65f) * 0.55f + s1 * 0.45f;
            xr = a22_lerpf(s1, sq, mix * 0.65f) * 0.55f + s0 * 0.45f;
            break;
        }
        case 1: /* TZFM: True Thru-Zero Linear FM (carrier phase runs backward cleanly!) */ {
            a22_f32 mod = a22_tsin(p1);
            a22_f32 df = d0 * (1.0f + tz_amt * mod);
            p0 += df;
            if (p0 >= 1.0f) p0 -= 1.0f;
            else if (p0 < 0.0f) p0 += 1.0f;
            p2 += d0 * 1.003f * (1.0f + tz_amt * 0.94f * mod);
            if (p2 >= 1.0f) p2 -= 1.0f;
            else if (p2 < 0.0f) p2 += 1.0f;
            xl = 0.65f * a22_tsin(p0) + 0.35f * a22_tri_wave(p0);
            xr = 0.65f * a22_tsin(p2) + 0.35f * a22_tri_wave(p2);
            break;
        }
        case 2: /* SUPER: 5-osc stereo supersaw / superpulse */ {
            a22_f32 c0 = a22_saw_bl(p0, d0);
            a22_f32 a1 = a22_saw_bl(p1, d1), a2 = a22_saw_bl(p2, d2), a3 = a22_saw_bl(p3, d3), a4 = a22_saw_bl(p4, d4);
            a22_f32 sq = a22_pulse_bl(p0, d0, pw);
            a22_f32 cen = a22_lerpf(c0, sq, track->analog.shape * 0.6f);
            xl = (cen * 0.45f + a1 * 0.35f + a3 * 0.25f + a2 * 0.15f) * 0.8f;
            xr = (cen * 0.45f + a2 * 0.35f + a4 * 0.25f + a1 * 0.15f) * 0.8f;
            break;
        }
        case 3: /* SUB: fat bass + clean sub-octave sine */ {
            a22_f32 sub = a22_phase_sin(p3);
            a22_f32 main = a22_lerpf(a22_saw_bl(p0, d0), a22_pulse_bl(p0, d0, pw), track->analog.shape) * 0.62f + a22_saw_bl(p1, d1) * 0.38f;
            xl = main * 0.7f + sub * (0.35f + 0.4f * track->analog.tzfm);
            xr = xl;
            break;
        }
        case 4: /* SYNC: hard-synced slave sawtooth */ {
            a22_f32 prev = p0;
            p0 += d0;
            if (p0 >= 1.0f) {
                p0 -= 1.0f;
                p1 = p0 * (d1 / a22_maxf(1e-6f, d0));
                if (p1 >= 1.0f) p1 -= (a22_f32)(int)p1;
            }
            (void)prev;
            xl = (a22_saw_bl(p1, d1) * 0.72f + a22_pulse_bl(p0, d0, 0.5f) * 0.35f) * 0.85f;
            xr = (a22_saw_bl(p1, d1) * 0.65f + a22_saw_bl(p2, d2) * 0.38f) * 0.85f;
            break;
        }
        case 5: /* ACID: 303 saw-to-square morph into screaming ladder */ {
            a22_f32 x = a22_lerpf(a22_saw_bl(p0, d0), a22_pulse_bl(p0, d0, 0.5f), track->analog.shape);
            xl = xr = x * 0.92f;
            break;
        }
        default: /* 7 CHORD: harmonic chord stack in stereo */ {
            a22_f32 c0 = a22_saw_bl(p0, d0), c1 = a22_saw_bl(p1, d1), c2 = a22_saw_bl(p2, d2), c3 = a22_saw_bl(p3, d3);
            xl = (c0 * 0.40f + c1 * 0.32f + c2 * 0.18f + c3 * 0.28f) * 0.85f;
            xr = (c0 * 0.40f + c1 * 0.18f + c2 * 0.32f + c3 * 0.28f) * 0.85f;
            break;
        }
        }

        /* 24 dB/oct ZDF Non-Linear Stereo a22_Ladder Filter (+ subtle JP-8000 high-air shelf on SUPER) */
        {
            a22_f32 rl = xl, rr = xr;
            xl = a22_ladder_tick(&fl, &v->s[0], xl * track->analog.drive) * comp * vel;
            xr = shared_ladder ? xl : a22_ladder_tick(&fl, &v->s[4], xr * track->analog.drive) * comp * vel;
            if (algo == 2) {
                a22_f32 air = 0.11f * track->analog.tzfm * vel;
                xl += (rl - xl) * air;
                xr += (rr - xr) * air;
            }
        }
        out_l[i] = xl;
        out_r[i] = xr;

        if (algo != 1 && algo != 4) {
            p0 += d0; if (p0 >= 1.0f) p0 -= 1.0f;
        }
        if (algo != 4) {
            p1 += d1; if (p1 >= 1.0f) p1 -= 1.0f;
        } else {
            p1 += d1; if (p1 >= 1.0f) p1 -= 1.0f;
        }
        if (algo != 1) {
            p2 += d2; if (p2 >= 1.0f) p2 -= 1.0f;
        }
        p3 += (algo == 3 ? d0 * 0.5f : d3); if (p3 >= 1.0f) p3 -= 1.0f;
        p4 += d4; if (p4 >= 1.0f) p4 -= 1.0f;
    }

    if (shared_ladder)
        __builtin_memcpy(&v->s[4], &v->s[0], 4 * sizeof(v->s[0]));
    v->ph[0] = p0; v->ph[1] = p1; v->ph[2] = p2; v->ph[3] = p3; v->ph[4] = p4;
    v->s[8] *= 1.0f - a22_onepole_k(0.030f, b->sr / (a22_f32)n);
}
