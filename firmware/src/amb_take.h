/* SPDX-License-Identifier: GPL-3.0-only
 * A song can carry one take inside its existing A/B object. Restart points
 * every 32 samples allow independent seek, pitch, trimming and loop overlap. */
#define AMB_TAKE_USB 1
enum { AMB_TAKE_BLOCKS=220, AMB_TAKE_FRAMES=32*AMB_TAKE_BLOCKS, AMB_MIC_SOURCE=16 };
typedef struct {uint16_t length,period_q8;uint8_t root,format,block[AMB_TAKE_BLOCKS][10];} amb_take_t;
#ifndef AMB_TAKE_STORAGE
#define AMB_TAKE_STORAGE __attribute__((section(".pool")))
#endif
static amb_take_t take_banks[2] AMB_TAKE_STORAGE;
static uint8_t take_active,take_generation[2];
static struct {volatile uint8_t running;unsigned frames,phase;int sum,predict,step,peak;} take_rec;
static amb_take_t *amb_current_take(void){return &take_banks[take_active];}
static int amb_take_delta(unsigned code,int *predict,int *step)
{
    int delta=code&1?3 * *step:*step;
    *predict=clamp(*predict+(code&2?-delta:delta),-127,127);
    *step=clamp(code&1?*step+(*step+1)/2:*step-(*step+3)/4,1,64);
    return *predict;
}
static void amb_decode_codes(const uint8_t *codes,int pred,int step,int16_t out[32])
{
    for(unsigned i=0;i<32;i++)
        out[i]=(int16_t)(256*amb_take_delta((codes[i/4]>>(2*(i%4)))&3,&pred,&step));
}
static void amb_take_decode(const uint8_t *b,int16_t out[32])
{
    amb_decode_codes(b+2,(int8_t)b[0],b[1],out);
}
static void amb_take_start(void)
{
    take_rec.running=0;take_rec.frames=take_rec.phase=0;
    take_rec.sum=take_rec.predict=take_rec.peak=0;take_rec.step=1;
    take_generation[take_active^1]++;
    RING_PUBLISH();take_rec.running=1;
}
static void amb_take_feed(const uint32_t *p,unsigned n)
{
    if(!take_rec.running)return;
    amb_take_t *t=&take_banks[take_active^1];
    for(unsigned j=0;j<n;j++){
        take_rec.sum+=(int16_t)p[j]+(int16_t)(p[j]>>16);
        if(++take_rec.phase<9)continue;
        int x=take_rec.sum/(18*256);
        take_rec.phase=0;take_rec.sum=0;
        if(x>take_rec.peak)take_rec.peak=x;
        if(-x>take_rec.peak)take_rec.peak=-x;
        if(!take_rec.frames && take_rec.peak<2)continue;
        unsigned i=take_rec.frames,at=i%32;
        uint8_t *b=t->block[i/32];
        if(!at){b[0]=(uint8_t)take_rec.predict;b[1]=(uint8_t)take_rec.step;}
        int d=x-take_rec.predict;
        unsigned code=(d<0?2u:0u) | ((d<0?-d:d)>2*take_rec.step);
        if(!(at%4))b[2+at/4]=0;
        b[2+at/4]|=code<<(2*(at%4));
        amb_take_delta(code,&take_rec.predict,&take_rec.step);
        if(++take_rec.frames==AMB_TAKE_FRAMES){take_rec.running=0;break;}
    }
}
static void amb_take_tune(amb_take_t *t)
{
    int16_t window[256];
    unsigned best=0,peak=0,blocks=t->length/32;
    t->root=60;t->period_q8=0;
    if(blocks<8)return;
    /* Prefer a voiced window away from the onset and final release. */
    for(unsigned b=1;b+8<=blocks;b+=4){
        int16_t part[32];unsigned power=0;amb_take_decode(t->block[b+3],part);
        for(unsigned i=0;i<32;i++)power+=(unsigned)(part[i]*part[i]>>8);
        if(power>peak){peak=power;best=b;}
    }
    for(unsigned b=0;b<8;b++)amb_take_decode(t->block[best+b],window+32*b);
    unsigned energy=0,error[78];
    for(unsigned i=0;i<176;i++){int x=window[i]>>8;energy+=x*x;}
    for(unsigned lag=4;lag<78;lag++){
        unsigned sum=0;
        for(unsigned i=0;i<176;i++){int d=(window[i]-window[i+lag])>>8;sum+=d*d;}
        error[lag]=sum;
    }
    for(unsigned lag=5;lag<77;lag++)if(error[lag]<energy/5 && error[lag]<error[lag-1] && error[lag]<=error[lag+1]){
        int a=error[lag-1],b=error[lag],c=error[lag+1];
        t->period_q8=(uint16_t)(256*lag+128*(a-c)/(a+c-2*b));
        uint32_t target=(uint32_t)(((uint64_t)4900<<32)/FS*256/t->period_q8);
        unsigned key=24;
        while(key<96 && pitch_inc(key*16+8)<target)key++;
        t->root=(uint8_t)key;break;
    }
}
static int amb_take_finish(void)
{
    take_rec.running=0;RING_PUBLISH();
    if(take_rec.frames<256 || take_rec.peak<2)return 0;
    amb_take_t *t=&take_banks[take_active^1];
    t->length=(uint16_t)take_rec.frames;t->format=1;
    /* Exclude audio from an earlier, longer take from exported songs. */
    unsigned bytes=(t->length+31u)/32u*10u;
    memset((uint8_t *)t->block+bytes,0,sizeof t->block-bytes);
    unsigned partial=t->length%32u;
    if(partial){
        uint8_t *codes=t->block[t->length/32]+2;
        if(partial%4)codes[partial/4]&=(1u<<(2*(partial%4)))-1;
        unsigned used=(partial+3u)/4;
        memset(codes+used,0,8-used);
    }
    amb_take_tune(t);RING_PUBLISH();take_active^=1;
    return 1;
}
static int amb_take_valid(const amb_take_t *t)
{
    if(t->format!=1 || t->length<256 || t->length>AMB_TAKE_FRAMES || t->root<24 || t->root>96 ||
       (t->period_q8 && (t->period_q8<4*256 || t->period_q8>78*256)))return 0;
    for(unsigned i=0;i<(t->length+31u)/32;i++)
        if(!t->block[i][1] || t->block[i][1]>64)return 0;
    return 1;
}
