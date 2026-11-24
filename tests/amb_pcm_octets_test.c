/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdint.h>
#include <stdio.h>
#include "../firmware/src/amb_samples.h"
#include "../firmware/src/amb_acoustic_pcm.h"
#include "../firmware/src/amb_pcm_octets.h"
static int check(const uint8_t *ref,unsigned size,const uint8_t *data,
                 const uint16_t *off,const uint32_t *width,unsigned period)
{
    for(unsigned i=0;i<size;i++)
        if(amb_octet_code(data,off,width,i,period)!=ref[i]){
            fprintf(stderr,"PCM mismatch at %u of %u\n",i,size);
            return 1;
        }
    return 0;
}
#define CHECK(raw,name) check(raw,sizeof(raw),amb_oct_##name,amb_oct_##name##_offsets, \
                              amb_oct_##name##_widths,amb_oct_##name##_period)
int main(void)
{
    AMB_INIT_OFFSETS(violin);
    return CHECK(AMB_SAMPLES[0],chime) || CHECK(AMB_SAMPLES[1],vowel) ||
           CHECK(amb_pcm_violin,violin) || CHECK(amb_pcm_tenor,tenor);
}
