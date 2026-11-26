




























#include "dm_effects.h"
#include "game_addrs.h"
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "dm.h"

static uint64_t dm_main_base(void) { for (int i = 0; i < g_nmod; i++) if (!strcmp(g_mod[i].name, "main")) return (uint64_t)g_mod[i].base; return 0; }

enum { STATES = 232, MAX_LISTS = 256, MAX_STATS_IN = 256, EVERY = 5 };
enum { TIMED = 0x8002 };
enum { MANA_WORDS = DM_FX_MANA_LEN };
typedef struct { int state, skill, level, ticks, lists, n; int stat[DM_EFFECTS_STATS][3]; } entry_t;
static pthread_mutex_t g_mu = PTHREAD_MUTEX_INITIALIZER;
static entry_t g_out[DM_EFFECTS_MAX];
static int g_n, g_gen, g_mana[MANA_WORDS];
static uint32_t g_tick;

static int g_player[DM_FX_PLAYER_LEN], g_skills[DM_FX_SKILLS_LEN], g_combat[DM_FX_COMBAT_LEN];
static uint64_t g_player_ms, g_skills_ms, g_combat_ms;
static const int k_player_stats[DM_FXP_NSTATS] = DM_FX_PLAYER_STAT_IDS;
static unsigned g_dropped;

int dm_guest_heap_readable(uint64_t a, size_t n);
static int rd(uint64_t a, void *buf, size_t n) {
    if (!a || !dm_guest_heap_readable(a, n)) return 0;
    memcpy(buf, (const void *)(uintptr_t)a, n);
    return 1;
}

static uint64_t mono_ms(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return (uint64_t)t.tv_sec * 1000 + (uint64_t)t.tv_nsec / 1000000; }
static uint64_t (*g_now_ms)(void) = mono_ms;


static int age_ms(uint64_t at, uint64_t now) { return !at ? 0x7fffffff : now <= at ? 0 : now - at >= 0x7fffffff ? 0x7fffffff : (int)(now - at); }

int dm_effects_copy(int *out, int max) {
    if (!out || max < DM_EFFECTS_HEAD) return 0;
    uint64_t now = g_now_ms();
    pthread_mutex_lock(&g_mu);
    int w = DM_EFFECTS_HEAD;
    memset(out, 0, DM_EFFECTS_HEAD * sizeof *out);
    out[DM_FX_GEN] = g_gen; out[DM_FX_COUNT] = 0; out[DM_FX_TICK] = (int)g_tick; out[DM_FX_VERSION] = DM_EFFECTS_VERSION; memcpy(out + DM_FX_MANA, g_mana, sizeof g_mana);
    memcpy(out + DM_FX_PLAYER, g_player, sizeof g_player); out[DM_FX_PLAYER + DM_FXP_AGE] = age_ms(g_player_ms, now);
    memcpy(out + DM_FX_SKILLS, g_skills, sizeof g_skills); out[DM_FX_SKILLS + DM_FXS_AGE] = age_ms(g_skills_ms, now);
    memcpy(out + DM_FX_COMBAT, g_combat, sizeof g_combat);
    out[DM_FX_COMBAT + DM_FXC_AGE] = age_ms(g_combat_ms, now); out[DM_FX_COMBAT + DM_FXC_COMBAT + DM_FXC_AGE] = age_ms(g_combat_ms, now);
    for (int i = 0; i < g_n && w + DM_FX_ENTRY + g_out[i].n * 3 <= max; i++, out[DM_FX_COUNT]++) {
        const entry_t *e = &g_out[i];
        out[w++] = e->state; out[w++] = e->skill; out[w++] = e->level; out[w++] = e->ticks; out[w++] = e->lists; out[w++] = e->n;
        for (int k = 0; k < e->n; k++) { memcpy(out + w, e->stat[k], sizeof e->stat[k]); w += 3; }
    }
    pthread_mutex_unlock(&g_mu);
    return w;
}

static entry_t *find(entry_t *t, int *n, int state) {
    for (int i = 0; i < *n; i++) if (t[i].state == state) return &t[i];
    if (*n >= DM_EFFECTS_MAX) return NULL;
    entry_t *e = &t[(*n)++];
    memset(e, 0, sizeof *e); e->state = state; e->skill = -1; e->ticks = -1;
    return e;
}



static int mana_seconds(const uint8_t *b, uint64_t G, uint64_t player) {
    uint8_t ver = 0; int32_t cls = -1; uint64_t rows = 0, count = 0; uint8_t s = 0;
    if (!rd(G + 0x106, &ver, 1) || ver < 1 || ver > 3 || !rd(player + 4, &cls, 4) || cls < 0) return 0;
    uint64_t t = *(const uint64_t *)(b + GA_DATA_TABLES + 16 * (uint64_t)ver);
    if (!rd(t + 0x1280, &count, 8) || (uint64_t)cls >= count || !rd(t + 0x1278, &rows, 8) || !rd(rows + 0xd4 * (uint64_t)cls + 0x36, &s, 1)) return 0;
    return s ? s : 300;
}

static void read_mana(const uint8_t *b, uint64_t G, uint64_t player, const uint32_t *bits, int *mana) {
    int (*gs)(uint64_t, int, int) = (int (*)(uint64_t, int, int))(uintptr_t)(b + GA_STAT_GET);
    int sec = mana_seconds(b, G, player), base = sec ? gs(player, 9, 0) / (sec * 25) : 0;
    if (sec && base < 1) base = 1;
    if (bits[85 >> 5] >> (85 & 31) & 1) base = 0;
    mana[0] = base; mana[1] = gs(player, 27, 0); mana[2] = gs(player, 26, 0); mana[3] = gs(player, 74, 0);
}

static void add_stats(entry_t *e, uint64_t arr, uint32_t cnt) {
    static uint32_t pairs[MAX_STATS_IN][2];
    if (!arr || !cnt) return;
    if (cnt > MAX_STATS_IN) cnt = MAX_STATS_IN;
    if (!rd(arr, pairs, (size_t)cnt * 8)) return;
    for (uint32_t i = 0; i < cnt; i++) {
        int stat = pairs[i][0] >> 16, layer = pairs[i][0] & 0xffff, value = (int32_t)pairs[i][1], k;
        for (k = 0; k < e->n; k++) if (e->stat[k][0] == stat && e->stat[k][1] == layer) { e->stat[k][2] += value; break; }
        if (k < e->n) continue;
        if (e->n < DM_EFFECTS_STATS) { e->stat[e->n][0] = stat; e->stat[e->n][1] = layer; e->stat[e->n][2] = value; e->n++; }
        else g_dropped++;
    }
}



static int ticks_left(float expiry, uint32_t tick) {
    if (!isfinite(expiry)) return -1;
    double left = (double)expiry - (double)tick;
    return left <= 0 ? 0 : left >= 0x7fffffff ? 0x7fffffff : (int)left;
}

static int read_effects(const uint8_t *b, uint64_t G, uint64_t player, entry_t *t, uint32_t *tick, int *mana) {
    uint64_t sl = 0; uint32_t flags = 0, bits[(STATES + 31) / 32];
    if (!rd(player + 0x88, &sl, 8) || !sl || !rd(sl + 0x1c, &flags, 4) || !(flags >> 31) || !rd(sl + 0xb30, bits, sizeof bits) || !rd(G + 0x170, tick, 4)) return -1;
    read_mana(b, G, player, bits, mana);
    static uint64_t seen[MAX_LISTS];
    int n = 0, lists = 0;
    for (int chain = 0; chain < 2; chain++) {
        uint64_t l = 0;
        if (!rd(sl + 0xd0 + 8 * (uint64_t)chain, &l, 8)) continue;
        while (l && lists < MAX_LISTS) {
            int again = 0;
            for (int i = 0; i < lists && !again; i++) again = seen[i] == l;
            if (again) break;
            seen[lists++] = l;
            uint8_t rec[0x70];
            if (!rd(l, rec, sizeof rec)) break;
            uint32_t lf, cnt; int32_t state, skill, level; float expiry; uint64_t arr, next;
            memcpy(&lf, rec + 0x1c, 4); memcpy(&state, rec + 0x20, 4); memcpy(&expiry, rec + 0x24, 4); memcpy(&skill, rec + 0x28, 4); memcpy(&level, rec + 0x2c, 4);
            memcpy(&arr, rec + 0x30, 8); memcpy(&cnt, rec + 0x38, 4); memcpy(&next, rec + 0x68, 8);
            l = next;
            if (state <= 0 || state >= STATES || !(bits[state >> 5] >> (state & 31) & 1)) continue;
            entry_t *e = find(t, &n, state); if (!e) continue;
            e->lists++;
            if (skill > 0 && level >= e->level) { e->skill = skill; e->level = level; }
            if (lf & TIMED) { int left = ticks_left(expiry, *tick); if (left > e->ticks) e->ticks = left; }
            add_stats(e, arr, cnt);
        }
    }
    for (int s = 1; s < STATES; s++) if (bits[s >> 5] >> (s & 31) & 1) find(t, &n, s);
    return n;
}




#ifdef DM_EFFECT_TESTS
static const uint8_t *g_img_test; static uint64_t g_img_test_len;
#endif
static int main_image(const uint8_t **img, uint64_t *len) {
#ifdef DM_EFFECT_TESTS
    if (g_img_test) { *img = g_img_test; *len = g_img_test_len; return 1; }
#endif
    for (int i = 0; i < g_nmod; i++) if (!strcmp(g_mod[i].name, "main")) { *img = g_mod[i].base; *len = g_mod[i].size; return *img != NULL; }
    return 0;
}
static int rd_main(uint64_t off, void *buf, size_t n) {
    const uint8_t *img; uint64_t len;
    if (!main_image(&img, &len) || off > len || n > len - off) return 0;
    memcpy(buf, img + off, n);
    return 1;
}

static int code_is(uint64_t off, const uint32_t *want, size_t n) { uint32_t got[40]; return n <= 40 && rd_main(off, got, n * 4) && !memcmp(got, want, n * 4); }






static const uint32_t k_stamina_code[] = { 0xb9400e88, 0x51000508, 0x2a0003f7, 0x7100191f, 0x54000082, 0x52800669, 0x1ac82529, 0x370000a9, 0x710f9eff, 0x540003ad, 0x52800118,
    0x14000004, 0, 0, 0xb868d938, 0x52800161, 0xaa1403e0, 0x2a1f03e2, 0, 0x6b0002df, 0x5400024a, 0x1ad82808, 0x34000137, 0x1b177d09, 0x5290a3ea,
    0x72aa3d6a, 0x9b2a7d29, 0xd37ffd2a, 0x9365fd29, 0x0b0a0129, 0x0b080128, 0x0b160108, 0x6b00011f };
enum { STAMINA_CODE = GA_AT_STAMINA_REGEN, STAMINA_SHIFTS = GA_STAMINA_SHIFTS, DIFFICULTY = GA_DIFFICULTY, GAME_INFO = GA_GAME_INFO, SKILL_FN = GA_SKILL_BONUS_LEVEL, SKILL_PLUS2 = GA_AT_SKILL_SHRINE_PLUS2 };
static const uint32_t *stamina_code(void) {
    static uint32_t w[sizeof k_stamina_code / 4];
    if (!w[0]) {
        memcpy(w, k_stamina_code, sizeof w);
        int64_t pages = (int64_t)(((uint64_t)STAMINA_SHIFTS & ~0xfffull) - (((uint64_t)STAMINA_CODE + 48) & ~0xfffull)) >> 12;
        w[12] = 0x90000009u | (uint32_t)(pages & 3) << 29 | (uint32_t)((pages >> 2) & 0x7ffff) << 5;
        w[13] = 0x91000129u | (uint32_t)(STAMINA_SHIFTS & 0xfff) << 10;
        w[18] = 0x94000000u | (uint32_t)((((int64_t)GA_STAT_GET - (int64_t)(STAMINA_CODE + 72)) >> 2) & 0x3ffffff);
    }
    return w;
}


static const uint32_t k_skill_fn[] = { 0xf81b0ff9, 0xa9015ff8, 0xa90257f6, 0xa9034ff4, 0xa9047bfd, 0x910103fd, 0xb4000781, 0xf9400037 };
static const uint32_t k_skill_plus2[] = { 0xb94b4119, 0xaa1303e0, 0x52800fe1, 0x2a1f03e2, 0x94001e36, 0x0b160008, 0x11000909, 0x721a033f, 0x1a890119, 0x17ffff89 };
int dm_build_session(void);



static void read_player(const uint8_t *b, uint64_t player, int *p) {
    int (*gs)(uint64_t, int, int) = (int (*)(uint64_t, int, int))(uintptr_t)(b + GA_STAT_GET);
    uint32_t id = 0, cls = 0, mode = 0;
    memset(p, 0, DM_FX_PLAYER_LEN * sizeof *p);
    if (!rd(player + 4, &cls, 4) || !rd(player + 8, &id, 4) || !rd(player + 0xc, &mode, 4)) return;
    int flags = DM_FXP_VALID;
    uint8_t diff = 0, classic = 0; uint64_t info = 0;
    if (rd_main(DIFFICULTY, &diff, 1) && diff <= 2) { flags |= DM_FXP_DIFF_OK; p[DM_FXP_DIFF] = diff; }
    if (rd_main(GAME_INFO, &info, 8) && info && rd(info + 0x5c, &classic, 1)) { flags |= DM_FXP_CLASSIC_OK; p[DM_FXP_CLASSIC] = classic == 1; }
    int32_t shifts[6];
    if (code_is(STAMINA_CODE, stamina_code(), sizeof k_stamina_code / 4) && rd_main(STAMINA_SHIFTS, shifts, sizeof shifts) && shifts[0] >= 0 && shifts[0] < 32 && shifts[1] >= 0 && shifts[1] < 32) {
        flags |= DM_FXP_STAMINA_OK; p[DM_FXP_STAM_STAND] = shifts[0]; p[DM_FXP_STAM_WALK] = shifts[1]; p[DM_FXP_STAM_RUN] = 8;
    }
    p[DM_FXP_FLAGS] = flags; p[DM_FXP_SESSION] = dm_build_session(); p[DM_FXP_UNIT] = (int)id; p[DM_FXP_CLASS] = (int)cls; p[DM_FXP_MODE] = (int)mode;
    for (int i = 0; i < DM_FXP_NSTATS; i++) p[DM_FXP_STATS + i] = gs(player, k_player_stats[i], 0);
}






#ifdef DM_EFFECT_TESTS
static int (*g_skill_fn_test)(uint64_t, uint64_t, int);
#endif
static void read_skills(const uint8_t *b, uint64_t pl, int *s) {
    memset(s, 0, DM_FX_SKILLS_LEN * sizeof *s);
    uint32_t id = 0, cls = 0; uint64_t sl = 0, l = 0, nd = 0; uint32_t sflags = 0, bits = 0;
    if (!rd(pl + 4, &cls, 4) || !rd(pl + 8, &id, 4)) return;
    if (!code_is(SKILL_FN, k_skill_fn, sizeof k_skill_fn / 4)) return;
    int (*bonus)(uint64_t, uint64_t, int) = (int (*)(uint64_t, uint64_t, int))(uintptr_t)(b + SKILL_FN);
#ifdef DM_EFFECT_TESTS
    if (g_skill_fn_test) bonus = g_skill_fn_test;
#endif
    int flags = DM_FXS_VALID;
    if (code_is(SKILL_PLUS2, k_skill_plus2, sizeof k_skill_plus2 / 4)) flags |= DM_FXS_PLUS2_OK;
    if (rd(pl + 0x88, &sl, 8) && sl && rd(sl + 0x1c, &sflags, 4) && sflags >> 31 && rd(sl + 0xb30 + 4 * (134 >> 5), &bits, 4) && bits >> (134 & 31) & 1) flags |= DM_FXS_SHRINE;
    int n = 0, dropped = 0;
    if (rd(pl + 0x100, &l, 8) && l && rd(l, &nd, 8)) {
        for (int k = 0; nd && k < 1024; k++) {
            uint8_t node[0x48]; uint64_t row = 0, next = 0; int32_t base = 0, item = 0; int16_t skill = 0;
            if (!rd(nd, node, sizeof node)) break;
            memcpy(&row, node, 8); memcpy(&next, node + 8, 8); memcpy(&base, node + 0x38, 4); memcpy(&item, node + 0x44, 4);
            if (row && item == -1 && base > 0 && rd(row, &skill, 2) && skill >= 0) {
                if (n < DM_FX_SKILLS_MAX) { int *e = s + DM_FXS_ENTRIES + 3 * n++; e[0] = skill; e[1] = base; e[2] = bonus(pl, nd, 0); }
                else dropped++;
            }
            if (next == nd) break;
            nd = next;
        }
    }
    s[DM_FXS_FLAGS] = flags; s[DM_FXS_SESSION] = dm_build_session(); s[DM_FXS_UNIT] = (int)id; s[DM_FXS_CLASS] = (int)cls; s[DM_FXS_COUNT] = n; s[DM_FXS_DROPPED] = dropped;
}






static int client_shrine(uint64_t pl, int state, int *v) {
    uint64_t sl = 0, seen[64]; uint32_t flags = 0; int lists = 0, found = 0;
    v[0] = v[1] = v[2] = 0;
    if (!rd(pl + 0x88, &sl, 8) || !sl || !rd(sl + 0x1c, &flags, 4) || !(flags >> 31)) return 0;
    for (int chain = 0; chain < 2; chain++) {
        uint64_t l = 0;
        if (!rd(sl + 0xd0 + 8 * (uint64_t)chain, &l, 8)) continue;
        while (l && lists < 64) {
            int again = 0;
            for (int i = 0; i < lists && !again; i++) again = seen[i] == l;
            if (again) break;
            seen[lists++] = l;
            uint8_t rec[0x70]; int32_t st; uint32_t cnt; uint64_t arr, next;
            if (!rd(l, rec, sizeof rec)) break;
            memcpy(&st, rec + 0x20, 4); memcpy(&arr, rec + 0x30, 8); memcpy(&cnt, rec + 0x38, 4); memcpy(&next, rec + 0x68, 8);
            l = next;
            if (st != state) continue;
            uint32_t pairs[8][2];
            if (!cnt || cnt > 8 || !rd(arr, pairs, (size_t)cnt * 8)) return 0;
            for (uint32_t i = 0; i < cnt; i++) {
                int stat = pairs[i][0] >> 16, layer = pairs[i][0] & 0xffff, val = (int32_t)pairs[i][1];
                if (layer != 0 || (stat != 171 && stat != 19 && stat != 25)) return 0;
                v[stat == 171 ? 0 : stat == 19 ? 1 : 2] += val;
            }
            found = 1;
        }
    }
    return found;
}

#ifdef DM_EFFECT_TESTS


static void client_state_sum(uint64_t pl, int state, int *s28, int *s96) {
    uint64_t sl = 0, seen[64]; uint32_t flags = 0; int lists = 0;
    *s28 = *s96 = 0;
    if (!rd(pl + 0x88, &sl, 8) || !sl || !rd(sl + 0x1c, &flags, 4) || !(flags >> 31)) return;
    for (int chain = 0; chain < 2; chain++) {
        uint64_t l = 0;
        if (!rd(sl + 0xd0 + 8 * (uint64_t)chain, &l, 8)) continue;
        while (l && lists < 64) {
            int again = 0;
            for (int i = 0; i < lists && !again; i++) again = seen[i] == l;
            if (again) break;
            seen[lists++] = l;
            uint8_t rec[0x70]; int32_t st; uint32_t cnt; uint64_t arr, next;
            if (!rd(l, rec, sizeof rec)) return;
            memcpy(&st, rec + 0x20, 4); memcpy(&arr, rec + 0x30, 8); memcpy(&cnt, rec + 0x38, 4); memcpy(&next, rec + 0x68, 8);
            l = next;
            if (st != state || !cnt || cnt > 8) continue;
            uint32_t pairs[8][2];
            if (!rd(arr, pairs, (size_t)cnt * 8)) return;
            for (uint32_t i = 0; i < cnt; i++) {
                int stat = pairs[i][0] >> 16, layer = pairs[i][0] & 0xffff, val = (int32_t)pairs[i][1];
                if (layer != 0) continue;
                if (stat == 28) *s28 += val; else if (stat == 96) *s96 += val;
            }
        }
    }
}
#endif





static void read_combat_slot(uint8_t *b, uint64_t pl, int state, int *c) {
    uint64_t sl = 0; uint32_t flags = 0, w = 0, id = 0;
    if (!rd(pl + 8, &id, 4) || !rd(pl + 0x88, &sl, 8) || !sl || !rd(sl + 0x1c, &flags, 4) || !(flags >> 31)
        || !rd(sl + 0xb30 + 4 * (state >> 5), &w, 4) || !(w >> (state & 31) & 1)) return;
    int v[3];
    if (!client_shrine(pl, state, v)) return;
    int o[8];
    unsigned ok = dm_itemcmp_shrine_values(b, pl, v[0], v[1], v[2], o);
    c[DM_FXC_FLAGS] = DM_FXC_VALID | (ok & 1 && v[0] ? DM_FXC_DEF_OK : 0) | (ok & 4 && v[1] ? DM_FXC_AR_OK : 0) | (ok & 8 && (v[1] || v[2]) ? DM_FXC_DMG_OK : 0);
    c[DM_FXC_SESSION] = dm_build_session(); c[DM_FXC_UNIT] = (int)id; c[DM_FXC_STATE] = state; c[DM_FXC_V171] = v[0]; c[DM_FXC_V19] = v[1]; c[DM_FXC_V25] = v[2];
    for (int i = 0; i < 8; i++) c[DM_FXC_DEF_B + i] = o[i];
}

static void read_combat(uint8_t *b, uint64_t pl, int *c) {
    memset(c, 0, DM_FX_COMBAT_LEN * sizeof *c);
    read_combat_slot(b, pl, 128, c + DM_FXC_ARMOR);
    read_combat_slot(b, pl, 129, c + DM_FXC_COMBAT);
#ifdef DM_EFFECT_TESTS
    { static int on = -1; static int last[DM_FX_COMBAT_LEN]; if (on < 0) on = getenv("DM_TEST_EFFECT_LOG") != NULL;
      if (on && memcmp(last, c, sizeof last)) { memcpy(last, c, sizeof last);
          for (int s = 0; s < 2; s++) { int *x = c + DM_FXC_LEN * s;
              if (!(x[DM_FXC_FLAGS] & DM_FXC_VALID)) continue;
              LOG("DM_TEST_EFFECT_LOG 战斗块：状态 %d 值 %d/%d/%d 标志 %d 防御 %d › %d 命中 %d › %d 伤害 %d-%d › %d-%d", x[DM_FXC_STATE], x[DM_FXC_V171], x[DM_FXC_V19], x[DM_FXC_V25], x[DM_FXC_FLAGS],
                  x[DM_FXC_DEF_B], x[DM_FXC_DEF_A], x[DM_FXC_AR_B], x[DM_FXC_AR_A], x[DM_FXC_DMIN_B], x[DM_FXC_DMAX_B], x[DM_FXC_DMIN_A], x[DM_FXC_DMAX_A]); } } }
#endif
}

void dm_effects_pump(uint8_t *b, uint64_t pl) {
    static unsigned frame;
    int s[DM_FX_SKILLS_LEN];
    if (!b || !pl) {
        pthread_mutex_lock(&g_mu);
        if (g_skills_ms || g_skills[DM_FXS_FLAGS]) { memset(g_skills, 0, sizeof g_skills); g_skills_ms = 0; g_gen++; }
        if (g_combat_ms || g_combat[DM_FXC_FLAGS] || g_combat[DM_FXC_COMBAT + DM_FXC_FLAGS]) { memset(g_combat, 0, sizeof g_combat); g_combat_ms = 0; g_gen++; }
        pthread_mutex_unlock(&g_mu);
        return;
    }
    if (++frame % 2) return;
    read_skills(b, pl, s);
    int c[DM_FX_COMBAT_LEN];
    read_combat(b, pl, c);
    uint64_t now = g_now_ms();
    pthread_mutex_lock(&g_mu);
    if (memcmp(g_skills, s, sizeof s)) { memcpy(g_skills, s, sizeof s); g_gen++; }
    g_skills_ms = s[DM_FXS_FLAGS] ? now : 0;
    if (memcmp(g_combat, c, sizeof c)) { memcpy(g_combat, c, sizeof c); g_gen++; }
    g_combat_ms = c[DM_FXC_ARMOR + DM_FXC_FLAGS] || c[DM_FXC_COMBAT + DM_FXC_FLAGS] ? now : 0;
    pthread_mutex_unlock(&g_mu);
#ifdef DM_EFFECT_TESTS



    { static int on = -1, last96 = -1, last28 = -1; if (on < 0) on = getenv("DM_TEST_EFFECT_LOG") != NULL;
      if (on) { int (*gs)(uint64_t, int, int) = (int (*)(uint64_t, int, int))(uintptr_t)(b + GA_STAT_GET); int v = gs(pl, 96, 0), s28 = 0, s96 = 0;
          client_state_sum(pl, 136, &s28, &s96);
          if (v != last96 || s28 != last28) { last96 = v; last28 = s28; LOG("DM_TEST_EFFECT_LOG 客户端跑走：属性 96 = %d 136 子表 28 = %d 96 = %d", v, s28, s96); } } }
#endif
}

#ifdef DM_EFFECT_TESTS
static void test_log(const entry_t *t, int n) {
    static int on = -1; static char last[4096];
    if (on < 0) on = getenv("DM_TEST_EFFECT_LOG") != NULL;
    if (!on) return;
    char b[4096] = ""; int o = 0;
    for (int i = 0; i < n && o < 3900; i++) {
        o += snprintf(b + o, sizeof b - o, "[%d sk %d lv %d", t[i].state, t[i].skill, t[i].level);
        for (int k = 0; k < t[i].n && o < 3900; k++) o += snprintf(b + o, sizeof b - o, " %d/%d=%d", t[i].stat[k][0], t[i].stat[k][1], t[i].stat[k][2]);
        o += snprintf(b + o, sizeof b - o, "] ");
    }
    if (strcmp(b, last)) { memcpy(last, b, (size_t)o + 1); LOG("DM_TEST_EFFECT_LOG：%s（丢掉的属性 %u）", b, g_dropped); }
    static uint32_t tk; if (!(++tk % 25) && n) { char c[512]; int p = 0; for (int i = 0; i < n && p < 480; i++) if (t[i].ticks >= 0) p += snprintf(c + p, sizeof c - p, "%d:%d ", t[i].state, t[i].ticks); if (p) LOG("DM_TEST_EFFECT_LOG 剩余：%s", c); }
}








static void test_fake(entry_t *t, int *n, uint32_t tick, int *mana, int *pb) {
    static int parsed, count; static uint32_t start;
    static struct { int state, ticks, level, skill, stats, stat[DM_EFFECTS_STATS][2]; } fake[24];
    if (!parsed) {
        parsed = 1; start = tick;
        const char *env = getenv("DM_TEST_EFFECT_FAKE"); char buf[600];
        if (!env) return;
        strlcpy(buf, env, sizeof buf);
        for (char *sv, *item = strtok_r(buf, ",", &sv); item && count < 24; item = strtok_r(NULL, ",", &sv)) {
            int state = 0, ticks = -1, used = 0;
            if (sscanf(item, "%d:%d%n", &state, &ticks, &used) < 2 || state <= 0 || state >= STATES) continue;
            fake[count].state = state; fake[count].ticks = ticks; fake[count].level = 0; fake[count].skill = 0; fake[count].stats = 0;
            for (char *p = item + used; *p == ':'; ) {
                int a, v, k = 0;
                if (p[1] == 'L' && sscanf(p + 2, "%d%n", &a, &k) == 1) { fake[count].level = a; p += 2 + k; continue; }
                if (p[1] == 'S' && sscanf(p + 2, "%d%n", &a, &k) == 1) { fake[count].skill = a; p += 2 + k; continue; }
                if (sscanf(p + 1, "%d=%d%n", &a, &v, &k) != 2) break;
                if (fake[count].stats < DM_EFFECTS_STATS) { fake[count].stat[fake[count].stats][0] = a; fake[count].stat[fake[count].stats][1] = v; fake[count].stats++; }
                p += 1 + k;
            }
            count++;
        }
        LOG("DM_TEST_EFFECT_FAKE：%d 个假状态（只在调试包，造现场用）", count);
    }
    for (int i = 0; i < count; i++) {
        int left = fake[i].ticks < 0 ? -1 : fake[i].ticks - (int)(tick - start);
        if (fake[i].ticks >= 0 && left <= 0) continue;
        entry_t *e = find(t, n, fake[i].state); if (!e) continue;
        e->ticks = left; e->level = fake[i].level; e->skill = fake[i].skill ? fake[i].skill : fake[i].level ? 1 : -1; e->lists = 1; e->n = 0; memset(e->stat, 0, sizeof e->stat);
        for (int k = 0; k < fake[i].stats; k++) {
            e->stat[k][0] = fake[i].stat[k][0]; e->stat[k][1] = 0; e->stat[k][2] = fake[i].stat[k][1]; e->n++;


            int a = fake[i].stat[k][0], v = fake[i].stat[k][1];
            if (a == 27) mana[1] += v; else if (a == 26) mana[2] += v; else if (a == 74) mana[3] += v;
            for (int j = 0; j < DM_FXP_NSTATS; j++) if (k_player_stats[j] == a && pb[DM_FXP_FLAGS]) pb[DM_FXP_STATS + j] += v;
        }
    }
}














enum { SHRINE_TABLE = GA_SHRINE_TABLE, SHRINE_FIRST = 6, SHRINE_LAST = 15, SHRINE_MAX = 16 };
typedef struct { int code, at, done; } shrine_t;


static const uint32_t k_shrine_fn[SHRINE_LAST - SHRINE_FIRST + 1] = { GA_SHRINE_FN_GENERIC, GA_SHRINE_FN_COMBAT, GA_SHRINE_FN_GENERIC, GA_SHRINE_FN_GENERIC, GA_SHRINE_FN_GENERIC, GA_SHRINE_FN_GENERIC, GA_SHRINE_SKILL_FN, GA_SHRINE_FN_GENERIC, GA_SHRINE_STAMINA_FN, GA_SHRINE_FN_GENERIC };
static const int k_shrine_state[SHRINE_LAST - SHRINE_FIRST + 1] = { 128, 129, 131, 132, 130, 133, 134, 135, 136, 137 };



static int shrine_parse(const char *env, shrine_t *out, int max) {
    char buf[200]; int n = 0;
    if (!env) return 0;
    strlcpy(buf, env, sizeof buf);
    for (char *sv, *item = strtok_r(buf, ",", &sv); item && n < max; item = strtok_r(NULL, ",", &sv)) {
        int code, at, used = 0;
        if (sscanf(item, " %d@%d%n", &code, &at, &used) != 2 || item[used] || code < SHRINE_FIRST || code > SHRINE_LAST || at < 0) continue;
        out[n].code = code; out[n].at = at; out[n].done = 0; n++;
    }
    return n;
}


static int shrine_entry_ok(const uint8_t *table, uint64_t base, int code) {
    if (code < SHRINE_FIRST || code > SHRINE_LAST) return 0;
    uint64_t fn; uint32_t state;
    memcpy(&fn, table + 16 * code, 8); memcpy(&state, table + 16 * code + 12, 4);
    return fn == base + k_shrine_fn[code - SHRINE_FIRST] && state == (uint32_t)k_shrine_state[code - SHRINE_FIRST];
}

static void test_shrine(const uint8_t *b, uint64_t G, uint64_t player, uint32_t tick) {
    static int parsed, count; static uint32_t start; static shrine_t s[SHRINE_MAX];
    if (!parsed) { parsed = 1; start = tick; count = shrine_parse(getenv("DM_TEST_SHRINE"), s, SHRINE_MAX); if (count) LOG("DM_TEST_SHRINE：%d 个神殿（只在调试包，造现场用）", count); }
    for (int i = 0; i < count; i++) {
        if (s[i].done || tick - start < (uint32_t)s[i].at) continue;
        s[i].done = 1;
        uint8_t ver = 0, pver = 0; uint64_t T = 0, cnt = 0, arr = 0, row = 0;
        int ok = rd(G + 0x106, &ver, 1) && ver >= 1 && ver <= 3 && rd(player + 0x1bd, &pver, 1) && pver == ver;
        if (ok) { memcpy(&T, b + GA_DATA_TABLES + 16 * (uint64_t)ver, 8); ok = rd(T + 0x19f0, &cnt, 8) && cnt > (uint64_t)s[i].code && rd(T + 0x19e8, &arr, 8); }
        uint8_t whole[0x1c];
        if (ok) { ok = arr <= UINT64_MAX - 0x1c * (uint64_t)(SHRINE_LAST + 1); row = arr + 0x1c * (uint64_t)s[i].code; }
        if (ok) ok = rd(row, whole, sizeof whole) && whole[0] == s[i].code;
        if (!ok || !shrine_entry_ok(b + SHRINE_TABLE, (uint64_t)(uintptr_t)b, s[i].code)) {
            LOG("DM_TEST_SHRINE：码 %d 没调（版本 %u / %u，行 %s，表项 %s）", s[i].code, ver, pver, ok ? "对" : "不对", shrine_entry_ok(b + SHRINE_TABLE, (uint64_t)(uintptr_t)b, s[i].code) ? "对" : "不对");
            continue;
        }
        uint64_t fn, ctx[3] = { G, player, player };
        memcpy(&fn, b + SHRINE_TABLE + 16 * s[i].code, 8);
        ((void (*)(uint32_t, uint64_t *, uint64_t))(uintptr_t)fn)(ver, ctx, row);
        LOG("DM_TEST_SHRINE：拍 %u 调了码 %d 的处理函数 main+0x%x（状态 %d）", tick, s[i].code, k_shrine_fn[s[i].code - SHRINE_FIRST], k_shrine_state[s[i].code - SHRINE_FIRST]);
    }
}












enum { CURSE_MAX = 16 };
typedef struct { int state, dur, at, done; } curse_t;



static int curse_parse(const char *env, curse_t *out, int max) {
    char buf[200]; int n = 0;
    if (!env) return 0;
    strlcpy(buf, env, sizeof buf);
    for (char *sv, *item = strtok_r(buf, ",", &sv); item && n < max; item = strtok_r(NULL, ",", &sv)) {
        int st, dur, at = 0, used = 0;
        if (sscanf(item, " %d@%d@%d%n", &st, &dur, &at, &used) != 3 && !(sscanf(item, " %d@%d%n", &st, &dur, &used) == 2 && (at = 0, 1))) continue;
        if (item[used] || st < 0 || st > 1023 || dur <= 0 || dur > 0x7ffffff0 || at < 0) continue;
        out[n].state = st; out[n].dur = dur; out[n].at = at; out[n].done = 0; n++;
    }
    return n;
}

static void test_curse(const uint8_t *b, uint64_t G, uint64_t player, uint32_t tick) {
    static int parsed, count, resist = -1, resist_at, resist_done; static uint32_t start; static curse_t s[CURSE_MAX];
    if (!parsed) {
        parsed = 1; start = tick;
        count = curse_parse(getenv("DM_TEST_CURSE"), s, CURSE_MAX);
        const char *r = getenv("DM_TEST_CURSE_RESIST");
        if (r) { int v = -1, at = 0, used = 0; char buf[32]; strlcpy(buf, r, sizeof buf);
            if (sscanf(buf, " %d@%d%n", &v, &at, &used) == 2 && !buf[used]) { resist = v; resist_at = at; }
            else if (sscanf(buf, " %d%n", &v, &used) == 1 && !buf[used]) { resist = v; resist_at = 0; }
            if (resist < 0 || resist > 1000000) resist = -1; }
        if (count) LOG("DM_TEST_CURSE：%d 个诅咒（只在调试包，造现场用）", count);
    }
    uint8_t ver = 0;
    if (resist >= 0 && !resist_done && tick - start >= (uint32_t)resist_at && rd(player + 0x1bd, &ver, 1) && ver >= 1 && ver <= 3) {
        uint64_t sl = 0;
        if (rd(player + 0x88, &sl, 8) && sl) {
            ((void (*)(uint32_t, uint64_t, int, int, int))(uintptr_t)(b + GA_STATLIST_SET))(ver, sl, 109, resist, 0);
            LOG("DM_TEST_CURSE_RESIST：拍 %u 写属性 109 = %d", tick, resist);
            resist_done = 1;
        }
    }
    for (int i = 0; i < count; i++) {
        if (s[i].done || tick - start < (uint32_t)s[i].at) continue;
        s[i].done = 1;
        uint8_t gv = 0, pver = 0;
        if (!rd(G + 0x106, &gv, 1) || gv < 1 || gv > 3 || !rd(player + 0x1bd, &pver, 1) || pver != gv) {
            LOG("DM_TEST_CURSE：状态 %d 没调（版本 %u / %u）", s[i].state, gv, pver);
            continue;
        }
        struct { uint64_t source, target; uint32_t skill, level, dur, stat, value, state; uint64_t cb; } arg = { player, player, 0, 0, (uint32_t)s[i].dur, 0xffffffffu, 0, (uint32_t)s[i].state, 0 };
        uint64_t (*apply)(void *) = (uint64_t (*)(void *))(uintptr_t)(b + GA_STATE_APPLY_TIMED);
        uint64_t r = apply(&arg);
        LOG("DM_TEST_CURSE：拍 %u 调 main+%#x 状态 %d %d 拍 → %s", tick, (unsigned)GA_STATE_APPLY_TIMED, s[i].state, s[i].dur, r ? "挂上" : "被拒");
    }
}
#endif

static uint32_t g_state_bits[DM_EFFECTS_STATE_WORDS]; static unsigned g_state_seq; static uint64_t g_state_game;
unsigned dm_effects_state_bits(uint32_t out[DM_EFFECTS_STATE_WORDS]) { memcpy(out, g_state_bits, sizeof g_state_bits); return g_state_seq; }
int dm_effects_state_bits_of(uint64_t G, uint32_t out[DM_EFFECTS_STATE_WORDS]) {
    int ok = G && g_state_game == G;
    if (ok) memcpy(out, g_state_bits, sizeof g_state_bits); else memset(out, 0, sizeof g_state_bits);
    return ok;
}
void dm_effects_tick(uint64_t G, uint64_t player) {
    static uint32_t tick; static entry_t t[DM_EFFECTS_MAX]; static const uint8_t *b;
    if (++tick % EVERY) return;
    if (!b) b = (const uint8_t *)dm_main_base();
    uint32_t now = 0; int mana[MANA_WORDS] = { 0 }, n = player && G && b ? read_effects(b, G, player, t, &now, mana) : -1, read = n >= 0;
    static int pb[DM_FX_PLAYER_LEN];
    if (n < 0) n = 0;
    if (read) read_player(b, player, pb); else memset(pb, 0, sizeof pb);
    { uint32_t bits[DM_EFFECTS_STATE_WORDS] = { 0 };
      for (int i = 0; i < n; i++) if (t[i].state >= 0 && t[i].state < 32 * DM_EFFECTS_STATE_WORDS) bits[t[i].state >> 5] |= 1u << (t[i].state & 31);
      if (memcmp(bits, g_state_bits, sizeof bits)) { memcpy(g_state_bits, bits, sizeof bits); g_state_seq++; }
      g_state_game = read ? G : 0; }
#ifdef DM_EFFECT_TESTS
    if (read) test_shrine(b, G, player, now);
    if (read) test_curse(b, G, player, now);
    if (read) test_fake(t, &n, now, mana, pb);
    test_log(t, n);
    { static int on = -1; if (on < 0) on = getenv("DM_TEST_EFFECT_RATE") != NULL;
      if (on && read) { int (*gs)(uint64_t, int, int) = (int (*)(uint64_t, int, int))(uintptr_t)(b + GA_STAT_GET); LOG("DM_TEST_EFFECT_RATE 拍 %u 生命 %d 法力 %d 上限 %d 基础 %d 加成 %d 恢复 %d 回血 %d", now, gs(player, 6, 0), gs(player, 8, 0), gs(player, 9, 0), mana[0], mana[1], mana[2], mana[3]); } }
#endif
    pthread_mutex_lock(&g_mu);
    if (n != g_n || memcmp(g_out, t, (size_t)n * sizeof *t) || memcmp(g_mana, mana, sizeof mana) || memcmp(g_player, pb, sizeof pb)) {
        memcpy(g_out, t, (size_t)n * sizeof *t); g_n = n; memcpy(g_mana, mana, sizeof mana); memcpy(g_player, pb, sizeof pb); g_gen++;
    }
    g_player_ms = pb[DM_FXP_FLAGS] ? g_now_ms() : 0;
    g_tick = now;
    pthread_mutex_unlock(&g_mu);
}
