/* SPDX-License-Identifier: GPL-3.0-only */
#define AMB_MAIN unused_amb_native_main
#include "amb_native_test.c"

int main(void)
{
    int32_t out[CTL*2];
    ui_power_on();amb_init();favorites.factory[15][13]=1;amb_fixture_scene(0);
    ambr.view=2;ambr.page=6;turn(EN_K3,1);assert(amb_crossfade());
    amb.release_ms=500;amb_prepare();panic_req=0;
    trk[0].engine=trk[0].eng_req;trk_note_on(&trk[0],60,96);
    for(unsigned i=0;i<FS/4/CTL;i++)mix_block(out,CTL);
    assert(amb_track_active(0));
    amb_patch(0,0,8);
    unsigned overlap_frames=0;
    for(unsigned i=0;i<FS/CTL;i++){
        mix_block(out,CTL);
        if(i==15)trk_note_on(&trk[0],67,90);
        if(amb_track_active(0) && amb_track_active(4) && trk[0].engine==ENGI_VOICE &&
            amb_fade_level(0,65536)>4096 && amb_fade_level(0,65536)<61000)overlap_frames+=CTL;
    }
    assert(overlap_frames>FS/5 && !ambr.handoff[4] && !ambr.fade_slot[0]);
    song.playing=1;amb_prepare();
    amb_piece_t next;amb_make_scene(&next,1);amb_install(&next);
    assert(!ambr.piece_transition);
    for(unsigned i=0;i<FS/2/CTL;i++)mix_block(out,CTL);
    assert(trk[0].eng_req==next.layer[0].engine && (ambr.handoff[4] || ambr.handoff[5]));
    song.playing=0;
    /* Pot uses the edited layer and returns to the HOME target on exit. */
    ambr.view=0;amb_target(0);amb_master(500,1);unsigned master=amb.master;
    ambr.view=1;ambr.page=1;ambr.edit_layer=3;
    amb_master(0,0);amb_master(1023,0);
    assert(amb.layer[3].level==127 && amb.master==master && ambr.target==0);
    ambr.view=0;amb_master(1023,0);assert(amb.master==master);
    amb_master(400,0);assert(amb.master==400 && amb.layer[3].level==127);
    /* A captured impulse is re-emitted through the windowed grain heads. */
    memset(&amb_grain[0],0,sizeof amb_grain[0]);amb_grain_controls[0][0]=100;
    amb_grain_controls[0][1]=58;amb_grain_controls[0][2]=7;amb_grain_controls[0][3]=40;
    int energy=0;
    for(unsigned i=0;i<150;i++){
        memset(out,0,sizeof out);
        if(i==100)for(unsigned j=0;j<CTL;j++)out[j]=12000;
        amb_grain_process(0,out,CTL);
        if(i>101)for(unsigned j=0;j<CTL;j++)energy+=abs(out[j]);
    }
    assert(energy>100);
    /* A fading-in layer at capacity may recycle only its own voices.
     * The other three chairs retain their mixer gain and released notes. */
    memset(ambr.handoff,0,sizeof ambr.handoff);
    memset(ambr.fade_slot,0,sizeof ambr.fade_slot);
    for(unsigned i=0;i<NTRK;i++)memset(trk[i].v,0,sizeof trk[i].v);
    ambr.fade_slot[0]=4;ambr.handoff[4]=1;ambr.tail_gain[4]=16384;
    for(unsigned i=1;i<4;i++){
        trk[i].v[0].active=1;
        assert(amb_fade_level(i,65536)==65536);
    }
    track_t *victim=0;
    assert(amb_fade_level(0,65536)==32768);
    assert(voice_victim(&trk[0],0,&victim)==NVOICE && quiet_tail(&trk[0],&victim)==NVOICE);
    puts("Single-layer crossfade keeps sibling gains and voices PASS");
    printf("Concurrent old and new synth voices for %u frames, song overlap, absolute layer gain and granular return PASS\n",overlap_frames);
}
