/* SPDX-License-Identifier: GPL-3.0-only */
/* Capture 64 points at four-sample spacing. The renderer reads only complete
 * frames, then asks the audio thread for the next one. */
static volatile int16_t amb_wave[2][64] __attribute__((section(".lowbuf")));
static volatile uint32_t amb_wave_ms;
static int16_t amb_wave_point(int32_t x){return (int16_t)clamp(x>>3,-32767,32767);}
static void amb_scope_sound(const int32_t *b,unsigned n)
{
    unsigned pos=amb_scope_fill;
    for(unsigned i=0;i<n && pos<64;i+=4)amb_wave[1][pos++]=amb_wave_point(b[i]);
    amb_wave_ms=fm1_ms;
    amb_scope_fill=pos;
}
static const uint16_t amb_filter_hz[]={80,125,200,315,500,800,1000,1250,1600,2000,2500,3150,4000,6300,10000,16000};
/* Matched one-pole coefficients, Q15, at the engine sample rate. */
static void amb_filter_process(unsigned k,int32_t *b,unsigned n,unsigned setting)
{
    static const uint16_t coefficient[]={371,578,921,1438,2253,3530,4351,5345,6681,8126,9817,11843,14230,19423,24873,29414};
    unsigned mode=setting>>4;
    unsigned pos=amb_scope_fill;
    int capture=amb_scope_target==k+1 && amb_scope_page==5 && pos<64;
    if(capture)for(unsigned i=0;i<n && pos+i/4<64;i+=4)amb_wave[0][pos+i/4]=amb_wave_point(b[i]);
    if(mode){
        int a22=trk[k].engine==ENGI_AMB_A22;
        unsigned channels=a22?2:trk[k].engine==ENGI_DRUM && dx_xon?6:1;
        int a=coefficient[setting&15];
        for(unsigned c=0;c<channels;c++){
            int32_t *buf=!c?b:a22?voice_bank[k].a22.side:c==1?dx_sl:c==2?dx_sr:dx_sb[c-3];
            int32_t low=amb_filter_state[k][c];
            for(unsigned i=0;i<n;i++){
                int32_t x=buf[i];
                low+=(int32_t)((((int64_t)x-low)*a)>>15);
                buf[i]=mode==1?low:x-low;
            }
            amb_filter_state[k][c]=low;
        }
    }
    if(capture)amb_scope_sound(b,n);
}
