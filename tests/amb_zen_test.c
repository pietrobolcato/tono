/* SPDX-License-Identifier: GPL-3.0-only */
#define AMB_MAIN unused_amb_main
#include "amb_native_test.c"

/* Compare pixels at a frozen transport position. This isolates the response to a
 * note and to the travel knob from the ordinary motion of each landscape. */
static uint32_t zen_image(unsigned scene,int open)
{
    ambr.zen_train=scene>=4;ambr.step=2+(scene<4?scene*32:(scene-4)*64);
    travel.colour=open*4;amb_draw_zen();
    return st_crc32(host_screen,sizeof host_screen);
}

int main(void)
{
    memset(amb_nor,255,sizeof amb_nor);ui_power_on();amb_init();
    fm1_in.buttons|=1u<<panel.btn[B_HOME];frame();press(B_ARP);
    fm1_in.buttons&=~(1u<<panel.btn[B_HOME]);frame();
    assert(ambr.zen && !ambr.view);
    song.playing=1;fm1_in.notes=1;amb_leds();
    for(unsigned i=0;i<FM1_NCOL;i++)
        assert(!(fm1_led[i] | fm1_led_dim[i] | fm1_led_breath[i] | fm1_led_mid[i] | fm1_led_hi[i]));
    song.playing=0;fm1_in.notes=0;
    turn(EN_ALGO,4);assert(travel.wanted==4);
    press(B_SCL);assert(ambr.zen_train && ambr.zen);
    amb_prepare();trk[0].engine=trk[0].eng_req;trk[0].xf_on=0;
    trk_note_on(&trk[0],60,88);
    zen_layer_t layer=zen_read_layer(0);
    assert(layer.hit && layer.note==60);
    fm1_ms+=600;assert(!zen_read_layer(0).hit);
    trk[0].p[P_MUTE]=1;trk_note_on(&trk[0],65,96);
    assert(!zen_read_layer(0).hit);
    trk[0].p[P_MUTE]=0;
    for(unsigned scene=0;scene<11;scene++){
        fm1_ms+=600;
        uint32_t idle=zen_image(scene,0);
        trk_note_on(&trk[0],61,91);
        assert(zen_image(scene,0)!=idle && zen_image(scene,16)!=zen_image(scene,-16));
    }
    /* Slide through each route boundary. A one-tick crossing must not
     * repaint a whole town, sea, or a forest in a single step. */
    static uint16_t previous[240*240];
    ambr.zen_train=1;travel.colour=0;
    for(unsigned boundary=1;boundary<=7;boundary++){
        ambr.step=boundary*64;ambr.phase=FS*60u/(amb.global[G_BPM]*4u)-1;
        amb_draw_zen();memcpy(previous,host_screen,sizeof previous);
        ambr.step++;ambr.phase=0;amb_draw_zen();
        unsigned changed=0;for(unsigned i=0;i<240*240;i++)changed+=previous[i]!=host_screen[i];
        assert(changed<240*240/4);
    }
    press(B_HOME);assert(!ambr.zen && !ambr.view);
    amb_leds();
    assert(fm1_led[panel.btn[B_PLAY]>>3] & (1u<<(panel.btn[B_PLAY]&7)));
    puts("Zen LED blackout and restore, HOME controls, actual onset, decay, muted voice and exit PASS");
    return 0;
}
