/* SPDX-License-Identifier: GPL-3.0-only */
#define AMB_MAIN unused_amb_native_main
#include "amb_native_test.c"
static double eq_energy(unsigned band,unsigned hz,int boost)
{
    unsigned i,j;int32_t b[CTL];double phase=0,energy=0;
    memset(amb.layer_eq[1],0,4);if(boost)amb.layer_eq[1][band]=6;
    for(j=0;j<600;j++){
        for(i=0;i<CTL;i++){b[i]=(int32_t)(sin(phase)*4000);phase+=6.283185307179586*hz/FS;}
        amb_layer_process(1,b,CTL);
        if(j>=400)for(i=0;i<CTL;i++)energy+=(double)b[i]*b[i];
    }return energy;
}
static void check_eq_display(void)
{
    static const unsigned frequency[]={80,800,8000};
    int32_t stereo[2*CTL],original[2*CTL];
    for(unsigned band=0;band<3;band++){
        memset(amb_eq_meters,0,sizeof amb_eq_meters);
        for(unsigned block=0;block<200;block++){
            for(unsigned i=0;i<CTL;i++){
                int v=(int)(8000*sin(6.283185307179586*frequency[band]*(block*CTL+i)/FS));
                stereo[2*i]=v;stereo[2*i+1]=-v;
            }
            memcpy(original,stereo,sizeof stereo);
            amb_measure_eq(0,stereo,CTL,2);
            assert(!memcmp(original,stereo,sizeof stereo));
        }
        for(unsigned j=0;j<3;j++)if(j!=band)assert(amb_eq_meters[0].peak[band]>amb_eq_meters[0].peak[j]);
        assert(!amb_eq_meters[1].peak[band]);
    }
    puts("EQ input meter: low/mid/high tones, opposite stereo polarity, separate targets and untouched audio PASS");
}

int main(void)
{
    check_eq_display();
    unsigned i,k,old_engine;int32_t out[CTL*2];amb_piece_t old;
    memset(amb_nor,255,sizeof amb_nor);ui_power_on();amb_init();amb_fixture_scene(0);favorites.factory[15][13]=1;assert(amb.release_ms==3000);
    amb.auto_index=0;ambr.solo=0;seq_start();for(i=0;i<FS*5u/CTL;i++)mix_block(out,CTL);
    old_engine=trk[0].engine;amb_patch(0,2,0);
    assert(ambr.handoff[0] && trk[0].engine==old_engine && trk[0].eng_req==old_engine);
    {int32_t env=0;for(k=0;k<NVOICE;k++)if(trk[0].v[k].active){assert(!trk[0].v[k].gate && trk[0].v[k].stage==3);env+=trk[0].v[k].env;}
     assert(env);for(i=0;i<FS/10u/CTL;i++)mix_block(out,CTL);assert(ambr.handoff[0]);}
    /* Multiple rapid patch choices coalesce, without restarting the old tail. */
    amb_patch(0,2,4);assert(ambr.handoff[0] && trk[0].engine==old_engine);
    for(i=0;i<FS*3u/CTL && ambr.handoff[0];i++)mix_block(out,CTL);
    assert(!ambr.handoff[0] && trk[0].eng_req==amb.layer[0].engine);
    assert(i<FS*3u/CTL);printf("release handoff completed in %.3f s PASS\n",0.1+(double)i*CTL/FS);
    /* The cap applies to internal-envelope FM6 as well as ordinary ADSR. */
    song.playing=0;amb_fixture_scene(0);amb.release_ms=500;amb_patch(0,2,0);amb_prepare();trk[0].engine=trk[0].eng_req;panic_req=0;
    trk_note_on(&trk[0],60,90);for(i=0;i<FS/CTL;i++)mix_block(out,CTL);
    amb_patch(0,0,0);assert(ambr.handoff[0]);
    for(i=0;i<FS/CTL && ambr.handoff[0];i++)mix_block(out,CTL);
    assert(!ambr.handoff[0] && i<=FS/2u/CTL+2);
    /* Every outgoing role obeys a short cap, without adding synth voices. */
    for(k=0;k<7;k++){
        song.playing=0;amb_fixture_scene(0);amb.release_ms=250;amb_patch(0,k,0);
        amb_prepare();trk[0].engine=trk[0].eng_req;panic_req=0;
        trk_note_on(&trk[0],k==4?36:60,90);for(i=0;i<FS/4u/CTL;i++)mix_block(out,CTL);
        amb_patch(0,(k+1)%7,0);
        for(i=0;i<FS/2u/CTL && ambr.handoff[0];i++){mix_block(out,CTL);assert(voices_busy()<=voice_cap());}
        assert(!ambr.handoff[0] && i<=FS/4u/CTL+2);
    }
    /* Entry edits the selected slot silently; the sequencer remains the player. */
    song.playing=0;amb_fixture_scene(0);amb_target(1);ambr.view=1;ambr.page=4;press(B_REC);ambr.cursor=5;
    amb.layer[0].notes[2]=0;fm1_in.notes=host_notes=1u<<4;amb_input();keyboard_block();
    assert(ambr.cursor==2 && amb.layer[0].notes[2] && kb_note[4]==KB_SILENT);
    fm1_in.notes=host_notes=0;keyboard_block();
    /* The displayed step follows the layer's divider, including rests. */
    amb.layer[0].generated=0;amb.layer[0].speed=2;ambr.handoff[0]=0;trk[0].engine=amb.layer[0].engine;
    amb_layer_step(0,20,1);assert(ambr.playhead[0]==5);
    amb_layer_step(0,21,1);assert(ambr.playhead[0]==5);
    /* EQ and cap round trip; legacy v3's 1400-byte payload migrates flat/default. */
    song.playing=0;amb_fixture_scene(0);amb.layer_eq[0][0]=6;amb.layer_eq[1][2]=-9;amb.release_ms=1750;
    ambr.slot=0;amb_save();old=amb;amb_fixture_scene(1);assert(amb_recall(0));assert(!memcmp(&old,&amb,sizeof amb));
    old.version=3;assert(!st_save(amb_object(1),&old,1400));assert(amb_recall(1));
    assert(amb.version==AMB_VERSION && amb.release_ms==3000);
    for(k=0;k<4;k++)for(i=0;i<4;i++)assert(!amb.layer_eq[k][i]);
    /* Flat EQ is exactly transparent and another layer's EQ cannot alter it. */
    {int32_t a[CTL],b[CTL];for(i=0;i<CTL;i++)a[i]=b[i]=(int32_t)i*98765-123456;
     amb.layer_eq[0][0]=12;amb_layer_process(1,a,CTL);assert(!memcmp(a,b,sizeof a));
     for(k=0;k<200;k++){for(i=0;i<CTL;i++)a[i]=4000;amb_layer_process(0,a,CTL);}
     assert(a[CTL-1]>4000);}
    {double gain[3][3];unsigned freq[3]={80,1000,9000};
        for(k=0;k<3;k++)for(i=0;i<3;i++){double flat=eq_energy(k,freq[i],0);gain[k][i]=eq_energy(k,freq[i],1)/flat;}
        assert(gain[0][0]>gain[0][1] && gain[0][1]>gain[0][2]);
        assert(gain[1][1]>gain[1][0] && gain[1][1]>gain[1][2]);
        assert(gain[2][2]>gain[2][1] && gain[2][1]>gain[2][0]);
        printf("layer EQ selective power gains low %.2f / mid %.2f / high %.2f PASS\n",gain[0][0],gain[1][1],gain[2][2]);
    }
    {int32_t b[CTL];for(i=0;i<4;i++)amb.layer_eq[2][i]=12;
        for(k=0;k<200;k++){for(i=0;i<CTL;i++)b[i]=(i&1)?2147483647:-2147483647;amb_layer_process(2,b,CTL);
            for(i=0;i<CTL;i++)assert(b[i]>-113000 && b[i]<113000);}
    }
    puts("AMB+ bounded natural release / coalesced patch changes / silent selected-step entry / divided playhead / independent persisted layer EQ / v3 migration PASS");return 0;
}
