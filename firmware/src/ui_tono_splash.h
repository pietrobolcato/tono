/* SPDX-License-Identifier: GPL-3.0-only */
#include "tono_version.h"
#define TONO_INK RGB(37,39,38)
#define TONO_PAPER RGB(234,235,230)
#define TONO_ORANGE RGB(222,86,22)
enum { THEME_DIETER, THEME_TE_DARK, THEME_TE_LIGHT, THEME_COUNT };
static unsigned amb_theme(void);
static uint16_t amb_accent(unsigned n);
#define TE_DARK (amb_theme()==THEME_TE_DARK)
#define TE_LIGHT (amb_theme()==THEME_TE_LIGHT)
#define ABG (TE_DARK?RGB(22,22,24):TE_LIGHT?RGB(245,242,234):TONO_PAPER)
#define AINK (TE_DARK?RGB(244,241,230):TONO_INK)
#define SPL_WRITE 800u
#define SPL_HOLD 900u

enum { TONO_ART_X = 31, TONO_ART_Y = 27, TONO_ART_W = 178, TONO_ART_H = 127 };
static uint8_t spl_anim;
static uint32_t tono_last_frame;

static void tono_round(int32_t x, int32_t y, int32_t w, int32_t h,
                       int32_t corner, uint16_t ink)
{
    int32_t row;
    for (row = 0; row < h; row++) {
        int32_t inset = corner - (row < h / 2 ? row : h - 1 - row);
        if (inset < 0) inset = 0;
        cv_rect(x + inset, y + row, w - 2 * inset, 1, ink);
    }
}

/* A rotary scale viewed head-on. The pointer settles on the index at noon. */
static void tono_disc(int x,int y,int r,uint16_t ink)
{
    int span=r;
    for(int j=0;j<=r;j++){
        while(span*span+j*j>r*r)span--;
        cv_rect(x-span,y+j,2*span+1,1,ink);
        if(j)cv_rect(x-span,y-j,2*span+1,1,ink);
    }
}
static void tono_boot_art(uint32_t ms,uint16_t paper,uint16_t ink,int mono)
{
    static const int8_t ticks[][2]={{-34,34},{-39,28},{-43,22},{-46,15},{-47,8},{-48,0},{-47,-8},{-46,-15},{-43,-22},{-39,-28},{-34,-34},{-28,-39},{-22,-43},{-15,-46},{-8,-47},{0,-48},{8,-47},{15,-46},{22,-43},{28,-39},{34,-34},{39,-28},{43,-22},{46,-15},{47,-8},{48,0},{47,8},{46,15},{43,22},{39,28},{34,34}};
    cv_begin(TONO_ART_W,TONO_ART_H,paper);
    if(amb_theme()!=THEME_DIETER){
        /* Four staggered meter columns use the same accent order as HOME. */
        for(unsigned lane=0;lane<4;lane++){
            int level=clamp((int)(ms*16/SPL_WRITE)-(int)lane*2,1,8-(int)lane);
            uint16_t color=mono?ink:amb_accent(lane);
            for(int j=0;j<level;j++)cv_rect(46+lane*23,88-j*8,17,5,color);
        }
        cv_blit(TONO_ART_X,TONO_ART_Y);
        return;
    }
    const int x=89,y=63;
    for(unsigned i=0;i<NELEM(ticks);i++){
        int dx=ticks[i][0],dy=ticks[i][1],inner=i%5?44:40;
        cv_line(x+dx,y+dy,x+dx*inner/48,y+dy*inner/48,ink);
    }
    tono_disc(x,y,36,ink);
    tono_disc(x,y,35,paper);
    unsigned step=ms*15/SPL_WRITE;
    int dx=ticks[step][0],dy=ticks[step][1];
    cv_line_t(x+dx*20/48,y+dy*20/48,x+dx*32/48,y+dy*32/48,
              mono?ink:TONO_ORANGE,3);
    cv_blit(TONO_ART_X,TONO_ART_Y);
}
/* Original 119 x 33 wordmark. Every size uses these same chamfered shapes. */
static void tono_logo(int x,int y,int divisor,uint16_t paper,uint16_t ink)
{
    static const uint8_t shapes[][6]={
        {0,0,30,6,0,1}, {12,0,6,33,0,1},
        {31,9,25,24,7,1}, {37,15,13,12,4,0},
        {64,9,23,24,6,1}, {70,15,11,22,3,0},
        {64,9,6,24,0,1}, {81,19,6,14,0,1},
        {94,9,25,24,7,1}, {100,15,13,12,4,0}
    };
    for(unsigned i=0;i<NELEM(shapes);i++){
        const uint8_t *s=shapes[i];
        int round=divisor/2,left=(s[0]+round)/divisor,top=(s[1]+round)/divisor;
        int width=(s[0]+s[2]+round)/divisor-left;
        int height=(s[1]+s[3]+round)/divisor-top;
        tono_round(x+left,y+top,width,height,(s[4]+round)/divisor,s[5]?ink:paper);
    }
}
static void tono_wordmark(uint16_t paper, uint16_t ink)
{
    cv_begin(240, 72, paper);
    tono_logo(61,7,1,paper,ink);
    cv_text_in(0, 43, 240, &AF_TONO_LABEL, "v" TONO_RELEASE_VERSION, ink, paper);
    cv_text_in(0, 58, 240, &AF_TONO_LABEL, "FM1 AMBIENT SYNTHESIZER", ink, paper);
    cv_blit(0, 163);
}

static void tono_boot_frame(int first)
{
    int mono = ux.mono || settings.palette == UI_BW_INDEX;
    uint16_t paper = mono ? T_BG : ABG;
    uint16_t ink = mono ? T_TEXT : AINK;
    if(first){cv_oy=0;lcd_fill(0,0,240,240,paper);}
    tono_boot_art(tono_last_frame,paper,ink,mono);
    if(first)tono_wordmark(paper,ink);
    lcd_sync();
}

static void draw_splash(void)
{
    tono_last_frame = spl_anim && !(ui_prefs & PREF_ANIM_OFF) ? 0u : SPL_WRITE;
    tono_boot_frame(1);
}

static void splash_write(uint32_t ms)
{
    if (!spl_anim || (ui_prefs & PREF_ANIM_OFF) || tono_last_frame == SPL_WRITE)
        return;
    if (ms < SPL_WRITE && ms - tono_last_frame < 50u)
        return;
    tono_last_frame = ms < SPL_WRITE ? ms : SPL_WRITE;
    tono_boot_frame(0);
}
