/* SPDX-License-Identifier: GPL-3.0-only */
#define AMB_MAIN unused_amb_native_main
#include "amb_native_test.c"
int main(void)
{
    unsigned i,k,t,cap[]={250,500,3000,6000,30000};int32_t out[CTL*2];
    memset(amb_nor,255,sizeof amb_nor);ui_power_on();amb_init();favorites.factory[15][13]=1;
    /* Q24 envelope x Q15 gain must retain full precision, without 32-bit
     * multiplication overflow that would collapse the audible release. */
    ambr.tail_env[0][0]=1<<24;ambr.tail_gain[0]=32768;assert(amb_tail_envelope(0,0)==1<<24);
    ambr.tail_gain[0]=16384;assert(amb_tail_envelope(0,0)==1<<23);

    for(t=0;t<NELEM(cap);t++){
        song.playing=0;amb_fixture_scene(0);ambr.view=2;ambr.page=6;amb_knob(1,((int)cap[t]-amb.release_ms)/250);assert(amb.release_ms==cap[t] && amb_valid(&amb));
        amb_patch(0,3,0);amb_prepare();trk[0].engine=trk[0].eng_req;panic_req=0;
        trk_note_on(&trk[0],60,100);for(i=0;i<FS/5u/CTL;i++)mix_block(out,CTL);
        assert(amb_track_active(0));amb_target(1);ambr.view=1;ambr.page=0;
        turn(EN_ALGO,1);press(B_REC);assert(amb.layer[0].role==4 && ambr.handoff[0]);
        {unsigned elapsed=0;int64_t late_energy=0,dry_energy=0;unsigned total=(cap[t]*FS+999)/1000;
            while(ambr.handoff[0] && elapsed<total+CTL*4){
                mix_block(out,CTL);elapsed+=CTL;
                if(cap[t]==30000 && elapsed==CTL*100){
                    unsigned remaining=ambr.tail_left[0];amb_begin_change(0);
                    assert(ambr.tail_left[0]==remaining);
                }
                if(elapsed<total*9/10)assert(ambr.handoff[0] && amb_track_active(0));
                if(elapsed>total*7/10 && elapsed<total*9/10)for(k=0;k<CTL*2;k++)late_energy+=(int64_t)out[k]*out[k];
                if(!dry_energy && elapsed>total*8/10 && elapsed<total*9/10){
                    int32_t dry[CTL];track_render(&trk[0],dry,CTL);
                    for(k=0;k<CTL;k++)dry_energy+=(int64_t)dry[k]*dry[k];
                }
            }
            assert(elapsed>=total && elapsed<=total+CTL*3);assert(late_energy>1000 && dry_energy>1000);
            printf("Bass -> Drums %u ms: %u frames (minimum %u), audible late tail PASS\n",cap[t],elapsed,total);
        }
    }
    /* Fast internal FM envelopes must not end the outgoing voice early. */
    song.playing=0;amb_fixture_scene(0);amb.release_ms=3000;amb_patch(0,2,0);
    amb_prepare();trk[0].engine=trk[0].eng_req;panic_req=0;trk_note_on(&trk[0],60,100);
    for(i=0;i<FS/5u/CTL;i++)mix_block(out,CTL);assert(amb_track_active(0));amb_patch(0,0,0);
    for(i=0;i<FS*59u/10u/CTL;i++)mix_block(out,CTL);
    assert(ambr.handoff[0] && amb_track_active(0));
    for(i=0;i<FS/5u/CTL;i++)mix_block(out,CTL);assert(!ambr.handoff[0]);
    /* Ordinary musical note-off keeps its original short release. */
    amb_fixture_scene(0);amb_patch(0,3,0);amb_prepare();trk[0].engine=trk[0].eng_req;panic_req=0;
    trk[0].p[P_REL]=0;trk_note_on(&trk[0],60,100);for(i=0;i<FS/5u/CTL;i++)track_render(&trk[0],out,CTL);
    trk_note_off(&trk[0],60);for(i=0;i<FS/2u/CTL;i++)track_render(&trk[0],out,CTL);
    assert(!amb_track_active(0) && !ambr.handoff[0]);
    /* A user PHRASE commits on four bars and releases only generated parts. */
    song.playing=0;amb_fixture_scene(0);amb.layer[2].generated=0;amb.layer[2].notes[3]=61;
    {amb_layer_t manual=amb.layer[2];
        amb_prepare();for(k=0;k<4;k++)trk[k].engine=trk[k].eng_req;panic_req=0;
        trk_note_on(&trk[0],60,100);for(i=0;i<FS/5u/CTL;i++)mix_block(out,CTL);
        ambr.step=63;amb_policy_choose(&amb.policy,1,3,1);amb_step();assert(!ambr.handoff[0]);
        amb_step();assert(amb.policy.active[1]==3 && ambr.handoff[0] && !ambr.handoff[2]);
        assert(!memcmp(&manual,&amb.layer[2],sizeof manual));}
    /* Whole-song recalls hold old FX, output EQ and master until all old
     * voices have released; silent incoming layers cannot enter early. */
    song.playing=0;amb_fixture_scene(0);amb_prepare();for(k=0;k<4;k++)trk[k].engine=trk[k].eng_req;panic_req=0;
    trk_note_on(&trk[0],60,100);for(i=0;i<FS/5u/CTL;i++)mix_block(out,CTL);
    {amb_piece_t next;int old_fx=song.g[G_RTYPE];unsigned old_master=amb.master;
        amb_make_scene(&next,1);next.global[G_RTYPE]=(old_fx+1)%5;next.master=400;next.eq[0]=6;
        amb_install(&next);assert(ambr.piece_transition && ambr.handoff[3]);
        amb_prepare();assert(song.g[G_RTYPE]==old_fx && ambr.piece_master==old_master);
        for(i=0;i<FS*59u/10u/CTL;i++)mix_block(out,CTL);
        assert(ambr.piece_transition && song.g[G_RTYPE]==old_fx);
        for(i=0;i<FS/5u/CTL;i++)mix_block(out,CTL);
        assert(!ambr.piece_transition && song.g[G_RTYPE]==next.global[G_RTYPE] && !amb_tail_any());}
    /* HOME header follows accepted layer gain, not selection or pot jitter. */
    amb_fixture_scene(0);ambr.view=0;amb_target(4);fm1_ms=1000;
    {unsigned master=amb.master;amb_master(0,0);amb_master(1023,0);
        assert(amb_level_feedback()==4 && amb.master==master);
        fm1_ms=2199;assert(amb_level_feedback()==4);amb_master(1023,0);
        fm1_ms=2200;assert(!amb_level_feedback());
        fm1_ms=2300;amb_master(900,0);assert(amb_level_feedback()==4);
        amb_target(1);assert(!amb_level_feedback());}
    puts("Minimum transition duration / actual Bass-to-Drums controls / FM6 / normal-note release unchanged PASS");return 0;
}
