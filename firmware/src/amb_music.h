/* SPDX-License-Identifier: GPL-3.0-only */
/* AMB+ event-level music; no saved layer access. */
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
#endif
