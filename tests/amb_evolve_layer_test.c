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
    puts("Layer evolution browse, manual patch retention, queued role replacement and sibling isolation PASS");
}
