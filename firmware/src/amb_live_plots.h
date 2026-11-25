/* SPDX-License-Identifier: GPL-3.0-only */
/* A triangular-window DFT at display rate. Axes are logarithmic in
 * frequency and level. Run only for the visible scope, outside the mixer. */
static unsigned amb_spectrum_channel(unsigned hz,unsigned channel,unsigned stride)
{
    uint32_t phase=0,step=hz*(0xffffffffu/FS);
    int re=0,im=0;
    for(unsigned i=channel;i<AMB_SCOPE_N;i+=stride,phase+=step){
        int w=i<128?i:255-i;
        int x=amb_wave[i]*w>>8;
        re+=(x*osc_sine(phase))>>15;
        im+=(x*osc_sine(phase+0x40000000u))>>15;
    }
    return ((re<0?-re:re)+(im<0?-im:im))*stride>>5;
}
static int amb_spectrum_height(unsigned hz)
{
    unsigned amp;
    if(amb_scope_target==AMB_SCOPE_MASTER){
        unsigned left=amb_spectrum_channel(hz,0,2),right=amb_spectrum_channel(hz,1,2);
        amp=left>right?left:right;
    }else amp=amb_spectrum_channel(hz,0,1);
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
    }else if(type>=FT_BELL){
        int k=ambr.edit_layer;
        int shape=layer?(amb_controls.fx_shape[k]&15):(resonance>>3);
        int db=layer?((int8_t)amb_controls.fx_shape[k]>>4):amb_controls.bell_gain[k];
        int a=(4096-(int)(ratio*ratio))/64,b=(int)ratio*amb_filter_damping(shape,1)/4096;
        int boosted=b*(int)amb_peak_q12[db+6]/4096;
        gain=256*(type==FT_BANDPASS?(unsigned)b:isqrt32(1+a*a+boosted*boosted))/isqrt32(1+a*a+b*b);
        if(type==FT_BANDPASS){
            unsigned single=gain;
            for(unsigned i=1;i<amb_slope_order(amb_slope(k,layer));i++)gain=gain*single/256;
        }
    }else{
        unsigned slope=amb_slope(ambr.edit_layer,layer);
        if(!slope && layer && !resonance)slope=1;
        unsigned norm=hz>(unsigned)cutoff?hz:(unsigned)cutoff;
        int lo=256*cutoff/norm,hi=256*hz/norm;
        int a=(lo*lo-hi*hi)>>8,b=(lo*hi)>>8;
        int damping=8192-resonance*7600/127;
        if(slope==5)damping/=2;
        if(slope==3)damping=damping*3135/8192;
        if(slope==2 && layer)damping=damping*5793/8192;
        int kd=b*damping>>12;
        int num=type==FT_LP?lo*lo>>8:type==FT_HP?hi*hi>>8:b;
        unsigned d=isqrt32(1+a*a+kd*kd);
        if(slope==1)num=isqrt32(num*num+b*b);
        gain=num*256/d;
        if(slope==5)gain=gain*(type==FT_LP?lo:hi)/isqrt32(1+lo*lo+hi*hi);
        if(slope==3){
            kd=b*7568>>12;
            gain=gain*num/isqrt32(1+a*a+kd*kd);
        }
    }
    if(!gain)return 126;
    unsigned exp=31-__builtin_clz(gain);
    return clamp(87+(8-(int)exp)*7-(int)(((gain-(1u<<exp))*7)>>exp),73,126);
}
/* Draw the measured spectrum first so the response trace always stays legible.
 * A master window stores interleaved channels; measuring both avoids cancellation. */
static void amb_plot_spectrum(int bottom)
{
    int ready=amb_scope_fill==AMB_SCOPE_N;
    if(ready && fm1_ms-amb_wave_ms<160){
        int last=0;
        for(unsigned i=0;i<32;i++){
            int h=amb_spectrum_height(CUTOFF_HZ[i*127/31]);
            if(i)for(int j=0;j<7;j++){
                int height=last+(h-last)*j/7;
                if(height)cv_rect(14+(i-1)*7+j,bottom-height,1,height,DISPLAY_GRID);
            }
            last=h;
        }
    }
    if(ready)amb_scope_fill=0;
}
static void amb_wave_plot(int layer)
{
    unsigned k=ambr.edit_layer;
    const amb_layer_t *l=&amb.layer[k];
    unsigned setting=amb_controls.filter[k];
    int type=layer?(int)(setting>>4)-1:l->base[P_FTYPE];
    unsigned effective=layer?amb_layer_filter_setting(k):0;
    int cut=layer?amb_frequency_hz(effective&15,effective>>16):CUTOFF_HZ[l->base[amb_sound_id(l,1,1)]&127];
    if(layer && type>=2)type+=FT_BELL-2;
    int res=layer?(amb_controls.fx_shape[k]&15)*127/15:l->base[amb_sound_id(l,1,2)];
    amb_display(69,72);
    amb_plot_spectrum(127);
    int last=0;
    for(unsigned i=0;i<32;i++){
        int y=amb_response_y(CUTOFF_HZ[i*127/31],cut,type,res,layer);
        if(i)cv_line(14+(i-1)*7,last,14+i*7,y,TONO_ORANGE);
        last=y;
    }
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

/* Real and imaginary gain of the DSP's 26/1024 and 307/1024 one-pole
 * crossovers at each log-frequency bin: H = a / (1 - (1-a) exp(-jw)). */
static int amb_eq_response_scaled(unsigned bin,const int gain[4])
{
    static const int8_t crossover[32][4]={
        {124,-20,127,-1},{122,-25,127,-2},{120,-29,127,-2},{117,-34,127,-2},
        {112,-40,127,-3},{106,-47,127,-3},{98,-53,127,-4},{89,-58,127,-5},
        {77,-61,127,-6},{65,-63,126,-7},{53,-62,126,-9},{39,-57,126,-12},
        {29,-52,125,-14},{22,-46,124,-17},{16,-40,123,-20},{12,-34,121,-24},
        {9,-29,118,-29},{6,-24,115,-34},{5,-20,109,-39},{4,-16,103,-44},
        {3,-14,95,-48},{3,-11,83,-52},{2,-9,72,-52},{2,-7,62,-51},
        {2,-6,52,-47},{2,-5,44,-42},{2,-4,38,-37},{2,-3,33,-31},
        {2,-2,29,-25},{2,-2,26,-20},{2,-1,24,-15},{2,-1,23,-8}
    };
    const int8_t *p=crossover[bin];
    int hi=gain[2],mid=gain[1],lo=gain[0];
    int re=hi+((lo-mid)*p[0]+(mid-hi)*p[2])/127;
    int im=((lo-mid)*p[1]+(mid-hi)*p[3])/127;
    unsigned magnitude=isqrt32(re*re+im*im);
    int exp=31-__builtin_clz(magnitude?magnitude:1);
    int db2=(exp-8)*12+(int)((magnitude-(1u<<exp))*12>>exp);
    return clamp(82-db2-gain[3],55,107);
}
