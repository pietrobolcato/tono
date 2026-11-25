/* SPDX-License-Identifier: GPL-3.0-only */
/* Full-rate, contiguous output for the spectrum. Only the visible layer
 * records; while the UI reads a complete frame the audio writer parks. */
static volatile int16_t amb_wave[AMB_SCOPE_N];
static volatile uint32_t amb_wave_ms;
static void amb_scope_sound(const int32_t *b,unsigned n)
{
    unsigned pos=amb_scope_fill;
    for(unsigned i=0;i<n && pos<AMB_SCOPE_N;i++)amb_wave[pos++]=clamp(b[i]>>3,-32767,32767);
    amb_wave_ms=fm1_ms;amb_scope_fill=pos;
}
static const uint16_t amb_filter_hz[]={80,125,200,315,500,800,1000,1250,1600,2000,2500,3150,4000,6300,10000,16000};
/* Matched one-pole coefficients, Q15, at the engine sample rate. */
static void amb_filter_process(unsigned k,int32_t *b,unsigned n,unsigned setting)
{
    static const uint16_t coefficient[]={371,578,921,1438,2253,3530,4351,5345,6681,8126,9817,11843,14230,19423,24873,29414};
    unsigned previous=amb_filter_previous[k];
    unsigned next=(setting&48)?setting:0;
    unsigned fade=next?(previous?0:1):(previous?2:0);
    if(!next)setting=previous;
    amb_filter_previous[k]=next;
    unsigned mode=(setting>>4)&3,shape=(setting>>8)&15;
    unsigned pos=amb_scope_fill;
    int capture=amb_scope_target==k+1 && amb_scope_page==5 && pos<AMB_SCOPE_N;
    if(mode){
        int a22=trk[k].engine==ENGI_AMB_A22;
        unsigned channels=a22?2:trk[k].engine==ENGI_DRUM && dx_xon?6:1;
        int a=coefficient[setting&15];
        static const uint8_t pitch[]={20,29,38,48,57,66,71,75,80,85,89,94,99,108,117,127};
        tsvf_t coef;int damping=amb_filter_damping(shape,mode==3);
        if(shape || mode==3)tsvf_coef_k(&coef,pitch[setting&15]<<8,damping);
        int32_t dry[CTL];
        for(unsigned c=0;c<channels;c++){
            int32_t *buf=!c?b:a22?voice_bank[k].a22.side:c==1?dx_sl:c==2?dx_sr:dx_sb[c-3];
            int32_t *state=amb_filter_state[k][c];
            if(fade){
                if(fade==1){state[0]=0;state[1]=buf[0];}
                memcpy(dry,buf,sizeof *dry*(n<CTL?n:CTL));
            }
            if(shape || mode==3){
                int gain=(int8_t)(setting>>8)>>4;
                amb_svf_block(buf,n,state,&coef,damping,mode==3?FT_BELL:mode-1,amb_peak_q12[gain+6]-4096);
            }else{
                int32_t low=state[1];
                for(unsigned i=0;i<n;i++){
                    int32_t x=buf[i];
                    low+=(int32_t)((((int64_t)x-low)*a)>>15);
                    buf[i]=mode==1?low:x-low;
                }
                state[0]=0;state[1]=low;
            }
            /* Enter and leave the insert continuously over one control block. */
            if(fade)for(unsigned i=0;i<n && i<CTL;i++){
                int weight=fade==1?(int)(i+1):(int)(CTL-1-i);
                buf[i]=dry[i]+(((buf[i]-dry[i])*weight)>>CTL_LOG2);
            }
        }
    }
    if(capture)amb_scope_sound(b,n);
}
