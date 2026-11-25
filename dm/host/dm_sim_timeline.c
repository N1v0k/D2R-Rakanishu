#include "dm_sim_internal.h"
#include <limits.h>
#include <stdlib.h>

#ifdef DM_GEAR_INSTR






#include <pthread.h>
#include <stdatomic.h>
enum { FI_GROUP, FI_SURV, FI_DEALT, FI_N };
typedef struct { uint64_t h1, h2; } fin_key;
static _Thread_local fin_key g_fin_cur;
void dm_sim_instr_set_leaf(uint64_t h1, uint64_t h2) { g_fin_cur.h1 = h1; g_fin_cur.h2 = h2; }
#define FIN_TAB (1u << 21)
static fin_key *g_fin[FI_N];
static pthread_mutex_t g_fin_mu[FI_N] = { PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER };
static atomic_long g_fin_fresh[FI_N], g_fin_dup[FI_N];
static _Thread_local fin_key g_fin_tail[FI_N][1024];
static _Thread_local int g_fin_tail_n[FI_N];
static _Thread_local fin_key g_fin_tail_ch[FI_N];
static _Thread_local int g_fin_tail_have[FI_N];
static atomic_long g_fin_dup_tail[FI_N], g_fin_tail_ovf;
static _Thread_local dm_gear_instr_cp g_cp[16];
static _Thread_local int g_npc, g_cp_at;
static _Thread_local int g_dealt_kill, g_dealt_frame;
static _Thread_local long g_dealt_iters;
static atomic_long g_fruns[FI_N], g_fiters[FI_N], g_fframes[FI_N];
static void fin_note(int kind) {
    fin_key k = g_fin_cur;
    if (!k.h1 && !k.h2) return;
    if (!g_fin[kind]) g_fin[kind] = calloc(FIN_TAB, sizeof **g_fin);
    if (g_fin[kind]) {
        int dup = 0;
        pthread_mutex_lock(&g_fin_mu[kind]);
        size_t i = (size_t)(k.h1 * 0x9e3779b97f4a7c15ULL >> 43) & (FIN_TAB - 1);
        for (size_t p = 0; p < FIN_TAB; p++) {
            fin_key *e = &g_fin[kind][(i + p) & (FIN_TAB - 1)];
            if (!e->h1 && !e->h2) { *e = k; break; }
            if (e->h1 == k.h1 && e->h2 == k.h2) { dup = 1; break; }
        }
        pthread_mutex_unlock(&g_fin_mu[kind]);
        atomic_fetch_add_explicit(&g_fin_dup[kind], dup, memory_order_relaxed);
        atomic_fetch_add_explicit(&g_fin_fresh[kind], !dup, memory_order_relaxed);
    }
    fin_key *tab = g_fin_tail[kind];
    if (g_fin_tail_have[kind] && g_fin_tail_ch[kind].h1 == k.h1 && g_fin_tail_ch[kind].h2 == k.h2) {
        atomic_fetch_add_explicit(&g_fin_dup_tail[kind], 1, memory_order_relaxed);
        return;
    }
    int n = g_fin_tail_n[kind];
    int hit = 0;
    for (int j = 0; j < n && !hit; j++) hit = tab[j].h1 == k.h1 && tab[j].h2 == k.h2;
    if (hit) atomic_fetch_add_explicit(&g_fin_dup_tail[kind], 1, memory_order_relaxed);
    if (n < 1024) g_fin_tail[kind][n] = k; else atomic_fetch_add_explicit(&g_fin_tail_ovf, 1, memory_order_relaxed);
    g_fin_tail_n[kind] = n + 1;
    g_fin_tail_ch[kind] = k; g_fin_tail_have[kind] = 1;
}
void dm_sim_instr_dealt_last(int *kill, int *frame, long *iters) { *kill = g_dealt_kill; *frame = g_dealt_frame; *iters = g_dealt_iters; }
void dm_sim_instr_dealt_cps(const dm_gear_instr_cp **cps, int *n) { *cps = g_cp; *n = g_npc; }
void dm_sim_instr_fights(long *runs, long *iters, long *frames) {
    for (int k = 0; k < FI_N; k++) {
        runs[k] = atomic_load(&g_fruns[k]); iters[k] = atomic_load(&g_fiters[k]); frames[k] = atomic_load(&g_fframes[k]);
    }
}
void dm_sim_instr_inputs(long *fresh, long *dup_walk, long *dup_tail, long *tail_overflow) {
    for (int k = 0; k < FI_N; k++) {
        fresh[k] = atomic_load(&g_fin_fresh[k]); dup_walk[k] = atomic_load(&g_fin_dup[k]); dup_tail[k] = atomic_load(&g_fin_dup_tail[k]);
    }
    *tail_overflow = atomic_load(&g_fin_tail_ovf);
}
void dm_sim_instr_tail_new(void) {
    for (int k = 0; k < FI_N; k++) { g_fin_tail_n[k] = 0; g_fin_tail_have[k] = 0; }
}
void dm_sim_instr_p6_reset(void) {
    for (int k = 0; k < FI_N; k++) {
        free(g_fin[k]); g_fin[k] = NULL;
        atomic_store(&g_fin_fresh[k], 0); atomic_store(&g_fin_dup[k], 0); atomic_store(&g_fin_dup_tail[k], 0);
        atomic_store(&g_fruns[k], 0); atomic_store(&g_fiters[k], 0); atomic_store(&g_fframes[k], 0);
    }
    atomic_store(&g_fin_tail_ovf, 0);
    dm_sim_instr_tail_new();
}
#endif

typedef struct {int low,step,count,offset;double mass;} roll_part;
typedef struct {int count;roll_part part[400];double raw;} roll_support;
static int roll_gcd(int a,int b) {while(b){int r=a%b;a=b;b=r;}return a;}
static int roll_support_read(const dm_sim_roll *r,int flat,roll_support *support) {
    memset(support,0,sizeof *support);
    if(!r || !r->known || r->low<0 || r->high<r->low || r->high>0x80000 ||
       r->resistance < -100 || r->resistance>100 || r->element<0 || r->element>=DM_EL_N ||
       !isfinite(r->chance) || r->chance<0 || r->chance>1 ||
       !isfinite(r->critical) || r->critical<0 || r->critical>1)return -1;
    int width=r->high-r->low;
    uint64_t words=UINT64_C(1)<<32;
    for(int critical=0;critical<2;critical++) {
        double probability=r->chance*(critical?r->critical:1-r->critical);
        if(probability<=0)continue;
        for(int remainder=0;remainder<2;remainder++) {
            int count=width?(remainder?(int)(words%(unsigned)width):width):1;
            double mass=probability*(width?(remainder?1.0/words:(double)(words/(unsigned)width)/words):1);
            if(!width && remainder)continue;
            if(!count)continue;
            support->raw+=mass*count*(r->low+(count-1)/2.0)*(critical+1)/256;
            int multiplier=(critical+1)*(100-r->resistance);
            if(!multiplier)continue;
            int gcd=roll_gcd(multiplier,100),period=100/gcd,step=multiplier/gcd;
            for(int residue=0;residue<period && residue<count;residue++) {
                if(support->count==400)return -1;
                int64_t numerator=(int64_t)(r->low+residue)*multiplier-(int64_t)flat*(100-r->resistance);
                int n=(count-1-residue)/period+1;
                int64_t skip=numerator<100?(100-numerator+(int64_t)step*100-1)/((int64_t)step*100):0;
                if(skip>=n)continue;
                int low=(int)((numerator+skip*step*100)/100);n-=(int)skip;
                support->part[support->count++]=(roll_part){low,step,n,0,mass};
            }
        }
    }
    return 0;
}
static int roll_mean_flat(const dm_sim_roll *r,int flat,double *raw,double *hit) {
    roll_support support;
    if(!raw || !hit || roll_support_read(r,flat,&support))return -1;
    *raw=support.raw;*hit=0;
    for(int i=0;i<support.count;i++) {
        const roll_part *p=&support.part[i];
        *hit+=p->mass*p->count*(p->low+(p->count-1)*(double)p->step/2)/256;
    }
    return 0;
}
int sim_roll_mean(const dm_sim_roll *r,double *raw,double *hit) {
    return roll_mean_flat(r,0,raw,hit);
}
double sim_kill_attempts(const dm_sim_roll *roll,int64_t life) {
    roll_support support;
    if(life<=0 || roll_support_read(roll,0,&support))return -2;

    life-=255;
    int minimum=INT32_MAX,maximum=0,divisor=0;double probability=0;
    for(int i=0;i<support.count;i++) {
        roll_part *p=&support.part[i];int high=p->low+(p->count-1)*p->step;
        if(p->low<minimum)minimum=p->low;
        if(high>maximum)maximum=high;
        divisor=roll_gcd(divisor,p->low);
        if(p->count>1)divisor=roll_gcd(divisor,p->step);
        probability+=p->mass*p->count;
    }
    if(probability<=0)return -1;
    if(life<=minimum)return 1/probability;
    if(minimum==maximum)return (double)(life/minimum+(life%minimum!=0))/probability;
    int64_t cells=life/divisor+(life%divisor!=0);

    if(cells>2097152 || cells*support.count>8000000)return -2;
    int sums_count=0;
    for(int i=0;i<support.count;i++) {
        roll_part *p=&support.part[i];p->low/=divisor;
        p->step=p->count==1?1:p->step/divisor;p->offset=sums_count;sums_count+=p->step;
    }
    double *expectation=calloc((size_t)cells+1,sizeof *expectation),*sums=calloc((size_t)sums_count,sizeof *sums);
    if(!expectation || !sums){free(expectation);free(sums);return -2;}
    for(int h=1;h<=cells;h++) {
        double accumulated=1;
        for(int i=0;i<support.count;i++) {
            const roll_part *p=&support.part[i];int index=p->offset+h%p->step;
            double value=sums[index];int enter=h-p->low,leave=enter-p->count*p->step;
            if(enter>=0)value+=expectation[enter];
            if(leave>=0)value-=expectation[leave];
            sums[index]=value;accumulated+=p->mass*value;
        }
        expectation[h]=accumulated/probability;
    }
    double result=expectation[cells];free(expectation);free(sums);
    return isfinite(result)?result:-2;
}

typedef struct {
    double life,mana,target,damage,phys,shortage,spent;
    double damage_element[DM_EL_N];
    double poison,ow,owprob,bone,cyclone,ward;
    double burn[DM_SIM_MAXBAR+1];int burn_end[DM_SIM_MAXBAR+1],burn_element[DM_SIM_MAXBAR+1];
    int poison_end,ow_end,prevent,frame,action,next_enemy,dead,kill,failed,nburn;
    double spare;

    int belt[DM_POT_N],potions,heal_end,mana_end,shortage_now;double heal_rate,mana_rate;
    double kill_shortage,kill_spent,kill_mana;int kill_frame,kill_seen;
} fight;
static int potions_on(const dm_sim_data *d,const dm_sim_char *c) {
    if(!c->potions || d->potions_loaded!=(1u<<DM_POT_N)-1 || c->cls<0 || c->cls>=d->ncls)return 0;
    for(int k=0;k<DM_POT_N;k++)if(c->belt[k]>0)return 1;
    return 0;
}
static float percent(double value,double whole){return whole>0?(float)fmin(100,fmax(0,100*value/whole)):0;}
static void trace_values(const fight *f,const dm_sim_char *c,const dm_sim_target *t,float v[DM_TS_N]) {
    v[DM_TS_ENEMY]=percent(f->target,t->hp);v[DM_TS_LIFE]=percent(f->life,c->life);v[DM_TS_MANA]=percent(f->mana,c->mana);
}
static void trace_add(dm_sim_trace *tr,int k,const float v[DM_TS_N],int shortage) {
    dm_sim_trace_bucket *b=&tr->b[k];
    if(!b->used){memset(b,0,sizeof *b);b->used=1;for(int s=0;s<DM_TS_N;s++)b->lo[s]=b->hi[s]=v[s];}
    for(int s=0;s<DM_TS_N;s++){b->lo[s]=fminf(b->lo[s],v[s]);b->hi[s]=fmaxf(b->hi[s],v[s]);b->last[s]=v[s];}
    if(shortage)b->shortage=1;
}

static void trace_merge(dm_sim_trace *tr) {
    int n=0;
    for(int j=0;j<tr->n;j+=2) {
        dm_sim_trace_bucket a=tr->b[j];
        if(j+1<tr->n && tr->b[j+1].used) {
            const dm_sim_trace_bucket *b=&tr->b[j+1];
            for(int s=0;s<DM_TS_N;s++){a.lo[s]=fminf(a.lo[s],b->lo[s]);a.hi[s]=fmaxf(a.hi[s],b->hi[s]);a.last[s]=b->last[s];}
            a.acts|=b->acts;a.shortage|=b->shortage;
        }
        tr->b[n++]=a;
    }
    for(int j=n;j<tr->n;j++)memset(&tr->b[j],0,sizeof tr->b[j]);
    tr->n=n;tr->width*=2;
}


static void trace_point(dm_sim_trace *tr,int frame,const float v[DM_TS_N],int shortage) {
    if(!tr || !tr->open || frame<tr->last_frame)return;
    while(frame/tr->width>=DM_SIM_TRACE_BUCKETS)trace_merge(tr);
    int k=frame/tr->width,gap=frame>tr->last_frame;
    if(gap && tr->last_short)for(int j=tr->last_frame/tr->width;j<tr->n && j<=k;j++)tr->b[j].shortage=1;
    for(int j=tr->n;j<=k;j++) {
        int at=j*tr->width;if(at>=frame && j==k)break;
        double q=gap?fmin(1,fmax(0,(at-tr->last_frame)/(double)(frame-tr->last_frame))):1;float w[DM_TS_N];
        for(int s=0;s<DM_TS_N;s++)w[s]=(float)(tr->last_value[s]+(v[s]-tr->last_value[s])*q);
        trace_add(tr,j,w,gap && tr->last_short);
    }
    if(k+1>tr->n)tr->n=k+1;
    trace_add(tr,k,v,shortage);
    tr->last_frame=frame;memcpy(tr->last_value,v,sizeof tr->last_value);tr->last_short=shortage;
}
static void trace_end(dm_sim_trace *tr,int frame,const float v[DM_TS_N],int kind) {
    if(!tr || !tr->open)return;
    trace_point(tr,frame,v,tr->last_short);tr->end=frame;tr->end_kind=kind;tr->open=0;
}
static void take(const dm_sim_data *d,const dm_sim_char *c,const dm_sim_target *t,fight *f,int pick,dm_sim_trace *tr) {
    const dm_sim_potion *p=&d->potion[pick];const dm_sim_class *k=&d->cls[c->cls];
    if(tr && tr->open && tr->npotion<DM_SIM_TRACE_POTIONS) {
        float v[DM_TS_N];trace_values(f,c,t,v);
        tr->potion[tr->npotion].frame=f->frame;tr->potion[tr->npotion].kind=pick;tr->potion[tr->npotion].life=v[DM_TS_LIFE];tr->potion[tr->npotion].mana=v[DM_TS_MANA];tr->npotion++;
    }
    f->belt[pick]--;
    if(p->kind==DM_PK_REJUV){f->life=fmin(c->life,f->life+c->life*p->life/100.0);f->mana=fmin(c->mana,f->mana+c->mana*p->mana/100.0);}
    else if(p->kind==DM_PK_HEAL){f->heal_rate=p->life*k->hppot/100.0/p->len;f->heal_end=f->frame+p->len;}
    else {f->mana_rate=p->mana*k->mppot/100.0/p->len;f->mana_end=f->frame+p->len;}
}


static void drink_life(const dm_sim_data *d,const dm_sim_char *c,const dm_sim_target *t,fight *f,dm_sim_trace *tr) {
    int pick=-1;
    if(f->life<c->life*0.25)for(int j=DM_POT_RVL;j>=DM_POT_RVS && pick<0;j--)if(f->belt[j])pick=j;
    if(pick<0 && f->life<c->life*0.5 && f->heal_end<=f->frame)for(int j=DM_POT_MP1-1;j>=DM_POT_HP1 && pick<0;j--)if(f->belt[j])pick=j;
    if(pick>=0)take(d,c,t,f,pick,tr);
}


static void drink_mana(const dm_sim_data *d,const dm_sim_char *c,const dm_sim_target *t,fight *f,dm_sim_trace *tr) {
    int pick=-1;
    if(f->shortage_now && f->mana_end<=f->frame)for(int j=DM_POT_RVS-1;j>=DM_POT_MP1 && pick<0;j--)if(f->belt[j])pick=j;
    if(pick>=0)take(d,c,t,f,pick,tr);
}

static int grant_level(const dm_sim_char *c,int id,int held) {
    int v=0;
    for(int i=0;i<c->ngrant;i++) {
        const dm_sim_grant *g=&c->grant[i];
        if(g->skill==id && (g->kind==DM_GR_AURA || g->kind==DM_GR_BUFF) && (held?g->held:g->lvl)>v)v=held?g->held:g->lvl;
    }
    return v;
}
static int is_active(const dm_sim_char *c,int id) {
    if(id>=0 && id<DM_SIM_MAXSKILL && c->active_lvl[id]>0)return 1;
    if(c->active_aura==id)return 1;
    for(int i=0;i<c->nactive_buff;i++)if(c->active_buff[i]==id)return 1;
    return grant_level(c,id,0)>0;
}
static int group(const dm_sim_skill *s) {
    if(!s)return 0;
    if(!strcmp(s->aurastate,"frozenarmor") || !strcmp(s->aurastate,"shiverarmor") || !strcmp(s->aurastate,"chillingarmor"))return 1;
    if(!strcmp(s->aurastate,"wolf") || !strcmp(s->aurastate,"bear"))return 2;
    if(!strcmp(s->aurastate,"fade") || !strcmp(s->aurastate,"quickness"))return 3;
    if(strstr(s->aurastate,"golem"))return 4;
    return 0;
}
static double score(const dm_sim_skill_out *o) {
    return o->flags&DM_SF_UNAVAIL?0:sim_sum(o->hit)*25/fmax(o->cooldown,1)+o->pet_dps+o->dot*25/fmax(o->dot_frames,o->cooldown)+o->burn*25/fmax(o->burn_frames,o->cooldown);
}
static double best_attack(const dm_sim_data *d,const dm_sim_char *c,const dm_sim_target *t,const dm_sim_tmod *m) {
    double best=0;
    for(int i=-1;i<c->nbar;i++) {
        int id=i<0?0:c->bar[i];
        if(id<0 || id>=DM_SIM_MAXSKILL || !d->skill[id] || d->skill[id]->fam&DM_FAM_MODIFIER)continue;
        dm_sim_skill_out o;sim_action(d,c,t,m,id,&o);double v=score(&o);if(v>best)best=v;
    }
    return best;
}
static void prepare_modify(const dm_sim_data *d,dm_sim_char *c,dm_sim_tmod *m,int id,int sign,int *bad) {
    if(id<0 || id>=DM_SIM_MAXSKILL || !d->skill[id] || !c->lvl[id])return;
    if(d->skill[id]->unavailable){c->lvl[id]=0;*bad=1;return;}
    dm_sim_char candidate=*c;dm_sim_tmod target=*m;int invalid=0;
    sim_modify(d,&candidate,&target,id,sign,&invalid);
    if(invalid){c->lvl[id]=0;*bad=1;return;}
    *c=candidate;*m=target;
}

static void level_modify(const dm_sim_data *d,dm_sim_char *c,dm_sim_tmod *m,int id,int lvl,int sign,int *bad) {
    if(lvl<=0 || id<0 || id>=DM_SIM_MAXSKILL)return;
    int16_t keep=c->lvl[id];c->lvl[id]=(int16_t)lvl;prepare_modify(d,c,m,id,sign,bad);
    if(!c->lvl[id] && lvl==keep)return;
    c->lvl[id]=keep;
}


static void own_aura(const dm_sim_data *d,dm_sim_char *c,dm_sim_tmod *m,int id,int sign,int *bad) {
    if(id<0 || id>=DM_SIM_MAXSKILL)return;
    int granted=grant_level(c,id,0),own=c->lvl[id];
    if(!granted){prepare_modify(d,c,m,id,sign,bad);return;}
    if(granted>=own)return;
    level_modify(d,c,m,id,sign>0?granted:own,-1,bad);level_modify(d,c,m,id,sign>0?own:granted,1,bad);
}
static int target_only(const dm_sim_skill *s){return (s->fam&DM_FAM_K)!=0 || !strcmp(s->aurastate,"conviction");}


static void grants_on(const dm_sim_data *d,const dm_sim_char *in,dm_sim_char *c,dm_sim_tmod *m,int *bad) {
    for(int i=0;i<in->ngrant;i++) {
        int id=in->grant[i].skill,first=1;
        if(id<0 || id>=DM_SIM_MAXSKILL || !d->skill[id] || (in->grant[i].kind!=DM_GR_AURA && in->grant[i].kind!=DM_GR_BUFF)){if(id<0 || id>=DM_SIM_MAXSKILL || !d->skill[id])*bad=1;continue;}
        for(int k=0;k<i;k++)if(in->grant[k].skill==id && (in->grant[k].kind==DM_GR_AURA || in->grant[k].kind==DM_GR_BUFF))first=0;
        if(!first)continue;
        int want=grant_level(in,id,0),held=grant_level(in,id,1);
        if(in->active_aura==id && c->lvl[id]>want)want=c->lvl[id];
        if(target_only(d->skill[id])){level_modify(d,c,m,id,want,1,bad);continue;}
        if(held==want)continue;
        level_modify(d,c,m,id,held,-1,bad);level_modify(d,c,m,id,want,1,bad);
    }
}

static double grant_cast(const dm_sim_data *d,const dm_sim_char *c,const dm_sim_target *t,const dm_sim_tmod *m,int id,int lvl,int *bad) {
    if(id<0 || id>=DM_SIM_MAXSKILL || !d->skill[id] || lvl<1){*bad=1;return 0;}
    dm_sim_char *x=malloc(sizeof *x);if(!x){*bad=1;return 0;}
    *x=*c;x->lvl[id]=(int16_t)lvl;x->ngrant=0;
    dm_sim_skill_out o;double v=0;
    if(sim_action(d,x,t,m,id,&o) || o.flags&(DM_SF_UNAVAIL|DM_SF_MODIFIER|DM_SF_UTILITY|DM_SF_NOWEAPON))*bad=1;
    else {
        v=sim_sum(o.hit)+o.dot+o.burn;
        if(o.pet_dps>0 || o.sf_frac>0 || o.cb_frac>0)*bad=1;
    }
    free(x);return v;
}
void dm_sim_hands_drop(const dm_sim_data *d,const dm_sim_char *now,dm_sim_char *c) {
    for(int id=1;id<DM_SIM_MAXSKILL;id++) {
        const dm_sim_skill *s=d->skill[id];
        if(!s || s->passive || s->aura || now->active_lvl[id]<=0 || c->active_lvl[id]<=0 || dm_sim_skill_hands(d,now,id) || !dm_sim_skill_hands(d,c,id))continue;

        int16_t level=c->lvl[id];int shield_min=c->shield_min,shield_max=c->shield_max,bad=0;
        dm_sim_tmod m;dm_sim_tmod_init(&m);
        c->lvl[id]=c->active_lvl[id];sim_modify(d,c,&m,id,-1,&bad);c->lvl[id]=level;
        c->shield_min=shield_min;c->shield_max=shield_max;c->active_lvl[id]=0;
        for(int k=0;k<c->nactive_buff;k++)if(c->active_buff[k]==id){c->active_buff[k]=c->active_buff[--c->nactive_buff];k--;}
        if(bad)c->capture_bad|=2;
    }
}
void sim_prepare(const dm_sim_data *d,const dm_sim_char *in,const dm_sim_target *t,dm_sim_char *c,dm_sim_tmod *m,dm_sim_rot_out *r) {
    *c=*in;dm_sim_tmod_init(m);memset(r,0,sizeof *r);r->primary=0;r->cooldown_skill=r->curse=-1;r->aura=c->active_aura;
    int bad=0;
    for(int id=0;id<DM_SIM_MAXSKILL;id++)if(is_active(in,id) && in->active_lvl[id]>0 && in->active_lvl[id]!=in->lvl[id]) {
        int16_t level=c->lvl[id];c->lvl[id]=in->active_lvl[id];prepare_modify(d,c,m,id,-1,&bad);
        c->lvl[id]=level;prepare_modify(d,c,m,id,1,&bad);
    }
    if(in->ngrant)grants_on(d,in,c,m,&bad);

    double best=best_attack(d,c,t,m);int aura=c->active_aura;
    for(int i=0;i<c->nbar;i++) {
        int id=c->bar[i];if(id<0 || id>=DM_SIM_MAXSKILL || !d->skill[id] || !c->lvl[id])continue;
        const dm_sim_skill *s=d->skill[id];
        if(s->aura && s->fam&(DM_FAM_AOFF|DM_FAM_ADEF|DM_FAM_ADMG) && !dm_sim_skill_hands(d,c,id)) {
            dm_sim_char test=*c;dm_sim_tmod tm=*m;
            if(!in->ngrant)dm_sim_tmod_init(&tm);
            if(in->active_aura>=0)own_aura(d,&test,&tm,in->active_aura,-1,&bad);
            own_aura(d,&test,&tm,id,1,&bad);
            double v=best_attack(d,&test,t,&tm);dm_sim_skill_out o;sim_action(d,&test,t,&tm,id,&o);v+=score(&o);
            if(v>best || aura<0){best=v;aura=id;}
        }
    }
    if(aura!=in->active_aura) {
        own_aura(d,c,m,in->active_aura,-1,&bad);own_aura(d,c,m,aura,1,&bad);c->active_aura=aura;
    } else if(aura>=0 && !strcmp(d->skill[aura]->aurastate,"conviction") && !grant_level(c,aura,0)) {

        prepare_modify(d,c,m,aura,1,&bad);
    }
    r->aura=aura;
    int chosen[5]={-1,-1,-1,-1,-1};
    for(int i=0;i<c->nbar;i++) {
        int id=c->bar[i];if(id<0 || id>=DM_SIM_MAXSKILL || !d->skill[id] || !c->lvl[id])continue;
        const dm_sim_skill *s=d->skill[id];if(!(s->fam&DM_FAM_B))continue;

        if(!is_active(in,id) && dm_sim_skill_hands(d,c,id))continue;
        int g=group(s);if(g && chosen[g]>=0)continue;if(g)chosen[g]=id;
        if(!is_active(in,id)) {
            for(int k=0;g && k<in->nactive_buff;k++)if(group(d->skill[in->active_buff[k]])==g)prepare_modify(d,c,m,in->active_buff[k],-1,&bad);
            prepare_modify(d,c,m,id,1,&bad);
        }
        if(!s->timed && !is_active(c,id) && c->nactive_buff<8)c->active_buff[c->nactive_buff++]=id;
    }
    best=best_attack(d,c,t,m);
    for(int i=0;i<c->nbar;i++) {
        int id=c->bar[i];if(id<0 || id>=DM_SIM_MAXSKILL || !d->skill[id] || !c->lvl[id] || !(d->skill[id]->fam&DM_FAM_K) || dm_sim_skill_hands(d,c,id))continue;
        dm_sim_char test=*c;dm_sim_tmod tm=*m;prepare_modify(d,&test,&tm,id,1,&bad);
        double v=best_attack(d,&test,t,&tm);
        if(v>best || (v==best && tm.lifetap>m->lifetap)){best=v;r->curse=id;}
    }
    if(r->curse>=0)prepare_modify(d,c,m,r->curse,1,&bad);
    c->struck=0;
    for(int i=0;i<c->ngrant;i++)if(c->grant[i].kind==DM_GR_STRUCK)
        c->struck+=sim_clamp(c->grant[i].chance,0,100)/100.0*grant_cast(d,c,t,m,c->grant[i].skill,c->grant[i].lvl,&bad);
    if(bad)r->flags|=DM_SF_ESTIMATE;
}
static double reduction(const dm_sim_data *d,const dm_sim_char *c,int e,double raw,double *heal) {
    if(e==DM_EL_PHYS)return fmax(0,raw-c->dr_flat)*(100-sim_clamp(c->res[e],-100,50))/100;
    if(e!=DM_EL_POIS)raw=fmax(0,raw-c->mdr_flat);
    int penalty=e==DM_EL_MAG?0:d->diff[c->diff].rp;
    int res=sim_clamp(c->res[e]+penalty,-100,sim_clamp(c->maxres[e],0,95));
    raw*=1-res/100.0;
    double ab=raw*sim_clamp(c->absorb_pct[e],0,40)/100.0;raw-=ab;
    double flat=fmin(raw,fmax(0,c->absorb_flat[e]));raw-=flat;*heal+=ab+flat;
    return raw;
}
typedef struct {double low,scale;int width[4],n;} damage_range;
static long double choose_small(long double n,int k) {
    long double result=1;
    for(int i=1;i<=k;i++)result=result*(n-i+1)/i;
    return result;
}
static void range_add(damage_range *r,int low,int high) {
    r->low+=low;
    if(high>low)r->width[r->n++]=high-low;
}
static long double uniform_tail(int n,const int *steps,int start,long double u) {
    if(u<=0)return 0;
    long double k=floorl(u),span=0;
    for(int i=start;i<n;i++)span+=steps[i];
    if(k<span)return uniform_tail(n,steps,start+1,u)-uniform_tail(n,steps,start+1,u-steps[start]);


    long double polynomial[6]={1},next[6];int degree=0;
    for(int i=1;i<=n;i++) {
        memset(next,0,sizeof next);
        for(int j=0;j<=degree;j++){next[j]+=polynomial[j]*i;next[j+1]+=polynomial[j];}
        memcpy(polynomial,next,sizeof next);degree++;
    }
    memset(next,0,sizeof next);
    for(int j=0;j<=degree;j++){next[j]+=polynomial[j]*(u-k);next[j+1]+=polynomial[j]/(n+1);}
    memcpy(polynomial,next,sizeof next);degree++;
    for(int i=start;i<n;i++) {
        memset(next,0,sizeof next);
        for(int j=0;j<degree;j++) {
            long double power=-steps[i];
            for(int a=j+1;a<=degree;a++){next[j]-=polynomial[a]*choose_small(a,j)*power;power*=-steps[i];}
        }
        memcpy(polynomial,next,sizeof next);degree--;
    }
    long double value=polynomial[degree];
    for(int j=degree-1;j>=0;j--)value=value*k+polynomial[j];
    for(int i=2;i<=n;i++)value/=i;
    return value;
}
static double range_tail(const damage_range *r,double threshold) {
    if(r->scale<=0)return 0;
    double high=r->low,mean=r->low;
    for(int i=0;i<r->n;i++){high+=r->width[i];mean+=r->width[i]/2.0;}
    if(threshold<=r->low*r->scale)return mean*r->scale-threshold;
    if(threshold>=high*r->scale)return 0;
    long double u=high-threshold/r->scale,denominator=1;
    int steps[4];
    for(int i=0;i<r->n;i++){steps[i]=r->width[i]+1;denominator*=steps[i];}
    for(int i=0;i<r->n;i++)for(int j=i+1;j<r->n;j++)if(steps[j]>steps[i]){int temp=steps[i];steps[i]=steps[j];steps[j]=temp;}
    return (double)(uniform_tail(r->n,steps,0,u)/denominator*r->scale);
}
static double shield_tail(const damage_range *r,double pool,double fraction,double cap,double threshold) {
    if(fraction<=0 || cap<=0)return range_tail(r,pool+threshold);
    double boundary=cap/fraction;
    if(threshold>=(1-fraction)*boundary)return range_tail(r,pool+cap+threshold);
    return (1-fraction)*range_tail(r,pool+threshold/(1-fraction))+fraction*range_tail(r,pool+boundary);
}
static int incoming_physical(const dm_sim_data *d,const dm_sim_char *c,const dm_sim_target *t,const dm_sim_tmod *m,
                             const fight *f,int critical,int maximum,double *value) {
    if(t->kind!=DM_TK_NORMAL || t->mon<0 || t->mon>=d->nmon || t->diff<0 || t->diff>2 ||
       !(t->flags&DM_MF_MELEE) || t->flags&(DM_MF_BOSS|DM_MF_PRIME|DM_MF_RANGED|DM_MF_UNDERESTIMATED) ||
       m->dmg_pct || c->mercenary || c->es_pct || c->bone_pool || (f && f->bone>0) ||
       c->dr_flat<0 || c->dr_flat>2048 || t->dmax<=0 || t->dmin>2047 || t->dmax>2048)return 0;
    const dm_sim_mon *base=&d->mon[t->mon];const dm_sim_mond *variant=&base->d[t->diff];
    if(!base->id[0] || base->skilldamage>=0)return 0;
    for(int k=0;k<8;k++)if(base->skills[k]>=0)return 0;
    if(base->a2[0]>0 && (variant->a1min!=variant->a2min || variant->a1max!=variant->a2max))return 0;
    for(int k=0;k<3;k++)if(t->eltype[k]==DM_EL_PHYS && t->elpct[k]>0)return 0;
    int low=256*(t->dmin<1?1:t->dmin),high=256*(t->dmax<2?2:t->dmax);
    if(high<=low)high=low+256;
    dm_sim_roll roll={1,low,high,sim_clamp(c->res[DM_EL_PHYS],-100,50),DM_EL_PHYS,1,critical};
    if(maximum) {
        roll_support support;if(roll_support_read(&roll,c->dr_flat*256,&support))return 0;
        int largest=0;
        for(int k=0;k<support.count;k++) {
            const roll_part *p=&support.part[k];int end=p->low+(p->count-1)*p->step;
            if(end>largest)largest=end;
        }
        *value=largest/256.0;return 1;
    }
    double raw;return !roll_mean_flat(&roll,c->dr_flat*256,&raw,value);
}
static double incoming_mean(const dm_sim_data *d,const dm_sim_char *c,const dm_sim_target *t,const dm_sim_tmod *m,
                            fight *f,int critical,double probability,double *healed) {
    double total=0;
    for(int e=0;e<DM_EL_N;e++) {
        double exact;
        if(e==DM_EL_PHYS && incoming_physical(d,c,t,m,f,critical,0,&exact)){total+=exact;continue;}
        damage_range r={.scale=(critical?2:1)*fmax(0,100+m->dmg_pct)/100};
        if(e==DM_EL_PHYS)range_add(&r,t->dmin,t->dmax);
        for(int k=0;k<3;k++)if(t->eltype[k]==e && t->elpct[k]>0)range_add(&r,t->elmin[k],t->elmax[k]);
        if(t->flags&DM_MF_PRIME && c->mercenary)r.scale*=(c->mercenary==2?d->diff[c->diff].pevspet:d->diff[c->diff].pevsmerc)/100.0;
        double pool=0,fraction=0,cap=0;
        double p=fmax(probability,.0001);
        if(f) {
            double *available=e==DM_EL_PHYS?&f->bone:e>=DM_EL_FIRE && e<=DM_EL_COLD?&f->cyclone:e==DM_EL_MAG?&f->ward:NULL;
            if(available) {
                pool=*available/p;
                *available=fmax(0,*available-(range_tail(&r,0)-range_tail(&r,pool))*probability);
            }
            if(e!=DM_EL_POIS && c->es_pct>0 && c->es_ratio>0) {
                fraction=sim_clamp(c->es_pct,0,95)/100.0;cap=f->mana*16/c->es_ratio/p;
                double consumed=range_tail(&r,pool)-shield_tail(&r,pool,fraction,cap,0);
                f->mana=fmax(0,f->mana-consumed*c->es_ratio/16*probability);
            }
        }
        double flat=e==DM_EL_PHYS?c->dr_flat:e==DM_EL_POIS?0:c->mdr_flat;
        int penalty=e==DM_EL_PHYS || e==DM_EL_MAG?0:d->diff[c->diff].rp;
        int maximum=e==DM_EL_PHYS?50:sim_clamp(c->maxres[e],0,95);
        double resist=1-sim_clamp(c->res[e]+penalty,-100,maximum)/100.0;
        double after_resist=resist*shield_tail(&r,pool,fraction,cap,flat);
        if(e==DM_EL_PHYS){total+=after_resist;continue;}
        double coefficient=resist*(1-sim_clamp(c->absorb_pct[e],0,40)/100.0);
        double damage=coefficient*shield_tail(&r,pool,fraction,cap,flat+fmax(0,c->absorb_flat[e])/coefficient);
        total+=damage;*healed+=after_resist-damage;
    }
    return total;
}
static double incoming(const dm_sim_data *d,const dm_sim_char *c,const dm_sim_target *t,const dm_sim_tmod *m,
                       fight *f,int maximum,double probability,double *healed) {
    if(maximum==0) {
        double total=0,critical=sim_clamp(t->crit,0,100)/100.0;
        double mana=0,bone=0,cyclone=0,ward=0;
        for(int cr=0;cr<2;cr++)for(int mask=0;mask<8;mask++) {
            double weight=cr?critical:1-critical;dm_sim_target branch=*t;
            for(int k=0;k<3;k++) {
                double p=sim_clamp(t->elpct[k],0,100)/100.0;weight*=mask&(1<<k)?p:1-p;
                branch.elpct[k]=mask&(1<<k)?100:0;
            }
            if(weight<=0)continue;
            fight state;if(f)state=*f;double heal=0;
            total+=weight*incoming_mean(d,c,&branch,m,f?&state:NULL,cr,probability,&heal);*healed+=weight*heal;
            if(f){mana+=weight*state.mana;bone+=weight*state.bone;cyclone+=weight*state.cyclone;ward+=weight*state.ward;}
        }
        if(f){f->mana=mana;f->bone=bone;f->cyclone=cyclone;f->ward=ward;}
        return total;
    }
    double total=0;
    double crit=maximum==3 || (maximum==1 && t->crit>0)?2:1;
    for(int e=0;e<DM_EL_N;e++) {
        double exact;
        if(e==DM_EL_PHYS && maximum==1 && incoming_physical(d,c,t,m,f,crit>1,1,&exact)){total+=exact;continue;}
        double raw=e==0?(maximum==1?t->dmax:(t->dmin+t->dmax)/2.0):0;
        for(int k=0;k<3;k++)if(t->eltype[k]==e && t->elpct[k]>0)
            raw+=maximum==1?t->elmax[k]:(t->elmin[k]+t->elmax[k])/2.0*t->elpct[k]/100;
        raw*=crit*fmax(0,100+m->dmg_pct)/100;
        if(t->flags&DM_MF_PRIME && c->mercenary)raw*=(c->mercenary==2?d->diff[c->diff].pevspet:d->diff[c->diff].pevsmerc)/100.0;
        if(f) {
            double *pool=e==0?&f->bone:e>=1 && e<=3?&f->cyclone:e==DM_EL_MAG?&f->ward:NULL;
            double take=pool?fmin(*pool/fmax(probability,.0001),raw):0;if(pool)*pool-=take*probability;raw-=take;
            if(e!=DM_EL_POIS && c->es_pct>0 && c->es_ratio>0) {
                take=fmin(raw*sim_clamp(c->es_pct,0,95)/100.0,f->mana*16/c->es_ratio/fmax(probability,.0001));
                f->mana-=take*c->es_ratio/16*probability;raw-=take;
            }
        }
        total+=reduction(d,c,e,raw,healed);
    }
    return total;
}
void sim_defense_hit(const dm_sim_data *d,const dm_sim_char *c,const dm_sim_target *t,const dm_sim_tmod *m,double *land) {
    (void)d;*land=dm_sim_cth((int)(t->ar*fmax(0,100+m->ar_pct)/100),c->def,t->mlvl,c->clvl)/100.0;
    *land*=(1-sim_clamp(c->block,0,75)/100.0)*(1-sim_clamp(c->dodge,0,100)/100.0);
}


typedef struct {int generation;double total,heal;} hit_memo;
static double incoming_memo(const dm_sim_data *d,const dm_sim_char *c,const dm_sim_target *t,const dm_sim_tmod *m,fight *f,double land,double *heal,
                            hit_memo *memo,int generation) {
    if((c->es_pct>0 && c->es_ratio>0) || f->bone>0 || f->cyclone>0 || f->ward>0)return incoming(d,c,t,m,f,0,land,heal);
    if(memo->generation!=generation) {
        double h=0;memo->total=incoming(d,c,t,m,f,0,land,&h);memo->heal=h;memo->generation=generation;*heal+=h;return memo->total;
    }
    double critical=sim_clamp(t->crit,0,100)/100.0,mana=0;
    for(int cr=0;cr<2;cr++)for(int mask=0;mask<8;mask++) {
        double weight=cr?critical:1-critical;
        for(int k=0;k<3;k++){double p=sim_clamp(t->elpct[k],0,100)/100.0;weight*=mask&(1<<k)?p:1-p;}
        if(weight>0)mana+=weight*f->mana;
    }
    f->mana=mana;f->bone=f->cyclone=f->ward=0;*heal+=memo->heal;return memo->total;
}
static void advance(const dm_sim_data *d,const dm_sim_char *c,const dm_sim_target *t,fight *f,int end,const double *pets,double poison_uptime,int finite) {
    int dt=end-f->frame;if(dt<=0)return;
    int regen=d->cls[c->cls].manaregen;
    double mr=regen>0?c->mana/(double)regen*(100+c->manaregen_pct)/100:0;
    f->mana=fmin(c->mana,f->mana+mr*dt/25);
    if(f->mana_end>f->frame)f->mana=fmin(c->mana,f->mana+f->mana_rate*fmin(dt,f->mana_end-f->frame));
    double dmg=0;
    for(int e=0;e<DM_EL_N;e++){double value=pets[e]*dt/25;dmg+=value;f->damage_element[e]+=value;}
    for(int i=0;i<f->nburn;i++)if(f->burn_end[i]>f->frame){double value=f->burn[i]*fmin(dt,f->burn_end[i]-f->frame);dmg+=value;f->damage_element[f->burn_element[i]]+=value;}
    if(f->poison_end>f->frame){double value=f->poison*fmin(dt,f->poison_end-f->frame);dmg+=value;f->damage_element[DM_EL_POIS]+=value;}
    if(f->ow_end>f->frame){double value=f->ow*f->owprob*fmin(dt,f->ow_end-f->frame);dmg+=value;f->damage_element[DM_EL_PHYS]+=value;}
    f->damage+=dmg;
    if(finite) {
        double reg=t->hp*t->regen/4096.0;
        if(f->prevent || f->poison_end>f->frame || f->ow_end>f->frame)reg=0;
        else reg*=1-poison_uptime;
        double net=dmg-reg*dt;
        if(net>0 && f->target<=net && f->kill<0)f->kill=f->frame+(int)ceil(dt*f->target/net);
        f->target=fmin(t->hp,f->target-net);
    }
    f->life=fmin(c->life,f->life+c->liferegen*dt/256.0);
    if(f->heal_end>f->frame)f->life=fmin(c->life,f->life+f->heal_rate*fmin(dt,f->heal_end-f->frame));
    f->frame=end;
}

typedef struct {int at,index,slot;} event;
static void enqueue(event *q,int *n,event e) {
    int i=(*n)++;while(i && q[(i-1)/2].at>e.at){q[i]=q[(i-1)/2];i=(i-1)/2;}q[i]=e;
}
static event dequeue(event *q,int *n) {
    event e=q[0],last=q[--*n];int i=0;
    while(i*2+1<*n){int child=i*2+1;if(child+1<*n && q[child+1].at<q[child].at)child++;if(last.at<=q[child].at)break;q[i]=q[child];i=child;}
    q[i]=last;return e;
}
static void refresh(const dm_sim_data *d,const dm_sim_char *base,const dm_sim_target *t,const dm_sim_tmod *initial,
                    const dm_sim_skill_out *original,int n,const int *active,dm_sim_char *c,dm_sim_tmod *m,dm_sim_skill_out *a) {
    *c=*base;*m=*initial;int bad=0;
    for(int i=0;i<n;i++)if(!active[i] && original[i].flags&DM_SF_MODIFIER)sim_modify(d,c,m,original[i].skill,-1,&bad);
    for(int i=0;i<n;i++) {sim_action(d,c,t,m,original[i].skill,&a[i]);if(bad)a[i].flags|=DM_SF_UNAVAIL;}
}



static int life_settled(const dm_sim_char *c,const dm_sim_tmod *m,const fight *f,const dm_sim_skill_out *a,int n,const int *active,const int *expires,const int *renewing) {
    if(c->ll || m->lifetap>0 || (c->es_pct>0 && c->es_ratio>0) || c->bone_pool || c->cyclone_pool || c->ward_pool || f->bone>0 || f->cyclone>0 || f->ward>0)return 0;
    for(int i=0;i<n;i++)if(a[i].flags&DM_SF_MODIFIER && (!active[i] || expires[i]>0 || renewing[i]))return 0;
    return 1;
}





static double damage_bound(const dm_sim_data *d,const dm_sim_char *base,const dm_sim_char *c,const dm_sim_target *t,const fight *f,
                           const dm_sim_skill_out *a,const dm_sim_skill_out *original,int n,const event *q,int nq,const dm_sim_skill_out *cast,int horizon) {
    int rest=horizon-f->frame;if(rest<0)rest=0;
    double total=0,free_rate=0,paid_rate=0,per_mana=0,pets=0,ow=0;int longest=1,stoppable=f->prevent || f->poison_end>f->frame || f->ow_end>f->frame || (c->prevent_heal && !(t->flags&DM_MF_NOPMH));
    for(int k=0;k<nq;k++){const dm_sim_skill_out *v=&cast[q[k].slot];total+=sim_sum(v->hit)+fmax(0,v->dot)+fmax(0,v->burn);}
    for(int i=0;i<f->nburn;i++)if(f->burn_end[i]>f->frame)total+=f->burn[i]*(fmin(f->burn_end[i],horizon)-f->frame);
    if(f->poison_end>f->frame)total+=f->poison*(fmin(f->poison_end,horizon)-f->frame);
    if(f->ow_end>f->frame)total+=f->ow*f->owprob*(fmin(f->ow_end,horizon)-f->frame);
    for(int i=0;i<2*n;i++) {
        const dm_sim_skill_out *v=i<n?&a[i]:&original[i-n];
        if(v->cb_frac>0)return INFINITY;
        if(i<n)pets+=fmax(sim_sum(v->pet_element),sim_sum(original[i].pet_element))/25;
        if(v->dot>0 || v->ow_chance>0 || v->pet_element[DM_EL_POIS]>0)stoppable=1;
        if(v->flags&(DM_SF_UNAVAIL|DM_SF_UTILITY|DM_SF_NOWEAPON))continue;
        int frames=sim_clamp(v->frames,1,10000);if(frames>longest)longest=frames;
        double damage=sim_sum(v->hit)+fmax(0,v->dot)+fmax(0,v->burn);
        if(v->mana_x256>0){paid_rate=fmax(paid_rate,damage/frames);per_mana=fmax(per_mana,damage*256/v->mana_x256);}
        else free_rate=fmax(free_rate,damage/frames);
        if(v->ow_chance>0)ow=fmax(ow,v->ow_rate);
    }
    pets+=fmax(base->struck,c->struck)/(t->cycle>0?t->cycle:30);

    double regen=stoppable?0:t->hp*t->regen/4096.0;
    double by_time=total+(free_rate+paid_rate+ow)*longest+fmax(0,free_rate+paid_rate+ow+pets-regen)*rest;
    if(c->ml>0 && d->diff[c->diff].msd>0)return by_time;
    double most=fmax(base->mana,c->mana),stock=f->mana;
    int k=d->cls[c->cls].manaregen;double mana_rate=k>0?most/k*(100+fmax(base->manaregen_pct,c->manaregen_pct))/100/25:0;
    if(f->mana_end>f->frame)stock+=f->mana_rate*(f->mana_end-f->frame);
    for(int j=DM_POT_MP1;j<DM_POT_N && f->potions;j++)if(f->belt[j]>0) {
        const dm_sim_potion *p=&d->potion[j];
        stock+=f->belt[j]*(p->kind==DM_PK_REJUV?most*p->mana/100.0:p->kind==DM_PK_MANA?p->mana*d->cls[c->cls].mppot/100.0:0);
    }
    double by_mana=total+(free_rate+ow)*longest+per_mana*stock+fmax(0,free_rate+ow+pets+per_mana*mana_rate-regen)*rest;
    return fmin(by_time,by_mana);
}




static double hit_mean(const dm_sim_data *d,const dm_sim_char *c,const dm_sim_target *t,const dm_sim_tmod *m) {
    double land=dm_sim_cth((int)(t->ar*fmax(0,100+m->ar_pct)/100),c->def,t->mlvl,c->clvl)/100.0;
    land*=(1-sim_clamp(c->block,0,75)/100.0)*(1-sim_clamp(c->dodge,0,100)/100.0);
    double heal=0;return incoming(d,c,t,m,NULL,0,land,&heal)*land;
}
static int outlives(const dm_sim_data *d,const dm_sim_char *base,const dm_sim_char *c,const dm_sim_target *t,const dm_sim_tmod *initial,const dm_sim_tmod *m,
                    const dm_sim_skill_out *original,int n,const fight *f,int horizon,int generation,double worst[5]) {
    if(worst[0]<0) {
        worst[0]=hit_mean(d,base,t,initial);worst[1]=base->liferegen;worst[2]=base->life;

        worst[3]=worst[0];worst[4]=0;
        int modifiers=0;for(int i=0;i<n;i++)modifiers|=original[i].flags&DM_SF_MODIFIER;
        if(modifiers) {
            int off[DM_SIM_MAXBAR+1]={0};dm_sim_char bare;dm_sim_tmod bare_mods;dm_sim_skill_out scratch[DM_SIM_MAXBAR+1];
            refresh(d,base,t,initial,original,n,off,&bare,&bare_mods,scratch);
            worst[0]=fmax(worst[0],hit_mean(d,&bare,t,&bare_mods));worst[1]=fmin(worst[1],bare.liferegen);worst[2]=fmin(worst[2],bare.life);
        }
    }
    if(worst[4]!=generation){worst[3]=hit_mean(d,c,t,m);worst[4]=generation;}
    double hit=fmax(worst[0],worst[3]),regen=fmin(worst[1],c->liferegen)/256.0,cycle=t->cycle>0?t->cycle:30;
    int rest=horizon-f->frame;if(rest<0)rest=0;

    return fmin(f->life,fmin(worst[2],c->life))>hit+fmax(0,hit/cycle-regen)*rest;
}


static fight run(const dm_sim_data *d,const dm_sim_char *base,const dm_sim_target *t,const dm_sim_tmod *initial,
                 dm_sim_skill_out *original,int n,int horizon,int finite,int defense,dm_sim_rot_out *out,int joint,int whole,dm_sim_trace *trace) {
    dm_sim_char current=*base;dm_sim_tmod mods=*initial;const dm_sim_char *c=&current;const dm_sim_tmod *m=&mods;
    dm_sim_skill_out a[DM_SIM_MAXBAR+1];memcpy(a,original,(size_t)n*sizeof *a);
    fight f={0}; f.nburn=n;f.life=c->life;f.mana=c->mana;f.target=t->hp;f.dead=f.kill=-1;
    f.bone=c->bone_pool;f.cyclone=c->cyclone_pool;f.ward=c->ward_pool;f.next_enemy=t->cycle>0?t->cycle:30;
    if(potions_on(d,c))for(int k=0;k<DM_POT_N;k++){f.belt[k]=sim_clamp(c->belt[k],0,16);f.potions|=f.belt[k]>0;}

    int held[DM_SIM_MAXBAR+1]={0};
    float v[DM_TS_N];
    if(trace) {
        memset(trace,0,sizeof *trace);trace->width=1;trace->open=1;trace->best=-1;trace->end=horizon;
        trace->nskill=n;for(int i=0;i<n;i++)trace->skill[i]=a[i].skill;
        trace_values(&f,c,t,trace->last_value);
    }
    int used[DM_SIM_MAXBAR+1]={0},ready[DM_SIM_MAXBAR+1]={0},expires[DM_SIM_MAXBAR+1]={0},active[DM_SIM_MAXBAR+1]={0},renewing[DM_SIM_MAXBAR+1]={0};
    event queue[128];int nq=0,nfree=128,free_slot[128];dm_sim_skill_out cast[128];
    for(int i=0;i<128;i++)free_slot[i]=127-i;

    for(int i=0;i<n;i++)if(a[i].pet_dps>0 || a[i].flags&DM_SF_MODIFIER) {
        active[i]=is_active(base,a[i].skill) || a[i].flags&DM_SF_MINION ||
            (a[i].pet_dps<=0 && !(a[i].flags&DM_SF_TIMED));
        expires[i]=a[i].dur;
    }
    int iterations=0,global_ready=0;
#ifdef DM_GEAR_INSTR


    int fi_kind = defense ? FI_SURV : (!joint && whole && finite ? FI_DEALT : -1);
    if (fi_kind == FI_DEALT) { g_npc = 0; g_cp_at = 0; g_dealt_iters = 0; }
    if (fi_kind >= 0) {
        atomic_fetch_add_explicit(&g_fruns[fi_kind], 1, memory_order_relaxed);
        fin_note(fi_kind);
    }
#endif



    int settled=0,full_belt[DM_POT_N];full_belt[0]=-1;

    int no_kill=0,next_check=250,next_life=0;double worst[5]={-1,0,0,0,-1};
    int generation=0;hit_memo memo={-1,0,0};
    while((f.frame<horizon || ((finite || defense) && f.frame==horizon)) && ++iterations<100000) {
        int changed=0;
        for(int i=0;i<n;i++)if(active[i] && expires[i]>0 && expires[i]<=f.frame){active[i]=0;if(a[i].flags&DM_SF_MODIFIER)changed=1;}
        if(changed){refresh(d,base,t,initial,original,n,active,&current,&mods,a);generation++;}
        if(trace && trace->open) {
            if(f.kill>=0){trace_values(&f,c,t,v);v[DM_TS_ENEMY]=0;trace_end(trace,f.kill<f.frame?f.kill:f.frame,v,DM_END_KILL);}
            else {trace_values(&f,c,t,v);trace_point(trace,f.frame,v,f.shortage_now);}
        }
        while(nq && queue[0].at<=f.frame) {
            event e=dequeue(queue,&nq);dm_sim_skill_out *v=&cast[e.slot];int i=e.index;free_slot[nfree++]=e.slot;
            if(renewing[i]) {
                renewing[i]=0;active[i]=1;expires[i]=f.frame+v->dur;
                if(v->flags&DM_SF_MODIFIER) {
                    refresh(d,base,t,initial,original,n,active,&current,&mods,a);generation++;
                    f.bone=fmax(f.bone,c->bone_pool);f.cyclone=fmax(f.cyclone,c->cyclone_pool);f.ward=fmax(f.ward,c->ward_pool);
                }
            }
            double dmg=sim_sum(v->hit);f.damage+=dmg;f.phys+=v->hit[0];
            for(int e=0;e<DM_EL_N;e++)f.damage_element[e]+=v->hit[e];
            if(finite) {
                f.target*=1-v->cb_frac;
                double sf=fmin(f.target*v->sf_frac,fmax(0,f.target-t->hp*v->sf_floor));
                f.target-=sf+dmg;if(f.target<=0 && f.kill<0)f.kill=f.frame;
            }
            if(v->burn>0 && v->burn_frames>0){f.burn_element[i]=v->burn_element;f.burn[i]=v->burn/v->burn_frames;f.burn_end[i]=f.frame+v->burn_frames;}
            if(v->dot>0 && v->dot_frames>0){f.poison=v->dot/v->dot_frames;f.poison_end=f.frame+v->dot_frames;}
            if(v->ow_chance>0){f.ow=v->ow_rate;f.owprob=1-(1-f.owprob)*(1-v->ow_chance);f.ow_end=f.frame+200;}
            if(v->pmh && !(t->flags&DM_MF_NOPMH) && d->skill[v->skill]->fam&DM_FAM_WEAPON && v->cth>0)f.prevent=1;
            if(m->lifetap>0)f.life=fmin(c->life,f.life+v->hit[0]*m->lifetap/100);
            int ls=d->diff[c->diff].lsd, ms=d->diff[c->diff].msd;
            if(ls>0)f.life=fmin(c->life,f.life+v->hit[0]*v->ll/100*t->drain/100/ls);
            if(ms>0)f.mana=fmin(c->mana,f.mana+v->hit[0]*v->ml/100*t->drain/100/ms);
        }
        if(f.kill>=0 && !f.kill_seen){f.kill_seen=1;f.kill_frame=f.frame;f.kill_shortage=f.shortage;f.kill_spent=f.spent;f.kill_mana=f.mana;}
        if(finite && f.kill>=0 && !defense)break;
        if(trace && trace->open && f.kill>=0){trace_values(&f,c,t,v);v[DM_TS_ENEMY]=0;trace_end(trace,f.frame,v,DM_END_KILL);}
        if(finite && f.kill<0 && !no_kill && !whole && f.frame>=next_check) {
            next_check=f.frame+250;

            double floor=f.target;for(int i=0;i<n;i++)if(a[i].sf_frac>0 || original[i].sf_frac>0)floor=fmin(floor,t->hp*fmin(a[i].sf_frac>0?a[i].sf_floor:1,original[i].sf_frac>0?original[i].sf_floor:1));
            if(floor>damage_bound(d,base,c,t,&f,a,original,n,queue,nq,cast,horizon)) {
                no_kill=1;
                if(!defense || f.dead>=0)break;
            }
        }
#ifdef DM_GEAR_INSTR


        if(fi_kind == FI_DEALT && f.kill < 0 && f.frame >= g_cp_at) {
            g_cp_at += 250;
            if(g_npc < 16) g_cp[g_npc++] = (dm_gear_instr_cp){ f.frame, t->hp - f.target,
                                                              damage_bound(d,base,c,t,&f,a,original,n,queue,nq,cast,horizon) };
        }
#endif
        if(joint && !settled && (f.kill>=0 || (no_kill && !(trace && trace->open))) && f.dead<0 && life_settled(c,m,&f,a,n,active,expires,renewing)){settled=1;nq=0;nfree=128;for(int i=0;i<128;i++)free_slot[i]=127-i;f.action=INT_MAX;}

        if(defense && f.dead<0 && (f.kill>=0 || no_kill || !finite) && !(trace && trace->open) && f.frame>=next_life) {
            next_life=f.frame+250;
            if(outlives(d,base,c,t,initial,m,original,n,&f,horizon,generation,worst))break;
        }
        if(defense && f.dead<0 && f.frame>=f.next_enemy) {
            if(settled && f.life==c->life && f.heal_end<=f.frame) {
                if(full_belt[0]>=0 && !memcmp(full_belt,f.belt,sizeof full_belt))break;
                memcpy(full_belt,f.belt,sizeof full_belt);
            }
            double land=dm_sim_cth((int)(t->ar*fmax(0,100+m->ar_pct)/100),c->def,t->mlvl,c->clvl)/100.0;
            land*=(1-sim_clamp(c->block,0,75)/100.0)*(1-sim_clamp(c->dodge,0,100)/100.0);
            double heal=0,dmg=incoming_memo(d,c,t,m,&f,land,&heal,&memo,generation)*land;
            f.life=fmin(c->life,f.life+heal*land)-dmg;
            f.next_enemy+=t->cycle>0?t->cycle:30;
            if(finite && c->struck>0){f.target-=land*c->struck;f.damage+=land*c->struck;if(f.target<=0 && f.kill<0)f.kill=f.frame;}
            if(f.life<=0) {
                f.dead=f.frame;f.spare=dmg>0?land*fmin(1,-f.life/dmg):0;
                if(trace && trace->open){f.life=0;trace_values(&f,c,t,v);trace_end(trace,f.frame,v,DM_END_DEATH);}
                if(!joint || f.kill>=0 || no_kill)break;
            }
        }

        if(trace && trace->open){trace_values(&f,c,t,v);trace_point(trace,f.frame,v,f.shortage_now);}
        if(f.potions && f.dead<0)drink_life(d,c,t,&f,trace);
        if(f.frame>=horizon)break;
        if(!settled && f.frame>=f.action) {
            int pick=-1,want=-1;double best=-1,blocked=-1,affordable=-1;
            for(int i=0;i<n;i++) {
                dm_sim_skill_out *v=&a[i];
                if(v->flags&(DM_SF_UNAVAIL|DM_SF_UTILITY|DM_SF_NOWEAPON))continue;
                if(ready[i]>f.frame || renewing[i] || (v->charges && used[i]>=v->charges))continue;
                const dm_sim_skill *skill=d->skill[v->skill];
                if(global_ready>f.frame && (skill->localdelay || skill->globaldelay))continue;
                double val=score(v);
                if(c->ordered && val>0)val=1e6-i;
                if(!c->ordered && v->cooldown>v->frames)val*=v->cooldown/(double)v->frames;
                if(v->flags&DM_SF_MODIFIER || v->pet_dps>0) {
                    int pool_empty=defense && v->flags&DM_SF_MODIFIER &&
                        ((!strcmp(skill->aurastate,"bonearmor") && f.bone<1.0/256) ||
                         (!strcmp(skill->aurastate,"cyclonearmor") && f.cyclone<1.0/256) ||
                         (!strcmp(skill->aurastate,"psychicward") && f.ward<1.0/256));
                    int early=(v->flags&DM_SF_TIMED) && expires[i]>0 && f.frame+v->frames>=expires[i];
                    if(!pool_empty && !early && active[i])continue;
                    val=1e12;
                } else if(v->sf_frac>0 && finite && f.target>t->hp*v->sf_floor+1)val+=f.target*v->sf_frac*25/fmax(1,v->frames);
                else if(v->sf_frac>0 && val==0)continue;

                int damage=!(v->flags&DM_SF_MODIFIER) && v->pet_dps<=0;double cost=v->mana_x256/256.0;
                if(damage && val>0 && cost<=c->mana){if(cost>f.mana){if(val>blocked){blocked=val;want=i;}}else affordable=fmax(affordable,val);}
                if(val>best && cost<=f.mana){pick=i;best=val;}
            }
            f.shortage_now=blocked>affordable;
            if(f.shortage_now && trace && trace->open)held[want]++;
            if(pick>=0) {
                dm_sim_skill_out *v=&a[pick];double cost=v->mana_x256/256.0;f.mana-=cost;f.spent+=cost;
                f.action=f.frame+sim_clamp(v->frames,1,10000);ready[pick]=f.frame+v->cooldown;used[pick]++;
                if(d->skill[v->skill]->globaldelay)global_ready=f.frame+d->skill[v->skill]->globaldelay;
                int at=f.frame+sim_clamp(v->action_frame,1,v->frames)+v->travel;
                if(v->pet_dps>0 || v->flags&DM_SF_MODIFIER){renewing[pick]=1;at=f.action;}
                if(nq>=128){f.failed=1;if(out)out->flags|=DM_SF_UNAVAIL;break;}
                int slot=free_slot[--nfree];cast[slot]=*v;event e={at,pick,slot};enqueue(queue,&nq,e);
                if(trace && trace->open && pick<64)trace->b[f.frame/trace->width].acts|=1ull<<pick;
                if(out && out->nseq<DM_SIM_MAXBAR) {
                    int seen=0;for(int k=0;k<out->nseq;k++)if(out->seq[k]==v->skill)seen=1;
                    if(!seen)out->seq[out->nseq++]=v->skill;
                }
            } else {
                int wait=25;
                for(int i=0;i<n;i++)if(ready[i]>f.frame && ready[i]-f.frame<wait)wait=ready[i]-f.frame;
                for(int i=0;i<n;i++)if(active[i] && a[i].flags&DM_SF_TIMED) {
                    int until=expires[i]-a[i].frames-f.frame;
                    if(until>0 && until<wait)wait=until;
                }
                f.action=f.frame+wait;
                int expensive=0;for(int i=0;i<n;i++)if(a[i].mana_x256/256.0>f.mana)expensive=1;
                if(expensive)f.shortage+=wait;
            }
            if(f.potions && f.dead<0)drink_mana(d,c,t,&f,trace);
        }
        if(trace && trace->open){trace_values(&f,c,t,v);trace_point(trace,f.frame,v,f.shortage_now);}
        int next=!settled && f.action<horizon?f.action:horizon;
        if(nq && queue[0].at<next)next=queue[0].at;
        for(int i=0;i<n;i++)if(active[i] && expires[i]>f.frame && expires[i]<next)next=expires[i];
        if(defense && f.dead<0 && f.next_enemy<next)next=f.next_enemy;
        if(f.heal_end>f.frame && f.heal_end<next)next=f.heal_end;
        if(f.mana_end>f.frame && f.mana_end<next)next=f.mana_end;
        for(int i=0;i<n;i++)if(f.burn_end[i]>f.frame && f.burn_end[i]<next)next=f.burn_end[i];
        if(f.poison_end>f.frame && f.poison_end<next)next=f.poison_end;
        if(f.ow_end>f.frame && f.ow_end<next)next=f.ow_end;
        if(next<=f.frame)next=f.frame+1;
        double pets[DM_EL_N]={0};for(int i=0;i<n;i++)if(active[i])for(int e=0;e<DM_EL_N;e++)pets[e]+=a[i].pet_element[e];
        double no_poison=1;for(int i=0;i<n;i++)if(active[i] && a[i].pet_element[DM_EL_POIS]>0)no_poison*=1-a[i].pet_poison_uptime;
        advance(d,c,t,&f,next,pets,1-no_poison,finite);
        if(f.ow_end<=f.frame)f.owprob=0;
        if(joint && (f.kill>=0 || no_kill) && f.dead>=0)break;
    }
    if(iterations>=100000)f.failed=1;
    if(f.kill>=0 && !f.kill_seen){f.kill_seen=1;f.kill_frame=f.frame;f.kill_shortage=f.shortage;f.kill_spent=f.spent;f.kill_mana=f.mana;}
    if(trace && trace->open) {
        trace_values(&f,c,t,v);
        if(f.kill>=0){v[DM_TS_ENEMY]=0;trace_end(trace,f.kill,v,DM_END_KILL);}else trace_end(trace,f.frame,v,DM_END_NONE);
    }
    if(trace){int most=-1;for(int i=0;i<n;i++)if(held[i] && (most<0 || held[i]>held[most]))most=i;trace->best=most<0?-1:a[most].skill;}
#ifdef DM_GEAR_INSTR
    if(fi_kind >= 0) {
        atomic_fetch_add_explicit(&g_fiters[fi_kind], iterations, memory_order_relaxed);
        atomic_fetch_add_explicit(&g_fframes[fi_kind], f.frame, memory_order_relaxed);
        if (fi_kind == FI_DEALT) { g_dealt_kill = f.kill; g_dealt_frame = f.frame; g_dealt_iters = iterations; }
    }
#endif
    return f;
}
static void steady_rates(const dm_sim_data *d,const dm_sim_char *c,dm_sim_skill_out *a,int n,dm_sim_rot_out *o) {
    double rate[DM_SIM_MAXBAR+1]={0},uptime[DM_SIM_MAXBAR+1]={0};
    double available=1;int filler=-1,delayed[DM_SIM_MAXBAR+1],ndelay=0;
    o->nseq=0;
    for(int i=0;i<n;i++) {
        if(a[i].flags&(DM_SF_UNAVAIL|DM_SF_UTILITY|DM_SF_NOWEAPON))continue;
        if(a[i].pet_dps>0 || a[i].flags&DM_SF_MODIFIER) {
            uptime[i]=1;
            if(a[i].dur>0 && !(a[i].flags&DM_SF_MINION)) {
                rate[i]=25.0/(a[i].dur+a[i].frames);uptime[i]=a[i].dur/(double)(a[i].dur+a[i].frames);
                if(a[i].flags&DM_SF_TIMED){rate[i]=25.0/a[i].dur;uptime[i]=1;}
                available-=rate[i]*a[i].frames/25;
            }
        } else if(score(&a[i])>0 || (n==1 && a[i].sf_frac>0)) {
            if(a[i].cooldown>a[i].frames)delayed[ndelay++]=i;
            else if(filler<0 || (!c->ordered && score(&a[i])>score(&a[filler])))filler=i;
        }
    }
    available=fmax(0,available);
    for(int i=0;i<ndelay;i++)for(int j=i+1;j<ndelay;j++) {
        int x=delayed[i],y=delayed[j];
        if(!c->ordered && score(&a[y])*a[y].cooldown/fmax(1,a[y].frames)>score(&a[x])*a[x].cooldown/fmax(1,a[x].frames)) {
            delayed[i]=y;delayed[j]=x;
        }
    }
    for(int k=0;k<ndelay;k++) {
        int i=delayed[k],period=a[i].cooldown;
        if(filler>=0 && (c->ordered ? i>filler : score(&a[i])*a[i].cooldown/fmax(1,a[i].frames)<=score(&a[filler])))continue;
        if(filler>=0)period=a[i].frames+(int)ceil((period-a[i].frames)/(double)a[filler].frames)*a[filler].frames;
        rate[i]=fmin(25.0/fmax(1,period),available*25/fmax(1,a[i].frames));available-=rate[i]*a[i].frames/25;
    }
    if(filler>=0)rate[filler]=available*25/fmax(1,a[filler].frames);
    memset(o->dps_element,0,sizeof o->dps_element);o->phys_dps=o->ll_dps=o->ml_dps=0;
    double cost=0,maxcost=0,poison=0,ow_miss=1,ow_rate=0;
    int contributors=0;
    for(int i=0;i<n;i++) {
        double r=rate[i];
        if(r>0 || uptime[i]>0) {
            cost+=r*a[i].mana_x256/256.0;
            if(r>0)maxcost=fmax(maxcost,a[i].mana_x256/256.0);
            contributors++;
        }
        for(int e=0;e<DM_EL_N;e++)o->dps_element[e]+=a[i].hit[e]*r+a[i].pet_element[e]*uptime[i];
        o->phys_dps+=a[i].hit[0]*r;o->ll_dps+=a[i].hit[0]*r*a[i].ll;o->ml_dps+=a[i].hit[0]*r*a[i].ml;
        if(a[i].dot_frames>0)poison=fmax(poison,a[i].dot*25/a[i].dot_frames*fmin(1,r*a[i].dot_frames/25));
        if(a[i].burn_frames>0)o->dps_element[a[i].burn_element]+=a[i].burn*25/a[i].burn_frames*fmin(1,r*a[i].burn_frames/25);
        if(a[i].ow_chance>0 && r>0) {
            double count=8*r,whole=floor(count),p=sim_clamp((int)round(a[i].ow_chance*10000),0,10000)/10000.0;
            ow_rate=fmax(ow_rate,a[i].ow_rate);ow_miss*=pow(1-p,whole)*(1-(count-whole)*p);
        }
    }
    for(int i=0;i<n;i++)if(uptime[i]>0 && !(a[i].flags&DM_SF_MINION) && o->nseq<DM_SIM_MAXBAR)o->seq[o->nseq++]=a[i].skill;
    for(int k=0;k<ndelay;k++)if(rate[delayed[k]]>0 && o->nseq<DM_SIM_MAXBAR)o->seq[o->nseq++]=a[delayed[k]].skill;
    if(filler>=0 && rate[filler]>0 && o->nseq<DM_SIM_MAXBAR)o->seq[o->nseq++]=a[filler].skill;
    o->ow_dps=ow_rate*25*(1-ow_miss);o->dps_element[DM_EL_PHYS]+=o->ow_dps;
    o->dps_element[DM_EL_POIS]+=poison;o->dps=sim_sum(o->dps_element);
    double regen=d->cls[c->cls].manaregen>0?c->mana/(double)d->cls[c->cls].manaregen*(100+c->manaregen_pct)/100:0;
    o->mana_rate=cost-regen;
    o->mana_seconds=maxcost>c->mana?0:o->mana_rate>0?c->mana/o->mana_rate:-1;

    if(contributors>1 && (ndelay>1 || available<1))o->flags|=DM_SF_ESTIMATE;
}
static double single_kill(const dm_sim_data *d,const dm_sim_char *c,const dm_sim_target *t,const dm_sim_skill_out *a) {
    (void)d;
    if(!a->roll.known || (a->flags&~(DM_SF_ESTIMATE|DM_SF_LENGTH|DM_SF_IMMUNE)) || a->hits!=1 || a->mana_x256 ||
       a->cb_frac || a->sf_frac || a->dot || a->burn || a->pet_dps || a->ow_chance ||
       c->nactive_buff || c->active_aura>=0 || t->regen || t->kind==DM_TK_STD ||
       a->frames<1 || a->action_frame<1 || a->cooldown<a->frames || a->travel<0)return -2;
    double raw,hit;
    if(sim_roll_mean(&a->roll,&raw,&hit) || fabs(raw-sim_sum(a->raw))>1e-9 || fabs(hit-sim_sum(a->hit))>1e-9)return -2;
    double count=sim_kill_attempts(&a->roll,(int64_t)t->hp*256);
    if(count<0)return count;
    int offset=sim_clamp(a->action_frame,1,a->frames)+a->travel;
    return (offset+(count-1)*a->cooldown)/25;
}


static double defense_stats(const dm_sim_data *d,const dm_sim_char *c,const dm_sim_target *t,const dm_sim_tmod *m,const dm_sim_rot_out *o,dm_sim_surv_out *surv) {
        memset(surv,0,sizeof *surv);
        double heal=0;surv->cth=dm_sim_cth((int)(t->ar*fmax(0,100+m->ar_pct)/100),c->def,t->mlvl,c->clvl)/100.0;
        surv->p_land=surv->cth*(1-sim_clamp(c->block,0,75)/100.0)*(1-sim_clamp(c->dodge,0,100)/100.0);
        surv->dmg_hit=incoming(d,c,t,m,NULL,0,1,&heal);double maxheal=0;
        fight shields={0};shields.bone=c->bone_pool;shields.cyclone=c->cyclone_pool;shields.ward=c->ward_pool;shields.mana=c->mana;
        surv->max_hit=incoming(d,c,t,m,&shields,1,1,&maxheal)/fmax(1,c->life);
        surv->in_dps=surv->dmg_hit*surv->p_land*25/fmax(t->cycle,1);
        surv->sustain=c->liferegen*25/256.0+heal*surv->p_land*25/fmax(t->cycle,1);
        surv->sustain+=o->phys_dps*m->lifetap/100;
        int ls=d->diff[c->diff].lsd;if(ls>0)surv->sustain+=(c->nhands==2?o->ll_dps:o->phys_dps*c->ll)/100*t->drain/100/ls;
        return heal;
}
static void defense_finish(const dm_sim_target *t,dm_sim_surv_out *surv) {
        surv->hits_to_death=surv->ttd<0?-1:surv->ttd*25/fmax(t->cycle,1)*surv->p_land;
        surv->underestimated|=t->ranged_share>0 || (t->flags&DM_MF_UNDERESTIMATED) || !t->dmax;
        int modeled=t->dmax>0;
        for(int k=0;k<3;k++)if(t->elpct[k]>0 && t->elmax[k]>0)modeled=1;
        if(!modeled && surv->ttd<0)surv->ttd=-2;
        if(surv->ttd==-2)surv->hits_to_death=-2;
}
static void defense_result(const dm_sim_data *d,const dm_sim_char *c,const dm_sim_target *t,const dm_sim_tmod *m,
                           dm_sim_skill_out *a,int n,const dm_sim_rot_out *o,dm_sim_surv_out *surv) {
        double heal=defense_stats(d,c,t,m,o,surv);
        int timed=0;for(int i=0;i<n;i++)if(a[i].flags&DM_SF_MODIFIER && a[i].dur>0)timed=1;
        if(!potions_on(d,c) && !timed && !c->ll && !m->lifetap && !c->es_pct && !c->bone_pool && !c->cyclone_pool && !c->ward_pool) {
            double hit=surv->dmg_hit*surv->p_land;
            double restore=c->liferegen*fmax(t->cycle,1)/256.0+heal*surv->p_land;
            double count=hit>=c->life?1:hit>restore?1+ceil((c->life-hit)/(hit-restore)):-1;
            surv->ttd=count<0?-1:count*fmax(t->cycle,1)/25;
            if(count>0 && hit>0)surv->spare=surv->p_land*fmin(1,fmax(0,-(c->life-hit-(count-1)*(hit-restore))/hit));
            if(surv->ttd>600){surv->ttd=-1;surv->spare=0;}
        } else {fight defense=run(d,c,t,m,a,n,DM_SIM_HORIZON,0,1,NULL,0,0,NULL);surv->ttd=defense.failed?-2:defense.dead<0?-1:defense.dead/25.0;if(surv->ttd>=0)surv->spare=defense.spare;if(defense.failed)surv->underestimated=1;}
        defense_finish(t,surv);
}
static void steady(const dm_sim_data *d,const dm_sim_char *c,const dm_sim_target *t,dm_sim_skill_out *a,int n,dm_sim_rot_out *o) {
    steady_rates(d,c,a,n,o);
    int ms=d->diff[c->diff].msd;
    if(ms>0)o->mana_rate-=(c->nhands==2?o->ml_dps:o->phys_dps*c->ml)/100*t->drain/100/ms;
    if(o->mana_seconds!=0)o->mana_seconds=o->mana_rate>0?c->mana/o->mana_rate:-1;
    o->mana_short=0;o->mana_spent=0;o->mana_end=c->mana;
}
void sim_timeline(const dm_sim_data *d,const dm_sim_char *c,const dm_sim_target *t,const dm_sim_tmod *m,
                  dm_sim_skill_out *a,int n,dm_sim_rot_out *o,dm_sim_surv_out *surv) {
    steady(d,c,t,a,n,o);
    double exact=n==1?single_kill(d,c,t,a):-2;
    if(exact>=-1) {
        o->ttk=exact;
        double duration=exact<0?600:exact;
        o->mana_end=fmax(0,c->mana-fmax(0,o->mana_rate)*duration);
        if(o->mana_seconds>=0 && o->mana_seconds<duration)o->mana_short=1-o->mana_seconds/fmax(duration,1);
    } else {
        fight resources=run(d,c,t,m,a,n,DM_SIM_HORIZON,0,0,NULL,0,0,NULL);
        o->ttk=-2;
        if(resources.failed)o->flags|=DM_SF_UNAVAIL;
        o->mana_short=resources.shortage/fmax(1,resources.frame);o->mana_spent=resources.spent;o->mana_end=resources.mana;
    }
    if(surv)defense_result(d,c,t,m,a,n,o,surv);
}

static void grant_triggers(const dm_sim_data *d,const dm_sim_char *c,const dm_sim_target *t,const dm_sim_tmod *m,dm_sim_skill_out *o) {
    if(!c->ngrant || o->flags&(DM_SF_UNAVAIL|DM_SF_MODIFIER|DM_SF_UTILITY|DM_SF_NOWEAPON) || !(d->skill[o->skill]->fam&DM_FAM_WEAPON))return;
    int bad=0,added=0;double hits=o->hits>0?o->hits:1;
    for(int i=0;i<c->ngrant;i++) {
        const dm_sim_grant *g=&c->grant[i];
        if(g->kind!=DM_GR_ATTACK && g->kind!=DM_GR_HIT)continue;
        double casts=sim_clamp(g->chance,0,100)/100.0*hits*(g->kind==DM_GR_HIT?o->cth:1);
        if(casts<=0)continue;
        dm_sim_skill_out x;dm_sim_char *y=malloc(sizeof *y);
        if(!y){o->flags|=DM_SF_ESTIMATE;continue;}
        *y=*c;y->ngrant=0;y->lvl[g->skill]=(int16_t)g->lvl;
        if(g->skill<0 || g->skill>=DM_SIM_MAXSKILL || !d->skill[g->skill] || sim_action(d,y,t,m,g->skill,&x) || x.flags&(DM_SF_UNAVAIL|DM_SF_MODIFIER|DM_SF_UTILITY|DM_SF_NOWEAPON))bad=1;
        else {
            added=1;
            for(int e=0;e<DM_EL_N;e++){o->hit[e]+=casts*x.hit[e];o->raw[e]+=casts*x.raw[e];}
            o->dot+=casts*x.dot;o->burn+=casts*x.burn;
            if(x.burn>0 && !o->burn_frames){o->burn_frames=x.burn_frames;o->burn_element=x.burn_element;}
            if(x.dot>0 && !o->dot_frames)o->dot_frames=x.dot_frames;
            if(x.pet_dps>0 || x.sf_frac>0 || x.cb_frac>0)bad=1;
        }
        free(y);
    }

    if(added)o->roll.known=0;
    if(bad)o->flags|=DM_SF_ESTIMATE;
}

static int grant_damage_auras(const dm_sim_data *d,const dm_sim_char *c,const dm_sim_target *t,const dm_sim_tmod *m,const dm_sim_rot_out *r,dm_sim_skill_out *a,int n) {
    for(int i=0;i<c->ngrant && n<=DM_SIM_MAXBAR;i++) {
        int id=c->grant[i].skill,lvl=grant_level(c,id,0),at=-1;
        if(c->grant[i].kind!=DM_GR_AURA || id<0 || id>=DM_SIM_MAXSKILL || !d->skill[id] || !(d->skill[id]->fam&DM_FAM_ADMG) || !d->skill[id]->aura)continue;
        for(int k=0;k<n;k++)if(a[k].skill==id)at=k;
        if(at>=0 && (id!=r->aura || c->lvl[id]>=lvl))continue;
        dm_sim_char *x=malloc(sizeof *x);if(!x)continue;
        *x=*c;x->lvl[id]=(int16_t)lvl;
        dm_sim_skill_out o;sim_action(d,x,t,m,id,&o);free(x);
        if(o.flags&DM_SF_UNAVAIL)continue;
        double hz=25.0/fmax(1,o.cooldown);
        for(int e=0;e<DM_EL_N;e++){o.pet_element[e]+=o.hit[e]*hz;o.hit[e]=0;}
        o.pet_dps=sim_sum(o.pet_element);o.dur=0;o.mana_x256=0;
        a[at>=0?at:n++]=o;
    }
    return n;
}
static int grant_charges(const dm_sim_char *c,int id) {
    for(int i=0;i<c->ngrant;i++)if(c->grant[i].kind==DM_GR_CHARGE && c->grant[i].skill==id && c->grant[i].charges>0)return c->grant[i].charges;
    return 0;
}

static void grant_action(const dm_sim_data *d,const dm_sim_char *c,const dm_sim_target *t,const dm_sim_tmod *m,int id,dm_sim_skill_out *o) {
    if(!c->ngrant)return;
    grant_triggers(d,c,t,m,o);
    int charges=c->blvl[id]?0:grant_charges(c,id);
    if(charges){o->charges=charges;o->mana_x256=0;}
}
static int actions(const dm_sim_data *d,const dm_sim_char *c,const dm_sim_target *t,const dm_sim_tmod *m,dm_sim_rot_out *r,dm_sim_skill_out *a) {
    int n=0,skipped=0,offense=0;double fill=-1,cd=-1;
    for(int i=0;i<=c->nbar;i++) {
        int id=c->ordered?(i<c->nbar?c->bar[i]:0):(i?c->bar[i-1]:0);if(id<0 || id>=DM_SIM_MAXSKILL || !d->skill[id] || (!c->lvl[id] && id))continue;
        int dup=0;for(int k=0;k<n;k++)if(a[k].skill==id)dup=1;if(dup)continue;
        const dm_sim_skill *s=d->skill[id];
        if(s->aura && id!=r->aura)continue;
        if(s->fam&DM_FAM_K && id!=r->curse && !(s->fam&DM_FAM_SPELL))continue;
        if(s->fam&DM_FAM_PA)continue;
        if(s->fam&DM_FAM_B && !s->timed && !is_active(c,id))continue;
        sim_action(d,c,t,m,id,&a[n]);
        if(a[n].flags&DM_SF_UNAVAIL){r->flags|=DM_SF_ESTIMATE;skipped=1;continue;}
        grant_action(d,c,t,m,id,&a[n]);
        if(s->fam&(DM_FAM_WEAPON|DM_FAM_SPELL|DM_FAM_ADMG|DM_FAM_P|DM_FAM_F|DM_FAM_L))offense=1;
        r->flags|=a[n].flags;
        if(s->fam&DM_FAM_ADMG && s->aura) {
            double hz=25.0/fmax(1,a[n].cooldown);
            for(int e=0;e<DM_EL_N;e++){a[n].pet_element[e]+=a[n].hit[e]*hz;a[n].hit[e]=0;}
            a[n].pet_dps=sim_sum(a[n].pet_element);a[n].dur=0;
        }
        double v=score(&a[n]);
        if(a[n].cooldown>a[n].frames && v>cd){cd=v;r->cooldown_skill=id;}
        else if(v>fill && !(s->fam&(DM_FAM_P|DM_FAM_F|DM_FAM_MODIFIER))){fill=v;r->primary=id;}
        n++;
    }
    if(c->ngrant){int before=n;n=grant_damage_auras(d,c,t,m,r,a,n);if(n>before)offense=1;}
    int trap=-1;double trap_score=-1;
    for(int i=0;i<n;i++)if(d->skill[a[i].skill]->fam&DM_FAM_F && score(&a[i])>trap_score){trap=i;trap_score=score(&a[i]);}
    for(int i=n-1;i>=0;i--)if(i!=trap && d->skill[a[i].skill]->fam&DM_FAM_F){memmove(a+i,a+i+1,(size_t)(n-i-1)*sizeof *a);n--;}
    if(skipped && !offense)r->flags|=DM_SF_UNAVAIL;
    return n;
}
int dm_sim_rotation(const dm_sim_data *d,const dm_sim_char *in,const dm_sim_target *t,dm_sim_rot_out *o) {
    if(!sim_valid_char(d,in) || !t || !o || in->diff!=t->diff)return -1;
    dm_sim_char c;dm_sim_tmod m;sim_prepare(d,in,t,&c,&m,o);
    dm_sim_skill_out a[DM_SIM_MAXBAR+1];int n=actions(d,&c,t,&m,o,a);sim_timeline(d,&c,t,&m,a,n,o,NULL);if(!n){o->flags|=DM_SF_UNAVAIL;o->ttk=-2;}return 0;
}
int dm_sim_fight_block(const dm_sim_data *d,const dm_sim_char *c) {
    dm_sim_target t;if(!d || !c || dm_sim_target_act(d,c,&t)<0)return -1;
    dm_sim_char *x=malloc(sizeof *x);if(!x)return -1;
    dm_sim_tmod m;dm_sim_rot_out r;sim_prepare(d,c,&t,x,&m,&r);
    int block=x->block;free(x);return block;
}
int dm_sim_swap_block(const dm_sim_data *d,const dm_sim_char *after,int claws_after) {
    if(!d || !after)return -1;
    dm_sim_char *x=malloc(sizeof *x);if(!x)return -1;
    *x=*after;int bad=0;
    if(claws_after){x->nhands=2;memcpy(x->hand[0].wclass,"HT2",4);x->block=dm_sim_weapon_block(d,x,&bad);}
    else if(dm_sim_two_claws(x)){x->nhands=1;memcpy(x->hand[0].wclass,"HT1",4);}
    int block=bad?-1:dm_sim_fight_block(d,x);
    free(x);return block;
}
int dm_sim_fight(const dm_sim_data *d,const dm_sim_char *in,const dm_sim_target *t,dm_sim_rot_out *o,dm_sim_surv_out *surv,dm_sim_trace *trace) {
    if(!sim_valid_char(d,in) || !t || !o || !surv || in->diff!=t->diff)return -1;
    if(trace){memset(trace,0,sizeof *trace);trace->width=1;trace->best=-1;}
    dm_sim_char c;dm_sim_tmod m;sim_prepare(d,in,t,&c,&m,o);
    dm_sim_skill_out a[DM_SIM_MAXBAR+1];int n=actions(d,&c,t,&m,o,a);
    steady(d,&c,t,a,n,o);defense_stats(d,&c,t,&m,o,surv);

    int defended=!(in->capture_bad&2);
    fight f=run(d,&c,t,&m,a,n,DM_SIM_HORIZON,1,defended,NULL,defended,0,defended?trace:NULL);
    o->ttk=f.failed?-2:f.kill<0?-1:f.kill/25.0;


    double exact=n==1 && !f.failed?single_kill(d,&c,t,a):-2;
    if(exact>=-1)o->ttk=exact;
    if(f.failed)o->flags|=DM_SF_UNAVAIL;
    int until=f.kill_seen?f.kill_frame:f.frame;
    o->mana_short=(f.kill_seen?f.kill_shortage:f.shortage)/fmax(1,until);o->mana_spent=f.kill_seen?f.kill_spent:f.spent;o->mana_end=f.kill_seen?f.kill_mana:f.mana;
    if(!n){o->flags|=DM_SF_UNAVAIL;o->ttk=-2;}
    if(!defended){memset(surv,0,sizeof *surv);surv->ttd=-2;surv->underestimated=1;surv->hits_to_death=-2;return 0;}
    surv->ttd=f.failed?-2:f.dead<0?-1:f.dead/25.0;if(surv->ttd>=0)surv->spare=f.spare;if(f.failed)surv->underestimated=1;
    defense_finish(t,surv);
    return 0;
}
int dm_sim_dealt(const dm_sim_data *d,const dm_sim_char *in,const dm_sim_target *t,int frames,dm_sim_dealt_out *o) {
    if(!sim_valid_char(d,in) || !t || !o || in->diff!=t->diff || frames<1 || frames>DM_SIM_HORIZON)return -1;
    memset(o,0,sizeof *o);o->frames=frames;
    dm_sim_char c;dm_sim_tmod m;dm_sim_rot_out r;sim_prepare(d,in,t,&c,&m,&r);
    dm_sim_skill_out a[DM_SIM_MAXBAR+1];int n=actions(d,&c,t,&m,&r,a);
    if(!n)return 0;
    fight f=run(d,&c,t,&m,a,n,frames,1,0,NULL,0,1,NULL);
    if(f.failed)return -1;
    o->life=fmin(t->hp,t->hp-f.target);o->killed=f.kill>=0;
    if(o->killed)o->frames=f.kill>0?f.kill:1;
    return 0;
}
int dm_sim_survive(const dm_sim_data *d,const dm_sim_char *in,const dm_sim_target *t,const dm_sim_rot_out *rot,dm_sim_surv_out *o) {
    if(!sim_valid_char(d,in) || !t || !o || in->diff!=t->diff)return -1;
    if(in->capture_bad&2){memset(o,0,sizeof *o);o->ttd=-2;o->underestimated=1;return 0;}
    dm_sim_char c;dm_sim_tmod m;dm_sim_rot_out r;sim_prepare(d,in,t,&c,&m,&r);
    dm_sim_skill_out a[DM_SIM_MAXBAR+1];int n=actions(d,&c,t,&m,&r,a);
    if(rot)defense_result(d,&c,t,&m,a,n,rot,o);else sim_timeline(d,&c,t,&m,a,n,&r,o);
    return 0;
}
static double best_sustained(const dm_sim_data *d,const dm_sim_char *c,const dm_sim_target *t,const dm_sim_tmod *m) {
    double best=0;
    for(int i=-1;i<c->nbar;i++) {
        int id=i<0?0:c->bar[i];if(!d->skill[id] || d->skill[id]->fam&DM_FAM_MODIFIER)continue;
        dm_sim_skill_out action;if(sim_action(d,c,t,m,id,&action))continue;
        dm_sim_rot_out result={0};steady_rates(d,c,&action,1,&result);if(result.dps>best)best=result.dps;
    }
    return best;
}

static void alone(const dm_sim_data *d,const dm_sim_char *c,const dm_sim_target *t,const dm_sim_tmod *m,dm_sim_skill_out *o) {
    dm_sim_rot_out r={0};sim_timeline(d,c,t,m,o,1,&r,NULL);o->dps=r.dps;o->ow_dps=r.ow_dps;o->ttk=r.ttk;o->mana_seconds=r.mana_seconds;o->flags|=r.flags;memcpy(o->dps_element,r.dps_element,sizeof o->dps_element);
    if(r.mana_short>0)o->flags|=DM_SF_NOMANA;
}






static void solo_rates(const dm_sim_data *d,const dm_sim_char *c,const dm_sim_target *t,const dm_sim_tmod *m,dm_sim_skill_out *o) {
    int F=o->frames>0?(o->frames<10000?o->frames:10000):1;
    if(o->travel+sim_clamp(o->action_frame,1,F)>60*F) { alone(d,c,t,m,o); return; }
    dm_sim_rot_out r={0};
    steady(d,c,t,o,1,&r);
    double exact=single_kill(d,c,t,o);
    if(exact>=-1) {
        r.ttk=exact;double duration=exact<0?600:exact;
        r.mana_end=fmax(0,c->mana-fmax(0,r.mana_rate)*duration);
        if(r.mana_seconds>=0&&r.mana_seconds<duration)r.mana_short=1-r.mana_seconds/fmax(duration,1);
    } else r.ttk=-2;
    o->dps=r.dps;o->ow_dps=r.ow_dps;o->ttk=r.ttk;o->mana_seconds=r.mana_seconds;o->flags|=r.flags;memcpy(o->dps_element,r.dps_element,sizeof o->dps_element);
    if(r.mana_short>0)o->flags|=DM_SF_NOMANA;
}
int dm_sim_skill_eval(const dm_sim_data *d,const dm_sim_char *in,const dm_sim_target *t,int id,dm_sim_skill_out *o) {
    if(!sim_valid_char(d,in) || !t || !o || id<0 || id>=DM_SIM_MAXSKILL || !d->skill[id] || in->diff!=t->diff)return -1;
    dm_sim_char c=*in;dm_sim_tmod m;dm_sim_tmod_init(&m);int bad=0;
    if(c.active_aura>=0 && c.active_aura<DM_SIM_MAXSKILL && d->skill[c.active_aura] && !strcmp(d->skill[c.active_aura]->aurastate,"conviction"))sim_modify(d,&c,&m,c.active_aura,1,&bad);
    int rc=sim_action(d,&c,t,&m,id,o);if(rc)return rc;
    if(o->flags&DM_SF_MODIFIER) {
        double before=best_sustained(d,&c,t,&m);
        dm_sim_char old=c;dm_sim_tmod oldmod=m;
        if(is_active(&c,id))sim_modify(d,&c,&m,id,-1,&bad);
        else sim_modify(d,&c,&m,id,1,&bad);
        double after=best_sustained(d,&c,t,&m);
        dm_sim_skill_out attack_before,attack_after;sim_action(d,&old,t,&oldmod,0,&attack_before);sim_action(d,&c,t,&m,0,&attack_after);
        dm_sim_rot_out rb={0},ra={0};dm_sim_surv_out sb,sa;
        sim_timeline(d,&old,t,&oldmod,&attack_before,1,&rb,&sb);sim_timeline(d,&c,t,&m,&attack_after,1,&ra,&sa);
        double delta=(sa.ttd<0?600:sa.ttd)-(sb.ttd<0?600:sb.ttd);o->survival_delta=is_active(in,id)?-delta:delta;
        o->dps=is_active(in,id)?before-after:after-before;
        if(bad)o->flags|=DM_SF_UNAVAIL;
        return 0;
    }
    alone(d,&c,t,&m,o);
    return 0;
}
int dm_sim_skills_prepared(const dm_sim_data *d,const dm_sim_char *in,const dm_sim_target *t,int n,const int *ids,dm_sim_skill_out *out) {
    if(!sim_valid_char(d,in) || !t || n<0 || (n && (!ids || !out)) || in->diff!=t->diff)return -1;
    dm_sim_char c;dm_sim_tmod m;dm_sim_rot_out r;sim_prepare(d,in,t,&c,&m,&r);
#ifdef DM_GEAR_INSTR
    atomic_fetch_add_explicit(&g_fruns[FI_GROUP], 1, memory_order_relaxed);
    fin_note(FI_GROUP);
#endif
    for(int i=0;i<n;i++) {
        dm_sim_skill_out *o=&out[i];int id=ids[i];
        if(id<0 || id>=DM_SIM_MAXSKILL || !d->skill[id] || sim_action(d,&c,t,&m,id,o)){memset(o,0,sizeof *o);o->skill=id;o->flags=DM_SF_UNAVAIL;continue;}
        grant_action(d,&c,t,&m,id,o);
        if(!(o->flags&DM_SF_MODIFIER))solo_rates(d,&c,t,&m,o);
    }
    return 0;
}

int dm_sim_damage_taken(const dm_sim_data *d,const dm_sim_char *in,const dm_sim_target *t,double out[DM_EL_N]) {
    if(!sim_valid_char(d,in) || !t || !out || (in->capture_bad&2))return -1;
    dm_sim_char c;dm_sim_tmod m;dm_sim_rot_out r;sim_prepare(d,in,t,&c,&m,&r);
    double land=dm_sim_cth((int)(t->ar*fmax(0,100+m.ar_pct)/100),c.def,t->mlvl,c.clvl)/100.0;
    land*=(1-sim_clamp(c.block,0,75)/100.0)*(1-sim_clamp(c.dodge,0,100)/100.0);
    for(int e=0;e<DM_EL_N;e++) {
        dm_sim_target one=*t;if(e){one.dmin=one.dmax=0;}
        for(int k=0;k<3;k++)if(one.eltype[k]!=e)one.elpct[k]=0;
        double heal=0;out[e]=incoming(d,&c,&one,&m,NULL,0,land,&heal)*land*25/fmax(1,t->cycle);
    }
    return 0;
}

void sim_grants_apply(const dm_sim_data *d,const dm_sim_char *in,dm_sim_char *c,dm_sim_tmod *m,int *bad) {if(in->ngrant)grants_on(d,in,c,m,bad);}

void sim_own_aura(const dm_sim_data *d,dm_sim_char *c,dm_sim_tmod *m,int id,int sign,int *bad) {own_aura(d,c,m,id,sign,bad);}
void sim_grant_action(const dm_sim_data *d,const dm_sim_char *c,const dm_sim_target *t,const dm_sim_tmod *m,int id,dm_sim_skill_out *o) {grant_action(d,c,t,m,id,o);}
double sim_grant_aura_dps(const dm_sim_data *d,const dm_sim_char *c,const dm_sim_target *t,const dm_sim_tmod *m) {
    if(!c->ngrant)return 0;
    dm_sim_rot_out r;memset(&r,0,sizeof r);r.aura=-1;
    dm_sim_skill_out a[DM_SIM_MAXBAR+1];int n=grant_damage_auras(d,c,t,m,&r,a,0);
    double v=0;for(int i=0;i<n;i++)v+=a[i].pet_dps;
    return v;
}
double sim_grant_struck(const dm_sim_data *d,const dm_sim_char *c,const dm_sim_target *t,const dm_sim_tmod *m,int *bad) {
    double v=0;
    for(int i=0;i<c->ngrant;i++)if(c->grant[i].kind==DM_GR_STRUCK)v+=sim_clamp(c->grant[i].chance,0,100)/100.0*grant_cast(d,c,t,m,c->grant[i].skill,c->grant[i].lvl,bad);
    return v;
}
int dm_sim_grant_dps(const dm_sim_data *d,const dm_sim_char *in,const dm_sim_target *t,double *aura,double *struck) {
    *aura=*struck=0;
    if(!sim_valid_char(d,in) || !t || in->diff!=t->diff)return -1;
    if(!in->ngrant)return 0;
    dm_sim_char *c=malloc(sizeof *c);if(!c)return -1;
    dm_sim_tmod m;dm_sim_rot_out r;sim_prepare(d,in,t,c,&m,&r);
    dm_sim_skill_out a[DM_SIM_MAXBAR+1];int n=grant_damage_auras(d,c,t,&m,&r,a,0);
    for(int i=0;i<n;i++)*aura+=a[i].pet_dps;
    if(c->struck>0 && !(in->capture_bad&2)) {
        double land=dm_sim_cth((int)(t->ar*fmax(0,100+m.ar_pct)/100),c->def,t->mlvl,c->clvl)/100.0;
        land*=(1-sim_clamp(c->block,0,75)/100.0)*(1-sim_clamp(c->dodge,0,100)/100.0);
        *struck=c->struck*land*25/(t->cycle>0?t->cycle:30);
    }
    free(c);return 0;
}

static int stat_outside(const dm_sim_data *d,int id) {
    static const char *const outside[]={"velocitypercent","staminarecoverybonus","skill_staminapercent","skill_concentration"};
    if(id<0 || id>=512)return 0;
    for(size_t i=0;i<sizeof outside/sizeof *outside;i++)if(!strcmp(d->stat_name[id],outside[i]))return 1;
    return 0;
}

int dm_sim_grants_modelled(const dm_sim_data *d,const dm_sim_char *in,const dm_sim_target *t) {
    if(!sim_valid_char(d,in) || !t)return 0;
    for(int i=0;i<in->ngrant;i++) {
        const dm_sim_grant *g=&in->grant[i];const dm_sim_skill *s=d->skill[g->skill];
        if(s->unavailable)return 0;
        if(g->kind==DM_GR_AURA && !(s->aura && s->fam&(DM_FAM_AOFF|DM_FAM_ADEF|DM_FAM_ADMG)))return 0;
        if(g->kind==DM_GR_BUFF && !(s->fam&DM_FAM_B))return 0;
        if(g->kind==DM_GR_AURA || g->kind==DM_GR_BUFF) {

            for(int k=0;k<6;k++)if(s->astat[k]>=0 && (s->fam&DM_FAM_ADMG || !(dm_sim_stat_modelled(d,s->astat[k]) || stat_outside(d,s->astat[k]))))return 0;
            for(int k=0;k<14;k++)if(s->pstat[k]>=0 && !(dm_sim_stat_modelled(d,s->pstat[k]) || stat_outside(d,s->pstat[k])))return 0;
        }
        if(g->kind>=DM_GR_ATTACK) {
            int bad=0;dm_sim_tmod m;dm_sim_tmod_init(&m);grant_cast(d,in,t,&m,g->skill,g->lvl,&bad);
            if(bad)return 0;
        }
    }
    return 1;
}
int dm_sim_grants_ok(const dm_sim_data *d,const dm_sim_char *in,dm_sim_grant_miss *miss) {
    dm_sim_grant_miss none;if(!miss)miss=&none;
    memset(miss,0,sizeof *miss);miss->stat=-1;
    dm_sim_target t;
    if(!in->ngrant || dm_sim_target_act(d,in,&t)<0)return 1;
    if(!dm_sim_grants_modelled(d,in,&t))return 0;
    for(int i=0;i<in->ngrant;i++) {
        const dm_sim_grant *g=&in->grant[i];
        if(!g->checked)continue;
        const dm_sim_skill *s=d->skill[g->skill];
        dm_sim_char *x=malloc(sizeof *x);if(!x)return 0;
        *x=*in;x->lvl[g->skill]=g->held;
        for(int k=0;k<6;k++) {
            if(s->astat[k]<0)continue;

            int bad=0,model=0,passive=0;
            for(int j=0;j<14;j++)if(s->pstat[j]==s->astat[k]){passive=1;model+=(int)sim_calc(d,x,g->skill,DM_CF_PASSIVE1+j,&bad);}
            if(!passive)model=(int)sim_calc(d,x,g->skill,DM_CF_AURASTAT1+k,&bad);
            if(bad || model!=g->game[k]){miss->skill=g->skill;miss->level=g->held;miss->stat=s->astat[k];miss->model=model;miss->game=g->game[k];free(x);return 0;}
        }
        free(x);
    }
    return 1;
}
