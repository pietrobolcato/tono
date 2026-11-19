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
enum { Z_DUNES,Z_ALPS,Z_TOWN,Z_QUAY,Z_SEA,Z_WOODS,Z_BRIDGE,Z_BIOMES };
/* Route coordinates identify a permanent location, including the transition
 * to its neighbour. The same tile owns the same building/tree on every frame. */
static int zen_weight(unsigned world,unsigned biome)
{
    unsigned current=(world>>9)%Z_BIOMES,phase=world&511;
    int into=clamp((int)phase-256,0,255);
    return current==biome?255-into:(current+1)%Z_BIOMES==biome?into:0;
}
static int zen_ridge(unsigned world,int depth,int open,int body)
{
    int wave=zen_sin(world*2)+zen_sin(world*5+83)/3;
    int peak=zen_abs((int)(world%180)-90)+zen_abs((int)(world%63)-31)/2;
    int height=135+depth*26+wave*(14-open/3)/128-body/6;
    /* Convert the scroll coordinates of this parallax plane to route distance. */
    unsigned route=world*(4-depth)/2;
    height-=zen_weight(route,Z_ALPS)*peak*(55-open/2)/(256*90);
    height+=zen_weight(route,Z_SEA)*(160+depth*24-height)/256;
    return height;
}
static void zen_train(unsigned time,int open,zen_layer_t l[4],zen_colour_t p)
{
    unsigned distance=time/2,seed=amb.policy.seed;
    int dark=clamp(44-open*2-zen_sin(time/6)/2,0,100);
    uint16_t sky=ux_mix(p.sky,RGB(19,28,39),dark),land=ux_mix(p.near,RGB(38,47,53),dark/2);
    uint16_t haze=ux_mix(p.far,sky,44),water=ux_mix(land,sky,66),silhouette=ux_mix(TONO_INK,land,22);
    cv_rect(0,0,240,240,sky);
    for(unsigned i=0;i<32;i++){
        unsigned h=amb_hash(seed+i);
        if(dark>(int)(h%92))cv_pset((h>>8)%240,5+((h>>18)%109),ux_mix(sky,p.light,dark));
    }
    int sx=125+zen_sin(time/6)/2,sy=72+zen_sin(time/9+190)/5;
    tono_disc(sx,sy,10+open/6,ux_mix(p.accent,p.light,dark));
    if(dark>35)tono_disc(sx+15-(dark-35)/7,sy-4,8+open/6,sky);
    for(unsigned i=0;i<6;i++){
        unsigned tile=i+distance/4/62,h=amb_hash(seed+tile);
        int x=(int)(i*62)-(int)(distance/4%62)-31,y=30+(int)(h%48);
        cv_rect(x,y,17+(h%20),2,haze);cv_rect(x+8,y-2,12+(h%9),2,haze);
    }
    for(int j=0;j<3;j++){
        unsigned scroll=distance*2/(4-j);
        uint16_t colour=j==0?haze:j==1?p.far:land;
        for(int x=0;x<240;x+=2){
            unsigned world=scroll+x,route=world*(4-j)/2;
            int y=zen_ridge(world,j,open,l[j].body);
            cv_rect(x,y,2,240-y,colour);
            if(!j && y<117 && zen_weight(route,Z_ALPS)>128)cv_rect(x,y,2,(118-y)/4+1,p.light);
            if(j==1){
                int settlement=zen_weight(route,Z_TOWN)+zen_weight(route,Z_QUAY);
                if(settlement>32)cv_rect(x,y+3,2,1+settlement/74,silhouette);
            }
            if(j==2){
                int bay=zen_weight(route,Z_QUAY)/2+zen_weight(route,Z_SEA);
                int shore=y+(255-bay)/6;
                if(bay){cv_rect(x,shore,2,bay/6,water);
                    if((world/6)%5==0)cv_rect(x,shore+bay/17,2,1,p.light);}
                int road=zen_weight(route,Z_TOWN)+zen_weight(route,Z_QUAY);
                if(road){int level=207+zen_sin(world*3)/20;
                    cv_rect(x,level,2,road/25,silhouette);
                    if((world&31)<14 && road>64)cv_rect(x,level+road/54,2,1,p.light);}
                int bridge=zen_weight(route,Z_BRIDGE);
                if(bridge>8){
                    int deck=204+(255-bridge)/7;
                    cv_rect(x,deck,2,3,silhouette);
                    if(world%43<4)cv_rect(x,deck,2,240-deck,silhouette);
                }
            }
        }
        if(!j)continue;
        for(unsigned i=0;i<12;i++){
            unsigned tile=i+scroll/28,world=tile*28,route=world*(4-j)/2,h=amb_hash(seed+tile+311u*j);
            int x=(int)(i*28)-(int)(scroll%28),y=zen_ridge(world,j,open,l[j].body);
            int town=zen_weight(route,Z_TOWN),quay=zen_weight(route,Z_QUAY),sea=zen_weight(route,Z_SEA);
            if(j==1 && town+quay/2>(int)(h%160)){
                int height=6+(int)((h>>9)%25)*town/255;
                cv_rect(x,y-height,13+(h%9),height+7,silhouette);
                if(h&4)cv_line_t(x,y-height,x+6,y-height-4,silhouette,2);
                for(int w=4;w<height;w+=5){
                    cv_rect(x+3,y-w,3,2,dark>55?p.light:haze);
                    if(h&1)cv_rect(x+9,y-w,2,2,haze);
                }
            }
            if(j!=2)continue;
            if(quay>(int)(h%200)){
                int py=y+8;
                cv_rect(x,py,21,3,silhouette);cv_rect(x+3,py,2,9,silhouette);
                cv_rect(x+16,py,2,8,silhouette);
                if(h&1){cv_rect(x+8,py-15,2,15,silhouette);cv_line(x+8,py-15,x+22,py-20,silhouette);}
            }else if(sea>(int)(h%224) && (h&3)==0){
                int py=y+(255-sea)/6+sea/11;
                cv_line(x,py,x+15,py,silhouette);cv_line(x+7,py-2,x+7,py-14,silhouette);
                cv_line_t(x+5,py-11,x+2,py-4,p.light,2);
            }else if(zen_weight(route,Z_WOODS)+zen_weight(route,Z_ALPS)/3>(int)(h%223)){
                int height=9+(h>>12)%20;
                cv_rect(x,y-height,2,height,silhouette);
                for(int w=0;w<height-5;w+=2)cv_rect(x-w/4,y-height+w,1+w/2,2,silhouette);
            }
        }
    }
    int pole=264-(int)(time%300);
    cv_rect(pole,85,3,151,silhouette);cv_rect(pole-4,89,12,3,silhouette);
    cv_line(pole-300,89,pole-150,103,silhouette);cv_line(pole-150,103,pole,89,silhouette);
    cv_line(pole,89,pole+150,103,silhouette);cv_line(pole+150,103,pole+300,89,silhouette);
    int rain=clamp((zen_sin(time/4+77)-55-open)/2,0,31);
    for(int i=0;i<rain;i++){unsigned h=amb_hash((unsigned)i+105);
        int x=(h+time/2)%245,y=(h/256+time)%248-8;cv_line(x,y,x-3,y+7,p.light);}
    cv_rect(0,235,240,5,silhouette);
    for(unsigned k=0;k<4;k++)if(l[k].hit>3){
        int x=21+k*52+(l[k].note%12)*2,y=56+(95-clamp(l[k].note,32,95))/2;
        int width=1+l[k].hit/14;cv_rect(x-width,y,2*width+1,1,p.accent);cv_rect(x,y-2,1,5,p.light);
    }
}
static void zen_contour(unsigned time,int open,zen_layer_t l[4],zen_colour_t p)
{
    int cx[4],cy[4],inverse_radius[4],weight[4];unsigned ph=time/2;
    unsigned scale=65536u/(14+open/3);
    for(unsigned k=0;k<4;k++){
        cx[k]=60+zen_sin(ph+k*229)*(34+open/2)/128;
        cy[k]=60+zen_sin(ph/2+k*347)*(38+open/2)/128;
        inverse_radius[k]=65536/(19+(int)l[k].body/3+(int)l[k].hit/8);
        weight[k]=(int)l[k].note%12;
    }
    /* Each layer owns a spatial band. Even the folded and spiralling fields
     * bend around a struck note and keep widening with its sounding envelope. */
    int bend[120];
    for(int x=0;x<120;x++){
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
    for(int y=-cv_oy/2;y<(120-cv_oy)/2;y++){
        uint16_t *row=cv_px+(cv_oy+2*y)*240;
        int sy=zen_sin(y*9+ph);
        for(int x=0;x<120;x++){
            int field=1000;
            for(unsigned k=0;k<4;k++){
                int dx=x-cx[k],dy=y-cy[k];
                int d=((dx*dx+dy*dy)*inverse_radius[k]>>16)-weight[k]/3;
                if(d<field)field=d;
            }
            int sx=zen_sin(x*8+ph+3*bend[x]),r=zen_abs(x-60)+zen_abs(y-60)+bend[x];
            int shape[4]={field,2*y+sx/5+bend[x]+zen_sin(x*17+y*6+ph)/12,
                          r+zen_sin(sy+3*r+sx+ph)/5,sy/4+zen_sin(x*12+sy/2+ph+4*bend[x])/3};
            int value=shape[mode]+(shape[(mode+1)&3]-shape[mode])*(int)fade/128;
            unsigned band=(unsigned)(value+512)*scale>>12;
            uint16_t pixel=swap16(tones[(band&15)<3?(band&32?1:4):((band>>4)%4==2?3:(band>>4)%4==1?2:0)]);
            row[0]=row[1]=row[240]=row[241]=pixel;row+=2;
        }
    }
}
/* Preserve the actual scenery through the counters. Use spare space beyond
 * this 120-row strip for a tiny copy, never a second framebuffer. */
static void zen_wordmark(void)
{
    enum {W=42,H=13,X=6,Y=5};
    _Static_assert(CV_MAX>=240*120+W*H,"wordmark scratch fits the canvas");
    uint16_t *under=cv_px+240*120;
    unsigned light=0;
    for(int y=0;y<H;y++)for(int x=0;x<W;x++){
        uint16_t *pixel=&cv_px[(Y+y)*240+X+x],c=swap16(*pixel);
        under[y*W+x]=*pixel;*pixel=0;
        light+=((c>>11)*2+((c>>5)&63)*3+(c&31));
    }
    uint16_t ink=swap16(light>W*H*125?TONO_INK:TONO_PAPER);
    tono_logo(X+1,Y+1,3,0,0xffff);
    for(int y=0;y<H;y++)for(int x=0;x<W;x++){
        uint16_t *pixel=&cv_px[(Y+y)*240+X+x];
        *pixel=*pixel?ink:under[y*W+x];
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
    for(int top=0;top<240;top+=120){
        cv_begin(240,120,p.sky);cv_oy=-top;
        if(ambr.zen_train)zen_train(clock,open,l,p);else zen_contour(clock,open,l,p);
        cv_oy=0;
        if(!top)zen_wordmark();
        cv_blit_cells(0,top);
    }
}
