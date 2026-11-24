/* SPDX-License-Identifier: GPL-3.0-only */
/* Keep each control and its action together. A second row carries navigation when needed. */
static int amb_nav_active(unsigned control)
{
    if(control<NB)
        return ((ambr.control_activity|fm1_in.buttons)>>panel.btn[control])&1u;
    return (ambr.control_activity>>control)&1u;
}

enum { NC_SELECT=NB+EN_SELECT,NC_ALGO=NB+EN_ALGO,NC_PRESETS=NB+EN_PRESET,NC_NONE };
static const char *amb_nav_label(unsigned control)
{
    return control==B_SCL?"SEL":control<NB?B_NAME[control]:
        control==NC_SELECT?"SELECT":control==NC_ALGO?"ALGO":"PRESETS";
}

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
static __attribute__((noinline)) void amb_footer(const amb_layer_t *l,int guide)
{
    unsigned control=B_SCL,secondary=NC_NONE;
    const char *verb="Page",*action=0;
    int lit=0,cancel=0;
    const char *page=0;
    char counter[]="Page 1/1";
    if(ambr.help && !guide){
        unsigned count=amb_help_pages(0);
        control=B_HOME;verb="Close";
        if(count>1){counter[5]+=(char)(ambr.help-1);counter[7]+=(char)(count-1);
            control=NC_SELECT;verb=counter;secondary=B_HOME;action="Close";}
    }
    else if(ambr.reset_pending || ambr.view==3 || (amb_generate_page() && ambr.local_preset>AMB_ALL_GENERATE)){
        if(ambr.reset_pending || ambr.overwrite){control=B_REC;verb=ambr.reset_pending?"Restore":"Replace";secondary=B_HOME;action="Cancel";}
        else{control=NC_SELECT;verb="Slot";secondary=B_REC;action="Save";cancel=1;}
    }else if(ambr.evolve_open && ambr.evolve_queued){control=B_HOME;verb="Cancel";}
    else if(ambr.local_preset || ambr.piece_preview!=255 || ambr.proposal!=255 || ambr.evolve_open){
        cancel=1;control=ambr.local_preset || ambr.piece_preview!=255?NC_PRESETS:NC_ALGO;verb=ambr.evolve_open?"Choose":ambr.local_preset?"Pick":"Browse";secondary=B_REC;action=ambr.evolve_open?"Trigger":ambr.piece_preview!=255?"Recall":"Apply";
        if(ambr.evolve_open && ambr.evolve_role==255){secondary=B_HOME;action="Cancel";cancel=0;}
        if(amb_generate_page() && ambr.local_preset>AMB_FACTORY_GENERATE && !amb_recipe_used(amb_user_recipe(ambr.local_preset-1-AMB_FACTORY_GENERATE))){secondary=B_HOME;action="Cancel";cancel=0;}
    }else if(ambr.view==0){control=NC_SELECT;verb="Target";secondary=B_EDIT;action=ambr.target?"Open":"FX";}
    else if(ambr.view==4){verb="Home";}
    else if(ambr.view==1){
        switch(ambr.page){
        case 0:control=B_EDIT;verb="Sound";secondary=NC_ALGO;action="Role";break;
        case 1:
            control=B_EDIT;verb=ambr.bank?"Exit":"Details";lit=!!ambr.bank;
            if(ambr.bank){page=l->role==6?(ambr.bank==1?"1/2":"2/2"):(const char*[]){"1/3","2/3","3/3"}[ambr.detail];}
            else if(amb_mic_page()){secondary=B_REC;action=amb_capture_page?"Finish":amb_replace_take?"Confirm":amb_current_take()->length?"Replace":"Record";}
            else{secondary=NC_ALGO;action="Browse";}break;
        case 3:control=B_EDIT;verb=ambr.bank?"Exit rate":"Rate";lit=!!ambr.bank;
            if(!ambr.bank){secondary=NC_SELECT;action="Group";}break;
        case 4:control=NC_SELECT;verb=l->role==4?"Lane":"Step";secondary=ambr.recording?B_REC:B_EDIT;action=ambr.recording?"Finish":l->generated?"Manual":"Auto";break;
        case 5:control=NC_SELECT;verb="Effect";break;
        }
    }else if(ambr.view==2){
        switch(ambr.page){
        case 0:control=NC_SELECT;verb="Interval";break;
        case 2:control=NC_PRESETS;verb="Browse";secondary=B_SAVE;action="Store";break;
        case 3:control=NC_SELECT;verb="Effect";break;
        case 5:control=NC_SELECT;verb="Theme";break;
        case 6:control=NC_SELECT;verb="Journey";secondary=B_EDIT;action="Timing";break;
        case 7:control=B_EDIT;verb="Exit";lit=1;page=(const char*[]){"1/3","2/3","3/3"}[timing_tab];break;
        }
    }
    if(page){counter[5]=page[0];counter[7]=page[2];secondary=NC_SELECT;action=counter;}
    const char *key=amb_nav_label(control),*second=secondary==NC_NONE?0:amb_nav_label(secondary);
    int knob=control>=NB,second_knob=secondary>=NB;
    int paging=(!ambr.help || guide) && !ambr.evolve_open && (ambr.view<3) && control!=B_SCL;
    const char *page_action=ambr.view?"Page":"Ensemble";
    int cancel_width=cancel?amb_nav_width("HOME","Cancel",0):0;
    int page_width=paging?amb_nav_width("SEL",page_action,0):0;
    /* Composite cap, plus sign and Help form a single command. */
    int help_width=ambr.help?0:27+text_w(&AF_TONO_NAV,"HOME")+text_w(&AF_TONO_NAV,"EDIT")+text_w(&AF_TONO_NAV,"Help");
    int end=3+amb_nav_width(key,verb,knob);
    if(second)end+=amb_nav_width(second,action,second_knob);
    int y=guide?25:end+cancel_width+page_width+help_width>232?61:76;
    cv_rect(8,y-4,224,1,ALINE);
    int x=amb_nav_item(y,8,key,verb,knob,lit || (!guide && amb_nav_active(control)));
    if(second)x=amb_nav_item(y,x,second,action,second_knob,(!guide && amb_nav_active(secondary)));
    if(cancel){
        if(x+cancel_width>232){y=guide?43:76;x=8;}
        x=amb_nav_item(y,x,"HOME","Cancel",0,!guide && amb_nav_active(B_HOME));
    }
    if(paging){
        if(x+page_width>232){y=guide?43:76;x=8;}
        x=amb_nav_item(y,x,"SEL",page_action,0,(!guide && amb_nav_active(B_SCL)));
    }
    if(!ambr.help){
        if(x+help_width>232){y=guide?43:76;x=8;}
        if(x>8){cv_rect(x,y+6,2,2,ADIM);x+=5;}
        x=amb_nav_cap(y,x,"HOME",0,amb_nav_active(B_HOME));cv_text(x,y+4-AF_TONO_NAV_CAP_Y,&AF_TONO_NAV,"+",ADIM);
        x=amb_nav_cap(y,x+6,"EDIT",0,amb_nav_active(B_EDIT));cv_text(x,y+4-AF_TONO_NAV_CAP_Y,&AF_TONO_NAV,"Help",AINK);
    }
}

/* These shortcuts apply after closing Help, like the page actions below. */
static void amb_help_shortcut(int y,const char *key,const char *action,int hold,int knob)
{
    int x=12;
    if(hold){
        x=amb_nav_cap(y,x,"HOME",0,0);
        cv_text(x,y+4-AF_TONO_NAV_CAP_Y,&AF_TONO_NAV,"+",ADIM);x+=6;
    }
    x=amb_nav_cap(y,x,key,knob,0);
    cv_text(x+2,y+4-AF_TONO_NAV_CAP_Y,&AF_TONO_NAV,action,AINK);
}
static void amb_help_actions(void)
{
    if(amb_proposing() || ambr.view==3)return;
    if(!ambr.view){
        if(ambr.zen){amb_help_shortcut(115,"SEL","Switch landscape",0,0);return;}
        amb_help_shortcut(111,"ALGO","Travel through the song",0,1);
        if(ambr.target)amb_help_shortcut(130,"PLAY","Lock or unlock this sound",1,0);
        else amb_help_shortcut(130,"ARP","Open Zen",1,0);
    }else if(ambr.view==1){
        amb_help_shortcut(111,"LFO","Link last control",1,0);
        if(!ambr.page)amb_help_shortcut(130,"SAVE","Restore this layer",1,0);
        else if(amb_local_count())amb_help_shortcut(130,"PRESETS","Browse page settings",0,1);
    }
}
