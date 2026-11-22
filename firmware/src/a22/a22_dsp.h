/* SPDX-License-Identifier: MIT. Tono bridge to A22 a93a58fb. */
#pragma once
#include <stdint.h>
enum { A22_CTRL = 32, A22_POLY = 6 };
typedef struct {
    float ph[6], s[12];
    uint32_t u[1];
    float env, menv, age, r1, pitch, vel_f, gain_l, gain_r;
    int idx, stage;
} a22_Voice;
typedef struct {
    float sr, isr, time, step, bend, k[4], menv_amt, vel_sens, punch;
    int algo;
} a22_Blk;
typedef struct {
    a22_Blk b;
    union {
        struct {int algo; float bark,tine,trem,warmth,trem_l,trem_r,rot_ph,wow,kd,lpk;} electric;
        struct {int algo; float shape,tzfm,cut,reso,drive,pwm_lfo,detune[4],comp;} analog;
    };
    float trem_ph,wow_ph,flut_ph,elec_t_was,pwm_ph,pulse_t_was;
    float att,kd,sus,kr,mkd;
    int model,env_rate;
    int16_t env_values[4],knob_values[4];
} A22Track;
typedef struct {
    a22_Voice voices[A22_POLY];
    A22Track track;
    int side[A22_CTRL];
} A22Part;
void tono_a22_init(void);
void tono_a22_block(A22Track *, const int16_t *engine, const int16_t *adsr,int rate);
void tono_a22_note(a22_Voice *, int index, int model, int note, int velocity, uint32_t random);
int tono_a22_render(A22Track *, a22_Voice *, int pitch16, int fine, int gate,
                    int *left,int *right,int count);
