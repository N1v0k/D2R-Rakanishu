#include "../dm_sim_internal.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#define NEAR(a,b) assert(fabs((double)(a)-(double)(b))<1e-6)
static dm_sim_data d;
static dm_sim_skill skills[24];
static dm_sim_char c;
static dm_sim_target t;
static dm_sim_tmod m;
static int64_t variable(const dm_sim_calc_env *e,int skill,int var,int *bad){(void)e;(void)bad;return var==DM_V_LVL?3:var==DM_V_BLVL?skill:0;}
static int64_t statval(const dm_sim_calc_env *e,int id){(void)e;return id*2;}
static void reset(void) {
    dm_sim_free(&d);memset(skills,0,sizeof skills);d.ok=1;d.ncls=8;dm_sim_char_init(&c,1);dm_sim_tmod_init(&m);
    c.clvl=50;c.life=1000;c.mana=1000;c.def=5;c.hand[0].min=c.hand[0].max=100;c.hand[0].strbonus=0;c.hand[0].kind=DM_WK_MELEE;
    for(int i=0;i<DM_ST_N;i++)d.stat[i]=-1;
    for(int i=0;i<3;i++){d.diff[i].lsd=d.diff[i].msd=1;d.diff[i].pvspe=100;d.diff[i].mercboss=50;}
    d.diff[1].rp=-40;d.diff[2].rp=-100;
    for(int i=0;i<24;i++) {
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
    memset(&t,0,sizeof t);t.hp=1000;t.mlvl=50;t.cycle=25;t.dmin=t.dmax=100;t.drain=100;
}
static void calc_tests(void) {
    reset();dm_sim_calc_env e={variable,statval,0,0,NULL};int bad=0;
    const struct {const char *p;int result;} cases[]={
        {"10 40 lvl DM",20},{"10 40 20 DM",35},{"10 40 1000 DM",40},{"10 5 3 LN",20},
        {"2 3 4 * +",14},{"-7 2 /",-3},{"-7 2 %",-1},{"5 0 /",0},{"1 3 8 ?: ",3},
        {"0 3 8 ?: ",8},{"3 8 min",3},{"3 8 max",8},{"@4:blvl #7 +",18},{"3 neg",-3},
        {"3 4 < 2 2 == &&",1},{"0 !",1},{"2 6 avg",4}
    };
    for(size_t i=0;i<sizeof cases/sizeof *cases;i++){int p=dm_sim_calc_compile(&d,cases[i].p);assert(p>=0);NEAR(dm_sim_calc_eval(&d,p,&e,&bad),cases[i].result);assert(!bad);}
    const char *invalid[]={"+","1 2","wat","1 ?: ","@512:lvl","1 2 ???","2147483648","#x",""};
    for(size_t i=0;i<sizeof invalid/sizeof *invalid;i++)assert(dm_sim_calc_compile(&d,invalid[i])==-2);
    int p=dm_sim_calc_compile(&d,"2147483647 2147483647 *");dm_sim_calc_eval(&d,p,&e,&bad);assert(bad);
    bad=0;dm_sim_calc_eval(&d,d.ncode,&e,&bad);assert(bad);
}
static void formula_tests(void) {
    int32_t v[5]={1,2,3,4,5};int levels[]={1,8,9,16,17,22,23,28,29},values[]={10,17,19,33,36,51,55,75,80};
    for(int i=0;i<9;i++)assert(dm_sim_level_value(10,v,levels[i])==values[i]);
    int32_t len[3]={1,2,3};assert(dm_sim_len_value(10,len,20)==45);
    assert(dm_sim_cth(0,1000,1,99)==5);assert(dm_sim_cth(100000,0,99,1)==95);assert(dm_sim_cth(100,100,50,50)==50);
    assert(dm_sim_cth(152,20,17,60)==38);assert(dm_sim_cth(100,101,60,37)==60);
    assert(dm_sim_cth(0,0,50,50)==95);assert(dm_sim_cth(0,-1,50,50)==95);
    assert(dm_sim_cth(-10,-20,50,50)==95);assert(dm_sim_cth(-10,0,50,50)==5);
    assert(dm_sim_cth(1,INT32_MIN,50,50)==5);assert(dm_sim_cth(INT32_MIN,0,50,50)==95);
    assert(dm_sim_cth(INT32_MIN,INT32_MIN,50,50)==95);assert(dm_sim_cth(INT32_MAX,1,50,50)==5);
    assert(dm_sim_cth(50000000,50000000,50,50)==7);assert(dm_sim_cth(100,100,0,0)==5);
    assert(dm_sim_eff_res(110,60,50,0,0,0)==48);assert(dm_sim_eff_res(120,50,99,0,0,0)==110);
    assert(dm_sim_eff_res(120,50,10,1,0,0)==75);assert(dm_sim_eff_res(120,50,10,1,1,0)==35);
    assert(dm_sim_eff_res(120,50,10,1,1,1)==75);assert(dm_sim_eff_res(0,100,100,0,0,0)==-100);
    int sorc[]={0,9,20,37,63,105,200};for(int i=0;i<7;i++)assert(dm_sim_frames_cast(14,256,sorc[i],0)==13-i);
    int pal[]={0,9,18,30,48,75,125};for(int i=0;i<7;i++)assert(dm_sim_frames_cast(16,256,pal[i],0)==15-i);
    assert(dm_sim_frames_cast(19,256,0,1)==19);
    assert(dm_sim_frames_attack(16,256,0,0,0,0)==15);assert(dm_sim_frames_attack(16,256,0,0,0,1)==16);
    assert(dm_sim_frames_attack(16,256,0,-1000,0,0)==107);
    reset();skills[1].mana=10;skills[1].manashift=7;skills[1].lvlmana=1;assert(dm_sim_mana_cost(&skills[1],1)==1280);
    skills[1].minmana=6;assert(dm_sim_mana_cost(&skills[1],1)==1536);
    skills[1].emin=12;skills[1].emax=28;skills[1].hitshift=7;int bad=0;
    NEAR(dm_sim_skill_value(&d,&c,1,1,DM_SV_EMIN,&bad),6);NEAR(dm_sim_skill_value(&d,&c,1,1,DM_SV_EMAX,&bad),14);
    skills[1].calc[DM_CF_EDMGSYM]=dm_sim_calc_compile(&d,"@2:blvl 10 *");c.blvl[2]=2;c.lvl[2]=50;c.mastery[DM_EL_FIRE]=50;
    NEAR(dm_sim_skill_value(&d,&c,1,1,DM_SV_EMIN,&bad),10);
}
static void family_tests(void) {
    reset();dm_sim_skill_out o;
    const uint32_t families[]={DM_FAM_W1,DM_FAM_W2,DM_FAM_W3,DM_FAM_W4,DM_FAM_W5,DM_FAM_Z1,DM_FAM_Z2,DM_FAM_Z3,DM_FAM_DOT,
        DM_FAM_L,DM_FAM_P,DM_FAM_F,DM_FAM_AOFF,DM_FAM_ADMG,DM_FAM_ADEF,DM_FAM_K,DM_FAM_B,DM_FAM_PA,DM_FAM_U};
    for(int i=0;i<19;i++) {
        dm_sim_skill *s=&skills[1];s->fam=families[i];s->emin=s->emax=100;s->etype=DM_EL_FIRE;s->srcdam=128;s->srvdofunc=0;
        s->kick=i==4;
        if(i<5){s->emin=s->emax=0;}
        if(i==8){s->etype=DM_EL_POIS;s->elen=50;}
        if(i==9){s->emin=s->emax=0;s->srvdofunc=20;s->calc[0]=dm_sim_calc_compile(&d,"25");}
        else s->calc[0]=-1;
        sim_action(&d,&c,&t,&m,1,&o);
        assert(!(o.flags&DM_SF_UNAVAIL));
        if(i==0 || i==1 || i==2)NEAR(o.hit[0],100);
        if(i==3)NEAR(o.hit[0],400);
        if(i==4)NEAR(o.hit[0],6.82);
        if(i==5 || i==6 || i==7 || i==13)NEAR(o.hit[DM_EL_FIRE],100);
        if(i==8){NEAR(o.dot,5000);NEAR(o.hit[DM_EL_POIS],0);}
        if(i==9)NEAR(o.sf_frac,.25);
        if(i==10 || i==11)NEAR(o.pet_dps,100*25.0/30);
        if(i==12 || i==14 || i==15 || i==16 || i==17)assert(o.flags&DM_SF_MODIFIER);
        if(i==18)assert(o.flags&DM_SF_UTILITY);
    }
    reset();c.ds=50;c.cs=50;c.mastery_cs=50;sim_action(&d,&c,&t,&m,0,&o);NEAR(o.hit[0],(100+255.0/512)*1.875);
    skills[0].srcdam=96;sim_action(&d,&c,&t,&m,0,&o);NEAR(o.hit[0],140.625);
    skills[0].etype=DM_EL_MAG;skills[0].role[DM_ROLE_ELPCT]=3;skills[0].calc[3]=dm_sim_calc_compile(&d,"100");
    sim_action(&d,&c,&t,&m,0,&o);NEAR(o.hit[0],0);NEAR(o.hit[DM_EL_MAG],140.625);
    reset();c.cb=100;sim_action(&d,&c,&t,&m,0,&o);NEAR(o.cb_frac,.25);
    t.kind=DM_TK_ACTBOSS;sim_action(&d,&c,&t,&m,0,&o);NEAR(o.cb_frac,.125);
    skills[0].fam=DM_FAM_W3;sim_action(&d,&c,&t,&m,0,&o);NEAR(o.cb_frac,.0625);
}
static void modifier_tests(void) {
    const uint32_t families[]={DM_FAM_AOFF,DM_FAM_ADEF,DM_FAM_K,DM_FAM_B,DM_FAM_PA};
    for(int i=0;i<5;i++) {
        reset();skills[1].fam=families[i];skills[1].astat[0]=0;
        strcpy(d.stat_name[0],i==1?"armorclass":i==2?"damageresist":i==4?"passive_critical_strike":"damagepercent");
        skills[1].calc[DM_CF_AURASTAT1]=dm_sim_calc_compile(&d,i==1?"100":i==2?"-50":"50");
        c.def=100;t.ar=100;dm_sim_skill_out o;assert(!dm_sim_skill_eval(&d,&c,&t,1,&o));
        if(i==1){NEAR(o.dps,0);NEAR(o.survival_delta,11);}
        else {
            double before=100+255.0/512,after=i==4?before*1.5:i==2?150+191.0/256:150+383.0/512;
            NEAR(o.dps,(after-before)*25/15);
        }
    }
    reset();skills[1].fam=DM_FAM_W1;skills[1].srvdofunc=150;skills[1].emin=skills[1].emax=0;
    c.shield=1;c.shield_min=20;c.shield_max=40;c.ds=100;t.block=95;
    dm_sim_skill_out o;sim_action(&d,&c,&t,&m,1,&o);NEAR(o.hit[0],36);NEAR(o.cth,1);
}
static void timeline_tests(void) {
    reset();dm_sim_skill_out a;dm_sim_rot_out r={0};
    sim_action(&d,&c,&t,&m,1,&a);a.frames=a.cooldown=25;a.action_frame=1;a.travel=0;
    a.roll=(dm_sim_roll){1,25600,25600,0,DM_EL_FIRE,1,0};
    sim_timeline(&d,&c,&t,&m,&a,1,&r,NULL);NEAR(r.dps,100);NEAR(r.dps_element[DM_EL_FIRE],100);NEAR(r.ttk,9.04);
    a.cb_frac=.25;sim_timeline(&d,&c,&t,&m,&a,1,&r,NULL);NEAR(r.ttk,-2);
    a.cb_frac=0;a.mana_x256=500*256;sim_timeline(&d,&c,&t,&m,&a,1,&r,NULL);assert(r.mana_short>.8);NEAR(r.dps,100);NEAR(r.mana_seconds,2);
    d.cls[c.cls].manaregen=10;sim_timeline(&d,&c,&t,&m,&a,1,&r,NULL);NEAR(r.dps,100);NEAR(r.mana_seconds,2.5);
    reset();sim_action(&d,&c,&t,&m,1,&a);memset(a.hit,0,sizeof a.hit);a.dot=100;a.dot_frames=50;a.frames=a.cooldown=25;a.action_frame=1;
    sim_timeline(&d,&c,&t,&m,&a,1,&r,NULL);NEAR(r.dps,50);
    a.dot=0;a.sf_frac=.25;a.sf_floor=.5;sim_timeline(&d,&c,&t,&m,&a,1,&r,NULL);NEAR(r.dps,0);NEAR(r.ttk,-2);
    reset();c.bar[0]=1;c.bar[1]=2;c.nbar=2;skills[1].localdelay=50;skills[1].emin=skills[1].emax=300;
    dm_sim_skill_out solo;dm_sim_skill_eval(&d,&c,&t,1,&solo);dm_sim_rotation(&d,&c,&t,&r);assert(r.dps>solo.dps);assert(r.nseq>=2);
    reset();sim_action(&d,&c,&t,&m,1,&a);a.frames=a.cooldown=17;a.mana_x256=500*256;
    sim_timeline(&d,&c,&t,&m,&a,1,&r,NULL);NEAR(r.dps,2500.0/17);double rich=r.dps;
    c.mana=0;sim_timeline(&d,&c,&t,&m,&a,1,&r,NULL);NEAR(r.dps,rich);NEAR(r.mana_seconds,0);NEAR(r.ttk,-2);
    reset();c.bar[0]=1;c.bar[1]=2;c.nbar=2;skills[1].localdelay=50;skills[1].emin=skills[1].emax=300;
    skills[1].mana=skills[2].mana=100;dm_sim_rotation(&d,&c,&t,&r);rich=r.dps;
    c.mana=0;dm_sim_rotation(&d,&c,&t,&r);NEAR(r.dps,rich);
    reset();skills[1].fam=DM_FAM_ADMG;skills[1].aura=1;skills[1].perdelay=50;c.bar[0]=1;c.nbar=1;
    dm_sim_rotation(&d,&c,&t,&r);NEAR(r.dps,(100+255.0/512)*25/15+50);
    reset();t.regen=4096;sim_action(&d,&c,&t,&m,1,&a);sim_timeline(&d,&c,&t,&m,&a,1,&r,NULL);assert(r.ttk<0);
    c.prevent_heal=1;sim_action(&d,&c,&t,&m,0,&a);sim_timeline(&d,&c,&t,&m,&a,1,&r,NULL);NEAR(r.ttk,-2);
}
static void survival_tests(void) {
    reset();dm_sim_surv_out s;dm_sim_rot_out r;dm_sim_survive(&d,&c,&t,NULL,&s);
    NEAR(s.dmg_hit,100);NEAR(s.max_hit,.1);NEAR(s.p_land,.05);NEAR(s.ttd,200);
    c.dr_flat=20;c.res[0]=50;dm_sim_survive(&d,&c,&t,NULL,&s);NEAR(s.dmg_hit,40);NEAR(s.ttd,500);
    t.dmin=t.dmax=0;t.eltype[0]=DM_EL_FIRE;t.elpct[0]=100;t.elmin[0]=t.elmax[0]=100;c.mdr_flat=20;c.res[DM_EL_FIRE]=75;
    dm_sim_survive(&d,&c,&t,NULL,&s);NEAR(s.dmg_hit,20);
    c.diff=t.diff=1;dm_sim_survive(&d,&c,&t,NULL,&s);NEAR(s.dmg_hit,52);
    c.diff=t.diff=2;dm_sim_survive(&d,&c,&t,NULL,&s);NEAR(s.dmg_hit,100);
    c.absorb_pct[DM_EL_FIRE]=20;c.absorb_flat[DM_EL_FIRE]=10;dm_sim_survive(&d,&c,&t,NULL,&s);NEAR(s.dmg_hit,70);
    reset();strcpy(skills[2].aurastate,"cyclonearmor");skills[2].fam=DM_FAM_B;
    strcpy(d.stat_name[0],"bonearmor");skills[2].astat[0]=0;skills[2].calc[DM_CF_AURASTAT1]=dm_sim_calc_compile(&d,"25600");
    int bad=0;sim_modify(&d,&c,&m,2,1,&bad);assert(!bad && c.cyclone_pool==100 && !c.bone_pool);
    dm_sim_survive(&d,&c,&t,NULL,&s);NEAR(s.max_hit,.1);
    t.dmin=t.dmax=0;t.eltype[0]=DM_EL_FIRE;t.elpct[0]=100;t.elmin[0]=t.elmax[0]=100;
    dm_sim_survive(&d,&c,&t,NULL,&s);NEAR(s.max_hit,0);
    reset();c.bone_pool=500;dm_sim_survive(&d,&c,&t,NULL,&s);assert(s.ttd>200);
    c.es_pct=50;c.mana=1000;dm_sim_survive(&d,&c,&t,NULL,&s);assert(s.ttd>200);
    reset();c.ll=50;c.bar[0]=0;c.nbar=1;dm_sim_rotation(&d,&c,&t,&r);dm_sim_survive(&d,&c,&t,&r,&s);assert(s.ttd<0);assert(s.sustain>0);
    t.drain=0;dm_sim_survive(&d,&c,&t,&r,&s);NEAR(s.sustain,0);
    reset();t.crit=5;c.dr_flat=150;dm_sim_survive(&d,&c,&t,NULL,&s);NEAR(s.dmg_hit,2.5);
    reset();t.dmin=t.dmax=0;t.eltype[0]=DM_EL_FIRE;t.elpct[0]=10;t.elmin[0]=t.elmax[0]=100;c.mdr_flat=20;
    dm_sim_survive(&d,&c,&t,NULL,&s);NEAR(s.dmg_hit,8);
    dm_sim_light_out l;s.underestimated=0;s.ttd=9;s.max_hit=.49;t.kind=DM_TK_NORMAL;dm_sim_light(&t,3,&s,0,&l);assert(l.light==DM_LIGHT_GREEN);
    s.max_hit=.5;dm_sim_light(&t,3,&s,0,&l);assert(l.light==DM_LIGHT_ORANGE);
    s.max_hit=1;dm_sim_light(&t,3,&s,0,&l);assert(l.light==DM_LIGHT_RED);
    s.max_hit=.1;s.ttd=2;dm_sim_light(&t,3,&s,0,&l);assert(l.light==DM_LIGHT_RED);
}
static void interaction_tests(void) {
    reset();dm_sim_skill_out a;dm_sim_rot_out r={0};
    sim_action(&d,&c,&t,&m,1,&a);a.frames=a.cooldown=25;a.action_frame=1;a.travel=100;
    a.roll=(dm_sim_roll){1,25600,25600,0,DM_EL_FIRE,1,0};
    sim_timeline(&d,&c,&t,&m,&a,1,&r,NULL);NEAR(r.dps,100);NEAR(r.ttk,13.04);
    reset();skills[1].fam=DM_FAM_F;sim_action(&d,&c,&t,&m,1,&a);a.pet_dps=100;a.pet_element[DM_EL_FIRE]=100;a.dur=50;a.mana_x256=2000*256;
    sim_timeline(&d,&c,&t,&m,&a,1,&r,NULL);NEAR(r.dps,100*50.0/(50+a.frames));NEAR(r.mana_seconds,0);
    assert(dm_sim_frames_rollback(16,8,256,50,5)==31);assert(dm_sim_frames_rollback(16,8,256,100,5)==47);
    reset();strcpy(d.stat_name[0],"damagepercent");strcpy(d.stat_name[1],"attackrate");
    skills[1].fam=skills[2].fam=DM_FAM_AOFF;skills[1].aura=skills[2].aura=1;
    skills[1].astat[0]=skills[2].astat[0]=0;skills[2].astat[1]=1;
    skills[1].calc[DM_CF_AURASTAT1]=dm_sim_calc_compile(&d,"100");
    skills[2].calc[DM_CF_AURASTAT1]=dm_sim_calc_compile(&d,"50");
    skills[2].calc[DM_CF_AURASTAT1+1]=dm_sim_calc_compile(&d,"100");
    c.active_aura=1;c.ed=100;c.bar[0]=2;c.nbar=1;dm_sim_char prepared;dm_sim_tmod mod;
    sim_prepare(&d,&c,&t,&prepared,&mod,&r);assert(r.aura==2);assert(prepared.ed==50);assert(prepared.sias==100);
    skills[1].calc[DM_CF_AURASTAT1]=dm_sim_calc_compile(&d,"100 lvl *");c.lvl[1]=2;c.active_lvl[1]=1;c.nbar=0;
    sim_prepare(&d,&c,&t,&prepared,&mod,&r);assert(prepared.ed==200);
    reset();strcpy(d.stat_name[0],"fireresist");
    d.state=calloc(2,sizeof *d.state);assert(d.state);d.nstate=2;strcpy(d.state[0].name,"full");d.state[0].sunderfull=1;strcpy(d.state[1].name,"reduced");
    skills[1].fam=skills[2].fam=DM_FAM_K;skills[1].astat[0]=skills[2].astat[0]=0;
    strcpy(skills[1].aurastate,"full");strcpy(skills[2].aurastate,"reduced");
    skills[1].calc[DM_CF_AURASTAT1]=dm_sim_calc_compile(&d,"-50");skills[2].calc[DM_CF_AURASTAT1]=dm_sim_calc_compile(&d,"-50");
    int bad=0;sim_modify(&d,&c,&m,1,1,&bad);sim_modify(&d,&c,&m,2,1,&bad);
    c.sunder[DM_EL_FIRE]=1;t.res[DM_EL_FIRE]=110;sim_action(&d,&c,&t,&m,3,&a);NEAR(a.hit[DM_EL_FIRE],65);
    reset();skills[1].emin=skills[1].emax=1000;c.es_pct=75;c.mana=1000;c.bar[0]=1;c.nbar=1;t.ar=100000;c.def=0;
    dm_sim_surv_out free_cast,costly;dm_sim_survive(&d,&c,&t,NULL,&free_cast);
    skills[1].mana=300;dm_sim_survive(&d,&c,&t,NULL,&costly);assert(costly.ttd<free_cast.ttd);
    reset();skills[1].fam=DM_FAM_K;skills[1].calc[DM_CF_AURALEN]=dm_sim_calc_compile(&d,"25");skills[1].mana=1;
    c.bar[0]=1;c.nbar=1;sim_action(&d,&c,&t,&m,1,&a);sim_timeline(&d,&c,&t,&m,&a,1,&r,NULL);assert(r.mana_spent>10);
}
static void malformed_tests(void) {
    reset();dm_sim_data tmp;assert(dm_sim_load(&tmp,"bad",3,"rotw")<0);assert(!tmp.ok);
    uint32_t f;assert(dm_sim_fam_parse("W1+",&f)<0);assert(dm_sim_fam_parse("W3+DoT",&f)==0);assert(f==(DM_FAM_W3|DM_FAM_DOT));
    char err[80];assert(dm_sim_profile_parse(&d,"level=80\nplayers=2\n",&c,err,sizeof err)<0);
    assert(dm_sim_profile_parse(&d,"level=bad\n",&c,err,sizeof err)<0);assert(dm_sim_profile_parse(&d,"nonsense=1\n",&c,err,sizeof err)<0);
    assert(dm_sim_profile_parse(&d,"class=1\nlevel=80\n",&c,err,sizeof err)==0);assert(c.clvl==80);
}
static void queue_limit_test(void) {
    reset();dm_sim_skill_out a;sim_action(&d,&c,&t,&m,1,&a);
    a.frames=a.cooldown=1;a.action_frame=1;a.travel=10000;a.mana_x256=0;a.dot=1;a.dot_frames=2;
    dm_sim_rot_out r={0};sim_timeline(&d,&c,&t,&m,&a,1,&r,NULL);
    assert(r.flags&DM_SF_UNAVAIL);NEAR(r.ttk,-2);
    c.cyclone_pool=1;dm_sim_surv_out surv;sim_timeline(&d,&c,&t,&m,&a,1,&r,&surv);
    assert(surv.underestimated);NEAR(surv.ttd,-2);
}
static void description_tests(void) {
    reset();skills[1].descdam=5;skills[1].srcdam=96;skills[1].emin=skills[1].emax=0;
    c.hand[0].elmin[DM_EL_FIRE]=10;c.hand[0].elmax[DM_EL_FIRE]=20;c.mastery[DM_EL_FIRE]=50;
    int out[2];assert(!dm_sim_skill_endpoints(&d,&c,1,out));assert(out[0]==86 && out[1]==97);
    c.damage_mastery_delta[1]=25;
    assert(!dm_sim_skill_endpoints(&d,&c,1,out));assert(out[0]==105 && out[1]==116);
    c.damage_mastery_delta[1]=0;skills[1].descdam=7;
    skills[1].calc[DM_CF_DDAM1]=dm_sim_calc_compile(&d,"25");skills[1].calc[DM_CF_DDAM2]=dm_sim_calc_compile(&d,"7");
    assert(!dm_sim_skill_endpoints(&d,&c,1,out));assert(out[0]==110 && out[1]==121);
    skills[1].descdam=18;
    assert(!dm_sim_skill_endpoints(&d,&c,1,out));assert(out[0]==110 && out[1]==121);
    skills[1].descdam=24;
    assert(!dm_sim_skill_endpoints(&d,&c,1,out));assert(out[0]==86 && out[1]==97);
    reset();skills[1].descdam=8;skills[1].srcdam=0;
    skills[1].calc[DM_CF_DDAM1]=dm_sim_calc_compile(&d,"2");skills[1].calc[DM_CF_DDAM2]=dm_sim_calc_compile(&d,"5");
    assert(!dm_sim_skill_endpoints(&d,&c,1,out));assert(out[0]==480 && out[1]==481);
    skills[1].descdam=9;skills[1].mindam=skills[1].maxdam=1;
    skills[1].minlev[0]=skills[1].maxlev[0]=4;c.lvl[1]=2;
    assert(!dm_sim_skill_endpoints(&d,&c,1,out));assert(out[0]==7575 && out[1]==7576);
    reset();skills[1].descdam=25;skills[1].srcdam=0;
    skills[1].calc[DM_CF_DDAM1]=dm_sim_calc_compile(&d,"17");skills[1].calc[DM_CF_DDAM2]=dm_sim_calc_compile(&d,"50");
    assert(!dm_sim_skill_endpoints(&d,&c,1,out));assert(out[0]==116 && out[1]==150);
    reset();skills[1].descdam=5;skills[1].emin=skills[1].emax=0;c.hand[0].min=c.hand[0].max=0;
    c.stats_known=1;c.stat_value[57]=c.stat_value[58]=31;c.poison_frames=49;c.mastery[DM_EL_POIS]=50;
    assert(!dm_sim_skill_endpoints(&d,&c,1,out));assert(out[0]==9 && out[1]==10);
    c.stats_known=0;c.ed=-50;c.hand[0].min=3;c.hand[0].max=5;
    assert(!dm_sim_skill_endpoints(&d,&c,1,out));assert(out[0]==1 && out[1]==2);
    c.endpoint_bad=1;assert(dm_sim_skill_endpoints(&d,&c,1,out)<0);c.endpoint_bad=0;
    c.endpoint_state_bad[1]=1;assert(dm_sim_skill_endpoints(&d,&c,1,out)<0);
    skills[1].srcdam=0;assert(!dm_sim_skill_endpoints(&d,&c,1,out));
    c.endpoint_state_bad[1]=0;skills[1].srcdam=128;c.stats_known=1;
    c.stat_value[57]=c.stat_value[58]=32767;c.poison_frames=32767;c.mastery[DM_EL_POIS]=300;
    assert(dm_sim_skill_endpoints(&d,&c,1,out)<0);
    skills[1].descdam=99;assert(dm_sim_skill_endpoints(&d,&c,1,out)<0);
}
int main(void) {
    description_tests();
    calc_tests();formula_tests();family_tests();modifier_tests();timeline_tests();survival_tests();interaction_tests();queue_limit_test();malformed_tests();dm_sim_free(&d);
    puts("sim: calc, formulas, 19 families, timeline, survival and malformed inputs passed");return 0;
}
