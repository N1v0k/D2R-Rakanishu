
#include "../dm_sim_internal.h"
#include "sim_test_io.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#define NEAR(a,b) assert(fabs((double)(a)-(double)(b))<1e-6)
static dm_sim_data d;
static dm_sim_skill skills[4];
static dm_sim_char c;
static dm_sim_target t;
static dm_sim_trace tr,again;


static void reset(void) {
    memset(&d,0,sizeof d);memset(skills,0,sizeof skills);d.ok=1;d.ncls=8;dm_sim_char_init(&c,1);
    c.clvl=50;c.life=1000;c.mana=1000;c.hand[0].min=c.hand[0].max=100;c.hand[0].strbonus=0;c.hand[0].kind=DM_WK_MELEE;
    for(int i=0;i<DM_ST_N;i++)d.stat[i]=-1;
    for(int i=0;i<3;i++){d.diff[i].lsd=d.diff[i].msd=1;d.diff[i].pvspe=100;}
    for(int i=0;i<4;i++) {
        dm_sim_skill *s=&skills[i];s->id=i;s->cls=1;s->fam=DM_FAM_Z1;s->hitshift=s->manashift=8;s->srcdam=128;s->etype=DM_EL_FIRE;s->emin=s->emax=100;
        s->alwayshit=1;s->summon=s->missile=s->descmissile=-1;strcpy(s->anim,"SC");
        for(int j=0;j<DM_CF_N;j++)s->calc[j]=-1;
        for(int j=0;j<DM_ROLE_N;j++)s->role[j]=-1;
        for(int j=0;j<6;j++)s->astat[j]=-1;
        for(int j=0;j<14;j++)s->pstat[j]=-1;
        for(int j=0;j<5;j++)s->sumskill[j]=-1;
        d.skill[i]=s;c.lvl[i]=1;
    }
    skills[0].fam=DM_FAM_W1;skills[0].emin=skills[0].emax=0;strcpy(skills[0].anim,"A1");
    c.bar[0]=1;c.nbar=1;
    memset(&t,0,sizeof t);t.hp=1000;t.mlvl=50;t.cycle=25;t.dmin=t.dmax=100;t.drain=100;t.ar=100000;c.def=0;

    const dm_sim_potion heal={DM_PK_HEAL,100,0,100},mana={DM_PK_MANA,0,500,50},rejuv={DM_PK_REJUV,35,35,0};
    for(int k=DM_POT_HP1;k<DM_POT_MP1;k++)d.potion[k]=heal;
    for(int k=DM_POT_MP1;k<DM_POT_RVS;k++)d.potion[k]=mana;
    d.potion[DM_POT_RVS]=rejuv;d.potion[DM_POT_RVL]=rejuv;d.potion[DM_POT_RVL].life=d.potion[DM_POT_RVL].mana=100;
    d.potions_loaded=(1u<<DM_POT_N)-1;
    for(int i=0;i<8;i++)d.cls[i].hppot=d.cls[i].mppot=100;
}
static int drunk(const dm_sim_trace *x,int kind){int n=0;for(int i=0;i<x->npotion;i++)n+=x->potion[i].kind==kind;return n;}
static int shaded(const dm_sim_trace *x){int n=0;for(int i=0;i<x->n;i++)n+=x->b[i].shortage;return n;}

static void drinks_on_lines(const dm_sim_trace *x) {
    for(int i=0;i<x->npotion;i++) {
        const dm_sim_trace_bucket *b=&x->b[x->potion[i].frame/x->width];
        assert(b->used && b->lo[DM_TS_LIFE]<=x->potion[i].life+1e-4f && b->lo[DM_TS_MANA]<=x->potion[i].mana+1e-4f);
    }
}
static void trace_shape(const dm_sim_trace *x) {
    assert(x->n>0 && x->n<=DM_SIM_TRACE_BUCKETS && x->width>0 && !(x->width&(x->width-1)));
    assert((x->n-1)*x->width<=x->end && x->end<x->n*x->width);
    for(int i=0;i<x->n;i++)for(int s=0;s<DM_TS_N;s++) {
        const dm_sim_trace_bucket *b=&x->b[i];
        assert(b->used && b->lo[s]<=b->last[s] && b->last[s]<=b->hi[s] && b->lo[s]>=0 && b->hi[s]<=100);
    }
}

static double kill_without_hits(void) {
    dm_sim_char k=c;k.capture_bad|=2;dm_sim_rot_out o;dm_sim_surv_out s;
    assert(!dm_sim_fight(&d,&k,&t,&o,&s,NULL));return o.ttk;
}
static void joint_tests(void) {


    reset();dm_sim_rot_out r,alone;dm_sim_surv_out s,surv;
    assert(!dm_sim_fight(&d,&c,&t,&r,&s,&tr));assert(!dm_sim_rotation(&d,&c,&t,&alone));assert(!dm_sim_survive(&d,&c,&t,&alone,&surv));
    NEAR(alone.ttk,-2);NEAR(r.ttk,kill_without_hits());NEAR(s.ttd,surv.ttd);NEAR(r.dps,alone.dps);NEAR(s.hits_to_death,surv.hits_to_death);
    assert(r.ttk>0 && s.ttd>r.ttk);
    trace_shape(&tr);assert(tr.end_kind==DM_END_KILL && tr.end==(int)lround(r.ttk*25));
    for(int k=0;k<DM_TS_N;k++){assert(tr.b[0].hi[k]==100);}
    assert(tr.b[tr.n-1].last[DM_TS_ENEMY]==0 && tr.b[tr.n-1].last[DM_TS_LIFE]>0);
    for(int i=1;i<tr.n;i++)assert(tr.b[i].hi[DM_TS_ENEMY]<=tr.b[i-1].last[DM_TS_ENEMY]+1e-4);

    uint64_t acts=0;for(int i=0;i<tr.n;i++)acts|=tr.b[i].acts;
    assert(tr.nskill==2 && tr.b[0].acts && !(acts>>tr.nskill));

    t.dmin=t.dmax=400;t.hp=5000;
    assert(!dm_sim_fight(&d,&c,&t,&r,&s,&tr));assert(!dm_sim_rotation(&d,&c,&t,&alone));assert(!dm_sim_survive(&d,&c,&t,&alone,&surv));
    NEAR(r.ttk,kill_without_hits());NEAR(s.ttd,surv.ttd);assert(s.ttd>0 && r.ttk>s.ttd);
    trace_shape(&tr);assert(tr.end_kind==DM_END_DEATH && tr.end==(int)lround(s.ttd*25) && tr.b[tr.n-1].last[DM_TS_LIFE]==0 && tr.b[tr.n-1].last[DM_TS_ENEMY]>0);

    reset();t.hp=1000000;t.dmin=t.dmax=1;
    assert(!dm_sim_fight(&d,&c,&t,&r,&s,&tr));NEAR(r.ttk,-1);NEAR(s.ttd,-1);
    trace_shape(&tr);assert(tr.end_kind==DM_END_NONE && tr.end==DM_SIM_HORIZON && tr.width>1);

    assert(!dm_sim_fight(&d,&c,&t,&r,&s,&again));assert(!memcmp(&tr,&again,sizeof tr));

    reset();skills[1].mana=10;skills[1].emin=skills[1].emax=300;c.mana=100;t.hp=5000;t.dmin=t.dmax=1;
    assert(!dm_sim_fight(&d,&c,&t,&r,&s,&tr));assert(tr.best==1);
    int first=-1;for(int i=0;i<tr.n && first<0;i++)if(tr.b[i].shortage)first=i;
    assert(first>0 && first*tr.width<=10*15+tr.width && (first+1)*tr.width>9*15);

    reset();skills[1].mana=40;skills[1].emin=skills[1].emax=300;c.mana=400;c.es_pct=50;c.es_ratio=16;t.hp=3000;t.dmin=t.dmax=60;
    assert(!dm_sim_fight(&d,&c,&t,&r,&s,&tr));assert(kill_without_hits()>0 && r.ttk>kill_without_hits());

    reset();c.capture_bad=2;assert(!dm_sim_fight(&d,&c,&t,&r,&s,&tr));assert(tr.n==0 && s.ttd==-2 && r.ttk>0);
}
static void exact_tests(void) {


    reset();c.nbar=0;c.hand[0].start_known=1;c.hand[0].min=60;c.hand[0].max=140;t.hp=1000;t.ac=2000;t.dmin=t.dmax=1;dm_sim_rot_out r,alone,row;dm_sim_surv_out s;
    dm_sim_anim swing={.mode="A1",.wclass="HTH",.frames=16,.speed=256,.action=6};d.anim=&swing;d.nanim=1;strcpy(c.hand[0].wclass,"HTH");
    assert(!dm_sim_rotation(&d,&c,&t,&alone));assert(alone.ttk>0);
    assert(!dm_sim_fight(&d,&c,&t,&r,&s,&tr));NEAR(r.ttk,alone.ttk);
    assert(!dm_sim_fight(&d,&c,&t,&row,&s,NULL));NEAR(row.ttk,alone.ttk);
    assert(tr.end_kind==DM_END_KILL && fabs(tr.end-alone.ttk*25)>1);

    t.regen=10;assert(!dm_sim_rotation(&d,&c,&t,&alone));NEAR(alone.ttk,-2);assert(!dm_sim_fight(&d,&c,&t,&r,&s,NULL));assert(r.ttk>0);
    d.anim=NULL;d.nanim=0;
}


static int kill_by_events(int casts,int period,int delay,double damage,int len,double life) {
    for(int k=0;k<casts;k++) {
        if(life<=damage)return k*period+delay+(int)ceil(life/(damage/len));
        life-=damage;
    }
    return -1;
}


static dm_sim_miss slow;
static void travelling(void) {
    reset();memset(&slow,0,sizeof slow);slow.vel=18;for(int j=0;j<DM_MF_N;j++)slow.calc[j]=-1;d.miss[0]=&slow;
    skills[0].requiresweapon=1;c.hand[0].kind=DM_WK_NONE;c.mana=170;
    skills[1].missile=0;skills[1].mana=10;skills[1].etype=DM_EL_POIS;skills[1].emin=skills[1].emax=10;skills[1].elen=10;
    skills[2].missile=0;skills[2].mana=10;skills[2].fam=DM_FAM_Z3;skills[2].frameburn=2;
    t.hp=1650;t.dmin=t.dmax=1;
}
static void early_end_tests(void) {
    dm_sim_rot_out r,row;dm_sim_surv_out s,rs;


    travelling();dm_sim_skill_out o;dm_sim_tmod none;dm_sim_tmod_init(&none);
    assert(!sim_action(&d,&c,&t,&none,1,&o) && o.frames==15 && o.cooldown==15 && 1+o.travel==15 && o.dot==100 && o.dot_frames==10 && !sim_sum(o.hit));
    assert(kill_by_events(17,15,15,100,10,t.hp)==260);
    assert(!dm_sim_fight(&d,&c,&t,&r,&s,&tr));assert(!dm_sim_fight(&d,&c,&t,&row,&rs,NULL));
    NEAR(r.ttk,10.4);NEAR(row.ttk,r.ttk);NEAR(s.ttd,-1);NEAR(rs.ttd,-1);


    travelling();c.bar[0]=2;skills[2].emin=skills[2].emax=10;t.hp=2450;
    assert(!sim_action(&d,&c,&t,&none,2,&o) && o.burn==150 && o.burn_frames==15 && 1+o.travel==15 && !sim_sum(o.hit));
    assert(kill_by_events(17,15,15,150,15,t.hp)==260);
    assert(!dm_sim_fight(&d,&c,&t,&r,&s,&tr));assert(!dm_sim_fight(&d,&c,&t,&row,&rs,NULL));
    NEAR(r.ttk,10.4);NEAR(row.ttk,r.ttk);

    reset();t.hp=100000;t.regen=4096;t.dmin=t.dmax=1;c.liferegen=256;
    assert(!dm_sim_fight(&d,&c,&t,&r,&s,&tr));assert(!dm_sim_fight(&d,&c,&t,&row,&rs,NULL));
    NEAR(r.ttk,-1);NEAR(row.ttk,-1);NEAR(s.ttd,-1);NEAR(rs.ttd,-1);assert(tr.end_kind==DM_END_NONE && tr.end==DM_SIM_HORIZON);

    t.dmin=t.dmax=40;c.liferegen=0;c.life=10000;
    assert(!dm_sim_fight(&d,&c,&t,&r,&s,&tr));assert(!dm_sim_fight(&d,&c,&t,&row,&rs,NULL));
    NEAR(r.ttk,-1);NEAR(row.ttk,-1);assert(s.ttd>0);NEAR(rs.ttd,s.ttd);

    reset();t.dmin=t.dmax=1;assert(!dm_sim_fight(&d,&c,&t,&r,&s,&tr));assert(!dm_sim_fight(&d,&c,&t,&row,&rs,NULL));
    assert(r.ttk>0);NEAR(row.ttk,r.ttk);NEAR(s.ttd,-1);NEAR(rs.ttd,-1);
}
static void belt_tests(void) {
    dm_sim_rot_out r;dm_sim_surv_out s,dry;

    reset();t.hp=1000000;t.dmin=t.dmax=50;c.potions=1;
    assert(!dm_sim_fight(&d,&c,&t,&r,&dry,&tr));assert(tr.npotion==0);
    c.belt[DM_POT_HP1]=3;assert(!dm_sim_fight(&d,&c,&t,&r,&s,&tr));
    assert(drunk(&tr,DM_POT_HP1)==3 && tr.npotion==3);NEAR(s.ttd,dry.ttd+6);drinks_on_lines(&tr);
    for(int i=0;i<tr.npotion;i++){assert(tr.potion[i].life<50);if(i)assert(tr.potion[i].frame-tr.potion[i-1].frame>=100);}

    d.cls[1].hppot=150;assert(!dm_sim_fight(&d,&c,&t,&r,&s,&tr));NEAR(s.ttd,dry.ttd+9);d.cls[1].hppot=100;

    c.belt[DM_POT_HP1]=0;c.belt[DM_POT_RVS]=1;c.belt[DM_POT_RVL]=1;assert(!dm_sim_fight(&d,&c,&t,&r,&s,&tr));
    assert(tr.npotion==2 && tr.potion[0].kind==DM_POT_RVL && tr.potion[1].kind==DM_POT_RVS && tr.potion[0].life<25 && tr.potion[1].life<25);
    assert(s.ttd>dry.ttd+10);drinks_on_lines(&tr);

    c.potions=0;assert(!dm_sim_fight(&d,&c,&t,&r,&s,&tr));assert(tr.npotion==0);NEAR(s.ttd,dry.ttd);

    reset();t.hp=1000000;t.dmin=t.dmax=330;c.potions=1;c.belt[DM_POT_RVS]=1;assert(!dm_sim_fight(&d,&c,&t,&r,&s,&tr));
    assert(tr.npotion==1 && tr.potion[0].life<10 && tr.width==1);drinks_on_lines(&tr);
    NEAR(tr.b[tr.potion[0].frame].lo[DM_TS_LIFE],tr.potion[0].life);assert(tr.b[tr.potion[0].frame].last[DM_TS_LIFE]>tr.potion[0].life+30);

    reset();skills[1].mana=50;skills[1].emin=skills[1].emax=300;c.mana=100;c.potions=1;t.dmin=t.dmax=1;t.hp=3000;
    assert(!dm_sim_fight(&d,&c,&t,&r,&dry,&tr));double slow=r.ttk;assert(tr.npotion==0);
    c.belt[DM_POT_MP1+4]=2;assert(!dm_sim_fight(&d,&c,&t,&r,&s,&tr));
    assert(drunk(&tr,DM_POT_MP1+4)==2 && r.ttk<slow);for(int i=0;i<tr.npotion;i++)assert(tr.potion[i].mana<50);
    drinks_on_lines(&tr);assert(tr.best==1);

    d.potions_loaded=0;assert(!dm_sim_fight(&d,&c,&t,&r,&s,&tr));assert(tr.npotion==0);NEAR(r.ttk,slow);

    reset();skills[1].mana=10;skills[1].emin=skills[1].emax=50;c.mana=50;c.ordered=1;c.potions=1;t.dmin=t.dmax=1;t.hp=100000;
    assert(!dm_sim_fight(&d,&c,&t,&r,&dry,&tr));assert(shaded(&tr)>0 && tr.best==1 && tr.npotion==0);
    c.belt[DM_POT_MP1+4]=2;assert(!dm_sim_fight(&d,&c,&t,&r,&s,&tr));
    assert(drunk(&tr,DM_POT_MP1+4)==2 && tr.best==1);drinks_on_lines(&tr);for(int i=0;i<tr.npotion;i++)assert(tr.potion[i].mana<20);

    reset();skills[1].mana=200;skills[1].emin=skills[1].emax=300;c.mana=100;c.potions=1;c.belt[DM_POT_MP1+4]=2;t.dmin=t.dmax=1;t.hp=100000;
    assert(!dm_sim_fight(&d,&c,&t,&r,&s,&tr));assert(tr.npotion==0 && !shaded(&tr) && tr.best==-1);
}
static void profile_tests(void) {
    reset();char err[80];
    assert(!dm_sim_profile_parse(&d,"class=1\nlevel=10\npotions=1\nbelt=hp2:3,rvs:2,hp1:1\n",&c,err,sizeof err));
    assert(c.potions==1 && c.belt[DM_POT_HP1+1]==3 && c.belt[DM_POT_RVS]==2 && c.belt[DM_POT_HP1]==1 && c.belt[DM_POT_MP1]==0);
    assert(dm_sim_profile_parse(&d,"class=1\nbelt=xx1:1\n",&c,err,sizeof err)<0);
    assert(dm_sim_profile_parse(&d,"class=1\nbelt=hp1:9,hp2:9\n",&c,err,sizeof err)<0);
    assert(dm_sim_profile_parse(&d,"class=1\nbelt=hp1\n",&c,err,sizeof err)<0);
}
static int acted(const dm_sim_trace *x,int skill){for(int k=0;k<x->nskill && k<64;k++)if(x->skill[k]==skill)for(int i=0;i<x->n;i++)if(x->b[i].acts>>k&1)return 1;return 0;}
static int listed(const dm_sim_rot_out *r,int skill){for(int k=0;k<r->nseq;k++)if(r->seq[k]==skill)return 1;return 0;}

static void minion_tests(void) {
    reset();t.dmin=t.dmax=0;dm_sim_rot_out alone,r;dm_sim_surv_out s;
    assert(!dm_sim_fight(&d,&c,&t,&alone,&s,&tr));
    skills[2].fam=DM_FAM_P;skills[2].mana=50;c.bar[1]=2;c.nbar=2;
    dm_sim_skill_out o;assert(!dm_sim_skill_eval(&d,&c,&t,2,&o));
    assert(o.flags&DM_SF_MINION && o.pets==1 && o.pet_dps>0 && o.mana_x256>0);
    assert(!dm_sim_fight(&d,&c,&t,&r,&s,&tr));
    assert(!acted(&tr,2) && !listed(&r,2) && r.nseq==1 && acted(&tr,r.seq[0]));
    assert(r.ttk>0 && r.ttk<alone.ttk && r.dps>alone.dps);

    assert(r.mana_spent<=alone.mana_spent);

    assert(tr.b[0].acts);
    assert(!dm_sim_rotation(&d,&c,&t,&r));assert(!listed(&r,2) && r.mana_seconds<0);
    skills[2].fam=DM_FAM_F;
    assert(!dm_sim_skill_eval(&d,&c,&t,2,&o));assert(!(o.flags&DM_SF_MINION) && !o.pets);
    assert(!dm_sim_fight(&d,&c,&t,&r,&s,&tr));assert(acted(&tr,2) && listed(&r,2));
}


static void real_minions(const dm_sim_data *real) {
    int checked=0,flagged=0;
    for(int i=0;i<real->nskill;i++) {
        const dm_sim_skill *k=&real->skills[i];if(!(k->fam&DM_FAM_P) || k->fam&DM_FAM_F || k->timed || k->cls<0 || k->cls>7)continue;
        for(int lv=1;lv<=20;lv+=19) {
            dm_sim_char w;dm_sim_char_init(&w,k->cls);w.clvl=lv==1?k->reqlevel>1?k->reqlevel:1:k->reqlevel+19;w.diff=0;w.life=50*lv+40;w.mana=20*lv+20;
            w.hand[0].min=2;w.hand[0].max=6;w.hand[0].kind=DM_WK_MELEE;w.lvl[0]=1;w.lvl[k->id]=w.blvl[k->id]=lv;w.bar[0]=k->id;w.nbar=1;
            dm_sim_target std;if(dm_sim_target_std(real,0,0,&std))continue;
            dm_sim_skill_out o;if(dm_sim_skill_eval(real,&w,&std,k->id,&o) || o.flags&DM_SF_UNAVAIL)continue;
            checked++;
            if(o.dur<DM_SIM_HORIZON){assert(!(o.flags&DM_SF_MINION));continue;}
            assert(o.flags&DM_SF_MINION && o.pets>=1);flagged++;
            dm_sim_rot_out r,attack;dm_sim_surv_out s;
            assert(!dm_sim_fight(real,&w,&std,&r,&s,&tr));

            assert(!acted(&tr,k->id) && !listed(&r,k->id));
            dm_sim_char bare=w;bare.nbar=0;assert(!dm_sim_fight(real,&bare,&std,&attack,&s,NULL));
            assert(r.mana_spent<=attack.mana_spent+1e-9);
            if(attack.ttk>0)assert(r.ttk>0 && r.ttk<=attack.ttk+1e-9);
        }
    }
    printf("standing summons: %d of %d summon states up from the start, none cast\n",flagged,checked);
    assert(flagged>=20);

    const dm_sim_skill *creeper=real->skill[222];assert(creeper && creeper->fam&DM_FAM_P);
    for(int lv=1;lv<=20;lv+=19) {
        dm_sim_char w;dm_sim_char_init(&w,creeper->cls);w.clvl=creeper->reqlevel+lv-1;w.diff=0;w.lvl[0]=1;w.lvl[222]=w.blvl[222]=lv;w.bar[0]=222;w.nbar=1;
        dm_sim_target std;assert(!dm_sim_target_std(real,0,0,&std));
        dm_sim_skill_out o;assert(!dm_sim_skill_eval(real,&w,&std,222,&o) && o.flags&DM_SF_MINION && o.pets==1);
        int bad=0;double per_frame=(dm_sim_skill_value(real,&w,222,lv,DM_SV_EMIN,&bad)+dm_sim_skill_value(real,&w,222,lv,DM_SV_EMAX,&bad))/2;assert(!bad && per_frame>0);
        double expect=per_frame*25*o.cth*(100-std.res[DM_EL_POIS])/100.0;
        printf("poison creeper level %d: %.2f poison per second\n",lv,o.pet_element[DM_EL_POIS]);
        NEAR(o.pet_element[DM_EL_POIS],expect);NEAR(o.pet_dps,expect);NEAR(o.pet_poison_uptime,1);assert(!o.dot);
    }
}

static void real_tests(const char *path) {
    size_t n;char *text=sim_read(path,&n);assert(text);dm_sim_data real={0};assert(!dm_sim_load(&real,text,n,"rotw"));free(text);
    NEAR(real.potion[DM_POT_HP1].life,30);NEAR(real.potion[DM_POT_HP1].len,192);NEAR(real.potion[DM_POT_MP1+1].mana,40);
    NEAR(real.potion[DM_POT_RVS].life,35);NEAR(real.potion[DM_POT_RVL].mana,100);NEAR(real.cls[7].hppot,150);NEAR(real.cls[7].mppot,200);
    dm_sim_char w;char err[80];
    assert(!dm_sim_profile_parse(&real,"class=7\nlevel=14\nlife=200\nmana=55\nstr=46\ndex=20\nvit=59\nene=20\nar=240\ndef=115\n"
        "weapon=12,22,100,0,0,1,0,0,1HS\nskill=394,8,8\nbar=394\nskill=395,5,4\nbar=395\nliferegen=2\n"
        "belt=hp1:1,hp2:3,mp1:1,mp2:1,rvs:2\n",&w,err,sizeof err));
    w.lvl[0]=1;int act2=-1;for(int i=0;i<real.nboss;i++)if(real.boss[i].mon==211)act2=i;assert(act2>=0);
    dm_sim_target boss;assert(!dm_sim_target_mon(&real,real.boss[act2].mon,0,0,DM_TK_ACTBOSS,1,&boss));
    dm_sim_rot_out r;dm_sim_surv_out s;
    assert(!dm_sim_fight(&real,&w,&boss,&r,&s,&tr));printf("act 2 boss without potions: kill %.2f s, death %.2f s\n",r.ttk,s.ttd);
    NEAR(round(s.ttd*10),100);assert(tr.end_kind==DM_END_DEATH && tr.npotion==0);
    int start=-1;for(int i=0;i<tr.n && start<0;i++)if(tr.b[i].shortage)start=i*tr.width;
    assert(start==60);
    w.potions=1;assert(!dm_sim_fight(&real,&w,&boss,&r,&s,&tr));printf("act 2 boss with the belt: kill %.2f s, death %.2f s, %d drinks\n",r.ttk,s.ttd,tr.npotion);
    NEAR(round(s.ttd*10),300);assert(tr.npotion==8 && tr.end_kind==DM_END_DEATH);trace_shape(&tr);drinks_on_lines(&tr);
    real_minions(&real);
    dm_sim_free(&real);
}
int main(void) {
    joint_tests();exact_tests();early_end_tests();belt_tests();profile_tests();minion_tests();
    const char *path=getenv("DM_SIM_DATA");if(path && *path)real_tests(path);
    puts(path && *path?"sim fight: joint run, trace, belt and exported potion tables passed":"sim fight: joint run, trace, exact kill time, early ends and belt passed");return 0;
}
