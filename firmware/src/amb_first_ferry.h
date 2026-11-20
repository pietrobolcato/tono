/* SPDX-License-Identifier: GPL-3.0-only */
static void amb_first_ferry(amb_piece_t *p)
{
    /* A flute carries the crossing; tine reflections answer over cello and water. */
    static const struct { uint8_t role, patch, mode, speed, density, level; } voices[]={
        {0,22,AMB_CHANT,1,58,92}, /* Concert Flute */
        {2,12,2,1,45,70},         /* Willow Tine */
        {1,13,2,1,72,62},         /* Bowed Cello */
        {5,12,0,2,92,16}          /* Ocean Beads */
    };
    static const uint8_t styles[]={2,4,1}; /* Odd Loops, Call Answer, Stagger */
    p->key=5;p->scale=1;p->auto_index=4;
    p->policy.seed=0x414d4211u;p->policy.variation=28;
    memcpy(p->policy.active,styles,sizeof styles);
    p->evolve[1]=22;p->evolve[2]=9;p->evolve[3]=12;
    p->global[G_BPM]=61;p->global[G_RTYPE]=5;p->global[G_RSIZE]=84;
    p->global[G_RDAMP]=72;p->global[G_DMIX]=15;p->global[G_DFDBK]=33;
    p->global[G_CLIP]=0;p->global[G_CDEPTH]=15;
    for(unsigned k=0;k<AMB_LAYERS;k++){
        amb_layer_t *l=&p->layer[k];
        amb_fill_patch(l,p->key,k,voices[k].role,voices[k].patch);
        l->mode=voices[k].mode;l->speed=voices[k].speed;
        l->density=voices[k].density;l->level=voices[k].level;
    }
    p->layer[0].base[P_PAN]=-12;
    p->layer[0].base[P_REV]=53;p->layer[0].base[P_DLY]=10;
    p->layer[0].base[P_ATK]=9;p->layer[0].base[P_REL]=66;
    p->layer[1].base[P_TRANS]=-12;p->layer[1].base[P_PAN]=24;
    p->layer[1].base[P_REV]=61;p->layer[1].base[P_DLY]=19;
    p->layer[2].base[P_PAN]=-3;p->layer[2].base[P_REV]=32;
    p->layer[3].base[P_PAN]=10;p->layer[3].base[P_DLY]=4;
    amb_anchor_master(p);
}
