/* SPDX-License-Identifier: GPL-3.0-only */
static void amb_analog_days(amb_piece_t *p)
{
    /* A Wurli carries the tune. Brass blooms behind it, while the sub and
     * distant mallets answer from opposite ends of the register. */
    static const struct {
        uint8_t role, patch, level, density, speed, mode, room, delay;
        int8_t pan, transpose;
    } chairs[AMB_LAYERS] = {
        {2,16,120,84,1,2,42,28,-10,-12}, /* Electric Wurli */
        {0,26, 83,67,2,3,61,12, 15,  0}, /* Analog Brass */
        {3,18, 90,68,2,0,13, 0,  0,  0}, /* Analog Sub */
        {2,20, 72,33,3,2,83,52, 29,  0}  /* Electric Vibes */
    };
    amb_policy_init(&p->policy,0x41444159u);
    p->key=5;p->scale=3;p->auto_index=2;
    p->policy.active[0]=2;p->policy.active[1]=1;p->policy.variation=40;
    p->global[G_BPM]=78;p->master=980;
    p->global[G_RTYPE]=5;p->global[G_RSIZE]=79;p->global[G_RDAMP]=70;
    p->global[G_DTYPE]=3;p->global[G_DTIME]=5;p->global[G_DFDBK]=38;
    p->global[G_DMIX]=44;p->global[G_DCOLOR]=53;p->global[G_CLIP]=0;
    for(unsigned k=0;k<AMB_LAYERS;k++){
        amb_layer_t *l=&p->layer[k];
        amb_fill_patch(l,p->key,k,chairs[k].role,chairs[k].patch);
        l->level=chairs[k].level;l->density=chairs[k].density;
        l->speed=chairs[k].speed;l->mode=chairs[k].mode;
        l->base[P_REV]=chairs[k].room;l->base[P_DLY]=chairs[k].delay;
        l->base[P_PAN]=chairs[k].pan;l->base[P_TRANS]=chairs[k].transpose;
    }
    p->layer[0].swing=13;
    p->layer[1].base[P_ATK]=36;p->layer[1].base[P_REL]=56;
    amb_anchor_master(p);
}
