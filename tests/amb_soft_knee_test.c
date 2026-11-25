/* SPDX-License-Identifier: GPL-3.0-only */
#define AMB_MAIN unused_amb_main
#include "amb_native_test.c"

static int expected_transfer(int x,int drive)
{
    int gain=256+(drive*drive>>3);
    int reference=soft_knee(softclip(16*gain),12000);
    return soft_knee(softclip((clamp(x,-65536,65536)*gain)>>8),12000)*
           ((4096<<14)/reference)>>14;
}

int main(void)
{
    memset(amb_nor,255,sizeof amb_nor);ui_power_on();amb_init();amb_fixture_scene(0);
    assert(DT_SOFT==0 && DT_AMP==5 && DT_SOFT_KNEE==6 && TP[P_DTYPE].max==DT_SOFT_KNEE);
    int32_t input[CTL],b[CTL],original_side[CTL];
    for(unsigned i=0;i<CTL;i++)input[i]=((int)i-(int)CTL/2)*889;
    for(unsigned role=0;role<8;role++){
        amb_patch(0,role,0);
        amb_layer_t *l=&amb.layer[0];
        l->base[P_DTYPE]=DT_SOFT_KNEE;l->base[P_DIST]=0;l->macro[0]=126;
        amb_base_revision[0]++;amb_prepare();assert(amb_valid(&amb));
        assert(trk[0].p[P_DIST]==0);dist_w[0]=0;memcpy(b,input,sizeof b);
        track_dist(trk,b,CTL);assert(!memcmp(input,b,sizeof b));
    }
    trk[0].engine=ENGI_ANALOG;trk[0].p[P_DTONE]=0;
    for(int drive=8;drive<=127;drive+=7){
        trk[0].p[P_DIST]=drive;dist_w[0]=4096;memcpy(b,input,sizeof b);
        track_dist(trk,b,CTL);
        for(unsigned i=0;i<CTL;i++)assert(b[i]==expected_transfer(input[i],drive));
        assert(abs(expected_transfer(4096,drive)-4096)<=2);
    }
    assert(amb_monitor_curve(65536)==25635 && amb_monitor_curve(-65536)==-25635);
    assert(abs(expected_transfer(65536,120))<abs(expected_transfer(65536,8)));
    trk[0].engine=ENGI_AMB_A22;trk[0].p[P_DIST]=100;dist_w[0]=4096;
    for(unsigned i=0;i<CTL;i++){
        original_side[i]=((int)i-(int)CTL/4)*399;
        voice_bank[0].a22.side[i]=original_side[i];b[i]=input[i];
    }
    track_dist(trk,b,CTL);
    for(unsigned i=0;i<CTL;i++){
        int l=expected_transfer(input[i]+original_side[i],100);
        int r=expected_transfer(input[i]-original_side[i],100);
        assert(b[i]==((l+r)>>1));
        assert(voice_bank[0].a22.side[i]==((l-r)>>1));
    }
    trk[0].engine=ENGI_DRUM;dx_xon=1;memcpy(b,input,sizeof b);
    for(unsigned i=0;i<CTL;i++){dx_sl[i]=original_side[i];dx_sr[i]=original_side[i]/3;}
    track_dist(trk,b,CTL);
    for(unsigned i=0;i<CTL;i++){
        assert(b[i]-dx_sl[i]==expected_transfer(input[i]-original_side[i],100));
        assert(b[i]-dx_sr[i]==expected_transfer(input[i]-original_side[i]/3,100));
    }
    trk[0].engine=ENGI_AMB_A22;
    trk[0].p[P_DIST]=0;
    for(unsigned j=0;j<32;j++){memcpy(b,input,sizeof b);track_dist(trk,b,CTL);}
    memcpy(b,input,sizeof b);memcpy(voice_bank[0].a22.side,original_side,sizeof b);
    track_dist(trk,b,CTL);assert(!memcmp(input,b,sizeof b));
    assert(!memcmp(voice_bank[0].a22.side,original_side,sizeof b));
    puts("Soft Knee: original IDs, dry eight-role bypass, exact monitor transfer, reference-level compensation, stereo L/R and zero-drive ramp PASS");
}
