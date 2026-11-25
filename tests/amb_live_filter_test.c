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
static void check_bell(void)
{
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
    check_bell();
    puts("PASS layer isolation, filter response and captured taps, linked limiter controls and metering.");
}
