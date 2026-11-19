


#define _POSIX_C_SOURCE 200809L
#include "t_gear_stock.h"
#include "gear_brute.h"
#include "../dm_splash_rule.h"
#include <pthread.h>
#include <time.h>

static atomic_int failures;
#define CHECK(cond, ...) do { if (!(cond)) { atomic_fetch_add(&failures, 1); fprintf(stderr, "FAIL %s:%d: ", __FILE__, __LINE__); fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); } } while (0)
static double now_s(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + t.tv_nsec / 1e9; }

static const char *const k_profiles[] = { "class0-level80.prof", "class1-level80.prof", "class2-level80.prof", "class3-level80.prof", "class4-level80.prof",
                                          "class5-level80.prof", "class6-level80.prof", "class7-level12.prof", "class7-level80.prof" };
static char *g_profile[9];
static dm_gear_tables g_gt;
static const dm_sim_data *g_d;
static dm_gear_goal g_goals[9];

typedef struct { dm_gear_search *s; const dm_gear_goal *g; atomic_int stop; dm_gear_result r; int rc; } run_ctx;
static void *run_thread(void *x) { run_ctx *c = x; c->rc = dm_gear_search_run(c->s, c->g, &c->stop, NULL, &c->r); return NULL; }


typedef struct { dm_gear_progress *pr; int last, fell; } share_seen;
static void share_better(const dm_gear_result *r, void *ctx) { (void)r; share_seen *x = ctx; int v = atomic_load(&x->pr->share); if (v < x->last) x->fell = 1; x->last = v; }

static long g_seeded, g_fixed, g_stocks, g_runs, g_leaves, g_combos, g_pruned, g_unrated, g_dual, g_aborts;
static double g_t_brute, g_t_search;
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

static void check_stock(gstock *g, const char *name, int abort_test, int splash_on) {
    long stocks = 0, runs = 0, leaves = 0, combos = 0, pruned = 0, unrated = 0, dual = 0, aborts = 0, seeded = 0; double t_brute = 0, t_search = 0;
    g->p.splash_on = splash_on;
    brute_out *b = malloc(sizeof *b); if (!b) abort();
    double t0 = now_s();
    int rc = gear_brute(&g->p, g_goals, 9, b);
    t_brute += now_s() - t0;
    if (rc) { CHECK(0, "%s: the worn gear has no rating", name); free(b); return; }
    dm_gear_search *s = dm_gear_search_new(&g->p);
    CHECK(s, "%s: no search context", name);
    if (!s) { free(b); return; }
    stocks++; combos += b->combos;
    static const atomic_int wide_on = 1;
    dm_gear_search *sw = dm_gear_search_new(&g->p);
    dm_gear_search_wide(sw, DM_GEAR_MAXTHREADS, &wide_on, NULL);
    static _Thread_local dm_gear_pick prev;
    for (int k = 0; k < 9; k++) {
        dm_gear_progress pr; memset(&pr, 0, sizeof pr);
        share_seen seen = { &pr, -1, 0 }; pr.better = share_better; pr.ctx = &seen;
        dm_gear_result r;
        t0 = now_s();
        int st = dm_gear_search_run(s, &g_goals[k], NULL, &pr, &r);

        if (sw) {
            dm_gear_progress pw; memset(&pw, 0, sizeof pw);
            share_seen sseen = { &pw, -1, 0 }; pw.better = share_better; pw.ctx = &sseen;
            dm_gear_result rw; int sw_st = dm_gear_search_run(sw, &g_goals[k], NULL, &pw, &rw);
            CHECK(sw_st == st && !memcmp(&rw.best, &r.best, sizeof r.best) && !memcmp(&rw.after, &r.after, sizeof r.after) && !dm_gear_key_cmp(rw.score.key, r.score.key)
                  && rw.tripwire == r.tripwire && !sseen.fell && (sw_st != DM_GEAR_S_DONE || atomic_load(&pw.share) == 1000000),
                  "%s goal %d: the run on several threads differs from the one on one thread", name, k);
        }

        for (int w = 0; w < 1; w++) {
            dm_gear_search_seed(s, k ? &prev : &r.best);
            dm_gear_result rs; int ss = dm_gear_search_run(s, &g_goals[k], NULL, NULL, &rs);
            dm_gear_search_seed(s, NULL);
            CHECK(ss == st && !memcmp(&rs.best, &r.best, sizeof r.best) && !memcmp(&rs.after, &r.after, sizeof r.after) && !dm_gear_key_cmp(rs.score.key, r.score.key),
                  "%s goal %d: the run started from %s differs from the one without", name, k, k ? "the result before" : "its own result");
            seeded += rs.seeded;
        }
        prev = r.best;

        CHECK(!seen.fell && atomic_load(&pr.share) >= seen.last && atomic_load(&pr.share) <= 1000000
              && (st != DM_GEAR_S_DONE || atomic_load(&pr.share) == 1000000),
              "%s goal %d: the tree share fell or overran (%d after %d)", name, k, atomic_load(&pr.share), seen.last);
        t_search += now_s() - t0;
        runs += atomic_load(&pr.runs); leaves += atomic_load(&pr.leaves); pruned += atomic_load(&pr.pruned);
        if (!k) { unrated += r.unrated; dual += r.dual_pairs; }
        CHECK(st == DM_GEAR_S_DONE, "%s goal %d: status %d", name, k, st);
        CHECK(!dm_gear_key_cmp(r.score.key, b->key[k]), "%s goal %g/%g: search key (%g, %g, %g), enumerator (%g, %g, %g) over %ld combinations",
              name, g_goals[k].survival_hits, g_goals[k].boss_share, r.score.key[0], r.score.key[1], r.score.key[2], b->key[k][0], b->key[k][1],
              b->key[k][2], b->combos);
        CHECK(r.tripwire == 0, "%s goal %d: %ld leaves above a bound of their subtree", name, k, r.tripwire);

        dm_gear_measures m; unsigned flags;
        if (!dm_gear_leaf_measure(&g->p, &r.best, &m, &flags)) {
            dm_gear_score_out o; dm_gear_score(&r.now, &m, &g_goals[k], &o);
            CHECK(!dm_gear_key_cmp(o.key, r.score.key) && dm_gear_place(&g->p, r.best.charm), "%s goal %d: the named pick does not give its key", name, k);
        } else CHECK(0, "%s goal %d: the named pick has no rating (%#x)", name, k, flags);

        for (int i = 0; g->p.charms_fixed && i < g->p.ncand; i++) if (g->p.cand[i].kind == DM_GEAR_K_CHARM)
            CHECK(r.best.charm[i] == (g->p.cand[i].inv && !(g->p.cand[i].unrated & DM_GEAR_U_LEVEL)), "%s goal %d: charm %d moved with charms fixed", name, k, i);
        if (abort_test && k == 4) {

            dm_gear_search *s2 = dm_gear_search_new(&g->p);
            run_ctx c = { s2, &g_goals[k], 0, { 0 }, 0 };
            atomic_store(&c.stop, 1);
            run_thread(&c);
            aborts += c.rc == DM_GEAR_S_ABORTED;
            CHECK(c.rc == DM_GEAR_S_ABORTED || c.rc == DM_GEAR_S_DONE, "%s: stopped run status %d", name, c.rc);
            dm_gear_result again; dm_gear_search_run(s2, &g_goals[k], NULL, NULL, &again);
            CHECK(!dm_gear_key_cmp(again.score.key, r.score.key), "%s: run after a stop differs", name);

            dm_gear_search_free(s2); s2 = dm_gear_search_new(&g->p);
            run_ctx d = { s2, &g_goals[k], 0, { 0 }, 0 };
            pthread_t th; pthread_create(&th, NULL, run_thread, &d);
            struct timespec w = { 0, 2000000 }; nanosleep(&w, NULL);
            atomic_store(&d.stop, 1); pthread_join(th, NULL);
            aborts += d.rc == DM_GEAR_S_ABORTED;
            dm_gear_search_run(s2, &g_goals[k], NULL, NULL, &again);
            CHECK(!dm_gear_key_cmp(again.score.key, r.score.key), "%s: run after a stop from another thread differs", name);
            dm_gear_search_free(s2);
        }
    }
    dm_gear_search_free(s); dm_gear_search_free(sw);
    free(b);
    pthread_mutex_lock(&g_lock);
    g_stocks += stocks; g_runs += runs; g_leaves += leaves; g_combos += combos; g_pruned += pruned; g_unrated += unrated; g_dual += dual;
    g_aborts += aborts; g_seeded += seeded; g_t_brute += t_brute; g_t_search += t_search;
    pthread_mutex_unlock(&g_lock);
}

static void random_spec(gspec *s, uint64_t seed) {
    memset(s, 0, sizeof *s);
    gs_seed = seed * 7919 + 17;
    int pi = gs_int(0, 8);
    s->profile = g_profile[pi]; s->act = gs_int(1, 5); s->seed = seed; s->variants = 1;
    int amazon = pi == 0 && gs_int(0, 1);
    s->count[DM_GEAR_K_WEAPON1] = gs_int(0, 2); s->count[DM_GEAR_K_WEAPON2] = gs_int(0, 1); s->count[DM_GEAR_K_SHIELD] = gs_int(0, 2);
    if (amazon) { s->count[DM_GEAR_K_BOW] = gs_int(1, 2); s->count[DM_GEAR_K_ARROWS] = gs_int(0, 1); s->count[DM_GEAR_K_WEAPON1] = 0; }
    s->count[DM_GEAR_K_ARMOR] = gs_int(0, 2); s->count[DM_GEAR_K_AMULET] = gs_int(0, 2); s->count[DM_GEAR_K_RING] = gs_int(0, 3);
    s->count[DM_GEAR_K_HEAD] = gs_int(0, 1); s->count[DM_GEAR_K_BELT] = gs_int(0, 1); s->count[DM_GEAR_K_BOOTS] = gs_int(0, 1); s->count[DM_GEAR_K_GLOVES] = gs_int(0, 1);
    s->charms_inv = gs_int(0, 2); s->charms_stash = gs_int(0, 3); s->blocked = gs_int(0, 8);
    s->sets = gs_int(0, 3) == 0 ? gs_int(2, 3) : 0; s->unrated = gs_int(0, 4) == 0; s->skills = gs_int(0, 1); s->chain = gs_int(0, 2) == 0;
}
static double combos(const gstock *g) {
    brute_plan *pl = malloc(sizeof *pl); if (!pl) abort();
    brute_make_plan(&g->p, pl);
    double n = 1;
    for (int k = 0; k < 8; k++) n *= pl->place[k].n;
    n *= ldexp(1, pl->nfree);
    free(pl);
    return n;
}


static void counterexamples(gstock *g) {
    const dm_sim_data *d = g_d;
    int allsk = 127, fcr = gs_stat(d, "item_fastercastrate"), fire = gs_stat(d, "fireresist"), maxfire = gs_stat(d, "maxfireresist");
    gspec s;

    memset(&s, 0, sizeof s); s.profile = "class=1\nlevel=80\ndiff=2\nact=1\nlife=1200\nmana=140\nstr=60\ndex=60\nvit=80\nene=40\nar=500\ndef=800\nfcr=63\n"
                                         "weapon=1,2,100,0,0,1,0,0,1HS\nres=0,75,75,75,75,0\nskill=59,20,20\nbar=59\n";
    s.act = 1; s.seed = 11; s.variants = 1; s.count[DM_GEAR_K_AMULET] = 0; s.count[DM_GEAR_K_RING] = 1;
    if (!gs_build(g, d, &g_gt, &s)) {
        for (int k = 0; k < 3; k++) { dm_gear_cand *a = gs_item(g, DM_GEAR_K_AMULET, 1, 0); a->item.own.n = 0; dm_gear_list_add(&a->item.own, allsk, 0, 1 + 2 * k); dm_gear_list_add(&a->item.own, 9, 0, (10 + 30 * (2 - k)) * 256); }
        dm_gear_problem_ready(&g->p); check_stock(g, "mana trap", 1, 0);
    }

    memset(&s, 0, sizeof s); s.profile = g_profile[1]; s.act = 2; s.seed = 12; s.variants = 1; s.count[DM_GEAR_K_RING] = 1;
    if (!gs_build(g, d, &g_gt, &s)) {
        for (int k = 0; k < 3; k++) { dm_gear_cand *c = gs_item(g, DM_GEAR_K_CHARM, 1, 0); c->item.own.n = 0; c->h = 1; dm_gear_list_add(&c->item.own, fcr, 0, 6 + k); c->item.req_lvl = 1; }
        dm_gear_problem_ready(&g->p); check_stock(g, "charms at a breakpoint", 0, 0);
    }

    memset(&s, 0, sizeof s); s.profile = g_profile[3]; s.act = 3; s.seed = 13; s.variants = 1; s.sets = 3; s.count[DM_GEAR_K_RING] = 1; s.count[DM_GEAR_K_AMULET] = 1;
    if (!gs_build(g, d, &g_gt, &s)) check_stock(g, "set from two pieces", 0, 0);

    memset(&s, 0, sizeof s); s.profile = g_profile[4]; s.act = 1; s.seed = 14; s.variants = 1; s.count[DM_GEAR_K_RING] = 1;
    if (!gs_build(g, d, &g_gt, &s)) {
        int str = dm_gear_list_get(&g->p.base.base, 0, 0);
        dm_gear_cand *a = gs_item(g, DM_GEAR_K_ARMOR, 1, 0); a->item.req_str = str + 12; dm_gear_list_add(&a->item.own, 0, 0, 20); dm_gear_list_add(&a->item.own, 31, 0, 900);
        dm_gear_cand *r = gs_item(g, DM_GEAR_K_RING, 1, 0); r->item.own.n = 0; dm_gear_list_add(&r->item.own, 0, 0, 12);
        dm_gear_cand *w = gs_item(g, DM_GEAR_K_WEAPON1, 1.4, 0); w->item.req_str = str + 25;
        dm_gear_problem_ready(&g->p); check_stock(g, "requirement chain", 0, 0);
    }

    memset(&s, 0, sizeof s); s.profile = g_profile[5]; s.act = 4; s.seed = 15; s.variants = 1;
    if (!gs_build(g, d, &g_gt, &s)) {
        int str = dm_gear_list_get(&g->p.base.base, 0, 0), dex = dm_gear_list_get(&g->p.base.base, 2, 0);
        dm_gear_cand *a = gs_item(g, DM_GEAR_K_HEAD, 1.5, 0); a->item.req_dex = dex + 10; dm_gear_list_add(&a->item.own, 0, 0, 15);
        dm_gear_cand *b = gs_item(g, DM_GEAR_K_GLOVES, 1.5, 0); b->item.req_str = str + 10; dm_gear_list_add(&b->item.own, 2, 0, 15);
        dm_gear_problem_ready(&g->p); check_stock(g, "requirement cycle", 0, 0);
    }

    memset(&s, 0, sizeof s); s.profile = g_profile[4]; s.act = 5; s.seed = 16; s.variants = 1; s.count[DM_GEAR_K_WEAPON1] = 2; s.count[DM_GEAR_K_WEAPON2] = 2; s.count[DM_GEAR_K_SHIELD] = 2;
    if (!gs_build(g, d, &g_gt, &s)) check_stock(g, "two hands or weapon and shield", 1, 0);

    memset(&s, 0, sizeof s); s.profile = g_profile[6]; s.act = 2; s.seed = 17; s.variants = 1; s.count[DM_GEAR_K_RING] = 5;
    if (!gs_build(g, d, &g_gt, &s)) check_stock(g, "ring pair", 0, 0);

    memset(&s, 0, sizeof s); s.profile = g_profile[2]; s.act = 5; s.seed = 18; s.variants = 1;
    if (!gs_build(g, d, &g_gt, &s)) {
        dm_gear_cand *a = gs_item(g, DM_GEAR_K_AMULET, 1, 0); a->item.own.n = 0; dm_gear_list_add(&a->item.own, fire, 0, 200);
        dm_gear_cand *b = gs_item(g, DM_GEAR_K_AMULET, 1, 0); b->item.own.n = 0; dm_gear_list_add(&b->item.own, maxfire, 0, 5); dm_gear_list_add(&b->item.own, fire, 0, 20);
        dm_gear_problem_ready(&g->p); check_stock(g, "resistance cap", 0, 0);
    }

    memset(&s, 0, sizeof s); s.profile = g_profile[4]; s.act = 3; s.seed = 19; s.variants = 1; s.unrated = 3; s.count[DM_GEAR_K_WEAPON1] = 3;
    if (!gs_build(g, d, &g_gt, &s)) check_stock(g, "aura items and two weapons", 1, 0);


    int life = gs_stat(d, "maxhp");
    memset(&s, 0, sizeof s); s.profile = g_profile[4]; s.act = 2; s.seed = 20; s.variants = 1; s.count[DM_GEAR_K_RING] = 1;
    if (!gs_build(g, d, &g_gt, &s)) {
        memset(g->p.grid, 1, sizeof g->p.grid);
        for (int y = 0; y < DM_GEAR_GRID_H; y++) g->p.grid[y][0] = 0;
        g->p.grid[0][5] = 0;
        for (int k = 0; k < 3; k++) {
            dm_gear_cand *c = gs_new(g, DM_GEAR_K_CHARM); c->item.req_lvl = 1;
            dm_gear_list_add(&c->item.own, life, 0, (k < 2 ? 20 : 50) * 256); dm_gear_list_add(&c->item.own, 22, 0, k < 2 ? 5 : 12);
            if (k < 2) { c->inv = 1; c->x = k ? 5 : 0; c->y = k ? 0 : 1; } else c->h = 3;
        }
        dm_gear_problem_ready(&g->p); check_stock(g, "equal charms in fixed cells", 0, 0);
    }

    memset(&s, 0, sizeof s); s.profile = g_profile[4]; s.act = 2; s.seed = 21; s.variants = 1; s.count[DM_GEAR_K_WEAPON1] = 2; s.count[DM_GEAR_K_RING] = 2;
    s.count[DM_GEAR_K_AMULET] = 1;
    if (!gs_build(g, d, &g_gt, &s)) {
        dm_gear_cand *c = gs_new(g, DM_GEAR_K_CHARM); c->item.req_lvl = 1; c->item.req_str = 10; c->unrated = DM_GEAR_U_CHARMREQ;
        c->inv = 1; c->x = DM_GEAR_GRID_W - 1; c->y = 0; c->h = 3;
        dm_gear_list_add(&c->item.own, 22, 0, 60); dm_gear_list_add(&c->item.own, 19, 0, 600); dm_gear_list_add(&c->item.own, life, 0, 80 * 256);
        dm_gear_problem_ready(&g->p); check_stock(g, "kept charm with values", 0, 0);
    }


    memset(&s, 0, sizeof s); s.profile = g_profile[6]; s.act = 1; s.seed = 22; s.variants = 1; s.count[DM_GEAR_K_RING] = 1;
    if (!gs_build(g, d, &g_gt, &s)) {
        dm_gear_cand *b = gs_item(g, DM_GEAR_K_BOOTS, 1, 0); b->item.req_str = 5000; b->dmin = 400; b->dmax = 900;
        dm_gear_cand *w = gs_item(g, DM_GEAR_K_WEAPON1, 3, 0); w->item.req_str = 5000;
        int ib = (int)(b - g->cand), iw = (int)(w - g->cand);
        dm_gear_problem_ready(&g->p);
        dm_gear_pick pk; dm_gear_pick_now(&g->p, &pk);
        dm_sim_char ch; unsigned flags;
        pk.at[DM_GEAR_P_BOOTS] = (int16_t)ib;
        CHECK(dm_gear_leaf_char(&g->p, &pk, &ch, NULL, &flags) && (flags & DM_GEAR_L_REQ), "boots below their requirement have a rating (%#x)", flags);
        dm_gear_pick_now(&g->p, &pk); pk.at[DM_GEAR_P_MAIN] = (int16_t)iw; pk.at[DM_GEAR_P_OFF] = -1;
        CHECK(dm_gear_leaf_char(&g->p, &pk, &ch, NULL, &flags) && (flags & DM_GEAR_L_REQ), "a weapon below its requirement has a rating (%#x)", flags);
        dm_gear_search *x = dm_gear_search_new(&g->p);
        for (int k = 0; x && k < 9; k++) {
            dm_gear_result r; dm_gear_search_run(x, &g_goals[k], NULL, NULL, &r);
            CHECK(r.best.at[DM_GEAR_P_BOOTS] != ib && r.best.at[DM_GEAR_P_MAIN] != iw, "goal %d: the search names an item below its requirement", k);
        }
        dm_gear_search_free(x);
        check_stock(g, "requirements out of reach", 0, 0);
    }

    memset(&s, 0, sizeof s); s.profile = g_profile[1]; s.act = 2; s.seed = 24; s.variants = 1; s.count[DM_GEAR_K_RING] = 1;
    if (!gs_build(g, d, &g_gt, &s)) {
        dm_gear_cand *other = gs_item(g, DM_GEAR_K_HEAD, 1, 0); other->item.own.n = 0; other->item.req_str = 0; other->cls_only = 4 + 1;
        dm_gear_list_add(&other->item.own, 31, 0, 4000); dm_gear_list_add(&other->item.own, life, 0, 400 * 256);
        dm_gear_cand *own = gs_item(g, DM_GEAR_K_HEAD, 1, 0); own->item.own.n = 0; own->item.req_str = 0; own->cls_only = g->p.now.cls + 1;
        dm_gear_list_add(&own->item.own, 31, 0, 2000); dm_gear_list_add(&own->item.own, life, 0, 200 * 256);
        int io = (int)(other - g->cand), iw = (int)(own - g->cand);
        dm_gear_problem_ready(&g->p);
        dm_gear_pick pk; dm_gear_pick_now(&g->p, &pk); dm_sim_char ch; unsigned flags;
        pk.at[DM_GEAR_P_HEAD] = (int16_t)io;
        CHECK(dm_gear_leaf_char(&g->p, &pk, &ch, NULL, &flags) && (flags & DM_GEAR_L_REQ), "a helm of another class has a rating (%#x)", flags);
        pk.at[DM_GEAR_P_HEAD] = (int16_t)iw;
        CHECK(!dm_gear_leaf_char(&g->p, &pk, &ch, NULL, &flags), "a helm of the own class has no rating (%#x)", flags);
        dm_gear_search *x = dm_gear_search_new(&g->p);
        for (int k = 0; x && k < 9; k++) {
            dm_gear_result r; dm_gear_search_run(x, &g_goals[k], NULL, NULL, &r);
            CHECK(r.best.at[DM_GEAR_P_HEAD] != io, "goal %d: the search names a helm of another class", k);
            CHECK(k != 3 || r.best.at[DM_GEAR_P_HEAD] == iw, "goal %d: the search does not take the better helm of the own class", k);
        }
        dm_gear_search_free(x);
        check_stock(g, "item of another class", 0, 0);
    }


    memset(&s, 0, sizeof s); s.profile = g_profile[0]; s.act = 3; s.seed = 25; s.variants = 1; s.count[DM_GEAR_K_BOW] = 1; s.count[DM_GEAR_K_RING] = 1;
    if (!gs_build(g, d, &g_gt, &s)) {
        dm_gear_cand *bow = gs_item(g, DM_GEAR_K_BOW, 4, 0); bow->item.req_str = 0;
        int ib = (int)(bow - g->cand);
        dm_gear_problem_ready(&g->p);
        dm_gear_pick pk; dm_gear_pick_now(&g->p, &pk); dm_sim_char ch; unsigned flags;
        CHECK(pk.at[DM_GEAR_P_MAIN] >= 0 && g->cand[pk.at[DM_GEAR_P_MAIN]].kind == DM_GEAR_K_BOW && pk.at[DM_GEAR_P_OFF] < 0, "the stock does not wear a bow alone");
        CHECK(!dm_gear_leaf_char(&g->p, &pk, &ch, NULL, &flags), "the worn bow without arrows has no rating (%#x)", flags);
        pk.at[DM_GEAR_P_MAIN] = (int16_t)ib;
        CHECK(dm_gear_leaf_char(&g->p, &pk, &ch, NULL, &flags) && (flags & DM_GEAR_L_AMMO), "a bow without arrows has a rating (%#x)", flags);
        dm_gear_search *x = dm_gear_search_new(&g->p);
        for (int k = 0; x && k < 9; k++) {
            dm_gear_result r; dm_gear_search_run(x, &g_goals[k], NULL, NULL, &r);
            CHECK(r.best.at[DM_GEAR_P_MAIN] != ib, "goal %d: the search names a bow without arrows", k);
        }
        dm_gear_search_free(x);
        check_stock(g, "bow without arrows", 0, 0);
        dm_gear_cand *quiver = gs_item(g, DM_GEAR_K_ARROWS, 1, 0); int iq = (int)(quiver - g->cand);
        dm_gear_problem_ready(&g->p);
        pk.at[DM_GEAR_P_OFF] = (int16_t)iq;
        CHECK(!dm_gear_leaf_char(&g->p, &pk, &ch, NULL, &flags), "a bow with arrows has no rating (%#x)", flags);
        check_stock(g, "bow with arrows from the stash", 0, 0);
    }



    memset(&s, 0, sizeof s); s.profile = g_profile[1]; s.act = 1; s.seed = 26; s.variants = 1; s.count[DM_GEAR_K_RING] = 1;
    if (!gs_build(g, d, &g_gt, &s)) {
        int fresh = -1, known = -1;
        dm_gear_problem_ready(&g->p);
        for (int id = 1; id < DM_SIM_MAXSKILL && fresh < 0; id++)
            if (d->skill[id] && d->skill[id]->cls == g->p.now.cls && !g->p.node[id] && d->skill[id]->passive && g->p.moves[id]) fresh = id;
        for (int i = 0; i < g->p.now.nbar && known < 0; i++) if (g->p.now.bar[i] > 0 && g->p.node[g->p.now.bar[i]]) known = g->p.now.bar[i];
        CHECK(fresh > 0 && known > 0, "no own class skill without a node (%d) or with one (%d)", fresh, known);
        dm_gear_cand *staff = gs_item(g, DM_GEAR_K_WEAPON1, 4, 0); staff->item.req_str = 0; dm_gear_list_add(&staff->item.own, 107, fresh, 3);
        dm_gear_list_add(&staff->item.own, life, 0, 300 * 256); dm_gear_list_add(&staff->item.own, 127, 0, 2);
        dm_gear_cand *wand = gs_item(g, DM_GEAR_K_WEAPON1, 1, 0); wand->item.req_str = 0; dm_gear_list_add(&wand->item.own, 107, known, 2);
        int is = (int)(staff - g->cand), iw = (int)(wand - g->cand);
        dm_gear_problem_ready(&g->p);
        dm_gear_pick pk; dm_gear_pick_now(&g->p, &pk); dm_sim_char ch; unsigned flags;
        pk.at[DM_GEAR_P_MAIN] = (int16_t)is; pk.at[DM_GEAR_P_OFF] = -1;
        CHECK(dm_gear_leaf_char(&g->p, &pk, &ch, NULL, &flags) && (flags & DM_GEAR_L_SKILL), "a weapon giving a new skill has a rating (%#x)", flags);
        pk.at[DM_GEAR_P_MAIN] = (int16_t)iw;
        CHECK(!dm_gear_leaf_char(&g->p, &pk, &ch, NULL, &flags), "a weapon raising a known skill has no rating (%#x)", flags);
        dm_gear_search *x = dm_gear_search_new(&g->p);
        for (int k = 0; x && k < 9; k++) {
            dm_gear_result r; dm_gear_search_run(x, &g_goals[k], NULL, NULL, &r);
            CHECK(r.best.at[DM_GEAR_P_MAIN] != is && r.unrated >= 1, "goal %d: the search names the weapon giving a new skill or does not count it (%d unrated)", k, r.unrated);
        }
        dm_gear_search_free(x);
        check_stock(g, "weapon giving a new skill", 0, 0);
    }



    memset(&s, 0, sizeof s); s.profile = g_profile[1]; s.act = 1; s.seed = 28; s.variants = 1; s.count[DM_GEAR_K_RING] = 1;
    if (!gs_build(g, d, &g_gt, &s)) {
        int idle = -1;
        dm_gear_problem_ready(&g->p);
        for (int id = 1; id < DM_SIM_MAXSKILL && idle < 0; id++) {
            const dm_sim_skill *k = d->skill[id];
            if (k && k->cls == g->p.now.cls && !g->p.node[id] && !g->p.moves[id] && (k->fam & DM_FAM_SPELL)) idle = id;
        }
        CHECK(idle > 0, "no own class spell without a node that moves no measure");
        if (idle > 0) {
            dm_gear_cand *staff = gs_item(g, DM_GEAR_K_WEAPON1, 4, 0); staff->item.req_str = 0; dm_gear_list_add(&staff->item.own, 107, idle, 3);
            dm_gear_list_add(&staff->item.own, life, 0, 300 * 256); dm_gear_list_add(&staff->item.own, 127, 0, 2);
            int is = (int)(staff - g->cand);
            dm_gear_problem_ready(&g->p);
            dm_gear_pick pk; dm_gear_pick_now(&g->p, &pk); dm_sim_char ch; unsigned flags;
            pk.at[DM_GEAR_P_MAIN] = (int16_t)is; pk.at[DM_GEAR_P_OFF] = -1;
            CHECK(!dm_gear_leaf_char(&g->p, &pk, &ch, NULL, &flags) && ch.lvl[idle] == 0, "a weapon giving an idle new skill has no rating (%#x)", flags);
            dm_gear_search *x = dm_gear_search_new(&g->p);
            for (int k = 0; x && k < 9; k++) {
                dm_gear_result r; dm_gear_search_run(x, &g_goals[k], NULL, NULL, &r);
                CHECK(r.best.at[DM_GEAR_P_MAIN] == is && r.unrated == 0, "goal %d: the search does not name the weapon giving an idle new skill (%d, %d unrated)",
                      k, r.best.at[DM_GEAR_P_MAIN], r.unrated);
            }
            dm_gear_search_free(x);
            check_stock(g, "weapon giving an idle new skill", 0, 0);
        }
    }


    memset(&s, 0, sizeof s);
    s.profile = "class=1\nlevel=80\ndiff=2\nlife=1200\nmana=900\nstr=60\ndex=60\nvit=80\nene=200\nar=500\ndef=800\nfcr=63\n"
                "weapon=1,2,100,0,0,1,0,0,1HS\nres=0,75,75,75,75,0\nskill=59,20,20\nbar=59\nskill=47,4,0\nbar=47\n";
    s.act = 1; s.seed = 27; s.variants = 1; s.count[DM_GEAR_K_WEAPON1] = 1;
    if (!gs_build(g, d, &g_gt, &s)) {
        dm_gear_problem_ready(&g->p);
        dm_gear_pick pk; dm_gear_pick_now(&g->p, &pk); dm_sim_char ch; unsigned flags;
        int wm = pk.at[DM_GEAR_P_MAIN], am = pk.at[DM_GEAR_P_ARMOR], other = -1;
        for (int i = 0; i < g->p.ncand; i++) if (!g->cand[i].worn && g->cand[i].kind == DM_GEAR_K_WEAPON1) other = i;
        CHECK(wm >= 0 && am >= 0 && other >= 0, "the stock lacks the worn weapon (%d), the worn armor (%d) or another weapon (%d)", wm, am, other);
        if (wm >= 0 && am >= 0 && other >= 0) {
            g->cand[other].item.req_str = 0;
            dm_gear_list_add(&g->cand[wm].item.own, 107, 47, 3); dm_gear_list_add(&g->cand[am].item.own, 127, 0, 1);
            dm_gear_problem_ready(&g->p);
            pk.at[DM_GEAR_P_MAIN] = (int16_t)other; pk.at[DM_GEAR_P_OFF] = -1;
            int rc = dm_gear_leaf_char(&g->p, &pk, &ch, NULL, &flags);
            CHECK(!rc && ch.lvl[47] == 0, "a skill only the weapon gave keeps level %d without it (%#x)", ch.lvl[47], flags);
            CHECK(!rc && ch.lvl[59] == g->p.now.lvl[59], "a learned skill moves without a change of its bonuses (%d -> %d)", g->p.now.lvl[59], ch.lvl[59]);
        }
        check_stock(g, "skill gone with its weapon", 0, 0);
    }



    for (int barb = 0; barb < 2; barb++) {
        char prof[2048]; snprintf(prof, sizeof prof, "%sgrant=obuff,149,10,0,10\n", g_profile[barb ? 4 : 1]);
        memset(&s, 0, sizeof s); s.profile = prof; s.act = 5; s.seed = 28; s.variants = 1; s.count[DM_GEAR_K_RING] = 1;
        if (gs_build(g, d, &g_gt, &s)) { CHECK(0, "set II buff stock %d not built", barb); continue; }
        dm_gear_problem_ready(&g->p);
        dm_gear_pick pk; dm_gear_pick_now(&g->p, &pk); dm_sim_char ch, base; unsigned flags;
        int wh = pk.at[DM_GEAR_P_HEAD], ww = pk.at[DM_GEAR_P_MAIN], gi = -1;
        for (int i = 0; i < g->p.now.ngrant; i++) if (g->p.now.grant[i].skill == 149) gi = i;
        CHECK(wh >= 0 && ww >= 0 && gi >= 0, "set II buff stock %d lacks the worn helm (%d), weapon (%d) or the grant (%d)", barb, wh, ww, gi);
        if (wh < 0 || ww < 0 || gi < 0) continue;
        dm_gear_cand *helm = gs_new(g, DM_GEAR_K_HEAD); uint32_t id = helm->item.id; *helm = g->cand[wh]; helm->worn = 0; helm->item.id = id;
        dm_gear_list_add(&helm->item.own, 127, 0, 2);
        dm_gear_cand *weap = gs_new(g, DM_GEAR_K_WEAPON1); id = weap->item.id; *weap = g->cand[ww]; weap->worn = 0; weap->item.id = id;
        dm_gear_list_add(&weap->item.own, 127, 0, 2);
        int ih = (int)(helm - g->cand), iw = (int)(weap - g->cand);
        dm_gear_problem_ready(&g->p);
        CHECK(!dm_gear_leaf_char(&g->p, &pk, &base, NULL, &flags) && base.grant[gi].lvl == 10, "stock %d: the worn gear moves the buff to %d (%#x)", barb,
              base.grant[gi].lvl, flags);
        dm_gear_pick ph = pk; ph.at[DM_GEAR_P_HEAD] = (int16_t)ih;
        int rc = dm_gear_leaf_char(&g->p, &ph, &ch, NULL, &flags);
        CHECK(!rc && ch.grant[gi].lvl == (barb ? 10 : 12), "stock %d: a helm with +2 to all skills gives the set II buff level %d (%#x)", barb, ch.grant[gi].lvl, flags);
        if (!rc && !barb) {
            dm_sim_char frozen = ch; frozen.grant[gi].lvl = 10;
            dm_gear_measures m1, m2;
            CHECK(!dm_gear_measure(d, &ch, 0, &m1) && !dm_gear_measure(d, &frozen, 0, &m2) && memcmp(&m1, &m2, sizeof m1),
                  "the higher set II buff level changes no measure");
        }
        dm_gear_pick pw = pk; pw.at[DM_GEAR_P_MAIN] = (int16_t)iw;
        rc = dm_gear_leaf_char(&g->p, &pw, &ch, NULL, &flags);
        CHECK(!rc && ch.grant[gi].lvl == 10, "stock %d: a weapon with +2 to all skills moves the set II buff to %d (%#x)", barb, ch.grant[gi].lvl, flags);
        check_stock(g, barb ? "set II buff with an own node" : "set II buff", 0, 1);
    }


    memset(&s, 0, sizeof s); s.profile = g_profile[4]; s.act = 2; s.seed = 23; s.variants = 1; s.count[DM_GEAR_K_RING] = 2; s.count[DM_GEAR_K_AMULET] = 1;
    if (!gs_build(g, d, &g_gt, &s)) {
        dm_gear_cand *a = gs_new(g, DM_GEAR_K_CHARM); a->item.req_lvl = 1; a->inv = 1; a->x = 0; a->y = 0;
        dm_gear_list_add(&a->item.own, life, 0, 10 * 256); dm_gear_list_add(&a->item.own, 22, 0, 2);
        dm_gear_cand *b = gs_new(g, DM_GEAR_K_CHARM); b->item.req_lvl = 1;
        dm_gear_list_add(&b->item.own, life, 0, 40 * 256); dm_gear_list_add(&b->item.own, 22, 0, 9);
        dm_gear_cand *lv = gs_new(g, DM_GEAR_K_CHARM); lv->item.req_lvl = 99; lv->unrated = DM_GEAR_U_LEVEL; lv->inv = 1; lv->x = 1; lv->y = 0;
        dm_gear_list_add(&lv->item.own, 22, 0, 30);
        g->p.charms_fixed = 1;
        dm_gear_problem_ready(&g->p); check_stock(g, "charms fixed", 1, 0);
        g->p.charms_fixed = 0;
    }
}




static void mana_stocks(gstock *g) {
    static const char *const lean = "class=7\nlevel=20\ndiff=0\nlife=300\nmana=90\nstr=30\ndex=30\nvit=40\nene=60\nar=900\ndef=400\n"
                                    "weapon=20,40,100,0,0,1,0,0,1HS\nres=0,30,30,30,30,0\n"
                                    "skill=394,12,12\nbar=394\nskill=398,3,3\nbar=398\nskill=395,4,4\nbar=395\n";
    for (int mana = 1; mana <= 2; mana++) {
        gspec s; memset(&s, 0, sizeof s);
        s.profile = lean; s.act = 1; s.seed = 2100 + (uint64_t)mana; s.variants = 1; s.mana = mana; s.skills = 1;
        s.count[DM_GEAR_K_WEAPON1] = 1; s.count[DM_GEAR_K_ARMOR] = 1; s.count[DM_GEAR_K_AMULET] = 1;
        s.count[DM_GEAR_K_RING] = 2; s.count[DM_GEAR_K_HEAD] = 1; s.count[DM_GEAR_K_GLOVES] = 1;
        s.charms_inv = 1; s.charms_stash = 0; s.blocked = 1;
        if (gs_build(g, g_d, &g_gt, &s)) { CHECK(0, "lean caster stock mana %d not built", mana); continue; }
        if (combos(g) > 1e5) { CHECK(0, "lean caster mana %d too large for the enumerator", mana); continue; }
        char name[64]; snprintf(name, sizeof name, "lean caster mana %d", mana);
        g->p.charms_fixed = 1;
        check_stock(g, name, mana == 1, 0);
        g->p.charms_fixed = 0;
    }
    static const int cls[] = { 1, 3, 7 };
    for (int i = 0; i < 3; i++) {
        gspec s; memset(&s, 0, sizeof s);
        s.profile = g_profile[cls[i]]; s.act = 1; s.seed = 2200 + (uint64_t)cls[i]; s.variants = 1; s.skills = 1; s.sets = 2; s.chain = 1; s.mana = 2;
        s.count[DM_GEAR_K_WEAPON1] = 2; s.count[DM_GEAR_K_SHIELD] = 1; s.count[DM_GEAR_K_ARMOR] = 1; s.count[DM_GEAR_K_AMULET] = 1;
        s.count[DM_GEAR_K_RING] = 1; s.count[DM_GEAR_K_HEAD] = 1;
        s.charms_inv = 1; s.charms_stash = 0; s.blocked = 1;
        if (gs_build(g, g_d, &g_gt, &s)) { CHECK(0, "caster stock class%d not built", cls[i]); continue; }
        if (combos(g) > 1e5) { CHECK(0, "caster class%d too large for the enumerator", cls[i]); continue; }
        char name[64]; snprintf(name, sizeof name, "caster class%d", cls[i]);
        g->p.charms_fixed = 1;
        check_stock(g, name, 0, 0);
        g->p.charms_fixed = 0;
    }
}






static void drop_own_stat(gstock *g, const char *name) {
    for (int i = 0; i < g->p.ncand; i++) {
        dm_gear_list *l = &g->cand[i].item.own; int n = 0;
        for (int v = 0; v < l->n; v++) if (strcmp(g_d->stat_name[l->v[v].stat], name)) l->v[n++] = l->v[v];
        l->n = n;
    }
}
static void adversarial_cases(gstock *g) {
    static const uint64_t regen_seeds[] = { 1, 2 }, aura_seeds[] = { 10, 21 }, grant_seeds[] = { 5, 7 };
    static char hf[8192];
    for (int mi = 0; mi < 4; mi++) {
        int mode = mi == 0 ? 1 : mi == 1 ? 6 : mi == 2 ? 7 : 8;
        const uint64_t *seeds = mi == 0 ? regen_seeds : mi == 3 ? grant_seeds : aura_seeds;
        int nseeds = (int)(mi == 0 ? sizeof regen_seeds / sizeof *regen_seeds
                          : mi == 3 ? sizeof grant_seeds / sizeof *grant_seeds : sizeof aura_seeds / sizeof *aura_seeds);
        for (int i = 0; i < nseeds; i++) {
            uint64_t seed = seeds[i];
            gspec s; memset(&s, 0, sizeof s);
            gs_seed = seed * 7919 + 17;
            int pi = gs_int(0, 8);
            s.profile = g_profile[pi]; s.act = gs_int(1, 5); s.seed = seed; s.variants = 1;
            if (mode == 6) { snprintf(hf, sizeof hf, "%sskill=114,20,20\nbar=114\n", g_profile[3]); s.profile = hf; pi = 3; }
            if (mode == 7) { snprintf(hf, sizeof hf, "%sskill=114,20,20\nbar=114\nskill=113,20,20\nbar=113\n", g_profile[3]); s.profile = hf; pi = 3; }


            if (mode == 8) { snprintf(hf, sizeof hf, "%sskill=113,20,20\nbar=113\naura=122\ngrant=aura,122,15,0,0\n", g_profile[3]); s.profile = hf; pi = 3; }
            int amazon = pi == 0 && gs_int(0, 1);
            s.count[DM_GEAR_K_WEAPON1] = gs_int(0, 2); s.count[DM_GEAR_K_WEAPON2] = gs_int(0, 1); s.count[DM_GEAR_K_SHIELD] = gs_int(0, 2);
            if (amazon) { s.count[DM_GEAR_K_BOW] = gs_int(1, 2); s.count[DM_GEAR_K_ARROWS] = gs_int(0, 1); s.count[DM_GEAR_K_WEAPON1] = 0; }
            s.count[DM_GEAR_K_ARMOR] = gs_int(0, 2); s.count[DM_GEAR_K_AMULET] = gs_int(0, 2); s.count[DM_GEAR_K_RING] = gs_int(0, 3);
            s.count[DM_GEAR_K_HEAD] = gs_int(0, 1); s.count[DM_GEAR_K_BELT] = gs_int(0, 1); s.count[DM_GEAR_K_BOOTS] = gs_int(0, 1); s.count[DM_GEAR_K_GLOVES] = gs_int(0, 1);
            s.charms_inv = gs_int(0, 1); s.charms_stash = gs_int(0, 2); s.blocked = gs_int(0, 8);
            s.sets = gs_int(0, 3) == 0 ? gs_int(2, 3) : 0; s.skills = gs_int(0, 1); s.chain = gs_int(0, 2) == 0;
            if (gs_build(g, g_d, &g_gt, &s)) continue;
            if (mode == 1) {
                drop_own_stat(g, "hpregen"); drop_own_stat(g, "lifedrainmindam");
                int st = gs_stat(g_d, "hpregen");
                for (int q = 0; q < g->p.ncand; q++) {
                    int k = g->cand[q].kind;
                    if ((k == DM_GEAR_K_ARMOR || k == DM_GEAR_K_RING || k == DM_GEAR_K_AMULET || k == DM_GEAR_K_HEAD || k == DM_GEAR_K_BELT) && gs_int(0, 1))
                        dm_gear_list_add(&g->cand[q].item.own, st, 0, -gs_int(2, 12) * 256);
                }
            }
            if (gs_capture(g) || combos(g) > 4e4) continue;
            char name[64]; snprintf(name, sizeof name, "adversarial mode %d seed %llu", mode, (unsigned long long)seed);
            check_stock(g, name, 0, 0);
        }
    }
}



static void defense_stays(void) {
    static gstock g;
    static const double goals[2] = { 15, 1e9 };
    int stocks = 0, off = 0;
    for (int pi = 0; pi < 9; pi++) for (int act = 1; act <= 5; act++) for (int step = 0; step < 4; step++) {
        gspec s; memset(&s, 0, sizeof s);
        s.profile = g_profile[pi]; s.act = act; s.seed = (uint64_t)(900 + 20 * pi + 4 * act + step); s.variants = 1;
        if (gs_build(&g, g_d, &g_gt, &s)) { CHECK(0, "defense stock %d/%d/%d not built", pi, act, step); continue; }
        for (int i = 0; i < g.p.ncand; i++) {
            dm_gear_cand *c = &g.cand[i];
            if (!c->worn || c->weapon >= 0 || c->kind == DM_GEAR_K_ARROWS || c->kind == DM_GEAR_K_BOLTS) continue;
            c->item.own.n = 0; c->item.req_str = c->item.req_dex = 0; c->item.req_lvl = 1; c->dmin = c->dmax = 0;
            dm_gear_list_add(&c->item.own, 31, 0, 20 + 150 * step + 37 * i);
        }
        if (gs_capture(&g)) { CHECK(0, "defense stock %d/%d/%d: no capture", pi, act, step); continue; }
        dm_gear_search *x = dm_gear_search_new(&g.p);
        if (!x) { CHECK(0, "defense stock %d/%d/%d: no search context", pi, act, step); continue; }
        dm_gear_pick now; dm_gear_pick_now(&g.p, &now);
        for (int k = 0; k < 2; k++) {
            dm_gear_goal goal = { goals[k], 0.5 }; dm_gear_result r;
            int st = dm_gear_search_run(x, &goal, NULL, NULL, &r);
            if (st == DM_GEAR_S_NOREF) continue;
            stocks++;
            CHECK(st == DM_GEAR_S_DONE, "defense stock %d/%d/%d goal %g: status %d", pi, act, step, goals[k], st);
            for (int p = 0; p < DM_GEAR_P_N; p++) if (p != DM_GEAR_P_MAIN && now.at[p] >= 0 && r.best.at[p] < 0) {
                off++;
                CHECK(0, "defense stock %d/%d/%d goal %g: place %d taken off (hits %.4f -> %.4f, survival %.2f -> %.2f s, group %.4f -> %.4f, "
                      "single %.4f -> %.4f)", pi, act, step, goals[k], p, r.now.hits, r.after.hits, r.now.survival_s, r.after.survival_s,
                      r.now.group, r.after.group, r.now.single, r.after.single);
            }
        }
        dm_gear_search_free(x);
    }
    printf("defense stays: %d searches of worn gear with defense only, %d places taken off\n", stocks, off);
}



static void worn_stays_on_ties(void) {
    static gstock g;
    int off = 0, n = 0, mf = gs_stat(g_d, "item_magicbonus");
    for (int pi = 0; pi < 9; pi++) for (int act = 1; act <= 5; act++) {
        gspec s; memset(&s, 0, sizeof s); s.profile = g_profile[pi]; s.act = act; s.seed = (uint64_t)(77 + 5 * pi + act); s.variants = 1;
        if (gs_build(&g, g_d, &g_gt, &s)) { CHECK(0, "tie stock %d/%d not built", pi, act); continue; }
        int head = -1, str = dm_gear_list_get(&g.p.base.base, 0, 0);
        for (int i = 0; i < g.p.ncand; i++) if (g.cand[i].worn && g.cand[i].kind == DM_GEAR_K_HEAD) head = i;
        if (head < 0) continue;
        g.cand[head].item.own.n = 0; dm_gear_list_add(&g.cand[head].item.own, mf, 0, 30);
        for (int i = 0; i < g.p.ncand; i++)
            if (g.cand[i].worn && g.cand[i].kind == DM_GEAR_K_ARMOR && dm_gear_list_get(&g.cand[i].item.own, 0, 0)) g.cand[i].item.own.n = 0;
        dm_gear_cand *a = gs_item(&g, DM_GEAR_K_ARMOR, 1, 0); a->item.own.n = 0; dm_gear_list_add(&a->item.own, 0, 0, 30); dm_gear_list_add(&a->item.own, 31, 0, 50);
        dm_gear_cand *w = gs_item(&g, DM_GEAR_K_WEAPON1, 4, 0); w->item.req_str = str + 25;
        dm_gear_list_add(&w->item.own, 127, 0, 3); dm_gear_list_add(&w->item.own, 7, 0, 300 * 256);
        if (gs_capture(&g)) { CHECK(0, "tie stock %d/%d: no capture", pi, act); continue; }
        static const atomic_int wide_on = 1;
        for (int threads = 1; threads <= DM_GEAR_MAXTHREADS; threads += DM_GEAR_MAXTHREADS - 1) {
            dm_gear_search *x = dm_gear_search_new(&g.p);
            if (!x) { CHECK(0, "tie stock %d/%d: no search context", pi, act); continue; }
            if (threads > 1) dm_gear_search_wide(x, threads, &wide_on, NULL);
            dm_gear_goal goal = { 15, 0.5 }; dm_gear_result r;
            int st = dm_gear_search_run(x, &goal, NULL, NULL, &r);
            if (st == DM_GEAR_S_DONE) {
                n++;
                if (r.best.at[DM_GEAR_P_HEAD] != head) { off++; CHECK(0, "tie stock %d/%d, %d threads: the head that changes nothing is not kept", pi, act, threads); }

                dm_gear_search_seed(x, &r.best); dm_gear_result r2; dm_gear_search_run(x, &goal, NULL, NULL, &r2); dm_gear_search_seed(x, NULL);
                CHECK(r2.best.at[DM_GEAR_P_HEAD] == head, "tie stock %d/%d: the warm run takes the head off", pi, act);
            }
            dm_gear_search_free(x);
        }
    }
    printf("worn stays on ties: %d searches with a head that changes nothing, %d took it off\n", n, off);
}











typedef struct { dm_gear_progress *pr; long half, tail, total; atomic_int quit, returned, stop, stop_at_first_leaf;
                 int last, fell, full, early999, early900; } share_watch;
static void *share_watcher(void *x) {
    share_watch *w = x;
    int last = -1;
    while (!atomic_load(&w->quit)) {
        int sh = atomic_load(&w->pr->share);
        long lv = atomic_load(&w->pr->leaves);
        if (sh < last) w->fell = 1;
        last = sh;
        if (!atomic_load(&w->returned)) {


            if (sh >= 1000000 && lv < w->total) w->full = 1;
            if (lv < w->tail && sh >= 999000) w->early999 = 1;
            if (lv < w->half && sh >= 900000) w->early900 = 1;
            if (atomic_load(&w->stop_at_first_leaf) && lv > 0) atomic_store(&w->stop, 1);
        }
        struct timespec t = { 0, 250000 }; nanosleep(&t, NULL);
    }
    return NULL;
}
static atomic_int g_handout_pace;
static void handout_pace(int wide) { (void)wide; atomic_fetch_add(&g_handout_pace, 1); }
static void handout_watch_start(share_watch *w, dm_gear_progress *pr, long half, long tail, long total) {
    memset(w, 0, sizeof *w); w->pr = pr; w->half = half; w->tail = tail; w->total = total;
}
static void handout_watch_stop(share_watch *w) {
    atomic_store(&w->returned, 1); atomic_store(&w->quit, 1);
}
static void handout_share(void) {
    static gstock g;
    gspec s; memset(&s, 0, sizeof s);
    s.profile = g_profile[1]; s.act = 3; s.seed = 13107; s.variants = 3;
    s.count[DM_GEAR_K_ARMOR] = 4;
    if (gs_build(&g, g_d, &g_gt, &s)) { CHECK(0, "handout stock not built"); return; }
    static const atomic_int wide_on = 1;
    dm_gear_goal goal = { 15, 0.5 };
    double key0[3] = { 0, 0, 0 }; dm_gear_pick pick0; memset(&pick0, 0, sizeof pick0);
    long leaves0 = 0;
    {
        dm_gear_search *x = dm_gear_search_new(&g.p);
        if (!x) { CHECK(0, "handout stock: no search context"); return; }
        dm_gear_progress pr; memset(&pr, 0, sizeof pr);
        dm_gear_result r; int st = dm_gear_search_run(x, &goal, NULL, &pr, &r);
        CHECK(st == DM_GEAR_S_DONE, "handout stock: the bound run ended with status %d", st);
        leaves0 = atomic_load(&pr.leaves);
        CHECK(leaves0 > 1000, "handout stock: only %ld leaves - the stock lost its shape", leaves0);
        memcpy(key0, r.score.key, sizeof key0); pick0 = r.best;
        dm_gear_search_free(x);
    }
    long half = leaves0 / 2, tail = leaves0 - 40;
    int pace = 0, pruned = 0;
    for (int threads = 1; threads <= DM_GEAR_MAXTHREADS; threads += DM_GEAR_MAXTHREADS - 1) {
        dm_gear_search *x = dm_gear_search_new(&g.p);
        if (!x) { CHECK(0, "handout stock: no search context"); return; }
        atomic_store(&g_handout_pace, 0);
        if (threads > 1) dm_gear_search_wide(x, threads, &wide_on, handout_pace);
        dm_gear_progress pr; memset(&pr, 0, sizeof pr);
        share_watch w; handout_watch_start(&w, &pr, half, tail, leaves0);
        pthread_t th; pthread_create(&th, NULL, share_watcher, &w);
        dm_gear_result r; int st = dm_gear_search_run(x, &goal, NULL, &pr, &r);
        handout_watch_stop(&w); pthread_join(th, NULL);
        long leaves = atomic_load(&pr.leaves); int share = atomic_load(&pr.share);
        if (threads > 1) { pace = atomic_load(&g_handout_pace); pruned = (int)atomic_load(&pr.pruned); }
        CHECK(st == DM_GEAR_S_DONE && !w.fell && !w.full && !w.early999 && !w.early900 && share == 1000000 && leaves == leaves0
              && (threads == 1 || (pace >= 3 && pruned > 0)),
              "handout share, %d threads: status %d, share %d, fell %d, full in run %d, 999000 early %d, 900000 early %d, leaves %ld (of %ld), pace %d, pruned %d",
              threads, st, share, w.fell, w.full, w.early999, w.early900, leaves, leaves0, pace, pruned);
        CHECK(!dm_gear_key_cmp(r.score.key, key0) && !memcmp(&r.best, &pick0, sizeof r.best),
              "handout share, %d threads: the run differs from the one-thread run", threads);
        if (threads > 1) {

            dm_gear_search *y = dm_gear_search_new(&g.p);
            if (!y) { CHECK(0, "handout stock: no search context"); return; }
            dm_gear_search_wide(y, threads, &wide_on, NULL);
            dm_gear_progress pa; memset(&pa, 0, sizeof pa);
            share_watch wa; handout_watch_start(&wa, &pa, half, tail, leaves0);
            atomic_store(&wa.stop_at_first_leaf, 1);
            pthread_t th2; pthread_create(&th2, NULL, share_watcher, &wa);
            dm_gear_result ra; int sta = dm_gear_search_run(y, &goal, &wa.stop, &pa, &ra);
            handout_watch_stop(&wa); pthread_join(th2, NULL);
            int sha = atomic_load(&pa.share);

            CHECK((sta == DM_GEAR_S_ABORTED && sha < 1000000) || (sta == DM_GEAR_S_DONE && sha == 1000000),
                  "handout share, stopped run: status %d, share %d", sta, sha);
            dm_gear_result rb; int stb = dm_gear_search_run(y, &goal, NULL, NULL, &rb);
            CHECK(stb == DM_GEAR_S_DONE && !dm_gear_key_cmp(rb.score.key, key0) && !memcmp(&rb.best, &pick0, sizeof rb.best),
                  "handout share: the run started after the stop differs (status %d)", stb);
            dm_gear_search_free(y);
        }
        dm_gear_search_free(x);
    }
    printf("hand-out share: four top children, %ld leaves, %d pace calls with helpers on %d threads, closed at a full share on one and on %d threads\n",
           leaves0, pace, DM_GEAR_MAXTHREADS, DM_GEAR_MAXTHREADS);
}



static void splash_switch(void) {
    static gstock g;
    gspec s; memset(&s, 0, sizeof s);
    s.profile = g_profile[2]; s.act = 2; s.seed = 6969; s.variants = 1;
    for (int k = DM_GEAR_K_HEAD; k < DM_GEAR_K_N; k++) if (k != DM_GEAR_K_CHARM && k != DM_GEAR_K_BOW && k != DM_GEAR_K_XBOW && k != DM_GEAR_K_ARROWS && k != DM_GEAR_K_BOLTS) s.count[k] = 1;
    if (gs_build(&g, g_d, &g_gt, &s)) { CHECK(0, "splash switch stock not built"); return; }
    if (gs_capture(&g)) { CHECK(0, "splash switch stock: no capture"); return; }
    dm_gear_goal goal = { 0, 0.5 };
    g.p.splash_on = 0; uint64_t off = dm_gear_problem_hash(&g.p, &goal);
    g.p.splash_on = 1; uint64_t on = dm_gear_problem_hash(&g.p, &goal);
    CHECK(off != on, "the fingerprint does not see the splash switch");
    check_stock(&g, "splash on", 0, 1);
    dm_gear_result first;
    for (int round = 0; round < 3; round++) {
        g.p.splash_on = round == 1;
        dm_gear_search *x = dm_gear_search_new(&g.p); if (!x) { CHECK(0, "splash switch round: no search context"); return; }
        dm_gear_result r; int st = dm_gear_search_run(x, &goal, NULL, NULL, &r);
        CHECK(st == DM_GEAR_S_DONE && r.tripwire == 0, "splash %d: status %d, %ld leaves above a bound", round == 1, st, r.tripwire);
        if (round == 0) first = r;
        else if (round == 2)
            CHECK(!dm_gear_key_cmp(r.score.key, first.score.key) && !memcmp(&r.best, &first.best, sizeof r.best) && !memcmp(&r.after, &first.after, sizeof r.after),
                  "splash back off: the re-run differs from the first one");
        dm_gear_search_free(x);
    }
}



static void splash_groups(void) {
    static gstock g;
    gspec s; memset(&s, 0, sizeof s);
    s.profile = g_profile[4]; s.act = 3; s.seed = 7070; s.variants = 1;
    s.count[DM_GEAR_K_WEAPON1] = 4; s.count[DM_GEAR_K_WEAPON2] = 3; s.count[DM_GEAR_K_RING] = 1;
    if (gs_build(&g, g_d, &g_gt, &s)) { CHECK(0, "splash groups stock not built"); return; }
    int groups[DM_SPLASH_UNKNOWN + 1] = { 0 };
    for (int i = 1; i < g.p.nweapon; i++) groups[dm_splash_group_of(g.weapon[i].w.stype, g.weapon[i].mode2)]++;
    CHECK(groups[DM_SPLASH_HEAVY] && groups[DM_SPLASH_MEDIUM] && groups[DM_SPLASH_LIGHT] && groups[DM_SPLASH_NONE] && !groups[DM_SPLASH_UNKNOWN],
          "the stock lacks a weapon group (heavy %d medium %d light %d none %d unknown %d)", groups[DM_SPLASH_HEAVY], groups[DM_SPLASH_MEDIUM],
          groups[DM_SPLASH_LIGHT], groups[DM_SPLASH_NONE], groups[DM_SPLASH_UNKNOWN]);
    for (int k = 0; k < 2; k++) { dm_gear_cand *r = gs_item(&g, DM_GEAR_K_RING, 1, 0); r->item.own.n = 0; dm_gear_list_add(&r->item.own, 0, 0, 20 + 15 * k); }
    dm_gear_problem_ready(&g.p);
    check_stock(&g, "splash groups off", 0, 0);
    check_stock(&g, "splash groups on", 1, 1);

    int raised = 0, same = 0;
    for (int i = 0; i < g.p.ncand; i++) {
        const dm_gear_cand *c = &g.cand[i];
        if (c->kind != DM_GEAR_K_WEAPON1 && c->kind != DM_GEAR_K_WEAPON2) continue;
        dm_gear_pick pk; dm_gear_pick_now(&g.p, &pk); pk.at[DM_GEAR_P_MAIN] = (int16_t)i;
        if (c->kind == DM_GEAR_K_WEAPON2) pk.at[DM_GEAR_P_OFF] = -1;
        dm_gear_measures m[2]; unsigned fl[2];
        for (int on = 0; on < 2; on++) { g.p.splash_on = on; if (dm_gear_leaf_measure(&g.p, &pk, &m[on], &fl[on])) break; }
        if (fl[0] || fl[1]) continue;
        const dm_gear_weapon *w = &g.weapon[c->weapon];
        int none = dm_splash_group_of(w->w.stype, w->mode2) == DM_SPLASH_NONE;
        CHECK(none ? m[1].group == m[0].group : m[1].group > m[0].group, "weapon %d (%s): group %g off, %g on", i, w->w.stype, m[0].group, m[1].group);
        raised += !none; same += none;
    }
    CHECK(raised >= 3 && same >= 1, "the splash groups stock rated %d splashing and %d other weapons", raised, same);
}



static long g_wseed;
static int one_change(int add, uint64_t seed, double *tc, double *tw, long *lc, long *lw) {
    static gstock g; gspec sp; random_spec(&sp, seed);
    if (gs_build(&g, g_d, &g_gt, &sp) || combos(&g) > 1e5) return 0;
    const dm_gear_goal *goal = &g_goals[4];
    dm_gear_result full; dm_gear_search *x = dm_gear_search_new(&g.p); dm_gear_search_run(x, goal, NULL, NULL, &full); dm_gear_search_free(x);

    int j = -1;
    for (int i = 0; i < g.p.ncand && j < 0; i++) {
        if (g.cand[i].worn || g.cand[i].kind == DM_GEAR_K_CHARM) continue;
        int in = 0; for (int k = 0; k < DM_GEAR_P_N; k++) in |= full.best.at[k] == i;
        if (in == add) j = i;
    }
    if (j < 0) return 0;
    int lvl = g.cand[j].item.req_lvl;
    uint64_t f_full = dm_gear_problem_hash(&g.p, goal);
    g.cand[j].item.req_lvl = 999; dm_gear_problem_ready(&g.p);
    CHECK(f_full != dm_gear_problem_hash(&g.p, goal), "stock %llu: the fingerprint does not see the changed item", (unsigned long long)seed);
    dm_gear_result before = full;
    if (add) { x = dm_gear_search_new(&g.p); dm_gear_search_run(x, goal, NULL, NULL, &before); dm_gear_search_free(x); g.cand[j].item.req_lvl = lvl; dm_gear_problem_ready(&g.p); }
    dm_gear_result rc, rw; dm_gear_progress pc, pw; memset(&pc, 0, sizeof pc); memset(&pw, 0, sizeof pw);
    double t0 = now_s();
    x = dm_gear_search_new(&g.p); int sc = dm_gear_search_run(x, goal, NULL, &pc, &rc); dm_gear_search_free(x);
    double t1 = now_s();
    x = dm_gear_search_new(&g.p); dm_gear_search_seed(x, &before.best); int sw = dm_gear_search_run(x, goal, NULL, &pw, &rw); dm_gear_search_free(x);
    double t2 = now_s();
    CHECK(sc == sw && !memcmp(&rc.best, &rw.best, sizeof rc.best) && !memcmp(&rc.after, &rw.after, sizeof rc.after) && !dm_gear_key_cmp(rc.score.key, rw.score.key),
          "stock %llu (%s): the warm run differs from the cold one", (unsigned long long)seed, add ? "added" : "removed");
    *tc += t1 - t0; *tw += t2 - t1; *lc += atomic_load(&pc.leaves); *lw += atomic_load(&pw.leaves); g_wseed += rw.seeded;
    g.cand[j].item.req_lvl = lvl;
    return 1;
}
static void one_changes(int want) {
    double tc = 0, tw = 0; long lc = 0, lw = 0; int n = 0;
    for (uint64_t i = 1; n < want && i < 40 * (uint64_t)want; i++) n += one_change((int)(i & 1), 5000 + i, &tc, &tw, &lc, &lw);
    printf("one item changed, %d stocks (%ld warm runs took the result before): cold %.2f s %ld leaves, warm %.2f s %ld leaves (x%.2f time, x%.2f leaves)\n",
           n, g_wseed, tc, lc, tw, lw, tw > 0 ? tc / tw : 0, lw > 0 ? (double)lc / lw : 0);
}


static int g_next, g_made;
static void *worker(void *x) {
    int want = *(int *)x;
    gstock *g = malloc(sizeof *g); if (!g) abort();
    for (;;) {
        pthread_mutex_lock(&g_lock);
        int i = -1, built = 0;
        while (g_made < want) {
            i = g_next++;
            gspec spec; random_spec(&spec, (uint64_t)i + 1);
            if (gs_build(g, g_d, &g_gt, &spec)) { CHECK(0, "stock %d not built", i); g_made++; continue; }
            if (combos(g) > 1e5) continue;
            g_made++; built = 1; break;
        }
        pthread_mutex_unlock(&g_lock);
        if (!built) break;
        char name[48]; snprintf(name, sizeof name, "stock %d", i);
        check_stock(g, name, i % 4 == 0, i % 2);

        int charms = 0; for (int c = 0; c < g->p.ncand; c++) charms += g->cand[c].kind == DM_GEAR_K_CHARM;
        if (charms) {
            g->p.charms_fixed = 1; snprintf(name, sizeof name, "stock %d, charms fixed", i);
            check_stock(g, name, i % 8 == 0, i % 2);
            g->p.charms_fixed = 0;
            pthread_mutex_lock(&g_lock); g_fixed++; pthread_mutex_unlock(&g_lock);
        }
    }
    free(g);
    return NULL;
}

int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IOLBF, 0);
    g_d = gs_tables(&g_gt);
    for (int i = 0; i < 9; i++) g_profile[i] = gs_profile(k_profiles[i]);
    static const double hits[3] = { 0, 15, 1e9 }, boss[3] = { 0, 0.5, 1 };
    for (int i = 0; i < 9; i++) { g_goals[i].survival_hits = hits[i / 3]; g_goals[i].boss_share = boss[i % 3]; }
    int nstocks = argc > 1 ? atoi(argv[1]) : 200, nworkers = argc > 2 ? atoi(argv[2]) : 8;
    double t0 = now_s();
    pthread_t th[64]; if (nworkers < 1) nworkers = 1; if (nworkers > 64) nworkers = 64;
    for (int w = 0; w < nworkers; w++) pthread_create(&th[w], NULL, worker, &nstocks);
    for (int w = 0; w < nworkers; w++) pthread_join(th[w], NULL);
    double t_random = now_s() - t0;
    long random_stocks = g_stocks;
    static gstock g;
    one_changes(argc > 3 ? atoi(argv[3]) : 40);
    counterexamples(&g);
    mana_stocks(&g);
    double ta = now_s();
    adversarial_cases(&g);
    printf("adversarial cases: %.1f s\n", now_s() - ta);
    splash_switch();
    splash_groups();
    defense_stays();
    worn_stays_on_ties();
    handout_share();
    printf("gear search: %ld stocks (%ld random, %ld of them again with the charms fixed, %.1f s) x 9 goals, %ld combinations enumerated (%.1f s), search %.1f s: %ld runs, %ld leaves, "
           "%ld cut; %ld unrated candidates, %ld dual pairs left out, %ld stopped runs, %ld warm runs that took the start value; %s\n",
           g_stocks, random_stocks, g_fixed, t_random, g_combos, g_t_brute, g_t_search, g_runs, g_leaves, g_pruned, g_unrated, g_dual, g_aborts,
           g_seeded, atomic_load(&failures) ? "FAILED" : "passed");
    return atomic_load(&failures) != 0;
}
