




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


static int sim_dual_class(uint8_t *b,int cls,uint64_t right,uint64_t left) {
    if(!sim_hand_weapon(b,right) || !sim_hand_weapon(b,left))return 0;
    if(cls==4)return 1;
    if(cls==6){const sim_gear *r=sim_gear_row(b,right),*l=sim_gear_row(b,left);return r && l && !strcmp(r->one,"HT1") && !strcmp(l->one,"HT1");}
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

static void sim_hand_from_cb(dm_sim_weapon *w,const dm_ic_cb *v) {
    w->min=v->mn[0]+v->bmn;w->max=v->mx[0]+v->bmx;
    w->strbonus=v->strb==1?100:v->strb;w->dexbonus=v->dexb==1?100:v->dexb;w->twohand=0;
    w->edmin=v->edmin;w->edmax=v->edmax;w->postflat=v->flat;
    const int ix[]={-1,0,2,6,9,4};
    for(int e=1;e<6;e++){w->elmin[e]=v->el[ix[e]];w->elmax[e]=v->el[ix[e]+1];}
}




static int sim_dual_fill(uint8_t *b,const dm_sim_data *d,uint64_t pl,dm_sim_char *c,const uint64_t *hand_items,const dm_ic_cb *cb,const int *dr_flag) {
    int values[2][IC_C_N+1];
    for(int h=0;h<2;h++)if(!dm_ic_cb_eval(&cb[h],values[h]))return -1;

    uint64_t r0=item_rec(b,hand_items[0]),r1=item_rec(b,hand_items[1]);
    if(!r0 || !r1)return -1;
    if(dm_ic_dual_types_passive(c->cls,*(uint16_t *)(r0+0x12e),*(uint16_t *)(r1+0x12e),cb[0].ar_mast || cb[0].dmast || cb[1].ar_mast || cb[1].dmast))return -2;
    dm_sim_weapon w[2];memset(w,0,sizeof w);
    for(int h=0;h<2;h++) {
        if(!sim_weapon(b,hand_items[h],1,c->cls,&w[h]))return -1;
        sim_hand_from_cb(&w[h],&cb[h]);
        w[h].ias=gstat(b,hand_items[h],93,0);
    }


    char cls[4]="HT2";
    if(c->cls==4){const char *l=w[1].wclass,*r=w[0].wclass;snprintf(cls,4,"1%c%c",!strcmp(l,"1HT")?'J':'S',!strcmp(r,"1HT")?'T':'S');}
    int known=0;for(int j=0;j<d->nanim;j++)if(!strcmp(d->anim[j].token,d->cls[c->cls].token) && !strcmp(d->anim[j].wclass,cls))known=1;
    if(!known)return -1;
    snprintf(w[0].wclass,4,"%s",cls);w[0].start_frame=w[1].start_frame=0;w[0].start_known=w[1].start_known=1;



    static const int extra[7]={136,135,60,62,117,121,122};
    int more[2][7]={{0}};
    for(int s=0;s<512;s++)if(dr_flag[s]>0 && d->stat_name[s][0] && s!=93 && s!=68) {
        int v0=gstat(b,hand_items[0],s,0),v1=gstat(b,hand_items[1],s,0),k=-1;
        for(int j=0;j<7;j++)if(extra[j]==s)k=j;
        if(k>=0){more[0][k]=v0;more[1][k]=v1;}
        c->stat_value[s]+=k>=0?(v0>v1?v0:v1):(v0+v1)/2;
    }
    for(int j=0;j<7;j++)if(dr_flag[extra[j]]<=0 || !d->stat_name[extra[j]][0])return -1;
    sim_values(d,c);
    int ds[2];for(int h=0;h<2;h++)ds[h]=gstat(b,hand_items[h],141,0);
    int ar_base[2],ar_pct[2],ed[2];
    for(int h=0;h<2;h++) {
        const dm_ic_cb *v=&cb[h];
        ar_base[h]=v->ar_zero?0:v->tohit+5*v->dex+v->thf-35;ar_pct[h]=v->tohitpct+v->ar_mast;ed[h]=v->dpct+v->dmast;
    }
    c->ar=(values[0][IC_C_AR]+values[1][IC_C_AR])/2;c->ar_base=(ar_base[0]+ar_base[1])/2;c->ar_base_known=1;
    c->ed=(ed[0]+ed[1])/2;
    for(int h=0;h<2;h++) {
        w[h].ar_base_d=ar_base[h]-c->ar_base;w[h].ar_pct_d=ar_pct[h]-(ar_pct[0]+ar_pct[1])/2;w[h].ed_d=ed[h]-c->ed;w[h].ds_d=ds[h]-(ds[0]+ds[1])/2;
        int *dv[7]={&w[h].cb_d,&w[h].ow_d,&w[h].ll_d,&w[h].ml_d,&w[h].pmh_d,&w[h].demon_d,&w[h].undead_d};
        for(int j=0;j<7;j++)*dv[j]=more[h][j]-(more[0][j]>more[1][j]?more[0][j]:more[1][j]);
    }
    c->nhands=2;c->hand[0]=w[0];c->hand[1]=w[1];

    int poison[2];for(int h=0;h<2;h++)poison[h]=cb[h].plen_ovr>0?cb[h].plen_ovr:cb[h].plen/(cb[h].pcnt>0?cb[h].pcnt:1);
    dm_sim_dual_poison(c,poison);

    for(int h=0;h<2;h++)if(raw_has(hand_items[h],195) || raw_has(hand_items[h],198))c->capture_bad|=SIM_GRANT_BAD;

    int charge=sim_charge_state(b,pl);if(charge<0)c->endpoint_bad=1;
    for(int id=0;id<DM_SIM_MAXSKILL;id++) {
        uint64_t node=skill_node(pl,id),record=node?*(uint64_t *)node:0;if(!record)continue;
        int bonus=0,delta=0;
        for(int h=0;h<2;h++){bonus+=cb[h].tohitpct+((int (*)(uint64_t,uint64_t,uint64_t,int))(b+0x5b2a30))(pl,hand_items[h],node,0);
            delta+=((int (*)(uint64_t,uint64_t,uint64_t,int))(b+0x5b2a30))(pl,hand_items[h],node,1)-cb[h].dmast;}
        c->ar_bonus[id]=bonus/2+((*(uint8_t *)(record+0x24)&8)?c->stat_value[325]:0);
        c->damage_mastery_delta[id]=delta/2;
        c->endpoint_state_bad[id]=charge>0 && (*(uint8_t *)(record+0x24)&8)!=0;
    }

    int mcs[2];for(int h=0;h<2;h++)mcs[h]=((int (*)(uint64_t,uint64_t,uint64_t,int))(b+0x5b2a30))(pl,hand_items[h],0,2);
    c->mastery_cs=(mcs[0]+mcs[1])/2;for(int h=0;h<2;h++)c->hand[h].cs_d=mcs[h]-c->mastery_cs;
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
    int filled=sim_dual_fill(b,d,pl,c,hands,cb,dr_flag);
    if(filled){c->capture_bad|=filled==-2?1|16:1;c->endpoint_bad=1;return;}
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
                          const uint64_t *hands,const uint64_t *after,const sim_dual_base *base_now,const int *aura_off,int dsk) {
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
        sim_values(d,a);a->nhands=1;memset(&a->hand[1],0,sizeof a->hand[1]);memset(&a->hand[0],0,sizeof a->hand[0]);
        sim_combat(a,&cb);
        if(!sim_weapon(b,w,mode,a->cls,&a->hand[0])){a->capture_bad|=1;a->endpoint_bad=1;}
        sim_ar_inputs(b,pl,w,&cb,a);a->mastery_cs=((int (*)(uint64_t,uint64_t,uint64_t,int))(b+0x5b2a30))(pl,w,0,2);
        a->sias=sim_get(a,d,"attackrate")-100+a->hand[0].wsm;
        return 1;
    }
    dm_ic_cb cb[2];for(int h=0;h<2;h++)sim_dual_cb(b,pl,a->cls,&ba,after[h],1,nd,alvl,aura_off,&cb[h]);


    for(int h=0;h<2;h++)if(after[h]!=hands[h]) {
        uint64_t rn=item_rec(b,hands[h]),ra=item_rec(b,after[h]);
        if(rn && ra && *(uint16_t *)(rn+0x12e)==*(uint16_t *)(ra+0x12e))continue;
        dm_ic_cb was;int mast=cb[h].ar_mast || cb[h].dmast;
        if(sim_hand_weapon(b,hands[h])){sim_dual_cb(b,pl,a->cls,&bn,hands[h],1,nd,alvl-dsk,NULL,&was);mast|=was.ar_mast || was.dmast;}
        if(mast){a->capture_bad|=1|16;a->endpoint_bad=1;return 1;}
    }
    int filled=sim_dual_fill(b,d,pl,a,after,cb,dr_flag);
    if(filled){a->capture_bad|=filled==-2?1|16:1;a->endpoint_bad=1;return 1;}
    a->def=c->def;a->block=0;a->shield=0;
    {int v[IC_C_N+1];if(dm_ic_cb_eval(&cb[0],v)){a->def=v[IC_C_DEF];}}

    a->sias=sim_get(a,d,"attackrate")-100;
    return 1;
}
