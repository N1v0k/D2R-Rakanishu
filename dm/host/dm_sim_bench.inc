
#ifndef DM_PLAYER_BUILD



static int g_sim_bench_ok, g_sim_bench_bad;
static struct { int have; uint32_t id; dm_sim_char c; } sim_bench;
static void sim_bench_lines(const dm_sim_data *d,const dm_sim_char *p,const dm_sim_char *g,char *out,size_t cap,int *bad) {
    dm_sim_target t;size_t k=0;*bad=0;out[0]=0;
    if(dm_sim_target_act(d,g,&t)<0){snprintf(out,cap," no standard enemy");*bad=1;return;}
    dm_sim_char pp,gp;dm_sim_tmod pm,gm;dm_sim_rot_out pr,gr;
    sim_prepare(d,p,&t,&pp,&pm,&pr);sim_prepare(d,g,&t,&gp,&gm,&gr);
#define CMPI(name,a,b) do { if((a)!=(b)){(*bad)++;k+=(size_t)snprintf(out+k,k<cap?cap-k:0," %s %d/%d",name,(int)(a),(int)(b));} } while(0)
#define CMPD(name,a,b) do { if(fabs((a)-(b))>1e-6*fmax(1,fabs(b))){(*bad)++;k+=(size_t)snprintf(out+k,k<cap?cap-k:0," %s %.2f/%.2f",name,(double)(a),(double)(b));} } while(0)
    CMPD("life",pp.life,gp.life);CMPD("mana",pp.mana,gp.mana);CMPI("str",pp.str,gp.str);CMPI("dex",pp.dex,gp.dex);
    CMPI("def",pp.def,gp.def);CMPI("block",pp.block,gp.block);CMPI("ar",pp.ar,gp.ar);CMPI("ar_pct",pp.ar_pct,gp.ar_pct);CMPI("ed",pp.ed,gp.ed);
    CMPI("ias",pp.ias,gp.ias);CMPI("sias",pp.sias,gp.sias);CMPI("fcr",pp.fcr,gp.fcr);CMPI("fhr",pp.fhr,gp.fhr);
    CMPI("liferegen",pp.liferegen,gp.liferegen);CMPI("manaregen",pp.manaregen_pct,gp.manaregen_pct);CMPI("ll",pp.ll,gp.ll);CMPI("ml",pp.ml,gp.ml);
    CMPI("dr",pp.dr_flat,gp.dr_flat);CMPI("mdr",pp.mdr_flat,gp.mdr_flat);CMPI("cs",pp.cs,gp.cs);CMPI("ds",pp.ds,gp.ds);CMPI("cb",pp.cb,gp.cb);
    for(int e=0;e<DM_EL_N;e++){char n[16];snprintf(n,sizeof n,"res%d",e);CMPI(n,pp.res[e],gp.res[e]);snprintf(n,sizeof n,"maxres%d",e);CMPI(n,pp.maxres[e],gp.maxres[e]);
        snprintf(n,sizeof n,"tres%d",e);CMPI(n,pm.res[e],gm.res[e]);snprintf(n,sizeof n,"elmin%d",e);CMPI(n,pp.hand[0].elmin[e],gp.hand[0].elmin[e]);
        snprintf(n,sizeof n,"elmax%d",e);CMPI(n,pp.hand[0].elmax[e],gp.hand[0].elmax[e]);}
    CMPI("min",pp.hand[0].min,gp.hand[0].min);CMPI("max",pp.hand[0].max,gp.hand[0].max);CMPI("hands",pp.nhands,gp.nhands);
    CMPI("wsm",pp.hand[0].wsm,gp.hand[0].wsm);CMPI("hias",pp.hand[0].ias,gp.hand[0].ias);
    if(strcmp(pp.hand[0].wclass,gp.hand[0].wclass)){(*bad)++;k+=(size_t)snprintf(out+k,k<cap?cap-k:0," class %s/%s",pp.hand[0].wclass,gp.hand[0].wclass);}
    for(int h=0;h<2 && gp.nhands==2 && pp.nhands==2;h++) {
        const dm_sim_weapon *x=&pp.hand[h],*y=&gp.hand[h];char n[24];
#define CMPH(field) do { snprintf(n,sizeof n,"h%d." #field,h);CMPI(n,x->field,y->field); } while (0)
        CMPH(min);CMPH(max);CMPH(wsm);CMPH(ias);CMPH(ar_base_d);CMPH(ar_pct_d);CMPH(ed_d);CMPH(ds_d);CMPH(cs_d);CMPH(edmin);CMPH(edmax);CMPH(strbonus);CMPH(dexbonus);
        CMPH(cb_d);CMPH(ow_d);CMPH(ll_d);CMPH(ml_d);CMPH(pmh_d);CMPH(demon_d);CMPH(undead_d);
        for(int e=1;e<DM_EL_N;e++){snprintf(n,sizeof n,"h%d.elmin%d",h,e);CMPI(n,x->elmin[e],y->elmin[e]);snprintf(n,sizeof n,"h%d.elmax%d",h,e);CMPI(n,x->elmax[e],y->elmax[e]);}
#undef CMPH
    }
    CMPI("ar_base",pp.ar_base,gp.ar_base);CMPI("pframes",pp.poison_frames,gp.poison_frames);CMPI("pmh",pp.prevent_heal,gp.prevent_heal);CMPI("cb",pp.cb,gp.cb);CMPI("ow",pp.ow,gp.ow);CMPI("tdef",pm.def_pct,gm.def_pct);CMPI("tdmg",pm.dmg_pct,gm.dmg_pct);
    for(int i=0;i<gp.nbar;i++){int id=gp.bar[i];char n[16];snprintf(n,sizeof n,"lvl%d",id);CMPI(n,pp.lvl[id],gp.lvl[id]);}
    for(int i=0;i<g->ngrant;i++){int id=g->grant[i].skill;char n[16];snprintf(n,sizeof n,"glvl%d",id);CMPI(n,pp.lvl[id]?pp.lvl[id]:0,gp.lvl[id]?gp.lvl[id]:0);}
    dm_gear_measures pmz,gmz;
    if(!dm_gear_measure(d,p,0,&pmz) && !dm_gear_measure(d,g,0,&gmz)) {
        CMPD("group",pmz.group,gmz.group);CMPD("single",pmz.single,gmz.single);CMPD("hits",pmz.hits,gmz.hits);CMPD("survival",pmz.survival_s,gmz.survival_s);
    } else {(*bad)++;k+=(size_t)snprintf(out+k,k<cap?cap-k:0," measures unavailable");}
#undef CMPI
#undef CMPD
}
static int sim_bench_read_predict(const dm_sim_data *d,void *ctx) {
    sim_capture *q=ctx;int rc=sim_capture_read(d,q);
    if(!rc){sim_bench.c=q->after.c;sim_bench.have=1;}
    return rc;
}
static int sim_bench_read_check(const dm_sim_data *d,void *ctx) {
    sim_capture *q=ctx;int rc=sim_capture_read(d,q);if(rc)return rc;
    static char line[1600];int bad=0;
    unsigned pb=sim_bench.c.capture_bad,gb=q->now.c.capture_bad;
    char grants[400];size_t k=0;grants[0]=0;
    for(int i=0;i<q->now.c.ngrant;i++){const dm_sim_grant *g=&q->now.c.grant[i];k+=(size_t)snprintf(grants+k,k<sizeof grants?sizeof grants-k:0," %d:%d:%d/%d:%d:%d",g->kind,g->skill,g->lvl,g->held,g->chance,g->charges);}
    {
        static char nodes[600];size_t n=0;nodes[0]=0;uint64_t l=*(uint64_t *)(q->pl+0x100),nd=l?*(uint64_t *)l:0;
        for(int i=0;nd && i<1024;i++,nd=*(uint64_t *)(nd+8)) {
            uint64_t r=*(uint64_t *)nd;if(!r || *(int32_t *)(nd+0x44)==-1)continue;
            n+=(size_t)snprintf(nodes+n,n<sizeof nodes?sizeof nodes-n:0," %d:%d+%d(%d)",*(int16_t *)r,*(int32_t *)(nd+0x38),((int (*)(uint64_t,uint64_t,int))(q->b+0x5b1710))(q->pl,nd,0),*(int32_t *)(nd+0x44));
        }
        static char states[900];size_t m=0;states[0]=0;
        for(int i=0;i<q->now.c.ngrant;i++) {
            const dm_sim_grant *g=&q->now.c.grant[i];if(g->kind!=DM_GR_AURA && g->kind!=DM_GR_BUFF)continue;
            int64_t node=sim_state_node(d,q->pl,g->skill);if(node<=0)continue;
            uint64_t arr=*(uint64_t *)((uint64_t)node+0x30),count=*(uint64_t *)((uint64_t)node+0x38);
            m+=(size_t)snprintf(states+m,m<sizeof states?sizeof states-m:0," [%d]",g->skill);
            for(uint64_t j=0;arr && j<count && j<64;j++)m+=(size_t)snprintf(states+m,m<sizeof states?sizeof states-m:0," %u/%u=%d",*(uint32_t *)(arr+8*j)>>16,*(uint32_t *)(arr+8*j)&0xffff,*(int32_t *)(arr+8*j+4));
        }
        LOG("[sim-bench] id %u: item skill nodes:%s; grant states:%s",sim_bench.id,n?nodes:" none",m?states:" none");
    }
    if((pb|gb)&(1|2|16)){snprintf(line,sizeof line," not comparable (capture %u / %u)",pb,gb);LOG("[sim-bench] id %u:%s; game grants:%s",sim_bench.id,line,k?grants:" none");return 0;}
    sim_bench_lines(d,&sim_bench.c,&q->now.c,line,sizeof line,&bad);
    if(bad)g_sim_bench_bad++;else g_sim_bench_ok++;
    LOG("[sim-bench] id %u: %s (predicted / game%s); game grants:%s; predicted capture %u game %u",sim_bench.id,bad?"MISMATCH":"exact",line,k?grants:" none",pb,gb);
    return 0;
}

static void sim_dual_probe(uint8_t *b,uint64_t pl) {
    static int on=-1;if(on<0)on=env_int("DM_TEST_DUAL_PROBE",0);
    if(!on)return;
    uint64_t hands[2]={0};
    for(uint64_t it=first_item(pl);it;it=next_item(it)){uint64_t di=*(uint64_t *)(it+0x10);if(di && *(uint32_t *)(it+0xc)==1){int sl=*(uint8_t *)(di+0x54);if(sl==4 || sl==5)hands[sl-4]=it;}}
    static uint64_t seen[3];static int ticks;uint64_t key=(uint64_t)(hands[0]?*(uint32_t *)(hands[0]+8):0)<<32|(hands[1]?*(uint32_t *)(hands[1]+8):0);
    unsigned mode=*(uint32_t *)(pl+0xc);
    if(key==seen[0] && mode==seen[1] && ++ticks%250)return;
    seen[0]=key;seen[1]=mode;
    static const int st[]={17,18,19,21,22,23,24,25,48,49,60,68,93,119,136,141,0,2,7,31,39};
    char line[1500];size_t k=0;
    uint64_t inv=*(uint64_t *)(pl+0x90),sel=inv?((uint64_t (*)(uint64_t))(b+0x560e70))(inv):0;
    k+=(size_t)snprintf(line+k,sizeof line-k,"mode %u selected %u hand mode %d page path %d;",mode,sel?*(uint32_t *)(sel+8):0,inv?((int (*)(uint64_t,uint64_t,int))(b+0x560d20))(pl,inv,0):-1,((int (*)(uint64_t))(b+0x5cb9e0))(pl));
    for(int h=0;h<2;h++){uint64_t it=hands[h],l=it?*(uint64_t *)(it+0x88):0;k+=(size_t)snprintf(line+k,k<sizeof line?sizeof line-k:0," slot %d id %u list flags %#x %#x;",4+h,it?*(uint32_t *)(it+8):0,l?*(uint32_t *)(l+0x18):0,l?*(uint32_t *)(l+0x1c):0);}
    for(size_t i=0;i<sizeof st/sizeof *st;i++)k+=(size_t)snprintf(line+k,k<sizeof line?sizeof line-k:0," %d=%d/%d/%d",st[i],gstat(b,pl,st[i],0),gstat(b,hands[0],st[i],0),gstat(b,hands[1],st[i],0));
    uint64_t nd=skill_node(pl,0);
    if(nd)for(int h=0;h<2;h++)if(hands[h]){int v[IC_C_N+3]={0};int lvl=*(int32_t *)(nd+0x38)+((int (*)(uint64_t,uint64_t,int))(b+0x5b1710))(pl,nd,0);cb_game(b,pl,nd,lvl,hands[h],v);
        k+=(size_t)snprintf(line+k,k<sizeof line?sizeof line-k:0,"; game with slot %d: ar %d dmg %d-%d phys %d-%d",4+h,v[IC_C_AR],v[IC_C_DMG],v[IC_C_N],v[IC_C_N+1],v[IC_C_N+2]);}


    int cls=(int)*(uint32_t *)(pl+4);
    if(nd && sim_dual_class(b,cls,hands[0],hands[1])) {
        sim_dual_base base;sim_dual_base_now(b,pl,hands,&base);int lvl=*(int32_t *)(nd+0x38)+((int (*)(uint64_t,uint64_t,int))(b+0x5b1710))(pl,nd,0);
        for(int h=0;h<2;h++){dm_ic_cb m;sim_dual_cb(b,pl,cls,&base,hands[h],1,nd,lvl,NULL,&m);int v[IC_C_N+1]={0};int ok=base.ok && dm_ic_cb_eval(&m,v);
            k+=(size_t)snprintf(line+k,k<sizeof line?sizeof line-k:0,"; model slot %d (%s, code %.3s): ar %d dmg %d-%d%s",4+h,sim_list_idle(hands[h])?"idle":"swinging",(const char *)&(uint32_t){item_code(b,hands[h])},v[IC_C_AR],v[IC_C_DMG],v[IC_C_N],ok?"":" unreadable");}
    }
    LOG("[sim-dual] %s",line);
}

static void sim_bench_predict(uint8_t *b,uint64_t pl,uint64_t cand,const uint64_t *olds,int nold,int sl) {
    sim_bench.have=0;sim_bench.id=cand?*(uint32_t *)(cand+8):0;
    sim_capture *q=calloc(1,sizeof *q);if(!q)return;
    q->b=b;q->pl=pl;q->cand=cand;q->nold=nold;q->sl=sl;q->swap=1;q->mask=0;
    for(int i=0;i<nold && i<4;i++){q->olds[i]=olds[i];uint64_t di=olds[i]?*(uint64_t *)(olds[i]+0x10):0;if(di)q->mask|=1<<*(uint8_t *)(di+0x54);}
    if(!q->mask && sl>0 && sl<16)q->mask=1<<sl;

    static int gd[DM_GEAR_NSTAT];if(g_bench_gear_on){bench_gear_deltas(gd);q->gdelta=gd;}
    dm_sim_try_read(*(uint8_t *)(pl+0x1bd)==3?"rotw":"lod",sim_bench_read_predict,q);
    free(q);
}

static void sim_bench_check(uint8_t *b,uint64_t pl) {
    if(!sim_bench.have)return;
    sim_capture *q=calloc(1,sizeof *q);if(!q)return;
    q->b=b;q->pl=pl;
    if(dm_sim_try_read(*(uint8_t *)(pl+0x1bd)==3?"rotw":"lod",sim_bench_read_check,q))LOG("[sim-bench] id %u: tables busy",sim_bench.id);
    sim_bench.have=0;free(q);
}
#endif
