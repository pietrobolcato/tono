/* SPDX-License-Identifier: GPL-3.0-only */
#define AMB_MAIN other_main
#include "amb_native_test.c"
int main(void){
    memset(amb_nor,255,sizeof amb_nor);ui_power_on();amb_init();
    press(B_GLO);press(B_SCL);press(B_EDIT);
    turn(EN_SELECT,1);assert(timing_tab==1);
    turn(EN_K1,1);assert(amb_vary_patterns());
    turn(EN_K2,1);assert(amb_lane_value(0,1)==WALK_BARS);
    turn(EN_SELECT,1);turn(EN_K1,1);turn(EN_K2,1);
    assert(amb_lane_value(1,0) && amb_lane_value(1,1)==WALK_BARS);
    for(unsigned lane=0;lane<2;lane++){amb_lane_adjust(lane,2,-120);amb_lane_adjust(lane,3,-120);}
    amb.layer[0].generated=0;amb_layer_t manual=amb.layer[0];
    song.playing=1;unsigned changed=0;
    for(unsigned bar=0;bar<25;bar++){
        unsigned roles[4];for(unsigned k=0;k<4;k++)roles[k]=amb.layer[k].role;
        ambr.step=16*bar;amb_step();
        unsigned diff=0,mask=0;for(unsigned k=0;k<4;k++){
            diff+=roles[k]!=amb.layer[k].role;mask|=1u<<amb.layer[k].role;
            assert(amb_valid(&amb));ambr.handoff[k]=ambr.fade_slot[k]=0;
        }
        assert(diff<=1 && (mask&3));changed+=diff;
        if(!bar)assert(!diff && amb.policy.active[0]==8);
        if(bar==1)assert(amb.policy.active[0]!=8);
    }
    assert(changed>8 && !memcmp(&manual,&amb.layer[0],sizeof manual));
    amb_lane_adjust(0,1,1); /* Seconds, with the previous one-unit interval. */
    fm1_ms=0xffffff00u;assert(!amb_lane_due(0));fm1_ms+=999;assert(!amb_lane_due(0));
    fm1_ms++;assert(amb_lane_due(0));assert(!amb_lane_due(0));
    amb_reset_lanes();fm1_ms+=10000;assert(!amb_lane_due(0));
    for(unsigned k=0;k<4;k++)amb.layer[k].generated=0;
    amb_layer_t locked[4];memcpy(locked,amb.layer,sizeof locked);amb_change_role();
    assert(!memcmp(locked,amb.layer,sizeof locked));
    amb_lane_adjust(0,1,1);unsigned serial=lane_clock[0].serial;
    amb_landscape_turn(1);assert(pattern_pending && lane_clock[0].serial==serial+1);
    amb_landscape_turn(1);assert(lane_clock[0].serial==serial+1);
    amb_ensemble_downbeat();assert(!pattern_pending);
    puts("Independent one-bar clocks, role diversity, manual track protection and valid snapshots PASS");
}
