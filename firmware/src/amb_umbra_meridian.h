/* SPDX-License-Identifier: GPL-3.0-only */
static void amb_umbra_meridian(amb_piece_t *p)
{
    /* A solitary vowel line over a low pedal and a slowly turning horizon. */
    static const struct { uint8_t role, patch, mode, speed, density, level; } voices[]={
        {0,8,AMB_CHANT,2,88,88}, /* Open Aah */
        {0,2,3,2,84,80},         /* Velvet Saw */
        {1,0,2,2,76,64},         /* Deep Current */
        {2,13,2,2,36,78},         /* Dew Bell */
    };
    static const uint8_t styles[]={5,2,0}; /* Tidal, Wander, Blend */
    p->key=1;p->scale=0;p->auto_index=3;
    amb_policy_init(&p->policy,0x554d4252u);
    p->policy.variation=36;
    memcpy(p->policy.active,styles,sizeof styles);
    p->evolve[0]=100;p->evolve[1]=37;p->evolve[2]=12;p->evolve[3]=12;
    p->global[G_BPM]=58;p->global[G_RTYPE]=5;p->global[G_RSIZE]=102;
    p->global[G_RDAMP]=76;p->global[G_WIDTH]=100;p->global[G_RMOD]=76;
    p->global[G_DMIX]=71;p->global[G_DFDBK]=70;p->global[G_DTYPE]=5;p->global[G_DTIME]=4;
    p->global[G_DCOLOR]=102;p->global[G_DMOD]=9;
    p->global[G_CLIP]=0;p->global[G_CDEPTH]=24;
    for(unsigned k=0;k<AMB_LAYERS;k++){
        amb_layer_t *l=&p->layer[k];
        amb_fill_patch(l,p->key,k,voices[k].role,voices[k].patch);
        l->mode=voices[k].mode;l->speed=voices[k].speed;
        l->density=voices[k].density;l->level=voices[k].level;
    }
    amb_layer_t *vocal=&p->layer[0],*bed=&p->layer[1],*pedal=&p->layer[2],*echo=&p->layer[3];
    vocal->base[P_PAN]=0;vocal->base[P_REV]=87;vocal->base[P_DLY]=28;
    vocal->base[P_ATK]=77;vocal->base[P_REL]=96;vocal->base[P_CHOR]=8;
    vocal->base[P_E0]=111;vocal->base[P_E1]=12;vocal->base[P_E2]=41;
    vocal->base[P_E3]=-3;vocal->base[P_E4]=39;vocal->base[P_E5]=31;vocal->base[P_E6]=66;
    vocal->base[P_FCUT]=94;vocal->base[P_LRATE]=89;vocal->lfo_dest=1;vocal->lfo_depth=24;
    bed->base[P_PAN]=13;bed->base[P_TRANS]=-12;bed->base[P_REV]=72;bed->base[P_DLY]=42;
    bed->base[P_E4]=48;bed->base[P_FCUT]=71;bed->base[P_E1]=8;
    bed->lfo_depth=32;bed->base[P_LRATE]=12;
    pedal->base[P_PAN]=-11;pedal->base[P_TRANS]=-12;pedal->base[P_REV]=44;
    echo->base[P_PAN]=39;echo->base[P_TRANS]=0;echo->base[P_REV]=58;echo->base[P_DLY]=104;
    echo->base[P_FCUT]=105;echo->base[P_ATK]=3;echo->base[P_REL]=84;
    amb_anchor_master(p);
}
