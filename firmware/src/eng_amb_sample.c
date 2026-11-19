/* SPDX-License-Identifier: GPL-3.0-only */
/* Bounded read-only PCM playback; sources are original offline examples.
 * Uses voice-local existing phase/filter state, no sample RAM or flash writes. */
#include "amb_samples.h"
#include "amb_acoustic_pcm.h"
static const char *const N_AS_SOURCE[]={"SOFT CHIME","TAPE VOWEL","SOLO VIOLIN","BOWED CELLO","KAWAI GRAND","TENOR SAX","CONCERT FLUTE"};
typedef struct {const int16_t *pcm;unsigned length,root,rate_q16,loop_percent;} amb_recording_t;
static const amb_recording_t amb_recordings[]={
    {AMB_SAMPLES[0],AMB_SAMPLE_N,60,16384,0},{AMB_SAMPLES[1],AMB_SAMPLE_N,60,16384,0},
    {amb_pcm_violin,NELEM(amb_pcm_violin),60,23777,50},
    {0,NELEM(amb_pcm_cello),60,17833,12},
    {amb_pcm_piano,NELEM(amb_pcm_piano),72,23777,80},
    {amb_pcm_tenor,NELEM(amb_pcm_tenor),72,23777,48},
    {amb_pcm_flute,NELEM(amb_pcm_flute),72,23777,48}
};
/* Cello uses companded storage to fit a longer bow in the firmware slot. */
static inline int32_t amb_pcm_at(const int16_t *pcm,unsigned at)
{
    return pcm ? pcm[at] : amb_cello_decode[amb_pcm_cello[at]];
}
static const char *const N_AS_STYLE[4]={"PAD","DRONE","TEXTURE","ONE-SHOT"};
static const char *const N_AS_LOOP[2]={"OFF","ON"};
static void amb_sample_on(track_t *t,voice_t *v)
{
    v->ph[0]=(uint32_t)clamp(t->p[P_E4],0,99)*amb_recordings[clamp(t->p[P_E0],0,NELEM(amb_recordings)-1)].length/100u<<16;
    v->s[0]=0;v->s[1]=0;
}
static void amb_sample_render(track_t *t,voice_t *v,int32_t *out,uint32_t n,const vmod_t *m)
{
    const amb_recording_t *recording=&amb_recordings[clamp(t->p[P_E0],0,NELEM(amb_recordings)-1)];
    const int16_t *src=recording->pcm;
    unsigned start=(unsigned)clamp(t->p[P_E4],0,99)*recording->length/100u;
    unsigned end=(unsigned)clamp(t->p[P_E5],1,100)*recording->length/100u;
    unsigned ph=v->ph[0],i,style=(unsigned)t->p[P_E1];
    unsigned length,fade,inc;int pitch=clamp(m->pitch16+t->p[P_E2]*16,24*16,96*16);
    if(end<=start+64) end=start+64;
    if(end>recording->length) {end=recording->length;start=end-64;}
    length=end-start;
    fade=(unsigned)(t->p[P_E6]+t->p[P_LN6])*length/600u+8u;
    if(fade>length/3)fade=length/3;
    unsigned loop=start+(length*recording->loop_percent)/100;
    if(fade>(end-loop)/3)fade=(end-loop)/3;
    inc=((pitch_inc((unsigned)pitch)>>12)*recording->rate_q16)/(pitch_inc(recording->root*16)>>12);
    if(style==2)inc=inc*(990u+(unsigned)((t->lfo_ph>>24)&31))/1000u;
    for(i=0;i<n;i++) {
        unsigned at=ph>>16,frac=(ph>>1)&32767u;int32_t x,b;
        if(at<start || at>=end) {
            if(style==3 || !t->p[P_E3]) {v->active=0;break;}
            unsigned resume=loop+(recording->loop_percent?fade:0);
            ph=(resume<<16)+(ph-(end<<16))%((end-resume-(recording->loop_percent?0:fade))<<16);at=ph>>16;
        }
        x=amb_pcm_at(src,at);b=amb_pcm_at(src,at+1<end?at+1:start);x+=((b-x)*(int32_t)frac)>>15;
        if(at>=end-fade && style!=3 && t->p[P_E3]) {
            unsigned d=at-(end-fade),other=loop+d;
            x=(x*(int32_t)(fade-d)+amb_pcm_at(src,other)*(int32_t)d)/(int32_t)fade;
        }
        if(at-start<8u)x=x*(int32_t)(at-start)/8;
        if((style==3 || !t->p[P_E3]) && end-at<fade)x=x*(int32_t)(end-at)/(int32_t)fade;
        v->s[0]+=(x-v->s[0])*(t->p[P_E7]+8)/160;
        x=v->s[0]*t->p[P_LN7]/100;
        out[i]+=voice_amp(x,m,i);
        ph+=inc;
    }
    v->ph[0]=ph;
    if(!m->amp0 && !m->amp1 && !v->gate)v->active=0;
}
static const preset_t AMB_SAMPLE_PRESETS[]={
 {"SOFT CHIME",{0,0,0,1,0,100,40,85},{65,90,100,105},0,TAG(PAD),FX(0,25,35,80),0,0},
 {"VIOLIN",{2,0,0,1,0,100,24,110},{22,64,115,65},0,TAG(PAD),FX(0,6,8,38),0,0},
 {"CELLO",{3,0,0,1,0,100,24,108},{24,65,115,74},0,TAG(PAD),FX(0,4,8,36),0,0},
 {"GRAND",{4,0,0,1,0,100,16,122},{0,95,14,72},0,TAG(PAD),FX(0,0,6,35),0,0},
 {"TENOR",{5,0,0,1,0,100,30,114},{8,68,108,49},0,TAG(PAD),FX(0,0,12,32),0,0},
 {"FLUTE",{6,0,0,1,0,100,28,120},{15,62,110,61},0,TAG(PAD),FX(0,0,16,42),0,0}
};
static const engine_t ENG_AMB_SAMPLE={
 .name="SAMPLE",.page_title={"SOURCE","PLAYBACK"},
 .edit={{"SOURCE",F_ENUM,0,NELEM(N_AS_SOURCE)-1,0,N_AS_SOURCE,0},{"STYLE",F_ENUM,0,3,0,N_AS_STYLE,0},
        {"PITCH",F_SEMI,-24,24,0,0,0},{"LOOP",F_ONOFF,0,1,1,N_AS_LOOP,0},
        {"START",F_PCT,0,99,0,0,0},{"END",F_PCT,1,100,100,0,0},
        {"XFADE",F_PCT,0,100,40,0,0},{"COLOR",F_PCT,0,127,85,0,0}},
 .presets=AMB_SAMPLE_PRESETS,.npresets=NELEM(AMB_SAMPLE_PRESETS),.knob={P_E2,P_E6,P_LD_PIT,P_REV},
 .poly=2,.sampled=1,.note_on=amb_sample_on,.render=amb_sample_render
};
