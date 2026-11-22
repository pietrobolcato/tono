/* SPDX-License-Identifier: GPL-3.0-only. Tono track adapter for Jake Handy's
 * MIT-licensed A22 instruments. Device builds link the FPU object separately. */
#ifndef TONO_A22_EXTERNAL_DSP
#include "a22/a22_dsp.c"
#endif

static void amb_a22_on(track_t *t,voice_t *v)
{
    unsigned i=(unsigned)(v-t->v);
    if(i>=A22_POLY)return;
    v->s[0]=0;
    tono_a22_note(&voice_bank[t-trk].a22.voices[i],(int)i,t->p[P_E0],v->note,v->vel,
                  (uint32_t)(uint16_t)v->mrnd*65537u);
}
static void amb_a22_block(track_t *t)
{
    A22Part *a=&voice_bank[t-trk].a22;
    memset(a->side,0,sizeof a->side);
    tono_a22_block(&a->track,&t->p[P_E0],&t->p[P_ATK],FS);
}
static int amb_a22_done(track_t *t,voice_t *v)
{
    return !amb_tail_releasing((unsigned)(t-trk)) && v->s[0];
}
/* The target's size-optimized compiler emits a hardware divide for /2.
 * Match C's truncation toward zero when converting left/right to mid/side. */
static inline int32_t amb_a22_half(int32_t sample)
{
    return (sample + (int32_t)((uint32_t)sample >> 31)) >> 1;
}
static void amb_a22_render(track_t *t,voice_t *v,int32_t *out,uint32_t n,const vmod_t *m)
{
    A22Part *a=&voice_bank[t-trk].a22;
    unsigned i=(unsigned)(v-t->v);
    if(i>=A22_POLY || n>A22_CTRL)return;
    int l[A22_CTRL],r[A22_CTRL];
    /* The Tono handoff supplies its own tail envelope, holding the current
     * body level for sustained sounds while the old track fades away. */
    v->s[0]=tono_a22_render(&a->track,&a->voices[i],m->pitch16,m->fine,
                            v->gate || amb_tail_releasing((unsigned)(t-trk)),l,r,(int)n);
    for(i=0;i<n;i++){
        int mid=amb_a22_half(l[i]+r[i]),side=amb_a22_half(l[i]-r[i]);
        int gain=amp_at(m,i);
        out[i]+=mulq15(mulq15(mid,gain),VOICE_FS);
        a->side[i]+=mulq15(mulq15(side,gain),VOICE_FS);
    }
}
#define A22_E(name,id,a,b,c,d,tag) {name,{id,a,b,c,d,0,0,0},{0,50,72,36},0,TAG(tag),FX(0,0,0,0),0,0}
#define A22_A(name,id,a,b,c,d,tag) {name,{id,a,b,c,d,0,0,0},{0,48,70,32},0,TAG(tag),FX(0,0,0,0),0,0}
static const preset_t A22_PATCH[]={
    A22_E("Rhodes",0,42,48,30,22,KEYS), A22_E("Wurli",1,64,34,38,32,KEYS),
    A22_E("DX EP",2,48,72,26,12,KEYS), A22_E("Clav",3,72,62,10,18,KEYS),
    A22_E("Organ",4,52,56,62,26,KEYS), A22_E("Felt",5,26,32,12,48,KEYS),
    A22_E("Vibes",6,28,64,54,24,BELL), A22_E("Tape",7,46,36,28,62,KEYS),
    A22_A("Dual",8,35,28,62,26,LEAD), A22_A("TZFM",9,44,56,68,32,LEAD),
    A22_A("Super",10,28,64,78,22,PAD), A22_A("Sub",11,48,42,52,34,BASS),
    A22_A("Sync",12,58,34,66,46,LEAD), A22_A("Acid",13,32,24,42,74,BASS),
    A22_A("Brass",14,26,36,58,28,PAD), A22_A("Chord",15,38,46,66,26,PAD)
};
#undef A22_E
#undef A22_A
static const engine_t ENG_AMB_A22={
    .name="A22",.page_title={"TONE","MIX"},
    .edit={{"MODEL",F_INT,0,15,0,0,0},{"BARK",F_INT,0,100,42,0,0},
           {"TINE",F_INT,0,100,48,0,0},{"TREM",F_INT,0,100,30,0,0},
           {"WARMTH",F_INT,0,100,22,0,0},{"",F_INT,0,0,0,0,0},
           {"",F_INT,0,0,0,0,0},{"",F_INT,0,0,0,0,0}},
    .presets=A22_PATCH,.npresets=16,.poly=A22_POLY,.knob={P_E1,P_E2,P_E3,P_E4},
    .note_on=amb_a22_on,.render=amb_a22_render,.block=amb_a22_block,.ownenv=1,.done=amb_a22_done
};
