/* SPDX-License-Identifier: GPL-3.0-only */
#define AMB_MAIN unused_amb_main
#include "../../tests/amb_native_test.c"
int main(void){
    memset(amb_nor,255,sizeof amb_nor);ui_power_on();amb_init();
    amb_patch(0,6,0);amb.layer[0].base[P_E0]=AMB_MIC_SOURCE;
    amb.layer[0].base[P_ATK]=12;amb.layer[0].base[P_E1]=0;
    amb.layer[0].base[P_E6]=20;amb.layer[0].base[P_REV]=48;
    assert(amb_valid(&amb));
    return fwrite(&amb,sizeof amb,1,stdout)==1?0:1;
}
