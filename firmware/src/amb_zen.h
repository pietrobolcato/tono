/* SPDX-License-Identifier: GPL-3.0-only
 * Render from actual voice allocations and envelopes. The audio ISR only
 * timestamps the note; muted, stolen and delayed notes follow the shared engine. */
typedef struct {unsigned hit,body,note;} zen_layer_t;
static zen_layer_t zen_read_layer(unsigned k)
{
    unsigned body=0;
    for(unsigned i=0;i<NVOICE;i++){
        const voice_t *v=&trk[k].v[i];
        if(v->active && v->env_out>0 && (unsigned)v->env_out>body)body=v->env_out;
    }
    unsigned dt=fm1_ms-zen_event[k].ms;
    unsigned hit=dt<480?zen_event[k].velocity*(480-dt)/480:0;
    if(trk[k].p[P_MUTE] || !amb.layer[k].level)hit=body=0;
    return (zen_layer_t){hit,clamp(body>>10,0,31),zen_event[k].note};
}
static int zen_sin(unsigned phase){return SINE[phase&1023u]>>8;}
static int zen_abs(int x){return x<0?-x:x;}
typedef struct {uint16_t sky,ink,far,near,accent,light;} zen_colour_t;
static int zen_height(int x,unsigned t,int depth,int biome,int open)
{
    unsigned phase=(unsigned)x*(2+depth)+t*(depth+1);
    int wave=zen_sin(phase)+zen_sin(phase*3+123)/3;
    int peak=zen_abs((int)(phase%192u)-96);
    switch(biome){
    case 1:return 95+depth*14-peak*(38-open/3)/(85+depth*12)+wave/20;
    case 3:return 104+depth*14+wave/32;
    case 4:return 96+depth*12+wave/13;
    default:return 95+depth*13+wave*(12-open/4)/(128+depth*16);
    }
}
/* Distance advances with the transport, near objects move faster than the
 * ridge line. Consecutive landscapes interpolate in the last quarter bar group. */
static void zen_train(unsigned time,int open,zen_layer_t l[4],zen_colour_t p)
{
    unsigned biome=(time>>10)%6u,next=(biome+1u)%6u,t=time>>2;
    int fade=clamp((int)(time&1023u)-768,0,255);
    int dark=clamp(44-open*2-zen_sin(time/6)/2,0,100);
    uint16_t sky=ux_mix(p.sky,RGB(19,28,39),dark),land=ux_mix(p.near,RGB(38,47,53),dark/2);
    cv_rect(8,29,224,116,sky);
    for(unsigned j=0;j<(unsigned)dark/3;j++){
        unsigned h=amb_hash(amb.policy.seed+j);
        cv_pset(10+(h%218),31+((h>>12)%49),p.light);
    }
    int sx=118+zen_sin(time/7)/2,sy=49+zen_sin(time/9+190)/12;
    tono_disc(sx,sy,6+open/6,dark>62?p.light:p.accent);
    if(dark>62)tono_disc(sx+3,sy-2,5+open/6,sky);
    for(int j=0;j<3;j++){
        uint16_t c=j==0?ux_mix(p.far,sky,27):j==1?p.far:land;
        for(int x=8;x<232;x+=2){
            int y=zen_height(x,t,j,biome,open);
            y+=(zen_height(x,t,j,next,open)-y)*fade/256;
            y-=l[j].body/7;
            cv_rect(x,y,2,145-y,c);
            if(j==0 && (biome==1 || next==1) && y<79)cv_rect(x,y,2,(80-y)/3+1,p.light);
        }
        if(j==1 && biome==2){
            for(unsigned i=0;i<16;i++){
                unsigned b=t/15+i,h=amb_hash(b+amb.policy.seed);
                int x=10+i*15-t%15,y=zen_height(x,t,1,biome,open),height=8+(int)(h%21);
                if(x<8 || x>221)continue;
                cv_rect(x,y-height,10,height+3,p.ink);
                for(int w=3;w<height-2;w+=5)cv_rect(x+2,y-w,2,1,dark>55?p.light:p.far);
            }
        }
    }
    if(biome==3){
        cv_rect(8,121,224,20,ux_mix(land,sky,45));
        int boat=28+(t%175);
        cv_line(boat,120,boat+13,120,p.ink);cv_line(boat+6,119,boat+6,112,p.ink);
        cv_line_t(boat+6,112,boat+2,117,p.light,2);
    }
    for(unsigned i=0;i<13;i++){
        int x=9+i*18-(time/2)%18;if(x<9 || x>225)continue;
        int y=127+zen_sin(x*4+t*2)/20;
        if(biome==2){cv_rect(x,y,12,2,p.ink);cv_rect(x+2,y,5,1,p.light);}
        if(biome==3){cv_rect(x,125+(i*17)%12,5+i%5,1,p.light);}
        if(biome==4 || (biome==1 && i%3==0)){
            int h=6+(i*3+t/12)%8;
            cv_line_t(x,y,x,y-h,p.ink,2);cv_line(x-3,y-4,x,y-h,p.ink);cv_line(x+3,y-4,x,y-h,p.ink);
        }
        if(biome==5){cv_rect(x,117,3,20,p.ink);cv_line(x,135,x+9,119,p.ink);}
    }
    if(biome==5)cv_rect(8,115,224,4,p.ink);
    const int pole=228-(time%266);
    if(pole>=10 && pole<=226){cv_rect(pole,75,2,69,p.ink);cv_rect(pole-2,78,6,2,p.ink);}
    if(zen_sin(time/4+77)>75+open){
        for(unsigned i=0;i<19;i++){
            unsigned h=amb_hash(i+105);
            int x=12+(h+time/2)%215,y=33+((h>>13)+time)%98;
            cv_line(x,y,x-2,y+5,p.light);
        }
    }
    cv_rect(8,141,224,4,p.ink);
    /* A carriage-window glint per real attack. Pitch sets height and the
     * layer sets its lane, including notes fired between rhythmic grid steps. */
    for(unsigned k=0;k<4;k++)if(l[k].hit>3){
        int x=21+k*52+(l[k].note%12)*2,y=40+(95-clamp(l[k].note,32,95))/3;
        int width=1+l[k].hit/14;cv_rect(x-width,y,2*width+1,1,p.accent);cv_rect(x,y-2,1,5,p.light);
    }
}
static void zen_contour(unsigned time,int open,zen_layer_t l[4],zen_colour_t p)
{
    int cx[4],cy[4],radius[4],weight[4];unsigned ph=time/2;
    for(unsigned k=0;k<4;k++){
        cx[k]=56+zen_sin(ph+k*229)*(34+open/2)/128;
        cy[k]=28+zen_sin(ph/2+k*347)*(18+open/4)/128;
        radius[k]=3+(int)l[k].body/3+(int)l[k].hit/8;
        weight[k]=(int)l[k].note%12;
    }
    /* Each layer owns a spatial band. Even the folded and spiralling fields
     * bend around a struck note and keep widening with its sounding envelope. */
    int bend[112];
    for(int x=0;x<112;x++){
        int displacement=0;
        for(unsigned k=0;k<4;k++){
            int center=12+(int)k*26+weight[k]-5;
            int distance=zen_abs(x-center);
            if(distance<24)displacement+=(24-distance)*((int)l[k].hit/3+(int)l[k].body/2)/32;
        }
        bend[x]=displacement;
    }
    unsigned mode=(time>>9)&3,fade=clamp((int)(time&511)-384,0,127);
    uint16_t tones[6]={p.sky,p.ink,p.far,p.near,p.accent,p.light};
    for(int y=0;y<58;y++){
        uint16_t *row=cv_px+(29+2*y)*240+8;
        int sy=zen_sin(y*9+ph);
        for(int x=0;x<112;x++){
            int field=1000;
            for(unsigned k=0;k<4;k++){
                int dx=x-cx[k],dy=y-cy[k];
                int d=(dx*dx+dy*dy)/(radius[k]+16)-weight[k]/3;
                if(d<field)field=d;
            }
            int sx=zen_sin(x*8+ph+3*bend[x]),r=zen_abs(x-56)+zen_abs(y-28)+bend[x];
            int shape[4]={field,2*y+sx/5+bend[x]+zen_sin(x*17+y*6+ph)/12,
                          r+zen_sin(sy+3*r+sx+ph)/5,sy/4+zen_sin(x*12+sy/2+ph+4*bend[x])/3};
            int value=shape[mode]+(shape[(mode+1)&3]-shape[mode])*(int)fade/128;
            unsigned band=(unsigned)(value+512)*16/(unsigned)(14+open/3);
            uint16_t pixel=swap16(tones[(band&15)<3?(band&32?1:4):((band>>4)%4==2?3:(band>>4)%4==1?2:0)]);
            row[0]=row[1]=row[240]=row[241]=pixel;row+=2;
        }
    }
}
static void amb_draw_zen(void)
{
    zen_layer_t l[4];for(unsigned k=0;k<4;k++)l[k]=zen_read_layer(k);
    int open=clamp(travel.colour/4,-18,18);
    int darkness=clamp(35-3*open,0,96);
    zen_colour_t p={ux_mix(TE_DARK?RGB(64,86,88):ABG,RGB(20,26,32),darkness),
        TE_DARK?TONO_PAPER:TONO_INK,amb_theme()?amb_accent(1):ALINE,
        amb_theme()?ux_mix(amb_accent(1),TONO_INK,52):ADIM,
        amb_theme()?amb_accent(0):TONO_ORANGE,TONO_PAPER};
    unsigned clock=ambr.step?((ambr.step-1)*16+ambr.phase*16/(FS*60u/(unsigned)(amb.global[G_BPM]*4))):0;
    if(ambr.zen_train)zen_train(clock,open,l,p);else zen_contour(clock,open,l,p);
}
