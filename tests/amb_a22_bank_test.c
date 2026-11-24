/* SPDX-License-Identifier: GPL-3.0-only */
#define AMB_MAIN unused_amb_native_main
#include "amb_native_test.c"

int main(int argc,char **argv)
{
    /* Last public patch IDs in v0.47-opt. SAMPLE stores its source separately. */
    static const unsigned old_counts[]={23,14,15,18,9,28,8,9};
    unsigned models=0;
    memset(amb_nor,255,sizeof amb_nor);
    ui_power_on();amb_init();amb_fixture_scene(AMB_SCENE_GARDEN);
    song.playing=0;
    /* Accept owner-shipped historical AMBP fixtures when supplied. */
    for(int i=1;i<argc;i++){
        FILE *f=fopen(argv[i],"rb");
        amb_piece_t original;
        assert(f && fread(&original,1,sizeof original,f)==sizeof original && fgetc(f)==EOF);
        fclose(f);
        assert(!st_save(amb_object(1),&original,sizeof original));
        assert(amb_recall(1));
        for(unsigned k=0;k<4;k++){
            const amb_layer_t *old=&original.layer[k],*loaded=&amb.layer[k];
            assert(old->role==loaded->role && old->patch==loaded->patch && old->engine==loaded->engine);
            assert(!memcmp(old->base,loaded->base,sizeof old->base));
            assert(!memcmp(old->notes,loaded->notes,sizeof old->notes));
        }
        printf("Historical sample/song data kept: %s\n",argv[i]);
    }
    for(unsigned role=0;role<NELEM(old_counts);role++){
        unsigned first=old_counts[role];
        for(unsigned j=0;j<first;j++)assert(amb_recipe(role,j)->engine!=ENGI_AMB_A22);
        for(unsigned patch=first;patch<amb_patch_count(role);patch++){
            if(amb_recipe(role,patch)->engine!=ENGI_AMB_A22)continue;
            amb_patch(0,role,patch-1);
            amb_target(1);ambr.view=1;ambr.page=1;ambr.bank=0;
            amb_layer_t *l=&amb.layer[0];
            l->generated=0;l->notes[7]=69;l->velocity[7]=84;
            turn(EN_ALGO,1);
            assert(l->patch==patch-1 && ambr.proposal==patch);
            press(B_REC);
            assert(l->engine==ENGI_AMB_A22 && l->patch==patch && amb_valid(&amb));
            assert(!l->generated && l->notes[7]==69 && l->velocity[7]==84);
            unsigned model=amb_recipe(role,patch)->preset;
            assert(model<16 && !(models&(1u<<model)));
            const char *prefix=model<8?"A22 Electric ":"A22 Analog ";
            assert(!strncmp(amb_patch_name(l),prefix,strlen(prefix)));
            assert(!strcmp(amb_patch_name(l)+strlen(prefix),ENG_AMB_A22.presets[model].name));
            assert(l->base[P_E0]==(int)model);
            models|=1u<<model;
            const preset_t *preset=&ENG_AMB_A22.presets[model];
            for(unsigned i=0;i<4;i++){
                unsigned id=amb_sound_id(l,0,i);
                assert(id==P_E1+i && l->base[id]==preset->e[1+i]);
                assert(l->base[P_ATK+i]==preset->env[i]);
                assert(amb_param_hint(l,id));
                assert(amb_route_name(l,amb_param_route(id)));
                assert(amb_param_desc(l,id)->max==100);
            }
            assert(!amb_route_name(l,amb_param_route(P_E0)));
            assert(l->base[P_REV]==0 && l->base[P_DLY]==0 && l->base[P_CHOR]==0);
            amb_knob(1,2);l->lfo_dest=amb_param_route(P_E2);l->lfo_depth=21;
            amb_layer_t saved=*l;
            ambr.slot=0;ambr.overwrite=1;amb_save();
            amb_fixture_scene(AMB_SCENE_GARDEN);
            assert(amb_recall(0) && !memcmp(&saved,&amb.layer[0],sizeof saved));
        }
    }
    assert(models==0xffffu);
    puts("All 16 A22 sounds: append-only IDs, silent proposal, committed model and controls, manual pattern, dry sends, LFO and slot recall PASS");
    return 0;
}
