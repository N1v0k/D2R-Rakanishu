


#define _POSIX_C_SOURCE 200809L
#include "sim_test_io.h"
#include "../dm_sim_internal.h"
#include <assert.h>
#include <math.h>
#include <string.h>
#include <time.h>

enum { MAXE = 4096, NV = 6 };
typedef struct { int n, lmin, lmax; double v[NV][MAXE]; int nv[NV]; } act_values;

static int by_value(const void *a, const void *b) { double x = *(const double *)a, y = *(const double *)b; return x < y ? -1 : x > y; }
static double median(double *v, int n) {
    if (!n) return 0;
    qsort(v, (size_t)n, sizeof *v, by_value);
    return n % 2 ? v[n / 2] : (v[n / 2 - 1] + v[n / 2]) / 2;
}


static int atypical(int area) { return area == 39 || area == 120 || (area >= 133 && area <= 136); }



static const int RANGE[3][6][2] = {
    { { 1, 43 }, { 1, 12 }, { 13, 23 }, { 19, 25 }, { 24, 32 }, { 31, 43 } },
    { { 36, 66 }, { 36, 43 }, { 43, 49 }, { 49, 55 }, { 56, 58 }, { 58, 66 } },
    { { 67, 85 }, { 67, 85 }, { 74, 85 }, { 79, 85 }, { 82, 85 }, { 80, 85 } },
};


static void collect(const dm_sim_data *d, int diff, act_values acts[6]) {
    memset(acts, 0, 6 * sizeof *acts);
    for (int a = 0; a < 6; a++) { acts[a].lmin = 1000; acts[a].lmax = -1; }
    for (int i = 0; i < d->nspawn; i++) {
        const dm_sim_spawn *s = &d->spawn[i];
        if (s->list != (diff > 0) || atypical(s->area)) continue;
        const dm_sim_area *area = NULL;
        for (int k = 0; k < d->narea; k++) if (d->area[k].id == s->area) area = &d->area[k];
        assert(area && area->act >= 1 && area->act <= 5);
        for (int j = 0; j < s->n; j++) {
            const dm_sim_mon *m = &d->mon[s->mon[j]];
            if ((m->flags & (DM_MF_BOSS | DM_MF_CRITTER)) || !(m->flags & DM_MF_ENABLED)) continue;
            assert(!(m->flags & DM_MF_NORATIO));
            const dm_sim_mond *x = &m->d[diff];
            int lvl = diff == 0 ? m->lvl[0] : area->mlvl[diff];
            double el = 0; int has_el = 0;
            for (int k = 0; k < 3; k++)
                if (m->eltype[k] >= 0 && (!m->elmode[k][0] || !strcmp(m->elmode[k], "A1")) && x->elpct[k] > 0) { el += (x->elmin[k] + x->elmax[k]) / 2.0; has_el = 1; }
            const double v[NV] = { (x->hpmin + x->hpmax) / 2.0, x->ac, x->a1th, x->a1min, x->a1max, el };
            for (int g = 0; g < 2; g++) {
                act_values *t = &acts[g ? area->act : 0];
                assert(t->n < MAXE);
                t->n++; if (lvl < t->lmin) t->lmin = lvl; if (lvl > t->lmax) t->lmax = lvl;
                for (int k = 0; k < NV; k++) if (k != NV - 1 || has_el) t->v[k][t->nv[k]++] = v[k];
            }
        }
    }
}
static void same_target(const dm_sim_target *a, const dm_sim_target *b) {
    assert(a->mlvl == b->mlvl && a->hp == b->hp && a->ac == b->ac && a->ar == b->ar && a->dmin == b->dmin && a->dmax == b->dmax);
    assert(!memcmp(a->elmin, b->elmin, sizeof a->elmin) && !memcmp(a->elmax, b->elmax, sizeof a->elmax) && !memcmp(a->res, b->res, sizeof a->res));
}

static int check_set(const char *text, size_t n, const char *set) {
    static dm_sim_data d;
    assert(!dm_sim_load(&d, text, n, set));
    static act_values acts[6];
    int checked = 0;
    for (int diff = 0; diff < 3; diff++) {
        collect(&d, diff, acts);
        for (int act = 0; act <= 5; act++) {
            act_values *e = &acts[act]; const dm_sim_std *s = &d.std[diff][act];
            assert(e->n > 0 && s->n == e->n && s->lmin == e->lmin && s->lmax == e->lmax && s->lmin < s->lmax);
            if (s->lmin != RANGE[diff][act][0] || s->lmax != RANGE[diff][act][1]) {
                fprintf(stderr, "%s diff %d act %d: levels %d..%d, game %d..%d\n", set, diff, act, s->lmin, s->lmax, RANGE[diff][act][0], RANGE[diff][act][1]);
                return -1;
            }
            double base[NV];
            for (int k = 0; k < NV; k++) base[k] = median(e->v[k], e->nv[k]);
            const int col[NV] = { diff, 3 + diff, 6 + diff, 9 + diff, 9 + diff, 9 + diff };

            const int levels[] = { 1, s->lmin - 1, s->lmin, s->lmin + 1, (s->lmin + s->lmax) / 2, s->lmax - 1, s->lmax, s->lmax + 1, 99 };
            for (size_t i = 0; i < sizeof levels / sizeof *levels; i++) {
                int clvl = levels[i] < 1 ? 1 : levels[i], want = clvl < s->lmin ? s->lmin : clvl > s->lmax ? s->lmax : clvl;
                dm_sim_target t;
                assert(dm_sim_std_level(&d, diff, act, clvl) == want && !dm_sim_target_std_at(&d, diff, act, clvl, &t) && t.mlvl == want);
                const int got[NV] = { t.hp, t.ac, t.ar, t.dmin, t.dmax, t.elmin[0] + t.elmin[1] + t.elmin[2] };
                for (int k = 0; k < NV; k++) {
                    double scaled = base[k] * d.monlvl[want][col[k]] / 100;

                    double slack = k == NV - 1 ? 3 : 1;
                    if (fabs(got[k] - scaled) > slack) { fprintf(stderr, "%s diff %d act %d level %d value %d: %d, tables %.2f\n", set, diff, act, clvl, k, got[k], scaled); return -1; }
                }

                dm_sim_target fixed; assert(!dm_sim_target_std(&d, diff, act, &fixed));
                for (int k = 0; k < DM_EL_N; k++) assert(t.res[k] == fixed.res[k] && t.res[k] < 100 && t.immune_share[k] == s->immune[k]);
                assert(t.kind == DM_TK_STD && t.ranged_share == s->ranged && t.cycle == s->cycle && t.block == s->block);
                checked++;
            }

            dm_sim_target lo, lo2, mid, hi, hi2;
            assert(!dm_sim_target_std_at(&d, diff, act, s->lmin, &lo) && !dm_sim_target_std_at(&d, diff, act, s->lmin - 5, &lo2));
            assert(!dm_sim_target_std_at(&d, diff, act, s->lmax, &hi) && !dm_sim_target_std_at(&d, diff, act, s->lmax + 7, &hi2));
            assert(!dm_sim_target_std_at(&d, diff, act, s->lmin + 1, &mid));
            same_target(&lo, &lo2); same_target(&hi, &hi2);
            assert(mid.mlvl == lo.mlvl + 1 && (mid.hp != lo.hp || mid.ar != lo.ar || mid.dmax != lo.dmax));
            assert(hi.hp > lo.hp);

            dm_sim_char c; dm_sim_char_init(&c, 0); c.diff = diff; c.act = act; c.clvl = s->lmax + 3;
            dm_sim_target viaact; assert(dm_sim_target_act(&d, &c, &viaact) == act); same_target(&viaact, &hi);
        }
    }

    static dm_sim_data copy; copy = d; copy.std[1][3].n = 0;
    dm_sim_char c; dm_sim_char_init(&c, 0); c.diff = 1; c.act = 3; c.clvl = 40; dm_sim_target t, all;
    assert(dm_sim_target_act(&copy, &c, &t) == 0 && !dm_sim_target_std_at(&d, 1, 0, 40, &all)); same_target(&t, &all);
    assert(dm_sim_std_level(&copy, 1, 3, 40) == -1);
    dm_sim_free(&d);
    return checked;
}

int main(void) {
    const char *path = getenv("DM_SIM_DATA");
    if (!path || !*path) { puts("t_sim_std: DM_SIM_DATA not set, skipped"); return 0; }
    struct timespec a, z; clock_gettime(CLOCK_MONOTONIC, &a);
    size_t n; char *text = sim_read(path, &n); assert(text);
    int checked = 0;
    for (int i = 0; i < 2; i++) { int k = check_set(text, n, i ? "lod" : "rotw"); assert(k > 0); checked += k; }
    free(text);
    clock_gettime(CLOCK_MONOTONIC, &z);
    printf("t_sim_std: %d standard enemy levels match the game tables (%.0f ms)\n", checked, (z.tv_sec - a.tv_sec) * 1000.0 + (z.tv_nsec - a.tv_nsec) / 1e6);
    return 0;
}
