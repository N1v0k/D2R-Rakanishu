








#include "dm.h"
#include "dm_looks_core.h"
#include "dm_looks.h"
#include <dirent.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

const char *dm_save_workdir(void);
uint64_t dm_fs_fail_count(void);
void dm_fs_save_poison(int e);
uint64_t dm_fs_d2s_work_seq(void); uint64_t dm_seq_next(void);
uint64_t dm_fs_commit_count(void);
void dm_ser_observe(uint64_t G); int dm_ser_state(void);

pthread_mutex_t g_looks_mu = PTHREAD_MUTEX_INITIALIZER;
dm_looks_store g_looks;
int g_looks_state;
static uint64_t g_written_rev;
static uint64_t g_sent_rev, g_sent_at;
static int g_sent_wrote;
static dm_look_pending *g_sent_done; static int g_sent_ndone;
static _Atomic int g_locked;
static char g_imports[8][64]; static int g_nimports;

static uint8_t *slurp(const char *path, size_t *n) {
    FILE *f = fopen(path, "rb"); if (!f) return NULL;
    uint8_t *d = NULL; size_t cap = 0; *n = 0;
    for (;;) {
        if (*n == cap) { size_t c = cap ? cap * 2 : 4096; if (c > (64u << 20)) { free(d); fclose(f); return NULL; } uint8_t *x = realloc(d, c); if (!x) { free(d); fclose(f); return NULL; } d = x; cap = c; }
        size_t r = fread(d + *n, 1, cap - *n, f); *n += r;
        if (r == 0) break;
    }
    int err = ferror(f); fclose(f);
    if (err) { free(d); return NULL; }
    return d;
}

static void sent_clear(void) { free(g_sent_done); g_sent_done = NULL; g_sent_ndone = 0; g_sent_at = 0; g_sent_wrote = 0; }


int dm_looks_latch(void) {
    if (g_locked) return 1;
    if (!dm_fs_fail_count()) return 0;
    LOG("ASSUME looks.save_failed: a save failed, changing looks is locked until restart (%d paid choices wait for the next save)", g_looks.npend);
    g_locked = 1;
    return 1;
}
int dm_looks_locked(void) { return g_locked; }

static const char *state_name(int st) { return st == DM_LOOKS_CORRUPT ? "damaged" : st == DM_LOOKS_NEWER ? "from a newer version" : "unreadable"; }


void dm_looks_load(void) {
    const char *wd = dm_save_workdir();
    dm_looks_store_free(&g_looks); g_nimports = 0; g_written_rev = 0; sent_clear(); g_locked = 0;
    if (!wd) { g_looks_state = DM_LOOKS_STATE_OFF; return; }
    char p[1100]; snprintf(p, sizeof p, "%s/" DM_LOOKS_FILE, wd);
    size_t n = 0; uint8_t *d = slurp(p, &n);
    if (!d && errno != ENOENT) { g_looks_state = DM_LOOKS_STATE_BROKEN; LOG("ASSUME looks.file: %s cannot be read (%d), looks stay original this session", DM_LOOKS_FILE, errno); return; }
    if (d) {
        int r = dm_looks_decode(d, n, &g_looks); free(d);
        if (r != DM_LOOKS_OK) {
            g_looks_state = r == DM_LOOKS_NEWER ? DM_LOOKS_STATE_NEWER : DM_LOOKS_STATE_BROKEN;
            LOG("ASSUME looks.file: %s is %s, looks stay original and the file is kept", DM_LOOKS_FILE, state_name(r));
            return;
        }
        g_written_rev = g_looks.rev;
    }
    g_looks_state = DM_LOOKS_STATE_OK;
    DIR *dir = opendir(wd); struct dirent *e;
    while (dir && (e = readdir(dir))) {
        if (strncmp(e->d_name, DM_LOOKS_IMPORT, strlen(DM_LOOKS_IMPORT)) || g_nimports >= 8 || strlen(e->d_name) >= sizeof g_imports[0]) continue;
        snprintf(p, sizeof p, "%s/%s", wd, e->d_name);
        dm_looks_store in; uint8_t *x = slurp(p, &n); int r = x ? dm_looks_decode(x, n, &in) : DM_LOOKS_CORRUPT; free(x);
        if (r != DM_LOOKS_OK) { LOG("looks: imported %s is %s, not merged and kept", e->d_name, state_name(r)); continue; }
        if (dm_looks_merge(&g_looks, &in)) LOG("looks: out of memory merging %s", e->d_name);
        else { strcpy(g_imports[g_nimports++], e->d_name); LOG("looks: merged %s (%d looks, %d items)", e->d_name, in.ncoll, in.nmap); }
        dm_looks_store_free(&in);
    }
    if (dir) closedir(dir);
    LOG("looks: %d looks collected, %d items dressed (%s)", g_looks.ncoll, g_looks.nmap, d ? "file" : "new");
}


int dm_looks_start_anew(void) {
    pthread_mutex_lock(&g_looks_mu);
    int ok = -1; const char *wd = dm_save_workdir();
    if (wd && (g_looks_state == DM_LOOKS_STATE_BROKEN || g_looks_state == DM_LOOKS_STATE_NEWER)) {
        char p[1100], q[1200];
        snprintf(p, sizeof p, "%s/" DM_LOOKS_FILE, wd);
        for (int i = 1; i < 100; i++) { struct stat st; snprintf(q, sizeof q, "%s.kept-%d", p, i); if (stat(q, &st)) break; }
        if (!rename(p, q) || errno == ENOENT) {
            dm_looks_store_free(&g_looks); g_written_rev = 0; sent_clear(); g_looks.rev = 1;
            g_looks_state = DM_LOOKS_STATE_OK; ok = 0;
            LOG("looks: started anew, the old file is kept as %s", strrchr(q, '/') + 1);
        }
    }
    pthread_mutex_unlock(&g_looks_mu);
    return ok;
}

static int write_file(const char *wd, const dm_looks_store *s) {
#ifndef DM_PLAYER_BUILD
    { static int nf = -1; if (nf < 0) { const char *e = getenv("DM_TEST_LOOKS_SAVE_FAIL"); nf = e ? atoi(e) : 0; } if (nf > 0) { nf--; LOG("looks: DM_TEST_LOOKS_SAVE_FAIL makes this write fail"); return -1; } }
#endif
    uint8_t *b; size_t n = dm_looks_encode(s, &b);
    if (!n) return -1;
    char p[1100], t[1110]; snprintf(p, sizeof p, "%s/" DM_LOOKS_FILE, wd); snprintf(t, sizeof t, "%s.tmp", p);
    FILE *f = fopen(t, "wb"); int ok = f && fwrite(b, 1, n, f) == n;
    if (f) ok = fclose(f) == 0 && ok;
    free(b);
    if (!ok || rename(t, p)) { remove(t); return -1; }
    return 0;
}



static void settle(void) {
    if (!g_sent_at) return;
    if (dm_fs_commit_count() >= g_sent_at) {
        uint64_t before = g_looks.rev;
        if (dm_looks_settle(&g_looks, g_sent_done, g_sent_ndone)) LOG("looks: out of memory committing the choices of the last save");
        if (g_sent_wrote) { g_written_rev = before == g_sent_rev ? g_looks.rev : g_sent_rev; g_nimports = 0; }
    }
    sent_clear();
}

void dm_looks_settle_committed(void) { if (g_sent_at && dm_fs_commit_count() >= g_sent_at) settle(); }




int dm_looks_pay_proof(uint64_t G, uint64_t *seq) {
    if (dm_looks_latch()) return -2;
    dm_ser_observe(G);
    if (dm_ser_state()) return 0;
    *seq = dm_seq_next();
    return 1;
}


static void looks_pre_commit(const char *wd) {
    pthread_mutex_lock(&g_looks_mu);
    dm_looks_latch();
    settle();
    if (g_looks_state == DM_LOOKS_STATE_OK) {
        char p[1200];
        int np = g_looks.npend;
        g_sent_done = malloc(sizeof *g_sent_done * (size_t)(np ? np : 1));
        if (!g_sent_done) { dm_fs_save_poison(ENOMEM); LOG("looks: out of memory, this save is refused"); pthread_mutex_unlock(&g_looks_mu); return; }
        g_sent_ndone = dm_looks_promotable(&g_looks, dm_fs_d2s_work_seq(), g_sent_done, np);
        g_sent_at = dm_fs_commit_count() + 1;
        if (g_sent_ndone || g_looks.rev != g_written_rev || g_nimports) {

            dm_looks_store t = { 0 }; uint8_t *b = NULL; size_t n = dm_looks_encode(&g_looks, &b);
            int bad = !n || dm_looks_decode(b, n, &t) != DM_LOOKS_OK || dm_looks_settle(&t, g_sent_done, g_sent_ndone) || write_file(wd, &t);
            free(b);
            if (bad) {
                dm_fs_save_poison(EIO);
                LOG("looks: %s not written, this save is refused and tried again", DM_LOOKS_FILE);
            } else {
                for (int i = 0; i < g_nimports; i++) { snprintf(p, sizeof p, "%s/%s", wd, g_imports[i]); remove(p); }
                g_sent_wrote = 1; g_sent_rev = g_looks.rev;
                LOG("looks: written with this save (%d looks, %d items, %d paid now, %d waiting)", t.ncoll, t.nmap, g_sent_ndone, g_looks.npend - g_sent_ndone);
            }
            dm_looks_store_free(&t);
        }
    }
    pthread_mutex_unlock(&g_looks_mu);
}
__attribute__((constructor)) static void looks_hook_install(void) { extern void (*g_dm_pre_commit_hook[DM_PRE_COMMIT_HOOKS])(const char *); g_dm_pre_commit_hook[7] = looks_pre_commit; }
