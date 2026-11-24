/* SPDX-License-Identifier: GPL-3.0-only */
#define AMB_MAIN unused_amb_main
#include "amb_native_test.c"

int main(void)
{
    memset(amb_nor,255,sizeof amb_nor);ui_power_on();amb_init();amb_fixture_scene(0);
    amb_target(1);press(B_LFO);
    amb_layer_t *layer=&amb.layer[0];
    layer->lfo_depth=0;
    char label[16]={0},text[24]={0};
    amb_knob_value(2,label,text);assert(!strcmp(text,"Off"));
    amb_knob(2,11);int first_rate=layer->base[P_LRATE];
    press(B_LFO);
    assert(amb_second_editor());amb_knob_value(2,label,text);assert(!strcmp(text,"Off"));
    amb_knob(0,2);amb_knob(1,22);amb_knob(2,40);
    assert(layer->lfo_depth==22 && layer->base[P_LRATE]==first_rate);
    amb_lfo_settings_t *second=&amb_controls.second[0];
    assert(second->depth==80 && second->wave==2 && second->rate!=first_rate);
    press(B_EDIT);amb_knob(0,1);assert(second->rate>=AMB_RANDOM_HZ);
    amb_knob(2,4);assert(amb_rate_min(second->rate)<amb_rate_max(second->rate));
    amb_knob(0,1);assert(amb_bar_rate(second->rate));press(B_EDIT);
    second->rate=61;second->dest=ROUTE_HOME_DENSITY;
    for(int i=0;i<150;i++)track_lfo_tick(&trk[0]);
    assert(amb_second[0].phase && amb_second[0].phase!=trk[0].lfo_ph);
    amb_second[0].val=-32767;
    assert(amb_routed(0,ROUTE_HOME_DENSITY,80,0,100)==40);
    assert(amb_routed(1,ROUTE_HOME_DENSITY,80,0,100)==80);
    ambr.slot=0;amb_save();second->depth=0;assert(amb_recall(0) && second->depth==80 && second->rate==61);
    press(B_LFO);assert(!amb_second_editor() && layer->base[P_LRATE]==first_rate);
    puts("PASS dual LFO independent timing, routing, editing, Off and song recall.");
}
