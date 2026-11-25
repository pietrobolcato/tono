/* SPDX-License-Identifier: GPL-3.0-only */
#define AMB_MAIN unused_amb_main
#include "amb_native_test.c"

static void verify_wire_revisions(void)
{
    static const size_t sizes[]={20,24,32,56,64,80};
    uint8_t first[ST_PAYLOAD_MAX],second[ST_PAYLOAD_MAX];
    for(unsigned take=0;take<2;take++){
        amb_fixture_scene(AMB_SCENE_GARDEN);
        if(take){
            amb_patch(0,6,0);amb.layer[0].base[P_E0]=AMB_MIC_SOURCE;
            amb_take_t *t=amb_current_take();memset(t,0,sizeof *t);
            t->format=1;t->root=60;t->length=AMB_TAKE_FRAMES;
            for(unsigned i=0;i<AMB_TAKE_BLOCKS;i++)t->block[i][1]=3;
        }
        for(unsigned revision=0;revision<NELEM(sizes);revision++){
            switch(revision){
            case 1:amb_controls.layer_thinning[1]=20;break;
            case 2:amb_controls.filter[3]=0x13;break;
            case 3:amb_controls.second[2].depth=22;break;
            case 4:amb_controls.fx_shape[2]=5;break;
            case 5:amb_controls.trem[2]=(amb_trem_t){-5,70,3,92};break;
            }
            unsigned base=take?sizeof(amb_take_song_t):sizeof(amb_piece_t);
            int expected=(int)(base+sizes[revision]);
            ambr.slot=2;ambr.overwrite=1;amb_save();
            assert(st_load(amb_object(2),first,sizeof first)==expected && first[base+1]==revision+1);
            memset(&amb_pending_song,0x7f,sizeof amb_pending_song);
            assert(amb_recall(2));ambr.slot=3;ambr.overwrite=1;amb_save();
            assert(st_load(amb_object(3),second,sizeof second)==expected && !memcmp(first,second,expected));
            /* Both schema mismatch and an unknown wire length fail before install. */
            first[base+1]=9;
            assert(!st_save(amb_object(3),first,expected) && !amb_recall(3));
            first[base+1]=(uint8_t)(revision+1);
            assert(!st_save(amb_object(3),first,expected-1) && !amb_recall(3));
        }
    }
}

int main(void)
{
    memset(amb_nor,255,sizeof amb_nor);
    ui_power_on();amb_init();amb_fixture_scene(AMB_SCENE_GARDEN);
    amb_target(1);press(B_FX);ambr.fx=6;turn(EN_SELECT,1);
    assert(ambr.view==1 && ambr.page==5 && ambr.fx==7);
    char label[16]={0},value[24]={0};
    amb_knob_value(1,label,value);
    assert(!strcmp(value,"Off") && !amb_page_bank());
    amb_lfo_settings_t old=amb_controls.second[0];unsigned depth=amb.layer[0].lfo_depth;
    amb_knob(0,15);amb_knob(1,50);amb_knob(2,1);amb_knob(3,32);
    amb_trem_t saved=amb_controls.trem[0];
    assert(saved.rate==30 && saved.depth==100 && saved.shape==1 && saved.phase==128);
    assert(depth==amb.layer[0].lfo_depth && !memcmp(&old,&amb_controls.second[0],sizeof old));
    amb_prepare();assert(!memcmp(&saved,&amb_trem_audio[0],sizeof saved));
    ambr.slot=0;amb_save();assert(amb_saved_song.compact.schema==6);
    amb_controls.trem[0].depth=0;assert(amb_recall(0));
    assert(!memcmp(&saved,&amb_controls.trem[0],sizeof saved));
    amb_patch(0,0,1);assert(!amb_controls.trem[0].depth);
    /* An actual 0.71 record ends after bell_gain. A reused read buffer
     * must clear the following 16 bytes before installing it. */
    amb_saved_song.compact.schema=5;
    assert(!st_save(amb_object(1),&amb_saved_song,sizeof amb+64));
    assert(amb_recall(1) && !amb_controls.trem[0].depth);
    /* Check the level that reaches each channel and the mono send at both
     * extrema. This oscillator must keep moving with the transport stopped. */
    unsigned low[2]={4096,4096},high[2]={0},different=0;
    amb_trem_audio[0]=(amb_trem_t){.rate=15,.depth=100,.phase=128};
    song.playing=0;
    for(unsigned i=0;i<FS*4u/CTL;i++){
        int level=1<<20,dl=0,l=4096,r=4096,dgl=0,dgr=0;
        amb_trem_mix(0,CTL,&level,&dl,&l,&dgl,&r,&dgr);
        if(i<2)continue;
        unsigned gain[2]={(unsigned)((int64_t)level*l>>20),(unsigned)((int64_t)level*r>>20)};
        assert(level >= (1<<19)-512 && level <= (1<<19)+512);
        for(unsigned c=0;c<2;c++){
            if(gain[c]<low[c])low[c]=gain[c];
            if(gain[c]>high[c])high[c]=gain[c];
        }
        different+=gain[0]>3000 && gain[1]<1000;
    }
    assert(low[0]<5 && low[1]<5 && high[0]>4090 && high[1]>4090 && different);
    amb_trem_audio[0].phase=0;
    for(unsigned i=0;i<200;i++){
        int level=1<<20,dl=0,l=4096,r=4096,dgl=0,dgr=0;
        amb_trem_mix(0,CTL,&level,&dl,&l,&dgl,&r,&dgr);
        if(i)assert(l==4096 && r==4096 && !dgl && !dgr);
    }
    amb_trem_audio[0].depth=0;
    for(unsigned i=0;i<2;i++){
        int level=1<<20,dl=0,l=4096,r=4096,dgl=0,dgr=0;
        amb_trem_mix(0,CTL,&level,&dl,&l,&dgl,&r,&dgr);
        if(i)assert(level==1<<20 && l==4096 && r==4096 && !dl && !dgl && !dgr);
    }
    verify_wire_revisions();
    puts("Layer tremolo: controls, independent LFOs, cold recall, prior schema, mono/stereo depth, stopped transport and dry exit PASS");
}
