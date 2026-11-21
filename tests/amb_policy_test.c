/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/src/amb_policy.h"
#include "../firmware/src/amb_harmony.h"
static uint64_t trace(amb_policy_t *p)
{
    const amb_participation_t layers[4] = {{1,40},{1,35},{1,48},{1,25}};
    uint64_t h = 1469598103934665603ull;
    unsigned step;
    for (step = 0; step < 4096; step++) {
        h ^= amb_policy_events(p, step, layers); h *= 1099511628211ull;
    }
    return h;
}
int main(void)
{
    amb_policy_t p;
    unsigned family, style, j, step;
    uint64_t hashes[9];
    amb_policy_init(&p, 42);
    amb_policy_choose(&p, 0, 5, 1);
    amb_policy_choose(&p, 0, 7, 1);
    amb_policy_choose(&p, 1, 3, 1);
    for (step = 1; step < 64; step++) { amb_policy_boundary(&p, step); assert(p.active[0] == 0); }
    amb_policy_boundary(&p, 64);
    assert(p.active[0] == 7 && p.active[1] == 3);
    amb_policy_choose(&p, 0, 3, 1);
    amb_policy_choose(&p, 0, 7, 1);
    assert(p.pending[0] == 255);
    for (family = 0; family < 3; family++) {
        for (style = 0; style < amb_style_count(family); style++) {
            amb_policy_init(&p, 42);
            amb_policy_choose(&p, family, style, 0);
            hashes[style] = trace(&p);
            assert(hashes[style] == trace(&p));
            for (j = 0; j < style; j++) assert(hashes[style] != hashes[j]);
        }
    }
    for (j = 0; j < 33; j++) {
        uint32_t b = amb_euclid(j, 32, 3); unsigned n = 0;
        for (step = 0; step < 32; step++) n += (b >> step) & 1u;
        assert(n == j);
    }
    for (j = 0; j < 7; j++) {
        int pcs[4], from[4] = {48,55,63,70}, to[4], count;
        count = amb_chord_tones(0x5adu, 0, j, pcs); /* C minor */
        amb_voice_lead(from, pcs, count, to, 48, 77);
        for (step = 0; step < 4; step++) {
            int found = 0; unsigned k;
            assert(to[step] >= 48 && to[step] <= 77);
            for (k = 0; k < (unsigned)count; k++) found |= to[step] % 12 == pcs[k];
            assert(found);
        }
    }
    {
        amb_participation_t l[4] = {{0,100},{1,40},{0,100},{1,20}}, before[4];
        memcpy(before,l,sizeof l);
        for (step = 0; step < 100000; step++) assert(!(amb_policy_events(&p,step,l) & 5u));
        assert(!memcmp(l,before,sizeof l));
    }
    puts("AMB+ policies: all 24 distinct/repeatable, phrase queue/cancel, opt-in immutability, Euclid, harmony PASS");
    return 0;
}
