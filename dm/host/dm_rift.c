




























#include "dm_rift.h"
#include "game_addrs.h"
#include <errno.h>
#include "dm_bossrush.h"
#include "dm_hud.h"
#include "dm_unit.h"
#include "dm_doorspot.h"
#include "dm_i18n.h"
#include "dm_cursor.h"
#include "hle.h"
#include "dm.h"
#include <stdarg.h>
#include <pthread.h>
#include <sys/stat.h>
#include <math.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

enum { LV_TOWN = 109, RIFT_OBJ = 342, TOWN_OBJ = 298, RIFT_SECS = DM_RIFT_SECS };


static const int k_open[] = { 3, 7, 43, 111, 112, 117 };
static const int k_maze[] = { 12, 16, 52, 53, 54, 55, 56, 57, 59, 60, 122, 123, 125, 126, 127 };
static const int k_guard[] = { 156, 211, 242, 243, 544 };

static uint8_t *B;
static _Atomic int g_merc_req;
static uint64_t rd(uint64_t a) { return a ? *(volatile uint64_t *)(uintptr_t)a : 0; }
static uint32_t tick_of(uint64_t G) { return *(volatile uint32_t *)(uintptr_t)(G + 0x170); }
static uint64_t level_of(uint64_t u) { return dm_unit_level_ptr(u); }
static int unit_level(uint64_t u) { return dm_unit_level(u); }
static uint64_t unit_room(uint64_t u) { uint64_t p = rd(u + 0x38); return !p ? 0 : *(uint32_t *)(uintptr_t)u == 2 ? rd(p) : rd(p + 0x20); }
static int unit_xy(uint64_t u, int *x, int *y) {
    uint64_t p = rd(u + 0x38); if (!p) return 0; uint32_t t = *(uint32_t *)(uintptr_t)u;
    if (t == 2) { *x = *(int32_t *)(uintptr_t)(p + 0x10); *y = *(int32_t *)(uintptr_t)(p + 0x14); } else { *x = *(uint16_t *)(uintptr_t)(p + 2); *y = *(uint16_t *)(uintptr_t)(p + 6); }
    return 1;
}
static int ground_items(uint64_t G) { int n = 0; for (int b = 0; b < 128; b++) for (uint64_t u = rd(G + 0x2e30 + 8 * b); u; u = rd(u + 0x158)) n += *(uint32_t *)(uintptr_t)u == 4 && *(uint32_t *)(uintptr_t)(u + 0xc) == 3; return n; }
static uint64_t find_unit(uint64_t G, int type, uint32_t id) { for (int b = 0; b < 128; b++) for (uint64_t u = rd(G + 0x2230 + type * 0x400 + 8 * b); u; u = rd(u + 0x158)) if (*(uint32_t *)(uintptr_t)(u + 8) == id) return u; return 0; }
static int gs(uint64_t u, int st) { return ((int (*)(uint64_t, int, int))(B + GA_STAT_GET))(u, st, 0); }
static void ss(uint64_t u, int st, int v) { ((void (*)(uint64_t, int, int, int))(B + GA_STAT_SET))(u, st, v, 0); }






static int gsb(uint64_t u, int st) { uint64_t sl = rd(u + 0x88); const uint32_t *a = sl ? (const uint32_t *)(uintptr_t)rd(sl + 0x30) : NULL; uint64_t n = sl ? rd(sl + 0x38) : 0;
    if (!a || n > 4096) return 0; for (uint64_t i = 0; i < n; i++) if (a[i * 2] == (uint32_t)st << 16) return (int)a[i * 2 + 1]; return 0; }
static uint64_t free_spot(uint64_t G, uint64_t room, int32_t pos[2]) { return dm_free_spot(B, G, room, pos); }
static void level_origin(uint64_t lv, int *x0, int *y0) { *x0 = *y0 = 1 << 30; int n = 0;
    for (uint64_t r = dm_level_rooms(lv); r && n < 2000; r = rd(r + 0x48), n++) { int x = *(int32_t *)(uintptr_t)(r + 0x60) * 5, y = *(int32_t *)(uintptr_t)(r + 0x64) * 5; if (x < *x0) *x0 = x; if (y < *y0) *y0 = y; } }
static uint64_t room_at(uint64_t lv, int px, int py) { int n = 0; for (uint64_t r = dm_level_rooms(lv); r && n < 2000; r = rd(r + 0x48), n++) { int x = *(int32_t *)(uintptr_t)(r + 0x60) * 5, y = *(int32_t *)(uintptr_t)(r + 0x64) * 5, w = *(int32_t *)(uintptr_t)(r + 0x68) * 5, h = *(int32_t *)(uintptr_t)(r + 0x6c) * 5;
    if (px >= x && px < x + w && py >= y && py < y + h) return rd(r + 0x58); } return 0; }


static uint64_t room_load(uint64_t G, uint64_t lv, int px, int py) { return dm_level_room_load(B, G, lv, px, py); }
static void warp(uint64_t G, uint64_t player, int lv) { dm_warp(B, G, player, lv); }
static uint64_t tables(uint64_t G) { return rd((uint64_t)(uintptr_t)(B + GA_DATA_TABLES + *(uint8_t *)(uintptr_t)(G + 0x106) * 16)); }



static void say(const char *key, ...) { char line[160], full[180]; va_list ap; va_start(ap, key); vsnprintf(line, sizeof line, dm_str(B, key), ap); va_end(ap);
    snprintf(full, sizeof full, dm_str(B, "dmRiftSayPrefix"), line); dm_game_notice(B, full); LOG("秘境：提示「%s」", full); }



enum { ST_IDLE, ST_RUN, ST_DONE };
static struct {
    _Atomic int st; int tier, maps[3], nmaps, cur;
    _Atomic int gdead, gtotal;
    int nwave, wave, wcls[3][2], wn[3], whp[3], wdm[3]; uint32_t wave_t;
    _Atomic int score, need; uint64_t t0_ns; _Atomic int timeout, guard_dead;
    int su_order[43], su_next, su_cnt; uint32_t su_live[3], su_seen[3], su_near[3], su_last;
    int inv_n, inv_cls[2], inv_pct[2], inv_st[2]; uint32_t inv_id[2], inv_t[2];
    uint32_t guard, guards[3]; int guard_cls, gcls[3], nplan, inc;
    int players_old;
    char who[64];
    int resume_tier;
} K;
static struct { uint64_t G, player; uint32_t last_t; int eligible; uint32_t door, next_door, town_door, pend, pend_at; int door_tried, lv_prev, dead; int used[24], nused; int redir; int last_lv, last_x, last_y, nd_lv, nd_x, nd_y, tp_lv, tp_x, tp_y, resume, gift_due; uint32_t tp_until; } L;


enum { MENU_NONE, MENU_SELECT, MENU_CHOICE, MENU_NEXT };
static struct { _Atomic int menu, req_start, req_cont, req_abandon, req_next, clvl; } M;











const char *dm_save_workdir(void);
enum { RIFT_UNLOCK0 = 8 };
static struct { int tier, best, clears, deaths, unlock, gift, pts; unsigned ms, sunder; int fast[101]; } R;
static struct { pthread_mutex_t mu; int dirty; uint64_t wrote; char who[64]; __typeof__(R) snap; } RS = { .mu = PTHREAD_MUTEX_INITIALIZER };
static int rec_path(char *o, size_t n) { const char *w = dm_save_workdir(); if (!w || !K.who[0]) return 0; snprintf(o, n, "%s/%s/%s.sw_rift", w, K.who, K.who); return 1; }
static int g_rec_ok;
static void rec_load(void) {
    g_rec_ok = 0;
    { int pend; pthread_mutex_lock(&RS.mu); pend = (RS.dirty || RS.wrote) && !strcmp(RS.who, K.who); if (pend) R = RS.snap; pthread_mutex_unlock(&RS.mu); if (pend) { g_rec_ok = 1; LOG("rift: a snapshot of the record is still waiting to be written (from the last game in this process); using it instead of the file"); return; } }
    memset(&R, 0, sizeof R); R.tier = 1; R.unlock = RIFT_UNLOCK0; int saw_best = 0; char p[1200], ln[128]; if (!rec_path(p, sizeof p)) return; errno = 0; FILE *f = fopen(p, "r"); if (!f) { g_rec_ok = errno == ENOENT; return; }
    while (fgets(ln, sizeof ln, f)) { int a, b; unsigned m;
        if (sscanf(ln, "tier %d", &a) == 1) R.tier = a; else if (sscanf(ln, "best %d", &a) == 1) { R.best = a; saw_best = 1; } else if (sscanf(ln, "clears %d", &a) == 1) R.clears = a;
        else if (sscanf(ln, "deaths %d", &a) == 1) R.deaths = a; else if (sscanf(ln, "unlock %d", &a) == 1) R.unlock = a; else if (sscanf(ln, "gift %d", &a) == 1) R.gift = a; else if (sscanf(ln, "pts %d", &a) == 1) R.pts = a; else if (sscanf(ln, "ms %x", &m) == 1) R.ms = m; else if (sscanf(ln, "sunder %x", &m) == 1) R.sunder = m;
        else if (sscanf(ln, "fast %d %d", &a, &b) == 2 && a >= 1 && a <= 100) R.fast[a] = b; }
    g_rec_ok = !ferror(f); fclose(f); if (!saw_best && R.tier > 1) { R.best = R.tier - 3 < 1 ? 1 : R.tier - 3; LOG("秘境：旧侧文件没有最高通关（早期包只记下次层数 %d），按每次最多涨 3 层补成 %d", R.tier, R.best); }
    if (R.tier < 1) R.tier = 1; if (R.tier > 100) R.tier = 100; if (R.best < 0) R.best = 0; if (R.best > 100) R.best = 100; if (R.unlock < R.tier) R.unlock = R.tier; if (R.unlock < RIFT_UNLOCK0) R.unlock = RIFT_UNLOCK0; if (R.unlock > 100) R.unlock = 100;
}












uint64_t dm_fs_commit_count(void);
static void rec_save(void) { if (!K.who[0]) return; pthread_mutex_lock(&RS.mu); RS.snap = R; strlcpy(RS.who, K.who, sizeof RS.who); RS.dirty = 1; RS.wrote = 0; pthread_mutex_unlock(&RS.mu); }
static void rec_pre_commit(const char *wd) {
    pthread_mutex_lock(&RS.mu);
    if (RS.wrote && dm_fs_commit_count() >= RS.wrote) RS.wrote = 0;
    else if (RS.wrote) { RS.wrote = 0; RS.dirty = 1; }
    if (RS.dirty && wd) { char d[1200], c[1200], p[1200], q[1220]; size_t L = strlen(wd); struct stat a, b;
        snprintf(d, sizeof d, "%s/%s/%s.d2s", wd, RS.who, RS.who); snprintf(c, sizeof c, "%.*s/%s/%s.d2s", (int)(L > 5 && !strcmp(wd + L - 5, ".work") ? L - 5 : L), wd, RS.who, RS.who);
        int fresh = !stat(d, &a) && (stat(c, &b) || a.st_mtimespec.tv_sec != b.st_mtimespec.tv_sec || a.st_mtimespec.tv_nsec != b.st_mtimespec.tv_nsec || a.st_size != b.st_size);
        if (fresh) { snprintf(p, sizeof p, "%s/%s/%s.sw_rift", wd, RS.who, RS.who); snprintf(q, sizeof q, "%s.tmp", p); FILE *f = fopen(q, "w");
            if (!f) LOG("【红】秘境：侧文件写不进 %s", q);
            else { fprintf(f, "tier %d\nunlock %d\nbest %d\nclears %d\ndeaths %d\nms %x\nsunder %x\ngift %d\npts %d\n", RS.snap.tier, RS.snap.unlock, RS.snap.best, RS.snap.clears, RS.snap.deaths, RS.snap.ms, RS.snap.sunder, RS.snap.gift, RS.snap.pts);
                for (int t = 1; t <= 100; t++) if (RS.snap.fast[t]) fprintf(f, "fast %d %d\n", t, RS.snap.fast[t]);
                if (fclose(f) || rename(q, p)) { remove(q); LOG("【红】秘境：侧文件换名失败"); }
                else { RS.dirty = 0; RS.wrote = dm_fs_commit_count() + 1; LOG("秘境：存盘 下次第 %d 层、最高 %d、通关 %d、死亡 %d（随存档提交 #%llu，和角色档一起：%s）", RS.snap.tier, RS.snap.best, RS.snap.clears, RS.snap.deaths, (unsigned long long)RS.wrote, p); } } }
        else { static int n; if (n++ < 20) LOG("秘境：进度待落盘，这次提交没有改写 %s 的角色档，等下一次", RS.who); } }
    pthread_mutex_unlock(&RS.mu);
}
__attribute__((constructor)) static void rec_hook_install(void) { extern void (*g_dm_pre_commit_hook[DM_PRE_COMMIT_HOOKS])(const char *); g_dm_pre_commit_hook[3] = rec_pre_commit; }






















static int pts_env(const char *k, int def, int lo, int hi) { const char *e = getenv(k); int v = e ? atoi(e) : def; return v < lo || v > hi ? def : v; }
static int pts_grant(uint64_t player) {
    int every = pts_env("DM_RIFT_PTS_EVERY", 2, 1, 100), ps = pts_env("DM_RIFT_PTS_SKILL", 1, 0, 20), pa = pts_env("DM_RIFT_PTS_STAT", 5, 0, 50);
    int due = R.best / every - R.pts / every, n = 0, sk0 = gs(player, 5), st0 = gs(player, 4);
    if (due <= 0) { if (R.pts < R.best) R.pts = R.best; return 0; }
    while (n < due && sk0 + (n + 1) * ps <= 255 && st0 + (n + 1) * pa <= 1023) n++;
    if (n) { if (ps) ss(player, 5, sk0 + n * ps); if (pa) ss(player, 4, st0 + n * pa); }
    R.pts = n == due ? R.best : (R.pts / every + n) * every;
    LOG("秘境：通关点数 最高通关 %d 层、该发 %d 份、发了 %d 份（每 %d 层 %d 技能点 %d 属性点）→ 未用技能点 %d → %d、未用属性点 %d → %d，已发到第 %d 层%s", R.best, due, n, every, ps, pa, sk0, gs(player, 5), st0, gs(player, 4), R.pts, n < due ? "【注意】未用点数太多放不下，剩下的等用掉后再补" : "");
    if (n) {
        char f[160], sk[64], st[64];
        snprintf(sk, sizeof sk, dm_strn(B, "dmRiftPtsSkill", n * ps, f, sizeof f), n * ps); snprintf(st, sizeof st, dm_strn(B, "dmRiftPtsStat", n * pa, f, sizeof f), n * pa);
        say("dmRiftPtsGot", R.best, sk, st);
    }
    if (n < due) say("dmRiftPtsHeld");
    return n;
}







void dm_rift_gamble_vals(int best, uint32_t *uniq, uint32_t *set) { if (best > 100) best = 100; *uniq = best >= 1 ? 10000u + 100u * (uint32_t)(best - 1) : 0; *set = *uniq * 2; }
static struct { uint64_t row; uint32_t uniq, set; int on; } GB;
static void gamble_restore(void) { if (!GB.on) return; *(uint32_t *)(uintptr_t)(GB.row + 0x74) = GB.uniq; *(uint32_t *)(uintptr_t)(GB.row + 0x70) = GB.set; GB.on = 0; LOG("秘境：赌博加成撤掉（暗金 %u、套装 %u，十万分之一）", GB.uniq, GB.set); }












static struct { uint64_t arr, caps; uint32_t *save_arr; uint32_t save_caps[100], n; int on; uint32_t bm[128]; int bm_on, nbm; uint64_t bmG; } GS;
static void shelf_restore(void) {
    if (GS.on) { memcpy((void *)(uintptr_t)GS.arr, GS.save_arr, GS.n * 4); memcpy((void *)(uintptr_t)GS.caps, GS.save_caps, sizeof GS.save_caps); GS.on = 0; LOG("秘境：赌博货架写回原版"); }
    if (GS.bm_on) { uint32_t *w = (uint32_t *)(uintptr_t)(GS.bmG + 0x3638); for (int i = 0; i < GS.nbm; i++) w[i] |= GS.bm[i]; GS.bm_on = 0; }
}
static void shelf_apply(uint64_t G, uint64_t player) {
    uint64_t T = tables(G); if (!T) return;
    if (!GS.bm_on) { uint64_t nu = rd(T + 0x13f0); GS.nbm = (int)((nu + 31) / 32); if (GS.nbm > 128) GS.nbm = 128; GS.bmG = G; memcpy(GS.bm, (void *)(uintptr_t)(G + 0x3638), GS.nbm * 4); GS.bm_on = 1; }
    memset((void *)(uintptr_t)(G + 0x3638), 0, GS.nbm * 4);
    if (GS.on) return;
    uint64_t arr = rd(T + 0x16d8), it = rd(T + 0x15c0), ut = rd(T + 0x13e8); uint32_t n = *(uint32_t *)(uintptr_t)(T + 0x16e0), nu = (uint32_t)rd(T + 0x13f0), ni = *(uint32_t *)(uintptr_t)(T + 0x15c8);
    uint32_t *caps = (uint32_t *)(uintptr_t)(T + 0x16f0), *a = (uint32_t *)(uintptr_t)arr; if (!arr || !it || !ut || !n || n > 4096) return;


    for (int L = 5; L < 100; L++) { uint32_t c = 0; for (uint32_t k = 0; k < n; k++) if (a[k] < ni && *(uint8_t *)(uintptr_t)(it + (uint64_t)a[k] * 0x1c0 + 0x10d) <= L) c++;
        if (c != caps[L]) { LOG("【红】秘境：赌博货架结构和读码不符（等级 %d：cap %u，按 +0x10d 数得 %u），不改货架", L, caps[L], c); GS.on = 0; return; } }
    int lo = gs(player, 12) - 5; if (lo < 5) lo = 5; if (lo > 99) lo = 99;
    uint8_t g101 = *(uint8_t *)(uintptr_t)(G + 0x101), g108 = *(uint8_t *)(uintptr_t)(G + 0x108);
    static uint32_t keep[4096]; uint32_t nk = 0;
    for (uint32_t k = 0; k < n; k++) { uint32_t c = a[k]; if (c >= ni) continue; uint32_t code = *(uint32_t *)(uintptr_t)(it + (uint64_t)c * 0x1c0 + 0x80); int ok = 0;
        for (uint32_t u = 0; u < nu && !ok; u++) { uint64_t r = ut + (uint64_t)u * 0x15c; uint8_t fl = *(uint8_t *)(uintptr_t)(r + 0x2c); int16_t f1 = *(int16_t *)(uintptr_t)(r + 0x8c), f2 = *(int16_t *)(uintptr_t)(r + 0x8e);
            if (*(uint32_t *)(uintptr_t)(r + 0x28) != code || !(fl & 1) || (fl & 4) || *(int16_t *)(uintptr_t)(r + 0x38) > lo) continue;
            if (!g101 && !(f1 == 0 || f2 < 0 || (f1 < 0 && g108))) continue;
            ok = 1; }
        if (ok) keep[nk++] = c; }
    if (!nk) { LOG("【红】秘境：赌博货架筛完一件不剩，不改"); return; }
    GS.arr = arr; GS.caps = (uint64_t)(uintptr_t)caps; GS.n = n; free(GS.save_arr); GS.save_arr = malloc(n * 4); memcpy(GS.save_arr, a, n * 4); memcpy(GS.save_caps, caps, sizeof GS.save_caps);
    memcpy(a, keep, nk * 4);
    for (int L = 0; L < 100; L++) { uint32_t c = 0; for (uint32_t k = 0; k < nk; k++) if (*(uint8_t *)(uintptr_t)(it + (uint64_t)keep[k] * 0x1c0 + 0x10d) <= L) c++; caps[L] = c; }
    GS.on = 1; LOG("秘境：赌博货架 %u → %u 件（只留人物等级 − 5 = %d 级就有合格暗金的底材）；cap[%d] %u → %u", n, nk, lo, lo + 4 > 99 ? 99 : lo + 4, GS.save_caps[lo + 4 > 99 ? 99 : lo + 4], caps[lo + 4 > 99 ? 99 : lo + 4]);
}
static void gamble_apply(uint64_t G) {
    uint32_t u, st; dm_rift_gamble_vals(R.best, &u, &st); if (!u || *(uint8_t *)(uintptr_t)(G + 0x104) != 2) { gamble_restore(); return; }
    uint64_t T = tables(G), base = T ? rd(T + 0x1518) : 0; uint64_t n = T ? *(uint32_t *)(uintptr_t)(T + 0x1520) : 0; if (!base || n < 3) return;
    uint64_t row = base + 2 * 0x84; if (!GB.on) { GB.row = row; GB.uniq = *(uint32_t *)(uintptr_t)(row + 0x74); GB.set = *(uint32_t *)(uintptr_t)(row + 0x70); GB.on = 1; }
    *(uint32_t *)(uintptr_t)(row + 0x74) = u; *(uint32_t *)(uintptr_t)(row + 0x70) = st;
    LOG("秘境：赌博加成 最高第 %d 层 → 暗金 %u、套装 %u（十万分之一；原值 %u / %u）", R.best, u, st, GB.uniq, GB.set);
}

static _Atomic int g_run_seq;
static _Atomic int g_cli_lv = -1;






static int rnd(int n) { return n > 0 ? (int)arc4random_uniform((uint32_t)n) : 0; }
static int in_rift_map(int lv) { for (int i = 0; i < K.nmaps; i++) if (K.maps[i] == lv) return 1; return 0; }
int dm_rift_active(void) { return atomic_load(&K.st) == ST_RUN; }











static int next_tier(void) { int nt = K.inc ? K.tier + K.inc : K.tier; return nt > 100 ? 100 : nt; }




static int rift_overtime(void) { return atomic_load(&K.st) == ST_RUN && K.t0_ns && guest_ns() - K.t0_ns >= (uint64_t)RIFT_SECS * 1000000000ull; }
static int g_left_done;
int dm_rift_hud(dm_rift_hud_t *o) {
    int st = atomic_load(&K.st); if (st != ST_RUN && st != ST_DONE) return 0;
    uint64_t now = guest_ns();
    int el = K.t0_ns ? (int)((now - K.t0_ns) / 1000000000ull) : 0; int need = atomic_load(&K.need);
    o->tier = K.tier; o->progress = need > 0 ? (float)atomic_load(&K.score) / need : 0; if (o->progress > 1) o->progress = 1;
    o->secs_left = st == ST_DONE ? g_left_done : RIFT_SECS - el; o->done = st == ST_DONE; o->guardian = K.guard != 0 && !atomic_load(&K.guard_dead); o->inc = K.inc; o->next = next_tier(); o->overtime = rift_overtime();


    o->gdead = atomic_load(&K.gdead); o->gtotal = atomic_load(&K.gtotal); o->wave = K.wave + 1 - (K.wave_t != 0); o->nwave = K.nwave;
    o->goal = o->overtime ? (o->guardian ? (o->gtotal >= 2 ? 5 : 4) : 3) : o->guardian ? (o->gtotal >= 2 ? 2 : 1) : 0;
    return 1;
}



enum { NQ = 256 };
static _Atomic uint32_t g_newq[NQ]; static _Atomic uint32_t g_newq_w;
static uint32_t g_swept[1024]; int dm_rift_swept(uint32_t id) { return id && g_swept[id & 1023] == id; }


static int hi_on(const char *seg, int from) { static int off = -1; if (off < 0) { const char *e = getenv("DM_RIFT_HI"); off = e && !strcmp(e, "0"); } if (off) return 0;
    const char *f = getenv("DM_RIFT_TEST_HI"); return (f && strstr(f, seg)) || K.tier >= from; }




static int hi_duo_x1000(int which) { int t = K.tier < 75 ? 75 : K.tier > 100 ? 100 : K.tier; return which ? 750 + 250 * (t - 75) / 25 : 510 + 265 * (t - 75) / 25; }




static const int k_duo[] = { 211, 242, 243, 544 };
static int g_entry_lv, g_entry_x, g_entry_y;




















#define SU_NEAR 12
#define SU_HP_X 3
#define SU_MINION_X1000 800
static uint32_t g_su_min[64]; static unsigned g_su_min_w;
static int su_minion(uint32_t id) { for (int i = 0; i < 64; i++) if (id && g_su_min[i] == id) return 1; return 0; }
#define SU_HP_SOFT 225000
static int su_nat_pct(int tier) { return tier >= 90 ? 0 : tier >= 85 ? 30 : tier >= 80 ? 55 : tier >= 75 ? 80 : 100; }
static int su_quota(int tier) { return tier >= 90 ? 1 << 30 : tier >= 85 ? 7 : tier >= 80 ? 5 : tier >= 75 ? 2 : 0; }
static int su_cap(int tier) { return tier >= 90 ? 3 : 2; }






static uint64_t su_spawn(uint64_t G, uint64_t room, int x, int y, int row) {
    uint8_t *bm = (uint8_t *)(uintptr_t)(G + 0x5668 + (row >> 3)), bit = (uint8_t)(1u << (row & 7)), old = *bm & bit; *bm &= (uint8_t)~bit;
    uint64_t u = ((uint64_t (*)(uint64_t, uint64_t, int, int, int, int))(B + GA_SUPERUNIQUE_SPAWN))(G, room, x, y, row, 0);
    uint8_t mid = *bm & bit; *bm = (uint8_t)((*bm & ~bit) | old); static int n; if (n++ < 3) LOG("秘境：刷超级金怪 行 %d：本局已出现位 刷前 %d、刷完原版置成 %d、写回 %d", row, old != 0, mid != 0, (*bm & bit) != 0); return u; }
static int g_near_now;
static const uint8_t k_su_pool[] = { 0, 1, 2, 3, 4, 6, 7, 8, 9, 11, 12, 13, 14, 16, 17, 19, 21, 22, 23, 24, 25, 32, 33, 34, 35, 36, 37, 38, 40, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59 };
static uint32_t g_inv_gone[2]; static int g_inv_gone_xy[2][2];
static uint32_t g_suseq_id;






static int boss_summon(uint32_t cls) { return (cls >= 562 && cls <= 566) || cls == 570; }
static int is_guard(uint32_t id) { return id && (id == K.guards[0] || id == K.guards[1] || id == K.guards[2] || id == K.inv_id[0] || id == K.inv_id[1] || id == g_inv_gone[0] || id == g_inv_gone[1]); }
static int pts_of(uint8_t fl) { return fl & 0x02 ? 15 : fl & 0x08 ? 10 : fl & 0x04 ? 5 : fl & 0x10 ? 2 : 1; }










static struct { int seq; uint32_t id[4096]; int hp[4096]; _Atomic long long kill_hp; _Atomic int kills; long long spawn_hp, guard_hp, dmg, win[25], win_sum, dmg_peak, near_sum; int spawns, wi, last_hp, deaths0, near_peak, near_n; } HS;
static int unit_align(uint64_t u);
static void hs_sync(void) { int sq = atomic_load(&g_run_seq); if (sq == HS.seq) return; memset(&HS, 0, sizeof HS); HS.seq = sq; HS.deaths0 = R.deaths; }
static void hs_spawn(uint32_t id, int hp) { hs_sync(); HS.id[id & 4095] = id; HS.hp[id & 4095] = hp; HS.spawns++; HS.spawn_hp += hp; }
static void hs_kill(uint32_t id) { if (HS.id[id & 4095] != id) return; atomic_fetch_add(&HS.kill_hp, HS.hp[id & 4095]); atomic_fetch_add(&HS.kills, 1); }
static void hs_tick(uint64_t G, uint64_t player, uint32_t t, int lv) {
    hs_sync(); int hp = gs(player, 6) >> 8, d = HS.last_hp > 0 && hp < HS.last_hp ? HS.last_hp - hp : 0; HS.last_hp = hp;
    HS.dmg += d; HS.win_sum += d - HS.win[HS.wi]; HS.win[HS.wi] = d; HS.wi = (HS.wi + 1) % 25; if (HS.win_sum > HS.dmg_peak) HS.dmg_peak = HS.win_sum;


    { static int on = -1; if (on < 0) on = getenv("DM_RIFT_TEST_HSLOG") != NULL; if (on && t % 750 == 0 && atomic_load(&K.st) == ST_RUN) { int sc = atomic_load(&K.score);
        LOG("秘境：读数（进行中）第 %d 层 拍 %u：分数 %d / %d，击杀 %d 只、生命和 %lld 点（每分 %.0f 点）；改过属性的敌对怪 %d 只、生命和 %lld 点；40 格内敌对单位 峰值 %d / 均值 %.1f", K.tier, t, sc, atomic_load(&K.need), atomic_load(&HS.kills), atomic_load(&HS.kill_hp), sc ? (double)atomic_load(&HS.kill_hp) / sc : 0.0, HS.spawns, HS.spawn_hp, HS.near_peak, HS.near_n ? (double)HS.near_sum / HS.near_n : 0.0); } }
    if (t % 25 || !in_rift_map(lv)) return;
    int px, py, n = 0; if (!unit_xy(player, &px, &py)) return;
    for (int b = 0; b < 128; b++) for (uint64_t u = rd(G + 0x2230 + 1 * 0x400 + 8 * b); u; u = rd(u + 0x158)) { uint32_t m = *(uint32_t *)(uintptr_t)(u + 0xc); int x, y;
        if (m == 0 || m == 12 || unit_level(u) != lv || !unit_xy(u, &x, &y) || abs(x - px) > 40 || abs(y - py) > 40 || unit_align(u)) continue; n++; }
    HS.near_sum += n; HS.near_n++; if (n > HS.near_peak) HS.near_peak = n; g_near_now = n;
}
static void hs_report(int secs) {
    LOG("秘境：读数 第 %d 层 用时 %d 秒：击杀 %d 只、生命和 %lld 点（另守护者 %lld 点）；改过属性的敌对怪 %d 只、生命和 %lld 点；承伤和 %lld 点、每秒峰值 %lld 点、死亡 %d 次；40 格内敌对单位 峰值 %d / 均值 %.1f（采样 %d 次）",
        K.tier, secs, atomic_load(&HS.kills), atomic_load(&HS.kill_hp), HS.guard_hp, HS.spawns, HS.spawn_hp, HS.dmg, HS.dmg_peak, R.deaths - HS.deaths0, HS.near_peak, HS.near_n ? (double)HS.near_sum / HS.near_n : 0.0, HS.near_n);
}
void dm_rift_client_pump(uint8_t *b) {
    static uint32_t seen[4096], dead[4096]; static int lvp = -1;







    { static int seq0 = -1; int sq = atomic_load(&g_run_seq); if (sq != seq0) { seq0 = sq; memset(seen, 0, sizeof seen); memset(dead, 0, sizeof dead); } }
    if (!b || atomic_load(&K.st) == ST_IDLE) return;
    uint64_t pl = ((uint64_t (*)(uint32_t))(b + GA_CLIENT_PLAYER_GET))(*(uint32_t *)(b + GA_LOCAL_PLAYER_SLOT)); if (!pl) { atomic_store(&g_cli_lv, -1); return; }




    { static int at = -2, n; if (at == -2) { const char *e = getenv("DM_RIFT_TEST_NOROOM"); at = e ? atoi(e) : -1; }
      uint64_t pth = rd(pl + 0x38); if (at > 0 && pth && atomic_load(&K.st) == ST_RUN && ++n == at) { uint64_t keep = rd(pth + 0x20); *(volatile uint64_t *)(uintptr_t)(pth + 0x20) = 0;
          LOG("秘境：测试 DM_RIFT_TEST_NOROOM 第 %d 次，客户端玩家房间指针临时清空后读关卡", n); int v = unit_level(pl); *(volatile uint64_t *)(uintptr_t)(pth + 0x20) = keep;
          LOG("秘境：测试 DM_RIFT_TEST_NOROOM 读到关卡 %d，没崩；房间指针已写回（现在读到 %d）", v, unit_level(pl)); } }
    int lv = unit_level(pl);
    atomic_store(&g_cli_lv, lv);
    if (atomic_load(&K.st) != ST_RUN) return;
    if (lv != lvp) { lvp = lv; LOG("秘境：客户端看到关卡 %d（%s秘境图）", lv, in_rift_map(lv) ? "是" : "不是"); }
    if (!in_rift_map(lv)) return;
    for (int k = 0; k < 128; k++) for (uint64_t u = rd((uint64_t)(uintptr_t)(b + GA_CLIENT_UNIT_TABLE + 1 * 0x400 + k * 8)); u; u = rd(u + 0x158)) {
        uint32_t id = *(uint32_t *)(uintptr_t)(u + 8), m = *(uint32_t *)(uintptr_t)(u + 0xc), h = id & 4095;
        if (is_guard(id)) continue;
        int alive = m != 0 && m != 12;
        if (alive) { if (seen[h] == id) continue;
            if (((int (*)(int, uint64_t, uint64_t))(b + GA_UNIT_INTERACT_CLASS))(0x165, pl, u) != 1) continue;
            seen[h] = id; uint32_t w = atomic_fetch_add(&g_newq_w, 1); atomic_store(&g_newq[w % NQ], id); continue; }
        if (seen[h] != id || dead[h] == id) continue;
        dead[h] = id; hs_kill(id); uint64_t md = rd(u + 0x10); uint8_t fl = md ? *(uint8_t *)(uintptr_t)(md + 0x1a) : 0; int p = pts_of(fl);
        int s = atomic_fetch_add(&K.score, p) + p; static int ln; if (ln++ < 40 || s % 50 < p) LOG("秘境：击杀 id %u（标志 %#x，+%d）→ 分数 %d / %d", id, fl, p, s, atomic_load(&K.need));
    }
}































static int grow_cfg(int which) { static int v[4] = { -1, -1, -1, -1 }; static const char *const nm[4] = { "DM_RIFT_GROW_HP100", "DM_RIFT_GROW_DM100", "DM_RIFT_GROW_FROM", "DM_RIFT_GROW_DM_FROM" }; static const int def[4] = { 487, 388, 9, 40 };
    if (v[which] < 0) { const char *e = getenv(nm[which]); v[which] = e && atoi(e) > 0 ? atoi(e) : def[which]; if (which < 2 && v[which] < 100) v[which] = 100; if (which == 2 && (v[which] < 9 || v[which] > 99)) v[which] = 9; if (which == 3 && v[which] < 9) v[which] = 9; } return v[which]; }




static int lvl_cap(void) { static int v = -1; if (v < 0) { const char *e = getenv("DM_RIFT_LVL_CAP"); int c = e ? atoi(e) : 109; v = c < 96 ? 96 : c > 110 ? 110 : c; } return v; }
int dm_rift_tier_level(int tier, int clvl) { if (tier < 9) return 0; if (tier <= 25) { int t = clvl + 2 + (tier - 9); return t > 96 ? 96 : t; } int t = 96 + (tier - 25), c = lvl_cap(); return t > c ? c : t; }


static int tier_total_x1000(int tier, int which) { int f = grow_cfg(2), top = grow_cfg(which); if (tier <= f) return 1000; if (tier > 100) tier = 100; return 1000 + (int)((long long)(top - 100) * 10 * (tier - f) / (100 - f)); }
static int tier_elite_x2(int tier) { return tier <= 40 ? 4 : tier <= 50 ? 5 : tier <= 60 ? 6 : tier <= 80 ? 7 : 8; }
static int target_level(uint64_t player) { return dm_rift_tier_level(K.tier, gs(player, 12)); }
static _Atomic uint32_t g_mlhp[112];






int dm_rift_tier_hp_x10(int tier, int clvl) { int p = tier < 8 ? tier : 8, l9 = dm_rift_tier_level(9, clvl); long long v = (long long)(p + 1) * 5; if (tier < 9) return (int)v;
    uint32_t h85 = atomic_load(&g_mlhp[85]), hl = l9 > 0 && l9 < 112 ? atomic_load(&g_mlhp[l9]) : 0; if (h85 && hl && hl > h85) v = v * hl / h85; return (int)(v * tier_total_x1000(tier, 0) / 1000); }
static uint32_t monlvl(uint64_t G, int lvl, int grp) { uint64_t T = tables(G), a = rd(T + 0x1178), n = rd(T + 0x1180); if (!a || lvl < 0 || (uint64_t)lvl >= n) return 0; return *(uint32_t *)(uintptr_t)(a + (uint64_t)lvl * 0x78 + (grp * 6 + 5) * 4); }






static long long hp_base_limit(int base, int full) { return base > 0 && full > base ? 0x7fffff00LL * base / full : 0x7fffff00LL; }
static int g_hp_soft;
static int g_hp_mul = 1000;
static void scale_monster(uint64_t G, uint64_t u, int lt, int l9) {
    int l0 = gs(u, 12); if (l0 <= 0) return; if (lt > lvl_cap()) lt = lvl_cap(); if (l9 > lt) l9 = lt;
    int tx = tier_total_x1000(K.tier, 0), lb = l0 > l9 ? l0 : l9, le = l0 > lt ? l0 : lt;
    if (le == l0 && lb == l0 && tx == 1000) return;
    uint32_t h0 = monlvl(G, l0, 2), hb = monlvl(G, lb, 2), d0 = monlvl(G, l0, 3), db = monlvl(G, lb, 3), a0 = monlvl(G, l0, 0), a1 = monlvl(G, le, 0); if (!h0 || !d0 || !a0) return;
    long long want = (long long)gs(u, 7) * hb / h0 * tx / 1000 * g_hp_mul / 1000; if (g_hp_soft > 0 && g_hp_mul > 1000) { long long w1 = want * 1000 / g_hp_mul, sf = (long long)g_hp_soft * 256 * tx / 1000; if (want > sf) want = w1 > sf ? w1 : sf; }
    long long hp = want, lim = hp_base_limit(gsb(u, 7), gs(u, 7)); if (hp > lim) hp = lim;



    if (hp != want) { static int n, sq = -1; int cur = atomic_load(&g_run_seq); if (cur != sq) { sq = cur; n = 0; } if (n++ < 3) LOG("秘境：第 %d 层 怪 id %u（类号 %u）生命按层应为 %lld 点，顶到原版上限 %lld 点（实得 ×%.2f，应得 ×%.2f）；伤害照加", K.tier, *(uint32_t *)(uintptr_t)(u + 8), *(uint32_t *)(uintptr_t)(u + 4), want >> 8, hp >> 8, (double)hp / gs(u, 7), (double)want / gs(u, 7)); }
    int dpct;
    { int f = grow_cfg(3), tt = K.tier > 100 ? 100 : K.tier, full = (int)(((long long)db * tier_total_x1000(100, 1) / d0 - 1000 + 5) / 10);
      dpct = tt <= f || f >= 100 ? 0 : (int)((long long)full * (tt - f) / (100 - f)); }



    { static int n, sq = -1; int cur = atomic_load(&g_run_seq); if (cur != sq) { sq = cur; n = 0; } if (n++ < 6) LOG("秘境：按层改怪 id %u 等级 %d → %d：生命 %d → %lld、伤害 +%d%%、防御 %d → %lld（第 %d 层生命总倍率 ×%d.%02d，起算层等级 %d；伤害另随等级走原版查表）", *(uint32_t *)(uintptr_t)(u + 8), l0, le, gs(u, 7) >> 8, hp >> 8, dpct, gs(u, 31), (long long)gs(u, 31) * a1 / a0, K.tier, tx / 1000, tx % 1000 / 10, lb); }
    ss(u, 7, (int)hp); ss(u, 6, (int)hp); ss(u, 25, gs(u, 25) + dpct); ss(u, 31, (int)((long long)gs(u, 31) * a1 / a0)); ss(u, 12, le);
}





static struct { uint32_t id, t; } g_dp[12]; static int g_dpn, g_dpseq = -1;
static void dmg_probe_add(uint32_t id, uint32_t t) { static int on = -1; if (on < 0) on = getenv("DM_RIFT_TEST_DMGLOG") != NULL; if (!on) return;
    int cur = atomic_load(&g_run_seq); if (cur != g_dpseq) { g_dpseq = cur; g_dpn = 0; } if (g_dpn < 12) { g_dp[g_dpn].id = id; g_dp[g_dpn].t = t; g_dpn++; } }
static void dmg_probe_pump(uint64_t G, uint32_t t) {
    for (int i = 0; i < g_dpn; i++) { if (!g_dp[i].id || t - g_dp[i].t < 50) continue; uint64_t u = find_unit(G, 1, g_dp[i].id); uint32_t id = g_dp[i].id; g_dp[i].id = 0; if (!u) continue;
        LOG("秘境：伤害读数 第 %d 层 怪 id %u 类号 %u 等级 %d：最小 %d 最大 %d 命中值 %d 伤害百分比 %d（人数档属性 100 = %d）", K.tier, id, *(uint32_t *)(uintptr_t)(u + 4), gs(u, 12), gs(u, 21), gs(u, 22), gs(u, 19), gs(u, 25), gs(u, 100)); }
}





















static int pet_cfg(int which) { static int v[2] = { -1, -1 }; static const char *const nm[2] = { "DM_RIFT_PET_HP100", "DM_RIFT_PET_FROM" };
    if (v[which] < 0) { const char *e = getenv(nm[which]); int x = e ? atoi(e) : 0;
        v[which] = which == 0 ? (!e ? grow_cfg(0) : x >= 100 ? x : 0) : (e && x >= 1 && x <= 99 ? x : grow_cfg(2)); } return v[which]; }
static struct { uint32_t cls; int base, boosted; uint64_t gone; } g_pf[16]; static int g_pfn; static char g_pf_who[sizeof K.who];
uint64_t dm_fs_commit_count(void);
static int pet_path(char *o, size_t n) { const char *w = dm_save_workdir(); if (!w || !K.who[0]) return 0; snprintf(o, n, "%s/%s/%s.dm_pets", w, K.who, K.who); return 1; }
static void pet_file_load(void) { g_pfn = 0; strlcpy(g_pf_who, K.who, sizeof g_pf_who); char p[1200], ln[96]; if (!pet_path(p, sizeof p)) return; FILE *f = fopen(p, "r"); if (!f) return;
    while (g_pfn < 16 && fgets(ln, sizeof ln, f)) { unsigned c; int b_, x; if (sscanf(ln, "%u %d %d", &c, &b_, &x) == 3 && b_ > 0 && x > 0) { g_pf[g_pfn].cls = c; g_pf[g_pfn].base = b_; g_pf[g_pfn].boosted = x; g_pf[g_pfn].gone = 0; g_pfn++; } }
    fclose(f); if (g_pfn) LOG("秘境：随从基数侧文件读到 %d 条（%s）", g_pfn, p); }
static void pet_file_save(void) { char p[1200], q[1220]; if (!pet_path(p, sizeof p)) return;
    if (!g_pfn) { remove(p); return; }
    snprintf(q, sizeof q, "%s.tmp", p); FILE *f = fopen(q, "w"); if (!f) { LOG("【红】秘境：随从基数侧文件写不进 %s", q); return; }
    for (int i = 0; i < g_pfn; i++) fprintf(f, "%u %d %d\n", g_pf[i].cls, g_pf[i].base, g_pf[i].boosted);
    if (fclose(f) || rename(q, p)) LOG("【红】秘境：随从基数侧文件改名失败 %s", p); }






static int bound_cap(uint64_t u, uint64_t player) {
    uint8_t ver = *(uint8_t *)(uintptr_t)(u + 0x1bd); if (ver < 1 || ver > 3) return 0;
    uint64_t row = ((uint64_t (*)(uint64_t))(B + GA_MONPET_ROW_GET))(u); if (!row || *(uint16_t *)(uintptr_t)(row + 0x50) != 7) return 0;
    int v = ((int (*)(int, uint64_t, uint64_t, uint32_t))(B + GA_MON_CALC_EVAL))(ver, u, player, *(uint32_t *)(uintptr_t)(row + 0x5c));
    return v > 0 && v <= 0x7fffff ? v << 8 : 0;
}
static void pet_hp_pump(uint64_t G, uint64_t player, uint32_t t) {
    static int dump = -1; if (dump < 0) dump = getenv("DM_RIFT_TEST_PETDUMP") != NULL;
    if (!K.who[0]) return;
    int top = pet_cfg(0), f = pet_cfg(1), tier = K.tier > 100 ? 100 : K.tier, on = top && atomic_load(&K.st) == ST_RUN && tier > f;


    static int was_on = -1; if (t % 10 && on == was_on) return; was_on = on;
    if (strcmp(g_pf_who, K.who)) pet_file_load();
    if (!top && !dump && !g_pfn) return;
    static struct { uint32_t id, cls; int base, last; } tb[64]; static unsigned tw;
    int mult = on ? 1000 + (int)((long long)(top - 100) * 10 * (tier - f) / (100 - f)) : 1000, act = top || g_pfn;
    uint64_t pd = rd(player + 0x10), lst = pd ? rd(pd + 0x98) : 0, arr = lst ? rd(lst) : 0, T = tables(G); uint32_t nt = T ? *(uint32_t *)(uintptr_t)(T + 0x1300) : 0; if (!arr || nt > 64) return;
    char line[700]; int lo = 0; line[0] = 0;
    struct { uint32_t cls; int base, boosted; } now[16]; int nn = 0, pf_dirty = 0;
    for (uint32_t ty = 1; ty < nt; ty++) { if (ty == 7) continue;
        int guard = 0; for (uint64_t nd = rd(arr + (uint64_t)ty * 0x20); nd && guard < 64; nd = rd(nd + 0x18), guard++) {
            uint32_t id = *(uint32_t *)(uintptr_t)(nd + 4); uint64_t u = find_unit(G, 1, id); if (!u) continue; uint32_t md = *(uint32_t *)(uintptr_t)(u + 0xc); if (md == 0 || md == 12) continue;




            int cur = gsb(u, 7), full = gs(u, 7); uint32_t cls = *(uint32_t *)(uintptr_t)(u + 4); if (cur <= 0 || full <= 0) continue;


            { static int pct = -1; static uint32_t done[16]; static unsigned dn; if (pct < 0) { const char *e = getenv("DM_RIFT_TEST_PETPCT"); pct = e ? atoi(e) : 0; }
              if (pct > 0) { int seen = 0; for (int i = 0; i < 16; i++) seen |= done[i] == id; if (!seen) { done[dn++ % 16] = id; int b0 = gsb(u, 7), f0 = gs(u, 7); ss(u, 76, pct);
                  LOG("秘境：测试 DM_RIFT_TEST_PETPCT 给随从 id %u（类号 %u）加生命上限 +%d%%：基础 %d、合计 %d → 基础 %d、合计 %d", id, cls, pct, b0 >> 8, f0 >> 8, gsb(u, 7) >> 8, gs(u, 7) >> 8); full = gs(u, 7); } } }
            if (dump && t % 125 == 0 && lo < (int)sizeof line - 110) lo += snprintf(line + lo, sizeof line - lo, " [类型 %u 类号 %u id %u 等级 %d 生命上限 %d（基础 %d）当前 %d]", ty, cls, id, gs(u, 12), full >> 8, cur >> 8, gs(u, 6) >> 8);
            if (!act) continue;
            int k = -1, fresh = 1; for (int i = 0; i < 64; i++) if (tb[i].id == id && tb[i].cls == cls) { k = i; break; }
            if (k < 0) { k = (int)(tw++ % 64); tb[k].id = id; tb[k].cls = cls; tb[k].base = cur; tb[k].last = cur;
                for (int i = 0; i < g_pfn; i++) if (g_pf[i].cls == cls && g_pf[i].boosted == cur) { tb[k].base = g_pf[i].base; LOG("秘境：随从 id %u（类号 %u）读档带着放大后的生命上限 %d，基数按侧文件取 %d", id, cls, cur >> 8, tb[k].base >> 8); break; } }
            else if (cur != tb[k].last) { tb[k].base = cur; tb[k].last = cur; }
            else fresh = 0;




            if (fresh && ty == 21) { int cap = bound_cap(u, player); if (cap > 0 && tb[k].base > cap) {
                LOG("秘境：%s 的束缚恶魔（类号 %u）生命基础 %d 超过上限，已设回 %d", K.who, cls, tb[k].base >> 8, cap >> 8); tb[k].base = cap;
                for (int i = 0; i < g_pfn; ) { if (g_pf[i].cls == cls && g_pf[i].base > cap) { g_pf[i] = g_pf[--g_pfn]; pf_dirty = 1; } else i++; } } }


            long long want = (long long)tb[k].base * mult / 1000, lim = hp_base_limit(cur, full); if (want > lim) want = lim;
            if ((int)want != cur) { int hp = gs(u, 6); long long nf = (long long)full * want / cur, nh = (long long)hp * want / cur; if (nh < 256) nh = 256; if (nh > nf) nh = nf;
                ss(u, 7, (int)want); ss(u, 6, (int)nh);
                LOG("秘境：随从 id %u（宠物类型 %u，类号 %u）生命上限基础项 %d → %lld（基数 %d，第 %d 层 ×%d.%02d）；合计 %d → %d", id, ty, cls, cur >> 8, want >> 8, tb[k].base >> 8, K.tier, mult / 1000, mult % 1000 / 10, full >> 8, gs(u, 7) >> 8); }
            tb[k].last = (int)want;
            if (tb[k].last != tb[k].base && nn < 16) { now[nn].cls = cls; now[nn].base = tb[k].base; now[nn].boosted = tb[k].last; nn++; } } }




    if (act) { uint64_t cc = dm_fs_commit_count(); struct { uint32_t cls; int base, boosted; uint64_t gone; } nl[16]; int m = 0;
        for (int i = 0; i < nn; i++) { nl[m].cls = now[i].cls; nl[m].base = now[i].base; nl[m].boosted = now[i].boosted; nl[m].gone = 0; m++; }
        for (int i = 0; i < g_pfn && m < 16; i++) { int live = 0; for (int j = 0; j < nn; j++) live |= now[j].cls == g_pf[i].cls && now[j].base == g_pf[i].base && now[j].boosted == g_pf[i].boosted; if (live) continue;
            uint64_t gone = g_pf[i].gone ? g_pf[i].gone : cc + 1; if (cc >= gone + 1) continue;
            nl[m].cls = g_pf[i].cls; nl[m].base = g_pf[i].base; nl[m].boosted = g_pf[i].boosted; nl[m].gone = gone; m++; }
        int same = m == g_pfn; for (int i = 0; same && i < m; i++) same = nl[i].cls == g_pf[i].cls && nl[i].base == g_pf[i].base && nl[i].boosted == g_pf[i].boosted && !nl[i].gone == !g_pf[i].gone;
        if (!same || pf_dirty) { g_pfn = m; for (int i = 0; i < m; i++) { g_pf[i].cls = nl[i].cls; g_pf[i].base = nl[i].base; g_pf[i].boosted = nl[i].boosted; g_pf[i].gone = nl[i].gone; } pet_file_save(); } }
    if (dump && t % 125 == 0) LOG("秘境：随从读数 拍 %u（%s，第 %d 层，倍率 ×%d.%02d，侧文件 %d 条）：%s", t, atomic_load(&K.st) == ST_RUN ? "秘境进行中" : "不在秘境", K.tier, mult / 1000, mult % 1000 / 10, g_pfn, lo ? line : " 没有随从");
}





static uint64_t quest_rec(uint64_t G, int type) { int n = 0; for (uint64_t q = rd(rd(G + 0x21d8)); q && n < 64; q = rd(q + 0x310), n++) { if (*(int32_t *)(uintptr_t)q == type) return q; } return 0; }




static uint32_t place_door(uint64_t G, uint64_t room, int cx, int cy, const char *what, int cls, int exact) {
    int32_t pos[2] = { cx, cy }; uint64_t rr = !room ? 0 : exact ? room : free_spot(G, room, pos); if (!rr) return 0;
    uint64_t q = quest_rec(G, 0x14); uint8_t o21 = q ? *(uint8_t *)(uintptr_t)(q + 0x21) : 0; uint32_t o24 = q ? *(uint32_t *)(uintptr_t)(q + 0x24) : 0;
    uint64_t u = ((uint64_t (*)(uint64_t, uint64_t, int, int, int, int, int, int, uint64_t))(B + GA_UNIT_SPAWN))(G, rr, 2, cls, pos[0], pos[1], 0, 0, 0);
    if (q) { int changed = *(uint8_t *)(uintptr_t)(q + 0x21) != o21 || *(uint32_t *)(uintptr_t)(q + 0x24) != o24; *(uint8_t *)(uintptr_t)(q + 0x21) = o21; *(uint32_t *)(uintptr_t)(q + 0x24) = o24;
        LOG("秘境：建门%s第三幕任务记录 +0x21 / +0x24（已写回 %u / %u）", changed ? "改了" : "没改", o21, o24); }
    if (!u) return 0;
    ((void (*)(uint64_t, int))(B + GA_UNIT_SET_MODE))(u, 2);
    LOG("秘境：%s开门（物件 %d id %u 在 %d,%d，模式 %u）", what, cls, *(uint32_t *)(uintptr_t)(u + 8), pos[0], pos[1], *(uint32_t *)(uintptr_t)(u + 0xc));
    return *(uint32_t *)(uintptr_t)(u + 8);
}










enum { DOOR_NPC_GAP = 10, DOOR_OBJ_GAP = 6 }; enum { DOOR_HOME_GAP = 14 }; enum { ND_GAP = 8 };
static int any_level(uint64_t u) { uint64_t p = rd(u + 0x38), room = p ? unit_room(u) : 0, ex = room ? rd(room + 0x18) : 0, lv = ex ? rd(ex + 0x90) : 0; return lv ? *(int32_t *)(uintptr_t)(lv + 0x1f8) : 0; }
static int door_gap(uint64_t G, int type, int lvno, int x, int y) { int best = 1 << 30;
    for (int b = 0; b < 128; b++) for (uint64_t u = rd(G + 0x2230 + type * 0x400 + 8 * b); u; u = rd(u + 0x158)) { int ux, uy; if (any_level(u) != lvno || !unit_xy(u, &ux, &uy)) continue;
        int d = abs(ux - x) > abs(uy - y) ? abs(ux - x) : abs(uy - y); if (d < best) best = d; }
    return best; }
static int preset_gap(uint64_t G, uint64_t lvt, int mon, int x, int y, int *cls) { int best = 1 << 30, n = 0;
    for (uint64_t r = dm_level_rooms(lvt); r && n < 2000; r = rd(r + 0x48), n++) { int rx = *(int32_t *)(uintptr_t)(r + 0x60) * 5, ry = *(int32_t *)(uintptr_t)(r + 0x64) * 5, w = *(int32_t *)(uintptr_t)(r + 0x68) * 5, h = *(int32_t *)(uintptr_t)(r + 0x6c) * 5;
        if (x < rx - DOOR_HOME_GAP || x > rx + w + DOOR_HOME_GAP || y < ry - DOOR_HOME_GAP || y > ry + h + DOOR_HOME_GAP) continue;
        if (!rd(r + 0x58)) ((void (*)(int, uint64_t))(B + GA_ROOM_INIT))(*(uint8_t *)(uintptr_t)(G + 0x106), r);
        for (uint64_t u = rd(r + 0x98), k = 0; u && k < 256; u = rd(u + 0x10), k++) { if (*(int32_t *)(uintptr_t)(u + 0x20) != (mon ? 1 : 2)) continue;
            int ux = rx + *(int32_t *)(uintptr_t)(u + 8), uy = ry + *(int32_t *)(uintptr_t)(u + 0x24), d = abs(ux - x) > abs(uy - y) ? abs(ux - x) : abs(uy - y); if (d < best) { best = d; if (cls) *cls = *(int32_t *)(uintptr_t)(u + 4); } } }
    return best; }




static int door_line_open(uint64_t lvt, int ax, int ay, int x, int y) { int n = abs(x - ax) > abs(y - ay) ? abs(x - ax) : abs(y - ay); uint64_t room = room_at(lvt, ax, ay); if (!room) return 0;
    for (int i = 1; i <= n; i++) { int cx = ax + (x - ax) * i / n, cy = ay + (y - ay) * i / n; uint64_t r2 = room_at(lvt, cx, cy); if (!r2) return 0;
        if (((uint32_t (*)(uint64_t, int, int, int))(B + GA_COLL_QUERY))(r2, cx, cy, 1) & 0xffff) return 0; }
    return 1; }






static uint32_t ds_fill(uint64_t G, uint64_t player, int x, int y, struct ds_grid *dg) {
    uint64_t lvp = level_of(player), pp = rd(player + 0x38); uint32_t mk = pp ? *(uint32_t *)(uintptr_t)(pp + 0x64) : 0; if (!(mk & 1) || (mk & ~0xffffu)) mk = 0x1c09;
    if (G) mk &= ~0x1000u;
    for (int iy = 0; iy < DS_W; iy++) for (int ix = 0; ix < DS_W; ix++) { int cx = x + ix - DS_R, cy = y + iy - DS_R; uint64_t r = G ? room_load(G, lvp, cx, cy) : room_at(lvp, cx, cy);
        uint32_t v = r ? ((uint32_t (*)(uint64_t, int, int, int))(B + GA_COLL_QUERY))(r, cx, cy, (int)mk) & 0xffff : 1; if (ix == DS_R && iy == DS_R) v &= ~0x1000u; dg->blk[iy * DS_W + ix] = v != 0; }
    ds_flood_r(dg, G ? 2 : 0); return mk;
}
static int door_clear(uint64_t lvt, int x, int y) { for (int dy = -5; dy <= 5; dy++) for (int dx = -5; dx <= 5; dx++) { uint64_t r = room_at(lvt, x + dx, y + dy); if (!r || (((uint32_t (*)(uint64_t, int, int, int))(B + GA_COLL_QUERY))(r, x + dx, y + dy, 1) & 0xffff)) return 0; } return 1; }
static uint64_t pick_door_spot(uint64_t G, uint64_t lvt, int lvno, int ax, int ay, int32_t out[2]) {
    long best = -1; uint64_t broom = 0; int bx = 0, by = 0, c[6] = {0};
    for (int dy = -40; dy <= 40; dy += 2) for (int dx = -40; dx <= 40; dx += 2) { long d2 = (long)dx * dx + (long)dy * dy; c[0]++;
        int32_t pos[2] = { ax + dx, ay + dy }; uint64_t room = room_at(lvt, pos[0], pos[1]), rr = room ? free_spot(G, room, pos) : 0; if (!rr) continue; c[1]++;
        if (preset_gap(G, lvt, 1, pos[0], pos[1], NULL) < DOOR_HOME_GAP || preset_gap(G, lvt, 0, pos[0], pos[1], NULL) < DOOR_OBJ_GAP) continue; c[2]++;
        if (door_gap(G, 1, lvno, pos[0], pos[1]) < DOOR_NPC_GAP || door_gap(G, 2, lvno, pos[0], pos[1]) < DOOR_OBJ_GAP) continue; c[3]++;
        if (!door_line_open(lvt, ax, ay, pos[0], pos[1])) continue; c[4]++; if (!door_clear(lvt, pos[0], pos[1])) continue; c[5]++;
        if (best >= 0 && d2 >= best) continue; best = d2; broom = room; bx = pos[0]; by = pos[1]; }
    { static int n; if (n++ < 2) LOG("秘境：门位筛选 候选 %d → 有空格 %d → 离预设点够远 %d → 离在场单位够远 %d → 与锚点直线通 %d → 周围无墙 %d", c[0], c[1], c[2], c[3], c[4], c[5]); }
    if (!broom) return 0;
    { int hc = 0, hn = preset_gap(G, lvt, 1, bx, by, &hc), ho = preset_gap(G, lvt, 0, bx, by, NULL), gn = door_gap(G, 1, lvno, bx, by), go = door_gap(G, 2, lvno, bx, by);
      LOG("秘境：门的落点按规则取 %d,%d（离锚点 %d,%d 横 %d 纵 %d 格；离最近的 NPC 预设点 %d 格（类号 %d）、物件预设点 %d 格；在场 NPC %d 格、物件 %d 格；-1 = 没有）", bx, by, ax, ay, bx - ax, by - ay, hn > 9999 ? -1 : hn, hc, ho > 9999 ? -1 : ho, gn > 9999 ? -1 : gn, go > 9999 ? -1 : go); }
    out[0] = bx; out[1] = by; return broom; }
static uint32_t make_rift_door(uint64_t G, uint64_t player) {
    uint64_t lvt = level_of(player); int x0, y0; level_origin(lvt, &x0, &y0);
    int ax = x0 + 64, ay = y0 + 80; int32_t pos[2];
    if (!room_at(lvt, ax, ay)) { static int n; int px, py; unit_xy(player, &px, &py); if (n++ < 3) LOG("秘境：门的锚点 %d,%d（关卡原点 %d,%d）所在房间还没载入，人在 %d,%d", ax, ay, x0, y0, px, py); return 0; }
    uint64_t room = pick_door_spot(G, lvt, unit_level(player), ax, ay, pos); if (!room) { static int n; if (n++ < 3) LOG("【红】秘境：锚点 %d,%d 周围 40 格内找不到合规的门位", ax, ay); return 0; }
    return place_door(G, room, pos[0], pos[1], "哈洛加斯", RIFT_OBJ, 0);
}






static int next_map_index(void) { return K.cur + 1 < K.nmaps ? K.cur + 1 : K.cur; }
static int rift_exit_active(uint64_t G, uint64_t player) { int st = atomic_load(&K.st); return (st == ST_RUN || st == ST_DONE) && G && G == L.G && player && player == L.player && in_rift_map(unit_level(player)); }


void svc_rift_exit(uint64_t *regs) {
    uint64_t G = regs[0], pl = regs[1], tile = regs[2]; regs[8] = 0; if (!rift_exit_active(G, pl)) return;
    LOG("秘境：转场 run %llu 状态 %d 层 %d 来源 tile", (unsigned long long)K.t0_ns, atomic_load(&K.st), K.tier);
    int nx = next_map_index(); LOG("秘境：在关卡 %d 走出口砖（类号 %u）→ 入口处改道进第 %d 张图 %d", unit_level(pl), tile ? *(uint32_t *)(uintptr_t)(tile + 4) : 0, nx + 1, K.maps[nx]);
    K.cur = nx; L.lv_prev = K.maps[nx]; regs[2] = (uint64_t)(uint32_t)K.maps[nx]; regs[3] = 11; regs[8] = 1;
}


















static void rift_level_obs(const uint64_t *regs) {
    uint64_t G = regs[0], pl = regs[1]; int lv = (int)(uint32_t)regs[2];


    if (G && pl) LOG("换图请求诊断：拍 %u 玩家 %u 当前 %d 目标 %d 方式 %u 秘境状态 %d", *(uint32_t *)(uintptr_t)(G + 0x170), *(uint32_t *)(uintptr_t)(pl + 8), unit_level(pl), lv, (uint32_t)regs[3], atomic_load(&K.st));
    if (!rift_exit_active(G, pl)) return;
    uint32_t pm = *(uint32_t *)(uintptr_t)(pl + 0xc);


    LOG("秘境：转场 run %llu 状态 %d 层 %d 来源 level 目标 %d 方式 %u 玩家模式 %u", (unsigned long long)K.t0_ns, atomic_load(&K.st), K.tier, lv, (uint32_t)regs[3], pm);
    if (pm == 0 || pm == 17) { LOG("秘境：在关卡 %d 死亡后复活回城（原版目的地 %d，方式 %d）→ 统一回哈洛加斯，秘境保持进行中", unit_level(pl), lv, (int)(uint32_t)regs[3]);
        ((uint64_t *)(uintptr_t)regs)[2] = LV_TOWN; return; }
    if (dm_hud_area_is_town(lv) || in_rift_map(lv)) return;
    int nx = next_map_index(); LOG("秘境：从关卡 %d 去 %d（按关卡号换关）→ 入口处换成第 %d 张图 %d", unit_level(pl), lv, nx + 1, K.maps[nx]);
    K.cur = nx; L.lv_prev = K.maps[nx]; ((uint64_t *)(uintptr_t)regs)[2] = (uint64_t)(uint32_t)K.maps[nx];
}

















void svc_save_act(uint64_t *regs) {
    uint64_t pl = regs[22], G = regs[21]; uint32_t act = (uint32_t)regs[9] & 0xff; int st = atomic_load(&K.st); regs[8] = 0;
    if ((st == ST_RUN || st == ST_DONE) && G && G == L.G && pl && pl == L.player) { int lv = unit_level(pl);
        if (in_rift_map(lv) && act != 4) { static int n; if (n++ < 6) LOG("秘境：人在秘境图 %d 里存档，存档里的当前幕 %u → 4（下次进局落哈洛加斯）", lv, act); regs[9] = 4; } }
}












int dm_rift_cleared_any(void) { return R.best >= 1; }
int dm_rift_merc_unlocked(void) { static int test = -1; if (test < 0) test = getenv("DM_TEST_MERC_SLOTS") != NULL; return R.best >= 10 || test; }
void dm_rift_merc_locked_hint(void) {


    static uint64_t last; struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); uint64_t now = (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000; if (!B || now - last < 500) return; last = now;
    dm_game_notice(B, dm_str(B, "dmMercSlotLocked")); LOG("佣兵装备位：提示「%s」", dm_str(B, "dmMercSlotLocked")); }
static int merc_class(uint64_t u) { uint8_t v = *(uint8_t *)(uintptr_t)(u + 0x1bd); if (!B || v < 1 || v > 3) return 0; uint64_t T = rd((uint64_t)(uintptr_t)(B + GA_DATA_TABLES + v * 16)), a = rd(T + 0x558), n = rd(T + 0x560);
    int cls = *(int32_t *)(uintptr_t)(u + 4); for (uint64_t k = 0; a && k < n && k < 4096; k++) if (*(int32_t *)(uintptr_t)(a + k * 0x150 + 8) == cls) return 1; return 0; }




int dm_rift_merc_slot_empty(int loc) {
    if (!B) return 0;
    for (int k = 0; k < 128; k++) for (uint64_t u = rd((uint64_t)(uintptr_t)(B + GA_CLIENT_UNIT_TABLE + 1 * 0x400 + k * 8)); u; u = rd(u + 0x158)) { if (!merc_class(u)) continue;
        uint64_t a0 = rd(u + 0xa0), iv = a0 && *(int32_t *)(uintptr_t)(a0 + 8) && rd(u + 0x98) ? rd(u + 0x98) : rd(u + 0x90); int n = 0;
        for (uint64_t it = iv ? rd(iv + 0x10) : 0; it && n < 40; n++) { uint64_t d = rd(it + 0x10); if (!d) break; if (*(uint8_t *)(uintptr_t)(d + 0x54) == loc) return 0; it = rd(d + 0xb0); }
        return 1; }
    return 0;
}
void svc_merc_slot(uint64_t *regs) {
    static const int ty[] = { 10, 12, 15, 16, 19 }; static int test = -1, nlog; regs[8] = 0; if ((uint32_t)regs[0]) return;
    if (!B) { for (int i = 0; i < g_nmod; i++) if (!strcmp(g_mod[i].name, "main")) B = (uint8_t *)g_mod[i].base; if (!B) return; }
    uint64_t it = regs[20], own = rd(regs[22]); if (!it || !own || *(uint32_t *)(uintptr_t)own != 1 || !merc_class(own)) return;
    int hit = 0; for (unsigned i = 0; i < sizeof ty / sizeof *ty && !hit; i++) hit = ((int (*)(uint64_t, int))(B + GA_ITEM_IS_TYPE))(it, ty[i]) != 0; if (!hit) return;
    if (test < 0) test = getenv("DM_TEST_MERC_SLOTS") != NULL;
    int req = atomic_load(&g_merc_req), unlocked = R.best >= 10 || test;
    if (req && !unlocked) { if (nlog++ < 12) LOG("佣兵装备位：玩家要给佣兵穿新部位的物品（单位 id %u），秘境最高通关 %d 层不到 10 层 → 不放行", *(uint32_t *)(uintptr_t)(it + 8), R.best); dm_rift_merc_locked_hint(); return; }
    regs[0] = 1; if (nlog++ < 12) LOG("佣兵装备位：%s，物品（单位 id %u，类号 %u）放行到佣兵（类号 %d）身上", req ? "玩家新穿" : "读档落位", *(uint32_t *)(uintptr_t)(it + 8), *(uint32_t *)(uintptr_t)(it + 4), *(int32_t *)(uintptr_t)(own + 4));
}




void svc_merc_slot_cl(uint64_t *regs) {
    static const int ty[] = { 10, 12, 15, 16, 19 }; regs[8] = 0; uint64_t it = regs[19]; if (!it || (uint32_t)regs[1] != 0x25) return;
    if (!B) { for (int i = 0; i < g_nmod; i++) if (!strcmp(g_mod[i].name, "main")) B = (uint8_t *)g_mod[i].base; if (!B) return; }
    if (!dm_rift_merc_unlocked()) return;
    for (unsigned i = 0; i < sizeof ty / sizeof *ty; i++) if (((int (*)(uint64_t, int))(B + GA_ITEM_IS_TYPE))(it, ty[i])) { regs[1] = (uint64_t)ty[i]; return; }
}




static void merc_dump(uint64_t G, uint64_t pl, uint32_t t, const char *why) {
    uint64_t m = ((uint64_t (*)(uint64_t, uint64_t, int, int))(B + GA_PET_FIND))(G, pl, 7, 0); if (!m) { LOG("佣兵装备位：拍 %u（%s）没有佣兵", t, why); return; }
    uint64_t a0 = rd(m + 0xa0), iv = a0 && *(int32_t *)(uintptr_t)(a0 + 8) && rd(m + 0x98) ? rd(m + 0x98) : rd(m + 0x90); char o[600]; int c = 0, n = 0; o[0] = 0;
    for (uint64_t it = iv ? rd(iv + 0x10) : 0; it && n < 40; n++) { uint64_t d = rd(it + 0x10); if (!d) break; uint32_t code = ((uint32_t (*)(uint64_t))(B + GA_ITEM_CODE))(it);
        c += snprintf(o + c, sizeof o - (size_t)c, " [%.3s 部位 %u 模式 %u id %u]", (const char *)&code, *(uint8_t *)(uintptr_t)(d + 0x54), *(uint32_t *)(uintptr_t)(it + 0xc), *(uint32_t *)(uintptr_t)(it + 8)); if (c > 520) break; it = rd(d + 0xb0); }
    { uint32_t mid = *(uint32_t *)(uintptr_t)(m + 8); uint64_t cu = rd((uint64_t)(uintptr_t)(B + GA_CLIENT_UNIT_TABLE + 1 * 0x400 + (mid & 0x7f) * 8)); while (cu && *(uint32_t *)(uintptr_t)(cu + 8) != mid) cu = rd(cu + 0x158);
      uint64_t ca0 = cu ? rd(cu + 0xa0) : 0, civ = cu ? (ca0 && *(int32_t *)(uintptr_t)(ca0 + 8) && rd(cu + 0x98) ? rd(cu + 0x98) : rd(cu + 0x90)) : 0; char co[400]; int cc = 0, cn = 0; co[0] = 0;
      for (uint64_t it = civ ? rd(civ + 0x10) : 0; it && cn < 40; cn++) { uint64_t d = rd(it + 0x10); if (!d) break; uint32_t code = ((uint32_t (*)(uint64_t))(B + GA_ITEM_CODE))(it); cc += snprintf(co + cc, sizeof co - (size_t)cc, " [%.3s 部位 %u]", (const char *)&code, *(uint8_t *)(uintptr_t)(d + 0x54)); if (cc > 340) break; it = rd(d + 0xb0); }
      LOG("佣兵装备位：拍 %u（%s）客户端佣兵%s物品 %d 件：%s", t, why, cu ? "" : "（没找到）", cn, co); }
    LOG("佣兵装备位：拍 %u（%s）佣兵 id %u 类号 %d 物品 %d 件：%s｜生命上限 %d 防御 %d 力量 %d 敏捷 %d 等级 %d 全抗(火) %d", t, why, *(uint32_t *)(uintptr_t)(m + 8), *(int32_t *)(uintptr_t)(m + 4), n, o, gs(m, 7) >> 8, gs(m, 31), gs(m, 0), gs(m, 2), gs(m, 12), gs(m, 39));
}
static void merc_test(uint64_t G, uint64_t pl, uint32_t t) {
    static const char *eq, *dp; static int init; if (!init) { init = 1; eq = getenv("DM_TEST_MERC_EQUIP"); dp = getenv("DM_TEST_MERC_DUMP"); }
    for (const char *p = dp; p && *p; ) { if ((uint32_t)atoi(p) == t) merc_dump(G, pl, t, "定时"); p = strchr(p, ','); if (p) p++; }
    for (const char *p = eq; p && *p; ) { unsigned at = 0, loc = 0; char code[8] = ""; if (sscanf(p, "%u:%3[^:]:%u", &at, code, &loc) == 3 && at == t) {
            uint64_t m = ((uint64_t (*)(uint64_t, uint64_t, int, int))(B + GA_PET_FIND))(G, pl, 7, 0), a0 = rd(pl + 0xa0), iv = a0 && *(int32_t *)(uintptr_t)(a0 + 8) && rd(pl + 0x98) ? rd(pl + 0x98) : rd(pl + 0x90), found = 0;
            for (uint64_t it = iv ? rd(iv + 0x10) : 0, n = 0; it && n < 400 && !found; n++) { uint64_t d = rd(it + 0x10); if (!d) break; uint32_t cd = ((uint32_t (*)(uint64_t))(B + GA_ITEM_CODE))(it);
                if (!memcmp(&cd, code, 3) && *(uint32_t *)(uintptr_t)(it + 0xc) == 0) found = it; it = rd(d + 0xb0); }
            if (!m || !found) LOG("佣兵装备位：测试 拍 %u 给佣兵穿 %s → 部位 %u：%s", t, code, loc, !m ? "没有佣兵" : "玩家物品栏里没找到这种物品");
            else { merc_dump(G, pl, t, "穿之前"); uint8_t pk[25] = { 0x5b }; uint32_t none = 0xffffffff; uint64_t ip = rd(found + 0x38); uint32_t cell = ip ? ((*(uint32_t *)(uintptr_t)(ip + 0x10) & 0xffff) | (*(uint32_t *)(uintptr_t)(ip + 0x14) << 16)) : 0;

                memcpy(pk + 1, (void *)(uintptr_t)(found + 8), 4); memcpy(pk + 5, (void *)(uintptr_t)(m + 8), 4); memcpy(pk + 9, &none, 4); memcpy(pk + 13, &loc, 4); memcpy(pk + 17, &cell, 4); memcpy(pk + 21, &cell, 4);
                atomic_store(&g_merc_req, 1); *(uint32_t *)(uintptr_t)(G + 0x19050) = 1; ((void (*)(uint64_t, uint64_t, uint8_t *, uint32_t))(B + GA_C2S_DISPATCH))(G, pl, pk, 25); *(uint32_t *)(uintptr_t)(G + 0x19050) = 0;
                LOG("佣兵装备位：测试 拍 %u 替玩家发 0x5b：物品 %s（id %u）→ 佣兵部位 %u；发完物品模式 %u、部位字段 %u、佣兵回执字节 %#x", t, code, *(uint32_t *)(uintptr_t)(found + 8), loc, *(uint32_t *)(uintptr_t)(found + 0xc), *(uint8_t *)(uintptr_t)(rd(found + 0x10) + 0x54), *(uint8_t *)(uintptr_t)(m + 0xb2));
                merc_dump(G, pl, t, "穿之后"); } }
        p = strchr(p, ';'); if (p) p++; }
}








static void rift_quest_obs(const uint64_t *regs) {
    static uint8_t stand_in[0x200]; uint64_t G = regs[0], mon = regs[1]; int st = atomic_load(&K.st);






    if ((st != ST_RUN && st != ST_DONE) || !G || G != L.G || !mon || !(in_rift_map(any_level(L.player)) || in_rift_map(any_level(mon)))) return;
    { int ml = any_level(mon); static int nb; if (!in_rift_map(ml) && nb++ < 20) LOG("秘境：怪物类号 %u（单位 id %u）死在邻图 %d（人在秘境图里），照样不走原版任务事件", *(uint32_t *)(uintptr_t)(mon + 4), *(uint32_t *)(uintptr_t)(mon + 8), ml); }
    static int n; if (n++ < 6 || (g_suseq_id && *(uint32_t *)(uintptr_t)(mon + 8) == g_suseq_id)) LOG("秘境：怪物类号 %u（单位 id %u）死在秘境图里，不走原版任务事件", *(uint32_t *)(uintptr_t)(mon + 4), *(uint32_t *)(uintptr_t)(mon + 8));
    ((uint64_t *)(uintptr_t)regs)[1] = (uint64_t)(uintptr_t)stand_in;
}














static void rift_preset_obs(const uint64_t *regs) {
    static uint8_t stand_in[0x40]; static int n_blk; uint64_t G = regs[0], room = regs[1], rec = regs[2]; int st = atomic_load(&K.st);
    if ((st != ST_RUN && st != ST_DONE) || !G || G != L.G || !room || !rec || *(int32_t *)(uintptr_t)(rec + 0x20) != 1 || !hi_on("pre", 90)) return;
    uint64_t ex = rd(room + 0x18), lv = ex ? rd(ex + 0x90) : 0; int lvno = lv ? *(int32_t *)(uintptr_t)(lv + 0x1f8) : 0; if (!in_rift_map(lvno)) return;
    uint64_t T = tables(G); if (!T) return; int N = *(int32_t *)(uintptr_t)(T + 0xf98), S = *(int32_t *)(uintptr_t)(T + 0x10b8), c = *(int32_t *)(uintptr_t)(rec + 4), blk;
    if (c < 0) return;
    if (c < N) blk = (c >= 497 && c <= 500) || (c >= 516 && c <= 519) || c == 528;
    else if (c < N + S) blk = 0;
    else { int p = c - N - S; blk = !(p == 0 || p == 4 || p == 5 || (p >= 14 && p <= 16) || (p >= 29 && p <= 31)); }
    if (!blk) return;
    memcpy(stand_in, (const void *)(uintptr_t)rec, 0x30); *(int32_t *)(stand_in + 4) = -1; *(uint64_t *)(stand_in + 0x18) = 0;
    ((uint64_t *)(uintptr_t)regs)[2] = (uint64_t)(uintptr_t)stand_in;
    static int all = -1; if (all < 0) all = getenv("DM_RIFT_TEST_TOUR") != NULL;
    if (n_blk++ < 6 || n_blk % 100 == 0 || all) LOG("秘境：第 %d 层 关卡 %d 不放预设怪 类号 %d（%s %d；monstats %d 行、超级独特 %d 行；本进程累计 %d 条）", K.tier, lvno, c, c < N ? "普通" : "放置码", c < N ? c : c - N - S, N, S, n_blk);
}
static void rift_gates(void) { static int once; if (once) return; once = 1;
    if (gate_obs_present(OBS_PRESET_SPAWN)) gate_obs_set(OBS_PRESET_SPAWN, rift_preset_obs); else LOG("秘境：这个产物不带放预设单位的门（r44 起的 touch 产物才带），90 层起普通预设怪照原版放");
    if (gate_obs_present(OBS_QUEST_KILL)) gate_obs_set(OBS_QUEST_KILL, rift_quest_obs); else LOG("秘境：这个产物不带任务事件的门（r36 起的 touch 产物才带），秘境里的首领死亡会走原版任务");
    if (gate_obs_present(OBS_LEVEL_WARP)) gate_obs_set(OBS_LEVEL_WARP, rift_level_obs);
    else LOG("秘境：这个产物不带出口换目的地的门（只有 r36 起的 touch 产物带），走出口会先落到原版的下一关、再由每拍检测送进下一张图"); }
void dm_rift_c2s(uint64_t G, const uint8_t *pkt, uint32_t len) {
    if (len && (pkt[0] == 0x51 || pkt[0] == 0x5b)) atomic_store(&g_merc_req, 1);
    if (G != L.G || len < 9 || pkt[0] != 0x41 || (!L.door && !L.next_door && !L.town_door)) return;
    uint32_t id, ty; memcpy(&id, pkt + 1, 4); memcpy(&ty, pkt + 5, 4);
    if (ty == 2 && id && (id == L.door || id == L.next_door || id == L.town_door)) { L.pend = id; L.pend_at = tick_of(G); }
}











static struct { int row, old, lv; } g_mz[32]; static int g_nmz; static uint8_t g_built[140];


static void built_scan(uint64_t player) { uint64_t dr = rd(level_of(player) + 0x1c8); int n = 0;
    for (uint64_t lv = dr ? rd(dr + 0x868) : 0; lv && n < 200; lv = rd(lv + 0x1b8), n++) { int no = *(int32_t *)(uintptr_t)(lv + 0x1f8); if (no > 0 && no < 140 && rd(lv + 0x10)) g_built[no] = 1; } }
static void maze_restore(uint64_t G, uint64_t player) {
    uint64_t T = tables(G), a = T ? rd(T + 0x18f8) : 0; int k = 0; memset(g_built, 0, sizeof g_built); built_scan(player);
    for (int i = 0; i < g_nmz; i++) { if (!a) continue;
        if (g_built[g_mz[i].lv]) { LOG("秘境：关卡 %d 新局里已按改小的房间数建好，LvlMaze 第 %d 行不写回", g_mz[i].lv, g_mz[i].row); g_mz[k++] = g_mz[i]; continue; }
        *(int32_t *)(uintptr_t)(a + (uint64_t)g_mz[i].row * 0x1c + 0xc) = g_mz[i].old; LOG("秘境：换局，LvlMaze 第 %d 行地狱房间数写回 %d", g_mz[i].row, g_mz[i].old); }
    g_nmz = k;
}


static struct { uint64_t G; int have, den; uint8_t umin, umax; } g_reg[140];
static int32_t *region_of(uint64_t G, int lv, uint8_t **umin, uint8_t **umax) { uint64_t r = lv > 0 && lv < 140 ? rd(G + 0x1d0 + 8ull * lv) : 0; if (!r) return 0;
    if (g_reg[lv].G != G || !g_reg[lv].have) { g_reg[lv].G = G; g_reg[lv].have = 1; g_reg[lv].den = *(int32_t *)(uintptr_t)(r + 0x2c4); g_reg[lv].umin = *(uint8_t *)(uintptr_t)(r + 0x2c8); g_reg[lv].umax = *(uint8_t *)(uintptr_t)(r + 0x2c9); }
    *umin = (uint8_t *)(uintptr_t)(r + 0x2c8); *umax = (uint8_t *)(uintptr_t)(r + 0x2c9); return (int32_t *)(uintptr_t)(r + 0x2c4); }
static void region_set(uint64_t G, int mode) {
    for (int i = 0; i < K.nmaps; i++) { int lv = K.maps[i]; uint8_t *umin, *umax; int32_t *den = region_of(G, lv, &umin, &umax); if (!den) continue;
        int d = mode ? 0 : g_reg[lv].den, a = mode ? 0 : g_reg[lv].umin, b = mode ? 0 : g_reg[lv].umax; if (*den == d && *umin == a && *umax == b) continue;
        LOG("秘境：关卡 %d 本局怪物区域 密度 %d → %d、精英组 %d–%d → %d–%d（%s）", lv, *den, d, *umin, *umax, a, b, mode ? "已结算，不再填怪" : "写回原值"); *den = d; *umin = (uint8_t)a; *umax = (uint8_t)b; } }






static void inv_gone_pump(uint64_t G, uint32_t t) {
    for (int i = 0; i < 2; i++) { uint32_t id = g_inv_gone[i]; if (!id || find_unit(G, 1, id)) continue;
        uint64_t T = tables(G), heads = rd(G + 0x1b0); int acts = T ? *(int32_t *)(uintptr_t)(T + 0x108) : 0, hit = 0; if (!heads || acts <= 0 || acts > 8) continue;
        for (int a = 0; a < acts && !hit; a++) for (uint64_t nd = rd(heads + 8ull * a), k = 0; nd && k < 4096 && !hit; nd = rd(nd + 0x20), k++) for (uint64_t r = rd(nd + 0x10), j = 0; r && j < 1024; r = rd(r + 0xb0), j++)
            if (*(uint32_t *)(uintptr_t)(r + 0xc) == id) { *(uint32_t *)(uintptr_t)(r + 0x14) |= 0x100; *(uint32_t *)(uintptr_t)(r + 0xc) = 0xffffffffu; hit = 1; break; }
        if (hit) { LOG("秘境：拍 %u 退场的闯入首领（单位 id %u）所在房间已卸载，它的存档记录改成不还原", t, id); g_inv_gone[i] = 0; }
        else { static uint32_t warned; if (warned != id && t % 250 == 0) { warned = id; LOG("秘境：拍 %u 退场的闯入首领（单位 id %u）不在单位表里，房间存档里也没找到它的记录", t, id); } } }
}






static int unit_align(uint64_t u) { uint64_t sl = rd(u + 0x88); if (!sl || !(*(uint32_t *)(uintptr_t)(sl + 0x1c) & 0x80000000u)) return 0;
    for (int pass = 0; pass < 2; pass++) for (uint64_t n = rd(sl + (pass ? 0xd8 : 0xd0)), k = 0; n && k < 512; n = rd(n + 0x68), k++)
        if (*(uint32_t *)(uintptr_t)(n + 0x20) == 0x69) return ((int (*)(int, uint64_t, int, int))(B + GA_STATLIST_GET))(*(uint8_t *)(uintptr_t)(u + 0x1bd), n, 0xac, 0);
    return 0; }






static struct { uint64_t G; int have, x0, y0, x1, y1; } g_box[140];
static void level_box(uint64_t G, int lv, uint64_t lvt) { if (lv <= 0 || lv >= 140 || (g_box[lv].have && g_box[lv].G == G)) return; int x0 = 1 << 30, y0 = 1 << 30, x1 = -1, y1 = -1, n = 0;
    for (uint64_t r = dm_level_rooms(lvt); r && n < 2000; r = rd(r + 0x48), n++) { int x = *(int32_t *)(uintptr_t)(r + 0x60) * 5, y = *(int32_t *)(uintptr_t)(r + 0x64) * 5, w = *(int32_t *)(uintptr_t)(r + 0x68) * 5, h = *(int32_t *)(uintptr_t)(r + 0x6c) * 5;
        if (x < x0) x0 = x; if (y < y0) y0 = y; if (x + w > x1) x1 = x + w; if (y + h > y1) y1 = y + h; }
    if (x1 < 0) return; g_box[lv].G = G; g_box[lv].have = 1; g_box[lv].x0 = x0; g_box[lv].y0 = y0; g_box[lv].x1 = x1; g_box[lv].y1 = y1; }
static void saved_purge(uint64_t G) { uint64_t T = tables(G), heads = rd(G + 0x1b0); int acts = T ? *(int32_t *)(uintptr_t)(T + 0x108) : 0, n = 0; if (!heads || acts <= 0 || acts > 8) return;
    for (int a = 0; a < acts; a++) for (uint64_t nd = rd(heads + 8ull * a), k = 0; nd && k < 4096; nd = rd(nd + 0x20), k++) for (uint64_t r = rd(nd + 0x10), j = 0; r && j < 1024; r = rd(r + 0xb0), j++) {
        if (*(uint8_t *)(uintptr_t)(r + 0x15) & 1) continue; int x = *(int32_t *)(uintptr_t)r, y = *(int32_t *)(uintptr_t)(r + 4), in = 0;
        for (int i = 0; i < K.nmaps && !in; i++) { int lv = K.maps[i]; in = lv > 0 && lv < 140 && g_box[lv].have && g_box[lv].G == G && x >= g_box[lv].x0 && x < g_box[lv].x1 && y >= g_box[lv].y0 && y < g_box[lv].y1; }
        if (!in) continue; *(uint32_t *)(uintptr_t)(r + 0x14) |= 0x100; *(uint32_t *)(uintptr_t)(r + 0xc) = 0xffffffffu; n++; }
    if (n) LOG("秘境：卸载房间的存档里 %d 条怪物记录改成不还原", n); }
static void presets_skip(uint64_t lvt) { int n = 0, set = 0; for (uint64_t r = dm_level_rooms(lvt); r && n < 2000; r = rd(r + 0x48), n++) for (uint64_t u = rd(r + 0x98), k = 0; u && k < 256; u = rd(u + 0x10), k++)
        if (*(int32_t *)(uintptr_t)(u + 0x20) == 1 && !(*(uint8_t *)(uintptr_t)(u + 0x28) & 1)) { *(uint8_t *)(uintptr_t)(u + 0x28) |= 1; set++; }
    if (set) LOG("秘境：已结算的图里 %d 个怪物预设点置了跳过位", set); }
static void rift_setup(uint64_t G) {
    uint64_t T = tables(G), a = T ? rd(T + 0x18f8) : 0, n = T ? rd(T + 0x1900) : 0;
    for (int i = 0; i < K.nmaps; i++) { int lv = K.maps[i]; uint64_t r = rd(G + 0x1d0 + 8ull * lv); if (!r) { LOG("【红】秘境：关卡 %d 没有怪物区域", lv); continue; }
        uint8_t *umin, *umax; int32_t *den = region_of(G, lv, &umin, &umax); int d0 = g_reg[lv].den, a0 = g_reg[lv].umin, b0 = g_reg[lv].umax;


        int e2 = tier_elite_x2(K.tier); *den = d0 * 3 / 2 > 10000 ? 10000 : d0 * 3 / 2; *umin = a0 * e2 / 4 > 255 ? 255 : a0 * e2 / 4; *umax = b0 * e2 / 4 > 255 ? 255 : b0 * e2 / 4;
        { const char *e = getenv("DM_RIFT_TEST_NAT"); int pct = e ? atoi(e) : hi_on("su", 75) ? su_nat_pct(K.tier) : 100; *den = (int)((long long)*den * pct / 100); *umin = (uint8_t)(*umin * pct / 100); *umax = (uint8_t)(*umax * pct / 100); }
        LOG("秘境：关卡 %d 本局怪物区域 密度 %d → %d（×1.5）、精英组 %d–%d → %d–%d（第 %d 层 ×%d.%02d）", lv, d0, *den, a0, b0, *umin, *umax, K.tier, e2 / 4, e2 % 4 * 25);
        for (uint64_t k = 0; a && k < n; k++) if (*(int32_t *)(uintptr_t)(a + k * 0x1c) == lv) { int32_t *rm = (int32_t *)(uintptr_t)(a + k * 0x1c + 0xc); int had = 0; for (int q = 0; q < g_nmz; q++) had |= g_mz[q].lv == lv;
            if (g_built[lv]) LOG("秘境：关卡 %d 服务端已建，LvlMaze 地狱房间数不动（%d）", lv, *rm);
            else if (*rm > 4 && !had && g_nmz < 32) { g_mz[g_nmz].row = (int)k; g_mz[g_nmz].old = *rm; g_mz[g_nmz++].lv = lv; *rm = (*rm * 6 + 9) / 10; LOG("秘境：关卡 %d LvlMaze 地狱房间数 %d → %d（留到换局）", lv, g_mz[g_nmz - 1].old, *rm); } break; } }
}





static void give_item2(uint64_t G, uint64_t player, const char *code4, int q, int ilvl, int urow, uint32_t fl);
static void give_item(uint64_t G, uint64_t player, const char *code4, int q, int ilvl) { give_item2(G, player, code4, q, ilvl, -1, 0); }
static void give_item2(uint64_t G, uint64_t player, const char *code4, int q, int ilvl, int urow, uint32_t fl) {
    uint64_t T = tables(G), ix = T ? rd(T + 0x258) : 0, ar = ix ? rd(ix) : 0, cnt = ix ? rd(ix + 8) : 0; uint32_t code; memcpy(&code, code4, 4); int cls = -1;
    for (uint64_t k = 0; ar && k < cnt; k++) if (*(uint32_t *)(uintptr_t)(ar + k * 8) == code) { cls = *(int32_t *)(uintptr_t)(ar + k * 8 + 4); break; }
    if (cls < 0) { LOG("【红】秘境：物品码 %.4s 查不到", code4); return; }
    int32_t p0[2] = { 0, 0 }, p1[2] = { 0, 0 }; ((void (*)(uint64_t, int32_t *))(B + GA_UNIT_GET_POS))(player, p0); uint64_t pth = rd(player + 0x38), room0 = pth ? rd(pth + 0x20) : 0;
    uint64_t room = ((uint64_t (*)(uint64_t, uint64_t, int32_t *, int32_t *, int))(B + GA_ROOM_FIND_DROP_SPOT))(G, room0, p0, p1, 1); if (!room) return;
    uint8_t prm[0xd8]; memset(prm, 0, sizeof prm);
    *(uint64_t *)(prm + 0) = player; *(uint64_t *)(prm + 0x10) = G; *(int32_t *)(prm + 0x18) = ilvl; *(int32_t *)(prm + 0x20) = cls; *(int32_t *)(prm + 0x24) = 3;
    *(int32_t *)(prm + 0x28) = p1[0]; *(int32_t *)(prm + 0x2c) = p1[1]; *(uint64_t *)(prm + 0x30) = room; *(uint16_t *)(prm + 0x38) = 1; *(uint16_t *)(prm + 0x3a) = *(uint16_t *)(uintptr_t)(G + 0x128); *(int32_t *)(prm + 0x40) = q;
    if (urow >= 0) { int32_t ur = urow + 1; memcpy(prm + 0x50, &ur, 4);
        uint32_t *w = (uint32_t *)(uintptr_t)(G + 0x3638 + 4 * ((uint32_t)urow >> 5)); *w &= ~(1u << (urow & 31)); }
    *(uint32_t *)(prm + 0x54) = fl;
    uint64_t it = ((uint64_t (*)(uint64_t, void *, int))(B + GA_ITEM_SPAWN))(G, prm, 0); uint64_t d = it ? rd(it + 0x14 * 0 + 0x10) : 0;
    LOG("秘境：掉落 %.4s 品质 %d → %s（物品数据 品质 %d、暗金行 %d、物品等级 %d、标志 %#x）", code4, q, it ? "成" : "【红】败", d ? *(int32_t *)(uintptr_t)d : -1, d ? *(int32_t *)(uintptr_t)(d + 0x34) : -1, d ? *(int32_t *)(uintptr_t)(d + 0x38) : -1, d ? *(uint32_t *)(uintptr_t)(d + 0x18) : 0);
}




static const char *k_eth[] = { "7s8", "7wc", "7pa", "7vo", "7h7", "7gd", "7cr", "7wa", "utp", "uui", "uar", "uit" };




static int rotw(void) { uint64_t g = rd((uint64_t)(uintptr_t)(B + GA_GAME_INFO)); return g && *(uint8_t *)(uintptr_t)(g + 0x5c) == 3; }
static void give_sunder(uint64_t G, uint64_t player) { int left[6], n = 0; for (int k = 0; k < 6; k++) if (!(R.sunder & (1u << k))) left[n++] = k;
    int k = n ? left[rnd(n)] : rnd(6); R.sunder |= 1u << k; static const int row[6] = { 426, 428, 429, 430, 431, 432 }; give_item2(G, player, "cm3 ", 7, 99, rotw() ? row[k] : 401 + k, 0); }
static void milestone(uint64_t G, uint64_t player, int m) {
    int N = m / 10; LOG("秘境：第 %d 层首通里程碑（第 %d 个）", m, N);
    char rc[5] = "r18 "; int r = 18 + rnd(16); rc[1] = (char)('0' + r / 10); rc[2] = (char)('0' + r % 10); give_item(G, player, rc, 2, 99);
    give_item(G, player, "toa ", 2, 99);
    { uint8_t d = *(uint8_t *)(uintptr_t)(G + 0x104); uint64_t q = rd(rd(rd(player + 0x10) + 0x40 + 8 * d)); if (q) { uint8_t *b = (uint8_t *)(uintptr_t)(q + 0x46); *b = (uint8_t)((*b & ~1) | 2); LOG("秘境：拉苏克打孔机会重置（任务块 +0x46 → %#x）", *b); } }
    for (int k = 1; k <= N; k++) switch (k) {
        case 1: case 9: give_sunder(G, player); break;
        case 2: give_item2(G, player, "cm1 ", 7, 110, 381, 0); break;
        case 3: give_item2(G, player, "cm2 ", 7, 110, 400, 0); break;
        case 4: { char c[5]; memcpy(c, k_eth[rnd((int)(sizeof k_eth / sizeof *k_eth))], 3); c[3] = ' '; c[4] = 0; give_item2(G, player, c, 2, 87, -1, 0x400000 | 0x800); break; }
        case 5: if (rotw()) give_item2(G, player, "cjw ", 7, 110, 420 + rnd(6), 0); else give_item2(G, player, "jew ", 7, 99, 392 + rnd(8), 0); break;
        case 6: give_item2(G, player, "obf ", 7, 99, 354, 0); break;
        case 7: give_item2(G, player, "ci3 ", 7, 99, 336, 0); break;
        case 8: give_item2(G, player, "uar ", 7, 99, 311, 0); break;
        case 10: { char rc[5] = "r28 "; for (int r = 28; r <= 33; r++) { rc[1] = (char)('0' + r / 10); rc[2] = (char)('0' + r % 10); give_item(G, player, rc, 2, 99); } break; } }



    R.ms |= 1u << (N - 1); say("dmRiftSayFirst", m);
}
static void finish(uint64_t G, uint64_t player, uint64_t guard) {
    uint64_t now = guest_ns();
    int left = RIFT_SECS - (int)((now - K.t0_ns) / 1000000000ull); g_left_done = left;
    hs_report(RIFT_SECS - left);
    K.inc = left >= 600 ? 3 : left >= 300 ? 2 : left > 0 ? 1 : 0; int next = next_tier();
    if (K.inc) { R.clears++; if (K.tier > R.best) R.best = K.tier; } if (K.inc && (!R.fast[K.tier] || RIFT_SECS - left < R.fast[K.tier])) R.fast[K.tier] = RIFT_SECS - left;
    if (K.inc) pts_grant(player);
    if (K.inc) for (int m = 10; m <= 100; m += 10) if (K.tier >= m && !(R.ms & (1u << (m / 10 - 1)))) milestone(G, player, m);
    R.tier = next; if (next > R.unlock) R.unlock = next; rec_save(); if (GB.on) gamble_apply(G);
    { int dm_request_save(void); LOG("秘境：结算请游戏存盘 → %d", dm_request_save()); }
    if (K.inc) { char c[5] = "xa1 "; c[2] = (char)('1' + rnd(5)); give_item(G, player, c, 2, 99); }
    if (K.inc && next == K.tier) LOG("秘境：第 %d 层完成（剩 %d:%02d）→ 已是最高层，没有下一层", K.tier, left > 0 ? left / 60 : 0, left > 0 ? left % 60 : 0);
    else if (K.inc) LOG("秘境：第 %d 层完成（剩 %d:%02d）→ 下次第 %d 层", K.tier, left > 0 ? left / 60 : 0, left > 0 ? left % 60 : 0, next); else LOG("秘境：第 %d 层超时未过关 → 不涨层、不计最高层与通关数，下次仍第 %d 层", K.tier, next);
    atomic_store(&K.st, ST_DONE); region_set(G, 1);
    pet_hp_pump(G, player, tick_of(G));
    if (K.players_old) dm_hud_request_players(K.players_old);






    { int x, y, gx, gy; uint64_t room = unit_room(player); if (!unit_xy(player, &x, &y) || !room) { LOG("【红】秘境：结算时读不到玩家位置，两扇门没开"); return; }
      float vx = -1, vy = 1; if (guard && unit_xy(guard, &gx, &gy) && (abs(x - gx) + abs(y - gy)) >= 2) { vx = (float)(x - gx); vy = (float)(y - gy); }
      { float n = sqrtf(vx * vx + vy * vy); vx /= n; vy /= n; } int bx = x + (int)lroundf(vx * 10), by = y + (int)lroundf(vy * 10), ox = (int)lroundf(-vy * 5), oy = (int)lroundf(vx * 5);
      L.nd_lv = unit_level(player); L.nd_x = x; L.nd_y = y;









      if (getenv("DM_RIFT_TEST_ND_FAR")) { uint64_t lvp = level_of(player); int fx = 0, fy = 0, fd = 0, n = 0, tried = 0; uint64_t done[24]; int nd = 0;
          while (!fx && tried < 24) { uint64_t br = 0; int best = -1; n = 0;
              for (uint64_t r = dm_level_rooms(lvp); r && n < 2000; r = rd(r + 0x48), n++) { int sk = 0; for (int q = 0; q < nd; q++) sk |= done[q] == r; if (sk) continue;
                  int cx = *(int32_t *)(uintptr_t)(r + 0x60) * 5 + *(int32_t *)(uintptr_t)(r + 0x68) * 5 / 2, cy = *(int32_t *)(uintptr_t)(r + 0x64) * 5 + *(int32_t *)(uintptr_t)(r + 0x6c) * 5 / 2, d = abs(cx - x) + abs(cy - y); if (d > best) { best = d; br = r; } }
              if (!br) break; done[nd++] = br; tried++;
              if (!rd(br + 0x58)) ((void (*)(int, uint64_t))(B + GA_ROOM_INIT))(*(uint8_t *)(uintptr_t)(G + 0x106), br);
              int rx = *(int32_t *)(uintptr_t)(br + 0x60) * 5, ry = *(int32_t *)(uintptr_t)(br + 0x64) * 5;
              for (uint64_t u = rd(br + 0x98), k = 0; u && k < 256 && !fx; u = rd(u + 0x10), k++) if (*(uint32_t *)(uintptr_t)(u + 0x20) == 1) { fx = rx + *(int32_t *)(uintptr_t)(u + 8); fy = ry + *(int32_t *)(uintptr_t)(u + 0x24); fd = abs(fx - x) + abs(fy - y); } }
          if (fx) { LOG("秘境：测试 DM_RIFT_TEST_ND_FAR 把守护者处从 %d,%d 换成远处的怪物预设点 %d,%d（相距 %d 格；试了 %d 间房，共 %d 间）", x, y, fx, fy, fd, tried, n); L.nd_x = fx; L.nd_y = fy; }
          else LOG("秘境：测试 DM_RIFT_TEST_ND_FAR 试了 %d 间最远的房都没有怪物预设点，守护者处不变", tried); }






      { static struct ds_grid dg; uint64_t lvp = level_of(player); uint32_t mk = ds_fill(0, player, x, y, &dg);
        int nx, ny, tx, ty, t1 = ds_pick(&dg, bx + ox - x, by + oy - y, 0, 0, 0, ND_GAP, &nx, &ny), t2 = ds_pick(&dg, bx - ox - x, by - oy - y, 1, nx, ny, ND_GAP, &tx, &ty);


        LOG("秘境：结算处门位 人 %d,%d 掩码 %#x 窗口内可达 %d 格；下一层想放 %d,%d（%s，走 %d 步）→ 取 %d,%d（档 %d，走 %d 步）；回城想放 %d,%d（%s，走 %d 步）→ 取 %d,%d（档 %d，走 %d 步）", x, y, mk, dg.reach,
            bx + ox, by + oy, ds_blk(&dg, bx + ox - x + DS_R, by + oy - y + DS_R) ? "挡" : "不挡", ds_dist(&dg, bx + ox - x + DS_R, by + oy - y + DS_R), x + nx, y + ny, t1, ds_dist(&dg, nx + DS_R, ny + DS_R),
            bx - ox, by - oy, ds_blk(&dg, bx - ox - x + DS_R, by - oy - y + DS_R) ? "挡" : "不挡", ds_dist(&dg, bx - ox - x + DS_R, by - oy - y + DS_R), x + tx, y + ty, t2, ds_dist(&dg, tx + DS_R, ty + DS_R));
        L.next_door = getenv("DM_RIFT_TEST_NONEXTDOOR") ? 0 : place_door(G, room_at(lvp, x + nx, y + ny), x + nx, y + ny, "结算处下一层", RIFT_OBJ, 1)                                                                                                                                                                                                   ; if (!L.next_door) LOG("【红】秘境：下一层的门开门失败");
        L.town_door = place_door(G, room_at(lvp, x + tx, y + ty), x + tx, y + ty, "结算处回城", TOWN_OBJ, 1); if (!L.town_door) LOG("【红】秘境：回城门开门失败"); }
      {






        int top = K.inc && next_tier() == K.tier;
        if (L.next_door && L.town_door) say(top ? "dmRiftSayTopBoth" : K.inc ? "dmRiftSayDoneBoth" : "dmRiftSayLateBoth");
        else if (L.next_door) say(top ? "dmRiftSayTopNext" : K.inc ? "dmRiftSayDoneNext" : "dmRiftSayLateNext"); else if (L.town_door) say(top ? "dmRiftSayTopTown" : K.inc ? "dmRiftSayDoneTown" : "dmRiftSayLateTown"); else say(top ? "dmRiftSayTop" : K.inc ? "dmRiftSayDone" : "dmRiftSayLate"); } }
}

static void menu_fill(dm_rift_menu_t *o, int m) {
    uint64_t now = guest_ns();
    int need = atomic_load(&K.need); o->menu = m; o->best = R.best; o->unlock = R.unlock < 1 ? 1 : R.unlock; o->def = R.tier; o->ms = R.ms; o->clvl = atomic_load(&M.clvl);
    o->tier = K.tier; o->done = atomic_load(&K.st) == ST_DONE; o->inc = K.inc; o->next = next_tier(); o->overtime = rift_overtime(); o->secs_left = o->done ? g_left_done : K.t0_ns ? RIFT_SECS - (int)((now - K.t0_ns) / 1000000000ull) : RIFT_SECS; if (o->secs_left < 0) o->secs_left = 0;
    o->progress = need > 0 ? (float)atomic_load(&K.score) / need : 0; if (o->progress > 1) o->progress = 1;
}
int dm_rift_menu(dm_rift_menu_t *o) { int m = atomic_load(&M.menu); if (m == MENU_NONE) return 0; menu_fill(o, m); return 1; }
int dm_rift_stats(dm_rift_stats_t *o) {
    if (!K.who[0]) return 0; o->best = R.best; o->clears = R.clears; o->deaths = R.deaths; o->tier = R.tier; o->unlock = R.unlock; o->ms = R.ms; memcpy(o->fast, R.fast, sizeof o->fast); return 1; }


int dm_rift_stats_of(const char *who, dm_rift_stats_t *o) { return who && who[0] && !strcmp(K.who, who) && g_rec_ok && dm_rift_stats(o); }
void dm_rift_menu_close(void) { atomic_store(&M.menu, MENU_NONE); }






static int town_menu(int st) { return st == ST_RUN || (st == ST_DONE && L.nd_lv) ? MENU_CHOICE : MENU_SELECT; }


static void menu_reset(void) { int a = atomic_exchange(&M.req_start, 0), b = atomic_exchange(&M.req_cont, 0), c = atomic_exchange(&M.req_abandon, 0), m = atomic_exchange(&M.menu, MENU_NONE);
    if (a || b || c || m) LOG("秘境：新局，清掉上一局留下的请求（开始 %d / 继续 %d / 放弃 %d / 页 %d）", a, b, c, m); }
void dm_rift_menu_peek(dm_rift_menu_t *o) { menu_fill(o, town_menu(atomic_load(&K.st))); }
void dm_rift_menu_start(int tier) { atomic_store(&M.req_start, tier); atomic_store(&M.menu, MENU_NONE); }
void dm_rift_menu_continue(void) { atomic_store(&M.req_cont, 1); atomic_store(&M.menu, MENU_NONE); }
void dm_rift_menu_abandon(void) { atomic_store(&M.req_abandon, 1); atomic_store(&M.menu, MENU_NONE); }
void dm_rift_menu_next(void) { atomic_store(&M.req_next, 1); atomic_store(&M.menu, MENU_NONE); }
static _Atomic int g_menu_seq; int dm_rift_menu_seq(void) { return atomic_load(&g_menu_seq); }
int dm_rift_menu_pending(void) { return atomic_load(&M.req_abandon) != 0; }



static int start_rift(uint64_t G, uint64_t player, uint32_t t, int tier) {


    int fo[8], nf = 0, fm[24], nm = 0; for (unsigned k = 0; k < sizeof k_open / sizeof *k_open; k++) { int u = 0; for (int q = 0; q < L.nused; q++) u |= L.used[q] == k_open[k]; if (!u) fo[nf++] = k_open[k]; }
    built_scan(player);
    for (unsigned k = 0; k < sizeof k_maze / sizeof *k_maze; k++) { int u = 0; for (int q = 0; q < L.nused; q++) u |= L.used[q] == k_maze[k]; if (!u && !g_built[k_maze[k]]) fm[nm++] = k_maze[k]; }
    { char bf[120]; int o = 0; bf[0] = 0; for (unsigned k = 0; k < sizeof k_maze / sizeof *k_maze; k++) if (g_built[k_maze[k]] && o < 100) o += snprintf(bf + o, sizeof bf - o, " %d", k_maze[k]); LOG("秘境：抽图 野外剩 %d、迷宫剩 %d（服务端已建不抽：%s）", nf, nm, o ? bf : " 无"); }
#ifndef DM_PLAYER_BUILD
    if (!nf || nm < 2 || getenv("DM_RIFT_TEST_EXHAUST")) {
#else
    if (!nf || nm < 2) {
#endif
        void dm_restart_request(void); K.resume_tier = tier;
#ifndef DM_PLAYER_BUILD
        setenv("DM_RIFT_TEST_EXHAUST_DONE", "1", 1); unsetenv("DM_RIFT_TEST_EXHAUST");
#endif
        LOG("秘境：本局新图用完（野外剩 %d、迷宫剩 %d）→ 一键重开，新局直接开第 %d 层", nf, nm, tier); dm_restart_request(); return 0; }
    K.tier = tier; { const char *e = getenv("DM_RIFT_TIER"); if (e) K.tier = atoi(e); }
    K.nmaps = 2 + rnd(2); K.maps[0] = fo[rnd(nf)]; K.maps[1] = fm[rnd(nm)]; K.maps[2] = 0;
    if (K.nmaps == 3) { do K.maps[2] = fm[rnd(nm)]; while (K.maps[2] == K.maps[1]); }


    { static int n = -1; const char *e = getenv("DM_RIFT_NEED"); if (n < 0) n = e ? atoi(e) : 125; atomic_store(&K.need, n); }


    { static int nth; const char *e = getenv("DM_RIFT_MAPS"); if (e) { for (int q = 0; q < nth; q++) { const char *nx = strchr(e, ';'); if (!nx) break; e = nx + 1; } nth++; K.maps[2] = 0; K.nmaps = sscanf(e, "%d,%d,%d", &K.maps[0], &K.maps[1], &K.maps[2]); } }


    { int bad = K.nmaps < 1 || K.nmaps > 3; for (int q = 0; q < K.nmaps && !bad; q++) bad = K.maps[q] <= 0 || K.maps[q] >= 137;
      if (bad) { LOG("【红】秘境：图表不成立（%d 张：%d %d %d），不开局", K.nmaps, K.maps[0], K.maps[1], K.maps[2]); say("dmRiftSayErr"); return 0; } }
    for (int q = 0; q < K.nmaps && L.nused < 24; q++) L.used[L.nused++] = K.maps[q];
    atomic_fetch_add(&g_run_seq, 1);
    rift_setup(G); K.cur = 0; atomic_store(&K.score, getenv("DM_RIFT_TEST_SCORE") ? atoi(getenv("DM_RIFT_TEST_SCORE")) : 0);
    atomic_store(&K.guard_dead, 0); atomic_store(&K.timeout, 0); atomic_store(&K.gdead, 0); atomic_store(&K.gtotal, 0); K.guard = 0; memset(K.guards, 0, sizeof K.guards); K.inc = 0;


    { K.nplan = hi_on("duo", 75) ? 2 : 1; K.gcls[2] = 0;
      if (K.nplan == 2) { int a = rnd(4), b = (a + 1 + rnd(3)) % 4; K.gcls[0] = k_duo[a]; K.gcls[1] = k_duo[b]; } else { K.gcls[0] = k_guard[rnd(5)]; K.gcls[1] = 0; }
      const char *e = getenv("DM_RIFT_TEST_GUARDS"); if (e) { int x = 0, y = 0, n = sscanf(e, "%d,%d", &x, &y); if (n >= 1 && x > 0) K.gcls[0] = x; if (n >= 2 && y > 0) K.gcls[1] = y; }






      { memset(K.inv_cls, 0, sizeof K.inv_cls); memset(K.inv_st, 0, sizeof K.inv_st); memset(K.inv_id, 0, sizeof K.inv_id); K.inv_n = !hi_on("invade", 80) ? 0 : (K.tier >= 90 || (getenv("DM_RIFT_TEST_HI") && strstr(getenv("DM_RIFT_TEST_HI"), "invade2"))) ? 2 : 1;
        int pool[4], np = 0; for (int q = 0; q < 4; q++) pool[np++] = k_duo[q];
        if (K.inv_n > np) K.inv_n = np;
        for (int q = 0; q < K.inv_n; q++) { int j = rnd(np); K.inv_cls[q] = pool[j]; pool[j] = pool[--np];
            K.inv_pct[q] = K.inv_n == 1 ? 30 + rnd(41) : q == 0 ? 25 + rnd(21) : 55 + rnd(26); }
        const char *e = getenv("DM_RIFT_TEST_INVADE"); if (e && K.inv_n) { int x = 0, y = 0, n = sscanf(e, "%d,%d", &x, &y); if (n >= 1 && x > 0) K.inv_cls[0] = x; if (n >= 2 && y > 0 && K.inv_n > 1) K.inv_cls[1] = y; }
        if ((e = getenv("DM_RIFT_TEST_INVPCT")) && K.inv_n) K.inv_pct[0] = atoi(e);
        if (K.inv_n) { int nd = atomic_load(&K.need); atomic_store(&K.need, nd * (100 + 12 * K.inv_n) / 100);
            if (K.inv_n >= 2) say("dmRiftSayFoes", K.inv_n);
            LOG("秘境：本场闯入首领 %d 位：类号 %d（进度 %d%% 时）%s；进度需求 %d → %d", K.inv_n, K.inv_cls[0], K.inv_pct[0], K.inv_n > 1 ? "与另一位（见闯入时的日志）" : "", nd, atomic_load(&K.need)); } }


      { K.su_next = K.su_cnt = 0; K.su_last = 0; memset(K.su_live, 0, sizeof K.su_live); int np = (int)sizeof k_su_pool; for (int q = 0; q < np; q++) K.su_order[q] = k_su_pool[q];
        for (int q = np - 1; q > 0; q--) { int j = rnd(q + 1), x = K.su_order[q]; K.su_order[q] = K.su_order[j]; K.su_order[j] = x; }
        const char *e = getenv("DM_RIFT_TEST_SUROWS"); if (e) { char bf[200]; strlcpy(bf, e, sizeof bf); int q = 0; for (char *sv, *k = strtok_r(bf, ",", &sv); k && q < np; k = strtok_r(NULL, ",", &sv)) K.su_order[q++] = atoi(k); }
        if (hi_on("su", 75)) LOG("秘境：本场特殊精英：原生怪留 %d%%，同时在场最多 %d 组，头几位的行号 %d、%d、%d", su_nat_pct(K.tier), su_cap(K.tier), K.su_order[0], K.su_order[1], K.su_order[2]); }






      K.wave = 0; K.wave_t = 0; K.nwave = 1; K.wn[0] = K.nplan; K.wcls[0][0] = K.gcls[0]; K.wcls[0][1] = K.gcls[1]; K.whp[0] = K.nplan == 2 ? hi_duo_x1000(0) : 1000; K.wdm[0] = K.nplan == 2 ? hi_duo_x1000(1) : 1000;
      if (K.nplan == 2 && hi_on("wave", 95)) { const char *tw = getenv("DM_RIFT_TEST_HI"); int three = K.tier >= 100 || (tw && strstr(tw, "wave3"));
          K.wn[0] = 1; K.wcls[0][0] = 156; K.wcls[0][1] = 0; K.wdm[0] = 1000; K.whp[0] = three ? 1000 : 500 + 100 * ((K.tier < 95 ? 95 : K.tier > 99 ? 99 : K.tier) - 95);
          K.wn[1] = 2; K.wcls[1][0] = K.gcls[0]; K.wcls[1][1] = K.gcls[1]; K.whp[1] = three ? 500 : hi_duo_x1000(0); K.wdm[1] = three ? 1000 : hi_duo_x1000(1); K.nwave = 2;
          if (three) { int q = 0; for (int k = 0; k < 4; k++) if (k_duo[k] != K.gcls[0] && k_duo[k] != K.gcls[1] && q < 2) K.wcls[2][q++] = k_duo[k]; K.wn[2] = q; K.whp[2] = 500; K.wdm[2] = 1000; K.nwave = 3; }
          LOG("秘境：本场守护者分 %d 波：安达利尔（生命 ×%d.%03d）→ 类号 %d 与 %d（×%d.%03d）%s", K.nwave, K.whp[0] / 1000, K.whp[0] % 1000, K.wcls[1][0], K.wcls[1][1], K.whp[1] / 1000, K.whp[1] % 1000, K.nwave == 3 ? " → 另外两位（×0.500）" : ""); }
      K.guard_cls = K.gcls[0]; if (K.nplan == 2 && K.nwave == 1) LOG("秘境：本场守护者两位，类号 %d 与 %d（生命 ×%d.%03d、伤害 ×%d.%03d）", K.gcls[0], K.gcls[1], hi_duo_x1000(0) / 1000, hi_duo_x1000(0) % 1000, hi_duo_x1000(1) / 1000, hi_duo_x1000(1) % 1000); }
    K.t0_ns = guest_ns();
    if (getenv("DM_RIFT_TEST_LATE")) { K.t0_ns -= (uint64_t)RIFT_SECS * 1000000000ull; LOG("秘境：测试 DM_RIFT_TEST_LATE 开局即超时"); }
    K.players_old = dm_hud_requested_players(); if (!K.players_old) K.players_old = 1;
    dm_quiet_difficulty_msg(1);
    dm_hud_request_players(K.tier < 8 ? K.tier : 8);
    L.nd_lv = 0;
    atomic_store(&K.st, ST_RUN); say("dmRiftSayStart", K.tier);
    LOG("秘境：拍 %u 开局 第 %d 层，图 %d → %d → %d（共 %d 张），/players %d", t, K.tier, K.maps[0], K.maps[1], K.maps[2], K.nmaps, K.tier < 8 ? K.tier : 8);




    { int dm_request_save(void); LOG("秘境：开局前请游戏存盘 → %d", dm_request_save()); }
    L.lv_prev = K.maps[0]; warp(G, player, K.maps[0]);
    if (!R.gift) L.gift_due = 1;
    return 1;
}




#define WAVE_GAP 75
static int guard_wave_due(uint32_t t, int lv) {


    return K.wave_t && t >= K.wave_t + WAVE_GAP && atomic_load(&K.st) == ST_RUN && in_rift_map(lv);
}
static int guard_wave(uint64_t G, uint64_t player, uint32_t t, int lowhp) {


    int lv = unit_level(player); if (!in_rift_map(lv) || !unit_room(player)) return 0;
    static const int ox[2][2] = { { 6, 0 }, { 6, -6 } }, oy[2][2] = { { 6, 0 }, { 6, 6 } }; int w = K.wave, nw = K.wn[w] < 1 ? 1 : K.wn[w] > 2 ? 2 : K.wn[w], x, y, n = 0; unit_xy(player, &x, &y); hs_sync();
    for (int i = 0; i < nw; i++) { int32_t pos[2] = { x + ox[nw - 1][i], y + oy[nw - 1][i] }; uint64_t rr = free_spot(G, unit_room(player), pos);
        uint64_t u = rr ? ((uint64_t (*)(uint64_t, uint64_t, int, int, int, int, int, int, uint64_t))(B + GA_UNIT_SPAWN))(G, rr, 1, K.wcls[w][i], pos[0], pos[1], 1, 0, 0x10) : 0;
        if (!u) { LOG("【红】秘境：守护者第 %d 波第 %d 位（类号 %d）没刷出来", w + 1, i + 1, K.wcls[w][i]); continue; }
        K.guards[i] = *(uint32_t *)(uintptr_t)(u + 8); K.gcls[i] = K.wcls[w][i]; if (!K.guard) K.guard = K.guards[i]; n++;
        if (lowhp) ss(u, 6, 256); else if (K.tier >= 9) { g_hp_mul = K.whp[w]; scale_monster(G, u, target_level(player) + 3, dm_rift_tier_level(9, gs(player, 12)) + 3); g_hp_mul = 1000; }
        if (K.wdm[w] != 1000 && !lowhp) ss(u, 25, (int)((100LL + gs(u, 25)) * K.wdm[w] / 1000 - 100));
        HS.guard_hp += gs(u, 7) >> 8;
        LOG("秘境：拍 %u 刷守护者%s%s 类号 %d id %u 等级 %d 生命 %d 伤害百分比 %d", t, K.nwave > 1 ? (w == 0 ? "（第 1 波）" : w == 1 ? "（第 2 波）" : "（第 3 波）") : "", nw == 2 ? (i ? "（第二位）" : "（第一位）") : "", K.wcls[w][i], K.guards[i], gs(u, 12), gs(u, 7) >> 8, gs(u, 25));
        LOG("秘境：守护者落点 第 %d 波 id %u 服务端关卡 %d 客户端关卡 %d 坐标 %d,%d", w + 1, K.guards[i], lv, atomic_load(&g_cli_lv), pos[0], pos[1]); }
    atomic_store(&K.gdead, 0); atomic_store(&K.gtotal, n);


    if (n) { if (w == 0) say(nw == 2 && n == 2 ? "dmRiftSayGuardians" : "dmRiftSayGuardian"); else say("dmRiftSayWave", w + 1, K.nwave); } return 1; }
void dm_rift_tick(uint64_t G, uint64_t player, uint8_t *main_base) {
    B = main_base; if (!G || !player || !B) return;
    atomic_store(&g_merc_req, 0); if (getenv("DM_TEST_MERC_EQUIP") || getenv("DM_TEST_MERC_DUMP")) merc_test(G, player, *(uint32_t *)(uintptr_t)(G + 0x170));
    rift_gates();
    pet_hp_pump(G, player, *(uint32_t *)(uintptr_t)(G + 0x170));
    { static int got; if (!got) { for (int l = 1; l < 112; l++) atomic_store(&g_mlhp[l], monlvl(G, l, 2)); got = atomic_load(&g_mlhp[85]) != 0; if (got) LOG("秘境：MonLvl 生命列已缓存（85 级 %u、96 级 %u、110 级 %u）", atomic_load(&g_mlhp[85]), atomic_load(&g_mlhp[96]), atomic_load(&g_mlhp[110])); } }
    uint32_t t = tick_of(G);
    if (G != L.G || player != L.player || t < L.last_t) {
        int st = atomic_load(&K.st);
        maze_restore(G, player);
        if (st == ST_RUN || st == ST_DONE) { region_set(G, 0); if (K.players_old) dm_hud_request_players(K.players_old); atomic_store(&K.st, ST_IDLE); LOG("秘境：换局，%s", st == ST_RUN ? "本次秘境作废" : "结束"); }
        dm_quiet_difficulty_msg(0); GS.bm_on = 0; shelf_restore(); gamble_restore(); memset(&L, 0, sizeof L);


        { static int games; if (games++ && getenv("DM_RIFT_TEST_STALE")) { atomic_store(&M.req_start, 7); atomic_store(&M.req_cont, 1); LOG("秘境：测试 DM_RIFT_TEST_STALE 换局时置上残留请求（开始 7 / 继续 1）"); } }
        if (!getenv("DM_RIFT_TEST_STALE_KEEP")) menu_reset();
        L.G = G; L.player = player; L.eligible = -1; L.lv_prev = -1; L.resume = K.resume_tier; K.resume_tier = 0;
    }
    L.last_t = t; built_scan(player);
    int lv = unit_level(player); if (!lv || !unit_room(player)) return;
    if (L.eligible < 0) { static int test = -1; if (test < 0) test = getenv("DM_RIFT_TEST") != NULL;
        uint8_t diff = *(uint8_t *)(uintptr_t)(G + 0x104); uint64_t q = rd(rd(rd(player + 0x10) + 0x40 + 8 * diff));
        L.eligible = test || (diff == 2 && q && (*(uint8_t *)(uintptr_t)(q + 0x50) & 1));
        { const char *nm = (const char *)(uintptr_t)rd(player + 0x10); if (nm) strlcpy(K.who, nm, sizeof K.who); }
        LOG("秘境：本局%s资格（难度 %u%s，角色「%s」）", L.eligible ? "有" : "没有", diff, test ? "，测试开关" : "", K.who);
        { const char *e = getenv("DM_RIFT_TEST_GIVE"); char b2[200]; if (e) { strlcpy(b2, e, sizeof b2); for (char *sv, *k = strtok_r(b2, ";", &sv); k; k = strtok_r(NULL, ";", &sv)) { int r, lv; char c[5] = "    "; if (sscanf(k, "%d:%3[a-z0-9]:%d", &r, c, &lv) == 3) { c[3] = ' '; give_item2(G, player, c, 7, lv, r, 0); } } } }
        rec_load();
        if (L.eligible) { { const char *e = getenv("DM_RIFT_TEST_BEST"); if (e) R.best = atoi(e); e = getenv("DM_RIFT_TEST_UNLOCK"); if (e) R.unlock = atoi(e); } } }
    if (!L.eligible) return;
    { static uint64_t seen; static uint32_t at; uint32_t tq = *(uint32_t *)(uintptr_t)(G + 0x170);
      if (player != seen) { seen = player; at = tq + 75; } else if (at && tq >= at) { at = 0; int p0 = R.pts; if (pts_grant(player) || R.pts != p0) rec_save(); } }
    int st = atomic_load(&K.st);
    dm_door_label(B, G, RIFT_OBJ, "dmDoorRift", lv == LV_TOWN || in_rift_map(lv));
    dm_door_label(B, G, TOWN_OBJ, "dmDoorRiftTown", in_rift_map(lv));
    if (lv == LV_TOWN) { if (!GB.on) gamble_apply(G); } else gamble_restore();
    if (lv == LV_TOWN && GB.on) shelf_apply(G, player); else shelf_restore();



    if (lv == LV_TOWN && !L.door && !L.door_tried) { L.door_tried = 1; L.door = make_rift_door(G, player); if (!L.door) L.door_tried = 0; }
    if (L.eligible > 0) atomic_store(&M.clvl, gs(player, 12));
    if (L.resume && L.eligible > 0 && lv == LV_TOWN && t > 50) { int tq = L.resume; L.resume = 0; LOG("秘境：一键重开后的新局 → 直接开第 %d 层", tq); start_rift(G, player, t, tq); return; }
    { static int at = -1, tr; if (at < 0) { const char *e = getenv("DM_RIFT_TEST_START"); at = e ? atoi(e) : 0; const char *c = e ? strchr(e, ':') : NULL; tr = c ? atoi(c + 1) : 1; }
      if (at > 0 && t >= (uint32_t)at && lv == LV_TOWN && L.eligible > 0 && atomic_load(&K.st) == ST_IDLE) { at = 0; LOG("秘境：测试 DM_RIFT_TEST_START 拍 %u 直接开第 %d 层", t, tr); start_rift(G, player, t, tr); return; } }
    { static int at = -1; if (at < 0) { const char *e = getenv("DM_RIFT_TEST_RESTART"); at = e ? atoi(e) : 0; } if (at > 0 && t >= (uint32_t)at) { at = 0; void dm_restart_request(void); LOG("秘境：测试 DM_RIFT_TEST_RESTART 拍 %u 请求一键重开", t); dm_restart_request(); } }


    { int mm = atomic_load(&M.menu); if (mm == MENU_NEXT ? (st != ST_DONE || lv != L.nd_lv || !L.next_door) : (mm && lv != LV_TOWN)) atomic_store(&M.menu, MENU_NONE); }


    if (atomic_exchange(&M.req_next, 0) && st == ST_DONE && L.next_door && lv == L.nd_lv) { L.next_door = L.town_door = 0; int nt = next_tier(); LOG("秘境：确认离开这一场，开第 %d 层（此刻地上物品 %d 件，不保留）", nt, ground_items(G)); start_rift(G, player, t, nt); return; }


    { static uint32_t at[8]; static int n = -1; if (n < 0) { n = 0; const char *e = getenv("DM_RIFT_TEST_NEXTDOOR"); char bf[96]; if (e) { strlcpy(bf, e, sizeof bf); for (char *sv, *k = strtok_r(bf, ",", &sv); k && n < 8; k = strtok_r(NULL, ",", &sv)) at[n++] = (uint32_t)atoi(k); } }
      static uint32_t d0; if (st != ST_DONE) d0 = 0; else if (!d0) d0 = t;
      for (int i = 0; i < n; i++) if (at[i] && d0 && t - d0 >= at[i] && L.next_door && lv == L.nd_lv && !atomic_load(&M.menu)) { at[i] = 0; atomic_store(&M.menu, MENU_NEXT); atomic_fetch_add(&g_menu_seq, 1);
          LOG("秘境：测试 DM_RIFT_TEST_NEXTDOOR 拍 %u（结算后 %u 拍）当作碰了守护者处的门 → 确认页；地上物品 %d 件", t, t - d0, ground_items(G)); break; } }
    { int tq = atomic_exchange(&M.req_start, 0); if (tq > 0 && lv == LV_TOWN && st != ST_RUN) { if (tq > R.unlock) tq = R.unlock; if (tq < 1) tq = 1; LOG("秘境：选层页开始第 %d 层", tq); L.next_door = L.town_door = 0; start_rift(G, player, t, tq); return; } }
    { static int at = -1; if (at < 0) { const char *e = getenv("DM_RIFT_TEST_SAVE"); at = e ? atoi(e) : 0; } if (at > 0 && t >= (uint32_t)at) { at = 0; int dm_request_save(void); LOG("秘境：测试 DM_RIFT_TEST_SAVE 拍 %u 请游戏存盘 → %d", t, dm_request_save()); } }
    { static int at = -1; if (at < 0) { const char *e = getenv("DM_RIFT_TEST_ABANDON"); at = e ? atoi(e) : 0; } if (at > 0 && t >= (uint32_t)at && st == ST_RUN) { at = 0; LOG("秘境：测试 DM_RIFT_TEST_ABANDON 拍 %u 替玩家放弃本层", t); atomic_store(&M.req_abandon, 1); } }
    if (atomic_exchange(&M.req_abandon, 0) && (st == ST_RUN || st == ST_DONE)) {
        region_set(G, 0); if (K.players_old) dm_hud_request_players(K.players_old); K.guard = 0; memset(K.guards, 0, sizeof K.guards); L.next_door = L.town_door = 0; L.nd_lv = 0; atomic_store(&K.st, ST_IDLE); st = ST_IDLE;
        LOG("秘境：放弃第 %d 层", K.tier); atomic_store(&M.menu, MENU_SELECT);
        pet_hp_pump(G, player, t);
        return; }
    if (atomic_exchange(&M.req_cont, 0)) {



        int tl = 0, tx = 0, ty = 0; if (st == ST_RUN && L.last_lv) { tl = L.last_lv; tx = L.last_x; ty = L.last_y; } else if (st == ST_DONE && L.nd_lv) { tl = L.nd_lv; tx = L.nd_x; ty = L.nd_y; }
        if (!tl && st == ST_RUN) tl = K.maps[K.cur];
        if (tl) { LOG("秘境：继续 → 关卡 %d（%d,%d）", tl, tx, ty); L.lv_prev = tl; warp(G, player, tl); if (tx) { L.tp_lv = tl; L.tp_x = tx; L.tp_y = ty; } return; } }


    if (L.tp_lv && lv == L.tp_lv && atomic_load(&g_cli_lv) == lv) { uint64_t lvp = level_of(player); int32_t pos[2] = { L.tp_x, L.tp_y }; uint64_t rs = room_load(G, lvp, pos[0], pos[1]), rr = rs ? free_spot(G, rs, pos) : 0;






        int old = getenv("DM_RIFT_TEST_TP_NOROOM") != NULL, okk = !rr ? 0 : old ? ((int (*)(uint64_t, uint64_t, uint64_t, int, int, int, int))(B + GA_UNIT_TELEPORT))(G, player, 0, pos[0], pos[1], 0, 0) : dm_teleport(B, G, player, rr, pos[0], pos[1]);
        if (okk) LOG("秘境：继续 落到 %d,%d%s", pos[0], pos[1], old ? "（测试：没给目标房间）" : "");
        else LOG("【红】秘境：继续 没能落到离开时的位置 %d,%d（房间 %s），留在落点", L.tp_x, L.tp_y, !rs ? "载入不了" : !rr ? "有、附近找不到空地" : "有、有空地、传送不成");
        L.tp_lv = 0; }
    if ((st == ST_RUN || st == ST_DONE) && in_rift_map(lv)) { int x, y; if (unit_xy(player, &x, &y)) { L.last_lv = lv; L.last_x = x; L.last_y = y; } }
    if (L.gift_due && st == ST_RUN && in_rift_map(lv) && !R.gift) { L.gift_due = 0; R.gift = 1; rec_save(); give_item2(G, player, "jew ", 7, 99, 392 + rnd(8), 0); say("dmRiftSayGift"); }
    { static uint32_t at[16]; static int n = -1; if (n < 0) { n = 0; const char *e = getenv("DM_RIFT_TEST_OPEN"); char b[160]; if (e) { strlcpy(b, e, sizeof b); for (char *sv, *k = strtok_r(b, ",", &sv); k && n < 16; k = strtok_r(NULL, ",", &sv)) at[n++] = (uint32_t)atoi(k); } }
      for (int i = 0; i < n; i++) if (at[i] && t >= at[i] && lv == LV_TOWN) { at[i] = 0; atomic_store(&M.menu, town_menu(st)); atomic_fetch_add(&g_menu_seq, 1); LOG("秘境：测试 DM_RIFT_TEST_OPEN 拍 %u 当作点了城里的门 → %s", t, atomic_load(&M.menu) == MENU_CHOICE ? "继续 / 放弃" : "选层页"); } }
    if (L.pend) { uint64_t du = find_unit(G, 2, L.pend); int px, py, dx, dy;
        if (!du || t - L.pend_at > 250) L.pend = 0;
        else if (unit_xy(player, &px, &py) && unit_xy(du, &dx, &dy) && abs(px - dx) <= 4 && abs(py - dy) <= 4) { L.pend = 0;
            int is_next = du && *(uint32_t *)(uintptr_t)(du + 8) == L.next_door;
            if (du && *(uint32_t *)(uintptr_t)(du + 8) == L.town_door) { LOG("秘境：点了守护者处的回城门 → 回哈洛加斯"); L.lv_prev = LV_TOWN; warp(G, player, LV_TOWN); return; }
            if (is_next) { atomic_store(&M.menu, MENU_NEXT); atomic_fetch_add(&g_menu_seq, 1); LOG("秘境：点了守护者处的门 → 确认页（要进第 %d 层；门与地上的东西先不动）", next_tier()); return; }


            atomic_store(&M.menu, town_menu(st)); atomic_fetch_add(&g_menu_seq, 1); LOG("秘境：点城里的门 → %s", atomic_load(&M.menu) == MENU_CHOICE ? "继续 / 放弃" : "选层页"); return; } }




    if (st == ST_DONE && lv == L.nd_lv && (L.next_door || L.town_door)) for (int b = 0; b < 128; b++) for (uint64_t u = rd(G + 0x2230 + 2 * 0x400 + 8 * b); u; u = rd(u + 0x158)) {
        uint32_t cls = *(uint32_t *)(uintptr_t)(u + 4), id = *(uint32_t *)(uintptr_t)(u + 8); int x, y; if ((cls != RIFT_OBJ && cls != TOWN_OBJ) || !unit_xy(u, &x, &y) || abs(x - L.nd_x) > 32 || abs(y - L.nd_y) > 32) continue;
        uint32_t *mine = cls == RIFT_OBJ ? &L.next_door : &L.town_door; if (!*mine) continue;
        if (*mine != id) { LOG("秘境：守护者处的%s门单位号 %u → %u（房间重建过）", cls == RIFT_OBJ ? "下一层" : "回城", *mine, id); *mine = id; }
        if (*(uint32_t *)(uintptr_t)(u + 0xc) != 2) { LOG("秘境：守护者处的%s门模式 %u → 2（重新开着）", cls == RIFT_OBJ ? "下一层" : "回城", *(uint32_t *)(uintptr_t)(u + 0xc)); ((void (*)(uint64_t, int))(B + GA_UNIT_SET_MODE))(u, 2); } }
    if ((st == ST_RUN || st == ST_DONE) && in_rift_map(lv)) level_box(G, lv, level_of(player));
    if (st == ST_DONE) { region_set(G, 1); if (in_rift_map(lv)) presets_skip(level_of(player)); }








    if (st == ST_DONE) { saved_purge(G); if (in_rift_map(lv)) { static uint32_t n_tot, chk_at; static int exp0, it0; int nk = 0;
        for (int b = 0; b < 128; b++) for (uint64_t u = rd(G + 0x2230 + 1 * 0x400 + 8 * b), nx; u; u = nx) { nx = rd(u + 0x158); uint32_t m = *(uint32_t *)(uintptr_t)(u + 0xc), fl = *(uint32_t *)(uintptr_t)(u + 0x124);
            if (m == 0 || m == 12 || (fl & 0x80000000u) || any_level(u) != lv || unit_align(u) != 0) continue;
            if (!n_tot && !nk) { exp0 = gs(player, 13); it0 = ground_items(G); }
            g_swept[*(uint32_t *)(uintptr_t)(u + 8) & 1023] = *(uint32_t *)(uintptr_t)(u + 8); *(uint32_t *)(uintptr_t)(u + 0x124) = fl | 0x20000; ((void (*)(uint64_t, uint64_t, uint64_t, int))(B + GA_UNIT_KILL))(G, u, 0, 1); nk++; }
        if (nk) { if (!n_tot) chk_at = t + 75; n_tot += nk; LOG("秘境：拍 %u 清掉在场的敌对怪 %d 只（本局累计 %u）", t, nk, n_tot); }
        if (chk_at && t >= chk_at) { chk_at = 0; LOG("秘境：清怪核对（首批后 75 拍）经验 %d → %d，地上物品 %d → %d 件", exp0, gs(player, 13), it0, ground_items(G)); } } }
    if (st != ST_RUN) return;
    { uint32_t pm = *(uint32_t *)(uintptr_t)(player + 0xc); int d = pm == 0 || pm == 17;
      if (d && !L.dead) { R.deaths++; rec_save(); LOG("秘境：第 %d 层里死亡（累计 %d 次）", K.tier, R.deaths); } L.dead = d; }







    if (lv != L.lv_prev) { int was = L.lv_prev; L.lv_prev = lv; L.redir = 0;
        if (in_rift_map(lv)) { for (int i = 0; i < K.nmaps; i++) if (K.maps[i] == lv) K.cur = i; }
        else if (!dm_hud_area_is_town(lv) && in_rift_map(was)) { L.redir = next_map_index() + 1; LOG("秘境：从关卡 %d 走过边界到了 %d（没有经过原版换关）", was, lv); } }
    if (L.redir) { if (atomic_load(&g_cli_lv) != lv) return;
        int nx = L.redir - 1; L.redir = 0; LOG("秘境：拍 %u 送进第 %d 张图 %d", t, nx + 1, K.maps[nx]); K.cur = nx; L.lv_prev = K.maps[nx]; warp(G, player, K.maps[nx]); return; }



    static int lowhp = -1; if (lowhp < 0) lowhp = getenv("DM_RIFT_TEST_LOWHP") != NULL;
    if (K.tier >= 9 || lowhp) { static uint32_t r_; uint32_t w = atomic_load(&g_newq_w); if (w - r_ > NQ) r_ = w - NQ; int lt = target_level(player);
        for (; r_ != w; r_++) { uint32_t id = atomic_load(&g_newq[r_ % NQ]); uint64_t u = find_unit(G, 1, id); if (u && !is_guard(id) && hi_on("summon", 75) && boss_summon(*(uint32_t *)(uintptr_t)(u + 4))) { static int n, sq = -1; int cur = atomic_load(&g_run_seq); if (cur != sq) { sq = cur; n = 0; }
            if (n++ < 4) LOG("秘境：首领召唤物 id %u（类号 %u）不按层改，保持原版：等级 %d 生命 %d", id, *(uint32_t *)(uintptr_t)(u + 4), gs(u, 12), gs(u, 7) >> 8); hs_spawn(id, gs(u, 7) >> 8); continue; }
          if (u && !is_guard(id)) { uint64_t md = rd(u + 0x10); int su = md && (*(uint8_t *)(uintptr_t)(md + 0x1a) & 0x02) && hi_on("su", 75), sm = !su && su_minion(id);
              if (K.tier >= 9) { if (su) { g_hp_mul = SU_HP_X * 1000; g_hp_soft = SU_HP_SOFT; } else if (sm) g_hp_mul = SU_MINION_X1000; scale_monster(G, u, lt, dm_rift_tier_level(9, gs(player, 12))); g_hp_mul = 1000; g_hp_soft = 0; } if (su) { static int n; if (n++ < 12) LOG("秘境：特殊精英本体 id %u（类号 %u，超级独特号 %d）生命另乘 %d：等级 %d 生命 %d", id, *(uint32_t *)(uintptr_t)(u + 4), *(int16_t *)(uintptr_t)(md + 0x2a), SU_HP_X, gs(u, 12), gs(u, 7) >> 8); } if (lowhp) ss(u, 6, 256); dmg_probe_add(id, t); hs_spawn(id, gs(u, 7) >> 8); } } }
    dmg_probe_pump(G, t); hs_tick(G, player, t, lv);


    { static int on = -1; if (on < 0) on = getenv("DM_RIFT_TEST_PETGOD") != NULL; if (on && t % 10 == 0 && in_rift_map(lv)) for (int b = 0; b < 128; b++) for (uint64_t u = rd(G + 0x2230 + 1 * 0x400 + 8 * b); u; u = rd(u + 0x158)) {
        uint32_t m = *(uint32_t *)(uintptr_t)(u + 0xc); if (m != 0 && m != 12 && unit_align(u)) ss(u, 6, gs(u, 7)); } }


    { static int il = -1; if (il < 0) il = getenv("DM_RIFT_TEST_ILVL") != NULL;
      if (il && t % 250 == 0) { int n = 0, mn = 999, mx = 0; for (int bb = 0; bb < 128; bb++) for (uint64_t it = rd(G + 0x2e30 + 8 * bb); it; it = rd(it + 0x158)) { uint64_t d = rd(it + 0x10); if (!d || *(uint32_t *)(uintptr_t)(it + 0xc) != 3) continue; int v = *(int32_t *)(uintptr_t)(d + 0x38); n++; if (v < mn) mn = v; if (v > mx) mx = v; }
          LOG("秘境：测试读数 拍 %u 地上物品 %d 件，物品等级 %d–%d", t, n, n ? mn : 0, mx); } }







    { static int on = -1, nxt; static uint64_t done[2048]; static int nd, lvd; static uint32_t skip[256], last_id; static int nskip, same;                                                                                                                                                                                                                       if (on < 0) on = getenv("DM_RIFT_TEST_CLEAR") != NULL;
      if (on && t % 38 == 0 && in_rift_map(lv) && atomic_load(&K.st) == ST_RUN) { if (lvd != lv) { lvd = lv; nd = 0; nxt = 0; nskip = 0; same = 0; }
          int px, py; unit_xy(player, &px, &py); uint64_t best = 0; long bd = -1;
          for (int b = 0; b < 128; b++) for (uint64_t u = rd(G + 0x2230 + 1 * 0x400 + 8 * b); u; u = rd(u + 0x158)) { uint32_t m = *(uint32_t *)(uintptr_t)(u + 0xc); int x, y;
              if (m == 0 || m == 12 || unit_level(u) != lv || unit_align(u) || !unit_xy(u, &x, &y)) continue; { uint32_t id = *(uint32_t *)(uintptr_t)(u + 8); int sk = 0; for (int q = 0; q < nskip && !sk; q++) sk = skip[q] == id; if (sk) continue; } long d = (long)(x - px) * (x - px) + (long)(y - py) * (y - py); if (bd < 0 || d < bd) { bd = d; best = u; } }
          if (best) { uint32_t bid = *(uint32_t *)(uintptr_t)(best + 8); same = bid == last_id ? same + 1 : 0; last_id = bid; if (same >= 4 && nskip < 256) { skip[nskip++] = bid; same = 0; LOG("秘境：测试清图 怪 id %u（类号 %u）连着 4 次没打死，跳过", bid, *(uint32_t *)(uintptr_t)(best + 4)); best = 0; } }
          if (best) { int x, y; unit_xy(best, &x, &y); if (bd > 9) { int32_t pos[2] = { x + 2, y + 2 }; uint64_t rr = free_spot(G, unit_room(best), pos); if (rr) dm_teleport(B, G, player, rr, pos[0], pos[1]); } }
          else { uint64_t lvp = level_of(player); int n = 0; uint64_t r = dm_level_rooms(lvp);
              for (; r && n < 2000; r = rd(r + 0x48), n++) { int seen = 0; for (int q = 0; q < nd && !seen; q++) seen = done[q] == r; if (!seen) break; }
              if (r) { if (nd < 2048) done[nd++] = r; int32_t pos[2] = { *(int32_t *)(uintptr_t)(r + 0x60) * 5 + *(int32_t *)(uintptr_t)(r + 0x68) * 5 / 2, *(int32_t *)(uintptr_t)(r + 0x64) * 5 + *(int32_t *)(uintptr_t)(r + 0x6c) * 5 / 2 };
                  uint64_t rs = room_load(G, lvp, pos[0], pos[1]), rr = rs ? free_spot(G, rs, pos) : 0; if (rr) dm_teleport(B, G, player, rr, pos[0], pos[1]); static int ln; if (ln++ % 20 == 0) LOG("秘境：测试清图 本图眼下没有活的敌对怪 → 跳到第 %d 间房", nd); } } } }


    { static int tour = -1, k; if (tour < 0) tour = getenv("DM_RIFT_TEST_TOUR") != NULL;
      if (tour && t % 25 == 0 && in_rift_map(lv) && atomic_load(&K.st) == ST_RUN) { uint64_t lvp = level_of(player), r = dm_level_rooms(lvp); int i = 0, nr = 0;
          for (uint64_t q = r; q && nr < 2000; q = rd(q + 0x48)) nr++;
          for (; r && i < k % (nr ? nr : 1); r = rd(r + 0x48)) i++;
          if (r) { int32_t pos[2] = { *(int32_t *)(uintptr_t)(r + 0x60) * 5 + *(int32_t *)(uintptr_t)(r + 0x68) * 5 / 2, *(int32_t *)(uintptr_t)(r + 0x64) * 5 + *(int32_t *)(uintptr_t)(r + 0x6c) * 5 / 2 };
              uint64_t rs = room_load(G, lvp, pos[0], pos[1]), rr = rs ? free_spot(G, rs, pos) : 0; int ok = rr && dm_teleport(B, G, player, rr, pos[0], pos[1]);
              static int ln; if (ln++ < 3 || k == nr - 1) LOG("秘境：测试逐间跳 第 %d / %d 间 → %d,%d %s", k + 1, nr, pos[0], pos[1], ok ? "成" : "不成"); }
          { static uint32_t seen[8192]; static int scls[8192], sfl[8192], ns;
            for (int b = 0; b < 128; b++) for (uint64_t v = rd(G + 0x2230 + 1 * 0x400 + 8 * b); v; v = rd(v + 0x158)) { uint32_t id = *(uint32_t *)(uintptr_t)(v + 8); if (any_level(v) != lv) continue; int q = 0; for (; q < ns && seen[q] != id; q++) {}
                if (q == ns && ns < 8192) { uint64_t md = rd(v + 0x10); uint32_t mo = *(uint32_t *)(uintptr_t)(v + 0xc); seen[ns] = id; scls[ns] = *(int32_t *)(uintptr_t)(v + 4); sfl[ns] = (md ? *(uint8_t *)(uintptr_t)(md + 0x1a) : 0) | (mo == 0 || mo == 12 ? 0x100 : 0); ns++; } }
            if (k == nr + 5) { static int cl[1024], cn[1024], cf[1024]; int nc = 0;
                for (int q = 0; q < ns; q++) { int j2 = 0; for (; j2 < nc && cl[j2] != scls[q]; j2++) {} if (j2 == nc && nc < 1024) { cl[nc] = scls[q]; cn[nc] = cf[nc] = 0; nc++; } if (j2 < 1024) { cn[j2]++; cf[j2] |= sfl[q]; } }
                LOG("[逐间跳读数] 关卡 %d 出现过的怪 %d 只、%d 种", lv, ns, nc); for (int j2 = 0; j2 < nc; j2++) LOG("[逐间跳读数] 类号 %d ×%d 标志 0x%03x（0x100 = 有一只首次见到时已是死的）", cl[j2], cn[j2], cf[j2]); } }
          k++; } }


    { static int pull = -1; if (pull < 0) pull = getenv("DM_RIFT_TEST_PULL") != NULL;
      if (pull && t % 38 == 0 && in_rift_map(lv) && ((K.guard && !atomic_load(&K.guard_dead)) || K.inv_st[0] == 2 || K.inv_st[1] == 2)) { uint64_t g = 0; for (int i = 0; i < 2 && !g; i++) { uint64_t gu = K.inv_st[i] == 2 ? find_unit(G, 1, K.inv_id[i]) : 0; uint32_t gm = gu ? *(uint32_t *)(uintptr_t)(gu + 0xc) : 0; if (gu && gm != 0 && gm != 12) g = gu; }
          for (int i = 0; i < 3 && !g; i++) { uint64_t gu = K.guards[i] ? find_unit(G, 1, K.guards[i]) : 0; uint32_t gm = gu ? *(uint32_t *)(uintptr_t)(gu + 0xc) : 0; if (gu && gm != 0 && gm != 12) g = gu; } int x, y; if (g && unit_xy(g, &x, &y)) dm_teleport(B, G, player, unit_room(g), x + 2, y + 2); }
      else if (pull && t % 38 == 0 && in_rift_map(lv)) { uint32_t w = atomic_load(&g_newq_w); int px, py, bd = 1 << 30, bx = 0, by = 0; unit_xy(player, &px, &py);
          static uint32_t bl[64], last; static int nbl, rep; uint32_t bid = 0;
          for (uint32_t k = 0; k < NQ && k < w; k++) { uint32_t id = atomic_load(&g_newq[(w - 1 - k) % NQ]); uint64_t u = find_unit(G, 1, id); if (!u || unit_level(u) != lv) continue; uint32_t m = *(uint32_t *)(uintptr_t)(u + 0xc); if (m == 0 || m == 12) continue;
              int x, y, skip = 0; for (int q = 0; q < nbl && q < 64; q++) skip |= bl[q] == id; if (skip || !unit_xy(u, &x, &y)) continue; int d = abs(x - px) + abs(y - py); if (d < bd) { bd = d; bx = x; by = y; bid = id; } }
          if (bid && bid == last) { if (++rep >= 4) { bl[nbl++ % 64] = bid; rep = 0; LOG("秘境：测试拉人 单位 id %u 连着 4 次是最近的还没死，不再找它", bid); } } else { last = bid; rep = 0; }
          if (bd < (1 << 30) && bd > 3) { uint64_t lvp = level_of(player); int32_t pos[2] = { bx + 1, by + 1 }; uint64_t rs = room_at(lvp, pos[0], pos[1]), rr = rs ? free_spot(G, rs, pos) : 0;
              int ok = dm_teleport(B, G, player, rr, pos[0], pos[1]); static int ln; if (ln++ < 5 || ln % 20 == 0) LOG("秘境：测试拉人 → %d,%d（最近的怪 id %u，%d 格）%s", pos[0], pos[1], bid, bd, ok ? "成" : "不成"); }
          else { static int ln2; if (ln2++ % 20 == 0) LOG("秘境：测试拉人 这一次没拉（最近的怪 id %u，%d 格；新怪队列写到 %u）", bid, bd == 1 << 30 ? -1 : bd, w); } } }


    { uint64_t now = guest_ns();
      if (!atomic_load(&K.timeout) && now - K.t0_ns > (uint64_t)RIFT_SECS * 1000000000ull) { atomic_store(&K.timeout, 1); say("dmRiftSayTimeout"); } }


    inv_gone_pump(G, t);
    if (in_rift_map(lv) && g_entry_lv != lv) { int ex, ey; if (unit_xy(player, &ex, &ey)) { g_entry_lv = lv; g_entry_x = ex; g_entry_y = ey; } } else if (!in_rift_map(lv)) g_entry_lv = 0;


    if (t % 25 == 0 && hi_on("su", 75) && atomic_load(&K.st) == ST_RUN && g_entry_lv == lv) { static int hx[2], hy[2], hk; int px, py, live = 0; unit_xy(player, &px, &py);
        int dx = hx[hk] ? px - hx[hk] : 0, dy = hx[hk] ? py - hy[hk] : 0; hx[hk] = px; hy[hk] = py; hk ^= 1;
        for (int i = 0; i < 3; i++) { if (!K.su_live[i]) continue; uint64_t u = find_unit(G, 1, K.su_live[i]); uint32_t m = u ? *(uint32_t *)(uintptr_t)(u + 0xc) : 1;
            if (u && (m == 0 || m == 12)) K.su_live[i] = 0; else if (!u) { if (t - K.su_seen[i] > 750) K.su_live[i] = 0; }
            else { K.su_seen[i] = t; int x, y; unit_xy(u, &x, &y);
                if (abs(x - px) + abs(y - py) <= 80) K.su_near[i] = t;
                else if (t - K.su_near[i] >= 125) { static const int cx[8] = { 70, 50, 0, -50, -70, -50, 0, 50 }, cy[8] = { 0, 50, 70, 50, 0, -50, -70, -50 }; int d0 = rnd(8), best = 0, ok = 0; int32_t pos[2];
                    if (dx || dy) for (int d = 0; d < 8; d++) { int v = dx * cx[d] + dy * cy[d]; if (v > best) { best = v; d0 = d; } }
                    for (int k = 0; k < 8 && !ok; k++) { int d = (d0 + (k & 1 ? (k + 1) / 2 : 8 - k / 2)) % 8; pos[0] = px + cx[d]; pos[1] = py + cy[d]; uint64_t rs = room_load(G, level_of(player), pos[0], pos[1]), rr = rs ? free_spot(G, rs, pos) : 0;
                        if (rr && abs(pos[0] - px) + abs(pos[1] - py) >= 60) ok = dm_teleport(B, G, u, rr, pos[0], pos[1]); }
                    K.su_near[i] = t; static int n; if (n++ < 8) LOG("秘境：拍 %u 特殊精英（id %u）离玩家 %d 格满 5 秒，%s", t, K.su_live[i], abs(x - px) + abs(y - py), ok ? "挪到玩家前方" : "周围没有空地，下次再挪"); } }
            live += K.su_live[i] != 0; }
        if (!K.guard && live < su_cap(K.tier) && K.su_cnt < su_quota(K.tier) && g_near_now < SU_NEAR && t - K.su_last >= 75) {
            static const int sx[8] = { 70, 50, 0, -50, -70, -50, 0, 50 }, sy[8] = { 0, 50, 70, 50, 0, -50, -70, -50 }; int d0 = rnd(8), best = 0, np = (int)sizeof k_su_pool, baal = 0, row;
            if (dx || dy) for (int d = 0; d < 8; d++) { int v = dx * sx[d] + dy * sy[d]; if (v > best) { best = v; d0 = d; } }
            for (int w = 0; w < K.nwave; w++) for (int q = 0; q < K.wn[w]; q++) baal |= K.wcls[w][q] == 544; for (int q = 0; q < K.inv_n; q++) baal |= K.inv_cls[q] == 544;
            do row = K.su_order[K.su_next++ % np]; while (baal && (row == 24 || row == 36 || row == 38));
            uint64_t u = 0; int32_t pos[2] = { 0, 0 }; int f_room = 0, f_near = 0, f_fn = 0;
            for (int k = 0; k < 8 && !u; k++) { int d = (d0 + (k & 1 ? (k + 1) / 2 : 8 - k / 2)) % 8; pos[0] = px + sx[d]; pos[1] = py + sy[d]; uint64_t rs = room_load(G, level_of(player), pos[0], pos[1]), rr = rs ? free_spot(G, rs, pos) : 0;
                if (!rr) { f_room++; continue; } if (abs(pos[0] - px) + abs(pos[1] - py) < 45 || abs(pos[0] - g_entry_x) + abs(pos[1] - g_entry_y) < 40) { f_near++; continue; } f_fn++;
                u = su_spawn(G, rr, pos[0], pos[1], row); }
            static int fails; if (u) fails = 0; else if (++fails < 10) K.su_next--; else fails = 0;
            if (!u) { K.su_last = t - 50; static int n; if (n++ % 20 == 0) LOG("秘境：拍 %u 特殊精英（行 %d）这一秒没放成（第 %d 次）：八个方向里没有房间 / 空地的 %d 个、离人或离入口太近的 %d 个、原版函数没刷出来的 %d 个", t, row, n, f_room, f_near, f_fn); }
            else { uint32_t uid = *(uint32_t *)(uintptr_t)(u + 8); int ux = pos[0], uy = pos[1], nm = 0;
                for (int b = 0; b < 128; b++) for (uint64_t v = rd(G + 0x2230 + 1 * 0x400 + 8 * b); v; v = rd(v + 0x158)) { uint32_t vid = *(uint32_t *)(uintptr_t)(v + 8); uint64_t md = rd(v + 0x10); int x, y;
                    if (vid > uid && md && (*(uint8_t *)(uintptr_t)(md + 0x1a) & 0x10) && unit_xy(v, &x, &y) && abs(x - ux) + abs(y - uy) <= 15) { g_su_min[g_su_min_w++ % 64] = vid; nm++; } }
                for (int i = 0; i < 3; i++) if (!K.su_live[i]) { K.su_live[i] = uid; K.su_seen[i] = K.su_near[i] = t; break; } K.su_cnt++; K.su_last = t;
                LOG("秘境：拍 %u 特殊精英第 %d 组刷出：行 %d 类号 %u id %u 在 %d,%d（离人 %d 格、离入口 %d 格；此前在场 %d 组、40 格内敌对 %d 只；随从 %d 只）", t, K.su_cnt, row, *(uint32_t *)(uintptr_t)(u + 4), *(uint32_t *)(uintptr_t)(u + 8), pos[0], pos[1],
                    abs(pos[0] - px) + abs(pos[1] - py), abs(pos[0] - g_entry_x) + abs(pos[1] - g_entry_y), live, g_near_now, nm); } } }


    { static int at = -1, hit, gx, gy, cls; static uint32_t hit_t; if (at < 0) { const char *e = getenv("DM_RIFT_TEST_GONE"); at = e ? atoi(e) : 0; }


      if (getenv("DM_RIFT_TEST_GONEKILL")) for (int i = 0; i < 2; i++) { static uint32_t t0[2], did[2]; uint64_t u = g_inv_gone[i] ? find_unit(G, 1, g_inv_gone[i]) : 0; if (!u) continue; if (did[i] != g_inv_gone[i]) { did[i] = g_inv_gone[i]; t0[i] = t; }
          uint32_t m = *(uint32_t *)(uintptr_t)(u + 0xc); int x, y; unit_xy(u, &x, &y);
          if (t - t0[i] == 10) { LOG("[退场读数] 拍 %u 把人传到退场的闯入首领跟前（它在 %d,%d，进度 %d）", t, x, y, atomic_load(&K.score)); dm_teleport(B, G, player, unit_room(u), x + 2, y + 2); }
          if (t - t0[i] >= 10 && m != 0 && m != 12) ss(u, 6, 256);
          if ((m == 0 || m == 12) && t0[i] != 1) { t0[i] = 1; LOG("[退场读数] 拍 %u 退场的闯入首领被打死了（模式 %u），进度 %d", t, m, atomic_load(&K.score)); } }
      if (at > 0) { for (int i = 0; i < 2; i++) { uint64_t u = g_inv_gone[i] ? find_unit(G, 1, g_inv_gone[i]) : 0; if (g_inv_gone_xy[i][0]) { gx = g_inv_gone_xy[i][0]; gy = g_inv_gone_xy[i][1]; cls = K.inv_cls[i]; } if (!u || t % 25) continue;
              int x, y, px, py; unit_xy(u, &x, &y); unit_xy(player, &px, &py); LOG("[退场读数] 拍 %u 第 %d 位：位置 %d,%d 模式 %u 生命 %d / %d 离人 %d 格", t, i + 1, x, y, *(uint32_t *)(uintptr_t)(u + 0xc), gs(u, 6) >> 8, gs(u, 7) >> 8, abs(x - px) + abs(y - py)); }
          if (gx && t >= (uint32_t)at && !hit) { int32_t pos[2] = { gx, gy }; uint64_t rs = room_load(G, level_of(player), gx, gy), rr = rs ? free_spot(G, rs, pos) : 0; hit = 1; hit_t = t; LOG("[退场读数] 拍 %u 把人传到退场落点 %d,%d（%s）", t, gx, gy, rr && dm_teleport(B, G, player, rr, pos[0], pos[1]) ? "成" : "没成"); }
          if (hit == 1 && t - hit_t >= 150) { hit = 2; int n = 0, px, py; unit_xy(player, &px, &py); for (int b = 0; b < 128; b++) for (uint64_t u = rd(G + 0x2230 + 1 * 0x400 + 8 * b); u; u = rd(u + 0x158)) { int x, y; if ((int)*(uint32_t *)(uintptr_t)(u + 4) == cls && unit_xy(u, &x, &y) && abs(x - px) + abs(y - py) < 80) n++; }
              LOG("[退场读数] 拍 %u 人已在退场落点 150 拍：80 格内类号 %d 的单位 %d 只（人在 %d,%d）", t, cls, n, px, py); } } }






    { static int at = -1, rows[66], nr, idx, ph, row, minions; static uint32_t t0, hi, cls;
      if (at < 0) { const char *e = getenv("DM_RIFT_TEST_SUSEQ"); at = e ? atoi(e) : 0; const char *c = e ? strchr(e, ':') : NULL; char bf[400]; strlcpy(bf, c ? c + 1 : "", sizeof bf); for (char *sv, *k = strtok_r(bf, ",", &sv); k && nr < 66; k = strtok_r(NULL, ",", &sv)) rows[nr++] = atoi(k); }
      if (at > 0 && idx < nr && in_rift_map(lv) && atomic_load(&K.st) == ST_RUN) {
          if (ph == 0 && t >= (uint32_t)at && t >= t0 + 10) { row = rows[idx]; int x, y; unit_xy(player, &x, &y); int32_t pos[2] = { 0, 0 }; uint64_t rr = 0, u = 0; hi = 0; minions = 0;
              for (int b = 0; b < 128; b++) for (uint64_t it = rd(G + 0x2e30 + 8 * b); it; it = rd(it + 0x158)) if (*(uint32_t *)(uintptr_t)(it + 8) > hi) hi = *(uint32_t *)(uintptr_t)(it + 8);
              for (int d = 0; d < 8 && !u; d++) { static const int ox[8] = { 12, 9, 0, -9, -12, -9, 0, 9 }, oy[8] = { 0, 9, 12, 9, 0, -9, -12, -9 }; pos[0] = x + ox[d]; pos[1] = y + oy[d]; rr = free_spot(G, unit_room(player), pos);
                  u = rr ? su_spawn(G, rr, pos[0], pos[1], row) : 0; }
              if (!u) { LOG("[验收轮] 拍 %u 行 %d 刷不出（房间 %s，函数返回 0）", t, row, rr ? "有" : "无"); idx++; t0 = t; }
              else { g_suseq_id = *(uint32_t *)(uintptr_t)(u + 8); cls = *(uint32_t *)(uintptr_t)(u + 4); t0 = t; ph = 1;
                  if (getenv("DM_RIFT_TEST_SUPUSH")) { int plv = atoi(getenv("DM_RIFT_TEST_SUPUSH"));                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      int px, py, bd = 1 << 30; uint64_t br = 0; unit_xy(player, &px, &py); uint64_t dr = rd(level_of(player) + 0x1c8); int n2 = 0;
                      for (uint64_t l2 = dr ? rd(dr + 0x868) : 0; l2 && n2 < 200; l2 = rd(l2 + 0x1b8), n2++) { if (*(int32_t *)(uintptr_t)(l2 + 0x1f8) != plv) continue; int m3 = 0;
                          for (uint64_t r3 = dm_level_rooms(l2); r3 && m3 < 2000; r3 = rd(r3 + 0x48), m3++) { int rx = *(int32_t *)(uintptr_t)(r3 + 0x60) * 5, ry = *(int32_t *)(uintptr_t)(r3 + 0x64) * 5, rw = *(int32_t *)(uintptr_t)(r3 + 0x68) * 5, rh = *(int32_t *)(uintptr_t)(r3 + 0x6c) * 5;
                              int cx = px < rx + 3 ? rx + 3 : px > rx + rw - 4 ? rx + rw - 4 : px, cy = py < ry + 3 ? ry + 3 : py > ry + rh - 4 ? ry + rh - 4 : py, d3 = abs(cx - px) + abs(cy - py); if (d3 < bd) { bd = d3; br = r3; } } }
                      if (br) { int rx = *(int32_t *)(uintptr_t)(br + 0x60) * 5, ry = *(int32_t *)(uintptr_t)(br + 0x64) * 5, rw = *(int32_t *)(uintptr_t)(br + 0x68) * 5, rh = *(int32_t *)(uintptr_t)(br + 0x6c) * 5;
                          int32_t pos[2] = { px < rx + 3 ? rx + 3 : px > rx + rw - 4 ? rx + rw - 4 : px, py < ry + 3 ? ry + 3 : py > ry + rh - 4 ? ry + rh - 4 : py };
                          if (!rd(br + 0x58)) ((void (*)(int, uint64_t))(B + GA_ROOM_INIT))(*(uint8_t *)(uintptr_t)(G + 0x106), br); uint64_t rr = rd(br + 0x58) ? free_spot(G, rd(br + 0x58), pos) : 0; int ok = rr && dm_teleport(B, G, u, rr, pos[0], pos[1]);
                          LOG("[验收轮] 本体 id %u 推过界 → 关卡 %d 的 %d,%d（人 %d,%d）%s；落地关卡 %d", g_suseq_id, plv, pos[0], pos[1], px, py, ok ? "成" : "不成", any_level(u)); } } } }
          else if (ph == 1) { uint64_t u = find_unit(G, 1, g_suseq_id); uint32_t m = u ? *(uint32_t *)(uintptr_t)(u + 0xc) : 0; int dead = !u || m == 0 || m == 12, x, y;
              if (t - t0 == 20) { int px, py; unit_xy(player, &px, &py); for (int b = 0; b < 128; b++) for (uint64_t v = rd(G + 0x2230 + 1 * 0x400 + 8 * b); v; v = rd(v + 0x158)) { uint64_t md = rd(v + 0x10); if (md && (*(uint8_t *)(uintptr_t)(md + 0x1a) & 0x10) && unit_xy(v, &x, &y) && abs(x - px) + abs(y - py) <= 40) minions++; } }
              if (!dead && t % 25 == 0 && unit_xy(u, &x, &y)) dm_teleport(B, G, player, unit_room(u), x + 2, y + 2);
              if (dead || t - t0 > 600) { LOG("[验收轮] 拍 %u 行 %d 类号 %u id %u %s（刷出后 %u 拍），40 格内带随从标志的 %d 只", t, row, cls, g_suseq_id, dead ? "已死" : "【没打死】", t - t0, minions); ph = 2; t0 = t; } }
          else if (ph == 2 && t - t0 >= 40) { char ln[700]; int o = 0, n = 0; ln[0] = 0;
              for (int b = 0; b < 128; b++) for (uint64_t it = rd(G + 0x2e30 + 8 * b); it; it = rd(it + 0x158)) { uint64_t d = rd(it + 0x10); if (!d || *(uint32_t *)(uintptr_t)(it + 0xc) != 3 || *(uint32_t *)(uintptr_t)(it + 8) <= hi) continue; n++; if (o < (int)sizeof ln - 16) o += snprintf(ln + o, sizeof ln - o, " %u:%d", *(uint32_t *)(uintptr_t)(it + 4), *(int32_t *)(uintptr_t)d); }
              LOG("[验收轮] 拍 %u 行 %d 之后新掉在地上的物品 %d 件（类号:品质）：%s", t, row, n, ln); g_suseq_id = 0; ph = 0; t0 = t; if (++idx == nr) LOG("[验收轮] 拍 %u 全部 %d 行走完", t, nr); } } }


    { static int at = -1, sc; if (at < 0) { const char *e = getenv("DM_RIFT_TEST_SCOREAT"); at = e ? atoi(e) : 0; const char *c = e ? strchr(e, ':') : NULL; sc = c ? atoi(c + 1) : 0; }
      if (at > 0 && t >= (uint32_t)at && atomic_load(&K.st) == ST_RUN) { at = 0; atomic_store(&K.score, sc); LOG("秘境：测试 DM_RIFT_TEST_SCOREAT 拍 %u 把进度分数设成 %d", t, sc); } }




    if (K.inv_n && atomic_load(&K.st) == ST_RUN && in_rift_map(lv)) { int nd = atomic_load(&K.need), busy = 0; for (int i = 0; i < K.inv_n; i++) busy |= K.inv_st[i] == 1 || K.inv_st[i] == 2;
        for (int i = 0; i < K.inv_n; i++) {
            if (K.inv_st[i] == 0 && !busy && !K.guard && nd > 0 && atomic_load(&K.score) * 100 >= (long long)nd * K.inv_pct[i]) { K.inv_st[i] = 1; K.inv_t[i] = t; busy = 1; say("dmRiftSayInvader"); LOG("秘境：拍 %u 闯入首领预警（第 %d 位，类号 %d，进度 %d%%）", t, i + 1, K.inv_cls[i], K.inv_pct[i]); }
            else if (K.inv_st[i] == 1 && K.guard) { K.inv_st[i] = 4; LOG("秘境：守护者已现身，第 %d 位闯入首领作罢", i + 1); }
            else if (K.inv_st[i] == 1 && t - K.inv_t[i] >= 75 && (t - K.inv_t[i]) % 25 == 0) { static const int dx[8] = { 36, 26, 0, -26, -36, -26, 0, 26 }, dy[8] = { 0, 26, 36, 26, 0, -26, -36, -26 }; int x, y; unit_xy(player, &x, &y); uint64_t u = 0; int32_t pos[2] = { 0, 0 };




                static struct ds_grid ig; ds_fill(G, player, x, y, &ig); int d0 = rnd(8), rx = 0, ry = 0, lo = t - K.inv_t[i] >= 75 + 125 ? 12 : 24, kk = ds_pick_ring(&ig, dx[d0], dy[d0], lo, 120, &rx, &ry);
                if (kk < 0) { static int nl; if (nl++ < 10) LOG("秘境：闯入首领这一秒没有合格落点（人 %d,%d 窗口内可达 %d 格，离人至少 %d 格、120 步内）", x, y, ig.reach, lo); }
                if (kk >= 0) { pos[0] = x + rx; pos[1] = y + ry; uint64_t rr = room_load(G, level_of(player), pos[0], pos[1]);
                    if (rr) u = ((uint64_t (*)(uint64_t, uint64_t, int, int, int, int, int, int, uint64_t))(B + GA_UNIT_SPAWN))(G, rr, 1, K.inv_cls[i], pos[0], pos[1], 1, 0, 0x10);
                    LOG("秘境：闯入首领落点 人 %d,%d 窗口内可达 %d 格 → 取 %d,%d（离人 %d 格、走 %d 步、四周 %d 圈可达）%s", x, y, ig.reach, pos[0], pos[1], abs(rx) > abs(ry) ? abs(rx) : abs(ry), ds_dist(&ig, rx + DS_R, ry + DS_R), kk, u ? "" : "，原版没刷出来"); }
                if (!u) { if (t - K.inv_t[i] > 325) { K.inv_st[i] = 4; LOG("秘境：第 %d 位闯入首领 10 秒内没找到落点，作罢", i + 1); } continue; }
                K.inv_id[i] = *(uint32_t *)(uintptr_t)(u + 8); K.inv_st[i] = 2;
                if (K.tier >= 9) { g_hp_mul = 300; scale_monster(G, u, target_level(player) + 3, dm_rift_tier_level(9, gs(player, 12)) + 3); g_hp_mul = 1000; }
                LOG("秘境：拍 %u 闯入首领刷出（第 %d 位）类号 %d id %u 在 %d,%d（离人 %d 格）等级 %d 生命 %d", t, i + 1, K.inv_cls[i], K.inv_id[i], pos[0], pos[1], abs(pos[0] - x) + abs(pos[1] - y), gs(u, 12), gs(u, 7) >> 8); }
            else if (K.inv_st[i] == 2) { uint64_t u = find_unit(G, 1, K.inv_id[i]); uint32_t m = u ? *(uint32_t *)(uintptr_t)(u + 0xc) : 1;

                { static uint32_t chk[2]; if (getenv("DM_RIFT_TEST_INVREACH") && u && chk[i] != K.inv_id[i] && (m != 0 && m != 12)) { chk[i] = K.inv_id[i]; static struct ds_grid cg; int px, py, ux, uy; unit_xy(player, &px, &py); unit_xy(u, &ux, &uy); ds_fill(G, player, px, py, &cg);
                    int ix = ux - px + DS_R, iy = uy - py + DS_R, best = -1; for (int dy2 = -3; dy2 <= 3; dy2++) for (int dx2 = -3; dx2 <= 3; dx2++) { int d = ds_dist(&cg, ix + dx2, iy + dy2); if (d >= 0 && (best < 0 || d < best)) best = d; }
                    LOG("[闯入可达] 关卡 %d 第 %d 位 类号 %d 人 %d,%d 它 %d,%d 相距 %d 格 → %s（走 %d 步）", lv, i + 1, K.inv_cls[i], px, py, ux, uy, abs(ux - px) > abs(uy - py) ? abs(ux - px) : abs(uy - py), best >= 0 ? "可达" : "【不可达】", best); } }
                if (u && (m == 0 || m == 12)) { K.inv_st[i] = 3; int add = nd * 12 / (100 + 12 * K.inv_n); if (add < 1) add = 1; int sc = atomic_fetch_add(&K.score, add) + add;
                    LOG("秘境：拍 %u 闯入首领死亡（第 %d 位，类号 %d）→ 进度 +%d，现 %d / %d", t, i + 1, K.inv_cls[i], add, sc, nd); } } } }
    if (!K.guard && atomic_load(&K.score) >= atomic_load(&K.need) && in_rift_map(lv)) {




        for (int i = 0; i < K.inv_n; i++) { if (K.inv_st[i] == 1) { K.inv_st[i] = 4; continue; } if (K.inv_st[i] != 2) continue;
            static const int fx[8] = { 150, 106, 0, -106, -150, -106, 0, 106 }, fy[8] = { 0, 106, 150, 106, 0, -106, -150, -106 }; uint64_t iu = find_unit(G, 1, K.inv_id[i]); int px, py, moved = 0; unit_xy(player, &px, &py);
            for (int k = 0; iu && k < 8 && !moved && !getenv("DM_RIFT_TEST_NOFAR"); k++) { int32_t fp[2] = { px + fx[k], py + fy[k] };                                                                                                                                                                                        uint64_t rs = room_load(G, level_of(player), fp[0], fp[1]), rr = rs ? free_spot(G, rs, fp) : 0;
                if (rr && abs(fp[0] - px) + abs(fp[1] - py) >= 100 && dm_teleport(B, G, iu, rr, fp[0], fp[1])) { moved = 1; LOG("秘境：拍 %u 守护者现身，闯入首领（第 %d 位，类号 %d）退场到 %d,%d（离人 %d 格），不再给进度", t, i + 1, K.inv_cls[i], fp[0], fp[1], abs(fp[0] - px) + abs(fp[1] - py)); } }
            if (!moved && iu && g_entry_lv == lv) { int32_t fp[2] = { g_entry_x, g_entry_y }; uint64_t rs = room_load(G, level_of(player), fp[0], fp[1]), rr = rs ? free_spot(G, rs, fp) : 0;
                if (rr && dm_teleport(B, G, iu, rr, fp[0], fp[1])) { moved = 1; LOG("秘境：拍 %u 守护者现身，闯入首领（第 %d 位，类号 %d）远处没有空地，退场到本图入口 %d,%d（离人 %d 格）", t, i + 1, K.inv_cls[i], fp[0], fp[1], abs(fp[0] - px) + abs(fp[1] - py)); } }
            if (!moved) LOG("【红】秘境：拍 %u 守护者现身，闯入首领（第 %d 位）退场落点与本图入口都没传成，留在场上（作罢，不给进度）", t, i + 1);
            g_inv_gone[i] = K.inv_id[i]; if (iu) unit_xy(iu, &g_inv_gone_xy[i][0], &g_inv_gone_xy[i][1]); K.inv_st[i] = 4; K.inv_id[i] = 0; say("dmRiftSayInvaderGone"); }
        guard_wave(G, player, t, lowhp); }


    if (guard_wave_due(t, lv) && guard_wave(G, player, t, lowhp)) K.wave_t = 0;


    { static int dps = -1; if (dps < 0) { const char *e = getenv("DM_RIFT_TEST_DUMMY"); dps = e ? atoi(e) : 0; }
      if (dps > 0) for (int i = 0; i < 2; i++) { uint64_t u = K.inv_st[i] == 2 ? find_unit(G, 1, K.inv_id[i]) : 0; uint32_t m = u ? *(uint32_t *)(uintptr_t)(u + 0xc) : 0; if (!u || m == 0 || m == 12) continue;
          long long h = (long long)gs(u, 6) - (long long)dps * 256 / 25; ss(u, 6, h < 256 ? 256 : (int)h); }
      if (dps > 0 && K.guard && !atomic_load(&K.guard_dead)) for (int i = 0; i < 3; i++) { uint64_t u = K.guards[i] ? find_unit(G, 1, K.guards[i]) : 0; uint32_t m = u ? *(uint32_t *)(uintptr_t)(u + 0xc) : 0; if (!u || m == 0 || m == 12) continue;
          long long h = (long long)gs(u, 6) - (long long)dps * 256 / 25; ss(u, 6, h < 256 ? 256 : (int)h); } }


    if (K.guard && !atomic_load(&K.guard_dead)) { int alive = 0, deadn = 0; uint64_t last = 0;
        for (int i = 0; i < 3; i++) { if (!K.guards[i]) continue; uint64_t u = find_unit(G, 1, K.guards[i]); if (!u) { alive++; continue; }
            uint32_t m = *(uint32_t *)(uintptr_t)(u + 0xc); if (m == 0 || m == 12) { last = u; deadn++; static uint32_t said[3]; if (said[i] != K.guards[i]) { said[i] = K.guards[i]; LOG("秘境：拍 %u 守护者死亡（类号 %d id %u）", t, K.gcls[i], K.guards[i]); } } else alive++; }
        if (!K.wave_t) atomic_store(&K.gdead, deadn);
        if (!alive && last && K.wave + 1 < K.nwave) { memset(K.guards, 0, sizeof K.guards); K.wave++; K.wave_t = t; LOG("秘境：拍 %u 守护者第 %d 波全部倒下，%d 拍后刷第 %d 波", t, K.wave, WAVE_GAP, K.wave + 1); }
        else if (!alive && last) { atomic_store(&K.guard_dead, 1); LOG("秘境：拍 %u 守护者全部死亡 → 结算", t); finish(G, player, last); } }
}
