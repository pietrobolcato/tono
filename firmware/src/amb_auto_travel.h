/* SPDX-License-Identifier: GPL-3.0-only
 * Global settings; these engine-15 bytes were unused in older PER4 records.
 * All zero decodes to Off, 4–4 bars/seconds, reach 16 and Long Journeys. */
enum { WALK_OFF, WALK_BARS, WALK_SECONDS };
static const char *const amb_walk_styles[]={"Long Journeys","New Chapters","Branching Paths"};
static const char *const amb_walk_clocks[]={"Off","Bars","Seconds"};
static const char *const amb_walk_labels[]={"UNIT","MIN","MAX","REACH"};
static uint8_t *amb_walk_config(void){return &favorites.factory[15][8];}
/* Seed RAM before persist_boot so even an explicitly disabled saved clock wins. */
static void amb_fresh_travel(void)
{
    static const uint8_t fresh[]={WALK_BARS,4,16,32,2,2}; /* Branching Paths, Endless */
    memcpy(amb_walk_config(),fresh,sizeof fresh);
}
static unsigned amb_journey_style(void){unsigned v=amb_walk_config()[4];return v<3?v:0;}
static unsigned amb_walk_value(unsigned k)
{
    static const uint8_t defaults[]={0,4,4,16};
    unsigned v=amb_walk_config()[k];
    return k==0?(v<=2?v:0):v && v<=(k==3?80u:120u)?v:defaults[k];
}
/* Existing records use 0 for off and 1/2 for an active clock.
 * Value 3 remembers seconds while the clock is off. */
static unsigned amb_walk_unit(void)
{
    unsigned v=amb_walk_config()[0];
    return v==WALK_SECONDS || v==3 ? WALK_SECONDS : WALK_BARS;
}
static struct {
    uint32_t due, serial;
    int goal;
    uint8_t armed, heading;
} auto_walk;
static void amb_auto_reset(void){auto_walk.armed=auto_walk.heading=0;}
static void amb_walk_clock(unsigned unit,int enabled)
{
    unsigned v=enabled?unit:unit==WALK_SECONDS?3u:0u;
    if(amb_walk_config()[0]==v)return;
    amb_walk_config()[0]=(uint8_t)v;
    amb_auto_reset();settings_save();ambr.dirty=1;
}
static void amb_journey_recentre(void);
static void amb_walk_adjust(unsigned k,int delta)
{
    uint8_t *cfg=amb_walk_config();
    if(k==4){
        unsigned old=amb_journey_style();cfg[k]=(uint8_t)amb_wrap((int)old+delta,3);
        if(cfg[k]!=old)amb_journey_recentre();
    }
    else{
        cfg[k]=(uint8_t)clamp((int)amb_walk_value(k)+delta,k?1:0,k==0?2:k==3?80:120);
        if(k==1 && cfg[k]>amb_walk_value(2))cfg[2]=cfg[k];
        if(k==2 && cfg[k]<amb_walk_value(1))cfg[1]=cfg[k];
    }
    amb_auto_reset();settings_save();ambr.dirty=1;
}
