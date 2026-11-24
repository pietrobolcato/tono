/* SPDX-License-Identifier: GPL-3.0-only */
/* Factory audition labels, in picker order. Each pair indexes the shared
 * vocabulary. A personal take has no predetermined timbre. */
static void amb_patch_character(unsigned role,unsigned patch)
{
    static const char words[][9]={"","Warm","drone","Buzzy","Rounded","Bright","Soft","Deep","Nasal","Airy","Hollow","Breathy","Dark","throb","Glassy","hum","shimmer","swell","wash","ring","Resonant","pluck","Woody","Metallic","chime","pulse","Muted","strike","hiss","Grainy","rattle","click","rasp","flutter","Sharp","speech","murmur","Thin"};
    static const uint8_t start[]={0,28,42,63,83,92,122,141,153};
    static const uint8_t labels[][2]={{1,2},{3,2},{4,2},{5,2},{6,2},{7,2},{8,2},{9,2},
        {3,2},{1,2},{10,2},{11,2},{12,13},{8,2},{1,2},{14,2},
        {4,2},{5,15},{1,15},{3,16},{1,17},{8,2},{1,2},{3,2},
        {4,15},{5,18},{1,15},{14,19},{1,17},{7,2},{14,17},{6,2},
        {20,17},{12,15},{9,18},{7,2},{10,2},{3,2},{12,2},{3,15},
        {1,2},{20,2},{1,19},{14,19},{6,21},{5,19},{7,19},{9,17},
        {10,21},{1,15},{3,2},{4,15},{22,21},{23,19},{14,24},{5,24},
        {1,15},{20,15},{3,2},{9,18},{5,2},{4,15},{10,15},{7,25},
        {1,21},{3,25},{5,25},{1,25},{6,25},{26,13},{1,25},{4,25},
        {9,18},{10,21},{3,25},{6,25},{8,17},{1,21},{22,27},{10,21},
        {3,25},{12,13},{20,25},{26,27},{7,25},{20,27},{22,25},{12,13},
        {23,25},{1,25},{5,27},{7,25},{9,18},{12,18},{5,28},{29,30},
        {11,18},{12,18},{9,18},{9,18},{12,18},{29,30},{26,31},{12,18},
        {5,28},{22,30},{3,32},{9,28},{29,33},{20,2},{9,18},{34,30},
        {29,30},{9,18},{26,35},{12,25},{29,35},{34,21},{9,17},{9,17},
        {1,2},{26,35},{3,2},{7,15},{14,2},{6,2},{1,2},{4,2},
        {9,2},{9,18},{29,30},{7,36},{11,25},{29,28},{1,2},{26,35},
        {37,35},{8,35},{0,0},{7,2},{9,18},{1,15},{5,21},{9,17},
        {14,19},{22,21},{11,17},{3,15},{22,21},{1,17},{5,19},{4,15},
        {34,19}};
    if(role>=NELEM(start)-1 || patch>=(unsigned)(start[role+1]-start[role]))return;
    const uint8_t *pair=labels[start[role]+patch];
    if(!pair[0])return;
    amb_text(12,105,words[pair[0]],AINK,12);
    amb_text(15+text_w(&AF_TONO_LABEL,words[pair[0]]),105,words[pair[1]],AINK,12);
}
