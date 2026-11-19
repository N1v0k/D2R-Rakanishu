





#include "dm_fightlog.h"
#include "dm.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

static pthread_mutex_t g_mu = PTHREAD_MUTEX_INITIALIZER;
static uint8_t g_out[DM_FL_IMAGE_MAX], g_in[DM_FL_IMAGE_MAX + 1];
static uint64_t g_wrote;
static const char *const k_bad[] = { "", "size", "magic", "version", "checksum", "fight count", "fight record" };

static int path_of(char *out, size_t cap, const char *root, const char *who, const char *suffix) {
    return root && who && who[0] && !strchr(who, '/') && snprintf(out, cap, "%s/%s/%s.sw_fights%s", root, who, who, suffix) < (int)cap;
}


static long read_file(const char *p) {
    FILE *f = fopen(p, "rb"); if (!f) return -1;
    size_t n = fread(g_in, 1, sizeof g_in, f); if (ferror(f)) n = sizeof g_in;
    fclose(f); return (long)n;
}
static int load_one(dm_fl_state *s, const char *who, const char *p, size_t n) {
    char q[1220]; int bad = dm_fl_owner(s, who, g_in, n);
    if (bad) {
        snprintf(q, sizeof q, "%s.bad", p);
        int moved = !rename(p, q);
        LOG("ERROR fightlog.file_rejected: the fight log of this character failed its %s check; %s, the list starts empty", k_bad[bad], moved ? "set aside as .bad" : "could not be set aside");
    } else if (n) LOG("fight log: %d fights loaded", dm_fl_count(s));
    return bad;
}
int dm_fl_store_load(dm_fl_state *s, const char *root, const char *who) {
    char p[1200]; long n = -1;
    if (path_of(p, sizeof p, root, who, "2")) n = read_file(p);
    int bad = n >= 0 ? load_one(s, who, p, (size_t)n) : 0;
    if (n >= 0 && !bad) return 0;
    if (path_of(p, sizeof p, root, who, "")) n = read_file(p); else n = -1;
    if (n < 0) { if (!bad) dm_fl_owner(s, who, g_in, 0); return bad; }
    return load_one(s, who, p, (size_t)n);
}

int dm_fl_store_follow(dm_fl_state *s, const char *root, const char *who) {
    if (dm_fl_owned_by(s, who)) return 0;
    dm_fl_store_load(s, root, who);
    return 1;
}

int dm_fl_store_commit(dm_fl_state *s, const char *wd, uint64_t commits) {
    char who[64], p[1200], t[1220], dir[1200]; uint32_t gen; struct stat st; int written = 0;
    pthread_mutex_lock(&g_mu);
    if (g_wrote) { if (commits < g_wrote) dm_fl_unsaved(s); g_wrote = 0; }
    size_t n = wd ? dm_fl_image(s, who, g_out, sizeof g_out, &gen) : 0;
    if (n && path_of(p, sizeof p, wd, who, "2") && snprintf(dir, sizeof dir, "%s/%s", wd, who) < (int)sizeof dir) {
        static int missing;
        if (stat(dir, &st) || !S_ISDIR(st.st_mode)) { if (missing++ < 3) LOG("fight log: no character folder in this save yet, written with a later one"); }
        else {
            snprintf(t, sizeof t, "%s.tmp", p);
            FILE *f = fopen(t, "wb");
            int ok = f && fwrite(g_out, 1, n, f) == n;
            if (f && fclose(f)) ok = 0;
            if (ok && !rename(t, p)) { dm_fl_saved(s, gen); g_wrote = commits + 1; written = 1; }
            else { remove(t); LOG("ERROR fightlog.write_failed: the fight log could not be written; tried again with the next save"); }
        }
    }
    pthread_mutex_unlock(&g_mu);
    return written;
}
