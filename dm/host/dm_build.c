





























#include "dm_build.h"
#include "hle.h"
#include <pthread.h>
#include <stdatomic.h>
#include <string.h>

int dm_skill_cap(void);
int dm_rift_cleared_any(void);
int dm_skill_base(uint64_t pl, int skill);

static pthread_mutex_t g_mu = PTHREAD_MUTEX_INITIALIZER;
static int g_snap[DM_BUILD_INTS], g_nsnap, g_session, g_valid;
static char g_name[DM_BUILD_NAME + 1];
static int g_req[DM_BUILD_MAX_SKILLS], g_nreq, g_reqgen;
static int g_tgt[6];
static _Atomic int g_open;
static char g_card[DM_BUILD_CARD_PARTS][DM_BUILD_CARD_TEXT];
static int g_card_s, g_card_to, g_card_at, g_card_st = -1;
static int g_keys[DM_BUILD_KEYS], g_nkeys, g_keys_s;
static int g_stat[3];
static _Atomic int g_free_stats;



static int base_stat(uint64_t pl, int id) {
    uint64_t sl = *(uint64_t *)(uintptr_t)(pl + 0x88); if (!sl) return -1;
    const uint32_t *a = (const uint32_t *)(uintptr_t)*(uint64_t *)(uintptr_t)(sl + 0x30); uint64_t n = *(uint64_t *)(uintptr_t)(sl + 0x38);
    if (n > 4096 || (n && !a)) return -1;
    for (uint64_t i = 0; i < n; i++) if (a[2 * i] == (uint32_t)id << 16) return (int)a[2 * i + 1];
    return 0;
}

int dm_skill_reason(uint8_t *b, uint64_t pl, int level, int skill, int base) {
    uint8_t ver = *(uint8_t *)(uintptr_t)(pl + 0x1bd); if (ver < 1 || ver > 3 || skill < 0) return 7;
    uint64_t t = *(uint64_t *)(uintptr_t)(b + 0x2d64a18 + (uint64_t)ver * 16); if (!t || (uint64_t)skill >= *(uint64_t *)(uintptr_t)(t + 0x11f0)) return 7;
    uint64_t rec = *(uint64_t *)(uintptr_t)(t + 0x11e8) + (uint64_t)skill * 0x2ec;
    int cap = *(int16_t *)(uintptr_t)(rec + 0x184); if (cap < 1) cap = 20;
    if (base >= cap) return cap == 20 && dm_skill_cap() > 20 && !dm_rift_cleared_any() && base < dm_skill_cap() ? 5 : 6;
    if (level < *(int16_t *)(uintptr_t)(rec + 0x214) + base) return 2;
    for (int i = 0; i < 3; i++) { int q = *(int16_t *)(uintptr_t)(rec + 0x21e + 2 * i); if (q >= 0 && dm_skill_base(pl, q) < 1) return 3; }
    return 0;
}

void dm_build_pump(uint8_t *b, uint64_t pl) {
    static uint64_t last_pl; static char last_name[DM_BUILD_NAME + 1]; static unsigned tick;
    uint64_t pd = b && pl ? *(uint64_t *)(uintptr_t)(pl + 0x10) : 0;
    if (!pd) {
        pthread_mutex_lock(&g_mu); if (g_valid) { g_valid = 0; LOG("构建指南：快照作废（没有玩家）"); } pthread_mutex_unlock(&g_mu);
        last_pl = 0; atomic_store(&g_free_stats, 0); return;
    }
    char nm[DM_BUILD_NAME + 1]; memcpy(nm, (const char *)(uintptr_t)pd, DM_BUILD_NAME); nm[DM_BUILD_NAME] = 0;
    int changed = pl != last_pl || strcmp(nm, last_name);
    pthread_mutex_lock(&g_mu); int was = g_valid; pthread_mutex_unlock(&g_mu);
    atomic_store(&g_free_stats, ((int (*)(uint64_t, int, int))(b + 0x5b9290))(pl, 4, 0));
    if (was && !changed && tick++ % 6) return;
    int ids[DM_BUILD_MAX_SKILLS], n, gen;
    pthread_mutex_lock(&g_mu); n = g_nreq; gen = g_reqgen; memcpy(ids, g_req, sizeof ids); pthread_mutex_unlock(&g_mu);
    int (*stat)(uint64_t, int, int) = (int (*)(uint64_t, int, int))(b + 0x5b9290);
    int s[DM_BUILD_INTS] = {0}, level = stat(pl, 12, 0);
    s[0] = 1; s[2] = (int)*(uint32_t *)(uintptr_t)(pl + 4); s[3] = level; s[4] = *(uint8_t *)(uintptr_t)(pl + 0x1bd);
    s[5] = stat(pl, 4, 0); s[6] = stat(pl, 5, 0);
    static const int attr[4] = { 0, 2, 3, 1 };
    for (int i = 0; i < 4; i++) { s[7 + i] = stat(pl, attr[i], 0); s[13 + i] = base_stat(pl, attr[i]); }
    s[11] = gen; s[12] = n;
    for (int i = 0; i < n; i++) { int base = dm_skill_base(pl, ids[i]); s[DM_BUILD_HEAD + 3 * i] = ids[i]; s[DM_BUILD_HEAD + 3 * i + 1] = base; s[DM_BUILD_HEAD + 3 * i + 2] = dm_skill_reason(b, pl, level, ids[i], base); }
    pthread_mutex_lock(&g_mu);
    if (!g_valid || changed) { g_session++; LOG("构建指南：会话 %d，角色「%s」职业 %d 等级 %d", g_session, nm, s[2], level); }
    s[1] = g_session; memcpy(g_snap, s, sizeof s); g_nsnap = DM_BUILD_HEAD + 3 * n; memcpy(g_name, nm, sizeof g_name); g_valid = 1;
    pthread_mutex_unlock(&g_mu);
    last_pl = pl; memcpy(last_name, nm, sizeof last_name);
}

int dm_build_state(int *out, int n, char *name, int name_cap, int *name_len) {
    pthread_mutex_lock(&g_mu);
    int k = g_valid ? (g_nsnap < n ? g_nsnap : n) : (n < DM_BUILD_HEAD ? n : DM_BUILD_HEAD);
    if (g_valid) memcpy(out, g_snap, sizeof(int) * (size_t)k); else memset(out, 0, sizeof(int) * (size_t)k);
    int l = g_valid ? (int)strlen(g_name) : 0; if (l > name_cap) l = name_cap;
    if (l > 0) memcpy(name, g_name, (size_t)l);
    *name_len = l;
    pthread_mutex_unlock(&g_mu);
    return k;
}

int dm_build_skills(const int *ids, int n) {
    if (n > DM_BUILD_MAX_SKILLS) n = DM_BUILD_MAX_SKILLS;
    if (n < 0) n = 0;
    pthread_mutex_lock(&g_mu);
    int same = n == g_nreq && !memcmp(ids, g_req, sizeof(int) * (size_t)n);
    if (!same) { memcpy(g_req, ids, sizeof(int) * (size_t)n); g_nreq = n; g_reqgen++; }
    int gen = g_reqgen;
    pthread_mutex_unlock(&g_mu);
    return gen;
}

int dm_build_session(void) { pthread_mutex_lock(&g_mu); int s = g_valid ? g_session : 0; pthread_mutex_unlock(&g_mu); return s; }

void dm_build_target(int session, int page, int row, int col, int show, int guide) {
    pthread_mutex_lock(&g_mu); g_tgt[0] = session; g_tgt[1] = page; g_tgt[2] = row; g_tgt[3] = col; g_tgt[4] = show; g_tgt[5] = guide; pthread_mutex_unlock(&g_mu);
}

void dm_build_target_get(int *session, int *page, int *row, int *col, int *show, int *guide) {
    pthread_mutex_lock(&g_mu); *session = g_tgt[0]; *page = g_tgt[1]; *row = g_tgt[2]; *col = g_tgt[3]; *show = g_tgt[4]; *guide = g_tgt[5]; pthread_mutex_unlock(&g_mu);
}

void dm_build_open_request(void) { int s = dm_build_session(); if (s) atomic_store(&g_open, s << 8 | 1); }
int dm_build_open_take(void) { return atomic_exchange(&g_open, 0); }



void dm_build_on_uimsg(const char *cat, const char *act, const char *arg) {
    if (strcmp(cat, "PanelManager") || strcmp(act, "ClosePanel") || strcmp(arg, "TouchBuildOpen")) return;
    dm_build_open_request(); LOG("构建指南：技能树里点了「指南 ›」");
}


static void utf8_trim(char *s, int n) {
    int i = n; while (i > 0 && ((unsigned char)s[i - 1] & 0xc0) == 0x80) i--;
    if (i == 0) { s[0] = 0; return; }
    unsigned char c = (unsigned char)s[i - 1]; int need = c < 0x80 ? 1 : (c & 0xe0) == 0xc0 ? 2 : (c & 0xf0) == 0xe0 ? 3 : (c & 0xf8) == 0xf0 ? 4 : 0;
    if (need == 0 || n - (i - 1) < need) s[i - 1] = 0; else s[n] = 0;
}

void dm_build_card(int session, const char *text, int len, int to, int lvl, int state) {
    char part[DM_BUILD_CARD_PARTS][DM_BUILD_CARD_TEXT]; memset(part, 0, sizeof part);
    int k = 0, at = 0;
    for (int i = 0; text && i < len && k < DM_BUILD_CARD_PARTS; i++) {
        if (!text[i]) { utf8_trim(part[k], at); k++; at = 0; continue; }
        if (at < DM_BUILD_CARD_TEXT - 1) part[k][at++] = text[i];
    }
    if (k < DM_BUILD_CARD_PARTS) utf8_trim(part[k], at);
    pthread_mutex_lock(&g_mu); memcpy(g_card, part, sizeof g_card); g_card_s = state < 0 ? 0 : session; g_card_to = to; g_card_at = lvl; g_card_st = state; pthread_mutex_unlock(&g_mu);
}

int dm_build_card_get(char out[DM_BUILD_CARD_PARTS][DM_BUILD_CARD_TEXT], int *to, int *at, int *state) {
    pthread_mutex_lock(&g_mu); memcpy(out, g_card, sizeof g_card); int s = g_card_s; *to = g_card_to; *at = g_card_at; *state = g_card_st; pthread_mutex_unlock(&g_mu); return s;
}

void dm_build_stat(int session, int stat, int show) { pthread_mutex_lock(&g_mu); g_stat[0] = session; g_stat[1] = stat; g_stat[2] = show; pthread_mutex_unlock(&g_mu); }
void dm_build_stat_get(int *session, int *stat, int *show) { pthread_mutex_lock(&g_mu); *session = g_stat[0]; *stat = g_stat[1]; *show = g_stat[2]; pthread_mutex_unlock(&g_mu); }
void dm_build_keys(int session, const int *ids, int n) {
    if (n < 0 || !ids) n = 0; if (n > DM_BUILD_KEYS) n = DM_BUILD_KEYS;
    pthread_mutex_lock(&g_mu); int same = g_nkeys == n && g_keys_s == (n ? session : 0) && !memcmp(g_keys, ids, (size_t)n * sizeof *ids);
    memcpy(g_keys, ids, (size_t)n * sizeof *ids); g_nkeys = n; g_keys_s = n ? session : 0; pthread_mutex_unlock(&g_mu);
    if (!same) LOG("构建指南：关键技能（会话 %d）%d 个：%d %d %d %d …", session, n, n > 0 ? ids[0] : -1, n > 1 ? ids[1] : -1, n > 2 ? ids[2] : -1, n > 3 ? ids[3] : -1);
}
int dm_build_keys_get(int *ids, int max) {
    pthread_mutex_lock(&g_mu); int n = g_valid && g_keys_s == g_session ? g_nkeys : 0; if (n > max) n = max; memcpy(ids, g_keys, (size_t)(n > 0 ? n : 0) * sizeof *ids); pthread_mutex_unlock(&g_mu); return n;
}
int dm_build_free_stats(void) { return atomic_load(&g_free_stats); }

int dm_custom_get(const char *key);
int dm_build_hints(void) { return dm_custom_get("build_hints") != 0; }
