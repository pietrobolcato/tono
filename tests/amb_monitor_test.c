/* SPDX-License-Identifier: GPL-3.0-only */
#define AMB_MAIN unused_amb_native_main
#include "amb_native_test.c"

static void toggle_fx(void)
{
    btn_down(B_HOME);frame();press(B_FX);btn_up(B_HOME);frame();
}

int main(void)
{
    int32_t out[CTL*2];
    memset(amb_nor,255,sizeof amb_nor);ui_power_on();amb_init();
    amb_target(1);press(B_EDIT);press(B_SCL);
    amb_piece_t saved=amb;
    toggle_fx();
    assert(amb_monitor_requested && ambr.view==1 && ambr.page==1);
    assert(!memcmp(&amb,&saved,sizeof amb));
    trk_note_on(&trk[0],48,90);
    amb_render(FS/2);
    assert(amb_monitor_blend==AMB_MONITOR_RAMP);
    mix_block(out,CTL);amb_output(out,CTL);
    unsigned audible=0;
    for(unsigned i=0;i<2*CTL;i++)audible|=out[i]!=0;
    assert(audible);
    /* A silent dry tap rejects even a full-scale stereo effect return. */
    memset(amb_monitor_bus,0,sizeof amb_monitor_bus);
    for(unsigned i=0;i<2*CTL;i++)out[i]=14000;
    amb_monitor_output(out,CTL);
    for(unsigned i=0;i<2*CTL;i++)assert(out[i]==0);
    toggle_fx();amb_render(FS/10);
    assert(!amb_monitor_blend && !amb_monitor_requested && !memcmp(&amb,&saved,sizeof amb));
    puts("Live FX bypass: same screen and piece, audible dry voice, zero wet leakage, reversible PASS");
}
