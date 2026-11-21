/* SPDX-License-Identifier: GPL-3.0-only */
/* Compare the manual reference and a seeded live generator under the same macro sweep. */
#define AMB_MAIN unused_amb_main
#include "../tests/amb_native_test.c"
int main(int argc,char **argv){
    if(argc<3 || argc>6)return 2;
    unsigned role=atoi(argv[1]);FILE *f=fopen(argv[2],"wb");if(!f)return 1;
    memset(amb_nor,255,sizeof amb_nor);ui_power_on();amb_init();amb_fixture_scene(0);
    amb_patch(0,role,argc>=5?atoi(argv[4]):role==3?11:1);amb_layer_t *l=&amb.layer[0];
    ambr.solo=0;l->generated=0;l->speed=0;l->level=96;
    amb.global[G_BPM]=96;
    if(argc>=4){
        amb.policy.seed=(uint32_t)strtoul(argv[3],NULL,0);
        l->generated=1;l->density=85;l->mode=2;
        amb.policy.active[0]=1;amb.policy.active[1]=argc==6?atoi(argv[5]):AMB_ACID_PHRASE;amb.policy.active[2]=0;
    }else{
        l->drums[3]=0x2208;
        const uint8_t phrase[16]={0,0,7,10,0,12,7,0,0,3,0,7,10,12,7,0};
        for(unsigned i=0;i<16;i++){l->notes[i]=(i==1||i==7||i==10)?0:(role==3?48:60)+phrase[i];l->velocity[i]=i%4==0?108:73;}
    }
    amb_prepare();amb_render(FS/2);
    if(argc>=4)seq_start();
    unsigned stride=(FS*60/96/4/CTL)*CTL,total=6*16*stride;
    wav_hdr(f,total);
    for(unsigned at=0;at<total;at+=CTL){
        unsigned step=at/stride;
        if(!(at%stride)){
            l->macro[0]=28+(step<48?step:95-step)*2;
            l->macro[1]=45+step%48;l->macro[2]=43+step/2;l->macro[3]=45+step/3;
            if(argc==3)amb_layer_step(0,step,1);
        }
        int32_t out[CTL*2];mix_block(out,CTL);amb_output(out,CTL);
        for(unsigned i=0;i<CTL;i++)wav_put(f,out[2*i],out[2*i+1]);
    }
    return fclose(f)!=0;
}
