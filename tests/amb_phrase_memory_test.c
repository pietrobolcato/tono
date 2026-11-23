#define AMB_MAIN amb_test_main
#include "amb_native_test.c"
int main(void)
{
    const unsigned pitches[]={60,62,64,65,67,69,71,72};
    ui_power_on();amb_init();amb.key=0;amb.chord=0;amb_pool_set(4095);
    for(unsigned mode=AMB_LOCKED;mode<=AMB_MARKOV;mode++){
        unsigned chosen[96];
        amb.policy.active[1]=mode;
        for(unsigned pass=0;pass<2;pass++){
            memset(amb_memory,0,sizeof amb_memory);
            for(unsigned i=0;i<96;i++){
                unsigned stock=i<16?pitches[i%8]:70;
                unsigned note=amb_remember(0,stock,0);
                if(pass)assert(note==chosen[i]);else chosen[i]=note;
                if(mode==AMB_LOCKED)assert(note==pitches[i%8]);
                if(i<(mode==AMB_MARKOV?16:8))assert(note==stock);
                if(pass)amb_remember(1,61,0);
            }
        }
        if(mode==AMB_DRIFT){unsigned mutations=0;for(unsigned i=16;i<96;i++)mutations+=chosen[i]==70;assert(mutations);}
        if(mode==AMB_MARKOV){unsigned changes=0;for(unsigned i=17;i<96;i++)changes+=chosen[i]!=chosen[i-1];assert(changes>5);}
    }
    for(unsigned id=AMB_SCENE_NIGHT_WIRE;id<=AMB_SCENE_KILN;id++){
        amb_piece_t p;amb_make_scene(&p,id);assert(amb_valid(&p));
        assert(!memcmp(p.policy.active,amb_generate_recipes[5+id-AMB_SCENE_NIGHT_WIRE].values,3));
        for(unsigned k=0;k<4;k++)assert(p.layer[k].generated);
    }
    for(unsigned i=0;i<NELEM(amb_generate_recipes);i++)
        for(unsigned j=0;j<i;j++)
            assert(memcmp(amb_generate_recipes[i].values,amb_generate_recipes[j].values,4));
    unsigned scene=AMB_SCENE_GARDEN;
    do{
        amb_piece_t p;amb_make_scene(&p,scene);
        unsigned found=0;
        for(unsigned i=0;i<NELEM(amb_generate_recipes);i++)
            if(!memcmp(p.policy.active,amb_generate_recipes[i].values,3) &&
               p.policy.variation==amb_generate_recipes[i].values[3])found++;
        assert(found==1);
        scene=amb_next_piece(scene,1);
    }while(scene!=AMB_SCENE_GARDEN);
    for(unsigned i=0;i<amb_pcm_cello_length;i++)
        assert(amb_octet_code(amb_oct_cello,amb_oct_cello_offsets,amb_oct_cello_widths,i,amb_oct_cello_period)==amb_packed_code(amb_pcm_cello,amb_pcm_cello_offsets,amb_pcm_cello_widths,i,amb_pcm_cello_period));
    for(unsigned i=0;i<amb_pcm_flute_length;i++)
        assert(amb_octet_code(amb_oct_flute,amb_oct_flute_offsets,amb_oct_flute_widths,i,amb_oct_flute_period)==amb_packed_code(amb_pcm_flute,amb_pcm_flute_offsets,amb_pcm_flute_widths,i,amb_pcm_flute_period));
    /* Decoded reference fingerprints from the 0.56 factory speech bank. */
    static const uint32_t speech_reference[]={0xd9951794u,0x9b74e45du,0xa7a905afu};
    amb_speech_init();
    for(unsigned n=0;n<3;n++){
        uint32_t hash=2166136261u;
        const amb_recording_t *r=&amb_recordings[13+n];
        for(unsigned b=0;b<(r->length+31)/32;b++){
            const uint8_t *block=r->pcm+b*8;
            amb_seek_t state=speech_seek[speech_offset[n]+b/2];
            if(b%2)state=amb_speech_block(block-8,state,0);
            int16_t out[32];amb_speech_block(block,state,out);
            for(unsigned i=0;i<32;i++)hash=(hash^(uint8_t)(out[i]/256))*16777619u;
        }
        assert(hash==speech_reference[n]);
    }
    puts("Three phrase modes, every factory Generate setup, and lossless strings, flute and speech verified");
}
