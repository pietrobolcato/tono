/* SPDX-License-Identifier: GPL-3.0-only */
#define AMB_MAIN unused_amb_native_main
#include "amb_native_test.c"

static void shortcut(unsigned button)
{
    btn_down(B_HOME);frame();press(button);btn_up(B_HOME);frame();
}

/* Every performance knob advertised in every reachable bank has a name,
 * an explanation, and a displayed result when turned away from a limit. */
static unsigned checks;
static void check_knobs(void)
{
    amb_piece_t piece=amb;
    __typeof__(ambr) runtime=ambr;
    amb_song_controls_t controls=amb_controls;
    for(unsigned knob=0;knob<4;knob++){
        char label[16]={0},before[24]={0},after[24]={0};
        if((ambr.view==1 && ambr.page==4 && !amb.layer[0].generated && amb.layer[0].role!=4 && !amb_mono_role(&amb.layer[0]) && knob==0) ||
           (ambr.view==1 && ambr.page==3 && ambr.bank &&
            (knob==3 || (knob==2 && amb_edit_rate(&amb.layer[0])<AMB_RANDOM_BARS))) ||
           (ambr.view==2 && ambr.page==1 && knob==3) ||
           (knob==3 && ambr.view==1 && ambr.page==1 && amb_filter_editor(&amb.layer[0]) && amb.layer[0].base[P_FTYPE]>FT_HP && amb.layer[0].base[P_FTYPE]!=FT_BELL) ||
           (knob>1 && ((ambr.view==2 && ambr.page==3 && ambr.fx==4) ||
                       (ambr.view==1 && ambr.page==5 && ambr.fx==6 && !(amb_controls.filter[0]&48))))){
            amb_knob(knob,2);amb_knob_value(knob,label,after);
            assert(!*label && !*after && !memcmp(&amb,&piece,sizeof amb) && ambr.cursor==runtime.cursor);
            assert(!memcmp(&amb_controls,&controls,sizeof controls));
            continue;
        }
        unsigned card=amb_knob_card(&amb.layer[0],knob);
        assert(card && card<HELP_COUNT);
        amb_knob_value(knob,label,before);
        assert(*label && strcmp(label,"-") && *before);
        static const int delta[]={1,9,-18};
        for(unsigned d=0;d<3 && (!d || !strcmp(before,after));d++){
            amb_knob(knob,delta[d]);amb_knob_value(knob,label,after);
        }
        if(!strcmp(before,after))fprintf(stderr,"inert display view %u page %u bank %u detail %u role %u knob %u %s %s\n",
            ambr.view,ambr.page,ambr.bank,ambr.detail,amb.layer[0].role,knob,label,after);
        assert(strcmp(before,after));amb=piece;ambr=runtime;amb_controls=controls;checks++;
    }
}
/* Check the actual firmware reader against every authored paragraph, including
 * entries spanning block-directory boundaries and overlapping LZ copies. */
static void audit_help_text(void)
{
    FILE *source=fopen("tools/help/guide.txt","r");
    assert(source);
    char line[512],overview[HELP_TEXT_CAP],hint[HELP_TEXT_CAP];
    unsigned card=1;
    while(fgets(line,sizeof line,source)){
        line[strcspn(line,"\r\n")]=0;
        if(!line[0] || line[0]=='#' || line[0]=='[')continue;
        assert(card<HELP_COUNT);
        amb_help_read(overview,hint,card,card);
        if(strcmp(line,overview) || strcmp(line,hint))fprintf(stderr,"Help card %u: expected [%s], read [%s] and [%s]\n",card,line,overview,hint);
        assert(!strcmp(line,overview) && !strcmp(line,hint));
        card++;
    }
    assert(card==HELP_GUIDE_COUNT);
    FILE *cues=fopen("tools/help/sound-cues.tsv","r");
    assert(cues);
    while(fgets(line,sizeof line,cues)){
        if(line[0]=='#')continue;
        char *cue=strrchr(line,'\t');
        assert(cue);cue++;cue[strcspn(cue,"\r\n")]=0;
        amb_help_read(overview,hint,card,card);
        assert(!strcmp(cue,hint));card++;
    }
    fclose(cues);assert(card==HELP_COUNT);
    fclose(source);
    printf("%u Help paragraphs decoded byte for byte\n",card-1);
}
static void audit_controls(void)
{
    amb_fixture_scene(0);ambr.edit_layer=0;
    for(unsigned role=0;role<NELEM(AMB_ROLE);role++)for(unsigned p=0;p<(role==6?1:amb_patch_count(role));p++){
        amb_fill_patch(&amb.layer[0],amb.key,0,role,p);
        ambr.view=0;amb_target(1);check_knobs();
        ambr.view=1;
        for(unsigned page=0;page<7;page++){
            ambr.page=page;ambr.bank=0;
            if(page==1){
                for(ambr.bank=0;ambr.bank<(role==6?3:2);ambr.bank++)
                    for(ambr.detail=0;ambr.detail<(ambr.bank && role!=6?3:1);ambr.detail++)check_knobs();
            }else if(page==3){
                for(unsigned lane=0;lane<2;lane++){
                    amb_lfo_page[0]=lane;
                    ambr.bank=0;check_knobs();
                    for(unsigned mode=0;mode<3;mode++){
                        ambr.bank=1;
                        int rate=mode==0?0:(mode==1?AMB_RANDOM_HZ:AMB_RANDOM_BARS)|(2<<7)|4;
                        if(lane)amb_controls.second[0].rate=rate;
                        else amb.layer[0].base[P_LRATE]=rate;
                        check_knobs();
                    }
                }
                amb_lfo_page[0]=0;
            }else if(page==4){
                for(unsigned gen=0;gen<2;gen++)for(unsigned mode=0;mode<(role==0?AMB_CHANT+1:5);mode++){
                    amb.layer[0].generated=gen;amb.layer[0].mode=mode;check_knobs();
                }
            }else if(page==5){for(ambr.fx=0;ambr.fx<8;ambr.fx++)check_knobs();}
            else check_knobs();
        }
    }
    ambr.view=0;amb_target(0);check_knobs();
    ambr.view=2;
    for(ambr.page=0;ambr.page<5;ambr.page++)
        for(ambr.fx=0;ambr.fx<(ambr.page==3?5:1);ambr.fx++)check_knobs();
    ambr.view=4;check_knobs();
    printf("%u context-specific knob bindings passed\n",checks);
}
int main(void)
{
    unsigned k,role,patch;
    amb_piece_t saved;
    memset(amb_nor,255,sizeof amb_nor);ui_power_on();amb_init();
    for(k=0;k<NELEM(AMB_SCENE);k++){
        amb_fixture_scene(k);
        assert(amb_valid(&amb));
    }
    for(role=0;role<6;role++)for(patch=0;patch<amb_patch_count(role);patch++){
        amb_fill_patch(&amb.layer[0],amb.key,0,role,patch);
        assert(amb.layer[0].patch==patch && amb_valid(&amb));
        assert(amb_patch_preset(&amb.layer[0])<ENGINES[amb.layer[0].engine]->npresets);
    }
    /* New Pad selections keep sounding under a held key. Earlier FM Glass
     * Haze snapshots still select their original FM operator program. */
    assert(amb_patch_count(0)>=16);
    for(unsigned i=13;i<=15;i++){
        amb_fill_patch(&amb.layer[0],amb.key,0,0,i);
        assert(amb.layer[0].engine==ENGI_ANALOG && amb.layer[0].base[P_VOICE]==V_POLY);
    }
    amb_fill_patch(&amb.layer[0],amb.key,0,0,3);
    assert(amb.layer[0].engine==2 && amb_patch_preset(&amb.layer[0])==9);
    amb.layer[0].engine=ENGI_FM6;
    for(unsigned i=0;i<8;i++)amb.layer[0].base[P_E0+i]=ENG_FM6.presets[12].e[i];
    assert(amb_valid(&amb) && amb_patch_preset(&amb.layer[0])==12);
    amb_fixture_scene(10);
    assert(amb_pool()==0x295 && amb.policy.active[0]==2 && amb.layer[2].role==2);
    amb_fixture_scene(6);assert(amb.scale==1 && amb.global[G_BPM]<70);
    amb_fixture_scene(7);assert(amb.scale==2 && amb.layer[0].engine==ENGI_VOICE);
    amb_save();saved=amb;amb_fixture_scene(0);assert(amb_recall(0));assert(!memcmp(&saved,&amb,sizeof amb));
    amb_target(1);press(B_EDIT);press(B_SCL);saved=amb;shortcut(B_EDIT);
    assert(ambr.help && ambr.page==1);turn(EN_SELECT,1);assert(ambr.help==2 && !memcmp(&saved,&amb,sizeof amb));
    press(B_HOME);assert(!ambr.help && ambr.view==1 && ambr.page==1);
    shortcut(B_SEQ);assert(ambr.view==4 && song.grid==2);
    for(k=0;k<4;k++){turn(EN_K1+k,-1);assert(amb.layer[k].level<saved.layer[k].level);}
    assert(ambr.target==1);press(B_HOME);assert(ambr.view==0);
    audit_help_text();
    audit_controls();
    puts("Factory scenes, expanded recipe bounds, vocal save/recall, modal help and ensemble controls PASS");
    return 0;
}
