/* SPDX-License-Identifier: GPL-3.0-only */
#include "amb_rain_drops.h"

static int rain_sample(const uint8_t *packed,unsigned pos)
{
    static const int8_t levels[16]={0,2,5,11,22,40,73,127,0,-2,-5,-11,-22,-40,-73,-127};
    return levels[(packed[pos>>1]>>((pos&1)*4))&15];
}

/* Randomly scatter recorded splashes over a continuous air and water bed.
 * The thunder envelope has its own clock, independent of the drop triggers. */
static void rain_render(track_t *t,voice_t *v,int32_t *out,uint32_t n,const vmod_t *m)
{
    int32_t rng=v->s[2],mid=v->s[1],thunder=v->s[3];
    unsigned pos=v->ph[0],drop=v->ph[1]&3,step=(v->ph[1]>>3)&255;
    int amp=v->ph[1]>>11;
    int rumble=v->s[0];
    int air=clamp(t->p[P_E1]+(m->shape>>8)-64,0,127),drops=t->p[P_E5],storm=t->p[P_E6];
    int bright=clamp(t->p[P_E2]+(m->cutoff>>8),0,127);
    int width=t->p[P_E3]+64, speed=22+t->p[P_E4]/2, grain=t->p[P_E7]+32;
    unsigned clock=(unsigned)v->s[4];
    int top=rain_spectrum[(clock>>9)&255]*air*bright>>13;
    uint32_t bed=v->s[5],end=v->s[6],start=v->s[7];
    uint32_t due=v->ph[2];
    if(!due){due=FS/2+((uint32_t)rng & 32767);thunder=2047<<10;}
    for(unsigned i=0;i<n;i++){
        unsigned r=noise32(&rng);
        if(bed>=end){
            unsigned span=3000+(r&2047);
            start=(((r>>16)*(8190-span))>>16)<<8;
            bed=start;
            end=start+(span<<8);
        }
        unsigned j=bed>>8;
        int field=rain_sample(rain_forest,j);
        field+=(rain_sample(rain_forest,j+1)-field)*(int)(bed&255)>>8;
        unsigned edge=bed-start<end-bed?bed-start:end-bed;
        if(edge<8192)field=field*(int)edge>>13;
        bed+=70;
        int noise=(int)(r>>16)-32768;
        mid+=(noise-mid)*95>>8;
        int fall=field*(air+20)+((noise-mid)*top>>8);
        unsigned frames=drop==2?1024:512;
        if(pos>=frames*256 || !step){
            if((r&65535u)<(unsigned)(3+drops*drops/120)){
                drop=(r>>16)&3;
                if(drop>2)drop=2;
                frames=drop==2?1024:512;
                step=speed+((r>>22)&15);
                if(drop==2)step/=2;
                amp=20+((r>>19)&63);
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
                int crack=rumble*rain_thunder_air[j>>4]>>6;
                fall+=(bass*192+crack)*storm>>6;
                thunder+=6; /* 258.5 Hz at the engine sample rate. */
            }
        }
        out[i]+=voice_amp(clamp(fall*width>>7,-30000,30000),m,i);
    }
    v->s[0]=rumble;v->s[5]=bed;v->s[6]=end;v->s[7]=start;v->s[1]=mid;v->s[2]=rng;v->s[3]=thunder;v->s[4]=(clock+n)&131071;
    v->ph[0]=pos;v->ph[1]=drop|(step<<3)|((unsigned)amp<<11);v->ph[2]=due;
}
