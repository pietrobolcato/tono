/* SPDX-License-Identifier: GPL-3.0-only */
/* Constant-time, read-only access to eight samples spaced one period apart.
 * Each group uses width+1 bytes. Width seven stores its eight points verbatim.
 * A directory entry contains the byte address and eight 4-bit widths. */
#if FELUCCA_FLASH
#define AMB_DIRECTORY_RAM __attribute__((section(".lowbuf")))
#else
#define AMB_DIRECTORY_RAM
#endif
#include "amb_pcm_octets_data.h"
static void amb_octet_offsets(uint16_t *offsets,const uint32_t *widths,unsigned count)
{
    unsigned offset=0;
    for(unsigned i=0;i<count;i++){
        offsets[i]=offset;
        unsigned row=widths[i];
        unsigned pairs=(row&0x0f0f0f0fu)+((row>>4)&0x0f0f0f0fu);
        offset+=8+((pairs*0x01010101u)>>24);
    }
}
#define AMB_INIT_OFFSETS(name) amb_octet_offsets(amb_oct_##name##_offsets,amb_oct_##name##_widths, \
                    sizeof(amb_oct_##name##_offsets)/sizeof(uint16_t))
static unsigned amb_octet_code(const uint8_t *data,const uint16_t *offsets,
                               const uint32_t *widths,unsigned sample,unsigned period)
{
    unsigned cycle=sample/period;
    unsigned group=(cycle>>3)*period+(sample-cycle*period);
    unsigned slot=group&7u, row=widths[group>>3];
    unsigned width=(row>>(4*slot))&15u;
    unsigned before=row & ~(~0u<<(4*slot));
    unsigned pairs=(before&0x0f0f0f0fu)+((before>>4)&0x0f0f0f0fu);
    unsigned offset=offsets[group>>3]+slot+((pairs*0x01010101u)>>24);
    unsigned index=cycle&7u;
    if(width==7) return data[offset+index];
    unsigned bit=index*width;
    const uint8_t *p=data+offset+1+(bit>>3);
    unsigned bits=p[0]|((unsigned)p[1]<<8);
    return data[offset]+((bits>>(bit&7))&((1u<<width)-1));
}
