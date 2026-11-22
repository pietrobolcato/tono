/* SPDX-License-Identifier: GPL-3.0-only */
/* Travel control encodings are shared by both generations of song saves. */
enum { WALK_OFF, WALK_BARS, WALK_SECONDS, WALK_SECONDS_OFF, WALK_MINUTES, WALK_HOURS };
static const char *const amb_walk_styles[]={"Long Journeys","New Chapters","Branching Paths"};
static const char *const amb_walk_clocks[]={"Off","Bars","Seconds","Seconds","Minutes","Hours"};
static const char *const amb_walk_labels[]={"UNIT","MIN","MAX","REACH"};
static uint8_t *amb_walk_config(void){return amb_controls.walk;}
static unsigned amb_journey_style(void){unsigned v=amb_walk_config()[4];return v<3?v:0;}
/* Keep legacy encodings for active Bars/Seconds and inactive Bars/Seconds. */
static unsigned amb_walk_unit(void)
{
    unsigned v=amb_walk_config()[0];
    if(v==WALK_SECONDS_OFF)return WALK_SECONDS;
    if(v>=6 && v<=7)return v-2;
    return v==WALK_SECONDS || v==WALK_MINUTES || v==WALK_HOURS?v:WALK_BARS;
}
static unsigned amb_walk_value(unsigned k)
{
    static const uint8_t defaults[]={0,4,4,16};
    unsigned v=amb_walk_config()[k];
    if(!k)return v==WALK_BARS || v==WALK_SECONDS || v==WALK_MINUTES || v==WALK_HOURS?v:WALK_OFF;
    return v && v<=(k==3?80u:120u)?v:defaults[k];
}
static unsigned amb_clock_turn(unsigned unit,int delta,unsigned minimum)
{
    // The legacy disabled-seconds code is not a selectable unit.
    int index=(int)unit-(unit>WALK_SECONDS);
    index=clamp(index+delta,(int)minimum,4);
    return (unsigned)(index+(index>WALK_SECONDS));
}
static uint32_t amb_clock_interval(unsigned unit)
{
    return unit==WALK_BARS?16u:unit==WALK_HOURS?3600000u:unit==WALK_MINUTES?60000u:1000u;
}
static struct {
    uint32_t due, serial;
    int goal;
    uint8_t armed, heading;
} auto_walk;
static void amb_auto_reset(void){auto_walk.armed=auto_walk.heading=0;}
static void amb_walk_clock(unsigned unit,int enabled)
{
    unsigned v=enabled?unit:unit==WALK_BARS?WALK_OFF:unit==WALK_SECONDS?WALK_SECONDS_OFF:unit+2u;
    if(amb_walk_config()[0]==v)return;
    amb_walk_config()[0]=(uint8_t)v;
    amb_auto_reset();ambr.dirty=1;
}
static void amb_journey_recentre(void);
static void amb_walk_adjust(unsigned k,int delta)
{
    if(!k){
        amb_walk_clock(amb_clock_turn(amb_walk_unit(),delta,WALK_BARS),amb_walk_value(0)!=WALK_OFF);
        return;
    }
    uint8_t *cfg=amb_walk_config();
    if(k==4){
        unsigned old=amb_journey_style();cfg[k]=(uint8_t)amb_wrap((int)old+delta,3);
        if(cfg[k]!=old)amb_journey_recentre();
    }
    else{
        cfg[k]=(uint8_t)clamp((int)amb_walk_value(k)+delta,1,k==3?80:120);
        if(k==1 && cfg[k]>amb_walk_value(2))cfg[2]=cfg[k];
        if(k==2 && cfg[k]<amb_walk_value(1))cfg[1]=cfg[k];
    }
    amb_auto_reset();ambr.dirty=1;
}
