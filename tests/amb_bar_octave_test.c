/* SPDX-License-Identifier: GPL-3.0-only */
#define AMB_MAIN unused_amb_native_main
#include "amb_native_test.c"

static void next_bar(void)
{
    while(ambr.step%16)amb_step();
    amb_step();
}

int main(void)
{
    memset(amb_nor,255,sizeof amb_nor);ui_power_on();amb_init();
    amb_target(1);song.playing=1;
    amb_layer_t original[4];memcpy(original,amb.layer,sizeof original);
    ambr.step=5;press(B_OCTUP);press(B_OCTUP);
    while(ambr.step%16){amb_step();assert(!amb_bar_octave[0]);}
    amb_step();assert(amb_bar_octave[0]==2 && !amb_bar_octave[1]);
    next_bar();assert(amb_bar_octave[0]==2);
    amb_target(0);press(B_OCTDN);next_bar();
    assert(amb_bar_octave[0]==1);
    for(unsigned k=1;k<4;k++)assert(amb_bar_octave[k]==-1);
    next_bar();
    assert(!memcmp(original,amb.layer,sizeof original));

    amb_target(1);ambr.view=1;ambr.edit_layer=2;
    amb.layer[2].generated=0;amb.layer[2].notes[0]=60;
    amb_layer_t manual=amb.layer[2];
    press(B_OCTDN);next_bar();assert(amb_bar_octave[2]==-2 && amb_bar_octave[0]==1);
    press(B_OCTUP);press(B_OCTDN);next_bar();
    assert(amb_bar_octave[2]==-2 && !memcmp(&manual,&amb.layer[2],sizeof manual));
    press(B_OCTUP);press(B_OCTUP);next_bar();assert(!amb_bar_octave[2]);
    amb_clock(CTL);song.playing=0;amb_clock(CTL);song.playing=1;amb_clock(CTL);
    assert(amb_bar_octave[0]==1 && amb_bar_octave[1]==-1 && !amb_bar_octave[2]);
    for(unsigned pass=0;pass<2;pass++){
        fm1_in.buttons|=1u<<panel.btn[B_HOME];host_pressed|=1u<<panel.btn[B_HOME];frame();
        press(B_OCTDN);fm1_in.buttons&=~(1u<<panel.btn[B_HOME]);frame();
        assert(amb.layer[2].base[P_MUTE]==(pass==0));
        assert(!amb.layer[0].base[P_MUTE] && ambr.view==1 && ambr.edit_layer==2 && ambr.target==1);
        next_bar();assert(!amb_bar_octave[2]);
    }
    puts("HOME modifier + OCT- in EDIT toggles only the edited layer; no octave queued PASS");
    puts("Octaves: generated/manual, queued downbeat, latched across bars and transport, Master and edit targets PASS");
}
