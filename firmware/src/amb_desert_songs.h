/* SPDX-License-Identifier: GPL-3.0-only */
typedef struct {
    uint8_t role, patch, mode, speed, density, level;
    uint8_t macro, lfo_dest, lfo_depth;
} amb_desert_voice_t;
typedef struct { uint8_t param; int8_t value; } amb_desert_param_t;
enum { AMB_DESERT_END = 255 };
typedef struct {
    const amb_desert_voice_t *voices;
    const amb_desert_param_t *global, *voice_params;
    uint8_t styles[3], key, scale, variation, evolve[2], pool[2];
} amb_desert_recipe_t;

static void amb_finish_desert(amb_piece_t *p,const amb_desert_recipe_t *recipe)
{
    memcpy(p->policy.active,recipe->styles,3);
    p->policy.variation=recipe->variation;
    p->key=recipe->key;p->scale=recipe->scale;p->auto_index=3;
    p->master=820;
    p->evolve[1]=recipe->evolve[0];p->evolve[2]=recipe->evolve[1];
    for(const amb_desert_param_t *v=recipe->global;v->param!=AMB_DESERT_END;v++)
        p->global[v->param]=v->value;
    const amb_desert_param_t *params=recipe->voice_params;
    for(unsigned k=0;k<AMB_LAYERS;k++){
        const amb_desert_voice_t *v=&recipe->voices[k];
        amb_layer_t *l=&p->layer[k];
        amb_fill_patch(l,p->key,k,v->role,v->patch);
        l->mode=v->mode;l->speed=v->speed;l->density=v->density;l->level=v->level;
        l->base[P_CHOR]=0;
        for(;params->param!=AMB_DESERT_END;params++)
            l->base[params->param]=params->value;
        params++;
        if(v->macro!=255)l->macro[2]=v->macro;
        if(v->lfo_dest!=255){l->lfo_dest=v->lfo_dest;l->lfo_depth=v->lfo_depth;}
    }
    p->layer[0].reserved=recipe->pool[0];p->layer[1].reserved=recipe->pool[1];
    amb_anchor_master(p);
}

/* Tales of Sand Dunes. Relative Phrygian pool. */
static const amb_desert_voice_t sand_dunes_voices[]={
    {1,0,2,1,86,71,32,3,39},
    {0,22,3,2,72,111,8,3,32},
    {4,8,0,0,21,127,255,255,0},
    {5,11,2,1,62,88,255,3,70},
};
static const amb_desert_param_t sand_dunes_global[]={
    {G_BPM,56},{G_RTYPE,0},{G_RSIZE,95},{G_RDAMP,83},{G_DMIX,20},
    {G_DTYPE,3},{G_DTIME,13},{G_DFDBK,52},{G_WIDTH,108},{G_RPRE,66},
    {G_RWIDE,104},{G_RFILT,-12},{G_CLIP,0},{G_CDEPTH,0},{AMB_DESERT_END,0},
};
static const amb_desert_param_t sand_dunes_voice_params[]={
    /* Low bed */
    {P_TRANS,-24},{P_PAN,0},{P_ATK,94},{P_E0,2},{P_E1,0},
    {P_E2,43},{P_E7,12},{P_E4,57},{P_E5,11},{P_E6,31},
    {P_REV,12},{P_DLY,0},{AMB_DESERT_END,0},
    /* Flute */
    {P_TRANS,0},{P_ATK,54},{P_DEC,102},{P_SUS,114},{P_REL,113},
    {P_FTYPE,0},{P_FCUT,90},{P_FRES,10},{P_E0,6},{P_E1,0},
    {P_E2,0},{P_E3,1},{P_E4,0},{P_E5,94},{P_E6,52},
    {P_E7,87},{P_LN7,119},{P_LRATE,17},{P_PAN,-13},{P_REV,54},
    {P_DLY,23},{AMB_DESERT_END,0},
    /* Percussion */
    {P_PAN,1},{P_E1,32},{P_E2,36},{P_E3,110},{P_E4,53},
    {P_E5,56},{P_E6,94},{P_E7,79},{P_LN0+DV_KICK,127},{P_LN0+DV_TOM,120},
    {P_LN0+DV_SNARE,31},{P_LN0+DV_HATC,28},{P_FCUT,90},{P_REV,83},{P_DLY,0},
    {AMB_DESERT_END,0},
    /* Wind */
    {P_PAN,9},{P_ATK,106},{P_LRATE,18},{P_REV,24},{P_DLY,16},
    {P_FTYPE,2},{P_FCUT,65},{P_FRES,20},{P_E1,76},{P_E2,65},
    {P_E3,66},{P_APDEP,48},{P_APRATE,16},{AMB_DESERT_END,0},
};
static const amb_desert_recipe_t sand_dunes_recipe={
    sand_dunes_voices,sand_dunes_global,sand_dunes_voice_params,
    {4,0,0},2,0,34,{43,11},{0xab,0x05}
};

/* Tales of Cold Desert Dunes. E Dorian with a raised fourth. */
static const amb_desert_voice_t cold_desert_voices[]={
    {0,18,2,2,67,78,19,5,21},
    {1,12,2,2,61,85,35,3,36},
    {2,13,2,3,9,106,255,255,0},
    {5,17,2,1,50,115,255,3,61},
};
static const amb_desert_param_t cold_desert_global[]={
    {G_BPM,67},{G_RTYPE,0},{G_RSIZE,106},{G_RDAMP,73},{G_DMIX,37},
    {G_DTYPE,1},{G_DPING,1},{G_DTIME,18},{G_DFDBK,52},{G_WIDTH,106},
    {G_RPRE,80},{G_RWIDE,113},{G_RFILT,-8},{G_CLIP,0},{G_CDEPTH,0},
    {AMB_DESERT_END,0},
};
static const amb_desert_param_t cold_desert_voice_params[]={
    /* Lead */
    {P_TRANS,12},{P_ATK,73},{P_DEC,95},{P_SUS,95},{P_REL,103},
    {P_FTYPE,0},{P_FCUT,89},{P_E0,4},{P_E1,12},{P_E2,-41},
    {P_E3,110},{P_E4,0},{P_E5,15},{P_E6,0},{P_E7,6},
    {P_LRATE,18},{P_PAN,-8},{P_REV,48},{P_DLY,25},{AMB_DESERT_END,0},
    /* Low horizon */
    {P_TRANS,-24},{P_ATK,107},{P_REL,119},{P_E0,0},{P_E1,5},
    {P_E2,-16},{P_E5,18},{P_E6,41},{P_E7,2},{P_FCUT,58},
    {P_PAN,0},{P_REV,22},{P_DLY,0},{P_LRATE,10},{AMB_DESERT_END,0},
    /* Bells */
    {P_TRANS,-12},{P_ATK,35},{P_DEC,102},{P_SUS,24},{P_REL,99},
    {P_E0,4},{P_E1,0},{P_E2,43},{P_E3,82},{P_E4,0},
    {P_E5,7},{P_E6,0},{P_E7,0},{P_FCUT,86},{P_PAN,27},
    {P_REV,65},{P_DLY,64},{AMB_DESERT_END,0},
    /* Wind */
    {P_TRANS,0},{P_ATK,108},{P_REL,110},{P_FTYPE,2},{P_FCUT,79},
    {P_FRES,19},{P_E2,2},{P_E6,44},{P_E7,73},{P_REV,21},
    {P_DLY,13},{P_APDEP,93},{P_APRATE,17},{P_PAN,4},{AMB_DESERT_END,0},
};
static const amb_desert_recipe_t cold_desert_recipe={
    cold_desert_voices,cold_desert_global,cold_desert_voice_params,
    {5,2,3},4,2,38,{29,21},{0xcd,0x06}
};
