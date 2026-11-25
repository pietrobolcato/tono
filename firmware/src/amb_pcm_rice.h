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
} amb_pcm_block_t;

typedef struct {
    amb_pcm_block_t block[4];
    uint8_t order[4];
} amb_pcm_cache_t;

static amb_pcm_cache_t amb_pcm_cache[NTRK];

static void amb_pcm_invalidate(unsigned track)
{
    amb_pcm_cache_t *c=&amb_pcm_cache[track];
    for(unsigned i=0;i<4;i++){ c->block[i].key=0;c->order[i]=(uint8_t)i; }
}

static unsigned amb_rice_code(unsigned track, unsigned source, const uint8_t *data,
                              const uint16_t *offsets, unsigned period, unsigned at)
{
    amb_pcm_cache_t *cache=&amb_pcm_cache[track];
    unsigned id=at>>8, index=at&255, key=1+(id<<3)+source, slot=cache->order[0];
    amb_pcm_block_t *b=&cache->block[slot];
    if(b->key!=key){
        unsigned pos=1;
        while(pos<4 && cache->block[cache->order[pos]].key!=key)pos++;
        if(pos==4)pos=3;
        slot=cache->order[pos];
        for(unsigned i=pos;i;i--)cache->order[i]=cache->order[i-1];
        cache->order[0]=(uint8_t)slot;
        b=&cache->block[slot];
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
    uint8_t *value=voice_bank[track].acoustic[slot];
    /* Decode a small run at once so the flash reader, predictor and Rice width
     * stay in registers. The sample array is the prediction history. */
    unsigned i=b->fill;
    if(i<=index){
        unsigned bit=b->bit, mode=b->mode, lag=b->lag, fraction=b->fraction, k=b->k;
        int error=b->error;
        unsigned end=(index|7u)+1;
        while(i<end){
            if(!(i&31))k=(b->header[i>>6]>>((i>>3)&4))&15;
            const uint8_t *p=b->bits+(bit>>3);
            unsigned bits=p[0]|((unsigned)p[1]<<8)|((unsigned)p[2]<<16);
            bits>>=bit&7;
            unsigned residual=0, sample;
            if(k==8){
                sample=bits&255;
                bit+=8;
            }else{
                unsigned z=0;
                while(!bits){
                    unsigned skip=24-(bit&7);
                    z+=skip;bit+=skip;p=b->bits+(bit>>3);
                    bits=p[0]|((unsigned)p[1]<<8)|((unsigned)p[2]<<16);
                }
                unsigned zeros=(unsigned)__builtin_ctz(bits);
                if(zeros+1+k>24-(bit&7)){
                    p=b->bits+((bit+zeros+1)>>3);
                    residual=(p[0]|((unsigned)p[1]<<8))>>((bit+zeros+1)&7);
                }else residual=bits>>(zeros+1);
                residual=((z+zeros)<<k)+(residual&((1u<<k)-1));
                bit+=zeros+1+k;
                sample=0;
            }
            int pred=i?value[i-1]:128;
            if(mode>=2 && i>lag){
                unsigned at=i-lag;
                int ref=value[at];
                ref+=((value[at-1]-ref)*(int)fraction+2)>>2;
                if(k!=8){
                    pred=ref+((mode&1)?error:0);
                    sample=pred+((int)(residual>>1)^-(int)(residual&1));
                }
                error=(int)sample-ref;
            }else if(k!=8){
                if(mode==1 && i>=2)pred+=pred-value[i-2];
                sample=pred+((int)(residual>>1)^-(int)(residual&1));
            }
            value[i++]=(uint8_t)sample;
        }
        b->k=k;b->bit=bit;b->fill=i;b->error=error;
    }
    return value[index];
}
