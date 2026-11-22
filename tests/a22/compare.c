/* SPDX-License-Identifier: GPL-3.0-only. Reference runs the actual upstream
 * voice functions and reconstructed per-voice section of synth.c::block.
 * The Tono implementation runs separately through its integer-only ABI. */
#include <stdio.h>
#include <stdlib.h>
#include "a22_dsp.h"
#include "dsp.h"
#include "types.h"
#define CTRL 32
#define NALGO 8
#include "electric.inc"
#include "analog.inc"
#include "knobs.h"
f32 sin_tab[SIN_N+1];
static const float trims[]={1.25f,1.22f,1.55f,1.65f,1.72f,1.25f,1.12f,1.32f,
                            1.35f,1.08f,1.85f,1.32f,1.35f,2.05f,1.38f,1.72f};

static int compare(int mode,int rate,int note,int velocity)
{
    unsigned long mismatch=0;
    int peak_error=0;
    int analog=mode>=8;
    const uint8_t *knobs=ALGO_K0[analog?3:1][mode&7];
    int16_t params[]={mode,knobs[0],knobs[1],knobs[2],knobs[3]};
    int16_t adsr[]={0,analog?48:50,analog?70:72,analog?32:36};
    Blk b={0};
    Voice v={0};
    A22Track track={0};
    a22_Voice voice={0};
    b.sr=(float)rate;
    b.isr=1.f/b.sr;
    b.step=32.f*b.isr;
    b.algo=mode&7;
    b.vel_sens=(analog?65.f:75.f)*.01f;
    b.menv_amt=(analog?62.f:55.f)*.01f;
    b.punch=(analog?18.f:15.f)*.01f;
    float control=b.sr/32.f;
    float kd=onepole_k(.025f*exp2fx(adsr[1]*.075f),control);
    float kr=onepole_k(.015f*exp2fx(adsr[3]*.088f),control);
    float mkd=onepole_k(.014f*exp2fx(34.f*.078f),control);
    v.pitch=(float)note; v.vel_f=(float)velocity*(1.f/127.f);
    v.idx=2; v.r1=.4375f; v.menv=1.f;
    float gl,gr;
    pan_gains(clampf(((float)note-64.f)*(1.f/44.f),-.38f,.38f),&gl,&gr);
    float gain=(analog?.42f:.40f)*trims[mode]*(.22f+.78f*v.vel_f*v.vel_f);
    gl*=gain;gr*=gain;
    trem_ph=wow_ph=flut_ph=elec_t_was=pwm_ph=pulse_t_was=0.f;
    if(analog)pulse_note(&v,&b);else elec_note(&v,&b);
    tono_a22_note(&voice,2,mode,note,velocity,0x70000000);
    for(int block=0;block<2500;block++){
        float l[CTRL],r[CTRL],e0=v.env;
        int left[CTRL],right[CTRL];
        if(block==240) params[2]=(params[2]+19)%101;
        for(int k=0;k<4;k++)b.k[k]=params[k+1]*.01f;
        b.time+=b.step;
        if(analog)pulse_block(&b);else elec_block(&b);
        if(block==0)v.env=1.f;
        else if(block<1654)v.env+=(adsr[2]*.01f-v.env)*kd;
        else {v.env-=v.env*kr+.00002f;if(v.env<=.0003f)v.env=0.f;}
        v.menv-=v.menv*mkd;if(v.menv<.0002f)v.menv=0.f;
        if(analog)pulse_render(&v,&b,l,r,CTRL);else elec_render(&v,&b,l,r,CTRL);
        v.age+=b.step;
        tono_a22_block(&track,params,adsr,rate);
        tono_a22_render(&track,&voice,note*16,0,block<1654,left,right,32);
        float de=(v.env-e0)*(1.f/32.f);
        for(int i=0;i<32;i++,e0+=de){
            int dl=abs(left[i]-(int)(l[i]*e0*gl*32768.f));
            int dr=abs(right[i]-(int)(r[i]*e0*gr*32768.f));
            mismatch+=(dl!=0)+(dr!=0);
            if(dl>peak_error)peak_error=dl;if(dr>peak_error)peak_error=dr;
        }
    }
    printf("mode %2d  %d Hz  note %2d velocity %3d  %lu/160000 differ  max %d LSB\n",
           mode,rate,note,velocity,mismatch,peak_error);
    return mismatch!=0;
}
int main(void)
{
    tono_a22_init();
    int difference=0;
    for(int i=0;i<=SIN_N;i++)sin_tab[i]=sin_turns((float)i/(float)SIN_N);
    for(int mode=0;mode<16;mode++){
        difference|=compare(mode,44100,48,80);
        difference|=compare(mode,44118,75,115);
    }
    return difference;
}
