/* SPDX-License-Identifier: GPL-3.0-only
 * Seekable lossless storage of factory acoustic PCM. A block is independent,
 * with one predictor and eight Rice widths for groups of 32 source frames.
 * Decode forward only as far as needed. Caching the decoded prefix also serves
 * reverse notes, adjacent samples for interpolation and overlapping loop tails. */
#include "amb_pcm_rice_data.h"

typedef struct {
    unsigned key, fill, bit, mode, lag, fraction, k;
    int error;
    const uint8_t *bits, *header;
    uint8_t value[256];
} amb_pcm_block_t;

typedef struct {
    amb_pcm_block_t block[2];
    unsigned recent;
} amb_pcm_cache_t;

/* Each audio worker owns complete tracks during a render, so separate per-track
 * caches avoid locks or shared mutable cursors across the two cores. */
static amb_pcm_cache_t amb_pcm_main_cache[5];
static amb_pcm_cache_t amb_pcm_low_cache AMB_DIRECTORY_RAM;

static unsigned amb_rice_code(amb_pcm_cache_t *cache, unsigned source, const uint8_t *data,
                              const uint16_t *offsets, unsigned period, unsigned at)
{
    unsigned id=at>>8, index=at&255, key=1+(id<<3)+source;
    amb_pcm_block_t *b=&cache->block[cache->recent];
    if(b->key!=key){
        cache->recent^=1;
        b=&cache->block[cache->recent];
        if(b->key!=key){
            const uint8_t *header=data+offsets[id];
            b->key=key;
            b->fill=b->bit=b->error=0;
            b->mode=*header++;
            b->lag=period+((int)b->mode-2)/8-2;
            b->fraction=((b->mode-2)>>1)&3;
            b->header=header;
            b->bits=header+4;
        }
    }
    while(b->fill<=index){
        unsigned i=b->fill, bit=b->bit;
        if(!(i&31))b->k=(b->header[i>>6]>>((i>>3)&4))&15;
        unsigned k=b->k;
        const uint8_t *p=b->bits+(bit>>3);
        unsigned bits=(p[0]|((unsigned)p[1]<<8)|((unsigned)p[2]<<16))>>(bit&7);
        unsigned ref=0, value;
        int pred=i?b->value[i-1]:128;
        if(b->mode>=2 && i>b->lag){
            unsigned prev=i-b->lag;
            ref=b->value[prev];
            ref+=(int)(((int)b->value[prev-1]-(int)ref)*(int)b->fraction+2)>>2;
            pred=(int)ref;
            if(b->mode&1)pred+=b->error;
        }else if(b->mode==1 && i>=2){
            pred+=pred-b->value[i-2];
        }
        if(k==8){
            value=bits&255;
            bit+=8;
        }else{
            while(!bits){
                unsigned advance=24-(bit&7);
                bit+=advance;
                p=b->bits+(bit>>3);
                bits=p[0]|((unsigned)p[1]<<8)|((unsigned)p[2]<<16);
            }
            unsigned zeros=(unsigned)__builtin_ctz(bits);
            unsigned quotient=bit-b->bit+zeros;
            bit+=zeros+1;
            p=b->bits+(bit>>3);
            unsigned low=(p[0]|((unsigned)p[1]<<8))>>(bit&7);
            unsigned residual=(quotient<<k)+(low&((1u<<k)-1));
            bit+=k;
            int difference=(int)(residual>>1)^-(int)(residual&1);
            value=pred+difference;
        }
        b->value[i]=(uint8_t)value;
        if(b->mode>=2 && i>b->lag)b->error=(int)value-(int)ref;
        b->bit=bit;
        b->fill=i+1;
    }
    return b->value[index];
}
