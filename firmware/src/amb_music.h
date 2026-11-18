/* SPDX-License-Identifier: GPL-3.0-only */
/* AMB+ event-level music and bounded source transitions; no saved layer access. */
#ifndef AMB_MUSIC_H
#define AMB_MUSIC_H
#include <stdint.h>
#include "amb_policy.h"
typedef struct {uint32_t turn;uint16_t remaining;uint8_t previous;} amb_walk_t;
static unsigned amb_walk_next(amb_walk_t *w,uint32_t seed,unsigned current,unsigned count)
{
    unsigned valid,available,safe,i,total=0,pick;uint8_t weights[7];
    if(count<3)count=3;if(count>7)count=7;current%=count;valid=(1u<<count)-1u;
    available=w->remaining&valid&~(1u<<current);
    if(!available)available=valid&~(1u<<current);
    safe=available;
    if(w->turn)safe&=~(1u<<w->previous);
    if(!safe)safe=valid&~(1u<<current)&~(1u<<w->previous);
    for(i=0;i<count;i++){
        unsigned distance=(i+count-current)%count;
        weights[i]=(uint8_t)((safe>>i)&1u?(distance==3 || distance==4?5:distance<=2?3:2):0);
        total+=weights[i];
    }
    pick=amb_hash(seed+0x9e3779b9u*(++w->turn)+current*173u)%total;
    for(i=0;i<count;i++)if(pick<weights[i])break;else pick-=weights[i];
    w->remaining=(uint16_t)(available&~(1u<<i));w->previous=(uint8_t)current;return i;
}
static unsigned amb_pad_pair(uint32_t seed,unsigned phrase,unsigned turn)
{
    switch(phrase%8){
    case 0:return (turn/3+amb_hash(seed+turn/7)%3)%6;
    case 1:return (turn/8+turn%3)%6;
    case 2:return amb_hash(seed+turn/2)%6;
    case 3:return (turn/4+(turn%4==3?1:0))%6;
    case 4:return (turn%4<2?turn/4:3+turn/4)%6;
    case 5:return (turn/3+turn/11)%6;
    case 6:return ((turn%16)/3+turn/64)%6;
    default:return (turn/16)%6;
    }
}
enum {AMB_FADE_STABLE,AMB_FADE_OUT,AMB_FADE_IN};
typedef struct {uint32_t pos;uint16_t from,g0,g1;uint8_t stage;} amb_fade_t;
static void amb_fade_begin(amb_fade_t *f,unsigned rate)
{
    (void)rate;
    if(f->stage==AMB_FADE_OUT)return; /* coalesce to latest target without a gain jump */
    f->from=f->stage?f->g1:32768u;f->g0=f->g1=f->from;f->pos=0;f->stage=AMB_FADE_OUT;
}
/* Returns one at silence: retire old sources and start fresh notes here.
 * The shared wet buffers never pass through this fader. */
static int amb_fade_tick(amb_fade_t *f,unsigned frames,unsigned rate)
{
    unsigned len,p,q;
    if(!f->stage){f->g0=f->g1=32768;return 0;}
    f->g0=f->g1;len=rate*(f->stage==AMB_FADE_OUT?6u:12u)/5u;
    f->pos=f->pos+frames>len?len:f->pos+frames;
    p=(f->pos<<15)/len; /* FM1 44.1 kHz / test 48 kHz: max < 2^32 */
    q=(p*p)>>15;q=(q*(98304u-2u*p))>>15; /* smoothstep, zero slope at both ends */
    f->g1=(uint16_t)(f->stage==AMB_FADE_OUT?((uint32_t)f->from*(32768u-q))>>15:q);
    if(f->stage==AMB_FADE_OUT && f->g1>f->g0)f->g1=f->g0;
    if(f->stage==AMB_FADE_IN && f->g1<f->g0)f->g1=f->g0;
    if(f->pos<len)return 0;
    f->pos=0;
    if(f->stage==AMB_FADE_OUT){f->stage=AMB_FADE_IN;return 1;}
    f->stage=AMB_FADE_STABLE;f->g1=32768;return 0;
}
#endif
