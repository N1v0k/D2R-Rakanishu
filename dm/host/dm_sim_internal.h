#ifndef DM_SIM_INTERNAL_H
#define DM_SIM_INTERNAL_H
#include "dm_sim_core.h"
#include <math.h>
#include <string.h>
static inline int sim_clamp(int x, int a, int b) { return x < a ? a : x > b ? b : x; }
static inline double sim_sum(const double *x) { double s = 0; for (int i = 0; i < DM_EL_N; i++) s += x[i]; return s; }
int64_t sim_skill_var(const dm_sim_data *d,const dm_sim_char *c,int id,int var,int *bad);
int sim_normal_physical(const dm_sim_char *c,int out[2]);
int sim_attack_rating(const dm_sim_data *d,const dm_sim_char *c,int id,int *bad);
int sim_roll_mean(const dm_sim_roll *,double *raw,double *hit);
double sim_kill_attempts(const dm_sim_roll *,int64_t life);
static inline int sim_valid_char(const dm_sim_data *d,const dm_sim_char *c) {
    if(!d || !d->ok || !c || c->cls<0 || c->cls>=d->ncls || c->diff<0 || c->diff>2 || c->players!=1 ||
       c->nbar<0 || c->nbar>DM_SIM_MAXBAR || c->nactive_buff<0 || c->nactive_buff>8 || c->nhands<1 || c->nhands>2 ||
       !isfinite(c->life) || !isfinite(c->mana) || c->life<=0 || c->life>10000000 || c->mana<0 || c->mana>10000000 || c->clvl<1 || c->clvl>99 ||
       c->active_aura < -1 || c->active_aura>=DM_SIM_MAXSKILL || c->fcr<0 || c->fcr>10000)return 0;
    if(c->active_aura>=0 && !d->skill[c->active_aura])return 0;
    for(int i=0;i<c->nbar;i++)if(c->bar[i]<0 || c->bar[i]>=DM_SIM_MAXSKILL || !d->skill[c->bar[i]])return 0;
    for(int i=0;i<c->nactive_buff;i++)if(c->active_buff[i]<0 || c->active_buff[i]>=DM_SIM_MAXSKILL || !d->skill[c->active_buff[i]])return 0;
    for(int i=0;i<DM_SIM_MAXSKILL;i++)if(c->lvl[i]<0 || c->lvl[i]>256 || c->blvl[i]<0 || c->blvl[i]>256)return 0;
    if(c->ngrant<0 || c->ngrant>DM_SIM_MAXGRANT)return 0;
    for(int i=0;i<c->ngrant;i++) {
        const dm_sim_grant *g=&c->grant[i];
        if(g->kind<0 || g->kind>=DM_GR_N || g->skill<0 || g->skill>=DM_SIM_MAXSKILL || !d->skill[g->skill] || g->lvl<1 || g->lvl>256 || g->held<0 || g->held>256 ||
           g->chance<0 || g->chance>100 || g->charges<0)return 0;
    }
    return 1;
}
double sim_calc(const dm_sim_data *, const dm_sim_char *, int, int, int *);
int sim_action(const dm_sim_data *, const dm_sim_char *, const dm_sim_target *, const dm_sim_tmod *, int, dm_sim_skill_out *);
void sim_modify(const dm_sim_data *, dm_sim_char *, dm_sim_tmod *, int, int, int *);
int dm_sim_stat_modelled(const dm_sim_data *d, int stat);
void sim_prepare(const dm_sim_data *, const dm_sim_char *, const dm_sim_target *, dm_sim_char *, dm_sim_tmod *, dm_sim_rot_out *);
void sim_timeline(const dm_sim_data *, const dm_sim_char *, const dm_sim_target *, const dm_sim_tmod *,
                  dm_sim_skill_out *, int, dm_sim_rot_out *, dm_sim_surv_out *);



void sim_grants_apply(const dm_sim_data *, const dm_sim_char *in, dm_sim_char *c, dm_sim_tmod *, int *bad);
void sim_own_aura(const dm_sim_data *, dm_sim_char *c, dm_sim_tmod *, int id, int sign, int *bad);
void sim_grant_action(const dm_sim_data *, const dm_sim_char *, const dm_sim_target *, const dm_sim_tmod *, int id, dm_sim_skill_out *o);
double sim_grant_aura_dps(const dm_sim_data *, const dm_sim_char *, const dm_sim_target *, const dm_sim_tmod *);
double sim_grant_struck(const dm_sim_data *, const dm_sim_char *, const dm_sim_target *, const dm_sim_tmod *, int *bad);

void sim_defense_hit(const dm_sim_data *, const dm_sim_char *, const dm_sim_target *, const dm_sim_tmod *, double *land);
#endif
