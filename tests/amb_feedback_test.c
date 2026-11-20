/* SPDX-License-Identifier: GPL-3.0-only */
#define AMB_MAIN unused_amb_native_main
#include "amb_native_test.c"
static int lamp(const uint8_t *a,unsigned id)
{unsigned q=led_pos[id];assert(q!=255);return !!(a[q>>3]&(1u<<(q&7)));}
int main(void)
{
    amb_layer_t saved;unsigned k,i;uint16_t pool;uint32_t ph;
    memset(amb_nor,255,sizeof amb_nor);ui_power_on();amb_init();song.playing=0;
    amb_leds();assert(lamp(fm1_led,panel.btn[B_EDIT]));assert(lamp(fm1_led,panel.btn[B_GLO]));
    ambr.view=2;ambr.page=2;amb_leds();assert(!lamp(fm1_led,panel.btn[B_REC]));
    amb_fixture_scene(0);ambr.edit_layer=2;ambr.view=1;ambr.page=4;amb_rec();
    for(i=k=0;i<16;i++)k+=amb.layer[2].notes[i]!=0;assert(k>0);
    turn(EN_SELECT,5);unsigned slot=ambr.cursor;
    for(unsigned key=0;key<27;key++){
        saved=amb.layer[2];host_notes=1u<<key;amb_input();
        assert(ambr.cursor==slot && amb.layer[2].notes[slot]==53+key);
        for(i=0;i<16;i++)if(i!=slot)assert(amb.layer[2].notes[i]==saved.notes[i]);
        amb_leds();assert(lamp(fm1_led,14+key));
    }
    press(B_ARP);assert(!amb.layer[2].notes[slot]);
    amb_patch(2,4,0);amb_rec();amb_rec();
    unsigned whites=0;
    for(unsigned key=0;key<27;key++){
        uint16_t old=amb.layer[2].drums[ambr.lane];
        host_notes=1u<<key;amb_input();
        if(key_black(key)){assert(amb.layer[2].drums[ambr.lane]==old);continue;}
        unsigned step=whites++;
        assert(ambr.cursor==step && amb.layer[2].drums[ambr.lane]==(old^(1u<<step)));
        amb_leds();assert(lamp(fm1_led,14+key)==!!(amb.layer[2].drums[ambr.lane]&(1u<<step)));
    }
    assert(whites==16);
    amb_fixture_scene(0);ambr.view=0;pool=amb_pool();host_notes=1u<<8;amb_input();
    assert(amb_pool()==(pool^(1u<<1)));assert(amb_valid(&amb));
    for(i=0;i<4;i++)assert((amb_pool()>>((ambr.voicing[i]-amb.key+120)%12))&1u);
    ambr.view=3;ambr.slot=0;amb_save();pool=amb_pool();amb_fixture_scene(2);assert(amb_recall(0));assert(amb_pool()==pool);
    amb_fixture_scene(0);saved=amb.layer[1];amb.layer[1].macro[0]=0;amb.layer[1].notes[2]=72;amb.layer[0].level=17;
    ambr.edit_layer=1;ambr.view=1;ambr.page=0;press(B_EDIT);assert(ambr.reset_pending);
    press(B_REC);assert(!memcmp(&saved,&amb.layer[1],sizeof saved));assert(amb.layer[0].level==17);
    trk[1].lfo_ph=0x12345678;ambr.step=999;ph=amb_lfo_phase(1);assert(ph==0x12345678);
    amb_fixture_scene(0);saved=amb.layer[0];for(i=0;i<4;i++)ambr.macro_smooth[0][i]=64*256;
    amb_prepare();assert(trk[0].p[P_E4]==saved.base[P_E4]);
    ambr.macro_smooth[0][0]=0;amb.layer[0].macro[0]=0;amb_prepare();k=trk[0].p[P_E4];
    ambr.macro_smooth[0][0]=127*256;amb.layer[0].macro[0]=127;amb_prepare();assert((int)k-trk[0].p[P_E4]>=65);
    assert(!memcmp(saved.base,amb.layer[0].base,sizeof saved.base));
    assert(amb.layer[0].density>=75 && amb.layer[1].density>=75);
    assert(amb.layer[0].base[P_SUS]>=105 && amb.layer[1].base[P_SUS]>=105);
    amb_fixture_scene(0);amb_prepare();trk[0].engine=trk[0].eng_req;amb_release(0);
    amb_note(0,60,70,FS*4);{voice_t *v=0;for(i=0;i<NVOICE;i++)if(trk[0].v[i].active&&trk[0].v[i].note==60)v=&trk[0].v[i];assert(v);
      v->env=12345;v->pitch_cur=60*16;
      assert(!amb_carry_note(0,62,FS*4));assert(v->env==12345&&v->note==60&&v->pitch16==60*16&&v->pitch_cur==60*16);
      assert(amb_carry_note(0,60,FS*5) && ambr.off[0]==FS*5);}
    amb_fixture_scene(0);ambr.view=0;amb_input();assert(song.grid==2);
    fm1_in.notes=1u<<8;keyboard_block();assert(kb_note[8]==KB_SILENT);fm1_in.notes=0;keyboard_block();
    {uint8_t notes[2];unsigned gate=0;assert(amb_generated_notes(0,0,1,notes,&gate)>0);assert(gate>=FS*16u);}
    /* Older experimental records have no custom pool; do not reuse reserved bytes. */
    amb_fixture_scene(0);song.playing=0;amb.version=2;amb.layer[0].reserved=254;amb.layer[1].reserved=255;
    assert(!st_save(amb_object(1),&amb,1400));assert(amb_recall(1));assert(amb.version==AMB_VERSION && amb_pool()==AMB_SCALE[amb.scale]);
    /* Scale edits and bar harmony may change voicing, never locked literal notes. */
    amb_fixture_scene(0);amb.layer[2].generated=0;amb.layer[2].notes[4]=61;saved=amb.layer[2];
    amb_pool_set(amb_pool()|2u);amb.chord=3;amb_revoice();assert(!memcmp(&saved,&amb.layer[2],sizeof saved));
    /* Cancel restore through a different page before any confirmation. */
    ambr.view=1;ambr.page=0;press(B_EDIT);assert(ambr.reset_pending);press(B_SCL);assert(!ambr.reset_pending);
    amb_fixture_scene(0);amb_prepare();for(k=0;k<4;k++){trk[k].engine=trk[k].eng_req;memset(trk[k].v,0,sizeof trk[k].v);}
    song.playing=1;ambr.last_play=0;amb_clock(CTL);assert(ambr.held_n[0] && ambr.held_n[1]);assert(ambr.off[0]>FS*16u);
    amb_release(2);ambr.solo=0;amb_layer_step(2,0,15);assert(!ambr.held_n[2]);song.playing=0;
    amb_fixture_scene(0);amb_prepare();trk[0].engine=trk[0].eng_req;memset(trk[0].v,0,sizeof trk[0].v);
    amb.layer[0].generated=0;amb.layer[0].notes[0]=61;amb_layer_step(0,0,0);
    assert(ambr.held_n[0]==1 && ambr.held[0][0]==61 && ambr.off[0]>=FS*16u);
    puts("AMB+ feedback: LEDs/generated overdub/note pool/track restore/live LFO/macros/sustain/harmony carry PASS");return 0;
}
