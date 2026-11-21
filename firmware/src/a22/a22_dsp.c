/* SPDX-License-Identifier: MIT. A22 Electric and Analog voice engines,
 * adapted from https://github.com/abektes/a22 at a93a58fb17f636dbdc5b8f866009946301958e98.
 * The original signal paths and preset gains run at Tono's actual sample rate.
 * All mutable state is per part or per voice, including the tremolo clocks. */
#include "a22_dsp.h"
#include "a22_filters.h"
#define a22_NALGO 8
#define a22_CTRL A22_CTRL
#include "a22_electric.inc"
#include "a22_analog.inc"

void tono_a22_block(A22Track *t, const int16_t *engine, const int16_t *adsr,int rate)
{
    a22_Blk *b=&t->b;
    int model=a22_iclamp(engine[0],0,15);
    b->algo=model&7;
    t->model=model;
    b->sr=(float)rate;
    b->isr=1.f/b->sr;
    b->step=A22_CTRL*b->isr;
    b->time+=b->step;
    for(int i=0;i<4;i++) b->k[i]=(float)a22_iclamp(engine[i+1],0,100)*.01f;
    b->menv_amt=(model<8?55.f:62.f)*.01f;
    b->vel_sens=(model<8?75.f:65.f)*.01f;
    b->punch=(model<8?15.f:18.f)*.01f;
    const float control=b->sr/(float)A22_CTRL;
    t->att=adsr[0] ? a22_minf(1.f,b->step/(.001f+.002f*a22_exp2fx(adsr[0]*.095f))) : 1.f;
    t->kd=a22_onepole_k(.025f*a22_exp2fx(adsr[1]*.075f),control);
    t->sus=a22_minf((float)adsr[2]*.01f,1.f);
    t->kr=a22_onepole_k(.015f*a22_exp2fx(adsr[3]*.088f),control);
    t->mkd=a22_onepole_k(.014f*a22_exp2fx(34.f*.078f),control);
    if(model<8) elec_block(t,b); else pulse_block(t,b);
}

void tono_a22_note(a22_Voice *v,int index,int model,int note,int velocity,uint32_t random)
{
    static const float trim[16]={1.25f,1.22f,1.55f,1.65f,1.72f,1.25f,1.12f,1.32f,
                                  1.35f,1.08f,1.85f,1.32f,1.35f,2.05f,1.38f,1.72f};
    v->age=v->env=0.f;
    v->idx=index;
    v->u[0]=0;
    v->stage=1;
    v->menv=1.f;
    v->r1=(float)(random>>8)*(1.f/16777216.f);
    v->vel_f=(float)velocity*(1.f/127.f);
    v->pitch=(float)note;
    float pan=a22_clampf((note-64.f)*(1.f/44.f),-.38f,.38f);
    float gain=(model<8?.40f:.42f)*trim[model&15]*(.22f+.78f*v->vel_f*v->vel_f);
    a22_pan_gains(pan,&v->gain_l,&v->gain_r);
    v->gain_l*=gain;
    v->gain_r*=gain;
    if(model<8) elec_note(v,0); else pulse_note(v,0);
}

int tono_a22_render(A22Track *t,a22_Voice *v,int pitch16,int fine,int gate,
                    int *left,int *right,int n)
{
    float l[A22_CTRL],r[A22_CTRL],e=v->env;
    if(!gate) v->stage=4;
    if(v->stage==1) {
        v->env+=t->att;
        if(v->env>=1.f){v->env=1.f;v->stage=2;}
    } else if(v->stage==2) v->env+=(t->sus-v->env)*t->kd;
    else {
        v->env-=v->env*t->kr+.00002f;
        if(v->env<=.0003f)v->env=0.f;
    }
    v->menv-=v->menv*t->mkd;
    if(v->menv<.0002f)v->menv=0.f;
    v->pitch=(float)pitch16*(1.f/16.f)+(float)fine*(17.31234f/4096.f);
    if(t->model<8)elec_render(t,v,&t->b,l,r,n);
    else pulse_render(t,v,&t->b,l,r,n);
    float de=(v->env-e)*(1.f/(float)A22_CTRL);
    for(int i=0;i<n;i++,e+=de) {
        left[i]=(int)(l[i]*e*v->gain_l*32768.f);
        right[i]=(int)(r[i]*e*v->gain_r*32768.f);
    }
    v->age+=t->b.step;
    return v->stage==4 && v->env==0.f;
}
