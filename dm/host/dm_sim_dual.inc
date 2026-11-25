




enum { SIM_LIST_IDLE = 0x40000000 };


static int sim_dr(uint8_t *b,uint64_t pl,int st) {
    unsigned ver=*(uint8_t *)(pl+0x1bd);if(ver<1 || ver>3)return -1;
    uint64_t table=*(uint64_t *)(b+0x2d64a18+ver*16);if(!table)return -1;
    uint64_t stats=*(uint64_t *)(table+0x1290);int count=*(int32_t *)(table+0x1298);
    if(!stats || count<1 || count>4096 || st<0 || st>=count)return -1;
    return (*(uint8_t *)(stats+(uint64_t)st*0x144+4)&4)!=0;
}
static int sim_hand_weapon(uint8_t *b,uint64_t it){return it && is_type(b,it,45);}
static int sim_list_idle(uint64_t it){uint64_t l=it?*(uint64_t *)(it+0x88):0;return l && (*(uint32_t *)(l+0x1c)&SIM_LIST_IDLE);}

static int sim_claw(uint8_t *b,uint64_t it){const sim_gear *r=sim_gear_row(b,it);return r && !strcmp(r->one,"HT1");}


static int sim_dual_class(uint8_t *b,int cls,uint64_t right,uint64_t left) {
    if(!sim_hand_weapon(b,right) || !sim_hand_weapon(b,left))return 0;
    if(cls==4)return 1;
    if(cls==6)return sim_claw(b,right) && sim_claw(b,left);
    return 0;
}

typedef struct { int g[CB_NG], dr[CB_NG], lv[2], ok; } sim_dual_base;
static void sim_dual_base_now(uint8_t *b,uint64_t pl,const uint64_t *hands,sim_dual_base *o) {
    memset(o,0,sizeof *o);o->ok=1;
    for(int i=0;i<CB_NG;i++){int s=k_cb_g[i],dr=sim_dr(b,pl,s);if(dr<0)o->ok=0;o->dr[i]=dr>0;o->g[i]=gstat(b,pl,s,0);
        for(int h=0;h<2;h++)if(o->dr[i] && sim_hand_weapon(b,hands[h]) && !sim_list_idle(hands[h]))o->g[i]-=gstat(b,hands[h],s,0);}
    o->lv[0]=lstat_(b,pl,171)+lstat_(b,pl,16);o->lv[1]=lstat_(b,pl,119);
    for(int h=0;h<2;h++)if(sim_hand_weapon(b,hands[h]) && !sim_list_idle(hands[h]))o->lv[1]-=lstat_(b,hands[h],119);
}

static void sim_dual_base_after(uint8_t *b,uint64_t pl,const sim_dual_base *now,uint64_t cand,const uint64_t *olds,int nold,const uint64_t *hands,
                                const uint64_t *after,sim_dual_base *o) {
    *o=*now;
    for(int k=0;k<=nold;k++) {
        uint64_t x=k?olds[k-1]:cand;if(!x)continue;
        int hand=0;for(int h=0;h<2;h++)hand|=(x==hands[h] || x==after[h]) && sim_hand_weapon(b,x);
        int sign=k?-1:1;
        for(int i=0;i<CB_NG;i++)if(hand?!o->dr[i]:1)o->g[i]+=sign*gstat(b,x,k_cb_g[i],0);
        o->lv[0]+=sign*(lstat_(b,x,171)+lstat_(b,x,16));if(!hand)o->lv[1]+=sign*lstat_(b,x,119);
    }
}


static void sim_dual_cb(uint8_t *b,uint64_t pl,int cls,const sim_dual_base *base,uint64_t w,int mode,uint64_t nd,int alvl,const int *off,dm_ic_cb *cb) {
    int g[CB_NG],lv[2]={base->lv[0],base->lv[1]+lstat_(b,w,119)};
    for(int i=0;i<CB_NG;i++)g[i]=base->g[i]+(base->dr[i]?gstat(b,w,k_cb_g[i],0):0);
    cb_fill(b,pl,g,lv,cls,0,w,mode,nd,alvl,cb);
    if(off)dm_ic_cb_minus(cb,off);
}






static int sim_dual_fill(uint8_t *b,const dm_sim_data *d,uint64_t pl,dm_sim_char *c,const uint64_t *hand_items,dm_ic_cb *cb,const int *dr_flag,
                         const dm_sim_char *now) {
    int values[2][IC_C_N+1];
    for(int h=0;h<2;h++)if(!dm_ic_cb_eval(&cb[h],values[h]))return -1;
    static dm_gear_hand in[2];static int val[2][512];static uint8_t dr[512];
    static int mth[2][DM_SIM_MAXSKILL],mdmg[2][DM_SIM_MAXSKILL],ab[2][DM_SIM_MAXSKILL],dm[2][DM_SIM_MAXSKILL],prog[DM_SIM_MAXSKILL];static dm_sim_char x;
    memset(in,0,sizeof in);
    for(int s=0;s<512;s++)dr[s]=dr_flag[s]>0;
    for(int h=0;h<2;h++) {
        if(!sim_weapon(b,hand_items[h],1,c->cls,&in[h].w))return -1;
        in[h].ias=gstat(b,hand_items[h],93,0);in[h].ds=gstat(b,hand_items[h],141,0);
        in[h].mcs=((int (*)(uint64_t,uint64_t,uint64_t,int))(b+0x5b2a30))(pl,hand_items[h],0,2);
        for(int s=0;s<512;s++)val[h][s]=dr[s] && d->stat_name[s][0]?gstat(b,hand_items[h],s,0):0;
    }
    const int *const vp[2]={val[0],val[1]};
    if(dm_gear_dual_stats(d,dr,vp,c->stat_value))return -1;
    sim_values(d,c);

    int charge=sim_charge_state(b,pl);if(charge<0)c->endpoint_bad=1;
    for(int id=0;id<DM_SIM_MAXSKILL;id++) {
        uint64_t node=skill_node(pl,id),record=node?*(uint64_t *)node:0;
        prog[id]=record && (*(uint8_t *)(record+0x24)&8)?c->stat_value[325]:0;
        for(int h=0;h<2;h++) {
            mth[h][id]=record?((int (*)(uint64_t,uint64_t,uint64_t,int))(b+0x5b2a30))(pl,hand_items[h],node,0):DM_SIM_NO_NODE;
            mdmg[h][id]=record?((int (*)(uint64_t,uint64_t,uint64_t,int))(b+0x5b2a30))(pl,hand_items[h],node,1):DM_SIM_NO_NODE;
        }
        if(record)c->endpoint_state_bad[id]=charge>0 && (*(uint8_t *)(record+0x24)&8)!=0;
    }
    for(int h=0;h<2;h++) {
        if(now) {
            x=*c;if(dm_gear_hand_passive(d,now,&x,&cb[h],mth[h],mdmg[h],in[h].mcs,prog))return -2;
            in[h].mcs=x.mastery_cs;
            for(int id=0;id<DM_SIM_MAXSKILL;id++){ab[h][id]=x.ar_bonus[id]-prog[id];dm[h][id]=x.damage_mastery_delta[id];}
        } else for(int id=0;id<DM_SIM_MAXSKILL;id++)if(mth[h][id]!=DM_SIM_NO_NODE){ab[h][id]=cb[h].tohitpct+mth[h][id];dm[h][id]=mdmg[h][id]-cb[h].dmast;}
        in[h].cb=cb[h];
    }
    if(dm_gear_dual_hands(d,c,in,vp))return -1;

    for(int h=0;h<2;h++)if(raw_has(hand_items[h],195) || raw_has(hand_items[h],198))c->capture_bad|=SIM_GRANT_BAD;

    for(int id=0;id<DM_SIM_MAXSKILL;id++)if(mth[0][id]!=DM_SIM_NO_NODE)dm_gear_dual_skill(ab[0][id],ab[1][id],dm[0][id],dm[1][id],prog[id],&c->ar_bonus[id],&c->damage_mastery_delta[id]);
    c->endpoint_bad=1;
    return 0;
}

static void sim_dr_flags(uint8_t *b,uint64_t pl,int *dr_flag){for(int s=0;s<512;s++)dr_flag[s]=sim_dr(b,pl,s);}

static void sim_dual_strip(uint8_t *b,const dm_sim_data *d,uint64_t pl,dm_sim_char *c,const uint64_t *hands,const int *dr_flag) {
    for(int s=0;s<512;s++)if(dr_flag[s]>0 && d->stat_name[s][0])
        for(int h=0;h<2;h++)if(sim_hand_weapon(b,hands[h]) && !sim_list_idle(hands[h]))c->stat_value[s]-=gstat(b,hands[h],s,0);
}

static void sim_dual_now(uint8_t *b,const dm_sim_data *d,uint64_t pl,dm_sim_char *c,const uint64_t *hands,const cb_res *R,sim_dual_base *base) {
    int dr_flag[512];sim_dr_flags(b,pl,dr_flag);
    sim_dual_base_now(b,pl,hands,base);
    int idle=sim_list_idle(hands[0])+sim_list_idle(hands[1]);
    uint64_t nd=skill_node(pl,0);
    if(!base->ok || idle!=1 || !nd){c->capture_bad|=1;c->endpoint_bad=1;return;}
    int sias=sim_get(c,d,"attackrate")-100;
    sim_dual_strip(b,d,pl,c,hands,dr_flag);
    int alvl=*(int32_t *)(nd+0x38)+((int (*)(uint64_t,uint64_t,int))(b+0x5b1710))(pl,nd,0);
    dm_ic_cb cb[2];for(int h=0;h<2;h++)sim_dual_cb(b,pl,c->cls,base,hands[h],1,nd,alvl,NULL,&cb[h]);
    int filled=sim_dual_fill(b,d,pl,c,hands,cb,dr_flag,NULL);
    if(filled){c->capture_bad|=1;c->endpoint_bad=1;return;}
    int wbad=0;c->block=dm_sim_weapon_block(d,c,&wbad);
    if(wbad){c->capture_bad|=1;c->endpoint_bad=1;return;}
    int active=sim_list_idle(hands[0])?1:0;
    c->sias=sias+c->hand[active].wsm;
    int v[IC_C_N+1];dm_ic_cb_eval(&cb[active],v);
    if(v[IC_C_AR]!=R->gamev[IC_C_AR] || v[IC_C_DMG]!=R->gamev[IC_C_DMG] || v[IC_C_N]!=R->gamev[IC_C_N]) {
        c->capture_bad|=1;
        static uint64_t said;uint64_t key=(uint64_t)(uint32_t)v[IC_C_DMG]<<32|(uint32_t)R->gamev[IC_C_DMG];
        if(said!=key){said=key;LOG("ASSUME bestgear.dual: hand %d sheet damage differs from game (%d-%d ar %d vs %d-%d ar %d)",4+active,v[IC_C_DMG],v[IC_C_N],v[IC_C_AR],
            R->gamev[IC_C_DMG],R->gamev[IC_C_N],R->gamev[IC_C_AR]);}
    }
}




static int sim_dual_after(uint8_t *b,const dm_sim_data *d,uint64_t pl,const dm_sim_char *c,dm_sim_char *a,uint64_t cand,const uint64_t *olds,int nold,
                          const uint64_t *hands,const uint64_t *after,const sim_dual_base *base_now,const int *aura_off,int dsk,int passive) {
    int now_dual=c->nhands==2,after_dual=sim_dual_class(b,c->cls,after[0],after[1]);
    if(!now_dual && !after_dual)return 0;
    if(now_dual && !base_now->ok){a->capture_bad|=1;a->endpoint_bad=1;return 1;}
    int dr_flag[512];sim_dr_flags(b,pl,dr_flag);
    sim_dual_base bn=*base_now;if(!now_dual)sim_dual_base_now(b,pl,hands,&bn);
    if(!bn.ok){a->capture_bad|=1;a->endpoint_bad=1;return 1;}
    sim_dual_base ba;sim_dual_base_after(b,pl,&bn,cand,olds,nold,hands,after,&ba);


    for(int s=0;s<512;s++)if(dr_flag[s]>0 && d->stat_name[s][0]) {
        int v=gstat(b,pl,s,0);
        for(int h=0;h<2;h++)if(sim_hand_weapon(b,hands[h]) && !sim_list_idle(hands[h]))v-=gstat(b,hands[h],s,0);
        for(int k=0;k<=nold;k++){uint64_t x=k?olds[k-1]:cand;if(!x)continue;int hand=0;for(int h=0;h<2;h++)hand|=(x==hands[h] || x==after[h]) && sim_hand_weapon(b,x);
            if(!hand)v+=(k?-1:1)*gstat(b,x,s,0);}
        a->stat_value[s]=v-aura_off[s];
        if(!after_dual)for(int h=0;h<2;h++)if(sim_hand_weapon(b,after[h]))a->stat_value[s]+=gstat(b,after[h],s,0);
    }
    uint64_t nd=skill_node(pl,0);
    if(!nd){a->capture_bad|=1;a->endpoint_bad=1;return 1;}
    int alvl=*(int32_t *)(nd+0x38)+((int (*)(uint64_t,uint64_t,int))(b+0x5b1710))(pl,nd,0)+dsk;
    if(!after_dual) {
        uint64_t w=sim_hand_weapon(b,after[0])?after[0]:after[1];
        int mode=w?hand_mode(b,pl,after[0],after[1]):0;
        dm_ic_cb cb;sim_dual_cb(b,pl,a->cls,&ba,w,mode,nd,alvl,aura_off,&cb);cb.can_block=can_block(b,after[0],after[1]);

        uint64_t ra=item_rec(b,w);int weapon_now[2],kept[2],type_now[2],mastery_now[2];
        for(int h=0;h<2;h++) {
            weapon_now[h]=sim_hand_weapon(b,hands[h]);kept[h]=hands[h]==w;uint64_t rn=item_rec(b,hands[h]);type_now[h]=rn?*(uint16_t *)(rn+0x12e):-1;mastery_now[h]=0;
            if(weapon_now[h] && !kept[h]){dm_ic_cb was;sim_dual_cb(b,pl,a->cls,&bn,hands[h],1,nd,alvl-dsk,NULL,&was);mastery_now[h]=was.ar_mast || was.dmast;}
        }
        if(dm_ic_one_after_mastery(a->cls,weapon_now,kept,type_now,mastery_now,ra?*(uint16_t *)(ra+0x12e):-1,cb.ar_mast || cb.dmast)){a->capture_bad|=1|16;a->endpoint_bad=1;return 1;}

        static int mth[DM_SIM_MAXSKILL],mdmg[DM_SIM_MAXSKILL],prog[DM_SIM_MAXSKILL];static dm_sim_char x;
        if(passive) {
            for(int id=0;id<DM_SIM_MAXSKILL;id++) {
                uint64_t node=skill_node(pl,id),record=node?*(uint64_t *)node:0;
                prog[id]=record && (*(uint8_t *)(record+0x24)&8)?a->stat_value[325]:0;
                mth[id]=record?((int (*)(uint64_t,uint64_t,uint64_t,int))(b+0x5b2a30))(pl,w,node,0):DM_SIM_NO_NODE;
                mdmg[id]=record?((int (*)(uint64_t,uint64_t,uint64_t,int))(b+0x5b2a30))(pl,w,node,1):DM_SIM_NO_NODE;
            }
            x=*a;
            if(dm_gear_hand_passive(d,c,&x,&cb,mth,mdmg,((int (*)(uint64_t,uint64_t,uint64_t,int))(b+0x5b2a30))(pl,w,0,2),prog)){a->capture_bad|=1|16;a->endpoint_bad=1;return 1;}
        }
        sim_values(d,a);a->nhands=1;memset(&a->hand[1],0,sizeof a->hand[1]);memset(&a->hand[0],0,sizeof a->hand[0]);
        sim_combat(a,&cb);
        if(!sim_weapon(b,w,mode,a->cls,&a->hand[0])){a->capture_bad|=1;a->endpoint_bad=1;}
        sim_ar_inputs(b,pl,w,&cb,a);a->mastery_cs=((int (*)(uint64_t,uint64_t,uint64_t,int))(b+0x5b2a30))(pl,w,0,2);
        if(passive){for(int id=0;id<DM_SIM_MAXSKILL;id++)if(mth[id]!=DM_SIM_NO_NODE){a->ar_bonus[id]=x.ar_bonus[id];a->damage_mastery_delta[id]=x.damage_mastery_delta[id];}a->mastery_cs=x.mastery_cs;}
        a->sias=sim_get(a,d,"attackrate")-100+a->hand[0].wsm;
        return 1;
    }
    dm_ic_cb cb[2];for(int h=0;h<2;h++)sim_dual_cb(b,pl,a->cls,&ba,after[h],1,nd,alvl,aura_off,&cb[h]);


    for(int h=0;h<2;h++)if(after[h]!=hands[h]) {
        uint64_t rn=item_rec(b,hands[h]),ra=item_rec(b,after[h]);
        if(rn && ra && *(uint16_t *)(rn+0x12e)==*(uint16_t *)(ra+0x12e))continue;
        if(a->cls==6 && sim_hand_weapon(b,hands[h]) && sim_claw(b,hands[h]))continue;
        dm_ic_cb was;int mast=cb[h].ar_mast || cb[h].dmast;
        if(sim_hand_weapon(b,hands[h])){sim_dual_cb(b,pl,a->cls,&bn,hands[h],1,nd,alvl-dsk,NULL,&was);mast|=was.ar_mast || was.dmast;}
        if(mast){a->capture_bad|=1|16;a->endpoint_bad=1;return 1;}
    }
    int filled=sim_dual_fill(b,d,pl,a,after,cb,dr_flag,passive?c:NULL);
    if(filled){a->capture_bad|=filled==-2?1|16:1;a->endpoint_bad=1;return 1;}
    int wbad=0;a->def=c->def;a->block=dm_sim_weapon_block(d,a,&wbad);a->shield=0;
    if(wbad){a->capture_bad|=1;a->endpoint_bad=1;return 1;}
    {int v[IC_C_N+1];if(dm_ic_cb_eval(&cb[0],v)){a->def=v[IC_C_DEF];}}

    a->sias=sim_get(a,d,"attackrate")-100;
    return 1;
}
