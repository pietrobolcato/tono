/* SPDX-License-Identifier: GPL-3.0-only */
enum { AMB_PIANO_PHRASING = 0x71 };
static void amb_finish_new_song(amb_piece_t *p,unsigned scene)
{
    amb_layer_t *lead=&p->layer[0], *answer=&p->layer[1],
        *third=&p->layer[2], *fourth=&p->layer[3];
    p->global[G_RTYPE]=5;
    p->global[G_CLIP]=0;
    if(scene==AMB_SCENE_EVENING_ARRAY){
        /* Warm running analogue notes under a slowly opening string canopy. */
        p->policy.active[0]=1;p->policy.variation=30;
        p->global[G_DFDBK]=55;p->global[G_DTIME]=1;
        lead->density=93;lead->level=102;lead->mode=0;lead->base[P_TRANS]=12;
        lead->base[P_E0]=1;lead->base[P_E4]=76;lead->base[P_E5]=44;lead->lfo_depth=26;lead->base[P_LRATE]=17;
        lead->base[P_ATK]=0;lead->base[P_DEC]=51;lead->base[P_SUS]=27;lead->base[P_REL]=24;
        lead->base[P_FCUT]=72;lead->base[P_FRES]=24;lead->base[P_ED_FLT]=21;
        lead->base[P_REV]=19;lead->base[P_DLY]=36;
        answer->density=96;answer->level=83;answer->mode=3;answer->base[P_TRANS]=12;
        answer->base[P_E4]=68;answer->base[P_FCUT]=91;
        answer->base[P_PAN]=-15;
        third->density=73;third->level=108;third->base[P_TRANS]=12;third->base[P_DLY]=60;third->base[P_PAN]=21;
        fourth->density=77;fourth->mode=2;fourth->base[P_REV]=45;
    }else if(scene==AMB_SCENE_FUMAROLE){
        /* One resonant line, soft submerged chords and the sound of water. */
        p->policy.variation=26;p->global[G_DFDBK]=88;p->global[G_RDAMP]=82;p->global[G_DTIME]=50;p->global[G_DTYPE]=3;
        p->global[G_DCOLOR]=56;p->global[G_DMIX]=110;
        lead->mode=0;lead->level=108;lead->base[P_TRANS]=12;lead->base[P_E0]=0;
        lead->base[P_E4]=72;lead->base[P_E5]=125;lead->base[P_E6]=15;
        lead->base[P_E2]=0;
        lead->lfo_depth=91;lead->macro[1]=106;lead->base[P_LRATE]=12;
        lead->base[P_ATK]=1;lead->base[P_DEC]=43;lead->base[P_SUS]=10;lead->base[P_REL]=33;
        lead->base[P_E1]=6;lead->base[P_FCUT]=92;lead->base[P_FRES]=24;lead->base[P_ED_FLT]=39;
        lead->base[P_DIST]=17;lead->base[P_GLIDE]=63;lead->base[P_CHOR]=0;
        lead->base[P_REV]=13;lead->base[P_DLY]=104;
        answer->density=88;answer->level=67;answer->mode=3;
        answer->base[P_TRANS]=12;answer->base[P_FCUT]=74;
        third->density=89;third->level=39;third->base[P_REV]=52;
        amb_fill_patch(fourth,p->key,3,5,0); /* Ocean Air, with the low end cleared. */
        fourth->mode=0;fourth->level=77;fourth->base[P_REV]=64;
        fourth->base[P_FTYPE]=1;fourth->base[P_FCUT]=38;
    }else{
        /* Four registers/fingers of one grand, interleaved on the same clock. */
        static const int8_t octave[]={0,-24,-12,12};
        p->policy.active[0]=3;p->policy.variation=30;p->auto_index=2;
        p->layer[2].reserved=AMB_PIANO_PHRASING;p->global[G_CDEPTH]=0;
        p->global[G_RSIZE]=58;
        p->global[G_RDAMP]=72;p->global[G_WIDTH]=82;
        for(unsigned i=0;i<AMB_LAYERS;i++){
            amb_layer_t *l=&p->layer[i];
            l->mode=i==1?1:2;
            l->density=i<3?86:58;l->swing=12+i*5;
            l->base[P_TRANS]=octave[i];l->base[P_REL]=78;
            l->base[P_FCUT]=i==0 || i==3?99:107;
            if(i==0 || i==3)l->base[P_ATK]=5;
            l->base[P_PAN]=(int)i*7-11;
            l->base[P_REV]=26;l->base[P_DLY]=l->base[P_CHOR]=0;
        }
        p->layer[1].level=83;fourth->level=73;
    }
    amb_anchor_master(p);
}
