#define _POSIX_C_SOURCE 200809L
#include "dm_gear_live.h"
#include "dm_sim_ui.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

enum { SLOTS = 8 };
enum { FREE, QUEUED, RUNNING, DONE, FAILED, GRANTS };
typedef struct {
    uint64_t key, frame;
    int state;
    unsigned used;
    char set[8];
    dm_sim_char now, after;
    dm_gear_goal goal;
    int splash_on;
    dm_gear_tip tip;
} slot;
static slot slots[SLOTS];
static unsigned clock_used;
static double last_ms;
static int started;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t wake = PTHREAD_COND_INITIALIZER;
static pthread_t worker;

static int newest_queued(void) {
    int best = -1;
    for (int i = 0; i < SLOTS; i++) if (slots[i].state == QUEUED && (best < 0 || slots[i].used > slots[best].used)) best = i;
    return best;
}
static void *run(void *unused) {
    (void)unused;
    static dm_sim_char now, after;
    for (;;) {
        pthread_mutex_lock(&lock);
        int i;
        while ((i = newest_queued()) < 0) pthread_cond_wait(&wake, &lock);
        slot *s = &slots[i];
        s->state = RUNNING;
        uint64_t key = s->key; char set[8]; memcpy(set, s->set, sizeof set);
        now = s->now; after = s->after; dm_gear_goal goal = s->goal; int splash_on = s->splash_on;
        pthread_mutex_unlock(&lock);
        struct timespec a, z; clock_gettime(CLOCK_MONOTONIC, &a);
        dm_gear_tip tip; int rc = -1;
        const dm_sim_data *d = dm_sim_pin(set);
        if (d) { rc = dm_gear_tip_rate(d, &now, &after, &goal, splash_on, &tip); dm_sim_unpin(d); }
        clock_gettime(CLOCK_MONOTONIC, &z);
        pthread_mutex_lock(&lock);

        if (s->state == RUNNING && s->key == key) { s->state = rc == DM_GEAR_RATE_GRANTS ? GRANTS : rc ? FAILED : DONE; if (!rc || rc == DM_GEAR_RATE_GRANTS) s->tip = tip; }
        last_ms = (z.tv_sec - a.tv_sec) * 1000.0 + (z.tv_nsec - a.tv_nsec) / 1e6;
        pthread_mutex_unlock(&lock);
    }
    return NULL;
}

int dm_gear_tip_get(uint64_t key, dm_gear_tip *out, uint64_t *asked) {
    pthread_mutex_lock(&lock);
    int r = DM_GEAR_TIP_UNKNOWN;
    for (int i = 0; i < SLOTS; i++) {
        slot *s = &slots[i];
        if (s->state == FREE || s->key != key) continue;
        r = s->state == DONE ? DM_GEAR_TIP_DONE : s->state == FAILED ? DM_GEAR_TIP_FAILED : s->state == GRANTS ? DM_GEAR_TIP_GRANTS : DM_GEAR_TIP_PENDING;
        if ((r == DM_GEAR_TIP_DONE || r == DM_GEAR_TIP_GRANTS) && out) *out = s->tip;
        if (asked) *asked = s->frame;
        s->used = ++clock_used;
        break;
    }
    pthread_mutex_unlock(&lock);
    return r;
}

int dm_gear_tip_put(uint64_t key, const char *set, const dm_sim_char *now, const dm_sim_char *after, const dm_gear_goal *goal, int splash_on, uint64_t frame) {
    if (!set || !now || !after || !goal) return -1;
    pthread_mutex_lock(&lock);
    if (!started) {
        if (pthread_create(&worker, NULL, run, NULL)) { pthread_mutex_unlock(&lock); return -1; }
        pthread_detach(worker); started = 1;
    }
    int pick = -1;
    for (int i = 0; i < SLOTS && pick < 0; i++) if (slots[i].state != FREE && slots[i].state != RUNNING && slots[i].key == key) pick = i;
    for (int i = 0; i < SLOTS && pick < 0; i++) if (slots[i].state == FREE) pick = i;
    if (pick < 0) for (int i = 0; i < SLOTS; i++) if (slots[i].state != RUNNING && (pick < 0 || slots[i].used < slots[pick].used)) pick = i;
    if (pick < 0) { pthread_mutex_unlock(&lock); return -1; }
    slot *s = &slots[pick];
    s->key = key; s->frame = frame; s->state = QUEUED; s->used = ++clock_used;
    snprintf(s->set, sizeof s->set, "%s", set);
    s->now = *now; s->after = *after; s->goal = *goal; s->splash_on = splash_on;
    pthread_cond_signal(&wake);
    pthread_mutex_unlock(&lock);
    return 0;
}

double dm_gear_tip_ms(void) { pthread_mutex_lock(&lock); double v = last_ms; pthread_mutex_unlock(&lock); return v; }

static struct { int session, hits, boss; } goal_now;
void dm_gear_goal_set(int session, int survival_hits, int boss_percent) {
    if (survival_hits < 0) survival_hits = 0;
    if (boss_percent < 0) boss_percent = 0;
    if (boss_percent > 100) boss_percent = 100;
    pthread_mutex_lock(&lock); goal_now.session = session; goal_now.hits = survival_hits; goal_now.boss = boss_percent; pthread_mutex_unlock(&lock);
}
void dm_gear_goal_get(int session, dm_gear_goal *g) {
    pthread_mutex_lock(&lock);
    int mine = session && goal_now.session == session;
    g->survival_hits = mine ? goal_now.hits : DM_GEAR_DEFAULT_HITS;
    g->boss_share = mine ? goal_now.boss / 100.0 : DM_GEAR_DEFAULT_BOSS;
    pthread_mutex_unlock(&lock);
}

static struct { uint64_t frame; int dots; float box[DM_GEAR_DOTS_BOX]; } shown;
void dm_gear_dots_set(uint64_t frame, int dots, const float box[DM_GEAR_DOTS_BOX]) {
    pthread_mutex_lock(&lock);
    shown.frame = frame; shown.dots = dots & 63;
    if (box) memcpy(shown.box, box, sizeof shown.box); else memset(shown.box, 0, sizeof shown.box);
    pthread_mutex_unlock(&lock);
}
int dm_gear_dots_get(uint64_t now, float box[DM_GEAR_DOTS_BOX]) {
    pthread_mutex_lock(&lock);
    int dots = shown.frame && now <= shown.frame + 2 ? shown.dots : 0;
    if (box) memcpy(box, shown.box, sizeof shown.box);
    pthread_mutex_unlock(&lock);
    return dots;
}
