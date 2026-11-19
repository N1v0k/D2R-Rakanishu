



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

static pthread_mutex_t g_mu = PTHREAD_MUTEX_INITIALIZER;
static dm_resume_marks g_marks;
static dm_resume_run g_run;
static dm_resume_pos g_here;
static char g_side_who[DM_RESUME_NAME_MAX];
static _Atomic int g_client_level = -1;
static _Atomic int g_want_client;

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
        else LOG("resume.mark_write_failed: the resume marker could not be written; no offer next time");
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
            LOG("resume: position area %d at %d,%d written for %s", p.level, p.x, p.y, p.who);
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


void dm_resume_client_pump(uint8_t *b) {
    if (!b || !atomic_load(&g_want_client)) return;
    uint64_t pl = ((uint64_t (*)(uint32_t))(b + 0x45f9d0))(*(uint32_t *)(b + 0x2d20e4c));
    atomic_store(&g_client_level, pl ? dm_unit_level(pl) : -1);
}


int dm_resume_offered(void) { pthread_mutex_lock(&g_mu); int lv = g_run.st == DM_RESUME_ASKING ? g_run.pos.level : 0; pthread_mutex_unlock(&g_mu); return lv; }
int dm_resume_choice(int here) {
    pthread_mutex_lock(&g_mu); int ok = dm_resume_choose(&g_run, here); if (ok && here) atomic_store(&g_want_client, 1); pthread_mutex_unlock(&g_mu);
    if (ok) LOG("resume: chosen %s", here ? "continue here" : "stay in camp");
    return ok;
}

void dm_resume_tick(uint64_t G, uint64_t player, uint8_t *b) {
    static uint64_t game; static uint32_t last_t, joined; static uint64_t fp_level, fp; static char who[DM_RESUME_NAME_MAX];
    uint32_t t = *(volatile uint32_t *)(uintptr_t)(G + 0x170);
    if (G != game || t < last_t) {
        game = G; joined = 0; fp_level = 0; who[0] = 0;
        pthread_mutex_lock(&g_mu); memset(&g_run, 0, sizeof g_run); g_here.who[0] = 0; pthread_mutex_unlock(&g_mu);
        atomic_store(&g_want_client, 0);
    }
    last_t = t;
    if (!who[0]) { uint64_t pd = dm_unit_rd64(player + 0x10); if (!pd) return; char nm[64]; strlcpy(nm, (const char *)(uintptr_t)pd, sizeof nm); if (!nm[0] || strlen(nm) >= sizeof who) return; strcpy(who, nm); }
    uint64_t lvp = dm_unit_level_ptr(player); int lv = lvp ? *(volatile int32_t *)(uintptr_t)(lvp + 0x1f8) : 0, x = 0, y = 0;
    int difficulty = *(volatile uint8_t *)(uintptr_t)(G + 0x104);
    if (lvp != fp_level) { fp_level = lvp; fp = lvp ? map_fp(lvp) : 0; }
    else if (lvp && t % 25 == 0) fp = map_fp(lvp);
    int allowed = lv && fp && unit_tile(player, &x, &y) && dm_resume_level_allowed(lv) && !dm_hud_player_mode_dead(*(volatile uint32_t *)(uintptr_t)(player + 0xc))
                  && !dm_rift_active() && !dm_bossrush_stage();
    pthread_mutex_lock(&g_mu);
    if (allowed) { strcpy(g_here.who, who); g_here.level = lv; g_here.x = x; g_here.y = y; g_here.difficulty = difficulty; g_here.map_fp = fp; }
    else g_here.who[0] = 0;
    pthread_mutex_unlock(&g_mu);


    if (!joined && lv && !dm_loadscreen_up() && dm_uiscreen_ingame()) {
        joined = 1;
        char mp[1200]; const char *cd = dm_save_commitdir(); dm_resume_pos p; int failed = 0;
        if (cd && mark_path(mp, sizeof mp)) {
            pthread_mutex_lock(&g_mu);
            dm_resume_why w = dm_resume_join(mp, cd, who, switch_on() && dm_resume_is_town(lv), difficulty, &p, &failed);
            if (w == DM_RESUME_OFFER) dm_resume_ask(&g_run, &p);
            pthread_mutex_unlock(&g_mu);
            if (failed) LOG("resume.consume_failed: the resume marker of %s could not be removed, no offer", who);
            else if (w == DM_RESUME_OFFER) LOG("resume: offering area %d at %d,%d to %s", p.level, p.x, p.y, who);
            else if (w == DM_RESUME_LEVEL) LOG("ASSUME resume.level_allowed: the sidecar of %s names area %d, which is never written", who, p.level);
            else if (w != DM_RESUME_NO_MARK) LOG("resume: no offer for %s: %s", who, w == DM_RESUME_SWITCH_OFF && !dm_resume_is_town(lv) ? "not in town" : dm_resume_why_name(w));
        }
    }

    pthread_mutex_lock(&g_mu);
    dm_resume_act act = dm_resume_step(&g_run, t, lv, atomic_load(&g_client_level)); dm_resume_pos want = g_run.pos;
    pthread_mutex_unlock(&g_mu);
    if (act == DM_RESUME_ACT_WARP) { atomic_store(&g_client_level, -1); dm_warp(b, G, player, want.level); LOG("resume: area change to %d", want.level); return; }
    if (act == DM_RESUME_ACT_NONE) return;
    atomic_store(&g_want_client, 0);
    if (act == DM_RESUME_ACT_GIVE_UP) { LOG("ASSUME resume.map_same: the client did not arrive in area %d in time, the player stays at the area's entry", want.level); return; }
    uint64_t now_fp = map_fp(lvp);
    if (now_fp != want.map_fp) { LOG("ASSUME resume.map_same: area %d has another room list (%016llx, was %016llx), the player stays at the area's entry", want.level, (unsigned long long)now_fp, (unsigned long long)want.map_fp); return; }
    int32_t pos[2] = { want.x, want.y };
    uint64_t room = dm_level_room_load(b, G, lvp, pos[0], pos[1]), spot = room ? dm_free_spot(b, G, room, pos) : 0;
    int moved = spot && dm_teleport(b, G, player, spot, pos[0], pos[1]), fx = 0, fy = 0;
    unit_tile(player, &fx, &fy);
    if (moved) LOG("resume: placed in area %d at %d,%d (saved %d,%d)", want.level, fx, fy, want.x, want.y);
    else LOG("ASSUME resume.map_same: area %d tile %d,%d %s, the player stays at %d,%d", want.level, want.x, want.y, !room ? "has no room" : !spot ? "has no free ground near it" : "refused the move", fx, fy);
}
