











#include "dm_resume_core.h"
#include "dm_custom.h"
#include "dm_hud.h"
#include "dm_rift.h"
#include "dm_bossrush.h"
#include "dm_unit.h"
#include "dm.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>

const char *dm_save_commitdir(void);
int dm_loadscreen_up(void);
int dm_uiscreen_ingame(void);
void dm_pause_want(int reasons, int on);
int dm_threat_tier(uint64_t unit);

enum { PAUSE_THERE = 256 };

static pthread_mutex_t g_mu = PTHREAD_MUTEX_INITIALIZER;
static dm_resume_marks g_marks;
static dm_resume_run g_run;
static dm_resume_pos g_here;
static char g_side_who[DM_RESUME_NAME_MAX];
static _Atomic int g_client_level = -1;
static _Atomic int g_want_client;
static _Atomic uint64_t g_shield;

static int mark_path(char *out, size_t cap) { const char *c = dm_save_commitdir(); return c && snprintf(out, cap, "%s.resume_mark", c) < (int)cap; }
static int switch_on(void) { return dm_custom_get("resume_pos") != 0; }

static void mark_remove(const char *why) {
    char p[1200]; if (!mark_path(p, sizeof p)) return;
    if (!dm_resume_mark_delete(p)) LOG("resume.mark_remove_failed: the resume marker could not be removed (%s); the fingerprint still guards the save", why);
}

void dm_resume_app_left(void) { pthread_mutex_lock(&g_mu); dm_resume_left(&g_marks); mark_remove("app left"); pthread_mutex_unlock(&g_mu); }
void dm_resume_app_back(void) { pthread_mutex_lock(&g_mu); if (dm_resume_back(&g_marks)) mark_remove("back in the app"); pthread_mutex_unlock(&g_mu); }

void dm_resume_exit_game(void) { pthread_mutex_lock(&g_mu); dm_resume_dropped(&g_marks); mark_remove("save and exit"); pthread_mutex_unlock(&g_mu); }

unsigned dm_resume_leave_save_begin(void) { pthread_mutex_lock(&g_mu); unsigned g = dm_resume_leave_begin(&g_marks); pthread_mutex_unlock(&g_mu); return g; }
void dm_resume_leave_save_end(unsigned gen) {
    pthread_mutex_lock(&g_mu);
    if (dm_resume_leave_end(&g_marks, gen)) {
        char p[1200]; const char *cd = dm_save_commitdir();
        if (!cd || !dm_resume_committed(cd, g_side_who)) LOG("resume: no marker, the committed save holds no matching position for %s", g_side_who);
        else if (mark_path(p, sizeof p) && dm_resume_mark_write(p, g_side_who)) LOG("resume: marker published for %s", g_side_who);
        else LOG("resume.mark_write_failed: the resume marker could not be written; no way back next time");
    }
    pthread_mutex_unlock(&g_mu);
}



static void pre_commit(const char *wd) {
    pthread_mutex_lock(&g_mu);
    if (dm_resume_sidecar_due(&g_marks) && g_here.who[0] && switch_on()) {
        dm_resume_pos p = g_here;
        p.save_fp = dm_resume_save_fp(wd, p.who);
        if (p.save_fp && dm_resume_sidecar_write(wd, &p)) {
            dm_resume_sidecar_done(&g_marks); strcpy(g_side_who, p.who);
            LOG("resume: position area %d at %d,%d%s written for %s", p.level, p.x, p.y, p.boss ? " (a boss died there)" : "", p.who);
        } else if (p.save_fp) LOG("resume.sidecar_write_failed: the resume position could not be written for %s", p.who);
    }
    pthread_mutex_unlock(&g_mu);
}
__attribute__((constructor)) static void hook_install(void) { extern void (*g_dm_pre_commit_hook[DM_PRE_COMMIT_HOOKS])(const char *); g_dm_pre_commit_hook[8] = pre_commit; }

static uint64_t map_fp(uint64_t lv) {
    uint64_t fp = DM_RESUME_FP_START; int n = 0;
    for (uint64_t r = dm_level_rooms(lv); r && n < 2000; r = dm_unit_rd64(r + 0x48), n++)
        fp = dm_resume_fp_room(fp, *(int32_t *)(uintptr_t)(r + 0x60), *(int32_t *)(uintptr_t)(r + 0x64), *(int32_t *)(uintptr_t)(r + 0x68), *(int32_t *)(uintptr_t)(r + 0x6c));
    return n ? fp : 0;
}
static int unit_tile(uint64_t u, int *x, int *y) { uint64_t p = dm_unit_rd64(u + 0x38); if (!p) return 0; *x = *(uint16_t *)(uintptr_t)(p + 2); *y = *(uint16_t *)(uintptr_t)(p + 6); return *x && *y; }
static uint32_t rd32(uint64_t a) { return *(volatile uint32_t *)(uintptr_t)a; }


static int acting(uint32_t mode) { return mode == 7 || mode == 8 || (mode >= 10 && mode <= 16) || mode == 18; }


static uint8_t g_boss_area[32];
static int boss_died(int level) { return level > 0 && level < 256 && (g_boss_area[level / 8] >> (level % 8) & 1); }
static void boss_scan(uint64_t G, int player_level) {
    for (int b = 0; b < 128; b++) for (uint64_t u = dm_unit_rd64(G + 0x2230 + 0x400 + 8 * (uint64_t)b), k = 0; u && k < 256; u = dm_unit_rd64(u + 0x158), k++) {
        uint32_t m = rd32(u + 0xc); if (m != 0 && m != 12) continue;
        uint64_t md = dm_unit_rd64(u + 0x10);
        if (dm_threat_tier(u) != 3 && !(md && (*(volatile uint8_t *)(uintptr_t)(md + 0x1a) & 2))) continue;
        int lv = dm_unit_level(u); if (lv <= 0) lv = player_level;
        if (lv <= 0 || lv >= 256 || boss_died(lv)) continue;
        g_boss_area[lv / 8] |= (uint8_t)(1 << (lv % 8));
        LOG("resume: a boss or super unique (class %u) died in area %d; a position there goes to the nearest waypoint", rd32(u + 4), lv);
    }
}


static int nearest_wp(uint8_t *b, uint64_t G, const dm_resume_pos *p, const char *dir) {
    static dm_resume_level lv[1024];
    uint8_t wps[DM_RESUME_WP_BYTES];
    if (!dm_resume_save_wps(dir, p->who, p->difficulty, wps)) { LOG("ASSUME resume.waypoints: the character file of %s has no readable waypoint section, stays in town", p->who); return 0; }
    int ver = *(volatile uint8_t *)(uintptr_t)(G + 0x106);
    uint64_t t = ver >= 1 && ver <= 3 ? *(uint64_t *)(uintptr_t)(b + 0x2d64a18 + (uint64_t)ver * 16) : 0;
    uint64_t levels = t ? dm_unit_rd64(t + 0x1480) : 0, defs = t ? dm_unit_rd64(t + 0x14c0) : 0;
    int64_t nl = t ? (int64_t)dm_unit_rd64(t + 0x1488) : 0, nd = t ? (int64_t)dm_unit_rd64(t + 0x14c8) : 0, n = nl < nd ? nl : nd;
    if (!levels || !defs || n <= p->level || n > 1024) { LOG("ASSUME resume.level_tables: no level tables (version %d, %lld levels, %lld definitions), stays in town", ver, (long long)nl, (long long)nd); return 0; }
    for (int i = 0; i < n; i++) {
        uint64_t r = levels + 0x194 * (uint64_t)i, d = defs + 0x9c * (uint64_t)i; uint8_t w = *(volatile uint8_t *)(uintptr_t)(r + 0xf4);
        if (*(volatile uint8_t *)(uintptr_t)(r + 4) != (uint8_t)i) { LOG("ASSUME resume.level_tables: level record %d names %u, stays in town", i, *(volatile uint8_t *)(uintptr_t)(r + 4)); return 0; }
        lv[i].act = *(volatile uint8_t *)(uintptr_t)(r + 0xd); lv[i].wp = w == 255 ? -1 : w;
        for (int k = 0; k < 8; k++) lv[i].vis[k] = *(volatile int32_t *)(uintptr_t)(d + 0x48 + 4 * (uint64_t)k);
    }
    int to = dm_resume_nearest_wp(p->level, lv, (int)n, wps);
    char nb[160] = ""; size_t k = 0;
    for (int i = 0; i < 8 && k < sizeof nb - 16; i++) { int v = lv[p->level].vis[i]; if (v > 0 && v < n) k += (size_t)snprintf(nb + k, sizeof nb - k, " %d(act %d, waypoint %d)", v, lv[v].act + 1, lv[v].wp); }
    LOG("resume: nearest activated waypoint of area %d (act %d): %s %d; neighbours%s; activated %02x %02x %02x %02x %02x", p->level, lv[p->level].act + 1,
        to ? "area" : "none outside town,", to, nb[0] ? nb : " none", wps[0], wps[1], wps[2], wps[3], wps[4]);
    return to;
}


void dm_resume_client_pump(uint8_t *b) {
    if (!b || !atomic_load(&g_want_client)) return;
    uint64_t pl = ((uint64_t (*)(uint32_t))(b + 0x45f9d0))(*(uint32_t *)(b + 0x2d20e4c));
    atomic_store(&g_client_level, pl ? dm_unit_level(pl) : -1);
}


int dm_resume_shields(uint64_t unit) { uint64_t s = atomic_load_explicit(&g_shield, memory_order_relaxed); return s && s == unit; }


int dm_resume_there(void) { pthread_mutex_lock(&g_mu); int lv = g_run.st == DM_RESUME_THERE ? g_run.target : 0; pthread_mutex_unlock(&g_mu); return lv; }
int dm_resume_leave(int to_camp) {
    pthread_mutex_lock(&g_mu); int ok = dm_resume_leave_there(&g_run, to_camp); if (ok && to_camp) atomic_store(&g_want_client, 0); pthread_mutex_unlock(&g_mu);
    if (ok) { dm_pause_want(PAUSE_THERE, 0); LOG("resume: %s", to_camp ? "to camp" : "continue here"); }
    return ok;
}

void dm_resume_tick(uint64_t G, uint64_t player, uint8_t *b) {
    static uint64_t game; static uint32_t last_t, joined; static uint64_t fp_level, fp; static char who[DM_RESUME_NAME_MAX];
    uint32_t t = *(volatile uint32_t *)(uintptr_t)(G + 0x170);
    if (G != game || t < last_t) {
        game = G; joined = 0; fp_level = 0; who[0] = 0; memset(g_boss_area, 0, sizeof g_boss_area);
        pthread_mutex_lock(&g_mu); memset(&g_run, 0, sizeof g_run); g_here.who[0] = 0; pthread_mutex_unlock(&g_mu);
        atomic_store(&g_want_client, 0); atomic_store(&g_shield, 0);
    }
    last_t = t;
    if (!who[0]) { uint64_t pd = dm_unit_rd64(player + 0x10); if (!pd) return; char nm[64]; strlcpy(nm, (const char *)(uintptr_t)pd, sizeof nm); if (!nm[0] || strlen(nm) >= sizeof who) return; strcpy(who, nm); }
    uint64_t lvp = dm_unit_level_ptr(player); int lv = lvp ? *(volatile int32_t *)(uintptr_t)(lvp + 0x1f8) : 0, x = 0, y = 0;
    int difficulty = *(volatile uint8_t *)(uintptr_t)(G + 0x104);
    if (lvp != fp_level) { fp_level = lvp; fp = lvp ? map_fp(lvp) : 0; }
    else if (lvp && t % 25 == 0) fp = map_fp(lvp);
    if (t % 5 == 0 && lv && !dm_resume_is_town(lv)) boss_scan(G, lv);
    uint32_t mode = rd32(player + 0xc);
    int allowed = lv && fp && unit_tile(player, &x, &y) && lv < 256 && !dm_resume_is_town(lv) && !dm_hud_player_mode_dead(mode) && !dm_rift_active() && !dm_bossrush_stage();
    pthread_mutex_lock(&g_mu);
    if (allowed) { strcpy(g_here.who, who); g_here.level = lv; g_here.x = x; g_here.y = y; g_here.difficulty = difficulty; g_here.boss = boss_died(lv); g_here.map_fp = fp; }
    else g_here.who[0] = 0;
    pthread_mutex_unlock(&g_mu);


    if (!joined && lv && !dm_loadscreen_up() && dm_uiscreen_ingame()) {
        joined = 1;
        char mp[1200]; const char *cd = dm_save_commitdir(); dm_resume_pos p; int failed = 0;
        memset(&p, 0, sizeof p);
        if (cd && mark_path(mp, sizeof mp)) {
            pthread_mutex_lock(&g_mu);
            dm_resume_why w = dm_resume_join(mp, cd, who, switch_on() && dm_resume_is_town(lv), difficulty, &p, &failed);
            pthread_mutex_unlock(&g_mu);
            int target = w == DM_RESUME_RETURN ? p.level : w == DM_RESUME_WAYPOINT ? nearest_wp(b, G, &p, cd) : 0;
            if (target) { pthread_mutex_lock(&g_mu); dm_resume_go(&g_run, &p, target, w == DM_RESUME_RETURN, lv); pthread_mutex_unlock(&g_mu); }
            if (failed) LOG("resume.consume_failed: the resume marker of %s could not be removed, stays in town", who);
            else if (w == DM_RESUME_RETURN) LOG("resume: back to area %d at %d,%d for %s", p.level, p.x, p.y, who);
            else if (w == DM_RESUME_WAYPOINT) LOG("resume: area %d %s; %s", p.level, p.boss ? "had a boss or super unique die" : "is not resumed on the spot",
                                                  target ? "to the nearest waypoint" : "stays in town");
            else if (w != DM_RESUME_NO_MARK) LOG("resume: stays in town for %s: %s", who, w == DM_RESUME_SWITCH_OFF && !dm_resume_is_town(lv) ? "not in town" : dm_resume_why_name(w));
        }
    }

    pthread_mutex_lock(&g_mu);
    dm_resume_act act = dm_resume_step(&g_run, t, lv, atomic_load(&g_client_level), acting(mode));
    dm_resume_run run = g_run; int shielded = dm_resume_shielded(&g_run);
    pthread_mutex_unlock(&g_mu);
    atomic_store(&g_shield, shielded ? player : 0);
    switch (act) {
    case DM_RESUME_ACT_WARP: atomic_store(&g_client_level, -1); atomic_store(&g_want_client, 1); dm_warp(b, G, player, run.target); LOG("resume: area change to %d", run.target); return;
    case DM_RESUME_ACT_CAMP: dm_warp(b, G, player, run.camp); LOG("resume: area change to the town %d", run.camp); return;
    case DM_RESUME_ACT_SHIELD_OFF: LOG("resume: protection ends (%s)", acting(mode) ? "own attack or skill" : "time"); return;
    case DM_RESUME_ACT_NONE: return;
    default: break;
    }

    atomic_store(&g_want_client, 0);
    dm_pause_want(PAUSE_THERE, 1);
    if (act == DM_RESUME_ACT_GIVE_UP) { LOG("ASSUME resume.map_same: the client did not arrive in area %d in time, the player stays where the area change put him", run.target); return; }
    int fx = 0, fy = 0;
    if (!run.on_spot) { unit_tile(player, &fx, &fy); LOG("resume: at the waypoint of area %d at %d,%d", run.target, fx, fy); return; }
    uint64_t now_fp = map_fp(lvp);
    if (now_fp != run.pos.map_fp) { LOG("ASSUME resume.map_same: area %d has another room list (%016llx, was %016llx), the player stays at the area's entry", run.target, (unsigned long long)now_fp, (unsigned long long)run.pos.map_fp); return; }
    int32_t pos[2] = { run.pos.x, run.pos.y };
    uint64_t room = dm_level_room_load(b, G, lvp, pos[0], pos[1]), spot = room ? dm_free_spot(b, G, room, pos) : 0;
    int moved = spot && dm_teleport(b, G, player, spot, pos[0], pos[1]);
    unit_tile(player, &fx, &fy);
    if (moved) LOG("resume: placed in area %d at %d,%d (saved %d,%d)", run.target, fx, fy, run.pos.x, run.pos.y);
    else LOG("ASSUME resume.map_same: area %d tile %d,%d %s, the player stays at %d,%d", run.target, run.pos.x, run.pos.y, !room ? "has no room" : !spot ? "has no free ground near it" : "refused the move", fx, fy);
}
