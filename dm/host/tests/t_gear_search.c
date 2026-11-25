


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
        if (!k) { unrated += r.unrated; dual += (g->p.now.cls == 4 || g->p.now.cls == 6) && g->p.dr_ok; }
        CHECK(st == DM_GEAR_S_DONE, "%s goal %d: status %d", name, k, st);
        CHECK(!dm_gear_key_cmp(r.score.key, b->key[k]), "%s goal %g/%g: search key (%g, %g, %g), enumerator (%g, %g, %g) over %ld combinations",
              name, g_goals[k].survival_hits, g_goals[k].boss_share, r.score.key[0], r.score.key[1], r.score.key[2], b->key[k][0], b->key[k][1],
              b->key[k][2], b->combos);
        CHECK(r.tripwire == 0, "%s goal %d: %ld leaves above a bound of their subtree", name, k, r.tripwire);

        { dm_gear_pick now; dm_gear_pick_now(&g->p, &now);
          CHECK(dm_gear_pick_kept(&now, &r.best) == b->kept[k], "%s goal %d: the result keeps %d of the worn gear, the enumerator's best %d", name, k,
                dm_gear_pick_kept(&now, &r.best), b->kept[k]); }

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



static int dual_stock(gstock *g, const char *profile, int act, uint64_t seed, int weapons, int shields, int two_hand) {
    gspec s; memset(&s, 0, sizeof s);
    s.profile = profile; s.act = act; s.seed = seed; s.variants = 1; s.dual = 1;
    s.count[DM_GEAR_K_WEAPON1] = weapons; s.count[DM_GEAR_K_SHIELD] = shields; s.count[DM_GEAR_K_WEAPON2] = two_hand; s.count[DM_GEAR_K_RING] = 1;
    return gs_build(g, g_d, &g_gt, &s);
}
static int weapon_cands(const gstock *g, int *w, int max) {
    int n = 0;
    for (int q = 0; q < g->p.ncand && n < max; q++) if (g->cand[q].kind == DM_GEAR_K_WEAPON1) w[n++] = q;
    return n;
}

static void set_value(gstock *g, int c, int st, int v) {
    dm_gear_list *l = &g->cand[c].item.own;
    dm_gear_list_add(l, st, 0, v - dm_gear_list_get(l, st, 0));
}

static int unrated_of(gstock *g) {
    dm_gear_search *x = dm_gear_search_new(&g->p); dm_gear_result r; dm_gear_goal goal = { 15, 0.5 };
    int st = x ? dm_gear_search_run(x, &goal, NULL, NULL, &r) : -9;
    dm_gear_search_free(x);
    return st == DM_GEAR_S_DONE ? r.unrated : -1;
}
static void dual_cases(gstock *g) {
    const dm_sim_data *d = g_d;


    {
        static int val[2][DM_GEAR_NSTAT]; static uint8_t dr[DM_GEAR_NSTAT]; static dm_sim_char c; static dm_gear_hand h[2];
        memset(val, 0, sizeof val); memset(dr, 0, sizeof dr); memset(h, 0, sizeof h);
        int ll = gs_stat(d, "lifedrainmindam"), ias = gs_stat(d, "item_fasterattackrate");
        dr[19] = dr[ll] = dr[ias] = 1; for (int j = 0; j < 7; j++) { static const int more[7] = { 136, 135, 60, 62, 117, 121, 122 }; dr[more[j]] = 1; }
        val[0][19] = 101; val[1][19] = 50; val[0][ll] = 3; val[1][ll] = 8; val[0][ias] = 20; val[1][ias] = 40;
        char err[128]; if (dm_sim_profile_parse(d, g_profile[4], &c, err, sizeof err)) { CHECK(0, "rule: %s", err); return; }
        c.stat_value[19] = 1000; c.stat_value[ll] = 2; c.stat_value[ias] = 10;
        const int *const vp[2] = { val[0], val[1] };
        CHECK(!dm_gear_dual_stats(d, dr, vp, c.stat_value) && c.stat_value[19] == 1075 && c.stat_value[ll] == 10 && c.stat_value[ias] == 10,
              "rule: stats %d %d %d", c.stat_value[19], c.stat_value[ll], c.stat_value[ias]);
        for (int k = 0; k < 2; k++) {
            dm_ic_cb *v = &h[k].cb; v->str = 150; v->dex = 100; v->lvl = 80; v->thf = 20; v->armed = 1; v->srcdam = 128; v->strb = 100; v->expansion = 1;
            v->mn[0] = k ? 5 : 20; v->mx[0] = k ? 9 : 40; v->tohit = 1000 + val[k][19]; v->tohitpct = k ? 30 : 10; v->dpct = k ? 50 : 0;
            snprintf(h[k].w.wclass, 4, k ? "1HT" : "1HS"); h[k].w.kind = DM_WK_MELEE; h[k].ias = val[k][ias]; h[k].ds = k ? 20 : 0; h[k].mcs = k ? 6 : 0;
        }
        h[0].w.wsm = -10; h[1].w.wsm = 10;
        CHECK(!dm_gear_dual_hands(d, &c, h, vp), "rule: hands not built");
        int base0 = 1101 + 500 + 20 - 35, base1 = 1050 + 500 + 20 - 35;
        CHECK(c.nhands == 2 && !strcmp(c.hand[0].wclass, "1JS") && c.hand[0].ias == 20 && c.hand[1].ias == 40 && c.hand[0].wsm == -10 && c.hand[1].wsm == 10
              && c.ar_base == (base0 + base1) / 2 && c.hand[0].ar_base_d == base0 - c.ar_base && c.hand[1].ar_base_d == base1 - c.ar_base
              && c.hand[0].ar_pct_d == 10 - 20 && c.hand[1].ar_pct_d == 30 - 20 && c.ed == 25 && c.hand[0].ed_d == -25 && c.hand[1].ed_d == 25
              && c.hand[0].ds_d == -10 && c.hand[1].ds_d == 10 && c.mastery_cs == 3 && c.hand[0].cs_d == -3 && c.hand[1].cs_d == 3
              && c.hand[0].ll_d == -5 && c.hand[1].ll_d == 0 && c.hand[0].min == 20 && c.hand[1].max == 9,
              "rule: hands %s ar_base %d (%d %d) ed %d ll_d %d %d", c.hand[0].wclass, c.ar_base, c.hand[0].ar_base_d, c.hand[1].ar_base_d, c.ed, c.hand[0].ll_d, c.hand[1].ll_d);
        int ar, dm; dm_gear_dual_skill(10 + 30, 30 + 41, 7, 2, 5, &ar, &dm);
        CHECK(ar == 55 + 5 && dm == 4, "rule: skill %d %d", ar, dm);
    }
    long pairs_best = 0, stocks = 0;


    for (int i = 0; i < 24; i++) {
        gs_seed = (uint64_t)i * 31 + 5;
        const char *prof = g_profile[4];
        static char lean[1024];
        if (i % 6 == 5) { snprintf(lean, sizeof lean, "%s", "class=4\nlevel=30\ndiff=0\nlife=400\nmana=25\nstr=80\ndex=60\nvit=60\nene=10\nar=1200\ndef=400\n"
                          "weapon=20,40,100,0,0,1,0,0,1HS\nres=0,30,30,30,30,0\nskill=147,10,10\nbar=147\nskill=152,8,8\nbar=152\n"); prof = lean; }
        if (dual_stock(g, prof, 1 + i % 5, 6000 + (uint64_t)i, i % 5, i % 4 == 0, i % 4 == 1)) { CHECK(0, "dual stock %d not built", i); continue; }
        int w[16], n = weapon_cands(g, w, 16);
        if (n < 2) { CHECK(0, "dual stock %d: no pair", i); continue; }
        if (i % 2) set_value(g, w[0], 60, 9);
        if (i % 3 == 1) set_value(g, w[1], 62, 11);
        if (i % 4 >= 2) { set_value(g, w[0], 57, 400); set_value(g, w[0], 58, 600); set_value(g, w[0], 59, 50); }
        if (i % 4 == 3) { set_value(g, w[n - 1], 57, 250); set_value(g, w[n - 1], 58, 300); set_value(g, w[n - 1], 59, 150); }
        if (i % 7 == 3) for (int id = 0; id < DM_SIM_MAXSKILL; id++) if (g->p.node[id]) { g->weapon[g->cand[w[1]].weapon].mth[id] = 40; g->weapon[g->cand[w[1]].weapon].mdmg[id] = 60; }
        if (i % 5 == 4 && g->p.ncand < GS_MAXCAND) { dm_gear_cand *x = &g->cand[g->p.ncand++]; *x = g->cand[w[n - 1]]; x->worn = 0; x->item.slot = 0; x->item.id += 700; }
        if (gs_capture(g)) { CHECK(0, "dual stock %d: no capture", i); continue; }
        if (combos(g) > 2e5) continue;
        char name[48]; snprintf(name, sizeof name, "dual stock %d", i);
        check_stock(g, name, i % 6 == 0, i % 2);
        dm_gear_search *x = dm_gear_search_new(&g->p); dm_gear_result r;
        if (x && dm_gear_search_run(x, &g_goals[4], NULL, NULL, &r) == DM_GEAR_S_DONE) {
            int a = r.best.at[DM_GEAR_P_MAIN], b = r.best.at[DM_GEAR_P_OFF];
            pairs_best += a >= 0 && b >= 0 && g->cand[a].weapon >= 0 && g->cand[b].weapon >= 0;
        }
        dm_gear_search_free(x);
        stocks++;
    }
    CHECK(pairs_best > 0, "no two weapon stock's best holds two weapons");

    static const char *const bash = "class=4\nlevel=80\ndiff=2\nlife=1200\nmana=600\nstr=150\ndex=120\nar=4000\ndef=1600\nfcr=63\nias=40\n"
                                    "weapon=100,160,100,0,0,1,0,0,1HS\nres=15,140,140,140,140,0\nskill=126,20,20\nbar=126\n";
    if (!dual_stock(g, bash, 2, 6100, 0, 0, 0)) {
        int w[4], n = weapon_cands(g, w, 4), right = -1, left = -1;
        for (int k = 0; k < n; k++) { if (g->cand[w[k]].item.slot == 4) right = w[k]; if (g->cand[w[k]].item.slot == 5) left = w[k]; }
        if (right >= 0 && left >= 0) {

            set_value(g, right, 21, 3); set_value(g, right, 22, 6); set_value(g, left, 21, 60); set_value(g, left, 22, 110);
            set_value(g, right, gs_stat(d, "maxhp"), 400 * 256);
            dm_gear_goal goal = { 1e9, 1 }; dm_gear_result r;
            dm_gear_search *x = !gs_capture(g) ? dm_gear_search_new(&g->p) : NULL;
            CHECK(x && dm_gear_search_run(x, &goal, NULL, NULL, &r) == DM_GEAR_S_DONE && r.best.at[DM_GEAR_P_MAIN] == left && r.best.at[DM_GEAR_P_OFF] == right,
                  "hands swapped: best %d / %d, the strong weapon %d worn left", x ? r.best.at[DM_GEAR_P_MAIN] : -9, x ? r.best.at[DM_GEAR_P_OFF] : -9, left);
            dm_gear_search_free(x);
            check_stock(g, "hands swapped", 0, 0);
        } else CHECK(0, "hands swapped: the pair is not worn in both hands");
    } else CHECK(0, "hands swapped: stock not built");

    if (!dual_stock(g, g_profile[4], 5, 6200, 0, 1, 0)) {
        int sh = -1;
        for (int q = 0; q < g->p.ncand; q++) if (g->cand[q].kind == DM_GEAR_K_SHIELD) sh = q;
        if (sh >= 0) {
            set_value(g, sh, 20, 60); set_value(g, sh, 31, 900); set_value(g, sh, gs_stat(d, "maxhp"), 200 * 256);
            dm_gear_goal goal = { 1e9, 0 }; dm_gear_result r;
            dm_gear_search *x = !gs_capture(g) ? dm_gear_search_new(&g->p) : NULL;
            CHECK(x && dm_gear_search_run(x, &goal, NULL, NULL, &r) == DM_GEAR_S_DONE && r.best.at[DM_GEAR_P_OFF] == sh, "shield for survival: off hand %d, shield %d",
                  x ? r.best.at[DM_GEAR_P_OFF] : -9, sh);
            dm_gear_search_free(x);
            check_stock(g, "shield or second weapon", 0, 0);
        } else CHECK(0, "shield for survival: no shield");
    } else CHECK(0, "shield for survival: stock not built");

    if (!dual_stock(g, g_profile[4], 1, 6300, 0, 0, 0)) {
        int w[4], n = weapon_cands(g, w, 4);
        if (n == 2) {
            snprintf(g->weapon[g->cand[w[0]].weapon].w.wclass, 4, "1HS"); snprintf(g->weapon[g->cand[w[1]].weapon].w.wclass, 4, "1HT");
            g->cand[w[0]].item.slot = 4; g->cand[w[1]].item.slot = 5;
            int rated = !gs_capture(g);
            static dm_sim_char c[2]; dm_gear_measures m[2]; unsigned fl[2] = { 0, 0 };
            for (int o = 0; o < 2 && rated; o++) {
                dm_gear_pick pk; dm_gear_pick_now(&g->p, &pk);
                pk.at[DM_GEAR_P_MAIN] = (int16_t)w[o]; pk.at[DM_GEAR_P_OFF] = (int16_t)w[1 - o];
                rated &= !dm_gear_leaf_char(&g->p, &pk, &c[o], NULL, &fl[o]) && !dm_gear_leaf_measure(&g->p, &pk, &m[o], &fl[o]);
            }
            CHECK(rated && !strcmp(c[0].hand[0].wclass, "1JS") && !strcmp(c[1].hand[0].wclass, "1ST"), "swing and thrust: rated %d, classes %s / %s (%#x %#x)",
                  rated, c[0].hand[0].wclass, c[1].hand[0].wclass, fl[0], fl[1]);

            int f[2] = { -1, -1 };
            for (int o = 0; o < 2; o++) for (int j = 0; j < d->nanim; j++)
                if (!strcmp(d->anim[j].token, d->cls[4].token) && !strcmp(d->anim[j].mode, "A1") && !strcmp(d->anim[j].wclass, o ? "1ST" : "1JS")) f[o] = d->anim[j].frames;
            if (rated && f[0] != f[1]) {
                dm_sim_target t; dm_sim_skill_out a[2];
                int ok = dm_sim_target_act(d, &c[0], &t) >= 0 && dm_sim_skill_eval(d, &c[0], &t, 0, &a[0]) >= 0 && dm_sim_skill_eval(d, &c[1], &t, 0, &a[1]) >= 0;
                CHECK(ok && a[0].frames != a[1].frames, "swing and thrust: the normal attack takes %d / %d frames in both orders", a[0].frames, a[1].frames);
            }
            check_stock(g, "swing and thrust", 0, 0);
        } else CHECK(0, "swing and thrust: %d weapons", n);
    } else CHECK(0, "swing and thrust: stock not built");

    if (!dual_stock(g, g_profile[4], 3, 6400, 0, 0, 0)) {
        int w[4], n = weapon_cands(g, w, 4), left = -1, armor = -1, worn_armor = -1;
        for (int k = 0; k < n; k++) { if (g->cand[w[k]].item.slot == 4) { g->cand[w[k]].worn = 0; g->cand[w[k]].item.slot = 0; g->cand[w[k]].item.req_lvl = 999; } else left = w[k]; }
        for (int q = 0; q < g->p.ncand; q++) if (g->cand[q].kind == DM_GEAR_K_ARMOR) { if (g->cand[q].worn) worn_armor = q; }
        if (worn_armor >= 0 && g->p.ncand < GS_MAXCAND) {
            dm_gear_cand *x = &g->cand[g->p.ncand]; *x = g->cand[worn_armor]; x->worn = 0; x->item.id += 900; armor = g->p.ncand++;
            set_value(g, armor, gs_stat(d, "maxhp"), 400 * 256); set_value(g, armor, 31, 1500);
        }
        dm_gear_goal goal = { 15, 0.5 }; dm_gear_result r;
        dm_gear_search *x = left >= 0 && armor >= 0 && !gs_capture(g) ? dm_gear_search_new(&g->p) : NULL;
        CHECK(x && dm_gear_search_run(x, &goal, NULL, NULL, &r) == DM_GEAR_S_DONE && r.best.at[DM_GEAR_P_ARMOR] == armor && r.best.at[DM_GEAR_P_MAIN] == -1 &&
              r.best.at[DM_GEAR_P_OFF] == left, "left weapon alone: best main %d off %d armor %d (left %d, armor %d)", x ? r.best.at[DM_GEAR_P_MAIN] : -9,
              x ? r.best.at[DM_GEAR_P_OFF] : -9, x ? r.best.at[DM_GEAR_P_ARMOR] : -9, left, armor);
        dm_gear_search_free(x);
        if (x) check_stock(g, "left weapon alone", 0, 0);
    } else CHECK(0, "left weapon alone: stock not built");


    if (!dual_stock(g, g_profile[4], 2, 6500, 1, 0, 0)) {
        int w[4], n = weapon_cands(g, w, 4);
        if (n == 3 && !gs_capture(g)) {
            dm_gear_pick pk; dm_gear_pick_now(&g->p, &pk); dm_gear_measures m; unsigned fl;
            int cand = -1; for (int k = 0; k < n; k++) if (!g->cand[w[k]].worn) cand = w[k];
            pk.at[DM_GEAR_P_OFF] = (int16_t)cand;
            CHECK(!dm_gear_leaf_measure(&g->p, &pk, &m, &fl), "unrated pairs: the plain pair has no rating (%#x)", fl);
            int before = unrated_of(g);
            set_value(g, cand, 224, 8); gs_capture(g);
            CHECK(dm_gear_leaf_measure(&g->p, &pk, &m, &fl) && (fl & DM_GEAR_L_DUAL), "unrated pairs: attack rating per level rated (%#x)", fl);

            CHECK(unrated_of(g) == before + 1, "unrated pairs: attack rating per level counts %d unrated, %d without", unrated_of(g), before);
            { dm_gear_pick one; dm_gear_pick_now(&g->p, &one); one.at[DM_GEAR_P_MAIN] = (int16_t)cand; one.at[DM_GEAR_P_OFF] = -1;
              CHECK(!dm_gear_leaf_measure(&g->p, &one, &m, &fl), "unrated pairs: the weapon with attack rating per level alone has no rating (%#x)", fl); }
            check_stock(g, "a weapon with attack rating per level", 0, 0);
            set_value(g, cand, 224, 0);
            g->cand[cand].item.green_read = 1; g->cand[cand].item.setitem = 0; g->cand[cand].item.green[0].n = 1; g->cand[cand].item.green[0].v[0] = (dm_gear_v){ 19, 0, 50 };
            gs_capture(g);
            CHECK(dm_gear_leaf_measure(&g->p, &pk, &m, &fl) && (fl & DM_GEAR_L_DUAL), "unrated pairs: set weapon with green bonuses rated (%#x)", fl);
            CHECK(unrated_of(g) == before + 1, "unrated pairs: set weapon with green bonuses counts %d unrated, %d without", unrated_of(g), before);
            g->cand[cand].item.green_read = 0; g->cand[cand].item.setitem = -1; g->cand[cand].item.green[0].n = 0;
            gs_capture(g); g->p.dr_ok = 0;
            CHECK(dm_gear_leaf_measure(&g->p, &pk, &m, &fl) && (fl & DM_GEAR_L_DUAL), "unrated pairs: no stat table rated (%#x)", fl);
        } else CHECK(0, "unrated pairs: %d weapons", n);
    } else CHECK(0, "unrated pairs: stock not built");
    printf("two weapons: %ld stocks against the enumerator, %ld of them with two weapons best\n", stocks, pairs_best);
}


static int sword_stock(gstock *g, const char *profile, int act, uint64_t seed, int dual, int worn_sword, int swords, int weapons, int shields) {
    gspec s; memset(&s, 0, sizeof s);
    s.profile = profile; s.act = act; s.seed = seed; s.variants = 1; s.dual = dual; s.worn_sword = worn_sword; s.swords = swords;
    s.count[DM_GEAR_K_WEAPON1] = weapons; s.count[DM_GEAR_K_SHIELD] = shields; s.count[DM_GEAR_K_RING] = 1;
    return gs_build(g, g_d, &g_gt, &s);
}
static int sword_cands(const gstock *g, int *w, int max) {
    int n = 0;
    for (int q = 0; q < g->p.ncand && n < max; q++) if (g->cand[q].kind == DM_GEAR_K_WEAPON2 && g->cand[q].weapon >= 0 && g->weapon[g->cand[q].weapon].has_one) w[n++] = q;
    return n;
}

static int sword_beside(const gstock *g, const dm_gear_pick *pk) {
    int m = pk->at[DM_GEAR_P_MAIN], o = pk->at[DM_GEAR_P_OFF];
    return m >= 0 && o >= 0 && (g->cand[m].kind == DM_GEAR_K_WEAPON2 || g->cand[o].kind == DM_GEAR_K_WEAPON2);
}
static int best_of(gstock *g, const dm_gear_goal *goal, dm_gear_result *r) {
    dm_gear_search *x = !gs_capture(g) ? dm_gear_search_new(&g->p) : NULL;
    int st = x ? dm_gear_search_run(x, goal, NULL, NULL, r) : -9;
    dm_gear_search_free(x);
    return st;
}
static void sword_cases(gstock *g) {
    const dm_sim_data *d = g_d;
    long stocks = 0, beside = 0;


    for (int i = 0; i < 18; i++) {
        gs_seed = (uint64_t)i * 37 + 11;
        int dual = i % 2, worn_sword = i % 3 == 0;
        if (sword_stock(g, g_profile[4], 1 + i % 5, 6600 + (uint64_t)i, dual, worn_sword, 1 + i % 3, i % 4, dual ? i % 3 == 1 : 1)) { CHECK(0, "sword stock %d not built", i); continue; }
        int w[8], n = sword_cands(g, w, 8);
        if (!n) { CHECK(0, "sword stock %d: no sword", i); continue; }
        if (i % 4 == 1) set_value(g, w[0], 60, 7);
        if (i % 4 == 2) { set_value(g, w[n - 1], 57, 300); set_value(g, w[n - 1], 58, 500); set_value(g, w[n - 1], 59, 75); }
        if (i % 5 == 2 && g->p.ncand < GS_MAXCAND) { dm_gear_cand *x = &g->cand[g->p.ncand++]; *x = g->cand[w[n - 1]]; x->worn = 0; x->item.slot = 0; x->item.id += 700; }
        if (gs_capture(g)) { CHECK(0, "sword stock %d: no capture", i); continue; }
        if (combos(g) > 2e5) continue;
        char name[48]; snprintf(name, sizeof name, "sword stock %d", i);
        check_stock(g, name, i % 6 == 0, i % 2);
        for (int k = 0; k < 9; k++) { dm_gear_result r; if (best_of(g, &g_goals[k], &r) == DM_GEAR_S_DONE) beside += sword_beside(g, &r.best); }
        stocks++;
    }
    CHECK(beside > 0, "no sword stock's best holds a sword beside another item");
    static const char *const lean = "class=4\nlevel=60\ndiff=2\nlife=500\nmana=200\nstr=150\ndex=80\nvit=60\nene=10\nar=2500\ndef=600\n"
                                    "weapon=20,40,100,0,0,1,0,0,1HS\nres=0,20,20,20,20,0\nskill=126,12,12\nbar=126\n";



    if (!sword_stock(g, lean, 5, 6700, 0, 0, 1, 0, 0)) {
        int w[2], sh = -1, wm = -1; int n = sword_cands(g, w, 2);
        for (int q = 0; q < g->p.ncand; q++) { if (g->cand[q].worn && g->cand[q].kind == DM_GEAR_K_SHIELD) sh = q; if (g->cand[q].worn && g->cand[q].kind == DM_GEAR_K_WEAPON1) wm = q; }
        if (n == 1 && sh >= 0 && wm >= 0) {
            set_value(g, sh, gs_stat(d, "maxhp"), 300 * 256);
            set_value(g, w[0], 21, 60); set_value(g, w[0], 22, 110); set_value(g, w[0], 23, 90); set_value(g, w[0], 24, 170);
            set_value(g, wm, 21, 4); set_value(g, wm, 22, 9); g->cand[w[0]].item.req_str = 0;

            for (int q = 0; q < g->p.ncand; q++) if (!g->cand[q].worn && g->cand[q].kind == DM_GEAR_K_RING) g->cand[q].item.req_lvl = 999;
            dm_gear_goal goal = { 1e9, 0 }; dm_gear_result r;
            CHECK(best_of(g, &goal, &r) == DM_GEAR_S_DONE && r.best.at[DM_GEAR_P_MAIN] == w[0] && r.best.at[DM_GEAR_P_OFF] == sh,
                  "sword with a shield: best %d / %d, sword %d shield %d", r.best.at[DM_GEAR_P_MAIN], r.best.at[DM_GEAR_P_OFF], w[0], sh);

            static dm_sim_char c; static dm_gear_char comp; unsigned fl; dm_gear_pick pk; dm_gear_pick_now(&g->p, &pk);
            pk.at[DM_GEAR_P_MAIN] = (int16_t)w[0];
            int one = !dm_gear_leaf_char(&g->p, &pk, &c, &comp, &fl) && !strcmp(c.hand[0].wclass, "1HS") && !c.hand[0].twohand && c.hand[0].min == comp.value[21] + comp.value[185]
                      && c.hand[0].max == comp.value[22] + comp.value[186] && c.nhands == 1;
            CHECK(one, "sword with a shield: class %s twohand %d damage %d-%d, one-hand totals %d-%d", c.hand[0].wclass, c.hand[0].twohand, c.hand[0].min, c.hand[0].max,
                  comp.value[21], comp.value[22]);
            pk.at[DM_GEAR_P_OFF] = -1;
            int two = !dm_gear_leaf_char(&g->p, &pk, &c, &comp, &fl) && !strcmp(c.hand[0].wclass, "2HS") && c.hand[0].twohand && c.hand[0].min == comp.value[23] + comp.value[185]
                      && c.hand[0].max == comp.value[24] + comp.value[186];
            CHECK(two, "sword alone: class %s twohand %d damage %d-%d, two-hand totals %d-%d", c.hand[0].wclass, c.hand[0].twohand, c.hand[0].min, c.hand[0].max, comp.value[23], comp.value[24]);
            check_stock(g, "sword with a shield", 0, 0);

            int cls = g->p.now.cls; g->p.now.cls = 0;
            pk.at[DM_GEAR_P_OFF] = (int16_t)sh;
            CHECK(dm_gear_leaf_char(&g->p, &pk, &c, &comp, &fl) && (fl & DM_GEAR_L_INPUT), "sword with a shield for class 0: rated (%#x)", fl);
            g->p.now.cls = cls;

            g->weapon[g->cand[w[0]].weapon].has_one = 0;
            CHECK(dm_gear_leaf_char(&g->p, &pk, &c, &comp, &fl) && (fl & DM_GEAR_L_INPUT), "two-hand weapon with a shield: rated (%#x)", fl);
            CHECK(best_of(g, &goal, &r) == DM_GEAR_S_DONE && !sword_beside(g, &r.best), "two-hand weapon with a shield: best %d / %d", r.best.at[DM_GEAR_P_MAIN], r.best.at[DM_GEAR_P_OFF]);
            check_stock(g, "two-hand weapon only alone", 0, 0);
        } else CHECK(0, "sword with a shield: %d swords, shield %d, weapon %d", n, sh, wm);
    } else CHECK(0, "sword with a shield: stock not built");


    if (!sword_stock(g, lean, 5, 6800, 1, 0, 1, 0, 0)) {
        int w[2], n = sword_cands(g, w, 2), right = -1, left = -1;
        for (int q = 0; q < g->p.ncand; q++) if (g->cand[q].worn && g->cand[q].kind == DM_GEAR_K_WEAPON1) { if (g->cand[q].item.slot == 4) right = q; else left = q; }
        if (n == 1 && right >= 0 && left >= 0) {
            set_value(g, right, gs_stat(d, "maxhp"), 300 * 256); set_value(g, left, 21, 2); set_value(g, left, 22, 4);
            set_value(g, w[0], 21, 60); set_value(g, w[0], 22, 110); g->cand[w[0]].item.req_str = 0;
            dm_gear_goal goal = { 1e9, 1 }; dm_gear_result r;
            int st = best_of(g, &goal, &r), m = r.best.at[DM_GEAR_P_MAIN], o = r.best.at[DM_GEAR_P_OFF];
            CHECK(st == DM_GEAR_S_DONE && ((m == right && o == w[0]) || (m == w[0] && o == right)), "sword as a second weapon: best %d / %d, sword %d life weapon %d", m, o, w[0], right);
            static dm_sim_char c; unsigned fl; dm_gear_pick pk; dm_gear_pick_now(&g->p, &pk); pk.at[DM_GEAR_P_OFF] = (int16_t)w[0];
            CHECK(!dm_gear_leaf_char(&g->p, &pk, &c, NULL, &fl) && c.nhands == 2 && !strcmp(c.hand[0].wclass, "1SS"), "sword as a second weapon: hands %d class %s (%#x)", c.nhands,
                  c.hand[0].wclass, fl);
            check_stock(g, "sword as a second weapon", 0, 0);
        } else CHECK(0, "sword as a second weapon: %d swords, weapons %d / %d", n, right, left);
    } else CHECK(0, "sword as a second weapon: stock not built");

    if (!sword_stock(g, g_profile[4], 3, 6900, 1, 1, 0, 0, 0)) {
        int w[2], n = sword_cands(g, w, 2), other = -1, armor = -1, worn_armor = -1;
        for (int q = 0; q < g->p.ncand; q++) if (g->cand[q].worn && g->cand[q].kind == DM_GEAR_K_WEAPON1) other = q;
        for (int q = 0; q < g->p.ncand; q++) if (g->cand[q].kind == DM_GEAR_K_ARMOR && g->cand[q].worn) worn_armor = q;
        if (n == 1 && other >= 0 && worn_armor >= 0 && g->p.ncand < GS_MAXCAND) {
            g->cand[other].worn = 0; g->cand[other].item.slot = 0; g->cand[other].item.req_lvl = 999; g->cand[w[0]].item.slot = 5;
            dm_gear_cand *x = &g->cand[g->p.ncand]; *x = g->cand[worn_armor]; x->worn = 0; x->item.id += 900; armor = g->p.ncand++;
            set_value(g, armor, gs_stat(d, "maxhp"), 400 * 256); set_value(g, armor, 31, 1500);
            dm_gear_goal goal = { 15, 0.5 }; dm_gear_result r;
            CHECK(best_of(g, &goal, &r) == DM_GEAR_S_DONE && r.best.at[DM_GEAR_P_ARMOR] == armor && r.best.at[DM_GEAR_P_MAIN] == -1 && r.best.at[DM_GEAR_P_OFF] == w[0],
                  "sword alone left: best main %d off %d armor %d (sword %d, armor %d)", r.best.at[DM_GEAR_P_MAIN], r.best.at[DM_GEAR_P_OFF], r.best.at[DM_GEAR_P_ARMOR], w[0], armor);
            check_stock(g, "sword alone left", 0, 0);
        } else CHECK(0, "sword alone left: %d swords, weapon %d, armor %d", n, other, worn_armor);
    } else CHECK(0, "sword alone left: stock not built");
    printf("two-handed swords: %ld stocks against the enumerator, %ld best results hold a sword beside another item\n", stocks, beside);
}


static const char *const k_claw_prof = "class=6\nlevel=70\ndiff=1\nlife=900\nmana=300\nstr=120\ndex=120\nar=2500\ndef=900\nias=20\n"
                                       "weapon=20,40,100,0,0,1,0,0,HT1\nres=15,60,60,60,60,0\nskill=252,12,12\nskill=260,15,15\nbar=260\nskill=254,8,8\nbar=254\n";
static int claw_stock(gstock *g, const char *profile, int act, uint64_t seed, int claws, int weapons, int shields) {
    gspec s; memset(&s, 0, sizeof s);
    s.profile = profile; s.act = act; s.seed = seed; s.variants = 1; s.dual = 1; s.claws = claws;
    s.count[DM_GEAR_K_WEAPON1] = weapons; s.count[DM_GEAR_K_SHIELD] = shields; s.count[DM_GEAR_K_RING] = 1;
    return gs_build(g, g_d, &g_gt, &s);
}

static void claw_mastery(gstock *g, int th, int dmg) {
    for (int q = 0; q < g->p.ncand; q++) {
        int w = g->cand[q].weapon;
        if (w < 0 || g->weapon[w].w.kind != DM_WK_CLAW) continue;
        for (int id = 0; id < DM_SIM_MAXSKILL; id++) if (g->p.node[id]) { g->weapon[w].mth[id] = (int16_t)th; g->weapon[w].mdmg[id] = (int16_t)dmg; }
    }
}
static void claw_cases(gstock *g) {
    long stocks = 0, pairs_best = 0;


    for (int i = 0; i < 18; i++) {
        gs_seed = (uint64_t)i * 41 + 7;
        const char *prof = i % 2 ? g_profile[6] : k_claw_prof;
        if (claw_stock(g, prof, 1 + i % 5, 6900 + (uint64_t)i, i % 3 == 2 ? 2 : 1, i % 4 + (i % 3 == 2), i % 4 == 0)) { CHECK(0, "claw stock %d not built", i); continue; }
        int w[16], n = weapon_cands(g, w, 16);
        if (n < 2) { CHECK(0, "claw stock %d: no pair", i); continue; }
        if (i % 2) set_value(g, w[0], 60, 7);
        if (i % 4 >= 2) { set_value(g, w[1], 57, 300); set_value(g, w[1], 58, 450); set_value(g, w[1], 59, 75); }
        if (i % 3 == 0) claw_mastery(g, 40 + i, 25 + i);
        if (i % 5 == 4 && g->p.ncand < GS_MAXCAND) { dm_gear_cand *x = &g->cand[g->p.ncand++]; *x = g->cand[w[0]]; x->worn = 0; x->item.slot = 0; x->item.id += 700; }
        if (gs_capture(g)) { CHECK(0, "claw stock %d: no capture", i); continue; }
        if (combos(g) > 2e5) continue;
        char name[48]; snprintf(name, sizeof name, "claw stock %d", i);
        check_stock(g, name, i % 6 == 0, i % 2);
        dm_gear_result r;
        if (best_of(g, &g_goals[4], &r) == DM_GEAR_S_DONE) {
            int a = r.best.at[DM_GEAR_P_MAIN], b = r.best.at[DM_GEAR_P_OFF];
            pairs_best += a >= 0 && b >= 0 && g->cand[a].weapon >= 0 && g->cand[b].weapon >= 0;
        }
        stocks++;
    }
    CHECK(pairs_best > 0, "no claw stock's best holds two claws");

    if (!claw_stock(g, k_claw_prof, 1, 7000, 1, 0, 0)) {
        dm_gear_pick pk; dm_gear_pick_now(&g->p, &pk); static dm_sim_char c; unsigned fl;
        CHECK(!dm_gear_leaf_char(&g->p, &pk, &c, NULL, &fl) && c.nhands == 2 && !strcmp(c.hand[0].wclass, "HT2"), "worn claws: hands %d class %s (%#x)", c.nhands,
              c.hand[0].wclass, fl);
    } else CHECK(0, "worn claws: stock not built");

    if (!claw_stock(g, k_claw_prof, 2, 7100, 2, 2, 1)) {
        int w[8], n = weapon_cands(g, w, 8), plain = -1, claw = -1;
        for (int k = 0; k < n; k++) { if (g->weapon[g->cand[w[k]].weapon].w.kind != DM_WK_CLAW) plain = w[k]; else if (g->cand[w[k]].worn) claw = w[k]; }
        if (plain >= 0 && claw >= 0) {
            dm_gear_pick pk; dm_gear_pick_now(&g->p, &pk); dm_gear_measures m; unsigned fl;
            pk.at[DM_GEAR_P_MAIN] = (int16_t)claw; pk.at[DM_GEAR_P_OFF] = (int16_t)plain;
            CHECK(dm_gear_leaf_measure(&g->p, &pk, &m, &fl) && (fl & DM_GEAR_L_DUAL), "claw beside a plain weapon rated (%#x)", fl);
            pk.at[DM_GEAR_P_MAIN] = (int16_t)plain; pk.at[DM_GEAR_P_OFF] = (int16_t)claw;
            CHECK(dm_gear_leaf_measure(&g->p, &pk, &m, &fl) && (fl & DM_GEAR_L_DUAL), "plain weapon beside a claw rated (%#x)", fl);
            pk.at[DM_GEAR_P_OFF] = -1;
            CHECK(!dm_gear_leaf_measure(&g->p, &pk, &m, &fl), "the plain weapon alone has no rating (%#x)", fl);
            CHECK(unrated_of(g) == 0, "a plain weapon beside claws counts %d unrated", unrated_of(g));
            check_stock(g, "a plain weapon beside claws", 0, 0);
        } else CHECK(0, "plain weapon: no plain weapon %d or worn claw %d", plain, claw);
    } else CHECK(0, "plain weapon: stock not built");


    if (!claw_stock(g, k_claw_prof, 3, 7200, 1, 1, 0)) {
        int w[8], n = weapon_cands(g, w, 8), strong = -1, copy = -1;
        for (int k = 0; k < n; k++) if (!g->cand[w[k]].worn) strong = w[k];
        dm_gear_result r;
        if (strong >= 0 && g->p.ncand < GS_MAXCAND) {
            set_value(g, strong, 21, 80); set_value(g, strong, 22, 140); set_value(g, strong, 0, 100);
            copy = g->p.ncand++; g->cand[copy] = g->cand[strong]; g->cand[copy].item.id += 700;
            int a = -9, b = -9;
            if (best_of(g, &g_goals[1], &r) == DM_GEAR_S_DONE) { a = r.best.at[DM_GEAR_P_MAIN]; b = r.best.at[DM_GEAR_P_OFF]; }
            CHECK((a == strong && b == copy) || (a == copy && b == strong), "stronger claws: best %d / %d, the strong claws %d / %d", a, b, strong, copy);
            check_stock(g, "two stronger claws", 0, 0);
        } else CHECK(0, "stronger claws: none in the stock");
    } else CHECK(0, "stronger claws: stock not built");
    printf("two claws: %ld stocks against the enumerator, %ld of them with two claws best\n", stocks, pairs_best);
}





static void passive_hand_cases(gstock *g) {
    const dm_sim_data *d = g_d; char err[128];
    static dm_sim_char now, x; static int mth[DM_SIM_MAXSKILL], mdmg[DM_SIM_MAXSKILL], prog[DM_SIM_MAXSKILL];
    enum { BLADE = 127, AXE = 128, CLAW_MASTERY = 252, DRAGON_CLAW = 260 };


    if (dm_sim_profile_parse(d, "class=6\nlevel=70\nlife=900\nmana=300\nweapon=20,40,100,0,0,5,0,0,HT2\noffhand=20,40,100,0,0,5,0,0,HT1\n"
                             "skill=252,11,11\nskill=260,15,15\nbar=260\n", &now, err, sizeof err)) { CHECK(0, "passive hand: %s", err); return; }
    for (int id = 0; id < DM_SIM_MAXSKILL; id++) { mth[id] = mdmg[id] = DM_SIM_NO_NODE; prog[id] = 0; }
    mth[0] = mth[DRAGON_CLAW] = 130; mdmg[0] = mdmg[DRAGON_CLAW] = 75; prog[DRAGON_CLAW] = 7;
    mth[CLAW_MASTERY] = mdmg[CLAW_MASTERY] = 0;
    dm_ic_cb cb; memset(&cb, 0, sizeof cb); cb.tohitpct = 20; cb.ar_mast = 130; cb.dmast = 75;
    x = now; x.lvl[CLAW_MASTERY] = 12;
    CHECK(!dm_gear_hand_passive(d, &now, &x, &cb, mth, mdmg, 17, prog) && x.ar_bonus[0] == 20 + 140 && x.ar_bonus[DRAGON_CLAW] == 20 + 140 + 7 &&
          x.damage_mastery_delta[0] == 0 && x.damage_mastery_delta[DRAGON_CLAW] == 0 && x.mastery_cs == 18 && cb.ar_mast == 140 && cb.dmast == 79 && cb.tohitpct == 20,
          "claw mastery 11 -> 12: ar %d / %d, delta %d, crit %d, hand %d / %d", x.ar_bonus[0], x.ar_bonus[DRAGON_CLAW], x.damage_mastery_delta[0], x.mastery_cs, cb.ar_mast, cb.dmast);

    memset(&cb, 0, sizeof cb); cb.tohitpct = 20; cb.ar_mast = 130; cb.dmast = 75; x = now;
    CHECK(!dm_gear_hand_passive(d, &now, &x, &cb, mth, mdmg, 17, prog) && x.ar_bonus[0] == 150 && x.mastery_cs == 17 && cb.ar_mast == 130 && cb.dmast == 75,
          "claw mastery unchanged: ar %d crit %d hand %d / %d", x.ar_bonus[0], x.mastery_cs, cb.ar_mast, cb.dmast);

    mth[0] = 129; x = now; x.lvl[CLAW_MASTERY] = 12;
    CHECK(dm_gear_hand_passive(d, &now, &x, &cb, mth, mdmg, 17, prog) < 0, "an unexplained mastery value is followed");


    if (dm_sim_profile_parse(d, "class=4\nlevel=70\nlife=900\nmana=300\nweapon=20,40,100,0,0,1,0,0,1SS\noffhand=20,40,100,0,0,1,0,0,1HS\n"
                             "skill=127,10,10\nskill=128,5,5\nskill=147,15,15\nbar=147\n", &now, err, sizeof err)) { CHECK(0, "passive hand: %s", err); return; }
    const struct { int th, dmg, crit, axe_only, th2, dmg2, crit2; } k[] = { { 112, 73, 23, 0, 120, 78, 24 }, { 72, 48, 17, 0, 80, 53, 19 },
                                                                           { 112, 73, 23, 1, 112, 73, 23 }, { 72, 48, 17, 1, 80, 53, 19 } };
    for (int i = 0; i < 4; i++) {
        for (int id = 0; id < DM_SIM_MAXSKILL; id++) { mth[id] = mdmg[id] = DM_SIM_NO_NODE; prog[id] = 0; }
        mth[0] = mth[147] = k[i].th; mdmg[0] = mdmg[147] = k[i].dmg; mth[BLADE] = mdmg[BLADE] = mth[AXE] = mdmg[AXE] = 0;
        memset(&cb, 0, sizeof cb); cb.ar_mast = k[i].th; cb.dmast = k[i].dmg;
        x = now; x.lvl[AXE] = 6; if (!k[i].axe_only) x.lvl[BLADE] = 11;
        CHECK(!dm_gear_hand_passive(d, &now, &x, &cb, mth, mdmg, k[i].crit, prog) && x.ar_bonus[147] == k[i].th2 && cb.ar_mast == k[i].th2 && cb.dmast == k[i].dmg2 &&
              x.mastery_cs == k[i].crit2, "class 4 hand %d: ar %d hand %d / %d crit %d", i, x.ar_bonus[147], cb.ar_mast, cb.dmast, x.mastery_cs);
    }


    if (claw_stock(g, k_claw_prof, 1, 7300, 1, 0, 0)) { CHECK(0, "passive claws: stock not built"); return; }
    for (int q = 0; q < g->p.ncand; q++) {
        int w = g->cand[q].weapon; if (w < 0 || g->weapon[w].w.kind != DM_WK_CLAW) continue;
        g->weapon[w].mcrit = 18;
        for (int id = 0; id < DM_SIM_MAXSKILL; id++) if (g->p.node[id]) { g->weapon[w].mth[id] = 140; g->weapon[w].mdmg[id] = 79; }
    }
    int ring = -1;
    for (int q = 0; q < g->p.ncand; q++) if (g->cand[q].kind == DM_GEAR_K_RING && !g->cand[q].worn) ring = q;
    if (ring < 0) { CHECK(0, "passive claws: no ring"); return; }
    static dm_sim_char with, without; unsigned fl_with = 0, fl_without = 0;
    dm_gear_pick pk;
    dm_gear_measures m_with, m_without; unsigned mf_with = 0, mf_without = 0;
    set_value(g, ring, 127, 1);
    if (gs_capture(g)) { CHECK(0, "passive claws: no capture"); return; }
    dm_gear_pick_now(&g->p, &pk); pk.at[DM_GEAR_P_RING1] = (int16_t)ring;
    int rc_with = dm_gear_leaf_char(&g->p, &pk, &with, NULL, &fl_with), measured_with = dm_gear_leaf_measure(&g->p, &pk, &m_with, &mf_with);
    set_value(g, ring, 127, 0);
    if (gs_capture(g)) { CHECK(0, "passive claws: no capture"); return; }
    dm_gear_pick_now(&g->p, &pk); pk.at[DM_GEAR_P_RING1] = (int16_t)ring;
    int rc_without = dm_gear_leaf_char(&g->p, &pk, &without, NULL, &fl_without), measured_without = dm_gear_leaf_measure(&g->p, &pk, &m_without, &mf_without);

    CHECK(!measured_with && !measured_without && m_with.single > m_without.single, "passive claws: rated %d / %d (%#x / %#x), single %.2f / %.2f",
          measured_with, measured_without, mf_with, mf_without, m_with.single, m_without.single);
    CHECK(!rc_with && !rc_without && !(fl_with & DM_GEAR_L_PASSIVE) && with.nhands == 2 && with.lvl[CLAW_MASTERY] == 13 && with.ar_bonus[0] - without.ar_bonus[0] == 10
          && with.ar_bonus[DRAGON_CLAW] - without.ar_bonus[DRAGON_CLAW] == 10, "passive claws: rc %d / %d flags %#x / %#x, level %d, ar %d / %d, dragon claw %d / %d",
          rc_with, rc_without, fl_with, fl_without, with.lvl[CLAW_MASTERY], with.ar_bonus[0], without.ar_bonus[0], with.ar_bonus[DRAGON_CLAW], without.ar_bonus[DRAGON_CLAW]);
    puts("passive masteries per hand: claw mastery, two class 4 masteries, the search with two claws and + skills");
}
static void hand_cases(gstock *g) {
    static const char *const what[GS_HAND_STOCKS] = { "Frenzy, a second weapon", "Double Swing, a second weapon", "Frenzy, one weapon",
        "Dragon Claw, a second claw", "Fists of Fire, a plain weapon worn", "Fists of Fire bare-handed", "Multiple Shot, a stronger sword" };
    for (int k = 0; k < GS_HAND_STOCKS; k++) {
        int focus;
        if (gs_hand_stock(g, g_d, &g_gt, k, 7500 + 40 * (uint64_t)k, &focus)) { CHECK(0, "%s: stock not built", what[k]); continue; }
        check_stock(g, what[k], 0, 0);
        dm_gear_pick now; dm_gear_pick_now(&g->p, &now);
        for (int q = 0; q < 3; q++) {
            dm_gear_result r;
            if (best_of(g, &g_goals[q], &r) != DM_GEAR_S_DONE) { CHECK(0, "%s, goal %d: not done", what[k], q); continue; }
            int a = r.best.at[DM_GEAR_P_MAIN], b = r.best.at[DM_GEAR_P_OFF];
            if (k == 0 || k == 1 || k == 3)
                CHECK((a == focus && b == now.at[DM_GEAR_P_MAIN]) || (b == focus && a == now.at[DM_GEAR_P_MAIN]), "%s, goal %d: hands %d / %d, want %d and %d", what[k], q, a, b,
                      focus, now.at[DM_GEAR_P_MAIN]);
        }
    }
}




static int wblock_stock(gstock *g, int level, uint64_t seed) {
    static char prof[1024], skill[32];
    snprintf(skill, sizeof skill, level ? "skill=263,%d,%d\n" : "\n", level, level < 20 ? level : 20);
    snprintf(prof, sizeof prof, "class=6\nlevel=70\ndiff=1\nlife=900\nmana=300\nstr=120\ndex=120\nar=2500\ndef=900\nias=20\n"
             "weapon=20,40,100,0,0,1,0,0,HT1\nres=15,60,60,60,60,0\nskill=252,12,12\nskill=254,8,8\nbar=254\n%s", skill);
    gspec s; memset(&s, 0, sizeof s);
    s.profile = prof; s.act = 2; s.seed = seed; s.variants = 1; s.claws = 1; s.count[DM_GEAR_K_WEAPON1] = 1; s.count[DM_GEAR_K_RING] = 1;
    if (gs_build(g, g_d, &g_gt, &s)) return -1;


    for (int q = 0; q < g->p.ncand; q++) {
        dm_gear_cand *c = &g->cand[q];
        if (c->kind == DM_GEAR_K_SHIELD) { c->item.own.n = 0; dm_gear_list_add(&c->item.own, 20, 0, 30); }
        if (c->kind == DM_GEAR_K_WEAPON1) { c->item.own.n = 0; dm_gear_list_add(&c->item.own, 21, 0, 10); dm_gear_list_add(&c->item.own, 22, 0, 20); c->item.req_str = 0; }
        if (c->kind == DM_GEAR_K_RING) c->item.own.n = 0;
    }
    return gs_capture(g);
}
static void wblock_cases(gstock *g) {
    dm_gear_goal goal = { 1e9, 0 }; int pairs = 0, walked = 0;
    static const struct { int level, ring, pair; const char *what; } cases[] = {
        { 20, 0, 1, "Weapon Block 20" }, { 0, 0, 0, "Weapon Block unlearned" }, { 1, 0, 0, "Weapon Block 1" }, { 1, 19, 1, "Weapon Block 1 and a ring with +19" } };
    for (size_t k = 0; k < sizeof cases / sizeof *cases; k++) {
        if (wblock_stock(g, cases[k].level, 7300 + k)) { CHECK(0, "%s: stock not built", cases[k].what); continue; }
        int ring = -1, claw = -1, shield = -1;
        for (int q = 0; q < g->p.ncand; q++) {
            if (g->cand[q].kind == DM_GEAR_K_RING && !g->cand[q].worn) ring = q;
            if (g->cand[q].kind == DM_GEAR_K_WEAPON1 && !g->cand[q].worn) claw = q;
            if (g->cand[q].kind == DM_GEAR_K_SHIELD && g->cand[q].worn) shield = q;
        }
        if (ring < 0 || claw < 0 || shield < 0) { CHECK(0, "%s: ring %d claw %d shield %d", cases[k].what, ring, claw, shield); continue; }
        if (cases[k].ring) dm_gear_list_add(&g->cand[ring].item.own, 107, 263, cases[k].ring);

        dm_gear_pick pk; dm_gear_pick_now(&g->p, &pk); pk.at[DM_GEAR_P_OFF] = (int16_t)claw;
        static dm_sim_char c; unsigned fl; int bad = 0;
        if (gs_capture(g) || dm_gear_leaf_char(&g->p, &pk, &c, NULL, &fl)) { CHECK(0, "%s: the pair has no character (%#x)", cases[k].what, fl); continue; }
        CHECK(c.block == dm_sim_weapon_block(g_d, &c, &bad) && (cases[k].level > 0) == (c.block > 0), "%s: the pair blocks %d", cases[k].what, c.block);
        dm_gear_result r;
        int st = best_of(g, &goal, &r), off = r.best.at[DM_GEAR_P_OFF];
        CHECK(st == DM_GEAR_S_DONE && (cases[k].pair ? off == claw : off == shield), "%s: off hand %d (claw %d, shield %d)", cases[k].what, off, claw, shield);
        pairs += st == DM_GEAR_S_DONE && off == claw;
        check_stock(g, cases[k].what, 0, 0);
        dm_gear_search *x = dm_gear_search_new(&g->p); dm_gear_walk w;
        CHECK(x && !dm_gear_search_test_walk(x, &w) && w.above == 0 && w.dual > 0, "%s: walk %ld leaves, %ld with two claws, %ld above a bound", cases[k].what,
              x ? w.leaves : -1, x ? w.dual : -1, x ? w.above : -1);
        walked += x != NULL;
        dm_gear_search_free(x);
    }
    printf("weapon block: %d stocks with a claw and a shield worn, %d best with two claws, %d walked leaf by leaf\n", (int)(sizeof cases / sizeof *cases), pairs, walked);
}






static const char *const k_tie_prof =
    "class=4\nlevel=12\ndiff=0\nlife=220\nmana=60\nstr=50\ndex=30\nar=260\ndef=60\nias=0\nres=0,0,0,0,0,0\nshield=0\nkick_min=1\nkick_max=4\n"
    "weapon=14,17,100,0,10,1,0,0,1HS\noffhand=5,10,100,0,0,1,0,0,1HS\nskill=126,3,3\n%s";
static int tie_stock(gstock *g, const char *bar, uint64_t seed, int armor_needed) {
    static char prof[1024]; snprintf(prof, sizeof prof, k_tie_prof, bar);
    gspec s; memset(&s, 0, sizeof s);
    s.profile = prof; s.act = 1; s.seed = seed; s.variants = 1; s.dual = 1; s.count[DM_GEAR_K_WEAPON1] = 1;
    if (gs_build(g, g_d, &g_gt, &s)) return -1;
    int w[8], n = weapon_cands(g, w, 8), right = -1, left = -1, fresh = -1;
    for (int k = 0; k < n; k++) { if (g->cand[w[k]].item.slot == 4) right = w[k]; else if (g->cand[w[k]].item.slot == 5) left = w[k]; else fresh = w[k]; }
    if (right < 0 || left < 0 || fresh < 0) return -1;

    for (int k = 0; k < n; k++) { g->cand[w[k]].item.own.n = 0; dm_gear_list_add(&g->cand[w[k]].item.own, 68, 0, -g->weapon[g->cand[w[k]].weapon].w.wsm); g->cand[w[k]].item.req_str = 0; }
    set_value(g, right, 21, 10); set_value(g, right, 22, 14); set_value(g, left, 21, 5); set_value(g, left, 22, 10);
    set_value(g, fresh, 21, 30); set_value(g, fresh, 22, 45);
    if (armor_needed) {
        int worn_armor = -1; for (int q = 0; q < g->p.ncand; q++) if (g->cand[q].kind == DM_GEAR_K_ARMOR && g->cand[q].worn) worn_armor = q;
        if (worn_armor < 0 || g->p.ncand >= GS_MAXCAND) return -1;
        set_value(g, worn_armor, 0, 0);
        dm_gear_cand *a = &g->cand[g->p.ncand]; *a = g->cand[worn_armor]; a->worn = 0; a->item.slot = 0; a->item.id += 900; g->p.ncand++;
        dm_gear_list_add(&a->item.own, 0, 0, 40); dm_gear_list_add(&a->item.own, 31, 0, 30);
        g->cand[fresh].item.req_str = dm_gear_list_get(&g->p.base.base, 0, 0) + 30;
    }
    return gs_capture(g);
}
static void tie_runs(gstock *g, const char *what, int fresh, int left, const dm_gear_pick *seed, int want_left) {
    static const atomic_int wide_on = 1; dm_gear_goal goal = { 15, 0.5 };
    dm_gear_result r[3]; int st[3];
    for (int k = 0; k < 3; k++) {
        dm_gear_search *x = dm_gear_search_new(&g->p);
        if (!x) { st[k] = -9; continue; }
        if (k == 1) dm_gear_search_wide(x, DM_GEAR_MAXTHREADS, &wide_on, NULL);
        if (k == 2) dm_gear_search_seed(x, seed);
        st[k] = dm_gear_search_run(x, &goal, NULL, NULL, &r[k]);
        dm_gear_search_free(x);
    }
    CHECK(st[0] == DM_GEAR_S_DONE && r[0].best.at[DM_GEAR_P_MAIN] == fresh && r[0].best.at[DM_GEAR_P_OFF] == (want_left ? left : -1),
          "%s: right %d left %d (the new weapon %d, the worn left %d)", what, r[0].best.at[DM_GEAR_P_MAIN], r[0].best.at[DM_GEAR_P_OFF], fresh, left);
    for (int k = 1; k < 3; k++)
        CHECK(st[k] == st[0] && !memcmp(&r[k].best, &r[0].best, sizeof r[0].best), "%s: the run %s picks another combination", what, k == 1 ? "on all threads" : "from the seed");
}
static void tie_cases(gstock *g) {
    int n = 0;
    for (int armor = 0; armor < 2; armor++) {
        const char *what = armor ? "a new right weapon only the tree reaches" : "a new right weapon";
        int built = -1; for (uint64_t seed = 7400; seed < 7420 && built; seed++) built = tie_stock(g, "bar=126\n", seed + 20 * (uint64_t)armor, armor);
        if (built) { CHECK(0, "%s: stock not built", what); continue; }
        int w[8], nw = weapon_cands(g, w, 8), left = -1, fresh = -1;
        for (int k = 0; k < nw; k++) { if (g->cand[w[k]].item.slot == 5) left = w[k]; else if (!g->cand[w[k]].worn) fresh = w[k]; }

        dm_gear_pick with, alone; dm_gear_pick_now(&g->p, &with); with.at[DM_GEAR_P_MAIN] = (int16_t)fresh;
        if (armor) for (int q = 0; q < g->p.ncand; q++) if (g->cand[q].kind == DM_GEAR_K_ARMOR && !g->cand[q].worn) with.at[DM_GEAR_P_ARMOR] = (int16_t)q;
        alone = with; alone.at[DM_GEAR_P_OFF] = -1;
        dm_gear_measures m1, m2; unsigned f1, f2;
        CHECK(!dm_gear_leaf_measure(&g->p, &with, &m1, &f1) && !dm_gear_leaf_measure(&g->p, &alone, &m2, &f2) && m1.group == m2.group && m1.single == m2.single &&
              m1.survival_s == m2.survival_s, "%s: the left weapon changes a measure (%g / %g)", what, m1.group, m2.group);
        tie_runs(g, what, fresh, left, &alone, 1);
        check_stock(g, what, 0, 0);
        n++;
    }

    if (!tie_stock(g, "bar=126\n", 7450, 0)) {
        int mf = gs_stat(g_d, "item_magicbonus"), worn[2] = { -1, -1 }, nworn = 0, better = -1;
        for (int q = 0; q < g->p.ncand; q++) if (g->cand[q].kind == DM_GEAR_K_RING) {
            if (g->cand[q].worn && nworn < 2) { worn[nworn++] = q; g->cand[q].item.own.n = 0; dm_gear_list_add(&g->cand[q].item.own, mf, 0, 10); }
        }
        if (nworn == 2 && g->p.ncand < GS_MAXCAND) {
            dm_gear_cand *x = &g->cand[g->p.ncand]; *x = g->cand[worn[0]]; x->worn = 0; x->item.slot = 0; x->item.id += 900; better = g->p.ncand++;
            x->item.own.n = 0; dm_gear_list_add(&x->item.own, 0, 0, 40);
            if (!gs_capture(g)) {
                dm_gear_goal goal = { 15, 0.5 }; dm_gear_result r;
                int st = best_of(g, &goal, &r), a = r.best.at[DM_GEAR_P_RING1], b = r.best.at[DM_GEAR_P_RING2];
                CHECK(st == DM_GEAR_S_DONE && (a == better || b == better) && (a == worn[0] || a == worn[1] || b == worn[0] || b == worn[1]),
                      "rings: %d / %d (better %d, worn %d %d)", a, b, better, worn[0], worn[1]);
                check_stock(g, "rings that change nothing", 0, 0);
                n++;
            } else CHECK(0, "rings: no capture");
        } else CHECK(0, "rings: %d worn", nworn);
    } else CHECK(0, "rings: stock not built");

    static const char *const bars[] = { "" }, *const names[] = { "the normal attack" };
    for (int k = 0; k < 1; k++) {
        if (tie_stock(g, bars[k], 7460 + (uint64_t)k, 0)) { CHECK(0, "%s: stock not built", names[k]); continue; }
        int w[8], nw = weapon_cands(g, w, 8), left = -1, fresh = -1;
        for (int q = 0; q < nw; q++) { if (g->cand[w[q]].item.slot == 5) left = w[q]; else if (!g->cand[w[q]].worn) fresh = w[q]; }
        dm_gear_pick alone; dm_gear_pick_now(&g->p, &alone); alone.at[DM_GEAR_P_MAIN] = (int16_t)fresh; alone.at[DM_GEAR_P_OFF] = -1;
        tie_runs(g, names[k], fresh, left, &alone, 0);
        check_stock(g, names[k], 0, 0);
        n++;
    }
    printf("ties: %d stocks where the worn gear that changes nothing stays and the left weapon that lowers the damage comes off\n", n);
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



static void test_run_limit(void) {
    static gstock g;
    gspec s; memset(&s, 0, sizeof s);
    s.profile = g_profile[1]; s.act = 3; s.seed = 13107; s.variants = 3;
    s.count[DM_GEAR_K_ARMOR] = 4;
    if (gs_build(&g, g_d, &g_gt, &s)) { CHECK(0, "run limit stock not built"); return; }
    dm_gear_goal goal = { 15, 0.5 };
    enum { WAIT_US = 2000, LIMIT = 50 };
    dm_gear_search *x = dm_gear_search_new(&g.p);
    if (!x) { CHECK(0, "run limit: no search context"); return; }
    dm_gear_progress pr; memset(&pr, 0, sizeof pr);
    double t0 = now_s();
    dm_gear_result r0; int st0 = dm_gear_search_run(x, &goal, NULL, &pr, &r0);
    double fast = now_s() - t0;
    long runs = atomic_load(&pr.runs);
    CHECK(st0 == DM_GEAR_S_DONE && runs > 20 * LIMIT, "run limit: status %d after %ld runs - the stock lost its shape", st0, runs);
    dm_gear_search_free(x);
    x = dm_gear_search_new(&g.p);
    if (!x) { CHECK(0, "run limit: no search context"); return; }
    dm_gear_search_test_run_us(x, WAIT_US, LIMIT);
    for (int round = 0; round < 2; round++) {
        memset(&pr, 0, sizeof pr);
        t0 = now_s();
        dm_gear_result r; int st = dm_gear_search_run(x, &goal, NULL, &pr, &r);
        double slow = now_s() - t0;
        CHECK(st == DM_GEAR_S_DONE && atomic_load(&pr.runs) == runs && !dm_gear_key_cmp(r.score.key, r0.score.key) && !memcmp(&r.best, &r0.best, sizeof r.best),
              "run limit round %d: status %d, %ld runs (%ld at full speed) or another result", round, st, atomic_load(&pr.runs), runs);

        CHECK(slow >= LIMIT * WAIT_US / 1e6 && slow < fast + 5 * LIMIT * WAIT_US / 1e6,
              "run limit round %d: %.3f s against %.3f s at full speed, %ld runs", round, slow, fast, runs);
    }
    dm_gear_search_free(x);
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
    dual_cases(&g);
    sword_cases(&g);
    claw_cases(&g);
    passive_hand_cases(&g);
    hand_cases(&g);
    wblock_cases(&g);
    tie_cases(&g);
    mana_stocks(&g);
    double ta = now_s();
    adversarial_cases(&g);
    printf("adversarial cases: %.1f s\n", now_s() - ta);
    splash_switch();
    splash_groups();
    defense_stays();
    worn_stays_on_ties();
    handout_share();
    test_run_limit();
    printf("gear search: %ld stocks (%ld random, %ld of them again with the charms fixed, %.1f s) x 9 goals, %ld combinations enumerated (%.1f s), search %.1f s: %ld runs, %ld leaves, "
           "%ld cut; %ld unrated candidates, %ld stocks with weapon pairs, %ld stopped runs, %ld warm runs that took the start value; %s\n",
           g_stocks, random_stocks, g_fixed, t_random, g_combos, g_t_brute, g_t_search, g_runs, g_leaves, g_pruned, g_unrated, g_dual, g_aborts,
           g_seeded, atomic_load(&failures) ? "FAILED" : "passed");
    return atomic_load(&failures) != 0;
}
