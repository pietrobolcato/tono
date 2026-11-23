/* SPDX-License-Identifier: GPL-3.0-only */
#define AMB_MAIN unused_amb_native_main
#include "amb_native_test.c"

static void settle(void)
{
    for(unsigned n=0;n<300;n++)amb_prepare();
}

static void check_random_rate(void)
{
    amb_fixture_scene(0);
    amb_target(1);
    press(B_LFO);
    amb_layer_t *l=&amb.layer[0];
    l->base[P_LRATE]=60;
    unsigned depth=l->lfo_depth, destination=l->lfo_dest;
    press(B_EDIT);
    assert(ambr.page==3 && ambr.bank);
    amb_knob(0,1);
    assert(l->base[P_LRATE]==amb_rate_range(48,72));
    amb_knob(1,100);
    assert(l->base[P_LRATE]==amb_rate_range(72,72));
    amb_knob(2,40);
    press(B_EDIT);
    amb_knob(1,4);
    int range=amb_rate_range(76,116);
    assert(l->base[P_LRATE]==range && !ambr.bank);
    amb_prepare();
    track_t *t=&trk[0];
    unsigned changes=0;
    for(unsigned n=0;n<24000;n++){
        uint32_t phase=t->lfo_ph, held=t->lfo_rnd;
        int rate=amb_cycle_rate(range,held,0);
        assert(rate>=76 && rate<=116);
        track_lfo_tick(t);
        assert(t->lfo_ph-phase==LFO_INC[rate]);
        if(t->lfo_ph>=phase)assert((uint32_t)t->lfo_rnd==held);
        else changes+=rate!=amb_cycle_rate(range,t->lfo_rnd,0);
    }
    assert(changes>2 && l->lfo_depth==depth && l->lfo_dest==destination);
    assert(amb_valid(&amb));
    l->base[P_LRATE]=amb_rate_range(92,91);assert(!amb_valid(&amb));
    l->base[P_LRATE]=range;
    ambr.slot=3;amb_save();
    amb_fixture_scene(0);assert(amb_recall(3));
    assert(l->base[P_LRATE]==range);
    amb_target(1);press(B_LFO);press(B_EDIT);
    int current=amb_cycle_rate(range,t->lfo_rnd,0);
    amb_knob(0,-1);assert(l->base[P_LRATE]==current);
    puts("Random LFO UI, cycle timing, bounds and song recall PASS");
}

static void link_last_edit(void)
{
    fm1_in.buttons|=1u<<panel.btn[B_HOME];
    frame();
    press(B_LFO);
    fm1_in.buttons&=~(1u<<panel.btn[B_HOME]);
    frame();
    assert(ambr.view==1 && ambr.page==3);
}

static void check_layer_shortcut(void)
{
    amb_fixture_scene(0);
    amb_target(1);
    amb_patch(0,0,8);
    amb_layer_t *l=&amb.layer[0];
    assert(l->engine==ENGI_VOICE);
    l->lfo_dest=2;l->lfo_depth=31;
    int rate=l->base[P_LRATE],wave=l->base[P_LWAVE],breath=l->base[P_E5];
    ambr.view=1;ambr.page=1;ambr.bank=0;
    turn(EN_K3,1);
    assert(l->base[P_E5]!=breath);
    press(B_LFO);
    assert(l->lfo_dest==2);
    link_last_edit();
    assert(!strcmp(amb_route_name(l,l->lfo_dest),"BREATH"));
    assert(l->lfo_depth==31 && l->base[P_LRATE]==rate && l->base[P_LWAVE]==wave);

    ambr.view=0;amb_master(512,1);amb_master(100,0);link_last_edit();
    assert(l->lfo_dest==3 && l->lfo_depth==31);
    /* The shortcut targets the actual layer fader, also used by percussion
     * and the wet sends. Depth zero must reproduce the manual level. */
    int saved_level=l->level;assert(saved_level>0);trk[0].lfo_fade=32767;
    l->lfo_depth=100;
    trk[0].lfo_val=-32768;amb_prepare();assert(trk[0].p[P_LEVEL]==0);
    trk[0].lfo_val=32767;amb_prepare();assert(trk[0].p[P_LEVEL]==saved_level);
    trk[0].lfo_val=0;amb_prepare();assert(abs(2*trk[0].p[P_LEVEL]-saved_level)<=1);
    l->lfo_depth=0;amb_prepare();assert(trk[0].p[P_LEVEL]==saved_level);
    assert(l->level==saved_level && trk[0].p[P_LD_AMP]==0);

    held_turn(B_HOME,1);
    assert(ambr.edit_layer==1);
    uint8_t previous=amb.layer[1].lfo_dest;
    link_last_edit();
    assert(amb.layer[1].lfo_dest==previous);
    assert(!strcmp(ambr.notice,"TURN A LAYER KNOB FIRST"));
    held_turn(B_HOME,-1);
    assert(ambr.edit_layer==0);

    ambr.page=5;ambr.fx=5;
    turn(EN_K1,1);
    link_last_edit();
    assert(l->lfo_dest==ROUTE_GRAIN);
    ambr.page=6;
    turn(EN_K2,1);
    link_last_edit();
    assert(l->lfo_dest==ROUTE_EQ+1);
    ambr.page=5;ambr.fx=2;
    turn(EN_K4,1);
    link_last_edit();
    assert(l->lfo_dest==ROUTE_EQ+1);
    assert(!strcmp(ambr.notice,"THIS CONTROL CANNOT USE LFO"));

    amb_patch(0,0,0);
    l->lfo_dest=2;
    ambr.page=2;
    turn(EN_K4,-1);
    l->base[P_ATK]=0;
    turn(EN_K1,-1);
    link_last_edit();
    assert(l->base[P_ATK]==0);
    assert(!strcmp(amb_route_name(l,l->lfo_dest),"ATK"));
    amb_patch(0,0,1);
    link_last_edit();
    assert(l->lfo_dest==0);
    assert(!strcmp(ambr.notice,"TURN A LAYER KNOB FIRST"));
}

int main(void)
{
    unsigned patches=0, routes=0, voice_patches=0;
    memset(amb_nor,255,sizeof amb_nor);
    ui_power_on();amb_init();amb_fixture_scene(0);
    amb_layer_t *l=&amb.layer[0];
    for(unsigned role=0;role<NELEM(AMB_ROLE);role++)for(unsigned patch=0;patch<(role==6?NELEM(N_AS_SOURCE):amb_patch_count(role));patch++){
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
    check_layer_shortcut();
    check_random_rate();
    puts("HOME + LFO uses last actual layer edit, keeps LFO settings and rejects unavailable targets PASS");
    printf("%u patch/source macro sweeps, %u destinations, %u voice Breath sweeps, master reversibility and v4/v5 recall PASS\n",patches,routes,voice_patches);
    return 0;
}
