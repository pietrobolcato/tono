/* SPDX-License-Identifier: GPL-3.0-only */
#define AMB_MAIN unused_amb_native_main
#include "amb_native_test.c"
int main(void)
{
    unsigned i,k,changes=0,old;int32_t out[CTL*2];int64_t energy=0;
    memset(amb_nor,255,sizeof amb_nor);ui_power_on();amb_init();
    amb.auto_index=1;amb.evolve[0]=100;ambr.solo=0;old=amb.chord;
    seq_start();
    for(i=0;i<FS*35u/CTL;i++){
        mix_block(out,CTL);
        if(amb.chord!=old){changes++;old=amb.chord;}
        for(k=0;k<4;k++)assert(!ambr.fade_request[k] && ambr.fade[k].stage==AMB_FADE_STABLE);
        for(k=0;k<NVOICE;k++)if(trk[0].v[k].active){
            assert(!trk[0].p[P_GLIDE]);
            assert(trk[0].v[k].pitch_cur==trk[0].v[k].pitch16); /* no forced sweep */
        }
        for(k=0;k<CTL*2;k++)energy+=(int64_t)out[k]*out[k];
    }
    assert(changes>=9 && energy>1000000);
    /* Changing tempo or a glide control is continuous editing, not a source transition. */
    ambr.view=2;ambr.page=1;amb_knob(2,1);amb_knob(3,1);
    for(k=0;k<4;k++)assert(!ambr.fade_request[k]);
    /* Explicit key/scale and chord changes still request the manual transition. */
    amb_knob(0,1);assert(ambr.fade_request[0]);
    printf("AUTO playback: %u chord changes, no source fades, no forced Pad portamento, continuous controls untouched PASS\n",changes);
    return 0;
}
