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
static void auto_sound_browser(void)
{
    amb_fixture_scene(AMB_SCENE_GARDEN);
    amb_target(1);press(B_EDIT);press(B_EDIT);
    assert(amb_sound_picker());
    for(unsigned role=0;role<8;role++){
        amb_replace_role(0,role,0);
        unsigned count=amb_sound_count(role);
        amb.layer[0].macro[2]=120;
        amb.layer[0].base[P_REV]=123;
        amb.layer_eq[0][2]=11;
        amb.grain[0][0]=97;
        amb_controls.layer_thinning[0]=90;
        amb_controls.filter[0]=18;
        amb_controls.second[0].depth=70;
        amb_piece_t before=amb;
        /* The last named entry, then Auto, then the first named entry. */
        amb_browse_proposal(count-1);
        assert(ambr.proposal==count-1);
        amb_browse_proposal(1);
        unsigned choice=amb_auto_sound;
        assert(ambr.proposal==count && choice>0 && choice<(role==5?amb_patch_count(role):count));
        assert(amb_controls.layer_thinning[0]==90 && amb_controls.second[0].depth==70);
        assert(role!=6 || choice!=AMB_MIC_SOURCE);
        assert(!memcmp(&amb,&before,sizeof before));
        amb_browse_proposal(1);
        assert(ambr.proposal==255);
        amb_browse_proposal(-1);
        press(B_HOME);
        assert(ambr.proposal==255 && !memcmp(&amb,&before,sizeof before));
        amb_browse_proposal(-1);
        assert(amb_auto_sound==choice);
        ambr.step+=48;
        press(B_REC);
        assert(ambr.proposal==255 && amb.layer[0].role==role);
        assert(!amb_controls.layer_thinning[0] && !amb_controls.second[0].depth && !amb_controls.filter[0]);
        assert(!amb.layer_eq[0][2] && !amb.grain[0][0]);
        assert(amb.layer[0].macro[2]==64 && amb.layer[0].base[P_REV]!=123);
        assert(!memcmp(&before.layer[1],&amb.layer[1],3*sizeof(amb_layer_t)));
        assert((role==6?amb.layer[0].base[P_E0]:amb.layer[0].patch)==choice);
    }
    amb_fixture_scene(AMB_SCENE_GARDEN);
    /* With the bright candidate already present, a different engine wins. */
    unsigned chapter=0,bright,dark;
    for(;chapter<64;chapter++){
        bright=amb_chapter_sound(0,0,1,chapter,0,0);
        dark=amb_chapter_sound(0,0,-1,chapter,0,0);
        if(amb_recipe(0,bright)->engine!=amb_recipe(0,dark)->engine)break;
    }
    assert(chapter<64);
    ambr.step=chapter*16;
    for(unsigned j=1;j<4;j++){amb.layer[j].engine=amb_recipe(0,bright)->engine;amb.layer[j].base[P_MUTE]=0;amb.layer[j].level=80;}
    assert(amb_pick_auto_sound(0,0)==dark);
    for(unsigned j=1;j<4;j++)amb.layer[j].base[P_MUTE]=1;
    assert(amb_pick_auto_sound(0,0)==bright);
    puts("Auto sound follows the mix, browses silently, wraps, cancels and commits the displayed recommendation for all roles PASS");
}
static void texture_family_browser(void)
{
    unsigned char seen[NELEM(amb_texture_patches)]={0};
    amb_fixture_scene(AMB_SCENE_GARDEN);
    amb_target(1);press(B_EDIT);press(B_EDIT);
    unsigned families=amb_sound_count(5);
    assert(families==11 && amb_patch_count(5)==32);
    unsigned voices[]={22,23,24,29};
    assert(amb_texture_count(22)==NELEM(voices));
    for(unsigned i=0;i<NELEM(voices);i++)assert(amb_texture_member(22,i)==voices[i]);
    for(unsigned family=0;family<families;family++){
        unsigned head=amb_sound_from_browser(5,family);
        for(unsigned i=0;i<amb_texture_count(head);i++){
            unsigned member=amb_texture_member(head,i);
            assert(member<NELEM(seen) && amb_texture_family(member)==family && !seen[member]);
            seen[member]=1;
            amb_patch(0,5,member);
            amb_piece_t before=amb;
            amb_browse_proposal(1);
            assert(ambr.proposal==family+1 && !memcmp(&before,&amb,sizeof amb));
            amb_browse_proposal(-1);
            assert(ambr.proposal==255 && !memcmp(&before,&amb,sizeof amb));
            if(family+1<families){
                amb_browse_proposal(1);amb_rec();
                assert(amb.layer[0].patch==amb_sound_from_browser(5,family+1));
            }
        }
    }
    for(unsigned member=0;member<NELEM(seen);member++)assert(seen[member]);
    printf("All %u texture IDs reachable across %u sound families; silent browse and stable member ID PASS\n",amb_patch_count(5),families);
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
    auto_sound_browser();
    texture_family_browser();
    puts("Focused sound matches the first selected-seat Auto destination in all three journeys and both directions PASS");
    puts("EDIT MODE Texture to Pad restores role timing, octave, grain and EQ; gain and siblings preserved PASS");
    puts("Layer evolution browse, manual patch retention, queued role replacement and sibling isolation PASS");
}
