/* SPDX-License-Identifier: GPL-3.0-only */
/* Bounds/overlap audit of Tono's real bitmap renderer at exactly 240x240. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void ar_text(int,int,int,int,const char*,unsigned);
static void ar_begin(void);
static void ar_cover(int,int,int,int);
static void ar_blit(int,int,unsigned);
#define GFX_HOOK_TEXT(a,b,c,d,s,f) ar_text(a,b,c,d,s,f)
#define GFX_HOOK_BEGIN() ar_begin()
#define GFX_HOOK_COVER(a,b,c,d) ar_cover(a,b,c,d)
#define GFX_HOOK_BLIT(x,y,r) ar_blit(x,y,r)
#define GFX_HOOK_PIXELS(n) ((void)0)
#define AMB_MAIN unused_amb_main
#include "amb_native_test.c"
typedef struct {int x0,y0,x1,y1;char s[40];} abox;
static abox ar_boxes[100];static unsigned ar_n,ar_bad,ar_frames,ar_version;static const char *ar_name;
static void ar_begin(void){ar_n=0;}
static void ar_cover(int a,int b,int c,int d)
{unsigned i;for(i=0;i<ar_n;i++)if(ar_boxes[i].x0>=a && ar_boxes[i].y0>=b && ar_boxes[i].x1<=c && ar_boxes[i].y1<=d)ar_boxes[i].s[0]=0;}
static void ar_text(int a,int b,int c,int d,const char *s,unsigned f)
{
    if(!strcmp(s,"v" TONO_RELEASE_VERSION))ar_version++;
    if(!strcmp(s,"SELECT TARGET / EDIT / SCL ENSEMBLE") ||
       !strcmp(s,"MASTER controls the selected level."))assert(ambr.help);
    if(!strcmp(s,"EDIT  Timing and reach") || !strcmp(s,"EDIT  Back to Travel") ||
       !strcmp(s,"REC apply / HOME cancel"))assert(b<93);
    if(f&2u){printf("CLIPPED %s %s\n",ar_name,s);ar_bad++;}
    assert(ar_n<100);ar_boxes[ar_n]=(abox){a,b,c,d,{0}};strncpy(ar_boxes[ar_n++].s,s,39);
}
static void ar_blit(int x,int y,unsigned row)
{
    unsigned i,j;(void)row;
    for(i=0;i<ar_n;i++) {
        abox *a=&ar_boxes[i];if(!a->s[0])continue;
        if(!strcmp(a->s,"EDIT  Timing and reach") || !strcmp(a->s,"EDIT  Back to Travel") ||
           !strcmp(a->s,"REC apply / HOME cancel"))assert(a->y0+y>=219);
        if(a->x0+x<0 || a->y0+y<0 || a->x1+x>240 || a->y1+y>240){printf("BOUNDS %s %s\n",ar_name,a->s);ar_bad++;}
        for(j=0;j<i;j++) {
            abox *b=&ar_boxes[j];if(!b->s[0])continue;
            if(a->x0<b->x1 && a->x1>b->x0 && a->y0<b->y1 && a->y1>b->y0){printf("OVERLAP %s '%s' '%s'\n",ar_name,a->s,b->s);ar_bad++;}
        }
    }
}
static void ar_save(const char *name,const char *dir)
{
    unsigned x,y;FILE *f;char path[256];
    snprintf(path,sizeof path,"%s/%s.ppm",dir,name);f=fopen(path,"wb");assert(f);fprintf(f,"P6\n240 240\n255\n");
    for(y=0;y<240;y++)for(x=0;x<240;x++){
        uint16_t c=swap16(host_screen[y*240+x]);unsigned char rgb[3]={(unsigned char)((c>>11)*255/31),(unsigned char)(((c>>5)&63)*255/63),(unsigned char)((c&31)*255/31)};fwrite(rgb,1,3,f);
    }
    fclose(f);
}
static void ar_frame(const char *name,const char *dir,int save)
{
    ar_name=name;ambr.dirty=1;amb_draw();ar_frames++;
    if(!ambr.zen || ambr.view || ambr.help){
        assert(swap16(host_screen[6*240+8])==AINK); /* T cap */
        assert(swap16(host_screen[12*240+22])==ABG); /* first o counter */
    }else{
        uint16_t ink=swap16(host_screen[6*240+8]);
        assert(ink==(TE_DARK?0xffff:0));
        assert(!ar_n); /* wordmark geometry only; no HUD text in either strip */
    }
    if(save)ar_save(name,dir);
}
static void ar_splash(const char *dir)
{
    static uint16_t settled[240*240];
    const unsigned index_pixel=64*240+120;
    uint32_t palette=settings.palette,prefs=ui_prefs;
    settings.palette=UI_JIANT_INDEX;
    palette_set(settings.palette);
    ar_name="tono_boot_start";
    spl_anim=1;
    ui_prefs&=~PREF_ANIM_OFF;
    draw_splash();
    assert(ar_version==1);
    ar_save(ar_name,dir);ar_frames++;
    assert(swap16(host_screen[index_pixel])!=TONO_ORANGE);
    assert(swap16(host_screen[0])==TONO_PAPER);

    ar_name="tono_boot_sweep";
    splash_write(SPL_WRITE*3/4);
    ar_save(ar_name,dir);ar_frames++;
    assert(swap16(host_screen[index_pixel])!=TONO_ORANGE);
    memcpy(settled,host_screen,sizeof settled);

    ar_name="tono_boot_final";
    splash_write(SPL_WRITE);
    ar_save(ar_name,dir);ar_frames++;
    assert(memcmp(settled,host_screen,sizeof settled));
    assert(swap16(host_screen[index_pixel])==TONO_ORANGE);
    memcpy(settled,host_screen,sizeof settled);
    {
        uint64_t sent=ui_sent_px;
        splash_write(SPL_WRITE+SPL_HOLD-1);
        assert(ui_sent_px==sent);
        assert(!memcmp(settled,host_screen,sizeof settled));
    }
    ar_name="tono_boot_motion_off";
    ui_prefs|=PREF_ANIM_OFF;
    draw_splash();
    splash_write(0);
    assert(!memcmp(settled,host_screen,sizeof settled));
    ar_frames++;

    ar_name="tono_boot_mono";
    settings.palette=UI_BW_INDEX;
    palette_set(settings.palette);
    draw_splash();
    assert(swap16(host_screen[0])==T_BG && T_BG!=T_TEXT);
    for(unsigned i=0;i<240*240;i++){
        uint16_t c=swap16(host_screen[i]);
        assert((c>>11)==(c&31u) && abs((int)((c>>5)&63u)/2-(int)(c&31u))<=1);
    }
    ar_save(ar_name,dir);ar_frames++;
    settings.palette=palette;
    palette_set(settings.palette);
    ui_prefs=prefs;
}
int main(int argc,char **argv)
{
    unsigned role,page,bank,target,fx,style;char name[80];const char *dir=argc>1?argv[1]:".";
    memset(amb_nor,255,sizeof amb_nor);ui_power_on();amb_init();amb_fixture_scene(0);
    ar_splash(dir);
    /* A scale edit changes the corresponding printed key and leaves the
     * other eleven tiles intact. Sample the fill above each note label. */
    ambr.view=2;ambr.page=1;
    unsigned original_pool=amb_pool(),rel=1;
    ar_frame("field_harmony",dir,0);
    uint16_t fills[12];
    for(unsigned j=0;j<12;j++)fills[j]=host_screen[79*240+19+j*17];
    amb_pool_set(original_pool^(1u<<rel));ar_frame("field_harmony_edit",dir,0);
    for(unsigned j=0;j<12;j++)assert((fills[j]!=host_screen[79*240+19+j*17])==(j==rel));
    amb_fixture_scene(0);ambr.view=0;
    for(target=0;target<5;target++){amb_target(target);snprintf(name,sizeof name,"home_%u",target);ar_frame(name,dir,1);}
    amb_target(4);fm1_ms=10000;amb_master(0,0);amb_master(800,0);ar_frame("layer_level_feedback",dir,1);
    fm1_ms+=1200;ar_frame("master_after_level_feedback",dir,1);
    for(role=0;role<7;role++){
        amb_patch(0,role,0);ambr.target=1;ambr.edit_layer=0;ambr.view=1;
        for(page=0;page<7;page++)for(bank=0;bank<(page==1?(role==6?3u:2u):1u);bank++)for(fx=0;fx<(page==5?6u:1u);fx++){
            ambr.page=(uint8_t)page;ambr.bank=(uint8_t)bank;ambr.fx=(uint8_t)fx;
            snprintf(name,sizeof name,"role_%u_page_%u_bank_%u_fx_%u",role,page,bank,fx);ar_frame(name,dir,role==0 || role==6);
        }
    }
    for(role=0;role<6;role++){
        amb_patch(0,role,0);ambr.view=1;ambr.page=1;ambr.bank=1;
        for(bank=0;bank<3;bank++){ambr.detail=(uint8_t)bank;snprintf(name,sizeof name,"details_role_%u_group_%u",role,bank);ar_frame(name,dir,1);}
    }
    amb_patch(0,5,2);ambr.view=1;ambr.page=1;ambr.bank=0;ar_frame("texture_high_air",dir,1);
    ambr.page=4;ambr.recording=0;amb.layer[0].generated=0;amb.layer[0].notes[ambr.cursor]=60;ar_frame("texture_manual_clear",dir,1);
    ambr.view=1;ambr.page=0;ambr.reset_pending=1;ar_frame("track_restore",dir,1);ambr.reset_pending=0;
    ambr.view=1;ambr.page=3;for(style=0;style<5;style++){
        amb.layer[0].base[P_LWAVE]=(int16_t)style;trk[0].p[P_LWAVE]=(int16_t)style;trk[0].lfo_ph=0xc0000000u;
        snprintf(name,sizeof name,"lfo_shape_%u",style);ar_frame(name,dir,1);
    }
    ambr.view=2;for(page=0;page<5;page++)for(fx=0;fx<(page==3?4u:1u);fx++){
        ambr.page=(uint8_t)page;ambr.fx=(uint8_t)fx;snprintf(name,sizeof name,"global_%u_fx_%u",page,fx);ar_frame(name,dir,1);
    }
    ambr.page=2;
    for(page=0;page<3;page++)for(style=0;style<amb_style_count(page);style++){
        amb_policy_choose(&amb.policy,page,style,0);snprintf(name,sizeof name,"generate_%u_%u",page,style);ar_frame(name,dir,style>=7 || style==5);
    }
    for(role=0;role<7;role++){
        amb_patch(0,role,0);ambr.view=1;ambr.page=4;
        for(bank=0;bank<2;bank++){
            amb.layer[0].generated=(uint8_t)bank;snprintf(name,sizeof name,"pattern_role_%u_%s",role,bank?"auto":"manual");ar_frame(name,dir,1);
        }
    }
    ambr.view=2;ambr.page=2;ambr.notice[0]=0;fm1_ms=5000;
    ar_frame("generate_guidance",dir,1);
    ambr.view=1;ambr.page=0;ambr.proposal=1;ar_frame("role_proposal",dir,1);ambr.proposal=255;
    ambr.page=4;ambr.recording=1;ar_frame("pattern_recording",dir,1);ambr.recording=0;
    amb_say("MASTER: PICKUP SAVED LEVEL");ar_frame("footer_notice",dir,1);ambr.notice[0]=0;
    ambr.view=1;ambr.page=6;amb.layer_eq[0][0]=6;amb.layer_eq[0][2]=-3;ar_frame("layer_eq",dir,1);
    ambr.handoff[0]=1;ambr.view=1;ambr.page=1;ar_frame("release_pending",dir,1);ambr.handoff[0]=0;
    ambr.view=0;ambr.proposal=2;ar_frame("chord_proposal",dir,1);ambr.proposal=255;
    for(style=0;style<NELEM(AMB_SCENE)+AMB_SLOTS;style++){ambr.piece_preview=(uint8_t)style;snprintf(name,sizeof name,"preset_%u",style);ar_frame(name,dir,style==AMB_SCENE_EVENING_ARRAY || style==AMB_SCENE_FUMAROLE || style==AMB_SCENE_EMBER_MERIDIAN || style==AMB_SCENE_COLD_DESERT);}
    ambr.piece_preview=255;ambr.view=3;for(style=0;style<8;style++){ambr.slot=(uint8_t)style;ambr.overwrite=style%2;snprintf(name,sizeof name,"save_%u",style);ar_frame(name,dir,1);}
    amb_fixture_scene(7);ambr.view=4;ar_frame("ensemble",dir,1);
    ambr.view=0;ambr.help=1;ar_frame("home_help",dir,1);
    ambr.view=1;ambr.page=1;ambr.help=1;ar_frame("vowel_help",dir,1);
    for(role=0;role<7;role++){
        amb_patch(0,role,role==0?10:0);
        for(page=0;page<7;page++)for(bank=0;bank<(page==1?(role==6?3u:2u):1u);bank++){
            ambr.page=page;ambr.bank=bank;
            for(fx=0;fx<(page==5?6u:1u);fx++){ambr.fx=fx;ar_frame("layer_help",dir,0);}
        }
    }
    ambr.view=2;for(page=0;page<5;page++){
        ambr.page=page;for(fx=0;fx<(page==3?4u:1u);fx++){ambr.fx=fx;ar_frame("global_help",dir,0);}
    }
    ambr.help=0;amb_fixture_scene(0);
    for(unsigned theme=0;theme<THEME_COUNT;theme++){
        ambr.view=2;ambr.page=5;if(theme)turn(EN_SELECT,1);
        for(unsigned k=0;k<4;k++)turn(EN_K1+k,1);
        assert(amb_theme()==theme);
        ambr.view=0;
        song.cpu_q8=320;
        song.batt_raw=565;
        amb.global[G_BPM]=140;
        amb_target(0);
        amb.layer[3].reserved=0xa1;
        for(unsigned layer=0;layer<4;layer++)amb_bar_octave[layer]=-3;
        snprintf(name,sizeof name,"home_status_%u",theme);ar_frame(name,dir,1);
        memset(amb_bar_octave,0,sizeof amb_bar_octave);
        assert(swap16(host_screen[95*240+98])==AQUEUE);
        amb.layer[3].reserved=0;
        assert(swap16(host_screen[12*240+218])==AINK);
        assert(swap16(host_screen[12*240+221])==AINK);
        assert(swap16(host_screen[12*240+224])==ABG);
        assert(swap16(host_screen[12*240+235])==ABG);
        usb.config=1;
        snprintf(name,sizeof name,"home_usb_power_%u",theme);ar_frame(name,dir,1);
        assert(swap16(host_screen[12*240+235])==AQUEUE);
        usb.suspended=1;
        ar_frame("home_usb_suspended",dir,0);
        assert(swap16(host_screen[12*240+235])==ABG);
        usb.config=usb.suspended=0;
        song.batt_raw=0;
        ar_frame("home_battery_unread",dir,0);
        assert(swap16(host_screen[12*240+218])==ABG);
        song.cpu_q8=0;
        amb.global[G_BPM]=72;
        ambr.view=2;ambr.page=7;
        for(timing_tab=0;timing_tab<3;timing_tab++){
            snprintf(name,sizeof name,"timing_%u_%u",theme,timing_tab);
            ar_frame(name,dir,1);
        }
        timing_tab=0;
        ambr.view=0;ambr.zen=1;
        for(unsigned sample=0;sample<11;sample++){
            ambr.zen_train=sample>=4;ambr.step=2+(sample<4?sample*32:(sample-4)*64);
            snprintf(name,sizeof name,"zen_%u_%u",theme,sample);ar_frame(name,dir,1);
        }
        ambr.zen=ambr.zen_train=0;
        for(unsigned view=0;view<5;view++){
            ambr.view=view;
            unsigned pages=view==1?NELEM(AMB_EDIT):view==2?NELEM(AMB_GLO):1;
            for(ambr.page=0;ambr.page<pages;ambr.page++){
                snprintf(name,sizeof name,"theme_%u_view_%u_page_%u",theme,view,ambr.page);
                ar_frame(name,dir,1);
                for(unsigned module=0;module<(view==1 && ambr.page==5?6u:view==2 && ambr.page==3?4u:1u);module++){
                    ambr.fx=module;
                    if(!amb_page_bank())continue;
                    for(unsigned choice=1;choice<=3;choice++){
                        ambr.local_preset=choice;ar_frame("page_variation",dir,0);
                    }
                    ambr.local_preset=0;
                }
            }
        }
    }
    ambr.view=1;ambr.page=1;ambr.bank=0;ambr.edit_layer=0;
    for(role=0;role<7;role++){
        amb_patch(0,role,0);
        unsigned count=role==6?NELEM(N_AS_SOURCE):amb_patch_count(role);
        for(unsigned patch=1;patch<count;patch++){
            ambr.proposal=patch;
            ar_frame("sound_proposal",dir,0);
        }
    }
    printf("Tono native bitmap audit: %u frames, %u clipping/bounds/overlap findings\n",ar_frames,ar_bad);return ar_bad?1:0;
}
