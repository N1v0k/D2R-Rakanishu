





#define _POSIX_C_SOURCE 200809L
#include "t_gear_stock.h"
#include "gear_brute.h"
#include "../dm_gear_bound.h"
#include <time.h>

static int failures;
#define CHECK(cond, ...) do { if (!(cond)) { failures++; if (failures < 40) { fprintf(stderr, "FAIL %s:%d: ", __FILE__, __LINE__); fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); } } } while (0)
static double now_s(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + t.tv_nsec / 1e9; }
static const char *const k_profiles[] = { "class0-level80.prof", "class1-level80.prof", "class2-level80.prof", "class3-level80.prof", "class4-level80.prof",
                                          "class5-level80.prof", "class6-level80.prof", "class7-level12.prof", "class7-level80.prof" };
static char *g_profile[9];
static dm_gear_tables g_gt;
static const dm_sim_data *g_d;
static uint8_t g_cls[DM_GEAR_NSTAT];

static int le(double a, double b) { return a <= b * (1 + 1e-9) + 1e-9 || (isinf(b) && b > 0); }


static void monotone(gstock *g, long *checks) {
    dm_gear_problem *p = &g->p;
    dm_gear_pick now; dm_gear_pick_now(p, &now);
    int good[64], ng = 0;
    for (int i = 0; i < DM_GEAR_NSTAT && ng < 64; i++) if (g_cls[i] == DM_GEAR_SC_GOOD || g_cls[i] == DM_GEAR_SC_REQ) good[ng++] = i;
    int bar = 0; for (int i = 0; i < p->now.nbar; i++) if (p->now.bar[i] > 0 && p->node[p->now.bar[i]]) bar = p->now.bar[i];
    for (int trial = 0; trial < 12; trial++) {
        static dm_gear_bound_in a, b;
        memset(&a, 0, sizeof a);
        a.splash_on = p->splash_on;
        a.main = now.at[DM_GEAR_P_MAIN]; a.off = now.at[DM_GEAR_P_OFF]; a.hands = 1;
        const dm_gear_list *worn = &p->nowc.gear;
        for (int k = 0; k < worn->n; k++) if (g_cls[worn->v[k].stat] != DM_GEAR_SC_UNKNOWN) {
            dm_gear_list_add(&a.hi, worn->v[k].stat, worn->v[k].layer, worn->v[k].value);
            if (g_cls[worn->v[k].stat] == DM_GEAR_SC_SKILL) dm_gear_list_add(&a.lo, worn->v[k].stat, worn->v[k].layer, worn->v[k].value);
        }
        for (int k = 0; k < 6; k++) { int st = good[gs_int(0, ng - 1)]; dm_gear_list_add(&a.hi, st, 0, gs_int(1, st == 7 || st == 9 ? 50 * 256 : 40)); }
        a.kick_min = gs_int(0, 10); a.kick_max = a.kick_min + gs_int(0, 10);
        b = a;
        for (int k = 0; k < 6; k++) { int st = good[gs_int(0, ng - 1)]; dm_gear_list_add(&b.hi, st, 0, gs_int(1, st == 7 || st == 9 ? 50 * 256 : 40)); }
        if (gs_int(0, 1)) { dm_gear_list_add(&b.hi, 127, 0, 1); }
        if (bar && gs_int(0, 1)) { dm_gear_list_add(&b.hi, 107, bar, gs_int(1, 3)); dm_gear_list_add(&b.lo, 107, bar, -gs_int(0, 2)); }
        b.kick_max += gs_int(0, 5);
        dm_gear_bound_out x, y;
        if (dm_gear_bound_eval(p, &p->nowc, &a, &x) || dm_gear_bound_eval(p, &p->nowc, &b, &y)) { CHECK(0, "bound evaluation failed"); continue; }
        (*checks)++;
        CHECK(le(x.group, y.group) && le(x.single, y.single) && le(x.survival_s, y.survival_s) && le(x.hits, y.hits) && le(x.recovery, y.recovery),
              "more gives a lower bound: group %g -> %g single %g -> %g hits %g -> %g", x.group, y.group, x.single, y.single, x.hits, y.hits);
    }
}


typedef struct {
    gstock *g; brute_plan *pl;
    int last[8 + DM_GEAR_MAXCAND]; int have; dm_gear_bound_out b[2][8 + DM_GEAR_MAXCAND + 1]; int ok[2][8 + DM_GEAR_MAXCAND + 1];
    long leaves, checks, bounds, finite, tighter, recovery, death, hits;
} adm;


static int g_exact;
static int g_no_order;
static void add_item(const dm_gear_problem *p, int c, dm_gear_list *hi, dm_gear_list *lo, int *varied) {
    const dm_gear_list *l = &p->cand[c].item.own;
    for (int k = 0; k < l->n; k++) {
        int st = l->v[k].stat, v = l->v[k].value, cls = g_cls[st];
        if (cls == DM_GEAR_SC_IRR || cls == DM_GEAR_SC_OUT || !v) continue;
        if (g_exact) {
            dm_gear_list_add(hi, st, l->v[k].layer, v);
            if (cls == DM_GEAR_SC_SKILL || cls == DM_GEAR_SC_UNKNOWN) dm_gear_list_add(lo, st, l->v[k].layer, v);
            continue;
        }
        if (cls == DM_GEAR_SC_UNKNOWN) *varied = 1;
        if (v > 0) dm_gear_list_add(hi, st, l->v[k].layer, v); else if (cls == DM_GEAR_SC_SKILL) dm_gear_list_add(lo, st, l->v[k].layer, v);
    }
}
static void add_bonus(int st, int ly, int v, int unmapped, dm_gear_list *hi, dm_gear_list *lo, int *varied) {
    if (unmapped || g_cls[st] == DM_GEAR_SC_IRR || g_cls[st] == DM_GEAR_SC_OUT || !v) return;
    if (g_cls[st] == DM_GEAR_SC_UNKNOWN) *varied = 1;
    if (v > 0) dm_gear_list_add(hi, st, ly, v); else if (g_cls[st] == DM_GEAR_SC_SKILL) dm_gear_list_add(lo, st, ly, v);
}

static void add_least(const dm_gear_problem *p, int c, dm_gear_list *least) {
    const dm_gear_list *l = &p->cand[c].item.own;
    for (int k = 0; k < l->n; k++) {
        int st = l->v[k].stat, v = l->v[k].value, cls = g_cls[st];
        if (cls != DM_GEAR_SC_IRR && cls != DM_GEAR_SC_OUT && (g_exact || v < 0)) dm_gear_list_add(least, st, l->v[k].layer, v);
    }
}
static void least_bonus(int st, int ly, int v, int unmapped, dm_gear_list *least) {
    if (!unmapped && g_cls[st] != DM_GEAR_SC_IRR && g_cls[st] != DM_GEAR_SC_OUT && v < 0) dm_gear_list_add(least, st, ly, v);
}

static int prefix_bound(adm *a, const int *choice, int k, int tight, dm_gear_bound_out *o) {
    const dm_gear_problem *p = &a->g->p; const brute_plan *pl = a->pl;
    static dm_gear_bound_in in; memset(&in, 0, sizeof in);
    in.splash_on = p->splash_on;
    in.main = in.off = -1;
    static dm_gear_list hi, lo, one, low;
    int sets[DM_GEAR_MAXCAND], nsets = 0;
    for (int depth = 0; depth < 8 + pl->nfree; depth++) {
        if (depth < 8) {
            const brute_choices *c = &pl->place[depth];
            dm_gear_list best = { 0 }, least = { 0 };
            int kick_min = 0, kick_max = 0;
            low.n = low.full = 0;
            for (int i = 0, first = 1; i < c->n; i++) {
                if (depth < k && i != choice[depth]) continue;
                hi.n = lo.n = hi.full = lo.full = one.n = one.full = 0;
                int m[2] = { c->a[i], c->b[i] };
                for (int j = 0; j < 2; j++) if (m[j] >= 0) add_least(p, m[j], &one);

                if (first) low = one;
                else {
                    for (int e = 0; e < low.n; e++) { int o = dm_gear_list_get(&one, low.v[e].stat, low.v[e].layer); if (o < low.v[e].value) low.v[e].value = o; }
                    for (int e = 0; e < one.n; e++) { int cur = dm_gear_list_get(&low, one.v[e].stat, one.v[e].layer); if (one.v[e].value < cur) dm_gear_list_add(&low, one.v[e].stat, one.v[e].layer, one.v[e].value - cur); }
                }
                first = 0;
                for (int j = 0; j < 2; j++) if (m[j] >= 0) {
                    add_item(p, m[j], g_exact ? &in.hi : &hi, g_exact ? &in.lo : &lo, &in.varied);
                    if (p->cand[m[j]].item.setitem >= 0) { int dup = 0; for (int q = 0; q < nsets; q++) dup |= sets[q] == m[j]; if (!dup) sets[nsets++] = m[j]; }
                    if (depth == 6) { if (p->cand[m[j]].dmin > kick_min) kick_min = p->cand[m[j]].dmin; if (p->cand[m[j]].dmax > kick_max) kick_max = p->cand[m[j]].dmax; }
                }
                for (int e = 0; e < hi.n; e++) { int cur = dm_gear_list_get(&best, hi.v[e].stat, hi.v[e].layer); if (hi.v[e].value > cur) dm_gear_list_add(&best, hi.v[e].stat, hi.v[e].layer, hi.v[e].value - cur); }
                for (int e = 0; e < lo.n; e++) { int cur = dm_gear_list_get(&least, lo.v[e].stat, lo.v[e].layer); if (lo.v[e].value < cur) dm_gear_list_add(&least, lo.v[e].stat, lo.v[e].layer, lo.v[e].value - cur); }
                if (depth == 0 && depth < k) { in.main = c->a[i]; in.off = c->b[i]; in.hands = 1; }
            }
            if (!g_exact) for (int e = 0; e < best.n; e++) dm_gear_list_add(&in.hi, best.v[e].stat, best.v[e].layer, best.v[e].value);
            if (!g_exact) for (int e = 0; e < least.n; e++) dm_gear_list_add(&in.lo, least.v[e].stat, least.v[e].layer, least.v[e].value);
            for (int e = 0; e < low.n; e++) dm_gear_list_add(&in.least, low.v[e].stat, low.v[e].layer, low.v[e].value);
            if (depth == 6) { in.kick_min = kick_min; in.kick_max = kick_max; if (depth < k) { in.least_kick_min = kick_min; in.least_kick_max = kick_max; } }
        } else {
            int c = pl->freec[depth - 8];
            if (depth < k && !choice[depth]) continue;
            add_item(p, c, &in.hi, &in.lo, &in.varied);
            add_least(p, c, &in.least);
        }
    }
    for (int i = 0; i < p->ncand; i++) if (pl->keep[i]) { add_item(p, i, &in.hi, &in.lo, &in.varied); add_least(p, i, &in.least); }
    const dm_gear_tables *t = p->t; uint8_t seen[DM_GEAR_MAXSET] = { 0 };
    for (int i = 0; i < nsets; i++) {
        const dm_gear_item *it = &p->cand[sets[i]].item; int set = t->item[it->setitem].set;
        static dm_gear_list green, conv, ngreen; green.n = conv.n = ngreen.n = 0;
        for (int r = 0; r < t->nitemb; r++) if (t->itemb[r].owner == it->setitem) {
            add_bonus(t->itemb[r].stat, t->itemb[r].layer, t->itemb[r].value, t->itemb[r].unmapped, &in.hi, &in.lo, &in.varied);
            least_bonus(t->itemb[r].stat, t->itemb[r].layer, t->itemb[r].value, t->itemb[r].unmapped, &in.least);
            if (!t->itemb[r].unmapped && t->itemb[r].value > 0) dm_gear_list_add(&green, t->itemb[r].stat, t->itemb[r].layer, t->itemb[r].value);
            if (!t->itemb[r].unmapped && t->itemb[r].value < 0) dm_gear_list_add(&ngreen, t->itemb[r].stat, t->itemb[r].layer, t->itemb[r].value);
        }

        dm_gear_item_conversions(t, p->base.clvl, &green, &conv);
        for (int r = 0; r < conv.n; r++) if (conv.v[r].value > 0) dm_gear_list_add(&in.hi, conv.v[r].stat, conv.v[r].layer, conv.v[r].value);
        conv.n = 0; dm_gear_item_conversions(t, p->base.clvl, &ngreen, &conv);
        for (int r = 0; r < conv.n; r++) least_bonus(conv.v[r].stat, conv.v[r].layer, conv.v[r].value, 0, &in.least);
        if (seen[set]++) continue;
        for (int r = 0; r < t->nsetb; r++) if (t->setb[r].owner == set) {
            add_bonus(t->setb[r].stat, t->setb[r].layer, t->setb[r].value, t->setb[r].unmapped, &in.hi, &in.lo, &in.varied);
            least_bonus(t->setb[r].stat, t->setb[r].layer, t->setb[r].value, t->setb[r].unmapped, &in.least);
        }
    }
    a->bounds++;
    in.tight = tight; in.order = tight && !g_no_order;
    return dm_gear_bound_eval(p, &p->nowc, &in, o);
}
static void adm_leaf(const dm_gear_pick *pk, const int *choice, int depth, void *ctx) {
    adm *a = ctx;

    int from = 1;
    if (a->have) { from = 1; while (from < depth && a->last[from - 1] == choice[from - 1]) from++; }
    for (int k = from; k < depth; k++) for (int t = 0; t < 2; t++) a->ok[t][k] = !prefix_bound(a, choice, k, t, &a->b[t][k]);
    memcpy(a->last, choice, sizeof *choice * (size_t)depth); a->have = 1;
    if (!brute_fit(&a->g->p, pk->charm)) return;
    dm_gear_measures m; unsigned flags;
    if (dm_gear_leaf_measure(&a->g->p, pk, &m, &flags)) return;
    a->leaves++;
    for (int k = 1; k < depth; k++) for (int t = 0; t < 2; t++) {
        if (!a->ok[t][k]) { CHECK(0, "prefix bound failed"); continue; }
        const dm_gear_bound_out *b = &a->b[t][k];
        a->checks++; a->finite += isfinite(b->group) && isfinite(b->single);
        a->tighter += t && a->ok[0][k] && b->single < a->b[0][k].single;
        a->recovery += t && isfinite(b->recovery); a->death += t && b->survival_s < DM_SIM_HORIZON / (double)DM_SIM_FPS;
        a->hits += t && a->ok[0][k] && b->hits < a->b[0][k].hits;
        CHECK(le(m.group, b->group) && le(m.single, b->single) && le(m.hits, b->hits) && le(m.survival_s, b->survival_s) && le(m.recovery, b->recovery),
              "leaf above the %s bound of its depth %d prefix: group %g / %g single %g / %g hits %g / %g survival %g / %g recovery %g / %g", t ? "tight" : "coarse",
              k, m.group, b->group, m.single, b->single, m.hits, b->hits, m.survival_s, b->survival_s, m.recovery, b->recovery);
    }
}



static void exact_leaf(const dm_gear_pick *pk, const int *choice, int depth, void *ctx) {
    adm *a = ctx;
    if (!brute_fit(&a->g->p, pk->charm)) return;
    dm_gear_measures m; unsigned flags;
    if (dm_gear_leaf_measure(&a->g->p, pk, &m, &flags)) return;
    a->leaves++;
    for (int t = 0; t < 2; t++) {
        dm_gear_bound_out b; a->bounds++;
        if (prefix_bound(a, choice, depth, t, &b)) { CHECK(0, "decided bound evaluation failed"); continue; }
        a->checks++; a->finite += isfinite(b.group) && isfinite(b.single);
        CHECK(le(m.group, b.group) && le(m.single, b.single) && le(m.hits, b.hits) && le(m.survival_s, b.survival_s) && le(m.recovery, b.recovery),
              "leaf above the %s bound of its fully decided input: group %g / %g single %g / %g hits %g / %g survival %g / %g recovery %g / %g",
              t ? "tight" : "coarse", m.group, b.group, m.single, b.single, m.hits, b.hits, m.survival_s, b.survival_s, m.recovery, b.recovery);
    }
}



static void casts_brute(const dm_gear_casts *x, int k, double span, double mana, double dmg, const double *spent, double *most) {
    if (k == x->n) {
        if (span < 0 || mana < 0) return;
        for (int g = 0; g < x->ng; g++) if (spent[g] > x->cap[g]) return;
        if (dmg > *most) *most = dmg;
        return;
    }
    for (int v = 0; v <= (int)x->uses[k]; v++) {
        double next[DM_GEAR_CASTS_MAXG];
        for (int g = 0; g < x->ng; g++) next[g] = spent[g] + ((x->below[g] >> k & 1) ? v * x->c[k] : -v * x->credit[k]);
        casts_brute(x, k + 1, span - v * x->f[k], mana - v * x->c[k], dmg + v * x->d[k], next, most);
    }
}
static int casts_ask(const dm_gear_casts *x, double target, double *best) { long nodes = 1000000; return dm_gear_casts_reach(x, target, &nodes, best); }
static void casts_cases(long *checks) {
    dm_gear_casts x; double best;


    for (int order = 0; order < 2; order++) {
        memset(&x, 0, sizeof x); x.n = 2; x.span = 3; x.mana = 3;
        int a = order, b = 1 - order;
        x.d[a] = 100; x.f[a] = 1; x.c[a] = 2; x.credit[a] = 1; x.uses[a] = 3;
        x.d[b] = 20; x.f[b] = 1; x.c[b] = -2; x.credit[b] = 2; x.uses[b] = 3;
        CHECK(casts_ask(&x, INFINITY, &best) == 0 && best == 220, "leech A, B, A with A at %d: most %g, not 220", a, best);
        CHECK(casts_ask(&x, 220, NULL) == 1 && casts_ask(&x, 221, NULL) == 0, "leech A, B, A with A at %d: reach of 220 / 221", a);

        x.ng = 1; x.below[0] = 1u << a; x.cap[0] = 3;
        CHECK(casts_ask(&x, INFINITY, &best) == 0 && best == 220, "leech below a main spell with A at %d: most %g, not 220", a, best);
        (*checks) += 3;
    }


    gs_seed = 4242;
    for (int t = 0; t < 3500; t++) {
        int wide = t >= 3000;
        memset(&x, 0, sizeof x);
        x.n = wide ? 6 : gs_int(1, 4); x.ng = wide ? 4 : gs_int(0, 2); x.span = gs_int(0, 10); x.mana = gs_int(0, 15);
        for (int k = 0; k < x.n; k++) { x.d[k] = gs_int(0, 20); x.f[k] = gs_int(1, 3); x.c[k] = gs_int(-3, 5); x.credit[k] = gs_int(0, 3); x.uses[k] = gs_int(0, wide ? 3 : 5); }
        for (int g = 0; g < x.ng; g++) { x.below[g] = (unsigned)gs_int(1, (1 << x.n) - 1); x.cap[g] = gs_int(0, 10); }
        double most = -1, zero[DM_GEAR_CASTS_MAXG] = { 0 };
        casts_brute(&x, 0, x.span, x.mana, 0, zero, &most);
        for (int rev = 0; rev < 2; rev++) {
            dm_gear_casts y = x;
            if (rev) for (int k = 0; k < x.n; k++) {
                int j = x.n - 1 - k;
                y.d[k] = x.d[j]; y.f[k] = x.f[j]; y.c[k] = x.c[j]; y.credit[k] = x.credit[j]; y.uses[k] = x.uses[j];
                for (int g = 0; g < x.ng; g++) y.below[g] = (y.below[g] & ~(1u << k)) | ((x.below[g] >> j & 1) << k);
            }
            int r = casts_ask(&y, INFINITY, &best);
            CHECK(r == 0 && best == fmax(most, 0), "random case %d%s: most %g, brute force %g", t, rev ? " reversed" : "", best, most);
            if (most >= 0) CHECK(casts_ask(&y, most, NULL) == 1 && casts_ask(&y, most + 1, NULL) == 0, "random case %d%s: reach around %g", t, rev ? " reversed" : "", most);
            (*checks)++;
        }
    }


    for (int prio = 0; prio < 2; prio++) {
        double budget[3] = { 9, 9 * (1 - 1e-12), 8.99 }, want[3] = { 30, 30, 20 };
        for (int k = 0; k < 3; k++) {
            memset(&x, 0, sizeof x); x.n = 1; x.span = 10; x.d[0] = 10; x.f[0] = 1; x.c[0] = 3; x.uses[0] = 5;
            if (prio) { x.mana = 100; x.ng = 1; x.below[0] = 1; x.cap[0] = budget[k]; } else x.mana = budget[k];
            CHECK(casts_ask(&x, INFINITY, &best) == 0 && best == want[k], "%s budget %.17g: most %g, not %g", prio ? "priority" : "mana", budget[k], best, want[k]);
            (*checks)++;
        }
    }
    memset(&x, 0, sizeof x); x.n = 1; x.span = 10; x.mana = 9; x.d[0] = 10; x.f[0] = 1; x.c[0] = 3; x.uses[0] = 5;
    CHECK(casts_ask(&x, 30, NULL) == 1 && casts_ask(&x, 30 * (1 + 1e-12), NULL) == 1 && casts_ask(&x, 30.01, NULL) == 0, "damage threshold around 30");

    memset(&x, 0, sizeof x); x.n = 1; x.span = 25; x.d[0] = 1.16; x.f[0] = 1; x.uses[0] = 25;
    CHECK(casts_ask(&x, 29, NULL) == 1, "25 hits of 1.16 do not reach 29");

    CHECK(casts_ask(&x, 30, &best) == 0, "25 hits of 1.16 reach 30");

    memset(&x, 0, sizeof x); x.n = 3; x.span = 30; x.mana = 1000;
    for (int k = 0; k < 3; k++) { x.d[k] = 10 + k; x.f[k] = 2 + k; x.c[k] = 1; x.uses[k] = 10; }
    long nodes = 1;
    CHECK(dm_gear_casts_reach(&x, INFINITY, &nodes, &best) == -1, "one node answers");
    nodes = 1000000; double most = 0; dm_gear_casts_reach(&x, INFINITY, &nodes, &most);
    long used = 1000000 - nodes; nodes = used + used / 2;
    CHECK(dm_gear_casts_reach(&x, INFINITY, &nodes, &best) == 0 && best == most && dm_gear_casts_reach(&x, INFINITY, &nodes, &best) == -1,
          "a budget for one and a half walks answers twice");
    (*checks) += 6;
}




static dm_gear_walk g_walk;
static long g_walks, g_last_dual;
static void walk_stock(gstock *g, const char *what, unsigned on, unsigned off) {
    dm_gear_search *s = dm_gear_search_new(&g->p);
    if (!s) { CHECK(0, "%s: no search context", what); return; }
    dm_gear_walk w;
    int rc = dm_gear_search_test_walk(s, &w);
    dm_gear_search_free(s);
    CHECK(!rc && w.leaves > 0, "%s: walk failed or no rated leaf", what);
    CHECK(w.above == 0, "%s: %ld of %ld rated leaves above the bound of a node they lie under", what, w.above, w.leaves);
    for (int k = 0; k < DM_GEAR_WALK_HOW; k++) {
        if (on >> k & 1) CHECK(w.how[k] > 0, "%s: no node bound with how bit %d (%ld bounds)", what, k, w.bounds);
        if (off >> k & 1) CHECK(w.how[k] == 0, "%s: %ld node bounds with how bit %d", what, w.how[k], k);
    }
    if (getenv("T_GEAR_WALK_VERBOSE")) {
        fprintf(stderr, "walk %-40s leaves %6ld bounds %6ld halves %4ld |", what, w.leaves, w.bounds, w.halves);
        for (int k = 0; k < DM_GEAR_WALK_HOW; k++) fprintf(stderr, " %ld", w.how[k]);
        fputc('\n', stderr);
    }
    g_walk.leaves += w.leaves; g_walk.bounds += w.bounds; g_walk.halves += w.halves; g_walk.above += w.above; g_walk.dual += w.dual;
    g_last_dual = w.dual;
    for (int k = 0; k < DM_GEAR_WALK_HOW; k++) g_walk.how[k] += w.how[k];
    g_walks++;
}




static const char *const k_lean = "class=7\nlevel=20\ndiff=0\nlife=300\nmana=90\nstr=30\ndex=30\nvit=40\nene=60\nar=900\ndef=400\n"
                                  "weapon=20,40,100,0,0,1,0,0,1HS\nres=0,30,30,30,30,0\n"
                                  "skill=394,12,12\nbar=394\nskill=398,3,3\nbar=398\nskill=395,4,4\nbar=395\n";
static gstock g_case;

enum { PS_ORDERED = 1, PS_SKILLS = 2, PS_KICKS = 4 };
static int premise_stock(const char *base, const char *extra, int flags, int mana) {
    static char prof[2048]; snprintf(prof, sizeof prof, "%s%s", base, extra);
    gspec s; memset(&s, 0, sizeof s);
    s.profile = prof; s.act = 1; s.seed = 1311; s.variants = 1; s.mana = mana; s.skills = !!(flags & PS_SKILLS);
    s.count[DM_GEAR_K_RING] = 1; s.count[DM_GEAR_K_AMULET] = 1; s.count[DM_GEAR_K_GLOVES] = 1; s.count[DM_GEAR_K_HEAD] = 1; s.count[DM_GEAR_K_BOOTS] = 1;
    if (gs_build(&g_case, g_d, &g_gt, &s)) return -1;
    if (flags & PS_KICKS) {
        int n = 0;
        for (int i = 0; i < g_case.p.ncand; i++) if (g_case.cand[i].kind == DM_GEAR_K_BOOTS) {
            g_case.cand[i].dmin = n ? 30 : 2; g_case.cand[i].dmax = n ? 70 : 4; n++;
        }
        if (n < 2 || gs_capture(&g_case)) return -1;
    }
    g_case.p.now.ordered = !!(flags & PS_ORDERED);
    dm_gear_stat_classes(&g_case.p, g_cls);
    return 0;
}
static void premise_case(const char *what, const char *base, const char *extra, int flags, int mana, unsigned on, unsigned off) {
    if (premise_stock(base, extra, flags, mana)) { CHECK(0, "%s: stock not built", what); return; }
    walk_stock(&g_case, what, on, off);
    brute_plan *pl = malloc(sizeof *pl); if (!pl) abort();
    brute_make_plan(&g_case.p, pl);
    adm *a = calloc(1, sizeof *a); if (!a) abort();
    a->g = &g_case; a->pl = pl;
    int before = failures;
    brute_walk(&g_case.p, pl, adm_leaf, a);
    CHECK(failures == before && a->leaves > 0, "%s: %d leaves above the oracle's prefix bounds", what, failures - before);
    free(a); free(pl);
}


static void nodes_leaf(const dm_gear_pick *pk, const int *choice, int depth, void *ctx) {
    adm *a = ctx; (void)pk;
    int from = 1;
    if (a->have) while (from < depth && a->last[from - 1] == choice[from - 1]) from++;
    memcpy(a->last, choice, sizeof *choice * (size_t)depth); a->have = 1;
    for (int k = from; k < depth; k++) {
        dm_gear_bound_out o, z;
        if (prefix_bound(a, choice, k, 1, &o)) { CHECK(0, "prefix bound failed"); continue; }
        if (!(o.how & DM_GEAR_HOW_NODES) || (o.how & DM_GEAR_HOW_CASTS)) continue;
        g_no_order = 1; int rc = prefix_bound(a, choice, k, 1, &z); g_no_order = 0;
        a->checks++;
        CHECK(!rc && o.single == z.single && o.group == z.group && o.survival_s == z.survival_s && o.hits == z.hits && o.recovery == z.recovery,
              "depth %d prefix out of nodes: single %.17g, without the order %.17g", k, o.single, z.single);
    }
}
static void nodes_case(void) {
    if (premise_stock(k_lean, "skill=92,1,1\nbar=92\n", 0, 1)) { CHECK(0, "out of nodes: stock not built"); return; }
    brute_plan *pl = malloc(sizeof *pl); if (!pl) abort();
    brute_make_plan(&g_case.p, pl);
    adm *a = calloc(1, sizeof *a); if (!a) abort();
    a->g = &g_case; a->pl = pl;
    brute_walk(&g_case.p, pl, nodes_leaf, a);
    CHECK(a->checks > 0, "lean caster, poison: no prefix bound ran out of nodes");
    if (getenv("T_GEAR_WALK_VERBOSE")) fprintf(stderr, "out of nodes: %ld prefix bounds equal without the order\n", a->checks);
    free(a); free(pl);
}



static void dual_values(gstock *g, int i) {
    int w[8], n = 0;
    for (int q = 0; q < g->p.ncand && n < 8; q++) if (g->cand[q].kind == DM_GEAR_K_WEAPON1) w[n++] = q;
    if (n < 2) return;
    dm_gear_list *a = &g->cand[w[0]].item.own, *b = &g->cand[w[1]].item.own;
    if (i % 2) dm_gear_list_add(a, 60, 0, 8);
    if (i % 3 == 1) dm_gear_list_add(b, 62, 0, 9);
    if (i % 4 >= 2) { dm_gear_list_add(a, 57, 0, 300); dm_gear_list_add(a, 58, 0, 500); dm_gear_list_add(a, 59, 0, 75); }
    if (i % 4 == 3) { dm_gear_list_add(b, 57, 0, 200); dm_gear_list_add(b, 58, 0, 260); dm_gear_list_add(b, 59, 0, 125); }
    if (i % 5 == 2) dm_gear_list_add(b, 93, 0, 20);
    if (i % 6 == 4 && g->p.ncand < GS_MAXCAND) { dm_gear_cand *x = &g->cand[g->p.ncand++]; *x = g->cand[w[n - 1]]; x->worn = 0; x->item.slot = 0; x->item.id += 500; }
}

typedef struct { const gstock *g; long shield, dual; } sword_count;
static void sword_leaf(const dm_gear_pick *pk, const int *choice, int depth, void *ctx) {
    (void)choice; (void)depth;
    sword_count *c = ctx; int m = pk->at[DM_GEAR_P_MAIN], o = pk->at[DM_GEAR_P_OFF];
    if ((c->shield && c->dual) || m < 0 || o < 0 || (c->g->cand[m].kind != DM_GEAR_K_WEAPON2 && c->g->cand[o].kind != DM_GEAR_K_WEAPON2)) return;
    dm_gear_measures x; unsigned fl;
    if (dm_gear_leaf_measure(&c->g->p, pk, &x, &fl)) return;
    if (c->g->cand[o].kind == DM_GEAR_K_SHIELD) c->shield++; else c->dual++;
}




static double g_tight_most;
static long g_tight_leaves;
static void tight_leaf(const dm_gear_pick *pk, const int *choice, int depth, void *ctx) {
    adm *a = ctx;
    if (!brute_fit(&a->g->p, pk->charm)) return;
    dm_gear_measures m; unsigned flags;
    if (dm_gear_leaf_measure(&a->g->p, pk, &m, &flags)) return;
    dm_gear_bound_out b;
    if (prefix_bound(a, choice, depth, 1, &b)) { CHECK(0, "plain fight: decided bound evaluation failed"); return; }
    if (!(m.survival_s < DM_SIM_HORIZON / (double)DM_SIM_FPS)) return;
    g_tight_leaves++;
    g_tight_most = fmax(g_tight_most, b.hits / m.hits);
    CHECK(le(m.hits, b.hits) && b.hits <= m.hits * 1.001, "plain fight: hits %g, bound of the decided input %g", m.hits, b.hits);
}
static void tight_case(void) {
    static const char *const plain = "class=7\nlevel=7\ndiff=0\nlife=120\nmana=40\nstr=25\ndex=20\nvit=30\nene=20\nar=60\ndef=20\n"
                                     "weapon=2,5,100,0,0,1,0,0,1HS\n";
    static gstock g;
    gspec s; memset(&s, 0, sizeof s);
    s.profile = plain; s.act = 4; s.seed = 7094; s.variants = 1;
    s.count[DM_GEAR_K_ARMOR] = 1; s.count[DM_GEAR_K_AMULET] = 1; s.count[DM_GEAR_K_RING] = 1; s.count[DM_GEAR_K_HEAD] = 1;
    s.count[DM_GEAR_K_BOOTS] = 1; s.count[DM_GEAR_K_GLOVES] = 1; s.count[DM_GEAR_K_BELT] = 1;
    if (gs_build(&g, g_d, &g_gt, &s)) { CHECK(0, "plain fight stock not built"); return; }
    dm_gear_stat_classes(&g.p, g_cls);
    brute_plan *pl = malloc(sizeof *pl); if (!pl) abort();
    brute_make_plan(&g.p, pl);
    adm *a = calloc(1, sizeof *a); if (!a) abort();
    a->g = &g; a->pl = pl;
    g_exact = 1; brute_walk(&g.p, pl, tight_leaf, a); g_exact = 0;
    CHECK(g_tight_leaves > 1000, "plain fight: only %ld leaves die within the horizon", g_tight_leaves);
    printf("plain fight: %ld leaves, the hits bound of the decided input at most %.5f times the leaf's\n", g_tight_leaves, g_tight_most);
    free(a); free(pl);
}

int main(int argc, char **argv) {
    g_d = gs_tables(&g_gt);
    for (int i = 0; i < 9; i++) g_profile[i] = gs_profile(k_profiles[i]);
    int nstocks = argc > 1 ? atoi(argv[1]) : 40;
    static gstock g;
    long mono = 0, stocks = 0, leaves = 0, checks = 0, bounds = 0, finite = 0, tighter = 0, recovery = 0, death = 0, hits = 0, casts = 0;
    double t0 = now_s();
    casts_cases(&casts);
    for (int i = 0; stocks < nstocks; i++) {
        gspec s; memset(&s, 0, sizeof s);
        gs_seed = (uint64_t)i * 104729 + 3;
        s.profile = g_profile[gs_int(0, 8)]; s.act = gs_int(1, 5); s.seed = (uint64_t)i + 1000; s.variants = 1;
        for (int k = DM_GEAR_K_HEAD; k < DM_GEAR_K_N; k++) if (k != DM_GEAR_K_CHARM && k != DM_GEAR_K_BOW && k != DM_GEAR_K_XBOW && k != DM_GEAR_K_ARROWS && k != DM_GEAR_K_BOLTS) s.count[k] = gs_int(0, k == DM_GEAR_K_RING ? 2 : 1);
        s.charms_inv = gs_int(0, 1); s.charms_stash = gs_int(0, 2); s.skills = gs_int(0, 1); s.sets = gs_int(0, 3) == 0 ? 2 : 0; s.chain = gs_int(0, 2) == 0;
        s.mana = gs_int(0, 4) == 0 ? gs_int(1, 2) : 0;
        if (gs_build(&g, g_d, &g_gt, &s)) continue;
        g.p.splash_on = i % 2;
        dm_gear_stat_classes(&g.p, g_cls);
        monotone(&g, &mono);
        brute_plan *pl = malloc(sizeof *pl); if (!pl) abort();
        brute_make_plan(&g.p, pl);
        double n = ldexp(1, pl->nfree); for (int k = 0; k < 8; k++) n *= pl->place[k].n;
        if (n > 3e4) { free(pl); continue; }
        adm *a = calloc(1, sizeof *a); if (!a) abort();
        a->g = &g; a->pl = pl;
        brute_walk(&g.p, pl, adm_leaf, a);
        walk_stock(&g, "random stock", 0, 0);
        leaves += a->leaves; checks += a->checks; bounds += a->bounds; finite += a->finite; tighter += a->tighter; recovery += a->recovery; death += a->death; hits += a->hits; stocks++;
        free(a); free(pl);
    }




    static const char *const lean = "class=7\nlevel=20\ndiff=0\nlife=300\nmana=90\nstr=30\ndex=30\nvit=40\nene=60\nar=900\ndef=400\n"
                                    "weapon=20,40,100,0,0,1,0,0,1HS\nres=0,30,30,30,30,0\n"
                                    "skill=394,12,12\nbar=394\nskill=398,3,3\nbar=398\nskill=395,4,4\nbar=395\n";
    for (int mana = 0; mana <= 2; mana++) for (int act = 1; act <= 5; act += 2) {
        gspec s; memset(&s, 0, sizeof s);
        s.profile = lean; s.act = act; s.seed = 1300 + (uint64_t)(10 * mana + act); s.variants = 1; s.mana = mana; s.skills = 1;
        s.count[DM_GEAR_K_RING] = 1; s.count[DM_GEAR_K_AMULET] = 1; s.count[DM_GEAR_K_GLOVES] = 1; s.count[DM_GEAR_K_HEAD] = 1;
        if (gs_build(&g, g_d, &g_gt, &s)) { CHECK(0, "lean caster stock mana %d not built", mana); continue; }
        dm_gear_stat_classes(&g.p, g_cls);
        monotone(&g, &mono);
        brute_plan *pl = malloc(sizeof *pl); if (!pl) abort();
        brute_make_plan(&g.p, pl);
        adm *a = calloc(1, sizeof *a); if (!a) abort();
        a->g = &g; a->pl = pl;
        int before = failures;
        brute_walk(&g.p, pl, adm_leaf, a);
        g_exact = 1;
        brute_walk(&g.p, pl, exact_leaf, a);
        g_exact = 0;
        char what[64]; snprintf(what, sizeof what, "lean caster mana %d act %d", mana, act);
        walk_stock(&g, what, 0, 0);
        CHECK(a->leaves > 0 && a->finite > 0, "lean caster stock mana %d act %d: no finite bound checked", mana, act);
        CHECK(failures == before, "lean caster stock mana %d act %d: %d leaves above their bounds", mana, act, failures - before);
        leaves += a->leaves; checks += a->checks; bounds += a->bounds; finite += a->finite; tighter += a->tighter; recovery += a->recovery;
        death += a->death; hits += a->hits; stocks++;
        free(a); free(pl);
    }

    static const char *const resource[] = {
        "class=3\nlevel=80\ndiff=2\nact=1\nlife=1200\nmana=100\nstr=100\ndex=100\nvit=80\nene=40\nar=2000\ndef=800\nfcr=63\n"
        "weapon=20,40,100,0,0,1,0,0,1HS\nres=0,75,75,75,75,0\nskill=112,20,20\nbar=112\nskill=120,20,20\nbar=120\n",
        "class=4\nlevel=80\ndiff=2\nact=1\nlife=1200\nmana=60\nstr=150\ndex=100\nvit=80\nene=20\nar=2000\ndef=800\nias=20\n"
        "weapon=60,120,100,0,0,1,0,0,1HS\nres=0,75,75,75,75,0\nskill=147,20,20\nbar=147\nskill=149,20,20\nbar=149\n" };
    for (int i = 0; i < 2; i++) for (int act = 1; act <= 5; act += 2) {
        gspec s; memset(&s, 0, sizeof s);
        s.profile = resource[i]; s.act = act; s.seed = 500 + (uint64_t)act; s.variants = 1; s.count[DM_GEAR_K_RING] = 1; s.count[DM_GEAR_K_AMULET] = 1;
        if (gs_build(&g, g_d, &g_gt, &s)) { CHECK(0, "resource stock %d not built", i); continue; }
        dm_gear_stat_classes(&g.p, g_cls);
        brute_plan *pl = malloc(sizeof *pl); if (!pl) abort();
        brute_make_plan(&g.p, pl);
        adm *a = calloc(1, sizeof *a); if (!a) abort();
        a->g = &g; a->pl = pl;
        int before = failures;
        brute_walk(&g.p, pl, adm_leaf, a);
        CHECK(a->leaves > 0 && a->finite > 0, "resource stock %d act %d: no finite bound checked", i, act);
        CHECK(failures == before, "resource stock %d act %d: leaves above their bounds", i, act);
        leaves += a->leaves; checks += a->checks; bounds += a->bounds; finite += a->finite; tighter += a->tighter; recovery += a->recovery; death += a->death; hits += a->hits; stocks++;
        free(a); free(pl);
    }



    static const char *const granted[] = {
        "grant=aura,102,16,0,0\n", "grant=hit,47,15,25,0\ngrant=attack,47,10,10,0\ngrant=struck,47,12,30,0\n",
        "grant=aura,98,10,0,0\ngrant=obuff,149,10,0,10\n" };
    for (int i = 0; i < 3; i++) for (int act = 1; act <= 5; act += 2) {
        char prof[1024];
        snprintf(prof, sizeof prof, "class=3\nlevel=80\ndiff=2\nact=1\nlife=1200\nmana=300\nstr=150\ndex=100\nvit=80\nene=40\nar=3000\ndef=1200\nias=20\n"
                 "weapon=60,120,100,0,0,1,0,0,1HS\nres=0,75,75,75,75,0\nskill=106,20,20\nbar=106\n%s", granted[i]);
        gspec s; memset(&s, 0, sizeof s);
        s.profile = prof; s.act = act; s.seed = 700 + (uint64_t)(10 * i + act); s.variants = 1;
        s.count[DM_GEAR_K_RING] = 1; s.count[DM_GEAR_K_AMULET] = 1; s.count[DM_GEAR_K_GLOVES] = 1; s.count[DM_GEAR_K_HEAD] = 1; s.skills = 1;
        if (gs_build(&g, g_d, &g_gt, &s)) { CHECK(0, "grant stock %d not built", i); continue; }
        dm_gear_stat_classes(&g.p, g_cls);
        monotone(&g, &mono);
        brute_plan *pl = malloc(sizeof *pl); if (!pl) abort();
        brute_make_plan(&g.p, pl);
        adm *a = calloc(1, sizeof *a); if (!a) abort();
        a->g = &g; a->pl = pl;
        int before = failures;
        brute_walk(&g.p, pl, adm_leaf, a);
        CHECK(a->leaves > 0 && a->finite > 0, "grant stock %d act %d: no finite bound checked", i, act);
        if (failures != before) fprintf(stderr, "FAIL grant stock %d act %d: %d leaves above their bounds\n", i, act, failures - before);
        leaves += a->leaves; checks += a->checks; bounds += a->bounds; finite += a->finite; tighter += a->tighter; recovery += a->recovery; death += a->death; hits += a->hits; stocks++;
        free(a); free(pl);
    }







    static const char *const hurt0 = "class=3\nlevel=80\ndiff=2\nact=1\nlife=1200\nmana=600\nstr=150\ndex=120\nvit=80\nene=40\nar=4000\ndef=1600\n"
                                     "fcr=63\nias=40\nweapon=100,160,100,0,0,1,0,0,1HS\nshield=1\nres=15,140,140,140,140,0\n"
                                     "skill=106,20,20\nbar=106\nskill=122,20,20\nbar=122\nskill=117,20,20\nbar=117\n";
    static const char *const hurt[] = { "", "skill=114,20,20\nbar=114\n", "skill=114,20,20\nbar=114\nskill=113,20,20\nbar=113\n",
                                        "skill=114,20,20\nbar=114\nskill=113,20,20\nbar=113\n",
                                        "aura=122\ngrant=aura,122,15,0,0\nskill=113,20,20\nbar=113\n",
                                        "aura=113\nskill=113,10,10\nbar=113\ngrant=aura,113,20,0,0\n" };
    for (int i = 1; i <= 6; i++) for (int act = 1; act <= 5; act += 2) {
        char prof[2048]; snprintf(prof, sizeof prof, "%s%s", hurt0, hurt[i - 1]);
        gspec s; memset(&s, 0, sizeof s);
        s.profile = prof; s.act = act; s.seed = 900 + (uint64_t)(10 * i + act); s.variants = 1;
        s.count[DM_GEAR_K_RING] = 1; s.count[DM_GEAR_K_AMULET] = 1;
        if (gs_build(&g, g_d, &g_gt, &s)) { CHECK(0, "hurt stock %d not built", i); continue; }
        if (i == 4) {
            for (int q = 0; q < g.p.ncand; q++) {
                dm_gear_list *l = &g.cand[q].item.own; int n = 0;
                for (int v = 0; v < l->n; v++) { int st = l->v[v].stat; const char *nm = g_d->stat_name[st];
                    if (strcmp(nm, "hpregen") && strcmp(nm, "lifedrainmindam")) l->v[n++] = l->v[v]; }
                l->n = n;
            }
            int st = gs_stat(g_d, "hpregen");
            for (int q = 0; q < g.p.ncand; q++) {
                int k = g.cand[q].kind;
                if (k == DM_GEAR_K_ARMOR || k == DM_GEAR_K_RING || k == DM_GEAR_K_AMULET || k == DM_GEAR_K_HEAD || k == DM_GEAR_K_BELT)
                    dm_gear_list_add(&g.cand[q].item.own, st, 0, -(4 + 2 * q) * 256);
            }
            if (gs_capture(&g)) { CHECK(0, "hurt stock %d act %d: no capture", i, act); continue; }
        }
        dm_gear_stat_classes(&g.p, g_cls);
        brute_plan *pl = malloc(sizeof *pl); if (!pl) abort();
        brute_make_plan(&g.p, pl);
        adm *a = calloc(1, sizeof *a); if (!a) abort();
        a->g = &g; a->pl = pl;
        int before = failures;
        brute_walk(&g.p, pl, adm_leaf, a);
        g_exact = 1;
        brute_walk(&g.p, pl, exact_leaf, a);
        g_exact = 0;
        CHECK(a->leaves > 0 && a->finite > 0, "hurt stock %d act %d: no finite bound checked", i, act);
        if (failures != before) fprintf(stderr, "FAIL hurt stock %d act %d: %d leaves above their bounds\n", i, act, failures - before);
        leaves += a->leaves; checks += a->checks; bounds += a->bounds; finite += a->finite; tighter += a->tighter; recovery += a->recovery; death += a->death; hits += a->hits; stocks++;
        free(a); free(pl);
    }


    {
        enum { CASTS = DM_GEAR_HOW_CASTS };

        premise_case("lean caster", k_lean, "", 0, 1, CASTS | DM_GEAR_HOW_A_READY, DM_GEAR_HOW_P_GRANTS | DM_GEAR_HOW_P_BUFF | DM_GEAR_HOW_P_AURAS);
        premise_case("lean caster, potions", k_lean, "", 0, 2, CASTS, 0);

        premise_case("lean caster, one spell", "class=7\nlevel=20\ndiff=0\nlife=300\nmana=90\nstr=30\ndex=30\nvit=40\nene=60\nar=900\ndef=400\n"
                     "weapon=20,40,100,0,0,1,0,0,1HS\nres=0,30,30,30,30,0\n", "skill=394,12,12\nbar=394\n", 0, 1, DM_GEAR_HOW_P_NONE, CASTS);
        premise_case("lean caster, fixed order", k_lean, "", PS_ORDERED, 1, DM_GEAR_HOW_P_ORDERED, CASTS);

        premise_case("lean caster, gear skill bonuses", k_lean, "", PS_SKILLS, 1, CASTS | DM_GEAR_HOW_A_KIND, 0);

        premise_case("lean caster, curse", k_lean, "skill=385,6,6\nbar=385\n", 0, 1, CASTS, 0);

        premise_case("lean caster, curse and hit skill", k_lean, "skill=385,6,6\nbar=385\ngrant=hit,47,15,25,0\n", 0, 1, DM_GEAR_HOW_P_GRANTS, CASTS);
        premise_case("lean caster, attack skill", k_lean, "grant=attack,47,10,10,0\n", 0, 1, DM_GEAR_HOW_P_GRANTS, CASTS);
        premise_case("lean caster, charges", k_lean, "grant=charge,53,10,20,0\n", 0, 1, DM_GEAR_HOW_P_GRANTS, CASTS);
        premise_case("lean caster, buff", k_lean, "skill=379,4,4\nbar=379\n", 0, 1, DM_GEAR_HOW_P_BUFF, CASTS);
        premise_case("lean caster, second aura", k_lean, "skill=113,6,6\nbar=113\n", 0, 1, DM_GEAR_HOW_P_AURAS, CASTS);


        premise_case("lean caster, summon", k_lean, "skill=373,4,4\nbar=373\n", 0, 1, CASTS, 0);
        premise_case("lean caster, poison", k_lean, "skill=92,1,1\nbar=92\n", 0, 1, CASTS | DM_GEAR_HOW_X_TERMS | DM_GEAR_HOW_NODES, 0);
        nodes_case();



        static const char *const sin = "class=6\nlevel=24\ndiff=0\nlife=300\nmana=40\nstr=40\ndex=40\nvit=40\nene=30\nar=900\ndef=400\n"
                                       "weapon=8,14,100,0,0,1,0,0,1HS\nres=0,30,30,30,30,0\n";
        premise_case("kicks, light and heavy boots", sin, "skill=255,4,4\nbar=255\nskill=251,20,20\nbar=251\n", PS_KICKS, 0, CASTS, 0);
        premise_case("charge-up with a kick finisher", sin, "skill=254,4,4\nbar=254\nskill=255,4,4\nbar=255\nskill=251,20,20\nbar=251\n", PS_KICKS, 0, CASTS, 0);

        if (!premise_stock(k_lean, "", 0, 1)) {
            brute_plan *pl = malloc(sizeof *pl); if (!pl) abort();
            brute_make_plan(&g_case.p, pl);
            adm *a = calloc(1, sizeof *a); if (!a) abort();
            a->g = &g_case; a->pl = pl;
            int choice[8 + DM_GEAR_MAXCAND] = { 0 }; dm_gear_bound_out b;
            CHECK(!prefix_bound(a, choice, 0, 1, &b) && (b.how & DM_GEAR_HOW_P_HANDS) && !(b.how & CASTS), "lean caster, hands open: how %#x", b.how);
            free(a); free(pl);
        } else CHECK(0, "lean caster, hands open: stock not built");
    }



    for (int i = 0; i < 12; i++) {
        gspec s; memset(&s, 0, sizeof s);
        gs_seed = (uint64_t)i * 7907 + 11;
        s.profile = g_profile[4]; s.act = 1 + i % 5; s.seed = 4000 + (uint64_t)i; s.variants = 1; s.dual = 1; s.mana = i % 3;
        s.count[DM_GEAR_K_WEAPON1] = 2 + i % 3; s.count[DM_GEAR_K_RING] = 1; s.count[DM_GEAR_K_GLOVES] = i % 2; s.count[DM_GEAR_K_SHIELD] = i % 4 == 0;
        s.count[DM_GEAR_K_WEAPON2] = i % 4 == 1; s.skills = i % 2; s.chain = i % 3 == 0;
        if (gs_build(&g, g_d, &g_gt, &s)) { CHECK(0, "dual stock %d not built", i); continue; }
        dual_values(&g, i);
        if (gs_capture(&g)) { CHECK(0, "dual stock %d: no capture", i); continue; }
        g.p.splash_on = i % 2;
        dm_gear_stat_classes(&g.p, g_cls);
        monotone(&g, &mono);
        brute_plan *pl = malloc(sizeof *pl); if (!pl) abort();
        brute_make_plan(&g.p, pl);
        adm *a = calloc(1, sizeof *a); if (!a) abort();
        a->g = &g; a->pl = pl;
        int before = failures;
        brute_walk(&g.p, pl, adm_leaf, a);
        char what[64]; snprintf(what, sizeof what, "dual stock %d", i);
        walk_stock(&g, what, 0, 0);
        CHECK(g_last_dual > 0, "%s: no rated leaf with two weapons", what);
        CHECK(failures == before, "%s: %d leaves above their bounds", what, failures - before);
        leaves += a->leaves; checks += a->checks; bounds += a->bounds; finite += a->finite; tighter += a->tighter; recovery += a->recovery; death += a->death; hits += a->hits; stocks++;
        free(a); free(pl);
    }



    for (int i = 0; i < 8; i++) {
        static const char *const claw6 = "class=6\nlevel=70\ndiff=1\nlife=900\nmana=300\nstr=120\ndex=120\nar=2500\ndef=900\nias=20\n"
                                         "weapon=20,40,100,0,0,1,0,0,HT1\nres=15,60,60,60,60,0\nskill=252,12,12\nskill=260,15,15\nbar=260\nskill=254,8,8\nbar=254\n";
        gspec s; memset(&s, 0, sizeof s);
        gs_seed = (uint64_t)i * 7933 + 17;
        s.profile = i % 2 ? g_profile[6] : claw6; s.act = 1 + i % 5; s.seed = 4300 + (uint64_t)i; s.variants = 1; s.dual = 1; s.claws = i % 3 == 2 ? 2 : 1;
        s.mana = i % 3; s.count[DM_GEAR_K_WEAPON1] = 2 + i % 2; s.count[DM_GEAR_K_RING] = 1; s.count[DM_GEAR_K_SHIELD] = i % 4 == 0; s.skills = i % 2;
        if (gs_build(&g, g_d, &g_gt, &s)) { CHECK(0, "claw stock %d not built", i); continue; }
        dual_values(&g, i);
        if (gs_capture(&g)) { CHECK(0, "claw stock %d: no capture", i); continue; }
        g.p.splash_on = i % 2;
        dm_gear_stat_classes(&g.p, g_cls);
        monotone(&g, &mono);
        brute_plan *pl = malloc(sizeof *pl); if (!pl) abort();
        brute_make_plan(&g.p, pl);
        adm *a = calloc(1, sizeof *a); if (!a) abort();
        a->g = &g; a->pl = pl;
        int before = failures;
        brute_walk(&g.p, pl, adm_leaf, a);
        char what[64]; snprintf(what, sizeof what, "claw stock %d", i);
        walk_stock(&g, what, 0, 0);
        CHECK(g_last_dual > 0, "%s: no rated leaf with two claws", what);
        CHECK(failures == before, "%s: %d leaves above their bounds", what, failures - before);
        leaves += a->leaves; checks += a->checks; bounds += a->bounds; finite += a->finite; tighter += a->tighter; recovery += a->recovery; death += a->death; hits += a->hits; stocks++;
        free(a); free(pl);
    }



    for (int i = 0; i < GS_HAND_STOCKS; i++) {
        int focus;
        if (gs_hand_stock(&g, g_d, &g_gt, i, 4500 + 40 * (uint64_t)i, &focus)) { CHECK(0, "hand stock %d not built", i); continue; }
        g.p.splash_on = i % 2;
        dm_gear_stat_classes(&g.p, g_cls);
        monotone(&g, &mono);
        brute_plan *pl = malloc(sizeof *pl); if (!pl) abort();
        brute_make_plan(&g.p, pl);
        adm *a = calloc(1, sizeof *a); if (!a) abort();
        a->g = &g; a->pl = pl;
        int before = failures;
        brute_walk(&g.p, pl, adm_leaf, a);
        char what[64]; snprintf(what, sizeof what, "hand stock %d", i);
        walk_stock(&g, what, 0, 0);
        CHECK(i == 2 || i >= 4 || g_last_dual > 0, "%s: no rated leaf with two weapons", what);
        CHECK(failures == before, "%s: %d leaves above their bounds", what, failures - before);
        leaves += a->leaves; checks += a->checks; bounds += a->bounds; finite += a->finite; tighter += a->tighter; recovery += a->recovery; death += a->death; hits += a->hits; stocks++;
        free(a); free(pl);
    }


    for (int i = 0; i < 8; i++) {
        gspec s; memset(&s, 0, sizeof s);
        gs_seed = (uint64_t)i * 7919 + 13;
        s.profile = g_profile[4]; s.act = 1 + i % 5; s.seed = 4200 + (uint64_t)i; s.variants = 1; s.dual = i % 2; s.worn_sword = i % 3 == 0; s.swords = 1 + i % 2;
        s.mana = i % 3; s.count[DM_GEAR_K_WEAPON1] = 1 + i % 2; s.count[DM_GEAR_K_RING] = 1; s.count[DM_GEAR_K_SHIELD] = 1; s.skills = i % 2;
        if (gs_build(&g, g_d, &g_gt, &s)) { CHECK(0, "sword stock %d not built", i); continue; }
        dual_values(&g, i);
        if (gs_capture(&g)) { CHECK(0, "sword stock %d: no capture", i); continue; }
        g.p.splash_on = i % 2;
        dm_gear_stat_classes(&g.p, g_cls);
        monotone(&g, &mono);
        brute_plan *pl = malloc(sizeof *pl); if (!pl) abort();
        brute_make_plan(&g.p, pl);
        adm *a = calloc(1, sizeof *a); if (!a) abort();
        a->g = &g; a->pl = pl;
        int before = failures;
        brute_walk(&g.p, pl, adm_leaf, a);
        char what[64]; snprintf(what, sizeof what, "sword stock %d", i);
        walk_stock(&g, what, 0, 0);
        sword_count sc = { &g, 0, 0 }; brute_walk(&g.p, pl, sword_leaf, &sc);
        CHECK(sc.shield > 0 && sc.dual > 0, "%s: rated leaves with a sword beside a shield %ld, beside a weapon %ld", what, sc.shield, sc.dual);
        CHECK(failures == before, "%s: %d leaves above their bounds", what, failures - before);
        leaves += a->leaves; checks += a->checks; bounds += a->bounds; finite += a->finite; tighter += a->tighter; recovery += a->recovery; death += a->death; hits += a->hits; stocks++;
        free(a); free(pl);
    }


    for (int act = 1; act <= 5; act += 2) {
        static const char *const lean4 = "class=4\nlevel=30\ndiff=0\nlife=400\nmana=25\nstr=80\ndex=60\nvit=60\nene=10\nar=1200\ndef=400\n"
                                        "weapon=20,40,100,0,0,1,0,0,1HS\nres=0,30,30,30,30,0\nskill=147,10,10\nbar=147\nskill=152,8,8\nbar=152\n"
                                        "grant=hit,47,15,25,0\n";
        gspec s; memset(&s, 0, sizeof s);
        s.profile = lean4; s.act = act; s.seed = 4100 + (uint64_t)act; s.variants = 1; s.dual = 1;
        s.count[DM_GEAR_K_WEAPON1] = 2; s.count[DM_GEAR_K_RING] = 2; s.count[DM_GEAR_K_AMULET] = 1;
        if (gs_build(&g, g_d, &g_gt, &s)) { CHECK(0, "trigger leech stock act %d not built", act); continue; }
        int ml = gs_stat(g_d, "manadrainmindam"), first = 1;
        for (int q = 0; q < g.p.ncand; q++) {
            dm_gear_cand *x = &g.cand[q];
            if (x->kind == DM_GEAR_K_WEAPON1) { if (first) dm_gear_list_add(&x->item.own, ml, 0, 12); first = 0; }
            if (x->kind == DM_GEAR_K_RING || x->kind == DM_GEAR_K_AMULET) { dm_gear_list_add(&x->item.own, 17, 0, 40 + 20 * q % 60); dm_gear_list_add(&x->item.own, 18, 0, 40 + 20 * q % 60); }
        }
        if (gs_capture(&g)) { CHECK(0, "trigger leech stock act %d: no capture", act); continue; }
        dm_gear_stat_classes(&g.p, g_cls);
        brute_plan *pl = malloc(sizeof *pl); if (!pl) abort();
        brute_make_plan(&g.p, pl);
        adm *a = calloc(1, sizeof *a); if (!a) abort();
        a->g = &g; a->pl = pl;
        int before = failures;
        brute_walk(&g.p, pl, adm_leaf, a);
        g_exact = 1; brute_walk(&g.p, pl, exact_leaf, a); g_exact = 0;
        char what[64]; snprintf(what, sizeof what, "trigger leech act %d", act);
        walk_stock(&g, what, 0, 0);
        CHECK(g_last_dual > 0, "%s: no rated leaf with two weapons", what);
        CHECK(failures == before, "%s: %d leaves above their bounds", what, failures - before);
        leaves += a->leaves; checks += a->checks; bounds += a->bounds; finite += a->finite; tighter += a->tighter; recovery += a->recovery; death += a->death; hits += a->hits; stocks++;
        free(a); free(pl);
    }
    tight_case();
    printf("gear bound: %ld monotone pairs; %ld stocks, %ld rated leaves against %ld prefix bounds (%ld checks, %ld with finite damage bounds; tight ones: "
           "%ld with a lower single target bound, %ld with a recovery bound, %ld with a time of death, %ld with fewer hits); %ld whole cast checks; "
           "search node walks of %ld stocks: %ld rated leaves (%ld with two weapons) under %ld node and %ld ring pair bounds, whole casts in %ld, %ld without an action "
           "below a main spell that costs mana, %ld out of nodes; in %.1f s; %s\n", mono, stocks, leaves, bounds, checks, finite, tighter, recovery, death, hits, casts, g_walks, g_walk.leaves, g_walk.dual,
           g_walk.bounds, g_walk.halves, g_walk.how[0], g_walk.how[1], g_walk.how[4], now_s() - t0, failures ? "FAILED" : "passed");
    return failures != 0;
}
