/* SPDX-License-Identifier: GPL-3.0-only */
#define AMB_MAIN unused_amb_main
#include "amb_native_test.c"

/* Capture just pitch and touch so reproducibility is independent of oscillator phase. */
static void phrase(uint32_t seed, unsigned base, uint8_t out[32])
{
    amb.policy.seed=seed;ambr.recent_valid[0]=0;amb_release(0);
    for(unsigned i=0;i<16;i++)amb_layer_step(0,base+i,i%4==0);
    memcpy(out,ambr.recent[0],16);memcpy(out+16,ambr.recent_vel[0],16);
}
int main(void)
{
    memset(amb_nor,255,sizeof amb_nor);ui_power_on();amb_init();amb_fixture_scene(0);
    for(unsigned role=3;role<=7;role+=4){
        amb_patch(0,role,role==3?11:1);
        amb_layer_t *l=&amb.layer[0];l->speed=0;l->generated=1;l->mode=2;l->density=80;
        amb_prepare();amb_render(7*FS);
        amb_policy_choose(&amb.policy,1,3,0);
        uint8_t old[32];phrase(41,0,old);
        assert(old[0]);
        for(unsigned i=0;i<16;i++)assert(!old[i] || old[16+i]==65);
        uint8_t acid_patch[32];
        amb_policy_choose(&amb.policy,1,AMB_ACID_PHRASE,0);
        phrase(41,0,acid_patch);
        amb_policy_choose(&amb.policy,1,3,0);
        amb_patch(0,role,0);amb_prepare();amb_render(7*FS);
        amb_policy_choose(&amb.policy,1,8,1);amb_policy_boundary(&amb.policy,63);
        assert(amb.policy.active[1]==3);
        amb_policy_boundary(&amb.policy,64);assert(amb.policy.active[1]==8 && amb_valid(&amb));
        unsigned active=0,accents=0,rests=0;uint32_t pitches=0;
        for(unsigned step=0;step<64;step++){
            amb_layer_step(0,step,step%4==0);
            unsigned slot=step%16,note=ambr.recent[0][slot];
            if(note){active++;pitches|=1u<<(note%24);accents+=ambr.recent_vel[0][slot]>=96;}else rests++;
        }
        assert(active>16 && rests && accents && __builtin_popcount(pitches)>=3);
        uint8_t a[32],b[32];
        phrase(41,0,a);assert(!memcmp(a,acid_patch,sizeof a));
        phrase(41,0,b);assert(!memcmp(a,b,sizeof a));
        phrase(97,0,b);assert(memcmp(a,b,sizeof a));
        phrase(41,64,b);assert(memcmp(a,b,sizeof a));
        amb_capture_generated(0);
        assert(l->drums[3] && !memcmp(l->velocity,ambr.recent_vel[0],16));
    }
    puts("Acid phrases: syncopation, pitch range, accents, rests and slide capture PASS");
}
