/* SPDX-License-Identifier: GPL-3.0-only */
#define AMB_MAIN unused_amb_main
#include "amb_native_test.c"
int main(void)
{
    memset(amb_nor,255,sizeof amb_nor);ui_power_on();amb_init();
    assert(amb_walk_value(0)==WALK_BARS && amb_walk_value(1)==4 && amb_walk_value(2)==16 && amb_walk_value(3)==32);
    assert(amb_journey_style()==2 && amb_endless() && amb_crossfade() && amb.release_ms==6000);
    assert(!amb_lane_value(0,0) && !amb_lane_value(1,0));
    /* Off is an explicit setting on this song. */
    memset(amb_walk_config(),0,6);
    assert(!amb_walk_value(0) && !amb_journey_style());
    press(B_GLO);turn(EN_SELECT,1);unsigned interval=amb.auto_index;
    press(B_SCL);assert(ambr.page==6);press(B_EDIT);assert(ambr.page==7);
    press(B_SCL);assert(ambr.page==1);turn(EN_K4,1);assert(amb.auto_index==interval);
    const unsigned pages[]={2,3,5}; /* Generate, Master FX (includes EQ), Theme. */
    for(unsigned i=0;i<NELEM(pages);i++){press(B_SCL);assert(ambr.page==pages[i]);}
    press(B_SCL);assert(ambr.page==0);press(B_SCL);
    turn(EN_K4,1);assert(amb_walk_value(0)==WALK_BARS);
    press(B_EDIT);turn(EN_K1,1);assert(amb_walk_value(0)==WALK_SECONDS);
    press(B_EDIT);assert(ambr.page==6);turn(EN_K4,-1);assert(!amb_walk_value(0) && amb_walk_unit()==WALK_SECONDS);
    turn(EN_K4,1);assert(amb_walk_value(0)==WALK_SECONDS);
    turn(EN_SELECT,1);assert(amb_journey_style()==1);
    press(B_EDIT);
    assert(ambr.view==2 && ambr.page==7 && !amb_page_bank());
    assert(!amb_vary_patterns());turn(EN_SELECT,1);turn(EN_K1,1);assert(amb_vary_patterns());
    turn(EN_K1,-1);turn(EN_SELECT,-1);assert(!amb_vary_patterns());
    turn(EN_K2,-3);turn(EN_K3,-3);
    assert(amb_walk_value(0)==WALK_SECONDS && amb_walk_value(1)==1 && amb_walk_value(2)==1);
    uint8_t pref[6];memcpy(pref,amb_walk_config(),sizeof pref);
    amb_fixture_scene(3);assert(!memcmp(amb_walk_config(),amb_factory_controls.walk,sizeof pref));
    memcpy(amb_walk_config(),pref,sizeof pref); /* clock tests use one-second intervals */
    song.playing=1;uint32_t start=fm1_ms=0xffffff00u;amb_auto_tick();
    fm1_ms=start+999;amb_auto_tick();assert(!travel.wanted);
    fm1_ms++;amb_auto_tick();assert(travel.wanted && !travel.indicator);
    press(B_HOME);fm1_ms=auto_walk.due;int before=travel.wanted;
    turn(EN_ALGO,1);assert(travel.wanted==before+1 && !auto_walk.armed);
    amb_auto_tick();assert((int32_t)(auto_walk.due-fm1_ms)>0);
    song.playing=0;fm1_ms+=4000;amb_auto_tick();assert(!auto_walk.armed && travel.wanted==before+1);
    amb_walk_adjust(0,-1);song.playing=1;ambr.step=1;amb_auto_tick();assert(auto_walk.due==16);
    ambr.step=15;amb_auto_tick();assert(travel.wanted==before+1);
    ambr.step=16;amb_auto_tick();assert(travel.wanted!=before+1 && auto_walk.due==32);
    amb_walk_adjust(2,9);unsigned intervals=0;
    for(unsigned i=0;i<90;i++){
        ambr.step=auto_walk.armed?auto_walk.due:32;amb_auto_tick();
        unsigned bars=(auto_walk.due-ambr.step)/16;assert(bars>=1 && bars<=10);intervals|=1u<<bars;
    }
    assert(intervals&(intervals-1));
    for(unsigned style=0;style<3;style++){
        amb_walk_config()[4]=style;amb_auto_reset();travel.wanted=0;
        int low=0,high=0;
        for(unsigned i=0;i<400;i++){
            travel.wanted+=amb_auto_move(amb_hash(amb.policy.seed+i));
            assert(travel.wanted>=-18 && travel.wanted<=18);
            if(travel.wanted<low)low=travel.wanted;
            if(travel.wanted>high)high=travel.wanted;
        }
        assert(low<-3 && high>3);
    }
    song.playing=0;amb_walk_config()[4]=0;amb_journey_recentre();
    amb.policy.active[0]=8;memset(amb.policy.pending,255,3);
    amb_landscape_turn(4);assert(amb.policy.active[0]==8);
    amb_controls.lanes[0]=4; /* Old Journey clock. */
    amb_controls.lanes[3]=1; /* Old Move tick clock. */
    amb_controls.walk[5]|=4|8;
    assert(amb_lane_value(0,1)==LANE_FOLLOW && amb_lane_value(1,1)==LANE_FOLLOW);
    amb_walk_clock(WALK_MINUTES,0);
    assert(amb_walk_unit()==WALK_MINUTES && !amb_walk_value(0));
    amb_walk_adjust(0,1);
    assert(amb_walk_unit()==WALK_HOURS && !amb_walk_value(0));
    amb_walk_adjust(2,-120);amb_walk_clock(WALK_HOURS,1);
    song.playing=1;start=fm1_ms=0xffff0000u;amb_auto_tick();
    assert(auto_walk.due==start+3600000u);
    fm1_ms=auto_walk.due-1;amb_auto_tick();assert(!pattern_pending && !role_pending);
    amb_landscape_turn(1);assert(!pattern_pending && !role_pending);
    fm1_ms++;amb_auto_tick();assert(pattern_pending && role_pending);
    amb_ensemble_downbeat();assert(!pattern_pending && !role_pending && amb.policy.active[0]==0);
    amb_walk_clock(WALK_MINUTES,1);amb_auto_tick();
    assert(auto_walk.due==fm1_ms+60000u);
    puts("Automatic travel: clock wrap, fixed/random intervals, bounds, all three routes, manual priority, persistent settings and STOP PASS");
    return 0;
}
