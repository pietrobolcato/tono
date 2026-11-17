/* SPDX-License-Identifier: GPL-3.0-only */
/* Real JIANT C DSP, renderer, controls and AMB+ runtime; hardware IO only is
 * stubbed. This is an automated test harness, not another interactive synth. */
#include <assert.h>
#define AMBPLUS 1
#define UI_MAIN unused_jiant_ui_main
#include "ui_test.c"
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
static void amb_render(unsigned frames)
{
    int32_t out[CTL*2];unsigned i;
    for(i=0;i<frames;i+=CTL) {mix_block(out,CTL);amb_output(out,CTL);}
}
int main(int argc,char **argv)
{
    unsigned i,k,scene;amb_piece_t before;int64_t energy;int32_t out[CTL*2];FILE *f=0;
    (void)argc;(void)argv;
    memset(amb_nor,255,sizeof amb_nor);ui_power_on();amb_init();
    printf("AMB+ piece %zu bytes; original A/B payload capacity %u\n",sizeof amb,ST_PAYLOAD_MAX);
    if(!amb_valid(&amb)) {
        for(k=0;k<4;k++) for(i=0;i<P_COUNT;i++) {const param_desc_t *d=param_desc_of(amb.layer[k].engine,i);if(amb.layer[k].base[i]<d->min || amb.layer[k].base[i]>d->max) printf("bad k%u %s %d expected %d..%d\n",k,d->label,amb.layer[k].base[i],d->min,d->max);}
    }
    assert(amb_valid(&amb));
    amb_target(1);amb_master(0,0);assert(amb.layer[0].level==92);
    amb_master(1023,0);assert(amb.layer[0].level==127);
    amb_target(0);before=amb;amb_master(1023,0);assert(amb.master==before.master);
    ambr.view=2;ambr.page=2;before=amb;
    turn(EN_SELECT,4);assert(!memcmp(before.layer,amb.layer,sizeof amb.layer));
    assert(!memcmp(&before.policy,&amb.policy,sizeof amb.policy));
    ambr.view=0;amb_target(1);press(B_EDIT);assert(ambr.view==1 && ambr.page==0);
    turn(EN_ALGO,2);assert(amb.layer[0].role==0);press(B_REC);assert(amb.layer[0].role==2);
    press(B_SCL);assert(ambr.page==1);turn(EN_SELECT,1);assert(amb.layer[0].patch==1);
    for(i=0;i<6;i++)press(B_SCL);assert(ambr.page==1);
    amb_factory(0);ambr.view=1;ambr.page=4;ambr.edit_layer=2;press(B_REC);assert(ambr.recording);
    host_notes=1u<<8;amb_input();assert(amb.layer[2].notes[0]==61 && !amb.layer[2].generated);
    press(B_REC);assert(!ambr.recording);
    before=amb;
    for(i=0;i<1000;i++) {amb_policy_choose(&amb.policy,i%3,i%(i%3==2?6:8),1);amb_policy_boundary(&amb.policy,i);amb_step();}
    assert(!memcmp(before.layer,amb.layer,sizeof amb.layer));
    song.playing=0;amb_factory(0);ambr.view=3;ambr.slot=0;amb_save();assert(amb_writes==1);
    before=amb;amb.master=333;amb_save();assert(ambr.overwrite && amb_writes==1);amb_save();assert(amb_writes==2);
    assert(amb_recall(0));assert(amb.master==333);
    amb.master=444;ambr.overwrite=1;amb_flash_fail=2;amb_save();amb_flash_fail=-1;
    assert(amb_recall(0));assert(amb.master==333);
    for(i=1;i<8;i++) {ambr.slot=(uint8_t)i;ambr.overwrite=0;amb_save();assert(amb_occupied(i));}
    for(i=0;i<8;i++) assert(amb_recall(i));
    amb.magic=0;assert(!amb_valid(&amb));amb=before;
    for(scene=0;scene<6;scene++) {
        energy=0;amb_factory(scene);amb.master=650;assert(amb_valid(&amb));seq_start();
        for(i=0;i<FS*12u/CTL;i++) {mix_block(out,CTL);amb_output(out,CTL);for(k=0;k<CTL*2;k++) {assert(out[k]>=-30000 && out[k]<=30000);energy+=(int64_t)out[k]*out[k];}}
        assert(energy>1000000);printf("scene %u energy %lld PASS\n",scene,(long long)energy);seq_stop();amb_render(FS*2);
    }
    if(argc>1)f=fopen(argv[1],"wb");
    if(f) {amb_factory(0);seq_start();wav_hdr(f,FS*20);for(i=0;i<FS*20u/CTL;i++){mix_block(out,CTL);amb_output(out,CTL);for(k=0;k<CTL;k++)wav_put(f,out[k*2],out[k*2+1]);}fclose(f);seq_stop();}
    puts("AMB+ native: controls/roles/pages/recording/non-destructive generation/8-slot A-B recovery/6-scene DSP PASS");
    return 0;
}
