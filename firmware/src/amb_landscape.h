/* SPDX-License-Identifier: GPL-3.0-only
 * Live HOME arrangement. Musical time drives colour and harmony independently
 * of encoder speed. The piece itself holds the audible edits, so SAVE captures
 * the current arrangement and recall starts a fresh performance from it. */
#define AMB_TRAVEL_RANGE 18
static struct {
    volatile int8_t wanted;
    int8_t harmony, colour, stage[4];
    uint8_t ready, chord[2*AMB_TRAVEL_RANGE+1];
    int16_t origin_macro[4][4], origin_clip, origin_echo_colour;
    amb_layer_t origin[4];
    uint8_t last_patch[4], protect[4];
} travel;

static const char *amb_landscape_name(int position)
{
    if(position<=-12)return "DARK / DRIVEN";
    if(position<=-5)return "SHADOW / GRAIN";
    if(position<0)return "WARM / CLOSE";
    if(position>=12)return "BRIGHT / OPEN";
    if(position>=5)return "LIGHT / AIR";
    return position?"CLEAR / WIDE":"ORIGINAL COLOUR";
}
static void amb_landscape_start(void)
{
    int scale[12],count=amb_scale(amb_pool(),scale);
    for(int direction=-1;direction<=1;direction+=2){
        amb_walk_t walk={0};unsigned chord=amb.chord;
        travel.chord[AMB_TRAVEL_RANGE]=chord;
        for(int i=1;i<=AMB_TRAVEL_RANGE;i++){
            chord=amb_walk_next(&walk,amb.policy.seed+(direction<0?41u:89u),chord,count);
            travel.chord[AMB_TRAVEL_RANGE+direction*i]=chord;
        }
    }
    for(unsigned k=0;k<4;k++)for(unsigned j=0;j<4;j++)
        travel.origin_macro[k][j]=amb.layer[k].macro[j];
    travel.origin_clip=amb.global[G_CLIP];travel.origin_echo_colour=amb.global[G_DCOLOR];
    travel.ready=1;
}
static int amb_colour_offset(unsigned role,unsigned knob,int colour)
{
    /* colour is four ticks per detent. SAMPLE's first macro controls pitch. */
    if(knob==0)return role==6?0:-colour/2;
    if(knob==1)return role==4?-colour/4:colour/6;
    if(knob==2)return role==4?-colour/5:role==5?-colour/3:colour/5;
    return colour/2;
}
static int amb_landscape_value(int current,int16_t *base,int old_offset,int next_offset,int lo,int hi)
{
    if(current!=clamp(*base+old_offset,lo,hi))*base=current-old_offset;
    return clamp(*base+next_offset,lo,hi);
}
static void amb_landscape_colour(int stopped)
{
    int old=travel.colour,delta=travel.wanted*4-old;
    if(!delta)return;
    int next=old+(stopped?delta:clamp(delta,-2,2));
    for(unsigned k=0;k<4;k++)for(unsigned j=0;j<4;j++){
        amb_layer_t *l=&amb.layer[k];
        int offset=amb_colour_offset(l->role,j,old);
        /* An intervening live knob edit becomes the new local starting value. */
        l->macro[j]=amb_landscape_value(l->macro[j],&travel.origin_macro[k][j],offset,amb_colour_offset(l->role,j,next),0,127);
    }
    amb.global[G_CLIP]=amb_landscape_value(amb.global[G_CLIP],&travel.origin_clip,-old/2,-next/2,GP[G_CLIP].min,GP[G_CLIP].max);
    amb.global[G_DCOLOR]=amb_landscape_value(amb.global[G_DCOLOR],&travel.origin_echo_colour,old/2,next/2,GP[G_DCOLOR].min,GP[G_DCOLOR].max);
    travel.colour=next;
}
static int amb_landscape_instrument(unsigned k,int position)
{
    /* At most one new player per downbeat. Manual sequences and explicitly
     * chosen sample sources keep their instruments throughout the gesture. */
    static const uint8_t palette[6][2][2]={
        {{5,20},{18,22}}, {{3,13},{9,12}}, {{8,4},{13,14}},
        {{7,6},{1,10}}, {{5,6},{2,8}}, {{1,10},{8,11}}
    };
    amb_layer_t *l=&amb.layer[k];
    int threshold=4+4*(int)k;
    int stage=position>=threshold?1:position<=-threshold?-1:0;
    if(!l->generated || l->role>=6 || travel.protect[k])return 0;
    if(travel.stage[k] && (l->role!=travel.origin[k].role || l->patch!=travel.last_patch[k])){
        travel.protect[k]=1;return 0;
    }
    if(stage==travel.stage[k] || ambr.handoff[k] || ambr.fade_slot[k])return 0;
    if(!travel.stage[k])travel.origin[k]=*l;
    const amb_layer_t *saved=&travel.origin[k];
    amb_layer_t replacement;
    const amb_layer_t *sound=saved;
    if(stage){
        const uint8_t *choices=palette[l->role][stage>0];
        unsigned choice=amb_hash(amb.policy.seed+19*k)&1;
        if(choices[choice]==saved->patch)choice^=1;
        replacement=*l;
        amb_fill_patch(&replacement,amb.key,k,l->role,choices[choice]);
        sound=&replacement;
    }
    amb_sound_change(k);
    int mute=l->base[P_MUTE],pan=l->base[P_PAN],transpose=l->base[P_TRANS];
    memcpy(l->base,sound->base,sizeof l->base);l->patch=sound->patch;l->engine=sound->engine;
    l->base[P_MUTE]=mute;l->base[P_PAN]=pan;l->base[P_TRANS]=transpose;
    if(!ambr.handoff[k])amb_apply_patch(k);
    travel.stage[k]=stage;travel.last_patch[k]=l->patch;
    return 1;
}
static int amb_landscape_bar(void)
{
    int position=travel.wanted;
    for(unsigned k=0;k<4;k++)if(amb_landscape_instrument(k,position) && song.playing)break;
    if(position==travel.harmony)return 0;
    int scale[12],count=amb_scale(amb_pool(),scale);
    amb.chord=travel.chord[AMB_TRAVEL_RANGE+position]%count;
    amb_revoice();travel.harmony=position;
    return 1;
}
static void amb_landscape_turn(int delta)
{
    ambr.loading=1;
    if(!travel.ready)amb_landscape_start();
    travel.wanted=clamp(travel.wanted+delta,-AMB_TRAVEL_RANGE,AMB_TRAVEL_RANGE);
    if(!song.playing){amb_landscape_colour(1);amb_landscape_bar();}
    amb_say(amb_landscape_name(travel.wanted));
    ambr.loading=0;
}
