/* SPDX-License-Identifier: GPL-3.0-only */
/* AMB+ native instrument. Saved parameter baselines are separate from runtime
 * modulation and scheduling. No extra playable browser implementation. */
#include "amb_controls.h"
#include "amb_policy.h"
#include "amb_harmony.h"
#include "amb_music.h"
#define AMB_MAGIC 0x50424d41u
#define AMB_VERSION 4u
#define AMB_LAYERS 4u
#define AMB_SLOTS 8u
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
} amb_piece_t;
_Static_assert(sizeof(amb_piece_t) <= ST_PAYLOAD_MAX, "whole AMB+ piece fits existing A/B object");
static amb_piece_t amb;
static int8_t amb_reset_eq[4][4];
static amb_layer_t amb_reset_layer[4]; /* last loaded piece, never a flash snapshot */
/* Main-loop edits publish a revision; audio copies a full deep baseline only
 * when it changes. Macro destinations are restored explicitly every block. */
static volatile uint32_t amb_base_revision[4];
static struct {
    uint32_t phase, step, off[4], due[4], due_step[4], play_t, last_draw, notice_ms;
    uint8_t due_mask[4];
    uint32_t applied_revision[4];
    uint8_t handoff[4],playhead[4],playhead_valid[4];
    uint32_t tail_left[4],tail_total[4];
    int32_t tail_env[4][NVOICE],tail_gain[4];
    uint8_t piece_transition;uint16_t piece_master;int8_t piece_eq[4],tail_eq[4][4];
    uint32_t level_feedback_ms;uint8_t level_feedback_target,level_feedback_active;
    amb_walk_t harmony_walk;
    int32_t evolve_smooth[4];
    uint8_t recent[4][16], recent_vel[4][16], recent_valid[4], reset_pending, detail, foundation_pending[4];
    uint8_t loading, last_play, target, edit_layer, view, page, bank, fx, lane, cursor, recording;
    uint8_t proposal, chord_q, piece_preview, slot, overwrite, dirty, panic, play_used, home_used;
    uint8_t gen_candidate[3], gen_edit[3];
    uint32_t gen_time[3];
    uint8_t held_n[4], held[4][4];
    int voicing[4];
    int8_t solo;
    int16_t macro_smooth[4][4];
    int32_t master_smooth;
    amb_pickup_t pickup;
    char notice[38];
} ambr;
static const uint16_t AMB_SCALE[4] = {0x5adu,0xab5u,0x6adu,0xad5u}; /* minor/major/dorian/lydian */
static const uint8_t AMB_AUTO[8] = {0,1,2,4,8,16,32,64};
static const char *const AMB_ROLE[7] = {"PAD","DRONE","BELLS","BASS","DRUMS","TEXTURE","SAMPLE"};
static const char *const AMB_EDIT[7] = {"MODE","SOUND","ENVELOPE","LFO","PATTERN","FX","LAYER EQ"};
static const char *const AMB_GLO[5] = {"EVOLVE","HARMONY","GENERATE","MASTER FX","EQ"};
static const char *const AMB_MOOD[7] = {"homeward","warmer","brighter","suspended","deeper","open","darker"};
static const char *const AMB_SCENE[6] = {"Morning Air","Rain Study","Dark Water","Low Sun","Glass Room","Night Garden"};
static const char *const AMB_MACRO[7][4] = {
    {"WARMTH","MOTION","SUSTAIN","SPACE"},{"WARMTH","DRIFT","SUSTAIN","SPACE"},
    {"SOFT","MOTION","SHIMMER","SPACE"},{"ROUND","PULSE","GLIDE","SPACE"},
    {"SOFT","DENSITY","DUST","SPACE"},{"BLUR","MOTION","GRAIN","SPACE"},
    {"PITCH","BLUR","MOTION","SPACE"}};
/* Each recipe chooses an actual JIANT preset, then deliberately revoices
 * its envelope, register/colour, gain and sends for the ambient role. */
typedef struct {
    const char *name;
    uint8_t engine, preset, attack, release, level, reverb, delay, color;
} amb_recipe_t;
#define AR(n,e,p,a,r,l,v,d,c) {n,e,p,a,r,l,v,d,c}
static const amb_recipe_t AMB_PATCH[6][8] = {
 {AR("Cloud Bloom",0,1,86,108,92,83,29,51),AR("Soft Choir",2,8,78,112,88,76,36,64),
  AR("Velvet Saw",0,15,71,100,90,68,26,45),AR("Glass Haze",12,12,65,108,84,86,43,72),
  AR("Warm Strings",0,11,82,102,91,72,24,57),AR("Low Sun",0,22,94,114,93,79,35,35),
  AR("Slow Orbit",2,6,88,110,85,85,45,60),AR("Distant Light",0,3,102,118,90,92,32,63)},
 {AR("Deep Current",0,22,110,120,90,75,35,28),AR("Hollow Earth",2,8,103,116,86,80,42,43),
  AR("Quiet Organ",7,0,94,112,80,70,24,58),AR("Tape Hum",0,7,116,122,90,65,48,36),
  AR("Underwater",2,6,105,120,86,87,39,40),AR("Harmonic Bed",7,2,97,115,78,82,34,67),
  AR("Dark Air",11,4,110,124,84,87,43,30),AR("Warm Voltage",0,19,100,117,91,72,33,43)},
 {AR("Felt Glass",12,11,0,100,86,88,45,65),AR("Distant Chime",12,8,4,110,80,94,53,79),
  AR("Wooden Star",12,14,0,86,92,65,37,49),AR("Rain Glass",2,9,3,94,84,84,58,72),
  AR("Soft FM",12,0,8,98,86,80,39,61),AR("Resonant Dust",12,10,1,89,84,73,49,54),
  AR("Floating Metal",0,13,2,106,82,92,56,85),AR("Night Sparks",12,7,0,90,84,83,62,70)},
 {AR("Round Sub",0,7,8,65,101,20,14,32),AR("Soft Pulse",0,2,14,72,96,28,23,39),
  AR("Breathing Low",0,19,48,88,96,38,26,35),AR("Warm Fifth",0,14,20,79,93,34,28,43),
  AR("Faded Mono",2,7,9,69,92,23,19,48),AR("Slow Glide",0,21,25,86,94,42,34,46),
  AR("Low Echo",12,2,12,83,90,36,49,53),AR("Root Bed",0,22,62,98,97,46,22,29)},
 {AR("Dust Steps",10,3,0,75,86,42,23,45),AR("Soft Pulse",10,2,0,81,91,34,19,52),
  AR("Brush Rain",10,5,0,70,82,64,32,60),AR("Broken Clock",10,8,0,62,81,51,46,68),
  AR("Slow Ritual",10,6,0,88,88,58,40,43),AR("Hollow Perc",10,0,0,74,84,70,35,55),
  AR("Hidden Beat",10,1,0,66,82,38,29,49),AR("Sparse Ticks",10,7,0,59,77,65,54,73)},
 {AR("Ocean Air",11,4,110,117,83,89,38,42),AR("Tape Mist",11,7,98,114,82,81,46,55),
  AR("High Air",11,0,87,108,81,93,53,68),AR("Paper Grain",11,6,28,83,78,69,43,64),
  AR("Distant Hiss",11,5,100,116,80,92,39,53),AR("Rainfield",11,1,60,101,77,85,56,73),
  AR("Organic Noise",11,0,105,115,80,88,47,60),AR("Midnight Static",11,5,96,120,76,94,37,41)}
};
#undef AR
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
    unsigned i;const amb_recipe_t *r=&AMB_PATCH[role<6?role:5][patch%8];
    const engine_t *e=ENGINES[role==6?ENGI_AMB_SAMPLE:r->engine];
    const preset_t *pr=&e->presets[role==6?0:r->preset];
    l->role=(uint8_t)role;l->patch=(uint8_t)(patch%8);l->engine=(uint8_t)(role==6?ENGI_AMB_SAMPLE:r->engine);
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
    l->base[P_FCUT]=r->color+25;l->base[P_FRES]=12;
    if(r->engine==0 && role!=6)l->base[P_E4]=r->color;
    if(role==1 || role==3)l->base[P_TRANS]=-12;
    if(role==3){l->base[P_VOICE]=V_MONO;l->base[P_GLIDE]=25+patch*5;}
    if(role==4){l->base[P_E2]=r->color;l->base[P_E3]=38+patch*4;l->base[P_E7]=0;}
    l->lfo_dest=0;l->lfo_depth=6;
    if(role==6){l->base[P_E4]=0;l->base[P_E5]=100;l->base[P_E6]=40;l->base[P_E7]=85;l->base[P_LN7]=100;l->base[P_LN6]=20;}
    for(i=0;i<4;i++)l->macro[i]=64;
}
/* Keep existing v1-v4 saved PHASE texture edits literal; only fresh factory
 * selections use the revised unpitched recipes. Their names stay truthful. */
static int amb_legacy_texture(const amb_layer_t *l)
{return l->role==5 && l->engine==2 && (l->patch==2 || l->patch==4);}
static const char *amb_patch_name(const amb_layer_t *l)
{return amb_legacy_texture(l)?(l->patch==2?"Glass Wind":"Distant Voices"):AMB_PATCH[l->role][l->patch].name;}
static unsigned amb_patch_preset(const amb_layer_t *l)
{return amb_legacy_texture(l)?(l->patch==2?6u:8u):l->role<6?AMB_PATCH[l->role][l->patch].preset:0;}
static void amb_release(unsigned k);
static int amb_tail_releasing(unsigned k){return k<4 && ambr.handoff[k];}
static int amb_tail_any(void){return ambr.handoff[0] || ambr.handoff[1] || ambr.handoff[2] || ambr.handoff[3];}
static int32_t amb_tail_envelope(unsigned k,unsigned voice)
{return mulq16(ambr.tail_env[k][voice],(uint32_t)ambr.tail_gain[k]<<1);}
static int amb_tail_finished(unsigned k){return !ambr.tail_left[k];}
static unsigned amb_track_active(unsigned k)
{unsigned i,n=0;for(i=0;i<NVOICE;i++)n+=trk[k].v[i].active!=0;return n;}
static void amb_begin_change(unsigned k)
{
    unsigned i,frames;
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
    amb_base_revision[k]++;ambr.foundation_pending[k]=(uint8_t)(l->role<2 || l->role==5);
    ambr.dirty=1;
}
static void amb_patch(unsigned k,unsigned role,unsigned patch)
{
    ambr.loading=1;amb_begin_change(k);amb_fill_patch(&amb.layer[k],amb.key,k,role,patch);
    if(!ambr.handoff[k])amb_apply_patch(k);
    ambr.loading=0;ambr.dirty=1;
}
static void amb_make_scene(amb_piece_t *p,unsigned scene)
{
    static const uint8_t roles[6][4]={{0,1,2,4},{0,5,2,4},{1,0,3,5},{0,3,2,5},{2,0,3,4},{5,1,2,4}};
    static const uint8_t roots[6]={0,5,2,7,3,9};unsigned i,j;
    memset(p,0,sizeof *p);p->magic=AMB_MAGIC;p->version=AMB_VERSION;
    amb_policy_init(&p->policy,0x414d4200u+scene);p->release_ms=3000;p->master=760;p->key=roots[scene%6];p->auto_index=3;
    p->evolve[0]=100;p->evolve[1]=35;p->evolve[2]=20;p->evolve[3]=15;
    for(i=0;i<G_COUNT;i++)p->global[i]=GP[i].def;
    p->global[G_BPM]=72+(int)scene*5;p->global[G_RSIZE]=94;p->global[G_RDAMP]=82;
    p->global[G_DMIX]=45;p->global[G_DFDBK]=55;p->global[G_DTIME]=7;p->global[G_RTYPE]=0;
    p->global[G_CLIP]=14;p->global[G_CDEPTH]=35;p->global[G_CRATE]=17;p->global[G_DMOD]=22;
    for(i=0;i<4;i++){
        amb_layer_t *l=&p->layer[i];l->generated=1;l->mode=roles[scene%6][i]==0?3:2;
        l->density=(uint8_t)(i==3?20:35);l->follow=100;l->speed=1;
        for(j=0;j<16;j++)l->velocity[j]=76;
        amb_fill_patch(l,p->key,i,roles[scene%6][i],(scene+i*2)%8);
        l->level=(uint8_t)l->base[P_LEVEL];
    }
}
static amb_piece_t amb_pending;
static volatile uint8_t amb_pending_ready;
static void amb_release(unsigned k);
static void amb_install(const amb_piece_t *p)
{
    unsigned k;
    ambr.loading=1;
    for(k=0;k<4;k++)amb_begin_change(k);
    if(amb_tail_any()){
        if(!ambr.piece_transition){ambr.piece_master=amb.master;memcpy(ambr.piece_eq,amb.eq,4);}
        ambr.piece_transition=1;
        /* Silent old layers also wait for the coordinated piece handoff. */
        for(k=0;k<4;k++)if(!ambr.handoff[k]){ambr.handoff[k]=1;ambr.tail_left[k]=ambr.tail_total[k]=0;memcpy(ambr.tail_eq[k],amb.layer_eq[k],4);}
    }else ambr.piece_transition=0;
    amb=*p;memcpy(amb_reset_layer,amb.layer,sizeof amb_reset_layer);memcpy(amb_reset_eq,amb.layer_eq,sizeof amb_reset_eq);
    memset(ambr.recent_valid,0,sizeof ambr.recent_valid);memset(ambr.playhead_valid,0,sizeof ambr.playhead_valid);memset(&ambr.harmony_walk,0,sizeof ambr.harmony_walk);ambr.reset_pending=0;
    ambr.solo=(int8_t)amb.layer[0].solo_plus1-1;ambr.chord_q=ambr.proposal=255;
    for(k=0;k<4;k++)if(!ambr.handoff[k])amb_apply_patch(k);
    amb_pickup_arm(&ambr.pickup,ambr.target?amb.layer[ambr.target-1].level*1023u/127u:amb.master);
    amb_revoice();ambr.loading=0;ambr.dirty=1;
}
static void amb_factory(unsigned scene)
{
    unsigned i,j;amb_make_scene(&amb_pending,scene);amb_install(&amb_pending);amb_pending_ready=0;
    trk_live=4;song.sel=0;ambr.target=ambr.edit_layer=0;ambr.phase=ambr.step=0;ambr.last_play=0;
    ambr.solo=-1;ambr.piece_preview=255;
    for(i=0;i<4;i++)for(j=0;j<4;j++)ambr.macro_smooth[i][j]=64*256;
    amb_say(AMB_SCENE[scene%6]);
}
static void amb_init(void)
{
    memset(&ambr,0,sizeof ambr);amb_factory(0);
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
static void amb_prepare(void)
{
    unsigned k,i; int32_t desired;
    if(ambr.loading)return;
    if(ambr.piece_transition && !amb_piece_waiting())ambr.piece_transition=0;
    if(!ambr.piece_transition)for(i=0;i<G_COUNT;i++)song.g[i]=amb.global[i];
    for(k=0;k<4;k++) {
        amb_layer_t *l=&amb.layer[k]; track_t *t=&trk[k]; int warm,motion,bloom,space;
        if(ambr.handoff[k]){if(ambr.tail_left[k] || amb_track_active(k) || (ambr.piece_transition && amb_piece_waiting()))continue;amb_apply_patch(k);}
        /* These arrays are naturally 16-bit aligned. The freestanding memcpy
         * copies bytes; doing this at control rate doubled the parameter-copy
         * work on pi32v2. Keep the saved/runtime separation with halfword IO. */
        if(ambr.applied_revision[k]!=amb_base_revision[k]) {
            for(i=0;i<P_COUNT;i++)t->p[i]=l->base[i];
            ambr.applied_revision[k]=amb_base_revision[k];
        }
        t->p[P_PAN]=l->base[P_PAN];
        t->p[P_DLY]=l->base[P_DLY];t->p[P_CHOR]=l->base[P_CHOR];
        t->p[P_E1]=l->base[P_E1];t->p[P_E2]=l->base[P_E2];t->p[P_E3]=l->base[P_E3];
        t->p[P_E4]=l->base[P_E4];t->p[P_E6]=l->base[P_E6];t->p[P_APDEP]=l->base[P_APDEP];
        for(i=0;i<4;i++) {
            int want=l->macro[i]*256;
            ambr.macro_smooth[k][i]+=(want-ambr.macro_smooth[k][i])/32;
        }
        warm=ambr.macro_smooth[k][0]/256-64; motion=ambr.macro_smooth[k][1]/256-64;
        bloom=ambr.macro_smooth[k][2]/256-64; space=ambr.macro_smooth[k][3]/256-64;
        t->eng_req=l->engine;
        t->p[P_LD_FLT]=t->p[P_LD_PIT]=t->p[P_LD_AMP]=0;
        if(l->lfo_dest==0)t->p[P_LD_FLT]=l->lfo_depth/2;
        else if(l->lfo_dest==1)t->p[P_LD_PIT]=l->lfo_depth/12;
        else if(l->lfo_dest==3)t->p[P_LD_AMP]=l->lfo_depth/2;
        t->p[P_LEVEL]=l->level;
        t->p[P_MUTE]=l->base[P_MUTE] || (ambr.solo>=0 && (unsigned)ambr.solo!=k);
        t->p[P_FCUT]=(int16_t)clamp(l->base[P_FCUT]-warm,8,127);
        if(l->engine==0) t->p[P_E4]=(int16_t)clamp(l->base[P_E4]-warm,8,120);
        t->p[P_LD_FLT]=(int16_t)clamp(t->p[P_LD_FLT]+motion/2,-63,63);
        if(l->lfo_dest==2)t->p[P_PAN]=(int16_t)clamp(l->base[P_PAN]+(t->lfo_val*l->lfo_depth)/65536,-64,63);
        t->p[P_REL]=(int16_t)clamp(l->base[P_REL]+bloom/2,0,127);
        t->p[P_REV]=(int16_t)clamp(l->base[P_REV]+space,0,120);
        if(l->role==2) t->p[P_FCUT]=(int16_t)clamp(l->base[P_FCUT]-warm,8,127);
        t->p[P_DLY]=(int16_t)clamp(l->base[P_DLY]+space/2,0,104);
        t->p[P_CHOR]=(int16_t)clamp(l->base[P_CHOR]+motion/2,0,100);
        if(l->engine==12)t->p[P_E2]=(int16_t)clamp(l->base[P_E2]-warm/2,-64,63);
        if(l->engine==11)t->p[P_E1]=(int16_t)clamp(l->base[P_E1]-warm,0,127);
        if(l->engine==2)t->p[P_E2]=(int16_t)clamp(l->base[P_E2]-warm,0,127);
        if(l->engine==7)t->p[P_E3]=(int16_t)clamp(l->base[P_E3]-warm/8,-8,8);
        if(l->role==0 || l->role==1)t->p[P_SUS]=(int16_t)clamp(l->base[P_SUS]+bloom/3,40,127);
        if(l->role==4) t->p[P_E4]=(int16_t)clamp(l->base[P_E4]+bloom/3,0,108);
        if(l->role==6) {
            t->p[P_ATK]=l->base[P_ATK];
            if(l->base[P_E1]==3){t->p[P_ATK]=0;t->p[P_REL]=45;}
            t->p[P_E2]=(int16_t)clamp(l->base[P_E2]+warm/4,-24,24);
            t->p[P_E6]=(int16_t)clamp(l->base[P_E6]+motion/2,0,100);
            t->p[P_APDEP]=(int16_t)clamp(l->base[P_APDEP]+bloom/2,0,64);
        }
        if(l->mode==3 && l->role!=4)t->p[P_VOICE]=V_POLY;
        if(l->role==3) t->p[P_GLIDE]=(int16_t)clamp(l->base[P_GLIDE]+bloom/2,0,100);
        if(song.playing && l->generated) {
            int off=(int)(amb_hash(amb.policy.seed+ambr.step/64+k*31)%17)-8;
            ambr.evolve_smooth[k]+=(off*65536-ambr.evolve_smooth[k])/2048;
            off=ambr.evolve_smooth[k]/65536;
            t->p[P_FCUT]=(int16_t)clamp(t->p[P_FCUT]+off*amb.evolve[1]/100,16,127);
            t->p[P_REV]=(int16_t)clamp(t->p[P_REV]+off*amb.evolve[3]/100,0,112);
        }
    }
    desired=ambr.panic?0:(ambr.piece_transition?ambr.piece_master:amb.master)*256;
    ambr.master_smooth+=(desired-ambr.master_smooth)/32;
    if(desired-ambr.master_smooth<32 && ambr.master_smooth-desired<32) ambr.master_smooth=desired;
    song.master_q12=master_of_pot((uint32_t)(ambr.master_smooth/256));
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
    ambr.recording=0;
    amb_say(on?"AUTO RESUMED / MANUAL NOTES KEPT":"MANUAL PATTERN / EDIT FOR AUTO");
}
static unsigned amb_generated_notes(unsigned k,unsigned step,unsigned mask,uint8_t *notes,unsigned *gate)
{
    amb_layer_t *l=&amb.layer[k];unsigned i,bar=step/16;int note=ambr.voicing[k];
    if(l->role==5 && !amb_legacy_texture(l)){
        if(step%(1u<<l->speed))return 0;
        if(l->mode!=0){
            if(!((mask>>k)&1))return 0;
            if(l->mode==1 && step%8)return 0;
            if(l->mode==3 && step%16>3)return 0;
            if(l->mode==4 && step%32)return 0;
        }
        notes[0]=60;*gate=l->mode==0?FS*60u:l->mode==2?FS*6u:FS;return 1;
    }
    if(!((mask>>k)&1) || step%(1u<<l->speed))return 0;
    if(l->mode==4 && (step%16 || bar%4==3))return 0;
    if(l->role==0){
        static const uint8_t pair[6][2]={{0,2},{1,3},{0,3},{0,1},{2,3},{1,2}};
        unsigned turn=step/8,phrase=amb.policy.active[1],sel;
        if(step%4)return 0;
        sel=amb_pad_pair(amb.policy.seed+k*97u,phrase,turn);
        i=l->mode==0?turn%4:l->mode==1?3-turn%4:l->mode==3?pair[sel][0]:amb_hash(amb.policy.seed+turn+phrase*31u)%4;
        notes[0]=(uint8_t)ambr.voicing[i];*gate=FS*(8u+l->macro[2]/4u);
        if(l->mode==3){
            int shift=((turn/24+phrase)%3==1 && phrase!=7)?12:0;
            notes[0]=(uint8_t)clamp((int)notes[0]+shift,42,89);
            notes[1]=(uint8_t)clamp(ambr.voicing[pair[sel][1]]+shift,42,89);return 2;
        }return 1;
    }
    if(l->mode==3){notes[0]=(uint8_t)ambr.voicing[k%4];notes[1]=(uint8_t)ambr.voicing[(k+2)%4];*gate=FS*6;return 2;}
    if(l->role==1){if(step%16)return 0;note=ambr.voicing[0];*gate=FS*(12u+l->macro[2]/3u);}
    else if(l->role==3){note=36+(ambr.voicing[0]%12);*gate=FS;}
    else if(l->role==2){
        unsigned motif=amb.policy.active[1]==6?step%128:step;
        if(l->mutation && amb_hash(amb.policy.seed+step)%100<l->mutation)motif+=bar*13;
        i=l->mode==0?(step>>l->speed)%4:l->mode==1?3-(step>>l->speed)%4:amb_hash(amb.policy.seed+motif/3+k)%4;
        note=ambr.voicing[i]+12;*gate=FS*2;
    }else *gate=FS*5;
    if(amb_hash(amb.policy.seed+k*37+step/64)%100>=l->follow)note=48+(int)k*5;
    notes[0]=(uint8_t)clamp(note,24,96);return 1;
}
static void amb_participation(amb_participation_t *part,unsigned step)
{
    unsigned k;for(k=0;k<4;k++)part[k]=(amb_participation_t){amb.layer[k].generated && !amb.layer[k].base[P_MUTE] && (ambr.solo<0 || (unsigned)ambr.solo==k),(uint8_t)clamp(amb.layer[k].density+
        ((amb.layer[k].role==4)?(ambr.macro_smooth[k][1]/256-64)/2:
         (amb.layer[k].role==0 || amb.layer[k].role==1)?(ambr.macro_smooth[k][2]/256-64)/2:0)+
        ((int)amb_triangle(step/16,32)-50)*amb.evolve[2]/500,0,100)};
}
static void amb_capture_generated(unsigned k)
{
    amb_layer_t *l=&amb.layer[k];unsigned i,n=0;
    for(i=0;i<16;i++)n+=ambr.recent[k][i]!=0;
    if(ambr.recent_valid[k] && n){memcpy(l->notes,ambr.recent[k],16);memcpy(l->velocity,ambr.recent_vel[k],16);}
    else {
        amb_policy_t preview=amb.policy;amb_participation_t parts[4];
        unsigned base=(ambr.step/64)*64;
        for(i=0;i<16;i++){
            unsigned gate=FS,step=base+(i<<l->speed);uint8_t notes[2],mask;
            amb_participation(parts,step);amb_policy_boundary(&preview,step);mask=amb_policy_events(&preview,step,parts);
            l->notes[i]=amb_generated_notes(k,step,mask,notes,&gate)?notes[0]:0;l->velocity[i]=(uint8_t)(52+amb_hash(step+k)%25);
        }
    }
    /* A very sparse phrase can contain no event in this window. Seed its
     * current actual voicing rather than opening an empty recording. */
    for(i=n=0;i<16;i++)n+=l->notes[i]!=0;
    if(!n){l->notes[0]=(uint8_t)(ambr.voicing[k]+(l->role==2?12:0));l->velocity[0]=70;}
}
static void amb_layer_step(unsigned k,unsigned step,unsigned mask)
{
    amb_layer_t *l=&amb.layer[k];unsigned i,tick=step%16,bar=step/16,gate=FS/2,n;uint8_t notes[2];
    if(ambr.handoff[k] || trk[k].engine!=l->engine || l->base[P_MUTE] || (ambr.solo>=0 && (unsigned)ambr.solo!=k))return;
    if(!(step%(1u<<l->speed))){ambr.playhead[k]=(uint8_t)((step>>l->speed)%16);ambr.playhead_valid[k]=1;}
    if(l->role==4){
        ambr.playhead[k]=(uint8_t)(step%16);ambr.playhead_valid[k]=1;
        static const uint8_t drum[4]={36,38,42,50};
        if(!l->generated){for(i=0;i<4;i++)if((l->drums[i]>>tick)&1u)trk_note_on(&trk[k],drum[i],64+i*4);}
        else if((mask>>k)&1){unsigned lane=amb_hash(amb.policy.seed+step+k*53+bar*l->mutation)%4;trk_note_on(&trk[k],drum[lane],52+amb_hash(step)%24);
            if(l->fill && tick==15 && bar%4==3)trk_note_on(&trk[k],42,45+l->fill/4);}
        return;
    }
    if(!l->generated){
        if(step%(1u<<l->speed))return;
        if(!l->notes[(step>>l->speed)%16]){if(l->role==5)amb_release(k);return;}
        if(l->role<2 || l->role==5){
            gate=FS*(l->role==0?8u+l->macro[2]/4u:12u+l->macro[2]/3u);
            if(amb_carry_note(k,l->notes[(step>>l->speed)%16],gate))return;
        }
        amb_release(k);amb_note(k,l->notes[(step>>l->speed)%16],l->velocity[(step>>l->speed)%16],gate);return;
    }
    n=amb_generated_notes(k,step,mask,notes,&gate);
    if(!(step% (1u<<l->speed))){i=(step>>l->speed)%16;ambr.recent[k][i]=n?notes[0]:0;ambr.recent_vel[k][i]=(uint8_t)(52+amb_hash(step+k)%25);ambr.recent_valid[k]=1;}
    if(!n)return;
    if(n==1 && (l->role==0 || l->role==1 || l->role==5) && amb_carry_note(k,notes[0],gate))return;
    if(l->role==0 && amb_keep_pad(k,notes,n,gate))return;
    amb_release(k);for(i=0;i<n;i++)amb_note(k,notes[i],i?55:65,gate);
}
static void amb_step(void)
{
    amb_participation_t part[4]; uint8_t mask; unsigned k,bar=ambr.step/16;
    if(ambr.step%16==0) {
        if(amb_pending_ready){amb_install(&amb_pending);amb_pending_ready=0;amb_say("PRESET RECALLED ON BAR");}
        else if(ambr.chord_q!=255) { amb.chord=ambr.chord_q;ambr.chord_q=255;amb_revoice(); }
        else if(bar && AMB_AUTO[amb.auto_index] && bar%AMB_AUTO[amb.auto_index]==0 && amb_hash(amb.policy.seed+bar)%100u<amb.evolve[0]) {
            int scale[12];unsigned count=(unsigned)amb_scale(amb_pool(),scale);
            amb.chord=(uint8_t)amb_walk_next(&ambr.harmony_walk,amb.policy.seed,amb.chord,count);amb_revoice();
        }
    }
    {uint8_t before[3];memcpy(before,amb.policy.active,3);
        amb_policy_boundary(&amb.policy,ambr.step);
        if(memcmp(before,amb.policy.active,3))for(k=0;k<4;k++)if(amb.layer[k].generated)amb_begin_change(k);
    }
    amb_participation(part,ambr.step);
    mask=amb_policy_events(&amb.policy,ambr.step,part);
    for(k=0;k<4;k++) {
        unsigned delay=(ambr.step&1u)?(FS*60u/(unsigned)(amb.global[G_BPM]*4))*amb.layer[k].swing/200u:0;
        if(delay) {ambr.due[k]=delay;ambr.due_step[k]=ambr.step;ambr.due_mask[k]=mask;}
        else amb_layer_step(k,ambr.step,mask);
    }
    ambr.step++;ambr.dirty=1;
}
static void amb_clock(uint32_t n)
{
    unsigned k;
    if(ambr.loading)return;
    for(k=0;k<4;k++)if(ambr.handoff[k]){
        unsigned elapsed,p,q;
        ambr.tail_left[k]=ambr.tail_left[k]>n?ambr.tail_left[k]-n:0;
        if(!ambr.tail_total[k]){ambr.tail_gain[k]=0;continue;}
        elapsed=ambr.tail_total[k]-ambr.tail_left[k];
        p=(unsigned)(((uint64_t)elapsed<<15)/ambr.tail_total[k]);q=(p*p)>>15;
        q=(q*(98304u-2u*p))>>15;ambr.tail_gain[k]=(int32_t)(32768u-q);
    }
    for(k=0;k<4;k++) if(ambr.off[k]) {
        if(ambr.off[k]<=n) amb_release(k); else ambr.off[k]-=n;
    }
    if(!song.playing) {
        for(k=0;k<4;k++)ambr.due[k]=0;
        if(ambr.last_play) for(k=0;k<4;k++) amb_release(k);
        ambr.last_play=0;return;
    }
    if(!ambr.last_play){
        ambr.phase=ambr.step=0;ambr.last_play=1;
        for(k=0;k<4;k++)ambr.foundation_pending[k]=(uint8_t)((amb.layer[k].role<2 || amb.layer[k].role==5) && amb.layer[k].generated);
        amb_step();
    }
    for(k=0;k<4;k++)if(ambr.foundation_pending[k] && !ambr.handoff[k] && trk[k].engine==amb.layer[k].engine){
        ambr.foundation_pending[k]=0;
        if(amb.layer[k].generated && !ambr.held_n[k])amb_layer_step(k,amb.layer[k].role==1?(ambr.step/16u)*16u:(ambr.step/4u)*4u,1u<<k);
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
    unsigned target=ambr.target;
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
/* Bounded three-band output EQ. One-pole splits at ~180 Hz / ~3 kHz;
 * smoothed gains, headroom and a final soft knee, both stereo channels. */
static void amb_output(int32_t *out,uint32_t n)
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
/* Three broad bands per layer before dry mix and shared sends. Flat EQ is
 * bit-exact bypass. Independent state per physical track, safe on two cores. */
static void amb_layer_process(unsigned k,int32_t *b,uint32_t n)
{
    static int32_t low[4][6],high[4][6],g[4][4];static uint8_t primed[4];
    static const uint16_t db[25]={1029,1154,1295,1453,1631,1830,2053,2304,2584,2900,3254,3650,4096,4596,5157,5786,6492,7285,8173,9170,10289,11544,12952,14532,16306};
    unsigned i,j,c,flat=1;if(k>=4)return;
    if(!primed[k]){for(j=0;j<4;j++)g[k][j]=4096;primed[k]=1;}
    for(j=0;j<4;j++){
        int target=db[(ambr.handoff[k]?ambr.tail_eq[k][j]:amb.layer_eq[k][j])+12],d=target-g[k][j];
        if(target!=4096 || g[k][j]!=4096)flat=0;
        g[k][j]+=d/16;if(d>-16 && d<16)g[k][j]=target;
    }
    if(flat)return;
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
       p->master>1023 || p->key>11 || p->scale>3 || p->chord>6 || p->auto_index>7) return 0;
    if(p->version>=3 && (((unsigned)p->layer[1].reserved>15u) ||
       ((p->layer[0].reserved || p->layer[1].reserved) && !(p->layer[0].reserved&1u))))return 0;
    if(p->version>=3){unsigned m=p->layer[0].reserved|((unsigned)p->layer[1].reserved<<8),n=0;
        for(i=0;i<12;i++)n+=(m>>i)&1u;if(m && n<3)return 0;}
    if(p->version>=4){
        if(p->release_ms<250 || p->release_ms>6000 || p->release_ms%250)return 0;
        for(k=0;k<4;k++)for(i=0;i<4;i++)if(p->layer_eq[k][i]<-12 || p->layer_eq[k][i]>12)return 0;
    }
    if(p->policy.variation>100) return 0;
    for(i=0;i<3;i++) if(p->policy.active[i]>=(i==2?6:8) ||
        (p->policy.pending[i]!=255 && p->policy.pending[i]>=(i==2?6:8))) return 0;
    for(i=0;i<4;i++) if(p->eq[i]<-12 || p->eq[i]>12 || p->evolve[i]>100) return 0;
    for(i=0;i<G_COUNT;i++) if(p->global[i]<GP[i].min || p->global[i]>GP[i].max) return 0;
    for(k=0;k<4;k++) {
        const amb_layer_t *l=&p->layer[k];
        if(l->role>6 || l->patch>7 || !eng_ok(l->engine) || l->level>127 || l->generated>1 ||
           l->solo_plus1>4 || l->lfo_dest>3 || l->lfo_depth>100 || l->speed>3 || l->mode>4 || l->density>100 || l->swing>100 || l->fill>100 || l->mutation>100 || l->follow>100) return 0;
        if(!amb_legacy_texture(l) && l->engine!=(l->role==6?ENGI_AMB_SAMPLE:AMB_PATCH[l->role][l->patch].engine))return 0;
        for(i=0;i<4;i++) if(l->macro[i]>127) return 0;
        for(i=0;i<16;i++) if(l->notes[i]>127 || l->velocity[i]>127) return 0;
        for(i=0;i<P_COUNT;i++) {
            const param_desc_t *d=param_desc_of(l->engine,i);
            if(l->base[i]<d->min || l->base[i]>d->max) return 0;
        }
    }
    return 1;
}
static void amb_save(void)
{
    if(song.playing) {amb_say("STOP PLAYBACK TO SAVE");return;}
#if FELUCCA_FLASH
    if(!flash_ok) {amb_say("UNSUPPORTED FLASH - SAVE REFUSED");return;}
#endif
    if(amb_occupied(ambr.slot) && !ambr.overwrite) {ambr.overwrite=1;amb_say("OCCUPIED - REC AGAIN");return;}
    if(!amb_valid(&amb)) {amb_say("INVALID STATE - SAVE REFUSED");return;}
    if(st_save(amb_object(ambr.slot),&amb,sizeof amb)) {amb_say("SAVE FAILED - OLD COPY KEPT");return;}
    ambr.overwrite=0;amb_say("SAVED - DATE UNAVAILABLE");
}
static int amb_stage_slot(unsigned slot)
{
    int n;amb_pending_ready=0;
    memset(&amb_pending,0,sizeof amb_pending);
    n=st_load(amb_object(slot),&amb_pending,sizeof amb_pending);
    if(!((n==1400 && amb_pending.version<=3) || (n==(int)sizeof amb_pending && amb_pending.version==AMB_VERSION)) || !amb_valid(&amb_pending)){amb_say("EMPTY / INCOMPATIBLE / DAMAGED");return 0;}
    if(amb_pending.version==1)amb_policy_init(&amb_pending.policy,0x414d4200u);
    if(amb_pending.version<3){amb_pending.layer[0].reserved=amb_pending.layer[1].reserved=0;}
    if(amb_pending.version<4){memset(amb_pending.layer_eq,0,sizeof amb_pending.layer_eq);amb_pending.release_ms=3000;}
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
static unsigned amb_sound_id(const amb_layer_t *l,unsigned bank,unsigned knob)
{
    static const uint8_t analog[2][4]={{P_E0,P_E1,P_E4,P_E5},{P_E2,P_E3,P_E6,P_E7}};
    static const uint8_t fm[2][4]={{P_E3,P_E2,P_E1,P_FCUT},{P_E6,P_GLIDE,P_E4,P_E0}};
    static const uint8_t filter[4]={P_FTYPE,P_FCUT,P_FRES,P_DIST};
    static const uint8_t play[4]={P_GLIDE,P_VOICE,P_TRANS,P_PAN};
    if(l->engine==ENGI_AMB_SAMPLE){
        static const uint8_t ids[3][4]={{P_E4,P_E5,P_LN7,P_LN6},{P_E1,P_E2,P_E3,P_E6},{P_E6,P_FCUT,P_APDEP,P_REV}};
        return ids[bank%3][knob];
    }
    if(bank && ambr.detail==1){if(l->engine==0 && knob>0)return P_E3+knob;return filter[knob];}
    if(bank && ambr.detail==2)return play[knob];
    if(l->engine==0)return analog[bank%2][knob];
    if(l->engine==12)return fm[bank%2][knob];
    return P_E0+(bank%2)*4+knob;
}
static void amb_knob(unsigned knob,int delta)
{
    amb_layer_t *l=&amb.layer[ambr.edit_layer]; unsigned id=0; int v;
    unsigned old_key=amb.key,old_pool=amb_pool();
    if(ambr.reset_pending)return;
    if(ambr.view==0 || (ambr.view==1 && ambr.page==0)) {
        if(ambr.target==0 && ambr.view==0) {
            static const uint8_t ids[4]={G_RSIZE,G_DMIX,G_CDEPTH,G_DJF};
            id=ids[knob];amb.global[id]=(int16_t)clamp(amb.global[id]+(knob==3?-delta*2:delta*4),GP[id].min,GP[id].max);
        } else {
            if(ambr.view==0) l=&amb.layer[ambr.target-1];
            l->macro[knob]=(uint8_t)clamp(l->macro[knob]+delta*2,0,127);
        }
    } else if(ambr.view==1) {
        if(ambr.page==1) id=amb_sound_id(l,ambr.bank,knob);
        else if(ambr.page==2) id=P_ATK+knob;
        else if(ambr.page==3) {
            if(knob==0)id=P_LWAVE;
            else if(knob==1)id=P_LRATE;
            else if(knob==2)l->lfo_depth=(uint8_t)clamp(l->lfo_depth+delta*2,0,100);
            else l->lfo_dest=(uint8_t)amb_wrap(l->lfo_dest+delta,4);
        } else if(ambr.page==4) {
            if(l->role==4) {
                uint8_t *v0=knob==0?&l->density:knob==1?&l->swing:knob==2?&l->fill:&l->mutation;
                *v0=(uint8_t)clamp(*v0+delta*2,0,100);
            } else if(knob==0) l->mode=(uint8_t)amb_wrap(l->mode+delta,5);
            else if(knob==1) l->speed=(uint8_t)clamp(l->speed+delta,0,3);
            else if(knob==2) l->density=(uint8_t)clamp(l->density+delta*2,0,100);
            else l->follow=(uint8_t)clamp(l->follow+delta*2,0,100);
        } else if(ambr.page==6)amb.layer_eq[ambr.edit_layer][knob]=(int8_t)clamp(amb.layer_eq[ambr.edit_layer][knob]+delta,-12,12);
        else if(ambr.page==5) {
            static const uint8_t ids[5][4]={{P_DTYPE,P_DIST,P_DTONE,P_FCUT},{P_CHOR,P_APRATE,P_APDEP,P_APWAVE},
                {P_DLY,P_FCUT,P_PAN,P_DIST},{P_REV,P_FCUT,P_PAN,P_CHOR},{P_PAN,P_APDEP,P_APRATE,P_APWAVE}};
            id=ids[ambr.fx%5][knob];
        }
        if(id) {const param_desc_t *d=param_desc_of(l->engine,id);l->base[id]=(int16_t)param_turn(d,l->base[id],delta);}
    } else if(ambr.view==2) {
        if(ambr.page==0) amb.evolve[knob]=(uint8_t)clamp(amb.evolve[knob]+delta*3,0,100);
        else if(ambr.page==1) {
            if(knob==0) amb.key=(uint8_t)amb_wrap(amb.key+delta,12);
            else if(knob==1){amb.scale=(uint8_t)amb_wrap(amb.scale+delta,4);amb.layer[0].reserved=amb.layer[1].reserved=0;}
            else if(knob==2) amb.global[G_BPM]=(int16_t)clamp(amb.global[G_BPM]+delta,40,140);
            else amb.release_ms=(uint16_t)clamp((int)amb.release_ms+delta*250,250,6000);
            if(knob<2 && (amb.key!=old_key || amb_pool()!=old_pool))amb_revoice();
        } else if(ambr.page==2) {
            if(knob<3) {
                unsigned cur=ambr.gen_edit[knob]?ambr.gen_candidate[knob]:amb.policy.pending[knob]!=255?amb.policy.pending[knob]:amb.policy.active[knob];
                ambr.gen_candidate[knob]=(uint8_t)amb_wrap((int)cur+delta,knob==2?6:8);
                ambr.gen_edit[knob]=1;ambr.gen_time[knob]=fm1_ms;
            } else amb.policy.variation=(uint8_t)clamp(amb.policy.variation+delta*2,0,100);
        } else if(ambr.page==3) {
            static const uint8_t ids[4][4]={{G_RSIZE,G_RFILT,G_RDAMP,G_RTYPE},{G_DMIX,G_DTIME,G_DFDBK,G_DTYPE},
                {G_CDEPTH,G_CRATE,G_WIDTH,G_RMOD},{G_DJF,G_CLIP,G_DCOLOR,G_DMOD}};
            const param_desc_t *d;id=ids[ambr.fx%4][knob];d=&GP[id];
            amb.global[id]=(int16_t)param_turn(d,amb.global[id],delta);
            if(id==G_DFDBK && amb.global[id]>86) amb.global[id]=86;
        } else amb.eq[knob]=(int8_t)clamp(amb.eq[knob]+delta,-12,12);
    }
    if(ambr.view==1 && ambr.page>0 && ambr.page<6 && ambr.page!=4) {
        unsigned k;for(k=0;k<4;k++)amb_base_revision[k]++;
    }
    ambr.dirty=1;
}
static void amb_rec(void)
{
    if(ambr.reset_pending){
        unsigned k=ambr.edit_layer,solo=amb.layer[0].solo_plus1;uint8_t p0=amb.layer[0].reserved,p1=amb.layer[1].reserved;
        ambr.loading=1;amb_begin_change(k);amb.layer[k]=amb_reset_layer[k];memcpy(amb.layer_eq[k],amb_reset_eq[k],4);
        amb.layer[0].solo_plus1=(uint8_t)solo;amb.layer[0].reserved=p0;amb.layer[1].reserved=p1;
        if(!ambr.handoff[k])amb_apply_patch(k);
        ambr.recent_valid[k]=0;ambr.reset_pending=0;ambr.loading=0;
        if(ambr.target==k+1)amb_pickup_arm(&ambr.pickup,amb.layer[k].level*1023u/127u);
        amb_say("TRACK RESTORED TO LOADED PIECE");return;
    }
    if(ambr.view==2 && ambr.page==2)return;
    if(ambr.view==3) {amb_save();return;}
    if(ambr.view==1 && ambr.page==4) {
        amb_layer_t *l=&amb.layer[ambr.edit_layer];
        if(!ambr.recording && l->generated && l->role!=4)amb_capture_generated(ambr.edit_layer);
        ambr.recording=!ambr.recording;l->generated=0;ambr.dirty=1;
        amb_say(ambr.recording?"STEP ENTRY / GENERATED NOTES KEPT":"RECORDING STOPPED");return;
    }
    if(ambr.piece_preview!=255) {
        unsigned p=ambr.piece_preview;
        if(p<6){
            if(song.playing){amb_pending_ready=0;amb_make_scene(&amb_pending,p);RING_PUBLISH();amb_pending_ready=1;amb_say("RECALL QUEUED TO NEXT BAR");}
            else amb_factory(p);
        }else amb_recall(p-6);
        ambr.piece_preview=255;return;
    }
    if(ambr.proposal==255) {if(amb_pending_ready){amb_pending_ready=0;amb_say("QUEUED RECALL CANCELLED");}else if(ambr.chord_q!=255){ambr.chord_q=255;amb_say("QUEUED CHORD CANCELLED");}return;}
    if(ambr.view==1 && ambr.page==0) {
        amb_patch(ambr.edit_layer,ambr.proposal,0);
    } else if(ambr.view==0 || (ambr.view==2 && ambr.page==0)) {
        if(song.playing) ambr.chord_q=ambr.proposal;
        else {amb.chord=ambr.proposal;amb_revoice();}
    }
    ambr.proposal=255;ambr.dirty=1;
}
static void amb_input(void)
{
    uint32_t pressed=fm1_input_edges(0),notes=fm1_input_note_edges();
    unsigned b,k; int s,sel=panel_enc(EN_SELECT),algo=panel_enc(EN_ALGO),pre=panel_enc(EN_PRESET);
    unsigned home=(fm1_in.buttons>>panel.btn[B_HOME])&1u,play=(fm1_in.buttons>>panel.btn[B_PLAY])&1u;
    static uint8_t home0,play0;
    for(k=0;k<3;k++) if(ambr.gen_edit[k] && fm1_ms-ambr.gen_time[k]>=90u) {
        amb_policy_choose(&amb.policy,k,ambr.gen_candidate[k],song.playing);
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
    if(home && sel) {
        ambr.home_used=1;
        if(ambr.view==1) {ambr.edit_layer=(uint8_t)amb_wrap(ambr.edit_layer+sel,4);song.sel=ambr.edit_layer;ambr.proposal=255;ambr.dirty=1;}
        sel=0;
    }
    if(!home && home0 && !ambr.home_used) {
        ambr.view=0;ambr.recording=0;ambr.reset_pending=0;ambr.overwrite=0;amb_pending_ready=0;
        if(ambr.target){ambr.edit_layer=ambr.target-1;song.sel=ambr.edit_layer;}ambr.proposal=ambr.piece_preview=255;ambr.dirty=1;
    }
    home0=(uint8_t)home;play0=(uint8_t)play;
    for(b=0;b<NB;b++) if((pressed>>panel.btn[b])&1u) {
        if(b==B_HOME || b==B_PLAY) continue;
        if(home && (b==B_OCTDN || b==B_OCTUP)) {
            ambr.home_used=1;
            if(ambr.target) {
                k=ambr.target-1;
                if(b==B_OCTDN){amb.layer[k].base[P_MUTE]=!amb.layer[k].base[P_MUTE];if(amb.layer[k].base[P_MUTE])amb_release(k);else ambr.foundation_pending[k]=(uint8_t)(amb.layer[k].role<2 || amb.layer[k].role==5);}
                else {unsigned j;ambr.solo=ambr.solo==(int)k?-1:(int8_t)k;amb.layer[0].solo_plus1=(uint8_t)(ambr.solo+1);
                    for(j=0;j<4;j++)if(ambr.solo>=0 && j!=(unsigned)ambr.solo)amb_release(j);
                    else ambr.foundation_pending[j]=(uint8_t)(amb.layer[j].role<2 || amb.layer[j].role==5);
                }
            }
        } else if(b==B_EDIT) {
            if(ambr.view==1 && ambr.page==0){ambr.reset_pending^=1;ambr.proposal=255;}
            else if(ambr.view==1 && ambr.page==1) ambr.bank=(uint8_t)((ambr.bank+1)%(amb.layer[ambr.edit_layer].role==6?3:2));
            else if(ambr.view==1 && ambr.page==4) {amb_set_generated(ambr.edit_layer,!amb.layer[ambr.edit_layer].generated);}
            else {ambr.view=ambr.target?1:2;ambr.page=ambr.target?0:3;ambr.bank=0;ambr.detail=0;ambr.proposal=ambr.piece_preview=255;}
        } else if(b==B_GLO) {ambr.view=2;ambr.page=0;ambr.recording=0;ambr.reset_pending=0;ambr.proposal=ambr.piece_preview=255;}
        else if(b==B_SCL) {
            if(ambr.view==1 || ambr.view==2) {ambr.reset_pending=0;ambr.page=(uint8_t)((ambr.page+1)%(ambr.view==1?7:5));ambr.recording=0;ambr.bank=0;ambr.detail=0;ambr.proposal=255;}
        } else if(b==B_SAVE) {ambr.reset_pending=0;ambr.view=3;ambr.slot=0;ambr.overwrite=0;ambr.recording=0;ambr.proposal=ambr.piece_preview=255;}
        else if(b==B_REC) amb_rec();
        else if(b==B_ARP && ambr.view==1 && ambr.page==4 && amb.layer[ambr.edit_layer].role!=4 && !amb.layer[ambr.edit_layer].generated){
            amb.layer[ambr.edit_layer].notes[ambr.cursor]=0;amb_say("SELECTED STEP CLEARED");
        }
        else if(b==B_OCTDN || b==B_OCTUP) song.octave=(int8_t)clamp(song.octave+(b==B_OCTUP?1:-1),-3,3);
        else if(b==B_ENV || b==B_LFO || b==B_SEQ || b==B_FX || b==B_ARP) {
            if(ambr.target && b==B_SEQ && ambr.view==1 && ambr.page==4)amb_set_generated(ambr.edit_layer,!amb.layer[ambr.edit_layer].generated);
            else if(ambr.target) {ambr.reset_pending=0;ambr.recording=0;ambr.view=1;ambr.page=b==B_ENV?2:b==B_LFO?3:b==B_FX?5:4;}
        }
        ambr.dirty=1;
    }
    if(sel) {
        ambr.reset_pending=0;
        if(ambr.piece_preview!=255) ambr.piece_preview=(uint8_t)amb_wrap(ambr.piece_preview+sel,14);
        else if(ambr.view==0) amb_target(amb_wrap(ambr.target+sel,5));
        else if(ambr.view==3) {ambr.slot=(uint8_t)amb_wrap(ambr.slot+sel,8);ambr.overwrite=0;}
        else if(ambr.view==1) {
            amb_layer_t *l=&amb.layer[ambr.edit_layer];
            if(ambr.page==1) {
                if(l->role==6){if(ambr.bank==0){amb_begin_change(ambr.edit_layer);l->base[P_E0]=(int16_t)amb_wrap(l->base[P_E0]+sel,2);if(!ambr.handoff[ambr.edit_layer])amb_apply_patch(ambr.edit_layer);}}
                else if(ambr.bank==0)amb_patch(ambr.edit_layer,l->role,amb_wrap(l->patch+sel,8));
                else ambr.detail=(uint8_t)amb_wrap(ambr.detail+sel,3);
            }
            else if(ambr.page==4) {
                if(l->role==4) ambr.lane=(uint8_t)amb_wrap(ambr.lane+sel,4);
                else ambr.cursor=(uint8_t)amb_wrap(ambr.cursor+sel,16);
            } else if(ambr.page==5) ambr.fx=(uint8_t)amb_wrap(ambr.fx+sel,5);
        } else if(ambr.view==2) {
            if(ambr.page==0) amb.auto_index=(uint8_t)amb_wrap(amb.auto_index+sel,8);
            else if(ambr.page==3) ambr.fx=(uint8_t)amb_wrap(ambr.fx+sel,4);
            /* SELECT deliberately inert on GENERATE. */
        }
        ambr.dirty=1;
    }
    if(algo) {
        ambr.reset_pending=0;
        if(ambr.view==1 && ambr.page==0) {
            unsigned cur=ambr.proposal==255?amb.layer[ambr.edit_layer].role:ambr.proposal;
            ambr.proposal=(uint8_t)amb_wrap((int)cur+algo,7);
            if(ambr.proposal==amb.layer[ambr.edit_layer].role) ambr.proposal=255;
        } else if(ambr.view==0 || (ambr.view==2 && ambr.page==0)) {
            unsigned cur=ambr.proposal==255?amb.chord:ambr.proposal;
            ambr.proposal=(uint8_t)amb_wrap((int)cur+algo,7);
            if(ambr.proposal==amb.chord) ambr.proposal=255;
        }
        ambr.dirty=1;
    }
    if(pre && ambr.view!=3) {
        ambr.reset_pending=0;
        ambr.piece_preview=(uint8_t)amb_wrap((ambr.piece_preview==255?0:ambr.piece_preview)+pre,14);
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
            if(l->role==4) {
                if(!key_black(k)) l->drums[ambr.lane]^=(uint16_t)(1u<<key_place(k));
            } else {unsigned at=ambr.cursor;
                l->notes[at]=(uint8_t)clamp(53+(int)k+song.octave*12,0,127);l->velocity[at]=88;
                ambr.cursor=(uint8_t)((at+1)%16);}
            ambr.dirty=1;
        }
        /* Step entry is silent on pitched and drum layers, even during PLAY. */
    }
    song.grid=(ambr.view==0 || (ambr.view==1 && ambr.page==4) || amb_tail_releasing(ambr.edit_layer))?2:0;
    song.rec=0;
}
static int amb_rec_action(void)
{
    if(ambr.reset_pending)return 1;
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
        int action=b==B_PLAY || (b==B_SAVE && (ambr.view!=3 || ambr.slot || ambr.overwrite)) ||
            (b==B_GLO && (ambr.view!=2 || ambr.page!=0 || ambr.proposal!=255 || ambr.piece_preview!=255)) ||
            (b==B_EDIT && edit_action) || b==B_OCTDN || b==B_OCTUP ||
            (b==B_HOME && (ambr.target || ambr.view || ambr.proposal!=255 || ambr.piece_preview!=255 || amb_pending_ready)) || (b==B_SCL && (ambr.view==1 || ambr.view==2)) ||
            (b==B_REC && amb_rec_action()) || (ambr.target && (b==B_ENV || b==B_LFO || b==B_SEQ || b==B_FX || (b==B_ARP && (!(ambr.view==1 && ambr.page==4) || (!amb.layer[ambr.edit_layer].generated && amb.layer[ambr.edit_layer].notes[ambr.cursor])))));
        if(action) led_put(lit,panel.btn[b],1);
    }
    if(ambr.recording) led_put(lit,panel.btn[B_REC],1);
    else if(amb_rec_action() && (amb_pending_ready || ambr.proposal!=255 || ambr.chord_q!=255 || ambr.piece_preview!=255 || ambr.overwrite || ambr.reset_pending)) {led_clear(lit,panel.btn[B_REC]);led_put(breath,panel.btn[B_REC],1);}
    for(k=0;k<27;k++) {
        unsigned pc=(53+k+12-amb.key)%12;
        int scale=(amb_pool()>>pc)&1u;
        if(ambr.recording && amb.layer[ambr.edit_layer].role==4 && !key_black(k)) {
            led_put(dim,14+k,1);
            if((amb.layer[ambr.edit_layer].drums[ambr.lane]>>key_place(k))&1u)led_put(lit,14+k,1);
        }
        if((ambr.view==0 || (ambr.recording && amb.layer[ambr.edit_layer].role!=4)) && scale)led_put(hi,14+k,1);
        if(ambr.view==0){int j;
            for(j=0;j<nt;j++)if((53u+k)%12u==(unsigned)pcs[j])led_put(lit,14+k,1);}

        if((fm1_in.notes>>k)&1) led_put(lit,14+k,1);
        else if(ambr.recording && !pc) led_put(lit,14+k,1);
    }
    if(song.playing) lit[LED_PLAY_GREEN>>3]|=(uint8_t)(1u<<(LED_PLAY_GREEN&7u));
    for(k=0;k<FM1_NCOL;k++) {fm1_led[k]=lit[k];fm1_led_dim[k]=dim[k];fm1_led_breath[k]=breath[k];fm1_led_mid[k]=0;fm1_led_hi[k]=hi[k];}
}
#define ABG RGB(6,17,8)
#define AINK RGB(215,255,164)
#define ADIM RGB(135,177,113)
#define ALINE RGB(49,79,48)
#define ASEL RGB(28,58,35)
#define AQUEUE RGB(235,205,140)
static void amb_text(int x,int y,const char *s,uint16_t color,unsigned maxchars)
{
    char b[40];unsigned i=0;
    while(s[i] && i<maxchars && i<sizeof b-1) {b[i]=s[i];i++;}
    b[i]=0;cv_text(x,y,&AF_X,b,color);
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
    if(ambr.view==0 || (ambr.view==1 && ambr.page==0)) {
        if(ambr.view==0 && ambr.target==0) {
            static const char *const labels[4]={"SPACE","ECHO","MOTION","WARMTH"};
            static const uint8_t ids[4]={G_RSIZE,G_DMIX,G_CDEPTH,G_DJF};
            str_cpy(label,labels[knob],16);v=amb.global[ids[knob]];if(knob==3)v=-v*100/64;else if(GP[ids[knob]].fmt==F_PCT)v=v*100/127;
        } else {
            if(ambr.view==0) l=&amb.layer[ambr.target-1];
            str_cpy(label,AMB_MACRO[l->role][knob],16);v=l->macro[knob]*100/127;
        }
    } else if(ambr.view==1) {
        if(ambr.page==1) id=amb_sound_id(l,ambr.bank,knob);
        else if(ambr.page==2) id=P_ATK+knob;
        else if(ambr.page==3) {
            static const char *const dest[4]={"FILTER","PITCH","PAN","VOLUME"};
            if(knob==0)id=P_LWAVE;else if(knob==1)id=P_LRATE;
            else {str_cpy(label,knob==2?"DEPTH":"DEST",16);if(knob==2)v=l->lfo_depth;else {str_cpy(value,dest[l->lfo_dest],24);return;}}
        }
        else if(ambr.page==4) {
            static const char *const drums[4]={"DENSITY","SWING","FILL","MUTATE"};
            static const char *const pitched[4]={"MODE","SPEED","DENSITY","FOLLOW"};
            static const char *const modes[5]={"UP","DOWN","WALK","CHORD","SPARSE"};
            static const char *const texture[5]={"STEADY","PULSE","DRIFT","BURSTS","SPARSE"};
            static const char *const speed[4]={"1/16","1/8","1/4","1/2"};
            str_cpy(label,l->role==4?drums[knob]:pitched[knob],16);
            if(l->role==4) v=knob==0?l->density:knob==1?l->swing:knob==2?l->fill:l->mutation;
            else if(knob==0) {str_cpy(value,l->role==5 && !amb_legacy_texture(l)?texture[l->mode]:modes[l->mode],24);return;}
            else if(knob==1) {str_cpy(value,speed[l->speed],24);return;}
            else v=knob==2?l->density:l->follow;
        } else if(ambr.page==6){static const char *const labels[4]={"LOW","MID","HIGH","OUTPUT"};str_cpy(label,labels[knob],16);v=amb.layer_eq[ambr.edit_layer][knob];}
        else {
            static const uint8_t ids[5][4]={{P_DTYPE,P_DIST,P_DTONE,P_FCUT},{P_CHOR,P_APRATE,P_APDEP,P_APWAVE},
                {P_DLY,P_FCUT,P_PAN,P_DIST},{P_REV,P_FCUT,P_PAN,P_CHOR},{P_PAN,P_APDEP,P_APRATE,P_APWAVE}};
            id=ids[ambr.fx%5][knob];
        }
        if(id) {d=param_desc_of(l->engine,id);v=l->base[id];}
        if(l->role==6 && ambr.page==1 && ambr.bank==0 && knob>=2) {str_cpy(label,knob==2?"GAIN":"FADE",16);d=0;v=l->base[id];}
    } else if(ambr.view==2) {
        if(ambr.page==0) {static const char *const labels[4]={"HARMONY","TIMBRE","RHYTHM","FX"};str_cpy(label,labels[knob],16);v=amb.evolve[knob];}
        else if(ambr.page==1) {
            static const char *const labels[4]={"KEY","SCALE","TEMPO","TAIL TIME"};
            static const char *const scales[4]={"MINOR","MAJOR","DORIAN","LYDIAN"};str_cpy(label,labels[knob],16);
            if(knob==0) {str_cpy(value,N_NOTE[amb.key],24);return;}
            if(knob==1) {str_cpy(value,scales[amb.scale],24);return;}
            if(knob==3){fmt_int(value,amb.release_ms);str_cpy(value+str_len(value),"ms",24-str_len(value));return;}
            v=amb.global[G_BPM];
        } else if(ambr.page==2) {
            static const char *const labels[4]={"RHYTHM","PHRASE","INTERPLAY","VARIATION"};str_cpy(label,labels[knob],16);
            v=knob==3?amb.policy.variation:ambr.gen_edit[knob]?ambr.gen_candidate[knob]+1:amb.policy.pending[knob]!=255?amb.policy.pending[knob]+1:amb.policy.active[knob]+1;
        } else if(ambr.page==3) {
            static const uint8_t ids[4][4]={{G_RSIZE,G_RFILT,G_RDAMP,G_RTYPE},{G_DMIX,G_DTIME,G_DFDBK,G_DTYPE},
                {G_CDEPTH,G_CRATE,G_WIDTH,G_RMOD},{G_DJF,G_CLIP,G_DCOLOR,G_DMOD}};
            id=ids[ambr.fx%4][knob];d=&GP[id];v=amb.global[id];
        } else {static const char *const labels[4]={"LOW","MID","HIGH","OUTPUT"};str_cpy(label,labels[knob],16);v=amb.eq[knob];}
    }
    if(d) {str_cpy(label,d->label,16);param_format(d,v,value,&unit);if(unit && str_len(value)+str_len(unit)<10)str_cpy(value+str_len(value),unit,24-str_len(value));}
    else {if(v<0) {value[0]='-';fmt_int(value+1,-v);}else fmt_int(value,v);}
}
static void amb_chord_label(unsigned degree,char *text)
{
    int pcs[4],n=amb_chord_tones(amb_pool(),amb.key,(int)degree,pcs);
    int third=(pcs[1]-pcs[0]+12)%12,fifth=(pcs[2]-pcs[0]+12)%12,seventh=n>3?(pcs[3]-pcs[0]+12)%12:0;
    str_cpy(text,N_NOTE[pcs[0]],16);
    str_cpy(text+str_len(text),fifth==6?(n>3?"m7b5":"dim"):third==3?(n>3?"m7":"m"):n>3?(seventh==11?"maj7":"7"):"",16-str_len(text));
}
static uint32_t amb_lfo_phase(unsigned k){return trk[k].lfo_ph;}
static void amb_fx_diagram(unsigned k,unsigned module)
{
    track_t *t=&trk[k];int send=0,x;
    if(module==2 || module==3){
        send=t->p[module==2?P_DLY:P_REV]*100/127;
        amb_text(12,84,"DRY",ADIM,8);cv_line(42,88,218,88,AINK);amb_text(188,73,"OUT",ADIM,6);
        cv_line(55,88,55,109,ADIM);cv_line(55,109,218,109,ADIM);cv_line(218,109,218,88,ADIM);
        cv_rect(80,103,80,13,ASEL);amb_text(85,105,module==2?"DELAY BUS":"REVERB BUS",AINK,12);
        amb_text(12,129,"SEND",ADIM,6);amb_number(44,129,send,AINK);amb_text(64,129,"% / SHARED TAIL",ADIM,24);
        cv_rect(92,120,116,3,ALINE);cv_rect(92,120,116*send/100,3,AINK);
    }else if(module==0){
        int last=118;amb_text(12,79,"DRIVE RESPONSE / MODEL",ADIM,32);
        for(x=0;x<100;x++){int v=(x-50)*300;int y=clamp(119-softclip(v*(128+t->p[P_DIST]*2)/128)/1100,95,141);
            if(x)cv_line(12+(x-1)*2,last,12+x*2,y,AINK);last=y;}
    }else if(module==1){
        amb_text(12,85,"CHORUS AMOUNT",ADIM,22);cv_rect(12,105,204,5,ALINE);cv_rect(12,105,204*t->p[P_CHOR]/127,5,AINK);
        amb_text(12,128,"MOTION USES SHARED CHORUS",ADIM,32);
    }else{
        amb_text(12,85,"STEREO POSITION",ADIM,26);cv_line(22,113,214,113,ADIM);
        x=22+(t->p[P_PAN]+64)*192/127;cv_rect(x-2,107,5,13,AINK);
        amb_text(12,129,"L",ADIM,1);amb_text(208,129,"R",ADIM,1);
    }
}
/* Interaction guidance has one home: three aligned lines above knob values.
 * Navigation and actions remain visible while a notice or pickup is shown. */
static void amb_guidance(const amb_layer_t *l,const char **first,const char **second)
{
    const char *a="SCL PAGE / KNOBS EDIT",*b="HOME RETURN";
    if(ambr.view==0){a="SELECT TARGET / EDIT OPEN";b="KEYS NOTE POOL / ALGO CHORD";}
    else if(ambr.view==3){a="SELECT SLOT / REC SAVE";b=ambr.overwrite?"REC AGAIN OVERWRITE / HOME CANCEL":"HOME CANCEL";}
    else if(ambr.view==1){
        if(ambr.page==0){a="SCL PAGE / ALGO LAYER TYPE";b="EDIT RESTORE LOADED TRACK";}
        else if(ambr.page==1){
            if(l->role==6){a=ambr.bank==0?"SCL PAGE / SELECT SOURCE":"SCL PAGE / KNOBS EDIT";b="EDIT NEXT SAMPLE PAGE";}
            else {a=ambr.bank?"SCL PAGE / SELECT DETAIL GROUP":"SCL PAGE / SELECT PATCH";b=ambr.bank?"EDIT PATCH / KNOBS EDIT":"EDIT DETAILS / KNOBS EDIT";}
        }else if(ambr.page==4){
            a=l->role==4?"SCL PAGE / SELECT DRUM LANE":!l->generated && l->notes[ambr.cursor]?"SELECT STEP / ARP CLEAR / SCL PAGE":"SCL PAGE / SELECT EDIT STEP";
            b=ambr.recording?"REC STOP / KEYS RECORD":l->generated?"EDIT OR SEQ MANUAL / REC CAPTURE":"EDIT OR SEQ AUTO / REC OVERDUB";
        }else if(ambr.page==5)a="SCL PAGE / SELECT FX MODULE";
    }else if(ambr.view==2){
        if(ambr.page==0){a="SCL PAGE / SELECT AUTO BARS";b="ALGO CHORD / KNOBS EVOLVE";}
        else if(ambr.page==1)b="OFF-SCALE KEYS REMAIN PLAYABLE";
        else if(ambr.page==2){a="SCL PAGE / KNOBS 1-3 STYLES";b="KNOB 4 VARIATION";}
        else if(ambr.page==3)a="SCL PAGE / SELECT FX MODULE";
    }
    if(ambr.reset_pending){a="REC RESTORE TRACK";b="HOME CANCEL";}
    else if(ambr.piece_preview!=255){a="SELECT PRESET / REC RECALL";b="HOME CANCEL";}
    else if(ambr.proposal!=255 && (ambr.view==0 || (ambr.page==0 && ambr.view<3))){
        a=ambr.view==1?"ALGO LAYER TYPE / REC COMMIT":"ALGO CHORD / REC COMMIT";b="HOME CANCEL";
    }
    *first=a;*second=b;
}
static void amb_draw(void)
{
    unsigned i,k;char b[32];amb_layer_t *l=&amb.layer[ambr.edit_layer];
    if(!ambr.dirty && fm1_ms-ambr.last_draw<((ambr.view==1 && ambr.page==4)?50u:(ambr.view==1 && ambr.page==3)?80u:250u))return;
    ambr.last_draw=fm1_ms;ambr.dirty=0;
    cv_begin(240,147,ABG);
    amb_text(8,8,"AMB+",AINK,8);
    amb_text(60,8,ambr.view==0?"HOME":ambr.view==1?"EDIT":ambr.view==2?"GLOBAL":"SAVE",ADIM,8);
    amb_number(146,8,ambr.view==0?ambr.target+1:ambr.view==3?ambr.slot+1:ambr.page+1,AINK);
    amb_text(155,8,ambr.view==0?"/5":ambr.view==1?"/7":ambr.view==2?"/5":"/8",AINK,3);
    amb_text(182,8,song.playing?">":"-",ADIM,1);amb_number(211,8,amb.global[G_BPM],AINK);
    cv_rect(8,25,224,1,ALINE);
    if(ambr.view==0) {
        {unsigned target=amb_level_feedback(),value=target?amb.layer[target-1].level:amb.master,max=target?127:1023;
            char label[16]="MASTER";
            if(target){label[0]='L';label[1]=(char)('0'+target);label[2]=' ';str_cpy(label+3,AMB_ROLE[amb.layer[target-1].role],sizeof label-3);}
            cv_rect(39,34,191,27,(target || ambr.target==0)?ASEL:ABG);
            amb_text(46,39,label,AINK,12);amb_number(193,39,value*100/max,ADIM);
            cv_rect(46,54,176,2,ALINE);cv_rect(46,54,176*value/max,2,AINK);
        }
        for(k=0;k<4;k++) {
            const amb_layer_t *p=&amb.layer[k];int x=39+(int)(k%2)*97,y=65+(int)(k/2)*33;
            cv_rect(x,y,94,30,ambr.target==k+1?ASEL:ABG);
            amb_number(x+4,y+3,k+1,AINK);amb_text(x+15,y+3,AMB_ROLE[p->role],AINK,11);
            amb_text(x+4,y+15,p->role==6?N_AS_SOURCE[p->base[P_E0]&1]:amb_patch_name(p),ADIM,14);
            if(p->base[P_MUTE] || (ambr.solo>=0 && ambr.solo!=(int)k)) amb_text(x+79,y+3,"M",AQUEUE,1);
            cv_rect(9+k*6,120-p->level/2,3,p->level/2,ambr.target==k+1?AINK:ADIM);
        }
        amb_text(8,134,"CHORD",ADIM,6);amb_chord_label(amb.chord,b);amb_text(49,134,b,AINK,10);
        if(ambr.chord_q!=255) amb_text(138,134,"CHORD QUEUED",AQUEUE,14);
    } else {
        if(ambr.view==1) {amb_text(9,35,"L",ADIM,1);amb_number(16,35,ambr.edit_layer+1,ADIM);amb_text(35,35,AMB_ROLE[l->role],ADIM,8);amb_text(122,35,AMB_EDIT[ambr.page],AINK,16);}
        else amb_text(9,35,ambr.view==2?AMB_GLO[ambr.page]:"USER SAVE DESTINATION",AINK,32);
        cv_rect(8,48,224,1,ALINE);
        if(ambr.view==1 && ambr.page==0) {
            amb_text(12,58,ambr.handoff[ambr.edit_layer]?"NEXT":"CURRENT",ADIM,12);amb_text(130,58,"PROPOSED",ADIM,12);
            amb_text(12,78,AMB_ROLE[l->role],AINK,12);amb_text(130,78,ambr.proposal==255?"-":AMB_ROLE[ambr.proposal],AQUEUE,12);
        } else if(ambr.view==1 && ambr.page==1) {
            if(l->role!=6 && ambr.bank){
                static const char *const group[3]={"ENGINE","FILTER","PLAY"};
                amb_text(12,54,ENGINES[l->engine]->name,ADIM,12);amb_text(125,54,group[ambr.detail],AINK,12);
                for(i=0;i<4;i++){
                    unsigned id=amb_sound_id(l,1,i);const param_desc_t *d=param_desc_of(l->engine,id);char val[24];const char *unit=0;
                    int width=(l->base[id]-d->min)*78/(d->max-d->min?d->max-d->min:1);param_format(d,l->base[id],val,&unit);
                    amb_text(12,73+i*18,d->label,ADIM,10);amb_text(76,73+i*18,val,AINK,10);
                    cv_rect(150,76+i*18,78,4,ALINE);cv_rect(150,76+i*18,width,4,AINK);
                }
            }else{
                amb_text(12,60,l->role==6?(ambr.bank==0?"SOURCE":ambr.bank==1?"PLAYBACK":"CHARACTER"):"PATCH",ADIM,12);
                amb_text(12,80,l->role==6?N_AS_SOURCE[l->base[P_E0]&1]:amb_patch_name(l),AINK,32);
                amb_text(12,102,ENGINES[l->engine]->name,ADIM,12);amb_number(188,102,l->patch+1,AINK);
            }
        } else if(ambr.view==1 && ambr.page==2) {
            int ax=16+amb.layer[ambr.edit_layer].base[P_ATK]/3,rx=229-l->base[P_REL]/3;
            cv_line(12,128,ax,65,AINK);cv_line(ax,65,99,88,AINK);cv_line(99,88,rx,88,AINK);cv_line(rx,88,230,130,AINK);
        } else if(ambr.view==1 && ambr.page==3) {
            track_t *t=&trk[ambr.edit_layer];uint32_t ph=amb_lfo_phase(ambr.edit_layer);int last=0;
            cv_line(14,96,224,96,ALINE);
            for(i=0;i<210;i++){int y=96-lfo_wave(t,i*20452225u)*28/32768;
                if(i)cv_line(13+i,last,14+i,y,ADIM);last=y;}
            {int x=14+(int)((ph>>16)*209u/65536u),y=96-lfo_wave(t,ph)*28/32768;
                cv_line(x,62,x,126,ALINE);cv_rect(x-2,y-2,5,5,AINK);}
            amb_text(12,133,"LIVE LFO PHASE / ONE CYCLE",ADIM,34);
        } else if(ambr.view==1 && ambr.page==4) {
            amb_text(12,60,l->generated?"AUTO / GENERATED":"MANUAL / EDITED",AINK,32);
            if(l->role==4) {static const char *const lanes[4]={"KICK","SNARE","HATS","PERC"};amb_text(12,78,lanes[ambr.lane],ADIM,12);}
            if(l->role!=4){amb_text(12,78,"PLAY",ADIM,5);if(song.playing && ambr.playhead_valid[ambr.edit_layer])amb_number(43,78,ambr.playhead[ambr.edit_layer]+1,AINK);else amb_text(43,78,"-",ADIM,1);amb_text(99,78,"EDIT",ADIM,5);amb_number(130,78,ambr.cursor+1,AINK);}
            for(i=0;i<16;i++) {
                int on=l->role==4?(l->drums[ambr.lane]>>i)&1u:(l->generated?ambr.recent[ambr.edit_layer][i]:l->notes[i])!=0;
                cv_rect(12+i*14,100,10,22,on?AINK:ALINE);
                if(l->role!=4 && i==ambr.cursor)cv_rect(12+i*14,133,10,1,ADIM);
                if(song.playing && ambr.playhead_valid[ambr.edit_layer] && i==ambr.playhead[ambr.edit_layer])cv_rect(12+i*14,127,10,2,AQUEUE);
            }
        } else if(ambr.view==1 && ambr.page==6) {
            int8_t *eq=amb.layer_eq[ambr.edit_layer];
            cv_line(15,100,65,100-eq[0]*2,AINK);cv_line(65,100-eq[0]*2,120,100-eq[1]*2,AINK);
            cv_line(120,100-eq[1]*2,175,100-eq[2]*2,AINK);cv_line(175,100-eq[2]*2,225,100,AINK);
            amb_text(12,130,"LAYER EQ / BEFORE FX SENDS / DB",ADIM,35);
        } else if(ambr.view==1 && ambr.page==5) {
            static const char *const modules[5]={"DRIVE","CHORUS / PAN","DELAY SEND","REVERB SEND","SPATIAL"};
            amb_text(12,60,modules[ambr.fx%5],AINK,32);amb_fx_diagram(ambr.edit_layer,ambr.fx%5);
        } else if(ambr.view==2 && ambr.page==0) {
            amb_text(12,60,"CURRENT CHORD",ADIM,20);amb_chord_label(amb.chord,b);amb_text(12,78,b,AINK,16);
            amb_text(130,60,"PROPOSED",ADIM,12);
            if(ambr.proposal!=255) {amb_chord_label(ambr.proposal,b);amb_text(130,78,b,AQUEUE,16);amb_text(130,94,AMB_MOOD[ambr.proposal],ADIM,14);}
            amb_text(12,115,"AUTO BARS",ADIM,16);amb_number(84,115,AMB_AUTO[amb.auto_index],AINK);
            amb_text(12,132,N_NOTE[amb.key],AINK,3);amb_text(40,132,amb.scale==0?"MINOR":amb.scale==1?"MAJOR":amb.scale==2?"DORIAN":"LYDIAN",ADIM,10);
        } else if(ambr.view==2 && ambr.page==2) {
            for(k=0;k<3;k++) {
                unsigned style=ambr.gen_edit[k]?ambr.gen_candidate[k]:amb.policy.pending[k]!=255?amb.policy.pending[k]:amb.policy.active[k];
                unsigned pending=ambr.gen_edit[k] || amb.policy.pending[k]!=255;
                amb_text(10,54+k*30,k==0?"RHYTHM":k==1?"PHRASE":"INTERPLAY",ADIM,10);
                amb_text(78,54+k*30,k==0?AMB_RHYTHMS[style]:k==1?AMB_PHRASES[style]:AMB_INTERPLAY[style],pending?AQUEUE:AINK,24);
                if(pending) amb_text(230,54+k*30,"Q",AQUEUE,1);
                {
                    amb_policy_t preview=amb.policy;
                    amb_participation_t parts[4]={{1,40},{1,35},{1,48},{1,25}};
                    int last=72+(int)k*30;
                    preview.active[k]=(uint8_t)style;
                    for(i=0;i<27;i++) {
                        if(k==0) {
                            int on=amb_rhythm_event(&preview,i,0,40);
                            cv_rect(11+i*8,68,5,on?4:1,on?ADIM:ALINE);
                        } else if(k==1) {
                            int y=102-(amb_phrase_event(&preview,i*16,0)?5:0);
                            if(i)cv_line(3+i*8,last,11+i*8,y,ADIM);last=y;
                        } else {
                            unsigned m=amb_policy_events(&preview,i,parts),j;
                            for(j=0;j<3;j++)cv_rect(11+i*8,127+j*4,5,2,(m>>j)&1u?ADIM:ALINE);
                        }
                    }
                }
            }
        } else if(ambr.view==2 && ambr.page==3) {
            static const char *const modules[4]={"REVERB","DELAY / TAPE","MOTION","WARMTH"};
            amb_text(12,64,modules[ambr.fx%4],AINK,30);
            amb_text(12,88,ambr.fx%4==0?N_RTYPE[fx.rtype<5?fx.rtype:0]:ambr.fx%4==1?N_DLYTYPE[amb.global[G_DTYPE]]:"SHARED MASTER PROCESSING",ADIM,34);
            amb_text(12,121,(ambr.fx%4==0 && fx.rtype!=amb.global[G_RTYPE])?"ECO: LIGHTER EFFECT ACTIVE":"SAME STATE AS HOME MASTER",ADIM,34);
        } else if(ambr.view==2 && ambr.page==1) {
            amb_text(12,64,"TONAL CENTRE",ADIM,24);amb_text(12,84,N_NOTE[amb.key],AINK,4);
        } else if(ambr.view==2 && ambr.page==4) {
            cv_line(15,100,65,100-amb.eq[0]*2,AINK);cv_line(65,100-amb.eq[0]*2,120,100-amb.eq[1]*2,AINK);
            cv_line(120,100-amb.eq[1]*2,175,100-amb.eq[2]*2,AINK);cv_line(175,100-amb.eq[2]*2,225,100,AINK);
            amb_text(12,130,"3 BANDS / OUTPUT TRIM (DB)",ADIM,34);
        } else if(ambr.view==3) {
            amb_text(12,62,"SLOT",ADIM,6);amb_number(49,62,ambr.slot+1,AINK);
            amb_text(12,83,amb_occupied(ambr.slot)?"OCCUPIED":"EMPTY",AQUEUE,20);
            amb_text(12,105,"DATE UNAVAILABLE",ADIM,25);
            amb_text(12,125,ambr.overwrite?"OVERWRITE ARMED":"DESTINATION READY",ambr.overwrite?AQUEUE:ADIM,32);
        }
    }
    if(ambr.reset_pending){
        GFX_HOOK_COVER(8,50,232,145);cv_rect(8,50,224,95,ASEL);
        amb_text(16,61,"RESTORE THIS TRACK?",AQUEUE,30);
        amb_text(16,81,"LAST LOADED PIECE VERSION",ADIM,31);
        amb_text(16,101,"SOUND / MACROS / PATTERN",ADIM,31);
    }else if(ambr.piece_preview!=255) {
        GFX_HOOK_COVER(8,60,232,130);cv_rect(8,60,224,70,ASEL);amb_text(16,67,"PRESET PROPOSAL",ADIM,30);
        amb_text(16,88,ambr.piece_preview<6?AMB_SCENE[ambr.piece_preview]:"USER SLOT",AINK,30);
        if(ambr.piece_preview>=6) amb_number(89,88,ambr.piece_preview-5,AINK);
    } else if(ambr.view==0 && ambr.proposal!=255) {
        GFX_HOOK_COVER(39,65,230,128);cv_rect(39,65,191,63,ASEL);amb_text(46,72,"NEXT HARMONY",ADIM,24);
        amb_chord_label(ambr.proposal,b);amb_text(46,88,b,AINK,16);amb_text(115,88,AMB_MOOD[ambr.proposal],ADIM,16);
    }
    cv_blit_cells(0,0);
    cv_begin(240,93,ABG);
    cv_rect(8,0,224,1,ALINE);
    {const char *first,*second,*status="";char notice[38];unsigned j;
        amb_guidance(l,&first,&second);
        if(amb_tail_any()){status="OLD SOUND RELEASING / NEXT QUEUED";}
        else if(ambr.notice[0] && fm1_ms-ambr.notice_ms<2500u){
            str_cpy(notice,ambr.notice,sizeof notice);
            for(j=0;notice[j];j++)if(notice[j]>='a' && notice[j]<='z')notice[j]-='a'-'A';
            status=notice;
        }else if(!ambr.pickup.picked)status="MASTER: PICKUP SAVED LEVEL";
        else if(ambr.view==0)status="LIT CHORD / GLOW NOTE POOL";
        else if(ambr.view==1 && ambr.page==1 && l->role==6)status="BUNDLED PCM / MIC UNAVAILABLE";
        amb_text(10,4,first,ADIM,36);amb_text(10,14,second,ADIM,36);amb_text(10,24,status,ADIM,36);
    }
    cv_rect(8,34,224,1,ALINE);
    if(ambr.view==3) {
        for(i=0;i<8;i++) {cv_rect(11+i*28,50,22,24,i==ambr.slot?ASEL:ALINE);amb_number(18+i*28,58,i+1,AINK);}
    } else for(k=0;k<4;k++) {
        char label[16]={0},value[24]={0};amb_knob_value(k,label,value);
        amb_text(5+k*60,43,label,ADIM,9);amb_text(5+k*60,63,value,AINK,9);
        cv_rect(12+k*60,85,36,2,ALINE);
    }
    cv_blit_cells(0,147);
    (void)b;
}
