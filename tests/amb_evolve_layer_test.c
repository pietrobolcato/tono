/* SPDX-License-Identifier: GPL-3.0-only */
#define AMB_MAIN unused_test_main
#include "amb_native_test.c"
static void compare_focused_with_auto(unsigned style,int direction)
{
    amb_fixture_scene(AMB_SCENE_GARDEN);ambr.view=0;amb_walk_config()[4]=style;
    ambr.evolve_layer=0;ambr.evolve_direction=direction;ambr.evolve_mode=0;
    ambr.evolve_anchor=travel.anchor;ambr.evolve_chapter=travel.chapter;
    ambr.evolve_previous=travel.previous_sound[0];
    unsigned start=amb.layer[0].patch;
    amb_evolution_preview();
    unsigned proposed=ambr.evolve_patch;
    assert(proposed!=start);
    for(unsigned i=0;i<18 && amb.layer[0].patch==start;i++)amb_landscape_turn(direction);
    assert(amb.layer[0].patch==proposed);
}
int main(void)
{
    memset(amb_nor,255,sizeof amb_nor);ui_power_on();amb_init();amb_target(1);
    amb.layer[0].generated=0;amb.layer[0].notes[4]=71;
    amb_piece_t original=amb;
    fm1_in.buttons|=1u<<panel.btn[B_HOME];frame();turn(EN_ALGO,-1);
    fm1_in.buttons&=~(1u<<panel.btn[B_HOME]);frame();
    assert(ambr.evolve_open && !memcmp(&amb,&original,sizeof amb));
    assert(!ambr.evolve_mode);
    unsigned sound=ambr.evolve_patch;
    turn(EN_SELECT,1);assert(!ambr.evolve_mode && ambr.evolve_patch==sound);
    turn(EN_ALGO,1);assert(ambr.evolve_mode);
    unsigned role=ambr.evolve_role,patch=ambr.evolve_patch;
    turn(EN_ALGO,3);assert(ambr.evolve_mode && ambr.evolve_role==role && ambr.evolve_patch==patch);
    turn(EN_ALGO,-1);assert(!ambr.evolve_mode && ambr.evolve_patch==sound);
    assert(!memcmp(&amb,&original,sizeof amb));
    press(B_REC);assert(amb.layer[0].patch==sound);
    assert(!ambr.evolve_open && amb.layer[0].role==original.layer[0].role && !amb.layer[0].generated && amb.layer[0].notes[4]==71);
    fm1_in.buttons|=1u<<panel.btn[B_HOME];frame();turn(EN_ALGO,1);
    fm1_in.buttons&=~(1u<<panel.btn[B_HOME]);frame();
    turn(EN_ALGO,1);unsigned next=ambr.evolve_role;
    assert(ambr.evolve_mode && next!=amb.layer[0].role);
    for(unsigned j=1;j<4;j++)assert(next!=amb.layer[j].role);
    assert(!memcmp(&amb.layer[1],&original.layer[1],3*sizeof(amb_layer_t)));
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
    for(unsigned style=0;style<3;style++)for(int sign=-1;sign<=1;sign+=2)
        compare_focused_with_auto(style,sign);
    amb_factory(AMB_SCENE_GARDEN);
    uint8_t proposed_role,proposed_patch;
    assert(amb_role_proposal(0,amb_hash(amb.policy.seed+4),&proposed_role,&proposed_patch));
    lane_clock[1].serial=4;amb_change_role();
    assert(amb.layer[0].role==proposed_role && (proposed_role==6?
        amb.layer[0].base[P_E0]:amb.layer[0].patch)==proposed_patch);
    amb.layer[0].role=6;amb.layer[0].base[P_E0]=AMB_MIC_SOURCE;
    assert(!amb_role_proposal(0,0,&proposed_role,&proposed_patch));
    puts("Focused sound matches the first selected-seat Auto destination in all three journeys and both directions PASS");
    puts("EDIT MODE Texture to Pad restores role timing, octave, grain and EQ; gain and siblings preserved PASS");
    puts("Layer evolution browse, manual patch retention, queued role replacement and sibling isolation PASS");
}
