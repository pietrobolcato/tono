/* SPDX-License-Identifier: GPL-3.0-only */
#define AMB_MAIN unused_amb_main
#include "amb_native_test.c"

/* Measure steady-state amplitude on the actual layer and synth processors. */
static double bell_response(unsigned hz,int gain,unsigned width,int synth)
{
    uint32_t phase=0,step=hz*(0xffffffffu/FS);
    double before=0,after=0;
    int32_t samples[CTL];
    unsigned packed=54u|(((gain&15)*16u+width)<<8);
    amb_bell_gain[0]=gain;
    for(unsigned j=0;j<320;j++){
        for(unsigned i=0;i<CTL;i++,phase+=step){
            samples[i]=osc_sine(phase)>>3;
            if(j>160)before+=(double)samples[i]*samples[i];
        }
        if(synth)amb_synth_bell(0,samples,CTL,71,width<<3);
        else amb_filter_process(0,samples,CTL,packed);
        if(j>160)for(unsigned i=0;i<CTL;i++)after+=(double)samples[i]*samples[i];
    }
    return sqrt(after/before);
}
/* A doubling in the stop band resolves rolloff from a mere gain change.
 * Correlate with the source to exclude fixed-point DC and rounding noise. */
static double slope_level(unsigned fx,unsigned type,unsigned slope,unsigned hz,unsigned cutoff)
{
    uint32_t phase=0,inc=hz*(0xffffffffu/FS);
    double re=0,im=0,ref=0;int32_t b[CTL];
    for(unsigned j=0;j<512;j++){
        uint32_t start=phase;
        for(unsigned i=0;i<CTL;i++,phase+=inc)b[i]=osc_sine(phase);
        if(fx)amb_filter_process(0,b,CTL,(type+1)*16+(cutoff==71?6:0)+((slope&3)<<6)+((slope&4)<<10));
        else {
            track_t *t=&trk[0];
            t->p[P_FTYPE]=type;t->p[P_FCUT]=cutoff;t->p[P_FRES]=0;
            amb_synth_slope[0]=slope;
            track_post(t,&ENG_WHEEL,b,CTL);
        }
        for(unsigned i=0;j>=128 && i<CTL;i++,start+=inc){
            double x=osc_sine(start);
            re+=b[i]*x;im+=b[i]*(double)osc_sine(start+0x40000000u);ref+=x*x;
        }
    }
    return hypot(re,im)/ref;
}
static void check_slopes(void)
{
    trk[0].p[P_LD_FLT]=trk[0].p[P_ED_FLT]=0;
    for(unsigned fx=0;fx<2;fx++)for(unsigned type=0;type<2;type++)for(unsigned slope=1;slope<=5;slope++){
        if(slope==4)continue;
        double a=slope_level(fx,type,slope,type?520:2300,71);
        double b=slope_level(fx,type,slope,type?260:4600,71);
        double db=20*log10(a/b),expected=6*amb_slope_order(slope);
        printf("%s %s %u dB/oct: measured %.1f dB/oct\n",fx?"layer":"synth",type?"HP":"LP",(unsigned)expected,db);
        assert(fabs(db-expected)<3.5);
    }
    /* At the 80 Hz lower endpoint, a quiet signal must still drive the
     * second integrator while hundreds of Hz above the corner drop away. */
    for(unsigned fx=0;fx<2;fx++){
        assert(slope_level(fx,FT_LP,3,45,20)>.9);
        assert(slope_level(fx,FT_LP,3,320,20)<.01);
    }
    amb_synth_slope[0]=0;
}
static void check_bell(void)
{
    int32_t dc[CTL];unsigned level=12000;
    /* The legacy one-pole and resonant SVF share the same low-pass units. */
    for(unsigned block=0;block<120;block++){
        for(unsigned i=0;i<CTL;i++)dc[i]=level;
        amb_filter_process(0,dc,CTL,22);
    }
    for(unsigned i=0;i<CTL;i++)dc[i]=level;
    amb_filter_process(0,dc,CTL,22|0x100);
    for(unsigned i=0;i<CTL;i++)assert(abs(dc[i]-(int)level)<32);
    for(unsigned i=0;i<CTL;i++)dc[i]=level;
    amb_filter_process(0,dc,CTL,0);
    assert(dc[CTL-1]==level);
    for(unsigned i=0;i<CTL;i++)dc[i]=-8000;
    amb_filter_process(0,dc,CTL,22|0x100);
    for(unsigned i=0;i<CTL;i++)assert(abs(dc[i]+8000)<32);
    for(int synth=0;synth<2;synth++){
        assert(bell_response(1000,0,8,synth)==1.0);
        assert(bell_response(1000,6,8,synth)>3.7);
        assert(bell_response(1000,-6,8,synth)<0.29);
        assert(bell_response(250,6,15,synth)>bell_response(250,6,0,synth)+0.35);
    }
    ambr.view=1;ambr.page=1;ambr.bank=1;ambr.detail=1;
    amb_layer_t *l=&amb.layer[0];
    l->base[P_FTYPE]=FT_COMB;
    char label[16],value[24];amb_knob_value(0,label,value);
    assert(!strcmp(value,"COMB"));amb_knob(0,1);assert(l->base[P_FTYPE]==FT_BELL);
    amb_knob(2,4);assert(amb_controls.bell_gain[0]==4);
    amb_knob(0,-1);assert(l->base[P_FTYPE]==FT_HP);amb_knob(0,1);
    ambr.page=5;ambr.fx=6;amb_controls.filter[0]=38;
    amb_knob(0,1);amb_knob(2,-3);amb_knob(3,11);
    assert(amb_controls.filter[0]==54 && amb_controls.fx_shape[0]==0xdb);
    assert(amb_response_y(1000,1000,FT_BELL,90,1)>amb_response_y(80,1000,FT_BELL,90,1));
    ambr.slot=5;amb_save();amb_fixture_scene(0);
    assert(amb_recall(5) && amb_controls.filter[0]==54 && amb_controls.fx_shape[0]==0xdb &&
           amb_controls.bell_gain[0]==4 && amb.layer[0].base[P_FTYPE]==FT_BELL);
}
static void check_bandpass(void)
{
    trk[0].p[P_LD_FLT]=trk[0].p[P_ED_FLT]=0;
    for(int fx=0;fx<=1;fx++){
        ambr.view=1;ambr.edit_layer=0;ambr.page=fx?5:1;ambr.bank=1;ambr.detail=1;ambr.fx=6;
        amb_knob(0,8);
        amb_knob(3,-200);
        char label[16],value[24];
        amb_knob_value(0,label,value);assert(!strcmp(value,"Band-pass"));
        amb_knob_value(3,label,value);assert(!strcmp(label,"WIDTH"));
        for(unsigned order=1;order<=4;order++)for(unsigned wide=0;wide<=1;wide++){
            amb_knob(2,-8);amb_knob(2,order-1);
            amb_knob(3,-200);
            amb_synth_slope[0]=amb_slope(0,0);
            if(wide)amb_knob(3,200);
            unsigned width=fx?(amb_controls.fx_shape[0]&15)*8:amb.layer[0].base[P_FRES];
            double levels[3];
            const unsigned hz[]={125,1000,8000};
            for(unsigned j=0;j<3;j++){
                uint32_t phase=0,step=hz[j]*(0xffffffffu/FS);
                double before=0,after=0;
                for(unsigned block=0;block<320;block++){
                    int32_t b[CTL];
                    for(unsigned i=0;i<CTL;i++,phase+=step){
                        b[i]=osc_sine(phase)>>1;
                        if(block>=160)before+=(double)b[i]*b[i];
                    }
                    if(fx)amb_filter_process(0,b,CTL,amb_layer_filter_setting(0));
                    else tf_run(&trk[0],b,CTL,FT_BANDPASS,TF_DRY,0,0,0,71,width);
                    if(block>=160)for(unsigned i=0;i<CTL;i++)after+=(double)b[i]*b[i];
                }
                levels[j]=sqrt(after/before);
            }
            printf("Band-pass %s width %u: 125/1000/8000 Hz %.3f %.3f %.3f\n",
                   fx?"layer":"synth",width,levels[0],levels[1],levels[2]);
            assert(levels[1]>.85 && levels[0]<.22 && levels[2]<.22);
            assert(wide && order==1?levels[0]>.09:levels[0]<.045);
            if(order>1)assert(levels[2] < .04);
        }
        assert(amb_response_y(125,1000,FT_BANDPASS,127,fx)>amb_response_y(1000,1000,FT_BANDPASS,127,fx));
    }
    ambr.slot=4;amb_save();amb_fixture_scene(0);assert(amb_recall(4));
    assert(amb.layer[0].base[P_FTYPE]==FT_BANDPASS && (amb_controls.filter[0]>>4)==4);
}
int main(void)
{
    int32_t block[CTL];
    memset(amb_nor,255,sizeof amb_nor);
    ui_power_on();amb_init();amb_fixture_scene(AMB_SCENE_GARDEN);
    ambr.view=1;ambr.edit_layer=0;ambr.page=5;ambr.fx=6;
    int16_t patch_cut=amb.layer[0].base[P_FCUT];
    amb_knob(0,1);
    assert(amb_controls.filter[0]==16 && amb_controls.filter[1]==0);
    assert(amb.layer[0].base[P_FCUT]==patch_cut);
    /* A single sharp edge persists in the input trace and rolls off in the
     * output. Each layer owns its own filter memory. */
    memset(block,0,sizeof block);amb_layer_process(0,block,CTL);
    for(unsigned i=0;i<CTL;i++)block[i]=i?0:16000;
    amb_scope_target=1;amb_scope_page=5;amb_scope_fill=0;
    amb_layer_process(0,block,CTL);
    assert(amb_scope_fill==CTL && amb_wave[0]==(block[0]>>3));
    assert(block[0]>0 && block[0]<300 && block[1]>0);
    block[0]=16000;amb_layer_process(1,block,CTL);assert(block[0]==16000);
    /* The displayed input spectrum must place a real signal in its own band. */
    uint32_t phase=0,step=2000u*(0xffffffffu/FS);
    for(unsigned i=0;i<AMB_SCOPE_N;i++,phase+=step)amb_wave[i]=osc_sine(phase)>>4;
    assert(amb_spectrum_height(2000)>amb_spectrum_height(500)+12);
    memset(amb_wave,0,sizeof amb_wave);
    assert(amb_spectrum_height(2000)==0);
    /* Screen y increases downward. LP and HP slope in opposite directions. */
    assert(amb_response_y(100,1000,0,0,1)<amb_response_y(4000,1000,0,0,1));
    assert(amb_response_y(100,1000,1,0,1)>amb_response_y(4000,1000,1,0,1));
    amb_knob(0,1);
    for(unsigned n=0;n<100;n++){
        for(unsigned i=0;i<CTL;i++)block[i]=16000;
        amb_layer_process(0,block,CTL);
    }
    assert(abs(block[CTL-1])<100);
    ambr.view=2;ambr.page=3;ambr.fx=4;
    amb_knob(0,-4);amb_knob(1,-2);
    assert(lim_ceiling==4125 && lim_shift==11);
    lim_meter_on=1;
    int32_t l=30000,r=3000;master_out(&l,&r);
    assert(l<=lim_ceiling && r<lim_ceiling/8 && lim_peak_in>lim_peak_out && lim_peak_env>lim_ceiling);
    check_slopes();
    check_bell();
    check_bandpass();
    amb_set_slope(0,0,0);amb_set_slope(0,1,0);
    ambr.fx=6;ambr.page=5;amb_controls.filter[0]=16;amb_controls.fx_shape[0]=0;
    char label[16],value[24];amb_knob_value(3,label,value);assert(!strcmp(value,"6dB/oct"));
    amb_knob(3,2);amb_knob_value(3,label,value);assert(!strcmp(value,"18dB/oct"));
    ambr.slot=6;amb_save();amb_fixture_scene(0);assert(amb_recall(6) && amb_slope(0,1)==5);
    ambr.view=1;ambr.edit_layer=0;ambr.fx=6;ambr.page=5;
    amb_knob(3,1);amb_knob_value(3,label,value);assert(!strcmp(value,"24dB/oct"));
    assert(amb_slope(0,1)==3 && amb_slope(0,0)==0 && amb_slope(1,1)==0);
    ambr.slot=7;amb_save();amb_fixture_scene(0);assert(amb_recall(7) && amb_slope(0,1)==3);

    puts("PASS layer isolation, filter response and captured taps, linked limiter controls and metering.");
}
