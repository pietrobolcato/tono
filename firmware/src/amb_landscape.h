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
    uint8_t indicator, guarded_layers;
    uint8_t distance, pending_layers, anchor, previous_sound[4];
    int8_t direction;
    uint32_t chapter, focused_chapter[4], focused_role[4], focused_moves[4];
    amb_walk_t walk;
} travel;

enum { LANE_FOLLOW = WALK_OFF };
static uint8_t timing_tab;
static const char *const timing_names[]={"Movement","Patterns","Layer types"};
static struct {uint32_t due,serial;uint8_t armed;} lane_clock[2];
static uint8_t role_pending,pattern_pending;
static unsigned amb_lane_value(unsigned lane,unsigned knob)
{
    if(!knob)return (amb_controls.walk[5]&(4u<<lane))?1u+(!lane && (amb_controls.walk[5]&16u)):0;
    unsigned raw=amb_controls.lanes[3*lane+knob-1];
    if(knob==1){
        // Old Move tick and Journey records both follow the automatic clock.
        if(raw==2 || raw==3 || raw==5 || raw==6)return raw-1;
        return LANE_FOLLOW;
    }
    return raw && raw<=120?raw:4;
}
static void amb_reset_lanes(void)
{
    lane_clock[0].armed=lane_clock[1].armed=role_pending=pattern_pending=0;
}
static unsigned amb_travel_guard(void)
{
    return ((ambr.evolve_open || ambr.evolve_queued)?1u<<ambr.evolve_layer:0)|amb_sound_locks()|(ambr.view==1?1u<<ambr.edit_layer:0);
}
static int amb_endless(void){return (amb_controls.walk[5]&2)!=0;}
static int amb_vary_patterns(void){return (amb_controls.walk[5]&4)!=0;}
static void amb_lane_adjust(unsigned lane,unsigned knob,int delta)
{
    unsigned old=amb_lane_value(lane,knob);
    unsigned unit=amb_lane_value(lane,1);
    if(knob>=2 && unit==LANE_FOLLOW)return;
    unsigned v=knob==1?amb_clock_turn(old,delta,LANE_FOLLOW):knob?(unsigned)clamp((int)old+delta,1,120):(unsigned)clamp((int)old+delta,0,lane?1:2);
    if(v==old)return;
    uint8_t *cfg=&amb_controls.lanes[3*lane];
    ambr.loading=1;
    if(!knob){
        cfg[0]=unit+1;
        unsigned flag=4u<<lane;
        amb_controls.walk[5]=(amb_controls.walk[5]&~(flag|(lane?0u:16u)))|(v?flag:0)|(v==2?16u:0);
    }else cfg[knob-1]=knob==1?v+1:v;
    if(knob==2 && amb_lane_value(lane,3)<v)cfg[2]=v;
    if(knob==3 && amb_lane_value(lane,2)>v)cfg[1]=v;
    lane_clock[lane].armed=0;
    if(lane)role_pending=0;else pattern_pending=0;
    ambr.loading=0;ambr.dirty=1;
}
static int amb_lane_due(unsigned lane)
{
    unsigned unit=amb_lane_value(lane,1);
    if(unit==LANE_FOLLOW || !amb_lane_value(lane,0))return 0;
    uint32_t now=unit==WALK_BARS?ambr.step:fm1_ms;
    int fire=lane_clock[lane].armed;
    if(fire && (int32_t)(now-lane_clock[lane].due)<0)return 0;
    unsigned lo=amb_lane_value(lane,2),hi=amb_lane_value(lane,3);
    unsigned dice=amb_hash(amb.policy.seed+lane*937u+lane_clock[lane].serial);
    unsigned wait=lo+dice%(hi>=lo?hi-lo+1:1);
    lane_clock[lane].due=now+wait*amb_clock_interval(unit);
    lane_clock[lane].armed=1;
    if(fire)lane_clock[lane].serial++;
    return fire;
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
    travel.guarded_layers|=amb_travel_guard();
    if(!delta)return;
    int next=old+(stopped?delta:clamp(delta,-2,2));
    for(unsigned k=0;k<4;k++)for(unsigned j=0;j<4;j++){
        amb_layer_t *l=&amb.layer[k];
        int offset=amb_colour_offset(l->role,j,old);
        /* An intervening live knob edit becomes the new local starting value. */
        int next_offset=amb_colour_offset(l->role,j,next);
        if(travel.guarded_layers&(1u<<k))travel.origin_macro[k][j]=l->macro[j]-next_offset;
        else l->macro[j]=amb_landscape_value(l->macro[j],&travel.origin_macro[k][j],offset,next_offset,0,127);
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
    amb_fresh_sound(k);
    int mute=l->base[P_MUTE];
    if(l->role!=sound->role){l->role=sound->role;l->mode=sound->mode;l->density=sound->density;}
    memcpy(l->base,sound->base,sizeof l->base);l->patch=sound->patch;l->engine=sound->engine;
    l->base[P_MUTE]=mute;
    memcpy(l->macro,sound->macro,sizeof l->macro);
    l->lfo_dest=sound->lfo_dest;l->lfo_depth=sound->lfo_depth;
    if(!ambr.handoff[k])amb_apply_patch(k);
}

static void amb_replace_role(unsigned k,unsigned role,unsigned patch)
{
    amb_layer_t *l=&amb.layer[k];
    amb_layer_t next={0};
    next.mode=role==0?3:2;next.speed=1;next.density=35;next.follow=100;
    unsigned donor=0;
    while(donor<4 && amb_reset_layer[donor].role!=role)donor++;
    if(donor<4)next=amb_reset_layer[donor];
    amb_fill_patch(&next,amb.key,k,role,role==6?0:patch);
    if(role==6)next.base[P_E0]=patch;
    amb_landscape_apply(k,&next);
    l->generated=1;l->speed=next.speed;l->follow=next.follow;
    l->swing=next.swing;l->fill=next.fill;l->mutation=next.mutation;
    amb_base_revision[k]++;
    ambr.recent_valid[k]=0;travel.previous_sound[k]=0;
    travel.pending_layers&=~(1u<<k);amb_travel_rebase(k);
}
/* One role-lane event, targeted at a particular seat in the ensemble. */
static unsigned amb_role_options(unsigned k)
{
    unsigned used=1u<<amb.layer[k].role,foundation=0;
    for(unsigned j=0;j<4;j++)if(j!=k){
        used|=1u<<amb.layer[j].role;
        if(!amb.layer[j].base[P_MUTE] && amb.layer[j].role<2)foundation=1;
    }
    return (foundation?255u:3u)&~used;
}
static int amb_role_proposal(unsigned k,unsigned dice,uint8_t *role,uint8_t *patch)
{
    if(amb.layer[k].role==6 && amb.layer[k].base[P_E0]==AMB_MIC_SOURCE)return 0;
    unsigned options=amb_role_options(k);
    for(unsigned n=0;n<8;n++){
        unsigned r=(dice+n)&7;
        if(!(options&(1u<<r)))continue;
        *role=r;*patch=r==6?(dice>>16)%13:(dice>>8)%amb_patch_count(r);
        return 1;
    }
    return 0;
}
static void amb_change_role(void)
{
    unsigned serial=lane_clock[1].serial,dice=amb_hash(amb.policy.seed+serial);
    if(ambr.solo>=0)return;
    for(unsigned i=0;i<4;i++){
        unsigned k=(serial+i)&3;
        amb_layer_t *l=&amb.layer[k];
        if(((amb_travel_guard()|role_pending)&(1u<<k)) || l->base[P_MUTE] || ambr.handoff[k] || ambr.fade_slot[k] ||
           (l->role==6 && l->base[P_E0]==AMB_MIC_SOURCE))continue;
        uint8_t role,patch;
        if(amb_role_proposal(k,dice,&role,&patch)){
            unsigned before=l->role;amb_replace_role(k,role,patch);amb_auto_event(AUTO_ROLE,k,before,role);return;
        }
    }
}
static void amb_pattern_tick(void)
{
    pattern_pending=0;
    if(amb_lane_value(0,0)==2){
        if(ambr.generate_queued)return; /* A manual REC has priority. */
        uint8_t choices[NELEM(amb_generate_recipes)];
        unsigned count=0;
        for(unsigned i=0;i<NELEM(amb_generate_recipes);i++)
            if(memcmp(amb.policy.active,amb_generate_recipes[i].values,3))choices[count++]=i+1;
        ambr.generate_queued=choices[amb_hash(amb.policy.seed+lane_clock[0].serial)%count];
        unsigned preset=ambr.generate_queued-1;amb_commit_generate();amb_auto_event(AUTO_PRESET,0,255,preset);
        return;
    }
    unsigned f=(lane_clock[0].serial-1)%3;
    if(amb.policy.pending[f]==255){
        unsigned previous=amb.policy.active[f];amb_policy_choose(&amb.policy,f,(amb.policy.active[f]+1)%amb_style_count(f),0);
        amb_auto_event(AUTO_RHYTHM+f,0,previous,amb.policy.active[f]);
    }
}
static void amb_ensemble_downbeat(void)
{
    if(amb_lane_due(0) || pattern_pending)amb_pattern_tick();
    if(amb_lane_due(1) || role_pending){amb_change_role();role_pending=0;}
}

/* A style switch establishes a local starting arrangement, even mid-song. */
static void amb_journey_recentre(void)
{
    ambr.loading=1;
    memset(&travel,0,sizeof travel);role_pending=pattern_pending=0;
    ambr.loading=0;
}

static unsigned amb_chapter_interval(unsigned style){return style==1?4:3;}
static unsigned amb_chapter_layers(unsigned style,unsigned chapter,uint8_t *anchor)
{
    if(style==1)return 1u<<((amb.policy.seed+chapter)&3);
    unsigned pick=amb_hash(amb.policy.seed+chapter);
    *anchor=(*anchor+1+pick%3)&3;
    return 15u^(1u<<*anchor);
}
static void amb_journey_phrase(unsigned style,int movement)
{
    unsigned total=travel.distance+(unsigned)(movement<0?-movement:movement);
    travel.direction=movement<0?-1:1;
    unsigned interval=amb_chapter_interval(style);
    travel.distance=total%interval;
    if(total<interval)return;
    travel.chapter+=total/interval;
    travel.pending_layers=amb_chapter_layers(style,travel.chapter,&travel.anchor);
}
static unsigned amb_chapter_sound(unsigned k,unsigned role,int direction,unsigned chapter,unsigned current,unsigned previous)
{
    static const uint8_t voices[8][2][4]={
        {{5,15,20,9},{18,22,17,8}},{{3,13,6,8},{9,12,5,2}},
        {{8,5,2,7},{13,14,11,10}},{{7,6,0,3},{1,10,8,9}},
        {{5,4,6,0},{2,8,7,1}},{{10,7,5,3},{12,15,2,1}},{{3,1,5,11},{4,6,0,8}},{{0,2,3,5},{1,4,2,0}}
    };
    const uint8_t *choices=voices[role][direction>0];
    unsigned index=amb_hash(amb.policy.seed+chapter*37+k)&3;
    while(choices[index]==current || choices[index]+1==previous)index=(index+1)&3;
    return choices[index];
}
static void amb_journey_orchestrate(int playing)
{
    for(unsigned j=0;j<4;j++){
        unsigned k=(j+travel.chapter)&3;
        if(!(travel.pending_layers&(1u<<k)) || ambr.handoff[k] || ambr.fade_slot[k])continue;
        const amb_layer_t *layer=&amb.layer[k];
        unsigned current=layer->role==6?layer->base[P_E0]:layer->patch;
        unsigned choice=amb_chapter_sound(k,layer->role,travel.direction,travel.chapter,current,travel.previous_sound[k]);
        travel.previous_sound[k]=current+1;
        amb_patch(k,layer->role,choice);
        if(amb_notices.travel)amb_auto_event(AUTO_SOUND,k,current,amb_sound_index(k));
        travel.pending_layers&=~(1u<<k);
        if(playing)break;
    }
}
static unsigned amb_stage_sound(unsigned k,unsigned role,int stage,unsigned original)
{
    /* Orchestration changes only the sound. The sequence stays with its layer. */
    static const uint8_t palette[8][2][2]={
        {{5,20},{18,22}}, {{3,13},{9,12}}, {{8,12},{13,14}},
        {{7,6},{1,10}}, {{5,6},{2,8}}, {{10,21},{12,15}}, {{3,1},{4,6}},{{0,3},{1,4}}
    };
    int magnitude=stage<0?-stage:stage;
    const uint8_t *choices=palette[role][stage>0];
    unsigned choice=(amb_hash(amb.policy.seed+19*k)+magnitude-1)&1;
    if(magnitude==1 && choices[choice]==original)choice^=1;
    return choices[choice];
}
static int amb_landscape_instrument(unsigned k,int position)
{
    amb_layer_t *l=&amb.layer[k];
    int stage=amb_travel_stage(k,position);
    if(stage==travel.stage[k] || ambr.handoff[k] || ambr.fade_slot[k])return 0;
    if(!travel.stage[k]){
        travel.origin[k]=*l;
        for(unsigned j=0;j<4;j++)travel.origin[k].macro[j]=clamp(travel.origin_macro[k][j],0,127);
    }
    unsigned before=amb_sound_index(k);
    const amb_layer_t *saved=&travel.origin[k];
    if(stage){
        unsigned original=l->role==6?saved->base[P_E0]:saved->patch;
        amb_patch(k,l->role,amb_stage_sound(k,l->role,stage,original));
    }else{
        amb_landscape_apply(k,saved);
        for(unsigned j=0;j<4;j++)l->macro[j]=clamp(saved->macro[j]+amb_colour_offset(l->role,j,travel.colour),0,127);
    }
    if(amb_notices.travel)amb_auto_event(AUTO_SOUND,k,before,amb_sound_index(k));
    travel.stage[k]=stage;
    return 1;
}
static int amb_landscape_bar(void)
{
    int position=travel.wanted;
    unsigned style=amb_journey_style();
    travel.guarded_layers|=amb_travel_guard();
    travel.pending_layers&=~travel.guarded_layers;
    if(style)amb_journey_orchestrate(song.playing);
    else{
        for(unsigned k=0;k<4;k++)if(travel.guarded_layers&(1u<<k))amb_travel_rebase(k);
        for(unsigned k=0;k<4;k++)if(amb_landscape_instrument(k,position) && song.playing)break;
    }
    if(position==travel.harmony)return 0;
    int scale[12],count=amb_scale(amb_pool(),scale);
    unsigned old_chord=amb.chord;
    if(style)amb.chord=amb_walk_next(&travel.walk,amb.policy.seed+travel.chapter*31+(uint32_t)position,amb.chord,count);
    else if(position>=-18 && position<=18)amb.chord=travel.chord[AMB_TRAVEL_RANGE+position]%count;
    else amb.chord=(amb_hash(amb.policy.seed+(uint32_t)position/4u)+(uint32_t)position)%count;
    amb_revoice();travel.harmony=position;
    if(amb_notices.travel)amb_auto_event(AUTO_CHORD,0,old_chord,amb.chord);
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
    if(travel.wanted!=previous){
        // Discard protected changes even if the edit or lock ends before the downbeat.
        travel.pending_layers&=~amb_travel_guard();
        travel.guarded_layers=(uint8_t)amb_travel_guard();
        for(unsigned k=0;k<4;k++)if(travel.guarded_layers&(1u<<k))amb_travel_rebase(k);
    }
    if(amb_journey_style() && delta && travel.wanted!=previous)
        amb_journey_phrase(amb_journey_style(),amb_endless()?delta:travel.wanted-previous);
    travel.pending_layers&=~travel.guarded_layers;
    if(!song.playing){amb_landscape_colour(1);amb_landscape_bar();}
    ambr.dirty=1;
    ambr.loading=0;
}

static void amb_follow_travel_tick(void)
{
    if(amb_vary_patterns() && amb_lane_value(0,1)==LANE_FOLLOW && !pattern_pending){
        lane_clock[0].serial++;pattern_pending=1;
    }
    if(amb_lane_value(1,0) && amb_lane_value(1,1)==LANE_FOLLOW && !role_pending){
        lane_clock[1].serial++;role_pending=(uint8_t)(0x80u|amb_travel_guard());
    }
}

/* Predict the next orchestration event that belongs to this seat. */
static void amb_evolution_preview(void)
{
    unsigned k=ambr.evolve_layer;
    amb_layer_t *l=&amb.layer[k];
    unsigned current=l->role==6?l->base[P_E0]:l->patch;
    unsigned style=amb_journey_style();
    ambr.evolve_role=l->role;
    if(!style){
        ambr.evolve_patch=amb_stage_sound(k,l->role,ambr.evolve_direction,current);
        return;
    }
    unsigned chapter=ambr.evolve_chapter;
    uint8_t anchor=ambr.evolve_anchor;
    do{chapter++;}while(!(amb_chapter_layers(style,chapter,&anchor)&(1u<<k)));
    ambr.evolve_patch=amb_chapter_sound(k,l->role,ambr.evolve_direction,chapter,current,ambr.evolve_previous);
    ambr.evolve_destination=chapter;
}
static void amb_commit_evolution(void)
{
    if(!ambr.evolve_queued)return;
    unsigned k=ambr.evolve_layer,role=ambr.evolve_role,patch=ambr.evolve_patch;
    amb_layer_t *l=&amb.layer[k];
    if(ambr.evolve_mode){
        if(role<8 && (amb_role_options(k)&(1u<<role))){
            travel.focused_role[k]++;
            amb_replace_role(k,role,patch);
        }
        else amb_say("Ensemble changed / try again");
    }else{
        travel.focused_moves[k]++;
        travel.previous_sound[k]=(role==6?l->base[P_E0]:l->patch)+1;
        amb_patch(k,role,patch);
        if(amb_journey_style())travel.focused_chapter[k]=ambr.evolve_destination;
        amb_travel_rebase(k);
    }
    ambr.evolve_open=ambr.evolve_queued=0;ambr.dirty=1;
}
