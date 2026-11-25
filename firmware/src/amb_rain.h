/* SPDX-License-Identifier: GPL-3.0-only */
#include "amb_rain_drops.h"

/* A held key can change between NOISE sources. Distinguish their engine
 * histories using a marker outside the range of the other sources' filters. */
#define RAIN_STATE_TAG 0x52400000u

static int rain_sample(const uint8_t *packed,unsigned pos)
{
    static const int8_t levels[16]={0,2,5,11,22,40,73,127,0,-2,-5,-11,-22,-40,-73,-127};
    return levels[(packed[pos>>1]>>((pos&1)*4))&15];
}

static int rain_bed(unsigned j)
{
    static const int8_t q[4]={1,4,-1,-4};
    return q[(rain_forest[j>>2]>>((j&3)*2))&3]*rain_scales[j>>5];
}

/* Randomly scatter recorded splashes over a continuous air and water bed.
 * The thunder envelope has its own clock, independent of the drop triggers. */
static void rain_render(track_t *t,voice_t *v,int32_t *out,uint32_t n,const vmod_t *m)
{
    if((uint32_t)v->s[6]!=RAIN_STATE_TAG)noise_note_on(t,v);
    int32_t rng=v->s[2],mid=v->s[1],thunder=v->s[3];
    unsigned pos=v->ph[0],drop=v->ph[1]&3,step=(v->ph[1]>>3)&255;
    int amp=v->ph[1]>>11;
    int rumble=v->s[0];
    int air=clamp(t->p[P_E1]+(m->shape>>8)-64,0,127),drops=t->p[P_E5],storm=t->p[P_E6];
    int bright=clamp(t->p[P_E2]+(m->cutoff>>8),0,127);
    int width=t->p[P_E3]+64, speed=22+t->p[P_E4]/2, grain=t->p[P_E7]+32;
    unsigned clock=(unsigned)v->s[4];
    uint32_t field_pos=v->s[5];
    int body=v->s[7];
    const uint8_t *band=rain_bands+((clock>>11)&511)*2;
    const uint8_t *next=rain_bands+(((clock>>11)+1)&511)*2;
    int weights[2];
    for(unsigned i=0;i<2;i++)weights[i]=band[i]+((next[i]-band[i])*(int)(clock&2047)>>11);
    uint32_t due=v->ph[2];
    if(!due){due=FS/2+((uint32_t)rng & 32767);thunder=2047<<10;}
    for(unsigned i=0;i<n;i++){
        unsigned r=noise32(&rng);
        int noise=(int)(r>>16)-32768;
        /* The original forest recording supplies the low bands, with the
         * higher spectrum following its independent 24-second field trace. */
        unsigned j=field_pos>>8;
        int field=rain_bed(j);
        field+=(rain_bed((j+1)&16383)-field)*(int)(field_pos&255)>>8;
        unsigned tail=(16384-j);
        if(tail<256){
            int head=rain_bed(256-tail);
            field=(field*(int)tail+head*(int)(256-tail))>>8;
        }
        field_pos+=24;
        if(field_pos>=16384*256u){
            /* Wrap into the first 256 frames already crossfaded above. */
            field_pos=(field_pos&(16384*256u-1)) | (256*256u);
        }
        body+=(noise-body)*40>>8;
        mid+=(noise-mid)*136>>8;
        int upper=(mid-body)*weights[0]+(noise-mid)*weights[1];
        int fall=(field*30+((upper>>8)*(bright+48)>>6))*(air+58)>>7;
        unsigned frames=drop==2?1024:512;
        if(pos>=frames*256 || !step){
            if((r&65535u)<(unsigned)(2+drops*drops/160)){
                drop=(r>>16)&3;
                if(drop>2)drop=2;
                frames=drop==2?1024:512;
                step=speed+((r>>22)&15);
                if(drop==2)step/=2;
                amp=13+((r>>19)&63);
                pos=0;
            }
        }
        if(pos<frames*256 && step){
            unsigned index=pos>>8;
            const uint8_t *take=rain_drops+256*drop;
            int sample=rain_sample(take,index);
            if(index+1<frames)sample+=(rain_sample(take,index+1)-sample)*(int)(pos&255)>>8;
            fall+=sample*amp*grain>>(drop==2?7:9);
            pos+=step;
        }
        if(storm){
            if(!--due){
                due=FS*10+(r&524287);
                thunder=0;
            }
            if((unsigned)thunder<(2047u<<10)){
                unsigned j=(unsigned)thunder>>10;
                int first=rain_sample(rain_thunder,j),second=rain_sample(rain_thunder,j+1);
                int bass=first+(((second-first)*(thunder&1023))>>10);
                rumble+=(noise-rumble)>>4;
                int crack=rumble*rain_thunder_air[j>>4]>>9;
                fall+=(bass*197+crack)*storm>>6;
                thunder+=6; /* 258.5 Hz at the engine sample rate. */
            }
        }
        out[i]+=voice_amp(clamp(fall*width>>7,-30000,30000),m,i);
    }
    v->s[0]=rumble;v->s[5]=field_pos;v->s[6]=RAIN_STATE_TAG;v->s[7]=body;v->s[1]=mid;v->s[2]=rng;v->s[3]=thunder;v->s[4]=(clock+n)&1048575;
    v->ph[0]=pos;v->ph[1]=drop|(step<<3)|((unsigned)amp<<11);v->ph[2]=due;
}
