/* SPDX-License-Identifier: GPL-3.0-only */
#define AMB_MAIN unused_amb_main
#include "amb_native_test.c"
#include "../firmware/src/amb_sound_variations.h"

/* Changing the NOISE source while holding a key must reinitialise its engine-specific history. */
static void check_held_rain_change(void)
{
    track_t t={0};voice_t v={0};vmod_t m={0};m.amp0=m.amp1=32767;m.shape=64<<8;
    t.p[P_E0]=NZ_ANLG;t.p[P_E1]=76;t.p[P_E2]=64;t.p[P_E3]=64;t.p[P_E6]=100;
    noise_note_on(&t,&v);int64_t energy=0;
    for(unsigned j=0;j<10000;j++){
        if(!(j%100))t.p[P_E0]=t.p[P_E0]==NZ_RAIN?NZ_ANLG:NZ_RAIN;
        int32_t b[CTL]={0};noise_render(&t,&v,b,CTL,&m);
        for(unsigned k=0;k<CTL;k++)energy+=(int64_t)b[k]*b[k];
        if(t.p[P_E0]==NZ_RAIN)assert((uint32_t)v.s[6]==RAIN_STATE_TAG);
    }
    assert(energy>1000000);
}

static void check_cloud_recall(void)
{
    amb_fixture_scene(AMB_SCENE_GARDEN);
    amb_patch(0,0,0);
    amb_layer_t *l=&amb.layer[0];
    const amb_lfo_settings_t manual={.rate=21,.depth=30,.dest=amb_param_route(P_E2)};
    amb_controls.second[0]=manual;
    amb_apply_variation(0,3);
    amb_apply_variation(0,0);
    assert(!memcmp(&manual,&amb_controls.second[0],sizeof manual));

    /* Previously named presets were stored as realized parameter values. */
    const amb_layer_t original=*l;
    amb_voice_offset(l,&original,P_ATK,18);
    amb_voice_offset(l,&original,P_REL,6);
    amb_voice_offset(l,&original,P_E4,-39);
    amb_voice_offset(l,&original,P_E5,-19);
    amb_voice_offset(l,&original,P_REV,6);
    amb_voice_offset(l,&original,P_DLY,-23);
    const amb_layer_t veil=*l;
    ambr.slot=1;amb_save();
    amb_apply_variation(0,2);
    assert(amb_recall(1) && !memcmp(l,&veil,sizeof veil));

    memset(&amb_controls.second[0],0,sizeof manual);
    amb_apply_variation(0,3);
    ambr.slot=2;amb_save();
    amb_apply_variation(0,1);
    assert(!amb_controls.second[0].depth);
    assert(amb_recall(2) && amb_controls.second[0].depth==30);
    amb_controls.second[0].rate=60;
    amb_apply_variation(0,0);
    assert(amb_controls.second[0].rate==60 && amb_controls.second[0].depth==30);
}

int main(void)
{
    memset(amb_nor,255,sizeof amb_nor);ui_power_on();amb_init();
    check_held_rain_change();
    check_cloud_recall();
    unsigned count=0;
    for(unsigned role=0;role<8;role++){
        unsigned patches=role==6?NELEM(N_AS_SOURCE):amb_patch_count(role);
        for(unsigned patch=0;patch<patches;patch++){
            amb_fixture_scene(0);
            amb_patch(0,role,patch);
            amb_layer_t *layer=&amb.layer[0];
            /* Recording start/end belong to the take. A player can set both
             * before trying any timbre on that source. */
            if(layer->engine==ENGI_AMB_SAMPLE){
                layer->base[P_E4]=12;
                layer->base[P_E5]=77;
            }
            amb_layer_t original=*layer,seen[4];
            unsigned n=amb_variation_count(layer);
            assert(n>=1 && n<=4);
            for(unsigned i=0;i<n;i++){
                assert(*amb_variation_name(layer,i));
                amb_apply_variation(0,i);
                assert(amb_valid(&amb) && layer->level==original.level);
                if(role==0 && patch==0){
                    const int waves[]={original.base[P_E0],3,2,3};
                    assert(n==4 && layer->base[P_E0]==waves[i]);
                    if(i==3){
                        assert(layer->base[P_E7]==19 && layer->base[P_E2]==29);
                        assert(amb_controls.second[0].dest==amb_param_route(P_E2));
                        assert(amb_controls.second[0].depth==30);
                    }
                }else assert(layer->base[P_E0]==original.base[P_E0]);
                assert(!memcmp(layer->notes,original.notes,sizeof original.notes));
                if(layer->engine==ENGI_AMB_SAMPLE)
                    assert(layer->base[P_E4]==12 && layer->base[P_E5]==77);
                for(unsigned j=0;j<i;j++)assert(memcmp(layer->base,seen[j].base,sizeof layer->base));
                seen[i]=*layer;amb_apply_variation(0,i);assert(!memcmp(&seen[i],layer,sizeof *layer));
                count++;
            }
            amb_apply_variation(0,0);
            assert(!memcmp(layer,&original,sizeof original));
        }
    }
    printf("Validated %u per-patch voicings, independent edits, stable reapplication and factory return.\n",count);
    return 0;
}
