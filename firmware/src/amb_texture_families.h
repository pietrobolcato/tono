/* SPDX-License-Identifier: GPL-3.0-only */
/* Members are existing sound IDs. Songs and Auto Travel keep using those IDs.
 * The first member is the family shown under ALGO. */
static const uint8_t amb_texture_members[]={
    0,1,2,4,6,7,11,27,   /* Ocean Air */
    3,5,8,9,10,21,       /* Paper Grain */
    12,13,14,15,16,19,20,/* Ocean Beads */
    17,18,22,23,24,25,26,28,29,30
};
static const uint8_t amb_texture_starts[]={0,8,14,21,22,23,24,25,26,27,28,29,30,31};
static unsigned amb_texture_family(unsigned id)
{
    for(unsigned f=0;f+1<NELEM(amb_texture_starts);f++)
        for(unsigned j=amb_texture_starts[f];j<amb_texture_starts[f+1];j++)
            if(amb_texture_members[j]==id)return f;
    return 0;
}
static unsigned amb_texture_count(unsigned id)
{
    unsigned f=amb_texture_family(id);
    return amb_texture_starts[f+1]-amb_texture_starts[f];
}
static unsigned amb_texture_member(unsigned id,unsigned index)
{
    return amb_texture_members[amb_texture_starts[amb_texture_family(id)]+index];
}
static unsigned amb_sound_from_browser(unsigned role,unsigned index)
{
    return role==5?amb_texture_members[amb_texture_starts[index]]:index;
}
