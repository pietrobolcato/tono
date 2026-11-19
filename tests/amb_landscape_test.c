/* SPDX-License-Identifier: GPL-3.0-only */
#define AMB_MAIN unused_main
#include "amb_native_test.c"
static void evolving_manual_turns(unsigned style)
{
    amb_fixture_scene(AMB_SCENE_GARDEN);ambr.view=0;song.playing=0;
    amb_walk_config()[4]=style;amb_journey_recentre();
    amb.layer[0].generated=0;amb.layer[0].notes[5]=65;amb.layer[0].velocity[5]=91;
    unsigned previous[4],older[4]={255,255,255,255};
    for(unsigned k=0;k<4;k++)previous[k]=amb.layer[k].patch;
    for(unsigned phrase=0;phrase<10;phrase++){
        unsigned span=style==1?4:3;
        for(unsigned i=0;i<span;i++)turn(EN_ALGO,phrase%2?-1:1);
        unsigned changed=0;
        for(unsigned k=0;k<4;k++){
            unsigned patch=amb.layer[k].patch;
            if(patch!=previous[k]){
                assert(patch!=older[k]);older[k]=previous[k];previous[k]=patch;changed++;
            }else if(style==2)assert(k==travel.anchor);
        }
        assert(changed==(style==1?1:3));
        assert(amb_valid(&amb) && !amb.layer[0].generated && amb.layer[0].notes[5]==65 && amb.layer[0].velocity[5]==91);
    }
    assert(travel.wanted==0 && travel.chapter==10);
    song.playing=1;turn(EN_ALGO,style==1?4:3);
    unsigned waiting=travel.pending_layers;assert(waiting);
    amb_landscape_bar();assert(__builtin_popcount(waiting)-__builtin_popcount(travel.pending_layers)==1);
    song.playing=0;
}
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
    assert(!memcmp(amb.layer[1].notes,initial[1].notes,16) && amb.layer[1].patch!=initial[1].patch);
    turn(EN_ALGO,-18);
    for(unsigned k=0;k<4;k++)assert(!memcmp(&amb.layer[k],&initial[k],sizeof initial[k]));
    assert(amb.chord==chord && amb_valid(&amb));
    amb_fixture_scene(0);assert(!travel.ready && !travel.wanted);
    song.playing=0;ambr.view=2;ambr.page=6;
    amb_knob(0,1);assert(amb_endless());
    ambr.view=0;
    for(int i=1;i<=80;i++){
        turn(EN_ALGO,1);assert(travel.wanted==i && amb_valid(&amb));
    }
    unsigned chord80=amb.chord,patch80=amb.layer[0].patch;
    turn(EN_ALGO,-2);turn(EN_ALGO,2);
    assert(travel.wanted==80 && amb.chord==chord80 && amb.layer[0].patch==patch80);
    for(int i=79;i>=-80;i--){turn(EN_ALGO,-1);assert(travel.wanted==i && amb_valid(&amb));}
    ambr.view=2;ambr.page=6;amb_knob(0,-1);
    assert(!amb_endless() && travel.wanted==-18);
    ambr.view=0;turn(EN_ALGO,18);turn(EN_ALGO,4);
    ambr.view=1;ambr.page=1;ambr.edit_layer=0;turn(EN_ALGO,1);press(B_REC);
    unsigned picked=amb.layer[0].patch;
    ambr.view=0;turn(EN_ALGO,-4);assert(amb.layer[0].patch==picked);
    turn(EN_ALGO,4);assert(amb.layer[0].patch!=picked);
    evolving_manual_turns(1);evolving_manual_turns(2);
    puts("Manual chapters and branches continue through reversals, retain an anchor and respect bar handoffs PASS");
    puts("Edited patch remains eligible, and becomes the return sound PASS");
    puts("Endless travel across both boundaries and reversible chord/patch selections PASS");
    puts("Live landscape: bar-queued harmony, gradual colour, instrument thresholds, manual pattern and return PASS");
}
