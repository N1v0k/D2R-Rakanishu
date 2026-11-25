






#include "../host/dm.h"
#include "player_guard.h"
#include <errno.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <unistd.h>

FILE *__real_fopen(const char *path, const char *mode);

static char g_dir[1100];
static atomic_int g_dir_set;
static pthread_mutex_t g_told_mu = PTHREAD_MUTEX_INITIALIZER;
static char g_told[64][64];
static int g_ntold;

void dm_player_guard_set_dir(const char *dir) {
    snprintf(g_dir, sizeof g_dir, "%s", dir);
    atomic_store_explicit(&g_dir_set, 1, memory_order_release);
}


static void told(const char *name) {
    int fresh = 1;
    pthread_mutex_lock(&g_told_mu);
    for (int i = 0; i < g_ntold && fresh; i++) fresh = strcmp(g_told[i], name) != 0;
    if (fresh && g_ntold < 64) snprintf(g_told[g_ntold++], sizeof g_told[0], "%s", name);
    pthread_mutex_unlock(&g_told_mu);
    if (fresh) LOG("boot: variant player ignores marker %s", name);
}

FILE *__wrap_fopen(const char *path, const char *mode) {
    if (path && atomic_load_explicit(&g_dir_set, memory_order_acquire)) {
        const char *name = dm_player_marker_blocked(g_dir, path);
        if (name) { if (!access(path, F_OK)) told(name); errno = ENOENT; return NULL; }
    }
    return __real_fopen(path, mode);
}
