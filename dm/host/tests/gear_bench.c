




#define _POSIX_C_SOURCE 200809L
#include "t_gear_stock.h"
#include <pthread.h>
#include <sys/resource.h>
#include <time.h>

static double now_s(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + t.tv_nsec / 1e9; }
static void nap(double s) { struct timespec t = { (time_t)s, (long)((s - (time_t)s) * 1e9) }; nanosleep(&t, NULL); }

typedef struct { dm_gear_search *s; dm_gear_goal goal; atomic_int stop, done; dm_gear_progress pr; dm_gear_result r; double start, end, last; } job;
static void better(const dm_gear_result *r, void *ctx) { (void)r; job *j = ctx; j->last = now_s() - j->start; }
static void *run(void *x) { job *j = x; dm_gear_search_run(j->s, &j->goal, &j->stop, &j->pr, &j->r); j->end = now_s(); atomic_store(&j->done, 1); return NULL; }

static void stock_spec(gspec *s, const char *profile, int variants, uint64_t seed) {
    memset(s, 0, sizeof s[0]);
    s->profile = profile; s->act = 3; s->seed = seed; s->variants = variants; s->skills = 1; s->sets = 2; s->chain = 1;

    s->count[DM_GEAR_K_WEAPON1] = 13; s->count[DM_GEAR_K_HEAD] = 1; s->count[DM_GEAR_K_ARMOR] = 4; s->count[DM_GEAR_K_AMULET] = 7;
    s->count[DM_GEAR_K_RING] = 8; s->count[DM_GEAR_K_BELT] = 2; s->count[DM_GEAR_K_BOOTS] = 1;
    s->charms_inv = 6; s->charms_stash = 6; s->blocked = 3;
}

static int bench(const char *name, gstock *g, double limit) {
    const char *fx = getenv("DM_GEAR_CHARMS_FIXED"); g->p.charms_fixed = fx && atoi(fx) > 0;
    const char *sp = getenv("DM_GEAR_SPLASH"); g->p.splash_on = sp && atoi(sp) > 0;
    double t0 = now_s();
    dm_gear_search *s = dm_gear_search_new(&g->p);
    if (!s) { printf("%-22s no context\n", name); return 1; }
    double t_new = now_s() - t0;

    static const atomic_int wide_on = 1;
    const char *e = getenv("DM_GEAR_THREADS"); if (e && atoi(e) > 1) dm_gear_search_wide(s, atoi(e), &wide_on, NULL);
    job *j = calloc(1, sizeof *j); if (!j) abort();
    j->s = s; j->goal.survival_hits = 15; j->goal.boss_share = 0.5; j->pr.better = better; j->pr.ctx = j;
    pthread_t th; t0 = j->start = now_s();
    pthread_create(&th, NULL, run, j);
    double first = -1;
    while (!atomic_load(&j->done)) {
        if (first < 0 && atomic_load(&j->pr.found)) first = now_s() - t0;
        if (now_s() - t0 > limit) atomic_store(&j->stop, 1);
        nap(0.001);
    }
    pthread_join(th, NULL);
    if (first < 0) first = j->end - t0;
    double total = j->end - t0;

    dm_gear_search *s2 = dm_gear_search_new(&g->p);
    job *k = calloc(1, sizeof *k); if (!k || !s2) abort();
    k->s = s2; k->goal = j->goal;
    pthread_create(&th, NULL, run, k);
    nap(0.2);
    double asked = now_s();
    atomic_store(&k->stop, 1);
    pthread_join(th, NULL);
    double latency = k->r.status == DM_GEAR_S_ABORTED ? (k->end - asked) * 1000 : -1;
    struct rusage ru; getrusage(RUSAGE_SELF, &ru);
    printf("%-22s %4d cand %9.3g raw %4d out | first %6.3f s, best %7.2f s, %s %8.2f s | runs %8ld new %8ld bounds %7ld cut %7ld leaves %8ld | %5.2f MB | stop %5.2f ms"
           " | unrated %d dual %d | score %g %g\n",
           name, g->p.ncand, j->r.raw_combinations, j->r.filtered, first, j->last, j->r.status == DM_GEAR_S_DONE ? "optimum" : "STOPPED", total,
           atomic_load(&j->pr.runs), atomic_load(&j->pr.fresh), atomic_load(&j->pr.bounds), atomic_load(&j->pr.pruned), atomic_load(&j->pr.leaves),
           dm_gear_search_bytes(s) / 1048576.0, latency, j->r.unrated, j->r.dual_pairs, j->r.score.key[0], j->r.score.key[1]);
    (void)t_new; (void)ru;
    fflush(stdout);
    dm_gear_search_free(s); dm_gear_search_free(s2); free(j); free(k);
    return 0;
}

int main(int argc, char **argv) {
    double limit = argc > 1 ? atof(argv[1]) : 600;
    static dm_gear_tables gt; const dm_sim_data *d = gs_tables(&gt);
    static gstock g;
    static const char *const classes[] = { "class1-level80.prof", "class3-level80.prof", "class5-level80.prof", "class7-level80.prof" };
    for (size_t c = 0; c < 4; c++) {
        char *profile = gs_profile(classes[c]);
        for (int k = 1; k <= 5; k += 2) {
            gspec s; stock_spec(&s, profile, k, 100 + c);
            if (gs_build(&g, d, &gt, &s)) { printf("%s x%d not built\n", classes[c], k); continue; }
            char name[48]; snprintf(name, sizeof name, "%.6s x%d", classes[c], k);
            bench(name, &g, limit);
        }
    }

    gspec s; stock_spec(&s, gs_profile("class3-level80.prof"), 1, 200); s.blocked = 9; s.charms_inv = 8; s.charms_stash = 8;
    if (!gs_build(&g, d, &gt, &s)) bench("class3 fragmented", &g, limit);

    for (int k = 1; k <= 3; k += 2) {
        stock_spec(&s, gs_profile("class4-level80.prof"), k, 300); if (!gs_build(&g, d, &gt, &s)) { char n[32]; snprintf(n, sizeof n, "class4 dual x%d", k); bench(n, &g, limit); }
        stock_spec(&s, gs_profile("class6-level80.prof"), k, 301);
        if (!gs_build(&g, d, &gt, &s)) {
            for (int i = 0; i < g.p.nweapon; i++) if (i) { snprintf(g.weapon[i].w.wclass, 4, "HT1"); g.weapon[i].w.kind = DM_WK_CLAW; }
            char n[32]; snprintf(n, sizeof n, "class6 claws x%d", k); bench(n, &g, limit);
        }
        stock_spec(&s, gs_profile("class3-level80.prof"), k, 302); s.unrated = 6;
        if (!gs_build(&g, d, &gt, &s)) { char n[32]; snprintf(n, sizeof n, "class3 aura items x%d", k); bench(n, &g, limit); }
    }


    static const char *const drop[] = { "lifedrainmindam", "hpregen", "fireresist", "lightresist", "coldresist", "poisonresist", "normal_damage_reduction" };
    for (size_t c = 0; c < 4; c++) for (int k = 1; k <= 3; k += 2) {
        stock_spec(&s, gs_profile(classes[c]), k, 100 + c); s.act = 5;
        if (gs_build(&g, d, &gt, &s)) continue;
        for (int i = 0; i < g.p.ncand; i++) {
            dm_gear_list *l = &g.cand[i].item.own; int n = 0;
            for (int v = 0; v < l->n; v++) {
                int keep = 1;
                for (size_t q = 0; q < sizeof drop / sizeof *drop; q++) keep &= strcmp(d->stat_name[l->v[v].stat], drop[q]) != 0;
                if (keep) l->v[n++] = l->v[v];
            }
            l->n = n;
        }
        if (gs_capture(&g)) continue;
        char n[32]; snprintf(n, sizeof n, "%.6s weak x%d", classes[c], k); bench(n, &g, limit);
    }
    struct rusage ru; getrusage(RUSAGE_SELF, &ru);
    printf("peak resident %.1f MB\n", ru.ru_maxrss / 1024.0);
    return 0;
}
