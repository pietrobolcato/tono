/* SPDX-License-Identifier: GPL-3.0-only
 * Desktop entry point for the firmware's take codec. Input: stereo 44.1 kHz
 * little-endian s16 from the companion. Output: the optional song trailer. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define AMB_TAKE_STORAGE
#include <math.h>
#define FS 44118
#define RING_PUBLISH() ((void)0)
static int clamp(int v,int lo,int hi){return v<lo?lo:v>hi?hi:v;}
static uint32_t pitch_inc(unsigned key16){
    return (uint32_t)(440.0*pow(2.0,((double)key16/16-69)/12)*4294967296.0/FS);
}
#include "../../firmware/src/amb_take.h"
int main(void){
    uint32_t frames[128];size_t n;
    amb_take_start();
    while((n=fread(frames,sizeof *frames,128,stdin)))amb_take_feed(frames,(unsigned)n);
    if(ferror(stdin) || !amb_take_finish() || !amb_take_valid(amb_current_take())){
        fputs("No usable audio. Select a passage with a voice or instrument.\n",stderr);return 1;
    }
    const amb_take_t *t=amb_current_take();
    fprintf(stderr,"%.2f seconds, %s MIDI %u\n",t->length/4900.0,t->period_q8?"matched":"reference",t->root);
    return fwrite(t,sizeof *t,1,stdout)==1?0:1;
}
