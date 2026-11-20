/* SPDX-License-Identifier: GPL-3.0-only
 * Live HOME arrangement. Musical time drives colour and harmony independently
 * of encoder speed. The piece itself holds the audible edits, so SAVE captures
 * the current arrangement and recall starts a fresh performance from it. */
#define AMB_TRAVEL_RANGE 18
static struct {
    volatile int32_t wanted;
    int32_t harmony, stage[4];
    int8_t colour;
    uint8_t ready, chord[2*AMB_TRAVEL_RANGE+1];
    int16_t origin_macro[4][4], origin_clip, origin_echo_colour;
    amb_layer_t origin[4];
    uint32_t turned_ms;
    uint8_t indicator;
    uint8_t distance, pending_layers, anchor, previous_sound[4];
    int8_t direction;
    int32_t pattern_origin;
    uint8_t pattern_base[3];
    uint32_t chapter;
    amb_walk_t walk;
} travel;

static int amb_endless(void){return (favorites.factory[15][13]&2)!=0;}
static int amb_vary_patterns(void){return (favorites.factory[15][13]&4)!=0;}
static int amb_pattern_position(void)
{
    return amb_journey_style()?(int)travel.chapter:travel.wanted;
}
static void amb_anchor_patterns(void)
{
    travel.pattern_origin=amb_pattern_position();
    for(unsigned f=0;f<3;f++)
        travel.pattern_base[f]=amb.policy.pending[f]==255?amb.policy.active[f]:amb.policy.pending[f];
}
static void amb_travel_patterns(int playing)
{
    if(!travel.ready || !amb_vary_patterns())return;
    int stage=amb_pattern_position()-travel.pattern_origin;
    if(!amb_journey_style())stage/=4;
    int distance=stage<0?-stage:stage;
    for(unsigned f=0;f<3;f++){
        int advances=(distance+2-(int)f)/3;
        int count=(int)amb_style_count(f);
        int target=(travel.pattern_base[f]+(stage<0?-advances:advances)%count+count)%count;
        if(target==amb.policy.active[f] || amb.policy.pending[f]!=255)continue;
        amb_policy_choose(&amb.policy,f,(unsigned)target,playing);
        if(playing)break;
    }
}
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
    amb_anchor_patterns();
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
    int position=travel.wanted;
    int distance=position<0?-position:position;
    int tone=clamp(distance,0,18)*4;
    if(distance>18){
        int phase=(distance-18)%16;
        tone=72+(phase<8?phase:16-phase)*2;
    }
    int old=travel.colour,delta=(position<0?-tone:tone)-old;
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
static int amb_travel_stage(unsigned k,int position)
{
    int threshold=4+4*(int)k;
    int distance=position<0?-position:position;
    int stage=distance<threshold?0:1+(distance-threshold)/16;
    return position<0?-stage:stage;
}
static void amb_travel_rebase(unsigned k)
{
    if(!travel.ready)return;
    travel.origin[k]=amb.layer[k];
    travel.stage[k]=amb_travel_stage(k,travel.wanted);
}

static void amb_landscape_apply(unsigned k,const amb_layer_t *sound)
{
    amb_layer_t *l=&amb.layer[k];
    amb_sound_change(k);
    int mute=l->base[P_MUTE],pan=l->base[P_PAN],transpose=l->base[P_TRANS];
    memcpy(l->base,sound->base,sizeof l->base);l->patch=sound->patch;l->engine=sound->engine;
    l->base[P_MUTE]=mute;l->base[P_PAN]=pan;l->base[P_TRANS]=transpose;
    if(!ambr.handoff[k])amb_apply_patch(k);
}

/* A style switch establishes a local starting arrangement, even mid-song. */
static void amb_journey_recentre(void)
{
    ambr.loading=1;
    memset(&travel,0,sizeof travel);
    ambr.loading=0;
}

static void amb_journey_phrase(unsigned style,int movement)
{
    unsigned total=travel.distance+(unsigned)(movement<0?-movement:movement);
    travel.direction=movement<0?-1:1;
    unsigned interval=style==1?4:3;
    travel.distance=total%interval;
    if(total<interval)return;
    travel.chapter+=total/interval;
    unsigned pick=amb_hash(amb.policy.seed+travel.chapter);
    if(style==1){
        /* Keep three timbres and rotate the fourth through the ensemble. */
        travel.pending_layers=1u<<((amb.policy.seed+travel.chapter)&3);
    }else{
        /* Rotate the anchor, vary the introduction order, carry its patch. */
        travel.anchor=(travel.anchor+1+(pick%3))&3;
        travel.pending_layers=15u^(1u<<travel.anchor);
    }
}
static void amb_journey_orchestrate(int playing)
{
    static const uint8_t voices[7][2][4]={
        {{5,15,20,9},{18,22,17,8}},{{3,13,6,8},{9,12,5,2}},
        {{8,5,2,7},{13,14,11,10}},{{7,6,0,3},{1,10,8,9}},
        {{5,4,6,0},{2,8,7,1}},{{10,7,5,3},{12,15,2,1}},{{3,1,5,11},{4,6,0,8}}
    };
    for(unsigned j=0;j<4;j++){
        unsigned k=(j+travel.chapter)&3;
        if(!(travel.pending_layers&(1u<<k)) || ambr.handoff[k] || ambr.fade_slot[k])continue;
        amb_layer_t replacement=amb.layer[k];
        int sample=replacement.role==6;
        unsigned current=sample?replacement.base[P_E0]:replacement.patch;
        const uint8_t *choices=voices[replacement.role][travel.direction>0];
        unsigned index=amb_hash(amb.policy.seed+travel.chapter*37+k)&3;
        while(choices[index]==current || choices[index]+1==travel.previous_sound[k])index=(index+1)&3;
        if(sample)replacement.base[P_E0]=choices[index];
        else amb_fill_patch(&replacement,amb.key,k,replacement.role,choices[index]);
        travel.previous_sound[k]=current+1;
        amb_landscape_apply(k,&replacement);
        travel.pending_layers&=~(1u<<k);
        if(playing)break;
    }
}
static int amb_landscape_instrument(unsigned k,int position)
{
    /* Orchestration changes only the sound. The sequence stays with its layer. */
    static const uint8_t palette[7][2][2]={
        {{5,20},{18,22}}, {{3,13},{9,12}}, {{8,12},{13,14}},
        {{7,6},{1,10}}, {{5,6},{2,8}}, {{10,21},{12,15}}, {{3,1},{4,6}}
    };
    amb_layer_t *l=&amb.layer[k];
    int stage=amb_travel_stage(k,position);
    int magnitude=stage<0?-stage:stage;
    if(stage==travel.stage[k] || ambr.handoff[k] || ambr.fade_slot[k])return 0;
    if(!travel.stage[k])travel.origin[k]=*l;
    const amb_layer_t *saved=&travel.origin[k];
    amb_layer_t replacement;
    const amb_layer_t *sound=saved;
    if(stage){
        const uint8_t *choices=palette[l->role][stage>0];
        unsigned choice=(amb_hash(amb.policy.seed+19*k)+magnitude-1)&1;
        unsigned original=l->role==6?saved->base[P_E0]:saved->patch;
        if(magnitude==1 && choices[choice]==original)choice^=1;
        replacement=*l;
        if(l->role==6)replacement.base[P_E0]=choices[choice];
        else amb_fill_patch(&replacement,amb.key,k,l->role,choices[choice]);
        sound=&replacement;
    }
    amb_landscape_apply(k,sound);
    travel.stage[k]=stage;
    return 1;
}
static int amb_landscape_bar(void)
{
    int position=travel.wanted;
    unsigned style=amb_journey_style();
    if(style)amb_journey_orchestrate(song.playing);
    else for(unsigned k=0;k<4;k++)if(amb_landscape_instrument(k,position) && song.playing)break;
    if(position==travel.harmony)return 0;
    int scale[12],count=amb_scale(amb_pool(),scale);
    if(style)amb.chord=amb_walk_next(&travel.walk,amb.policy.seed+travel.chapter*31+(uint32_t)position,amb.chord,count);
    else if(position>=-18 && position<=18)amb.chord=travel.chord[AMB_TRAVEL_RANGE+position]%count;
    else amb.chord=(amb_hash(amb.policy.seed+(uint32_t)position/4u)+(uint32_t)position)%count;
    amb_revoice();travel.harmony=position;
    return 1;
}
static void amb_landscape_turn(int delta)
{
    ambr.loading=1;
    if(!travel.ready)amb_landscape_start();
    int previous=travel.wanted;
    int next=previous+delta;
    /* Wrap the distant numeric readout while staying on the same side. */
    if(amb_endless())travel.wanted=next>999999?20+(next-1000000)%16:next< -999999?-20-(-next-1000000)%16:next;
    else travel.wanted=clamp(next,-AMB_TRAVEL_RANGE,AMB_TRAVEL_RANGE);
    if(amb_journey_style() && delta && travel.wanted!=previous)
        amb_journey_phrase(amb_journey_style(),amb_endless()?delta:travel.wanted-previous);
    if(!song.playing){amb_travel_patterns(0);amb_landscape_colour(1);amb_landscape_bar();}
    ambr.dirty=1;
    ambr.loading=0;
}
