/* SPDX-License-Identifier: GPL-3.0-only */
#define AMB_MAIN unused_amb_main
#include "amb_native_test.c"

/* Compare pixels at a frozen transport position. This isolates the response to a
 * note and to the travel knob from the ordinary motion of each landscape. */
static uint32_t zen_image(unsigned scene,int open)
{
    ambr.zen_train=scene>=4;ambr.step=2+(scene<4?scene*32:(scene-4)*64);
    travel.colour=open*4;cv_begin(240,147,ABG);amb_draw_zen();
    return st_crc32(&cv_px[29*240],116*240*2);
}

int main(void)
{
    memset(amb_nor,255,sizeof amb_nor);ui_power_on();amb_init();
    fm1_in.buttons|=1u<<panel.btn[B_HOME];frame();press(B_ARP);
    fm1_in.buttons&=~(1u<<panel.btn[B_HOME]);frame();
    assert(ambr.zen && !ambr.view);
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
    for(unsigned scene=0;scene<10;scene++){
        fm1_ms+=600;
        uint32_t idle=zen_image(scene,0);
        trk_note_on(&trk[0],61,91);
        assert(zen_image(scene,0)!=idle && zen_image(scene,16)!=zen_image(scene,-16));
    }
    press(B_HOME);assert(!ambr.zen && !ambr.view);
    puts("Zen HOME controls, actual onset, decay, muted voice and exit PASS");
    return 0;
}
