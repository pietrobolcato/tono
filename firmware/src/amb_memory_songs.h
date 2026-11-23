/* SPDX-License-Identifier: GPL-3.0-only */
static unsigned amb_param_route(unsigned id);
static void amb_memory_song(amb_piece_t *p,unsigned index)
{
    static const struct {
        uint8_t key,scale,bpm,space,echo;
        struct {uint8_t role,patch,level,density,speed,mode,rev,dly;int8_t pan;} chair[4];
    } scores[]={
        {4,1,84,95,53,{{3,11,93,66,1,2,28,46,-4},{0,4,67,76,1,3,64,18,-16},
                        {2,12,76,53,1,2,62,62,24},{1,7,42,67,2,0,48,10,7}}},
        {2,2,76,76,32,{{2,15,100,79,1,2,49,26,-13},{3,11,67,48,0,2,22,31,2},
                        {5,19,46,56,0,3,56,17,18},{0,21,72,42,2,5,68,19,21}}},
        {7,3,68,104,62,{{4,5,82,72,0,0,99,42,-10},{4,7,60,47,1,0,94,53,23},
                        {5,14,58,52,1,2,92,34,-18},{1,0,33,62,2,2,57,19,5}}}
    };
    const __typeof__(scores[0]) *s=&scores[index];
    amb_policy_init(&p->policy,0x4543484fu+index*997);
    const uint8_t *recipe=amb_generate_recipes[5+index].values;
    memcpy(p->policy.active,recipe,3);p->policy.variation=recipe[3];
    p->key=s->key;p->scale=s->scale;p->auto_index=index==1?1:2;p->master=920;
    p->global[G_BPM]=s->bpm;p->global[G_RTYPE]=5;p->global[G_RSIZE]=s->space;
    p->global[G_RDAMP]=84;p->global[G_DTYPE]=3;p->global[G_DTIME]=5;
    p->global[G_DFDBK]=s->echo;p->global[G_DMIX]=s->echo;p->global[G_CLIP]=0;
    for(unsigned k=0;k<4;k++){
        const __typeof__(s->chair[0]) *c=&s->chair[k];
        amb_layer_t *l=&p->layer[k];
        amb_fill_patch(l,p->key,k,c->role,c->patch);
        l->level=c->level;l->density=c->density;l->speed=c->speed;l->mode=c->mode;
        l->base[P_PAN]=c->pan;l->base[P_REV]=c->rev;l->base[P_DLY]=c->dly;
        l->swing=index==1?35:8;
        if(c->role==3){
            l->macro[0]=index?52:65;l->macro[1]=index?66:78;l->macro[2]=68;
            l->base[P_E4]=index?51:65;l->base[P_E5]=index?56:74;l->lfo_depth=index?23:37;
            l->base[P_LRATE]=12;l->base[P_REL]=23;
        }
        if(index==2 && k==3){l->base[P_FTYPE]=1;l->base[P_FCUT]=40;}
        if(index==0){
            if(k==0){l->base[P_DLY]=55;l->density=54;l->base[P_APDEP]=19;l->base[P_APRATE]=11;l->lfo_dest=amb_param_route(P_E4);l->lfo_depth=40;l->base[P_LRATE]=5;}
            if(k==1 || k==3){p->layer_eq[k][1]=-6;l->base[P_ATK]=86;}
            if(k==2){l->base[P_REL]=62;l->base[P_DLY]=80;}
            p->global[G_DPING]=1;p->global[G_WIDTH]=103;
        }
        if(index==1 && k==2){l->lfo_dest=0;l->lfo_depth=64;l->base[P_FCUT]=70;l->base[P_LRATE]=16;}
        if(index==1 && k==1){l->speed=1;l->swing=62;l->lfo_dest=3;l->lfo_depth=32;l->base[P_LRATE]=11;}
        if(index==1 && k==0){l->base[P_TRANS]=-12;l->base[P_FCUT]=85;l->speed=0;l->swing=48;}
        if(index==1 && k==3){
            p->layer_eq[k][1]=-4;l->base[P_PAN]=26;l->mode=AMB_CHANT;
            l->base[P_ATK]=36;l->base[P_SUS]=86;l->base[P_REL]=55;
        }
    }
    if(index==0)p->layer[2].reserved=AMB_BERLIN_PHRASING;
    amb_anchor_master(p);
}
