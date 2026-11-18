/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../firmware/src/amb_music.h"
int main(void)
{
    amb_policy_t policy;amb_participation_t parts[4]={{1,80},{1,80},{1,35},{1,20}};
    amb_policy_init(&policy,7);amb_policy_choose(&policy,0,1,1);amb_policy_boundary(&policy,64);assert(policy.active[0]==1);(void)amb_policy_events(&policy,0,parts);
    amb_walk_t w={0},repeat={0};unsigned d=0,a=0,old=99,i,seen=1,hashes[8];
    for(i=0;i<128;i++){
        unsigned next=amb_walk_next(&w,0x414d4200u,d,7);
        assert(next<7 && next!=d && next!=old);
        assert(next==amb_walk_next(&repeat,0x414d4200u,a,7));
        if(i<6)seen|=1u<<next;old=d;d=next;a=next;
    }assert(seen==127);
    for(a=3;a<=7;a++){
        w=(amb_walk_t){0};old=99;d=0;
        for(i=0;i<64;i++){unsigned next=amb_walk_next(&w,7,d,a);assert(next<a && next!=d && next!=old);old=d;d=next;}
    }
    for(a=0;a<8;a++){
        uint32_t h=2166136261u;unsigned pairs=0;
        for(i=0;i<128;i++){unsigned pair=amb_pad_pair(0x414d4200u,a,i);assert(pair<6);pairs|=1u<<pair;h=(h^pair)*16777619u;}
        assert((pairs&(pairs-1))!=0);hashes[a]=h;for(i=0;i<a;i++)assert(hashes[i]!=h);
    }
    {amb_fade_t f={0};unsigned prev=32768,done=0;
        amb_fade_begin(&f,48000);assert(f.stage==AMB_FADE_OUT);
        for(i=0;i<200000;i+=32){int event=amb_fade_tick(&f,32,48000);
            assert(f.g0<=32768 && f.g1<=32768);
            if(!done)assert(f.g1<=prev);else assert(f.g1>=prev);
            assert((int)f.g1-(int)f.g0<64 && (int)f.g0-(int)f.g1<64);
            if(event){assert(!done && f.g1==0);done=1;}prev=f.g1;
        }assert(done && f.stage==0 && f.g1==32768);
        amb_fade_begin(&f,48000);amb_fade_tick(&f,24000,48000);prev=f.g1;
        amb_fade_begin(&f,48000);assert(f.g1==prev); /* requeue cannot jump gain */
    }
    puts("AMB+ music: diverse anti-repeat progression / 8 Pad motifs / continuous tail-bridge fades PASS");return 0;
}
