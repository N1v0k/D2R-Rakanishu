












#include "dm_streak.h"
#include "dm_fightlog.h"
#include "dm_bossrush.h"
#include "dm_custom.h"
#include "dm_hud.h"
#include "dm_rift.h"
#include "dm.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static uint8_t *B;
static dm_sk_state g_sk;
static int g_mode = -1;
static pthread_mutex_t g_mu = PTHREAD_MUTEX_INITIALIZER;
static int g_hud[DM_SK_HUD_N];

static uint64_t rd(uint64_t a) { return a ? *(volatile uint64_t *)(uintptr_t)a : 0; }
static uint32_t rd32(uint64_t a) { return *(volatile uint32_t *)(uintptr_t)a; }
static int ustat(uint64_t u, int id) { return ((int (*)(uint64_t, int, int))(uintptr_t)(B + 0x5b9290))(u, id, 0); }

static uint32_t exp_max(uint64_t G, uint64_t player) {
    uint8_t set = *(volatile uint8_t *)(uintptr_t)(G + 0x106); if (set < 1 || set > 3) return 0;
    uint64_t T = rd((uint64_t)(uintptr_t)(B + 0x2d64a18 + set * 16)), x = T ? rd(T + 0x1500) : 0; if (!x) return 0;
    uint32_t cls = rd32(player + 4); if (cls >= 8) cls = 0;
    int32_t top = (int32_t)rd32(x + 4 * cls); if (top < 1 || top > 255) return 0;
    return rd32(x + (uint64_t)top * 0x24 + 4 * cls);
}
static int players_alive(uint64_t G) {
    int n = 0;
    for (int b = 0; b < 128; b++) for (uint64_t u = rd(G + 0x2230 + 8 * (uint64_t)b), k = 0; u && k < 256; u = rd(u + 0x158), k++) n += !dm_hud_player_mode_dead(rd32(u + 0xc));
    return n;
}
const char *dm_save_workdir(void);
uint64_t dm_fs_commit_count(void);
static void hist_hook(const char *wd) { dm_sk_hist_commit(dm_sk_hist_core(), wd, dm_fs_commit_count()); }
__attribute__((constructor)) static void hist_hook_install(void) { extern void (*g_dm_pre_commit_hook[DM_PRE_COMMIT_HOOKS])(const char *); g_dm_pre_commit_hook[6] = hist_hook; }
int dm_streak_text(char *out, size_t cap) {
    int n = dm_sk_hist_text(dm_sk_hist_core(), out, cap);
    if (n < 0) return n;
    int w = dm_sk_tiers_text(out + n, cap - (size_t)n);
    if (w < 0) { out[0] = 0; return -2; }
    return n + w;
}

static uint32_t g_player_id;
static int g_credit_gate;

#ifndef DM_PLAYER_BUILD


static int cost_on(void) { static int on = -1; if (on < 0) { const char *e = getenv("DM_TEST_FIGHTLOG_COST"); on = e && strcmp(e, "0"); } return on; }
static uint64_t cpu_ns(void) { struct timespec t; clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t); return (uint64_t)t.tv_sec * 1000000000ull + (uint64_t)t.tv_nsec; }
static struct { uint64_t ticks, tick_ns, max, kill_ticks, kill_ns, kills, credits, credit_ns, ends; } g_cost;
static void cost_tick(uint64_t d, int kills, int ends) {
    g_cost.ticks++; g_cost.tick_ns += d; if (d > g_cost.max) g_cost.max = d;
    if (kills) { g_cost.kill_ticks++; g_cost.kill_ns += d; g_cost.kills += (uint64_t)kills; }
    g_cost.ends += (uint64_t)ends;
    if (g_cost.ticks < 250) return;
    LOG("[streak-cost] mode=%d ticks=%llu tick_ns=%llu max=%llu kill_ticks=%llu kills=%llu kill_ns=%llu credits=%llu credit_ns=%llu ends=%llu", g_mode,
        (unsigned long long)g_cost.ticks, (unsigned long long)(g_cost.tick_ns / g_cost.ticks), (unsigned long long)g_cost.max,
        (unsigned long long)g_cost.kill_ticks, (unsigned long long)g_cost.kills, (unsigned long long)(g_cost.kills ? g_cost.kill_ns / g_cost.kills : 0),
        (unsigned long long)g_cost.credits, (unsigned long long)(g_cost.credits ? g_cost.credit_ns / g_cost.credits : 0), (unsigned long long)g_cost.ends);
    memset(&g_cost, 0, sizeof g_cost);
}


static void test_seed(dm_sk_state *s) {
    static int n = -1, next; static uint32_t k[16]; static uint64_t base;
    if (n < 0) {
        n = 0; const char *e = getenv("DM_TEST_STREAK_SEED"); char *p = (char *)e;
        while (p && *p && *p != ':' && n < 16) { k[n++] = (uint32_t)strtoul(p, &p, 10); if (*p == ',') p++; }
        if (p && *p == ':') base = strtoull(p + 1, NULL, 10);
        if (n) LOG("kill streak: test switch DM_TEST_STREAK_SEED=%s", e);
    }
    if (next >= n) return;
    s->kills += k[next++]; s->base += base;
}
#endif


static void credit(const uint64_t *regs) {
    if (g_mode <= DM_SK_OFF || !regs[1] || rd32(regs[1] + 8) != g_player_id || (int32_t)(uint32_t)regs[3] <= 0) return;
    uint64_t lr = regs[101] - (uint64_t)(uintptr_t)B, dead;
    if (lr == 0x7d831c) dead = regs[19];
    else if (lr == 0x7dcfb0 || lr == 0x7dd098) dead = rd(regs[29] - 8);
    else return;
    if (dead) dm_sk_credit(&g_sk, rd32(regs[0] + 0x170), rd32(dead + 8), (uint32_t)regs[3]);
}
static void credit_obs(const uint64_t *regs) {
#ifndef DM_PLAYER_BUILD
    if (cost_on()) { uint64_t t0 = cpu_ns(); credit(regs); g_cost.credit_ns += cpu_ns() - t0; g_cost.credits++; return; }
#endif
    credit(regs);
}
static void hud_set(int i, int v) { g_hud[i] = v; }
int dm_streak_hud(int *out, int max) {
    int n = max < DM_SK_HUD_N ? max : DM_SK_HUD_N; if (n <= 0) return 0;
    pthread_mutex_lock(&g_mu); memcpy(out, g_hud, (size_t)n * sizeof *out); pthread_mutex_unlock(&g_mu);
    return n;
}


static void ended(uint64_t G, uint64_t player, const dm_sk_end *e, uint32_t *exp, int area, int difficulty) {
    uint32_t cap = exp_max(G, player), before = *exp, after = before; uint64_t bonus = 0;
    int apply = e->kind == DM_SK_END_TIME && g_mode == DM_SK_BONUS && !e->no_bonus && e->pct > 0 && e->base > 0;
    if (apply && !g_credit_gate) { static int logged; if (!logged++) LOG("ASSUME streak.credit_gate: the pack has no kill credit gate; no bonus"); apply = 0; }
    if (apply && !cap) { static int logged; if (!logged++) LOG("ASSUME streak.exp_table: the experience table is not readable; no bonus"); apply = 0; }
    if (apply && players_alive(G) != 1) { static int logged; if (logged++ < 3) LOG("ASSUME streak.single_player: %d players in the game at the end of a streak; no bonus", players_alive(G)); apply = 0; }
    if (apply) {
        bonus = dm_sk_bonus(e->base, e->pct);
        if (before >= cap) bonus = 0; else if (bonus > (uint64_t)(cap - before)) bonus = cap - before;
        if (bonus > 0x7fffffff) bonus = 0x7fffffff;
    }
    if (bonus) {
        ((void (*)(uint64_t, uint64_t, int))(uintptr_t)(B + 0x7ddc40))(G, player, (int)bonus);
        after = (uint32_t)ustat(player, 13);
        if (after != before + (uint32_t)bonus) LOG("ASSUME streak.grant_exact: experience %u + bonus %llu gave %u", before, (unsigned long long)bonus, after);
        *exp = after;
    }
    dm_sk_rec r = { e->kills, e->last - e->first, (uint32_t)bonus, (uint16_t)(area > 0 && area < 0x10000 ? area : 0), (uint8_t)difficulty, (uint8_t)e->kind };
    int kept = dm_custom_get("fight_log") != 0 && difficulty >= 0 && difficulty < DM_SK_DIFFS, record = dm_sk_hist_keep(dm_sk_hist_core(), &r, kept);
    LOG("[streak] {\"end\":%d,\"kills\":%u,\"ticks\":%u,\"base\":%llu,\"pct\":%d,\"no_bonus\":%d,\"mode\":%d,\"bonus\":%llu,\"exp\":[%u,%u],\"area\":%d,\"difficulty\":%d,\"kept\":%d,\"record\":%d}",
        e->kind, e->kills, e->last - e->first, (unsigned long long)e->base, e->pct, e->no_bonus, g_mode, (unsigned long long)bonus, before, after, r.area, difficulty, kept, record);
    pthread_mutex_lock(&g_mu);
    hud_set(DM_SK_HUD_END_SEQ, g_hud[DM_SK_HUD_END_SEQ] + 1); hud_set(DM_SK_HUD_END_KILLS, (int)e->kills);
    hud_set(DM_SK_HUD_END_BONUS, (int)bonus); hud_set(DM_SK_HUD_END_KIND, e->kind); hud_set(DM_SK_HUD_END_RECORD, record); hud_set(DM_SK_HUD_END_TIER, dm_sk_tier(e->kills));
    pthread_mutex_unlock(&g_mu);
}

void dm_streak_tick(uint64_t G, uint64_t player, uint8_t *main_base, int dead, int area, int difficulty) {
    if (!G || !player || !main_base) return;
#ifndef DM_PLAYER_BUILD
    uint64_t t0 = cost_on() ? cpu_ns() : 0;
    int cost_kills = 0, cost_ends = 0;
#endif
    B = main_base;
    static uint64_t game; static uint32_t last_t; static int hooked, dropped_logged, full_logged, kill_area, prev_area;
    if (!hooked) { hooked = 1; g_credit_gate = gate_obs_present(OBS_KILL_CREDIT); if (g_credit_gate) gate_obs_set(OBS_KILL_CREDIT, credit_obs); }
    uint32_t t = rd32(G + 0x170);
    int m = dm_custom_get("streak");
    if (m < DM_SK_OFF || m > DM_SK_BONUS) m = DM_SK_BONUS;
    if (m != g_mode) { if (m == DM_SK_OFF) dm_sk_reset(&g_sk); LOG("kill streak: setting %d", m); g_mode = m; }
    if (G != game || t < last_t) {
        game = G; dm_sk_reset(&g_sk); kill_area = prev_area = 0;
        uint64_t pd = rd(player + 0x10); char name[64] = "";
        if (pd) { memcpy(name, (const void *)(uintptr_t)pd, sizeof name - 1); name[sizeof name - 1] = 0; }
        dm_sk_hist_follow(dm_sk_hist_core(), dm_save_workdir(), name);
    }
    last_t = t; g_player_id = rd32(player + 8);
    int kills = 0, shown = 0; uint32_t k = 0; int left = 0, pct = 0;
    if (g_mode > DM_SK_OFF) {
        const uint32_t *ids; int n = dm_fl_kills(dm_fl_core(), t, &ids), killed = n && !dead;
#ifndef DM_PLAYER_BUILD
        if (killed) cost_kills = n;
#endif
        if (killed) {
            int no_bonus = dm_rift_active() || dm_bossrush_context(t);
            for (int i = 0; i < n; i++) {
                dm_sk_kill(&g_sk, t, ids[i], no_bonus);
#ifndef DM_PLAYER_BUILD
                if (g_sk.kills == 1) test_seed(&g_sk);
#endif
            }
            prev_area = kill_area; if (area > 0) kill_area = area;
        }
        if (!dead) dm_sk_hits(&g_sk, t, dm_fl_hits(dm_fl_core(), t));
        uint32_t exp = (uint32_t)ustat(player, 13);
        dm_sk_end out[2]; int ne = dm_sk_tick(&g_sk, t, dead, out);
        if (g_sk.dropped && !dropped_logged) { dropped_logged = 1; LOG("ASSUME streak.credits: more than %d kill credits waited for their kills; %u lost", DM_SK_CREDITS, g_sk.dropped); }
        if (g_sk.ids_full && !full_logged) { full_logged = 1; LOG("ASSUME streak.ids: more than %d units in one streak; further ones count without the revival check", DM_SK_IDS); }
        for (int i = 0; i < ne; i++) ended(G, player, &out[i], &exp, killed ? prev_area : kill_area, difficulty);
#ifndef DM_PLAYER_BUILD
        cost_ends = ne;
#endif
        shown = dm_sk_shown(&g_sk, t, &k, &left);
        if (shown && g_mode == DM_SK_BONUS && !g_sk.no_bonus && g_credit_gate) { uint32_t cap = exp_max(G, player); if (cap && exp < cap) pct = dm_sk_pct(k); }
        kills = shown ? (int)k : 0;
    }
    pthread_mutex_lock(&g_mu);
    hud_set(DM_SK_HUD_TICK, (int)t); hud_set(DM_SK_HUD_KILLS, kills); hud_set(DM_SK_HUD_LEFT, shown ? left : 0);
    hud_set(DM_SK_HUD_WINDOW, DM_SK_WINDOW); hud_set(DM_SK_HUD_PCT, pct); hud_set(DM_SK_HUD_TIER, kills ? dm_sk_tier((uint32_t)kills) : 0);
    pthread_mutex_unlock(&g_mu);
#ifndef DM_PLAYER_BUILD
    if (t0) cost_tick(cpu_ns() - t0, cost_kills, cost_ends);
#endif
}
