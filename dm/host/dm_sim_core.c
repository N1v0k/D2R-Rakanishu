#include "dm_sim_internal.h"
#include <stdio.h>
#include <limits.h>

int dm_sim_level_value(int base, const int32_t lev[5], int lvl) {
    const int steps[5] = {7, 8, 6, 6, 228};
    int64_t v = base;
    int left = sim_clamp(lvl, 1, 256) - 1;
    for (int i = 0; i < 5; i++) { int n = left < steps[i] ? left : steps[i]; v += (int64_t)n * lev[i]; left -= n; }
    return v > INT_MAX ? INT_MAX : v < INT_MIN ? INT_MIN : (int)v;
}
int dm_sim_len_value(int base, const int32_t lev[3], int lvl) {
    int32_t five[5] = {lev[0], lev[1], lev[2], lev[2], lev[2]};
    return dm_sim_level_value(base, five, lvl);
}
int dm_sim_mana_cost(const dm_sim_skill *s, int lvl) {
    int64_t v = ((int64_t)s->mana + (int64_t)s->lvlmana * (sim_clamp(lvl, 1, 256) - 1)) * (1 << sim_clamp(s->manashift, 0, 16));
    if (v < s->minmana * 256) v = s->minmana * 256;
    return v > INT_MAX ? INT_MAX : (int)v;
}
int dm_sim_frames_attack(int fpd, int speed, int ias, int sias, int wsm, int sequence) {
    ias = sim_clamp(ias, 0, 10000);
    int inc = sim_clamp(100 + sias + 120 * ias / (120 + ias) - wsm, 15, 175);
    int rate = sim_clamp(speed, 1, 4096) * inc / 100;
    return sim_clamp((256 * sim_clamp(fpd, 1, 256) + rate - 1) / rate - !sequence, 1, 10000);
}
int dm_sim_frames_rollback(int length,int action,int rate,int rollback,int hits) {
    rate=sim_clamp(rate,1,10000);hits=sim_clamp(hits,1,32);rollback=sim_clamp(rollback,0,100);
    int start=0,total=0;
    for(int i=0;i<hits-1;i++) {
        int frames=(256*(action-start)+rate-1)/rate;if(frames<1)frames=1;
        total+=frames;start=((256*start+rate*frames)/256)*(100-rollback)/100;
    }
    int last=(256*(length-start)+rate-1)/rate-1;
    return total+(last<1?1:last);
}
int dm_sim_frames_cast(int fpd, int speed, int fcr, int sequence) {
    fcr = sim_clamp(fcr, 0, 10000);
    int rate = sim_clamp(speed, 1, 4096) * (100 + sim_clamp(120 * fcr / (120 + fcr), 0, 75)) / 100;
    return sim_clamp((256 * sim_clamp(fpd, 1, 256) + rate - 1) / rate - !sequence, 1, 10000);
}
int dm_sim_frames_recover(int fpd, int speed, int v, int base) {
    v = sim_clamp(v, 0, 10000);
    int rate = sim_clamp(speed, 1, 4096) * (base + 120 * v / (120 + v)) / 100;
    return sim_clamp((256 * sim_clamp(fpd, 1, 256) + rate - 1) / rate - 1, 1, 10000);
}
static int64_t chance_signed(uint32_t value) {
    return value<=INT32_MAX?(int64_t)value:(int64_t)value-4294967296LL;
}
static uint32_t chance_divide(uint32_t a,uint32_t b) {
    int64_t divisor=chance_signed(b);
    return divisor?(uint32_t)(chance_signed(a)/divisor):0;
}
int dm_sim_cth(int ar, int def, int alvl, int dlvl) {

    uint32_t a=(uint32_t)ar,b=def<0?0:(uint32_t)def;
    if(def<0)a-=(uint32_t)def;
    if(a&0x80000000u){b-=a;a=0;if(b&0x80000000u)b=0;}
    uint32_t denominator=a+b,ratio=denominator?chance_divide(a*100u,denominator)<<1:200u;
    int64_t chance=chance_signed(chance_divide(ratio*(uint32_t)alvl,(uint32_t)alvl+(uint32_t)dlvl));
    return chance<5?5:chance>95?95:(int)chance;
}
int dm_sim_eff_res(int base, int skill_red, int plain_red, int sunder, int full, int reduce) {
    int immune = base >= 100, r = immune && sunder ? 95 : base;
    if (immune && !(sunder && full)) skill_red /= 5;
    else if (sunder && immune && reduce) skill_red /= 5;
    r -= skill_red;
    if (r < 100) r -= plain_red;
    return r < -100 ? -100 : r;
}
void dm_sim_char_init(dm_sim_char *c, int cls) {
    memset(c, 0, sizeof *c);
    c->cls = sim_clamp(cls, 0, 7); c->clvl = 1; c->players = 1; c->life = 100; c->mana = 50;
    c->str = c->dex = c->vit = c->ene = 20; c->ar = 100; c->active_aura = -1; c->es_ratio = 32;
    c->nhands = 1; c->hand[0].min = 1; c->hand[0].max = 2; c->hand[0].strbonus = 100;
    strcpy(c->hand[0].wclass, "HTH"); c->lvl[0] = 1;
    for (int e = 0; e < DM_EL_N; e++) c->maxres[e] = e == DM_EL_PHYS ? 50 : 75;
}
void dm_sim_tmod_init(dm_sim_tmod *m) {
    memset(m, 0, sizeof *m);
    for (int e = 0; e < DM_EL_N; e++) m->skill_res_state[e] = -1;
}
static int mon_scale(const dm_sim_data *d, const dm_sim_mon *m, int value, int lvl, int col) {
    return m->flags & DM_MF_NORATIO ? value : (int)((int64_t)value * d->monlvl[lvl][col] / 100);
}
int dm_sim_target_mon(const dm_sim_data *d, int mon, int diff, int mlvl, int kind, int players, dm_sim_target *t) {
    if (!d || !d->ok || !t || mon < 0 || mon >= d->nmon || diff < 0 || diff > 2 || players != 1 || !d->mon[mon].id[0]) return -1;
    const dm_sim_mon *m = &d->mon[mon]; const dm_sim_mond *x = &m->d[diff];
    memset(t, 0, sizeof *t);
    int elite = kind >= DM_TK_CHAMPION && kind <= DM_TK_SUPERUNIQUE;
    int lv = diff == 0 || mlvl <= 0 || (m->flags & (DM_MF_BOSS | DM_MF_NORATIO)) ? m->lvl[diff] : mlvl;
    if ((m->flags & DM_MF_PET) && mlvl > 0) lv = mlvl;
    if (elite && !(m->flags & DM_MF_BOSS)) lv += kind == DM_TK_CHAMPION ? 2 : 3;
    lv = sim_clamp(lv, 0, 110);
    t->mon = mon; t->kind = kind; t->diff = diff; t->mlvl = lv; t->flags = m->flags;t->undead_share=m->flags&DM_MF_UNDEAD?100:0;t->demon_share=m->flags&DM_MF_DEMON?100:0;
    snprintf(t->name, sizeof t->name, "%s", m->namekey);
    t->hp = (mon_scale(d,m,x->hpmin,lv,diff) + mon_scale(d,m,x->hpmax,lv,diff)) / 2;
    t->ac = mon_scale(d,m,x->ac,lv,3+diff); t->ar = mon_scale(d,m,x->a1th,lv,6+diff);
    t->dmin = mon_scale(d,m,x->a1min,lv,9+diff); t->dmax = mon_scale(d,m,x->a1max,lv,9+diff);
    t->block = m->flags & DM_MF_SHIELD ? sim_clamp(x->block,0,100) : 0;
    t->drain = x->drain; t->regen = m->regen; t->crit = m->crit;
    t->cycle = m->a1[1] > 0 ? (m->a1[0] * 256 + m->a1[1]-1) / m->a1[1] + x->aidel : 30;
    if (t->cycle < 1) t->cycle = 1;
    for (int e = 0; e < DM_EL_N; e++) t->res[e] = x->res[e];
    for (int k = 0; k < 3; k++) {
        t->eltype[k] = m->eltype[k]; t->elpct[k] = !strcmp(m->elmode[k],"A1") || !m->elmode[k][0] ? x->elpct[k] : 0;
        t->elmin[k] = mon_scale(d,m,x->elmin[k],lv,9+diff); t->elmax[k] = mon_scale(d,m,x->elmax[k],lv,9+diff);
    }
    if (elite) {
        int champ = kind == DM_TK_CHAMPION, minion = kind == DM_TK_MINION;
        int hpidx = (minion ? 1 : champ ? 4 : 7) + diff;
        t->hp = (int)((int64_t)t->hp * (100 + d->uconst[hpidx]) / 100);
        int bonus = champ ? d->diff[diff].cbonus : d->diff[diff].ubonus;
        t->ar = (int)((int64_t)t->ar * (100 + d->uconst[champ ? 10 : minion ? 12 : 13]) / 100);
        if (champ) { t->dmin = t->dmin * (10000 + d->uconst[11]*bonus) / 10000; t->dmax = t->dmax * (10000+d->uconst[11]*bonus)/10000; }
        int ei=(minion?16:champ?22:28)+diff;
        for(int k=0;k<3;k++) {
            t->elmin[k]=(int)((int64_t)t->elmin[k]*(100+d->uconst[ei])/100);
            t->elmax[k]=(int)((int64_t)t->elmax[k]*(100+d->uconst[ei+3])/100);
        }
    }
    t->ranged_share = !(m->flags & DM_MF_MELEE) || (m->flags & DM_MF_RANGED) ? 100 : 0;
    if(!t->dmax)t->flags|=DM_MF_UNDERESTIMATED;
    return 0;
}
int dm_sim_target_su(const dm_sim_data *d, int idx, int diff, dm_sim_target *t) {
    if (!d || idx < 0 || idx >= d->nsu) return -1;
    const dm_sim_su *s = &d->su[idx]; int lv = 0;
    for (int i = 0; i < d->narea; i++) if (d->area[i].id == s->area) lv = d->area[i].mlvl[sim_clamp(diff,0,2)];
    if (dm_sim_target_mon(d,s->mon,diff,lv,DM_TK_SUPERUNIQUE,1,t)) return -1;
    snprintf(t->name,sizeof t->name,"%s",s->namekey); t->area = s->area; t->act = s->act; t->area_estimated = s->area_guess;
    for (int i = 0; i < 3; i++) {
        int mod=-1;for(int k=0;k<8;k++)if(d->umod[k]==s->mods[i])mod=k;
        switch(mod) {
        case 0: t->dmin = t->dmin*(10000+d->uconst[15]*d->diff[diff].ubonus)/10000; t->dmax = t->dmax*(10000+d->uconst[15]*d->diff[diff].ubonus)/10000; break;
        case 2: t->res[DM_EL_FIRE] += 75; t->flags|=DM_MF_UNDERESTIMATED; break;
        case 3: t->res[DM_EL_LTNG] += 75; t->flags|=DM_MF_UNDERESTIMATED; break;
        case 4: t->res[DM_EL_COLD] += 75; t->flags|=DM_MF_UNDERESTIMATED; break;
        case 5: t->res[DM_EL_PHYS] += 50; t->ac *= 2; break;
        case 1: for (int e = DM_EL_FIRE; e <= DM_EL_COLD; e++) t->res[e] += 40; break;
        }
    }
    return 0;
}
int dm_sim_target_spawn(const dm_sim_data *d,int spawn,int index,int diff,int kind,dm_sim_target *t) {
    if(!d || spawn<0 || spawn>=d->nspawn || diff<0 || diff>2)return -1;
    const dm_sim_spawn *s=&d->spawn[spawn];
    if(index<0 || index>=s->n || s->list!=(diff>0))return -1;
    for(int i=0;i<d->narea;i++)if(d->area[i].id==s->area) {
        if(dm_sim_target_mon(d,s->mon[index],diff,d->area[i].mlvl[diff],kind,1,t))return -1;
        t->area=s->area;t->act=d->area[i].act;return 0;
    }
    return -1;
}
int dm_sim_target_std(const dm_sim_data *d, int diff, int act, dm_sim_target *t) {
    if (!d || !d->ok || !t || diff < 0 || diff > 2 || act < 0 || act > 5 || !d->std[diff][act].n) return -1;
    const dm_sim_std *s = &d->std[diff][act]; memset(t,0,sizeof *t);
    snprintf(t->name,sizeof t->name,"standard-%d-%d",diff,act); t->kind=DM_TK_STD; t->mon=-1; t->diff=diff; t->act=act;
    t->mlvl=s->mlvl; t->hp=s->hp; t->ac=s->ac; t->ar=s->ar; t->dmin=s->dmin; t->dmax=s->dmax; t->cycle=s->cycle;
    t->regen=s->regen; t->block=s->block; t->drain=s->drain; t->crit=5; t->ranged_share=s->ranged;t->undead_share=s->undead;t->demon_share=s->demon;
    for (int e=0;e<DM_EL_N;e++) { t->res[e]=s->res[e]; t->immune_share[e]=s->immune[e]; }

    for (int k=0;k<3;k++) { t->eltype[k]=DM_EL_FIRE+k; t->elpct[k]=s->elshare*s->elpct/100; t->elmin[k]=t->elmax[k]=s->elavg/3; }
    return 0;
}
int dm_sim_std_level(const dm_sim_data *d, int diff, int act, int clvl) {
    if (!d || !d->ok || diff < 0 || diff > 2 || act < 0 || act > 5 || !d->std[diff][act].n) return -1;
    return sim_clamp(clvl, d->std[diff][act].lmin, d->std[diff][act].lmax);
}
static int std_scale(const dm_sim_data *d, int32_t hundredths, int lvl, int col) {
    return (int)((int64_t)hundredths * d->monlvl[lvl][col] / 10000);
}
int dm_sim_target_std_at(const dm_sim_data *d, int diff, int act, int clvl, dm_sim_target *t) {
    int lv = dm_sim_std_level(d, diff, act, clvl);
    if (lv < 0 || dm_sim_target_std(d, diff, act, t)) return -1;
    const dm_sim_std *s = &d->std[diff][act];
    t->mlvl = lv; t->hp = std_scale(d, s->bhp, lv, diff); t->ac = std_scale(d, s->bac, lv, 3 + diff); t->ar = std_scale(d, s->bar, lv, 6 + diff);
    t->dmin = std_scale(d, s->bdmin, lv, 9 + diff); t->dmax = std_scale(d, s->bdmax, lv, 9 + diff);
    if (t->hp < 1) t->hp = 1;
    for (int k = 0; k < 3; k++) t->elmin[k] = t->elmax[k] = std_scale(d, s->bel, lv, 9 + diff) / 3;
    return 0;
}
int dm_sim_area_act(const dm_sim_data *d, int area) {
    if (!d) return 0;
    for (int i = 0; i < d->narea; i++) if (d->area[i].id == area) return d->area[i].act >= 1 && d->area[i].act <= 5 ? d->area[i].act : 0;
    return 0;
}
int dm_sim_target_act(const dm_sim_data *d, const dm_sim_char *c, dm_sim_target *t) {
    if (!c) return -1;
    if (c->act >= 1 && c->act <= 5 && !dm_sim_target_std_at(d, c->diff, c->act, c->clvl, t)) return c->act;
    return dm_sim_target_std_at(d, c->diff, 0, c->clvl, t) ? -1 : 0;
}
int dm_sim_light(const dm_sim_target *t, double ttk, const dm_sim_surv_out *s, int immune, dm_sim_light_out *o) {
    if (!t || !s || !o) return -1;
    double green=3, max=20;
    if (t->kind>=DM_TK_CHAMPION && t->kind<=DM_TK_SUPERUNIQUE) { green=15; max=60; }
    if (t->kind>=DM_TK_ACTBOSS && t->kind<=DM_TK_CLONE) { green=60; max=300; }
    if (t->kind==DM_TK_UBER || t->kind==DM_TK_ANCIENT) { green=120; max=600; }
    o->m=s->max_hit; o->r=ttk>0 ? (s->ttd<0 ? 600 : s->ttd)/ttk : 0; o->reason=DM_LR_NONE;
    if(ttk==-2 || s->ttd==-2){o->light=DM_LIGHT_UNKNOWN;o->r=0;return 0;}
    if (immune) o->reason=DM_LR_IMMUNE;
    else if (ttk<0 || ttk>max) o->reason=DM_LR_TTK;
    else if (o->m>=1) o->reason=DM_LR_ONESHOT;
    else if (o->r<1) o->reason=DM_LR_SAFETY;
    o->light=o->reason ? DM_LIGHT_RED : o->r>=3 && o->m<0.5 && ttk<=green && s->ttd!=-2 ? DM_LIGHT_GREEN : DM_LIGHT_ORANGE;
    return 0;
}
