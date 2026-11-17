/* SPDX-License-Identifier: GPL-3.0-only */
/* Bounded read-only PCM playback; sources are original offline examples.
 * Uses voice-local existing phase/filter state, no sample RAM or flash writes. */
#include "amb_samples.h"
static const char *const N_AS_SOURCE[2]={"SOFT CHIME","TAPE VOWEL"};
static const char *const N_AS_STYLE[4]={"PAD","DRONE","TEXTURE","ONE-SHOT"};
static const char *const N_AS_LOOP[2]={"OFF","ON"};
static void amb_sample_on(track_t *t,voice_t *v)
{
    v->ph[0]=(uint32_t)t->p[P_E4]*AMB_SAMPLE_N/100u<<16;
    v->s[0]=0;v->s[1]=0;
}
static void amb_sample_render(track_t *t,voice_t *v,int32_t *out,uint32_t n,const vmod_t *m)
{
    const int16_t *src=AMB_SAMPLES[t->p[P_E0]&1];
    unsigned start=(unsigned)t->p[P_E4]*AMB_SAMPLE_N/100u;
    unsigned end=(unsigned)t->p[P_E5]*AMB_SAMPLE_N/100u;
    unsigned ph=v->ph[0],i,style=(unsigned)t->p[P_E1];
    unsigned length,fade,inc;int pitch=clamp(m->pitch16+t->p[P_E2]*16,24*16,96*16);
    if(end<=start+64) end=start+64;
    if(end>AMB_SAMPLE_N) {end=AMB_SAMPLE_N;start=end-64;}
    length=end-start;
    fade=(unsigned)(t->p[P_E6]+t->p[P_LN6])*length/600u+8u;
    if(fade>length/3)fade=length/3;
    inc=((pitch_inc((unsigned)pitch)>>12)*16384u)/(pitch_inc(60*16)>>12);
    if(style==2)inc=inc*(990u+(unsigned)((t->lfo_ph>>24)&31))/1000u;
    for(i=0;i<n;i++) {
        unsigned at=ph>>16,frac=(ph>>1)&32767u;int32_t x,b;
        if(at<start || at>=end) {
            if(style==3 || !t->p[P_E3]) {v->active=0;break;}
            ph=(start<<16)+(ph-(end<<16))%((length-fade)<<16);at=ph>>16;
        }
        x=src[at];b=src[at+1<end?at+1:start];x+=((b-x)*(int32_t)frac)>>15;
        if(at>=end-fade && style!=3 && t->p[P_E3]) {
            unsigned d=at-(end-fade),other=start+d;
            x=(x*(int32_t)(fade-d)+src[other]*(int32_t)d)/(int32_t)fade;
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
static const preset_t AMB_SAMPLE_PRESETS[1]={
 {"SOFT CHIME",{0,0,0,1,0,100,40,85},{65,90,100,105},0,TAG(PAD),FX(0,25,35,80),0,0}
};
static const engine_t ENG_AMB_SAMPLE={
 .name="SAMPLE",.page_title={"SOURCE","PLAYBACK"},
 .edit={{"SOURCE",F_ENUM,0,1,0,N_AS_SOURCE,0},{"STYLE",F_ENUM,0,3,0,N_AS_STYLE,0},
        {"PITCH",F_SEMI,-24,24,0,0,0},{"LOOP",F_ONOFF,0,1,1,N_AS_LOOP,0},
        {"START",F_PCT,0,99,0,0,0},{"END",F_PCT,1,100,100,0,0},
        {"XFADE",F_PCT,0,100,40,0,0},{"COLOR",F_PCT,0,127,85,0,0}},
 .presets=AMB_SAMPLE_PRESETS,.npresets=1,.knob={P_E2,P_E6,P_LD_PIT,P_REV},
 .poly=2,.sampled=1,.note_on=amb_sample_on,.render=amb_sample_render
};
