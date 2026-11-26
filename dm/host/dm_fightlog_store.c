





#include "dm_fightlog.h"
#include "dm.h"
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

static pthread_mutex_t g_mu = PTHREAD_MUTEX_INITIALIZER;
static uint8_t g_out[DM_FL_IMAGE_MAX], g_in[DM_FL_IMAGE_MAX + 1];
static uint64_t g_wrote;


static char g_pend_who[64]; static int g_pend[2];
static char g_block_who[64];
static const char *const k_aside[2] = { "2.bad", ".bad" };
static const char *const k_bad[] = { "", "size", "magic", "version", "checksum", "fight count", "fight record" };

static int path_of(char *out, size_t cap, const char *root, const char *who, const char *suffix) {
    return root && who && who[0] && !strchr(who, '/') && snprintf(out, cap, "%s/%s/%s.sw_fights%s", root, who, who, suffix) < (int)cap;
}
static int is_file(const char *p) { struct stat st; return !stat(p, &st) && S_ISREG(st.st_mode); }


static long read_file(const char *p) {
    FILE *f = fopen(p, "rb"); if (!f) return -1;
    size_t n = fread(g_in, 1, sizeof g_in, f); if (ferror(f)) n = sizeof g_in;
    fclose(f); return (long)n;
}

static int set_aside(const char *p, const char *suffix, char *q, size_t cap) {
    struct stat st;
    for (int k = 1; k <= 99; k++) {
        if ((k == 1 ? snprintf(q, cap, "%s%s", p, suffix) : snprintf(q, cap, "%s%s.%d", p, suffix, k)) >= (int)cap) return 0;
        if (stat(q, &st) && errno == ENOENT) return !rename(p, q);
    }
    return 0;
}
static int load_one(dm_fl_state *s, const char *who, const char *p, size_t n) {
    char q[1220]; int skipped = 0, bad = dm_fl_owner(s, who, g_in, n, &skipped);
    if (bad) {
        int moved = set_aside(p, ".bad", q, sizeof q);
        if (!moved) snprintf(g_block_who, sizeof g_block_who, "%s", who);
        LOG("ERROR fightlog.file_rejected: the fight log of this character failed its %s check; %s, %d fights before the damage kept", k_bad[bad],
            moved ? "set aside as .bad" : "could not be set aside and is not written over", dm_fl_count(s));
    } else if (n) LOG("fight log: %d fights loaded", dm_fl_count(s));
    if (skipped) LOG("ERROR fightlog.record_skipped: %d fight records of this character failed their check and were left out", skipped);
    return bad;
}



static void merge_aside(dm_fl_state *s, const char *root, const char *who, int k, int newer) {
    char p[1200], q[1220]; int added = 0, skipped = 0;
    if (!path_of(p, sizeof p, root, who, k_aside[k])) return;
    long n = read_file(p);
    int bad = n < 0 ? DM_FL_BAD_SIZE : dm_fl_merge(s, g_in, (size_t)n, newer, &added, &skipped);
    if (bad) LOG("ERROR fightlog.aside_rejected: a fight log set aside earlier failed its %s check; %d fights recovered from it", k_bad[bad], added);
    else LOG("fight log: %d fights recovered from a file set aside earlier (%d records left out)", added, skipped);
    if (added) { snprintf(g_pend_who, sizeof g_pend_who, "%s", who); g_pend[k] = 1; }
    else if (!set_aside(p, ".merged", q, sizeof q)) LOG("ERROR fightlog.aside_kept: a fight log set aside earlier could not be renamed; read again next time");
}
int dm_fl_store_load(dm_fl_state *s, const char *root, const char *who) {
    char p[1200]; long n = -1; int had[2], v2 = 0;
    pthread_mutex_lock(&g_mu);
    g_pend_who[0] = 0; g_pend[0] = g_pend[1] = 0;
    if (who && !strcmp(g_block_who, who)) g_block_who[0] = 0;
    for (int k = 0; k < 2; k++) had[k] = path_of(p, sizeof p, root, who, k_aside[k]) && is_file(p);
    if (path_of(p, sizeof p, root, who, "2")) n = read_file(p);
    int bad = n >= 0 ? load_one(s, who, p, (size_t)n) : 0;
    if (n < 0 || (bad && !dm_fl_count(s))) {
        long n1 = path_of(p, sizeof p, root, who, "") ? read_file(p) : -1;
        if (n1 >= 0) bad = load_one(s, who, p, (size_t)n1);
        else if (n < 0) dm_fl_owner(s, who, g_in, 0, NULL);
    } else v2 = 1;
    if (had[0]) merge_aside(s, root, who, 0, !v2);
    if (had[1]) merge_aside(s, root, who, 1, 0);
    pthread_mutex_unlock(&g_mu);
    return bad;
}

int dm_fl_store_follow(dm_fl_state *s, const char *root, const char *who) {
    if (dm_fl_owned_by(s, who)) return 0;
    dm_fl_store_load(s, root, who);
    return 1;
}

int dm_fl_store_commit(dm_fl_state *s, const char *wd, uint64_t commits) {
    char who[64], p[1200], t[1220], dir[1200]; uint32_t gen; struct stat st; int written = 0;
    pthread_mutex_lock(&g_mu);
    if (g_wrote) {
        if (commits < g_wrote) dm_fl_unsaved(s);
        else { g_pend_who[0] = 0; g_pend[0] = g_pend[1] = 0; }
        g_wrote = 0;
    }
    size_t n = wd ? dm_fl_image(s, who, g_out, sizeof g_out, &gen) : 0;
    if (n && g_block_who[0] && !strcmp(who, g_block_who)) {
        static int said;
        if (!said++) LOG("ERROR fightlog.write_blocked: a rejected fight log of this character could not be set aside; it is not written over");
        n = 0;
    }
    if (n && path_of(p, sizeof p, wd, who, "2") && snprintf(dir, sizeof dir, "%s/%s", wd, who) < (int)sizeof dir) {
        static int missing;
        if (stat(dir, &st) || !S_ISDIR(st.st_mode)) { if (missing++ < 3) LOG("fight log: no character folder in this save yet, written with a later one"); }
        else {
            snprintf(t, sizeof t, "%s.tmp", p);
            FILE *f = fopen(t, "wb");
            int ok = f && fwrite(g_out, 1, n, f) == n;
            if (f && fclose(f)) ok = 0;
            if (ok && !rename(t, p)) {
                dm_fl_saved(s, gen); g_wrote = commits + 1; written = 1;

                for (int k = 0; k < 2; k++) if (g_pend[k] && !strcmp(who, g_pend_who) && path_of(p, sizeof p, wd, who, k_aside[k]) && is_file(p) && !set_aside(p, ".merged", t, sizeof t))
                    LOG("ERROR fightlog.aside_kept: a fight log set aside earlier could not be renamed; read again next time");
            }
            else { remove(t); LOG("ERROR fightlog.write_failed: the fight log could not be written; tried again with the next save"); }
        }
    }
    pthread_mutex_unlock(&g_mu);
    return written;
}
