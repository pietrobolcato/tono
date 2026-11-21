/* SPDX-License-Identifier: GPL-3.0-only */
#define AMB_MAIN other_main
#include "amb_native_test.c"
/* Frozen hashes of every decoded frame from the original CC0 PCM excerpts. */
static void check_packed_factory_recordings(void)
{
    static const unsigned sources[]={3,4,6};
    static const uint32_t expected[]={0x1afd66d7u,0xc5ff8c6eu,0x87959290u};
    for(unsigned i=0;i<NELEM(sources);i++){
        const unsigned source=sources[i];
        const amb_recording_t *r=&amb_recordings[source];
        uint32_t hash=2166136261u;
        for(unsigned frame=0;frame<r->length;frame++){
            uint16_t sample=(uint16_t)amb_recorded_at(source,r->pcm,frame);
            hash=(hash^sample)*16777619u;
        }
        assert(hash==expected[i]);
    }
}
int main(void)
{
    check_packed_factory_recordings();
    memset(amb_nor,255,sizeof amb_nor);ui_power_on();amb_init();
    amb_take_start();
    for(unsigned i=0;i<44100;i++){
        int16_t s=(int16_t)(14000*sin(i*2.*3.14159265359*220./44100));
        uint32_t frame=(uint16_t)s|((uint32_t)(uint16_t)s<<16);
        amb_take_feed(&frame,1);
    }
    assert(amb_take_finish() && amb_current_take()->root==57);
    /* Trigger a captured A3 at A4 and measure the rendered crossings. */
    track_t t={0};voice_t v={0};vmod_t mod={0};
    t.p[P_E0]=AMB_MIC_SOURCE;t.p[P_E5]=100;t.p[P_LN7]=100;t.p[P_E7]=110;
    mod.pitch16=69*16;mod.amp0=mod.amp1=32767;
    amb_sample_on(&t,&v);int last=0;unsigned cycles=0;
    for(unsigned frame=0;frame<22048;frame+=CTL){
        int32_t audio[CTL]={0};amb_sample_render(&t,&v,audio,CTL,&mod);
        for(unsigned j=0;j<CTL;j++){if(last<=0 && audio[j]>0)cycles++;last=audio[j];}
    }
    assert(cycles>=217 && cycles<=224);
    for(unsigned loop=0;loop<4;loop++){
        t.p[P_E3]=loop;t.p[P_E5]=10;t.p[P_E1]=4;v.active=1;
        amb_sample_on(&t,&v);
        for(unsigned frame=0;frame<FS;frame+=CTL){
            int32_t audio[CTL]={0};amb_sample_render(&t,&v,audio,CTL,&mod);
        }
        assert(v.active==(loop!=0));
        if(loop==3)assert(v.s[1]);
    }
    amb_patch(0,6,0);amb.layer[0].base[P_E0]=AMB_MIC_SOURCE;
    ambr.slot=2;amb_save();
    amb_take_t expected=*amb_current_take();
    amb_current_take()->length=0;amb_recall(2);
    assert(!memcmp(&expected,amb_current_take(),sizeof expected));
    amb_take_start();assert(!amb_take_finish());
    assert(!memcmp(&expected,amb_current_take(),sizeof expected));
    puts("USB PCM capture, A3 pitch match, silent attempt preserves take, song save and recall PASS");
}
