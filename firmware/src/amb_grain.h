/* SPDX-License-Identifier: GPL-3.0-only */
/* Two overlapping Hann-like triangular windows on a quarter-rate mono history.
 * Each grain latches pitch, length and start. No allocation on the audio path. */
typedef struct {
    int16_t history[512];
    unsigned write,valid,phase,period,read[2],step[2],head,seed;
    int wet,previous,mix;
} amb_grain_t;
static amb_grain_t amb_grain[NTRK];
static int8_t amb_grain_controls[NTRK][4];
static const char *const amb_grain_label[]={"GRAIN MIX","LENGTH","PITCH","SCATTER"};
static const char *const amb_grain_hint[]={"Dry and granular blend","Grain window duration","Grain pitch in semitones","Spread grain start times"};
static int amb_grain_read(amb_grain_t *g,unsigned address)
{
    unsigned i=(address>>12)&511,f=address&4095;
    int a=g->history[i],b=g->history[(i+1)&511];
    return a+((b-a)*(int)f>>12);
}
static void amb_grain_process(unsigned k,int32_t *b,unsigned n)
{
    amb_grain_t *g=&amb_grain[k];const int8_t *ctl=amb_grain_controls[k];
    if(!ctl[0] && !g->mix){g->valid=g->phase=g->period=0;return;}
    int difference=ctl[0]-g->mix;
    g->mix+=difference/4?difference/4:difference;
    for(unsigned i=0;i+3<n;i+=4){
        int dry=(b[i]/4+b[i+1]/4+b[i+2]/4+b[i+3]/4);
        g->history[g->write]=clamp(dry,-32768,32767);
        g->write=(g->write+1)&511;
        if(g->valid<768)g->valid++;
        if(!g->phase){
            g->head^=1;
            g->period=32+ctl[1]; /* half-window in quarter-rate samples */
            g->seed=g->seed*1664525u+1013904223u;
            unsigned lag=2*g->period+2+((g->seed>>24)*(unsigned)ctl[3]>>7);
            g->read[g->head]=((g->write-lag)&511)<<12;
            g->step[g->head]=((pitch_inc((60+ctl[2])*16)>>12)*4096u)/(pitch_inc(60*16)>>12);
        }
        int newest=amb_grain_read(g,g->read[g->head]);
        int oldest=amb_grain_read(g,g->read[1-g->head]);
        g->previous=g->wet;
        g->wet=oldest+(newest-oldest)*(int)g->phase/(int)g->period;
        for(unsigned h=0;h<2;h++)g->read[h]+=g->step[h];
        if(++g->phase>=g->period)g->phase=0;
        if(g->valid<512)continue;
        int mix=g->mix*(g->valid-512)/256;
        for(unsigned j=0;j<4;j++){
            int wet=g->previous+(g->wet-g->previous)*(int)(j+1)/4;
            b[i+j]+=(wet-b[i+j])*mix/100;
        }
    }
}
