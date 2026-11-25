/* SPDX-License-Identifier: GPL-3.0-only */
/* All page presets are absolute settings. Only SOUND derives its offsets
 * from the chosen patch, so repeating a preset never accumulates edits. */
typedef struct {const char *name[3]; int8_t values[3][4];} amb_page_bank_t;
#include "amb_sound_variations.h"

static unsigned amb_family_size(const amb_layer_t *l)
{return l->role==5?amb_texture_count(l->patch):1;}
static unsigned amb_sound_presets(const amb_layer_t *l)
{
    unsigned family=amb_family_size(l);
    return family>1?family:amb_variation_count(l);
}
static const char *amb_sound_preset_name(const amb_layer_t *l,unsigned index)
{
    if(amb_family_size(l)>1)return amb_recipe(5,amb_texture_member(l->patch,index))->name;
    return amb_variation_name(l,index);
}
static const amb_page_bank_t amb_lfo_bank={{"Slow breathing","Gentle sway","Rhythmic pulse"},
    {{0,8,16},{0,26,26},{1,-7,38}}};
static const amb_page_bank_t amb_env_bank={{"Soft arrival","Plucked","Long bloom"},
    {{65,69,93,80},{2,52,22,48},{88,92,111,112}}};
static const amb_page_bank_t amb_eq_bank={{"Round body","Clear air","Even balance"},
    {{3,0,-2,0},{-2,1,3,0},{0,0,0,0}}};
static const amb_page_bank_t amb_fx_banks[]={
    {{"Velvet edge","Worn speaker","Folded reed"},{{0,16,-10,97},{0,35,-25,74},{2,41,-10,84}}},
    {{"Still water","Slow drift","Wide ripple"},{{12,8,0,0},{29,12,12,0},{47,23,26,0}}},
    {{"One reflection","Soft echoes","Distant repeats"},{{12,112,0,0},{40,92,0,4},{68,74,0,3}}},
    {{"Small room","Open hall","Far away"},{{17,110,0,0},{53,103,0,8},{86,85,0,17}}},
    {{"Centred","Gentle arc","Wide orbit"},{{0,0,10,0},{0,22,16,0},{0,54,12,0}}},
    {{"Soft halo","Cloud of glass","Low mist"},{{13,68,0,12},{32,45,12,30},{25,85,-12,19}}}
};
static const amb_page_bank_t amb_master_banks[]={
    {{"Quiet room","Open plate","Dark chamber"},{{32,90,60,0},{85,104,42,5},{75,65,85,0}}},
    {{"Short reflection","Tape trail","Long echoes"},{{18,4,22,0},{37,8,53,1},{49,12,67,0}}},
    {{"Narrow","Slow stereo","Wide haze"},{{6,12,16,4},{23,17,60,12},{40,22,94,24}}},
    {{"Clear","Mellow","Tape dusk"},{{0,0,66,0},{-8,12,47,5},{-16,20,34,13}}}
};
static const amb_page_bank_t *amb_page_bank(void)
{
    if(ambr.view==2 && ambr.page==3)return ambr.fx%5==4?0:&amb_master_banks[ambr.fx%4];
    if(ambr.view==2 && ambr.page==4)return &amb_eq_bank;
    if(ambr.view!=1)return 0;
    switch(ambr.page){
    case 2:{
        const amb_layer_t *l=&amb.layer[ambr.edit_layer];
        return amb_envelope_id(l,0)==P_ATK?&amb_env_bank:0;
    }
    case 3:return ambr.bank?0:&amb_lfo_bank;
    case 5:return ambr.fx>=6?0:&amb_fx_banks[ambr.fx%6];
    case 6:return &amb_eq_bank;
    default:return 0;
    }
}
static int amb_generate_page(void){return ambr.view==2 && ambr.page==2;}
static unsigned amb_local_count(void)
{
    if(ambr.view==1 && ambr.page==1)return amb_sound_presets(&amb.layer[ambr.edit_layer]);
    return amb_generate_page()?AMB_ALL_GENERATE:amb_page_bank()?3:0;
}
static void amb_apply_page_preset(void)
{
    const amb_page_bank_t *bank=amb_page_bank();
    const int index=ambr.local_preset-1;
    ambr.local_preset=0;
    if(amb_generate_page()){
        if(index>=AMB_FACTORY_GENERATE){
            uint8_t *v=amb_user_recipe((index-AMB_FACTORY_GENERATE)%AMB_USER_GENERATE);
            if(index>=AMB_ALL_GENERATE){
#if FELUCCA_FLASH
                if(!flash_ok){amb_say("UNSUPPORTED FLASH - SAVE REFUSED");return;}
#endif
                if(amb_recipe_used(v) && !ambr.overwrite){
                    ambr.overwrite=1;ambr.local_preset=index+1;ambr.dirty=1;return;
                }
                unsigned q=ambr.generate_queued;
                if(q)memcpy(v,q>AMB_FACTORY_GENERATE?amb_user_queue:amb_generate_recipes[q-1].values,4);
                else{
                    for(unsigned k=0;k<3;k++)v[k]=amb_gen_style(k);
                    v[3]=amb.policy.variation;
                }
                v[3]|=128;settings_save();ambr.overwrite=0;
                amb_say(song.playing?"CAPTURED - WRITES ON STOP":"GENERATOR STORED");return;
            }
            if(!amb_recipe_used(v)){amb_say("EMPTY GENERATOR SLOT");return;}
            memcpy(amb_user_queue,v,4);amb_user_queue[3]&=127;
        }
        memset(ambr.gen_edit,0,sizeof ambr.gen_edit);
        ambr.generate_queued=(uint8_t)(index+1);
        if(!song.playing)amb_commit_generate();
        ambr.dirty=1;
        return;
    }
    unsigned k=ambr.edit_layer;
    amb_layer_t *l=&amb.layer[k];
    if(ambr.view==1 && ambr.page==1){
        if((unsigned)index>=amb_sound_presets(l))return;
        if(amb_family_size(l)>1){
            amb_patch(k,l->role,amb_texture_member(l->patch,index));
            amb_travel_rebase(k);
        }else amb_apply_variation(k,index);
        amb_controls.sound_preset[k]=index+1;
        amb_base_revision[k]++;
        ambr.dirty=1;
        return;
    }
    if(!bank || index<0 || index>=3)return;
    const int8_t *v=bank->values[index];
    if(bank==&amb_lfo_bank){
        if(amb_second_editor()){amb_lfo_settings_t *c=amb_edit_second();c->wave=v[0];c->rate=v[1];c->depth=v[2];}
        else {l->base[P_LWAVE]=v[0];l->base[P_LRATE]=v[1];l->lfo_depth=v[2];}
    }else if(bank==&amb_eq_bank){
        memcpy(ambr.view==2?amb.eq:amb.layer_eq[k],v,4);
    }else if(ambr.view==2){
        for(unsigned i=0;i<4;i++){
            unsigned id=amb_master_fx[ambr.fx%4][i];
            amb.global[id]=clamp(v[i],GP[id].min,GP[id].max);
        }
    }else if(ambr.page==5 && ambr.fx==5){
        memcpy(amb.grain[k],v,4);
    }else for(unsigned i=0;i<4;i++){
        unsigned id=bank==&amb_env_bank?amb_envelope_id(l,i):amb_fx_id(l,i);
        const param_desc_t *d=amb_param_desc(l,id);
        l->base[id]=clamp(v[i],d->min,d->max);
    }
    amb_base_revision[k]++;
    ambr.dirty=1;
}
