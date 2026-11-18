/* SPDX-License-Identifier: GPL-3.0-only */
/* Each recipe chooses an actual JIANT preset, then deliberately revoices
 * its envelope, register/colour, gain and sends for the ambient role. */
typedef struct {
    const char *name;
    uint8_t engine, preset, attack, release, level, reverb, delay, color;
} amb_recipe_t;
#define AR(n,e,p,a,r,l,v,d,c) {n,e,p,a,r,l,v,d,c}
static const amb_recipe_t amb_pad_patches[] = {
  AR("Cloud Bloom",0,1,86,108,92,83,29,51),AR("Soft Choir",2,8,78,112,88,76,36,64),
  AR("Velvet Saw",0,15,71,100,90,68,26,45),AR("Glass Haze",2,9,84,116,84,75,19,58),
  AR("Warm Strings",0,11,82,102,91,72,24,57),AR("Low Sun",0,22,94,114,93,79,35,35),
  AR("Slow Orbit",2,6,88,110,85,85,45,60),AR("Distant Light",0,3,102,118,90,92,32,63),
  AR("Open Aah",5,0,76,112,85,74,12,93),AR("Round Ooh",5,4,86,116,86,82,16,95),
  AR("Plainchant",5,5,52,108,91,86,8,97),AR("Reed Cloud",7,3,84,112,78,62,14,86),
  AR("Float Garden",3,11,91,112,70,77,28,66),
  AR("Amber Brass",0,9,74,116,87,55,12,43),AR("Cedar Triangle",0,21,81,116,94,58,8,61),
  AR("Umber Pulse",0,24,90,116,82,62,15,37)
};
static const amb_recipe_t amb_drone_patches[] = {
  AR("Deep Current",0,22,110,120,90,75,35,28),AR("Hollow Earth",2,8,103,116,86,80,42,43),
  AR("Quiet Organ",7,0,94,112,80,70,24,58),AR("Tape Hum",0,7,116,122,90,65,48,36),
  AR("Underwater",2,6,105,120,86,87,39,40),AR("Harmonic Bed",7,2,97,115,78,82,34,67),
  AR("Dark Air",11,4,110,124,84,87,43,30),AR("Warm Voltage",0,19,100,117,91,72,33,43),
  AR("Low Ooh",5,4,90,118,90,70,12,91),AR("Chapel Aah",5,0,88,120,84,83,9,89),
  AR("Cantus",5,5,64,114,87,88,10,92),AR("Slow Chip",3,12,94,110,76,62,20,78)
};
static const amb_recipe_t amb_bells_patches[] = {
  AR("Felt Glass",12,11,0,100,86,88,45,65),AR("Distant Chime",12,8,4,110,80,94,53,79),
  AR("Wooden Star",12,14,0,86,92,65,37,49),AR("Rain Glass",2,9,3,94,84,84,58,72),
  AR("Soft FM",12,0,8,98,86,80,39,61),AR("Resonant Dust",12,10,1,89,84,73,49,54),
  AR("Floating Metal",0,13,2,106,82,92,56,85),AR("Night Sparks",12,7,0,90,84,83,62,70),
  AR("Felt Piano",2,8,12,90,94,76,21,92),AR("Nylon Harp",2,13,9,91,84,58,19,80),
  AR("Porcelain",3,15,15,85,75,70,34,88)
};
static const amb_recipe_t amb_bass_patches[] = {
  AR("Round Sub",0,7,8,65,101,20,14,32),AR("Soft Pulse",0,2,14,72,96,28,23,39),
  AR("Breathing Low",0,19,48,88,96,38,26,35),AR("Warm Fifth",0,14,20,79,93,34,28,43),
  AR("Faded Mono",2,7,9,69,92,23,19,48),AR("Slow Glide",0,21,25,86,94,42,34,46),
  AR("Low Echo",12,2,12,83,90,36,49,53),AR("Root Bed",0,22,62,98,97,46,22,29),
  AR("Reed Pedal",7,9,28,77,84,34,9,72)
};
static const amb_recipe_t amb_drums_patches[] = {
  AR("Dust Steps",10,3,0,75,86,42,23,45),AR("Soft Pulse",10,2,0,81,91,34,19,52),
  AR("Brush Rain",10,5,0,70,82,64,32,60),AR("Broken Clock",10,8,0,62,81,51,46,68),
  AR("Slow Ritual",10,6,0,88,88,58,40,43),AR("Hollow Perc",10,0,0,74,84,70,35,55),
  AR("Hidden Beat",10,1,0,66,82,38,29,49),AR("Sparse Ticks",10,7,0,59,77,65,54,73)
};
static const amb_recipe_t amb_texture_patches[] = {
  AR("Ocean Air",11,4,110,117,83,89,38,42),AR("Tape Mist",11,7,98,114,82,81,46,55),
  AR("High Air",11,0,87,108,81,93,53,68),AR("Paper Grain",11,6,28,83,78,69,43,64),
  AR("Distant Hiss",11,5,100,116,80,92,39,53),AR("Rainfield",11,1,60,101,77,85,56,73),
  AR("Organic Noise",11,0,105,115,80,88,47,60),AR("Midnight Static",11,5,96,120,76,94,37,41)
};
#undef AR
static const struct { const amb_recipe_t *patches; unsigned count; } amb_bank[] = {
    {amb_pad_patches, NELEM(amb_pad_patches)},
    {amb_drone_patches, NELEM(amb_drone_patches)},
    {amb_bells_patches, NELEM(amb_bells_patches)},
    {amb_bass_patches, NELEM(amb_bass_patches)},
    {amb_drums_patches, NELEM(amb_drums_patches)},
    {amb_texture_patches, NELEM(amb_texture_patches)},
};
static unsigned amb_patch_count(unsigned role)
{
    /* SAMPLE historically stores a recipe index, its source lives in E0. */
    return role<NELEM(amb_bank)?amb_bank[role].count:8;
}
static const amb_recipe_t *amb_recipe(unsigned role,unsigned patch)
{
    return &amb_bank[role<6?role:5].patches[patch];
}
