#define _GNU_SOURCE
#include "dm_gear_tab.h"
#include "dm_gear_dump.h"
#include "dm_gear_live.h"
#include "dm_sim_ui.h"
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

dm_gear_snap *dm_gear_snap_new(int ncand, int nweapon) {
    if (ncand < 0 || ncand > DM_GEAR_MAXCAND || nweapon < 1) return NULL;
    dm_gear_snap *s = calloc(1, sizeof *s);
    if (!s) return NULL;
    s->cand = calloc((size_t)ncand + 1, sizeof *s->cand); s->weapon = calloc((size_t)nweapon, sizeof *s->weapon); s->show = calloc((size_t)ncand + 1, sizeof *s->show);
    if (!s->cand || !s->weapon || !s->show) { dm_gear_snap_free(s); return NULL; }
    s->p.cand = s->cand; s->p.weapon = s->weapon;
    return s;
}
void dm_gear_snap_free(dm_gear_snap *s) {
    if (!s) return;
    if (s->tables) dm_sim_unpin(s->tables);
    free(s->cand); free(s->weapon); free(s->show); free(s);
}
static void free_list(dm_gear_snap *s) { while (s) { dm_gear_snap *n = s->next; dm_gear_snap_free(s); s = n; } }

__attribute__((weak)) void dm_gear_tab_log(const char *line) { fputs(line, stderr); }

#ifndef DM_PLAYER_BUILD
__attribute__((weak)) const char *dm_android_runtime_dir(void);

static const char *dump_dir(void) {
    const char *e = getenv("DM_GEAR_DUMP_DIR");
    if (e && *e) return e;
    return dm_android_runtime_dir ? dm_android_runtime_dir() : NULL;
}
#endif

static double now_ms(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec * 1e3 + t.tv_nsec / 1e6; }
enum { ST_NONE, ST_SEARCHING, ST_DONE, ST_STOPPED, ST_FAILED };


#define SHOW_EVERY_MS 3000

static pthread_mutex_t mu = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cv = PTHREAD_COND_INITIALIZER;
static atomic_int want, stop, charms_search = 1;
static int started, status, kick, cur_state;


static dm_gear_snap *cur, *incoming, *pending, *trash;
static dm_gear_progress prog;
static dm_gear_result shown;
static int have;
static dm_gear_result disp; static uint64_t disp_rev; static double disp_at; static int disp_have;
static long runs_before, starts;

static void unref_locked(dm_gear_snap *s, dm_gear_snap **drop) { if (s && --s->refs == 0) { s->next = *drop; *drop = s; } }

static int same_measures(const dm_gear_measures *a, const dm_gear_measures *b) {
    return a->group == b->group && a->single == b->single && a->survival_s == b->survival_s && a->hits == b->hits && a->recovery == b->recovery;
}

static void find_idle(dm_gear_snap *s) {
    if (s->idle_done) return;
    s->idle_done = 1; memset(&s->idle, 0, sizeof s->idle);
    if (s->p.charms_fixed) return;
    dm_gear_measures with, without; unsigned flags;
    if (dm_gear_leaf_measure(&s->p, &s->now, &with, &flags)) return;
    static dm_gear_pick t;
    for (int i = 0; i < s->p.ncand; i++) {
        if (s->cand[i].kind != DM_GEAR_K_CHARM || !s->now.charm[i]) continue;
        t = s->now; t.charm[i] = 0;
        if (!dm_gear_leaf_measure(&s->p, &t, &without, &flags) && same_measures(&with, &without)) s->idle.charm[i] = 1;
    }
}





static void show_result(const dm_gear_result *r, const dm_gear_snap *s, int final) {
    static dm_gear_pick best, with; best = r->best;
    for (int i = 0; s && s->idle_done && i < s->p.ncand; i++) {
        if (!s->idle.charm[i] || best.charm[i]) continue;
        with = best; with.charm[i] = 1;
        dm_gear_measures m; unsigned flags;
        if (dm_gear_place(&s->p, with.charm) && !dm_gear_leaf_measure(&s->p, &with, &m, &flags) && same_measures(&m, &r->after)) best = with;
    }
    pthread_mutex_lock(&mu);
    if (final || !have || dm_gear_key_cmp(r->score.key, shown.score.key) >= 0) {
        shown.best = best; shown.now = r->now; shown.after = r->after; shown.score = r->score;
        shown.unrated = r->unrated; have = 1;
    }
    pthread_mutex_unlock(&mu);
}
static void better(const dm_gear_result *r, void *ctx) { show_result(r, ctx, 0); }


#define KEPT_N 8
static struct { uint64_t fp; dm_gear_result r; } kept[KEPT_N];
static int kept_find(uint64_t fp) { for (int i = 0; fp && i < KEPT_N; i++) if (kept[i].fp == fp) return i; return -1; }
static void kept_put(uint64_t fp, const dm_gear_result *r) {
    int i = kept_find(fp); if (i < 0) i = KEPT_N - 1;
    memmove(&kept[1], &kept[0], sizeof *kept * (size_t)i);
    kept[0].fp = fp; kept[0].r = *r;
}


static int map_pick(const dm_gear_snap *o, const dm_gear_pick *pk, const dm_gear_snap *s, dm_gear_pick *out) {
    int n = s->p.ncand, ok = 1;
    uint64_t *hs = malloc(sizeof *hs * ((size_t)n + 1)); uint8_t *used = calloc((size_t)n + 1, 1);
    if (!hs || !used) { free(hs); free(used); return 0; }
    for (int j = 0; j < n; j++) hs[j] = dm_gear_cand_hash(&s->p, j);
    memset(out, 0, sizeof *out);
    for (int k = 0; k < DM_GEAR_P_N + o->p.ncand && ok; k++) {
        int i = k < DM_GEAR_P_N ? pk->at[k] : pk->charm[k - DM_GEAR_P_N] ? k - DM_GEAR_P_N : -1;
        if (k < DM_GEAR_P_N) out->at[k] = -1;
        if (i < 0 || i >= o->p.ncand) continue;
        uint64_t h = dm_gear_cand_hash(&o->p, i); int at = -1;
        for (int j = 0; j < n && at < 0; j++) if (!used[j] && hs[j] == h && s->show[j].id == o->show[i].id) at = j;
        for (int j = 0; j < n && at < 0; j++) if (!used[j] && hs[j] == h) at = j;
        if (at < 0) { ok = 0; break; }
        used[at] = 1;
        if (k < DM_GEAR_P_N) out->at[k] = (int16_t)at; else out->charm[at] = 1;
    }
    free(hs); free(used);
    return ok;
}

static void prepare_show(dm_gear_snap *s) { dm_gear_pick_now(&s->p, &s->now); }


static void pace(int wide) { setpriority(PRIO_PROCESS, (id_t)syscall(SYS_gettid), wide ? 0 : 19); }
static dm_gear_search *search_of(dm_gear_snap *s, const dm_gear_pick *seed) {
    if (s->state != DM_GTAB_OK || !s->tables) return NULL;
    s->p.d = s->tables;
    if (dm_gear_problem_ready(&s->p)) return NULL;
    long cores = sysconf(_SC_NPROCESSORS_ONLN);
    int threads = cores > 1 ? (int)(cores - 1) : 1;
#ifndef DM_PLAYER_BUILD

    const char *dd = dump_dir();
    if (dd) dm_gdump_write(dd, s->rev, &s->p, &s->goal, threads, seed);
#endif
    dm_gear_search *x = dm_gear_search_new(&s->p);
    dm_gear_search_wide(x, threads, &want, pace);
#ifndef DM_PLAYER_BUILD


    { const char *e = getenv("DM_TEST_GEAR_RUN_US"); if (e) { const char *c = strchr(e, ':'); dm_gear_search_test_run_us(x, atol(e), c ? atol(c + 1) : 0); } }
#endif
    return x;
}
static void *run(void *unused) {
    (void)unused;
    setpriority(PRIO_PROCESS, (id_t)syscall(SYS_gettid), 19);
    pthread_setname_np(pthread_self(), "dm-gear-search");
    dm_gear_search *x = NULL;
    static dm_gear_result res;
    pthread_mutex_lock(&mu);
    for (;;) {
        int go;
        while (!(go = atomic_load(&want) && (pending || (cur && (status == ST_STOPPED || (status == ST_FAILED && kick && cur_state == DM_GTAB_OK))))) && !trash)
            pthread_cond_wait(&cv, &mu);
        dm_gear_snap *drop = trash; trash = NULL;
        if (!go) { pthread_mutex_unlock(&mu); free_list(drop); pthread_mutex_lock(&mu); continue; }
        dm_gear_snap *fresh = pending; pending = NULL; incoming = fresh;
        kick = 0; status = ST_SEARCHING; atomic_store(&stop, 0); starts++;
        runs_before += atomic_load(&prog.runs);
        atomic_store(&prog.runs, 0); atomic_store(&prog.fresh, 0); atomic_store(&prog.bounds, 0); atomic_store(&prog.pruned, 0);
        atomic_store(&prog.leaves, 0); atomic_store(&prog.estimate, 0); atomic_store(&prog.found, 0); atomic_store(&prog.share, 0);
        prog.better = better; prog.ctx = NULL;
        pthread_mutex_unlock(&mu);
        free_list(drop); drop = NULL;
        if (fresh) {
            dm_gear_search_free(x); x = NULL;
            prepare_show(fresh);
            fresh->fp = 0;
            if (fresh->state == DM_GTAB_OK && fresh->tables) { fresh->p.d = fresh->tables; fresh->fp = dm_gear_problem_hash(&fresh->p, &fresh->goal); }
            if (kept_find(fresh->fp) >= 0) { if (dm_gear_problem_ready(&fresh->p)) fresh->fp = 0; }
            if (kept_find(fresh->fp) < 0) {


                static dm_gear_pick was, seed;
                pthread_mutex_lock(&mu); int had = have && cur && cur->state == DM_GTAB_OK; if (had) was = shown.best; pthread_mutex_unlock(&mu);
                int seed_on = had && map_pick(cur, &was, fresh, &seed);
                x = search_of(fresh, seed_on ? &seed : NULL);
                if (x && seed_on) dm_gear_search_seed(x, &seed);
            }
            pthread_mutex_lock(&mu);
            unref_locked(cur, &drop); cur = fresh; incoming = NULL; cur->refs = 1;
            cur_state = cur->state == DM_GTAB_OK && !cur->tables ? DM_GTAB_NOSIM : cur->state;
            have = 0; memset(&shown, 0, sizeof shown);
            pthread_mutex_unlock(&mu);
            free_list(drop); drop = NULL;
        } else if (!x) x = search_of(cur, NULL);
        dm_gear_snap *s = cur;
        int rc = DM_GEAR_S_INPUT;
        memset(&res, 0, sizeof res);
        double t0 = now_ms();
        int k = kept_find(s->fp);
        if (k >= 0) {
            pthread_mutex_lock(&mu); shown = kept[k].r; res = shown; have = 1; kept_put(s->fp, &shown); pthread_mutex_unlock(&mu);
            atomic_store(&prog.found, 1); rc = DM_GEAR_S_DONE;
            char kl[96]; snprintf(kl, sizeof kl, "[dm] best gear: kept result %016llx\n", (unsigned long long)s->fp); dm_gear_tab_log(kl);
        } else if (x) {
            find_idle(s); prog.ctx = s; rc = dm_gear_search_run(x, &s->goal, &stop, &prog, &res);
            if (res.seeded) dm_gear_tab_log("[dm] best gear: started from the result before\n");
            if (rc == DM_GEAR_S_DONE) show_result(&res, s, 1);
            if (rc == DM_GEAR_S_DONE && s->fp) { pthread_mutex_lock(&mu); if (have) kept_put(s->fp, &shown); pthread_mutex_unlock(&mu); }
        }

        char line[512];
        snprintf(line, sizeof line, "[dm] best gear: search %016llx ended %d after %ld runs, %.0f ms; now %.4f/%.4f/%.4fs/%.4f hits/%.4f, best %.4f/%.4f/%.4fs/%.4f hits/%.4f, "
                 "picks %d %d %d %d %d %d %d %d %d %d\n", (unsigned long long)s->rev, rc, atomic_load(&prog.runs), now_ms() - t0, res.now.group, res.now.single,
                 res.now.survival_s, res.now.hits, res.now.recovery, res.after.group, res.after.single, res.after.survival_s, res.after.hits, res.after.recovery,
                 res.best.at[0], res.best.at[1], res.best.at[2], res.best.at[3], res.best.at[4], res.best.at[5], res.best.at[6], res.best.at[7], res.best.at[8], res.best.at[9]);
        dm_gear_tab_log(line);

        if (res.tripwire) {
            char al[128]; snprintf(al, sizeof al, "ASSUME bestgear.bound: %ld leaves above the bound of a node they lie under\n", res.tripwire);
            dm_gear_tab_log(al);
        }
#ifndef DM_PLAYER_BUILD
        { const char *dd = dump_dir(); if (dd) dm_gdump_result(dd, s->rev, line); }
#endif
#ifndef DM_PLAYER_BUILD

        { static long hold = -1; if (hold < 0) { const char *e = getenv("DM_TEST_GEAR_HOLD_MS"); hold = e ? atol(e) : 0; }
          for (long w = 0; rc == DM_GEAR_S_DONE && w < hold && !atomic_load(&stop); w += 20) usleep(20000); }
#endif
        pthread_mutex_lock(&mu);
        if (cur_state == DM_GTAB_OK && rc == DM_GEAR_S_NOREF) cur_state = DM_GTAB_NOREF;
        status = rc == DM_GEAR_S_DONE ? ST_DONE : rc == DM_GEAR_S_ABORTED ? ST_STOPPED : ST_FAILED;
    }
    return NULL;
}
static int start_locked(void) {
    if (started) return 0;
    pthread_t th;
    if (pthread_create(&th, NULL, run, NULL)) return -1;
    pthread_detach(th); started = 1;
    return 0;
}
void dm_gear_tab_want(int on) {
    pthread_mutex_lock(&mu);
    int was = atomic_exchange(&want, on != 0);
    if (!on) atomic_store(&stop, 1);
    else {
        if (!was) kick = 1;
        if (!start_locked()) pthread_cond_signal(&cv);
    }
    pthread_mutex_unlock(&mu);
}
int dm_gear_tab_wanted(void) { return atomic_load(&want); }
int dm_gear_tab_offer(dm_gear_snap *s) {
    if (!s) return 0;
    if (pthread_mutex_trylock(&mu)) return 1;
    s->refs = 0; s->next = NULL;
    const dm_gear_snap *last = pending ? pending : incoming ? incoming : cur;
    if (last && last->rev == s->rev) { s->next = trash; trash = s; }
    else {
        if (pending) { pending->next = trash; trash = pending; }
        pending = s;
        atomic_store(&stop, 1);
    }
    if (!start_locked()) pthread_cond_signal(&cv);
    pthread_mutex_unlock(&mu);
    return 0;
}
long dm_gear_tab_runs(void) { pthread_mutex_lock(&mu); long r = runs_before + atomic_load(&prog.runs); pthread_mutex_unlock(&mu); return r; }
long dm_gear_tab_starts(void) { pthread_mutex_lock(&mu); long r = starts; pthread_mutex_unlock(&mu); return r; }
int dm_gear_tab_best(dm_gear_result *out) {
    pthread_mutex_lock(&mu); int h = have && cur && !pending && !incoming; if (h) *out = shown; pthread_mutex_unlock(&mu);
    return h;
}

int dm_gear_tab_ring_rows(const int best[2], const int now[2], int place[2], int cand[2]) {
    int keep[2], fresh[2], nfresh = 0, n = 0;
    for (int j = 0; j < 2; j++) keep[j] = now[j] >= 0 && (now[j] == best[0] || now[j] == best[1]);
    for (int j = 0; j < 2; j++) if (best[j] >= 0 && best[j] != now[0] && best[j] != now[1]) fresh[nfresh++] = best[j];

    int open[2], nopen = 0;
    for (int j = 0; j < 2; j++) if (now[j] >= 0 && !keep[j]) open[nopen++] = j;
    for (int j = 0; j < 2; j++) if (now[j] < 0) open[nopen++] = j;
    int used = 0;
    for (int f = 0; f < nfresh && used < nopen; f++) { place[n] = open[used++]; cand[n++] = fresh[f]; }
    for (int u = used; u < nopen; u++) if (now[open[u]] >= 0) { place[n] = open[u]; cand[n++] = -1; }
    return n;
}


int dm_gear_tab_rows(const dm_gear_problem *p, const dm_gear_pick *now, const dm_gear_pick *best, dm_gtab_row *rows, int max) {
    if (max > DM_GTAB_ROWS) max = DM_GTAB_ROWS;
    int n = 0, hands = -1;
    static const int order[] = { DM_GEAR_P_MAIN, DM_GEAR_P_OFF, DM_GEAR_P_HEAD, DM_GEAR_P_ARMOR, DM_GEAR_P_AMULET, DM_GEAR_P_GLOVES, DM_GEAR_P_BELT, DM_GEAR_P_BOOTS };
    for (size_t i = 0; i < sizeof order / sizeof *order; i++) {
        int k = order[i];
        if (best->at[k] == now->at[k]) continue;
        int unit = n;
        if (k == DM_GEAR_P_MAIN || k == DM_GEAR_P_OFF) { if (hands < 0) hands = n; unit = hands; }
        if (n < max) rows[n] = (dm_gtab_row){ k, best->at[k], 0, unit, 0, { { 0 } } };
        n++;
    }
    int b[2] = { best->at[DM_GEAR_P_RING1], best->at[DM_GEAR_P_RING2] }, w[2] = { now->at[DM_GEAR_P_RING1], now->at[DM_GEAR_P_RING2] }, place[2], cand[2];
    int nr = dm_gear_tab_ring_rows(b, w, place, cand);
    for (int j = 0; j < nr; j++, n++) if (n < max) rows[n] = (dm_gtab_row){ place[j] ? DM_GEAR_P_RING2 : DM_GEAR_P_RING1, cand[j], 0, n, 0, { { 0 } } };
    for (int i = 0; i < p->ncand; i++)
        if (p->cand[i].kind == DM_GEAR_K_CHARM && !best->charm[i] != !now->charm[i]) { if (n < max) rows[n] = (dm_gtab_row){ -1, i, best->charm[i] ? 1 : 2, n, 0, { { 0 } } }; n++; }
    return n;
}
static dm_gtab_units set_one(int u) { dm_gtab_units s = { { 0 } }; s.w[u >> 6] = 1ull << (u & 63); return s; }
static void set_add(dm_gtab_units *s, int u) { s->w[u >> 6] |= 1ull << (u & 63); }
static void set_cut(dm_gtab_units *s, int u) { s->w[u >> 6] &= ~(1ull << (u & 63)); }
static void set_join(dm_gtab_units *s, const dm_gtab_units *t) { for (size_t k = 0; k < sizeof s->w / sizeof *s->w; k++) s->w[k] |= t->w[k]; }

static int set_meet(const dm_gtab_units *s, const dm_gtab_units *t) { for (size_t k = 0; k < sizeof s->w / sizeof *s->w; k++) if (s->w[k] & t->w[k]) return 1; return 0; }

static int set_within(const dm_gtab_units *s, const dm_gtab_units *t) { for (size_t k = 0; k < sizeof s->w / sizeof *s->w; k++) if (s->w[k] & ~t->w[k]) return 0; return 1; }

static void apply(dm_gear_pick *pk, const dm_gtab_row *rows, int n, const dm_gtab_units *mask) {
    for (int i = 0; i < n; i++) {
        if (!dm_gtab_has(mask, rows[i].unit)) continue;
        if (rows[i].place >= 0) pk->at[rows[i].place] = (int16_t)rows[i].cand;
        else pk->charm[rows[i].cand] = rows[i].charm == 1;
    }
}
static void apply_one(dm_gear_pick *pk, const dm_gtab_row *rows, int n, int u) { dm_gtab_units s = set_one(u); apply(pk, rows, n, &s); }

static int unit_active(const dm_gear_problem *p, const dm_gear_pick *pk, const dm_gtab_row *rows, int n, int u, dm_gear_char *comp) {
    uint8_t act[DM_GEAR_P_N + 1];
    if (dm_gear_pick_compose(p, pk, comp, act)) return 0;
    for (int i = 0; i < n; i++) {
        if (rows[i].unit != u) continue;
        if (rows[i].place >= 0 && rows[i].cand >= 0 && !act[rows[i].place]) return 0;
        if (rows[i].place < 0 && rows[i].charm == 1 && !act[DM_GEAR_P_N]) return 0;
    }
    return 1;
}
int dm_gear_tab_order(const dm_gear_problem *p, const dm_gear_pick *now, dm_gtab_row *rows, int n) {
    if (!p || !p->ready || n <= 0 || n > DM_GTAB_ROWS) return n == 0 ? 0 : -1;
    static const dm_gtab_units none;
    for (int i = 0; i < n; i++) { rows[i].needs = none; rows[i].first = 0; if (rows[i].unit < 0 || rows[i].unit >= n) return -1; }
    static _Thread_local dm_gear_pick st, t;
    static _Thread_local dm_gear_char comp;
    int unit[DM_GTAB_ROWS], nu = 0;
    dm_gtab_units all = none;
    for (int i = 0; i < n; i++) if (!dm_gtab_has(&all, rows[i].unit)) { set_add(&all, rows[i].unit); unit[nu++] = rows[i].unit; }

    t = *now; apply(&t, rows, n, &all);
    for (int j = 0; j < nu; j++) if (!unit_active(p, &t, rows, n, unit[j], &comp)) return -1;

    uint8_t act[DM_GEAR_P_N + 1];
    if (dm_gear_pick_compose(p, now, &comp, act)) return -1;
    int str0 = comp.value[0], dex0 = comp.value[2], lowers[DM_GTAB_ROWS] = { 0 };
    for (int j = 0; j < nu; j++) {
        t = *now; apply_one(&t, rows, n, unit[j]);
        if (dm_gear_pick_compose(p, &t, &comp, act)) return -1;
        lowers[j] = comp.value[0] < str0 || comp.value[2] < dex0;
    }


    int seq[DM_GTAB_ROWS]; dm_gtab_units done = none;
    st = *now;
    for (int step = 0; step < nu; step++) {
        int pick = -1;
        for (int pass = 0; pass < 2 && pick < 0; pass++)
            for (int j = 0; j < nu && pick < 0; j++) {
                if (dm_gtab_has(&done, unit[j]) || lowers[j] != pass) continue;
                t = st; apply_one(&t, rows, n, unit[j]);
                if (unit_active(p, &t, rows, n, unit[j], &comp)) pick = j;
            }
        if (pick < 0) return -1;
        seq[step] = pick; set_add(&done, unit[pick]); apply_one(&st, rows, n, unit[pick]);
    }

    dm_gtab_units needs[DM_GTAB_ROWS], before = none, needed = none;
    for (int step = 0; step < nu; step++) {
        int j = seq[step], u = unit[j];
        t = *now; apply_one(&t, rows, n, u);
        dm_gtab_units S = none;
        if (!unit_active(p, &t, rows, n, u, &comp)) {
            S = before;
            for (int k = step - 1; k >= 0; k--) {
                if (!dm_gtab_has(&S, unit[seq[k]])) continue;
                dm_gtab_units T = S; set_cut(&T, unit[seq[k]]); set_add(&T, u);
                t = *now; apply(&t, rows, n, &T);
                if (unit_active(p, &t, rows, n, u, &comp)) set_cut(&S, unit[seq[k]]);
            }
        }
        needs[j] = S; set_join(&needed, &S); set_add(&before, u);
    }


    int order[DM_GTAB_ROWS]; dm_gtab_units out = none;
    for (int step = 0; step < nu; step++) {
        int pick = -1, rank = 9;
        for (int j = 0; j < nu; j++) {
            if (dm_gtab_has(&out, unit[j]) || !set_within(&needs[j], &out)) continue;
            int r = (dm_gtab_has(&needed, unit[j]) ? 0 : 2) + lowers[j];
            if (r < rank) { rank = r; pick = j; }
        }
        if (pick < 0) break;
        order[step] = pick; set_add(&out, unit[pick]);
    }
    int works = set_within(&all, &out);
    st = *now;
    for (int step = 0; works && step < nu; step++) {
        int u = unit[order[step]];
        t = st; apply_one(&t, rows, n, u);
        works = unit_active(p, &t, rows, n, u, &comp); st = t;
    }
    if (!works) memcpy(order, seq, sizeof order);
    dm_gtab_row in[DM_GTAB_ROWS]; memcpy(in, rows, sizeof *rows * (size_t)n);
    int w = 0;
    for (int step = 0; step < nu; step++) {
        int j = order[step];
        for (int i = 0; i < n; i++) if (in[i].unit == unit[j]) { rows[w] = in[i]; rows[w].needs = needs[j]; rows[w].first = dm_gtab_has(&needed, unit[j]); w++; }
    }
    return 0;
}


static int hands_fit(const dm_gear_problem *p, const dm_gear_pick *pk) {
    int m = pk->at[DM_GEAR_P_MAIN], o = pk->at[DM_GEAR_P_OFF];
    if (m >= 0 && m == o) return 0;
    if (m >= 0 && o >= 0 && p->cand[m].weapon >= 0 && p->cand[o].weapon >= 0) return dm_gear_pair_weapon(p, m) && dm_gear_pair_weapon(p, o);
    int mk = m >= 0 ? p->cand[m].kind : DM_GEAR_K_NONE, ok = o >= 0 ? p->cand[o].kind : DM_GEAR_K_NONE;
    if (mk == DM_GEAR_K_WEAPON2 && !dm_gear_one_hand(p, m)) return o < 0;
    if (ok == DM_GEAR_K_WEAPON2 && !dm_gear_one_hand(p, o)) return m < 0;
    if (mk == DM_GEAR_K_BOW) return o < 0 || ok == DM_GEAR_K_ARROWS;
    if (mk == DM_GEAR_K_XBOW) return o < 0 || ok == DM_GEAR_K_BOLTS;
    return ok != DM_GEAR_K_ARROWS && ok != DM_GEAR_K_BOLTS;
}
int dm_gear_tab_pkg_picks(const dm_gear_problem *p, const dm_gear_pick *now, const dm_gtab_row *rows, int n, int i, dm_gear_pick *before, dm_gear_pick *after) {
    if (!p || !now || !rows || i < 0 || i >= n || n > DM_GTAB_ROWS || rows[i].place < 0) return -1;
    for (int k = 0; k < n; k++) if (rows[k].unit < 0 || rows[k].unit >= n) return -1;
    int u = rows[i].unit;

    dm_gtab_units dep = set_one(u), rest = { { 0 } };
    for (int changed = 1; changed;) {
        changed = 0;
        for (int k = 0; k < n; k++) if (!dm_gtab_has(&dep, rows[k].unit) && set_meet(&rows[k].needs, &dep)) { set_add(&dep, rows[k].unit); changed = 1; }
    }
    for (int k = 0; k < n; k++) if (!dm_gtab_has(&dep, rows[k].unit)) set_add(&rest, rows[k].unit);
    set_add(&rest, u);
    *after = *now; apply(after, rows, n, &rest);


    *before = *after; before->at[rows[i].place] = now->at[rows[i].place];
    for (int k = 0; k < n; k++) if (k != i && rows[k].unit == u && rows[k].place >= 0 && rows[k].cand < 0) before->at[rows[k].place] = now->at[rows[k].place];
    if (!hands_fit(p, before)) for (int k = 0; k < n; k++) if (rows[k].unit == u && rows[k].place >= 0) before->at[rows[k].place] = now->at[rows[k].place];
    return 0;
}


typedef struct { char *o; size_t cap, n; int over; } sb;
static void put(sb *b, const char *fmt, ...) __attribute__((format(printf, 2, 3)));

static void put(sb *b, const char *fmt, ...) {
    if (b->over) return;
    va_list ap; va_start(ap, fmt);
    int k = vsnprintf(b->o + b->n, b->cap - b->n, fmt, ap);
    va_end(ap);
    if (k < 0 || (size_t)k >= b->cap - b->n) { b->over = 1; return; }
    b->n += (size_t)k;
}
static void str(sb *b, const char *s) {
    put(b, "\"");
    for (const unsigned char *p = (const unsigned char *)s; *p; p++) {
        if (*p == '"' || *p == '\\') put(b, "\\%c", *p);
        else if (*p < 0x20) put(b, "\\u%04x", *p);
        else put(b, "%c", *p);
    }
    put(b, "\"");
}
static const char *state_name(int st) {
    static const char *const n[] = { "ok", "nochar", "set2", "nosim", "noref" };
    return st >= 0 && st <= DM_GTAB_NOREF ? n[st] : "nosim";
}


static void row(sb *b, const dm_gear_snap *s, int place, int c, int charm, int supplier, int *first) {
    put(b, "%s{\"slot\":", *first ? "" : ","); *first = 0;
    str(b, place >= 0 ? s->slot[place] : "");
    if (charm) put(b, ",\"charm\":%d", charm);
    if (supplier) put(b, ",\"first\":1");
    if (c >= 0) {
        const dm_gtab_show *w = &s->show[c];
        char colour[2] = { w->colour, 0 };
        put(b, ",\"name\":"); str(b, w->name); put(b, ",\"c\":"); str(b, colour); put(b, ",\"code\":"); str(b, w->code);
        put(b, ",\"q\":%d,\"where\":%d,\"page\":%d,\"row\":%d,\"gfx\":%d,\"id\":%u", w->quality, w->where, w->page, w->row, w->gfx, w->id);
    } else put(b, ",\"name\":\"\",\"where\":-1");
    put(b, ",\"p\":%d}", place);
}

void dm_gear_grid(uint8_t grid[DM_GEAR_GRID_H][DM_GEAR_GRID_W], const dm_gear_inv_cells *it, int n) {
    memset(grid, 0, sizeof(uint8_t) * DM_GEAR_GRID_H * DM_GEAR_GRID_W);
    for (int i = 0; i < n; i++) {
        const dm_gear_inv_cells *e = &it[i];
        if (e->charm && e->placed) continue;
        for (int y = e->y; y >= 0 && y < e->y + e->h && y < DM_GEAR_GRID_H; y++) for (int x = e->x; x >= 0 && x < e->x + e->w && x < DM_GEAR_GRID_W; x++) grid[y][x] = 1;
    }
}


static pthread_mutex_t tip_mu = PTHREAD_MUTEX_INITIALIZER;
static atomic_ullong tip_want;
static uint32_t tip_id;
static int tip_place;
static int tip_dots;
static char *tip_text, *tip_block;
void dm_gear_tab_charms(int search) { atomic_store(&charms_search, search ? 1 : 0); }
int dm_gear_tab_charms_wanted(void) { return atomic_load(&charms_search); }
static unsigned long long tip_key(uint32_t id, int place) { return id ? (unsigned long long)(uint32_t)(place < 0 ? 0 : place + 1) << 32 | id : 0; }
void dm_gear_tab_tip_ask(uint32_t id, int place) { atomic_store(&tip_want, tip_key(id, place)); }
uint32_t dm_gear_tab_tip_wanted(int *place) {
    unsigned long long k = atomic_load(&want) ? atomic_load(&tip_want) : 0;
    *place = (int)(k >> 32) - 1;
    return (uint32_t)k;
}
static char *copy_of(const char *s) { size_t n = s ? strlen(s) : 0; char *c = malloc(n + 1); if (c) memcpy(c, s ? s : "", n + 1); return c; }
int dm_gear_tab_tip_put(uint32_t id, int place, const char *text, const char *block, int dots) {
    if (pthread_mutex_trylock(&tip_mu)) return 1;
    char *t = copy_of(text), *k = copy_of(block);
    if (t && k) { free(tip_text); free(tip_block); tip_text = t; tip_block = k; tip_id = id; tip_place = place < 0 ? -1 : place; tip_dots = dots; t = k = NULL; }
    pthread_mutex_unlock(&tip_mu);
    free(t); free(k);
    return 0;
}
int dm_gear_tab_tip_hash(uint32_t id, uint64_t *vhash) {
    int found = 0;
    pthread_mutex_lock(&mu);
    for (int i = 0; cur && i < cur->p.ncand && !found; i++) if (cur->show[i].id == id && cur->show[i].vhash) { *vhash = cur->show[i].vhash; found = 1; }
    pthread_mutex_unlock(&mu);
    return found;
}


static pthread_mutex_t text_mu = PTHREAD_MUTEX_INITIALIZER, pkg_mu = PTHREAD_MUTEX_INITIALIZER;
static struct { uint64_t rev, best; int n, nhands, hands[DM_GTAB_HANDS][2]; dm_gtab_row rows[DM_GTAB_ROWS]; uint64_t pkg_key; } order_cache;
static dm_gtab_pkg pkg_shown;
static int pkg_have;
static unsigned pkg_gen;
static uint64_t pick_hash(const dm_gear_pick *pk, int ncand) {
    uint64_t h = 1469598103934665603ull;
    for (int k = 0; k < DM_GEAR_P_N; k++) h = (h ^ (uint16_t)pk->at[k]) * 1099511628211ull;
    for (int i = 0; i < ncand; i++) if (pk->charm[i]) h = (h ^ (uint32_t)(i + 1) << 8) * 1099511628211ull;
    return h;
}
static void pkg_publish(const dm_gtab_pkg *k) {
    pthread_mutex_lock(&pkg_mu); pkg_shown = *k; pkg_shown.gen = ++pkg_gen; pkg_have = 1; pthread_mutex_unlock(&pkg_mu);
}
int dm_gear_tab_tip_pkg(uint32_t id, int place, dm_gtab_pkg *out) {
    if (pthread_mutex_trylock(&pkg_mu)) return -1;
    int r = pkg_have && pkg_shown.id == id && pkg_shown.place == place;
    if (r) *out = pkg_shown;
    pthread_mutex_unlock(&pkg_mu);
    return r;
}

static void pkg_values(const dm_gear_problem *p, const dm_gear_char *c, int *v) {
    for (int i = 0; i < IC_S_N; i++) {
        int id = dm_ic_stat_id[i];
        v[i] = id == 83 ? dm_gear_list_get(&c->raw, 83, p->now.cls) : id < DM_GEAR_NSTAT ? p->now.stat_value[id] + p->ref_sv[id] + c->value[id] - p->nowc.value[id] : 0;
    }
}


static dm_splash_area pkg_splash(const dm_sim_char *c) {
    const dm_sim_weapon *w = &c->hand[0];
    return dm_splash_area_of(dm_splash_group_of(w->stype[0] ? w->stype : NULL, w->twohand), c->cls, c->str, c->dex, w->srange);
}

static void pkg_make(const dm_gear_snap *s, const dm_gtab_row *rows, int n, int i, dm_gtab_pkg *k) {
    static _Thread_local dm_gear_pick before, after;
    static _Thread_local dm_sim_char cb, ca;
    static _Thread_local dm_gear_char gb, ga;
    const dm_gear_problem *p = &s->p;
    unsigned fb = 0, fa = 0;
    if (dm_gear_tab_pkg_picks(p, &s->now, rows, n, i, &before, &after)) return;
    if (dm_gear_leaf_char_cb(p, &before, &cb, &gb, &fb, k->cb_before) | dm_gear_leaf_char_cb(p, &after, &ca, &ga, &fa, k->cb_after)) {
        char line[160]; snprintf(line, sizeof line, "[dm] best gear: row %#x place %d: no package (flags %#x / %#x)\n", s->show[rows[i].cand].id, rows[i].place, fb, fa);
        dm_gear_tab_log(line);
        return;
    }
    k->ok = 1; k->cb = 1;

    if (s->tables && (dm_sim_two_claws(&cb) || dm_sim_two_claws(&ca))) {
        int before = dm_sim_fight_block(s->tables, &cb), after = dm_sim_fight_block(s->tables, &ca);
        if (before >= 0 && after >= 0) { k->cb_before[IC_C_BLK] = before; k->cb_after[IC_C_BLK] = after; }
    }
    pkg_values(p, &gb, k->before); pkg_values(p, &ga, k->after);
    k->splash_before = pkg_splash(&cb); k->splash_after = pkg_splash(&ca);
    for (int id = 0; id < DM_SIM_MAXSKILL; id++) if (p->node[id]) { k->lvl_before[id] = cb.lvl[id]; k->lvl_after[id] = ca.lvl[id]; }
    int rc = s->tables ? dm_gear_tip_rate(s->tables, &cb, &ca, &s->goal, s->p.splash_on, &k->tip) : -1;
    k->rated = rc == 0 ? DM_GEAR_TIP_DONE : rc == DM_GEAR_RATE_GRANTS ? DM_GEAR_TIP_GRANTS : DM_GEAR_TIP_FAILED;

    size_t w = 0;
    for (int j = 0; j < n; j++) {
        if (!dm_gtab_has(&rows[i].needs, rows[j].unit) || rows[j].cand < 0 || (j > 0 && rows[j - 1].unit == rows[j].unit && rows[j - 1].cand >= 0)) continue;
        int m = snprintf(k->with + w, sizeof k->with - w, "%s%s", w ? " + " : "", s->show[rows[j].cand].name);
        if (m < 0 || (size_t)m >= sizeof k->with - w) break;
        w += (size_t)m;
    }
}
int dm_gear_tab_hands(const dm_gear_problem *p, const dm_gear_pick *pk, int out[][2], int max) {
    static _Thread_local dm_sim_char c;
    unsigned flags;
    if (dm_gear_leaf_char(p, pk, &c, NULL, &flags)) return 0;
    return dm_sim_bar_hands(p->d, &c, out, max);
}


static int ordered_rows(const dm_gear_snap *s, const dm_gear_result *r, dm_gtab_row *rows, int hands[DM_GTAB_HANDS][2], int *nhands) {
    pthread_mutex_lock(&text_mu);
    uint64_t bh = pick_hash(&r->best, s->p.ncand);
    if (order_cache.rev != s->rev || order_cache.best != bh || !order_cache.rev) {
        order_cache.rev = s->rev; order_cache.best = bh; order_cache.pkg_key = 0;
        int all = dm_gear_tab_rows(&s->p, &s->now, &r->best, order_cache.rows, DM_GTAB_ROWS);
        if (all > DM_GTAB_ROWS) {
            static uint64_t said_rows; if (said_rows != s->rev) { said_rows = s->rev; dm_gear_tab_log("ASSUME bestgear.rows: more rows than a full inventory allows; rows left out\n"); }
            all = DM_GTAB_ROWS;
        }
        order_cache.n = all;
        order_cache.nhands = dm_gear_tab_hands(&s->p, &r->best, order_cache.hands, DM_GTAB_HANDS);
        if (dm_gear_tab_order(&s->p, &s->now, order_cache.rows, all)) {
            static uint64_t said; if (said != s->rev) { said = s->rev; dm_gear_tab_log("ASSUME bestgear.order: no put on order of the result's rows; rows in place order\n"); }
            dm_gear_tab_rows(&s->p, &s->now, &r->best, order_cache.rows, DM_GTAB_ROWS);
        }
    }
    int n = order_cache.n; memcpy(rows, order_cache.rows, sizeof *rows * (size_t)n);
    *nhands = order_cache.nhands; memcpy(hands, order_cache.hands, sizeof order_cache.hands);
    unsigned long long k = atomic_load(&tip_want);
    uint32_t id = (uint32_t)k; int place = (int)(k >> 32) - 1;
    uint64_t pk = (bh ^ s->rev) * 1099511628211ull ^ k;
    if (id && place >= 0 && pk != order_cache.pkg_key) {
        order_cache.pkg_key = pk;
        static dm_gtab_pkg made; memset(&made, 0, sizeof made); made.id = id; made.place = place;
        for (int i = 0; i < n; i++) if (rows[i].place == place && rows[i].cand >= 0 && s->show[rows[i].cand].id == id) { pkg_make(s, rows, n, i, &made); break; }
        pkg_publish(&made);
    }
    pthread_mutex_unlock(&text_mu);
    return n;
}

static void tip(sb *b) {
    unsigned long long k = atomic_load(&tip_want);
    uint32_t id = (uint32_t)k; int place = (int)(k >> 32) - 1;
    if (!id) return;
    pthread_mutex_lock(&tip_mu);
    if (tip_id == id && tip_place == place && tip_text) { put(b, ",\"tip\":{\"id\":%u,\"p\":%d,\"dots\":%d,\"t\":", id, place, tip_dots); str(b, tip_text); put(b, ",\"b\":"); str(b, tip_block); put(b, "}"); }
    pthread_mutex_unlock(&tip_mu);
}
static _Atomic(dm_gear_tab_extra_fn) extra_fn;
static atomic_uint jobs_ended;
void dm_gear_tab_jobs(unsigned ends) { atomic_store(&jobs_ended, ends); }
void dm_gear_tab_extra(dm_gear_tab_extra_fn fn) { atomic_store(&extra_fn, fn); }
dm_gear_snap *dm_gear_tab_hold(void) {
    pthread_mutex_lock(&mu);
    dm_gear_snap *s = pending || incoming ? NULL : cur; if (s) s->refs++;
    pthread_mutex_unlock(&mu);
    return s;
}
void dm_gear_tab_unhold(dm_gear_snap *s) {
    if (!s) return;
    dm_gear_snap *drop = NULL;
    pthread_mutex_lock(&mu); unref_locked(s, &drop); pthread_mutex_unlock(&mu);
    free_list(drop);
}
int dm_gear_tab_text(char *out, size_t cap) {
    if (!out || cap < 64) return -1;
    sb b = { out, cap, 0, 0 };

    pthread_mutex_lock(&mu);
    dm_gear_snap *s = cur; if (s) s->refs++;
    int stale = s && s->jobs != atomic_load(&jobs_ended);
    int st = pending || incoming || stale ? ST_SEARCHING : status, state = cur_state, result = s && have && !pending && !incoming && !stale;
    dm_gear_result r;
    if (result) {

        double t = now_ms();
        int changed = !disp_have || disp_rev != s->rev || dm_gear_key_cmp(shown.score.key, disp.score.key) != 0 || memcmp(&shown.best, &disp.best, sizeof shown.best);
        if (changed && (st != ST_SEARCHING || !disp_have || disp_rev != s->rev || t - disp_at >= SHOW_EVERY_MS)) { disp = shown; disp_rev = s->rev; disp_have = 1; disp_at = t; }
        r = disp;
    }
    long runs = atomic_load(&prog.runs), estimate = atomic_load(&prog.estimate); int share = atomic_load(&prog.share);
    pthread_mutex_unlock(&mu);
    static const char *const st_name[] = { "none", "searching", "done", "stopped", "failed" };
    put(&b, "{\"rev\":\"%016llx\",\"status\":\"%s\",\"state\":\"%s\"", (unsigned long long)(s ? s->rev : 0), st_name[st], s ? state_name(state) : "none");
    put(&b, ",\"runs\":%ld,\"estimate\":%ld,\"share\":%d", runs, estimate, share);
    if (s) put(&b, ",\"pages\":%d,\"shared\":%d,\"skipped\":%d,\"charms\":%d", s->stash_pages, s->shared_tabs, s->skipped, !s->p.charms_fixed);
    static _Thread_local dm_gtab_row rows[DM_GTAB_ROWS];
    int nrows = 0; uint64_t key = 0;
    if (result) {
        put(&b, ",\"unrated\":%d,\"dots\":%d", r.unrated, dm_gear_dots_code(&r.score));
        put(&b, ",\"rows\":[");
        int first = 1;
        int hands[DM_GTAB_HANDS][2], nhands = 0;
        nrows = ordered_rows(s, &r, rows, hands, &nhands);
        for (int i = 0; i < nrows; i++) row(&b, s, rows[i].place, rows[i].cand, rows[i].charm, rows[i].first, &first);
        put(&b, "]");

        put(&b, ",\"hands\":[");
        for (int i = 0; i < nhands; i++) put(&b, "%s[%d,%d]", i ? "," : "", hands[i][0], hands[i][1]);
        put(&b, "]");
        key = (pick_hash(&r.best, s->p.ncand) ^ s->rev) * 1099511628211ull; key = key ? key : 1;

        dm_gear_tip t; memset(&t, 0, sizeof t);
        t.now = r.now; t.after = r.after; t.score = r.score; t.goal_hits = s->goal.survival_hits;
        for (int k = 0; k < DM_GEAR_M_N; k++) t.reasons.group[k] = -1;
        dm_gear_words w; memset(&w, 0, sizeof w);
        for (int k = 0; k < DM_GEAR_M_N; k++) w.measure[k] = s->measure[k];
        w.missed = s->missed; w.reached = s->reached; w.below = s->below; w.below_same = s->below_same; w.more = w.less = "%s";
        for (int g = 0; g < DM_GEAR_R_N; g++) w.group[g] = "";
        char line[DM_GEAR_LINES][DM_GEAR_LINEB];
        int n = first ? 0 : dm_gear_tip_text(&t, &w, s->comma, line, DM_GEAR_LINES);
        put(&b, ",\"totals\":[");
        for (int i = 0; i < n; i++) {

            const char *text = strlen(line[i]) > 4 ? line[i] + 4 : "";
            put(&b, "%s{\"c\":\"%c\",\"t\":", i ? "," : "", strlen(line[i]) > 3 ? line[i][3] : '5'); str(&b, text); put(&b, "}");
        }
        put(&b, "]");
    }
    dm_gear_tab_extra_fn extra = atomic_load(&extra_fn);
    if (extra && !b.over) {
        int k = extra(s, key, result ? rows : NULL, result ? nrows : 0, result ? &r.now : NULL, result ? &r.after : NULL, b.o + b.n, b.cap - b.n);
        if (k < 0 || (size_t)k >= b.cap - b.n) b.over = 1; else b.n += (size_t)k;
    }
    tip(&b);
    put(&b, "}");
    dm_gear_snap *drop = NULL;
    if (s) { pthread_mutex_lock(&mu); unref_locked(s, &drop); pthread_mutex_unlock(&mu); }
    free_list(drop);
    return b.over ? -1 : (int)b.n;
}
