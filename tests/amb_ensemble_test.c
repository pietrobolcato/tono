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
    for(unsigned knob=0;knob<4;knob++){
        char label[16]={0},before[24]={0},after[24]={0};
        if((ambr.view==1 && ambr.page==4 && !amb.layer[0].generated && amb.layer[0].role!=4 && knob==0) ||
           (ambr.view==2 && ambr.page==1 && knob==3)){
            amb_knob(knob,2);amb_knob_value(knob,label,after);
            assert(!*label && !*after && !memcmp(&amb,&piece,sizeof amb) && ambr.cursor==runtime.cursor);
            continue;
        }
        const char *hint=amb_knob_hint(&amb.layer[0],knob);
        assert(hint && *hint && strlen(hint)<=25);
        amb_knob_value(knob,label,before);
        assert(*label && strcmp(label,"-") && *before);
        static const int delta[]={1,9,-18};
        for(unsigned d=0;d<3 && (!d || !strcmp(before,after));d++){
            amb_knob(knob,delta[d]);amb_knob_value(knob,label,after);
        }
        if(!strcmp(before,after))fprintf(stderr,"inert display view %u page %u bank %u detail %u role %u knob %u %s %s\n",
            ambr.view,ambr.page,ambr.bank,ambr.detail,amb.layer[0].role,knob,label,after);
        assert(strcmp(before,after));amb=piece;ambr=runtime;checks++;
    }
}
static void audit_controls(void)
{
    amb_fixture_scene(0);ambr.edit_layer=0;
    for(unsigned role=0;role<7;role++)for(unsigned p=0;p<(role==6?1:amb_patch_count(role));p++){
        amb_fill_patch(&amb.layer[0],amb.key,0,role,p);
        ambr.view=0;amb_target(1);check_knobs();
        ambr.view=1;
        for(unsigned page=0;page<7;page++){
            ambr.page=page;
            if(page==1){
                for(ambr.bank=0;ambr.bank<(role==6?3:2);ambr.bank++)
                    for(ambr.detail=0;ambr.detail<(ambr.bank && role!=6?3:1);ambr.detail++)check_knobs();
            }else if(page==4){
                for(unsigned gen=0;gen<2;gen++)for(unsigned mode=0;mode<(role==0?AMB_CHANT+1:5);mode++){
                    amb.layer[0].generated=gen;amb.layer[0].mode=mode;check_knobs();
                }
            }else if(page==5){for(ambr.fx=0;ambr.fx<6;ambr.fx++)check_knobs();}
            else check_knobs();
        }
    }
    ambr.view=0;amb_target(0);check_knobs();
    ambr.view=2;
    for(ambr.page=0;ambr.page<5;ambr.page++)
        for(ambr.fx=0;ambr.fx<(ambr.page==3?4:1);ambr.fx++)check_knobs();
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
    assert(ambr.help && ambr.page==1);turn(EN_SELECT,1);assert(!memcmp(&saved,&amb,sizeof amb));
    press(B_HOME);assert(!ambr.help && ambr.view==1 && ambr.page==1);
    shortcut(B_SEQ);assert(ambr.view==4 && song.grid==2);
    for(k=0;k<4;k++){turn(EN_K1+k,-1);assert(amb.layer[k].level<saved.layer[k].level);}
    assert(ambr.target==1);press(B_HOME);assert(ambr.view==0);
    audit_controls();
    puts("Factory scenes, expanded recipe bounds, vocal save/recall, modal help and ensemble controls PASS");
    return 0;
}
