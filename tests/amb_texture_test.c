/* SPDX-License-Identifier: GPL-3.0-only */
#define AMB_MAIN unused_amb_native_main
#include "amb_native_test.c"
int main(void)
{
    unsigned i,k;int32_t out[CTL*2];amb_layer_t saved;
    memset(amb_nor,255,sizeof amb_nor);ui_power_on();amb_init();
    for(i=0;i<amb_patch_count(5);i++){
        song.playing=0;amb_fixture_scene(0);amb_patch(0,5,i);
        assert(amb.layer[0].engine==(i<12?11:ENGI_AMB_SAMPLE) && amb.layer[0].mode==0 && amb.layer[0].density==100);
        amb_prepare();trk[0].engine=trk[0].eng_req;panic_req=0;
        for(k=0;k<128;k++){uint8_t notes[2];unsigned gate;
            assert(amb_generated_notes(0,k,0,notes,&gate)==(k%2?0:1));if(!(k%2))assert(notes[0]==60 && gate>=FS*30u);}
    }
    song.playing=0;amb_fixture_scene(0);amb_patch(0,5,12);ambr.solo=0;amb.auto_index=0;seq_start();
    for(i=0;i<FS*20u/CTL;i++){mix_block(out,CTL);assert(ambr.held_n[0]==1 && amb_track_active(0));}
    /* Explicit rests release the continuous part; no instant audition from entry. */
    ambr.view=1;ambr.page=4;ambr.edit_layer=0;ambr.cursor=3;press(B_REC);press(B_REC);
    assert(!amb.layer[0].generated && amb.layer[0].notes[3]);press(B_ARP);assert(!amb.layer[0].notes[3]);
    amb_layer_step(0,6,1);assert(!ambr.held_n[0]);
    /* Existing saved PHASE texture patches keep their sound and truthful name. */
    song.playing=0;amb_fixture_scene(0);amb_fill_patch(&amb.layer[0],0,0,0,1);
    amb.layer[0].role=5;amb.layer[0].patch=4;saved=amb.layer[0];assert(amb_valid(&amb));
    assert(!strcmp(amb_patch_name(&amb.layer[0]),"Distant Voices"));ambr.slot=0;amb_save();
    amb_fixture_scene(0);assert(amb_recall(0));assert(!memcmp(&saved,&amb.layer[0],sizeof saved));
    amb_patch(0,5,4);assert(amb.layer[0].engine==11 && !strcmp(amb_patch_name(&amb.layer[0]),"Distant Hiss"));
    puts("Texture: 12 noise and 10 recorded-grain recipes / steady AUTO despite sparse policy / explicit manual rest / legacy saved sound preserved PASS");return 0;
}
