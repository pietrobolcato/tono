/* SPDX-License-Identifier: GPL-3.0-only */
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#define AMBPLUS 1
#define FS 44118
#define RING_PUBLISH() __asm__ volatile("" ::: "memory")
static int clamp(int v,int lo,int hi){return v<lo?lo:v>hi?hi:v;}
static uint32_t pitch_inc(unsigned p){return 440.*pow(2.,((double)p/16-69)/12)*4294967296./FS;}
#include "../firmware/src/amb_take.h"
#define main upstream_uac
#include "uac_test.c"
#undef main
int main(void)
{
    assert(!upstream_uac());
    amb_take_start();
    for(unsigned ms=0;ms<2000;ms++){
        uint32_t packet[45];unsigned start=ms*441/10,end=(ms+1)*441/10;
        for(unsigned j=start;j<end;j++){
            int16_t x=sin(j*2.*3.14159265359*330./44100)*20000;
            packet[j-start]=(uint16_t)x|((uint32_t)(uint16_t)x<<16);
        }
        uin_take(packet,4*(end-start));
    }
    assert(!take_rec.running && amb_take_finish());
    assert(amb_current_take()->root==64 && amb_current_take()->length==AMB_TAKE_FRAMES);
    puts("Real EP4 packet handler captured E4 even with a full monitoring ring.");
    return 0;
}
