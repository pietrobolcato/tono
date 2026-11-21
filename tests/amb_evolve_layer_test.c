/* SPDX-License-Identifier: GPL-3.0-only */
#define AMB_MAIN unused_test_main
#include "amb_native_test.c"
int main(void)
{
    memset(amb_nor,255,sizeof amb_nor);ui_power_on();amb_init();amb_target(1);
    amb.layer[0].generated=0;amb.layer[0].notes[4]=71;
    amb_piece_t original=amb;
    fm1_in.buttons|=1u<<panel.btn[B_HOME];frame();turn(EN_ALGO,1);
    fm1_in.buttons&=~(1u<<panel.btn[B_HOME]);frame();
    assert(ambr.evolve_open && !memcmp(&amb,&original,sizeof amb));
    turn(EN_ALGO,1);press(B_REC);
    assert(!ambr.evolve_open && amb.layer[0].role==original.layer[0].role && !amb.layer[0].generated && amb.layer[0].notes[4]==71);
    fm1_in.buttons|=1u<<panel.btn[B_HOME];frame();turn(EN_ALGO,1);
    fm1_in.buttons&=~(1u<<panel.btn[B_HOME]);frame();
    turn(EN_SELECT,1);unsigned next=ambr.evolve_role;
    song.playing=1;press(B_REC);assert(ambr.evolve_queued && amb.layer[0].role!=next);
    amb_commit_evolution();assert(amb.layer[0].role==next && amb.layer[0].generated && amb_valid(&amb));
    for(unsigned i=1;i<4;i++)assert(!memcmp(&amb.layer[i],&original.layer[i],sizeof(amb_layer_t)));
    song.playing=0;
    amb_factory(AMB_SCENE_GARDEN);
    amb_target(1);press(B_EDIT);
    assert(ambr.view==1 && ambr.page==0 && ambr.edit_layer==0);
    turn(EN_ALGO,5);press(B_REC); /* Pad to Texture. */
    assert(amb.layer[0].role==5);
    amb.layer[0].generated=0;amb.layer[0].notes[4]=71;
    amb.layer[0].level=63;amb_bar_octave[0]=2;
    amb.grain[0][0]=90;amb.layer_eq[0][0]=-9;
    original=amb;
    turn(EN_ALGO,-5); /* A proposal alone preserves the playing layer. */
    assert(!memcmp(&original,&amb,sizeof amb));
    press(B_REC);
    assert(amb.layer[0].role==0 && amb.layer[0].patch==0);
    assert(amb.layer[0].generated && amb_bar_octave[0]==0);
    assert(amb.layer[0].level==63 && amb.layer[0].speed==amb_reset_layer[0].speed);
    assert(!memcmp(amb.grain[0],amb_reset_grain[0],4));
    assert(!memcmp(amb.layer_eq[0],amb_reset_eq[0],4));
    assert(!memcmp(&amb.layer[1],&original.layer[1],3*sizeof(amb_layer_t)));
    puts("EDIT MODE Texture to Pad restores role timing, octave, grain and EQ; gain and siblings preserved PASS");
    puts("Layer evolution browse, manual patch retention, queued role replacement and sibling isolation PASS");
}
