/* SPDX-License-Identifier: GPL-3.0-only */
#define AMB_MAIN unused_amb_main
#include "amb_native_test.c"

static void effects(void)
{
    /* Direct impulse through each new processor. Verify its first actual
     * repeats and plate stereo tail without a song or a playing voice. */
    unsigned first[2];
    for (unsigned mode = 2; mode <= 4; mode++) {
        memset(&tape_fx, 0, sizeof tape_fx);
        first[0] = first[1] = 0;
        for (unsigned at = 0; at < 4 * FS; at += CTL) {
            int32_t in[CTL] = {0}, l[CTL], r[CTL];
            if (!at)
                in[0] = 24000;
            tape_run(mode, in, l, r, CTL, div_samples(7), 18022);
            for (unsigned i = 0; i < CTL; i++) {
                if (!first[0] && l[i])
                    first[0] = at + i;
                if (!first[1] && r[i])
                    first[1] = at + i;
            }
        }
        if (mode == 3) {
            assert(first[0] > FS * 2 && first[0] < FS * 3);
        } else {
            assert(first[0] > 7000 && first[0] < 7200 && first[1] > 2600 && first[1] < 2800);
        }
        printf("Delay %u first stereo returns at %u / %u frames\n", mode, first[0], first[1]);
    }
    tape_fx.kind=4;tape_fx.shortfill=3550;tape_fx.fill=TAPE_LENGTH;
    fx_trip(0,CTL);
    assert(tape_fx.kind==4 && !tape_fx.fill && !tape_fx.shortfill);
    song.g[G_DTYPE]=0;
    assert(!tape_select() && fx.dsw==2);
    rev_clear();
    uint64_t stereo = 0, late = 0;
    for (unsigned at = 0; at < 2 * FS; at += CTL) {
        int32_t in[CTL] = {0}, out[CTL] = {0};
        memset(fx_sl, 0, sizeof fx_sl);
        memset(fx_sr, 0, sizeof fx_sr);
        if (!at)
            in[0] = 32000;
        rev_run(5, in, out, CTL);
        for (unsigned i = 0; i < CTL; i++) {
            stereo += abs(fx_sl[i] - fx_sr[i]);
            if (at > FS)
                late += abs(out[i]);
        }
    }
    assert(stereo && late);
    printf("Plate stereo difference %llu and second-second tail %llu\n", (unsigned long long)stereo,
           (unsigned long long)late);
}
static void loops(void)
{
    amb_fixture_scene(11);
    song.playing = 1;
    for (unsigned k = 0; k < 4; k++)
        trk[k].engine = amb.layer[k].engine;
    for (unsigned step = 0; step < 4 * 29 * 8; step++) {
        amb_step();
        garden_layer_t *g = &garden_layer[1];
        for (unsigned i = 0; i < 4; i++)
            assert(g->loop[i].position == (step / 4 + 1) % garden_length[i]);
        for (unsigned i = 0; i < NELEM(g->event); i++)
            if (g->event[i].velocity)
                assert(amb_pool() >> ((g->event[i].note + 12 - amb.key) % 12) & 1u);
        for (unsigned k = 0; k < 4; k++)
            garden_advance(k, FS * 60 / 72 / 4);
    }
    song.playing = 0;
    amb_clock(CTL);
    for (unsigned k = 0; k < 4; k++)
        assert(!garden_layer[k].ready);
    puts("Concurrent 13/17/23/29 beat loops, pitch pool and transport cleanup PASS");
}

int main(void)
{
    memset(amb_nor, 255, sizeof amb_nor);
    ui_power_on();
    amb_init();
    assert(ambr.preset_position == 11);
    assert(!strcmp(AMB_SCENE[11], "Garden") && amb_valid(&amb));
    assert(amb.key == 2 && amb.scale == 2 && amb.global[G_BPM] == 72 && amb.policy.active[0] == 8);
    assert(amb.global[G_RTYPE] == 5 && amb.global[G_DTYPE] == 4);
    assert(!strcmp(amb_patch_name(&amb.layer[0]), "Garden Rhodes"));
    assert(!strcmp(amb_patch_name(&amb.layer[1]), "Garden Bell"));
    static const uint8_t bank[] = {AMB_SCENE_GARDEN, AMB_SCENE_INN,
        AMB_SCENE_LAST_TRAM, AMB_SCENE_FIRST_FERRY, AMB_SCENE_EVENING_ARRAY,
        AMB_SCENE_FUMAROLE, AMB_SCENE_WINDOW_LIGHT, AMB_SCENE_LINEN_AND_BOW,
        AMB_SCENE_UMBRA_MERIDIAN, AMB_SCENE_LONG_LEE, AMB_SCENE_COLD_DESERT,
        AMB_SCENE_EMBER_MERIDIAN};
    for(unsigned i=0;i<NELEM(bank);i++){
        assert(amb_next_piece(AMB_SCENE_GARDEN,(int)i)==bank[i]);
        assert(amb_next_piece(bank[i],-(int)i)==AMB_SCENE_GARDEN);
    }
    assert(amb_next_piece(AMB_SCENE_GARDEN,NELEM(bank))==AMB_SCENE_GARDEN);
    turn(EN_PRESET,-1);
    assert(ambr.piece_preview==AMB_SCENE_EMBER_MERIDIAN);
    press(B_REC);
    assert(ambr.preset_position==AMB_SCENE_EMBER_MERIDIAN && amb_valid(&amb));
    /* A different arrangement can borrow the conductor without importing sounds or FX. */
    amb_fixture_scene(6);
    amb.layer[0].generated = 0;
    amb.layer[0].notes[3] = 65;
    amb_layer_t prior[4];
    memcpy(prior, amb.layer, sizeof prior);
    amb_policy_choose(&amb.policy, 0, 8, 1);
    amb_policy_boundary(&amb.policy, 63);
    assert(amb.policy.active[0] != 8);
    amb_policy_boundary(&amb.policy, 64);
    assert(amb.policy.active[0] == 8);
    seq_start();
    amb_render(9 * FS);
    seq_stop();
    amb_render(CTL);
    assert(!memcmp(prior, amb.layer, sizeof prior) && amb.global[G_RTYPE] == 0);
    amb_patch(1, 0, 16);
    amb_policy_choose(&amb.policy, 0, 2, 0);
    assert(amb.policy.active[0] == 2 && !strcmp(amb_patch_name(&amb.layer[1]), "Garden Rhodes"));
    song.g[G_BPM] = 72;
    effects();
    loops();
    puts("Garden factory, independent selection, manual track and boundary PASS");
}
