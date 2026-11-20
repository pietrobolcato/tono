/* SPDX-License-Identifier: GPL-3.0-only */
#define AMB_MAIN unused_amb_main
#include "amb_native_test.c"

static int in_pool(unsigned note)
{
    return (amb_pool() >> ((note+12-amb.key)%12))&1u;
}

/* Garden schedules free-running breaths and delayed strikes in audio frames.
 * A sixteenth-only dry run cannot count them. Observe actual native voice
 * starts after each mixer block, including short drum hits between beats. */
static void garden_catalog(unsigned scene)
{
    uint32_t age[4][NVOICE]={{0}};
    unsigned events[4]={0};
    int32_t output[2*CTL];
    seq_start();
    while(ambr.step<1024 || !ambr.last_play){
        mix_block(output,CTL);
        for(unsigned k=0;k<4;k++)for(unsigned v=0;v<NVOICE;v++){
            uint32_t current=trk[k].v[v].age;
            if(current!=age[k][v]){events[k]++;age[k][v]=current;}
        }
    }
    seq_stop();amb_render(CTL);
    printf("%s: actual layer starts in 64 bars: %u %u %u %u\n",AMB_SCENE[scene],
           events[0],events[1],events[2],events[3]);
    /* The pedal changes with the 28–51 second drift or its optional fifth. */
    for(unsigned k=0;k<4;k++)assert(events[k]);
    if(scene==AMB_SCENE_GARDEN)assert(events[0]>20 && events[1]>20 && events[2]>=2 && events[2]<24 && events[3]>100);
}

static void catalog(void)
{
    assert(NELEM(AMB_SCENE)>=17);
    for(unsigned scene=0;scene<NELEM(AMB_SCENE);scene++){
        amb_fixture_scene(scene);
        unsigned events[4]={0},distinct=0,early=0;
        uint8_t phrases[32][32]={{0}};
        for(unsigned k=0;k<4;k++){
            assert(amb.layer[k].generated);
            trk[k].engine=amb.layer[k].engine;
        }
        if(amb.policy.active[0]==8){garden_catalog(scene);continue;}
        /* The actual sequencer writes its emitted pitched notes to PATTERN. */
        for(unsigned step=0;step<1024;step++){
            amb_participation_t parts[4];amb_participation(parts,step);
            unsigned mask=amb_policy_events(&amb.policy,step,parts);
            amb_step();
            for(unsigned k=0;k<4;k++){
                amb_layer_t *l=&amb.layer[k];
                if(l->role==4){events[k]+=(mask>>k)&1;continue;}
                unsigned stride=1u<<l->speed;
                if(step%stride)continue;
                unsigned note=ambr.recent[k][(step/stride)%16];
                if(!note)continue;
                events[k]++;
                if(!k){phrases[step/32][step%32]=note;early+=step<128;}
            }
        }
        for(unsigned i=0;i<32;i++){
            unsigned j=0;while(j<i && memcmp(phrases[j],phrases[i],32))j++;
            distinct+=j==i;
        }
        printf("%s: events in 64 bars %u %u %u %u, lead phrases %u/32, first 8 bars %u\n",
            AMB_SCENE[scene],events[0],events[1],events[2],events[3],distinct,early);
        for(unsigned k=0;k<4;k++)assert(events[k]);
        if(scene==6 || scene==7 || scene==10){assert(distinct>=12 && early>=5);}
    }
}

/* Count changes in the actual gated lead oscillator during real playback.
 * Rests, note length and reverb alone cannot make two pitch lines distinct. */
static void sounding_lead(unsigned scene, unsigned min_changes, unsigned min_lines)
{
    amb_fixture_scene(scene);seq_start();
    uint32_t lines[12]={0};unsigned last=255,changes=0,distinct=0;
    int32_t out[CTL*2];
    unsigned frames=48*4*FS*60u/(unsigned)amb.global[G_BPM];
    for(unsigned at=0;at<frames;at+=CTL){
        mix_block(out,CTL);
        unsigned group=ambr.step?(ambr.step-1)/64:0;
        if(group>=12)break;
        for(unsigned v=0;v<NVOICE;v++){
            const voice_t *voice=&trk[0].v[v];
            if(!voice->active || !voice->gate || voice->note==last)continue;
            last=voice->note;assert(in_pool(last));changes++;
            lines[group]=lines[group]*16777619u+last;
        }
    }
    seq_stop();amb_render(FS);
    for(unsigned i=0;i<12;i++){
        unsigned j=0;while(j<i && lines[j]!=lines[i])j++;
        if(lines[i] && j==i)distinct++;
    }
    printf("%s: %u sounding pitch changes, %u distinct nonempty four-bar pitch lines in 48 bars\n",
        AMB_SCENE[scene],changes,distinct);
    assert(changes>=min_changes && distinct>=min_lines);
}
int main(void)
{
    memset(amb_nor,255,sizeof amb_nor);ui_power_on();amb_init();catalog();
    sounding_lead(6,30,8);sounding_lead(7,30,8);sounding_lead(10,30,8);
    amb_fixture_scene(AMB_SCENE_LAST_TRAM);
    /* The real strikes and reeds must use their recorded decay and attack
     * while the sequencer still supplies both parts. */
    for(unsigned k=0;k<2;k++){
        assert(amb.layer[k].engine==ENGI_AMB_SAMPLE && amb.layer[k].generated);
        assert(!amb.layer[k].base[P_E3]);
    }
    /* Leave room for the piano answers and the rests between calls. */
    sounding_lead(AMB_SCENE_LAST_TRAM,20,6);
    amb_fixture_scene(7);
    uint8_t notes[2];unsigned gate;
    for(unsigned key=0;key<12;key++)for(unsigned chord=0;chord<7;chord++){
        amb.key=key;amb.chord=chord;amb_pool_set(0x493);amb_revoice();
        for(unsigned step=0;step<256;step+=2){
            assert(amb_generated_notes(0,step,1,notes,&gate)==1);
            assert(in_pool(notes[0]) && notes[0]>=48 && notes[0]<=84 && gate);
        }
        amb.layer[2].follow=0;
        assert(amb_generated_notes(2,0,4,notes,&gate)==1 && in_pool(notes[0]));
    }
    amb_fixture_scene(7);song.playing=0;ambr.view=0;ambr.piece_preview=7;amb_rec();
    uint32_t seed=amb.policy.seed;ambr.piece_preview=7;amb_rec();assert(amb.policy.seed!=seed);
    amb_target(1);press(B_SEQ);press(B_REC);assert(!amb.layer[0].generated);
    unsigned n=0;for(unsigned i=0;i<16;i++)n+=amb.layer[0].notes[i]!=0;assert(n);
    amb.layer[0].notes[4]=61;press(B_REC);amb_layer_t saved=amb.layer[0];
    press(B_EDIT);assert(amb.layer[0].generated);press(B_EDIT);assert(!memcmp(&saved,&amb.layer[0],sizeof saved));
    /* A pre-update manual Stone Cloister used CHORD mode. Keep its literal
     * off-scale edit and its saved seed on recall through the user slot. */
    amb.layer[0].mode=3;saved=amb.layer[0];seed=amb.policy.seed;
    ambr.view=3;ambr.slot=0;amb_save();amb_fixture_scene(6);
    assert(amb_recall(0) && amb.policy.seed==seed && !memcmp(&saved,&amb.layer[0],sizeof saved));
    puts("Generative catalog, modal pool, fresh takes, manual capture and recall PASS");
    return 0;
}
