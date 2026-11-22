/* SPDX-License-Identifier: GPL-3.0-only */
#define AMB_MAIN unused_amb_main
#include "amb_native_test.c"
/* Hold a drone and measure its final dry stem through the real LFO and mixer.
 * Exclude the attack and LFO zero crossings. */
static void check_volume_depth(unsigned depth)
{
    memset(amb_nor,255,sizeof amb_nor);ui_power_on();amb_init();fm6_init();
    amb_fixture_scene(11);amb_target(1);amb_patch(0,1,0);
    amb_layer_t *l=&amb.layer[0];l->lfo_dest=3;l->lfo_depth=depth;
    l->base[P_LWAVE]=0;l->base[P_LRATE]=64;l->base[P_LFADE]=0;
    l->base[P_ATK]=0;l->level=100;amb_base_revision[0]++;amb_prepare();
    amb_render(7*FS);trk_note_on(&trk[0],60,110);fx_stems=1;
    double sum[2]={0};unsigned cnt[2]={0};
    for(unsigned i=0;i<8*FS;i+=CTL){
        int32_t output[CTL*2];mix_block(output,CTL);
        int phase=trk[0].lfo_val;
        if(i<FS || abs(phase)<29000)continue;
        unsigned band=phase<0;
        for(unsigned j=0;j<CTL;j++){double x=stem_buf[0][j];sum[band]+=x*x;cnt[band]++;}
    }
    assert(cnt[0] && cnt[1]);
    double loud=sqrt(sum[0]/cnt[0]),quiet=sqrt(sum[1]/cnt[1]);
    printf("Depth %u: crest RMS %.1f, trough RMS %.1f\n",depth,loud,quiet);
    assert(loud>10 && (l->lfo_depth?quiet<loud*.12:quiet>loud*.65));
}

int main(int argc,char **argv)
{
    assert(argc==2);
    check_volume_depth((unsigned)atoi(argv[1]));
    return 0;
}
