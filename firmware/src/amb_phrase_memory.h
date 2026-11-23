/* MIT-derived event algorithms. See LICENSES/Marbles.txt and LICENSES/Downspout.txt.
 * Called on an automatic attack, never on an audio sample. */
static unsigned amb_remember(unsigned k,unsigned note,int percussion)
{
    unsigned style=amb.policy.active[1];
    if(style<AMB_LOCKED || (percussion && style==AMB_MARKOV))return note;
    amb_memory_t *m=&amb_memory[k];
    unsigned tag=style+(percussion?16:0);
    if(m->style!=tag){
        memset(m,0,sizeof *m);
        m->style=tag;m->rng=amb.policy.seed+k*997u;
    }
    m->rng=1664525u*m->rng+1013904223u;
    if(style!=AMB_MARKOV){
        int root=0;
        if(!percussion){
            int scale[12],count=amb_scale(amb_pool(),scale);
            root=nearest(60,(amb.key+scale[amb.chord%count])%12,54,65);
        }
        /* Marbles deterministic shift register: append restarts the read
         * head at the newest entry, ordinary replay advances one place. */
        if(m->count<8 || (style==AMB_DRIFT && (m->rng>>8)<2684355u)){
            m->pos=m->head;
            m->history[m->head]=(int)note-root;
            m->head=(m->head+1)&7;
            if(m->count<8)m->count++;
        }else m->pos=(m->pos+1)&7;
        int pitch=root+m->history[m->pos];
        return percussion?(unsigned)pitch:amb_pool_nearest(clamp(pitch,1,127));
    }
    unsigned key=amb.key,low=amb.layer[k].role==3?40:60;
    if(m->count<16){
        m->history[m->count++]=(note+12-key)%12;
        return note;
    }
    /* Sparse equivalent of Downspout's learned transition counts. Sixteen
     * observations fit in one row of bytes; no dense 12^3 table per chair. */
    if(!m->pos){m->previous=0;m->older=255;m->pitch=low+12;}
    uint8_t first[12]={0},second[12]={0};
    unsigned matches=0,peak=0;
    for(unsigned i=1;i<16;i++)if(m->history[i-1]==m->previous){
        unsigned to=m->history[i];
        if(++first[to]>peak)peak=first[to];
        if(i>=2 && m->history[i-2]==m->older){second[to]++;matches++;}
    }
    static const uint8_t stepwise[]={2,7,8,4,3,2,1,2,3,4,8,7};
    unsigned weights[12],total=0,pool=amb_pool();
    for(unsigned i=0;i<12;i++){
        unsigned prior=stepwise[(i+12-m->previous)%12];
        unsigned w=matches>=2?second[i]*8:peak?3*prior+136*first[i]/peak:prior;
        weights[i]=(pool&(1u<<i))?w:0;
        total+=weights[i];
    }
    unsigned to=0;
    if(total){
        unsigned draw=(m->rng>>8)%total;
        while(to<11 && draw>=weights[to])draw-=weights[to++];
    }
    m->pitch=nearest(m->pitch,(key+to)%12,low,low+23);
    m->older=m->previous;m->previous=to;m->pos=(m->pos+1)&15;
    return m->pitch;
}
