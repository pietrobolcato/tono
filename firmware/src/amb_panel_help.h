/* SPDX-License-Identifier: GPL-3.0-only */
#include "amb_help_data.h"
/* Each engine group follows its eight physical parameter slots. */
static unsigned amb_param_hint(const amb_layer_t *l,unsigned id)
{
    static const uint8_t engines[]={
        [0]=H_oscillator_waveform, [2]=H_first_phase_waveform, [3]=H_chip_model, [5]=H_starting_vowel_a_e_i_o_u, [7]=H_drawbar_registration, [10]=H_morph_the_drum_kit, [11]=H_noise_generator, [12]=H_operator_routing, [15]=H_bundled_audio_source, [16]=H_modulator_pitch_ratio, [17]=H_metal_in_the_hats
    };
    if(l->role==4 && id>=P_LN0 && id<=P_LN7)return H_level_of_this_drum_voice;
    if(l->engine==3 && id>=P_E0){
        const param_desc_t *d=amb_param_desc(l,id);
        if(d==&LOFI_MOTN)return H_motion_of_the_formula;
        if(d==&LOFI_FDCY)return H_feed_back_formula_output;
        if(d==&LOFI_LOOP)return H_loop_the_formula_clock;
        if(d==&LOFI_VAR)return H_vary_the_formula;
        if(d==&LOFI_FALGO || d==&LOFI_ALGO)return H_choose_a_sound_formula;
    }
    if(l->engine==ENGI_AMB_A22 && id>=P_E0){

        return id>=P_E1 && id<=P_E4?(l->base[P_E0]>=8?H_oscillator_shape:H_strength_of_the_attack)+id-P_E1:0;
    }
    if(id>=P_E0)return engines[l->engine]+id-P_E0;
    switch(id){
    case P_FTYPE:return H_filter_response_shape;
    case P_FCUT:return H_filter_cutoff;
    case P_FRES:return H_emphasize_cutoff;
    case P_DIST:return H_drive_intensity;
    case P_DTYPE:return H_drive_circuit;
    case P_DTONE:return H_drive_brightness;
    case P_CHOR:return H_send_to_shared_chorus;
    case P_DLY:return H_send_to_shared_delay;
    case P_REV:return H_send_to_shared_reverb;
    case P_APRATE:return H_auto_pan_speed;
    case P_APDEP:return H_auto_pan_width;
    case P_APWAVE:return H_auto_pan_waveform;
    case P_PAN:return H_stereo_position;
    case P_VOICE:return H_poly_mono_or_legato;
    case P_GLIDE:return H_time_to_slide_to_a_note;
    case P_TRANS:return H_transpose_the_layer;
    case P_LN7:return H_playback_gain;
    case P_LN6:return H_sample_edge_softness;
    default:return 0;
    }
}
static unsigned amb_macro_card(const amb_layer_t *l,unsigned knob)
{
    static const uint8_t macro[][4]={
        {H_drive_attack_and_tone,H_chorus_and_filter_motion,H_hold_notes_raise_sustain,H_room_and_echo_sends},
        {H_drive_attack_and_tone,H_chorus_and_filter_motion,H_hold_notes_raise_sustain,H_room_and_echo_sends},
        {H_round_off_the_attack_tone,H_chorus_and_filter_motion,H_lengthen_the_ring,H_room_and_echo_sends},
        {H_darken_the_bass_tone,H_chorus_and_filter_motion,H_portamento_and_release,H_room_and_echo_sends},
        {H_soften_hit_brightness,H_more_generated_hits,H_noise_in_the_hit,H_room_and_echo_sends},
        {H_darken_the_noise,H_chorus_and_filter_motion,H_noise_drift_and_release,H_room_and_echo_sends},
        {H_transpose_the_sample,H_smooth_loop_boundaries,H_stereo_movement,H_room_and_echo_sends}
    };
    if(!knob && l->engine==ENGI_VOICE)return H_shift_the_starting_vowel;
    if(amb_acid(l))return H_acid+knob;
    return macro[l->role==7?3:l->role][knob];
}
static unsigned amb_knob_card(const amb_layer_t *l,unsigned knob)
{
    static const uint16_t manual[]={H_select_an_edit_slot,H_layer_step_interval,H_pitch_of_this_slot,H_strength_of_this_slot};
    static const uint16_t manual_drum[]={H_select_an_edit_slot,H_choose_a_drum_voice,H_enable_this_slot,H_delay_offbeat_hits};
    if(ambr.view==4)return H_layer_output_level;
    if(ambr.view==0 || (ambr.view==1 && ambr.page==0)){
        if(!ambr.view){
            if(knob==1 && !amb_home_echo())
                return ambr.target?H_fewer_notes_home_macro:H_fewer_notes_home_echo;
            if(!ambr.target)return H_room_size_sends_width+knob;
            l=&amb.layer[ambr.target-1];
        }
        return amb_macro_card(l,knob);
    }
    if(ambr.view==2 && ambr.page==7)return (timing_tab?H_enable_this_lane:H_count_bars_or_seconds)+knob;
    if(ambr.view==2 && ambr.page==6)return H_bounded_or_endless_travel+knob;
    if(ambr.view==2)return ambr.page<3?H_chance_of_a_chord_change+ambr.page*4+knob:ambr.page==3?H_reverb_room_size+(ambr.fx%4)*4+knob:H_low_band_gain+knob;
    if(ambr.page==1)return amb_param_hint(l,amb_sound_id(l,ambr.bank,knob));
    if(ambr.page==2)return l->role==4 || (l->role==6 && l->base[P_E1]==3)?amb_param_hint(l,amb_envelope_id(l,knob)):H_time_to_full_level+knob;
    if(ambr.page==3){
        int rate=l->base[P_LRATE],random=rate>=AMB_RANDOM_BARS;
        if(ambr.bank && (knob==3 || (!random && knob==2)))return 0;
        if(amb_bar_rate(rate) && ambr.bank && knob==1)return H_shortest_cycle_in_bars;
        if(amb_bar_rate(rate) && ambr.bank && knob==2)return H_longest_cycle_in_bars;
        if(ambr.bank && knob==0)return H_fixed_or_random_per_cycle;
        if(random && knob==1)return ambr.bank?H_lowest_cycle_frequency:H_shift_both_rate_limits;
        if(random && ambr.bank && knob==2)return H_highest_cycle_frequency;
        return H_modulation_waveform+knob;
    }
    if(ambr.page==4){
        unsigned id=amb_pattern_param(l,knob);
        if(id)return amb_param_hint(l,id);
        if(!knob && l->generated && l->role==5 && !amb_legacy_texture(l))return H_texture_motion;
        if(!l->generated && amb_mono_role(l) && !knob)return H_slide_into_selected_step;
        if(!l->generated)return (l->role==4?manual_drum:manual)[knob];
        return (l->role==4?H_generated_hit_density:H_note_selection_pattern)+knob;
    }
    if(ambr.page==5 && ambr.fx==2 && knob==3)return H_shared_delay_algorithm;
    if(ambr.page==5 && ambr.fx==5)return H_grain+knob;
    return ambr.page==5?amb_param_hint(l,amb_fx_id(l,knob)):H_low_band_gain+knob;
}

/* The target uses the same description as its edit page. */
static unsigned amb_route_card(const amb_layer_t *l)
{
    unsigned r=l->lfo_dest;
    static const uint16_t basic[]={H_filter_cutoff,H_basic_pitch,H_stereo_position,H_layer_output_level};
    static const uint16_t pattern[]={H_note_selection_pattern,H_layer_step_interval,H_generated_event_density,H_chance_to_follow_chord,H_delay_offbeat_hits,H_phrase_end_fill_strength,H_vary_drum_lane_choices,H_pitch_of_this_slot,H_strength_of_this_slot};
    if(r<4)return basic[r];
    if(r<ROUTE_MACRO){
        unsigned id=amb_route_param[r-4];
        if(id>=P_ATK && id<=P_REL)return H_time_to_full_level+id-P_ATK;
        if(id==P_LRATE || id==P_LWAVE)return id==P_LRATE?H_cycles_per_second:H_modulation_waveform;
        return amb_param_hint(l,id);
    }
    if(r<ROUTE_EQ)return amb_macro_card(l,r-ROUTE_MACRO);
    if(r<ROUTE_MODE)return H_low_band_gain+r-ROUTE_EQ;
    if(r<ROUTE_GRAIN)return pattern[r-ROUTE_MODE];
    return r==ROUTE_HOME_DENSITY?H_fewer_notes_home_macro:H_grain+r-ROUTE_GRAIN;
}

typedef struct { unsigned bit; } amb_help_reader_t;

static unsigned amb_help_bits(amb_help_reader_t *r,unsigned count)
{
    unsigned value=0;
    while(count--){
        unsigned i=r->bit++;
        value=(value<<1)|((help_data[i/8]>>(7-(i&7)))&1);
    }
    return value;
}
static unsigned amb_help_symbol(amb_help_reader_t *r,const uint8_t *table)
{
    const uint8_t *symbols=table+1+*table;
    unsigned code=0;
    for(;;){
        unsigned count=*++table;
        code=2*code+amb_help_bits(r,1);
        if(code<count)return symbols[code];
        symbols+=count;code-=count;
    }
}

/* The previous canvas blit can be asynchronous. Once it is done the canvas
 * is disposable until cv_begin clears it. Decode there and copy out the two
 * small cards before drawing. No heap, extra BSS or audio buffer is needed. */
static void amb_help_read(char *overview,char *hint,unsigned first,unsigned second)
{
    _Static_assert(HELP_RAW_SIZE<=sizeof cv_px,"Help must fit the idle canvas");
    lcd_sync();
    uint8_t *text=(uint8_t *)cv_px;
    amb_help_reader_t r={0};
    for(unsigned at=0;at<HELP_RAW_SIZE;){
        unsigned token=amb_help_symbol(&r,help_literal);
        if(token<128){text[at++]=(uint8_t)token;continue;}
        unsigned n=token-128+3;
        if(token>=136){
            unsigned bits=token-133;
            n=3+(1u<<bits)+amb_help_bits(&r,bits);
        }
        unsigned bits=amb_help_symbol(&r,help_distance);
        unsigned distance=(1u<<bits)+amb_help_bits(&r,bits);
        for(unsigned k=0;k<n;k++,at++)text[at]=text[at-distance];
    }
    for(unsigned k=0;k<2;k++){
        unsigned id=k?second:first;
        const char *p=(const char *)text+help_blocks[id/8];
        for(unsigned skip=id%8;skip;skip--)while(*p++){}
        str_cpy(k?hint:overview,p,HELP_TEXT_CAP);
    }
}

static void amb_help_paragraph(int y,char *text)
{
    char *line=text;
    while(*line){
        char *cut=line,*next=line;
        do{
            while(*next && *next!=' ')next++;
            char saved=*next;*next=0;
            int fits=text_w(&AF_TONO_LABEL,line)<=214;
            *next=saved;
            if(!fits)break;
            cut=next;
            if(!saved)break;
            next++;
        }while(1);
        char saved=*cut;*cut=0;
        cv_text(12,y+1-AF_TONO_LABEL_CAP_Y,&AF_TONO_LABEL,line,AINK);
        if(!saved)break;
        line=cut+1;y+=14;
    }
}
