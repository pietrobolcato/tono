/* SPDX-License-Identifier: GPL-3.0-only */
#define AMB_MAIN unused_amb_main
#include "amb_native_test.c"
int main(void)
{
    memset(amb_nor,255,sizeof amb_nor);ui_power_on();amb_init();
    assert(!amb_walk_value(0) && !amb_journey_style());
    press(B_GLO);for(int i=0;i<7;i++)press(B_SCL);
    assert(ambr.view==2 && ambr.page==7 && !amb_page_bank());
    turn(EN_SELECT,1);assert(amb_journey_style()==1);
    turn(EN_K1,2);turn(EN_K2,-3);turn(EN_K3,-3);
    assert(amb_walk_value(0)==WALK_SECONDS && amb_walk_value(1)==1 && amb_walk_value(2)==1);
    uint8_t pref[5];memcpy(pref,amb_walk_config(),sizeof pref);
    amb_fixture_scene(3);assert(!memcmp(amb_walk_config(),pref,sizeof pref));
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
    puts("Automatic travel: clock wrap, fixed/random intervals, bounds, all three routes, manual priority, persistent settings and STOP PASS");
    return 0;
}
