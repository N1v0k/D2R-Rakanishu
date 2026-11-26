











#include "dm_glossary.h"
#include "game_addrs.h"
#include "dm_build.h"
#include <pthread.h>
#include <stdatomic.h>
#include <string.h>

void dm_itemcmp_sheet(uint8_t *b, uint64_t pl, int *def, int *block, int *ar);

static pthread_mutex_t g_mu = PTHREAD_MUTEX_INITIALIZER;
static int g_snap[DM_GLOSS_INTS], g_gen;
static _Atomic int g_want;
static const int k_stats[DM_GL_NSTATS] = DM_GL_STAT_IDS;

void dm_glossary_want(int on) { atomic_store(&g_want, on ? 1 : 0); }

static int gl_lstat(uint8_t *b, uint64_t u, int st) {
    uint64_t l = *(uint64_t *)(uintptr_t)(u + 0x88);
    return l ? ((int (*)(uint8_t, uint64_t, int, int))(uintptr_t)(b + GA_STATLIST_GET))(*(uint8_t *)(uintptr_t)(u + 0x1bd), l, st, 0) : 0;
}

static void invalidate(void) { pthread_mutex_lock(&g_mu); g_snap[DM_GL_VALID] = 0; pthread_mutex_unlock(&g_mu); }

void dm_glossary_pump(uint8_t *b, uint64_t pl) {
    static int was, tick;
    int want = atomic_load(&g_want);
    if (!want) { if (was) invalidate(); was = 0; return; }
    if (!b || !pl || !*(uint64_t *)(uintptr_t)(pl + 0x10) || !*(uint64_t *)(uintptr_t)(pl + 0x88)) { invalidate(); was = 0; return; }
    if (was && ++tick < 10) return;
    was = 1; tick = 0;
    int (*gs)(uint64_t, int, int) = (int (*)(uint64_t, int, int))(uintptr_t)(b + GA_STAT_GET);
    int s[DM_GLOSS_INTS] = {0};
    s[DM_GL_VER] = DM_GLOSS_VERSION; s[DM_GL_VALID] = 1; s[DM_GL_SESSION] = dm_build_session();
    s[DM_GL_CLASS] = (int)*(uint32_t *)(uintptr_t)(pl + 4); s[DM_GL_LEVEL] = gs(pl, 12, 0);
    uint8_t diff = *(uint8_t *)(uintptr_t)(b + GA_DIFFICULTY); s[DM_GL_DIFF] = diff <= 2 ? diff : -1;
    uint64_t gi = *(uint64_t *)(uintptr_t)(b + GA_GAME_INFO); s[DM_GL_EXPANSION] = gi ? *(uint8_t *)(uintptr_t)(gi + 0x5c) != 1 : -1;
    dm_itemcmp_sheet(b, pl, &s[DM_GL_DEF], &s[DM_GL_BLOCK], &s[DM_GL_AR]);
    s[DM_GL_AC] = gs(pl, 31, 0); s[DM_GL_DEX] = gs(pl, 2, 0); s[DM_GL_SKILLPCT] = gl_lstat(b, pl, 171); s[DM_GL_ACPCT] = s[DM_GL_SKILLPCT] + gl_lstat(b, pl, 16); s[DM_GL_ACOVR] = gs(pl, 182, 0);
    uint64_t l = *(uint64_t *)(uintptr_t)(pl + 0x88);
    s[DM_GL_STATE101] = l && *(int32_t *)(uintptr_t)(l + 0x1c) < 0 && (*(uint8_t *)(uintptr_t)(l + 0xb3c) & 0x20);


    s[DM_GL_ARMOR_SHRINE] = l && *(int32_t *)(uintptr_t)(l + 0x1c) < 0 && (*(uint32_t *)(uintptr_t)(l + 0xb30 + 4 * (128 >> 5)) >> (128 & 31) & 1);
    for (int i = 0; i < DM_GL_NSTATS; i++) s[DM_GL_STATS + i] = gs(pl, k_stats[i], 0);
    pthread_mutex_lock(&g_mu); s[DM_GL_GEN] = ++g_gen; memcpy(g_snap, s, sizeof s); pthread_mutex_unlock(&g_mu);
}

int dm_glossary_copy(int *out, int n) {
    if (!out || n <= 0) return 0;
    int k = n < DM_GLOSS_INTS ? n : DM_GLOSS_INTS;
    pthread_mutex_lock(&g_mu); memcpy(out, g_snap, (size_t)k * sizeof *out); pthread_mutex_unlock(&g_mu);
    out[DM_GL_VER] = DM_GLOSS_VERSION;
    return k;
}
