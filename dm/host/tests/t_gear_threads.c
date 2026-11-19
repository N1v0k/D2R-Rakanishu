

#define _POSIX_C_SOURCE 200809L
#include "sim_test_io.h"
#include "../dm_gear_core.h"
#include "../dm_gear_live.h"
#include "../dm_itemcmp_core.h"
#include "../dm_sim_ui.h"
#include <assert.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

enum { EVALUATIONS = 100000, MEASURES = 8, RELOADS = 20 };
static char overlay[64];
int dm_overlay_file(const char *relative, char *out, size_t cap) {
    assert(!strcmp(relative, "sim.tsv"));
    return snprintf(out, cap, "%s", overlay) > 0;
}

static dm_ic_cb valid_cb(void) {
    dm_ic_cb c; memset(&c, 0, sizeof c);
    c.str = 100; c.dex = 50; c.lvl = 30; c.thf = 10; c.bf = 20; c.expansion = 1; c.ac = 300; c.acpct = 40; c.toblock = 30; c.can_block = 1; c.tohit = 200;
    c.armed = 1; c.mn[0] = 10; c.mx[0] = 30; c.srcdam = 128; c.dpct = 150; c.strb = 1; c.el[0] = 5; c.el[1] = 20; c.mast[0] = 30;
    return c;
}
static dm_ic_cb overflow_cb(void) {
    dm_ic_cb c = valid_cb(); c.mn[0] = 30000; c.mx[0] = 30000; c.str = 30000; c.strb = 30000; c.toblock = 30000; c.dex = 30000;
    return c;
}
typedef struct { dm_ic_cb cb, other; int rc, values[IC_C_N + 1]; unsigned cap; int bad; } eval_job;
static void *evaluate(void *arg) {
    eval_job *j = arg;
    for (int i = 0; i < EVALUATIONS; i++) {
        int v[IC_C_N + 1];
        int rc = dm_ic_cb_eval(&j->cb, v);
        unsigned cap = dm_ic_cb_new_cap(&j->other, &j->cb);
        if (rc != j->rc || (rc && memcmp(v, j->values, sizeof v)) || cap != j->cap) j->bad++;
    }
    return NULL;
}

typedef struct { const char *profile; dm_gear_measures alone; int bad; } measure_job;
static void *measure(void *arg) {
    measure_job *j = arg;
    for (int i = 0; i < MEASURES; i++) {
        const dm_sim_data *d = dm_sim_pin("rotw");
        if (!d) { j->bad++; continue; }
        dm_sim_char c; char err[128]; dm_gear_measures m;
        if (dm_sim_profile_parse(d, j->profile, &c, err, sizeof err) || dm_gear_measure(d, &c, 0, &m) || memcmp(&m, &j->alone, sizeof m)) j->bad++;
        dm_sim_unpin(d);
    }
    return NULL;
}
static void touch(int seconds) {
    struct stat st; assert(!stat(overlay, &st));
    struct timespec times[2] = { { st.st_atime, 0 }, { st.st_mtime + seconds, 0 } };
    assert(!utimensat(AT_FDCWD, overlay, times, 0));
}
static int read_generation(const dm_sim_data *d, void *ctx) { *(unsigned *)ctx = d->generation; return 0; }
typedef struct { int busy; const dm_sim_data *d; } try_job;
static int try_inside(const dm_sim_data *d, void *ctx) { (void)d; try_job *j = ctx; j->d = dm_sim_try_pin("rotw", &j->busy); return 0; }
static void *reload(void *arg) {
    int *bad = arg;
    for (int i = 1; i <= RELOADS; i++) {
        touch(1);
        unsigned generation = 0;
        int rc = dm_sim_read("rotw", read_generation, &generation);
        if (rc || !generation) (*bad)++;
        rc = dm_sim_try_read("rotw", read_generation, &generation);
        if (rc && rc != -3) (*bad)++;
    }
    return NULL;
}

static const char *PROFILE[2] = {
    "class=1\nlevel=80\ndiff=2\nact=1\nlife=1200\nmana=600\nstr=60\ndex=60\nar=500\ndef=800\nfcr=63\nweapon=1,2,100,0,0,1,0,0,1HS\nres=0,75,75,75,75,0\n"
    "skill=59,20,20\nbar=59\n",
    "class=3\nlevel=80\ndiff=2\nact=4\nlife=1500\nmana=300\nstr=150\ndex=120\nar=4000\ndef=1600\nias=40\nweapon=100,160,100,0,0,1,0,0,1HS\n"
    "res=15,75,75,75,75,0\nshield=1\nshield_min=20\nshield_max=30\nskill=106,20,20\nbar=106\n" };

int main(void) {
    const char *path = getenv("DM_SIM_DATA");
    if (!path || !*path) { fputs("DM_SIM_DATA is required\n", stderr); return 2; }

    eval_job jobs[2] = { { .cb = valid_cb(), .other = valid_cb() }, { .cb = overflow_cb(), .other = valid_cb() } };
    for (int k = 0; k < 2; k++) { jobs[k].rc = dm_ic_cb_eval(&jobs[k].cb, jobs[k].values); jobs[k].cap = dm_ic_cb_new_cap(&jobs[k].other, &jobs[k].cb); }
    assert(jobs[0].rc == 1 && jobs[1].rc == 0 && jobs[1].cap == 1u << IC_C_BLK && jobs[0].cap == 0);
    pthread_t t[3];
    for (int k = 0; k < 2; k++) assert(!pthread_create(&t[k], NULL, evaluate, &jobs[k]));
    for (int k = 0; k < 2; k++) assert(!pthread_join(t[k], NULL));
    if (jobs[0].bad || jobs[1].bad) { fprintf(stderr, "FAIL concurrent evaluations differ from single runs: %d %d\n", jobs[0].bad, jobs[1].bad); return 1; }

    snprintf(overlay, sizeof overlay, "/tmp/gear-threads-XXXXXX");
    int fd = mkstemp(overlay); assert(fd >= 0);
    size_t size; char *data = sim_read(path, &size); assert(data);
    FILE *f = fdopen(fd, "wb"); assert(f && fwrite(data, 1, size, f) == size && !fclose(f)); free(data);

    const dm_sim_data *first = dm_sim_pin("rotw"); assert(first && first->ok);
    char name[sizeof first->skill[59]->name]; memcpy(name, first->skill[59]->name, sizeof name);
    unsigned generation = first->generation;
    touch(1);
    const dm_sim_data *second = dm_sim_pin("rotw"); assert(second && second->ok);
    int lifetime = second != first && second->generation > generation && first->generation == generation && first->ok &&
                   !memcmp(first->skill[59]->name, name, sizeof name);
    dm_sim_char c; char err[128]; dm_gear_measures a, b;
    assert(!dm_sim_profile_parse(first, PROFILE[0], &c, err, sizeof err));
    lifetime &= !dm_gear_measure(first, &c, 0, &a) && !dm_gear_measure(second, &c, 0, &b) && !memcmp(&a, &b, sizeof a);
    dm_sim_unpin(first);
    const dm_sim_data *again = dm_sim_pin("rotw");
    lifetime &= again == second;
    dm_sim_unpin(again); dm_sim_unpin(second);
    if (!lifetime) { fprintf(stderr, "FAIL a pinned generation did not stay valid across a reload\n"); return 1; }


    {
        try_job inside = { 0, NULL }; int busy = 1;
        assert(!dm_sim_read("rotw", try_inside, &inside));
        const dm_sim_data *p = dm_sim_try_pin("rotw", &busy);
        int ok = !inside.d && inside.busy && p && !busy && p->generation == dm_sim_generation();
        unsigned g0 = dm_sim_generation(), g1 = 0;
        touch(1);
        assert(!dm_sim_read("rotw", read_generation, &g1));
        ok &= g1 != g0 && dm_sim_generation() == g1 && p->ok && p->generation == g0;
        dm_sim_unpin(p);
        if (!ok) { fprintf(stderr, "FAIL pinning without waiting\n"); return 1; }
    }

    measure_job measures[2];
    for (int k = 0; k < 2; k++) {
        const dm_sim_data *d = dm_sim_pin("rotw"); assert(d);
        measures[k] = (measure_job){ .profile = PROFILE[k] };
        assert(!dm_sim_profile_parse(d, PROFILE[k], &c, err, sizeof err) && !dm_gear_measure(d, &c, 0, &measures[k].alone));
        dm_sim_unpin(d);
    }
    int reload_bad = 0;
    for (int k = 0; k < 2; k++) assert(!pthread_create(&t[k], NULL, measure, &measures[k]));
    assert(!pthread_create(&t[2], NULL, reload, &reload_bad));
    for (int k = 0; k < 3; k++) assert(!pthread_join(t[k], NULL));



    dm_sim_char now, after; dm_gear_goal goal = { 15, 0.5 }; dm_gear_tip direct, queued;
    const dm_sim_data *d = dm_sim_pin("rotw"); assert(d);
    assert(!dm_sim_profile_parse(d, PROFILE[0], &now, err, sizeof err)); after = now; after.life += 200; now.act = after.act = 1;
    assert(!dm_gear_tip_rate(d, &now, &after, &goal, 0, &direct));
    dm_sim_unpin(d);
    int worker_bad = dm_gear_tip_get(11, NULL, NULL) != DM_GEAR_TIP_UNKNOWN;
    for (uint64_t key = 1; key <= 10; key++) worker_bad |= dm_gear_tip_put(key, "rotw", &now, &after, &goal, 0, key) != 0;
    int state = DM_GEAR_TIP_PENDING;
    for (int i = 0; i < 20000 && state == DM_GEAR_TIP_PENDING; i++) { state = dm_gear_tip_get(10, &queued, NULL); if (state == DM_GEAR_TIP_PENDING) nanosleep(&(struct timespec){ 0, 1000000 }, NULL); }
    worker_bad |= state != DM_GEAR_TIP_DONE || memcmp(&queued.score, &direct.score, sizeof direct.score) || memcmp(&queued.reasons, &direct.reasons, sizeof direct.reasons);
    int known = 0; for (uint64_t key = 1; key <= 10; key++) known += dm_gear_tip_get(key, NULL, NULL) != DM_GEAR_TIP_UNKNOWN;
    worker_bad |= known != 8;
    dm_sim_char nowhere = now; nowhere.act = 0;
    worker_bad |= dm_gear_tip_put(12, "rotw", &nowhere, &nowhere, &goal, 0, 12) != 0;
    for (int i = 0; i < 20000 && (state = dm_gear_tip_get(12, NULL, NULL)) == DM_GEAR_TIP_PENDING; i++) nanosleep(&(struct timespec){ 0, 1000000 }, NULL);
    worker_bad |= state != DM_GEAR_TIP_FAILED;
    dm_gear_goal g; dm_gear_goal_get(5, &g); worker_bad |= g.survival_hits != DM_GEAR_DEFAULT_HITS || g.boss_share != DM_GEAR_DEFAULT_BOSS;
    dm_gear_goal_set(5, 20, 30); dm_gear_goal_get(5, &g); worker_bad |= g.survival_hits != 20 || g.boss_share != 0.3;
    dm_gear_goal_get(6, &g); worker_bad |= g.survival_hits != DM_GEAR_DEFAULT_HITS;
    float box[DM_GEAR_DOTS_BOX] = { 0.1f, 0.2f, 0.3f, 0.4f, 0.02f, 0.01f, 0.005f, 0.12f, 0.08f, 1.0f }, seen[DM_GEAR_DOTS_BOX];
    dm_gear_dots_set(100, 2 | 7 << 3, box);
    worker_bad |= dm_gear_dots_get(102, seen) != (2 | 7 << 3) || memcmp(seen, box, sizeof box) || dm_gear_dots_get(103, seen) != 0;
    unlink(overlay);
    if (worker_bad) { fprintf(stderr, "FAIL the tooltip worker, goal or dots\n"); return 1; }
    if (measures[0].bad || measures[1].bad || reload_bad) {
        fprintf(stderr, "FAIL measures beside reloads: %d %d differ, %d reads failed\n", measures[0].bad, measures[1].bad, reload_bad); return 1;
    }
    printf("gear threads: %d evaluations per thread (valid and overflowing) as alone, pinned generation across a reload, pins without waiting, %d measures per thread beside "
           "%d reloads, the tooltip worker passed\n", EVALUATIONS, MEASURES, RELOADS);
    return 0;
}
