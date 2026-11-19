/* SPDX-License-Identifier: GPL-3.0-only */
#define AMB_MAIN unused_amb_native_main
#include "amb_native_test.c"

static void settle(void)
{
    for(unsigned n=0;n<300;n++)amb_prepare();
}
int main(void)
{
    unsigned patches=0, routes=0, voice_patches=0;
    memset(amb_nor,255,sizeof amb_nor);
    ui_power_on();amb_init();amb_fixture_scene(0);
    amb_layer_t *l=&amb.layer[0];
    for(unsigned role=0;role<7;role++)for(unsigned patch=0;patch<(role==6?NELEM(N_AS_SOURCE):amb_patch_count(role));patch++){
        amb_patch(0,role,role==6?0:patch);
        if(role==6)l->base[P_E0]=patch;
        amb_base_revision[0]++;
        int16_t saved[P_COUNT],quiet[P_COUNT];
        memcpy(saved,l->base,sizeof saved);
        for(unsigned knob=0;knob<4;knob++){
            l->macro[knob]=0;settle();memcpy(quiet,trk[0].p,sizeof quiet);
            l->macro[knob]=127;settle();
            unsigned changed=0;
            for(unsigned i=0;i<P_COUNT;i++)changed+=quiet[i]!=trk[0].p[i];
            if(changed<2)fprintf(stderr,"role %u patch %u knob %u changed %u\n",role,patch,knob,changed); assert(changed>=2);
            l->macro[knob]=64;
        }
        ambr.view=1;ambr.page=3;l->lfo_depth=100;
        uint8_t seen[ROUTE_COUNT]={0};
        do{
            unsigned first=l->lfo_dest;
            do{
                unsigned route=l->lfo_dest;
                assert(!seen[route] && amb_route_name(l,route));seen[route]=1;routes++;
                if(l->engine==ENGI_VOICE && !strcmp(amb_route_name(l,route),"BREATH")){
                    trk[0].lfo_val=-32767;amb_prepare();int low=trk[0].p[P_E5];
                    trk[0].lfo_val=32767;amb_prepare();assert(trk[0].p[P_E5]-low>=60);
                    assert(amb_valid(&amb));voice_patches++;
                }
                amb_knob(3,1);
            }while(l->lfo_dest!=first);
            turn(EN_SELECT,1);
        }while(l->lfo_dest!=0);
        for(unsigned i=0;i<ROUTE_COUNT;i++)assert(seen[i]==!!amb_route_name(l,i));
        settle();
        assert(!memcmp(saved,l->base,sizeof saved));patches++;
    }
    assert(voice_patches>0);
    amb_fixture_scene(0);ambr.view=0;amb_target(0);amb_prepare();
    int baseline_drive=trk[0].p[P_DIST],baseline_cut=trk[0].p[P_E4];
    amb_knob(3,30);settle();
    assert(trk[0].p[P_DIST]>baseline_drive && trk[0].p[P_E4]<baseline_cut);
    assert(song.g[G_DCOLOR]<amb.global[G_DCOLOR] && song.g[G_CLIP]>amb.global[G_CLIP]);
    amb_knob(3,-30);settle();assert(trk[0].p[P_DIST]==baseline_drive && trk[0].p[P_E4]==baseline_cut);
    amb_piece_t old=amb;old.version=4;old.release_ms=5750;
    assert(!st_save(amb_object(1),&old,1420));assert(amb_recall(1));
    assert(amb.version==5 && amb.release_ms==5750);
    amb.layer[0].lfo_dest=ROUTE_EQ;amb.release_ms=30000;ambr.slot=2;amb_save();
    amb_fixture_scene(0);assert(amb_recall(2));
    assert(amb.layer[0].lfo_dest==ROUTE_EQ && amb.release_ms==30000);
    printf("%u patch/source macro sweeps, %u destinations, %u voice Breath sweeps, master reversibility and v4/v5 recall PASS\n",patches,routes,voice_patches);
    return 0;
}
