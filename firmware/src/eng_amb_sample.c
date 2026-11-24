/* SPDX-License-Identifier: GPL-3.0-only */
/* Bounded read-only PCM playback; sources are original offline examples.
 * Uses voice-local existing phase/filter state, no sample RAM or flash writes. */
#include "amb_pcm_octets.h"
#include "amb_acoustic_pcm.h"
#include "amb_texture_pcm.h"
#include "amb_speech.h"
#include "amb_field_pcm.h"
#include "amb_take.h"
enum { AMB_TEXTURE_SOURCE = 7, AMB_FIELD_SOURCE = AMB_MIC_SOURCE+1 };
static const char *const N_AS_SOURCE[]={"SOFT CHIME","TAPE VOWEL","SOLO VIOLIN","BOWED CELLO","KAWAI GRAND","TENOR SAX","CONCERT FLUTE","OCEAN DRUM","CABASA RUB","GUIRO SCRAPE","SEED SHAKER","RATCHET","BOWED CYMBAL","EAGLE LANDED","RADIO CHECK","MOON SPEECH","MIC TAKE","KIEL HARBOUR","SUBWAY VOICE"};
typedef struct {const uint8_t *pcm;unsigned length,root,rate_q16,loop_percent;} amb_recording_t;
static const amb_recording_t amb_recordings[]={
    {amb_oct_chime,amb_oct_chime_length,60,2972,24},{amb_oct_vowel,amb_oct_vowel_length,60,2972,24},
    {amb_oct_violin,amb_oct_violin_length,60,14861,29},
    {amb_oct_cello,amb_oct_cello_length,60,17833,12},
    {amb_pcm_piano,amb_pcm_piano_length,72,17833,29},
    {amb_oct_tenor,amb_oct_tenor_length,72,14861,29},
    {amb_oct_flute,amb_oct_flute_length,72,14861,15},
    /* Six phrase maps, each addressing grains of its own percussion take. */
    {amb_texture_frames[0],24576,60,8025,9},{amb_texture_frames[1],24576,60,8025,11},
    {amb_texture_frames[2],24576,60,8025,7},{amb_texture_frames[3],24576,60,8025,13},
    {amb_texture_frames[4],24576,60,8025,8},{amb_texture_frames[5],24576,60,8025,12},
    {speech_eagle,SPEECH_EAGLE_FRAMES,60,7280,0},{speech_radio,SPEECH_RADIO_FRAMES,60,7280,0},
    {speech_moon,SPEECH_MOON_FRAMES,60,7280,0},{0,0,60,7280,12},
    {field_harbour,field_harbour_frames,60,field_harbour_rate,0},
    {field_subway,field_subway_frames,60,field_subway_rate,0}
};
/* One exact ADPCM seek point per 64 frames. Sequential playback also reuses
 * the state at the end of the even block for its neighbouring odd block. */
#define SPEECH_SEEKS(pcm) ((sizeof(pcm)/8+1)/2)
typedef struct { int8_t pred; uint8_t step; } amb_seek_t;
static amb_seek_t speech_seek[SPEECH_SEEKS(speech_eagle)+SPEECH_SEEKS(speech_radio)+SPEECH_SEEKS(speech_moon)];
static const unsigned speech_offset[]={0,SPEECH_SEEKS(speech_eagle),SPEECH_SEEKS(speech_eagle)+SPEECH_SEEKS(speech_radio)};
static amb_seek_t amb_speech_block(const uint8_t *block,amb_seek_t state,int16_t *out)
{
    int pred=state.pred,step=state.step;
    for(unsigned i=0;i<32;i++){
        int value=amb_take_delta((block[i/4]>>(2*(i%4)))&3,&pred,&step);
        if(out)out[i]=(int16_t)(256*value);
    }
    return (amb_seek_t){pred,step};
}
static void amb_sample_seek_init(void)
{
    AMB_INIT_OFFSETS(flute);
    AMB_INIT_OFFSETS(violin);
    for(unsigned n=0;n<3;n++){
        const amb_recording_t *r=&amb_recordings[13+n];
        amb_seek_t state={0,1};
        for(unsigned b=0;b<(r->length+31)/32;b++){
            if(!(b&1))speech_seek[speech_offset[n]+b/2]=state;
            state=amb_speech_block(r->pcm+8*b,state,0);
        }
    }
}
/* Direct lookup supports arbitrary pitch and sample trims without a decode buffer. */
static inline int32_t amb_pcm_at(const uint8_t *pcm,unsigned at)
{
    return amb_pcm_decode[pcm[at]];
}
/* Sixteen samples spaced one period apart share a base and an unsigned
 * bit width. The two adjacent offsets encode the width as 1 + 2*width
 * bytes per group. A lookup never decodes preceding audio. */
static inline unsigned amb_packed_code(const uint8_t *data,const uint16_t *offsets,
                                       const uint32_t *widths,unsigned at,unsigned period)
{
    unsigned cycle=at/period;
    unsigned group=(cycle>>4)*period+(at-cycle*period);
    unsigned slot=group&7u, word=widths[group>>3];
    unsigned width=(word>>(4*slot))&15u;
    unsigned before=word & ~(~0u<<(4*slot));
    unsigned pairs=(before&0x0f0f0f0fu)+((before>>4)&0x0f0f0f0fu);
    unsigned offset=offsets[group>>3]+slot+2*((pairs*0x01010101u)>>24);
    unsigned bit=(cycle&15)*width;
    const uint8_t *bytes=data+offset+1+(bit>>3);
    unsigned bits=bytes[0] | ((unsigned)bytes[1]<<8);
    return data[offset]+((bits>>(bit&7))&((1u<<width)-1));
}
static int amb_recorded_at(unsigned source,const uint8_t *src,unsigned at)
{
    unsigned code;
    switch(source){
    case 0: code=amb_octet_code(src,amb_oct_chime_offsets,amb_oct_chime_widths,at,amb_oct_chime_period);break;
    case 1: code=amb_octet_code(src,amb_oct_vowel_offsets,amb_oct_vowel_widths,at,amb_oct_vowel_period);break;
    case 2: code=amb_octet_code(src,amb_oct_violin_offsets,amb_oct_violin_widths,at,amb_oct_violin_period);break;
    case 3: code=amb_octet_code(src,amb_oct_cello_offsets,amb_oct_cello_widths,at,amb_oct_cello_period);break;
    case 4: code=amb_packed_code(src,amb_pcm_piano_offsets,amb_pcm_piano_widths,at,amb_pcm_piano_period);break;
    case 5: code=amb_octet_code(src,amb_oct_tenor_offsets,amb_oct_tenor_widths,at,amb_oct_tenor_period);break;
    case 6: code=amb_octet_code(src,amb_oct_flute_offsets,amb_oct_flute_widths,at,amb_oct_flute_period);break;
    default: return amb_pcm_at(src,at);
    }
    return amb_pcm_decode[code];
}
static inline int amb_texture_point(unsigned bank,unsigned code,unsigned pos)
{
    if(code&64)pos=127-pos;
    unsigned byte=amb_texture_grains[bank][((code>>4)&3)*64+(pos>>1)];
    int x=amb_texture_levels[(pos&1)?(byte>>4):(byte&15)]*(int)(code&15)*11;
    return code&128?-x:x;
}
static __attribute__((noinline)) int amb_texture_read(unsigned bank,unsigned at)
{
    unsigned block=at>>7,pos=at&127,code=amb_texture_frames[bank][block];
    int x=amb_texture_point(bank,code,pos);
    if(pos<6 && block){
        int last=amb_texture_point(bank,amb_texture_frames[bank][block-1],127);
        x=last+(x-last)*(int)(pos+1)/7;
    }
    return x;
}
/* A group directory permits fast seeks in either direction. Quiet background
 * frames carry 16 samples, the foreground carries all 64. */
static void amb_field_decode(unsigned bank,const uint8_t *src,unsigned id,int16_t out[32])
{
    static const uint16_t *const offsets[]={field_harbour_offs,field_subway_offs};
    static const uint16_t *const flags[]={field_harbour_flags,field_subway_flags};
    unsigned group=id>>5,within=(id>>1)&15,mask=flags[bank][group];
    unsigned previous=mask&((1u<<within)-1u);
    previous-=((previous>>1)&0x5555u);
    previous=(previous&0x3333u)+((previous>>2)&0x3333u);
    previous=(previous+(previous>>4))&0x0f0fu;
    unsigned count=(previous+(previous>>8))&31u;
    unsigned full=(mask>>within)&1u,wide=bank==1?2:3;
    unsigned bits=full?wide:1, magnitude=(1u<<(bits-1))-1u;
    const uint8_t *block=src+offsets[bank][group]+within*4+(wide*8-2)*count;
    int value=(int8_t)block[0]*256,old=value,step=field_scale[block[1]&63];
    unsigned mode=block[1]>>6,start=(id&1)*32,end=full?start+32:16;
    int16_t low[16];
    for(unsigned i=0;i<end;i++){
        unsigned bit=i*bits, byte=bit>>3;
        unsigned code=block[2+byte]>>(bit&7);
        if((bit&7)+bits>8)code|=(unsigned)block[3+byte]<<(8-(bit&7));
        int pred=mode==3?value:mode==2?(28*value-13*old)>>4:mode==1?value>>1:0;
        int delta=(1+2*(int)(code&magnitude))*step;
        old=value;value=clamp(pred+((code&(magnitude+1))?-delta:delta),-32767,32767);
        if(!full)low[i]=value;
        else if(i>=start)out[i-start]=value;
    }
    if(!full)for(unsigned j=0;j<32;j++){
        unsigned i=start+j,k=i>>2;int a=low[k],b=low[k+(k<15)];
        out[j]=a+((b-a)*(int)(i&3))/4;
    }
}
static const char *const N_AS_STYLE[]={"PAD","DRONE","TEXTURE","ONE-SHOT","ORIGINAL"};
static const char *const N_AS_LOOP[]={"OFF","FORWARD","REVERSE","BOUNCE"};
static void amb_sample_on(track_t *t,voice_t *v)
{
    unsigned src=clamp(t->p[P_E0],0,NELEM(amb_recordings)-1);
    v->s[2]=take_active;v->s[3]=take_generation[take_active];
    unsigned len=src==AMB_MIC_SOURCE?amb_current_take()->length:amb_recordings[src].length;
    v->ph[0]=(uint32_t)clamp(t->p[P_E4],0,99)*len/100u<<16;
    v->s[0]=0;v->s[1]=0;v->s[4]=0;
}
static void amb_sample_render(track_t *t,voice_t *v,int32_t *out,uint32_t n,const vmod_t *m)
{
    unsigned source=clamp(t->p[P_E0],0,NELEM(amb_recordings)-1);
    const amb_recording_t *recording=&amb_recordings[source];
    const uint8_t *src=recording->pcm;
    const amb_take_t *take=&take_banks[v->s[2]&1];
    unsigned size=recording->length;
    if(source==AMB_MIC_SOURCE){
        if(v->s[3]!=take_generation[v->s[2]&1] || !take->length){v->active=0;return;}
        size=take->length;
    }
    unsigned start=(unsigned)clamp(t->p[P_E4],0,99)*size/100u;
    unsigned end=(unsigned)clamp(t->p[P_E5],1,100)*size/100u;
    unsigned ph=v->ph[0],i,style=(unsigned)t->p[P_E1];
    unsigned length,fade,inc;int pitch=clamp(m->pitch16+t->p[P_E2]*16,24*16,96*16);
    if(end<=start+64) end=start+64;
    if(end>size) {end=size;start=end-64;}
    length=end-start;
    fade=(unsigned)(t->p[P_E6]+t->p[P_LN6])*length/600u+8u;
    if(fade>length/3)fade=length/3;
    unsigned loop=start+(length*recording->loop_percent)/100;
    if(fade>(end-loop)/3)fade=(end-loop)/3;
    inc=((pitch_inc((unsigned)pitch)>>12)*recording->rate_q16)/(pitch_inc(recording->root*16)>>12);
    if(source==AMB_MIC_SOURCE && take->period_q8)
        inc=(unsigned)(((uint64_t)pitch_inc((unsigned)pitch)*take->period_q8)>>24);
    if(style==4)inc=((pitch_inc((unsigned)(60+t->p[P_E2])*16)>>12)*recording->rate_q16)/(pitch_inc(60*16)>>12);
    if(style==2)inc=inc*(990u+(unsigned)((t->lfo_ph>>24)&31))/1000u;
    unsigned repeat=t->p[P_E3];
    unsigned block_id[2]={~0u,~0u};int16_t decoded[2][32];amb_seek_t even_end={0,1};
    unsigned cached=~0u;int32_t first=0,second=0,overlap=0;
    for(i=0;i<n;i++) {
        unsigned at=ph>>16,frac=(ph>>1)&32767u;int32_t x,b;
        if(at<start || at>=end) {
            if(style==3 || !t->p[P_E3]) {v->active=0;break;}
            unsigned resume=repeat==3?start:loop+fade;
            if(repeat==3){v->s[4]^=1;v->s[1]=1;}
            cached=~0u;
            ph=(resume<<16)+(ph-(end<<16))%((end-resume)<<16);at=ph>>16;
        }
        if(cached!=at){
            unsigned following=at+1<end?at+1:(repeat==3?at:start);
            unsigned tail=at>=end-fade?loop+at-(end-fade):at,read=at;
            if(repeat==2 || (repeat==3 && v->s[4])){
                read=end-1-(at-start);following=end-1-(following-start);tail=end-1-(tail-start);
            }
            if(source>=13){
                unsigned positions[3]={read,following,tail};
                int32_t values[3];
                for(unsigned j=0;j<(at>=end-fade?3u:2u);j++){
                    unsigned pos=positions[j];
                    unsigned id=pos/32,slot=id&1;
                    if(block_id[slot]!=id){
                        if(source>=AMB_FIELD_SOURCE)amb_field_decode(source-AMB_FIELD_SOURCE,src,id,decoded[slot]);
                        else if(source==AMB_MIC_SOURCE)amb_take_decode(take->block[id],decoded[slot]);
                        else {
                            const uint8_t *block=src+8*id;
                            amb_seek_t state=speech_seek[speech_offset[source-13]+id/2];
                            if(slot)state=block_id[0]==id-1?even_end:amb_speech_block(block-8,state,0);
                            state=amb_speech_block(block,state,decoded[slot]);
                            if(!slot)even_end=state;
                        }
                        block_id[slot]=id;
                    }
                    values[j]=decoded[slot][pos%32];
                }
                first=values[0];second=values[1];if(at>=end-fade)overlap=values[2];
            }else if(source>=AMB_TEXTURE_SOURCE){
                unsigned bank=source-AMB_TEXTURE_SOURCE;
                first=amb_texture_read(bank,read);second=amb_texture_read(bank,following);
                if(at>=end-fade)overlap=amb_texture_read(bank,tail);
            }else{
                first=cached!=~0u && at==cached+1?second:amb_recorded_at(source,src,read);
                second=amb_recorded_at(source,src,following);
                if(at>=end-fade)overlap=amb_recorded_at(source,src,tail);
            }
            cached=at;
        }
        x=first;b=second;x+=((b-x)*(int32_t)frac)>>15;
        if(at>=end-fade && style!=3 && repeat && repeat!=3) {
            unsigned d=at-(end-fade);
            x=(x*(int32_t)(fade-d)+overlap*(int32_t)d)/(int32_t)fade;
        }
        if(at-start<8u && (repeat!=3 || !v->s[1]))x=x*(int32_t)(at-start)/8;
        if((style==3 || !t->p[P_E3]) && end-at<fade)x=x*(int32_t)(end-at)/(int32_t)fade;
        v->s[0]+=(x-v->s[0])*(t->p[P_E7]+8)/160;
        x=v->s[0]*t->p[P_LN7]/100;
        out[i]+=voice_amp(x,m,i);
        ph+=inc;
    }
    v->ph[0]=ph;
    if(!m->amp0 && !m->amp1 && !v->gate)v->active=0;
}
static const preset_t AMB_SAMPLE_PRESETS[]={
 {"SOFT CHIME",{0,0,0,1,0,100,40,85},{65,90,100,105},0,TAG(PAD),FX(0,25,35,80),0,0},
 {"VIOLIN",{2,0,0,1,0,100,24,110},{22,64,115,65},0,TAG(PAD),FX(0,6,8,38),0,0},
 {"CELLO",{3,0,0,1,0,100,24,108},{24,65,115,74},0,TAG(PAD),FX(0,4,8,36),0,0},
 {"GRAND",{4,3,0,0,0,100,8,122},{0,112,45,70},0,TAG(PAD),FX(0,0,6,35),0,0},
 {"TENOR",{5,3,0,0,0,100,8,127},{1,65,108,56},0,TAG(PAD),FX(0,0,12,32),0,0},
 {"FLUTE",{6,0,0,1,0,100,28,120},{15,62,110,61},0,TAG(PAD),FX(0,0,16,42),0,0}
};
static const engine_t ENG_AMB_SAMPLE={
 .name="SAMPLE",.page_title={"SOURCE","PLAYBACK"},
 .edit={{"SOURCE",F_ENUM,0,NELEM(N_AS_SOURCE)-1,0,N_AS_SOURCE,0},{"STYLE",F_ENUM,0,4,0,N_AS_STYLE,0},
        {"PITCH",F_SEMI,-24,24,0,0,0},{"LOOP",F_ENUM,0,3,1,N_AS_LOOP,0},
        {"START",F_PCT,0,99,0,0,0},{"END",F_PCT,1,100,100,0,0},
        {"XFADE",F_PCT,0,100,40,0,0},{"COLOR",F_PCT,0,127,85,0,0}},
 .presets=AMB_SAMPLE_PRESETS,.npresets=NELEM(AMB_SAMPLE_PRESETS),.knob={P_E2,P_E6,P_LD_PIT,P_REV},
 .poly=2,.sampled=1,.note_on=amb_sample_on,.render=amb_sample_render
};
