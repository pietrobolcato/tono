/* SPDX-License-Identifier: GPL-3.0-only */
/* All page presets are absolute settings. Only SOUND derives its offsets
 * from the chosen patch, so repeating a preset never accumulates edits. */
typedef struct {const char *name[3]; int8_t values[3][4];} amb_page_bank_t;
static const amb_page_bank_t amb_sound_bank={{"Near and intimate","Soft amber","Distant bloom"},
    {{-10,-12,-30,-20},{14,-20,-8,4},{27,9,23,21}}}; /* attack, brightness, room, echo */
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
    if(ambr.view==2 && ambr.page==3)return &amb_master_banks[ambr.fx%4];
    if(ambr.view==2 && ambr.page==4)return &amb_eq_bank;
    if(ambr.view!=1)return 0;
    switch(ambr.page){
    case 1:return &amb_sound_bank;
    case 2:{
        const amb_layer_t *l=&amb.layer[ambr.edit_layer];
        return amb_envelope_id(l,0)==P_ATK?&amb_env_bank:0;
    }
    case 3:return &amb_lfo_bank;
    case 5:return &amb_fx_banks[ambr.fx%6];
    case 6:return &amb_eq_bank;
    default:return 0;
    }
}
static void amb_apply_page_preset(void)
{
    const amb_page_bank_t *bank=amb_page_bank();
    const int index=ambr.local_preset-1;
    ambr.local_preset=0;
    if(!bank || index<0 || index>=3)return;
    const int8_t *v=bank->values[index];
    unsigned k=ambr.edit_layer;
    amb_layer_t *l=&amb.layer[k];
    if(bank==&amb_sound_bank){
        const amb_recipe_t *r=amb_recipe(l->role,l->patch);
        unsigned color=l->engine==0?P_E4:P_FCUT;
        int attack=r->attack, tone=r->color+(color==P_FCUT?25:0), room=r->reverb, echo=r->delay;
        if(l->engine==ENGI_AMB_A22)attack=ENG_AMB_A22.presets[r->preset].env[0];
        if(l->role==6){
            /* Keep the articulation of each recording when changing its space. */
            static const uint8_t sources[][4]={{3,108,50,20},{42,90,55,16},{20,115,46,9},
                {24,111,42,6},{0,120,42,7},{8,116,38,11},{15,123,50,14},
                {90,104,75,18},{78,105,54,14},{61,113,43,23},{72,116,72,26},{64,99,38,20},{99,111,87,37},{7,110,50,21},{5,115,65,35},{8,112,54,16},{5,112,48,20},{13,123,16,0},{5,124,10,0}};
            _Static_assert(NELEM(sources)==NELEM(N_AS_SOURCE),"one variation baseline per source");
            const uint8_t *source=sources[l->base[P_E0]];
            attack=source[0];tone=source[1];room=source[2];echo=source[3];
        }
        l->base[P_ATK]=clamp(attack+v[0],0,amb_param_desc(l,P_ATK)->max);
        l->base[color]=clamp(tone+v[1],0,127);
        l->base[P_REV]=clamp(room+v[2],0,127);
        l->base[P_DLY]=clamp(echo+v[3],0,127);
    }else if(bank==&amb_lfo_bank){
        l->base[P_LWAVE]=v[0];l->base[P_LRATE]=v[1];l->lfo_depth=v[2];
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
