#include "dm_sim_internal.h"
#include "dm_splash_rule.h"
#include <stdio.h>

typedef struct { const dm_sim_data *d; const dm_sim_char *c; int override_id, override_lvl; } calc_ctx;
static int64_t calc_var(const dm_sim_calc_env *, int, int, int *);
static int64_t calc_stat(const dm_sim_calc_env *env, int id) {
    const calc_ctx *x=env->user; const dm_sim_char *c=x->c;
    for (int i=0;i<DM_ST_N;i++) if (x->d->stat[i]==id) {
        switch(i) {
        case DM_ST_STR:return c->str; case DM_ST_DEX:return c->dex; case DM_ST_VIT:return c->vit; case DM_ST_ENE:return c->ene;
        case DM_ST_MAXHP:return c->life*256; case DM_ST_MAXMANA:return c->mana*256; case DM_ST_AC:return c->def; case DM_ST_TOHIT:return c->ar;
        case DM_ST_DMG_PCT:return c->ed; case DM_ST_DS:return c->ds; case DM_ST_CRIT:return c->cs; case DM_ST_CB:return c->cb; case DM_ST_OW:return c->ow;
        case DM_ST_LL:return c->ll; case DM_ST_ML:return c->ml; case DM_ST_IAS:return c->ias; case DM_ST_FCR:return c->fcr; case DM_ST_SIAS:return c->sias;
        case DM_ST_RES_PHYS:return c->res[0]; case DM_ST_RES_MAG:return c->res[5];
        case DM_ST_MAST_FIRE:case DM_ST_MAST_LTNG:case DM_ST_MAST_COLD:case DM_ST_MAST_POIS:case DM_ST_MAST_MAG:return c->mastery[i-DM_ST_MAST_FIRE+1];
        case DM_ST_PIERCE_FIRE:case DM_ST_PIERCE_LTNG:case DM_ST_PIERCE_COLD:case DM_ST_PIERCE_POIS:case DM_ST_PIERCE_MAG:return c->pierce[i-DM_ST_PIERCE_FIRE+1];
        case DM_ST_PIERCE_PHYS:return c->pierce[0];
        default: if (i>=DM_ST_RES_FIRE && i<=DM_ST_RES_POIS) return c->res[i-DM_ST_RES_FIRE+1]; break;
        }
    }
    return id>=0 && id<512 ? c->stat_value[id] : 0;
}
static int64_t slot(const dm_sim_calc_env *e, const dm_sim_skill *s, int cf, int *bad) {
    const calc_ctx *x=e->user;
    dm_sim_calc_env sub=*e; sub.depth++;
    return dm_sim_calc_eval(x->d,s->calc[cf],&sub,bad);
}
static double skill_value(const dm_sim_calc_env *env, int which, int mastery, int *bad) {
    const calc_ctx *x=env->user; const dm_sim_skill *s=x->d->skill[env->skill];
    if (!s) { *bad=1; return 0; }
    int lv=env->skill==x->override_id ? x->override_lvl : x->c->lvl[env->skill];
    if (!lv) return 0;
    if (which==DM_SV_ELEN) return dm_sim_len_value(s->elen,s->elevlen,lv)*(100.0+slot(env,s,DM_CF_ELENSYM,bad))/100;
    int elem=which<DM_SV_PMIN, max=which==DM_SV_EMAX || which==DM_SV_PMAX;
    if(!elem && s->kick)return (int)fmax(1,x->c->str+x->c->dex-20)/(max?3:4);
    int v=dm_sim_level_value(elem?(max?s->emax:s->emin):(max?s->maxdam:s->mindam),
                             elem?(max?s->emaxlev:s->eminlev):(max?s->maxlev:s->minlev),lv);
    if(!elem)v+=(int)((int64_t)v*slot(env,s,DM_CF_DMGSYM,bad)/100);
    double raw=ldexp(v,sim_clamp(s->hitshift,0,16)-8);

    if(elem && (max || raw>1 || s->eminlev[0]))
        raw=floor(raw*256*(100+slot(env,s,DM_CF_EDMGSYM,bad))/100)/256;
    if (elem && mastery && s->etype>0 && s->etype<DM_EL_N) raw=floor(raw*256*(100+x->c->mastery[s->etype])/100)/256;
    return fmax(0,raw);
}
static int64_t calc_var(const dm_sim_calc_env *env, int id, int var, int *bad) {
    const calc_ctx *x=env->user; const dm_sim_skill *s=id>=0 && id<DM_SIM_MAXSKILL?x->d->skill[id]:NULL;
    if (!s || env->depth>24) { *bad=1; return 0; }
    dm_sim_calc_env e=*env; e.skill=id;
    int lv=id==x->override_id?x->override_lvl:x->c->lvl[id];
    if (var>=DM_V_CLC1 && var<DM_V_AST1) return slot(&e,s,var-DM_V_CLC1,bad);
    if (var>=DM_V_AST1 && var<DM_V_PST1) return slot(&e,s,DM_CF_AURASTAT1+var-DM_V_AST1,bad);
    if (var>=DM_V_PST1 && var<DM_V_N) return slot(&e,s,DM_CF_PASSIVE1+var-DM_V_PST1,bad);
    switch(var) {
    case DM_V_LVL:return lv; case DM_V_BLVL: return x->c->blvl[id]; case DM_V_ULVL:return x->c->clvl;
    case DM_V_EDMN:case DM_V_EDMX:case DM_V_EDNS:case DM_V_EDXS:
    case DM_V_ENMA:case DM_V_EXMA:case DM_V_ENMS:case DM_V_EXMS: {
        int max=var==DM_V_EDMX || var==DM_V_EDXS || var==DM_V_EXMA || var==DM_V_EXMS;
        int scaled=var==DM_V_EDNS || var==DM_V_EDXS || var==DM_V_ENMS || var==DM_V_EXMS;
        return (int64_t)(skill_value(&e,max?DM_SV_EMAX:DM_SV_EMIN,var>=DM_V_ENMA,bad)*(scaled?256:1));
    }
    case DM_V_PNMA:case DM_V_PXMA:case DM_V_PNMS:case DM_V_PXMS:
        return (int64_t)(skill_value(&e,var==DM_V_PXMA || var==DM_V_PXMS?DM_SV_PMAX:DM_SV_PMIN,0,bad)*(var>=DM_V_PNMS?256:1));
    case DM_V_EDLN:case DM_V_EDMA:return (int64_t)skill_value(&e,DM_SV_ELEN,0,bad);
    case DM_V_TOHT:return s->calc[DM_CF_TOHIT]>=0 ? slot(&e,s,DM_CF_TOHIT,bad):s->tohit+s->levtohit*(lv-1);
    case DM_V_MANA:return dm_sim_mana_cost(s,lv)/256; case DM_V_USMC:return dm_sim_mana_cost(s,lv);
    case DM_V_MPS:return dm_sim_mana_cost(s,lv)*25/256;
    case DM_V_LEN:return slot(&e,s,DM_CF_AURALEN,bad); case DM_V_RNG:return slot(&e,s,DM_CF_AURARANGE,bad);
    case DM_V_PETS:return slot(&e,s,DM_CF_PETMAX,bad); case DM_V_MAEL:return s->etype>0 && s->etype<DM_EL_N?x->c->mastery[s->etype]:0;
    case DM_V_MATH:case DM_V_MADM:case DM_V_MACR: {
        if(lv<1)return 0;
        const char *suffix=var==DM_V_MATH?"th":var==DM_V_MADM?"dmg":"crit";
        char melee[64],thrown[64];
        snprintf(melee,sizeof melee,"passive_mastery_melee_%s",suffix);
        snprintf(thrown,sizeof thrown,"passive_mastery_throw_%s",suffix);
        for(int k=0;k<14;k++)if(s->pstat[k]>=0 && s->pstat[k]<512 &&
            (!strcmp(x->d->stat_name[s->pstat[k]],melee) || !strcmp(x->d->stat_name[s->pstat[k]],thrown)))
            return slot(&e,s,DM_CF_PASSIVE1+k,bad);
        return 0;
    }
    case DM_V_FNUM:return x->c->missile_frame;
    case DM_V_SKPT:return x->c->blvl[id]; default:*bad=1;return 0;
    }
}
int64_t sim_skill_var(const dm_sim_data *d,const dm_sim_char *c,int id,int var,int *bad) {
    if(!d || !c || !bad || id<0 || id>=DM_SIM_MAXSKILL || !d->skill[id]){if(bad)*bad=1;return 0;}
    calc_ctx x={d,c,-1,0};dm_sim_calc_env e={calc_var,calc_stat,id,0,&x};
    return calc_var(&e,id,var,bad);
}
double sim_calc(const dm_sim_data *d,const dm_sim_char *c,int id,int cf,int *bad) {
    if (id<0 || id>=DM_SIM_MAXSKILL || !d->skill[id] || cf<0 || cf>=DM_CF_N) { *bad=1;return 0; }
    calc_ctx x={d,c,-1,0}; dm_sim_calc_env e={calc_var,calc_stat,id,0,&x};
    return (double)slot(&e,d->skill[id],cf,bad);
}
double dm_sim_skill_value(const dm_sim_data *d,const dm_sim_char *c,int id,int lv,int which,int *bad) {
    if (!d || !c || !bad || id<0 || id>=DM_SIM_MAXSKILL || !d->skill[id]) { if(bad)*bad=1;return 0; }
    calc_ctx x={d,c,id,lv}; dm_sim_calc_env e={calc_var,calc_stat,id,0,&x};
    double value=skill_value(&e,which,1,bad);
    return d->skill[id]->etype==DM_EL_POIS || d->skill[id]->frameburn ? value : floor(value);
}
int64_t dm_sim_skill_raw(const dm_sim_data *d,const dm_sim_char *c,int id,int lv,int which,int *bad) {
    if(!d || !c || !bad || id<0 || id>=DM_SIM_MAXSKILL || !d->skill[id] || which<DM_SV_EMIN || which>DM_SV_PMAX) {
        if(bad)*bad=1;
        return 0;
    }
    calc_ctx x={d,c,id,lv};dm_sim_calc_env e={calc_var,calc_stat,id,0,&x};
    return (int64_t)(skill_value(&e,which,1,bad)*256);
}
static int elem_stat(const char *n) {
    const char *names[]={"damageresist","fireresist","lightresist","coldresist","poisonresist","magicresist"};
    for(int i=0;i<6;i++) if(!strcmp(n,names[i]))return i;
    return -1;
}
static void apply_stat(const dm_sim_data *d,dm_sim_char *c,dm_sim_tmod *m,const dm_sim_skill *s,int id,int v,int target,int sign) {
    if(id<0 || id>=512)return;
    const char *n=d->stat_name[id]; int e=elem_stat(n);
    if(target) {
        if(e>=0) {
            m->res[e]+=v;m->immune_res[e]+=v/5;int full=0;
            for(int k=0;k<d->nstate;k++)if(!strcmp(s->auratargetstate[0]?s->auratargetstate:s->aurastate,d->state[k].name)){m->skill_res_state[e]=k;full=d->state[k].sunderfull && !d->state[k].sunderreduce;}
            m->sunder_res[e]+=full?v:v/5;m->res_rules|=1<<e;
        }
        if(!strcmp(n,"damagepercent"))m->dmg_pct+=v;
        if(!strcmp(n,"item_armor_percent") || !strcmp(n,"skill_armor_percent"))m->def_pct+=v;
        if(!strcmp(n,"item_tohit_percent"))m->ar_pct+=v;
        return;
    }
    c->stat_value[id]+=v;
    if(e>=0)c->res[e]+=v;
    else if(!strcmp(n,"damagepercent") || !strcmp(n,"skill_damagepercent"))c->ed+=v;
    else if(!strcmp(n,"attackrate"))c->sias+=v;
    else if(!strcmp(n,"item_tohit_percent"))c->ar_pct+=v;
    else if(!strcmp(n,"armorclass"))c->def+=v;
    else if(!strcmp(n,"tohit")){c->ar+=v;if(c->ar_base_known)c->ar_base+=v;}
    else if(!strcmp(n,"skill_armor_percent") || !strcmp(n,"item_armor_percent"))c->def=(int)(sign>0?c->def*(100.0+v)/100:c->def*100.0/fmax(1,100-v));
    else if(!strcmp(n,"item_maxhp_percent"))c->life=floor(256*(sign>0?c->life*(100.0+v)/100:c->life*100.0/fmax(1,100-v)))/256;
    else if(!strcmp(n,"item_maxmana_percent"))c->mana=floor(256*(sign>0?c->mana*(100.0+v)/100:c->mana*100.0/fmax(1,100-v)))/256;
    else if(!strncmp(n,"max",3) && strstr(n,"resist") && n[3]) {
        static const char *const maxres[DM_EL_N]={"","maxfireresist","maxlightresist","maxcoldresist","maxpoisonresist",""};
        for(int k=1;k<DM_EL_N;k++)if(maxres[k][0] && !strcmp(n,maxres[k]))c->maxres[k]+=v;
    }
    else if(!strcmp(n,"hpregen"))c->liferegen+=v;
    else if(!strcmp(n,"manarecoverybonus") || !strcmp(n,"item_manaregen"))c->manaregen_pct+=v;
    else if(!strcmp(n,"item_allskills")) {for(int j=1;j<DM_SIM_MAXSKILL;j++)if(c->lvl[j]>0)c->lvl[j]=(int16_t)sim_clamp(c->lvl[j]+v,1,256);}
    else if(!strcmp(n,"passive_dodge") || !strcmp(n,"passive_avoid"))c->dodge+=v;
    else if(!strcmp(n,"passive_weaponblock"))c->block+=v;
    else if(!strcmp(n,"toblock"))c->block+=(int)(v*fmax(0,c->dex-15)/(2*fmax(1,c->clvl)));
    else if(!strcmp(n,"passive_critical_strike"))c->cs+=v;
    else if(strstr(n,"mastery") && strstr(n,"crit"))c->mastery_cs+=v;
    else if(strstr(n,"mastery") && strstr(n,"dmg"))c->ed+=v;
    else if(strstr(n,"mastery") && strstr(n,"_th"))c->ar_pct+=v;
    else if(!strcmp(n,"bonearmor")) {
        if(!strcmp(s->aurastate,"cyclonearmor"))c->cyclone_pool+=v/256;else c->bone_pool+=v/256;
    }
    else if(!strcmp(n,"cyclonearmor"))c->cyclone_pool+=v/256;
    else if(!strcmp(n,"psychicward"))c->ward_pool+=v/256;
    else if(!strcmp(n,"poisonlength") || !strcmp(n,"skill_poison_override_length"))c->poison_frames=sign>0?v:0;
    else if(strstr(n,"mindam") || strstr(n,"maxdam")) {
        const char *prefix[]={"", "fire", "light", "cold", "poison", "magic"};
        for(int k=1;k<DM_EL_N;k++)if(!strncmp(n,prefix[k],strlen(prefix[k]))) {
            int value=k==DM_EL_POIS?(int)((int64_t)v*c->poison_frames/256):v;
            for(int h=0;h<c->nhands;h++)if(strstr(n,"mindam"))c->hand[h].elmin[k]+=value;else c->hand[h].elmax[k]+=value;
        }
    }
    else for(int k=DM_ST_MAST_FIRE;k<=DM_ST_PIERCE_PHYS;k++) if(d->stat[k]==id) {
        if(k<=DM_ST_MAST_MAG)c->mastery[k-DM_ST_MAST_FIRE+1]+=v;
        else c->pierce[k==DM_ST_PIERCE_PHYS?0:k-DM_ST_PIERCE_FIRE+1]+=v;
    }
}
int dm_sim_stat_modelled(const dm_sim_data *d,int id) {
    if(!d || id<0 || id>=512 || !d->stat_name[id][0])return 0;
    const char *n=d->stat_name[id];
    static const char *const known[]={"damageresist","fireresist","lightresist","coldresist","poisonresist","magicresist","damagepercent","skill_damagepercent",
        "attackrate","item_tohit_percent","armorclass","tohit","skill_armor_percent","item_armor_percent","item_maxhp_percent","item_maxmana_percent","hpregen",
        "manarecoverybonus","item_manaregen","item_allskills","passive_dodge","passive_avoid","passive_weaponblock","toblock","passive_critical_strike",
        "maxfireresist","maxlightresist","maxcoldresist","maxpoisonresist","firemindam","firemaxdam","lightmindam","lightmaxdam","coldmindam","coldmaxdam",
        "poisonmindam","poisonmaxdam","magicmindam","magicmaxdam","bonearmor","cyclonearmor","psychicward","poisonlength","skill_poison_override_length"};
    for(size_t i=0;i<sizeof known/sizeof *known;i++)if(!strcmp(n,known[i]))return 1;
    if(strstr(n,"mastery") && (strstr(n,"crit") || strstr(n,"dmg") || strstr(n,"_th")))return 1;
    for(int k=DM_ST_MAST_FIRE;k<=DM_ST_PIERCE_PHYS;k++)if(d->stat[k]==id)return 1;
    return 0;
}
void sim_modify(const dm_sim_data *d,dm_sim_char *c,dm_sim_tmod *m,int id,int sign,int *bad) {
    if(id<0 || id>=DM_SIM_MAXSKILL || !d->skill[id] || !c->lvl[id])return;
    const dm_sim_skill *s=d->skill[id];
    int target=(s->fam&DM_FAM_K)!=0 || !strcmp(s->aurastate,"conviction");
    int vals[20];
    for(int i=0;i<6;i++) vals[i]=(int)sim_calc(d,c,id,DM_CF_AURASTAT1+i,bad)*sign;
    for(int i=0;i<14;i++) vals[6+i]=(int)sim_calc(d,c,id,DM_CF_PASSIVE1+i,bad)*sign;
    for(int i=0;i<6;i++)for(int j=0;j<14;j++)if(s->astat[i]>=0 && s->astat[i]==s->pstat[j])vals[i]=0;
    for(int i=0;i<6;i++)if(s->astat[i]>=0 && !strcmp(d->stat_name[s->astat[i]],"skill_poison_override_length"))apply_stat(d,c,m,s,s->astat[i],vals[i],target,sign);
    for(int i=0;i<6;i++)if(s->astat[i]<0 || strcmp(d->stat_name[s->astat[i]],"skill_poison_override_length"))apply_stat(d,c,m,s,s->astat[i],vals[i],target,sign);
    for(int i=0;i<14;i++)apply_stat(d,c,m,s,s->pstat[i],vals[6+i],target,sign);
    if(id==DM_SK_CURSE_LIFESTEAL)m->lifetap+=sign*(int)sim_calc(d,c,id,0,bad);
    if(id==DM_SK_SHIELD_BUFF) {
        c->shield_min+=sign*(int)dm_sim_skill_value(d,c,id,c->lvl[id],DM_SV_PMIN,bad);
        c->shield_max+=sign*(int)dm_sim_skill_value(d,c,id,c->lvl[id],DM_SV_PMAX,bad);
        int bonus=(int)sim_calc(d,c,id,0,bad);c->def=(int)(sign>0?c->def*(100.0+bonus)/100:c->def*100.0/fmax(1,100+bonus));
    }
    if(!strcmp(s->name,"Wearwolf"))c->form=sign>0?1:0;
    if(!strcmp(s->name,"Wearbear"))c->form=sign>0?2:0;
    if(!strcmp(s->aurastate,"energyshield")) { c->es_pct=sign>0?(int)sim_calc(d,c,id,0,bad):0; c->es_ratio=(int)sim_calc(d,c,id,1,bad); }
}
static double missile_calc(const dm_sim_data *d,const dm_sim_char *c,const dm_sim_skill *s,const dm_sim_miss *miss,int slot_index,int *bad) {
    calc_ctx x={d,c,-1,0};dm_sim_calc_env e={calc_var,calc_stat,s->id,0,&x};
    return dm_sim_calc_eval(d,miss->calc[slot_index],&e,bad);
}
static int ground_support(const dm_sim_data *d,const dm_sim_char *c,const dm_sim_skill *s,const dm_sim_miss *miss,dm_sim_roll *roll) {
    if(miss->etype!=DM_EL_FIRE || miss->srcdamage || miss->srcmissdamage || miss->skill>=0 || miss->missile_skill ||
       miss->srvdofunc!=5 || miss->vel || miss->accel || miss->maxvel || miss->initsteps || miss->hitshift<0 || miss->hitshift>16)return -1;
    int bad=0,ends[2];
    int64_t synergy=(int64_t)missile_calc(d,c,s,miss,DM_MF_EDMGSYM,&bad);
    int mastery=miss->apply_mastery?c->mastery[miss->etype]:0;
    if(bad || synergy<0 || synergy>65536 || mastery<0 || mastery>65536)return -1;
    for(int k=0;k<2;k++) {
        int64_t base=dm_sim_level_value(k?miss->emax:miss->emin,k?miss->emaxlev:miss->eminlev,c->lvl[s->id]);
        if(base<1 || base>1048576 || base*synergy>INT32_MAX)return -1;
        int64_t raw=(base+base*synergy/100)*(1<<miss->hitshift);
        if(raw>1048576 || raw*mastery>INT32_MAX)return -1;
        raw+=raw*mastery/100;
        if(raw<1 || raw>0x80000)return -1;
        ends[k]=(int)raw;
    }
    *roll=(dm_sim_roll){1,ends[0]<ends[1]?ends[0]:ends[1],ends[0]>ends[1]?ends[0]:ends[1],0,miss->etype,1,0};
    return 0;
}
static double role(const dm_sim_data *d,const dm_sim_char *c,const dm_sim_skill *s,int r,int *bad) {
    return s->role[r]<0?0:sim_calc(d,c,s->id,s->role[r],bad);
}

static int attack_rate(const dm_sim_char *c,int skill_sias,int speed,int hand) {
    const dm_sim_weapon *w=&c->hand[c->nhands==2 && hand==1?1:0];
    int ias=sim_clamp(c->ias+w->ias,0,10000);
    double effective=120*ias/(120+ias)-w->wsm;
    if(c->nhands==2 && hand<0) {
        ias=sim_clamp(c->ias+c->hand[1].ias,0,10000);
        effective=(effective+120*ias/(120+ias)-c->hand[1].wsm)/2;
    }
    int inc=sim_clamp(100+(int)(effective+c->sias+skill_sias),15,c->form?250:175);
    return speed*inc/100;
}

static int animation(const dm_sim_data *d,const dm_sim_char *c,const dm_sim_skill *s,int sias,int *flags,int *action,int *length,int *rate_out,int *base_action,int *start_out,
                     int hand,const char *mode_hand) {
    int weapon=((s->fam&DM_FAM_WEAPON)!=0 || s->useattackrate) && strcmp(s->anim,"SC");
    int seq=!strcmp(s->anim,"SQ"), len=16, speed=256;
    const char *token=c->form==1?"40":c->form==2?"TG":d->cls[c->cls].token;
    int found=0;
    const char *mode=mode_hand?mode_hand:s->anim; const char *wc=c->hand[0].wclass[0]?c->hand[0].wclass:"HTH";
    if(seq) { *flags|=DM_SF_SEQ|DM_SF_ESTIMATE|DM_SF_LENGTH; mode="A1"; }
    *action=-1;
    for(int i=0;i<d->nanim;i++) {
        const dm_sim_anim *a=&d->anim[i];
        if(!strcmp(a->token,token) && !strcmp(a->mode,mode) && !strcmp(a->wclass,c->form?"HTH":wc)) {
            len=a->frames;speed=a->speed;*action=a->action;found=1;break;
        }
    }
    if(!found)*flags|=DM_SF_ESTIMATE|DM_SF_LENGTH;
    int human_length=0,human_speed=0;
    *start_out=0;
    if(c->form && weapon) {
        *flags|=DM_SF_ESTIMATE|DM_SF_LENGTH;
        for(int i=0;i<d->nanim;i++) {
            const dm_sim_anim *a=&d->anim[i];
            if(!strcmp(a->token,d->cls[c->cls].token) && !strcmp(a->mode,mode) && !strcmp(a->wclass,wc)) {
                human_length=a->frames;human_speed=a->speed;break;
            }
        }
    }
    if(seq && s->seqnum==12) {len=19;speed=256;weapon=0;*action=10;}
    else if(seq && s->seqnum==1) {len=!strcmp(wc,"1HT") || !strcmp(wc,"2HT")?18:21;*action=6;sias-=30;}
    else if(seq && s->seqnum==11) {len=17;*action=6;sias-=30;}
    else if(seq) *flags|=DM_SF_ESTIMATE|DM_SF_LENGTH;
    if(!seq && !c->form && (!strcmp(mode,"A1") || !strcmp(mode,"A2"))) {
        const dm_sim_weapon *w=&c->hand[0];
        if(w->start_known && w->start_frame>=0 && w->start_frame<len) {
            *start_out=w->start_frame;len-=w->start_frame;
            if(*action>=0)*action=*action<w->start_frame?-1:*action-w->start_frame;
        } else { *action=-1;*flags|=DM_SF_ESTIMATE|DM_SF_LENGTH; }
    }
    if(s->srvdofunc==50)sias-=40;
    if(s->srvdofunc==9) {
        int bad=0;sias+=(int)sim_calc(d,c,s->id,DM_CF_AURASTAT1+1,&bad);
        *flags|=DM_SF_ESTIMATE|DM_SF_LENGTH;
    }
    int rate=attack_rate(c,sias,speed,hand);
    if(human_length>0 && human_speed>0) {
        int human_rate=attack_rate(c,sias,human_speed,hand);
        rate=human_rate*len/human_length;
        if(rate>0 && 256*human_length/human_rate<256*len/rate)rate++;
    }
    *length=len;*rate_out=rate;*base_action=*action;
    int frames=weapon?sim_clamp((256*len+rate-1)/rate-!seq,1,10000):dm_sim_frames_cast(len,speed,c->fcr,seq);
    if(!weapon)rate=speed*(100+sim_clamp(120*c->fcr/(120+c->fcr),0,75))/100;
    if(*action<0)*flags|=DM_SF_ESTIMATE;
    else *action=sim_clamp((int)ceil(fmax(1,*action)*256/rate),1,frames);
    if(s->localdelay || s->globaldelay)frames=frames<d->cls[c->cls].mincast?d->cls[c->cls].mincast:frames;
    return frames;
}
int sim_normal_physical(const dm_sim_char *c,int out[2]) {
    if(!c || !out || c->nhands!=1)return -1;
    const dm_sim_weapon *w=&c->hand[0];
    int64_t pct=c->ed+(int64_t)c->str*w->strbonus/100+(int64_t)c->dex*w->dexbonus/100;
    if(pct<-90)pct=-90;
    int64_t lo=(w->min>1?w->min:1)*(100+pct+w->edmin)/100+w->postflat;
    int64_t hi=(w->max>2?w->max:2)*(100+pct+w->edmax)/100+w->postflat;
    if(lo<INT32_MIN || lo>INT32_MAX || hi<INT32_MIN || hi>INT32_MAX)return -1;
    out[0]=(int)lo;out[1]=(int)hi;return 0;
}
static int description_scale(int64_t value,int64_t percent,int64_t *out) {
    if(value<=-32768 || value>=32768 || percent<=-32768 || percent>=32768)return -1;
    *out=value+value*percent/100;
    return *out<INT32_MIN || *out>INT32_MAX?-1:0;
}
int dm_sim_skill_endpoints(const dm_sim_data *d,const dm_sim_char *c,int id,int out[2]) {
    if(!d || !c || !out || id<0 || id>=DM_SIM_MAXSKILL || !d->skill[id])return -1;
    const dm_sim_skill *s=d->skill[id];
    int type=s->descdam;
    if(type!=5 && type!=7 && type!=8 && type!=9 && type!=18 && type!=24 && type!=25)return -1;
    int bad=0;int64_t total[2]={0};
    if(s->srcdam && type!=8) {
        if(c->nhands!=1 || c->endpoint_bad || c->endpoint_state_bad[id] || c->charges)return -1;
        const dm_sim_weapon *w=&c->hand[0];
        int64_t extra=type==7 || type==18?(int64_t)sim_calc(d,c,id,DM_CF_DDAM1,&bad):0;
        int64_t flat=type==7 || type==18?(int64_t)sim_calc(d,c,id,DM_CF_DDAM2,&bad):0;
        if(c->str<0 || c->str>=32768 || c->dex<0 || c->dex>=32768 ||
           w->strbonus<0 || w->strbonus>=32768 || w->dexbonus<0 || w->dexbonus>=32768)return -1;
        int64_t percent=c->ed+extra+c->damage_mastery_delta[id]+(int64_t)c->str*w->strbonus/100+(int64_t)c->dex*w->dexbonus/100;
        if(percent<-90)percent=-90;
        for(int k=0;k<2;k++) {
            int64_t base=k?w->max:w->min;if(base<k+1)base=k+1;
            int64_t endpoint_percent=percent+(k?w->edmax:w->edmin);
            if(base>=32768 || endpoint_percent<=-32768 || endpoint_percent>=32768)return -1;
            total[k]=base*(100+endpoint_percent)/100;
            total[k]+=flat+w->postflat;
        }
        for(int e=1;e<DM_EL_N;e++)if(e!=DM_EL_POIS) {
            int64_t lo,hi;int mastery=e==DM_EL_MAG?0:c->mastery[e];
            if(description_scale(w->elmin[e],mastery,&lo) || description_scale(w->elmax[e],mastery,&hi))return -1;
            total[0]+=lo<hi?lo:hi;total[1]+=hi;
        }
        if(!c->stats_known && (w->elmin[DM_EL_POIS] || w->elmax[DM_EL_POIS]))return -1;
        if(c->stats_known && c->stat_value[58]) {
            if(c->poison_frames<0 || c->poison_frames>=32768)return -1;
            for(int k=0;k<2;k++) {
                int64_t rate;
                if(description_scale(c->stat_value[57+k],c->mastery[DM_EL_POIS],&rate))return -1;
                if(rate*c->poison_frames<INT32_MIN || rate*c->poison_frames>INT32_MAX)return -1;
                total[k]+=(rate*c->poison_frames)>>8;
            }
        }
        if(total[1]>=1){if(total[0]<1)total[0]=1;if(total[1]<=total[0])total[1]=total[0]+1;}
        for(int k=0;k<2;k++) {
            if(total[k]<=-0x100000 || total[k]>=0x100000)return -1;
            total[k]=total[k]*s->srcdam/128;
        }
    }
    int64_t length=1;
    if((type==5 || type==24 || type==25) && s->etype==DM_EL_POIS)length=(int64_t)dm_sim_skill_value(d,c,id,c->lvl[id],DM_SV_ELEN,&bad);
    if(length<0 || length>INT32_MAX)return -1;
    for(int k=0;k<2;k++) {
        int64_t physical=dm_sim_skill_raw(d,c,id,c->lvl[id],DM_SV_PMIN+k,&bad);
        int64_t elemental=dm_sim_skill_raw(d,c,id,c->lvl[id],DM_SV_EMIN+k,&bad);
        if(physical<0 || physical>INT32_MAX || elemental<0 || elemental>INT32_MAX)return -1;
        if(elemental*length>INT32_MAX)return -1;
        if(type==8) {
            int64_t a=(int64_t)sim_calc(d,c,id,DM_CF_DDAM1,&bad),b=(int64_t)sim_calc(d,c,id,DM_CF_DDAM2,&bad);
            if(!a)a=1;
            if(!b)b=1;
            if(a<INT32_MIN/12 || a>INT32_MAX/12 || elemental*12*a<INT32_MIN || elemental*12*a>INT32_MAX || b<INT32_MIN || b>INT32_MAX)return -1;
            total[k]=(elemental*12*a/b)>>8;
        } else if(type==9) {
            if(s->hitshift<0 || s->hitshift>30)return -1;
            int64_t base=(int64_t)(k?s->maxdam:s->mindam)*(1LL<<s->hitshift);
            if(base<0 || base+elemental>INT32_MAX/75)return -1;
            total[k]+=((base+elemental)*75)>>8;
        } else total[k]+=(physical>>8)+(elemental*length>>8);
        if(type==25) {
            int64_t pct=(int64_t)sim_calc(d,c,id,DM_CF_DDAM1+k,&bad);
            if(pct<INT32_MIN || pct>INT32_MAX || total[k]<INT32_MIN || total[k]>INT32_MAX)return -1;
            float scaled=(float)total[k]*(((float)pct+100.0f)/100.0f);
            if(!isfinite(scaled) || (double)scaled<INT32_MIN || (double)scaled>=INT32_MAX)return -1;
            total[k]=(int)scaled;
        }
        if(total[k]<INT32_MIN || total[k]>=INT32_MAX)return -1;
    }
    if(total[1]<=total[0])total[1]=total[0]+1;
    if(bad)return -1;
    out[0]=(int)total[0];out[1]=(int)total[1];return 0;
}
static int hand_attack_rating(const dm_sim_data *d,const dm_sim_char *c,int id,const dm_sim_weapon *w,int *bad) {
    const dm_sim_skill *s=d->skill[id];
    int64_t bonus=s->calc[DM_CF_TOHIT]>=0?(int64_t)sim_calc(d,c,id,DM_CF_TOHIT,bad):s->tohit+s->levtohit*(c->lvl[id]-1);
    int base=(c->ar_base_known?c->ar_base:c->ar)+w->ar_base_d;
    int64_t pct=c->ar_pct+bonus+(c->ar_base_known?c->ar_bonus[id]:0)+w->ar_pct_d;
    int64_t result=base+base*pct/100;
    if(result<INT32_MIN || result>INT32_MAX){*bad=1;return 0;}
    return (int)result;
}
int sim_attack_rating(const dm_sim_data *d,const dm_sim_char *c,int id,int *bad){return hand_attack_rating(d,c,id,&c->hand[0],bad);}

static int skill_hands(const dm_sim_char *c,const dm_sim_skill *s,int *either) {
    *either=0;
    if(c->nhands!=2)return 1;
    switch(s->weapsel) {
    case 0: if(s->id==0){*either=1;return 3;} return 1;
    case 1: return 2;
    case 2: case 3: return 3;
    default: return 1;
    }
}
static void own_roll(const dm_sim_data *d,const dm_sim_char *c,const dm_sim_target *t,const dm_sim_skill *s,dm_sim_skill_out *o,const int *resistance) {
    int element=s->etype,missile=s->missile>=0?s->missile:s->descmissile;
    if(s->fam!=DM_FAM_Z1 || s->srcdam || s->timed || s->frameburn || s->burnfactor || (s->hitfactor && s->hitfactor!=100) ||
       s->role[DM_ROLE_NMISS]>=0 || s->role[DM_ROLE_HITS]>=0 || s->role[DM_ROLE_ATTACKS]>=0 || s->role[DM_ROLE_JUMPS]>=0 ||
       element<=0 || element>=DM_EL_N || element==DM_EL_POIS || o->hits!=1 ||
       (o->flags&~(DM_SF_ESTIMATE|DM_SF_LENGTH|DM_SF_IMMUNE)) || o->dot || o->burn || o->pet_dps ||
       c->mercenary || c->capture_bad || t->kind==DM_TK_STD || c->mastery[element]<0 ||
       ((t->flags&DM_MF_PRIME) && d->diff[c->diff].pvspe!=100) ||
       missile<0 || missile>=DM_SIM_MAXMISS || !d->miss[missile] || d->miss[missile]->typefilter)return;
    const dm_sim_miss *projectile=d->miss[missile];
    if(projectile->nexthit || projectile->nextdelay)return;
    for(int k=0;k<3;k++)if(projectile->sub[k]>=0)return;
    for(int k=0;k<4;k++)if(projectile->hitsub[k]>=0)return;
    for(int e=0;e<DM_EL_N;e++)if(e!=element && o->raw[e])return;
    int bad=0,level=c->lvl[s->id];
    if(dm_sim_skill_raw(d,c,s->id,level,DM_SV_PMIN,&bad) || dm_sim_skill_raw(d,c,s->id,level,DM_SV_PMAX,&bad))return;
    int64_t low=dm_sim_skill_raw(d,c,s->id,level,DM_SV_EMIN,&bad),high=dm_sim_skill_raw(d,c,s->id,level,DM_SV_EMAX,&bad);
    if(bad || low<1 || high<low || high>0x80000)return;
    o->roll=(dm_sim_roll){1,(int)low,(int)high,sim_clamp(resistance[element],-100,100),element,o->cth,0};
    double raw,hit;
    if(sim_roll_mean(&o->roll,&raw,&hit)){o->roll.known=0;return;}
    o->raw[element]=raw;o->hit[element]=hit;
    if(projectile->explosion>=0)o->roll.known=0;
}
static void normal_roll(const dm_sim_data *d,const dm_sim_char *c,const dm_sim_target *t,const dm_sim_skill *s,dm_sim_skill_out *o,int resistance) {
    const dm_sim_weapon *w=&c->hand[0];
    if(o->skill || s->fam!=DM_FAM_W1 || s->srcdam!=128 || s->mindam || s->maxdam ||
       c->nhands!=1 || c->form || c->charges || c->mercenary || c->endpoint_bad || c->endpoint_state_bad[0] || c->capture_bad || c->roll_inputs_bad ||
       w->kind==DM_WK_BOW || w->kind==DM_WK_XBOW || w->kind==DM_WK_THROW || w->postflat ||
       (c->stats_known && c->ed!=c->stat_value[25]) || o->hits!=1 || (o->flags&~(DM_SF_ESTIMATE|DM_SF_LENGTH|DM_SF_IMMUNE)) ||
       ((t->flags&DM_MF_PRIME) && d->diff[c->diff].pvspe!=100) || t->kind==DM_TK_STD ||
       (t->demon_share!=0 && t->demon_share!=100) || t->undead_share || c->dmg_demon || c->dmg_undead)return;
    if(c->stats_known && w->kind==DM_WK_NONE && (c->stat_value[21] || c->stat_value[22]))return;
    for(int e=1;e<DM_EL_N;e++)if(o->raw[e] || w->elmin[e] || w->elmax[e])return;
    int64_t low=(int64_t)w->min*256,high=(int64_t)w->max*256;
    if(low<256)low=256;
    if(high<=low)high=low+256;
    int64_t strength=(int64_t)c->str*w->strbonus,dexterity=(int64_t)c->dex*w->dexbonus;
    if(strength<INT32_MIN || strength>INT32_MAX || dexterity<INT32_MIN || dexterity>INT32_MAX)return;
    int64_t percent=c->ed+strength/100+dexterity/100;
    if(percent<INT32_MIN || percent>INT32_MAX)return;
    if(percent < -90)percent=-90;
    if(low>0x100000 || high>0x100000 || percent+w->edmin>0x10000 || percent+w->edmax>0x10000)return;
    if(low*(percent+w->edmin)<INT32_MIN || low*(percent+w->edmin)>INT32_MAX ||
       high*(percent+w->edmax)<INT32_MIN || high*(percent+w->edmax)>INT32_MAX)return;
    low+=low*(percent+w->edmin)/100;high+=high*(percent+w->edmax)/100;
    if(low<0 || high<low || high>0x80000)return;
    double critical=1-(1-sim_clamp(c->ds,0,100)/100.0)*(1-sim_clamp(c->cs,0,100)/100.0)*(1-sim_clamp(c->mastery_cs,0,100)/100.0);
    o->roll=(dm_sim_roll){1,(int)low,(int)high,sim_clamp(resistance,-100,100),DM_EL_PHYS,o->cth,critical};
    double raw,hit;
    if(sim_roll_mean(&o->roll,&raw,&hit)){o->roll.known=0;return;}
    o->raw[0]=raw;o->hit[0]=hit;
}
void dm_sim_dual_poison(dm_sim_char *c,const int frames[2]) {
    int longest=0;
    for(int h=0;h<2;h++) {
        dm_sim_weapon *w=&c->hand[h];int length=frames[h]>0?frames[h]:0;
        w->elmin[DM_EL_POIS]=(int)((int64_t)w->elmin[DM_EL_POIS]*length/256);w->elmax[DM_EL_POIS]=(int)((int64_t)w->elmax[DM_EL_POIS]*length/256);
        if((w->elmin[DM_EL_POIS] || w->elmax[DM_EL_POIS]) && length>longest)longest=length;
    }
    c->poison_frames=longest?longest:frames[0]>0?frames[0]:0;
}



int dm_splash_skill_ok(const dm_sim_skill *s) {
    if(!s) return 0;
    if(dm_splash_skill_excluded(s->kick,s->srvdofunc)) return 0;
    if(s->id==DM_SK_ECHOING_STRIKE || s->id==DM_SK_BLADE_WARP) return 1;
    if(!(s->fam & (DM_FAM_W1 | DM_FAM_W2 | DM_FAM_W4 | DM_FAM_W5))) return 0;
    return !strcmp(s->range,"h2h") || !strcmp(s->range,"both") || !strcmp(s->range,"none");
}
int dm_splash_char_ok(const dm_sim_char *c, const dm_sim_skill *s) {
    if(!c || !dm_splash_skill_ok(s)) return 0;
    const char *w=c->hand[0].wclass;
    int shoots=!c->form && (!strcmp(w,"BOW") || !strcmp(w,"XBW"));
    return !(shoots && (s->id==0 || !strcmp(s->range,"both")));
}
double dm_splash_hand_reach(const dm_sim_char *c,const dm_sim_weapon *w,int extra) {
    dm_splash_group g=dm_splash_group_of(w->stype[0]?w->stype:NULL,w->twohand);
    if(g==DM_SPLASH_UNKNOWN)return 1;
    dm_splash_area a=dm_splash_area_of(g,c->cls,c->str,c->dex,w->srange);
    return dm_splash_reach(&a,extra);
}
int sim_action(const dm_sim_data *d,const dm_sim_char *c,const dm_sim_target *t,const dm_sim_tmod *mod,int id,dm_sim_skill_out *o) {
    memset(o,0,sizeof *o); o->skill=id;o->ttk=-1;o->hits=1;o->cth=1;o->ll=c->ll;o->ml=c->ml;o->pmh=c->prevent_heal;
    if(!d || !d->ok || id<0 || id>=DM_SIM_MAXSKILL || !d->skill[id] || c->cls<0 || c->cls>=d->ncls || c->players!=1 || c->diff<0 || c->diff>2 || t->extra_targets<0 || t->extra_targets>1023) {o->flags=DM_SF_UNAVAIL;return -1;}
    const dm_sim_skill *s=d->skill[id]; int lv=c->lvl[id],bad=0;
    if((c->capture_bad&4) && (s->fam&DM_FAM_W5 || id==DM_SK_SMITE)){o->flags|=DM_SF_UNAVAIL;return 0;}
    if((c->capture_bad&1) && (s->fam&DM_FAM_WEAPON || s->srcdam>0)){o->flags=DM_SF_UNAVAIL;return 0;}
    if(s->unavailable){o->flags=DM_SF_UNAVAIL|DM_SF_ESTIMATE;return 0;}
    if(lv<=0 && id) {o->flags=DM_SF_UNAVAIL;return -1;}
    int animation_length=0,animation_rate=256,animation_action=1,animation_start=0;
    int either,hands_used=skill_hands(c,s,&either),lone=hands_used==2?1:0;
    int skill_sias=(int)role(d,c,s,DM_ROLE_SIAS,&bad);
    if(either) {


        int f2=0,a2=0,l2=0,r2=0,b2=0,s2=0;
        o->frames=animation(d,c,s,skill_sias,&o->flags,&o->action_frame,&animation_length,&animation_rate,&animation_action,&animation_start,0,"A1");
        f2=animation(d,c,s,skill_sias,&o->flags,&a2,&l2,&r2,&b2,&s2,1,"A2");
        o->frames=(o->frames+f2+1)/2;
        if(o->action_frame>=0 && a2>=0)o->action_frame=(o->action_frame+a2+1)/2;else o->action_frame=-1;
        o->flags|=DM_SF_ESTIMATE|DM_SF_LENGTH;
    } else o->frames=animation(d,c,s,skill_sias,&o->flags,&o->action_frame,&animation_length,&animation_rate,&animation_action,&animation_start,
                               c->nhands==2 && hands_used!=3?lone:-1,NULL);
    o->mana_x256=dm_sim_mana_cost(s,lv);
    o->dur=(int)sim_calc(d,c,id,DM_CF_AURALEN,&bad);
    if(!o->dur)o->dur=(int)role(d,c,s,DM_ROLE_DUR,&bad);
    o->cooldown=s->localdelay>s->globaldelay?s->localdelay:s->globaldelay;
    if(o->cooldown<o->frames)o->cooldown=o->frames;
    if(s->fam&DM_FAM_U){o->flags|=DM_SF_UTILITY;return 0;}
    if(s->fam&DM_FAM_MODIFIER && !(s->fam&(DM_FAM_WEAPON|DM_FAM_SPELL|DM_FAM_ADMG|DM_FAM_P|DM_FAM_F))) {
        o->flags|=DM_SF_MODIFIER; if(bad)o->flags|=DM_SF_UNAVAIL;return 0;
    }
    if(s->fam&DM_FAM_L) {
        if(s->srvdofunc==20) {o->sf_frac=fmax(0,sim_calc(d,c,id,0,&bad))/100;o->sf_floor=d->diff[c->diff].sfmin/100.0;o->flags|=DM_SF_PCTLIFE;}
        else {o->flags|=DM_SF_CORPSE|DM_SF_NOTARGET; if(!(s->fam&DM_FAM_F))return 0;}
    }
    double phys=(dm_sim_skill_value(d,c,id,lv,DM_SV_PMIN,&bad)+dm_sim_skill_value(d,c,id,lv,DM_SV_PMAX,&bad))/2;
    double kickbase=s->kick?phys:0;
    if(s->kick)phys=0;
    double el=(dm_sim_skill_value(d,c,id,lv,DM_SV_EMIN,&bad)+dm_sim_skill_value(d,c,id,lv,DM_SV_EMAX,&bad))/2;
    o->dot_frames=(int)dm_sim_skill_value(d,c,id,lv,DM_SV_ELEN,&bad);
    double dmgpct=s->fam&DM_FAM_W4?0:role(d,c,s,DM_ROLE_DMGPCT,&bad), flat=role(d,c,s,DM_ROLE_FLAT,&bad);

    int splashing=t->extra_targets>0 && dm_splash_char_ok(c,s);
    double hand_raw[2][DM_EL_N]={{0}},hand_base[2]={0,0},swing[2]={0,0},own_phys=phys;
    if(s->fam&DM_FAM_WEAPON || (s->timed && s->srcdam>0)) {
        int smite=s->srvdofunc==150;
        if(s->requiresweapon && c->hand[0].kind==DM_WK_NONE) {o->flags|=DM_SF_NOWEAPON;return 0;}
        if(smite && !c->shield){o->flags|=DM_SF_NOWEAPON;return 0;}
        int def=(int)(t->ac*fmax(0,100+mod->def_pct)/100.0);

        double hand_cth[2]={0,0};int nused=0;
        for(int h=0;h<2;h++)if(hands_used>>h&1) {
            int ar=hand_attack_rating(d,c,id,&c->hand[h],&bad);
            hand_cth[h]=s->alwayshit || smite?1:dm_sim_cth(ar,def,c->clvl,t->mlvl)/100.0*(1-t->block/100.0);
            if(!strcmp(s->anim,"SC"))hand_cth[h]=1;
            nused++;
        }
        o->cth=(hand_cth[0]+hand_cth[1])/nused;
        int hands=nused;

        double base=0,wp=0,cb_mix=0,ow_mix=0,ll_mix=0,ml_mix=0;int pmh=0;
        for(int h=0;h<2;h++) {
            if(!(hands_used>>h&1))continue;
            const dm_sim_weapon *w=&c->hand[h];
            double crit=1-(1-sim_clamp(c->ds+w->ds_d,0,100)/100.0)*(1-sim_clamp(c->cs,0,100)/100.0)*(1-sim_clamp(c->mastery_cs+w->cs_d,0,100)/100.0);
            double share=o->cth>0?hand_cth[h]/o->cth:1;
            double b=(w->min+w->max)/2.0, attrs=(c->str*w->strbonus+c->dex*w->dexbonus)/100.0;
            if(smite){b=(c->shield_min+c->shield_max)/2.0;attrs=c->str;crit=0;}
            if(s->fam&DM_FAM_W5){b=kickbase+(c->kick_min+c->kick_max)/2.0;attrs=c->str*1.2;crit=0;}
            double src=smite || s->fam&DM_FAM_W5?1:s->srcdam/128.0;
            double ed=c->ed+w->ed_d+dmgpct+attrs+(c->dmg_demon+w->demon_d)*t->demon_share/100.0+(c->dmg_undead+w->undead_d)*t->undead_share/100.0;
            base+=b/hands;hand_base[h]=b/hands;
            double weapon=(b+flat)*fmax(0,100+ed)/100;
            if(!smite && !(s->fam&DM_FAM_W5))weapon+=(w->min*w->edmin+w->max*w->edmax)/200.0+w->postflat;
            double part=weapon*(1+crit)*src/hands*share;
            wp+=part;hand_raw[h][DM_EL_PHYS]=part;swing[h]=share/hands;
            cb_mix+=sim_clamp(c->cb+w->cb_d,0,100)*share/hands;ow_mix+=sim_clamp(c->ow+w->ow_d,0,100)*share/hands;
            ll_mix+=part*(c->ll+w->ll_d);ml_mix+=part*(c->ml+w->ml_d);pmh|=c->prevent_heal+w->pmh_d>0;
            if(!smite)for(int e=1;e<DM_EL_N;e++){double x=(w->elmin[e]+w->elmax[e])/2.0*src/hands*share/(e==DM_EL_POIS?fmax(1,c->poison_frames):1);o->raw[e]+=x;hand_raw[h][e]=x;}
        }
        if(nused==2){o->ll=wp>0?ll_mix/wp:0;o->ml=wp>0?ml_mix/wp:0;o->pmh=pmh;}
        else if(c->nhands==2){const dm_sim_weapon *w=&c->hand[hands_used>>1&1];o->ll=c->ll+w->ll_d;o->ml=c->ml+w->ml_d;o->pmh=c->prevent_heal+w->pmh_d>0;}
        double conv=sim_clamp((int)role(d,c,s,DM_ROLE_ELPCT,&bad),0,100)/100.0;
        if(s->role[DM_ROLE_ELPCT]<0 && s->missile>=0 && s->missile<DM_SIM_MAXMISS && d->miss[s->missile] && d->miss[s->missile]->calc[DM_MF_DMG]>=0)
            conv=sim_clamp((int)missile_calc(d,c,s,d->miss[s->missile],DM_MF_DMG,&bad),0,100)/100.0;
        if(s->etype>0 && s->etype<DM_EL_N) {
            o->raw[s->etype]+=wp*conv;wp*=1-conv;
            for(int h=0;h<2;h++){hand_raw[h][s->etype]+=hand_raw[h][DM_EL_PHYS]*conv;hand_raw[h][DM_EL_PHYS]*=1-conv;}
        }
        static const int added[3]={DM_EL_FIRE,DM_EL_COLD,DM_EL_LTNG},added_role[3]={DM_ROLE_FIREPCT,DM_ROLE_COLDPCT,DM_ROLE_LTNGPCT};
        for(int k=0;k<3;k++) {
            double pct=role(d,c,s,added_role[k],&bad);
            o->raw[added[k]]+=base*pct/100;
            for(int h=0;h<2;h++)hand_raw[h][added[k]]+=hand_base[h]*pct/100;
        }
        phys+=wp;
        double count=role(d,c,s,DM_ROLE_HITS,&bad); if(count<1)count=role(d,c,s,DM_ROLE_ATTACKS,&bad);
        if(count<1)count=s->seqnum==1?3:s->seqnum==11?2:1;
        if(s->srvdofunc==12)count=fmax(2,sim_calc(d,c,id,2,&bad));
        count=fmin(count,32);
        int cycle=s->srvdofunc==13 && (s->srvstfunc==9 || s->srvstfunc==37);
        int attacks=(int)count;
        if(cycle) {
            int maximum=(int)sim_calc(d,c,id,0,&bad),eligible=t->extra_targets+1;
            if(maximum<1 || maximum>32) {o->flags|=DM_SF_UNAVAIL;return 0;}
            attacks=s->srvstfunc==9 && eligible<maximum?eligible:maximum;
            count=(attacks+eligible-1)/eligible;
            o->flags|=DM_SF_ESTIMATE;
        }
        o->hits=count;o->burst_attacks=cycle?attacks:0;
        if(s->fam&(DM_FAM_W2|DM_FAM_W4|DM_FAM_W5))o->flags|=DM_SF_ESTIMATE;
        if(s->rollback_param>0 && attacks>1) {
            int rollback=sim_clamp(s->param[s->rollback_param-1],0,100);
            if(cycle && animation_action>=0) {
                int restart=(animation_action+animation_start)*(100-rollback)/100;
                int repeat=((animation_action+animation_start-restart)*256+animation_rate-1)/animation_rate;
                int last=((animation_length+animation_start-restart)*256+animation_rate-1)/animation_rate-1;
                o->frames=o->action_frame+(attacks-2)*(repeat<1?1:repeat)+(last<1?1:last);
            } else o->frames=dm_sim_frames_rollback(animation_length,animation_action,animation_rate,rollback,attacks);
        }
        if(s->seqnum==10) {o->frames=fmax(4,floor(animation_action*256.0/animation_rate));o->hits=count=c->nhands==2?2:1;}
        if(s->fam&DM_FAM_W4) {o->frames*=4;o->hits=count=4;o->flags|=DM_SF_ESTIMATE|DM_SF_LENGTH;}
        double after=role(d,c,s,DM_ROLE_AFTER1,&bad);
        double multi=after>0?(s->srvdofunc==166 && !s->param[10]?count*after/100:1+(count-1)*after/100):count;
        phys*=multi;el*=multi;
        for(int e=1;e<DM_EL_N;e++)o->raw[e]*=multi;
        int ranged=(s->fam&DM_FAM_W3)!=0;
        double frac=(t->kind>=DM_TK_SUPERUNIQUE && t->kind<=DM_TK_CLONE)?0.125:0.25;
        frac*=ranged?0.5:1;
        o->cb_frac=1-pow(1-cb_mix/100.0*frac*o->cth*fmax(0,1-fmax(0,t->res[0])/100.0),count);
        double L=c->clvl, rate=L<=15?9*L+31:L<=30?18*L-104:L<=45?27*L-374:L<=60?36*L-779:45*L-1319;
        o->ow_rate=rate/256*(t->kind>=DM_TK_SUPERUNIQUE && t->kind<=DM_TK_CLONE?0.5:1);
        o->ow_chance=1-pow(1-ow_mix/100.0*o->cth,count);
    }
    o->raw[0]+=phys;
    if(s->etype>0 && s->etype<DM_EL_N)o->raw[s->etype]+=el;
    if(s->fam&DM_FAM_Z3) {
        if(s->frameburn && s->etype>0 && s->etype!=DM_EL_POIS) {
            o->burn_frames=o->dur>0?o->dur:50;
            int mi=s->descmissile>=0?s->descmissile:s->missile;
            if(mi>=0 && mi<DM_SIM_MAXMISS && d->miss[mi]) {
                const dm_sim_miss *miss=d->miss[mi];double duration=missile_calc(d,c,s,miss,DM_MF_RANGE,&bad);
                if(duration>0)o->burn_frames=(int)duration;
            }
            if(s->frameburn==2)o->burn_frames=o->frames;
        }
        o->flags|=DM_SF_ESTIMATE;
    }
    if(s->fam&DM_FAM_Z3 || (s->fam&DM_FAM_Z1 && s->role[DM_ROLE_NMISS]>=0)) {
        double count=s->hitfactor>0?s->hitfactor/100.0:1;
        if(s->role[DM_ROLE_NMISS]>=0)count=fmin(count,fmax(1,role(d,c,s,DM_ROLE_NMISS,&bad)));
        o->hits=count;o->flags|=DM_SF_ESTIMATE;
        for(int e=0;e<DM_EL_N;e++)o->raw[e]*=count;
    }
    int source=s->missile>=0?s->missile:s->descmissile;
    if(source>=0 && source<DM_SIM_MAXMISS && d->miss[source] && d->miss[source]->typefilter) {
        const dm_sim_miss *miss=d->miss[source];
        double share=((miss->hittypes&1)?t->undead_share:0)+((miss->hittypes&2)?t->demon_share:0);
        share=fmin(100,share)/100;
        for(int e=0;e<DM_EL_N;e++)o->raw[e]*=share+(1-share)*miss->otherpct/100.0;
    }
    dm_sim_roll ground={0};
    if(s->burnfactor>0 && s->burnmiss>=0 && s->burnmiss<DM_SIM_MAXMISS && d->miss[s->burnmiss]) {
        const dm_sim_miss *miss=d->miss[s->burnmiss];
        if(ground_support(d,c,s,miss,&ground))bad=1;
        int duration=(int)sim_calc(d,c,id,DM_CF_BURNLEN,&bad);
        o->burn_frames=duration>0?duration-1:0;o->burn_element=miss->etype;
        o->flags|=DM_SF_ESTIMATE;
    }
    if(s->fam&DM_FAM_ADMG) {
        int period=(int)sim_calc(d,c,id,DM_CF_PERDELAY,&bad);
        o->frames=o->cooldown=period>0?period:50;o->mana_x256=0;
        if(!sim_sum(o->raw) && s->srvdofunc==65) {
            double flat=0,percent=0;
            for(int j=0;j<6;j++)if(s->astat[j]>=0) {
                const char *name=d->stat_name[s->astat[j]];
                if(!strcmp(name,"item_attackertakesdamage"))flat=sim_calc(d,c,id,DM_CF_AURASTAT1+j,&bad);
                if(!strcmp(name,"thorns_percent"))percent=sim_calc(d,c,id,DM_CF_AURASTAT1+j,&bad);
            }
            double land=dm_sim_cth(t->ar,c->def,t->mlvl,c->clvl)/100.0*(1-sim_clamp(c->block,0,75)/100.0);
            o->raw[0]=(flat+(t->dmin+t->dmax)/2.0*percent/100)*land*o->frames/fmax(1,t->cycle);o->flags|=DM_SF_ESTIMATE;
        }
        if(!sim_sum(o->raw))o->flags|=DM_SF_MODIFIER|DM_SF_ESTIMATE;
    }
    int mage=!strcmp(s->pettype,"skeletonmage");
    double pet_rate=0;
    if(s->fam&(DM_FAM_P|DM_FAM_F)) {
        o->flags|=DM_SF_PET|DM_SF_ESTIMATE;
        if(!mage && s->summon>=0 && s->summon<d->nmon) {
            dm_sim_target pet;
            int pl=(int)role(d,c,s,DM_ROLE_PETLVL,&bad);if(pl<1)pl=c->clvl;
            if(!dm_sim_target_mon(d,s->summon,0,pl,DM_TK_NORMAL,1,&pet)) {
                if(!sim_sum(o->raw))o->raw[0]=(pet.dmin+pet.dmax)/2.0;
                double ed=0,flatpet=0,ar=pet.ar;
                for(int j=0;j<20;j++) {
                    int st=j<6?s->astat[j]:s->pstat[j-6];if(st<0)continue;
                    double v=sim_calc(d,c,id,j<6?DM_CF_AURASTAT1+j:DM_CF_PASSIVE1+j-6,&bad);
                    if(!strcmp(d->stat_name[st],"damagepercent"))ed+=v;
                    if(!strcmp(d->stat_name[st],"item_normaldamage"))flatpet+=v;
                    if(!strcmp(d->stat_name[st],"tohit"))ar+=v;
                }
                o->raw[0]=(o->raw[0]+flatpet)*(100+ed)/100*dm_sim_cth((int)ar,t->ac,pl,t->mlvl)/100.0*(1-t->block/100.0);
            }
            if(s->sumskill[0]>=0 && s->sumskill[0]<DM_SIM_MAXSKILL && d->skill[s->sumskill[0]]) {
                int sub=s->sumskill[0],sl=(int)sim_calc(d,c,id,DM_CF_SUMSK1,&bad);const dm_sim_skill *ss=d->skill[sub];
                if(ss->etype>0 && ss->etype<DM_EL_N) {
                    double val=(dm_sim_skill_value(d,c,sub,sl,DM_SV_EMIN,&bad)+dm_sim_skill_value(d,c,sub,sl,DM_SV_EMAX,&bad))/2;
                    if(val>0){memset(o->raw,0,sizeof o->raw);o->raw[ss->etype]=val;}
                }
            }
        }
        double count=fmax(1,sim_calc(d,c,id,DM_CF_PETMAX,&bad));if(s->fam&DM_FAM_F)count=fmin(count,5);
        int cycle=30;
        if(s->summon>=0 && s->summon<d->nmon) {const dm_sim_mon *m=&d->mon[s->summon];if(m->a1[1]>0)cycle=fmax(1,ceil(m->a1[0]*256.0/m->a1[1])+m->d[0].aidel);}
        if(mage) {
            memset(o->raw,0,sizeof o->raw);
            int sub=s->sumskill[0],sl=(int)sim_calc(d,c,id,DM_CF_SUMSK1,&bad);
            const dm_sim_skill *spell=sub>=0 && sub<DM_SIM_MAXSKILL?d->skill[sub]:NULL;
            if(!spell || sl<1)bad=1;
            else for(int k=0;k<4;k++) {
                int mi=spell->variants[k];
                const dm_sim_miss *miss=mi>=0 && mi<DM_SIM_MAXMISS?d->miss[mi]:NULL;
                if(!miss || miss->etype<=0 || miss->etype>=DM_EL_N){bad=1;continue;}
                double damage=ldexp((dm_sim_level_value(miss->emin,miss->eminlev,sl)+dm_sim_level_value(miss->emax,miss->emaxlev,sl))/2.0,miss->hitshift-8);
                if(miss->etype==DM_EL_POIS) {
                    int duration=dm_sim_len_value(miss->elen,miss->elevlen,sl);

                    o->pet_poison_uptime=(1-pow(.75,count))*fmin(1,duration/(double)cycle);
                    o->raw[DM_EL_POIS]=damage*25*o->pet_poison_uptime;
                } else o->raw[miss->etype]+=damage*count*.25*25/cycle;
            }
        } else {pet_rate=count*25/cycle;for(int e=0;e<DM_EL_N;e++)o->raw[e]*=pet_rate;}
        if(o->dur<=0)o->dur=s->fam&DM_FAM_F?250:DM_SIM_HORIZON;
        if(!(s->fam&DM_FAM_F) && !s->timed && o->dur>=DM_SIM_HORIZON){o->flags|=DM_SF_MINION;o->pets=(int)count;}
        if(!sim_sum(o->raw))o->flags|=DM_SF_MODIFIER;
    }
    int plain[DM_EL_N];memcpy(plain,c->pierce,sizeof plain);
    if(mage)memset(plain,0,sizeof plain);
    for(int i=0;i<d->nskill;i++) {
        const dm_sim_skill *passive=&d->skills[i];if(!(passive->fam&DM_FAM_PA) || c->lvl[passive->id]<=0 || !passive->passivestate[0])continue;
        int reduced=0;for(int j=0;j<d->nstate;j++)if(!strcmp(passive->passivestate,d->state[j].name))reduced=d->state[j].sunderreduce;
        if(!reduced)continue;
        for(int j=0;j<14;j++)for(int e=1;e<DM_EL_N;e++)if(t->res[e]>=100 && passive->pstat[j]==d->stat[DM_ST_PIERCE_FIRE+e-1] && passive->pstat[j]>=0) {
            int contribution=(int)sim_calc(d,c,passive->id,DM_CF_PASSIVE1+j,&bad);
            contribution=sim_clamp(contribution,0,plain[e]>0?plain[e]:0);plain[e]-=contribution-contribution/5;
        }
    }
    int resistance[DM_EL_N];
    for(int e=0;e<DM_EL_N;e++) {
        int st=mod->skill_res_state[e];const dm_sim_state *state=st>=0 && st<d->nstate?&d->state[st]:NULL;
        int red=-mod->res[e];if(e==DM_EL_MAG && t->res[e]>=100 && !c->sunder[e])red=0;
        int res;
        if(mod->res_rules&(1<<e)) {
            int immune=t->res[e]>=100;
            res=immune && c->sunder[e]?95:t->res[e];
            res+=immune?(c->sunder[e]?mod->sunder_res[e]:e==DM_EL_MAG?0:mod->immune_res[e]):mod->res[e];
            if(res<100)res-=plain[e];
            if(res<-100)res=-100;
        } else res=dm_sim_eff_res(t->res[e],red,plain[e],c->sunder[e],state?state->sunderfull:0,state?state->sunderreduce:0);
        resistance[e]=res;
        if(ground.known && e==o->burn_element) {
            double raw,hit;ground.resistance=sim_clamp(res,-100,100);
            if(sim_roll_mean(&ground,&raw,&hit))bad=1;
            else o->burn=hit*o->burn_frames*s->burnfactor/100.0*(t->flags&DM_MF_PRIME?d->diff[c->diff].pvspe/100.0:1);
        }
        o->raw[e]*=o->cth;
        o->hit[e]=res>=100?0:o->raw[e]*(100-res)/100;
        if(t->flags&DM_MF_PRIME)o->hit[e]*=d->diff[c->diff].pvspe/100.0;
        if(c->mercenary==1 && t->flags&DM_MF_BOSS)o->hit[e]*=d->diff[c->diff].mercboss/100.0;
        if(e==DM_EL_LTNG)o->sf_frac*=res>=100?0:fmin(1,(100-res)/100.0);
    }
    if(splashing) {


        double weight=0,reach=0,by_swing=0,pweight=0,preach=0;
        for(int h=0;h<2;h++) {
            if(!(hands_used>>h&1))continue;
            double f=dm_splash_hand_reach(c,&c->hand[h],t->extra_targets),x=0,p=0;
            for(int e=0;e<DM_EL_N;e++) {
                double own=e==DM_EL_PHYS?own_phys:e==s->etype?el:0;
                double v=(hand_raw[h][e]+swing[h]*own)*(resistance[e]>=100?0:(100-resistance[e])/100.0);
                if(e==DM_EL_POIS)p+=v;else x+=v;
            }
            weight+=x;reach+=x*f;by_swing+=swing[h]*f;pweight+=p;preach+=p*f;
        }
        o->splash=weight>0?reach/weight:by_swing;
        o->splash_pois=pweight>0?preach/pweight:o->splash;
    }
    if(s->frameburn && o->burn_frames>0 && s->etype>0 && s->etype<DM_EL_N) {o->burn_element=s->etype;o->burn=o->hit[s->etype]*o->burn_frames;o->hit[s->etype]=0;}
    if(c->poison_frames>0 && s->fam&DM_FAM_WEAPON && o->dot_frames<=0)o->dot_frames=c->poison_frames;
    if(o->hit[DM_EL_POIS]>0 && o->dot_frames>0) {o->dot=o->hit[DM_EL_POIS]*o->dot_frames;o->hit[DM_EL_POIS]=0;}
    if(s->fam&(DM_FAM_P|DM_FAM_F)){o->pet_dps=sim_sum(o->hit);memcpy(o->pet_element,o->hit,sizeof o->pet_element);memset(o->hit,0,sizeof o->hit);}


    if(o->flags&DM_SF_MINION && o->dot>0 && o->dot_frames>0 && pet_rate>0) {
        o->pet_poison_uptime=fmin(1,o->dot_frames*pet_rate/25);
        o->pet_element[DM_EL_POIS]+=o->dot/o->dot_frames/pet_rate*25*o->pet_poison_uptime;
        o->pet_dps=sim_sum(o->pet_element);o->dot=0;o->dot_frames=0;
    }
    if(s->timed) {
        int period=(int)sim_calc(d,c,id,DM_CF_STRIKEPERIOD,&bad);
        if(period<=0 || o->dur<=0)bad=1;
        for(int e=0;e<DM_EL_N;e++){o->pet_element[e]=o->hit[e]*25/fmax(1,period);o->hit[e]=0;}
        o->pet_dps=sim_sum(o->pet_element);o->flags|=DM_SF_TIMED|DM_SF_ESTIMATE;
    }
    int missile=s->missile>=0?s->missile:s->descmissile;
    if(missile>=0 && missile<DM_SIM_MAXMISS && d->miss[missile] && d->miss[missile]->vel>0)o->travel=(int)ceil(10.0*25/d->miss[missile]->vel);
    if(sim_sum(o->raw)>0 && !sim_sum(o->hit) && !o->dot && !o->burn && !o->pet_dps)o->flags|=DM_SF_IMMUNE;
    if(s->fam&DM_FAM_W4) {
        int finisher=0;
        for(int i=0;i<c->nbar;i++) {
            int candidate=c->bar[i];
            if(candidate>=0 && candidate<DM_SIM_MAXSKILL && d->skill[candidate] && c->lvl[candidate] && d->skill[candidate]->fam&DM_FAM_W5){finisher=candidate;break;}
        }
        dm_sim_char charged=*c;charged.charges=3;
        charged.ed+=(int)(3*role(d,c,s,DM_ROLE_DMGPCT,&bad));
        dm_sim_skill_out finish;
        if(sim_action(d,&charged,t,mod,finisher,&finish))bad=1;
        int build=3-sim_clamp(c->charges,0,3);

        double charge_hit=sim_sum(o->hit)*build/4,finish_hit=sim_sum(finish.hit);
        if(o->splash>0 && charge_hit+finish_hit>0)o->splash=(charge_hit*o->splash+finish_hit*(finish.splash>0?finish.splash:1))/(charge_hit+finish_hit);
        for(int e=0;e<DM_EL_N;e++){o->hit[e]=o->hit[e]*build/4+finish.hit[e];o->raw[e]=o->raw[e]*build/4+finish.raw[e];}
        o->frames=o->frames*build/4+finish.frames;o->cooldown=o->frames;
        o->cb_frac=1-pow(1-o->cb_frac,build/4.0)*(1-finish.cb_frac);
        o->mana_x256=o->mana_x256*build+finish.mana_x256;o->hits=build+finish.hits;
    }
    o->cooldown=s->localdelay>s->globaldelay?s->localdelay:s->globaldelay;
    if(o->cooldown<o->frames)o->cooldown=o->frames;
    if(s->cls==7)o->flags|=DM_SF_ESTIMATE;
    if(bad){o->flags|=DM_SF_UNAVAIL;memset(o->hit,0,sizeof o->hit);o->dot=o->pet_dps=0;return -1;}
    own_roll(d,c,t,s,o,resistance);
    normal_roll(d,c,t,s,o,resistance[DM_EL_PHYS]);
    return 0;
}

enum { MASTERY_TH, MASTERY_DMG, MASTERY_CRIT, MASTERY_N };
static int mastery_kind(const char *name) {
    if(strncmp(name,"passive_mastery_melee_",22) && strncmp(name,"passive_mastery_throw_",22))return -1;
    const char *k=name+22;return !strcmp(k,"th")?MASTERY_TH:!strcmp(k,"dmg")?MASTERY_DMG:!strcmp(k,"crit")?MASTERY_CRIT:-1;
}

static int passive_values(const dm_sim_data *d,const dm_sim_char *c,int id,int lvl,int out[14]) {
    dm_sim_char probe=*c;probe.lvl[id]=(int16_t)lvl;int bad=0;
    for(int k=0;k<14;k++)out[k]=lvl>0 && d->skill[id]->pstat[k]>=0?(int)sim_calc(d,&probe,id,DM_CF_PASSIVE1+k,&bad):0;
    return bad?-1:0;
}
int dm_sim_swap_skills_ok(const dm_sim_data *d,const dm_sim_char *now,const dm_sim_char *after,const unsigned char *has_node,
                          const unsigned char *known,int passive_unknown) {
    for(int id=1;id<DM_SIM_MAXSKILL;id++) {
        if(!d->skill[id] || after->lvl[id]==now->lvl[id])continue;
        if(d->skill[id]->passive && passive_unknown)return 0;
        if(after->lvl[id] && !has_node[id] && !known[id])return 0;
    }
    return 1;
}
int dm_sim_passive_levels(const dm_sim_data *d,const dm_sim_char *now,dm_sim_char *after,const int *mastery_th,const int *mastery_dmg,
                          int mastery_crit,dm_sim_passive_shift *shift) {
    memset(shift,0,sizeof *shift);

    int nm=0,mid[32],old[32][MASTERY_N],delta[32][MASTERY_N];
    for(int id=1;id<DM_SIM_MAXSKILL;id++) {
        const dm_sim_skill *s=d->skill[id];if(!s || !s->passive)continue;
        int changed=after->lvl[id]!=now->lvl[id],was[14],will[14];
        if(!changed && now->lvl[id]<=0)continue;
        if(passive_values(d,now,id,now->lvl[id],was) || passive_values(d,after,id,after->lvl[id],will))return -1;
        int mastery=0,o[MASTERY_N]={0},dl[MASTERY_N]={0};
        for(int k=0;k<14;k++) {
            int st=s->pstat[k];if(st<0 || st>=512)continue;
            const char *name=d->stat_name[st];int kind=mastery_kind(name),dv=will[k]-was[k];
            if(kind>=0){mastery=1;o[kind]=was[k];dl[kind]=dv;continue;}
            if(!changed || !dv)continue;
            if(!strcmp(name,"item_tohit_percent"))shift->tohit_pct+=dv;
            else if(!strcmp(name,"skill_armor_percent") || !strcmp(name,"item_armor_percent"))shift->armor_pct+=dv;
            else after->stat_value[st]+=dv;
        }

        if(changed && !strcmp(s->name,"Shape Shifting") && now->form)return -1;
        if(!mastery)continue;
        if(nm==32)return -1;
        mid[nm]=id;memcpy(old[nm],o,sizeof o);memcpy(delta[nm],dl,sizeof dl);nm++;
    }
    int any=0;for(int i=0;i<nm;i++)for(int k=0;k<MASTERY_N;k++)any|=delta[i][k]!=0;
    if(!any)goto attack;

    for(int i=0;i<nm;i++)if(now->lvl[mid[i]]<=0 && (delta[i][0] || delta[i][1] || delta[i][2]))return -1;
    {
        int node_th=0,node_dmg=0,serving=-1;
        for(int id=0;id<DM_SIM_MAXSKILL;id++) {
            if(mastery_th[id]==DM_SIM_NO_NODE)continue;
            int th=0,dmg=0,found=-1;
            for(int i=0;i<nm;i++)if(old[i][MASTERY_TH]==mastery_th[id] && old[i][MASTERY_DMG]==mastery_dmg[id] && (old[i][MASTERY_TH] || old[i][MASTERY_DMG])) {
                if(found>=0 && memcmp(delta[i],delta[found],sizeof delta[i]))return -1;
                th=delta[i][MASTERY_TH];dmg=delta[i][MASTERY_DMG];found=i;
            }

            if(found<0 && (mastery_th[id] || mastery_dmg[id]))return -1;
            if(!id){node_th=th;node_dmg=dmg;serving=found;}
            after->ar_bonus[id]+=th;after->damage_mastery_delta[id]+=dmg;
        }
        for(int id=0;id<DM_SIM_MAXSKILL;id++)if(mastery_th[id]!=DM_SIM_NO_NODE)after->damage_mastery_delta[id]-=node_dmg;
        shift->ar_mastery=node_th;shift->damage_mastery=node_dmg;

        if(serving<0 ? mastery_crit!=0 : old[serving][MASTERY_CRIT]!=mastery_crit)return -1;
        if(serving>=0)after->mastery_cs+=delta[serving][MASTERY_CRIT];
    }
attack:
    for(int id=0;id<DM_SIM_MAXSKILL;id++)if(mastery_th[id]!=DM_SIM_NO_NODE)after->ar_bonus[id]+=shift->tohit_pct;

    for(int id=1;id<DM_SIM_MAXSKILL;id++)if(d->skill[id] && d->skill[id]->passive && after->active_lvl[id]>0)after->active_lvl[id]=after->lvl[id];
    return 0;
}
