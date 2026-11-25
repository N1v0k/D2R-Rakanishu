#include "../dm_sim_internal.h"
#include <assert.h>
#include <stdio.h>
#define NEAR(a,b) do { double av=(a),bv=(b); if(fabs(av-bv)>=1e-6)fprintf(stderr,"line %d: %.9f != %.9f\n",__LINE__,av,bv); assert(fabs(av-bv)<1e-6); } while(0)

static int skill(const dm_sim_data *d,int id) {
    assert(id>=0 && id<DM_SIM_MAXSKILL && d->skill[id]);return id;
}
static int monster(const dm_sim_data *d,int idx) {
    assert(idx>=0 && idx<d->nmon && d->mon[idx].id[0]);return idx;
}
static dm_sim_char character(const dm_sim_data *d,int skill_id) {
    int id=skill(d,skill_id);dm_sim_char c;dm_sim_char_init(&c,d->skill[id]->cls);
    c.clvl=90;c.life=1200;c.mana=100000;c.ar=4000;c.str=150;c.dex=120;
    c.hand[0].min=100;c.hand[0].max=160;c.hand[0].kind=DM_WK_MELEE;
    strcpy(c.hand[0].wclass,"1HS");c.lvl[id]=c.blvl[id]=20;
    return c;
}
static dm_sim_skill_out action(const dm_sim_data *d,const dm_sim_char *c,const dm_sim_target *t,int id) {
    dm_sim_tmod m;dm_sim_tmod_init(&m);dm_sim_skill_out o;
    assert(!sim_action(d,c,t,&m,skill(d,id),&o));assert(!(o.flags&DM_SF_UNAVAIL));return o;
}
static void spells(const dm_sim_data *d) {
    dm_sim_target t;assert(!dm_sim_target_std(d,0,0,&t));
    const int hits[]={64,256,38,67};
    const int factors[]={3,1,3,1};
    for(int k=0;k<4;k++) {
        int id=skill(d,hits[k]),bad=0;dm_sim_char c=character(d,hits[k]);
        if(k==0)c.lvl[skill(d,39)]=c.blvl[skill(d,39)]=20;
        dm_sim_skill_out o=action(d,&c,&t,hits[k]);
        double one=(dm_sim_skill_value(d,&c,id,20,DM_SV_EMIN,&bad)+dm_sim_skill_value(d,&c,id,20,DM_SV_EMAX,&bad))/2;
        assert(!bad);assert(o.flags&DM_SF_ESTIMATE);assert(!o.burn);NEAR(o.hits,factors[k]);NEAR(o.raw[d->skill[id]->etype],one*factors[k]);
        if(k==0){NEAR(one,376.5);assert(!dm_sim_skill_eval(d,&c,&t,id,&o));NEAR(o.dps,1129.5);}
    }
    const int burns[]={51,41,46,225,230};
    for(int k=0;k<5;k++){dm_sim_char c=character(d,burns[k]);dm_sim_skill_out o=action(d,&c,&t,burns[k]);assert(o.burn>0 && o.burn_frames>0 && o.flags&DM_SF_ESTIMATE);}
    dm_sim_char c=character(d,56);dm_sim_skill_out meteor=action(d,&c,&t,56);
    NEAR(meteor.raw[DM_EL_FIRE],898);NEAR(meteor.hits,1);assert(meteor.burn>0 && meteor.burn_frames==314);assert(meteor.flags&DM_SF_ESTIMATE);
    int mi=d->skill[skill(d,56)]->burnmiss;assert(mi>=0 && d->miss[mi]->hitshift==3);
    int low=dm_sim_level_value(d->miss[mi]->emin,d->miss[mi]->eminlev,20)<<3;
    int high=dm_sim_level_value(d->miss[mi]->emax,d->miss[mi]->emaxlev,20)<<3;
    uint64_t words=UINT64_C(1)<<32,width=(uint64_t)(high-low);double total=0;
    for(int value=low;value<high;value++)total+=(value*(100-t.res[DM_EL_FIRE])/100)*(words/width+((uint64_t)(value-low)<words%width));
    NEAR(meteor.burn,total/words/256*314);
    const int timed[]={249,250,277,57};
    for(int k=0;k<4;k++) {
        c=character(d,timed[k]);int id=skill(d,timed[k]);dm_sim_skill_out a,b;
        assert(!dm_sim_skill_eval(d,&c,&t,id,&a));c.fcr=200;assert(!dm_sim_skill_eval(d,&c,&t,id,&b));
        assert(a.flags&DM_SF_TIMED);assert(a.flags&DM_SF_ESTIMATE);assert(a.dur>0);assert(a.frames>b.frames);
        NEAR(a.dps,b.dps);assert(a.dps>0 && !sim_sum(a.hit));
        int bad=0;double period=sim_calc(d,&c,id,DM_CF_STRIKEPERIOD,&bad);assert(!bad);
        double expected=0;for(int e=0;e<DM_EL_N;e++)expected+=a.raw[e]*25/period*(100-t.res[e])/100;NEAR(a.dps,expected);
        if(k==2){c.hand[0].min=c.hand[0].max=0;assert(!dm_sim_skill_eval(d,&c,&t,id,&b));assert(b.dps<a.dps);NEAR(period,25);}
    }
    c=character(d,101);dm_sim_skill_out a=action(d,&c,&t,101);
    dm_sim_target undead;assert(!dm_sim_target_mon(d,monster(d,5),0,0,DM_TK_NORMAL,1,&undead));
    dm_sim_skill_out b=action(d,&c,&undead,101);NEAR(a.raw[DM_EL_MAG],b.raw[DM_EL_MAG]*(t.undead_share+t.demon_share)/100);
    int found=0;for(int i=0;i<d->nsu;i++)if(d->su[i].idx==6) {
        dm_sim_target tower;assert(!dm_sim_target_su(d,i,0,&tower));assert(!tower.undead_share && tower.demon_share==100);
        a=action(d,&c,&tower,101);NEAR(a.raw[DM_EL_MAG],b.raw[DM_EL_MAG]);found=1;
    }assert(found);
    dm_sim_target animal;assert(!dm_sim_target_mon(d,monster(d,63),0,0,DM_TK_NORMAL,1,&animal));
    a=action(d,&c,&animal,101);NEAR(sim_sum(a.raw),0);
}
static void weapons(const dm_sim_data *d) {
    dm_sim_target t;assert(!dm_sim_target_std(d,0,0,&t));dm_sim_char c=character(d,151);c.ias=40;
    dm_sim_skill_out a=action(d,&c,&t,151);assert(a.frames==5 && a.cooldown==5);
    assert(!dm_sim_skill_eval(d,&c,&t,skill(d,151),&a));NEAR(a.dps,sim_sum(a.hit)*5);
    dm_sim_tmod m;dm_sim_tmod_init(&m);dm_sim_rot_out r={0};sim_timeline(d,&c,&t,&m,&a,1,&r,NULL);
    NEAR(r.ttk,-2);
    c=character(d,111);a=action(d,&c,&t,111);dm_sim_skill_out b=action(d,&c,&t,0);
    NEAR(a.raw[0]/a.cth,b.raw[0]/b.cth);
    for(int e=1;e<=3;e++)NEAR(a.raw[e]/a.cth,130*1.84);
    const int names[]={116,254,265};
    for(int k=0;k<3;k++){c=character(d,names[k]);a=action(d,&c,&t,names[k]);for(int e=1;e<DM_EL_N;e++)NEAR(a.raw[e],0);}
    c=character(d,238);c.ias=0;c.form=1;a=action(d,&c,&t,238);assert(a.frames==10);
    c=character(d,242);c.form=2;a=action(d,&c,&t,242);assert(a.frames==12);
    c.form=0;a=action(d,&c,&t,242);assert(a.flags&DM_SF_ESTIMATE);
    c.form=2;a=action(d,&c,&t,0);assert(a.frames==19 && (a.flags&DM_SF_ESTIMATE));
    char err[128];assert(dm_sim_profile_parse(d,"weapon=100,160,100,0,0,1,0,0,XYZ\n",&c,err,sizeof err)<0);
}
static void targets(const dm_sim_data *d) {
    const int names[]={705,704,709,333,5,243};
    const int regen[]={4,4,4,2,2,0};
    for(int i=0;i<6;i++) {
        dm_sim_target t;int mon=monster(d,names[i]);assert(!dm_sim_target_mon(d,mon,2,80,DM_TK_NORMAL,1,&t));assert(t.regen==regen[i]);
        if(i==4)for(int kind=DM_TK_CHAMPION;kind<=DM_TK_SUPERUNIQUE;kind++){assert(!dm_sim_target_mon(d,mon,2,80,kind,1,&t));assert(t.regen==2);}
    }
    for(int i=0;i<d->nboss;i++)if(d->boss[i].kind==DM_BK_ANCIENT) {
        dm_sim_target t;assert(!dm_sim_target_mon(d,d->boss[i].mon,2,0,DM_TK_ANCIENT,1,&t));assert(t.regen==4);
    }
    dm_sim_char c=character(d,66);c.diff=2;c.sunder[0]=1;dm_sim_tmod m;dm_sim_tmod_init(&m);int bad=0;
    sim_modify(d,&c,&m,skill(d,66),1,&bad);assert(!bad);NEAR(m.sunder_res[0],-100);
    dm_sim_target t;assert(!dm_sim_target_mon(d,monster(d,38),2,80,DM_TK_NORMAL,1,&t));assert(t.res[0]>=100);
    dm_sim_skill_out a;assert(!sim_action(d,&c,&t,&m,0,&a));NEAR(a.hit[0],a.raw[0]*1.05);
    int mon=monster(d,3);
    for(int df=0;df<3;df++)for(int kind=DM_TK_CHAMPION;kind<=DM_TK_SUPERUNIQUE;kind++) {
        assert(!dm_sim_target_mon(d,mon,df,80,kind,1,&t));const dm_sim_mond *raw=&d->mon[mon].d[df];
        int ei=(kind==DM_TK_MINION?16:kind==DM_TK_CHAMPION?22:28)+df;
        int mn=raw->elmin[0]*d->monlvl[t.mlvl][9+df]/100,mx=raw->elmax[0]*d->monlvl[t.mlvl][9+df]/100;
        assert(t.elmin[0]==mn*(100+d->uconst[ei])/100);assert(t.elmax[0]==mx*(100+d->uconst[ei+3])/100);
        int ar=raw->a1th*d->monlvl[t.mlvl][6+df]/100;
        assert(t.ar==ar*(100+d->uconst[kind==DM_TK_CHAMPION?10:kind==DM_TK_MINION?12:13])/100);
    }
    int enchanted=0;for(int i=0;i<d->nsu;i++)for(int k=0;k<3;k++)for(int e=2;e<=4;e++)if(d->su[i].mods[k]==d->umod[e]) {
        assert(!dm_sim_target_su(d,i,2,&t));assert(t.flags&DM_MF_UNDERESTIMATED);enchanted++;
    }assert(enchanted);
    assert(!dm_sim_target_mon(d,monster(d,526),2,0,DM_TK_QUEST,1,&t));
    dm_sim_surv_out surv;assert(!dm_sim_survive(d,&c,&t,NULL,&surv));assert(surv.underestimated && surv.ttd==-2);
    dm_sim_light_out light;assert(!dm_sim_light(&t,1,&surv,0,&light));assert(light.light!=DM_LIGHT_GREEN);
    assert(!dm_sim_target_std(d,2,0,&t));assert(t.drain==66);
}
static void pets_and_defense(const dm_sim_data *d) {
    dm_sim_target t;assert(!dm_sim_target_std(d,0,0,&t));dm_sim_char c=character(d,75);
    dm_sim_skill_out a=action(d,&c,&t,75);int sk=skill(d,75);
    dm_sim_target pet;assert(!dm_sim_target_mon(d,d->skill[sk]->summon,0,90,DM_TK_NORMAL,1,&pet));
    double expected=(pet.dmin+pet.dmax)/2.0*7.65*dm_sim_cth(pet.ar+400,t.ac,90,t.mlvl)/100.0*(1-t.block/100.0)*25/pet.cycle;
    NEAR(a.raw[0],expected);
    c=character(d,85);c.lvl[skill(d,79)]=c.blvl[skill(d,79)]=20;a=action(d,&c,&t,85);sk=skill(d,85);
    assert(!dm_sim_target_mon(d,d->skill[sk]->summon,0,90,DM_TK_NORMAL,1,&pet));
    int calc_bad=0;double ed=0,ar=pet.ar;
    for(int j=0;j<14;j++)if(d->skill[sk]->pstat[j]>=0) {
        const char *stat=d->stat_name[d->skill[sk]->pstat[j]];double value=sim_calc(d,&c,sk,DM_CF_PASSIVE1+j,&calc_bad);
        if(!strcmp(stat,"damagepercent"))ed+=value;
        if(!strcmp(stat,"tohit"))ar+=value;
    }
    assert(!calc_bad && ed>0 && ar>pet.ar);
    expected=(pet.dmin+pet.dmax)/2.0*(100+ed)/100*dm_sim_cth((int)ar,t.ac,90,t.mlvl)/100.0*(1-t.block/100.0)*25/pet.cycle;
    NEAR(a.raw[0],expected);
    c=character(d,80);assert(!dm_sim_skill_eval(d,&c,&t,skill(d,80),&a));assert(!(a.flags&DM_SF_UNAVAIL) && (a.flags&DM_SF_ESTIMATE));assert(a.dps>0);
    c=character(d,235);c.diff=2;dm_sim_char armored=c;dm_sim_tmod m;dm_sim_tmod_init(&m);int bad=0;
    sim_modify(d,&armored,&m,skill(d,235),1,&bad);assert(!bad);assert(!armored.bone_pool && armored.cyclone_pool==268);
    assert(!dm_sim_target_mon(d,monster(d,19),2,80,DM_TK_NORMAL,1,&t));
    dm_sim_surv_out before,after;assert(!dm_sim_survive(d,&c,&t,NULL,&before));assert(!dm_sim_survive(d,&armored,&t,NULL,&after));
    assert(after.max_hit<before.max_hit);NEAR((before.max_hit-after.max_hit)*c.life,fmin(2*t.elmax[0],268)*2);
    dm_sim_target renew_target;assert(!dm_sim_target_mon(d,monster(d,75),2,80,DM_TK_NORMAL,1,&renew_target));
    dm_sim_char renewal=c;renewal.bar[renewal.nbar++]=skill(d,235);dm_sim_surv_out recast;
    assert(!dm_sim_survive(d,&armored,&renew_target,NULL,&after));
    assert(!dm_sim_survive(d,&renewal,&renew_target,NULL,&recast));assert(recast.ttd<0 || recast.ttd>after.ttd);
    assert(!dm_sim_target_mon(d,monster(d,5),2,80,DM_TK_NORMAL,1,&t));
    assert(!dm_sim_survive(d,&c,&t,NULL,&before));assert(!dm_sim_survive(d,&armored,&t,NULL,&after));NEAR(before.max_hit,after.max_hit);NEAR(before.ttd,after.ttd);
    c=character(d,117);c.clvl=80;c.block=20;c.shield=1;sim_modify(d,&c,&m,skill(d,117),1,&bad);assert(c.block==42 && !bad);
    c=character(d,68);sim_modify(d,&c,&m,skill(d,68),1,&bad);assert(c.bone_pool==305 && c.cyclone_pool==0);
    int found=0;for(int i=0;i<d->nmon && !found;i++) {
        if(!d->mon[i].id[0])continue;
        assert(!dm_sim_target_mon(d,i,0,0,DM_TK_NORMAL,1,&t));
        for(int k=0;k<3;k++)if(t.eltype[k]==DM_EL_POIS && t.elpct[k]>0 && t.elmax[k]>0) {
            c=character(d,0);c.dr_flat=100000;c.mdr_flat=0;assert(!dm_sim_survive(d,&c,&t,NULL,&before));
            c.mdr_flat=100000;assert(!dm_sim_survive(d,&c,&t,NULL,&after));NEAR(before.dmg_hit,after.dmg_hit);found=1;break;
        }
    }assert(found);
}
static void forms_and_classes(const dm_sim_data *d) {
    dm_sim_target t;assert(!dm_sim_target_std(d,0,0,&t));
    dm_sim_char c=character(d,233);
    const char *classes[]={"HTH","1HS","STF","2HS","2HT"};
    const int human[]={16,19,17,21,23};
    for(int form=1;form<=2;form++) {
        c.form=form;int first=0;
        for(int k=0;k<5;k++) {
            strcpy(c.hand[0].wclass,classes[k]);
            dm_sim_skill_out a=action(d,&c,&t,0);
            int length=0;
            for(int i=0;i<d->nanim;i++)if(!strcmp(d->anim[i].token,form==1?"40":"TG") && !strcmp(d->anim[i].mode,"A1"))length=d->anim[i].frames;
            assert(length>0);int rate=256*length/human[k];
            assert(a.frames==(256*length+rate-1)/rate-1 && (a.flags&DM_SF_ESTIMATE));
            if(k==0)first=a.frames;else assert(a.frames!=first);
        }
    }
    const char *dual[]={"1JS","1JT","1SS","1ST"};
    for(int k=0;k<4;k++) {
        char profile[256],err[128];snprintf(profile,sizeof profile,"class=4\nlevel=90\nweapon=100,160,100,0,0,1,0,0,%s\noffhand=100,160,100,0,0,1,0,0,1HS\nskill=147,20,20\nbar=147\n",dual[k]);
        assert(!dm_sim_profile_parse(d,profile,&c,err,sizeof err));
        dm_sim_skill_out a=action(d,&c,&t,147);assert(sim_sum(a.hit)>0);
    }
}
static void survival_lights(const dm_sim_data *d) {
    dm_sim_char c=character(d,106);c.clvl=80;c.mana=500;c.life=2000;c.def=1000;
    c.hand[0].min=500;c.hand[0].max=800;c.ias=40;
    c.bar[c.nbar++]=skill(d,106);for(int e=1;e<=4;e++)c.res[e]=175;
    for(int standard=0;standard<2;standard++)for(int diff=0;diff<=2;diff+=2) {
        c.diff=diff;dm_sim_target t;
        if(standard)assert(!dm_sim_target_std(d,diff,0,&t));
        else {assert(!dm_sim_target_mon(d,monster(d,0),diff,80,DM_TK_NORMAL,1,&t));assert(!(t.flags&DM_MF_UNDERESTIMATED));}
        dm_sim_rot_out r;dm_sim_surv_out s;dm_sim_light_out light;
        assert(!dm_sim_rotation(d,&c,&t,&r));assert(!dm_sim_survive(d,&c,&t,&r,&s));
        assert(!dm_sim_light(&t,r.ttk,&s,0,&light));
        assert(s.ttd!=-2 && r.ttk==-2 && light.light==DM_LIGHT_UNKNOWN);
        if(standard)assert(s.underestimated);
        if(diff==0)assert(s.ttd==-1);else assert(s.ttd>0);
    }
    const int utility[]={141,68};
    for(int i=0;i<2;i++){dm_sim_target t;assert(!dm_sim_target_mon(d,monster(d,utility[i]),0,0,DM_TK_NORMAL,1,&t));assert(!(t.flags&DM_MF_UNDERESTIMATED));}
    dm_sim_target t;assert(!dm_sim_target_std(d,2,0,&t));
    assert(d->std[2][0].elavg==46 && d->std[2][0].elpct==40);
    c=character(d,106);c.diff=2;
    for(int e=1;e<=4;e++)c.res[e]=100;
    dm_sim_surv_out s;assert(!dm_sim_survive(d,&c,&t,NULL,&s));
    NEAR(s.max_hit*c.life,2*(t.dmax+45));
    NEAR(s.dmg_hit,((t.dmin+t.dmax)/2.0+45*t.elpct[0]/100.0)*1.05);
    c.life=(int)(2*(t.dmax+45));assert(!dm_sim_survive(d,&c,&t,NULL,&s));
    dm_sim_light_out light;assert(!dm_sim_light(&t,1,&s,0,&light));assert(light.reason==DM_LR_ONESHOT);
    c.life*=2;assert(!dm_sim_survive(d,&c,&t,NULL,&s));
    assert(!dm_sim_light(&t,1,&s,0,&light));NEAR(s.max_hit,.5);assert(light.light==DM_LIGHT_ORANGE);
}
static void mages(const dm_sim_data *d) {
    int id=skill(d,80),mastery=skill(d,69);
    dm_sim_target t;assert(!dm_sim_target_std(d,0,0,&t));
    dm_sim_char c=character(d,80);
    dm_sim_target pet;assert(!dm_sim_target_mon(d,d->skill[id]->summon,0,90,DM_TK_NORMAL,1,&pet));
    assert(pet.cycle==35);

    const int levels[]={1,20,20},masteries[]={0,0,20},counts[]={1,8,8};
    const double fire[]={4,21,123},cold[]={3,12,101},light[]={4,21,114},poison[]={12,78,364};
    for(int k=0;k<3;k++) {
        c.lvl[id]=levels[k];c.lvl[mastery]=masteries[k];dm_sim_skill_out a=action(d,&c,&t,80);
        assert((a.flags&DM_SF_ESTIMATE) && !(a.flags&DM_SF_UNAVAIL));
        NEAR(a.pet_element[0],0);NEAR(a.pet_element[DM_EL_MAG],0);
        NEAR(a.pet_element[DM_EL_FIRE],fire[k]*counts[k]*.25*25/35);
        NEAR(a.pet_element[DM_EL_COLD],cold[k]*counts[k]*.25*25/35);
        NEAR(a.pet_element[DM_EL_LTNG],light[k]*counts[k]*.25*25/35);
        NEAR(a.pet_element[DM_EL_POIS],poison[k]/256*25*(1-pow(.75,counts[k])));
        NEAR(a.pet_poison_uptime,1-pow(.75,counts[k]));
        dm_sim_skill_out evaluated;assert(!dm_sim_skill_eval(d,&c,&t,id,&evaluated));
        NEAR(evaluated.ttk,-2);
        c.active_lvl[id]=c.lvl[id];
        assert(!dm_sim_skill_eval(d,&c,&t,id,&evaluated));
        NEAR(evaluated.ttk,-2);
        c.active_lvl[id]=0;
        dm_sim_target immune=t;immune.res[DM_EL_COLD]=100;dm_sim_skill_out b=action(d,&c,&immune,80);
        NEAR(b.pet_element[DM_EL_COLD],0);NEAR(b.pet_dps,a.pet_dps-a.pet_element[DM_EL_COLD]);
        for(int e=1;e<=4;e++) {
            immune=t;immune.res[e]=50;b=action(d,&c,&immune,80);
            NEAR(b.pet_dps,a.pet_dps-a.pet_element[e]*.5);
        }
        dm_sim_char equipped=c;for(int e=1;e<=4;e++){equipped.mastery[e]=200;equipped.pierce[e]=50;}
        b=action(d,&equipped,&t,80);NEAR(b.pet_dps,a.pet_dps);
    }
    c.bar[c.nbar++]=id;dm_sim_rot_out r;assert(!dm_sim_rotation(d,&c,&t,&r));assert(!(r.flags&DM_SF_UNAVAIL) && r.dps>0);
    int other=skill(d,47);c.lvl[other]=20;c.bar[c.nbar++]=other;
    dm_sim_skill *broken=d->skill[other];int saved=broken->unavailable;broken->unavailable=1;
    dm_sim_rot_out partial;assert(!dm_sim_rotation(d,&c,&t,&partial));
    assert(!(partial.flags&DM_SF_UNAVAIL) && (partial.flags&DM_SF_ESTIMATE));NEAR(partial.dps,r.dps);NEAR(partial.ttk,r.ttk);
    broken->unavailable=saved;
    int calc=broken->calc[DM_CF_EDMGSYM];broken->calc[DM_CF_EDMGSYM]=-2;
    assert(!dm_sim_rotation(d,&c,&t,&partial));assert(!(partial.flags&DM_SF_UNAVAIL));NEAR(partial.dps,r.dps);
    broken->calc[DM_CF_EDMGSYM]=calc;c.nbar--;
    int buff=skill(d,52);c.lvl[buff]=20;c.bar[c.nbar++]=buff;
    broken=d->skill[buff];calc=broken->calc[DM_CF_AURASTAT1];broken->calc[DM_CF_AURASTAT1]=-2;
    assert(!dm_sim_rotation(d,&c,&t,&partial));assert(!(partial.flags&DM_SF_UNAVAIL));NEAR(partial.dps,r.dps);NEAR(partial.ttk,r.ttk);
    broken->calc[DM_CF_AURASTAT1]=calc;
    dm_sim_target cold_target;assert(!dm_sim_target_mon(d,monster(d,501),2,80,DM_TK_NORMAL,1,&cold_target));
    assert(cold_target.res[DM_EL_COLD]>=100);c.diff=2;
    dm_sim_skill_out immune=action(d,&c,&cold_target,80);
    NEAR(immune.pet_element[DM_EL_COLD],0);
    assert(immune.pet_element[DM_EL_FIRE]>0 && immune.pet_element[DM_EL_LTNG]>0 && immune.pet_element[DM_EL_POIS]>0);
    cold_target.res[DM_EL_COLD]=0;dm_sim_skill_out vulnerable=action(d,&c,&cold_target,80);
    NEAR(immune.pet_dps,vulnerable.pet_dps-vulnerable.pet_element[DM_EL_COLD]);
}
static void timed_start(const dm_sim_data *d) {
    const int names[]={249,250,277,57};
    dm_sim_target t;assert(!dm_sim_target_std(d,0,0,&t));
    for(int k=0;k<4;k++) {
        int id=skill(d,names[k]);dm_sim_char c=character(d,names[k]);
        dm_sim_skill_out a=action(d,&c,&t,names[k]);
        dm_sim_tmod m;dm_sim_tmod_init(&m);dm_sim_rot_out cold={0},warm={0},empty={0};
        sim_timeline(d,&c,&t,&m,&a,1,&cold,NULL);
        c.active_buff[c.nactive_buff++]=id;sim_timeline(d,&c,&t,&m,&a,1,&warm,NULL);
        NEAR(cold.ttk,-2);NEAR(warm.ttk,-2);
        assert(cold.mana_spent>=a.mana_x256/256.0 && warm.mana_spent<=cold.mana_spent);
        c.nactive_buff=0;c.mana=0;sim_timeline(d,&c,&t,&m,&a,1,&empty,NULL);
        assert(empty.ttk==-2);NEAR(empty.dps,cold.dps);NEAR(empty.mana_spent,0);
        c.bar[c.nbar++]=id;c.hand[0].min=c.hand[0].max=0;
        assert(!dm_sim_rotation(d,&c,&t,&empty));assert(empty.ttk==-2 && empty.dps>0);
        assert(!dm_sim_skill_eval(d,&c,&t,id,&a));assert(a.ttk==-2);
    }
}
void sim_review(const dm_sim_data *d) {
    spells(d);weapons(d);targets(d);pets_and_defense(d);
    forms_and_classes(d);survival_lights(d);mages(d);timed_start(d);
    printf("%s: combat review regressions on exported tables passed\n",d->set_name);
}
