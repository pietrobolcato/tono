/* SPDX-License-Identifier: GPL-3.0-only */
#define AMB_MAIN unused_amb_main
#include "amb_native_test.c"

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
    assert(amb_scope_fill==CTL/4 && amb_wave[0][0]==2000 && amb_wave[1][0]<50);
    assert(block[0]>0 && block[0]<300 && block[1]>0);
    block[0]=16000;amb_layer_process(1,block,CTL);assert(block[0]==16000);
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
    puts("PASS layer isolation, filter response and captured taps, linked limiter controls and metering.");
}
