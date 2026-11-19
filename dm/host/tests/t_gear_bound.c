




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

static int prefix_bound(adm *a, const int *choice, int k, int tight, dm_gear_bound_out *o) {
    const dm_gear_problem *p = &a->g->p; const brute_plan *pl = a->pl;
    static dm_gear_bound_in in; memset(&in, 0, sizeof in);
    in.splash_on = p->splash_on;
    static dm_gear_list hi, lo;
    int sets[DM_GEAR_MAXCAND], nsets = 0;
    for (int depth = 0; depth < 8 + pl->nfree; depth++) {
        if (depth < 8) {
            const brute_choices *c = &pl->place[depth];
            dm_gear_list best = { 0 }, least = { 0 };
            int kick_min = 0, kick_max = 0;
            for (int i = 0; i < c->n; i++) {
                if (depth < k && i != choice[depth]) continue;
                hi.n = lo.n = hi.full = lo.full = 0;
                int m[2] = { c->a[i], c->b[i] };
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
            if (depth == 6) { in.kick_min = kick_min; in.kick_max = kick_max; }
        } else {
            int c = pl->freec[depth - 8];
            if (depth < k && !choice[depth]) continue;
            add_item(p, c, &in.hi, &in.lo, &in.varied);
        }
    }
    for (int i = 0; i < p->ncand; i++) if (pl->keep[i]) add_item(p, i, &in.hi, &in.lo, &in.varied);
    const dm_gear_tables *t = p->t; uint8_t seen[DM_GEAR_MAXSET] = { 0 };
    for (int i = 0; i < nsets; i++) {
        const dm_gear_item *it = &p->cand[sets[i]].item; int set = t->item[it->setitem].set;
        static dm_gear_list green, conv; green.n = conv.n = 0;
        for (int r = 0; r < t->nitemb; r++) if (t->itemb[r].owner == it->setitem) {
            add_bonus(t->itemb[r].stat, t->itemb[r].layer, t->itemb[r].value, t->itemb[r].unmapped, &in.hi, &in.lo, &in.varied);
            if (!t->itemb[r].unmapped && t->itemb[r].value > 0) dm_gear_list_add(&green, t->itemb[r].stat, t->itemb[r].layer, t->itemb[r].value);
        }

        dm_gear_item_conversions(t, p->base.clvl, &green, &conv);
        for (int r = 0; r < conv.n; r++) if (conv.v[r].value > 0) dm_gear_list_add(&in.hi, conv.v[r].stat, conv.v[r].layer, conv.v[r].value);
        if (seen[set]++) continue;
        for (int r = 0; r < t->nsetb; r++) if (t->setb[r].owner == set) add_bonus(t->setb[r].stat, t->setb[r].layer, t->setb[r].value, t->setb[r].unmapped, &in.hi, &in.lo, &in.varied);
    }
    a->bounds++;
    in.tight = tight;
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

int main(int argc, char **argv) {
    g_d = gs_tables(&g_gt);
    for (int i = 0; i < 9; i++) g_profile[i] = gs_profile(k_profiles[i]);
    int nstocks = argc > 1 ? atoi(argv[1]) : 40;
    static gstock g;
    long mono = 0, stocks = 0, leaves = 0, checks = 0, bounds = 0, finite = 0, tighter = 0, recovery = 0, death = 0, hits = 0;
    double t0 = now_s();
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
    printf("gear bound: %ld monotone pairs; %ld stocks, %ld rated leaves against %ld prefix bounds (%ld checks, %ld with finite damage bounds; tight ones: "
           "%ld with a lower single target bound, %ld with a recovery bound, %ld with a time of death, %ld with fewer hits) in %.1f s; %s\n", mono, stocks, leaves,
           bounds, checks, finite, tighter, recovery, death, hits, now_s() - t0, failures ? "FAILED" : "passed");
    return failures != 0;
}
