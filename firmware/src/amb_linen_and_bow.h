/* SPDX-License-Identifier: GPL-3.0-only */
static void amb_linen_and_bow(amb_piece_t *p)
{
    /* Two hands on a felted grand share a room with solo cello and violin. */
    static const struct {
        uint8_t role, patch, mode, speed, density, level;
        int8_t octave, pan;
        uint8_t attack, release, cut, reverb;
    } voice[]={
        {2,14,0,1,83,110,-12,-9,4,65,84,27},
        {2,14,2,2,45,67,0,18,7,70,81,44},
        {1,13,0,1,90,70,-12,-15,35,75,105,37},
        {0,19,AMB_CHANT,3,58,53,0,17,30,72,103,43}
    };
    static const uint8_t styles[]={5,2,3}; /* Tidal, Wander, Complement */
    p->key=2;p->scale=2;p->auto_index=3;
    p->policy.seed=0x414d4215u;p->policy.variation=32;
    memcpy(p->policy.active,styles,sizeof styles);
    p->evolve[1]=20;p->evolve[2]=12;p->evolve[3]=9;
    p->global[G_BPM]=69;p->global[G_RTYPE]=5;p->global[G_RSIZE]=57;
    p->global[G_RDAMP]=83;p->global[G_DMIX]=0;p->global[G_CDEPTH]=0;
    p->global[G_WIDTH]=82;p->global[G_CLIP]=0;
    for(unsigned k=0;k<AMB_LAYERS;k++){
        amb_layer_t *l=&p->layer[k];
        amb_fill_patch(l,p->key,k,voice[k].role,voice[k].patch);
        l->mode=voice[k].mode;l->speed=voice[k].speed;
        l->density=voice[k].density;l->level=voice[k].level;
        l->base[P_TRANS]=voice[k].octave;l->base[P_PAN]=voice[k].pan;
        l->base[P_ATK]=voice[k].attack;l->base[P_REL]=voice[k].release;
        l->base[P_REV]=voice[k].reverb;l->base[P_FCUT]=voice[k].cut;
        l->base[P_DLY]=l->base[P_CHOR]=0;
    }
    p->layer[0].swing=11;p->layer[1].swing=20;p->layer[1].mutation=25;
    p->layer[2].swing=22;p->layer[3].swing=27;
    p->layer[2].reserved=AMB_PIANO_PHRASING;
    amb_anchor_master(p);
}
