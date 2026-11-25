/* SPDX-License-Identifier: GPL-3.0-only */
/* A triangular-window DFT at display rate. Axes are logarithmic in
 * frequency and level. Run only for the visible scope, outside the mixer. */
static int amb_spectrum_height(unsigned hz)
{
    uint32_t phase=0,step=hz*(0xffffffffu/FS);
    int re=0,im=0;
    for(unsigned i=0;i<AMB_SCOPE_N;i++,phase+=step){
        int w=i<128?i:255-i;
        int x=amb_wave[i]*w>>8;
        re+=(x*osc_sine(phase))>>15;
        im+=(x*osc_sine(phase+0x40000000u))>>15;
    }
    unsigned amp=((re<0?-re:re)+(im<0?-im:im))>>5;
    if(!amp)return 0;
    int exp=31-__builtin_clz(amp);
    return clamp(4*exp+((amp-(1u<<exp))*4>>exp)-10,0,49);
}
/* Nominal magnitude at each frequency, before nonlinear drive. Plot gain
 * on a dB axis with room above unity for the resonant shoulder. */
static int amb_response_y(unsigned hz,int cutoff,int type,int resonance,int layer)
{
    if(type<0)return 87;
    unsigned ratio=clamp(hz*64/cutoff,1,1024), gain;
    if(type==3){
        unsigned k=ambr.edit_layer;
        unsigned inc=pitch_inc(clamp(comb_p16[k]+(amb.layer[k].base[P_FCUT]-64)*8,0,2047));
        unsigned fundamental=(unsigned)(((uint64_t)inc*FS)>>32);
        int cosine=osc_sine(0x40000000u+hz*(0xffffffffu/(fundamental?fundamental:1)));
        unsigned feedback=125+resonance;
        gain=(256-feedback)*256/isqrt32(1+(256-feedback)*(256-feedback)+feedback*((32767-cosine)>>6));
    }else if(type==FT_BELL){
        int k=ambr.edit_layer;
        int shape=layer?(amb_controls.fx_shape[k]&15):(resonance>>3);
        int db=layer?((int8_t)amb_controls.fx_shape[k]>>4):amb_controls.bell_gain[k];
        int a=(4096-(int)(ratio*ratio))/64,b=(int)ratio*amb_filter_damping(shape,1)/4096;
        int boosted=b*(int)amb_peak_q12[db+6]/4096;
        gain=256*isqrt32(1+a*a+boosted*boosted)/isqrt32(1+a*a+b*b);
    }else if(layer && !resonance){
        unsigned d=isqrt32(64*64+ratio*ratio);
        gain=(type==0?64:ratio)*256/d;
    }else{
        int delta=4096-(int)(ratio*ratio);
        int damping=ratio*(128-resonance*116/127);
        /* Normalize before squaring to keep resonant peaks in 32 bits. */
        int a=delta/64,b=damping/64;
        unsigned d=isqrt32(1+a*a+b*b);
        gain=(type==0?64:type==1?ratio*ratio/64:ratio)*256/d;
    }
    if(!gain)return 126;
    unsigned exp=31-__builtin_clz(gain);
    return clamp(87+(8-(int)exp)*7-(int)(((gain-(1u<<exp))*7)>>exp),73,126);
}
static void amb_wave_plot(int layer)
{
    unsigned k=ambr.edit_layer;
    const amb_layer_t *l=&amb.layer[k];
    unsigned setting=amb_controls.filter[k];
    int type=layer?(int)(setting>>4)-1:l->base[P_FTYPE];
    int cut=layer?amb_filter_hz[setting&15]:CUTOFF_HZ[l->base[amb_sound_id(l,1,1)]&127];
    if(layer && type==2)type=FT_BELL;
    int res=layer?(amb_controls.fx_shape[k]&15)*127/15:l->base[amb_sound_id(l,1,2)];
    amb_display(69,72);
    int ready=amb_scope_fill==AMB_SCOPE_N && fm1_ms-amb_wave_ms<160;
    int last=81,last_height=0;
    for(unsigned i=0;i<32;i++){
        unsigned hz=CUTOFF_HZ[i*127/31];
        int h=ready?amb_spectrum_height(hz):0;
        int y=amb_response_y(hz,cut,type,res,layer);
        if(i){
            int x=14+(i-1)*7;
            for(int j=0;j<7;j++){
                int height=last_height+(h-last_height)*j/7;
                if(height)cv_rect(x+j,127-height,1,height,DISPLAY_GRID);
            }
            cv_line(x,last,x+7,y,TONO_ORANGE);
        }
        last=y;last_height=h;
    }
    if(amb_scope_fill==AMB_SCOPE_N)amb_scope_fill=0;
    amb_text(14,130,"30 Hz",SCOPE_INK,6);amb_text(189,130,"16 kHz",SCOPE_INK,8);
    cv_bg=ABG;
}
/* Map a peak to a logarithmic axis. Each doubling takes the same width. */
static int amb_meter_width(unsigned amplitude)
{
    if(amplitude<32)return 0;
    int width=0;
    while(amplitude>=64 && width<150){amplitude>>=1;width+=15;}
    return clamp(width+(amplitude-32)*15/32,0,150);
}
static void amb_limiter_plot(void)
{
    int in=lim_peak_in,out=lim_peak_out,envelope=lim_peak_env;
    lim_peak_in=lim_peak_out=lim_peak_env=0;
    int reduction=0;
    /* GR has its own 24 dB scale, independent of input headroom. */
    if(envelope>lim_ceiling)reduction=150-amb_meter_width(((uint32_t)lim_ceiling<<15)/envelope);
    const int widths[]={amb_meter_width(in),amb_meter_width(out),clamp(reduction*5/2,0,150)};
    static const char *const labels[]={"IN","OUT","GR"};
    amb_display(49,90);
    cv_text(17,50,&AF_M,"Limiter",SCOPE_INK);
    for(unsigned i=0;i<3;i++){
        int y=75+i*19,w=widths[i];
        amb_text(17,y,labels[i],SCOPE_INK,4);
        cv_rect(57,y+2,150,5,DISPLAY_GRID);
        if(w)cv_rect(i==2?207-w:57,y+2,w,5,i==2?TONO_ORANGE:DISPLAY_TRACE);
    }
    static uint8_t history[48];
    static uint32_t last;
    if(fm1_ms-last>240)memset(history,0,sizeof history);
    last=fm1_ms;
    for(unsigned i=0;i<48;i++){
        history[i]=i==47?widths[2]/15:history[i+1];
        if(i)cv_line(17+4*(i-1),126+history[i-1],17+4*i,126+history[i],TONO_ORANGE);
    }
    cv_bg=ABG;
}
