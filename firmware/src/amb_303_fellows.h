/* SPDX-License-Identifier: GPL-3.0-only */
/* The four chairs share GLO's Euclidean rhythm, Acid Line phrase and Blend interplay. */
static void amb_303_fellows(amb_piece_t *p)
{
    static const struct {
        uint8_t role, patch, density, speed, level, mode;
        int8_t pan;
    } parts[] = {
        {3, 11, 58, 0, 88, 2, -3},  /* Acid Undertow */
        {0, 18, 68, 1, 83, 3, -14}, /* Linen Pad */
        {2, 9, 33, 2, 65, 2, 27},   /* Nylon Harp */
        {5, 12, 72, 2, 28, 0, 13}   /* Ocean Beads */
    };
    amb_policy_init(&p->policy, 0x53414c54u);
    p->key = 2;
    p->scale = 2;
    p->auto_index = 3;
    p->policy.active[0] = 1;
    p->policy.active[1] = AMB_ACID_PHRASE;
    p->policy.variation = 28;
    p->global[G_BPM] = 74;
    p->global[G_RTYPE] = 5;
    p->global[G_RSIZE] = 95;
    p->global[G_RDAMP] = 78;
    p->global[G_DTYPE] = 3;
    p->global[G_DTIME] = 5;
    p->global[G_DFDBK] = 56;
    p->global[G_DCOLOR] = 63;
    p->global[G_DMIX] = 46;
    p->global[G_CDEPTH] = 14;
    p->global[G_CLIP] = 0;
    p->master = 835;

    for (unsigned k = 0; k < AMB_LAYERS; k++) {
        amb_layer_t *layer = &p->layer[k];
        amb_fill_patch(layer, p->key, k, parts[k].role, parts[k].patch);
        layer->generated = 1;
        layer->density = parts[k].density;
        layer->speed = parts[k].speed;
        layer->mode = parts[k].mode;
        layer->level = parts[k].level;
        layer->base[P_PAN] = parts[k].pan;
    }
    amb_layer_t *bass = &p->layer[0];
    bass->macro[0] = 54;
    bass->macro[1] = 78;
    bass->macro[2] = 77;
    bass->macro[3] = 65;
    bass->base[P_REV] = 26;
    bass->base[P_DLY] = 50;
    bass->swing = 9;
    p->layer[1].base[P_ATK] = 78;
    p->layer[1].base[P_REV] = 68;
    p->layer[2].base[P_REV] = 76;
    p->layer[2].base[P_DLY] = 53;
    amb_anchor_master(p);
}
