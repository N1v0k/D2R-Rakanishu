

#include "dm_gear_equip_tab.h"
#include "dm_gear_equip.h"
#include "dm_feats.h"
#include "dm.h"
#include <pthread.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EQ_MAX (DM_GO_MAXCHANGE + 1)
#define CAPTURE_FRAMES 15
#define CAPTURE_SOON 4
#define BUILDS 8
enum { JOB_NONE, JOB_QUEUED, JOB_RUN, JOB_CHANGED, JOB_AWAY, JOB_BUSY };

typedef struct { int row, ok; dm_gew_target t; } tg_t;
typedef struct { uint64_t key, hash; int rc; dm_go_plan plan; dm_gear_equip_cap cap; } plan_t;

static pthread_mutex_t mu = PTHREAD_MUTEX_INITIALIZER;
static struct {

    unsigned gen; uint64_t key; int n; tg_t tg[EQ_MAX]; int row_tg[DM_GTAB_ROWS], all, worthy;

    unsigned cgen; int known, open; int crc[EQ_MAX]; dm_gear_equip_cap cap[EQ_MAX];

    int seen, open_now;

    plan_t pl[EQ_MAX];

    int req, rworthy; unsigned seq; dm_gear_equip_cap rcap; dm_go_plan rplan; dm_gew_target rtg;
} g;
static atomic_uint job_seq;
static atomic_int job_kind;


static int put(char *out, size_t cap, size_t *n, const char *fmt, ...) __attribute__((format(printf, 4, 5)));
static int put(char *out, size_t cap, size_t *n, const char *fmt, ...) {
    if (*n >= cap) return -1;
    va_list ap; va_start(ap, fmt); int k = vsnprintf(out + *n, cap - *n, fmt, ap); va_end(ap);
    if (k < 0 || (size_t)k >= cap - *n) { *n = cap; return -1; }
    *n += (size_t)k; return 0;
}
static const char *end_name(int end) {
    switch (end) { case DM_GE_DONE: return "done"; case DM_GE_RETURNED: return "returned"; case DM_GE_PARTIAL: return "partial";
                   case DM_GE_IN_HAND: return "in_hand"; case DM_GE_LOST: return "lost"; default: return "run"; }
}

static void job_text(char *out, size_t cap, size_t *m) {
    unsigned sq = atomic_load(&job_seq); int kind = atomic_load(&job_kind);
    if (!sq || kind == JOB_NONE) return;
    int why = 0, done = 0, end = kind == JOB_RUN ? dm_gear_equip_state(&why, &done, NULL) : 0, k = 0, nu = 0, off = 0;
    pthread_mutex_lock(&mu);
    nu = g.rtg.nunit; k = dm_gew_units_done(&g.rtg, &g.rplan, done);
    for (int u = 0; u < nu && u < DM_GO_MAXCHANGE; u++) {
        int empties = g.rtg.unit_slots[u] != 0;
        for (int sl = 0; sl < DM_GO_SLOTS; sl++) if (g.rtg.unit_slots[u] >> sl & 1) empties &= g.rtg.slot[sl].kind == DM_GEW_EMPTY;
        off += empties;
    }
    pthread_mutex_unlock(&mu);
    const char *st = kind == JOB_QUEUED ? "queued" : kind == JOB_CHANGED ? "changed" : kind == JOB_AWAY ? "away" : kind == JOB_BUSY ? "busy" : end_name(end);
    if (end == DM_GE_DONE) k = nu;
    put(out, cap, m, ",\"job\":{\"seq\":%u,\"st\":\"%s\",\"k\":%d,\"n\":%d,\"off\":%d,\"why\":\"%s\"}", sq, st, k, nu, off, kind == JOB_RUN ? dm_ge_why_name(why) : "");
}

static int text(const dm_gear_snap *s, uint64_t key, const dm_gtab_row *rows, int n, const dm_gear_measures *now, const dm_gear_measures *after, char *out, size_t cap) {
    size_t m = 0;
    if (!rows) { job_text(out, cap, &m); return m >= cap ? -1 : (int)m; }
    static _Thread_local tg_t tg[EQ_MAX]; static _Thread_local int row_tg[DM_GTAB_ROWS]; int ntg = 0, all = -1;

    memset(tg, 0, sizeof tg);
    for (int i = 0; i < DM_GTAB_ROWS; i++) row_tg[i] = -1;
    for (int i = 0; i < n && i < DM_GTAB_ROWS; i++) {
        if (rows[i].charm || rows[i].place < 0) continue;
        int k = 0; while (k < ntg && rows[tg[k].row].unit != rows[i].unit) k++;
        if (k == ntg) { if (ntg >= EQ_MAX - 1) continue; tg[k].row = i; tg[k].ok = !dm_gew_target_of(s, rows, n, i, &tg[k].t); ntg++; }
        row_tg[i] = k;
    }
    if (ntg) { all = ntg; tg[all].row = -1; tg[all].ok = !dm_gew_target_of(s, rows, n, -1, &tg[all].t); ntg++; }
    pthread_mutex_lock(&mu);
    if (g.key != key || g.n != ntg || memcmp(g.tg, tg, sizeof tg)) {
        g.key = key; g.n = ntg; memcpy(g.tg, tg, sizeof tg); g.all = all; g.gen++;
    }
    memcpy(g.row_tg, row_tg, sizeof row_tg);
    g.worthy = now && after && dm_ft_equip_worthy(now->group, after->group, now->single, after->single);
    unsigned gen = g.gen, cgen = g.cgen; int known = g.known && cgen == gen, open = g.open, stash = known ? open : g.seen ? g.open_now : -1;
    static _Thread_local dm_gear_equip_cap caps[EQ_MAX]; static _Thread_local int crc[EQ_MAX]; static _Thread_local uint64_t have[EQ_MAX]; static _Thread_local int rc[EQ_MAX];
    for (int k = 0; k < ntg; k++) {
        crc[k] = known ? g.crc[k] : -3;
        if (crc[k] == 0) caps[k] = g.cap[k];
        have[k] = g.pl[k].key == key && crc[k] == 0 && g.pl[k].hash == caps[k].hash ? 1 : 0; rc[k] = g.pl[k].rc;
    }
    pthread_mutex_unlock(&mu);

    for (int k = 0; k < ntg; k++) {
        if (!tg[k].ok || crc[k] || have[k]) continue;
        static _Thread_local dm_go_plan plan; static _Thread_local dm_gear_equip_cap c; c = caps[k];
        int e = dm_gear_equip_plan_seq(s, &c, tg[k].t.seq, tg[k].t.nseq, &plan);
        if (!e && dm_ge_check(&c.w, &plan, c.gp)) e = DM_GO_E_ROOM;
        rc[k] = e;
        pthread_mutex_lock(&mu);
        if (g.gen == gen) { g.pl[k].key = key; g.pl[k].hash = c.hash; g.pl[k].rc = e; g.pl[k].plan = plan; g.pl[k].cap = c; }
        pthread_mutex_unlock(&mu);
        { static _Thread_local uint64_t said[EQ_MAX]; if (said[k] != (key ^ c.hash)) { said[k] = key ^ c.hash;
            char line[128]; snprintf(line, sizeof line, "[gear-equip] tab: target %d (row %d, %d units, first slot %d) plan %d: %d steps\n", k, tg[k].row,
                                     tg[k].t.nunit, tg[k].t.nseq ? tg[k].t.seq[0] : 0, e, e ? 0 : plan.nstep); dm_gear_tab_log(line); } }
    }


    put(out, cap, &m, ",\"eqkey\":\"%016llx\",\"stash\":%d,\"eq\":[", (unsigned long long)key, stash);
    int every = ntg > 1, buttons = 0;
    for (int i = 0; i < n; i++) {
        int k = i < DM_GTAB_ROWS ? row_tg[i] : -1, v = -1;
        if (k >= 0 && tg[k].ok) v = !known || crc[k] || (rc[k] == -10 || rc[k] == DM_GO_E_INPUT) ? 2 : rc[k] == 0 ? 1 : 0;
        if (k >= 0 && tg[k].row == i) { buttons += v == 1 || v == 0; every &= v == 1; }
        else if (k >= 0) v = -1;
        else if (!rows[i].charm) every = 0;
        put(out, cap, &m, "%s%d", i ? "," : "", v);
    }
    int eqall = all >= 0 && every && buttons >= 2 && tg[all].ok && known && !crc[all] && rc[all] == 0;
    put(out, cap, &m, "],\"eqall\":%d", eqall);
    job_text(out, cap, &m);
    return m >= cap ? -1 : (int)m;
}

int dm_gear_equip_tab_tap(const char *key, int row) {
    unsigned long long k = key ? strtoull(key, NULL, 16) : 0;
    int kind = atomic_load(&job_kind);
    pthread_mutex_lock(&mu);
    int rc = 0;
    if (g.req || kind == JOB_QUEUED || (kind == JOB_RUN && dm_gear_equip_state(NULL, NULL, NULL) == DM_GE_RUNNING)) rc = -3;
    else if (!g.known || !g.open) rc = -2;
    else if (k != g.key || row < -1 || row >= DM_GTAB_ROWS) rc = -1;
    else {
        int t = row < 0 ? g.all : g.row_tg[row];
        if (t < 0 || t >= g.n || !g.tg[t].ok || g.pl[t].key != g.key || g.pl[t].rc || g.cgen != g.gen || g.crc[t] || g.pl[t].hash != g.cap[t].hash) rc = -1;
        else {
#ifndef DM_PLAYER_BUILD
            static int stale = -1; if (stale < 0) stale = getenv("DM_TEST_GEAR_STALE") != NULL;
            if (stale == 1) { stale = 0; LOG("[gear-equip] tab: DM_TEST_GEAR_STALE: this tap counts as a changed result"); pthread_mutex_unlock(&mu); return -1; }
#endif
            g.rcap = g.pl[t].cap; g.rplan = g.pl[t].plan; g.rtg = g.tg[t].t; g.rworthy = row < 0 && g.worthy; g.req = 1; rc = (int)++g.seq;
            if (rc <= 0) rc = (int)(g.seq = 1);
            atomic_store(&job_seq, (unsigned)rc); atomic_store(&job_kind, JOB_QUEUED);
        }
    }
    pthread_mutex_unlock(&mu);
    LOG("[gear-equip] tab: tap row %d -> %d", row, rc);
    return rc;
}


void dm_gear_equip_tab_init(void) { dm_gear_tab_extra(text); }

void dm_gear_equip_tab_pump(uint8_t *b, uint64_t pl) {
    static int init; if (!init) { init = 1; dm_gear_equip_tab_init(); }

    static dm_gear_equip_cap cap; static dm_go_plan plan;
    if (!pthread_mutex_trylock(&mu)) {
        int req = g.req, worthy = g.rworthy; unsigned seq = g.seq; if (req) { cap = g.rcap; plan = g.rplan; g.req = 0; }
        pthread_mutex_unlock(&mu);
        if (req) {
            dm_gear_equip_worthy(worthy);
            int st = dm_gear_equip_start(b, pl, &cap, &plan);
            LOG("[gear-equip] tab: request %u start -> %d (%d steps, %d changes)", seq, st, plan.nstep, plan.nchange);
            atomic_store(&job_kind, st == 0 ? JOB_RUN : st == 1 || st == 4 ? JOB_CHANGED : st == 2 ? JOB_AWAY : JOB_BUSY);
            return;
        }
    }

    static int frame, rounds, soon; static unsigned gen;
    if (!dm_gear_tab_wanted()) { frame = 0; if (!pthread_mutex_trylock(&mu)) { g.known = 0; g.seen = 0; pthread_mutex_unlock(&mu); } return; }
    if (frame-- > 0) return;
    frame = soon ? CAPTURE_SOON : CAPTURE_FRAMES;
    static tg_t tg[EQ_MAX]; static int n;
    if (pthread_mutex_trylock(&mu)) return;
    gen = g.gen; n = g.n; memcpy(tg, g.tg, sizeof tg);
    pthread_mutex_unlock(&mu);
    static dm_gear_equip_cap caps[EQ_MAX]; static int crc[EQ_MAX];
    int open = dm_gear_equip_open(b), ready = dm_gear_equip_ready(b, pl); soon = 0;
    if (!(rounds++ % 8)) dm_gear_equip_fresh();
    dm_gear_equip_builds(BUILDS);
    for (int k = 0; k < n; k++) {
        crc[k] = !tg[k].ok ? -1 : !ready ? -4 : dm_gear_equip_capture_want(b, pl, &tg[k].t, &caps[k]);
        soon |= crc[k] == -2;
        static int said[EQ_MAX]; static unsigned said_gen;
        if (said_gen != gen) { said_gen = gen; for (int q = 0; q < EQ_MAX; q++) said[q] = 99; }
        if (said[k] != crc[k] && crc[k] != -2) { said[k] = crc[k]; LOG("[gear-equip] tab: target %d capture %d (%d items%s%s)", k, crc[k], crc[k] ? 0 : caps[k].w.nitem, crc[k] == -1 ? ", " : "", crc[k] == -1 ? dm_gear_equip_fail() : ""); }
    }
    dm_gear_equip_builds(-1);
    if (pthread_mutex_trylock(&mu)) return;
    g.seen = 1; g.open_now = open;
    if (g.gen == gen) { g.cgen = gen; g.known = 1; g.open = open; memcpy(g.crc, crc, sizeof crc); for (int k = 0; k < n; k++) if (!crc[k]) g.cap[k] = caps[k]; }
    pthread_mutex_unlock(&mu);
}
