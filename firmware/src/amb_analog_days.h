/* SPDX-License-Identifier: GPL-3.0-only */
static void amb_analog_days(amb_piece_t *p,int warm)
{
    /* Two electric pianos trade phrases above a sustained pad and a low pulse. */
    static const struct {
        uint8_t role, patch, level, density, speed, mode, room, delay;
        int8_t pan, transpose;
    } chairs[AMB_LAYERS] = {
        {2,16,114,72,0,2,35,15,-12,-12}, /* Electric Wurli */
        {0,18,101,75,1,2,48, 5, 12,-12}, /* Linen Pad */
        {3,10, 93,50,2,2,15, 0,  0,  0}, /* Linden Bass */
        {2,19,103,44,1,1,55,20, 24,-12}  /* Electric Felt */
    };
    amb_policy_init(&p->policy,0x41444159u);
    p->key=5;p->scale=1;p->auto_index=2;
    p->policy.active[0]=1;p->policy.active[1]=5;p->policy.variation=24;
    p->global[G_BPM]=76;p->master=980;
    p->global[G_RTYPE]=5;p->global[G_RSIZE]=79;p->global[G_RDAMP]=70;
    p->global[G_DTYPE]=3;p->global[G_DTIME]=5;p->global[G_DFDBK]=38;
    p->global[G_DMIX]=44;p->global[G_DCOLOR]=53;p->global[G_CLIP]=0;
    for(unsigned k=0;k<AMB_LAYERS;k++){
        amb_layer_t *l=&p->layer[k];
        unsigned patch=chairs[k].patch;
        if(warm && k==1)patch=26; /* A22 Analog Brass */
        if(warm && k==2)patch=18; /* A22 Analog Sub */
        amb_fill_patch(l,p->key,k,chairs[k].role,patch);
        l->base[P_VOICE]=V_MONO;
        l->level=chairs[k].level;l->density=chairs[k].density;
        l->speed=chairs[k].speed;l->mode=chairs[k].mode;
        l->base[P_REV]=chairs[k].room;l->base[P_DLY]=chairs[k].delay;
        l->base[P_PAN]=chairs[k].pan;l->base[P_TRANS]=chairs[k].transpose;
    }
    p->layer[0].swing=13;
    p->layer[0].base[P_E1]=33;p->layer[0].base[P_E2]=23;
    p->layer[0].base[P_E3]=14;p->layer[0].base[P_E4]=57;
    p->layer[0].base[P_FCUT]=92;p->layer[0].base[P_ATK]=5;
    p->layer[1].base[P_ATK]=34;p->layer[1].base[P_REL]=62;
    p->layer[2].base[P_ATK]=7;p->layer[2].base[P_REL]=45;
    p->layer[3].base[P_E2]=15;p->layer[3].base[P_FCUT]=82;
    p->layer[3].base[P_REL]=52;p->eq[2]=-3;
    if(warm){
        p->layer[1].level=108;p->layer[1].base[P_ATK]=26;p->layer[1].base[P_REL]=57;
        p->layer[1].base[P_E3]=42;p->layer[1].base[P_E4]=15;
        p->layer[2].level=110;p->layer[2].base[P_TRANS]=12;
    }
    amb_anchor_master(p);
}
