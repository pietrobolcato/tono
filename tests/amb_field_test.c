/* SPDX-License-Identifier: GPL-3.0-only */
/* Re-open each ten-second factory take and audition it through the shared mixer. */
#define AMB_MAIN other_amb_main
#include "amb_native_test.c"

int main(int argc,char **argv)
{
    if(argc!=3)return 2;
    unsigned which=(unsigned)atoi(argv[1]);
    if(which>=3)return 2;
    memset(amb_nor,255,sizeof amb_nor);
    ui_power_on();amb_init();amb_fixture_scene(AMB_SCENE_GARDEN);
    amb_patch(0,5,28+which);
    amb_layer_t *l=&amb.layer[0];
    unsigned src=AMB_FIELD_SOURCE+which;
    assert(l->base[P_E0]==(int)src && l->base[P_E1]==4 && l->base[P_E3]==1);
    assert(amb_valid(&amb));
    ambr.slot=0;ambr.overwrite=1;amb_save();
    amb_fixture_scene(AMB_SCENE_GARDEN);
    assert(amb_recall(0) && l->role==5 && l->patch==28+which && l->base[P_E0]==(int)src);
    l->level=110;l->base[P_PAN]=0;ambr.solo=0;
    amb_base_revision[0]++;amb_prepare();amb_render(FS/2);
    trk_note_on(&trk[0],60,110);
    FILE *f=fopen(argv[2],"wb");if(!f)return 1;
    unsigned frames=12*FS/CTL*CTL;
    wav_hdr(f,frames);
    for(unsigned at=0;at<frames;at+=CTL){
        int32_t out[CTL*2];mix_block(out,CTL);amb_output(out,CTL);
        for(unsigned i=0;i<CTL;i++)wav_put(f,out[2*i],out[2*i+1]);
    }
    return fclose(f)!=0;
}
