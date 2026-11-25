/* SPDX-License-Identifier: GPL-3.0-only */
/* Offline audition of real Tono voices and effects using the native test harness. */
#define AMB_MAIN unused_amb_main
#include "../tests/amb_native_test.c"

static const char *const role_names[]={"Pad","Drone","Bells","Bass","Drums","Texture","Sample","Lead"};

static void initialize(void)
{
    memset(amb_nor,255,sizeof amb_nor);
    ui_power_on();
    amb_init();
}

static void catalog(void)
{
    for(unsigned role=0;role<NELEM(role_names);role++){
        unsigned count=role==6?AMB_MIC_SOURCE:amb_patch_count(role);
        for(unsigned id=0;id<count;id++){
            const char *name=role==6?N_AS_SOURCE[id]:amb_recipe(role,id)->name;
            printf("%u\t%u\t%s\t%s\n",role,id,role_names[role],name);
        }
    }
}

static void protected_scenes(void)
{
    const unsigned scenes[]={AMB_SCENE_GARDEN,AMB_SCENE_INN};
    for(unsigned i=0;i<NELEM(scenes);i++){
        amb_make_scene(&amb_pending,scenes[i]);
        printf("%s\t%08x\n",AMB_SCENE[scenes[i]],st_crc32(&amb_pending,sizeof amb_pending));
        for(unsigned k=0;k<4;k++){
            const amb_layer_t *layer=&amb_pending.layer[k];
            printf("%u\t%u\t%s\t%d\n",layer->role,layer->patch,amb_patch_name(layer),layer->base[P_E0]);
        }
    }
}

static void audition(unsigned role,unsigned id,FILE *wav,int mixed,int variation)
{
    static const uint8_t melodic_notes[]={60,64,67,62,69};
    static const uint8_t low_notes[]={48,55,50,57,48};
    static const uint8_t drum_notes[]={36,38,42,46,36,42,38,46};
    const unsigned frames=(variation<0?30:6)*FS;
    unsigned note=0;
    initialize();
    amb_fixture_scene(0);
    amb_patch(0,role,role==6?0:id);
    if(role==6)amb.layer[0].base[P_E0]=(int16_t)id;
    if(variation>=0){
        if(role==6 && id==AMB_MIC_SOURCE){
            /* Only synthetic test audio is used for the microphone source. */
            amb_take_start();
            for(unsigned i=0;i<FS;i++){
                int16_t v=(int16_t)(8500*sin(i*2.*3.14159265*220/FS)+3200*sin(i*2.*3.14159265*660/FS));
                uint32_t stereo=(uint16_t)v | ((uint32_t)(uint16_t)v<<16);
                amb_take_feed(&stereo,1);
            }
            assert(amb_take_finish());
        }
        ambr.view=1;ambr.page=1;ambr.edit_layer=0;ambr.local_preset=variation+1;
        assert((unsigned)variation<amb_local_count());
        amb_apply_page_preset();
    }
    amb.layer[0].level=(uint8_t)amb.layer[0].base[P_LEVEL];
    amb.master=680;
    amb_prepare();
    trk[0].engine=amb.layer[0].engine;
    trk[0].xf_on=trk[0].xf=trk[0].xin=0;
    amb_render(FS/2); /* Apply the patch and drain its queued all-notes-off. */
    assert(amb_valid(&amb));
    wav_hdr(wav,frames);
    for(unsigned at=0;at<frames;at+=CTL){
        unsigned quantum=(role==7 || role==3?FS*3/4:role==4?FS/2:FS*6)/CTL*CTL;
        if(variation>=0 && role!=5 && role!=4 && role!=3 && role!=7)quantum=FS*2/CTL*CTL;
        unsigned position=at%quantum;
        unsigned phrase=at/quantum;
        unsigned release=(role==7 || role==3?FS/2:role==4?FS/8:role==2?FS*2:FS*4)/CTL*CTL;
        if(variation>=0 && release>=quantum)release=quantum*3/4/CTL*CTL;
        if(mixed && position==0 && !(phrase%4)){
            trk_all_off(&trk[1]);trk_note_on(&trk[1],48,55);
            trk_note_on(&trk[2],phrase%8?67:64,58);
        }
        if(position==0 && (role!=5 || phrase==0)){
            note=role==4?drum_notes[phrase%NELEM(drum_notes)]:
                 role==1 || role==3?low_notes[phrase%NELEM(low_notes)]:melodic_notes[phrase%NELEM(melodic_notes)];
            trk_note_on(&trk[0],note,76+(phrase%3)*12);
        }
        if(position==release && (role!=5 || phrase==4))trk_note_off(&trk[0],note);
        int32_t out[CTL*2];
        mix_block(out,CTL);
        amb_output(out,CTL);
        unsigned count=frames-at<CTL?frames-at:CTL;
        for(unsigned i=0;i<count;i++)wav_put(wav,out[2*i],out[2*i+1]);
    }
}

int main(int argc,char **argv)
{
    if(argc==2 && !strcmp(argv[1],"catalog")){catalog();return 0;}
    if(argc==2 && !strcmp(argv[1],"protected")){initialize();protected_scenes();return 0;}
    if(argc==2 && !strcmp(argv[1],"variations")){
        initialize();amb_fixture_scene(0);
        for(unsigned role=0;role<NELEM(role_names);role++){
            unsigned count=role==6?NELEM(N_AS_SOURCE):amb_patch_count(role);
            for(unsigned id=0;id<count;id++){
                if(role==5 && amb_texture_member(id,0)!=id)continue;
                amb_patch(0,role,id);
                amb_layer_t *l=&amb.layer[0];
                for(unsigned j=0;j<amb_sound_presets(l);j++)
                    printf("%u\t%u\t%u\t%s\t%s\t%s\n",role,id,j,role_names[role],
                        role==6?N_AS_SOURCE[id]:amb_patch_name(l),amb_sound_preset_name(l,j));
            }
        }
        return 0;
    }
    if(argc!=4 && argc!=5 && argc!=6)return 2;
    unsigned role=(unsigned)atoi(argv[1]),id=(unsigned)atoi(argv[2]);
    if(role>=NELEM(role_names) || id>=(role==6?NELEM(N_AS_SOURCE):amb_patch_count(role)))return 2;
    FILE *wav=fopen(argv[3],"wb");
    if(!wav)return 1;
    audition(role,id,wav,argc==5,argc==6 && !strcmp(argv[4],"variation")?atoi(argv[5]):-1);
    return fclose(wav)!=0;
}
