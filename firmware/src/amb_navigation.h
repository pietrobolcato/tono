/* SPDX-License-Identifier: GPL-3.0-only */
/* Keep each control and its action together. A second row carries navigation when needed. */
static int amb_nav_cap(int y,int x,const char *name,int knob,int lit)
{
    int width=text_w(&AF_TONO_NAV,name)+(knob?21:6);
    uint16_t fg=lit?TONO_INK:AINK;
    cv_bg=lit?amb_primary_accent():ABG;
    tono_round(x,y,width,13,1,lit?cv_bg:ADIM);
    if(!lit)tono_round(x+1,y+1,width-2,11,1,ABG);
    if(knob){
        tono_disc(x+8,y+6,4,fg);tono_disc(x+8,y+6,3,cv_bg);cv_line(x+8,y+6,x+10,y+4,fg);
    }
    cv_text(x+(knob?16:3),y+4-AF_TONO_NAV_CAP_Y,&AF_TONO_NAV,name,fg);cv_bg=ABG;
    return x+width+2;
}
static int amb_nav_item(int y,int x,const char *key,const char *verb,int knob,int lit)
{
    if(x>8){cv_rect(x,y+6,2,2,ADIM);x+=5;}
    x=amb_nav_cap(y,x,key,knob,lit);
    cv_text(x,y+4-AF_TONO_NAV_CAP_Y,&AF_TONO_NAV,verb,AINK);return x+text_w(&AF_TONO_NAV,verb)+3;
}
/* Width advanced by amb_nav_item, including a separator before the item. */
static int amb_nav_width(const char *key,const char *verb,int knob)
{
    return text_w(&AF_TONO_NAV,key)+text_w(&AF_TONO_NAV,verb)+(knob?31:16);
}
static __attribute__((noinline)) void amb_footer(const amb_layer_t *l)
{
    const char *key="SEL",*verb="Page",*second=0,*action=0;
    int knob=0,lit=0,second_knob=0,cancel=0;
    const char *page=0;
    char counter[]="Page 1/1";
    if(ambr.help){
        unsigned count=amb_help_pages(0);
        key="HOME";verb="Close";
        if(count>1){counter[5]+=(char)(ambr.help-1);counter[7]+=(char)(count-1);
            key="SELECT";knob=1;verb=counter;second="HOME";action="Close";}
    }
    else if(ambr.reset_pending || ambr.view==3 || (amb_generate_page() && ambr.local_preset>AMB_ALL_GENERATE)){
        if(ambr.reset_pending || ambr.overwrite){key="REC";verb=ambr.reset_pending?"Restore":"Replace";second="HOME";action="Cancel";}
        else{key="SELECT";knob=1;verb="Slot";second="REC";action="Save";cancel=1;}
    }else if(ambr.evolve_open && ambr.evolve_queued){key="HOME";verb="Cancel";}
    else if(ambr.local_preset || ambr.piece_preview!=255 || ambr.proposal!=255 || ambr.evolve_open){
        cancel=1;key=ambr.local_preset || ambr.piece_preview!=255?"PRESETS":"ALGO";knob=1;verb=ambr.evolve_open?"Choose":ambr.local_preset?"Pick":"Browse";second="REC";action=ambr.evolve_open?"Trigger":ambr.piece_preview!=255?"Recall":"Apply";
        if(ambr.evolve_open && ambr.evolve_role==255){second="HOME";action="Cancel";cancel=0;}
        if(amb_generate_page() && ambr.local_preset>AMB_FACTORY_GENERATE && !amb_recipe_used(amb_user_recipe(ambr.local_preset-1-AMB_FACTORY_GENERATE))){second="HOME";action="Cancel";cancel=0;}
    }else if(ambr.view==0){key="SELECT";verb="Target";knob=1;second="EDIT";action=ambr.target?"Open":"FX";}
    else if(ambr.view==4){verb="Home";}
    else if(ambr.view==1){
        switch(ambr.page){
        case 0:key="EDIT";verb="Sound";second="ALGO";action="Role";second_knob=1;break;
        case 1:
            key="EDIT";verb=ambr.bank?"Exit":"Details";lit=!!ambr.bank;
            if(ambr.bank){page=l->role==6?(ambr.bank==1?"1/2":"2/2"):(const char*[]){"1/3","2/3","3/3"}[ambr.detail];}
            else if(amb_mic_page()){second="REC";action=amb_capture_page?"Finish":amb_replace_take?"Confirm":amb_current_take()->length?"Replace":"Record";}
            else{second="ALGO";action="Browse";second_knob=1;}break;
        case 3:key="EDIT";verb=ambr.bank?"Exit rate":"Rate";lit=!!ambr.bank;
            if(!ambr.bank){second="SELECT";action="Group";second_knob=1;}break;
        case 4:key="SELECT";knob=1;verb=l->role==4?"Lane":"Step";second=ambr.recording?"REC":"EDIT";action=ambr.recording?"Finish":l->generated?"Manual":"Auto";break;
        case 5:key="SELECT";knob=1;verb="Effect";break;
        }
    }else if(ambr.view==2){
        switch(ambr.page){
        case 0:key="SELECT";knob=1;verb="Interval";break;
        case 2:key="PRESETS";knob=1;verb="Browse";second="SAVE";action="Store";break;
        case 3:key="SELECT";knob=1;verb="Effect";break;
        case 5:key="SELECT";knob=1;verb="Theme";break;
        case 6:key="SELECT";knob=1;verb="Journey";second="EDIT";action="Timing";break;
        case 7:key="EDIT";verb="Exit";lit=1;page=(const char*[]){"1/3","2/3","3/3"}[timing_tab];break;
        }
    }
    if(page){counter[5]=page[0];counter[7]=page[2];second="SELECT";second_knob=1;action=counter;}
    int turn=(int32_t)(ambr.select_until-fm1_ms)>0 && ambr.select_until;
    int paging=!ambr.help && !ambr.evolve_open && (ambr.view<3) && !str_eq(key,"SEL");
    const char *page_action=ambr.view?"Page":"Ensemble";
    int cancel_width=cancel?amb_nav_width("HOME","Cancel",0):0;
    int page_width=paging?amb_nav_width("SEL",page_action,0):0;
    /* Composite cap, plus sign and Help form a single command. */
    int help_width=ambr.help?0:27+text_w(&AF_TONO_NAV,"HOME")+text_w(&AF_TONO_NAV,"EDIT")+text_w(&AF_TONO_NAV,"Help");
    int end=3+amb_nav_width(key,verb,knob);
    if(second)end+=amb_nav_width(second,action,second_knob);
    int y=end+cancel_width+page_width+help_width>232?61:76;
    cv_rect(8,y-4,224,1,ALINE);
    int x=amb_nav_item(y,8,key,verb,knob,lit || (turn && str_eq(key,"SELECT")));
    if(second)x=amb_nav_item(y,x,second,action,second_knob,turn && str_eq(second,"SELECT"));
    if(cancel){
        if(x+cancel_width>232){y=76;x=8;}
        x=amb_nav_item(y,x,"HOME","Cancel",0,0);
    }
    if(paging){
        if(x+page_width>232){y=76;x=8;}
        x=amb_nav_item(y,x,"SEL",page_action,0,0);
    }
    if(!ambr.help){
        if(x+help_width>232){y=76;x=8;}
        if(x>8){cv_rect(x,y+6,2,2,ADIM);x+=5;}
        x=amb_nav_cap(y,x,"HOME",0,0);cv_text(x,y+4-AF_TONO_NAV_CAP_Y,&AF_TONO_NAV,"+",ADIM);
        x=amb_nav_cap(y,x+6,"EDIT",0,0);cv_text(x,y+4-AF_TONO_NAV_CAP_Y,&AF_TONO_NAV,"Help",AINK);
    }
}
