/* SPDX-License-Identifier: GPL-3.0-only */
#define AMB_MAIN unused_amb_main
#include "amb_native_test.c"

static void set_travel(int style)
{
    ambr.view=2;ambr.page=7;timing_tab=0;
    amb_walk_adjust(4,style-(int)amb_journey_style());
    amb_walk_clock(WALK_MINUTES,1);
    amb_walk_adjust(1,5);amb_walk_adjust(2,3);amb_walk_adjust(3,4);
    amb_lane_adjust(0,0,1);amb_lane_adjust(0,1,2);amb_lane_adjust(0,2,5);
    amb_lane_adjust(1,0,1);amb_lane_adjust(1,1,1);amb_lane_adjust(1,3,7);
    ambr.page=6;
    amb_knob(0,-1);amb_knob(2,-1);
}
static void recall_factory(unsigned scene)
{
    ambr.view=0;ambr.piece_preview=(uint8_t)scene;
    amb_rec();
}
static void check_defaults(void)
{
    assert(!memcmp(&amb_controls,&amb_factory_controls,sizeof amb_controls));
    assert(!amb_monitor_requested && !ambr.zen_train);
    for(unsigned i=0;i<4;i++)assert(!amb_bar_octave[i] && !amb_octave_delta(i+1));
}
int main(void)
{
    memset(amb_nor,255,sizeof amb_nor);ui_power_on();amb_init();
    const unsigned scene=AMB_SCENE_GARDEN;
    amb_factory(scene);
    check_defaults();
    /* New songs retain their own gains when a physical pot is first sampled. */
    unsigned factory_master=amb.master;
    amb_master(117,1);assert(amb.master==factory_master);
    set_travel(1);
    amb_target(0);amb_master(225,0);assert(amb.master==(unsigned)clamp((int)factory_master+108,0,1023));
    ambr.view=0;amb_target(1);
    unsigned layer_level=amb.layer[0].level;
    amb_master(141,0);assert(amb.layer[0].level<layer_level);
    amb.layer[0].base[P_REV]=77;amb.layer[0].macro[2]=117;
    amb.layer[0].generated=0;amb.layer[0].notes[2]=66;amb.layer[0].velocity[2]=104;
    amb.layer[0].base[P_MUTE]=1;amb.layer[0].solo_plus1=3;ambr.solo=2;
    amb.layer[3].reserved=0xa5;
    amb.eq[1]=-4;amb.layer_eq[1][2]=7;amb.grain[1][3]=62;
    amb.layer[1].lfo_depth=23;amb.layer[1].lfo_dest=3;
    amb.global[G_BPM]=97;amb.global[G_DFDBK]=74;amb.release_ms=15250;
    amb.key=8;amb.scale=2;amb.evolve[0]=70;
    amb.policy.active[1]=7;amb.policy.variation=62;
    amb_bar_octave[0]=-2;amb_bar_octave[3]=1;amb_monitor_requested=1;ambr.zen_train=1;
    amb_theme_turn(1);
    amb_piece_t a=amb;
    amb_song_controls_t controls=amb_controls;
    ambr.slot=0;amb_save();
    st_hdr_t hdr;assert(!st_head(amb_object(0),0,&hdr) && hdr.len==1460);

    /* The factory gesture must reload authoring defaults regardless of the
     * user's last state. Changing device theme survives song changes. */
    recall_factory(AMB_SCENE_LAST_TRAM);check_defaults();assert(amb_theme()==1);
    assert(amb.master!=a.master && amb.global[G_BPM]!=97);
    amb_theme_turn(1);
    ambr.slot=1;amb_save();
    assert(amb_recall(0) && !memcmp(&amb,&a,sizeof a));
    assert(!memcmp(amb_controls.walk,controls.walk,6) && !memcmp(amb_controls.lanes,controls.lanes,6));
    assert(amb_bar_octave[0]==-2 && amb_bar_octave[3]==1 && amb_monitor_requested && ambr.zen_train);
    assert(amb_theme()==2 && ambr.solo==2 && amb_sound_locks()==5);
    song.playing=1;assert(amb_recall(1));assert(amb.global[G_BPM]==97);
    ambr.step=16;amb_step();check_defaults();song.playing=0;
    /* Cold recall after runtime state is reinitialised. */
    amb_init();assert(amb_recall(0) && !memcmp(&amb,&a,sizeof a));assert(amb_theme()==2);
    assert(amb_bar_octave[0]==-2 && amb_monitor_requested);
    /* A naked pre-upgrade v5 import never inherits the current song. */
    assert(!st_save(amb_object(2),&a,sizeof a) && amb_recall(2));
    assert(!memcmp(&amb,&a,sizeof a));check_defaults();
    /* Both old and new full-capacity takes must fit unchanged. */
    amb_patch(0,6,0);amb.layer[0].base[P_E0]=AMB_MIC_SOURCE;
    amb_take_t *t=amb_current_take();
    t->format=1;t->root=60;t->length=AMB_TAKE_FRAMES;t->period_q8=0;
    for(unsigned b=0;b<AMB_TAKE_BLOCKS;b++){
        memset(t->block[b],(uint8_t)b,10);t->block[b][1]=1+(b%64);
    }
    amb_take_song_t old={.piece=amb,.take=*t};
    assert(!st_save(amb_object(3),&old,sizeof old));
    set_travel(0);ambr.slot=4;amb_save();
    assert(!st_head(amb_object(4),0,&hdr) && hdr.len==3668);
    memset(amb_current_take(),0,sizeof *t);assert(amb_recall(4));
    assert(amb_journey_style()==0 && !memcmp(amb_current_take(),&old.take,sizeof old.take));
    assert(amb_recall(3));check_defaults();assert(!memcmp(amb_current_take(),&old.take,sizeof old.take));
    /* A queued save owns the values at the SAVE gesture, including its trailer. */
    ambr.slot=5;amb_bar_octave[0]=2;ambr.zen_train=1;song.playing=1;
    amb_save();assert(amb_save_slot==6);
    amb_bar_octave[0]=-2;ambr.zen_train=0;song.playing=0;
    amb_commit_snapshot();assert(amb_recall(5));
    assert(amb_bar_octave[0]==2 && ambr.zen_train);
    /* Checksum-valid objects with a malformed trailer must not install. */
    amb_saved_song.extended.octave[0]=7;
    assert(!st_save(amb_object(6),&amb_saved_song,sizeof amb_saved_song));
    assert(!amb_recall(6) && amb_bar_octave[0]==2 && ambr.zen_train);
    song.playing=0;recall_factory(AMB_SCENE_ANALOG_DAYS);
    assert(amb_journey_style()==1 && amb_walk_value(0)==WALK_BARS);
    amb_walk_adjust(4,1);ambr.slot=7;amb_save();
    recall_factory(AMB_SCENE_GARDEN);check_defaults();
    song.playing=1;recall_factory(AMB_SCENE_ANALOG_DAYS);
    assert(amb_pending_ready && amb_journey_style()==2);
    ambr.step=16;amb_step();assert(amb_journey_style()==1);
    song.playing=0;assert(amb_recall(7) && amb_journey_style()==2);
    puts("PASS song-specific controls, gains, all 4 layers, factory defaults, queued/cold recall, independent theme, old raw saves and maximum-length takes.");
}
