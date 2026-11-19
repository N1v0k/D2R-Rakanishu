#include "dm_gear_bound.h"
#include "dm_sim_internal.h"
#include "dm_splash_rule.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>


static const int16_t k_cb_good[] = { 0, 2, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 31, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 119, 171, 182,
                                     185, 186, 325, 329, 330, 331, 332 };

static const int16_t k_cb_unknown[] = { 101, 326 };
static const int16_t k_skill_bonus[] = { 83, 107, 126, 127, 188 };

void dm_gear_stat_classes(const dm_gear_problem *p, uint8_t cls[DM_GEAR_NSTAT]) {
    memset(cls, DM_GEAR_SC_IRR, DM_GEAR_NSTAT);
    for (size_t k = 0; k < sizeof p->sv / sizeof *p->sv; k++) if (p->sv[k] >= 0 && p->sv[k] < DM_GEAR_NSTAT) cls[p->sv[k]] = DM_GEAR_SC_GOOD;
    for (size_t k = 0; k < sizeof k_cb_good / sizeof *k_cb_good; k++) cls[k_cb_good[k]] = DM_GEAR_SC_GOOD;
    for (size_t k = 0; k < sizeof k_cb_unknown / sizeof *k_cb_unknown; k++) cls[k_cb_unknown[k]] = DM_GEAR_SC_UNKNOWN;
    for (size_t k = 0; k < sizeof k_skill_bonus / sizeof *k_skill_bonus; k++) cls[k_skill_bonus[k]] = DM_GEAR_SC_SKILL;

    static const char *const more[] = { "passive_mastery_melee_th", "passive_mastery_melee_dmg", "passive_mastery_melee_crit", "passive_mastery_throw_th",
                                        "passive_mastery_throw_dmg", "passive_mastery_throw_crit", "skill_armor_percent", "item_armor_percent" };
    for (size_t k = 0; k < sizeof more / sizeof *more; k++)
        for (int i = 0; i < DM_GEAR_NSTAT; i++) if (!strcmp(p->d->stat_name[i], more[k])) cls[i] = DM_GEAR_SC_GOOD;
    cls[0] = cls[2] = DM_GEAR_SC_REQ;

    const dm_sim_data *d = p->d;
    for (int i = 0; i + 1 < d->ncode; i += 2) if (d->code[i] == DM_OP_STAT && d->code[i + 1] >= 0 && d->code[i + 1] < DM_GEAR_NSTAT && cls[d->code[i + 1]] == DM_GEAR_SC_IRR)
        cls[d->code[i + 1]] = DM_GEAR_SC_UNKNOWN;


    const dm_gear_tables *t = p->t;
    for (int k = 0; k < t->nop; k++) { int op = t->op[k].op; if (op == 3 || op == 5 || op == 6 || op == 7 || op == 11) cls[t->op[k].stat] = DM_GEAR_SC_OUT; }

    for (int pass = 0; pass < 2; pass++) for (int k = 0; k < t->nop; k++) {
        int src = t->op[k].stat, all_good = 1, any = 0, req = 0;
        for (int g = 0; g < t->op[k].ntg; g++) {
            int c = cls[t->op[k].tg[g]];
            if (c == DM_GEAR_SC_IRR) continue;
            any = 1; all_good &= c == DM_GEAR_SC_GOOD || c == DM_GEAR_SC_REQ; req |= c == DM_GEAR_SC_REQ;
        }
        if (!any || cls[src] != DM_GEAR_SC_IRR) continue;
        cls[src] = (uint8_t)(t->op[k].op == 2 || t->op[k].op == 4 ? (all_good ? (req ? DM_GEAR_SC_REQ : DM_GEAR_SC_GOOD) : DM_GEAR_SC_UNKNOWN) : DM_GEAR_SC_UNKNOWN);
    }
}

static int group_of(const dm_sim_skill *s) {
    if (!s) return 0;
    if (!strcmp(s->aurastate, "frozenarmor") || !strcmp(s->aurastate, "shiverarmor") || !strcmp(s->aurastate, "chillingarmor")) return 1;
    if (!strcmp(s->aurastate, "wolf") || !strcmp(s->aurastate, "bear")) return 2;
    if (!strcmp(s->aurastate, "fade") || !strcmp(s->aurastate, "quickness")) return 3;
    if (strstr(s->aurastate, "golem")) return 4;
    return 0;
}
static int active(const dm_sim_char *c, int id) {
    if (id >= 0 && id < DM_SIM_MAXSKILL && c->active_lvl[id] > 0) return 1;
    if (c->active_aura == id) return 1;
    for (int i = 0; i < c->nactive_buff; i++) if (c->active_buff[i] == id) return 1;
    return 0;
}
static void modify(const dm_sim_data *d, dm_sim_char *c, dm_sim_tmod *m, int id, int sign, int *bad) {
    if (id < 0 || id >= DM_SIM_MAXSKILL || !d->skill[id] || !c->lvl[id]) return;
    if (d->skill[id]->unavailable) { *bad = 1; return; }
    static _Thread_local dm_sim_char x; dm_sim_tmod y = *m; int invalid = 0;
    x = *c; sim_modify(d, &x, &y, id, sign, &invalid);
    if (invalid) { *bad = 1; return; }
    *c = x; *m = y;
}




typedef struct { double dg, d, p, f, fx, af, cd, pg, c, petg, pet, area, ph; int splash;
#ifdef DM_GEAR_INSTR
                 int id, lo, hi; double aura;
#endif
} act;

#ifdef DM_GEAR_INSTR


static _Thread_local int g_cap_on;
static _Thread_local dm_gear_instr_terms g_last, g_win_s, g_win_g;
void dm_gear_instr_cap(int on) { g_cap_on = on; }
int dm_gear_instr_get_terms(dm_gear_instr_terms *single, dm_gear_instr_terms *group) {
    if (!g_cap_on || !g_win_s.have) return -1;
    if (single) *single = g_win_s;
    if (group) *group = g_win_g;
    return 0;
}
#endif

static double uses(const act *a, double t) { return t < a->af ? 0 : floor((t - a->af) / a->cd) + 1; }


static double lp_dual(const act *a, int n, double t, double span, double mana, double beta) {
    double best = INFINITY;
    for (int k = -1; k < n; k++) {
        double alpha = k < 0 ? 0 : (a[k].d - beta * a[k].c) / a[k].f;
        if (alpha < 0) continue;
        double u = alpha * span + (isfinite(mana) ? beta * mana : 0);
        for (int i = 0; i < n; i++) { double g = a[i].d - alpha * a[i].f - beta * a[i].c; if (g > 0) u += g * uses(&a[i], t); }
        if (u < best) best = u;
    }
    return best;
}





static double lp_bound(const act *a, int n, double t, double span, double mana) {
    if (!isfinite(mana)) return lp_dual(a, n, t, span, mana, 0);
    double hi = 0; for (int i = 0; i < n; i++) if (a[i].c > 0) hi = fmax(hi, a[i].d / a[i].c);
    double lo = 0;
    for (int it = 0; it < 100 && hi - lo > 1e-12 * (1 + hi); it++) {
        double x = lo + (hi - lo) / 3, y = hi - (hi - lo) / 3;
        if (lp_dual(a, n, t, span, mana, x) <= lp_dual(a, n, t, span, mana, y)) hi = y; else lo = x;
    }
    return fmin(lp_dual(a, n, t, span, mana, 0), lp_dual(a, n, t, span, mana, (lo + hi) / 2));
}
#ifdef DM_GEAR_INSTR



static double lp_mix(const act *a, int n, double t, double span, double mana, double *mix, double *mana_used, double *frames_used) {
    if (!isfinite(mana)) mana = 1e18;
    int idx[20]; int k = 0;
    for (int i = 0; i < n; i++) {
        double u = uses(&a[i], t);
        if (u <= 0) continue;
        if (a[i].d > 0 && k < (int)(sizeof idx / sizeof *idx)) idx[k++] = i; else if (a[i].d > 0) k = 99;
    }
    for (int i = 0; i < n; i++) mix[i] = 0;
    *mana_used = 0; *frames_used = 0;
    if (k == 0 || k > (int)(sizeof idx / sizeof *idx)) return 0;
    double uval[20]; for (int j = 0; j < k; j++) uval[j] = uses(&a[idx[j]], t);
    double best = -1, bestmix[20] = { 0 };
    for (unsigned long mask = 0; mask < (1UL << k); mask++) {
        double F = span, M = mana;
        int freei[20], nf = 0;
        for (int j = 0; j < k; j++) {
            if (mask >> j & 1) { F -= uval[j] * a[idx[j]].f; M -= uval[j] * a[idx[j]].c; }
            else freei[nf++] = j;
        }
        if (F < -1e-9 || M < -1e-9) continue;
        for (int f = -1; f < nf; f++) {
            double val[20];
            for (int j = 0; j < k; j++) val[j] = 0;
            if (f >= 0) { const act *x = &a[idx[freei[f]]]; double v = x->f > 0 ? F / x->f : uval[freei[f]];
                if (x->c > 0) v = fmin(v, M / x->c); val[f] = fmin(fmax(v, 0), uval[freei[f]]); }
            for (int take = 0; take <= (f < 0 ? 0 : f); take++) {
                if (take > 0) {
                    int g = take - 1;
                    const act *x = &a[idx[freei[f]]], *y = &a[idx[freei[g]]];
                    double det = x->f * y->c - y->f * x->c;
                    double vx = det != 0 ? (F * y->c - M * y->f) / det : -1, vy = det != 0 ? (x->f * M - F * x->c) / det : -1;
                    if (vx < -1e-9 || vy < -1e-9 || vx > uval[freei[f]] + 1e-9 || vy > uval[freei[g]] + 1e-9) continue;
                    vx = fmin(fmax(vx, 0), uval[freei[f]]); vy = fmin(fmax(vy, 0), uval[freei[g]]);
                    if (F - vx * x->f - vy * y->f < -1e-6 || M - vx * x->c - vy * y->c < -1e-6) continue;
                    for (int j = 0; j < k; j++) val[j] = 0;
                    val[f] = vx; val[g] = vy;
                }
                double dmg = 0;
                for (int j = 0; j < k; j++) dmg += ((mask >> j & 1) ? uval[j] : val[j]) * a[idx[j]].d;
                if (dmg > best + 1e-12) {
                    best = dmg; *frames_used = 0; *mana_used = 0;
                    for (int j = 0; j < k; j++) {
                        bestmix[j] = (mask >> j & 1) ? uval[j] : val[j];
                        *frames_used += bestmix[j] * a[idx[j]].f; *mana_used += bestmix[j] * a[idx[j]].c;
                    }
                }
            }
        }
    }
    for (int j = 0; j < k; j++) mix[idx[j]] = bestmix[j];
    return best;
}
#endif
static double share_by(const act *a, int n, double t, double cap) {
    double v = 0; for (int i = 0; i < n; i++) v += a[i].p * uses(&a[i], t);
    return fmin(cap, v);
}




static double act_cost(const dm_sim_data *d, const dm_sim_target *stdp, int diff, const dm_sim_skill_out *a) {
    double c = a->mana_x256 / 256.0;
    int ms = d->diff[diff].msd;
    if (ms > 0 && a->ml > 0 && a->hit[0] > 0) c -= a->ml / 100 * stdp->drain / 100 / ms * a->hit[0];
    return c;
}


static double share_compound(const act *a, int n, double t, double cap, double boss) {
    double left = 1;
    for (int i = 0; i < n; i++) { double q = boss > 0 ? fmin(1, a[i].p / boss) : 1, u = uses(&a[i], t); if (u > 0) left *= pow(1 - q, u); }
    return fmin(cap, boss * (1 - left));
}





static double least_hit(const dm_sim_data *d, const dm_sim_char *c, const dm_sim_target *t, const dm_sim_tmod *m) {
    double mean[DM_EL_N] = { 0 };
    mean[DM_EL_PHYS] = (t->dmin + t->dmax) / 2.0;
    for (int k = 0; k < 3; k++)
        if (t->eltype[k] >= 0 && t->eltype[k] < DM_EL_N && t->elpct[k] > 0) mean[t->eltype[k]] += (t->elmin[k] + t->elmax[k]) / 2.0 * fmin(100, t->elpct[k]) / 100;
    double crit = t->crit < 0 ? 0 : t->crit > 100 ? 1 : t->crit / 100.0, total = 0;
    for (int cr = 0; cr < 2; cr++) {
        double scale = (cr ? 2 : 1) * fmax(0, 100 + m->dmg_pct) / 100, weight = cr ? crit : 1 - crit;
        for (int e = 0; e < DM_EL_N; e++) {
            if (mean[e] <= 0) continue;
            double flat = e == DM_EL_PHYS ? c->dr_flat : e == DM_EL_POIS ? 0 : c->mdr_flat;
            int penalty = e == DM_EL_PHYS || e == DM_EL_MAG ? 0 : d->diff[c->diff].rp;
            int top = e == DM_EL_PHYS ? 50 : c->maxres[e] < 0 ? 0 : c->maxres[e] > 95 ? 95 : c->maxres[e];
            int res = c->res[e] + penalty; res = res < -100 ? -100 : res > top ? top : res;
            total += weight * (1 - res / 100.0) * fmax(0, scale * mean[e] - fmax(0, flat) - (e == DM_EL_PHYS));
        }
    }
    return total;
}











static void survival_bound(const dm_sim_data *d, const dm_sim_char *R, const dm_sim_char *c, const dm_sim_char *all, const dm_sim_tmod *mdef,
                           const dm_sim_target *std, const act *sk, int ns, double leech, int steady, dm_gear_bound_out *o) {
    if (std->cycle <= 0 || (std->flags & DM_MF_PRIME)) return;
    int phys = std->dmax > 0, elem[DM_EL_N] = { 0 };
    for (int k = 0; k < 3; k++) if (std->eltype[k] >= 0 && std->eltype[k] < DM_EL_N && std->elpct[k] > 0) elem[std->eltype[k]] = 1;
    const dm_sim_char *ch[3] = { R, c, all };
    double life = 0, regen = -INFINITY;
    for (int k = 0; k < 3; k++) {
        const dm_sim_char *x = ch[k];
        if (x->mercenary || x->es_pct > 0 || (phys && x->bone_pool > 0) || ((elem[DM_EL_FIRE] || elem[DM_EL_LTNG] || elem[DM_EL_COLD]) && x->cyclone_pool > 0) ||
            (elem[DM_EL_MAG] && x->ward_pool > 0)) return;
        for (int e = 0; e < DM_EL_N; e++) if (x->absorb_flat[e] > 0 || x->absorb_pct[e] > 0) return;
        life = fmax(life, x->life); regen = fmax(regen, x->liferegen / 256.0); leech = fmax(leech, x->ll);
    }
    double land, hit = least_hit(d, all, std, mdef);
    sim_defense_hit(d, all, std, mdef, &land);
    if (!(land > 0) || !(hit > 0)) return;
    double rate = 0, most = 0;
    for (int k = 0; k < ns; k++) {
        most = fmax(most, sk[k].ph); if (sk[k].f > 0) rate = fmax(rate, sk[k].ph / sk[k].f);
    }
    int ls = d->diff[all->diff].lsd;
    double back = fmax(0, mdef->lifetap) / 100 + (ls > 0 ? fmax(0, leech) / 100 * std->drain / 100 / ls : 0);
    double cycle = std->cycle, gain = regen + back * rate;

    o->recovery = 100 * fmax(0, gain) * cycle / (hit * land) * (1 + 1e-9) + 1e-9;
    double once = 2 * back * most;
    if (all->potions && d->potions_loaded == (1u << DM_POT_N) - 1 && all->cls >= 0 && all->cls < d->ncls)
        for (int k = 0; k < DM_POT_N; k++) {
            int n = all->belt[k] < 0 ? 0 : all->belt[k] > 16 ? 16 : all->belt[k];
            if (d->potion[k].kind == DM_PK_REJUV) once += n * life * d->potion[k].life / 100.0;
            else if (d->potion[k].kind == DM_PK_HEAL) once += n * d->potion[k].life * d->cls[all->cls].hppot / 100.0;
        }
    double net = hit * land - gain * cycle;
    int attacks = (int)(DM_SIM_HORIZON / cycle);
    if (!(net > 0) || (attacks - 1) * net < life + once) return;
    double last = floor((life + once) / net) + 1;
    o->survival_s = fmin(o->survival_s, last * cycle / DM_SIM_FPS * (1 + 1e-9));



    double hits = steady ? (life + once) / (hit - fmax(0, gain) * cycle / land) : 0.95 * last;
    o->hits = fmin(o->hits, hits * (1 + 1e-9));
}

static _Thread_local dm_sim_char R, L, c, probe;
static int bound_state(const dm_gear_problem *p, const dm_gear_bound_in *in, const dm_sim_target *stdp, int boss, double cycle, int aura, dm_gear_bound_out *o);
enum { AURA_ALL = -2, AURA_KEEP = -1 };
int dm_gear_bound_eval(const dm_gear_problem *p, const dm_gear_char *now, const dm_gear_bound_in *in, dm_gear_bound_out *o) {
    if (!p || !p->ready || !now || !in || !o) return -1;
#ifdef DM_GEAR_INSTR
    if (g_cap_on) g_win_s.have = 0;
#endif
    const dm_sim_data *d = p->d;
    dm_sim_target std;
    if (dm_sim_target_act(d, &p->now, &std) < 0) return -1;


    double cycle = std.cycle > 0 ? std.cycle : 30;
    o->survival_s = DM_SIM_HORIZON / (double)DM_SIM_FPS;
    o->hits = DM_SIM_HORIZON / fmin(cycle, fmax(std.cycle, 1));
    o->recovery = o->group = o->single = INFINITY;
    if (in->varied) return 0;
    int boss = dm_gear_boss_life(d, p->now.diff, p->now.act);
    if (boss < 0) return 0;



    static _Thread_local dm_gear_item item;
    static _Thread_local dm_gear_char hi, lo;
    unsigned char on = 1; unsigned fr, fl;
    memset(&item, 0, sizeof item); item.setitem = -1; item.own = in->hi;
    if (dm_gear_compose(p->t, &p->base, &item, 1, &on, &hi)) return -1;
    item.own = in->lo;
    if (dm_gear_compose(p->t, &p->base, &item, 1, &on, &lo)) return -1;
    dm_gear_char_from(p, now, &hi, NULL, in->main, in->off, in->kick_min, in->kick_max, &R, &fr);
    dm_gear_char_from(p, now, &lo, NULL, in->main, in->off, in->kick_min, in->kick_max, &L, &fl);
    if (fr) return 0;

    for (int id = 0; id < DM_SIM_MAXSKILL; id++) {
        const dm_sim_skill *s = d->skill[id];
        if (!s || !s->passive || !p->node[id] || L.lvl[id] >= R.lvl[id]) continue;
        if (R.lvl[id] - L.lvl[id] > 64) return 0;
        probe = R;
        for (int k = 0; k < 14; k++) {
            if (s->pstat[k] < 0) continue;
            double prev = -INFINITY;
            for (int lv = L.lvl[id]; lv <= R.lvl[id]; lv++) {
                int bad = 0; probe.lvl[id] = (int16_t)lv;
                double v = lv > 0 ? sim_calc(d, &probe, id, DM_CF_PASSIVE1 + k, &bad) : 0;
                if (bad || v < prev) return 0;
                prev = v;
            }
        }
    }
    if (!in->tight) {
        int rc = bound_state(p, in, &std, boss, cycle, AURA_ALL, o);
#ifdef DM_GEAR_INSTR
        if (g_cap_on) g_win_s = g_win_g = g_last;
#endif
        return rc;
    }


    const dm_gear_bound_out start = *o;
    int choice[DM_SIM_MAXBAR + 1], nchoice = 0;
    choice[nchoice++] = AURA_KEEP;
    for (int i = 0; i < R.nbar && nchoice <= DM_SIM_MAXBAR; i++) {
        int id = R.bar[i], dup = 0; const dm_sim_skill *s = id >= 0 && id < DM_SIM_MAXSKILL ? d->skill[id] : NULL;
        if (!s || !R.lvl[id] || !s->aura || !(s->fam & (DM_FAM_AOFF | DM_FAM_ADEF | DM_FAM_ADMG)) || id == R.active_aura) continue;
        for (int k = 0; k < nchoice; k++) dup |= choice[k] == id;
        if (!dup) choice[nchoice++] = id;
    }
    for (int k = 0; k < nchoice; k++) {
        dm_gear_bound_out x = start;
        int rc = bound_state(p, in, &std, boss, cycle, choice[k], &x);
        if (rc) return rc;
#ifdef DM_GEAR_INSTR
        if (g_cap_on && (!k || x.single > o->single)) g_win_s = g_last;
        if (g_cap_on && (!k || x.group > o->group)) g_win_g = g_last;
#endif
        if (!k) *o = x;
        else { o->group = fmax(o->group, x.group); o->single = fmax(o->single, x.single); o->survival_s = fmax(o->survival_s, x.survival_s);
               o->hits = fmax(o->hits, x.hits); o->recovery = fmax(o->recovery, x.recovery); }
    }
    return 0;
}



static int bound_state(const dm_gear_problem *p, const dm_gear_bound_in *in, const dm_sim_target *stdp, int boss, double cycle, int aura, dm_gear_bound_out *o) {
#ifdef DM_GEAR_INSTR
    if (g_cap_on) g_last.have = 0;
#endif
    const dm_sim_data *d = p->d;
    dm_sim_target std = *stdp;
    dm_sim_tmod m; dm_sim_tmod_init(&m);
    int bad = 0;
    c = R;
    for (int id = 0; id < DM_SIM_MAXSKILL; id++) if (active(&R, id) && R.active_lvl[id] > 0 && R.active_lvl[id] != R.lvl[id]) {
        int16_t level = c.lvl[id]; c.lvl[id] = R.active_lvl[id]; modify(d, &c, &m, id, -1, &bad); c.lvl[id] = level; modify(d, &c, &m, id, 1, &bad);
    }

    sim_grants_apply(d, &R, &c, &m, &bad);
    if (aura == AURA_ALL)
        for (int i = 0; i < c.nbar; i++) {
            int id = c.bar[i]; const dm_sim_skill *s = id >= 0 && id < DM_SIM_MAXSKILL ? d->skill[id] : NULL;
            if (!s || !c.lvl[id] || !s->aura || !(s->fam & (DM_FAM_AOFF | DM_FAM_ADEF | DM_FAM_ADMG))) continue;
            if (id != R.active_aura || !strcmp(s->aurastate, "conviction")) modify(d, &c, &m, id, 1, &bad);
        }
    else if (aura == AURA_KEEP) {
        int id = R.active_aura;

        if (id >= 0 && id < DM_SIM_MAXSKILL && d->skill[id] && !strcmp(d->skill[id]->aurastate, "conviction")) {
            int granted = 0;
            for (int k = 0; k < c.ngrant; k++) if (c.grant[k].skill == id && (c.grant[k].kind == DM_GR_AURA || c.grant[k].kind == DM_GR_BUFF) && c.grant[k].lvl > 0) granted = 1;
            if (!granted) modify(d, &c, &m, id, 1, &bad);
        }
    } else {


        sim_own_aura(d, &c, &m, R.active_aura, -1, &bad); sim_own_aura(d, &c, &m, aura, 1, &bad);
    }
    int chosen[5] = { -1, -1, -1, -1, -1 };
    for (int i = 0; i < c.nbar; i++) {
        int id = c.bar[i]; const dm_sim_skill *s = id >= 0 && id < DM_SIM_MAXSKILL ? d->skill[id] : NULL;
        if (!s || !c.lvl[id] || !(s->fam & DM_FAM_B)) continue;
        int g = group_of(s); if (g && chosen[g] >= 0) continue; if (g) chosen[g] = id;
        if (!active(&R, id)) {
            for (int k = 0; g && k < R.nactive_buff; k++) if (group_of(d->skill[R.active_buff[k]]) == g) modify(d, &c, &m, R.active_buff[k], -1, &bad);
            modify(d, &c, &m, id, 1, &bad);
        }
        if (!s->timed && !active(&c, id) && c.nactive_buff < 8) c.active_buff[c.nactive_buff++] = id;
    }
    for (int i = 0; i < c.nbar; i++) {
        int id = c.bar[i]; const dm_sim_skill *s = id >= 0 && id < DM_SIM_MAXSKILL ? d->skill[id] : NULL;
        if (s && c.lvl[id] && (s->fam & DM_FAM_K)) modify(d, &c, &m, id, 1, &bad);
    }
    if (bad) return 0;



    static _Thread_local dm_sim_char all, scratch;
    dm_sim_tmod mdef = m; int defense = in->tight, steady = 0;
    if (defense) {
        all = c;
        for (int k = 0; k < R.nactive_buff; k++) if (!active(&all, R.active_buff[k])) modify(d, &all, &mdef, R.active_buff[k], 1, &bad);
        for (int i = 0; i < all.nbar; i++) {
            int id = all.bar[i]; const dm_sim_skill *s = id >= 0 && id < DM_SIM_MAXSKILL ? d->skill[id] : NULL;
            if (!s || !all.lvl[id] || !(s->fam & DM_FAM_B) || active(&all, id)) continue;
            modify(d, &all, &mdef, id, 1, &bad);
            if (all.nactive_buff < 8) all.active_buff[all.nactive_buff++] = id;
        }


        double ar = 0, dmg = 0;
        int st_ar = -1, st_dmg = -1;
        for (int k = 0; k < DM_GEAR_NSTAT; k++) { if (!strcmp(d->stat_name[k], "item_tohit_percent")) st_ar = k; if (!strcmp(d->stat_name[k], "damagepercent")) st_dmg = k; }
        for (int i = -1; i < c.nbar && !bad; i++) {
            int id = i < 0 ? R.active_aura : c.bar[i];
            const dm_sim_skill *s = id >= 0 && id < DM_SIM_MAXSKILL ? d->skill[id] : NULL;
            if (!s || !c.lvl[id] || !((s->fam & DM_FAM_K) || !strcmp(s->aurastate, "conviction"))) continue;
            int top = c.lvl[id], low = L.lvl[id] < 1 ? 1 : L.lvl[id], keep = c.lvl[id];
            if (low > top) low = top;
            if (top - low > 64) { bad = 1; break; }
            double a = 0, g = 0;
            for (int lv = low; lv <= top; lv++) {
                c.lvl[id] = (int16_t)lv;
                for (int k = 0; k < 20; k++) {
                    int st = k < 6 ? s->astat[k] : s->pstat[k - 6], dup = 0;
                    if (st < 0 || (st != st_ar && st != st_dmg)) continue;
                    for (int j = 0; k < 6 && j < 14; j++) dup |= s->pstat[j] == st;
                    if (dup) continue;
                    double v = fabs(sim_calc(d, &c, id, k < 6 ? DM_CF_AURASTAT1 + k : DM_CF_PASSIVE1 + k - 6, &bad));
                    if (st == st_ar) a = fmax(a, v); else g = fmax(g, v);
                }
            }
            c.lvl[id] = (int16_t)keep;
            ar += a; dmg += g;
        }
        if (R.ngrant && !bad) { dm_sim_tmod tm; dm_sim_tmod_init(&tm); scratch = R; sim_grants_apply(d, &R, &scratch, &tm, &bad); ar += abs(tm.ar_pct); dmg += abs(tm.dmg_pct); }
        mdef.ar_pct = (int)-ar; mdef.dmg_pct = (int)-dmg;


        steady = ar == 0;
        for (int i = 0; i < c.nbar && steady; i++) for (int j = 0; j < i && steady; j++) {
            const dm_sim_skill *x = c.bar[i] >= 0 && c.bar[i] < DM_SIM_MAXSKILL ? d->skill[c.bar[i]] : NULL, *y = c.bar[j] >= 0 && c.bar[j] < DM_SIM_MAXSKILL ? d->skill[c.bar[j]] : NULL;
            if (x && y && (x->fam & DM_FAM_B) && (y->fam & DM_FAM_B) && group_of(x) && group_of(x) == group_of(y) && c.bar[i] != c.bar[j]) steady = 0;
        }
        defense = !bad; bad = 0;
    }


    double pool = fmax(R.mana, c.mana), regen_pct = fmax(R.manaregen_pct, c.manaregen_pct);
    c.mana = 9e6;



    dm_sim_target pack = std, lone = std;
    pack.extra_targets = DM_GEAR_PACK_EXTRA; lone.hp = boss; lone.regen = 0;
    int ids[DM_SIM_MAXBAR + 1], n = 0;
    for (int i = -1; i < c.nbar; i++) {
        int id = i < 0 ? 0 : c.bar[i], dup = 0;
        if (id < 0 || id >= DM_SIM_MAXSKILL || !d->skill[id] || (id && !c.lvl[id])) continue;
        for (int k = 0; k < n; k++) dup |= ids[k] == id;
        if (!dup) ids[n++] = id;
    }
    act sk[DM_SIM_MAXBAR + 1]; int ns = 0;
    double ow = 0, streams = 0, field = 0, leech = c.ll; int crush = 0;
    for (int k = 0; k < n; k++) {
        int id = ids[k]; const dm_sim_skill *s = d->skill[id];
        int top = c.lvl[id] < 1 ? 1 : c.lvl[id], low = L.lvl[id] < 1 ? 1 : L.lvl[id];
        if (low > top) low = top;
        if (top - low > 64) return 0;
        act x = { 0 }; int damage = 0;
        x.f = x.af = x.cd = x.pg = 1e9; x.c = 1e18;
        for (int lv = low; lv <= top; lv++) {
            probe = c; probe.lvl[id] = (int16_t)lv;
            for (int side = 0; side < 2; side++) {
                dm_sim_skill_out a;
                if (sim_action(d, &probe, side ? &lone : &pack, &m, id, &a) || (a.flags & DM_SF_UNAVAIL)) return 0;

                sim_grant_action(d, &probe, side ? &lone : &pack, &m, id, &a);
                leech = fmax(leech, a.ll);
                if (a.flags & (DM_SF_MODIFIER | DM_SF_UTILITY | DM_SF_NOWEAPON)) continue;
                damage = 1;
                int frames = a.frames > 0 ? a.frames : 1, period = a.cooldown > frames ? a.cooldown : frames;
                double dealt = sim_sum(a.hit) + a.dot + a.burn, pets = a.pet_dps / DM_SIM_FPS;
                if (a.ow_chance > 0 && a.ow_rate > ow) ow = a.ow_rate;
                x.c = fmin(x.c, act_cost(d, &std, c.diff, &a));
                x.ph = fmax(x.ph, a.hit[0]);
                if (!side) { x.dg = fmax(x.dg, dealt); x.pg = fmin(x.pg, period); x.petg = fmax(x.petg, pets); continue; }
                x.d = fmax(x.d, dealt); x.p = fmax(x.p, (a.cb_frac + a.sf_frac) * boss); x.f = fmin(x.f, frames); x.fx = fmax(x.fx, frames); x.cd = fmin(x.cd, period);


                x.af = fmin(x.af, (a.action_frame < 1 ? 1 : a.action_frame > frames ? frames : a.action_frame) + (a.travel > 0 ? a.travel : 0)); x.pet = fmax(x.pet, pets);
                if (a.cb_frac > 0) crush = 1;
                if (a.sf_frac > 0) field = fmax(field, 1 - fmax(0, a.sf_floor));
            }
        }
        if (!damage) continue;
        x.area = s->area ? DM_GEAR_AREA_HITS : 1;
        x.splash = in->splash_on && dm_splash_skill_ok(s);

        if (s->aura && (s->fam & DM_FAM_ADMG)) { streams += x.d / x.cd + x.pet;
#ifdef DM_GEAR_INSTR
            x.aura = x.d / x.cd;
#endif
            x.d = 0; }
        else streams += x.pet;
#ifdef DM_GEAR_INSTR
        x.id = id; x.lo = low; x.hi = top;
#endif
        sk[ns++] = x;
    }

    double item_aura = sim_grant_aura_dps(d, &c, &lone, &m) / DM_SIM_FPS, item_aura_pack = sim_grant_aura_dps(d, &c, &pack, &m);
    double struck = sim_grant_struck(d, &c, &pack, &m, &bad) * DM_SIM_FPS / cycle;
    if (bad) return 0;
    streams += item_aura;







    double regen = d->cls[c.cls].manaregen > 0 ? pool / d->cls[c.cls].manaregen * (100 + regen_pct) / 100 : 0;
    double stock = pool, pot_rate = 0, pot_all = 0;

    if (c.potions && d->potions_loaded == (1u << DM_POT_N) - 1 && c.cls >= 0 && c.cls < d->ncls)
        for (int k = 0; k < DM_POT_N; k++) {
            int n = c.belt[k] < 0 ? 0 : c.belt[k] > 16 ? 16 : c.belt[k];
            if (n <= 0 || d->potion[k].mana <= 0) continue;
            if (d->potion[k].kind == DM_PK_REJUV) stock += n * pool * d->potion[k].mana / 100.0;
            else if (d->potion[k].kind == DM_PK_MANA) {
                pot_all += n * d->potion[k].mana * d->cls[c.cls].mppot / 100.0;
                if (d->potion[k].len > 0) pot_rate = fmax(pot_rate, d->potion[k].mana * d->cls[c.cls].mppot / 100.0 / d->potion[k].len);
            }
        }









    double reach = 1;
    if (in->splash_on && in->hands)
        for (int h = 0; h < c.nhands && h < 2; h++) reach = fmax(reach, dm_splash_hand_reach(&c, &c.hand[h], DM_GEAR_PACK_EXTRA));
    else if (in->splash_on)
        for (int g = DM_SPLASH_NONE; g <= DM_SPLASH_UNKNOWN; g++) {
            dm_splash_area a = dm_splash_area_of((dm_splash_group)g, c.cls, INT_MAX, INT_MAX, 2);
            reach = fmax(reach, dm_splash_reach(&a, DM_GEAR_PACK_EXTRA));
        }
    double group = 0;
#ifdef DM_GEAR_INSTR
    int gw = -1;
#endif
    for (int k = 0; k < ns; k++) {
        const act *x = &sk[k];
        double a = DM_SIM_FPS / x->pg, rate = a;
        if (x->c > 0) rate = fmin(a, (pool / 10 + regen) / x->c);
        double v;
        if (x->splash)
            v = (x->dg * rate + DM_SIM_FPS * x->petg) * (x->area * reach)
              + DM_SIM_FPS * ow * DM_GEAR_PACK_UNITS;
        else v = (x->dg * rate + DM_SIM_FPS * (x->petg + ow)) * x->area;
        if (v > group) { group = v;
#ifdef DM_GEAR_INSTR
            gw = k;
#endif
        }
    }




    double alongside = streams + ow, window = DM_GEAR_SINGLE_FRAMES, share = boss * fmin(1, field + crush);


    double credit = 0;
    for (int k = 0; k < ns; k++) credit = fmax(credit, sk[k].fx - sk[k].af);
    if (credit < 0) credit = 0;


#define MANA_AT(t) (stock + regen * (t) / DM_SIM_FPS + fmin(pot_rate * (t), pot_all))
#define DEALT(t) (lp_bound(sk, ns, (t), (t) + credit, MANA_AT(t)) + alongside * (t) + \
                  (in->tight ? share_compound(sk, ns, (t), share, boss) : share_by(sk, ns, (t), share)))
    double dealt_w = DEALT(window);
    double rate = dealt_w / window;
#ifdef DM_GEAR_INSTR
    double i_lp = lp_bound(sk, ns, window, window + credit, MANA_AT(window));
    double i_share = in->tight ? share_compound(sk, ns, window, share, boss) : share_by(sk, ns, window, share);
    double i_pet = 0, i_aura = 0;
    for (int k = 0; k < ns; k++) { i_pet += sk[k].pet; i_aura += sk[k].aura; }
    int i_kill = 0;
#endif
    if (dealt_w >= boss) {

        int lo_k = 1, hi_k = (int)window;
        while (lo_k < hi_k) {
            int mid = (lo_k + hi_k) / 2;
            double v = DEALT(mid);
            if (v >= boss) hi_k = mid; else lo_k = mid + 1;
        }
        rate = fmax(rate, boss / (double)lo_k);
#ifdef DM_GEAR_INSTR
        i_kill = lo_k;
#endif
    }
#ifdef DM_GEAR_INSTR
    double i_lpt = i_kill ? i_kill : window;
    double i_mix[DM_SIM_MAXBAR + 1] = { 0 }, i_mana = 0, i_frames = 0;
    double i_val = lp_mix(sk, ns, i_lpt, i_lpt + credit, MANA_AT(i_lpt), i_mix, &i_mana, &i_frames);
#endif
#undef DEALT
#undef MANA_AT
    o->group = group + item_aura_pack * DM_GEAR_AREA_HITS + struck;
    o->single = DM_SIM_FPS * rate;
#ifdef DM_GEAR_INSTR
    if (g_cap_on) {
        dm_gear_instr_terms *t = &g_last;
        memset(t, 0, sizeof *t);
        t->have = 1; t->astate = aura;
        t->single = o->single; t->group = o->group; t->win = window; t->boss = boss;
        t->lp = DM_SIM_FPS * i_lp / window; t->pets = DM_SIM_FPS * i_pet; t->auras = DM_SIM_FPS * i_aura;
        t->item_auras = DM_SIM_FPS * item_aura; t->ow = DM_SIM_FPS * ow; t->share = DM_SIM_FPS * i_share / window;
        t->boost = fmax(0, o->single - (t->lp + t->pets + t->auras + t->item_auras + t->ow + t->share));
        t->pool = pool; t->regen = regen; t->stock = stock; t->pot_rate = pot_rate; t->ns = ns; t->crush = crush; t->field = field;
        t->pot_all = pot_all; t->kill = i_kill; t->lp_t = i_lpt; t->lp_mana = i_mana; t->lp_frames = i_frames; t->lp_val = i_val; t->credit = credit;
        for (int k = 0; k < ns; k++) {
            t->sk[k].id = sk[k].id; t->sk[k].lo = sk[k].lo; t->sk[k].hi = sk[k].hi; t->sk[k].area = (int)sk[k].area;
            t->sk[k].d = sk[k].d; t->sk[k].dg = sk[k].dg; t->sk[k].p = sk[k].p; t->sk[k].f = sk[k].f; t->sk[k].af = sk[k].af; t->sk[k].cd = sk[k].cd;
            t->sk[k].c = sk[k].c; t->sk[k].pet = sk[k].pet; t->sk[k].aura = sk[k].aura; t->sk[k].mix = i_mix[k];
        }
        t->gwin = gw; t->g_best = group; t->g_item = item_aura_pack * DM_GEAR_AREA_HITS; t->g_struck = struck;
    }
#endif
    if (defense) survival_bound(d, &R, &c, &all, &mdef, &std, sk, ns, leech, steady, o);
    return 0;
}

static double pct(double now, double v) { return now > 0 ? 100 * (v - now) / now : 0; }
void dm_gear_bound_key(const dm_gear_measures *now, const dm_gear_bound_out *b, const dm_gear_goal *goal, double key[3]) {
    key[0] = dm_gear_hits_key(b->hits, goal->survival_hits);
    double share = goal->boss_share < 0 ? 0 : goal->boss_share > 1 ? 1 : goal->boss_share;
    if (now->group > 0 || now->single > 0) {
        double g = share < 1 && now->group > 0 ? pct(now->group, b->group) : 0, s = share > 0 && now->single > 0 ? pct(now->single, b->single) : 0;
        key[1] = isinf(g) || isinf(s) ? INFINITY : dm_gear_steps((1 - share) * g + share * s);
    } else key[1] = isinf(b->survival_s) ? INFINITY : dm_gear_steps(pct(now->survival_s, b->survival_s));

    if (isinf(b->group) || isinf(b->single) || isinf(b->survival_s) || isinf(b->recovery)) key[2] = INFINITY;
    else {
        double change[DM_GEAR_M_N] = { pct(now->group, b->group), pct(now->single, b->single), pct(now->survival_s, b->survival_s), b->recovery - now->recovery };
        key[2] = 0; for (int k = 0; k < DM_GEAR_M_N; k++) key[2] += change[k] / DM_GEAR_M_N;
    }
}
