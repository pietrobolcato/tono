/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef AMB_LFO_RATE_H
#define AMB_LFO_RATE_H
/* Ordinary signed rates (-12..127) keep their meaning. The two bounds use
 * the same logarithmic Hz steps as the fixed knob, in the same save word. */
enum { AMB_RANDOM_HZ = 0x4000 };
static unsigned amb_rate_min(int rate){return ((unsigned)rate>>7)&127u;}
static unsigned amb_rate_max(int rate){return (unsigned)rate&127u;}
static int amb_rate_range(unsigned lo,unsigned hi){return AMB_RANDOM_HZ|(lo<<7)|hi;}
static int amb_cycle_rate(int rate,uint32_t held,unsigned layer)
{
    if(rate<AMB_RANDOM_HZ)return rate;
    unsigned lo=amb_rate_min(rate),hi=amb_rate_max(rate);
    /* Mix the once-per-cycle sample-and-hold clock without consuming musical
     * RNG state or making a high S&H amplitude imply a high frequency. */
    uint32_t draw=(held^(0x9e3779b9u*(layer+1u)))*0x85ebca6bu;
    return (int)lo+(int)(((draw>>16)*(hi-lo+1u))>>16);
}
#endif
