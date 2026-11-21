/* SPDX-License-Identifier: GPL-3.0-only */
/* Tales of Sand Dunes: an acoustic silhouette with rough wind and isolated impacts. */
static void amb_finish_long_lee(amb_piece_t *p)
{
    static const struct { uint8_t role, patch, mode, speed, density, level; } voices[]={
        {1,0,2,1,86,71}, {0,22,3,2,72,111}, {4,8,0,0,21,127}, {5,11,2,1,62,88}
    };
    static const uint8_t styles[]={4,0,0};
    static const struct { uint8_t param; int8_t value; } global[]={
        {G_BPM,56},{G_RTYPE,0},{G_RSIZE,95},{G_RDAMP,83},{G_DMIX,20},
        {G_DTYPE,3},{G_DTIME,13},{G_DFDBK,52},{G_WIDTH,108},{G_RPRE,66},
        {G_RWIDE,104},{G_RFILT,-12},{G_CLIP,0},{G_CDEPTH,0}
    };
    static const struct { uint8_t layer, param; int8_t value; } voice_params[]={
        {0,P_TRANS,-24},{0,P_PAN,0},{0,P_ATK,94},{0,P_E0,2},{0,P_E1,0},{0,P_E2,43},
        {0,P_E7,12},{0,P_E4,57},{0,P_E5,11},{0,P_E6,31},{0,P_REV,12},{0,P_DLY,0},
        {1,P_TRANS,0},{1,P_ATK,54},{1,P_DEC,102},{1,P_SUS,114},{1,P_REL,113},
        {1,P_FTYPE,0},{1,P_FCUT,90},{1,P_FRES,10},{1,P_E0,6},{1,P_E1,0},
        {1,P_E2,0},{1,P_E3,1},{1,P_E4,0},{1,P_E5,94},{1,P_E6,52},
        {1,P_E7,87},{1,P_LN7,119},{1,P_LRATE,17},{1,P_PAN,-13},{1,P_REV,54},
        {1,P_DLY,23},
        {2,P_PAN,1},{2,P_E1,32},{2,P_E2,36},{2,P_E3,110},{2,P_E4,53},{2,P_E5,56},
        {2,P_E6,94},{2,P_E7,79},{2,P_LN0+DV_KICK,127},{2,P_LN0+DV_TOM,120},
        {2,P_LN0+DV_SNARE,31},{2,P_LN0+DV_HATC,28},{2,P_FCUT,90},{2,P_REV,83},{2,P_DLY,0},
        {3,P_PAN,9},{3,P_ATK,106},{3,P_LRATE,18},{3,P_REV,24},{3,P_DLY,16},
        {3,P_FTYPE,2},{3,P_FCUT,65},{3,P_FRES,20},{3,P_E1,76},{3,P_E2,65},{3,P_E3,66},
        {3,P_APDEP,48},{3,P_APRATE,16}
    };
    memcpy(p->policy.active,styles,sizeof styles);
    p->policy.variation=34;
    p->key=2;p->scale=0;p->auto_index=3;
    p->master=820;
    p->evolve[1]=43;p->evolve[2]=11;
    for(unsigned i=0;i<NELEM(global);i++)
        p->global[global[i].param]=global[i].value;
    for(unsigned k=0;k<AMB_LAYERS;k++){
        amb_layer_t *l=&p->layer[k];
        amb_fill_patch(l,p->key,k,voices[k].role,voices[k].patch);
        l->mode=voices[k].mode;l->speed=voices[k].speed;
        l->density=voices[k].density;l->level=voices[k].level;
        l->base[P_CHOR]=0;
    }
    for(unsigned i=0;i<NELEM(voice_params);i++)
        p->layer[voice_params[i].layer].base[voice_params[i].param]=voice_params[i].value;
    /* Relative Phrygian pool with the semitone above the tonic. */
    p->layer[0].reserved=0xab;p->layer[1].reserved=0x05;
    p->layer[0].macro[2]=32;p->layer[0].lfo_dest=3;p->layer[0].lfo_depth=39;
    p->layer[1].macro[2]=8;p->layer[1].lfo_dest=3;p->layer[1].lfo_depth=32;
    p->layer[3].lfo_dest=3;p->layer[3].lfo_depth=70;
    amb_anchor_master(p);
}
