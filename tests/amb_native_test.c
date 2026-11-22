/* SPDX-License-Identifier: GPL-3.0-only */
/* Real JIANT C DSP, renderer, controls and AMB+ runtime; hardware IO only is
 * stubbed. This is an automated test harness, not another interactive synth. */
#include <assert.h>
#define AMBPLUS 1
#ifndef AMB_TEST_PRODUCTION_LIBRARY
#define AMB_RETAIN_ARCHIVED_SCENES 1
#endif
#define UI_MAIN unused_jiant_ui_main
#include "ui_test.c"
/* Matrix data from the pinned JIANT HAL (GPL-3.0-only, Leo Kuroshita). */
static const int8_t FM1_KEYMAP[6][FM1_NCOL] = {
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},          /* PA0: encoders */
    { 5, 11,  4, 10,  3,  9,  2,  8, -1, -1, -1},          /* PA5 */
    {34, 35, 36, 37, 38, 40, 39, 13,  7,  6, 12},          /* PA6 */
    {23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33},          /* PA7 */
    { 0,  1, 15, 14, 17, 16, 19, 18, 20, 21, 22},          /* PA8 */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},          /* PB7: encoder 6 */
};
static uint8_t amb_nor[0x100000];
static int amb_flash_fail=-1;
static unsigned amb_writes;
static int st_read(uint32_t off,void *dst,uint32_t n)
{ assert(off+n<=sizeof amb_nor);memcpy(dst,amb_nor+off,n);return 0; }
static int st_erase(uint32_t off)
{
    assert((off>=0x97000 && off<0x9f000) || (off>=0xa0000 && off<0xa8000));
    if(amb_flash_fail==0)return -1;if(amb_flash_fail>0)amb_flash_fail--;
    memset(amb_nor+off,255,4096);amb_writes++;return 0;
}
static int st_prog(uint32_t off,const void *src,uint32_t n)
{
    const uint8_t *p=src;unsigned i;
    assert((off>=0x97000 && off+n<=0x9f000) || (off>=0xa0000 && off+n<=0xa8000));
    if(amb_flash_fail==0)return -1;if(amb_flash_fail>0)amb_flash_fail--;
    for(i=0;i<n;i++)amb_nor[off+i]&=p[i];return 0;
}
#include "../firmware/src/storage.c"
#include "../firmware/src/amb_native.c"
/* Deterministic fixture setup: discard prior synthetic audio before starting
 * the next independent case. UI controls still call the real amb_factory. */
static void amb_fixture_scene(unsigned scene)
{unsigned k;for(k=0;k<NTRK;k++)memset(trk[k].v,0,sizeof trk[k].v);memset(ambr.handoff,0,sizeof ambr.handoff);amb_factory(scene);}
static void amb_render(unsigned frames)
{
    int32_t out[CTL*2];unsigned i;
    for(i=0;i<frames;i+=CTL) {mix_block(out,CTL);amb_output(out,CTL);}
}

static void check_page_presets(void)
{
    amb_fixture_scene(0);
    unsigned checks=0;
    for(unsigned role=0;role<NELEM(AMB_ROLE);role++){
        amb_patch(0,role,0);
        for(unsigned view=1;view<=2;view++)for(unsigned page=0;page<(view==1?7:6);page++){
            unsigned count=view==1 && page==5?6:view==2 && page==3?4:1;
            for(unsigned fx=0;fx<count;fx++)for(unsigned index=1;index<=3;index++){
                ambr.view=view;ambr.page=page;ambr.fx=fx;ambr.edit_layer=0;ambr.local_preset=0;
                amb_piece_t old=amb;
                turn(EN_PRESET,index);
                assert(ambr.view==view && ambr.piece_preview==255 && !memcmp(&amb,&old,sizeof amb));
                if(!amb_page_bank()){assert(!ambr.local_preset);continue;}
                assert(ambr.local_preset==index && amb_rec_action());
                press(B_HOME);assert(!ambr.local_preset && !memcmp(&amb,&old,sizeof amb));
                ambr.view=view;ambr.page=page;ambr.fx=fx;
                turn(EN_PRESET,index);press(B_REC);
                assert(amb_valid(&amb) && !ambr.local_preset);
                assert(!memcmp(&old.layer[1],&amb.layer[1],3*sizeof(amb_layer_t)));
                assert(amb.layer[0].engine==old.layer[0].engine && amb.layer[0].patch==old.layer[0].patch);
                amb_piece_t saved=amb;
                turn(EN_PRESET,index);press(B_REC);assert(!memcmp(&saved,&amb,sizeof amb));
                checks++;
            }
        }
    }
    printf("%u local presets across roles/pages: silent browse, HOME cancel, valid repeatable commit, scope PASS\n",checks);
}

#ifndef AMB_MAIN
#define AMB_MAIN main
#endif
int AMB_MAIN(int argc,char **argv)
{
    unsigned i,k,scene;amb_piece_t before;int64_t energy;int32_t out[CTL*2];FILE *f=0;
    (void)argc;(void)argv;
    memset(amb_nor,255,sizeof amb_nor);ui_power_on();amb_init();amb_fixture_scene(0);
    printf("AMB+ piece %zu bytes; original A/B payload capacity %u\n",sizeof amb,ST_PAYLOAD_MAX);
    if(!amb_valid(&amb)) {
        for(k=0;k<4;k++) for(i=0;i<P_COUNT;i++) {const param_desc_t *d=param_desc_of(amb.layer[k].engine,i);if(amb.layer[k].base[i]<d->min || amb.layer[k].base[i]>d->max) printf("bad k%u %s %d expected %d..%d\n",k,d->label,amb.layer[k].base[i],d->min,d->max);}
    }
    assert(amb_valid(&amb));
    /* Same small turn at either end of the pot must adjust the stored
     * layer level by that distance, even on the first turn after selection. */
    for(unsigned pos=160;pos<=800;pos+=640){
        amb_target(0);unsigned song_gain=amb.master;amb_master(pos,1);amb.layer[0].level=89; /* 70% */
        unsigned sibling=amb.layer[1].level;
        amb_target(1);amb_master(pos,0);amb_master(pos+2,0);
        assert(amb.layer[0].level==89);
        amb_master(pos-32,0);assert(amb.layer[0].level==85);
        amb_master(pos+32,0);assert(amb.layer[0].level==93);
        amb_master(pos,0);assert(amb.layer[0].level==89);
        assert(amb.master==song_gain && amb.layer[1].level==sibling);
    }
    amb_master(1023,0);amb.layer[0].level=1;
    amb_master(600,0);assert(amb.layer[0].level==0);
    amb_master(632,0);assert(amb.layer[0].level==4);
    amb.layer[0].level=126;amb_master(800,0);assert(amb.layer[0].level==127);
    amb_master(768,0);assert(amb.layer[0].level==123);
    amb_target(0);before=amb;amb_master(768,0);assert(amb.master==before.master);
    amb_master(752,0);assert(amb.master==before.master-16 && amb.layer[0].level==123);
    ambr.view=2;ambr.page=2;before=amb;
    turn(EN_SELECT,4);assert(!memcmp(before.layer,amb.layer,sizeof amb.layer));
    assert(!memcmp(&before.policy,&amb.policy,sizeof amb.policy));
    ambr.view=0;amb_target(1);press(B_EDIT);assert(ambr.view==1 && ambr.page==0);
    turn(EN_ALGO,2);turn(EN_SELECT,1);assert(ambr.proposal==2 && amb.layer[0].role==0);press(B_REC);assert(amb.layer[0].role==2);
    press(B_SCL);assert(ambr.page==1);before=amb;turn(EN_SELECT,1);assert(ambr.proposal==255);turn(EN_ALGO,1);turn(EN_SELECT,1);
    assert(ambr.proposal==1 && !memcmp(&amb,&before,sizeof amb));
    press(B_REC);assert(amb.layer[0].patch==1 && ambr.proposal==255);
    before=amb;turn(EN_PRESET,1);assert(ambr.local_preset && ambr.view==1 && !memcmp(&before,&amb,sizeof amb));
    press(B_REC);assert(!ambr.local_preset && amb.layer[0].patch==before.layer[0].patch && amb.layer[0].base[P_REV]!=before.layer[0].base[P_REV]);
    turn(EN_PRESET,1);press(B_LFO);assert(!ambr.local_preset && ambr.page==3);
    turn(EN_SELECT,-1);assert(amb.layer[0].lfo_dest==ROUTE_GRAIN);
    turn(EN_K4,1);assert(amb.layer[0].lfo_dest==ROUTE_GRAIN+1);
    before=amb;turn(EN_PRESET,2);press(B_REC);assert(amb.layer[0].lfo_dest==before.layer[0].lfo_dest && amb.layer[0].lfo_depth!=before.layer[0].lfo_depth);
    ambr.page=1;
    for(i=0;i<7;i++)press(B_SCL);assert(ambr.page==1);
    amb_fixture_scene(0);ambr.view=1;ambr.page=4;ambr.edit_layer=2;press(B_REC);assert(ambr.recording);
    ambr.cursor=2;amb.layer[2].notes[2]=0;host_notes=1u<<4;amb_input();assert(amb.layer[2].notes[2]==57 && ambr.cursor==2 && !amb.layer[2].generated);
    press(B_REC);assert(!ambr.recording);
    before=amb;
    for(i=0;i<1000;i++) {amb_policy_choose(&amb.policy,i%3,i%(i%3==2?6:8),1);amb_policy_boundary(&amb.policy,i);amb_step();}
    assert(!memcmp(before.layer,amb.layer,sizeof amb.layer));
    song.playing=0;amb_fixture_scene(0);ambr.view=3;ambr.slot=0;amb_save();assert(amb_writes==1);
    before=amb;amb.master=333;amb_save();assert(ambr.overwrite && amb_writes==1);amb_save();assert(amb_writes==2);
    assert(amb_recall(0));assert(amb.master==333);
    amb.master=444;ambr.overwrite=1;amb_flash_fail=2;amb_save();amb_flash_fail=-1;
    assert(amb_recall(0));assert(amb.master==333);
    /* Browse only recallable records. A valid header with a corrupt payload or
     * an unrelated JIANT record still needs overwrite confirmation. */
    uint32_t invalid_piece=0;
    assert(!st_save(amb_object(2),&invalid_piece,sizeof invalid_piece));
    assert(!st_save(amb_object(3),&amb,sizeof amb));
    amb_nor[st_sector(amb_object(3),0)+ST_PAYLOAD_OFF]^=1;
    amb_scan_slots();
    assert(amb_slot_state[0]==AMB_SAVED && amb_slot_state[1]==AMB_EMPTY);
    assert(amb_slot_state[2]==AMB_UNREADABLE && amb_slot_state[3]==AMB_UNREADABLE);
    ambr.view=0;ambr.piece_preview=255;ambr.preset_position=AMB_SCENE_ANALOG_DAYS;
    turn(EN_PRESET,1);assert(ambr.piece_preview==NELEM(AMB_SCENE));
    turn(EN_PRESET,1);assert(ambr.piece_preview==AMB_SCENE_GARDEN);
    turn(EN_SELECT,-1);assert(ambr.piece_preview==NELEM(AMB_SCENE));press(B_HOME);
    /* Confirm while performing, navigate away, edit, and stop. The frozen
     * snapshot goes to the original slot. No erase/program during playback. */
    unsigned writes_before=amb_writes;
    ambr.view=3;ambr.slot=1;ambr.overwrite=0;song.playing=1;amb.master=555;
    amb_save();assert(amb_save_slot==2 && song.playing && amb_writes==writes_before);
    amb.master=777;ambr.slot=4;amb_save();
    assert(amb_save_slot==2 && amb_writes==writes_before);press(B_HOME);
    song.playing=0;amb_input();
    assert(!amb_save_slot && amb_writes==writes_before+1 && amb_slot_state[1]==AMB_SAVED);
    assert(amb_recall(1) && amb.master==555);
    printf("Song library: validated occupancy, skip unreadable slots, live snapshot and stop-time commit PASS\n");
    for(i=1;i<8;i++) {ambr.slot=(uint8_t)i;ambr.overwrite=0;amb_save();if(ambr.overwrite)amb_save();assert(amb_occupied(i));}
    for(i=0;i<8;i++) assert(amb_recall(i));
    amb.magic=0;assert(!amb_valid(&amb));amb=before;
    amb_fixture_scene(AMB_SCENE_GARDEN);ambr.view=0;turn(EN_PRESET,1);press(B_REC);
    assert(st_crc32(&amb,sizeof amb)==0x99937c93u);
    for(scene=0;scene<NELEM(AMB_SCENE);scene++) {
        energy=0;amb_fixture_scene(scene);amb.master=650;assert(amb_valid(&amb));seq_start();
        for(i=0;i<FS*12u/CTL;i++) {mix_block(out,CTL);amb_output(out,CTL);for(k=0;k<CTL*2;k++) {assert(out[k]>=-30000 && out[k]<=30000);energy+=(int64_t)out[k]*out[k];}}
        assert(energy>1000000);printf("scene %u energy %lld PASS\n",scene,(long long)energy);seq_stop();amb_render(FS*2);
    }
    for(scene=0;scene<7;scene++) {
        unsigned count=scene==6?NELEM(N_AS_SOURCE):amb_patch_count(scene),patch;
        uint64_t hashes[count];
        /* The personal source is empty on boot and exercised with input in amb_take_test. */
        for(patch=0;patch<count;patch++) {
            if(scene==6 && patch==AMB_MIC_SOURCE)continue;
            int32_t mono[CTL];uint64_t hash=1469598103934665603ull;
            amb_fixture_scene(0);song.playing=0;
            for(k=0;k<NTRK;k++)memset(&trk[k],0,sizeof trk[k]);
            amb_patch(0,scene,scene==6?0:patch);
            if(scene==6)amb.layer[0].base[P_E0]=(int16_t)patch;
            amb_prepare();trk[0].engine=amb.layer[0].engine;
            trk[0].xf_on=trk[0].xf=trk[0].xin=0;
            trk_note_on(&trk[0],scene==4?36:60,80);energy=0;
            for(i=0;i<FS*3u/CTL;i++) {
                track_render(&trk[0],mono,CTL);
                for(k=0;k<CTL;k++){energy+=(int64_t)mono[k]*mono[k];hash^=(uint32_t)mono[k];hash*=1099511628211ull;}
            }
            assert(energy>100000);hashes[patch]=hash;
            for(k=0;k<patch;k++)if(scene!=6 || k!=AMB_MIC_SOURCE)assert(hashes[k]!=hash);
            printf("role %u patch/source %u isolated PCM energy %lld hash %016llx PASS\n",scene,patch,(long long)energy,(unsigned long long)hash);
        }
    }
    for(k=0;k<4;k++) {
        amb_fixture_scene(0);amb_patch(k,6,0);amb_prepare();trk[k].engine=ENGI_AMB_SAMPLE;
        assert(amb_valid(&amb));
        for(i=0;i<4;i++) {
            amb.layer[k].base[P_E1]=(int16_t)i;amb_base_revision[k]++;
            amb_prepare();trk_note_on(&trk[k],60,80);amb_render(FS);trk_all_off(&trk[k]);amb_render(FS*2);
        }
    }
    /* Whole-state macro sweep leaves all deep baselines intact. */
    amb_fixture_scene(0);before=amb;
    for(i=0;i<128;i++) {
        for(k=0;k<4;k++)amb.layer[k].macro[i%4]=(uint8_t)i;
        amb_prepare();
        for(k=0;k<4;k++)assert(!memcmp(before.layer[k].base,amb.layer[k].base,sizeof amb.layer[k].base));
    }
    amb_fixture_scene(0);amb.layer[0].swing=100;ambr.step=1;amb_step();assert(ambr.due[0]>0);
    ambr.view=2;ambr.page=2;before=amb;turn(EN_ALGO,4);press(B_REC);assert(!memcmp(&before,&amb,sizeof amb));
    amb_fixture_scene(0);ambr.view=0;seq_start();ambr.step=9;ambr.piece_preview=2;amb_rec();
    assert(amb_pending_ready && amb.key==0);
    while(ambr.step<16)amb_step();assert(amb.key==0);
    amb_step();assert(!amb_pending_ready && amb.key==2 && song.playing);
    seq_stop();ambr.last_play=0;
    amb_fixture_scene(0);amb.layer[0].level=0;amb_patch(0,2,0);assert(amb.layer[0].level==0);
    amb_fixture_scene(0);song.playing=0;amb.version=1;memset(&amb.policy,0,sizeof amb.policy);
    assert(!st_save(amb_object(7),&amb,1400));assert(amb_recall(7));
    assert(amb.version==AMB_VERSION && amb.policy.variation==40 && amb.policy.active[0]==0 && amb.policy.pending[0]==255);
    amb_fixture_scene(0);ambr.view=2;ambr.page=2;amb_leds();
    {unsigned q=led_pos[panel.btn[B_REC]];assert(q!=255);assert(!(fm1_led_dim[q>>3]&(1u<<(q&7))));}
    ambr.view=1;ambr.page=4;ambr.edit_layer=0;ambr.recording=1;amb_leds();
    {unsigned q=led_pos[panel.btn[B_REC]];assert(fm1_led[q>>3]&(1u<<(q&7)));}
    song.playing=0;ambr.recording=0;ambr.view=0;amb.layer[0].solo_plus1=3;ambr.overwrite=1;ambr.slot=6;
    amb_save();ambr.solo=-1;assert(amb_recall(6));assert(ambr.solo==2);
    amb_fixture_scene(0);ambr.solo=-1;amb.layer[0].solo_plus1=0;
    for(k=0;k<NTRK;k++)memset(trk[k].v,0,sizeof trk[k].v);
    amb_prepare();for(k=0;k<4;k++){trk[k].engine=trk[k].eng_req;trk[k].xf_on=trk[k].xf=trk[k].xin=0;trk_note_on(&trk[k],k==3?36:48+k*4,80);}
    for(i=0;i<40;i++){trk_note_on(&trk[i%4],48+(i%24),80);assert(voices_busy()<=4);}
    printf("offsets: piece.policy=%zu piece.layer=%zu layer.bytes=%zu runtime.step=%zu\n",offsetof(amb_piece_t,policy),offsetof(amb_piece_t,layer),sizeof(amb_layer_t),offsetof(__typeof__(ambr),step));
    if(argc>1)f=fopen(argv[1],"wb");
    check_page_presets();
    if(f) {amb_fixture_scene(0);seq_start();wav_hdr(f,FS*20);for(i=0;i<FS*20u/CTL;i++){mix_block(out,CTL);amb_output(out,CTL);for(k=0;k<CTL;k++)wav_put(f,out[k*2],out[k*2+1]);}fclose(f);seq_stop();}
    puts("AMB+ native: controls/roles/pages/recording/non-destructive generation/8-slot A-B recovery/factory catalog DSP PASS");
    return 0;
}
