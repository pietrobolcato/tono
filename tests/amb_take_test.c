/* SPDX-License-Identifier: GPL-3.0-only */
#define AMB_MAIN other_main
#include "amb_native_test.c"
int main(void)
{
    memset(amb_nor,255,sizeof amb_nor);ui_power_on();amb_init();
    amb_take_start();
    for(unsigned i=0;i<44100;i++){
        int16_t s=(int16_t)(14000*sin(i*2.*3.14159265359*220./44100));
        uint32_t frame=(uint16_t)s|((uint32_t)(uint16_t)s<<16);
        amb_take_feed(&frame,1);
    }
    assert(amb_take_finish() && amb_current_take()->root==57);
    amb_patch(0,6,0);amb.layer[0].base[P_E0]=AMB_MIC_SOURCE;
    ambr.slot=2;amb_save();
    amb_take_t expected=*amb_current_take();
    amb_current_take()->length=0;amb_recall(2);
    assert(!memcmp(&expected,amb_current_take(),sizeof expected));
    amb_take_start();assert(!amb_take_finish());
    assert(!memcmp(&expected,amb_current_take(),sizeof expected));
    puts("USB PCM capture, A3 pitch match, silent attempt preserves take, song save and recall PASS");
}
