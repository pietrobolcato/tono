/* SPDX-License-Identifier: GPL-3.0-only */
/* Pure event policies: only global policy state can change. The layer
 * participation/density input is const; no patches or patterns are reachable. */
#ifndef AMB_POLICY_H
#define AMB_POLICY_H
#include <stdint.h>
static const char *const AMB_RHYTHMS[] = {
    "AMB DRIFT", "EUCLIDEAN", "ODD LOOPS", "POLYRHYTHM", "SPARSE", "TIDAL", "BROKEN", "SLOW CLOCK","GARDEN"};
enum { AMB_ACID_PHRASE = 8 };
static const char *const AMB_PHRASES[] = {
    "AMB BREATHE", "LONG ARC", "WANDER", "BLOOM", "CALL ANSWER", "DRIFT SHIFT", "MEMORY", "MINIMAL", "ACID LINE"};
static const char *const AMB_INTERPLAY[6] = {
    "BLEND", "STAGGER", "ALTERNATE", "COMPLEMENT", "SPARSE TALK", "TIDE TOGETHER"};
typedef struct {
    uint32_t seed;
    uint8_t active[3], pending[3], variation;
} amb_policy_t;
typedef struct { uint8_t enabled, density; } amb_participation_t;
static uint32_t amb_hash(uint32_t x)
{
    x ^= x >> 16; x *= 0x7feb352du;
    x ^= x >> 15; x *= 0x846ca68bu;
    return x ^ (x >> 16);
}
static void amb_policy_init(amb_policy_t *p, uint32_t seed)
{
    unsigned i;
    p->seed = seed;
    p->variation = 40;
    for (i = 0; i < 3; i++) { p->active[i] = 0; p->pending[i] = 255; }
}
static unsigned amb_style_count(unsigned family){return family<2?9u:6u;}
static void amb_policy_choose(amb_policy_t *p, unsigned family, unsigned style, int playing)
{
    unsigned n = amb_style_count(family);
    if (family > 2 || style >= n) return;
    if (!playing) { p->active[family] = (uint8_t)style; p->pending[family] = 255; }
    else p->pending[family] = style == p->active[family] ? 255 : (uint8_t)style;
}
static void amb_policy_boundary(amb_policy_t *p, uint32_t step)
{
    unsigned i;
    if (step % 64u) return;
    for (i = 0; i < 3; i++)
        if (p->pending[i] != 255) { p->active[i] = p->pending[i]; p->pending[i] = 255; }
}
static unsigned amb_triangle(unsigned t, unsigned period)
{
    unsigned ph = t % period;
    return (ph < period / 2 ? ph : period - ph) * 100u / (period / 2);
}
static int amb_rhythm_event(const amb_policy_t *p, uint32_t t, unsigned layer, unsigned density)
{
    static const uint8_t odd[4] = {5, 7, 11, 13};
    unsigned ph, len, hits, r = amb_hash(p->seed + t * 31u + layer * 997u) % 100u;
    density = density > 100 ? 100 : density;
    switch (p->active[0]) {
    default:
    case 0: len = odd[layer]; hits = 1 + density / 30;
        return t % 3u == layer % 3u && (t % len) * hits % len < hits;
    case 1: hits = 1 + density / 14;
        return (((t + layer * 3u) % 16u) * hits % 16u) < hits;
    case 2: return (t + layer) % odd[layer] == 0;
    case 3: return (t + layer * 2u) % (layer % 2 ? 7u : 3u) == 0 && t % 4u != 3;
    case 4: return t % 4u == 0 && r < 8u + density / 4u;
    case 5: ph = amb_triangle(t / 4u + layer * 7u, 64);
        return r < (density * (15u + ph) / 160u) && t % 2u == 0;
    case 6: ph = (t + layer * 3u) % 23u;
        return ph == 0 || ph == 5 || ph == 13 || (ph == 18 && r < density);
    case 8: return (t % 4u)==0; /* quarter-note bell loop clock in the rhythm preview */
    case 7: return (t + layer * 4u) % 32u == 0;
    }
}
static int amb_phrase_event(const amb_policy_t *p, uint32_t t, unsigned layer)
{
    unsigned bar = t / 16u, r = amb_hash(p->seed ^ (bar * 17u + layer * 79u)) % 100u;
    switch (p->active[1]) {
    default:
    case 0: return r < 25u + amb_triangle(bar + layer * 3u, 24) * 3u / 4u;
    case 1: return r < 10u + amb_triangle(bar, 128) * 9u / 10u;
    case 2: return (bar + amb_hash(p->seed + bar / 3u) % 7u + layer) % 7u != 0;
    case 3: return layer <= amb_triangle(bar, 32) / 25u && bar % 32u != 31;
    case 4: return (bar / 2u + layer) % 4u < 2u;
    case 5: return (t + bar / 4u + layer * 7u) % 37u < 24u;
    case 6: return (bar % 16u < 4u || (bar % 16u >= 8u && bar % 16u < 12u)) && r < 82u;
    case AMB_ACID_PHRASE: return 1; /* Rhythm supplies the cues for this line. */
    case 7: return bar % 16u < 2u && t % 8u == 0;
    }
}
static uint8_t amb_policy_events(const amb_policy_t *p, uint32_t step,
                                  const amb_participation_t layers[4])
{
    unsigned i, mask = 0, emitted = 0;
    for (i = 0; i < 4; i++) {
        unsigned t = step, v = p->variation > 100 ? 100 : p->variation;
        unsigned r = amb_hash(p->seed + step * 131u + i * 3907u) % 100u;
        int on;
        if (!layers[i].enabled) continue;
        if (p->active[2] == 1) t += i * 3u;
        if (p->active[2] == 5 && (step / 16u) % 12u >= 8u) continue;
        on = amb_rhythm_event(p, t, i, layers[i].density) && amb_phrase_event(p, t, i);
        if (v && r < v / 12u && step % 4u == i) on = 1;
        switch (p->active[2]) {
        case 2: on &= (step / 16u) % 4u == i; break;
        case 3: on = on ? !emitted : !emitted && step % 8u == (i * 2u) && r < 35; break;
        case 4: on &= !emitted && (step + i) % 3u == 0; break;
        case 5: on &= r < amb_triangle(step / 4u, 48); break;
        default: break;
        }
        if (on) { mask |= 1u << i; emitted++; }
    }
    return (uint8_t)mask;
}
#endif
