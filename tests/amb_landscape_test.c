/* SPDX-License-Identifier: GPL-3.0-only */
#define AMB_MAIN unused_main
#include "amb_native_test.c"
int main(void)
{
    memset(amb_nor,255,sizeof amb_nor);ui_power_on();amb_init();
    amb.layer[1].generated=0;amb.layer[1].notes[3]=67;
    amb_layer_t initial[4];memcpy(initial,amb.layer,sizeof initial);
    song.playing=1;ambr.step=1;unsigned chord=amb.chord;
    turn(EN_ALGO,1);assert(travel.wanted==1 && ambr.proposal==255 && amb.chord==chord);
    while(ambr.step<16)amb_step();
    assert(amb.chord==chord && amb.layer[0].macro[3]>initial[0].macro[3]);
    amb_step();assert(amb.chord!=chord && travel.harmony==1);
    song.playing=0;for(unsigned k=0;k<4;k++)memset(trk[k].v,0,sizeof trk[k].v);
    turn(EN_ALGO,17);assert(travel.wanted==18 && amb.layer[0].patch!=initial[0].patch);
    assert(!memcmp(amb.layer[1].notes,initial[1].notes,16) && amb.layer[1].patch==initial[1].patch);
    turn(EN_ALGO,-18);
    for(unsigned k=0;k<4;k++)assert(!memcmp(&amb.layer[k],&initial[k],sizeof initial[k]));
    assert(amb.chord==chord && amb_valid(&amb));
    amb_fixture_scene(0);assert(!travel.ready && !travel.wanted);
    puts("Live landscape: bar-queued harmony, gradual colour, instrument thresholds, manual pattern and return PASS");
}
