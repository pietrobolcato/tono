/* SPDX-License-Identifier: GPL-3.0-only */
#define AMB_MAIN unused_amb_main
#include "amb_native_test.c"
#include "../firmware/src/amb_sound_variations.h"

int main(void)
{
    memset(amb_nor,255,sizeof amb_nor);ui_power_on();amb_init();
    unsigned count=0;
    for(unsigned role=0;role<8;role++){
        unsigned patches=role==6?NELEM(N_AS_SOURCE):amb_patch_count(role);
        for(unsigned patch=0;patch<patches;patch++){
            amb_fixture_scene(0);
            amb_patch(0,role,patch);
            amb_layer_t *layer=&amb.layer[0];
            /* Recording start/end belong to the take. A player can set both
             * before trying any timbre on that source. */
            if(layer->engine==ENGI_AMB_SAMPLE){
                layer->base[P_E4]=12;
                layer->base[P_E5]=77;
            }
            amb_layer_t original=*layer,seen[4];
            unsigned n=amb_variation_count(layer);
            assert(n>=1 && n<=4);
            for(unsigned i=0;i<n;i++){
                assert(*amb_variation_name(layer,i));
                amb_apply_variation(0,i);
                assert(amb_valid(&amb) && layer->level==original.level);
                assert(layer->base[P_E0]==original.base[P_E0]);
                assert(!memcmp(layer->notes,original.notes,sizeof original.notes));
                if(layer->engine==ENGI_AMB_SAMPLE)
                    assert(layer->base[P_E4]==12 && layer->base[P_E5]==77);
                for(unsigned j=0;j<i;j++)assert(memcmp(layer->base,seen[j].base,sizeof layer->base));
                seen[i]=*layer;amb_apply_variation(0,i);assert(!memcmp(&seen[i],layer,sizeof *layer));
                count++;
            }
            amb_apply_variation(0,0);
            assert(!memcmp(layer,&original,sizeof original));
        }
    }
    printf("Validated %u per-patch voicings, independent edits, stable reapplication and factory return.\n",count);
    return 0;
}
