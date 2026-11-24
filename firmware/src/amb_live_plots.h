/* SPDX-License-Identifier: GPL-3.0-only */
static void amb_wave_plot(int layer)
{
    amb_text(12,68,layer?"LAYER FILTER":"FILTER OUTPUT",AINK,16);
    amb_display(80,58);
    cv_line(17,109,222,109,DISPLAY_GRID);
    if(amb_scope_fill==64){
        if(fm1_ms-amb_wave_ms<160){
            int peak=128;
            for(unsigned ch=layer?0:1;ch<2;ch++)
                for(unsigned i=0;i<64;i++){
                    int v=amb_wave[ch][i];if(v<0)v=-v;if(v>peak)peak=v;
                }
            int gain=(22<<12)/peak;
            for(unsigned ch=layer?0:1;ch<2;ch++){
                int last=109-((amb_wave[ch][0]*gain)>>12);
                for(unsigned i=1;i<64;i++){
                    int next=109-((amb_wave[ch][i]*gain)>>12);
                    cv_line(18+(i-1)*3,last,18+i*3,next,ch?TONO_ORANGE:SCOPE_INK);
                    last=next;
                }
            }
        }
        amb_scope_fill=0;
    }
    if(layer){
        amb_text(18,82,"IN",SCOPE_INK,3);
        amb_text(39,82,"OUT",TONO_ORANGE,4);
    }
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
        history[i]=i==47?widths[2]/10:history[i+1];
        if(i)cv_line(17+4*(i-1),121+history[i-1],17+4*i,121+history[i],TONO_ORANGE);
    }
    cv_bg=ABG;
}
