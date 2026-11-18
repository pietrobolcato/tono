/* SPDX-License-Identifier: GPL-3.0-only */
/* Bounds/overlap audit of AMB+'s real bitmap renderer at exactly 240x240. */
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
static abox ar_boxes[100];static unsigned ar_n,ar_bad,ar_frames;static const char *ar_name;
static void ar_begin(void){ar_n=0;}
static void ar_cover(int a,int b,int c,int d)
{unsigned i;for(i=0;i<ar_n;i++)if(ar_boxes[i].x0>=a && ar_boxes[i].y0>=b && ar_boxes[i].x1<=c && ar_boxes[i].y1<=d)ar_boxes[i].s[0]=0;}
static void ar_text(int a,int b,int c,int d,const char *s,unsigned f)
{
    if(f&2u){printf("CLIPPED %s %s\n",ar_name,s);ar_bad++;}
    assert(ar_n<100);ar_boxes[ar_n]=(abox){a,b,c,d,{0}};strncpy(ar_boxes[ar_n++].s,s,39);
}
static void ar_blit(int x,int y,unsigned row)
{
    unsigned i,j;(void)row;
    for(i=0;i<ar_n;i++) {
        abox *a=&ar_boxes[i];if(!a->s[0])continue;
        if(a->x0+x<0 || a->y0+y<0 || a->x1+x>240 || a->y1+y>240){printf("BOUNDS %s %s\n",ar_name,a->s);ar_bad++;}
        for(j=0;j<i;j++) {
            abox *b=&ar_boxes[j];if(!b->s[0])continue;
            if(a->x0<b->x1 && a->x1>b->x0 && a->y0<b->y1 && a->y1>b->y0){printf("OVERLAP %s '%s' '%s'\n",ar_name,a->s,b->s);ar_bad++;}
        }
    }
}
static void ar_frame(const char *name,const char *dir,int save)
{
    unsigned x,y;FILE *f;char path[256];ar_name=name;ambr.dirty=1;amb_draw();ar_frames++;
    if(!save)return;
    snprintf(path,sizeof path,"%s/%s.ppm",dir,name);f=fopen(path,"wb");assert(f);fprintf(f,"P6\n240 240\n255\n");
    for(y=0;y<240;y++)for(x=0;x<240;x++){
        uint16_t c=swap16(host_screen[y*240+x]);unsigned char rgb[3]={(unsigned char)((c>>11)*255/31),(unsigned char)(((c>>5)&63)*255/63),(unsigned char)((c&31)*255/31)};fwrite(rgb,1,3,f);
    }
    fclose(f);
}
int main(int argc,char **argv)
{
    unsigned role,page,bank,target,fx,style;char name[80];const char *dir=argc>1?argv[1]:".";
    memset(amb_nor,255,sizeof amb_nor);ui_power_on();amb_init();
    for(target=0;target<5;target++){amb_target(target);snprintf(name,sizeof name,"home_%u",target);ar_frame(name,dir,1);}
    for(role=0;role<7;role++){
        amb_patch(0,role,0);ambr.target=1;ambr.edit_layer=0;ambr.view=1;
        for(page=0;page<6;page++)for(bank=0;bank<(page==1?(role==6?3u:2u):1u);bank++)for(fx=0;fx<(page==5?5u:1u);fx++){
            ambr.page=(uint8_t)page;ambr.bank=(uint8_t)bank;ambr.fx=(uint8_t)fx;
            snprintf(name,sizeof name,"role_%u_page_%u_bank_%u_fx_%u",role,page,bank,fx);ar_frame(name,dir,role==0 || role==6);
        }
    }
    for(role=0;role<6;role++){
        amb_patch(0,role,0);ambr.view=1;ambr.page=1;ambr.bank=1;
        for(bank=0;bank<3;bank++){ambr.detail=(uint8_t)bank;snprintf(name,sizeof name,"details_role_%u_group_%u",role,bank);ar_frame(name,dir,1);}
    }
    ambr.view=1;ambr.page=0;ambr.reset_pending=1;ar_frame("track_restore",dir,1);ambr.reset_pending=0;
    ambr.view=1;ambr.page=3;for(style=0;style<5;style++){
        amb.layer[0].base[P_LWAVE]=(int16_t)style;trk[0].p[P_LWAVE]=(int16_t)style;trk[0].lfo_ph=0xc0000000u;
        snprintf(name,sizeof name,"lfo_shape_%u",style);ar_frame(name,dir,1);
    }
    ambr.view=2;for(page=0;page<5;page++)for(fx=0;fx<(page==3?4u:1u);fx++){
        ambr.page=(uint8_t)page;ambr.fx=(uint8_t)fx;snprintf(name,sizeof name,"global_%u_fx_%u",page,fx);ar_frame(name,dir,1);
    }
    ambr.page=2;
    for(page=0;page<3;page++)for(style=0;style<(page==2?6u:8u);style++){
        amb_policy_choose(&amb.policy,page,style,0);snprintf(name,sizeof name,"generate_%u_%u",page,style);ar_frame(name,dir,style==7 || style==5);
    }
    for(role=0;role<7;role++){
        amb_patch(0,role,0);ambr.view=1;ambr.page=4;
        for(bank=0;bank<2;bank++){
            amb.layer[0].generated=(uint8_t)bank;snprintf(name,sizeof name,"pattern_role_%u_%s",role,bank?"auto":"manual");ar_frame(name,dir,1);
        }
    }
    ambr.view=2;ambr.page=2;ambr.notice[0]=0;fm1_ms=5000;
    ambr.fade[0].stage=AMB_FADE_OUT;ar_frame("transition_out",dir,1);
    ambr.fade[0].stage=AMB_FADE_IN;ar_frame("transition_in",dir,1);ambr.fade[0].stage=0;
    ambr.view=0;ambr.proposal=2;ar_frame("chord_proposal",dir,1);ambr.proposal=255;
    for(style=0;style<14;style++){ambr.piece_preview=(uint8_t)style;snprintf(name,sizeof name,"preset_%u",style);ar_frame(name,dir,style==0 || style==13);}
    ambr.piece_preview=255;ambr.view=3;for(style=0;style<8;style++){ambr.slot=(uint8_t)style;ambr.overwrite=style%2;snprintf(name,sizeof name,"save_%u",style);ar_frame(name,dir,1);}
    printf("AMB+ native bitmap audit: %u frames, %u clipping/bounds/overlap findings\n",ar_frames,ar_bad);return ar_bad?1:0;
}
