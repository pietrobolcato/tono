/* SPDX-License-Identifier: GPL-3.0-only */
#define AMB_MAIN unused_amb_native_main
#include "amb_native_test.c"
int main(void)
{
    unsigned i,k,seen=1,last=99,current=0,zero=0;int32_t out[CTL*2];int64_t wet=0;
    amb_layer_t manual;uint64_t hashes[8];
    memset(amb_nor,255,sizeof amb_nor);ui_power_on();amb_init();amb.auto_index=0;
    /* The actual runtime AUTO path must not reproduce the old 0 <-> 3 loop. */
    amb.auto_index=1;amb.evolve[0]=100;
    for(i=0;i<8*16;i++){
        if(ambr.step && ambr.step%16==0){last=current;current=amb.chord;}
        amb_step();if(i%16==0 && i){assert(amb.chord!=current && amb.chord!=last);seen|=1u<<amb.chord;}
    }assert(seen==127);
    amb_factory(0);amb.auto_index=0;
    manual=amb.layer[0];
    for(k=0;k<8;k++){
        uint64_t hash=1469598103934665603ull;unsigned distinct=0;uint16_t pairs[64];
        amb.policy.active[1]=(uint8_t)k;
        for(i=0;i<64;i++){
            uint8_t notes[2];unsigned gate=0,j;assert(amb_generated_notes(0,i*8,1,notes,&gate)==2);
            pairs[i]=(uint16_t)(notes[0]|notes[1]<<8);
            for(j=0;j<i && pairs[j]!=pairs[i];j++);if(j==i)distinct++;
            hash=(hash^pairs[i])*1099511628211ull;
            assert((amb_pool()>>((notes[0]-amb.key+120)%12))&1u);assert((amb_pool()>>((notes[1]-amb.key+120)%12))&1u);
        }
        assert(distinct>=3);hashes[k]=hash;for(i=0;i<k;i++)assert(hash!=hashes[i]);
    }assert(!memcmp(&manual,&amb.layer[0],sizeof manual));
    /* Global harmony changes leave a locked manual pattern and its gain alone. */
    amb_factory(0);amb.auto_index=0;amb.layer[2].generated=0;amb.layer[2].notes[7]=61;manual=amb.layer[2];
    ambr.solo=0;amb_prepare();for(k=0;k<NTRK;k++)memset(trk[k].v,0,sizeof trk[k].v);
    for(k=0;k<4;k++)trk[k].engine=trk[k].eng_req;
    seq_start();for(i=0;i<FS*8u/CTL;i++)mix_block(out,CTL);
    {uint8_t old[4];uint32_t pitches[NVOICE];unsigned nk=ambr.held_n[0];assert(nk);
        memcpy(old,ambr.held[0],4);for(i=0;i<NVOICE;i++)pitches[i]=(uint32_t)trk[0].v[i].pitch16;
        amb.chord=4;amb_revoice();assert(ambr.fade_request[0] && !ambr.fade_request[2]);
        assert(!memcmp(old,ambr.held[0],4));for(i=0;i<NVOICE;i++)assert(pitches[i]==(uint32_t)trk[0].v[i].pitch16);
    }
    for(i=0;i<FS*5u/CTL;i++){
        mix_block(out,CTL);
        if(!zero && ambr.fade[0].stage==AMB_FADE_IN && ambr.fade[0].g1==0){
            int32_t lq=4096<<8,dl=0;zero=1;
            assert(ambr.held_n[0]); /* a fresh incoming bed exists, at silence */
            for(k=0;k<CTL;k++)wet+=(int64_t)fx_wet[k]*fx_wet[k];
            {uint16_t old=ambr.fade[0].g0;ambr.fade[0].g0=0;amb_source_gain(0,&lq,&dl,CTL);ambr.fade[0].g0=old;}
            assert(!lq && !dl); /* wet energy below must not be a dry source */
        }
    }
    assert(zero && wet>1000 && ambr.fade[0].stage==AMB_FADE_STABLE);
    assert(!memcmp(&manual,&amb.layer[2],sizeof manual));
    printf("wet return energy at zero new-source gain: %lld PASS\n",(long long)wet);
    /* GENERATE commit is still at the 4-bar boundary, with the same tail bridge. */
    amb_factory(0);amb.auto_index=0;ambr.step=63;amb_policy_choose(&amb.policy,0,3,1);amb_step();assert(!ambr.fade_request[0]);
    amb_step();assert(amb.policy.active[0]==3 && ambr.fade_request[0]);
    /* Either EDIT or a second SEQ press returns to AUTO without deleting manual notes. */
    amb_factory(0);song.playing=0;amb_target(1);ambr.view=1;ambr.page=4;
    amb.layer[0].generated=0;amb.layer[0].notes[3]=61;manual=amb.layer[0];press(B_EDIT);assert(amb.layer[0].generated);
    assert(!memcmp(manual.notes,amb.layer[0].notes,16));press(B_SEQ);assert(!amb.layer[0].generated);assert(!memcmp(manual.notes,amb.layer[0].notes,16));
    /* Voice pressure must not restart the one Pad tone that survived. */
    amb_factory(0);amb.auto_index=0;amb_prepare();for(k=0;k<NTRK;k++)memset(trk[k].v,0,sizeof trk[k].v);
    for(k=0;k<4;k++)trk[k].engine=trk[k].eng_req;
    amb_note(0,60,70,FS*4);{voice_t *v=0;
        for(i=0;i<NVOICE;i++)if(trk[0].v[i].active)v=&trk[0].v[i];assert(v);v->env=12345;v->stage=2;v->pitch_cur=60*16;
        trk_note_on(&trk[1],48,70);trk_note_on(&trk[2],64,70);trk_note_on(&trk[3],36,70);
        amb.policy.active[1]=1;amb_layer_step(0,16,1);assert(v->stage==2 && v->env==12345 && v->pitch_cur==60*16);assert(voices_busy()<=4);
    }
    puts("AMB+ transitions: real wet tails / fresh source fade-in / manual locks / four-bar style changes / AUTO return PASS");return 0;
}
