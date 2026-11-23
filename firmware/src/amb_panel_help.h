/* SPDX-License-Identifier: GPL-3.0-only */
/* Short explanations fit the 240-pixel LCD beside each live control label. */
static const char *amb_param_hint(const amb_layer_t *l,unsigned id)
{
    static const char *const engines[][8]={
        [0]={"Oscillator waveform","Spread oscillator tuning","Blend the oscillators","Mix in noise","Filter cutoff","Filter resonance","Filter drive","Second oscillator pitch"},
        [2]={"First phase waveform","Second phase waveform","Phase distortion amount","Envelope to distortion","Spread the two lines","Combine the two lines","Sub oscillator level","Phase feedback"},
        [3]={"Chip model","Wave or formula family","Pulse width or table","Crush or variation","Chip filter cutoff","Vibrato or fine motion","Shape the chip waveform","Bit mask or loop length"},
        [5]={"Starting vowel A E I O U","Vowel to move toward","Time between vowels","Raise or lower formants","Glottal brightness","Air in the voice","Sharpen vowel resonances","Randomize vowel per note"},
        [7]={"Drawbar registration","Lower drawbars","Middle drawbars","Upper drawbars","Percussion harmonic","Key click level","Organ overdrive","Rotary speaker speed"},
        [10]={"Morph the drum kit","Tune the kit","Dark to bright","Length of each hit","Noise in the hit","FM at the attack","Wavefold the hit","Drum saturation"},
        [11]={"Noise generator","Noise brightness","Noise filter frequency","Filter peak","Pitch tracking","Grain density or register","Internal noise movement","Resolution or clock"},
        [12]={"Operator routing","Operator feedback","Modulation amount","Modulator frequency ratio","Modulator envelope time","Velocity to modulation","Spread carrier tuning","FM voice source"},
        [15]={"Bundled audio source","Playback character","Pitch in semitones","Repeat the sample","Start of playback","End of playback","Soften loop joins","Sample brightness"},
        [16]={"Modulator pitch ratio","Sustained FM brightness","Swell (-) or strike (+)","Transient length","Fine tuning in cents","Second harmonic level","Sub octave level","Carrier feedback"},
        [17]={"Metal in the hats","Tune kick and metal","Hat filter frequency","Length of each hit","Noise in the hats","Kick pitch drop","Extra harmonic","Pulse saturation"}
    };
    if(l->role==4 && id>=P_LN0 && id<=P_LN7)return "Level of this drum voice";
    if(l->engine==3 && id>=P_E0){
        const param_desc_t *d=amb_param_desc(l,id);
        if(d==&LOFI_MOTN)return "Motion of the formula";
        if(d==&LOFI_FDCY)return "Feed back formula output";
        if(d==&LOFI_LOOP)return "Loop the formula clock";
        if(d==&LOFI_VAR)return "Vary the formula";
        if(d==&LOFI_FALGO || d==&LOFI_ALGO)return "Choose a sound formula";
    }
    if(l->engine==ENGI_AMB_A22 && id>=P_E0){
        static const char *const shape[2][4]={
            {"Strength of the attack","Bright tine overtones","Tremolo intensity","Round off the top end"},
            {"Oscillator shape","Cross-modulation amount","Internal filter cutoff","Internal filter peak"}
        };
        return id>=P_E1 && id<=P_E4?shape[l->base[P_E0]>=8][id-P_E1]:"";
    }
    if(id>=P_E0)return engines[l->engine][id-P_E0];
    switch(id){
    case P_FTYPE:return "Filter response shape";
    case P_FCUT:return "Filter cutoff";
    case P_FRES:return "Emphasize cutoff";
    case P_DIST:return "Drive intensity";
    case P_DTYPE:return "Drive circuit";
    case P_DTONE:return "Drive brightness";
    case P_CHOR:return "Send to shared chorus";
    case P_DLY:return "Send to shared delay";
    case P_REV:return "Send to shared reverb";
    case P_APRATE:return "Auto-pan speed";
    case P_APDEP:return "Auto-pan width";
    case P_APWAVE:return "Auto-pan waveform";
    case P_PAN:return "Stereo position";
    case P_VOICE:return "Poly, mono or legato";
    case P_GLIDE:return "Time to slide to a note";
    case P_TRANS:return "Transpose the layer";
    case P_LN7:return "Playback gain";
    case P_LN6:return "Sample edge softness";
    default:return "";
    }
}
static const char *amb_knob_hint(const amb_layer_t *l,unsigned knob)
{
    static const char *const eq[]={"Low band gain","Middle band gain","High band gain","Output trim"};
    static const char *const master[]={"Room size, sends, width","Repeats, sends and return","Chorus, pan and room mod","Body, drive and echo tone"};
    static const char *const macro[][4]={
        {"Drive, attack and tone","Chorus and filter motion","Hold notes, raise sustain","Room and echo sends"},
        {"Drive, attack and tone","Chorus and filter motion","Hold notes, raise sustain","Room and echo sends"},
        {"Round off the attack tone","Chorus and filter motion","Lengthen the ring","Room and echo sends"},
        {"Darken the bass tone","Chorus and filter motion","Portamento and release","Room and echo sends"},
        {"Soften hit brightness","More generated hits","Noise in the hit","Room and echo sends"},
        {"Darken the noise","Chorus and filter motion","Noise drift and release","Room and echo sends"},
        {"Transpose the sample","Smooth loop boundaries","Stereo movement","Room and echo sends"}
    };
    static const char *const global[][4]={
        {"Chance of a chord change","Slow filter variation","Vary generated density","Vary the reverb send"},
        {"Transpose the note pool","Replace the scale pool","Transport speed","Auto chord interval"},
        {"Rhythmic event grid","Phrase contour","How layers take turns","Chance of extra events"}
    };
    static const char *const fx[][4]={
        {"Reverb room size","Reverb filter","High frequency decay","Reverb algorithm"},
        {"Echo level","Echo interval","Echo repeats","Delay algorithm"},
        {"Chorus movement depth","Chorus movement rate","Stereo width","Reverb modulation"},
        {"Sweep the master filter","Output saturation","Delay brightness","Delay modulation"}
    };
    static const char *const env[]={"Time to full level","Time down to sustain","Level while held","Time after note-off"};
    static const char *const lfo[]={"Modulation waveform","Cycles per second","Amount of modulation","Patch, FX or pattern"};
    static const char *const drum[]={"Generated hit density","Delay offbeat hits","Phrase-end fill strength","Vary drum lane choices"};
    static const char *const pattern[]={"Note selection pattern","Layer step interval","Generated event density","Chance to follow chord"};
    static const char *const manual[]={"Select an edit slot","Layer step interval","Pitch of this slot","Strength of this slot"};
    static const char *const manual_drum[]={"Select an edit slot","Choose a drum voice","Enable this slot","Delay offbeat hits"};
    if(ambr.view==4)return "Layer output level";
    if(ambr.view==0 || (ambr.view==1 && ambr.page==0)){
        if(!ambr.view){if(!ambr.target)return knob==1 && !amb_home_echo()?"Fewer notes / HOME: echo":master[knob];l=&amb.layer[ambr.target-1];}
        if(!knob && l->engine==ENGI_VOICE)return "Shift the starting vowel";
        if(amb_acid(l))return acid_macro[knob];
        return macro[l->role==7?3:l->role][knob];
    }
    if(ambr.view==2 && ambr.page==7){
        static const char *const timing[]={"Count bars or seconds","Shortest wait","Longest wait","Farthest destination"};
        static const char *const lane[]={"Enable this lane","Follow journey or set interval","Shortest interval","Longest interval"};
        return timing_tab?lane[knob]:timing[knob];
    }
    if(ambr.view==2 && ambr.page==6){
        static const char *const live[]={"Bounded or endless travel","Sound replacement time","Release or crossfade","Move while playing"};
        return live[knob];
    }
    if(ambr.view==2)return ambr.page<3?global[ambr.page][knob]:ambr.page==3?fx[ambr.fx%4][knob]:eq[knob];
    if(ambr.page==1)return amb_param_hint(l,amb_sound_id(l,ambr.bank,knob));
    if(ambr.page==2)return l->role==4 || (l->role==6 && l->base[P_E1]==3)?amb_param_hint(l,amb_envelope_id(l,knob)):env[knob];
    if(ambr.page==3){
        int rate=l->base[P_LRATE],random=rate>=AMB_RANDOM_BARS;
        if(ambr.bank && (knob==3 || (!random && knob==2)))return "";
        if(amb_bar_rate(rate) && ambr.bank && knob==1)return "Shortest cycle in bars";
        if(amb_bar_rate(rate) && ambr.bank && knob==2)return "Longest cycle in bars";
        if(ambr.bank && knob==0)return "Fixed or random per cycle";
        if(random && knob==1)return ambr.bank?"Lowest cycle frequency":"Shift both rate limits";
        if(random && ambr.bank && knob==2)return "Highest cycle frequency";
        return lfo[knob];
    }
    if(ambr.page==4){
        unsigned id=amb_pattern_param(l,knob);
        if(id)return amb_param_hint(l,id);
        if(!l->generated && amb_acid(l) && !knob)return "Slide into selected step";
        if(!l->generated)return (l->role==4?manual_drum:manual)[knob];
        return (l->role==4?drum:pattern)[knob];
    }
    if(ambr.page==5 && ambr.fx==2 && knob==3)return "Shared delay algorithm";
    if(ambr.page==5 && ambr.fx==5)return amb_grain_hint[knob];
    return ambr.page==5?amb_param_hint(l,amb_fx_id(l,knob)):eq[knob];
}
