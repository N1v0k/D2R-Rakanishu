#define main sim_baseline_main
#include "t_sim.c"
#undef main

static void placement_cost(void) {
    reset();
    skills[1].fam=DM_FAM_F;c.mana=0;t.hp=50;
    dm_sim_skill_out action;
    sim_action(&d,&c,&t,&m,1,&action);
    action.pet_dps=100;action.pet_element[DM_EL_FIRE]=100;
    action.dur=50;action.mana_x256=2000*256;
    dm_sim_rot_out rotation={0};
    sim_timeline(&d,&c,&t,&m,&action,1,&rotation,NULL);
    assert(rotation.ttk==-2);
    NEAR(rotation.mana_spent,0);
    c.mana=2000;
    sim_timeline(&d,&c,&t,&m,&action,1,&rotation,NULL);
    NEAR(rotation.ttk,-2);
    NEAR(rotation.mana_spent,2000);
    c.mana=0;c.active_lvl[1]=c.lvl[1];
    sim_timeline(&d,&c,&t,&m,&action,1,&rotation,NULL);
    NEAR(rotation.ttk,-2);
    NEAR(rotation.mana_spent,0);
}
static void flat_reduction(void) {
    reset();t.dmin=0;t.dmax=100;c.dr_flat=75;
    dm_sim_surv_out result;
    assert(!dm_sim_survive(&d,&c,&t,NULL,&result));
    NEAR(result.dmg_hit,325.0/101);
    reset();t.dmin=t.dmax=0;t.eltype[0]=DM_EL_FIRE;t.elpct[0]=100;
    t.elmin[0]=0;t.elmax[0]=100;c.mdr_flat=75;
    assert(!dm_sim_survive(&d,&c,&t,NULL,&result));
    NEAR(result.dmg_hit,325.0/101);
    t.eltype[1]=DM_EL_FIRE;t.elpct[1]=100;t.elmin[1]=0;t.elmax[1]=10;
    c.res[DM_EL_FIRE]=50;c.absorb_pct[DM_EL_FIRE]=20;c.absorb_flat[DM_EL_FIRE]=3;
    double expected=0;
    for(int a=0;a<=100;a++)for(int b=0;b<=10;b++)expected+=fmax(0,fmax(0,a+b-75)*.5*.8-3)/(101*11);
    assert(!dm_sim_survive(&d,&c,&t,NULL,&result));
    NEAR(result.dmg_hit,expected);
    reset();t.dmin=0;t.dmax=1;c.dr_flat=1;
    for(int k=0;k<3;k++){t.eltype[k]=DM_EL_PHYS;t.elpct[k]=100;t.elmin[k]=0;t.elmax[k]=k==2?10000000:1;}
    assert(!dm_sim_survive(&d,&c,&t,NULL,&result));
    NEAR(result.dmg_hit,5000000.5+1.0/(8*10000001));
}
static void fractional_resources(void) {
    reset();c.mana=52.5;t.hp=50;
    skills[1].mana=105;skills[1].manashift=7;
    dm_sim_skill_out action;
    assert(!dm_sim_skill_eval(&d,&c,&t,1,&action));
    assert(action.ttk==-2);
    char error[128];
    assert(!dm_sim_profile_parse(&d,"life=159.5\nmana=52.5\n",&c,error,sizeof error));
    NEAR(c.life,159.5);NEAR(c.mana,52.5);
    assert(dm_sim_profile_parse(&d,"mana=nan\n",&c,error,sizeof error)<0);
    assert(dm_sim_profile_parse(&d,"life=inf\n",&c,error,sizeof error)<0);
    assert(dm_sim_profile_parse(&d,"mana=0.1\n",&c,error,sizeof error)<0);
}
static void fixed_element_minimum(void) {
    reset();skills[1].emin=skills[1].emax=1;
    skills[1].calc[DM_CF_EDMGSYM]=dm_sim_calc_compile(&d,"42");
    int bad=0;
    assert(dm_sim_skill_raw(&d,&c,1,1,DM_SV_EMIN,&bad)==256);
    assert(dm_sim_skill_raw(&d,&c,1,1,DM_SV_EMAX,&bad)==363);
    c.mastery[DM_EL_FIRE]=50;
    assert(dm_sim_skill_raw(&d,&c,1,1,DM_SV_EMIN,&bad)==384);
    skills[1].eminlev[0]=1;
    assert(dm_sim_skill_raw(&d,&c,1,1,DM_SV_EMIN,&bad)==544);
    skills[1].eminlev[0]=0;skills[1].emin=2;
    assert(dm_sim_skill_raw(&d,&c,1,1,DM_SV_EMIN,&bad)==1090);
    assert(!bad);
}
static void physical_synergy_rounding(void) {
    reset();skills[1].mindam=25;skills[1].maxdam=35;
    skills[1].calc[DM_CF_DMGSYM]=dm_sim_calc_compile(&d,"27");
    int bad=0;
    assert(dm_sim_skill_raw(&d,&c,1,1,DM_SV_PMIN,&bad)==7936);
    assert(dm_sim_skill_raw(&d,&c,1,1,DM_SV_PMAX,&bad)==11264);
    skills[1].hitshift=4;
    assert(dm_sim_skill_raw(&d,&c,1,1,DM_SV_PMIN,&bad)==496);
    assert(!bad);
}
static void kick_base_damage(void) {
    reset();skills[1].kick=1;c.str=100;c.dex=80;
    int bad=0;
    assert(dm_sim_skill_raw(&d,&c,1,1,DM_SV_PMIN,&bad)==10240);
    assert(dm_sim_skill_raw(&d,&c,1,1,DM_SV_PMAX,&bad)==13568);
    assert(!bad);
}
static void additive_attack_rating(void) {
    reset();c.ar=533;c.ar_base=395;c.ar_base_known=1;
    c.ar_bonus[1]=35;skills[1].tohit=10;skills[1].levtohit=0;
    skills[1].calc[DM_CF_TOHIT]=-1;
    int bad=0;
    assert(sim_attack_rating(&d,&c,1,&bad)==572);
    skills[1].tohit=20;
    assert(sim_attack_rating(&d,&c,1,&bad)==612);
    c.ar_pct=-36;skills[1].tohit=0;
    assert(sim_attack_rating(&d,&c,1,&bad)==392);
    assert(!bad);
}
static void normal_physical_endpoints(void) {
    reset();c.str=30;c.dex=33;c.ed=0;
    c.hand[0].min=100;c.hand[0].max=200;
    c.hand[0].strbonus=c.hand[0].dexbonus=50;
    int range[2];
    assert(!sim_normal_physical(&c,range));
    assert(range[0]==131 && range[1]==262);
    c.ed=-200;c.hand[0].edmin=5;c.hand[0].postflat=3;
    assert(!sim_normal_physical(&c,range));
    assert(range[0]==18 && range[1]==23);
}
static void referenced_mastery(void) {
    reset();c.ar_pct=999;c.ed=888;c.mastery_cs=77;c.lvl[2]=3;
    strcpy(d.stat_name[400],"passive_mastery_melee_th");
    strcpy(d.stat_name[401],"passive_mastery_throw_dmg");
    strcpy(d.stat_name[402],"passive_mastery_melee_crit");
    for(int k=0;k<3;k++){
        skills[2].pstat[k]=400+k;
        skills[2].calc[DM_CF_PASSIVE1+k]=dm_sim_calc_compile(&d,k==0?"lvl 10 *":k==1?"lvl 5 *":"lvl 2 *");
    }
    int bad=0;
    skills[1].calc[0]=dm_sim_calc_compile(&d,"@2:math @2:madm + @2:macr +");
    assert(sim_calc(&d,&c,1,0,&bad)==51);
    c.lvl[2]=0;assert(sim_calc(&d,&c,1,0,&bad)==0);
    assert(!bad);
}
static void fractional_spell_roll(void) {
    reset();skills[1].srcdam=0;skills[1].hitshift=7;skills[1].emin=91;skills[1].emax=121;
    dm_sim_miss missile={0};missile.explosion=-1;
    for(int k=0;k<3;k++)missile.sub[k]=-1;
    for(int k=0;k<4;k++)missile.hitsub[k]=-1;
    skills[1].missile=0;d.miss[0]=&missile;t.res[DM_EL_FIRE]=33;
    dm_sim_skill_out out;assert(!sim_action(&d,&c,&t,&m,1,&out));
    assert(out.roll.known && out.roll.low==11648 && out.roll.high==15488);
    double before=0,after=0;
    for(int raw=11648;raw<15488;raw++){before+=raw/256.0/3840;after+=(raw*67/100)/256.0/3840;}
    assert(fabs(out.raw[DM_EL_FIRE]-before)<.00001 && fabs(out.hit[DM_EL_FIRE]-after)<.00001);
    d.miss[0]=NULL;
}
static void native_death_threshold(void) {
    dm_sim_roll roll={.known=1,.low=257,.high=257,.chance=1};
    NEAR(sim_kill_attempts(&roll,512),1);
    roll.low=roll.high=256;NEAR(sim_kill_attempts(&roll,512),2);
    roll.low=roll.high=255;NEAR(sim_kill_attempts(&roll,512),2);
    roll.low=roll.high=1;NEAR(sim_kill_attempts(&roll,1),1);
    roll.chance=.5;NEAR(sim_kill_attempts(&roll,1),2);
    roll.low=roll.high=0;NEAR(sim_kill_attempts(&roll,1),-1);
}
static void expected_attempts(void) {
    dm_sim_roll roll={.known=1,.low=100*256,.high=150*256,.chance=1};
    NEAR(sim_kill_attempts(&roll,125*256),1.4800786208361387);
    roll.chance=.5;
    NEAR(sim_kill_attempts(&roll,125*256),2.9601572416722775);
    NEAR(sim_kill_attempts(&roll,151*256),4);
    roll.resistance=50;
    assert(fabs(sim_kill_attempts(&roll,125*256)-4.922051342808853)<.00001);
    roll.low=roll.high=100*256;roll.resistance=0;roll.critical=.75;
    NEAR(sim_kill_attempts(&roll,150*256),2.5);
    roll.chance=0;NEAR(sim_kill_attempts(&roll,150*256),-1);
    roll.chance=1;roll.known=0;NEAR(sim_kill_attempts(&roll,150*256),-2);
    roll.known=1;roll.high=roll.low=0;NEAR(sim_kill_attempts(&roll,256),-1);
    roll.low=roll.high=256;roll.critical=0;
    NEAR(sim_kill_attempts(&roll,1000000000),3906250);
    roll.high=512;NEAR(sim_kill_attempts(&roll,1000000000),-2);
}
static void unknown_action_time(void) {
    reset();dm_sim_skill_out action;dm_sim_rot_out rotation={0};
    assert(!sim_action(&d,&c,&t,&m,0,&action));
    assert(action.action_frame==-1 && (action.flags&DM_SF_ESTIMATE));
    sim_timeline(&d,&c,&t,&m,&action,1,&rotation,NULL);
    NEAR(rotation.ttk,-2);
    d.anim=calloc(1,sizeof *d.anim);assert(d.anim);d.nanim=1;
    strcpy(d.anim[0].mode,"A1");strcpy(d.anim[0].wclass,"HTH");
    d.anim[0].frames=16;d.anim[0].speed=256;d.anim[0].action=-1;
    assert(!sim_action(&d,&c,&t,&m,0,&action));
    assert(action.action_frame==-1 && (action.flags&DM_SF_ESTIMATE));
    sim_timeline(&d,&c,&t,&m,&action,1,&rotation,NULL);
    NEAR(rotation.ttk,-2);
    d.anim[0].action=6;
    c.hand[0].start_known=1;
    assert(!sim_action(&d,&c,&t,&m,0,&action));
    assert(action.action_frame==6);
    sim_timeline(&d,&c,&t,&m,&action,1,&rotation,NULL);
    assert(rotation.ttk>0);
    c.hand[0].start_frame=2;
    assert(!sim_action(&d,&c,&t,&m,0,&action));
    assert(action.action_frame==4 && action.frames==13);
}
static void standard_physical_mean(void) {
    reset();d.mon=calloc(1,sizeof *d.mon);assert(d.mon);d.nmon=1;
    strcpy(d.mon[0].id,"test");d.mon[0].flags=DM_MF_MELEE;d.mon[0].skilldamage=-1;
    for(int i=0;i<8;i++)d.mon[0].skills[i]=-1;
    t.mon=0;t.flags=DM_MF_MELEE;t.dmin=2;t.dmax=5;c.dr_flat=1;c.res[DM_EL_PHYS]=33;
    dm_sim_surv_out result;assert(!dm_sim_survive(&d,&c,&t,NULL,&result));
    NEAR(result.dmg_hit,1.6717528897506782);
    t.crit=100;c.dr_flat=3;c.res[DM_EL_PHYS]=50;
    assert(!dm_sim_survive(&d,&c,&t,NULL,&result));
    NEAR(result.dmg_hit,1.9980468153953552);
    t.crit=0;t.dmin=t.dmax=3;c.dr_flat=c.res[DM_EL_PHYS]=0;
    assert(!dm_sim_survive(&d,&c,&t,NULL,&result));
    NEAR(result.dmg_hit,3.498046875);NEAR(result.max_hit*c.life,3.99609375);
}
static void monster_critical_elements(void) {
    for(int e=DM_EL_FIRE;e<DM_EL_N;e++) {
        reset();t.dmin=t.dmax=0;t.crit=5;
        t.eltype[0]=e;t.elpct[0]=100;t.elmin[0]=t.elmax[0]=100;
        dm_sim_surv_out result;assert(!dm_sim_survive(&d,&c,&t,NULL,&result));
        NEAR(result.dmg_hit,105);NEAR(result.max_hit*c.life,200);
    }
}
static void transformed_attack_rate(void) {
    reset();c.cls=5;c.form=1;c.sias=20;
    strcpy(d.cls[5].token,"DZ");strcpy(c.hand[0].wclass,"1HS");
    d.anim=calloc(2,sizeof *d.anim);d.nanim=2;
    strcpy(d.anim[0].token,"40");strcpy(d.anim[0].mode,"A1");strcpy(d.anim[0].wclass,"HTH");
    d.anim[0].frames=13;d.anim[0].speed=256;d.anim[0].action=7;
    strcpy(d.anim[1].token,"DZ");strcpy(d.anim[1].mode,"A1");strcpy(d.anim[1].wclass,"1HS");
    d.anim[1].frames=19;d.anim[1].speed=256;d.anim[1].action=9;
    dm_sim_skill_out o;
    assert(!sim_action(&d,&c,&t,&m,0,&o));
    assert(o.action_frame==9 && o.frames==15);
    c.ias=9;
    skills[0].role[DM_ROLE_HITS]=0;skills[0].calc[0]=dm_sim_calc_compile(&d,"5");
    skills[0].rollback_param=1;skills[0].param[0]=70;
    assert(!sim_action(&d,&c,&t,&m,0,&o));
    assert(o.action_frame==9 && o.frames==39 && o.hits==5);
}
static void fend_group_timing(void) {
    reset();c.cls=0;c.ias=6;c.hand[0].wsm=-10;c.hand[0].start_known=1;c.hand[0].start_frame=2;
    strcpy(d.cls[0].token,"AM");strcpy(c.hand[0].wclass,"2HT");
    d.anim=calloc(1,sizeof *d.anim);d.nanim=1;
    strcpy(d.anim[0].token,"AM");strcpy(d.anim[0].mode,"A1");strcpy(d.anim[0].wclass,"2HT");
    d.anim[0].frames=18;d.anim[0].speed=256;d.anim[0].action=11;
    skills[2]=skills[0];skills[2].id=2;
    skills[2].srvstfunc=9;skills[2].srvdofunc=13;
    skills[2].calc[0]=dm_sim_calc_compile(&d,"12");
    skills[2].rollback_param=1;skills[2].param[0]=30;t.extra_targets=2;
    dm_sim_skill_out o;
    assert(!sim_action(&d,&c,&t,&m,2,&o));
    assert(o.action_frame==8 && o.frames==21 && o.burst_attacks==3 && o.hits==1);
    NEAR(o.hit[0],100);
    t.extra_targets=3;
    assert(!sim_action(&d,&c,&t,&m,2,&o));
    assert(o.frames==25 && o.burst_attacks==4 && o.hits==1);
    t.extra_targets=0;
    assert(!sim_action(&d,&c,&t,&m,2,&o));
    assert(o.frames==13 && o.burst_attacks==1 && o.hits==1);
    t.extra_targets=19;
    assert(!sim_action(&d,&c,&t,&m,2,&o));
    assert(o.burst_attacks==12 && o.hits==1);
    skills[2].srvstfunc=37;skills[2].calc[0]=dm_sim_calc_compile(&d,"5");
    t.extra_targets=2;
    assert(!sim_action(&d,&c,&t,&m,2,&o));
    assert(o.burst_attacks==5 && o.hits==2);NEAR(o.hit[0],200);
    t.extra_targets=5;
    assert(!sim_action(&d,&c,&t,&m,2,&o));
    assert(o.burst_attacks==5 && o.hits==1);NEAR(o.hit[0],100);
    t.extra_targets=-1;
    assert(sim_action(&d,&c,&t,&m,2,&o)<0 && (o.flags&DM_SF_UNAVAIL));
}
static void meteor_ground_roll(void) {
    reset();skills[1].burnmiss=0;skills[1].burnfactor=100;
    skills[1].calc[DM_CF_BURNLEN]=dm_sim_calc_compile(&d,"30");
    d.misses=calloc(1,sizeof *d.misses);d.nmiss=1;d.miss[0]=d.misses;
    dm_sim_miss *miss=d.miss[0];miss->skill=-1;miss->srvdofunc=5;miss->apply_mastery=1;miss->emin=15;miss->emax=25;miss->hitshift=3;miss->etype=DM_EL_FIRE;
    for(int k=0;k<DM_MF_N;k++)miss->calc[k]=-1;
    miss->calc[DM_MF_EDMGSYM]=dm_sim_calc_compile(&d,"3");
    t.res[DM_EL_FIRE]=33;
    dm_sim_skill_out o;
    assert(!sim_action(&d,&c,&t,&m,1,&o));
    assert(o.burn_frames==29);
    NEAR(o.burn,12.046044912870457);
    c.mastery[DM_EL_FIRE]=30;
    assert(!sim_action(&d,&c,&t,&m,1,&o));
    NEAR(o.burn,15.692720829538302);
    miss->apply_mastery=0;
    assert(!sim_action(&d,&c,&t,&m,1,&o));
    NEAR(o.burn,12.046044912870457);
    miss->calc[DM_MF_EDMGSYM]=dm_sim_calc_compile(&d,"10");
    assert(!sim_action(&d,&c,&t,&m,1,&o));
    NEAR(o.burn,12.959117525651891);
    t.res[DM_EL_FIRE]=100;
    assert(!sim_action(&d,&c,&t,&m,1,&o));
    NEAR(o.burn,0);
}
int main(void) {
    fend_group_timing();
    meteor_ground_roll();
    transformed_attack_rate();
    native_death_threshold();
    monster_critical_elements();
    standard_physical_mean();
    unknown_action_time();
    fractional_spell_roll();
    expected_attempts();
    referenced_mastery();
    normal_physical_endpoints();
    additive_attack_rating();
    placement_cost();
    flat_reduction();
    fractional_resources();
    fixed_element_minimum();
    physical_synergy_rounding();
    kick_base_damage();
    dm_sim_free(&d);
    puts("sim validation: placement requires mana; captured active pets persist");
    return 0;
}
