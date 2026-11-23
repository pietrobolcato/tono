/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Charles Vestal
 * Adapted from fm1-amb da0047dc, app/dsp/amb.c. The four independent
 * quarter-note loops store scale degrees, so harmony changes can retune a
 * motif in place. This module emits ordinary track notes. */
typedef struct {
    int8_t offset[29];
    uint8_t velocity[29], position, rest;
} garden_loop_t;
typedef struct {
    uint32_t due, off;
    uint8_t note, velocity, playing, shade;
} garden_event_t;
typedef struct {
    uint32_t rng, now, next_event, breath_due[4], pulse_pattern[3];
    garden_loop_t loop[4];
    garden_event_t event[12];
    uint8_t ready, role, pulse_rest, hits, rotation[3], pulse_count[3], bass_note;
} garden_layer_t;
static garden_layer_t garden_layer[4];
static const uint8_t garden_length[] = {13, 17, 23, 29}, garden_register[] = {72, 67, 79, 62};
static const uint8_t garden_pulse_len[] = {16, 12, 7}, garden_hits[] = {5, 7, 3};
static unsigned garden_random(garden_layer_t *g, unsigned range)
{
    uint32_t x = g->rng;
    x ^= x << 13;
    x ^= x >> 17;
    g->rng = x ^ (x << 5);
    return (g->rng >> 8) % range;
}
static int garden_degree(garden_layer_t *g, unsigned count)
{
    if (garden_random(g, 100) < 72) {
        unsigned offset = 2 * garden_random(g, count >= 6 ? 4 : 3);
        return offset + (garden_random(g, 100) < 30 ? count : 0);
    }
    return 1 + 2 * garden_random(g, 3);
}
static int garden_pick(garden_layer_t *g, garden_loop_t *l, unsigned len)
{
    unsigned p = garden_random(g, len);
    for (unsigned i = 0; i < len; i++, p = (p + 1) % len)
        if (l->offset[p] >= 0)
            return (int)p;
    return -1;
}
static void garden_pass(garden_layer_t *g, unsigned lane, unsigned density, unsigned evolve, unsigned count, int arc,
                        int first)
{
    garden_loop_t *l = &g->loop[lane];
    int len = garden_length[lane], n = 0;
    int target = (clamp((int)density * (100 + arc * 8 / 10) / 20, 0, 500) + garden_random(g, 100)) / 100;
    for (int i = 0; i < len; i++)
        n += l->offset[i] >= 0;
    for (; n < target; n++)
        for (int tries = 0; tries < 8; tries++) {
            int slot = garden_random(g, len);
            if (l->offset[slot] >= 0)
                continue;
            l->offset[slot] = (int8_t)garden_degree(g, count);
            l->velocity[slot] = 50 + garden_random(g, 50);
            break;
        }
    for (; n > target; n--) {
        int slot = garden_pick(g, l, len);
        if (slot >= 0)
            l->offset[slot] = -1;
    }
    if (first)
        return;
    l->rest = garden_random(g, 1000) < evolve * 15 / 10 + (unsigned)(arc < 0 ? -arc * 15 / 10 : 0);
    if (garden_random(g, 1000) < evolve * 7) {
        int slot = garden_pick(g, l, len);
        if (slot < 0)
            return;
        if (garden_random(g, 2))
            l->offset[slot] = (int8_t)garden_degree(g, count);
        else {
            unsigned dest = garden_random(g, len);
            if (l->offset[dest] < 0) {
                l->offset[dest] = l->offset[slot];
                l->velocity[dest] = l->velocity[slot];
                l->offset[slot] = -1;
            }
        }
    }
}
static void garden_cancel(unsigned k)
{
    garden_layer_t *g = &garden_layer[k];
    if (!g->ready)
        return;
    for (unsigned i = 0; i < NELEM(g->event); i++)
        if (g->event[i].playing)
            trk_note_off(&trk[k], g->event[i].note);
    g->ready = 0;
}
static void garden_schedule(unsigned k, unsigned note, unsigned vel, unsigned wait, unsigned gate, unsigned shade)
{
    garden_layer_t *g = &garden_layer[k];
    for (unsigned i = 0; i < NELEM(g->event); i++) {
        garden_event_t *e = &g->event[i];
        if (e->velocity)
            continue;
        if ((int32_t)(g->now + wait - g->next_event) < 0)
            g->next_event = g->now + wait;
        *e = (garden_event_t){g->now + wait, g->now + wait + gate, (uint8_t)note, (uint8_t)vel, 0, (uint8_t)shade};
        return;
    }
}
static void garden_advance(unsigned k, unsigned frames)
{
    garden_layer_t *g = &garden_layer[k];
    if (!g->ready)
        return;
    if (!amb.layer[k].generated || amb.policy.active[0] != 8 || amb.layer[k].role != g->role || !song.playing ||
        ambr.handoff[k] || amb.layer[k].base[P_MUTE] || (ambr.solo >= 0 && ambr.solo != (int)k)) {
        garden_cancel(k);
        return;
    }
    g->now += frames;
    if ((int32_t)(g->now - g->next_event) < 0)
        return;
    g->next_event = g->now + FS;

    for (unsigned i = 0; i < NELEM(g->event); i++) {
        garden_event_t *e = &g->event[i];
        if (!e->velocity)
            continue;
        if (!e->playing && (int32_t)(g->now - e->due) >= 0) {
            if(!amb_keep_onset(k,e->due)){
                *e=(garden_event_t){0};
                continue;
            }
            /* A retrigger takes ownership of its pitch's eventual NOTE OFF. */
            for (unsigned j = 0; j < NELEM(g->event); j++)
                if (g->event[j].playing && g->event[j].note == e->note)
                    g->event[j] = (garden_event_t){0};
            track_t *t = &trk[k];
            trk_note_on(t, e->note, e->velocity);
            e->playing = 1;
            if (e->shade && t->engine == ENGI_SINEFM) {
                static const int ratio[] = {0, 0, 1, -1}, index[] = {128, 70, 192, 102};
                voice_t *v = &t->v[t->m_vi];
                if (v->active && v->note == e->note) {
                    v->s[2] = ratio[e->shade - 1];
                    v->s[3] = index[e->shade - 1];
                }
            }
        }
        if (e->playing && (int32_t)(g->now - e->off) >= 0) {
            trk_note_off(&trk[k], e->note);
            *e = (garden_event_t){0};
        } else {
            uint32_t next = e->playing ? e->off : e->due;
            if ((int32_t)(next - g->next_event) < 0)
                g->next_event = next;
        }
    }
}
static void garden_pulse(garden_layer_t *g, unsigned k, unsigned step, unsigned density, unsigned evolve, int allow)
{
    static const uint8_t rotation[] = {0, 2, 3}, gm[] = {36, 42, 46};
    if (g->hits != density + 1) {
        for (unsigned i = 0; i < 3; i++) {
            g->pulse_count[i] = (density * garden_hits[i] + 50) / 100;
            g->rotation[i] = rotation[i];
            g->pulse_pattern[i] = amb_euclid(g->pulse_count[i], garden_pulse_len[i], rotation[i]);
        }
        g->hits = density + 1;
    }
    if (!(step % 16)) {
        if (g->pulse_rest)
            g->pulse_rest--;
        else if (garden_random(g, 10000) < evolve * 15 / 10)
            g->pulse_rest = 2 + garden_random(g, 3);
        if (garden_random(g, 400) < evolve) {
            unsigned i = garden_random(g, 3), home = (density * garden_hits[i] + 50) / 100;
            if (home) {
                if (garden_random(g, 2))
                    g->rotation[i] =
                        (g->rotation[i] + (garden_random(g, 2) ? 1 : garden_pulse_len[i] - 1)) % garden_pulse_len[i];
                else
                    g->pulse_count[i] = clamp((int)g->pulse_count[i] + (garden_random(g, 2) ? 1 : -1),
                                              home > 1 ? home - 1 : 1, home + 1);
                g->pulse_pattern[i] = amb_euclid(g->pulse_count[i], garden_pulse_len[i], g->rotation[i]);
            }
        }
    }
    for (unsigned i = 0; i < 3; i++)
        if (allow && !g->pulse_rest && (g->pulse_pattern[i] >> (step % garden_pulse_len[i]) & 1))
        {
            unsigned velocity=(step % garden_pulse_len[i] ? 60 : 88) + garden_random(g,21);
            if(amb_keep_onset(k,step))trk_note_on(&trk[k],amb_remember(k,gm[i],1),velocity);
        }
}
/* Upstream drift is measured in seconds, then committed on a bar. AUTO
 * BARS scales its average duration; at the factory setting it is 39.4 s.
 * Manual chord selection also restarts the pedal's halfway/fifth clock. */
static struct {
    uint32_t serial, age, limit;
    uint8_t fifth, rest, degree;
} garden_drift;
static void garden_reset_drift(void)
{
    unsigned r = amb_hash(amb.policy.seed + (++garden_drift.serial) * 71u);
    unsigned base = (FS / 100u) * 3940u / 8u * AMB_AUTO[amb.auto_index];
    garden_drift.limit = base / 1000u * (700 + r % 601u);
    garden_drift.age = 0;
    garden_drift.fifth = (r >> 16) % 100 < 35;
    garden_drift.rest = 0;
    garden_drift.degree = amb.chord;
}
static struct {
    uint32_t left;
    int32_t value, target;
} garden_arc;
static void garden_harmony(unsigned step)
{
    unsigned frames = FS * 60u / (4u * (unsigned)amb.global[G_BPM]);
    if (!step || !garden_drift.limit) {
        garden_drift.serial = 0;
        if (!step)
            memset(&garden_arc, 0, sizeof garden_arc);
    }
    if (garden_arc.left <= frames) {
        unsigned r = amb_hash(amb.policy.seed + step * 29u);
        int target = (int)(r % 201u) - 100;
        target = target * (60 + (int)((r >> 8) % 41u)) / 100 + (target > 0 ? 25 : -25);
        garden_arc.target = clamp(target, -100, 100) * 256;
        garden_arc.left = (50 + (r >> 16) % 101u) * FS;
    } else
        garden_arc.left -= frames;
    garden_arc.value += (garden_arc.target - garden_arc.value) * (int)(frames / 32) / (35 * FS / 32);
    if (!step || !garden_drift.limit || garden_drift.degree != amb.chord)
        garden_reset_drift();
    if (!amb.auto_index)
        return;
    garden_drift.age += frames;
    if (step % 16 || garden_drift.age < garden_drift.limit)
        return;
    unsigned r = amb_hash(amb.policy.seed + step), old = amb.chord;
    if (r % 100 < amb.evolve[0]) {
        static const uint8_t weights[] = {40, 20, 10, 30, 30, 30, 4};
        int scale[12], n = amb_scale(amb_pool(), scale), sum = 0;
        for (int i = 0; i < n; i++)
            if (i != (int)old)
                sum += n == 7 ? weights[i] : i ? 15 : 30;
        int pick = (int)((r >> 8) % (unsigned)sum);
        for (int i = 0; i < n; i++)
            if (i != (int)old) {
                pick -= n == 7 ? weights[i] : i ? 15 : 30;
                if (pick < 0) {
                    amb.chord = i;
                    break;
                }
            }
        amb_revoice();
    }
    garden_reset_drift();
    garden_drift.rest = amb.chord != old && (r >> 16) % 1000 < amb.policy.variation * 3u;
}
static void garden_step(unsigned k, unsigned step)
{
    amb_layer_t *l = &amb.layer[k];
    garden_layer_t *g = &garden_layer[k];
    int scale[12], count = amb_scale(amb_pool(), scale);
    unsigned density = clamp((int)amb_routed(k,ROUTE_DENSITY,l->density,0,100) + (l->role == 4 ? ((int)l->macro[1] - 64) / 2 : 0), 0, 100);
    unsigned variation = clamp((int)amb.policy.variation + (int)amb_routed(k,ROUTE_MUTATE,l->mutation,0,100), 0, 100);
    unsigned stride = 1u << amb_routed(k,ROUTE_SPEED,l->speed,0,3), slot = (step / stride) % 16;
    unsigned mode=amb_routed(k,ROUTE_MODE,l->mode,0,l->role==0?AMB_CHANT:4);
    unsigned follow=amb_routed(k,ROUTE_FOLLOW,l->follow,0,100);
    unsigned degree_base =
        follow == 100 || amb_hash(amb.policy.seed + step / 16 + k) % 100 < follow ? amb.chord : 0;
    int arc = garden_arc.value * (int)amb.evolve[1] / 25600;
    int allow = (!amb.policy.active[1] || amb_phrase_event(&amb.policy, step, k));
    switch (amb.policy.active[2]) {
    case 1:
        allow &= (step / 4 + k) % 5 != 0;
        break;
    case 2:
        allow &= step / 16 % 4 == k;
        break;
    case 3:
        allow &= (step / 8 + k) % 4 != 0;
        break;
    case 4:
        allow &= (step / 4 + k) % 3 == 0;
        break;
    case 5:
        allow &= step / 16 % 12 < 8;
        break;
    }
    if (!g->ready || g->role != l->role) {
        garden_cancel(k);
        memset(g, 0, sizeof *g);
        g->role = l->role;
        g->ready = 1;
        g->rng = amb_hash(amb.policy.seed + k * 997u) | 1u;
        if (l->role == 2 || l->role == 6 || l->role == 7)
            for (unsigned i = 0; i < 4; i++) {
                memset(g->loop[i].offset, -1, sizeof g->loop[i].offset);
                garden_pass(g, i, density, variation, count, arc, 1);
            }
        static const unsigned startup_ms[] = {0, 1355, 2883, 935};
        for (unsigned i = 0; i < 4; i++)
            g->breath_due[i] = FS * startup_ms[i] / 1000;
    }
    if (!(step % stride)) {
        ambr.recent[k][slot] = 0;
        ambr.recent_valid[k] = 1;
    }
    if (l->role == 4) {
        if (l->engine == ENGI_SOFTPULSE)
            trk[k].p[P_ROOT] = (amb.key + scale[amb.chord % count]) % 12;
        garden_pulse(g, k, step, density, variation, allow);
        if (allow && step % 64 == 63 && garden_random(g, 100) < amb_routed(k,ROUTE_FILL,l->fill,0,100) && amb_keep_onset(k,step))
            trk_note_on(&trk[k], 42, 56);
        return;
    }
    unsigned note = 0, vel = 0;
    if (l->role < 2) {
        static const unsigned stretch[] = {100, 121, 143, 167};
        unsigned lookahead = FS * 60u / (4u * (unsigned)amb.global[G_BPM]);
        for (unsigned i = 0; i < (l->role == 1 ? 1u : 4u); i++)
            if ((int32_t)(g->now + lookahead - g->breath_due[i]) >= 0) {
                unsigned wait = (int32_t)(g->breath_due[i] - g->now) > 0 ? g->breath_due[i] - g->now : 0;
                unsigned duration = (4 * FS + FS * 12u * (unsigned)l->macro[2] / 100) * stretch[i] / 100;
                duration = duration / 100 * (85 + garden_random(g, 31));
                duration = duration / 2 * stride;
                g->breath_due[i] = g->now + wait + duration;
                if (allow && (mode != 4 || i == 0)) {
                    unsigned voice = mode == 1 ? 3 - i : mode == 0 ? (i + step / 16) % 4 : i;
                    note = ambr.voicing[voice];
                    vel = 65 + garden_random(g, 25);
                    garden_schedule(k, note, vel, wait, duration / 100 * (42 + density / 4), 0);
                }
            }
    } else if (l->role == 3) {
        unsigned pc = (amb.key + scale[amb.chord % count]) % 12;
        if (garden_drift.fifth && garden_drift.age >= garden_drift.limit / 2 && amb.auto_index)
            pc = (amb.key + scale[(amb.chord + 4) % count]) % 12;
        unsigned target = garden_drift.rest ? 0 : 36 + pc;
        if (allow && (target != g->bass_note || (!g->event[0].velocity && !g->event[1].velocity))) {
            for (unsigned i = 0; i < NELEM(g->event); i++)
                if (g->event[i].playing)
                    g->event[i].off = g->now;
            g->next_event = g->now;
            g->bass_note = note = target;
            vel = 80;
            if (target)
                garden_schedule(k, note, vel, 0, FS * 600, 0);
        }
    } else if (!(step % stride)) {
        for (unsigned i = 0; i < 4; i++) {
            garden_loop_t *loop = &g->loop[i];
            unsigned pos = loop->position;
            if (allow && !loop->rest && loop->offset[pos] >= 0 && (mode != 4 || !(i & 1u))) {
                int degree = degree_base + loop->offset[pos];
                if (mode == 0)
                    degree = degree_base + 2 * (int)(pos % 4);
                if (mode == 1)
                    degree = degree_base + 2 * (int)(3 - pos % 4);
                note = garden_register[i] - (garden_register[i] - amb.key) % 12 + scale[degree % count] +
                       12 * (degree / count);
                while (note > 103)
                    note -= 12;
                note = amb_remember(k,note,0);
                vel = loop->velocity[pos];
                unsigned wait = garden_random(g, 2600);
                garden_schedule(k, note, vel, wait, FS * 3, i + 1);
                if (note <= 91 && garden_random(g, 100) < 8u + (unsigned)(arc > 0 ? arc * 12 / 100 : 0))
                    garden_schedule(k, note + 12, vel * 2 / 5, wait + 300 + garden_random(g, 2000), FS * 3, i + 1);
            }
            if (++loop->position == garden_length[i]) {
                loop->position = 0;
                garden_pass(g, i, density, variation, count, arc, 0);
            }
        }
    }
    if (note) {
        ambr.recent[k][slot] = note;
        ambr.recent_vel[k][slot] = vel;
        ambr.recent_valid[k] = 1;
    }
}
