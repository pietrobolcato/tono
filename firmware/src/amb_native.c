/* SPDX-License-Identifier: GPL-3.0-only */
/* AMB+ native instrument. Saved parameter baselines are separate from runtime
 * modulation and scheduling. No extra playable browser implementation. */
#include "amb_controls.h"
#include "amb_policy.h"
#include "amb_harmony.h"
#include "amb_music.h"
#define AMB_MAGIC 0x50424d41u
#define AMB_VERSION 5u
#define AMB_LAYERS 4u
#define AMB_CHANT 5u
#define AMB_SLOTS 8u
#define AMB_MAX_TAIL_MS 30000
#define AMB_SAMPLE_ROLE 6u

typedef struct {
    uint8_t role, patch, engine, generated;
    uint8_t macro[4], density, swing, fill, mutation;
    uint8_t follow, mode, speed, level;
    uint8_t lfo_dest, lfo_depth, solo_plus1, reserved; /* layer 0 carries piece solo (0=none, 1..4) */
    uint8_t notes[16], velocity[16];
    uint16_t drums[4];
    int16_t base[P_COUNT];
} amb_layer_t;
typedef struct {
    uint32_t magic, version;
    amb_policy_t policy;
    uint16_t master;
    uint8_t key, scale, chord, auto_index, evolve[4];
    int8_t eq[4];
    int16_t global[G_COUNT];
    amb_layer_t layer[4];
    int8_t layer_eq[4][4];
    uint16_t release_ms; /* v4 append-only; v1..3 retain their 1400-byte prefix */
    uint8_t master_anchor[4]; /* HOME primary values, encoded relative to GP.min */
    int8_t grain[4][4]; /* mix, window, pitch and scatter */
} amb_piece_t;
_Static_assert(sizeof(amb_piece_t) <= ST_PAYLOAD_MAX, "whole AMB+ piece fits existing A/B object");
static const uint8_t amb_master_macros[4]={G_RSIZE,G_DMIX,G_CDEPTH,G_DJF};
static void amb_anchor_master(amb_piece_t *p)
{
    for(unsigned i=0;i<4;i++){
        unsigned id=amb_master_macros[i];
        p->master_anchor[i]=(uint8_t)(p->global[id]-GP[id].min);
    }
}
static amb_piece_t amb;
#include "amb_grain.h"
static int8_t amb_reset_grain[4][4];
static int8_t amb_reset_eq[4][4];
static amb_layer_t amb_reset_layer[4]; /* last loaded piece, never a flash snapshot */
/* Main-loop edits publish a revision; audio copies a full deep baseline only
 * when it changes. Macro destinations are restored explicitly every block. */
static volatile uint32_t amb_base_revision[4];
typedef struct { uint8_t pending, quiet_steps; } amb_grid_cue_t;
static struct {
    uint32_t phase, step, off[4], due[4], due_step[4], play_t, last_draw, notice_ms;
    uint8_t due_mask[4];
    amb_grid_cue_t grid_cue[4];
    /* Single-writer counters. One Master tap reaches all layers at the same
     * boundary even if the audio interrupt lands during an input frame. */
    volatile uint8_t octave_taps[5];
    uint8_t octave_seen[5];
    uint32_t factory_take;
    uint32_t applied_revision[4];
    int master_offset[4];
    uint8_t last_routed_param[4]; /* parameter ID plus one */
    uint8_t handoff[NTRK],overlap[4],fade_slot[4],playhead[4],playhead_valid[4];
    uint32_t tail_left[NTRK],tail_total[NTRK];
    int32_t tail_env[NTRK][NVOICE],tail_gain[NTRK];
    uint8_t piece_transition;uint16_t piece_master;int8_t piece_eq[4],tail_eq[NTRK][4];
    uint32_t level_feedback_ms;uint8_t level_feedback_target,level_feedback_active;
    amb_walk_t harmony_walk;
    int32_t evolve_smooth[4];
    uint8_t recent[4][16], recent_vel[4][16], recent_valid[4], reset_pending, detail, foundation_pending[4];
    uint8_t zen,zen_train;
    uint8_t help, local_preset, preset_position;
    uint8_t loading, last_play, target, edit_layer, view, page, bank, fx, lane, cursor, recording;
    uint8_t proposal, chord_q, piece_preview, slot, overwrite, dirty, panic, play_used, home_used;
    uint8_t gen_candidate[3], gen_edit[3];
    uint32_t gen_time[3];
    uint8_t held_n[4], held[4][4];
    int voicing[4];
    int8_t solo;
    int16_t macro_smooth[4][4];
    int32_t master_smooth;
    amb_pickup_t pickup;uint8_t pickup_target;
    char notice[38];
} ambr;
static const uint16_t AMB_SCALE[4] = {0x5adu,0xab5u,0x6adu,0xad5u}; /* minor/major/dorian/lydian */
static const uint8_t AMB_AUTO[8] = {0,1,2,4,8,16,32,64};
static const char *const AMB_ROLE[7] = {"PAD","DRONE","BELLS","BASS","DRUMS","TEXTURE","SAMPLE"};
static const char *const AMB_EDIT[7] = {"MODE","SOUND","ENVELOPE","LFO","PATTERN","FX","LAYER EQ"};
static const char *const AMB_GLO[] = {"EVOLVE","HARMONY","GENERATE","MASTER FX","EQ","THEME","TRAVEL","TRAVEL / TIMING"};
/* Timing is a detail of Travel. Both share one position in the SCL cycle. */
static const uint8_t amb_global_order[]={0,6,1,2,3,4,5};
static unsigned amb_global_index(unsigned page)
{
    if(page>=6)return 1;
    return page?page+1:0;
}
static const char *const AMB_MOOD[7] = {"homeward","warmer","brighter","suspended","deeper","open","darker"};
#include "amb_nostalgic.h"
_Static_assert(sizeof(amb_piece_t)==1440 && AMB_VERSION==5, "upgrade embedded song before changing format");
enum { AMB_SCENE_GARDEN = 11, AMB_SCENE_INN = 12 };
static const char *const AMB_SCENE[] = {"Morning Air","Rain Study","Dark Water","Low Sun","Glass Room","Night Garden","Window Seats","Stone Cloister","Tidal Loom","Kite Strings","Departure Hall","Garden","Nostalgic New Inn","Pear Orchard","Tide Atlas","Pocket Orrery","Last Tram"};
static const char *const AMB_MACRO[7][4] = {
    {"WARMTH","MOTION","SUSTAIN","SPACE"},{"WARMTH","DRIFT","SUSTAIN","SPACE"},
    {"SOFT","MOTION","SHIMMER","SPACE"},{"ROUND","PULSE","GLIDE","SPACE"},
    {"SOFT","DENSITY","DUST","SPACE"},{"BLUR","MOTION","GRAIN","SPACE"},
    {"PITCH","BLUR","MOTION","SPACE"}};
#include "amb_palette.h"
static void amb_say(const char *s) { str_cpy(ambr.notice,s,sizeof ambr.notice); ambr.dirty = 1; ambr.notice_ms=fm1_ms; }
static unsigned amb_wrap(int n, unsigned count) { n %= (int)count; return (unsigned)(n < 0 ? n + (int)count : n); }
/* Two formerly reserved bytes store the shared scale pool without changing
 * the piece size/sector layout. Zero means the chosen stock scale (v1/v2). */
static uint16_t amb_pool(void)
{
    unsigned m=amb.layer[0].reserved | ((unsigned)amb.layer[1].reserved<<8);
    return (uint16_t)(m?m:AMB_SCALE[amb.scale]);
}
static void amb_pool_set(unsigned pool)
{
    pool=(pool|1u)&0xfffu;amb.layer[0].reserved=(uint8_t)pool;
    amb.layer[1].reserved=(uint8_t)(pool>>8);
}
/* All harmony edits use the same natural note/release path as AUTO. */
static void amb_revoice(void)
{
    int pcs[4],old[4];unsigned i;uint8_t busy=ambr.loading;
    ambr.loading=1;
    for(i=0;i<4;i++)old[i]=ambr.voicing[i]?ambr.voicing[i]:48+(int)i*7;
    amb_voice_lead(old,pcs,amb_chord_tones(amb_pool(),amb.key,amb.chord,pcs),ambr.voicing,48,77);
    ambr.loading=busy;
}
static void amb_fill_patch(amb_layer_t *l,unsigned key,unsigned k,unsigned role,unsigned patch)
{
    unsigned i;patch%=amb_patch_count(role);
    const amb_recipe_t *r=amb_recipe(role,patch);
    const engine_t *e=ENGINES[role==6?ENGI_AMB_SAMPLE:r->engine];
    const preset_t *pr=&e->presets[role==6?0:r->preset];
    l->role=(uint8_t)role;l->patch=(uint8_t)patch;l->engine=(uint8_t)(role==6?ENGI_AMB_SAMPLE:r->engine);
    for(i=0;i<P_E0;i++)l->base[i]=TP[i].def;
    for(i=0;i<8;i++)l->base[P_E0+i]=pr->e[i];
    l->base[P_ATK]=r->attack;l->base[P_DEC]=pr->env[1];l->base[P_SUS]=pr->env[2];l->base[P_REL]=r->release;
    l->base[P_ED_FLT]=pr->fenv;l->base[P_VOICE]=PR_MONO(pr)?V_LEGATO:V_POLY;
    l->base[P_LEVEL]=r->level;l->base[P_REV]=r->reverb;l->base[P_DLY]=r->delay;
    l->base[P_CHOR]=(int16_t)(pr->fx[1]?pr->fx[1]-1:24);
    l->base[P_ROOT]=(int16_t)key;l->base[P_SCALE]=2;l->base[P_QUANT]=0;
    l->base[P_AMODE]=l->base[P_AHOLD]=0;
    l->base[P_LRATE]=12+(int)(patch%4)*3;l->base[P_LD_FLT]=6;l->base[P_LPHASE]=-1;
    l->base[P_DIST]=role==4?4:0;l->base[P_PAN]=(int)k*22-33;
    if(role==0 || role==1 || role==5){l->base[P_SUS]=118;l->base[P_REL]=116;l->density=(uint8_t)(role==5?100:84);}
    if(role==5)l->mode=0;
    if(role!=0 && l->mode==AMB_CHANT)l->mode=2;
    l->base[P_FCUT]=clamp(r->color+25,0,127);l->base[P_FRES]=12;
    if(r->engine==0 && role!=6)l->base[P_E4]=r->color;
    if(role==0 && patch>=13 && patch<=15){
        /* The stock brass, triangle lead and organ provide the oscillator
         * shapes. Voice them as held chords with restrained beating and drive. */
        static const struct {uint8_t cents,mix,res,drive,chorus;int8_t interval,filter_env;} warm[]={
            {5,54,21,22,17,0,16}, {3,32,13,28,8,-12,6}, {4,28,24,13,19,0,9}
        };
        unsigned n=patch-13;
        l->base[P_VOICE]=V_POLY;l->base[P_E1]=warm[n].cents;l->base[P_E2]=warm[n].mix;
        l->base[P_E5]=warm[n].res;l->base[P_E6]=warm[n].drive;l->base[P_E7]=warm[n].interval;
        l->base[P_ED_FLT]=warm[n].filter_env;l->base[P_CHOR]=warm[n].chorus;
    }
    if(role==1 || role==3)l->base[P_TRANS]=-12;
    if(role==3){l->base[P_VOICE]=V_MONO;l->base[P_GLIDE]=25+patch*5;}
    if(l->engine==ENGI_SINEFM){l->base[P_CHOR]=0;l->base[P_DRIFT]=127;}
    if(l->engine==ENGI_SOFTPULSE){l->base[P_DIST]=l->base[P_CHOR]=0;}
    else if(role==4){l->base[P_E2]=r->color;l->base[P_E3]=38+patch*4;l->base[P_E7]=0;}
    l->lfo_dest=0;l->lfo_depth=6;
    if(l->engine==ENGI_AMB_SAMPLE){l->base[P_LN7]=100;l->base[P_LN6]=20;}
    if(role==5 && patch>=8 && patch<12){
        static const uint8_t texture[][5]={{27,42,29,6,107},{76,94,48,92,24},{80,64,32,19,11},{48,84,89,0,86}};
        const uint8_t *v=texture[patch-8];
        l->base[P_E1]=v[0];l->base[P_E2]=v[1];l->base[P_E3]=v[2];l->base[P_E4]=0;l->base[P_E5]=v[3];l->base[P_E6]=v[4];
    }
    if(role==5 && patch>=22){
        l->base[P_E0]=13+(patch-22);l->base[P_E1]=4;l->base[P_E6]=4;
    }else if(role==5 && patch>=12){
        static const int8_t texture[][3]={
            {0,-2,39},{1,0,46},{2,-5,25},{3,3,52},{4,-4,26},
            {5,-3,55},{0,-14,65},{1,8,58},{2,-13,36},{4,-16,60}
        };
        const int8_t *bed=texture[patch-12];
        l->base[P_E0]=AMB_TEXTURE_SOURCE+bed[0];l->base[P_E1]=2;l->base[P_E2]=bed[1];
        l->base[P_E6]=bed[2];l->base[P_E7]=r->color;
    }
    if(role==6){l->base[P_E4]=0;l->base[P_E5]=100;l->base[P_E6]=40;l->base[P_E7]=85;l->base[P_LN7]=100;l->base[P_LN6]=20;}
    for(i=0;i<4;i++)l->macro[i]=64;
}
/* Keep existing v1-v4 saved PHASE texture edits literal; only fresh factory
 * selections use the revised unpitched recipes. Their names stay truthful. */
static int amb_legacy_texture(const amb_layer_t *l)
{return l->role==5 && l->engine==2 && (l->patch==2 || l->patch==4);}
static int amb_legacy_glass(const amb_layer_t *l)
{return l->role==0 && l->patch==3 && l->engine==ENGI_FM6;}
static const char *amb_patch_name(const amb_layer_t *l)
{return amb_legacy_texture(l)?(l->patch==2?"Glass Wind":"Distant Voices"):amb_recipe(l->role,l->patch)->name;}
static unsigned amb_patch_preset(const amb_layer_t *l)
{return amb_legacy_glass(l)?12u:amb_legacy_texture(l)?(l->patch==2?6u:8u):l->role<6?amb_recipe(l->role,l->patch)->preset:0;}
static void amb_release(unsigned k);
static int amb_tail_releasing(unsigned k){return k<NTRK && ambr.handoff[k];}
static int amb_tail_any(void){return ambr.handoff[0] || ambr.handoff[1] || ambr.handoff[2] || ambr.handoff[3] || ambr.handoff[4] || ambr.handoff[5];}
static int32_t amb_tail_envelope(unsigned k,unsigned voice)
{return mulq16(ambr.tail_env[k][voice],(uint32_t)ambr.tail_gain[k]<<1);}
static int amb_tail_finished(unsigned k){return !ambr.tail_left[k];}
static unsigned amb_track_active(unsigned k)
{unsigned i,n=0;for(i=0;i<NVOICE;i++)n+=trk[k].v[i].active!=0;return n;}
static void garden_cancel(unsigned k);
static void amb_begin_change(unsigned k)
{
    unsigned i,frames;
    garden_cancel(k);
    ambr.grid_cue[k]=(amb_grid_cue_t){0};
    if(ambr.handoff[k])return;
    if(!amb_track_active(k)){amb_release(k);ambr.due[k]=0;return;}
    /* A replacement uses the selected duration for short and long synth
     * releases. Capture the current envelope, never raise it or add voices.
     * Smooth endpoints and the ordinary per-sample amp ramp avoid a cut.
     * Internal FM envelopes are held while oscillators keep running. */
    frames=((unsigned)amb.release_ms*FS+999u)/1000u;
    ambr.tail_left[k]=ambr.tail_total[k]=((frames+CTL-1u)/CTL)*CTL;
    ambr.tail_gain[k]=32767;memcpy(ambr.tail_eq[k],amb.layer_eq[k],4);
    for(i=0;i<NVOICE;i++)ambr.tail_env[k][i]=trk[k].v[i].env;
    ambr.handoff[k]=1;amb_release(k);trk_all_off(&trk[k]);
    ambr.due[k]=0;ambr.dirty=1;
}
/* Release-only slots 5 and 6 use the existing mixer. They never accept
 * sequencer notes and stay inside the live voice governor. */
static int amb_crossfade(void){return !(favorites.factory[15][13]&1);}
static void amb_sound_change(unsigned k)
{
    amb_begin_change(k);
    ambr.overlap[k]=ambr.handoff[k] && amb_crossfade();
}
static int amb_fading_in(unsigned k){return k<4 && ambr.fade_slot[k];}
static int amb_fade_level(unsigned k,int level)
{
    if(!amb_fading_in(k))return level;
    unsigned slot=ambr.fade_slot[k];
    return mulq16(level,(32768u-ambr.tail_gain[slot])*2u);
}
static __attribute__((noinline)) int amb_move_release(unsigned k)
{
    unsigned dst;
    for(dst=4;dst<NTRK && ambr.handoff[dst];dst++){}
    if(dst==NTRK)return 0;
    track_t *from=&trk[k],*to=&trk[dst];
    /* Everything through the oscillator and LFO state, excluding events. */
    amb_bar_octave[dst]=amb_bar_octave[k];
    memcpy(to,from,(unsigned)((uint8_t*)from->held-(uint8_t*)from));
    to->eng_req=to->engine;
    to->mo_cur=from->mo_cur;to->mo_prev=from->mo_prev;
    to->xf_on=to->xf=to->xin=0;
    to->p[P_LEVEL]=from->p[P_LEVEL];
    if(from->engine==ENGI_FM6){
        memcpy(fm6_note[dst],fm6_note[k],sizeof fm6_note[k]);
        memcpy(fm6_patch[dst],fm6_patch[k],sizeof fm6_patch[k]);
        fm6_pgen[dst]=fm6_pgen[k];fm6_eff[dst]=fm6_eff[k];fm6_lfo[dst]=fm6_lfo[k];
        fm6_slot[dst]=fm6_slot[k];fm6_trim[dst]=fm6_trim[k];fm6_tgen[dst]=fm6_tgen[k];
    }else if(from->engine==7){
        drw_t[dst]=drw_t[k];memcpy(drw_v[dst],drw_v[k],sizeof drw_v[k]);
    }else if(from->engine==ENGI_DRUM){
        memcpy(drum_kit[dst],drum_kit[k],sizeof drum_kit[k]);
    }else if(from->engine==ENGI_VOICE){
        sing[dst]=sing[k];sing_x[dst]=sing_x[k];sing_w[dst]=sing_w[k];lyc[dst]=lyc[k];
        memcpy(lyr_txt[dst],lyr_txt[k],sizeof lyr_txt[k]);lyr_gen[dst]=lyr_gen[k];
    }
    tf_ic1[dst]=tf_ic1[k];tf_ic2[dst]=tf_ic2[k];tf_on[dst]=tf_on[k];tf_ft[dst]=tf_ft[k];
    if(tf_cmb[k]){memcpy(comb_buf[dst],comb_buf[k],sizeof comb_buf[k]);comb_w[dst]=comb_w[k];}
    tf_cmb[dst]=tf_cmb[k];ap_ph[dst]=ap_ph[k];mp_seen[dst]=0;
    ambr.tail_total[dst]=ambr.tail_total[k];ambr.tail_left[dst]=ambr.tail_left[k];
    ambr.tail_gain[dst]=ambr.tail_gain[k];memcpy(ambr.tail_env[dst],ambr.tail_env[k],sizeof ambr.tail_env[k]);
    amb_grain[dst]=amb_grain[k];amb_grain[k].valid=amb_grain[k].phase=amb_grain[k].period=0;
    memcpy(amb_grain_controls[dst],amb_grain_controls[k],4);
    memcpy(ambr.tail_eq[dst],ambr.tail_eq[k],4);
    if(ambr.fade_slot[k]){
        unsigned gain=amb_fade_level(k,65536);
        for(unsigned i=0;i<NVOICE;i++)ambr.tail_env[dst][i]=mulq16(ambr.tail_env[dst][i],gain);
    }
    ambr.handoff[dst]=1;ambr.fade_slot[k]=dst;
    for(unsigned i=0;i<NVOICE;i++)from->v[i].active=0;
    ambr.handoff[k]=ambr.overlap[k]=0;
    return 1;
}
static int amb_piece_waiting(void)
{
    unsigned k;for(k=0;k<4;k++)if(ambr.handoff[k] && (ambr.tail_left[k] || amb_track_active(k)))return 1;
    return 0;
}
static void amb_apply_patch(unsigned k)
{
    amb_layer_t *l=&amb.layer[k];track_t *t=&trk[k];
    load_begin(t,LOAD_SOUND);t->eng_req=l->engine;
    t->preset=(uint8_t)amb_patch_preset(l);
    memcpy(t->p,l->base,sizeof t->p);fm6_track_loaded(t);
    panic_req|=(uint8_t)(1u<<k);load_end(t);ambr.handoff[k]=0;
    amb_base_revision[k]++;ambr.overlap[k]=0;ambr.foundation_pending[k]=(uint8_t)(l->role<2 || l->role==5);
    ambr.dirty=1;
}
static void amb_patch(unsigned k,unsigned role,unsigned patch)
{
    ambr.loading=1;amb_sound_change(k);memset(ambr.recent[k],0,16);ambr.recent_valid[k]=0;
    amb_fill_patch(&amb.layer[k],amb.key,k,role,patch);
    if(!ambr.handoff[k])amb_apply_patch(k);
    ambr.loading=0;ambr.dirty=1;
}
#include "amb_auto_travel.h"
#include "amb_landscape.h"
/* UI scheduler. BARS publishes during the final sixteenth before the due
 * downbeat; the existing music engine applies both colour and harmony there.
 * Seconds use elapsed transport time and the same musical handoff as a turn. */
static int amb_auto_move(unsigned dice)
{
    unsigned style=amb_journey_style();
    int reach=(int)amb_walk_value(3),pos=travel.wanted;
    int target=auto_walk.goal;
    if(style==0 && auto_walk.heading && target==pos){auto_walk.heading=0;return 0;}
    if(!auto_walk.heading || target==pos || (style==2 && (dice&7)==0)){
        int distance=1+(int)((dice>>7)%(unsigned)reach);
        if(!style && (dice&8))distance=(distance+reach+1)/2;
        int sign=dice&1?1:-1;
        if(style==0 && pos && (dice&6))sign=pos>0?-1:1;
        if(!amb_endless() && (pos<=-18 || pos>=18))sign=pos>0?-1:1;
        target=pos+distance*sign;
        if(!amb_endless())target=clamp(target,-18,18);
        auto_walk.goal=target;auto_walk.heading=1;
    }
    int remaining=target-pos;
    int stride=style==2?1+(int)((dice>>17)%3):1+(int)((dice>>17)&1);
    return clamp(remaining,-stride,stride);
}
static void amb_auto_tick(void)
{
    unsigned clock=amb_walk_value(0);
    if(!clock || !song.playing || ambr.loading){amb_auto_reset();return;}
    unsigned now=clock==WALK_BARS?ambr.step:fm1_ms;
    if(auto_walk.armed && (int32_t)(now-auto_walk.due)<0)return;
    unsigned dice=amb_hash(amb.policy.seed+(++auto_walk.serial)*65537u);
    if(auto_walk.armed){
        int delta=amb_auto_move(dice);
        if(delta)amb_landscape_turn(delta);
    }
    unsigned lo=amb_walk_value(1),hi=amb_walk_value(2);
    unsigned delay=lo+((dice>>2)%(hi>=lo?hi-lo+1:1));
    auto_walk.due=clock==WALK_BARS?(now/16+delay)*16:now+1000*delay;
    auto_walk.armed=1;
}

static void amb_make_scene(amb_piece_t *p,unsigned scene)
{
    static const uint8_t roles[6][4]={{0,1,2,4},{0,5,2,4},{1,0,3,5},{0,3,2,5},{2,0,3,4},{5,1,2,4}};
    static const uint8_t roots[6]={0,5,2,7,3,9};
    /* Key, scale, tempo, harmonic interval, conductor, room, echo and four chairs.
     * The last column of each chair is its level in this arrangement. */
    typedef struct {uint8_t key,scale,bpm,auto_index,styles[3],room,echo;
                    uint8_t chair[4][4];} arrangement_t;
    static const arrangement_t arrangements[]={
        {2,1,58,4,{1,5,0},96,28,{{2,8,2,93},{0,9,1,73},{1,11,2,56},{2,9,2,67}}},
        {2,2,62,5,{1,5,0},104,12,{{0,10,1,94},{1,8,3,67},{2,9,3,46},{5,0,2,28}}},
        {5,3,80,3,{5,5,3},82,58,{{0,12,1,67},{2,10,0,82},{3,8,2,77},{5,3,1,38}}},
        {9,1,92,2,{1,2,0},64,32,{{2,9,0,91},{0,11,2,75},{3,8,1,75},{4,2,0,50}}},
        {0,1,54,5,{2,5,0},97,22,{{2,8,0,85},{0,9,2,56},{2,8,0,62},{0,14,3,65}}},
        {2,2,72,4,{8,0,0},102,57,{{0,16,1,89},{2,11,2,94},{3,9,1,67},{4,8,1,84}}},
        /* Pear Orchard: two independent tine voices over a sustained Skylight drone. */
        {5,3,66,4,{8,0,0},87,33,{{0,17,1,87},{2,12,2,81},{1,12,2,48},{2,13,3,59}}},
        /* Tide Atlas: bowed bass register, nylon and linen against an ocean drum. */
        {4,2,57,5,{5,3,0},99,25,{{0,18,1,82},{2,9,1,85},{1,13,2,71},{5,12,2,29}}},
        /* Pocket Orrery: crossing short pulses, with the shaker as quiet clockwork. */
        {7,1,91,3,{3,2,1},64,39,{{2,12,0,87},{0,14,2,68},{3,10,1,73},{5,15,1,29}}},
        /* Last Tram: nocturnal sax, piano and pedal with the air of a tape machine. */
        {9,0,63,4,{2,4,0},78,19,{{0,21,0,108},{2,14,0,108},{3,9,1,83},{5,1,1,25}}}
    };
    _Static_assert(NELEM(AMB_SCENE)==7+NELEM(arrangements),"factory catalog");
    scene%=NELEM(AMB_SCENE);
    if(scene==AMB_SCENE_INN){
        uint8_t *out=(uint8_t *)p;
        memset(p,0,sizeof *p);
        for(unsigned at=0;at<sizeof amb_nostalgic;){
            unsigned run=amb_nostalgic[at++];
            if(run<128){memcpy(out,amb_nostalgic+at,run);at+=run;}
            out+=run&127;
        }
        return;
    }
    const arrangement_t *arr=scene<6?0:&arrangements[scene-6-(scene>AMB_SCENE_INN)];
    unsigned i,j;
    memset(p,0,sizeof *p);p->magic=AMB_MAGIC;p->version=AMB_VERSION;
    amb_policy_init(&p->policy,0x414d4200u+scene);p->release_ms=6000;p->master=760;
    p->key=arr?arr->key:roots[scene];p->scale=arr?arr->scale:0;p->auto_index=arr?arr->auto_index:3;
    if(arr)memcpy(p->policy.active,arr->styles,3);
    p->evolve[0]=100;p->evolve[1]=35;p->evolve[2]=20;p->evolve[3]=15;
    for(i=0;i<G_COUNT;i++)p->global[i]=GP[i].def;
    p->global[G_BPM]=arr?arr->bpm:72+(int)scene*5;p->global[G_RSIZE]=arr?arr->room:94;p->global[G_RDAMP]=arr?62:82;
    p->global[G_DMIX]=arr?arr->echo:45;p->global[G_DFDBK]=55;p->global[G_DTIME]=7;p->global[G_RTYPE]=0;
    p->global[G_CLIP]=14;p->global[G_CDEPTH]=35;p->global[G_CRATE]=17;p->global[G_DMOD]=22;
    for(i=0;i<4;i++){
        unsigned role=arr?arr->chair[i][0]:roles[scene][i];
        amb_layer_t *l=&p->layer[i];l->generated=1;l->mode=role==0?3:2;
        l->density=(uint8_t)(i==3?20:35);l->follow=100;l->speed=arr?arr->chair[i][2]:1;
        for(j=0;j<16;j++)l->velocity[j]=76;
        amb_fill_patch(l,p->key,i,role,arr?arr->chair[i][1]:(scene+i*2)%8);
        l->level=arr?arr->chair[i][3]:(uint8_t)l->base[P_LEVEL];
    }
    /* Pitch and timing come from the live conductor for every factory layer. */
    if(scene==AMB_SCENE_GARDEN){
        p->eq[2]=-5;p->global[G_WIDTH]=102;p->global[G_RMOD]=126;
        p->master=1010;p->global[G_RTYPE]=5;p->global[G_DTYPE]=4;p->global[G_DFDBK]=59;
        p->layer[1].density=40;p->layer[3].density=35;p->policy.variation=35;
        p->layer[0].base[P_FCUT]=78;p->layer[2].base[P_TRANS]=0;p->layer[2].base[P_GLIDE]=20;
        for(i=0;i<4;i++)p->layer[i].base[P_PAN]=0;
        p->layer[0].base[P_PAN]=-8;p->layer[1].base[P_PAN]=10;
    }
    for(i=0;i<4;i++){p->grain[i][1]=50;p->grain[i][3]=20;}
    amb_anchor_master(p);
    if(scene==6)p->layer[0].density=46;
    if(scene==7)p->layer[0].mode=AMB_CHANT;
    if(scene==10){
        /* Two pianos on five- and eleven-step loops. The lower, closer
         * instrument and the distant answer share a major pentatonic pool. */
        p->layer[0].reserved=0x95;p->layer[1].reserved=0x02;
        p->layer[0].base[P_TRANS]=-12;p->layer[0].base[P_PAN]=-18;
        p->layer[2].base[P_PAN]=26;p->layer[2].base[P_E2]=38;
        p->layer[2].base[P_REV]=87;p->layer[2].base[P_DLY]=12;
        p->layer[1].mode=2;p->layer[3].mode=2;
        p->policy.variation=18;p->evolve[1]=15;p->evolve[2]=12;
    }
}
typedef struct {amb_piece_t piece;amb_take_t take;} amb_take_song_t;
_Static_assert(sizeof(amb_take_song_t)<=ST_PAYLOAD_MAX,"voice take fits an existing song");
static amb_take_song_t amb_pending_song __attribute__((section(".pool")));
#define amb_pending amb_pending_song.piece
static uint8_t amb_load_take,amb_capture_page,amb_replace_take;
static uint32_t amb_input_seen,amb_input_ms;

static volatile uint8_t amb_pending_ready;
static void amb_release(unsigned k);
static void amb_install(const amb_piece_t *p)
{
    if(amb_load_take){
        take_rec.running=amb_capture_page=amb_replace_take=0;
        take_generation[take_active^1]++;
        take_banks[take_active^1]=amb_pending_song.take;RING_PUBLISH();take_active^=1;
        amb_load_take=0;
    }

    unsigned k;
    ambr.loading=1;
    for(k=0;k<4;k++)amb_sound_change(k);
    if(amb_tail_any() && !amb_crossfade()){
        if(!ambr.piece_transition){ambr.piece_master=amb.master;memcpy(ambr.piece_eq,amb.eq,4);}
        ambr.piece_transition=1;
        /* Silent old layers also wait for the coordinated piece handoff. */
        for(k=0;k<4;k++)if(!ambr.handoff[k]){ambr.handoff[k]=1;ambr.tail_left[k]=ambr.tail_total[k]=0;memcpy(ambr.tail_eq[k],amb.layer_eq[k],4);}
    }else ambr.piece_transition=0;
    memset(&travel,0,sizeof travel);amb_auto_reset();amb_reset_lanes();
    amb=*p;memcpy(amb_reset_grain,amb.grain,sizeof amb_reset_grain);memcpy(amb_reset_layer,amb.layer,sizeof amb_reset_layer);memcpy(amb_reset_eq,amb.layer_eq,sizeof amb_reset_eq);
    memset(ambr.recent,0,sizeof ambr.recent);
    memset(ambr.recent_valid,0,sizeof ambr.recent_valid);memset(ambr.playhead_valid,0,sizeof ambr.playhead_valid);memset(&ambr.harmony_walk,0,sizeof ambr.harmony_walk);ambr.reset_pending=0;
    ambr.solo=(int8_t)amb.layer[0].solo_plus1-1;ambr.chord_q=ambr.proposal=255;
    for(k=0;k<4;k++)if(!ambr.handoff[k])amb_apply_patch(k);
    amb_pickup_arm(&ambr.pickup,ambr.target?amb.layer[ambr.target-1].level*1023u/127u:amb.master);
    if(amb.policy.active[0]==8)for(k=0;k<4;k++)ambr.voicing[k]=50+(int)k*7;
    amb_revoice();ambr.loading=0;ambr.dirty=1;
}
static void amb_start_factory(unsigned scene)
{
    unsigned i,j;amb_install(&amb_pending);amb_pending_ready=0;
    trk_live=4;song.sel=0;ambr.target=ambr.edit_layer=0;ambr.phase=ambr.step=0;ambr.last_play=0;
    ambr.local_preset=0;ambr.piece_preview=255;ambr.preset_position=(uint8_t)scene;
    for(i=0;i<4;i++)for(j=0;j<4;j++)ambr.macro_smooth[i][j]=64*256;
    amb_say(AMB_SCENE[scene%NELEM(AMB_SCENE)]);
}
static void amb_factory(unsigned scene)
{
    amb_load_take=0;amb_make_scene(&amb_pending,scene);amb_start_factory(scene);
}
static void amb_scan_slots(void);
static void amb_init(void)
{
#if defined(USB_DUPLEX) && FELUCCA_UAC
    usb_duplex_switch(1);
#endif
    amb_speech_init();tono_init_tables();
    memset(&ambr,0,sizeof ambr);amb_factory(AMB_SCENE_GARDEN);amb_scan_slots();
    ambr.dirty=1; kb_mask=0;perf_mask=0;song.grid=2;
}
static void amb_release(unsigned k)
{
    unsigned i;
    for(i=0;i<ambr.held_n[k];i++) trk_note_off(&trk[k],ambr.held[k][i]);
    ambr.held_n[k]=0;ambr.off[k]=0;
}
static void amb_note(unsigned k,unsigned note,unsigned vel,unsigned duration)
{
    unsigned n=ambr.held_n[k];
    trk_note_on(&trk[k],note,vel);
    if(n<4) ambr.held[k][ambr.held_n[k]++]=(uint8_t)note;
    ambr.off[k]=duration;
}
/* Audio sees only copies plus bounded macro/evolution offsets. Saved base
 * arrays, source IDs and manual patterns are never written here. */
/* Wire IDs are stable across saves. The first four are the original routes. */
static const uint8_t amb_route_param[]={
    P_E0,P_E1,P_E2,P_E3,P_E4,P_E5,P_E6,P_E7,
    P_ATK,P_DEC,P_SUS,P_REL,P_FTYPE,P_FRES,P_DIST,P_DTYPE,P_DTONE,
    P_CHOR,P_DLY,P_REV,P_APDEP,P_APRATE,P_APWAVE,P_GLIDE,P_VOICE,P_TRANS,
    P_LRATE,P_LWAVE,P_LN0+DV_KICK,P_LN0+DV_SNARE,P_LN0+DV_HATC,P_LN0+DV_TOM,P_LN7,P_LN6
};
enum { ROUTE_PARAM=4, ROUTE_MACRO=4+NELEM(amb_route_param), ROUTE_EQ=ROUTE_MACRO+4,
       ROUTE_MODE=ROUTE_EQ+4, ROUTE_SPEED, ROUTE_DENSITY, ROUTE_FOLLOW, ROUTE_SWING,
       ROUTE_FILL, ROUTE_MUTATE, ROUTE_NOTE, ROUTE_VEL, ROUTE_GRAIN, ROUTE_COUNT=ROUTE_GRAIN+4 };
static const param_desc_t *amb_param_desc(const amb_layer_t *l,unsigned id);
static int amb_lfo_offset(unsigned k,int span)
{
    int phase=trk[k].lfo_val*amb.layer[k].lfo_depth/32767;
    int scaled=phase*span;
    return (scaled+(scaled<0?-100:100))/200;
}
static int amb_routed(unsigned k,unsigned route,int base,int lo,int hi)
{
    return amb.layer[k].lfo_dest==route?clamp(base+amb_lfo_offset(k,hi-lo),lo,hi):base;
}
static void amb_global_offset(unsigned id,int offset)
{
    if(offset)song.g[id]=clamp(amb.global[id]+offset,GP[id].min,GP[id].max);
}
static unsigned amb_control_slot=4;
static __attribute__((noinline)) void amb_prepare(void)
{
    unsigned k,i; int32_t desired;
    if(ambr.loading)return;
    if(ambr.piece_transition && !amb_piece_waiting())ambr.piece_transition=0;
    if(!ambr.piece_transition && (amb_control_slot==4 || !amb_control_slot)){
        for(i=0;i<G_COUNT;i++)song.g[i]=amb.global[i];
        for(i=0;i<4;i++){
            unsigned id=amb_master_macros[i];
            int target=(amb.global[id]-GP[id].min-amb.master_anchor[i])*256;
            int diff=target-ambr.master_offset[i];
            int increment=diff/(amb_control_slot==4?16:4);
            ambr.master_offset[i]+=increment?increment:diff;
        }
        int room=ambr.master_offset[0]/256,echo=ambr.master_offset[1]/256;
        int motion=ambr.master_offset[2]/256,heat=-ambr.master_offset[3]/256;
        amb_global_offset(G_DJF,heat*3/4);
        amb_global_offset(G_CLIP,heat/3);
        amb_global_offset(G_DCOLOR,-heat/2);
        amb_global_offset(G_WIDTH,room/3+motion/2);
        amb_global_offset(G_RMOD,motion/2);
        if(amb.global[G_RTYPE]!=3)amb_global_offset(G_RDAMP,heat/2);
        if(echo)song.g[G_DFDBK]=clamp(amb.global[G_DFDBK]+echo/3,0,86);
    }
    for(k=0;k<4;k++) {
        amb_layer_t *l=&amb.layer[k]; track_t *t=&trk[k]; int warm,motion,bloom,space;
        if(ambr.overlap[k] && ambr.handoff[k] && ambr.tail_left[k] && amb_move_release(k))amb_apply_patch(k);
        if(ambr.handoff[k]){if(ambr.tail_left[k] || amb_track_active(k) || (ambr.piece_transition && amb_piece_waiting()))continue;amb_apply_patch(k);}
        /* The four performance-control updates are interleaved across audio
         * blocks. Edits and engine changes still reach the very next block. */
        int stagger=amb_control_slot<4 && l->lfo_dest<2 && ambr.applied_revision[k]==amb_base_revision[k];
        if(stagger && k!=amb_control_slot)continue;
        /* These arrays are naturally 16-bit aligned. The freestanding memcpy
         * copies bytes; doing this at control rate doubled the parameter-copy
         * work on pi32v2. Keep the saved/runtime separation with halfword IO. */
        if(ambr.last_routed_param[k]){
            unsigned last=ambr.last_routed_param[k]-1;
            t->p[last]=l->base[last];
            ambr.last_routed_param[k]=0;
        }
        if(ambr.applied_revision[k]!=amb_base_revision[k]) {
            for(i=0;i<P_COUNT;i++)t->p[i]=l->base[i];
            ambr.applied_revision[k]=amb_base_revision[k];
        }
        t->p[P_PAN]=l->base[P_PAN];
        t->p[P_DLY]=l->base[P_DLY];t->p[P_CHOR]=l->base[P_CHOR];
        t->p[P_E1]=l->base[P_E1];t->p[P_E2]=l->base[P_E2];t->p[P_E3]=l->base[P_E3];
        t->p[P_E4]=l->base[P_E4];t->p[P_E6]=l->base[P_E6];t->p[P_APDEP]=l->base[P_APDEP];
        for(i=0;i<4;i++) {
            int want=amb_routed(k,ROUTE_MACRO+i,l->macro[i],0,127)*256;
            int diff=want-ambr.macro_smooth[k][i];
            int increment=diff/(stagger?8:32);
            ambr.macro_smooth[k][i]+=increment?increment:diff;
        }
        warm=ambr.macro_smooth[k][0]/256-64; motion=ambr.macro_smooth[k][1]/256-64;
        bloom=ambr.macro_smooth[k][2]/256-64; space=ambr.macro_smooth[k][3]/256-64;
        int heat=-ambr.master_offset[3]/256;
        int tone=(l->role==6?0:warm)+heat/2;
        int room=ambr.master_offset[0]/256, echo=ambr.master_offset[1]/256;
        int movement=motion+ambr.master_offset[2]/512;
        for(unsigned j=0;j<4;j++)amb_grain_controls[k][j]=amb_routed(k,ROUTE_GRAIN+j,
            amb.grain[k][j],j==2?-12:0,j==2?12:100);
        int dust=l->role==5?bloom:l->role==6?motion:l->role==2?bloom/2:l->role<2?space/3:0;
        if(l->role==5 || l->role==6 || l->role==2){
            int master_motion=ambr.master_offset[2]/256;
            if(master_motion>0)dust+=master_motion/3;
        }
        if(dust>0){
            amb_grain_controls[k][0]=clamp(amb_grain_controls[k][0]+dust/2,0,100);
            amb_grain_controls[k][3]=clamp(amb_grain_controls[k][3]+dust/2,0,100);
        }
        t->eng_req=l->engine;
        t->p[P_LD_FLT]=t->p[P_LD_PIT]=t->p[P_LD_AMP]=0;
        if(l->lfo_dest==0)t->p[P_LD_FLT]=l->lfo_depth/2;
        else if(l->lfo_dest==1)t->p[P_LD_PIT]=l->lfo_depth/12;
        else if(l->lfo_dest==3)t->p[P_LD_AMP]=l->lfo_depth/2;
        t->p[P_LEVEL]=l->level;
        t->p[P_MUTE]=l->base[P_MUTE] || (ambr.solo>=0 && (unsigned)ambr.solo!=k);
        t->p[P_FCUT]=(int16_t)clamp(l->base[P_FCUT]-tone,8,127);
        if(l->engine==0) t->p[P_E4]=(int16_t)clamp(l->base[P_E4]-tone,8,120);
        t->p[P_LD_FLT]=(int16_t)clamp(t->p[P_LD_FLT]+motion/2,-63,63);
        if(l->lfo_dest==2)t->p[P_PAN]=(int16_t)clamp(l->base[P_PAN]+(t->lfo_val*l->lfo_depth)/65536,-64,63);
        t->p[P_REL]=(int16_t)clamp(l->base[P_REL]+bloom/2,0,127);
        t->p[P_REV]=(int16_t)clamp(l->base[P_REV]+space,0,120);
        if(l->engine==ENGI_VOICE)t->p[P_E0]=(int16_t)clamp(l->base[P_E0]+warm,0,127);
        if(l->role==5 && !amb_legacy_texture(l))t->p[P_E6]=(int16_t)clamp(l->base[P_E6]+bloom,0,l->engine==ENGI_AMB_SAMPLE?100:127);
        t->p[P_DLY]=(int16_t)clamp(l->base[P_DLY]+space/2,0,104);
        t->p[P_CHOR]=(int16_t)clamp(l->base[P_CHOR]+motion/2,0,100);
        if(l->engine==12)t->p[P_E2]=(int16_t)clamp(l->base[P_E2]-warm/2,-64,63);
        if(l->engine==11)t->p[P_E1]=(int16_t)clamp(l->base[P_E1]-warm,0,127);
        if(l->engine==ENGI_SINEFM)t->p[P_E1]=(int16_t)clamp(l->base[P_E1]-warm/2,0,127);
        if(l->engine==2)t->p[P_E2]=(int16_t)clamp(l->base[P_E2]-warm,0,127);
        if(l->engine==7)t->p[P_E3]=(int16_t)clamp(l->base[P_E3]-warm/8,-8,8);
        if(l->role==0 || l->role==1)t->p[P_SUS]=(int16_t)clamp(l->base[P_SUS]+bloom/3,40,127);
        if(l->engine==ENGI_SOFTPULSE && t->p[P_ROOT]!=l->base[P_ROOT])t->p[P_ROOT]=l->base[P_ROOT];
        if(l->engine==ENGI_SOFTPULSE)t->p[P_E2]=clamp(l->base[P_E2]-warm,0,127);
        if(l->role==4) t->p[P_E4]=(int16_t)clamp(l->base[P_E4]+bloom/3,0,108);
        if(l->role==6) {
            t->p[P_E7]=clamp(l->base[P_E7]-warm/3,0,127);
            t->p[P_ATK]=l->base[P_ATK];
            if(l->base[P_E1]==3){t->p[P_ATK]=0;t->p[P_REL]=45;}
            t->p[P_E2]=(int16_t)clamp(l->base[P_E2]+warm/4,-24,24);
            t->p[P_E6]=(int16_t)clamp(l->base[P_E6]+motion/2,0,100);
            t->p[P_APDEP]=(int16_t)clamp(l->base[P_APDEP]+bloom/2,0,64);
        }
        t->p[P_VOICE]=(l->generated && l->mode==3 && l->role!=4 && l->role!=5)?V_POLY:l->base[P_VOICE];
        if(l->role==3) t->p[P_GLIDE]=(int16_t)clamp(l->base[P_GLIDE]+bloom/2,0,100);
        /* At centre these additions are transparent. Increase body by driving
         * into a darker tone, and place distant layers deeper in both returns. */
        t->p[P_DIST]=clamp(l->base[P_DIST]+tone/3+(l->role==4?bloom/3:0),0,127);
        t->p[P_DTONE]=clamp(l->base[P_DTONE]-tone/2,-64,63);
        t->p[P_APDEP]=clamp(t->p[P_APDEP]+movement/(l->role==3?5:3),0,127);
        t->p[P_REV]=clamp(t->p[P_REV]+room/2+(l->role==2?bloom/3:0),0,127);
        t->p[P_DLY]=clamp(t->p[P_DLY]+echo/2+(l->role==2?bloom/4:0),0,127);
        t->p[P_CHOR]=clamp(t->p[P_CHOR]+ambr.master_offset[2]/512+space/4,0,127);
        if(l->role!=6)t->p[P_ATK]=clamp(l->base[P_ATK]+tone/(l->role==3 || l->role==4?6:3),0,127);
        if(l->role<3)t->p[P_DEC]=clamp(l->base[P_DEC]+bloom/2,0,127);
        if(song.playing && l->generated) {
            int off=(int)(amb_hash(amb.policy.seed+ambr.step/64+k*31)%17)-8;
            ambr.evolve_smooth[k]+=(off*65536-ambr.evolve_smooth[k])/(stagger?512:2048);
            off=ambr.evolve_smooth[k]/65536;
            t->p[P_FCUT]=(int16_t)clamp(t->p[P_FCUT]+off*amb.evolve[1]/100,16,127);
            if(l->engine==0)t->p[P_E4]=(int16_t)clamp(t->p[P_E4]+off*amb.evolve[1]/100,0,127);
            t->p[P_REV]=(int16_t)clamp(t->p[P_REV]+off*amb.evolve[3]/100,0,112);
        }
        if(l->lfo_dest>=ROUTE_PARAM && l->lfo_dest<ROUTE_MACRO){
            unsigned id=amb_route_param[l->lfo_dest-ROUTE_PARAM];
            const param_desc_t *d=amb_param_desc(l,id);
            t->p[id]=amb_routed(k,l->lfo_dest,t->p[id],d->min,d->max);
            ambr.last_routed_param[k]=(uint8_t)(id+1);
        }
    }
    desired=ambr.panic?0:(ambr.piece_transition?ambr.piece_master:amb.master)*256;
    ambr.master_smooth+=(desired-ambr.master_smooth)/32;
    if(desired-ambr.master_smooth<32 && ambr.master_smooth-desired<32) ambr.master_smooth=desired;
    song.master_q12=master_of_pot((uint32_t)(ambr.master_smooth/256));
}
static void amb_control_tick(void)
{
    static unsigned next;
    amb_control_slot=next;next=(next+1)&3;
    amb_prepare();amb_control_slot=4;
}
/* Hold an exact common note. Different pitches use the engine's normal
 * note-on/release path; portamento is never imposed by the generator. */
static int amb_carry_note(unsigned k,unsigned note,unsigned duration)
{
    track_t *t=&trk[k];unsigned i;
    if(ambr.held_n[k]!=1 || ambr.held[k][0]!=note)return 0;
    for(i=0;i<NVOICE;i++)if(t->v[i].active && t->v[i].gate && t->v[i].note==note){ambr.off[k]=duration;return 1;}
    return 0;
}
/* Preserve exact common tones and release changed tones normally. Under a
 * full budget, keep the foundation rather than stealing it for an extra tone. */
static int amb_keep_pad(unsigned k,const uint8_t *notes,unsigned n,unsigned gate)
{
    track_t *t=&trk[k];uint8_t keep[4],present[2]={0,0};unsigned i,j,kept=0;
    for(i=0;i<ambr.held_n[k];i++){
        unsigned old=ambr.held[k][i],live=0,wanted=n;
        for(j=0;j<NVOICE;j++)if(t->v[j].active && t->v[j].gate && t->v[j].note==old){live=1;break;}
        for(j=0;j<n;j++)if(notes[j]==old){wanted=j;break;}
        if(live && wanted<n){keep[kept++]=(uint8_t)old;present[wanted]=1;}
        else trk_note_off(t,old);
    }
    ambr.held_n[k]=(uint8_t)kept;for(i=0;i<kept;i++)ambr.held[k][i]=keep[i];
    for(i=0;i<n;i++)if(!present[i]){
        if(ambr.held_n[k] && voices_busy()>=voice_cap())continue;
        amb_note(k,notes[i],i?55:65,gate);
    }
    ambr.off[k]=gate;return ambr.held_n[k]!=0;
}
static void amb_set_generated(unsigned k,unsigned on)
{
    if(amb.layer[k].generated==!!on)return;
    amb.layer[k].generated=(uint8_t)!!on;
    ambr.grid_cue[k]=(amb_grid_cue_t){0};ambr.recording=0;
    amb_say(on?"AUTO RESUMED / MANUAL NOTES KEPT":"MANUAL PATTERN / EDIT FOR AUTO");
}
/* Project a free pedal into the selected note pool when FOLLOW is low. */
static unsigned amb_pool_nearest(unsigned note)
{
    unsigned pool=amb_pool();
    unsigned pc=(note+12-amb.key)%12;
    for(unsigned distance=0;distance<=6;distance++){
        if(pool&(1u<<((pc+distance)%12)))return note+distance;
        if(pool&(1u<<((pc+12-distance)%12)))return note-distance;
    }
    return note; /* the tonic is always enabled */
}
/* A two-bar contour at eighth-note speed. A phrase picks its reciting
 * degree, approach, neighbours and cadence from the piece seed. Using
 * absolute musical time keeps silent capture and playback in agreement. */
static unsigned amb_chant_note(unsigned layer,unsigned step)
{
    unsigned slot=step>>amb_routed(layer,ROUTE_SPEED,amb.layer[layer].speed,0,3),pos=slot%16;
    uint32_t shape=amb_hash(amb.policy.seed+layer*97u+(slot/16)*0x9e3779b9u+
                            amb.policy.active[1]*313u);
    unsigned recite=2+shape%3,opening=(shape>>8)%3,landing=(shape>>12)%2;
    unsigned degree=opening+pos;
    if(degree>recite)degree=recite;
    if(pos>=3 && pos<12 && ((shape>>(pos+4))&3u)==0)
        degree=(shape&(1u<<pos))?degree+1:degree-1;
    if(degree>landing+15-pos)degree=landing+15-pos;
    int scale[12];unsigned count=amb_scale(amb_pool(),scale),root=amb.chord%count;
    unsigned target=root+degree;
    int tonic=nearest(62,(amb.key+scale[root])%12,56,67);
    return (unsigned)(tonic+12*(int)(target/count)+scale[target%count]-scale[root]);
}
/* If rhythm and layer divisions run out of phase, a part can miss every
 * offered note. Keep on-grid hits literal. After a bar without a hit, one
 * off-grid cue may move to the next slot. Never fill a conductor rest
 * from a cue older than one local interval. */
static unsigned amb_align_cue(const amb_layer_t *l,unsigned k,unsigned step,unsigned mask,amb_grid_cue_t *cue)
{
    unsigned mode=amb_routed(k,ROUTE_MODE,l->mode,0,l->role==0?AMB_CHANT:4);
    unsigned speed=amb_routed(k,ROUTE_SPEED,l->speed,0,3);
    unsigned bit=1u<<k,stride=1u<<speed;
    if(l->role==5 || mode==4){*cue=(amb_grid_cue_t){0};return mask;}
    if(l->role==1 && mode!=3)stride*=8;
    if(l->role==0 && mode!=AMB_CHANT)stride*=2;
    if(step&(stride-1u)){
        if(mask&bit)cue->pending=1;
        return mask;
    }
    unsigned elapsed=cue->quiet_steps+stride;
    if(cue->pending && elapsed>=16)mask|=bit;
    cue->quiet_steps=(mask&bit)?0:(uint8_t)(elapsed>16?16:elapsed);
    cue->pending=0;
    return mask;
}
static unsigned amb_generated_notes(unsigned k,unsigned step,unsigned mask,uint8_t *notes,unsigned *gate)
{
    amb_layer_t *l=&amb.layer[k];unsigned i,bar=step/16;int note=ambr.voicing[k];
    unsigned mode=amb_routed(k,ROUTE_MODE,l->mode,0,l->role==0?AMB_CHANT:4);
    unsigned speed=amb_routed(k,ROUTE_SPEED,l->speed,0,3);
    unsigned mutation=amb_routed(k,ROUTE_MUTATE,l->mutation,0,100);
    if(l->role==5 && !amb_legacy_texture(l)){
        if(step%(1u<<speed))return 0;
        if(mode!=0){
            if(!((mask>>k)&1))return 0;
            if(mode==1 && step%8)return 0;
            if(mode==3 && step%16>3)return 0;
            if(mode==4 && step%32)return 0;
        }
        notes[0]=60;*gate=mode==0?FS*60u:mode==2?FS*6u:FS;return 1;
    }
    if(!((mask>>k)&1) || step%(1u<<speed))return 0;
    if(mode==4 && (step%16 || bar%4==3))return 0;
    if(l->role==0 && mode==AMB_CHANT){
        notes[0]=(uint8_t)amb_chant_note(k,step);
        *gate=(FS*60u/(unsigned)amb.global[G_BPM])*(1u+l->macro[2]/32u);
        return 1;
    }
    if(l->role==0){
        static const uint8_t pair[6][2]={{0,2},{1,3},{0,3},{0,1},{2,3},{1,2}};
        unsigned turn=step/8,phrase=amb.policy.active[1],sel;
        if(step%(2u<<speed))return 0;
        sel=amb_pad_pair(amb.policy.seed+k*97u,phrase,turn);
        i=mode==0?turn%4:mode==1?3-turn%4:mode==3?pair[sel][0]:amb_hash(amb.policy.seed+turn+phrase*31u)%4;
        notes[0]=(uint8_t)ambr.voicing[i];*gate=FS*(8u+l->macro[2]/4u);
        if(mode==3){
            int shift=((turn/24+phrase)%3==1 && phrase!=7)?12:0;
            notes[0]=(uint8_t)clamp((int)notes[0]+shift,42,89);
            notes[1]=(uint8_t)clamp(ambr.voicing[pair[sel][1]]+shift,42,89);return 2;
        }return 1;
    }
    if(mode==3){notes[0]=(uint8_t)ambr.voicing[k%4];notes[1]=(uint8_t)ambr.voicing[(k+2)%4];*gate=FS*6;return 2;}
    if(l->role==1){if(step%(8u<<speed))return 0;note=ambr.voicing[0];*gate=FS*(12u+l->macro[2]/3u);}
    else if(l->role==3){note=36+(ambr.voicing[0]%12);*gate=FS;}
    else if(l->role==2){
        unsigned motif=amb.policy.active[1]==6?step%128:step;
        if(mutation && amb_hash(amb.policy.seed+step)%100<mutation)motif+=bar*13;
        i=mode==0?(step>>speed)%4:mode==1?3-(step>>speed)%4:amb_hash(amb.policy.seed+motif/3+k)%4;
        note=ambr.voicing[i]+12;*gate=FS*2;
    }else *gate=FS*5;
    if((l->role==1 || l->role==3 || l->role==6) && mode<2){
        unsigned ix=(step/(l->role==1?(8u<<speed):(1u<<speed)))%4;
        if(mode==1)ix=3-ix;
        note=l->role==3?36+ambr.voicing[ix]%12:ambr.voicing[ix];
    }
    if(amb_hash(amb.policy.seed+k*37+step/64)%100>=amb_routed(k,ROUTE_FOLLOW,l->follow,0,100))note=(int)amb_pool_nearest(48+k*5);
    notes[0]=(uint8_t)clamp(note,24,96);return 1;
}
static void amb_participation(amb_participation_t *part,unsigned step)
{
    unsigned k;for(k=0;k<4;k++)part[k]=(amb_participation_t){amb.layer[k].generated && !amb.layer[k].base[P_MUTE] && (ambr.solo<0 || (unsigned)ambr.solo==k),(uint8_t)clamp(amb_routed(k,ROUTE_DENSITY,amb.layer[k].density,0,100)+
        ((amb.layer[k].role==4)?(ambr.macro_smooth[k][1]/256-64)/2:
         (amb.layer[k].role==0 || amb.layer[k].role==1)?(ambr.macro_smooth[k][2]/256-64)/2:0)+
        ((int)amb_triangle(step/16,32)-50)*amb.evolve[2]/500,0,100)};
}
static void amb_capture_generated(unsigned k)
{
    amb_layer_t *l=&amb.layer[k];unsigned i,n=0;
    for(i=0;i<16;i++)n+=ambr.recent[k][i]!=0;
    if(ambr.recent_valid[k] && (n || amb.policy.active[0]==8)){memcpy(l->notes,ambr.recent[k],16);memcpy(l->velocity,ambr.recent_vel[k],16);}
    else if(amb.policy.active[0]==8 && l->role!=5){
        memset(l->notes,0,sizeof l->notes);
    }else {
        amb_participation_t parts[4];amb_grid_cue_t cue={0};
        unsigned stride=1u<<amb_routed(k,ROUTE_SPEED,l->speed,0,3),period=16*stride,base=ambr.step-ambr.step%period;
        for(unsigned offset=0;offset<period;offset++){
            unsigned step=base+offset,gate=FS;
            uint8_t notes[2];amb_participation(parts,step);
            unsigned mask=amb_align_cue(l,k,step,amb_policy_events(&amb.policy,step,parts),&cue);
            if(offset%stride)continue;
            unsigned slot=offset/stride;
            l->notes[slot]=amb_generated_notes(k,step,mask,notes,&gate)?notes[0]:0;
            l->velocity[slot]=65;
        }
    }
    /* A very sparse phrase can contain no event in this window. Seed its
     * current actual voicing rather than opening an empty recording. */
    for(i=n=0;i<16;i++)n+=l->notes[i]!=0;
    if(!n && (amb.policy.active[0]!=8 || !ambr.recent_valid[k])){l->notes[0]=(uint8_t)(ambr.voicing[k]+(l->role==2?12:0));l->velocity[0]=70;}
}
#include "amb_garden.h"
static void amb_layer_step(unsigned k,unsigned step,unsigned mask)
{
    amb_layer_t *l=&amb.layer[k];unsigned i,tick=step%16,bar=step/16,gate=FS/2,n;uint8_t notes[2];
    unsigned speed=amb_routed(k,ROUTE_SPEED,l->speed,0,3);
    unsigned fill=amb_routed(k,ROUTE_FILL,l->fill,0,100);
    unsigned mutation=amb_routed(k,ROUTE_MUTATE,l->mutation,0,100);
    if(ambr.handoff[k] || trk[k].engine!=l->engine || l->base[P_MUTE] || (ambr.solo>=0 && (unsigned)ambr.solo!=k)){
        ambr.grid_cue[k]=(amb_grid_cue_t){0};return;
    }
    if(!(step%(1u<<speed))){ambr.playhead[k]=(uint8_t)((step>>speed)%16);ambr.playhead_valid[k]=1;}
    if(l->generated && amb.policy.active[0]==8 && l->role!=5){garden_step(k,step);return;}
    if(l->role==4){
        ambr.playhead[k]=(uint8_t)(step%16);ambr.playhead_valid[k]=1;
        static const uint8_t drum[4]={36,38,42,50};
        if(!l->generated){for(i=0;i<4;i++)if((l->drums[i]>>tick)&1u)trk_note_on(&trk[k],drum[i],64+i*4);}
        else if((mask>>k)&1){unsigned lane=amb_hash(amb.policy.seed+step+k*53+bar*mutation)%4;trk_note_on(&trk[k],drum[lane],52+amb_hash(step)%24);
            if(fill && tick==15 && bar%4==3)trk_note_on(&trk[k],42,45+fill/4);}
        return;
    }
    if(!l->generated){
        if(step%(1u<<speed))return;
        unsigned slot=(step>>speed)%16;
        unsigned note=l->notes[slot]?amb_routed(k,ROUTE_NOTE,l->notes[slot],1,127):0;
        if(!note){if(l->role==5)amb_release(k);return;}
        if(l->role<2 || l->role==5){
            gate=FS*(l->role==0?8u+l->macro[2]/4u:12u+l->macro[2]/3u);
            if(amb_carry_note(k,note,gate))return;
        }
        amb_release(k);amb_note(k,note,amb_routed(k,ROUTE_VEL,l->velocity[slot],1,127),gate);return;
    }
    mask=amb_align_cue(l,k,step,mask,&ambr.grid_cue[k]);
    n=amb_generated_notes(k,step,mask,notes,&gate);
    if(!(step% (1u<<speed))){i=(step>>speed)%16;ambr.recent[k][i]=n?notes[0]:0;ambr.recent_vel[k][i]=65;ambr.recent_valid[k]=1;}
    if(!n)return;
    if(n==1 && (l->role==0 || l->role==1 || l->role==5) && amb_carry_note(k,notes[0],gate))return;
    if(l->role==0 && amb_keep_pad(k,notes,n,gate))return;
    amb_release(k);for(i=0;i<n;i++)amb_note(k,notes[i],i?55:65,gate);
}
static int amb_octave_delta(unsigned target)
{
    return (int8_t)(ambr.octave_taps[target]-ambr.octave_seen[target]);
}
static void amb_octave_boundary(void)
{
    int change[5];
    for(unsigned i=0;i<5;i++){
        uint8_t taps=ambr.octave_taps[i];
        change[i]=(int8_t)(taps-ambr.octave_seen[i]);
        ambr.octave_seen[i]=taps;
    }
    for(unsigned k=0;k<4;k++)amb_bar_octave[k]=clamp(amb_bar_octave[k]+change[0]+change[k+1],-3,3);
}
static void amb_queue_octave(int direction)
{
    unsigned target=ambr.view==1?ambr.edit_layer+1:ambr.target;
    if(amb_octave_delta(target)*direction<3)ambr.octave_taps[target]+=direction;
    ambr.dirty=1;
}
static void amb_step(void)
{
    amb_participation_t part[4]; uint8_t mask; unsigned k,bar=ambr.step/16;
    if(travel.ready)amb_landscape_colour(0);
    if(ambr.step%16==0) {
        amb_octave_boundary();
        if(amb_pending_ready){amb_install(&amb_pending);amb_pending_ready=0;amb_say("PRESET RECALLED ON BAR");}
        else if(ambr.chord_q!=255) { amb.chord=ambr.chord_q;ambr.chord_q=255;amb_revoice(); }
        else if((!travel.ready || !amb_landscape_bar()) && amb.policy.active[0]!=8 && bar && AMB_AUTO[amb.auto_index] && bar%AMB_AUTO[amb.auto_index]==0 && amb_hash(amb.policy.seed+bar)%100u<amb.evolve[0]) {
            int scale[12];unsigned count=(unsigned)amb_scale(amb_pool(),scale);
            amb.chord=(uint8_t)amb_walk_next(&ambr.harmony_walk,amb.policy.seed,amb.chord,count);amb_revoice();
        }
    }
    {uint8_t before[3];memcpy(before,amb.policy.active,3);
        if(!(ambr.step%16u))amb_ensemble_downbeat();
        if(!(ambr.step%64u))amb_travel_patterns(1);
        amb_policy_boundary(&amb.policy,ambr.step);
        if(memcmp(before,amb.policy.active,3))for(k=0;k<4;k++)if(amb.layer[k].generated)amb_begin_change(k);
    }
    if(amb.policy.active[0]==8)garden_harmony(ambr.step);
    else garden_drift.limit=0;
    amb_participation(part,ambr.step);
    mask=amb_policy_events(&amb.policy,ambr.step,part);
    for(k=0;k<4;k++) {
        unsigned delay=(ambr.step&1u)?(FS*60u/(unsigned)(amb.global[G_BPM]*4))*amb_routed(k,ROUTE_SWING,amb.layer[k].swing,0,100)/200u:0;
        if(delay) {ambr.due[k]=delay;ambr.due_step[k]=ambr.step;ambr.due_mask[k]=mask;}
        else amb_layer_step(k,ambr.step,mask);
    }
    ambr.step++;ambr.dirty=1;
}
static __attribute__((noinline)) void amb_clock(uint32_t n)
{
    unsigned k;
    if(ambr.loading)return;
    for(k=0;k<NTRK;k++)if(ambr.handoff[k]){
        unsigned elapsed,p,q;
        ambr.tail_left[k]=ambr.tail_left[k]>n?ambr.tail_left[k]-n:0;
        if(!ambr.tail_total[k]){ambr.tail_gain[k]=0;continue;}
        elapsed=ambr.tail_total[k]-ambr.tail_left[k];
        p=(unsigned)(((uint64_t)elapsed<<15)/ambr.tail_total[k]);q=(p*p)>>15;
        q=(q*(98304u-2u*p))>>15;ambr.tail_gain[k]=(int32_t)(32768u-q);
    }
    for(k=4;k<NTRK;k++)if(ambr.handoff[k] && !ambr.tail_left[k] && !amb_track_active(k)){
        ambr.handoff[k]=0;
        for(unsigned j=0;j<4;j++)if(ambr.fade_slot[j]==k)ambr.fade_slot[j]=0;
    }
    for(k=0;k<4;k++) if(ambr.off[k]) {
        if(ambr.off[k]<=n) amb_release(k); else ambr.off[k]-=n;
    }
    for(k=0;k<4;k++)garden_advance(k,n);
    if(!song.playing) {
        for(k=0;k<4;k++)ambr.due[k]=0;
        if(ambr.last_play)for(k=0;k<4;k++)amb_release(k);
        amb_reset_lanes();ambr.last_play=0;return;
    }
    if(!ambr.last_play){
        ambr.phase=ambr.step=0;ambr.last_play=1;memset(ambr.grid_cue,0,sizeof ambr.grid_cue);
        for(k=0;k<4;k++)ambr.foundation_pending[k]=(uint8_t)((amb.layer[k].role<2 || amb.layer[k].role==5) && amb.layer[k].generated);
        amb_step();
    }
    for(k=0;k<4;k++)if(ambr.foundation_pending[k] && !ambr.handoff[k] && trk[k].engine==amb.layer[k].engine){
        ambr.foundation_pending[k]=0;
        if((amb.policy.active[0]!=8 || amb.layer[k].role==5) && amb.layer[k].generated && !ambr.held_n[k])amb_layer_step(k,ambr.step-(ambr.step%((amb.layer[k].role==1?8u:2u)<<amb.layer[k].speed)),1u<<k);
    }
    for(k=0;k<4;k++)if(ambr.due[k]) {
        if(ambr.due[k]<=n) {ambr.due[k]=0;amb_layer_step(k,ambr.due_step[k],ambr.due_mask[k]);}
        else ambr.due[k]-=n;
    }
    ambr.phase+=n;
    while(ambr.phase>=FS*60u/(unsigned)(amb.global[G_BPM]*4)) {
        ambr.phase-=FS*60u/(unsigned)(amb.global[G_BPM]*4);amb_step();
    }
}
static unsigned amb_level_feedback(void)
{
    return ambr.level_feedback_active && ambr.target && ambr.target==ambr.level_feedback_target &&
        (uint32_t)(fm1_ms-ambr.level_feedback_ms)<1200u?ambr.target:0;
}
static void amb_master(int32_t raw,int boot)
{
    unsigned target=ambr.view==1 && ambr.page==1?ambr.edit_layer+1:ambr.target;
    if(target!=ambr.pickup_target){
        ambr.pickup_target=target;
        amb_pickup_arm(&ambr.pickup,target?amb.layer[target-1].level*1023u/127u:amb.master);
    }
    if(raw<0 || raw>1023) return;
    if(boot) { amb.master=(uint16_t)raw;ambr.master_smooth=raw*256;amb_pickup_arm(&ambr.pickup,(uint16_t)raw); }
    if(!amb_pickup_read(&ambr.pickup,raw)) return;
    if(target==0){amb.master=(uint16_t)raw;if(ambr.piece_transition)ambr.piece_master=(uint16_t)raw;}
    else {
        unsigned level=(unsigned)raw*127u/1023u;
        if(level!=amb.layer[target-1].level){ambr.level_feedback_active=1;ambr.level_feedback_target=(uint8_t)target;ambr.level_feedback_ms=fm1_ms;}
        amb.layer[target-1].level=(uint8_t)level;
    }
    ambr.dirty=1;
}
/* The EQ pages share three broad bands with the audio filters. Each target has
 * one audio writer. The display reads an independent envelope per band. */
typedef struct {
    int32_t low[2], mid_lowpass[2];
    volatile unsigned peak[3], updated_ms;
} amb_eq_meter_t;
static amb_eq_meter_t amb_eq_meters[5]; /* master, then layers 1–4 */

static void amb_measure_eq(unsigned target,const int32_t *samples,unsigned frames,unsigned channels)
{
    amb_eq_meter_t *meter=&amb_eq_meters[target];
    unsigned peak[3]={0};
    if(fm1_ms-meter->updated_ms>=200)memset(meter,0,sizeof *meter);
    for(unsigned c=0;c<channels;c++){
        int low=meter->low[c],mid_lp=meter->mid_lowpass[c];
        for(unsigned i=c;i<frames*channels;i+=channels){
            int x=clamp(samples[i],-262144,262144);
            low+=((x-low)*26)>>10;
            mid_lp+=((x-mid_lp)*307)>>10;
            unsigned bass=low<0?-low:low,mid=mid_lp<low?low-mid_lp:mid_lp-low;
            unsigned high=x<mid_lp?mid_lp-x:x-mid_lp;
            if(bass>peak[0])peak[0]=bass;
            if(mid>peak[1])peak[1]=mid;
            if(high>peak[2])peak[2]=high;
        }
        meter->low[c]=low;meter->mid_lowpass[c]=mid_lp;
    }
    for(unsigned j=0;j<3;j++){
        unsigned old=meter->peak[j];
        unsigned fall=(old+127)>>7;
        meter->peak[j]=peak[j]>old-fall?peak[j]:old-fall;
    }
    meter->updated_ms=fm1_ms;
}

/* Bounded three-band output EQ. One-pole splits at ~180 Hz / ~3 kHz;
 * smoothed gains, headroom and a final soft knee, both stereo channels. */
static inline __attribute__((always_inline)) void amb_master_eq_output(int32_t *out,uint32_t n)
{
    static int32_t low[2],high[2],g[4]={4096,4096,4096,4096};
    static const uint16_t db[25]={1029,1154,1295,1453,1631,1830,2053,2304,2584,2900,3254,3650,4096,4596,5157,5786,6492,7285,8173,9170,10289,11544,12952,14532,16306};
    const int8_t *eq=ambr.piece_transition?ambr.piece_eq:amb.eq;
    unsigned i,c,k;
    /* Exact flat EQ needs no crossover filters. The upstream master already
     * protects the output; retain AMB+'s additional knee for larger peaks.
     * Crossover state starts settling while smoothed gains move from unity
     * when an EQ band is deliberately changed. */
    if(!eq[0] && !eq[1] && !eq[2] && !eq[3] &&
       g[0]==4096 && g[1]==4096 && g[2]==4096 && g[3]==4096) {
        for(i=0;i<n*2u;i++)if(out[i]>12000 || out[i]<-12000)out[i]=soft_knee(out[i],12000);
        return;
    }
    for(k=0;k<4;k++){int d=(int)db[eq[k]+12]-g[k];g[k]+=d/16;if(d>-16 && d<16)g[k]=db[eq[k]+12];}
    for(i=0;i<n;i++) for(c=0;c<2;c++) {
        int32_t x=out[2*i+c], y;
        low[c]+=(x-low[c])*26/1024; high[c]+=(x-high[c])*307/1024;
        y=(low[c]*g[0]+(high[c]-low[c])*g[1]+(x-high[c])*g[2])/4096;
        y=(int32_t)((clamp(y,-65535,65535)*g[3])/4096);
        out[2*i+c]=soft_knee(y,12000);
    }
}
static __attribute__((noinline)) void amb_output(int32_t *out,uint32_t n)
{
    if(ambr.view==2 && ambr.page==4 && !ambr.help)amb_measure_eq(0,out,n,2);
    amb_master_eq_output(out,n);
    if(amb_monitor_active)amb_monitor_output(out,n);
}
/* Three broad bands per layer before dry mix and shared sends. Flat EQ is
 * bit-exact bypass. Independent state per physical track, safe on two cores. */
static __attribute__((noinline)) void amb_layer_process(unsigned k,int32_t *b,uint32_t n)
{
    static int32_t low[NTRK][6],high[NTRK][6],g[NTRK][4];static uint8_t primed[NTRK];
    static const uint16_t db[25]={1029,1154,1295,1453,1631,1830,2053,2304,2584,2900,3254,3650,4096,4596,5157,5786,6492,7285,8173,9170,10289,11544,12952,14532,16306};
    unsigned i,j,c,flat=1;if(k>=4 && !ambr.handoff[k])return;
    if(amb_grain_controls[k][0] || amb_grain[k].mix || amb_grain[k].valid)
        amb_grain_process(k,b,n);
    if(ambr.view==1 && ambr.page==6 && ambr.edit_layer==k && !ambr.help)amb_measure_eq(k+1,b,n,1);
    const int8_t *eq=ambr.handoff[k]?ambr.tail_eq[k]:amb.layer_eq[k];
    int moving_eq=k<4 && amb.layer[k].lfo_dest>=ROUTE_EQ && amb.layer[k].lfo_dest<ROUTE_MODE;
    if(!primed[k] && !(eq[0] || eq[1] || eq[2] || eq[3] || moving_eq))return;
    if(!primed[k]){for(j=0;j<4;j++)g[k][j]=4096;primed[k]=1;}
    for(j=0;j<4;j++){
        int target=db[(ambr.handoff[k]?ambr.tail_eq[k][j]:amb_routed(k,ROUTE_EQ+j,amb.layer_eq[k][j],-12,12))+12],d=target-g[k][j];
        if(target!=4096 || g[k][j]!=4096)flat=0;
        g[k][j]+=d/16;if(d>-16 && d<16)g[k][j]=target;
    }
    if(flat){primed[k]=0;return;}
    /* Drum IMAGE and per-lane sends are corrected after the mono mix. Shape
     * those private side buses too, with independent histories and one gain
     * update, so their correction cannot bypass a layer's EQ. */
    for(c=0;c<(trk[k].engine==ENGI_DRUM && dx_xon?6u:1u);c++){
        int32_t *buf=c==0?b:c==1?dx_sl:c==2?dx_sr:dx_sb[c-3];
        for(i=0;i<n;i++){
            int32_t x=buf[i];int64_t y;
            low[k][c]+=(int32_t)((((int64_t)x-low[k][c])*26)/1024);
            high[k][c]+=(int32_t)((((int64_t)x-high[k][c])*307)/1024);
            y=((int64_t)low[k][c]*g[k][0]+(int64_t)(high[k][c]-low[k][c])*g[k][1]+(int64_t)(x-high[k][c])*g[k][2])/4096;
            y=(y*g[k][3])/4096;
            /* Leave the mixer's fixed-point multiply ample headroom even at
             * +12 dB on all bands plus trim; flat EQ bypass stays bit-exact. */
            buf[i]=soft_knee((int32_t)(y>268435456?268435456:y<-268435456?-268435456:y),96000);
        }
    }
}
/* Uses existing project A/B pairs: 0x97000..0x9efff, 0xa0000..0xa7fff.
 * No new sector, bootloader or reserved/save-bound change. Legacy payloads
 * are occupied and guarded, never silently migrated or erased. */
static uint32_t amb_object(unsigned slot) { return slot<4?OBJ_PROJECT0+slot:OBJ_SONG1+slot-4; }
static int amb_occupied(unsigned slot)
{
    st_hdr_t h;
    return st_head(amb_object(slot),0,&h)==0 || st_head(amb_object(slot),1,&h)==0;
}
static int amb_valid(const amb_piece_t *p)
{
    unsigned k,i;
    if(p->magic!=AMB_MAGIC || (p->version<1 || p->version>AMB_VERSION) ||
       p->master>1023 || p->key>11 || p->scale>3 || p->chord>11 || p->auto_index>7) return 0;
    if(p->version>=3 && (((unsigned)p->layer[1].reserved>15u) ||
       ((p->layer[0].reserved || p->layer[1].reserved) && !(p->layer[0].reserved&1u))))return 0;
    if(p->version>=3){unsigned m=p->layer[0].reserved|((unsigned)p->layer[1].reserved<<8),n=0;
        for(i=0;i<12;i++)n+=(m>>i)&1u;if(m && n<3)return 0;}
    if(p->version>=4){
        if(p->release_ms<250 || p->release_ms>AMB_MAX_TAIL_MS || p->release_ms%250)return 0;
        for(k=0;k<4;k++)for(i=0;i<4;i++)if(p->layer_eq[k][i]<-12 || p->layer_eq[k][i]>12)return 0;
    }
    if(p->version>=5)for(i=0;i<4;i++){
        unsigned id=amb_master_macros[i];
        for(unsigned j=0;j<4;j++)if(p->grain[i][j]<(j==2?-12:0) || p->grain[i][j]>(j==2?12:100))return 0;
        if(p->master_anchor[i]>GP[id].max-GP[id].min)return 0;
    }
    if(p->policy.variation>100) return 0;
    for(i=0;i<3;i++) if(p->policy.active[i]>=amb_style_count(i) ||
        (p->policy.pending[i]!=255 && p->policy.pending[i]>=amb_style_count(i))) return 0;
    for(i=0;i<4;i++) if(p->eq[i]<-12 || p->eq[i]>12 || p->evolve[i]>100) return 0;
    for(i=0;i<G_COUNT;i++) if(p->global[i]<GP[i].min || p->global[i]>GP[i].max) return 0;
    for(k=0;k<4;k++) {
        const amb_layer_t *l=&p->layer[k];
        if(l->role>6 || l->patch>=amb_patch_count(l->role) || !eng_ok(l->engine) || l->level>127 || l->generated>1 ||
           l->solo_plus1>4 || l->lfo_dest>=(p->version<5?4:ROUTE_COUNT) || l->lfo_depth>100 || l->speed>3 || l->mode>(l->role==0?AMB_CHANT:4u) || l->density>100 || l->swing>100 || l->fill>100 || l->mutation>100 || l->follow>100) return 0;
        if(!amb_legacy_texture(l) && !amb_legacy_glass(l) && l->engine!=(l->role==6?ENGI_AMB_SAMPLE:amb_recipe(l->role,l->patch)->engine))return 0;
        for(i=0;i<4;i++) if(l->macro[i]>127) return 0;
        for(i=0;i<16;i++) if(l->notes[i]>127 || l->velocity[i]>127) return 0;
        for(i=0;i<P_COUNT;i++) {
            const param_desc_t *d=param_desc_of(l->engine,i);
            if(l->base[i]<d->min || l->base[i]>d->max) return 0;
        }
    }
    return 1;
}
enum { AMB_EMPTY, AMB_SAVED, AMB_UNREADABLE };
static uint8_t amb_slot_state[AMB_SLOTS],amb_save_slot;
static amb_take_song_t amb_saved_song;
#define amb_snapshot amb_saved_song.piece
static unsigned amb_save_bytes;
static int amb_loadable(const amb_piece_t *p,int n)
{
    return ((n==1400 && p->version<=3) || (n==1420 && p->version==4) ||
        (n==(int)sizeof *p && p->version==AMB_VERSION)) && amb_valid(p);
}
static int amb_read_slot(unsigned slot)
{
    int n=st_load(amb_object(slot),&amb_pending_song,sizeof amb_pending_song);
    amb_load_take=n==(int)sizeof amb_pending_song && amb_take_valid(&amb_pending_song.take);
    return amb_load_take?(int)sizeof amb_pending:n;
}
static void amb_scan_slots(void)
{
    /* Boot scan uses the recall buffer before either transport or input is active. */
    for(unsigned i=0;i<AMB_SLOTS;i++){
        int n=amb_read_slot(i);
        amb_slot_state[i]=amb_loadable(&amb_pending,n)?AMB_SAVED:
            amb_occupied(i)?AMB_UNREADABLE:AMB_EMPTY;
    }
}
static unsigned amb_next_piece(unsigned p,int clicks)
{
    int dir=clicks<0?-1:1;
    while(clicks){
        do {p=amb_wrap((int)p+dir,NELEM(AMB_SCENE)+AMB_SLOTS);}
        while(p>=NELEM(AMB_SCENE) && amb_slot_state[p-NELEM(AMB_SCENE)]!=AMB_SAVED);
        clicks-=dir;
    }
    return p;
}
static void amb_commit_snapshot(void)
{
    unsigned slot=amb_save_slot-1;
    if(st_save(amb_object(slot),&amb_saved_song,amb_save_bytes))
        amb_say("SAVE FAILED - OLD COPY KEPT");
    else {amb_slot_state[slot]=AMB_SAVED;amb_say("SONG SAVED");}
    amb_save_slot=0;
}
static void amb_save(void)
{
#if FELUCCA_FLASH
    if(!flash_ok){amb_say("UNSUPPORTED FLASH - SAVE REFUSED");return;}
#endif
    if(amb_save_slot){amb_say("SAVE QUEUED UNTIL STOP");return;}
    for(unsigned k=0;k<4;k++)if(amb.layer[k].engine==ENGI_AMB_SAMPLE && amb.layer[k].base[P_E0]==AMB_MIC_SOURCE && !amb_current_take()->length){
        amb_say("RECORD A TAKE BEFORE SAVING");return;
    }
    if(amb_occupied(ambr.slot) && !ambr.overwrite){ambr.overwrite=1;amb_say("OCCUPIED - REC AGAIN");return;}
#if FELUCCA_FLASH
    uint32_t flags=irq_save();
#endif
    amb_snapshot=amb;
    amb_save_bytes=sizeof amb_snapshot;
    for(unsigned k=0;k<4;k++)if(amb.layer[k].engine==ENGI_AMB_SAMPLE && amb.layer[k].base[P_E0]==AMB_MIC_SOURCE && amb_current_take()->length){
        amb_saved_song.take=*amb_current_take();amb_save_bytes=sizeof amb_saved_song;break;
    }
#if FELUCCA_FLASH
    irq_restore(flags);
#endif
    if(!amb_valid(&amb_snapshot)){amb_say("INVALID STATE - SAVE REFUSED");return;}
    ambr.overwrite=0;amb_save_slot=ambr.slot+1;
    if(song.playing)amb_say("CAPTURED - WRITES ON STOP");
    else amb_commit_snapshot();
}
static int amb_stage_slot(unsigned slot)
{
    amb_pending_ready=0;
    memset(&amb_pending,0,sizeof amb_pending);
    int n=amb_read_slot(slot);
    if(!amb_loadable(&amb_pending,n)){amb_say("SLOT UNAVAILABLE");return 0;}
    if(amb_pending.version==1)amb_policy_init(&amb_pending.policy,0x414d4200u);
    if(amb_pending.version<3){amb_pending.layer[0].reserved=amb_pending.layer[1].reserved=0;}
    if(amb_pending.version<4){memset(amb_pending.layer_eq,0,sizeof amb_pending.layer_eq);amb_pending.release_ms=3000;}
    if(amb_pending.version<5){
        amb_anchor_master(&amb_pending);
        for(unsigned k=0;k<4;k++){amb_pending.grain[k][1]=50;amb_pending.grain[k][3]=20;}
    }
    amb_pending.version=AMB_VERSION;
    return 1;
}
static int amb_recall(unsigned slot)
{
    if(!amb_stage_slot(slot))return 0;
    if(song.playing){RING_PUBLISH();amb_pending_ready=1;amb_say("RECALL QUEUED TO NEXT BAR");}
    else {amb_install(&amb_pending);amb_say("RECALLED");}
    return 1;
}
static void amb_target(unsigned target)
{
    ambr.target=(uint8_t)(target%5);
    if(target) {ambr.edit_layer=(uint8_t)(target-1);song.sel=target-1;}
    amb_pickup_arm(&ambr.pickup,target?amb.layer[target-1].level*1023u/127u:amb.master);
    ambr.dirty=1;
}
static const param_desc_t *amb_param_desc(const amb_layer_t *l,unsigned id)
{
    /* The queued sound can differ from the sounding track until its tail
     * ends. Resolve mode-specific names from the saved destination. */
    const param_desc_t *d=0;
    if(id>=P_E0){
        if(l->engine==3)d=lofi_param_desc(l->base,id-P_E0);
        if(l->engine==11)d=noise_param_desc(l->base,id-P_E0);
    }
    return d?d:param_desc_of(l->engine,id);
}
static unsigned amb_sound_id(const amb_layer_t *l,unsigned bank,unsigned knob)
{
    static const uint8_t analog[2][4]={{P_E0,P_E1,P_E4,P_E5},{P_E2,P_E3,P_E6,P_E7}};
    static const uint8_t fm[2][4]={{P_E3,P_E2,P_E1,P_FCUT},{P_E6,P_GLIDE,P_E4,P_E0}};
    static const uint8_t filter[4]={P_FTYPE,P_FCUT,P_FRES,P_DIST};
    static const uint8_t play[4]={P_GLIDE,P_VOICE,P_TRANS,P_PAN};
    if(l->role==6){
        static const uint8_t ids[3][4]={{P_E4,P_E5,P_LN7,P_LN6},{P_E1,P_E2,P_E3,P_E6},{P_E6,P_FCUT,P_APDEP,P_REV}};
        return ids[bank%3][knob];
    }
    if(bank && ambr.detail==1){if(l->engine==0 && knob>0)return P_E3+knob;return filter[knob];}
    if(bank && ambr.detail==2){
        if(l->role==4){
            static const uint8_t lanes[4]={P_LN0+DV_KICK,P_LN0+DV_SNARE,P_LN0+DV_HATC,P_LN0+DV_TOM};
            return lanes[knob];
        }
        if(knob==1 && l->generated && l->mode==3 && l->role!=4 && l->role!=5)return P_CHOR;
        return play[knob];
    }
    if(l->engine==0)return analog[bank%2][knob];
    if(l->engine==ENGI_VOICE){
        static const uint8_t voice[2][4]={{P_E0,P_E4,P_E5,P_E3},{P_E1,P_E2,P_E6,P_E7}};
        return voice[bank%2][knob];
    }
    if(l->engine==12)return fm[bank%2][knob];
    unsigned id=P_E0+(bank%2)*4+knob;
    if(l->engine==3 && amb_param_desc(l,id)==&LOFI_NONE)return P_FCUT;
    return id;
}

static const uint8_t amb_master_fx[4][4]={
    {G_RSIZE,G_RFILT,G_RDAMP,G_RTYPE},{G_DMIX,G_DTIME,G_DFDBK,G_DTYPE},
    {G_CDEPTH,G_CRATE,G_WIDTH,G_RMOD},{G_DJF,G_CLIP,G_DCOLOR,G_DMOD}
};
static unsigned amb_fx_id(const amb_layer_t *l,unsigned knob)
{
    static const uint8_t ids[5][4]={
        {P_DTYPE,P_DIST,P_DTONE,P_FCUT},{P_CHOR,P_APRATE,P_APDEP,P_APWAVE},
        {P_DLY,P_FCUT,P_PAN,P_DIST},{P_REV,P_FCUT,P_PAN,P_CHOR},{P_PAN,P_APDEP,P_APRATE,P_APWAVE}
    };
    unsigned id=ids[ambr.fx%5][knob];
    return id==P_FCUT && l->engine==0?P_E4:id;
}
static unsigned amb_pattern_param(const amb_layer_t *l,unsigned knob)
{
    /* STEADY noise is held continuously. Its three performance controls
     * shape that held voice. Non-pitched noise cannot follow a chord. */
    if(l->generated && l->role==5 && !amb_legacy_texture(l)){
        if(l->mode==0 && knob==1)return l->engine==ENGI_AMB_SAMPLE?P_E2:P_E1;
        if(l->mode==0 && knob==2)return P_E6;
        if(knob==3)return P_FCUT;
    }
    if(l->generated && knob==3 && (l->role==0 || l->mode==3) && l->role!=4)return P_TRANS;
    return 0;
}
static const char *const amb_speeds[]={"1/16","1/8","1/4","1/2"};
static const char *const amb_drum_lanes[]={"KICK","SNARE","HATS","PERC"};
static void amb_manual_knob(amb_layer_t *l,unsigned knob,int delta)
{
    unsigned at=ambr.cursor;
    if(knob==0){if(l->role==4)ambr.cursor=(uint8_t)amb_wrap(at+delta,16);}
    else if(l->role==4){
        if(knob==1)ambr.lane=(uint8_t)amb_wrap(ambr.lane+delta,4);
        if(knob==2){
            uint16_t bit=(uint16_t)(1u<<at);
            if(delta>0)l->drums[ambr.lane]|=bit;else l->drums[ambr.lane]&=(uint16_t)~bit;
        }
        if(knob==3)l->swing=(uint8_t)clamp(l->swing+delta*2,0,100);
    }else if(knob==1)l->speed=(uint8_t)clamp(l->speed+delta,0,3);
    else if(knob==2){
        if(!l->notes[at] && delta>0)l->notes[at]=(uint8_t)ambr.voicing[ambr.edit_layer];
        else l->notes[at]=(uint8_t)clamp(l->notes[at]+delta,0,127);
        if(l->notes[at] && !l->velocity[at])l->velocity[at]=76;
    }else l->velocity[at]=(uint8_t)clamp(l->velocity[at]+delta,1,127);
}
static unsigned amb_envelope_id(const amb_layer_t *l,unsigned knob)
{
    if(l->role==4){
        static const uint8_t hit[4]={P_E3,P_E2,P_E5,P_E4};
        return hit[knob];
    }
    if(l->role==6 && l->base[P_E1]==3){
        static const uint8_t one_shot[4]={P_E4,P_E5,P_LN7,P_LN6};
        return one_shot[knob];
    }
    return P_ATK+knob;
}
enum { THEME_DIETER, THEME_TE_DARK, THEME_TE_LIGHT, THEME_COUNT };
static const char *const theme_names[]={"Dieter","Teenage Engineering / Dark","Teenage Engineering / Light"};
/* Global PER4 settings, spare engine-15 byte between hint counters and CPU settings. */
static unsigned amb_theme(void)
{
    unsigned value=favorites.factory[15][14];
    return value<THEME_COUNT?value:THEME_DIETER;
}
static void amb_theme_turn(int delta)
{
    unsigned theme=amb_wrap((int)amb_theme()+delta,THEME_COUNT);
    if(theme!=amb_theme()){
        favorites.factory[15][14]=(uint8_t)theme;
        settings_save();
        ambr.dirty=1;
    }
}
static const char *amb_route_name(const amb_layer_t *l,unsigned route)
{
    static const char *const original[]={"FILTER","PITCH","PAN","VOLUME"};
    static const char *const other[]={"EQ LOW","EQ MID","EQ HIGH","EQ OUTPUT","PAT MODE","PAT SPEED","DENSITY","FOLLOW","SWING","FILL","MUTATE","STEP NOTE","VELOCITY"};
    if(route<4)return original[route];
    if(route<ROUTE_MACRO){
        unsigned id=amb_route_param[route-4];
        const param_desc_t *d=amb_param_desc(l,id);
        if(route>=ROUTE_MACRO-6){
            if(route>=ROUTE_MACRO-2)return l->role==6?(route==ROUTE_MACRO-2?"GAIN":"FADE"):0;
            if(l->role!=4)return 0;
            static const char *const lanes[]={"KICK","SNARE","HATS","TOM"};
            return lanes[route-(ROUTE_MACRO-6)];
        }
        return d->min<d->max?d->label:0;
    }
    if(route<ROUTE_EQ)return l->engine==ENGI_VOICE && route==ROUTE_MACRO?"VOWEL MACRO":AMB_MACRO[l->role][route-ROUTE_MACRO];
    if(route>=ROUTE_GRAIN)return amb_grain_label[route-ROUTE_GRAIN];
    if(route>=ROUTE_MODE){
        if(route==ROUTE_NOTE || route==ROUTE_VEL){if(l->generated || l->role==4)return 0;}
        else if(route==ROUTE_SWING){if(l->role!=4)return 0;}
        else if(route==ROUTE_FILL || route==ROUTE_MUTATE){if(l->role!=4 || !l->generated)return 0;}
        else if(route==ROUTE_SPEED){if(l->role==4)return 0;}
        else if(!l->generated || (route!=ROUTE_DENSITY && l->role==4))return 0;
    }
    return other[route-ROUTE_EQ];
}
static const char *const amb_route_groups[]={"Basic","Instrument","Envelope","Filter","Drive","Space","Play","LFO","Macros","EQ","Pattern","Grain"};
static unsigned amb_route_group(unsigned r)
{
    if(r<4)return 0;
    if(r>=ROUTE_GRAIN)return 11;
    if(r>=ROUTE_MODE)return 10;
    if(r>=ROUTE_EQ)return 9;
    if(r>=ROUTE_MACRO)return 8;
    unsigned id=amb_route_param[r-4];
    if(r<12 || r>=ROUTE_MACRO-6)return 1;
    if(id>=P_ATK && id<=P_REL)return 2;
    switch(id){
    case P_FTYPE:case P_FRES:return 3;
    case P_DIST:case P_DTYPE:case P_DTONE:return 4;
    case P_CHOR:case P_DLY:case P_REV:case P_APDEP:case P_APRATE:case P_APWAVE:return 5;
    case P_LRATE:case P_LWAVE:return 7;
    default:return 6;
    }
}
static unsigned amb_group_turn(const amb_layer_t *l,int delta)
{
    unsigned group=amb_route_group(l->lfo_dest),route=l->lfo_dest;
    int direction=delta<0?-1:1;
    while(delta){
        do{
            group=amb_wrap((int)group+direction,NELEM(amb_route_groups));
            for(route=0;route<ROUTE_COUNT;route++)
                if(amb_route_group(route)==group && amb_route_name(l,route))break;
        }while(route==ROUTE_COUNT);
        delta-=direction;
    }
    return route;
}
static unsigned amb_route_turn(const amb_layer_t *l,int delta)
{
    unsigned route=l->lfo_dest;
    int direction=delta<0?-1:1;
    while(delta){
        do {route=amb_wrap((int)route+direction,ROUTE_COUNT);}while(!amb_route_name(l,route) || amb_route_group(route)!=amb_route_group(l->lfo_dest));
        delta-=direction;
    }
    return route;
}
static void amb_knob(unsigned knob,int delta)
{
    amb_layer_t *l=&amb.layer[ambr.edit_layer]; unsigned id=0;

    if(ambr.local_preset || ambr.reset_pending || ambr.piece_preview!=255 || ambr.help)return;
    if(ambr.view==4){
        amb_layer_t *part=&amb.layer[knob];
        int level=clamp(part->level+delta*2,0,127);
        if(level!=part->level && ambr.target==knob+1)amb_pickup_arm(&ambr.pickup,level*1023u/127u);
        part->level=(uint8_t)level;ambr.dirty=1;return;
    }
    if(ambr.view==0 || (ambr.view==1 && ambr.page==0)) {
        if(ambr.target==0 && ambr.view==0) {

            id=amb_master_macros[knob];amb.global[id]=(int16_t)clamp(amb.global[id]+(knob==3?-delta*2:delta*4),GP[id].min,GP[id].max);
        } else {
            if(ambr.view==0) l=&amb.layer[ambr.target-1];
            l->macro[knob]=(uint8_t)clamp(l->macro[knob]+delta*2,0,127);
        }
    } else if(ambr.view==1) {
        if(ambr.page==1) id=amb_sound_id(l,ambr.bank,knob);
        else if(ambr.page==2) id=amb_envelope_id(l,knob);
        else if(ambr.page==3) {
            if(knob==0)id=P_LWAVE;
            else if(knob==1)id=P_LRATE;
            else if(knob==2)l->lfo_depth=(uint8_t)clamp(l->lfo_depth+delta*2,0,100);
            else l->lfo_dest=amb_route_turn(l,delta);
        } else if(ambr.page==4) {
            id=amb_pattern_param(l,knob);
            if(!l->generated)amb_manual_knob(l,knob,delta);
            else if(l->role==4) {
                uint8_t *v0=knob==0?&l->density:knob==1?&l->swing:knob==2?&l->fill:&l->mutation;
                *v0=(uint8_t)clamp(*v0+delta*2,0,100);
            } else if(knob==0){
                l->mode=(uint8_t)amb_wrap(l->mode+delta,l->role==0?AMB_CHANT+1:5);
                ambr.grid_cue[ambr.edit_layer]=(amb_grid_cue_t){0};
            }
            else if(!id && knob==1) l->speed=(uint8_t)clamp(l->speed+delta,0,3);
            else if(!id && knob==2) l->density=(uint8_t)clamp(l->density+delta*2,0,100);
            else if(!id)l->follow=(uint8_t)clamp(l->follow+delta*2,0,100);
        } else if(ambr.page==6)amb.layer_eq[ambr.edit_layer][knob]=(int8_t)clamp(amb.layer_eq[ambr.edit_layer][knob]+delta,-12,12);
        else if(ambr.page==5) {
            if(ambr.fx==5){
                int v=amb.grain[ambr.edit_layer][knob]+delta*(knob==2?1:2);
                amb.grain[ambr.edit_layer][knob]=clamp(v,knob==2?-12:0,knob==2?12:100);
            }else if(ambr.fx==2 && knob==3){
                amb.global[G_DTYPE]=param_turn(&GP[G_DTYPE],amb.global[G_DTYPE],delta);
            }else id=amb_fx_id(l,knob);
        }
        if(id) {const param_desc_t *d=amb_param_desc(l,id);l->base[id]=(int16_t)param_turn(d,l->base[id],delta);}
    } else if(ambr.view==2) {
        if(ambr.page==5)return;
        if(ambr.page==7){
            if(timing_tab)amb_lane_adjust(timing_tab-1,knob,delta);
            else if(knob)amb_walk_adjust(knob,delta);
            else amb_walk_clock(delta>0?WALK_SECONDS:WALK_BARS,amb_walk_value(0)!=0);
            return;
        }
        if(ambr.page==6){
            if(knob==3)amb_walk_clock(amb_walk_unit(),delta>0);
            if(knob==2 && amb_crossfade()!=(delta>0)){
                favorites.factory[15][13]^=1;settings_save();ambr.dirty=1;
            }
            if(knob==1)amb.release_ms=(uint16_t)clamp((int)amb.release_ms+delta*250,250,AMB_MAX_TAIL_MS);
            if(knob==0 && amb_endless()!=(delta>0)){
                favorites.factory[15][13]^=2;
                if(!amb_endless() && travel.ready)amb_landscape_turn(0);
                settings_save();ambr.dirty=1;
            }
            return;
        }
        if(ambr.page==0) amb.evolve[knob]=(uint8_t)clamp(amb.evolve[knob]+delta*3,0,100);
        else if(ambr.page==1) {
            unsigned old_key=amb.key,old_pool=amb_pool();
            if(knob==0) amb.key=(uint8_t)amb_wrap(amb.key+delta,12);
            else if(knob==1){amb.scale=(uint8_t)amb_wrap(amb.scale+delta,4);amb.layer[0].reserved=amb.layer[1].reserved=0;}
            else if(knob==2) amb.global[G_BPM]=(int16_t)clamp(amb.global[G_BPM]+delta,40,140);
            else return;
            if(knob<2 && (amb.key!=old_key || amb_pool()!=old_pool))amb_revoice();
        } else if(ambr.page==2) {
            if(knob<3) {
                unsigned cur=ambr.gen_edit[knob]?ambr.gen_candidate[knob]:amb.policy.pending[knob]!=255?amb.policy.pending[knob]:amb.policy.active[knob];
                ambr.gen_candidate[knob]=(uint8_t)amb_wrap((int)cur+delta,amb_style_count(knob));
                ambr.gen_edit[knob]=1;ambr.gen_time[knob]=fm1_ms;
            } else amb.policy.variation=(uint8_t)clamp(amb.policy.variation+delta*2,0,100);
        } else if(ambr.page==3) {
            const param_desc_t *d;id=amb_master_fx[ambr.fx%4][knob];d=&GP[id];
            amb.global[id]=(int16_t)param_turn(d,amb.global[id],delta);
            if(id==G_DFDBK && amb.global[id]>86) amb.global[id]=86;
        } else amb.eq[knob]=(int8_t)clamp(amb.eq[knob]+delta,-12,12);
    }
    if(ambr.view==1 && id)amb_base_revision[ambr.edit_layer]++;
    ambr.dirty=1;
}
static int amb_sound_picker(void)
{
    return ambr.view==1 && ambr.page==1 && ambr.bank==0;
}
#include "amb_page_presets.h"
static int amb_mic_page(void)
{
    return amb_sound_picker() && amb.layer[ambr.edit_layer].role==6 && amb.layer[ambr.edit_layer].base[P_E0]==AMB_MIC_SOURCE;
}
static void amb_end_capture(void)
{
    int ok=amb_take_finish();amb_capture_page=amb_replace_take=0;
    amb_say(ok?(amb_current_take()->period_q8?"TAKE READY - PITCH MATCHED":"TAKE READY - ROOT C4"):"NO INPUT - TAKE KEPT");
}
static void amb_rec(void)
{
    if(ambr.local_preset){amb_apply_page_preset();return;}
    if(ambr.reset_pending){
        unsigned k=ambr.edit_layer,solo=amb.layer[0].solo_plus1;uint8_t p0=amb.layer[0].reserved,p1=amb.layer[1].reserved;
        ambr.loading=1;amb_begin_change(k);amb.layer[k]=amb_reset_layer[k];memcpy(amb.grain[k],amb_reset_grain[k],4);memcpy(amb.layer_eq[k],amb_reset_eq[k],4);
        amb.layer[0].solo_plus1=(uint8_t)solo;amb.layer[0].reserved=p0;amb.layer[1].reserved=p1;
        if(!ambr.handoff[k])amb_apply_patch(k);
        amb_travel_rebase(k);ambr.recent_valid[k]=0;ambr.reset_pending=0;ambr.loading=0;
        if(ambr.target==k+1)amb_pickup_arm(&ambr.pickup,amb.layer[k].level*1023u/127u);
        amb_say("TRACK RESTORED TO LOADED PIECE");return;
    }
    if(ambr.view==2 && ambr.page==2)return;
    if(ambr.view==3) {amb_save();return;}
    if(ambr.view==1 && ambr.page==4) {
        amb_layer_t *l=&amb.layer[ambr.edit_layer];
        if(!ambr.recording && l->generated && l->role!=4)amb_capture_generated(ambr.edit_layer);
        ambr.recording=!ambr.recording;l->generated=0;ambr.dirty=1;
        amb_say(ambr.recording?"WHITE KEYS TOGGLE STEPS":"RECORDING STOPPED");return;
    }
    if(ambr.piece_preview!=255) {
        unsigned p=ambr.piece_preview;ambr.preset_position=(uint8_t)p;
        if(p<NELEM(AMB_SCENE)){
            amb_load_take=0;amb_pending_ready=0;amb_make_scene(&amb_pending,p);
            /* A confirmed factory recall is a new take. A slot recall keeps
             * its stored seed. The default at power-on remains reproducible. */
            if(p!=AMB_SCENE_INN)amb_pending.policy.seed=amb_hash(amb_pending.policy.seed+fm1_ms+0x9e3779b9u*(++ambr.factory_take));
            if(song.playing){RING_PUBLISH();amb_pending_ready=1;amb_say("RECALL QUEUED TO NEXT BAR");}
            else amb_start_factory(p);
        }else amb_recall(p-NELEM(AMB_SCENE));
        ambr.piece_preview=255;return;
    }
    if(amb_mic_page() && ambr.proposal==255 && !amb_pending_ready){
        if(amb_capture_page){amb_end_capture();return;}
        if(amb_current_take()->length && !amb_replace_take){
            amb_replace_take=1;amb_say("REPLACE TAKE? REC AGAIN");return;
        }
        amb_replace_take=0;amb_take_start();amb_input_seen=0;amb_input_ms=fm1_ms;amb_capture_page=1;
        amb_say("SING NOW / REC TO FINISH");return;
    }
    if(ambr.proposal==255) {if(amb_pending_ready){amb_pending_ready=0;amb_say("QUEUED RECALL CANCELLED");}else if(ambr.chord_q!=255){ambr.chord_q=255;amb_say("QUEUED CHORD CANCELLED");}return;}

    if(ambr.view==1 && ambr.page==0) {
        amb_patch(ambr.edit_layer,ambr.proposal,0);
    } else if(amb_sound_picker()){
        amb_layer_t *l=&amb.layer[ambr.edit_layer];
        if(l->role==6){
            amb_sound_change(ambr.edit_layer);
            l->base[P_E0]=ambr.proposal;
            if(!ambr.handoff[ambr.edit_layer])amb_apply_patch(ambr.edit_layer);
        }else amb_patch(ambr.edit_layer,l->role,ambr.proposal);
    } else if(ambr.view==0 || (ambr.view==2 && ambr.page==0)) {
        if(song.playing) ambr.chord_q=ambr.proposal;
        else {amb.chord=ambr.proposal;amb_revoice();}
    }
    if(ambr.view==1 && (ambr.page==0 || amb_sound_picker()))amb_travel_rebase(ambr.edit_layer);
    ambr.proposal=255;ambr.dirty=1;
}
static void amb_browse_proposal(int delta)
{
    if(amb_sound_picker()){
        const amb_layer_t *l=&amb.layer[ambr.edit_layer];
        unsigned current=l->role==6?(l->base[P_E0]):l->patch;
        unsigned count=l->role==6?NELEM(N_AS_SOURCE):amb_patch_count(l->role);
        unsigned next=amb_wrap((ambr.proposal==255?current:ambr.proposal)+delta,count);
        ambr.proposal=(next==current)?255:(uint8_t)next;
    }else if(ambr.view==1 && ambr.page==0) {
        unsigned cur=ambr.proposal==255?amb.layer[ambr.edit_layer].role:ambr.proposal;
        ambr.proposal=(uint8_t)amb_wrap((int)cur+delta,7);
        if(ambr.proposal==amb.layer[ambr.edit_layer].role) ambr.proposal=255;
    } else if(ambr.view==0 || (ambr.view==2 && ambr.page==0)) {
        unsigned cur=ambr.proposal==255?amb.chord:ambr.proposal;
        ambr.proposal=(uint8_t)amb_wrap((int)cur+delta,7);
        if(ambr.proposal==amb.chord) ambr.proposal=255;
    }
}
static void amb_input(void)
{
    if(amb_capture_page){
        if(take_rec.frames!=amb_input_seen){amb_input_seen=take_rec.frames;amb_input_ms=fm1_ms;}
        if(!take_rec.running || fm1_ms-amb_input_ms>(amb_input_seen?750u:10000u))amb_end_capture();
    }
    if(amb_save_slot && !song.playing)amb_commit_snapshot();
    uint32_t pressed=fm1_input_edges(0),notes=fm1_input_note_edges();
    unsigned b,k; int s,sel=panel_enc(EN_SELECT),algo=panel_enc(EN_ALGO),pre=panel_enc(EN_PRESET);
    if(algo && ambr.view==0)amb_auto_reset();else amb_auto_tick();
    unsigned home=(fm1_in.buttons>>panel.btn[B_HOME])&1u,play=(fm1_in.buttons>>panel.btn[B_PLAY])&1u;
    static uint8_t home0,play0;
    for(k=0;k<3;k++) if(ambr.gen_edit[k] && fm1_ms-ambr.gen_time[k]>=90u) {
        amb_policy_choose(&amb.policy,k,ambr.gen_candidate[k],song.playing);
        amb_anchor_patterns();
        ambr.gen_edit[k]=0;ambr.dirty=1;
    }
    if(home && !home0) ambr.home_used=0;
    if(play && !play0) {ambr.play_t=fm1_ms;ambr.play_used=0;}
    if(play && !ambr.play_used && fm1_ms-ambr.play_t>=800u) {
        ambr.panic=1;transport_req=2;ambr.play_used=1;amb_say("PANIC - OUTPUT FADING");
    }
    if(!play && play0 && !ambr.play_used) {
        ambr.panic=0;transport_req=song.playing?2:1;ambr.dirty=1;
    }
    /* Help remembers its underlying page. Consume encoder events while
     * reading it, including the frame that closes the overlay. */
    if((home && (pressed&(1u<<panel.btn[B_EDIT]))) || ambr.help){
        if(home && (pressed&(1u<<panel.btn[B_EDIT]))){ambr.help^=1;ambr.home_used=1;}
        else if(!home && home0 && !ambr.home_used)ambr.help=0;
        for(k=0;k<4;k++)(void)panel_enc(EN_K1+k);
        home0=(uint8_t)home;play0=(uint8_t)play;ambr.dirty|=(pressed!=0 || !ambr.help);
        kb_mask=perf_mask=0;song.rec=0;song.grid=2;return;
    }
    if((pressed & ~(1u<<panel.btn[B_REC] | 1u<<panel.btn[B_PLAY])) || sel || algo)ambr.local_preset=0;
    if(ambr.view)ambr.zen=0;
    if(home && sel) {
        ambr.home_used=1;
        if(ambr.view==1) {ambr.edit_layer=(uint8_t)amb_wrap(ambr.edit_layer+sel,4);song.sel=ambr.edit_layer;ambr.proposal=255;ambr.dirty=1;}
        sel=0;
    }
    if(!home && home0 && !ambr.home_used) {
        ambr.zen=0;
        ambr.view=0;ambr.recording=0;ambr.reset_pending=0;ambr.overwrite=0;amb_pending_ready=0;
        if(ambr.target){ambr.edit_layer=ambr.target-1;song.sel=ambr.edit_layer;}ambr.proposal=ambr.piece_preview=255;ambr.dirty=1;
    }
    home0=(uint8_t)home;play0=(uint8_t)play;
    for(b=0;b<NB;b++) if((pressed>>panel.btn[b])&1u) {
        if(b==B_HOME || b==B_PLAY) continue;
        if(home && b==B_ARP){
            ambr.home_used=1;ambr.zen=!ambr.zen;ambr.view=0;ambr.proposal=ambr.piece_preview=255;
            ambr.recording=ambr.reset_pending=ambr.overwrite=0;ambr.dirty=1;
        }else if(ambr.zen && b==B_SCL && !home){
            ambr.zen_train^=1;ambr.dirty=1;
        }else if(home && b==B_FX){
            ambr.home_used=1;amb_monitor_requested^=1;
            amb_say(amb_monitor_requested?"ALL FX BYPASSED":"EFFECTS ON");
        }else if((home && b==B_SEQ) || (b==B_SCL && (ambr.view==0 || ambr.view==4))){
            ambr.view=ambr.view==4?0:4;ambr.home_used=(uint8_t)home;
            ambr.proposal=ambr.piece_preview=255;ambr.reset_pending=ambr.recording=ambr.overwrite=0;
            for(k=0;k<4;k++)trk[k].peak=0;
        }else if(home && (b==B_OCTDN || b==B_OCTUP)) {
            ambr.home_used=1;
            unsigned target=b==B_OCTDN && ambr.view==1?ambr.edit_layer+1:ambr.target;
            if(target) {
                k=target-1;
                if(b==B_OCTDN){amb.layer[k].base[P_MUTE]=!amb.layer[k].base[P_MUTE];amb_say(amb.layer[k].base[P_MUTE]?"LAYER MUTED":"LAYER ON");if(amb.layer[k].base[P_MUTE])amb_release(k);else ambr.foundation_pending[k]=(uint8_t)(amb.layer[k].role<2 || amb.layer[k].role==5);}
                else {unsigned j;ambr.solo=ambr.solo==(int)k?-1:(int8_t)k;amb.layer[0].solo_plus1=(uint8_t)(ambr.solo+1);
                    for(j=0;j<4;j++)if(ambr.solo>=0 && j!=(unsigned)ambr.solo)amb_release(j);
                    else ambr.foundation_pending[j]=(uint8_t)(amb.layer[j].role<2 || amb.layer[j].role==5);
                }
            }
        } else if(b==B_EDIT) {
            if(ambr.view==2 && ambr.page>=6){ambr.page=ambr.page==6?7:6;timing_tab=0;}
            else if(ambr.view==1 && ambr.page==0){ambr.reset_pending^=1;ambr.proposal=255;}
            else if(ambr.view==1 && ambr.page==1){ambr.proposal=255;ambr.bank=(uint8_t)((ambr.bank+1)%(amb.layer[ambr.edit_layer].role==6?3:2));}
            else if(ambr.view==1 && ambr.page==4) {amb_set_generated(ambr.edit_layer,!amb.layer[ambr.edit_layer].generated);}
            else {ambr.view=ambr.target?1:2;ambr.page=ambr.target?0:3;ambr.bank=0;ambr.detail=0;ambr.proposal=ambr.piece_preview=255;}
        } else if(b==B_GLO) {ambr.view=2;ambr.page=0;ambr.recording=0;ambr.reset_pending=0;ambr.proposal=ambr.piece_preview=255;}
        else if(b==B_SCL) {
            if(ambr.view==1 || ambr.view==2) {ambr.reset_pending=0;ambr.page=ambr.view==1?(uint8_t)((ambr.page+1)%7):amb_global_order[(amb_global_index(ambr.page)+1)%NELEM(amb_global_order)];ambr.recording=0;ambr.bank=0;ambr.detail=0;ambr.proposal=255;}
        } else if(b==B_SAVE) {ambr.reset_pending=0;ambr.view=3;ambr.slot=0;ambr.overwrite=0;ambr.recording=0;ambr.proposal=ambr.piece_preview=255;}
        else if(b==B_REC) amb_rec();
        else if(b==B_ARP && ambr.view==1 && ambr.page==4 && amb.layer[ambr.edit_layer].role!=4 && !amb.layer[ambr.edit_layer].generated){
            amb.layer[ambr.edit_layer].notes[ambr.cursor]=0;amb_say("SELECTED STEP CLEARED");
        }
        else if(b==B_OCTDN || b==B_OCTUP){
            if(song.playing || ambr.view==0 || ambr.view==4)amb_queue_octave(b==B_OCTUP?1:-1);
            else song.octave=(int8_t)clamp(song.octave+(b==B_OCTUP?1:-1),-3,3);
        }
        else if(b==B_ENV || b==B_LFO || b==B_SEQ || b==B_FX || b==B_ARP) {
            if(ambr.target && b==B_SEQ && ambr.view==1 && ambr.page==4)amb_set_generated(ambr.edit_layer,!amb.layer[ambr.edit_layer].generated);
            else if(ambr.target) {ambr.reset_pending=0;ambr.recording=0;ambr.view=1;ambr.page=b==B_ENV?2:b==B_LFO?3:b==B_FX?5:4;ambr.proposal=ambr.piece_preview=255;}
        }
        ambr.dirty=1;
    }
    if(sel) {
        ambr.reset_pending=0;
        if(ambr.piece_preview!=255) ambr.piece_preview=(uint8_t)amb_next_piece(ambr.piece_preview,sel);
        else if(ambr.proposal!=255){ /* ALGORITHM owns this proposal until REC or navigation. */ }
        else if(ambr.view==0) amb_target(amb_wrap(ambr.target+sel,5));
        else if(ambr.view==3) {ambr.slot=(uint8_t)amb_wrap(ambr.slot+sel,8);ambr.overwrite=0;}
        else if(ambr.view==1) {
            amb_layer_t *l=&amb.layer[ambr.edit_layer];
            if(ambr.page==1) {
                if(!amb_sound_picker() && l->role!=6)ambr.detail=(uint8_t)amb_wrap(ambr.detail+sel,3);
            }
            else if(ambr.page==3)l->lfo_dest=amb_group_turn(l,sel);
            else if(ambr.page==4) {
                if(l->role==4) ambr.lane=(uint8_t)amb_wrap(ambr.lane+sel,4);
                else ambr.cursor=(uint8_t)amb_wrap(ambr.cursor+sel,16);
            } else if(ambr.page==5) ambr.fx=(uint8_t)amb_wrap(ambr.fx+sel,6);
        } else if(ambr.view==2) {
            if(ambr.page==0) amb.auto_index=(uint8_t)amb_wrap(amb.auto_index+sel,8);
            else if(ambr.page==3) ambr.fx=(uint8_t)amb_wrap(ambr.fx+sel,4);
            else if(ambr.page==5) amb_theme_turn(sel);
            else if(ambr.page==6)amb_walk_adjust(4,sel);
            else if(ambr.page==7)timing_tab=amb_wrap(timing_tab+sel,3);
            /* SELECT deliberately inert on GENERATE. */
        }
        ambr.dirty=1;
    }
    if(algo && ambr.piece_preview==255){
        ambr.reset_pending=0;
        if(ambr.view==0){amb_landscape_turn(algo);travel.turned_ms=fm1_ms;travel.indicator=1;}
        else amb_browse_proposal(algo);
        ambr.dirty=1;
    }
    if(pre && amb_page_bank()){
        ambr.reset_pending=0;ambr.proposal=255;
        ambr.local_preset=1+amb_wrap((ambr.local_preset?ambr.local_preset-1:pre>0?-1:0)+pre,3);
        ambr.dirty=1;
    }else if(pre && ambr.view==0) {
        ambr.reset_pending=0;
        ambr.piece_preview=(uint8_t)amb_next_piece(ambr.piece_preview==255?ambr.preset_position:ambr.piece_preview,pre);
        ambr.proposal=255;ambr.recording=0;ambr.view=0;ambr.dirty=1;
    }
    for(k=0;k<4;k++) if((s=panel_enc(EN_K1+k))) amb_knob(k,s);
    kb_mask=0;perf_mask=0;perf_solo=0;
    if(ambr.view==0 && ambr.piece_preview==255 && ambr.proposal==255){
        for(k=0;k<27;k++)if((notes>>k)&1u){
            unsigned rel=(53u+k+12u-amb.key)%12u;
            if(rel){unsigned next=amb_pool()^(1u<<rel),count=0,j;for(j=0;j<12;j++)count+=(next>>j)&1u;
                if(count>=3){amb_pool_set(next);amb_revoice();amb_say("SHARED NOTE POOL UPDATED");}
                else amb_say("KEEP AT LEAST THREE NOTES");}
            else amb_say("TONIC STAYS IN NOTE POOL");
        }
    }
    if(ambr.view==1 && ambr.page==4 && ambr.recording) {
        amb_layer_t *l=&amb.layer[ambr.edit_layer];
        for(k=0;k<27;k++) if((notes>>k)&1) {
            if(key_black(k))continue;
            unsigned at=key_place(k);
            ambr.cursor=(uint8_t)at;
            if(l->role==4)l->drums[ambr.lane]^=(uint16_t)(1u<<at);
            else if(l->notes[at])l->notes[at]=0;
            else{
                l->notes[at]=(uint8_t)clamp(ambr.voicing[ambr.edit_layer],1,127);
                if(!l->velocity[at])l->velocity[at]=88;
            }
            ambr.dirty=1;
        }
        /* Step entry is silent on pitched and drum layers, even during PLAY. */
    }
    song.grid=(ambr.view==0 || ambr.view==4 || (ambr.view==1 && ambr.page==4) || amb_tail_releasing(ambr.edit_layer))?2:0;
    if(!amb_mic_page() || ambr.proposal!=255 || ambr.help){
        take_rec.running=amb_capture_page=amb_replace_take=0;
    }
    song.rec=0;
}
static int amb_rec_action(void)
{
    if(ambr.local_preset || ambr.reset_pending || amb_mic_page())return 1;
    if(ambr.view==2 && ambr.page==2)return 0;
    return amb_pending_ready || ambr.proposal!=255 || ambr.chord_q!=255 || ambr.piece_preview!=255 || ambr.view==3 || (ambr.view==1 && ambr.page==4);
}
static void amb_leds(void)
{
    uint8_t lit[FM1_NCOL]={0},dim[FM1_NCOL]={0},breath[FM1_NCOL]={0},hi[FM1_NCOL]={0};unsigned b,k;
    int pcs[4],nt=amb_chord_tones(amb_pool(),amb.key,amb.chord,pcs);
    led_pos_init();
    for(b=0;b<NB;b++) {
        int edit_action=(ambr.view==1 && (ambr.page==0 || ambr.page==1 || ambr.page==4)) ||
            (ambr.target?(ambr.view!=1 || ambr.page!=0):(ambr.view!=2 || ambr.page!=3)) || ambr.proposal!=255 || ambr.piece_preview!=255;
        int action=(b==B_FX && ((fm1_in.buttons>>panel.btn[B_HOME])&1u)) || b==B_PLAY || (b==B_SAVE && (ambr.view!=3 || ambr.slot || ambr.overwrite)) ||
            (b==B_GLO && (ambr.view!=2 || ambr.page!=0 || ambr.proposal!=255 || ambr.piece_preview!=255)) ||
            (b==B_EDIT && edit_action) || b==B_OCTDN || b==B_OCTUP ||
            (b==B_HOME && (ambr.target || ambr.view || ambr.proposal!=255 || ambr.piece_preview!=255 || amb_pending_ready)) || (b==B_SCL && ambr.view!=3) ||
            (b==B_REC && amb_rec_action()) || (ambr.target && (b==B_ENV || b==B_LFO || b==B_SEQ || b==B_FX || (b==B_ARP && (!(ambr.view==1 && ambr.page==4) || (!amb.layer[ambr.edit_layer].generated && amb.layer[ambr.edit_layer].notes[ambr.cursor])))));
        if((fm1_in.buttons>>panel.btn[B_HOME])&1u && b==B_ARP)action=1;
        if(ambr.zen && b==B_HOME)action=1;
        if(ambr.help)action=b==B_HOME || b==B_PLAY;
        if(action) led_put(lit,panel.btn[b],1);
    }
    if(!ambr.help && ambr.recording) led_put(lit,panel.btn[B_REC],1);
    else if(!ambr.help && amb_rec_action() && (ambr.local_preset || amb_pending_ready || ambr.proposal!=255 || ambr.chord_q!=255 || ambr.piece_preview!=255 || ambr.overwrite || ambr.reset_pending)) {led_clear(lit,panel.btn[B_REC]);led_put(breath,panel.btn[B_REC],1);}
    for(k=0;k<27;k++) {
        if(ambr.help)break;
        unsigned pc=(53+k+12-amb.key)%12;
        int scale=(amb_pool()>>pc)&1u;
        if(ambr.view==1 && ambr.page==4 && ambr.recording){
            if(!key_black(k)){
                const amb_layer_t *l=&amb.layer[ambr.edit_layer];
                unsigned at=key_place(k);
                int on=l->role==4?((l->drums[ambr.lane]>>at)&1):l->notes[at];
                led_put(on?lit:dim,14+k,1);
            }
            continue;
        }
        if(ambr.view==0 && scale)led_put(hi,14+k,1);
        if(ambr.view==0){int j;
            for(j=0;j<nt;j++)if((53u+k)%12u==(unsigned)pcs[j])led_put(lit,14+k,1);}

        if((fm1_in.notes>>k)&1) led_put(lit,14+k,1);

    }
    if(song.playing) lit[LED_PLAY_GREEN>>3]|=(uint8_t)(1u<<(LED_PLAY_GREEN&7u));
    for(k=0;k<FM1_NCOL;k++) {fm1_led[k]=lit[k];fm1_led_dim[k]=dim[k];fm1_led_breath[k]=breath[k];fm1_led_mid[k]=0;fm1_led_hi[k]=hi[k];}
}
#define TE_DARK (amb_theme()==THEME_TE_DARK)
#define TE_LIGHT (amb_theme()==THEME_TE_LIGHT)
#define ABG (TE_DARK?RGB(22,22,24):TE_LIGHT?RGB(245,242,234):TONO_PAPER)
#define AINK (TE_DARK?RGB(244,241,230):TONO_INK)
#define ADIM (TE_DARK?RGB(191,190,184):RGB(68,69,67))
#define ALINE (TE_DARK?RGB(76,76,80):RGB(174,175,171))
#define ASEL (TE_DARK?RGB(52,52,55):RGB(209,210,205))
#define AQUEUE (TE_DARK?TONO_ORANGE:RGB(145,54,9))
#define SCOPE_BG (TE_LIGHT?RGB(237,238,236):TONO_INK)
#define SCOPE_INK (TE_LIGHT?TONO_INK:TONO_PAPER)
static uint16_t amb_accent(unsigned n)
{
    static const uint16_t bright[]={RGB(255,105,57),RGB(114,188,214),RGB(224,130,175),RGB(239,190,95)};
    static const uint16_t vivid[]={RGB(255,66,32),RGB(0,148,246),RGB(245,53,146),RGB(255,167,0)};
    return (TE_LIGHT?vivid:bright)[n%4];
}

/* Small lettering uses a denser ink in the same hue as its indicator. */
static uint16_t amb_accent_text(unsigned n)
{
    static const uint16_t inks[]={RGB(202,45,8),RGB(0,99,189),RGB(186,27,111),RGB(147,82,0)};
    return TE_LIGHT?inks[n%4]:amb_accent(n);
}

static void field_curve(int x0,int y0,int x1,int y1,int floor,uint16_t fill)
{
    for(int x=x0;x<x1;x++){
        int y=y0+(y1-y0)*(x-x0)/(x1-x0);
        cv_rect(x,y,1,floor-y,fill);
    }
    cv_line(x0,y0,x1,y1,AINK);
}
static void amb_text(int x,int y,const char *s,uint16_t color,unsigned maxchars)
{
    char b[40];unsigned i=0;
    while(s[i] && i<maxchars && i<sizeof b-1) {b[i]=s[i];i++;}
    b[i]=0;cv_text(x,y+1-AF_TONO_LABEL_CAP_Y,&AF_TONO_LABEL,b,color);
}
static void amb_number(int x,int y,int n,uint16_t color)
{
    char b[24];if(n<0) {b[0]='-';fmt_int(b+1,-n);} else fmt_int(b,n);
    amb_text(x,y,b,color,10);
}
static void amb_knob_value(unsigned knob,char *label,char *value)
{
    amb_layer_t *l=&amb.layer[ambr.edit_layer]; const param_desc_t *d=0;int v=0;unsigned id=0;
    const char *unit=0;
    if(ambr.view==4){
        label[0]='L';label[1]=(char)('1'+knob);str_cpy(label+2," GAIN",14);v=amb.layer[knob].level*100/127;
    }else if(ambr.view==0 || (ambr.view==1 && ambr.page==0)) {
        if(ambr.view==0 && ambr.target==0) {
            static const char *const labels[4]={"SPACE","ECHO","MOTION","WARMTH"};

            str_cpy(label,labels[knob],16);v=amb.global[amb_master_macros[knob]];if(knob==3)v=-v*100/64;else if(GP[amb_master_macros[knob]].fmt==F_PCT)v=v*100/127;
        } else {
            if(ambr.view==0) l=&amb.layer[ambr.target-1];
            str_cpy(label,l->engine==ENGI_VOICE && knob==0?"VOWEL":AMB_MACRO[l->role][knob],16);v=l->macro[knob]*100/127;
        }
    } else if(ambr.view==1) {
        if(ambr.page==1) id=amb_sound_id(l,ambr.bank,knob);
        else if(ambr.page==2) id=amb_envelope_id(l,knob);
        else if(ambr.page==3) {
            const char *dest=amb_route_name(l,l->lfo_dest);
            if(knob==0)id=P_LWAVE;else if(knob==1)id=P_LRATE;
            else {str_cpy(label,knob==2?"DEPTH":"DEST",16);if(knob==2)v=l->lfo_depth;else {str_cpy(value,dest?dest:"INACTIVE",24);return;}}
        }
        else if(ambr.page==4) {
            static const char *const drums[4]={"DENSITY","SWING","FILL","MUTATE"};
            static const char *const pitched[4]={"MODE","SPEED","DENSITY","FOLLOW"};
            static const char *const modes[]={"UP","DOWN","WALK","CHORD","SPARSE","CHANT"};
            static const char *const texture[5]={"STEADY","PULSE","DRIFT","BURSTS","SPARSE"};
            id=amb_pattern_param(l,knob);
            if(!l->generated){
                if(!knob && l->role!=4){label[0]=value[0]=0;return;}
                static const char *const pitched_manual[]={"","SPEED","NOTE","VELOCITY"};
                static const char *const drum_manual[]={"STEP","LANE","HIT","SWING"};
                str_cpy(label,(l->role==4?drum_manual:pitched_manual)[knob],16);
                if(knob==0)v=ambr.cursor+1;
                else if(knob==1){str_cpy(value,l->role==4?amb_drum_lanes[ambr.lane]:amb_speeds[l->speed],24);return;}
                else if(knob==2 && l->role==4){str_cpy(value,((l->drums[ambr.lane]>>ambr.cursor)&1u)?"ON":"REST",24);return;}
                else if(knob==2 && !l->notes[ambr.cursor]){str_cpy(value,"REST",24);return;}
                else v=knob==2?l->notes[ambr.cursor]:l->role==4?l->swing:l->velocity[ambr.cursor];
            }else if(!id){
                str_cpy(label,l->role==4?drums[knob]:pitched[knob],16);
                if(l->role==4) v=knob==0?l->density:knob==1?l->swing:knob==2?l->fill:l->mutation;
                else if(knob==0) {str_cpy(value,l->role==5 && !amb_legacy_texture(l)?texture[l->mode]:modes[l->mode],24);return;}
                else if(knob==1) {str_cpy(value,amb_speeds[l->speed],24);return;}
                else v=knob==2?l->density:l->follow;
            }
        } else if(ambr.page==6){static const char *const labels[4]={"LOW","MID","HIGH","OUTPUT"};str_cpy(label,labels[knob],16);v=amb.layer_eq[ambr.edit_layer][knob];}
        else {
            if(ambr.fx==5){
                str_cpy(label,amb_grain_label[knob],16);
                v=amb.grain[ambr.edit_layer][knob];
                if(knob==1){v=(32+v)*8000/FS;unit="ms";}
                if(knob==2)unit="st";
            }else if(ambr.fx==2 && knob==3){d=&GP[G_DTYPE];v=amb.global[G_DTYPE];}
            else id=amb_fx_id(l,knob);
        }
        if(id) {d=amb_param_desc(l,id);v=l->base[id];}
        if(l->role==6 && ((ambr.page==1 && ambr.bank==0) || (ambr.page==2 && l->base[P_E1]==3)) && knob>=2) {str_cpy(label,knob==2?"GAIN":"FADE",16);d=0;v=l->base[id];}
    } else if(ambr.view==2) {
        if(ambr.page==0) {static const char *const labels[4]={"HARMONY","TIMBRE","RHYTHM","FX"};str_cpy(label,labels[knob],16);v=amb.evolve[knob];}
        else if(ambr.page==1) {
            if(knob==3){label[0]=value[0]=0;return;}
            static const char *const labels[]={"KEY","SCALE","TEMPO"};
            static const char *const scales[4]={"MINOR","MAJOR","DORIAN","LYDIAN"};str_cpy(label,labels[knob],16);
            if(knob==0) {str_cpy(value,N_NOTE[amb.key],24);return;}
            if(knob==1) {str_cpy(value,scales[amb.scale],24);return;}
            v=amb.global[G_BPM];
        } else if(ambr.page==7){
            if(timing_tab){
                static const char *const labels[]={"ACTIVE","CLOCK","MIN","MAX"};
                unsigned lane=timing_tab-1,unit=amb_lane_value(lane,1);
                str_cpy(label,labels[knob],16);
                if(!knob)str_cpy(value,amb_lane_value(lane,0)?"On":"Off",24);
                else if(knob==1)str_cpy(value,lane_units[unit],24);
                else if(!unit || unit==3)str_cpy(value,"-",24);
                else fmt_int(value,amb_lane_value(lane,knob));
                return;
            }
            str_cpy(label,amb_walk_labels[knob],16);
            if(!knob){str_cpy(value,amb_walk_clocks[amb_walk_unit()],24);return;}
            v=amb_walk_value(knob);
        } else if(ambr.page==6){
            if(!knob){str_cpy(label,"TRAVEL",16);str_cpy(value,amb_endless()?"Endless":"Bounded",24);return;}
            if(knob==2){str_cpy(label,"HANDOFF",16);str_cpy(value,amb_crossfade()?"Crossfade":"Release",24);return;}
            if(knob==3){str_cpy(label,"AUTO",16);str_cpy(value,amb_walk_value(0)?"On":"Off",24);return;}
            str_cpy(label,"TAIL TIME",16);
            static const char *const fraction[]={" s",".25 s",".5 s",".75 s"};
            fmt_int(value,amb.release_ms/1000);
            str_cpy(value+str_len(value),fraction[amb.release_ms%1000/250],12);
            return;
        } else if(ambr.page==2) {
            static const char *const labels[4]={"RHYTHM","PHRASE","INTERPLAY","VARIATION"};str_cpy(label,labels[knob],16);
            v=knob==3?amb.policy.variation:ambr.gen_edit[knob]?ambr.gen_candidate[knob]+1:amb.policy.pending[knob]!=255?amb.policy.pending[knob]+1:amb.policy.active[knob]+1;
        } else if(ambr.page==3) {
            id=amb_master_fx[ambr.fx%4][knob];d=&GP[id];v=amb.global[id];
        } else {static const char *const labels[4]={"LOW","MID","HIGH","OUTPUT"};str_cpy(label,labels[knob],16);v=amb.eq[knob];}
    }
    if(d) {str_cpy(label,d->label,16);param_format(d,v,value,&unit);if(unit && str_len(value)+str_len(unit)<10)str_cpy(value+str_len(value),unit,24-str_len(value));}
    else {if(v<0) {value[0]='-';fmt_int(value+1,-v);}else fmt_int(value,v);if(unit)str_cpy(value+str_len(value),unit,8);}
}
static void amb_chord_label(unsigned degree,char *text)
{
    int pcs[4],n=amb_chord_tones(amb_pool(),amb.key,(int)degree,pcs);
    int third=(pcs[1]-pcs[0]+12)%12,fifth=(pcs[2]-pcs[0]+12)%12,seventh=n>3?(pcs[3]-pcs[0]+12)%12:0;
    str_cpy(text,N_NOTE[pcs[0]],16);
    str_cpy(text+str_len(text),fifth==6?(n>3?"m7b5":"dim"):third==3?(n>3?"m7":"m"):n>3?(seventh==11?"maj7":"7"):"",16-str_len(text));
}
static uint32_t amb_lfo_phase(unsigned k){return trk[k].lfo_ph;}
/* Compare the programmed destination to the current edited baseline. */
static void amb_sound_preview(const amb_layer_t *current,unsigned next)
{
    const int sample=current->role==6;
    amb_text(12,55,"CURRENT",ADIM,9);amb_text(83,55,sample?N_AS_SOURCE[current->base[P_E0]]:amb_patch_name(current),AINK,22);
    amb_text(12,72,"PROPOSED",ADIM,9);
    cv_text(83,68,&AF_M,sample?N_AS_SOURCE[next]:amb_recipe(current->role,next)->name,AINK);
    if(sample){
        static const char *const character[]={"Soft struck harmonic","Taped vowel sustain","Bowed string and vibrato","Low bowed string section","Hammer attack, long decay","Breathy reed sustain","Airy flute sustain",
            "Rolling beads and surf","Soft metallic brushing","Ridged wooden scrape",
            "Dry shifting seeds","A ticking wooden rattle","Bowed metal shimmer","Apollo landing radio","Orbiter radio check","Kennedy at Rice University","Your voice or instrument"};
        _Static_assert(NELEM(character)==NELEM(N_AS_SOURCE),"Describe every sample source");
        amb_text(12,96,character[current->base[P_E0]],ADIM,34);
        amb_text(12,112,character[next],AINK,34);
    }else{
        static const struct {unsigned id;const char *up,*down;} changes[]={
            {P_ATK,"Blooms more slowly","Speaks more quickly"},{P_REL,"Lingers longer","Fades sooner"},
            {P_FCUT,"Brighter and more open","Darker and rounder"},{P_REV,"Feels farther away","Feels closer"},
            {P_DLY,"More trailing echoes","Fewer trailing echoes"},{P_CHOR,"More shimmer","Steadier tone"},
            {P_SUS,"Fuller held notes","Thinner held notes"}
        };
        amb_layer_t candidate=*current;
        amb_fill_patch(&candidate,amb.key,ambr.edit_layer,current->role,next);
        int best[2]={0,0};const char *line[2]={0,0};
        for(unsigned i=0;i<NELEM(changes);i++){
            unsigned id=changes[i].id;
            int diff=candidate.base[id==P_FCUT && candidate.engine==0?P_E4:id]-current->base[id==P_FCUT && current->engine==0?P_E4:id], magnitude=diff<0?-diff:diff;
            const char *description=diff>0?changes[i].up:changes[i].down;
            if(magnitude>best[0]){
                best[1]=best[0];line[1]=line[0];best[0]=magnitude;line[0]=description;
            }else if(magnitude>best[1]){best[1]=magnitude;line[1]=description;}
        }
        const char *character=0;
        if(candidate.engine==ENGI_VOICE)character="Vocal, like a held vowel";
        else if(candidate.engine==ENGI_SINEFM)character="Rounded, ringing overtones";
        else if(candidate.engine==ENGI_AMB_SAMPLE)character=N_AS_SOURCE[candidate.base[P_E0]];
        else if(current->engine==ENGI_VOICE)character="Less voice-like";
        if(character)amb_text(12,91,character,ADIM,32);
        amb_text(12,104,line[0]?line[0]:"A different tone colour",AINK,36);
        if(line[1])amb_text(12,117,line[1],AINK,36);
    }

}

/* An impulse-response sketch: the horizontal axis is elapsed time. */
static void amb_space_plot(int reverb,int y,int amount,uint16_t ink)
{
    int type=amb.global[G_DTYPE], time=delay_samples();
    int spacing=clamp(time/(FS/48)+4,5,60);
    int reflection=0;
    int feedback=clamp(dly_fb(amb.global[G_DFDBK])*100/32768,0,100);
    if(type==6){spacing=clamp(spacing,6,11);feedback=clamp(feedback,0,32);}
    int height=amount?10+amount*16/127:0;
    int h=height;
    cv_line(17,y,219,y,ALINE);
    for(int x=18;x<217 && h>0;){
        cv_rect(x,y-h,2,h,!reverb && type==5 && (reflection++&1)?TONO_ORANGE:ink);
        if(reverb){
            x+=fx.rtype==1?6:fx.rtype==4?2:3;
            int decay=9+amb.global[G_RSIZE]/3;
            h=height*(220-x)/(202+(x-18)*50/decay);
            if((x/3)%3==0)h=h*(192-amb.global[G_RDAMP])/192;
        }else{
            if((type==3 || type==4) && x+spacing/2<219)cv_rect(x+spacing/2,y-h/2,2,h/2,ink);
            x+=spacing;
            /* Display level on a compressed scale so quiet repeats stay legible. */
            h=feedback?h*(50+feedback/2)/100:0;
        }
    }
}
static void amb_fx_diagram(unsigned k,unsigned module)
{
    track_t *t=&trk[k];int send=0,x;
    if(module==5){
        const int8_t *v=amb_grain_controls[k];amb_text(12,78,"OVERLAPPING GRAINS",ADIM,24);
        int width=24+v[1]/2;
        for(unsigned i=0;i<4;i++){
            int start=13+i*35;
            cv_line(start,119,start+width/2,119-v[0]/4,AINK);
            cv_line(start+width/2,119-v[0]/4,start+width,119,AINK);
        }
    }else if(module==2 || module==3){
        send=t->p[module==2?P_DLY:P_REV]*100/127;
        amb_text(12,78,module==2?N_DLYTYPE[amb.global[G_DTYPE]]:N_RTYPE[fx.rtype],AINK,17);
        amb_text(143,78,"SHARED BUS",ADIM,12);
        amb_space_plot(module==3,119,t->p[module==2?P_DLY:P_REV],AINK);
        amb_text(12,129,"SEND %",ADIM,8);amb_number(62,129,send,AINK);
    }else if(module==0){
        int last=118;amb_text(12,79,"DRIVE RESPONSE / MODEL",ADIM,32);
        for(x=0;x<100;x++){int v=(x-50)*300;int y=clamp(119-softclip(v*(128+t->p[P_DIST]*2)/128)/1100,95,141);
            if(x)cv_line(12+(x-1)*2,last,12+x*2,y,AINK);last=y;}
    }else if(module==1){
        amb_text(12,85,"CHORUS AMOUNT",ADIM,22);cv_rect(12,105,204,5,ALINE);cv_rect(12,105,204*t->p[P_CHOR]/127,5,AINK);
        amb_text(12,128,"MOTION USES SHARED CHORUS",ADIM,32);
    }else{
        amb_text(12,85,"STEREO POSITION",ADIM,26);cv_line(22,113,214,113,ADIM);
        int pan=mp_seen[k]?mp_pan[k]/256:t->p[P_PAN];
        cv_rect(118,109,1,9,ADIM);
        x=22+clamp(pan+64,0,127)*192/127;cv_rect(x-2,107,5,13,TONO_ORANGE);
        amb_text(12,129,"L",ADIM,1);amb_text(208,129,"R",ADIM,1);
    }
}
#include "amb_panel_help.h"

/* Contextual shortcuts belong to the modal guide. */
static void amb_guidance(const amb_layer_t *l,const char **first,const char **second)
{
    const char *a="SCL PAGE / KNOBS EDIT",*b="HOME RETURN";
    if(ambr.zen){a="SCL CONTOUR / TRAIN RIDE";b="HOME RETURN / ALGO TRAVEL";}
    else if(ambr.view==4){a="K1-K4 LAYER GAIN / SCL HOME";b="BARS SHOW LIVE LAYER OUTPUT";}
    else if(ambr.view==0){a="SELECT TARGET / EDIT / SCL ENSEMBLE";b="ALGO TRAVEL / HOME + ARP ZEN";}
    else if(ambr.view==3){a="SELECT SLOT / REC SAVE";b=ambr.overwrite?"REC AGAIN OVERWRITE / HOME CANCEL":"HOME CANCEL";}
    else if(ambr.view==1){
        if(ambr.page==0){a="SCL PAGE / ALGO LAYER TYPE";b="EDIT RESTORE LOADED TRACK";}
        else if(ambr.page==1){
            if(l->role==6){a=ambr.bank==0?"SCL PAGE / ALGO SOURCE":"SCL PAGE / KNOBS EDIT";b="EDIT NEXT SAMPLE PAGE";}
            else {a=ambr.bank?"SCL PAGE / SELECT DETAIL GROUP":"SCL PAGE / ALGO PATCH";b=ambr.bank?"EDIT PATCH / KNOBS EDIT":"EDIT DETAILS / KNOBS EDIT";}
            if(amb_sound_picker()){a="ALGO SOUND / PRESETS VARIATION";b="REC APPLY / HOME CANCEL";}
        }else if(ambr.page==3){a="SELECT GROUP / K4 DESTINATION";b="PRESETS VARIATION / REC APPLY";
        }else if(ambr.page==4){
            a=l->role==4?"SCL PAGE / SELECT DRUM LANE":!l->generated && l->notes[ambr.cursor]?"SELECT STEP / ARP CLEAR / SCL PAGE":"SCL PAGE / SELECT EDIT STEP";
            b=ambr.recording?"WHITE KEYS TOGGLE STEPS / REC END":l->generated?"EDIT OR SEQ MANUAL / REC CAPTURE":"EDIT OR SEQ AUTO / REC OVERDUB";
        }else if(ambr.page==5)a="SCL PAGE / SELECT FX MODULE";
    }else if(ambr.view==2){
        if(ambr.page==0){a="SELECT CHORD INTERVAL";b="ALGO CHORD / KNOBS EVOLVE";}
        else if(ambr.page==7){a="SELECT TIMING LANE";b="EDIT BACK / SCL NEXT TOPIC";}
        else if(ambr.page==6){a="SELECT JOURNEY STYLE";b="EDIT FOR TRAVEL TIMING";}
        else if(ambr.page==2){a="SCL PAGE / KNOBS 1-3 STYLES";b="KNOB 4 VARIATION";}
        else if(ambr.page==3)a="SCL PAGE / SELECT FX MODULE";
        else if(ambr.page==5){a="SELECT CHANGES THEME";b="SCL NEXT PAGE / HOME RETURN";}
    }
    if(ambr.reset_pending){a="REC RESTORE TRACK";b="HOME CANCEL";}
    else if(ambr.piece_preview!=255){a="PRESETS BROWSE / REC RECALL";b="HOME CANCEL";}
    else if(ambr.proposal!=255 && (ambr.view==0 || (ambr.page==0 && ambr.view<3))){
        a="ALGO PROPOSE / REC COMMIT";b="HOME CANCEL";
    }
    *first=a;*second=b;
}

static const char *amb_screen_help(void)
{
    static const char *const edit[]={
        "Choose the role this layer plays.", "Choose a patch and shape its sound.",
        "Shape how each note starts and ends.", "Set how a parameter moves over time.",
        "Edit this layer's rhythm and notes.", "Shape the layer and its effect sends.",
        "Balance the frequencies of this layer."
    };
    static const char *const global[]={
        "Set how the piece changes as it plays.", "Set the key and scale of the piece.",
        "Choose how the layers generate music.", "Shape the effects shared by all layers.",
        "Balance the tone of the whole mix.", "Choose the instrument display theme.", "Set travel and sound transitions.", "Steer the song automatically."
    };
    if(ambr.reset_pending)return "Restore the layer from the loaded piece.";
    if(ambr.piece_preview!=255)return "Load a factory piece or a saved slot.";
    switch(ambr.view){
    case 1:return edit[ambr.page];
    case 2:return global[ambr.page];
    case 3:return "Store the current piece in a song slot.";
    case 4:return "Watch the level and state of each layer.";
    default:return "Mix the layers and guide the harmony.";
    }
}

/* Layer numbers are shared by HOME and ENSEMBLE. The orange mark tracks
 * the selected target, including when several tracks have the same role. */
#define DISPLAY_GRID (TE_LIGHT?RGB(174,172,167):RGB(76,80,77))
#define DISPLAY_TRACE (TE_LIGHT?RGB(0,148,246):RGB(172,209,221))
static void amb_display(int top,int height)
{
    cv_bg=SCOPE_BG;cv_rect(8,top,224,height,SCOPE_BG);
    cv_rect(8,top,3,height,TONO_ORANGE);
}

/* All time and response plots share the same 200-pixel plotting area. */
static void amb_scope(int reference_y)
{
    amb_display(49,75);
    for(int x=19;x<=219;x+=50)
        for(int y=57;y<109;y+=5)cv_rect(x,y,1,1,DISPLAY_GRID);
    cv_line(19,reference_y,219,reference_y,DISPLAY_GRID);
}

static void amb_eq_plot(const int8_t *bands,unsigned target)
{
    amb_scope(82);
    const amb_eq_meter_t *meter=&amb_eq_meters[target];
    for(unsigned band=0;band<3;band++){
        unsigned level=fm1_ms-meter->updated_ms<200?meter->peak[band]:0;
        for(unsigned segment=0;segment<12 && level>16;segment++){
            cv_rect(53+50*band,105-4*segment,32,3,DISPLAY_GRID);
            level=level*5/9;
        }
    }
    int last=82;
    for(unsigned i=0;i<4;i++){
        int next=i<3?82-bands[i]*2:82;
        cv_line(19+50*i,last,69+50*i,next,DISPLAY_TRACE);
        if(i<3)tono_disc(69+50*i,next,2,TONO_ORANGE);
        last=next;
    }
    amb_text(60,112,"LOW",SCOPE_INK,3);amb_text(109,112,"MID",SCOPE_INK,3);amb_text(157,112,"HIGH",SCOPE_INK,4);
    cv_bg=ABG;
}

static void amb_led_number(int x,int y,unsigned value)
{
    static const uint8_t digits[]={63,6,91,79,102,109,125,7,127,111};
    static const uint8_t segments[][4]={{2,0,9,2},{11,2,2,7},{11,11,2,7},{2,18,9,2},{0,11,2,7},{0,2,2,7},{2,9,9,2}};
    unsigned place=value>=100?100:value>=10?10:1;
    for(;place;place/=10,x+=16){
        unsigned mask=digits[(value/place)%10];
        for(unsigned i=0;i<7;i++){
            const uint8_t *seg=segments[i];
            cv_rect(x+seg[0],y+seg[1],seg[2],seg[3],(mask>>i)&1?amb_accent(0):DISPLAY_GRID);
        }
    }
    amb_text(x, y+12,"%",amb_accent_text(0),1);
}

static void amb_mix_ring(void)
{
    static const int8_t quadrant[][2]={{16,1},{15,5},{13,9},{10,12},{6,15},{1,16}};
    for(unsigned k=0;k<4;k++){
        int last_x=0,last_y=0;
        for(unsigned j=0;j<NELEM(quadrant);j++){
            int x=quadrant[j][0],y=quadrant[j][1];
            for(unsigned n=0;n<k;n++){int t=x;x=-y;y=t;}
            if(j)cv_line_t(203+last_x,52+last_y,203+x,52+y,
                (j-1)*127<5u*amb.layer[k].level?amb_accent(k):DISPLAY_GRID,3);
            last_x=x;last_y=y;
        }
    }
}

/* Role pictograms keep their shapes when a layer is reassigned. */
static void amb_role_symbol(int x,int y,unsigned role,uint16_t ink)
{
    static const uint16_t rows[][9]={
        {0x104,0x28a,0x451,0x820,0x104,0x28a,0x451,0x820,0}, /* pad */
        {0,0xfff,0,0,0xfff,0,0,0xfff,0},                     /* drone */
        {0x060,0x198,0x204,0x492,0x801,0x060,0x060,0x060,0x1f8}, /* bells */
        {0x060,0x060,0x060,0x060,0x060,0x0f0,0x1f8,0x0f0,0}, /* bass */
        {0x060,0x060,0x063,0x663,0x663,0x66f,0x66f,0x66f,0}, /* drums */
        {0x444,0,0x111,0,0x888,0,0x222,0,0x444},             /* texture */
        {0x180,0x1c0,0x1e0,0x1f0,0x1f8,0x1f0,0x1e0,0x1c0,0x180} /* sample */
    };
    for(int row=0;row<9;row++)
        for(int col=0;col<12;col++)
            if(rows[role][row]&(1u<<(11-col)))cv_rect(x+col,y+row,1,1,ink);
}

static void amb_draw_home(void)
{
    unsigned target=amb_level_feedback(),value=target?amb.layer[target-1].level:amb.master;
    unsigned percent=value*100u/(target?127u:1023u);
    char label[16]="MASTER",chord[16];
    if(target){label[0]='L';label[1]=(char)('0'+target);label[2]=' ';str_cpy(label+3,AMB_ROLE[amb.layer[target-1].role],sizeof label-3);}
    amb_display(29,56);
    if(!ambr.target){
        cv_bg=TONO_ORANGE;cv_rect(14,32,53,13,cv_bg);
        amb_text(17,35,label,TONO_INK,14);cv_bg=SCOPE_BG;
    }else amb_text(17,35,label,SCOPE_INK,14);
    amb_text(98,35,"CHORD",SCOPE_INK,7);
    char level[8];
    fmt_int(level,percent);
    if(amb_theme()){
        amb_led_number(17,50,percent);
        if(ambr.chord_q==255)amb_mix_ring();
    }else{
        cv_text(17,45,&AF_TONO_LEVEL,level,SCOPE_INK);
        amb_text(20+text_w(&AF_TONO_LEVEL,level),61,"%",SCOPE_INK,1);
    }
    amb_chord_label(amb.chord,chord);cv_text(98,48,&AF_M,chord,SCOPE_INK);
    if(ambr.chord_q!=255)amb_text(162,52,"QUEUED",TONO_ORANGE,7);
    for(unsigned i=0;i<24;i++)cv_rect(17+9*i,77,7,2,i*100<24*percent?TONO_ORANGE:DISPLAY_GRID);
    for(unsigned k=0;k<4;k++){
        const amb_layer_t *l=&amb.layer[k];int left=8+(k%2)*114,y=90+(k/2)*28;
        if(amb_theme()){
            uint16_t accent=amb_accent(k);cv_bg=ABG;
            amb_role_symbol(left,y+1,l->role,accent);
            if(ambr.target==k+1)cv_rect(left,y+14,12,2,accent);
            amb_number(left+19,y,k+1,amb_accent_text(k));
            amb_text(left+28,y,AMB_ROLE[l->role],amb_accent_text(k),8);
        }else{
            cv_bg=ambr.target==k+1?TONO_ORANGE:AINK;cv_rect(left,y,14,13,cv_bg);
            amb_number(left+4,y+2,k+1,ABG);cv_bg=ABG;
            amb_text(left+19,y,AMB_ROLE[l->role],AINK,8);
        }
        amb_text(left+19,y+13,l->role==6?N_AS_SOURCE[l->base[P_E0]]:amb_patch_name(l),ADIM,14);
        if(l->base[P_MUTE] || (ambr.solo>=0 && ambr.solo!=(int)k))amb_text(left+99,y,"M",AQUEUE,1);
    }
}

static void amb_draw_ensemble(void)
{
    amb_text(10,33,"LAYER",ADIM,9);amb_text(102,33,"SIGNAL",ADIM,9);amb_text(192,33,"STATE",ADIM,7);
    for(unsigned k=0;k<4;k++){
        track_t *track=&trk[k];
        const unsigned role=amb.layer[k].role;
        const unsigned muted=track->p[P_MUTE] || (ambr.solo>=0 && ambr.solo!=(int)k);
        const int y=48+23*(int)k;
        const int width=meter_px(track->peak)*74/(TS_MH-2);
        unsigned held=0,active=0;
        track->peak=0;
        for(unsigned i=0;i<NVOICE;i++)if(track->v[i].active){active++;held+=track->v[i].gate!=0;}
        cv_bg=k%2?ASEL:ABG;cv_rect(8,y,224,20,cv_bg);
        cv_rect(8,y+19,224,1,ALINE);
        if(held && !muted)tono_disc(18,y+9,3,TONO_ORANGE);
        amb_number(35,y+5,k+1,ADIM);amb_text(46,y+5,AMB_ROLE[role],AINK,8);
        cv_rect(102,y+8,74,4,ALINE);cv_rect(102,y+8,width,4,TONO_ORANGE);
        amb_text(192,y+5,muted?"MUTE":ambr.handoff[k]?"TAIL":held?"NOTE":active?"REL":"REST",
                 muted || ambr.handoff[k]?AQUEUE:held?AINK:ADIM,6);
    }
}

#include "amb_zen.h"
static void amb_draw(void)
{
    unsigned i,k;char b[32];amb_layer_t *l=&amb.layer[ambr.edit_layer];
    if(!ambr.dirty && fm1_ms-ambr.last_draw<(ambr.zen?80u:(ambr.view==4 || (ambr.view==1 && ambr.page==4))?50u:((ambr.view==1 && (ambr.page==3 || ambr.page==6 || (ambr.page==5 && ambr.fx==4))) || (ambr.view==2 && ambr.page==4))?80u:250u))return;
    ambr.last_draw=fm1_ms;ambr.dirty=0;
    if(fm1_ms-travel.turned_ms>=1200u)travel.indicator=0;
    static uint8_t was_zen;
    unsigned zen_layout=ambr.zen && !ambr.view && !ambr.help && ambr.piece_preview==255;
    if(zen_layout!=was_zen){dc_frame(1);was_zen=(uint8_t)zen_layout;}
    if(zen_layout){amb_draw_zen();return;}
    cv_begin(240,147,ABG);


    tono_logo(8,6,3,ABG,AINK);
    amb_text(60,8,ambr.help?"HELP":ambr.view==4?"ENSEMBLE":ambr.view==0?(ambr.zen?(ambr.zen_train?"TRAIN RIDE":"CONTOUR"):"HOME"):ambr.view==1?"EDIT":ambr.view==2?"GLOBAL":"SAVE",ADIM,10);
    if(!ambr.help && (ambr.view==0 || ambr.view==1 || ambr.view==4)){
        unsigned target=ambr.view==1?ambr.edit_layer+1:ambr.target;
        int queued=amb_octave_delta(0)+(target?amb_octave_delta(target):0);
        int active=amb_bar_octave[target?target-1:0];
        if(!target)for(unsigned k=1;k<4;k++)if(amb_bar_octave[k]!=active){active=0;break;}
        int offset=clamp(queued?queued:active,-3,3);
        if(offset){
            char tag[]="Q+1";tag[0]=queued?'Q':'O';tag[1]=offset<0?'-':'+';
            tag[2]='0'+(offset<0?-offset:offset);amb_text(116,8,tag,AQUEUE,3);
        }
    }
    if(ambr.view!=4)amb_number(146,8,ambr.view==0?ambr.target+1:ambr.view==3?ambr.slot+1:ambr.view==2?amb_global_index(ambr.page)+1:ambr.page+1,AINK);
    if(ambr.view!=4)amb_text(155,8,ambr.view==0?"/5":ambr.view==3?"/8":"/7",AINK,3);
    amb_text(175,8,song.playing?">":"-",song.playing?TONO_ORANGE:ADIM,1);amb_number(188,8,amb.global[G_BPM],AINK);amb_text(211,8,"BPM",ADIM,3);
    cv_rect(8,25,224,1,ALINE);cv_bg=ABG;
    if(ambr.help){
        amb_text(10,32,amb_screen_help(),AINK,39);
        if(ambr.view==2 && ambr.page==5){
            amb_text(12,61,"SELECT",ADIM,20);
            amb_text(12,81,"Switch the global display theme.",AINK,36);

        }else if(ambr.view==3 || ambr.reset_pending || ambr.piece_preview!=255){
            amb_text(12,51,ambr.view==3?"SAVE PIECE":ambr.reset_pending?"RESTORE LAYER":"RECALL PIECE",AINK,30);
            amb_text(12,81,ambr.reset_pending?"REC confirms the restore":"SELECT chooses a destination",ADIM,36);
            amb_text(12,108,ambr.view==3?"REC captures / STOP writes":ambr.reset_pending?"HOME cancels after closing help":"REC loads the selected piece",ADIM,36);
        }else for(k=0;k<4;k++){
            char label[16]={0},value[24]={0};int y=55+23*k;
            amb_knob_value(k,label,value);if(!label[0])continue;
            cv_bg=k%2?ABG:ASEL;cv_rect(6,y-5,228,20,cv_bg);amb_number(10,y,k+1,ADIM);
            amb_text(22,y,label,AINK,9);amb_text(84,y,amb_knob_hint(l,k),ADIM,25);
        }
    }else if(ambr.view==4){
        amb_draw_ensemble();
    }else if(ambr.view==0) {
        amb_draw_home();
    } else {
        amb_display(29,20);
        if(ambr.view==1){
            cv_text(16,30,&AF_M,ambr.page==2 && l->role==4?"HIT SHAPE":AMB_EDIT[ambr.page],SCOPE_INK);
            amb_text(151,35,"L",SCOPE_INK,1);amb_number(158,35,ambr.edit_layer+1,SCOPE_INK);
            amb_text(173,35,AMB_ROLE[l->role],SCOPE_INK,8);
        }else cv_text(16,30,&AF_M,ambr.view==2?AMB_GLO[ambr.page]:"SAVE",SCOPE_INK);
        cv_bg=ABG;
        if(ambr.view==1 && ambr.page==0) {
            cv_rect(12,97,83,1,ALINE);if(ambr.proposal!=255)cv_rect(130,97,83,2,TONO_ORANGE);
            amb_text(12,58,ambr.handoff[ambr.edit_layer]?"NEXT":"CURRENT",ADIM,12);amb_text(130,58,"PROPOSED",ADIM,12);
            amb_text(12,78,AMB_ROLE[l->role],AINK,12);amb_text(130,78,ambr.proposal==255?"-":AMB_ROLE[ambr.proposal],AQUEUE,12);
        } else if(ambr.view==1 && ambr.page==1) {
            if(amb_sound_picker() && ambr.proposal!=255)amb_sound_preview(l,ambr.proposal);
            else if(l->role!=6 && ambr.bank){
                static const char *const group[3]={"ENGINE","FILTER","PLAY"};
                amb_text(12,54,ENGINES[l->engine]->name,ADIM,12);amb_text(125,54,group[ambr.detail],AINK,12);
                for(i=0;i<4;i++){
                    unsigned id=amb_sound_id(l,1,i);const param_desc_t *d=amb_param_desc(l,id);char val[24];const char *unit=0;
                    int width=(l->base[id]-d->min)*78/(d->max-d->min?d->max-d->min:1);param_format(d,l->base[id],val,&unit);
                    amb_text(12,73+i*18,d->label,ADIM,10);amb_text(76,73+i*18,val,AINK,10);
                    cv_rect(150,76+i*18,78,4,ALINE);cv_rect(150,76+i*18,width,4,AINK);
                }
            }else{
                
                amb_text(12,60,l->role==6?(ambr.bank==0?"SOURCE":ambr.bank==1?"PLAYBACK":"CHARACTER"):"PATCH",ADIM,12);
                cv_text(12,76,&AF_M,l->role==6?N_AS_SOURCE[l->base[P_E0]]:amb_patch_name(l),AINK);
                if(amb_mic_page()){
                    const amb_take_t *t=amb_current_take();
                    amb_text(12,103,amb_capture_page?(take_rec.frames?"Recording":"Waiting for USB audio"):t->length?"REC  Replace take":"REC  Record USB audio",AINK,32);
                    if(amb_capture_page)cv_rect(12,123,210*take_rec.frames/AMB_TAKE_FRAMES,3,TONO_ORANGE);
                    else if(t->length){
                        char note[8];str_cpy(note,N_NOTE[t->root%12],6);
                        unsigned len=0;while(note[len])len++;
                        note[len]=(char)('0'+t->root/12-1);note[len+1]=0;
                        amb_text(12,121,t->period_q8?"Matched root":"Reference root",ADIM,20);amb_text(99,121,note,AINK,5);
                    }
                }else{amb_text(12,102,ENGINES[l->engine]->name,ADIM,12);amb_number(188,113,l->patch+1,AINK);}
            }
        } else if(ambr.view==1 && ambr.page==2) {
            amb_scope(106);
            if(l->role==4){
                for(i=0;i<4;i++){
                    unsigned id=amb_envelope_id(l,i);
                    const param_desc_t *d=amb_param_desc(l,id);
                    int height=clamp((l->base[id]-d->min)*46/(d->max-d->min),0,46);
                    cv_rect(29+i*50,106-height,25,height,DISPLAY_TRACE);
                    cv_rect(29+i*50,105-height,25,2,TONO_ORANGE);
                }
                amb_text(17,112,"HIT PARAMETERS",SCOPE_INK,18);
            }else if(l->role==6 && l->base[P_E1]==3){
                int start=19+2*clamp(l->base[P_E4],0,100);
                int end=19+2*clamp(l->base[P_E5],0,100);
                cv_line(19,82,219,82,DISPLAY_GRID);
                cv_rect(start,79,end>start?end-start:1,7,DISPLAY_TRACE);
                cv_line(start,74,start,90,TONO_ORANGE);
                cv_line(end,74,end,90,TONO_ORANGE);
                amb_text(17,112,"ONE-SHOT REGION",SCOPE_INK,18);
            }else{
                int peak=23+l->base[P_ATK]/3,hold=peak+6+l->base[P_DEC]/2;
                int release=215-l->base[P_REL]/3,sustain=106-l->base[P_SUS]*48/127;
                cv_line(19,106,peak,58,DISPLAY_TRACE);
                cv_line(peak,58,hold,sustain,DISPLAY_TRACE);
                cv_line(hold,sustain,release,sustain,DISPLAY_TRACE);
                cv_line(release,sustain,219,106,DISPLAY_TRACE);
                tono_disc(peak,58,2,TONO_ORANGE);tono_disc(hold,sustain,2,TONO_ORANGE);
                tono_disc(release,sustain,2,TONO_ORANGE);
                amb_text(17,112,"NOTE ON",SCOPE_INK,8);amb_text(174,112,"RELEASE",SCOPE_INK,8);
            }
            cv_bg=ABG;
        } else if(ambr.view==1 && ambr.page==3) {
            track_t *t=&trk[ambr.edit_layer];uint32_t ph=amb_lfo_phase(ambr.edit_layer);int last=0;
            amb_scope(82);
            for(i=0;i<=200;i++){
                int y=82-lfo_wave(t,i*21474836u)*22/32768;
                if(i){cv_line(18+i,last,19+i,y,DISPLAY_TRACE);cv_rect(19+i,y+1,1,1,DISPLAY_TRACE);}
                last=y;
            }
            int x=19+(int)((ph>>16)*200u/65536u),y=82-lfo_wave(t,ph)*22/32768;
            cv_line(x,56,x,107,TONO_ORANGE);tono_disc(x,y,3,TONO_ORANGE);
            amb_text(17,112,"0",SCOPE_INK,1);amb_text(178,112,"1 CYCLE",SCOPE_INK,8);cv_bg=ABG;
            char label[16],dest[24];amb_knob_value(3,label,dest);
            amb_text(10,133,amb_route_groups[amb_route_group(l->lfo_dest)],ADIM,12);amb_text(84,133,dest,AINK,18);
        } else if(ambr.view==1 && ambr.page==4) {
            amb_display(49,75);
            for(i=0;i<16;i++){
                int x=19+13*i, on=l->role==4?(l->drums[ambr.lane]>>i)&1u:(l->generated?ambr.recent[ambr.edit_layer][i]:l->notes[i])!=0;
                int height=l->generated || l->role==4?21:7+l->velocity[i]*28/127;
                if(i%4==0)cv_rect(x,60,8,1,DISPLAY_GRID);
                cv_rect(x,98-(on?height:1),8,on?height:1,on?(amb_theme()?amb_accent(ambr.edit_layer):DISPLAY_TRACE):DISPLAY_GRID);
                if(song.playing && ambr.playhead_valid[ambr.edit_layer] && ambr.playhead[ambr.edit_layer]==i)cv_rect(x,101,8,2,TONO_ORANGE);
                if(i==ambr.cursor){cv_line(x,108,x+4,105,SCOPE_INK);cv_line(x+4,105,x+8,108,SCOPE_INK);}
            }
            amb_text(17,112,l->generated?"AUTO":"MANUAL",SCOPE_INK,8);
            amb_text(175,112,"16 STEPS",SCOPE_INK,9);cv_bg=ABG;
            amb_text(10,132,"PLAY",ADIM,5);
            if(song.playing && ambr.playhead_valid[ambr.edit_layer])amb_number(40,132,ambr.playhead[ambr.edit_layer]+1,AQUEUE);
            else amb_text(40,132,"-",ADIM,1);
            if(l->role==4)amb_text(98,132,amb_drum_lanes[ambr.lane],AINK,12);
            else {amb_text(98,132,"EDIT",ADIM,5);amb_number(129,132,ambr.cursor+1,AINK);}
        } else if(ambr.view==1 && ambr.page==6) {
            amb_eq_plot(amb.layer_eq[ambr.edit_layer],ambr.edit_layer+1);
            amb_text(12,130,"LIVE INPUT / LAYER EQ (DB)",ADIM,35);
        } else if(ambr.view==1 && ambr.page==5) {
            static const char *const modules[6]={"DRIVE","CHORUS / PAN","DELAY SEND","REVERB SEND","SPATIAL","GRANULAR"};
            cv_text(12,53,&AF_M,modules[ambr.fx%6],AINK);amb_fx_diagram(ambr.edit_layer,ambr.fx%6);
        } else if(ambr.view==2 && ambr.page==0) {
            amb_text(12,60,"CURRENT CHORD",ADIM,20);amb_chord_label(amb.chord,b);cv_text(12,76,&AF_M,b,AINK);
            amb_text(130,60,"PROPOSED",ADIM,12);
            if(ambr.proposal!=255) {amb_chord_label(ambr.proposal,b);amb_text(130,78,b,AQUEUE,16);amb_text(130,94,AMB_MOOD[ambr.proposal],ADIM,14);}
            amb_text(12,115,"CHORD INTERVAL",ADIM,16);
            if(amb.auto_index){amb_number(111,115,AMB_AUTO[amb.auto_index],AINK);amb_text(132,115,AMB_AUTO[amb.auto_index]==1?"BAR":"BARS",ADIM,5);}
            else amb_text(111,115,"Off",AINK,4);
            amb_text(12,132,N_NOTE[amb.key],AINK,3);amb_text(40,132,amb.scale==0?"MINOR":amb.scale==1?"MAJOR":amb.scale==2?"DORIAN":"LYDIAN",ADIM,10);
        } else if(ambr.view==2 && ambr.page==2) {
            amb_display(49,75);
            amb_participation_t parts[4]={{1,40},{1,35},{1,48},{1,25}};
            for(k=0;k<3;k++){
                int y=53+k*23;
                unsigned style=ambr.gen_edit[k]?ambr.gen_candidate[k]:amb.policy.pending[k]!=255?amb.policy.pending[k]:amb.policy.active[k];
                amb_policy_t preview=amb.policy;preview.active[k]=(uint8_t)style;
                if(ambr.gen_edit[k] || amb.policy.pending[k]!=255)amb_text(209,y,"Q",TONO_ORANGE,1);
                amb_text(17,y,k==0?"RHYTHM":k==1?"PHRASE":"INTERPLAY",amb_theme()?amb_accent_text(k):DISPLAY_TRACE,10);
                amb_text(88,y,k==0?AMB_RHYTHMS[style]:k==1?AMB_PHRASES[style]:AMB_INTERPLAY[style],SCOPE_INK,24);
                int prev=y+17;
                for(i=0;i<26;i++){
                    int x=19+i*8;
                    if(k==0){
                        int on=amb_rhythm_event(&preview,i,0,40);
                        cv_rect(x,y+(on?12:16),5,on?5:1,on?(amb_theme()?amb_accent(k):DISPLAY_TRACE):DISPLAY_GRID);
                    }else if(k==1){
                        int next=y+17-(amb_phrase_event(&preview,i*16,0)?5:0);
                        if(i)cv_line(x-8,prev,x,next,amb_theme()?amb_accent(k):DISPLAY_TRACE);prev=next;
                    }else{
                        unsigned m=amb_policy_events(&preview,i,parts);
                        for(int lane=0;lane<4;lane++)cv_rect(x,y+11+lane*3,5,2,(m>>lane)&1?(amb_theme()?amb_accent(lane):DISPLAY_TRACE):DISPLAY_GRID);
                    }
                }
            }
            cv_bg=ABG;amb_text(10,132,"GENERATOR PREVIEW",ADIM,20);
        } else if(ambr.view==2 && ambr.page==3) {
            static const char *const modules[4]={"REVERB","DELAY / TAPE","MOTION","WARMTH"};
            amb_display(49,75);
            cv_text(17,50,&AF_M,modules[ambr.fx%4],SCOPE_INK);
            if(ambr.fx%4==0)amb_text(162,58,N_RTYPE[fx.rtype<NELEM(N_RTYPE)?fx.rtype:0],TE_LIGHT?amb_accent_text(1):DISPLAY_TRACE,11);
            if(ambr.fx%4==1)amb_text(162,58,N_DLYTYPE[amb.global[G_DTYPE]],TE_LIGHT?amb_accent_text(1):DISPLAY_TRACE,11);
            const char *const names[]={"SIZE","MIX","DEPTH","CLIP"};
            const unsigned ids[]={G_RSIZE,G_DMIX,G_CDEPTH,G_CLIP};
            unsigned id=ids[ambr.fx%4], v=amb.global[id], max=GP[id].max;
            if(ambr.fx%4<2)amb_space_plot(ambr.fx%4==0,102,ambr.fx%4?v:127,DISPLAY_TRACE);
            else for(i=0;i<25;i++){
                int x=19+i*8,h=4+i*22/24;
                cv_rect(x,101-h,6,h,i*max<25*v?DISPLAY_TRACE:DISPLAY_GRID);
            }
            if(ambr.fx%4>=2){int x=19+v*198/max;cv_line(x,73,x,103,TONO_ORANGE);}
            amb_text(17,112,names[ambr.fx%4],SCOPE_INK,6);amb_number(189,112,v*100/max,SCOPE_INK);
            amb_text(208,112,"%",SCOPE_INK,1);cv_bg=ABG;
            amb_text(10,132,ambr.fx%4==0 && fx.rtype!=amb.global[G_RTYPE]?"ECO: LIGHTER EFFECT ACTIVE":"SHARED MASTER PROCESSING",ADIM,32);
        } else if(ambr.view==2 && ambr.page==1) {
            amb_display(49,75);cv_text(17,51,&AF_M,N_NOTE[amb.key],SCOPE_INK);
            amb_text(43,58,amb.scale==0?"MINOR":amb.scale==1?"MAJOR":amb.scale==2?"DORIAN":"LYDIAN",TE_LIGHT?amb_accent_text(1):DISPLAY_TRACE,8);
            for(i=0;i<12;i++){
                int x=17+i*17;unsigned in=(amb_pool()>>i)&1u;
                cv_bg=i==0?TONO_ORANGE:in?DISPLAY_TRACE:DISPLAY_GRID;cv_rect(x,77,15,24,cv_bg);
                amb_text(x+1,85,N_NOTE[(amb.key+i)%12],in?(TE_LIGHT?TONO_INK:SCOPE_BG):SCOPE_INK,2);
            }
            cv_bg=SCOPE_BG;amb_text(17,112,"NOTE POOL",SCOPE_INK,10);
            cv_bg=ABG;
        } else if(ambr.view==2 && ambr.page>=6){
            static const char *const hints[]={"Reverse to retrace your sound.","Four travel steps bring a new sound.","Keep one instrument as others change."};
            int timing=ambr.page==7;
            amb_text(12,58,timing?"TIMING":"JOURNEY",ADIM,20);
            cv_text(12,77,&AF_M,timing?timing_names[timing_tab]:amb_walk_styles[amb_journey_style()],AINK);
            amb_text(12,100,timing?"SELECT chooses timing lane":hints[amb_journey_style()],ADIM,40);
            const char *status=!amb_walk_value(0)?"Auto off  /  manual turns only":
                amb_walk_value(1)==amb_walk_value(2)?"Auto on  /  fixed interval":"Auto on  /  random interval";
            if(timing && timing_tab)status=timing_tab==1?"Rotate rhythm, phrase, interplay":"Replace one generated layer";
            amb_text(12,126,status,AINK,36);
        } else if(ambr.view==2 && ambr.page==4) {
            amb_eq_plot(amb.eq,0);
            amb_text(12,130,"LIVE MIX / MASTER EQ (DB)",ADIM,34);
        } else if(ambr.view==2 && ambr.page==5){
            for(i=0;i<THEME_COUNT;i++){
                int y=55+26*i;
                cv_bg=i==amb_theme()?ASEL:ABG;cv_rect(8,y,224,22,cv_bg);
                if(i==amb_theme())cv_rect(8,y,3,22,TONO_ORANGE);
                amb_text(17,y+6,theme_names[i],AINK,32);
            }
        } else if(ambr.view==3) {
            amb_text(12,62,"SLOT",ADIM,6);amb_number(49,62,ambr.slot+1,AINK);
            cv_text(12,79,&AF_M,amb_slot_state[ambr.slot]==AMB_SAVED?"SAVED":amb_slot_state[ambr.slot]==AMB_EMPTY?"EMPTY":"UNREADABLE",AQUEUE);
            for(i=0;i<8;i++){int x=13+i*27;cv_rect(x,105,22,6,i==ambr.slot?AQUEUE:amb_slot_state[i]==AMB_SAVED?AINK:ALINE);}
            amb_text(12,125,amb_save_slot?"CAPTURED - WRITES ON STOP":ambr.overwrite?"OVERWRITE ARMED":"DESTINATION READY",ambr.overwrite?AQUEUE:ADIM,32);
        }
    }
    if(!ambr.help && ambr.local_preset && amb_page_bank()){
        GFX_HOOK_COVER(8,50,232,145);cv_bg=ASEL;cv_rect(8,50,224,95,cv_bg);
        const char *subject=ambr.page==1 && ambr.view==1?(l->role==6?N_AS_SOURCE[l->base[P_E0]]:amb_patch_name(l)):"PROPOSED VARIATION";
        amb_text(16,62,subject,ADIM,32);
        cv_text(16,79,&AF_M,amb_page_bank()->name[ambr.local_preset-1],AINK);
        if(ambr.view==1 && ambr.page==1){
            static const char *const feel[]={"Up close, with a clearer attack","Rounder, with a softer entrance","A slow swell in a larger space"};
            amb_text(16,103,feel[ambr.local_preset-1],ADIM,36);
        }

    }else if(!ambr.help && ambr.reset_pending){
        GFX_HOOK_COVER(8,50,232,145);cv_bg=ASEL;cv_rect(8,50,224,95,cv_bg);
        amb_text(16,61,"RESTORE THIS TRACK?",AQUEUE,30);
        amb_text(16,81,"LAST LOADED PIECE VERSION",ADIM,31);
        amb_text(16,101,"SOUND / MACROS / PATTERN",ADIM,31);
    }else if(!ambr.help && ambr.piece_preview!=255) {
        GFX_HOOK_COVER(8,29,232,145);cv_bg=ASEL;cv_rect(8,29,224,116,cv_bg);amb_text(16,67,"PRESET PROPOSAL",ADIM,30);
        cv_text(16,85,&AF_M,ambr.piece_preview<NELEM(AMB_SCENE)?AMB_SCENE[ambr.piece_preview]:"USER SLOT",AINK);
        if(ambr.piece_preview>=NELEM(AMB_SCENE)) amb_number(113,90,ambr.piece_preview-NELEM(AMB_SCENE)+1,AINK);
    } else if(!ambr.help && ambr.view==0 && ambr.proposal!=255) {
        GFX_HOOK_COVER(8,86,232,146);cv_bg=ASEL;cv_rect(8,86,224,60,cv_bg);amb_text(18,91,"NEXT HARMONY",ADIM,24);
        amb_chord_label(ambr.proposal,b);amb_text(18,112,b,AINK,16);amb_text(115,112,AMB_MOOD[ambr.proposal],ADIM,16);
    }
    cv_blit_cells(0,0);
    cv_begin(240,93,ABG);
    cv_rect(8,0,224,1,ALINE);
    if(ambr.help){
        const char *first,*second;
        amb_guidance(l,&first,&second);
        amb_text(10,8,first,AINK,39);
        amb_text(10,23,second,AINK,39);
        amb_text(10,44,ambr.view==1 && ambr.page==1?"MASTER adjusts this layer after pickup.":"MASTER picks up the saved level.",ADIM,39);
        if(amb_page_bank())
            amb_text(10,57,"PRESETS variation / REC apply",ADIM,39);
        amb_text(10,75,"HOME close / HOME + FX bypass",AQUEUE,37);
        cv_blit_cells(0,147);
        return;
    }
    if(ambr.view==2 && ambr.page==5){
        amb_text(10,18,"SELECT",ADIM,16);
        cv_text(10,32,&AF_M,"Display theme",AINK);

    }else if(ambr.view==3) {
        for(i=0;i<8;i++){
            int x=11+i*28;unsigned selected=i==ambr.slot;
            cv_bg=selected?AINK:ASEL;
            cv_rect(x,15,22,24,cv_bg);
            amb_number(x+7,23,i+1,selected?ABG:AINK);
            if(selected)cv_rect(x,41,22,3,TONO_ORANGE);
        }
    }else if(!ambr.reset_pending && ambr.piece_preview==255)for(k=0;k<4;k++) {
        char label[16]={0},value[24]={0};amb_knob_value(k,label,value);if(!label[0])continue;cv_bg=ABG;
        int left=9+k*57;
        if(k)cv_rect(left-4,12,1,42,ALINE);
        uint16_t accent=amb_theme()?amb_accent(k):TONO_ORANGE;
        amb_text(left,14,label,amb_theme()?amb_accent_text(k):ADIM,9);cv_rect(left,28,17,2,accent);
        if(text_w(&AF_M,value)<=54)cv_text(left,34,&AF_M,value,AINK);
        else amb_text(left,37,value,AINK,9);
        if(amb_theme() && ambr.view==0){
            int amount;
            if(ambr.target)amount=amb.layer[ambr.target-1].macro[k]*100/127;
            else{
                unsigned id=amb_master_macros[k];
                amount=k==3?-amb.global[id]*100/64:amb.global[id]*100/GP[id].max;
            }
            for(int tick=0;tick<9;tick++)cv_rect(left+5*tick,53,4,2,
                tick*100<9*amount?accent:ALINE);
        }
    }
    cv_bg=ABG;
    if(ambr.view==1 && ambr.page==1 && ambr.level_feedback_active &&
       ambr.level_feedback_target==ambr.edit_layer+1 && fm1_ms-ambr.level_feedback_ms<1200u){
        amb_text(10,59,"LAYER GAIN",ADIM,10);amb_number(70,59,amb.layer[ambr.edit_layer].level*100/127,AQUEUE);
    }else if(ambr.view==0 && travel.indicator){
        int position=travel.wanted;
        amb_text(10,59,"TRAVEL",ADIM,7);
        amb_text(55,59,amb_landscape_name(position),AINK,20);
        if(position>0)amb_text(166,59,"+",AQUEUE,1);
        amb_number(174,59,position,AQUEUE);
        int marker=120+clamp(position,-18,18)*6;
        cv_rect(12,70,217,1,ALINE);
        cv_rect(120,68,1,3,ADIM);
        cv_rect(marker-2,67,5,4,TONO_ORANGE);
    }else if(ambr.notice[0] && fm1_ms-ambr.notice_ms<2500u)
        amb_text(10,59,ambr.notice,ADIM,39);
    else if(amb_tail_any())
        amb_text(10,59,ambr.handoff[4] || ambr.handoff[5]?"CROSSFADE":"OLD SOUND RELEASING / NEXT QUEUED",ADIM,39);
    else if(amb_monitor_requested)amb_text(10,59,"FX BYPASSED",AQUEUE,13);
    cv_rect(8,72,224,1,ALINE);
    const char *hint="Help";
    if(ambr.reset_pending)hint="REC restore / HOME cancel";
    else if(ambr.piece_preview!=255)hint="REC recall / HOME cancel";
    else if(ambr.proposal!=255 || ambr.local_preset)hint="REC apply / HOME cancel";
    else if(ambr.view==2 && ambr.page==6)hint="EDIT  Timing and reach";
    else if(ambr.view==2 && ambr.page==7)hint="EDIT  Back to Travel";
    else if(ambr.view==1 && ambr.page==1)
        hint=l->role==6?"EDIT  Next sound page":ambr.bank?"EDIT  Back to patch":"EDIT  Sound details";
    amb_text(10,78,hint,ADIM,30);
    amb_text(162,78,"HOME + EDIT",AINK,12);
    cv_blit_cells(0,147);
    (void)b;
}
