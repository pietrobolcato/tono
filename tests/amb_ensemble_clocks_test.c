/* SPDX-License-Identifier: GPL-3.0-only */
#define AMB_MAIN other_main
#include "amb_native_test.c"
static void check_auto_off(void)
{
    for(unsigned unit=LANE_FOLLOW;unit<=WALK_SECONDS;unit++){
        amb_fixture_scene(0);press(B_HOME);
        amb_walk_clock(WALK_BARS,1);
        amb_lane_adjust(0,0,2);amb_lane_adjust(1,0,1);
        song.playing=1;
        amb_follow_travel_tick();
        assert(pattern_pending && role_pending);
        if(unit){
            for(unsigned lane=0;lane<2;lane++){
                amb_lane_adjust(lane,1,unit);
                amb_lane_adjust(lane,2,-120);amb_lane_adjust(lane,3,-120);
                assert(!amb_lane_due(lane));
            }
        }
        amb_notices.travel=1;amb_landscape_turn(8);
        unsigned chord=amb.chord,roles[4],patches[4];
        uint8_t styles[3];memcpy(styles,amb.policy.active,sizeof styles);
        for(unsigned k=0;k<4;k++){roles[k]=amb.layer[k].role;patches[k]=amb.layer[k].patch;}
        btn_down(B_HOME);frame();press(B_GLO);btn_up(B_HOME);frame();
        assert(!amb_walk_value(0) && !role_pending && !pattern_pending && !travel.pending_layers);
        for(unsigned i=0;i<512;i++){
            ambr.step=i;fm1_ms+=250;amb_auto_tick();amb_step();
            assert(amb.chord==chord && !memcmp(styles,amb.policy.active,sizeof styles));
            for(unsigned k=0;k<4;k++)assert(roles[k]==amb.layer[k].role && patches[k]==amb.layer[k].patch);
        }
        assert(!lane_clock[0].armed && !lane_clock[1].armed && !auto_walk.armed);
        amb_policy_choose(&amb.policy,0,(styles[0]+1)%amb_style_count(0),1);
        ambr.step=512;amb_step();assert(amb.policy.active[0]!=styles[0]);
        amb_walk_clock(WALK_BARS,1);
        amb_ensemble_downbeat();
        assert(!pattern_pending && !role_pending);
        if(unit){
            assert(lane_clock[0].armed);
            unsigned serial=lane_clock[0].serial;
            ambr.step+=16;fm1_ms+=1000;amb_ensemble_downbeat();
            assert(lane_clock[0].serial==serial+1);
        }
        song.playing=0;
    }
    puts("Auto Off cancels pending travel and both lanes, blocks timed changes, accepts manual rhythm and resumes on a fresh interval PASS");
}
int main(void){
    memset(amb_nor,255,sizeof amb_nor);ui_power_on();amb_init();
    press(B_GLO);press(B_SCL);press(B_EDIT);
    turn(EN_SELECT,1);assert(timing_tab==1);
    assert(amb_lane_value(0,1)==LANE_FOLLOW);
    turn(EN_K1,1);assert(amb_vary_patterns() && amb_lane_value(0,1)==LANE_FOLLOW);
    turn(EN_K2,1);assert(amb_lane_value(0,1)==WALK_BARS);
    turn(EN_SELECT,1);turn(EN_K1,1);turn(EN_K2,1);
    assert(amb_lane_value(1,0) && amb_lane_value(1,1)==WALK_BARS);
    for(unsigned lane=0;lane<2;lane++){amb_lane_adjust(lane,2,-120);amb_lane_adjust(lane,3,-120);}
    amb.layer[0].generated=0;
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
    assert(changed>8 && amb.layer[0].generated);
    amb_lane_adjust(0,1,1); /* Seconds, with the previous one-unit interval. */
    fm1_ms=0xffffff00u;assert(!amb_lane_due(0));fm1_ms+=999;assert(!amb_lane_due(0));
    fm1_ms++;assert(amb_lane_due(0));assert(!amb_lane_due(0));
    amb_reset_lanes();fm1_ms+=10000;assert(!amb_lane_due(0));
    for(unsigned k=0;k<4;k++)amb.layer[k].generated=0;
    amb_change_role();
    assert(amb.layer[0].generated+amb.layer[1].generated+amb.layer[2].generated+amb.layer[3].generated==1);
    amb_lane_adjust(0,1,-2);unsigned serial=lane_clock[0].serial;
    amb_landscape_turn(1);assert(!pattern_pending && lane_clock[0].serial==serial);
    amb_follow_travel_tick();assert(pattern_pending && lane_clock[0].serial==serial+1);
    amb_follow_travel_tick();assert(lane_clock[0].serial==serial+1);
    amb_ensemble_downbeat();assert(!pattern_pending);
    amb_lane_adjust(0,0,1);
    assert(amb_lane_value(0,0)==2);
    for(unsigned i=0;i<32;i++){
        amb_policy_t old=amb.policy;
        lane_clock[0].serial++;
        amb_pattern_tick();
        assert(memcmp(old.active,amb.policy.active,3));
        unsigned match=0;
        for(unsigned j=0;j<NELEM(amb_generate_recipes);j++)
            match|=!memcmp(amb.policy.active,amb_generate_recipes[j].values,3)
                  && amb.policy.variation==amb_generate_recipes[j].values[3];
        assert(match);
    }
    amb_lane_adjust(0,0,-1);assert(amb_lane_value(0,0)==1);
    amb_lane_adjust(0,0,-1);assert(!amb_lane_value(0,0));
    check_auto_off();
    puts("Independent one-bar clocks, role diversity, manual to generated role handoff and valid snapshots PASS");
}
