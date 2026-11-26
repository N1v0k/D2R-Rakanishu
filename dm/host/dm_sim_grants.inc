



enum { SIM_GRANT_BAD = 32 };

static void sim_grant_put(dm_sim_char *c,int kind,int skill,int lvl,int held,int chance,int charges,int set2) {
    if(skill<=0 || skill>=DM_SIM_MAXSKILL || lvl<1)return;
    if(lvl>256)lvl=256;
    for(int i=0;i<c->ngrant;i++) {
        dm_sim_grant *g=&c->grant[i];
        if(g->kind!=kind || g->skill!=skill || g->set2!=set2)continue;

        if(kind==DM_GR_CHARGE){if(lvl>g->lvl)g->lvl=(int16_t)lvl;g->charges=(int16_t)(g->charges+charges);return;}
        if(kind==DM_GR_AURA || kind==DM_GR_BUFF){if(lvl>g->lvl)g->lvl=(int16_t)lvl;if(held>g->held)g->held=(int16_t)held;return;}
        if(g->lvl==lvl){g->chance=(int16_t)(g->chance+chance>100?100:g->chance+chance);return;}
    }
    if(c->ngrant>=DM_SIM_MAXGRANT){c->capture_bad|=SIM_GRANT_BAD;return;}
    dm_sim_grant *g=&c->grant[c->ngrant++];
    memset(g,0,sizeof *g);
    g->kind=(int16_t)kind;g->skill=(int16_t)skill;g->lvl=(int16_t)lvl;g->held=(int16_t)held;g->chance=(int16_t)chance;g->charges=(int16_t)charges;g->set2=(int16_t)set2;
}

static int sim_list_entries(uint64_t u,uint64_t *arr,uint64_t *count) {
    uint64_t list=u?*(uint64_t *)(u+0x88):0;*arr=*count=0;
    if(!list)return 0;
    uint64_t base=*(int32_t *)(list+0x1c)<0?list+0xe8:list+0x30;
    *arr=*(uint64_t *)base;*count=*(uint64_t *)(base+8);
    if(*count>512 || (*count && !*arr)){*arr=*count=0;return -1;}
    return 0;
}

static int64_t sim_state_node(const dm_sim_data *d,uint64_t pl,int skill) {
    uint64_t root=*(uint64_t *)(pl+0x88);
    if(!root || *(int32_t *)(root+0x1c)>=0 || skill<0 || skill>=DM_SIM_MAXSKILL || !d->skill[skill])return 0;
    const char *name=d->skill[skill]->aurastate;if(!name[0])return 0;
    int state=-1;for(int j=0;j<d->nstate;j++)if(!strcmp(d->state[j].name,name))state=d->state[j].id;
    if(state<=0 || state>510 || !(*(uint32_t *)(root+0xb30+4*(state>>5))>>(state&31)&1))return 0;
    for(int chain=0;chain<2;chain++) {
        uint64_t node=*(uint64_t *)(root+0xd0+8*chain);int walked=0;
        for(;node && walked<1024;node=*(uint64_t *)(node+0x68),walked++)if(*(int32_t *)(node+0x20)==state)return (int64_t)node;
        if(walked==1024)return -1;
    }
    return 0;
}
static int sim_node_stat(uint64_t node,int stat) {
    uint64_t arr=*(uint64_t *)(node+0x30),count=*(uint64_t *)(node+0x38);int v=0;
    for(uint64_t i=0;arr && i<count && i<512;i++)if((*(uint32_t *)(arr+8*i)>>16)==(uint32_t)stat && !(*(uint32_t *)(arr+8*i)&0xffff))v+=*(int32_t *)(arr+8*i+4);
    return v;
}

static int sim_state_level(uint64_t node){return node?sim_node_stat(node,351):0;}

static void sim_item_grants(uint8_t *b,const dm_sim_data *d,uint64_t pl,uint64_t it,dm_sim_char *c,int worn_now) {
    uint64_t arr,count;
    if(sim_list_entries(it,&arr,&count)){c->capture_bad|=SIM_GRANT_BAD;return;}
    for(uint64_t i=0;i<count;i++) {
        uint32_t key=*(uint32_t *)(arr+8*i);int value=*(int32_t *)(arr+8*i+4),stat=(int)(key>>16),layer=(int)(key&0xffff);
        if(stat==151) {
            int64_t node=worn_now?sim_state_node(d,pl,layer):0;
            if(node<0)c->capture_bad|=SIM_GRANT_BAD;
            int held=node>0?sim_state_level((uint64_t)node):0;
            if(node>0 && !held)held=value;
            sim_grant_put(c,DM_GR_AURA,layer,value,held,0,0,0);
        } else if(stat==195 || stat==198 || stat==201) {
            sim_grant_put(c,stat==195?DM_GR_ATTACK:stat==198?DM_GR_HIT:DM_GR_STRUCK,layer>>6,layer&63,0,value>100?100:value,0,0);
        } else if(stat==204) {
            if(value&255)sim_grant_put(c,DM_GR_CHARGE,layer>>6,layer&63,0,0,value&255,0);
        }
    }
    (void)b;
}


static int sim_set2_total(uint8_t *b,uint64_t pl,int st,int layer,const uint64_t *hands,int nh,const uint64_t *plus,int np,const uint64_t *minus,int nm,
                          const uint64_t *set2,int n2) {
    return gstat(b,pl,st,layer)-gsum(b,hands,nh,st,layer)+gsum(b,plus,np,st,layer)-gsum(b,minus,nm,st,layer)+gsum(b,set2,n2,st,layer);
}



static void sim_set2_buffs(uint8_t *b,const dm_sim_data *d,uint64_t pl,int cls,const uint64_t *plus,int np,const uint64_t *minus,int nm,dm_sim_char *c) {
    uint64_t hands[2],set2[2];int nh=0,n2=0;
    for(uint64_t it=first_item(pl);it;it=next_item(it)) {
        uint64_t di=*(uint64_t *)(it+0x10);if(!di || *(uint32_t *)(it+0xc)!=1)continue;
        int sl=*(uint8_t *)(di+0x54);
        if((sl==4 || sl==5) && nh<2 && item_effective(b,pl,it))hands[nh++]=it;
        else if((sl==11 || sl==12) && n2<2 && item_effective(b,pl,it))set2[n2++]=it;
    }
    for(int k=0;k<n2;k++) {
        uint64_t arr,count;if(sim_list_entries(set2[k],&arr,&count)){c->capture_bad|=SIM_GRANT_BAD;continue;}
        for(uint64_t i=0;i<count;i++) {
            uint32_t key=*(uint32_t *)(arr+8*i);int value=*(int32_t *)(arr+8*i+4),stat=(int)(key>>16),layer=(int)(key&0xffff);
            int skill=stat==97?layer:stat==204?layer>>6:-1,lvl=stat==204?(value&255?layer&63:0):0;
            if(skill<=0 || skill>=DM_SIM_MAXSKILL || !d->skill[skill])continue;
            const dm_sim_skill *s=d->skill[skill];
            if(!(s->fam&DM_FAM_B) || !s->timed)continue;
            if(stat==97) {
                int si=-1;for(int j=0;j<D.nsk && si<0;j++)if(D.sk[j].id==skill)si=j;
                int own=si>=0 && D.sk[si].cls==cls,pg=own?D.sk[si].page:0,ly=cls*8+pg-1;
                uint64_t node=skill_node(pl,skill);
#define T(st,ly_) sim_set2_total(b,pl,st,ly_,hands,nh,plus,np,minus,nm,set2,n2)
                lvl=(node?*(int32_t *)(node+0x38):0)+T(97,skill)+T(127,0)+(own?T(83,cls)+T(107,skill)+(pg>=1 && pg<=3?T(188,ly):0):0);
#undef T
            }
            if(lvl<1)continue;
            int64_t node=sim_state_node(d,pl,skill);if(node<0)c->capture_bad|=SIM_GRANT_BAD;
            sim_grant_put(c,DM_GR_BUFF,skill,lvl,node>0?sim_state_level((uint64_t)node):0,0,0,stat==97);
        }
    }
}

static void sim_grants_of(uint8_t *b,const dm_sim_data *d,uint64_t pl,int cls,const uint64_t *worn,const int *now,int nworn,const uint64_t *plus,int np,
                          const uint64_t *minus,int nm,dm_sim_char *c) {
    c->ngrant=0;
    for(int k=0;k<nworn;k++)sim_item_grants(b,d,pl,worn[k],c,now[k]);
    sim_set2_buffs(b,d,pl,cls,plus,np,minus,nm,c);
}


static void sim_grants_states(const dm_sim_data *d,uint64_t pl,dm_sim_char *c) {
    for(int i=0;i<c->ngrant;i++) {
        dm_sim_grant *g=&c->grant[i];const dm_sim_skill *s=d->skill[g->skill];
        if(g->kind!=DM_GR_AURA || !g->held || !s || (s->fam&DM_FAM_K) || !strcmp(s->aurastate,"conviction"))continue;
        int64_t node=sim_state_node(d,pl,g->skill);if(node<=0)continue;
        for(int k=0;k<6;k++)g->game[k]=s->astat[k]<0?0:sim_node_stat((uint64_t)node,s->astat[k]);
        g->checked=1;
    }
}


static void sim_grants_unheld(const dm_sim_data *d,uint64_t pl,const dm_sim_char *now,dm_sim_char *a,const uint64_t *olds,int nold,int *off,uint8_t *b) {
    for(int i=0;i<now->ngrant;i++) {
        const dm_sim_grant *g=&now->grant[i];
        if(g->kind!=DM_GR_AURA || !g->held)continue;
        int gone=0;
        for(int k=0;k<nold;k++){uint64_t arr,count;if(sim_list_entries(olds[k],&arr,&count))continue;for(uint64_t j=0;j<count;j++)if(*(uint32_t *)(arr+8*j)==(151u<<16|(uint32_t)g->skill))gone=1;}
        if(!gone)continue;

        if(now->active_lvl[g->skill]>0){a->capture_bad|=SIM_GRANT_BAD;continue;}
        int64_t node=sim_state_node(d,pl,g->skill);
        if(node<=0){a->capture_bad|=SIM_GRANT_BAD;continue;}
        uint64_t arr=*(uint64_t *)((uint64_t)node+0x30),count=*(uint64_t *)((uint64_t)node+0x38);
        for(uint64_t j=0;arr && j<count && j<512;j++) {
            uint32_t key=*(uint32_t *)(arr+8*j);
            if((key&0xffff) || (key>>16)>=512 || (key>>16)==350 || (key>>16)==351)continue;
            a->stat_value[key>>16]-=*(int32_t *)(arr+8*j+4);off[key>>16]+=*(int32_t *)(arr+8*j+4);
        }
        for(int k=0;k<a->ngrant;k++)if(a->grant[k].kind==DM_GR_AURA && a->grant[k].skill==g->skill)a->grant[k].held=0;
    }
    (void)b;
}

static int sim_worn_items(uint8_t *b,uint64_t pl,uint64_t *worn) {
    int n=0;
    for(uint64_t it=first_item(pl);it && n<16;it=next_item(it)) {
        uint64_t di=*(uint64_t *)(it+0x10);if(!di || *(uint32_t *)(it+0xc)!=1)continue;
        int sl=*(uint8_t *)(di+0x54);
        if(sl>=1 && sl<=10 && item_effective(b,pl,it))worn[n++]=it;
    }
    return n;
}

static void sim_oskill_check(uint8_t *b,const dm_sim_data *d,uint64_t pl,dm_sim_char *c) {
    for(int id=1;id<DM_SIM_MAXSKILL;id++) {
        if(!d->skill[id] || gstat(b,pl,97,id)<=0)continue;
        uint64_t node=skill_node(pl,id);if(!node || *(int32_t *)(node+0x38)>=1)continue;
        int si=-1;for(int j=0;j<D.nsk && si<0;j++)if(D.sk[j].id==id)si=j;
        int own=si>=0 && D.sk[si].cls==c->cls,pg=own?D.sk[si].page:0,ly=c->cls*8+pg-1;
        int rule=gstat(b,pl,97,id)+gstat(b,pl,127,0)+(own?gstat(b,pl,83,c->cls)+gstat(b,pl,107,id)+(pg>=1 && pg<=3?gstat(b,pl,188,ly):0):0);
        int game=((int (*)(uint64_t,uint64_t,int))(b+GA_SKILL_BONUS_LEVEL))(pl,node,0);
        if(rule!=game) {
            c->capture_bad|=SIM_GRANT_BAD;
            static uint64_t said;uint64_t key=(uint64_t)id<<20|(uint64_t)(rule&1023)<<10|(uint64_t)(game&1023);
            if(said!=key){said=key;LOG("ASSUME bestgear.item_skill: skill %d predicted level %d differs from game %d",id,rule,game);}
        }
    }
}
static void sim_grants_now(uint8_t *b,const dm_sim_data *d,uint64_t pl,dm_sim_char *c) {
    uint64_t worn[16];int now[16],n=sim_worn_items(b,pl,worn);
    for(int k=0;k<n;k++)now[k]=1;
    sim_grants_of(b,d,pl,c->cls,worn,now,n,NULL,0,NULL,0,c);
    sim_grants_states(d,pl,c);
    sim_oskill_check(b,d,pl,c);
}


static void sim_charm_grant_flag(dm_sim_char *a,uint64_t it) {
    uint64_t arr,count; if(!it || sim_list_entries(it,&arr,&count))return;
    for(uint64_t i=0;i<count;i++) {
        uint32_t key=*(uint32_t *)(arr+8*i);int stat=(int)(key>>16);
        if(stat==151||stat==195||stat==198||stat==201||stat==204||stat==97) {
            a->capture_bad|=SIM_GRANT_BAD;
            static uint64_t said;uint64_t k=(uint64_t)*(uint32_t *)(it+8)<<16|(uint32_t)stat;
            if(said!=k){said=k;LOG("ASSUME itemcmp.charm_grant: charm %u carries grant stat %d",*(uint32_t *)(it+8),stat);}
            return;
        }
    }
}
static void sim_grants_after(uint8_t *b,const dm_sim_data *d,uint64_t pl,const dm_sim_char *c,dm_sim_char *a,uint64_t cand,const uint64_t *olds,int nold,int sl,int charm,int *off) {
    uint64_t worn[17],kept[17],plus[1],minus[4];int now[17],n=sim_worn_items(b,pl,worn),m=0,np=0,nm=0;
    for(int k=0;k<n;k++){int out=0;for(int j=0;j<nold;j++)out|=olds[j]==worn[k];if(!out){kept[m]=worn[k];now[m++]=1;}}
    if(cand && sl>=1 && sl<=10){kept[m]=cand;now[m++]=0;}


    if(charm==IC_CHARM_IN && cand){plus[np++]=cand;sim_charm_grant_flag(a,cand);}
    else if(cand && sl>=1 && sl<=10 && sl!=4 && sl!=5)plus[np++]=cand;
    for(int j=0;j<nold && nm<4;j++){uint64_t di=olds[j]?*(uint64_t *)(olds[j]+0x10):0;

        int wornitem=di && *(uint32_t *)(olds[j]+0xc)==1,os=wornitem?*(uint8_t *)(di+0x54):0;
        if(!wornitem || os!=4 && os!=5)minus[nm++]=olds[j];}
    if(charm==IC_CHARM_OUT && nold)sim_charm_grant_flag(a,olds[0]);
    sim_grants_of(b,d,pl,c->cls,kept,now,m,plus,np,minus,nm,a);
    sim_grants_unheld(d,pl,c,a,olds,nold,off,b);
}





static int sim_bonus_change(uint8_t *b,int cls,int id,uint64_t cand,const uint64_t *olds,int nold) {
    int si=-1;for(int j=0;j<D.nsk && si<0;j++)if(D.sk[j].id==id)si=j;
    int own=si>=0 && D.sk[si].cls==cls,pg=own?D.sk[si].page:0,ly=cls*8+pg-1,tab=own && pg>=1 && pg<=3;
#define DELTA(st,layer) (gstat(b,cand,st,layer)-gsum(b,olds,nold,st,layer))
    return DELTA(97,id)+DELTA(127,0)+(own?DELTA(83,cls)+DELTA(107,id):0)+(tab?DELTA(188,ly):0);
#undef DELTA
}
static int sim_oskill_after(uint8_t *b,uint64_t pl,int cls,int id,uint64_t cand,const uint64_t *olds,int nold) {
    if(!gstat(b,cand,97,id) && !gsum(b,olds,nold,97,id))return -1;
    int si=-1;for(int j=0;j<D.nsk && si<0;j++)if(D.sk[j].id==id)si=j;
    int own=si>=0 && D.sk[si].cls==cls,pg=own?D.sk[si].page:0,ly=cls*8+pg-1,tab=own && pg>=1 && pg<=3;
    int change=sim_bonus_change(b,cls,id,cand,olds,nold);
    uint64_t node=skill_node(pl,id);
    if(node && *(int32_t *)(node+0x38)>=1)return *(int32_t *)(node+0x38)+((int (*)(uint64_t,uint64_t,int))(b+GA_SKILL_BONUS_LEVEL))(pl,node,0)+change;
    int granted=gstat(b,pl,97,id)+gstat(b,cand,97,id)-gsum(b,olds,nold,97,id);
    if(granted<=0)return 0;
    int bonus=gstat(b,pl,127,0)+(own?gstat(b,pl,83,cls)+gstat(b,pl,107,id):0)+(tab?gstat(b,pl,188,ly):0);
    return granted+bonus+change-(gstat(b,cand,97,id)-gsum(b,olds,nold,97,id));
}


static int sim_elem_after(uint8_t *b,const dm_sim_data *d,uint64_t pl,int cls,int id,uint64_t cand,const uint64_t *olds,int nold,dm_sim_char *a) {
    int involved=0,fire=0,other=0;
    for(int el=0;el<8;el++){int v=gstat(b,cand,126,el)-gsum(b,olds,nold,126,el);if(gstat(b,cand,126,el) || gsum(b,olds,nold,126,el))involved=1;if(el==1)fire=v;else if(v)other=1;}
    if(!involved)return -1;
    if(other){a->capture_bad|=SIM_GRANT_BAD;return -1;}
    uint64_t node=skill_node(pl,id);if(!node || *(int32_t *)(node+0x38)<1)return -1;
    int si=-1;for(int j=0;j<D.nsk && si<0;j++)if(D.sk[j].id==id)si=j;
    if(si<0 || D.sk[si].cls!=cls)return -1;
    int pg=D.sk[si].page,ly=cls*8+pg-1;
#define DELTA(st,layer) (gstat(b,cand,st,layer)-gsum(b,olds,nold,st,layer))
    int change=DELTA(107,id)+DELTA(127,0)+DELTA(83,cls)+(pg>=1 && pg<=3?DELTA(188,ly):0)+(d->skill[id]->etype==DM_EL_FIRE?fire:0);
#undef DELTA
    return *(int32_t *)(node+0x38)+((int (*)(uint64_t,uint64_t,int))(b+GA_SKILL_BONUS_LEVEL))(pl,node,0)+change;
}
