/* SPDX-License-Identifier: MIT. Adapted from Jake Handy, A22. See LICENSE. */
/* A22 - ELECTRIC: Boutique Electro-Mechanical Keys (FM + Additive + Stereo Tremolo).
 *
 * Eight studio-tuned keyboard voices: Suitcase Rhodes, Wurlitzer 200A, Crystal DX-EP,
 * Funky D6 Clavinet, B3 Tonewheel Organ, Felted Upright Piano, Vibraphone, and Tape Keys.
 * True stereo autopan tremolo & rotary width, velocity-sensitive tine bark, and warm tone.
 *
 *   K1 (BLUE)   BARK    FM overtone drive & velocity bite
 *   K2 (GREEN)  TINE    metallic strike, bell sparkle & hammer click
 *   K3 (WHITE)  TREM    stereo autopan tremolo / Leslie rotary depth
 *   K4 (ORANGE) WARMTH  cabinet warmth / felt damping / tape age */


typedef struct a22_EAlgo {
    a22_f32 dec;             /* natural note decay time (s), 0 = sustain */
    a22_f32 att;             /* onset time (s) */
    a22_f32 bark;            /* base bark FM index */
    a22_f32 tine;            /* base tine harmonic level */
} a22_EAlgo;

static const a22_EAlgo a22_EA[a22_NALGO] = {
    /* 0 RHODES */ {2.8f, 0.0f,   0.52f, 0.42f},
    /* 1 WURLI  */ {1.5f, 0.0f,   0.85f, 0.30f},
    /* 2 DX-EP  */ {3.1f, 0.0f,   0.65f, 0.58f},
    /* 3 CLAV   */ {0.75f,0.0f,   0.92f, 0.45f},
    /* 4 ORGAN  */ {0.0f, 0.002f, 0.35f, 0.45f},
    /* 5 FELT   */ {3.2f, 0.004f, 0.25f, 0.28f},
    /* 6 VIBES  */ {3.8f, 0.0f,   0.20f, 0.52f},
    /* 7 TAPE   */ {2.5f, 0.008f, 0.48f, 0.32f},
};



static void elec_block(A22Track *track, const a22_Blk *b, int controls_changed)
{
    a22_f32 dt = b->step;
    a22_f32 rate = (b->algo == 4) ? (1.2f + 5.6f * b->k[2]) : (2.6f + 3.8f * b->k[2]);
    a22_f32 tr, dp;
    track->electric.algo = b->algo & 7;
    track->electric.bark = b->k[0];
    track->electric.tine = b->k[1];
    track->electric.trem = b->k[2];
    track->electric.warmth = b->k[3];

    if (b->time < track->elec_t_was) track->trem_ph = track->wow_ph = track->flut_ph = 0.0f;
    track->elec_t_was = b->time;
    track->trem_ph = a22_fractf(track->trem_ph + rate * dt);
    track->wow_ph = a22_fractf(track->wow_ph + 0.58f * dt);
    track->flut_ph = a22_fractf(track->flut_ph + 5.9f * dt);
    track->electric.rot_ph = track->trem_ph;
    tr = a22_tsin(track->trem_ph);
    dp = track->electric.trem * 0.72f;
    track->electric.trem_l = 1.0f - dp * (0.5f + 0.5f * tr);
    track->electric.trem_r = 1.0f - dp * (0.5f - 0.5f * tr);
    track->electric.wow = (track->electric.algo == 7) ? (0.04f + 0.35f * track->electric.warmth) * (0.65f * a22_tsin(track->wow_ph) + 0.2f * a22_tsin(track->flut_ph)) : 0.0f;
    if (controls_changed) {
        track->electric.kd = a22_EA[track->electric.algo].dec > 0.0f ? 1.0f - a22_onepole_k(a22_EA[track->electric.algo].dec, b->sr / (a22_f32)a22_CTRL) : 1.0f;
        a22_f32 cut = 14000.0f * a22_exp2fx(-track->electric.warmth * 3.8f);
        track->electric.lpk = a22_onepole_k(1.0f / (a22_H1_TAU * cut), b->sr);
    }
}

static void elec_note(a22_Voice *v, const a22_Blk *b)
{
    int i;
    (void)b;
    if (!v->u[0]) v->u[0] = 0xE1EC0001u + v->idx * 6551u;
    if (v->env < 0.001f) {
        for (i = 0; i < 6; i++) v->ph[i] = 0.0f;
        for (i = 0; i < 12; i++) v->s[i] = 0.0f;
    }
    v->s[0] = 1.0f;   /* body decay */
    v->s[1] = 1.0f;   /* tine / hammer strike decay */
    v->s[2] = 0.0f;   /* feedback reed */
}

static void elec_render(A22Track *track, a22_Voice *v, const a22_Blk *b, a22_f32 *out_l, a22_f32 *out_r, int n)
{
    const a22_EAlgo *a = &a22_EA[track->electric.algo];
    a22_f32 pitch = v->pitch + b->bend + track->electric.wow;
    a22_f32 f = a22_mtof(pitch);
    a22_f32 d0 = a22_minf(f * b->isr, 0.44f);
    a22_f32 key = a22_clampf(1.0f - (pitch - 60.0f) * (1.0f / 54.0f), 0.42f, 1.35f);
    a22_f32 vel = a22_lerpf(1.0f, 0.30f + 0.90f * v->vel_f, b->vel_sens);
    a22_f32 bark = a->bark * (0.25f + 1.45f * track->electric.bark) * vel * a22_lerpf(1.0f, 0.35f + 1.0f * v->menv, b->menv_amt);
    a22_f32 tine = a->tine * (0.10f + 1.65f * track->electric.tine) * vel;
    a22_f32 body = v->s[0], hit = v->s[1], fb = v->s[2];
    a22_f32 p0 = v->ph[0], p1 = v->ph[1], p2 = v->ph[2], p3 = v->ph[3], p4 = v->ph[4];
    a22_f32 d1 = d0, d2 = d0, d3 = d0, d4 = d0;
    a22_f32 amp = a->att > 0.0f ? a22_sat(v->age / a->att) : 1.0f;
    a22_f32 lpk = track->electric.lpk;
    a22_f32 lpl = v->s[4], lpr = v->s[5];
    int algo = track->electric.algo, i;

    switch (algo) {
    case 0: /* RHODES: fundamental + 1x bark + 7x/14x metallic tine */
    case 7: /* TAPE */
        d1 = a22_minf(d0 * 1.0018f, 0.44f);
        d2 = a22_minf(d0 * 7.0f, 0.44f);
        d3 = a22_minf(d0 * 14.0f, 0.44f);
        break;
    case 1: /* WURLI: 2x reed overtone + 4x attack ping */
        d1 = a22_minf(d0 * 2.0f, 0.44f);
        d2 = a22_minf(d0 * 4.0f, 0.44f);
        d3 = a22_minf(d0 * 9.0f, 0.44f);
        break;
    case 2: /* DX-EP: stereo detuned pairs + 14x crystal bell */
        d1 = a22_minf(d0 * 1.0032f, 0.44f);
        d2 = a22_minf(d0 * 1.0f, 0.44f);
        d3 = a22_minf(d0 * 14.0f, 0.44f);
        d4 = a22_minf(d0 * 1.0032f * 14.0f, 0.44f);
        break;
    case 3: /* CLAV: bright odd/even plucked comb */
        d1 = a22_minf(d0 * 2.0f, 0.44f);
        d2 = a22_minf(d0 * 3.0f, 0.44f);
        d3 = a22_minf(d0 * 5.0f, 0.44f);
        break;
    case 4: /* ORGAN: 16', 8', 5-1/3', 4', 2' drawbars */
        d1 = a22_minf(d0 * 0.5f, 0.44f);
        d2 = a22_minf(d0 * 1.5f, 0.44f);
        d3 = a22_minf(d0 * 2.0f, 0.44f);
        d4 = a22_minf(d0 * 4.0f, 0.44f);
        break;
    case 5: /* FELT: slightly stretched acoustic piano partials */
        d1 = a22_minf(d0 * 2.002f, 0.44f);
        d2 = a22_minf(d0 * 3.008f, 0.44f);
        d3 = a22_minf(d0 * 4.022f, 0.44f);
        d4 = a22_minf(d0 * 1.0014f, 0.44f);
        break;
    case 6: /* VIBES: bar + 4x + 10x mallet chime */
        d1 = a22_minf(d0 * 4.0f, 0.44f);
        d2 = a22_minf(d0 * 9.92f, 0.44f);
        break;
    }

    for (i = 0; i < n; i++) {
        a22_f32 xl = 0.0f, xr = 0.0f;
        switch (algo) {
        case 0: /* RHODES */
        case 7: /* TAPE */ {
            a22_f32 mod = bark * key * (0.20f + 0.80f * body) * 0.55f * a22_phase_sin(p0);
            a22_f32 bell = tine * hit * (0.55f * a22_phase_sin(p2) + 0.45f * hit * a22_phase_sin(p3));
            xl = a22_tsin(p0 + mod) + bell;
            xr = a22_tsin(p1 + mod * 0.96f) + bell * 0.92f;
            if (algo == 7) {
                a22_f32 nz = a22_noise_s(&v->u[0]) * 0.015f * track->electric.warmth;
                xl = a22_softclip(xl * (1.0f + 0.6f * track->electric.warmth)) + nz;
                xr = a22_softclip(xr * (1.0f + 0.6f * track->electric.warmth)) + nz;
            }
            break;
        }
        case 1: /* WURLI: asymmetric reed bite + 4x/9x metallic attack ping */ {
            a22_f32 m = bark * (0.18f + 0.82f * hit) * 0.38f;
            a22_f32 reed;
            fb = a22_tsin(p0 + m * fb + 0.30f * body * bark * a22_phase_sin(p1));
            reed = fb + 0.16f * track->electric.bark * fb * fb + tine * 0.35f * hit * a22_phase_sin(p2) + 0.14f * hit * hit * a22_phase_sin(p3);
            xl = a22_softclip(reed * (1.0f + 0.82f * track->electric.bark));
            xr = a22_softclip((reed - 0.08f * hit * a22_phase_sin(p2)) * (1.0f + 0.78f * track->electric.bark));
            break;
        }
        case 2: /* DX-EP */ {
            a22_f32 m = bark * key * (0.28f + 0.72f * body) * 0.55f;
            xl = 0.72f * a22_tsin(p0 + m * a22_phase_sin(p2)) + 0.32f * tine * hit * a22_tsin(p3 + 0.25f * a22_phase_sin(p2));
            xr = 0.72f * a22_tsin(p1 + m * a22_phase_sin(p2)) + 0.32f * tine * hit * a22_tsin(p4 + 0.25f * a22_phase_sin(p2));
            break;
        }
        case 3: /* CLAV */ {
            a22_f32 x = a22_tsin(p0 + bark * (0.35f + 0.95f * body) * a22_phase_sin(p1))
                  + 0.42f * body * a22_tsin(p2 + 0.5f * bark * a22_phase_sin(p0))
                  + 0.28f * tine * hit * a22_phase_sin(p3);
            xl = a22_softclip(x * 1.45f) * 0.78f;
            xr = xl;
            break;
        }
        case 4: /* ORGAN (B3 Drawbar + Key Click + Stereo Rotary) */ {
            a22_f32 click = hit * hit * a22_noise_s(&v->u[0]) * 0.25f * tine;
            a22_f32 drw = 0.45f * a22_phase_sin(p1) + 0.55f * a22_phase_sin(p0) + (0.22f + 0.38f * track->electric.bark) * a22_phase_sin(p2)
                    + (0.18f + 0.35f * track->electric.tine) * a22_phase_sin(p3) + 0.22f * track->electric.tine * a22_phase_sin(p4) + click;
            drw = a22_softclip(drw * (1.0f + 1.2f * track->electric.bark)) * 0.72f;
            xl = drw * (0.85f + 0.15f * a22_tsin(track->electric.rot_ph + 0.15f));
            xr = drw * (0.85f - 0.15f * a22_tsin(track->electric.rot_ph + 0.15f));
            break;
        }
        case 5: /* FELT: low-passed wooden felt hammer thud + stretched partials */ {
            a22_f32 h_lp = v->s[3] + 0.16f * (a22_noise_s(&v->u[0]) - v->s[3]);
            a22_f32 thump = hit * hit * h_lp * 0.44f * (0.35f + track->electric.tine);
            a22_f32 hi = (0.38f + 0.18f * track->electric.bark) * body * a22_phase_sin(p1) + 0.20f * body * body * a22_phase_sin(p2) + 0.10f * hit * a22_phase_sin(p3);
            v->s[3] = h_lp;
            xl = 0.76f * a22_phase_sin(p0) + hi + thump;
            xr = 0.76f * a22_phase_sin(p4) + hi * 0.94f + thump;
            break;
        }
        default: /* 6 VIBES: resonant aluminum bar + 4x/10x overtone + stereo motor shimmer */ {
            a22_f32 ov = 0.34f * tine * hit * a22_phase_sin(p1) + 0.14f * tine * hit * hit * a22_phase_sin(p2);
            xl = a22_phase_sin(p0) + ov;
            xr = a22_tsin(p0 + 0.015f * track->electric.trem) + ov * 0.92f;
            break;
        }
        }
        xl = xl * 0.65f * amp * (a->dec > 0.0f ? body : 1.0f) * track->electric.trem_l;
        xr = xr * 0.65f * amp * (a->dec > 0.0f ? body : 1.0f) * track->electric.trem_r;
        lpl += (xl - lpl) * lpk;
        lpr += (xr - lpr) * lpk;
        out_l[i] = lpl;
        out_r[i] = lpr;

        p0 += d0; if (p0 >= 1.0f) p0 -= 1.0f;
        p1 += d1; if (p1 >= 1.0f) p1 -= 1.0f;
        p2 += d2; if (p2 >= 1.0f) p2 -= 1.0f;
        p3 += d3; if (p3 >= 1.0f) p3 -= 1.0f;
        p4 += d4; if (p4 >= 1.0f) p4 -= 1.0f;
    }

    v->ph[0] = p0; v->ph[1] = p1; v->ph[2] = p2; v->ph[3] = p3; v->ph[4] = p4;
    v->s[0] = body * (key < 1.0f ? track->electric.kd * (1.0f - (1.0f - track->electric.kd) * (1.0f - key) * 2.5f) : track->electric.kd);
    {
        a22_f32 h_tau = algo == 4 ? 0.015f : (algo == 5 ? 0.06f : (0.09f + 0.07f * key));
        v->s[1] = hit * (1.0f - a22_onepole_k(h_tau, b->sr / (a22_f32)n));
    }
    v->s[2] = fb;
    v->s[4] = lpl; v->s[5] = lpr;
}
