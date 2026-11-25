/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef AMB_SOUND_VARIATIONS_H
#define AMB_SOUND_VARIATIONS_H
/* Each row is a patch's performance menu. Articulations are shared where
 * they make musical sense. They are voiced from that patch's own engine
 * program and envelope, so an electric tine and a brass oscillator respond
 * to the same performance direction through different synthesis controls. */
enum {
    V_ORIGINAL,V_VEIL,V_BLOOM,V_CLOSE,V_REED,V_CHIME,V_VELVET,V_DUB,
    V_PEDAL,V_PLUCK,V_BRASS,V_SLIDE,V_BRUSH,V_FLOOR,V_SPARK,V_WASH,V_RADIO
};
static const struct {
    const char *name;
    int8_t attack,release,tone,body,space,echo;
} amb_voicing[]={
    {"Original",0,0,0,0,0,0},
    {"Veil",18,6,-39,-19,6,-23},
    {"Slow bloom",16,22,6,14,28,7},
    {"Close mic",-38,-24,12,-13,-65,-52},
    {"Reed edge",-29,-13,15,38,-31,-21},
    {"Silver tine",-23,4,33,29,-8,-19},
    {"Felt",-7,-36,-41,-26,-36,-33},
    {"Dub send",-32,-24,-12,9,-21,48},
    {"Low pedal",-12,17,-42,-16,-40,-31},
    {"Palm mute",-45,-42,-21,-33,-54,-45},
    {"Open brass",-27,3,34,21,-18,-18},
    {"Liquid acid",-5,-17,12,29,-23,32},
    {"Brushwork",-12,-20,-30,-19,-42,-26},
    {"Floor tom",-11,18,-28,32,15,-26},
    {"Pinpoints",-29,-33,31,-26,-34,-15},
    {"Far bank",10,16,-34,-14,33,9},
    {"Intercom",-23,-12,23,21,-43,-40}
};
typedef uint8_t amb_voicing_menu_t[3];
/* Patch order follows amb_palette.h; SAMPLE uses the persisted source ID. */
static const amb_voicing_menu_t amb_pad_voicings[]={
    {V_VEIL,V_BRASS},{V_BLOOM,V_REED},{V_VEIL,V_BRASS},{V_CHIME,V_BLOOM},
    {V_CLOSE,V_PEDAL},{V_PEDAL,V_BLOOM},{V_DUB,V_VEIL},{V_CLOSE,V_PLUCK},
    {V_REED,V_WASH},{V_CLOSE,V_BLOOM},{V_VEIL,V_REED},{V_CLOSE,V_BRASS},
    {V_CLOSE,V_DUB},{V_BRASS,V_VEIL},{V_PEDAL,V_DUB},{V_PLUCK,V_BLOOM},
    {V_VELVET,V_CHIME,V_DUB},{V_CLOSE,V_DUB},{V_CHIME,V_BLOOM},
    {V_CLOSE,V_BLOOM},{V_CLOSE,V_WASH},{V_REED,V_WASH},{V_CLOSE,V_BLOOM},
    {V_REED,V_WASH},{V_VELVET,V_DUB},{V_VEIL,V_BRASS},{V_VELVET,V_BLOOM},
    {V_PLUCK,V_WASH}
};
static const amb_voicing_menu_t amb_drone_voicings[]={
    {V_PEDAL,V_BRASS},{V_VEIL,V_BLOOM},{V_REED,V_WASH},{V_CLOSE,V_BLOOM},
    {V_PEDAL,V_DUB},{V_VEIL,V_REED},{V_REED,V_WASH},{V_PEDAL,V_BRASS},
    {V_VEIL,V_REED},{V_CLOSE,V_WASH},{V_REED,V_BLOOM},{V_PLUCK,V_WASH},
    {V_CHIME,V_VEIL},{V_CLOSE,V_BLOOM}
};
static const amb_voicing_menu_t amb_bell_voicings[]={
    {V_VELVET,V_CHIME},{V_CLOSE,V_WASH},{V_PLUCK,V_DUB},{V_VELVET,V_BLOOM},
    {V_CHIME,V_DUB},{V_SPARK,V_WASH},{V_PLUCK,V_BLOOM},{V_CHIME,V_WASH},
    {V_BLOOM,V_CLOSE},{V_PLUCK,V_CHIME},{V_SPARK,V_BLOOM},
    {V_VELVET,V_CHIME,V_BLOOM},{V_CHIME,V_WASH},{V_SPARK,V_DUB},{V_VELVET,V_WASH},
    {V_VELVET,V_CHIME},{V_REED,V_DUB},{V_CHIME,V_VELVET},{V_PLUCK,V_DUB},
    {V_CLOSE,V_BLOOM},{V_CHIME,V_WASH}
};
static const amb_voicing_menu_t amb_bass_voicings[]={
    {V_PEDAL,V_PLUCK},{V_PLUCK,V_DUB},{V_PEDAL,V_REED},{V_CLOSE,V_DUB},
    {V_PEDAL,V_PLUCK},{V_CLOSE,V_DUB},{V_VELVET,V_DUB},{V_PEDAL,V_BRASS},
    {V_REED,V_PLUCK},{V_PEDAL,V_CHIME},{V_PLUCK,V_REED},{V_PLUCK,V_SLIDE,V_DUB},
    {V_CLOSE,V_DUB},{V_CLOSE,V_PEDAL},{V_PEDAL,V_BRASS},{V_PLUCK,V_DUB},
    {V_CHIME,V_PEDAL},{V_REED,V_PEDAL},{V_PEDAL,V_PLUCK},{V_PLUCK,V_SLIDE}
};
static const amb_voicing_menu_t amb_drum_voicings[]={
    {V_BRUSH,V_FLOOR},{V_PLUCK,V_DUB},{V_BRUSH,V_SPARK},{V_SPARK,V_DUB},
    {V_FLOOR,V_WASH},{V_FLOOR,V_DUB},{V_BRUSH,V_PLUCK},{V_SPARK,V_WASH},
    {V_BRUSH,V_FLOOR,V_DUB}
};
static const amb_voicing_menu_t amb_sample_voicings[]={
    {V_CHIME,V_DUB},{V_CLOSE,V_BLOOM},{V_CLOSE,V_BLOOM},{V_CLOSE,V_WASH},
    {V_VELVET,V_WASH},{V_CLOSE,V_DUB},{V_CLOSE,V_BLOOM},{V_BRUSH,V_WASH},
    {V_BRUSH,V_SPARK},{V_CLOSE,V_DUB},{V_SPARK,V_WASH},{V_PLUCK,V_DUB},
    {V_VEIL,V_BLOOM},{V_RADIO,V_WASH},{V_CLOSE,V_DUB},{V_RADIO,V_WASH},
    {V_CLOSE,V_WASH},{V_CLOSE,V_WASH},{V_RADIO,V_DUB}
};
static const amb_voicing_menu_t amb_lead_voicings[]={
    {V_CLOSE,V_DUB},{V_PLUCK,V_SLIDE,V_DUB},{V_REED,V_BRASS},
    {V_CLOSE,V_WASH},{V_CHIME,V_DUB},{V_PLUCK,V_BRASS},{V_CLOSE,V_BLOOM},
    {V_REED,V_DUB},{V_CHIME,V_WASH},{V_BRASS,V_DUB},{V_CHIME,V_VEIL},{V_REED,V_DUB}
};
static const amb_voicing_menu_t amb_texture_voicings[]={
    [17]={V_CLOSE,V_BLOOM},[18]={V_PEDAL,V_WASH},[22]={V_RADIO,V_WASH},
    [23]={V_CLOSE,V_DUB},[24]={V_RADIO,V_WASH},[25]={V_CLOSE,V_WASH},
    [26]={V_RADIO,V_BLOOM},[28]={V_CLOSE,V_WASH},[29]={V_RADIO,V_DUB}
};
#define AMB_CHECK_MENU(role) _Static_assert(NELEM(amb_##role##_voicings)==NELEM(amb_##role##_patches), "Every patch has a voicing menu")
AMB_CHECK_MENU(pad);AMB_CHECK_MENU(drone);_Static_assert(NELEM(amb_bell_voicings)==NELEM(amb_bells_patches),"Bells menus");
AMB_CHECK_MENU(bass);AMB_CHECK_MENU(lead);AMB_CHECK_MENU(texture);
_Static_assert(NELEM(amb_drum_voicings)==NELEM(amb_drums_patches),"Drum menus");
_Static_assert(NELEM(amb_sample_voicings)==NELEM(N_AS_SOURCE),"Sample menus");
#undef AMB_CHECK_MENU
static const uint8_t *amb_variation_menu(const amb_layer_t *l)
{
    static const amb_voicing_menu_t *const menus[]={
        amb_pad_voicings,amb_drone_voicings,amb_bell_voicings,amb_bass_voicings,
        amb_drum_voicings,amb_texture_voicings,amb_sample_voicings,amb_lead_voicings
    };
    return menus[l->role][l->role==6?l->base[P_E0]:l->patch];
}
static unsigned amb_variation_count(const amb_layer_t *l)
{
    const uint8_t *menu=amb_variation_menu(l);
    return menu[2]?4:menu[0]?3:1;
}
static unsigned amb_variation_id(const amb_layer_t *l,unsigned index)
{
    return index && index<amb_variation_count(l)?amb_variation_menu(l)[index-1]:V_ORIGINAL;
}
static const char *amb_variation_name(const amb_layer_t *l,unsigned index)
{return amb_voicing[amb_variation_id(l,index)].name;}
static void amb_voice_offset(amb_layer_t *l,const amb_layer_t *original,unsigned id,int delta)
{
    const param_desc_t *d=amb_param_desc(l,id);
    l->base[id]=clamp(original->base[id]+delta,d->min,d->max);
}
static void amb_apply_variation(unsigned k,unsigned index)
{
    amb_layer_t *l=&amb.layer[k];
    if(index>=amb_variation_count(l))return;
    const unsigned id=amb_variation_id(l,index);
    const int tone=amb_voicing[id].tone,body=amb_voicing[id].body;
    amb_layer_t original={0};
    amb_fill_patch(&original,amb.key,k,l->role,l->patch);
    if(amb_legacy_glass(l)){
        original.engine=l->engine;
        const preset_t *legacy=&ENGINES[l->engine]->presets[amb_patch_preset(l)];
        for(unsigned j=0;j<8;j++)original.base[P_E0+j]=legacy->e[j];
    }
    /* Only write fields this menu voices. Keep custom source, sample trim,
     * gain, notes, macros, modulation routes and the layer insert chain. */
    unsigned colour=P_FCUT,shape=P_FRES;
    int colour_change=tone,shape_change=body;
    switch(l->engine){
    case ENGI_ANALOG: colour=P_E4;shape=P_E5;break;
    case ENGI_PHASE: colour=P_E2;shape=P_E3;break;
    case ENGI_FM6: colour=P_E2;shape=P_E4;colour_change=tone/2;shape_change=body/2;break;
    case ENGI_SINEFM: colour=P_E1;shape=P_E2;break;
    case ENGI_VOICE: colour=P_E4;shape=P_E5;break;
    case ENGI_WHEEL: colour=P_E3;shape=P_E5;colour_change=tone/5;break;
    case ENGI_NOISE: colour=P_E1;shape=P_E2;break;
    case ENGI_DRUM:
    case ENGI_SOFTPULSE: colour=P_E2;shape=P_E3;break;
    case ENGI_LOFI: colour=P_FCUT;shape=P_DIST;shape_change=body/2;break;
    case ENGI_AMB_SAMPLE: colour=P_E7;shape=P_FCUT;break;
    case ENGI_AMB_A22:
        if(l->base[P_E0]<8){colour=P_E2;shape=P_E1;}
        else {colour=P_E3;shape=P_E4;}
        break;
    }
    amb_voice_offset(l,&original,colour,colour_change);
    amb_voice_offset(l,&original,shape,shape_change);
    amb_voice_offset(l,&original,P_ATK,amb_voicing[id].attack);
    amb_voice_offset(l,&original,P_REL,amb_voicing[id].release);
    amb_voice_offset(l,&original,P_REV,amb_voicing[id].space);
    amb_voice_offset(l,&original,P_DLY,amb_voicing[id].echo);
    /* A field take changes perspective markedly when the tape runs slower.
     * Keep the recording, loop and owner's trim points intact. */
    if(l->role==5 && (l->patch==22 || l->patch==23 || l->patch>=28)){
        int speed=id==V_RADIO?4:id==V_CLOSE?-5:0;
        if(l->patch>=28 && (id==V_DUB || id==V_WASH))speed=-9;
        if(l->patch==28 && id==V_CLOSE)speed=4;
        amb_voice_offset(l,&original,P_E2,speed);
    }
    if(amb_acid(l) || (l->engine==ENGI_AMB_A22 && l->base[P_E0]==13)){
        int liquid=id==V_SLIDE,shortened=id==V_PLUCK;
        amb_voice_offset(l,&original,P_GLIDE,liquid?22:shortened?-48:0);
        amb_voice_offset(l,&original,P_DEC,liquid?11:shortened?-29:0);
        if(amb_acid(l))amb_voice_offset(l,&original,P_ED_FLT,liquid?4:shortened?-21:0);
    }
}
#endif
