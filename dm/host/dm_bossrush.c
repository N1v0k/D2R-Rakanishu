



































#include "dm_bossrush.h"
#include "game_addrs.h"
#include "dm_feats.h"
#include "dm_unit.h"
#include "hle.h"
#include "dm.h"
#include "dm_i18n.h"
#include <stdarg.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

extern _Atomic int64_t g_dm_tz_time;
extern _Atomic uint64_t g_dm_tz_live_ns;

enum { DOOR_OBJ = 298, LV_TOWN = 109, LV_SUMMIT = 120, LV_CHAMBER = 132, NOTHING_CLS = 817 };
static const struct { int lv, n, cls[3]; const char *name; } k_stage[12] = {
    {LV_SUMMIT, 1, {156}, "dmBrAndariel"}, {LV_SUMMIT, 1, {211}, "dmBrDuriel"}, {LV_SUMMIT, 1, {242}, "dmBrMephisto"}, {LV_SUMMIT, 1, {243}, "dmBrDiablo"}, {LV_SUMMIT, 1, {544}, "dmBrBaal"},
    {LV_CHAMBER, 1, {156}, "dmBrAndariel"}, {LV_CHAMBER, 1, {211}, "dmBrDuriel"}, {LV_CHAMBER, 1, {242}, "dmBrMephisto"}, {LV_CHAMBER, 1, {243}, "dmBrDiablo"}, {LV_CHAMBER, 1, {544}, "dmBrBaal"},
    {LV_CHAMBER, 1, {333}, "dmBrDiabloClone"}, {LV_CHAMBER, 3, {704, 705, 709}, "dmBrUbers"},
};

static struct {
    uint64_t G, player; uint32_t last_t; int stage, total, done, eligible;
    uint32_t boss[3]; int nboss, boss_xy[2]; uint32_t next_at;
    int boss_hp[3], boss_dead[3], spawn_xy[3][2];
    int lv_prev; uint32_t in_since; int arena_lv, arena_xy[2], tp_due;
    uint32_t door_at; int table_off; uint8_t table_ver;
    uint32_t door_town, door_summit; int town_xy[2], summit_xy[2];
    uint32_t pend_door, pend_at; int town_tried;
    int tz_on; int64_t tz_time; uint32_t loot_at;
    uint32_t rush_at;
} R;

static uint8_t *B;
static uint64_t rd(uint64_t a) { return a ? *(volatile uint64_t *)(uintptr_t)a : 0; }
static uint32_t tick_of(uint64_t G) { return *(volatile uint32_t *)(uintptr_t)(G + 0x170); }
static int unit_level(uint64_t u) { return dm_unit_level(u); }
static uint64_t unit_room(uint64_t u) { uint64_t p = rd(u + 0x38); return !p ? 0 : *(uint32_t *)(uintptr_t)u == 2 ? rd(p) : rd(p + 0x20); }
static int unit_xy(uint64_t u, int *x, int *y) {
    uint64_t p = rd(u + 0x38); if (!p) return 0; uint32_t t = *(uint32_t *)(uintptr_t)u;
    if (t == 2) { *x = *(int32_t *)(uintptr_t)(p + 0x10); *y = *(int32_t *)(uintptr_t)(p + 0x14); } else { *x = *(uint16_t *)(uintptr_t)(p + 2); *y = *(uint16_t *)(uintptr_t)(p + 6); }
    return 1;
}
static uint64_t find_unit(uint64_t G, int type, uint32_t id, int cls) {
    for (int b = 0; b < 128; b++) for (uint64_t u = rd(G + 0x2230 + type * 0x400 + 8 * b); u; u = rd(u + 0x158)) {
        uint32_t c = *(uint32_t *)(uintptr_t)(u + 4), i = *(uint32_t *)(uintptr_t)(u + 8);
        if (id ? i == id : (int)c == cls) return u; }
    return 0;
}



void dm_game_notice(uint8_t *main_base, const char *text) {
    dm_game_chat(main_base, text);




    { static uint64_t msg[4]; msg[0] = 0xd106c9eabf113da5ull; msg[1] = 0x8fa6af1ab5b75b23ull; msg[2] = 0; msg[3] = 1;
      static char box[200]; strlcpy(box, text, sizeof box);
      ((void (*)(const char *, void *))(B + GA_NOTIFY_BOX_SHOW))(box, msg); }
}

void dm_game_chat(uint8_t *main_base, const char *text) {
    B = main_base; static char line[200]; strlcpy(line, text, sizeof line);
    static uint64_t txt[3], who[6], o1[8], o2[8], o3[8], o4[8];
    size_t n = strlen(line); txt[0] = (uint64_t)(uintptr_t)line; txt[1] = n; txt[2] = 0x8000000000000000ull | n;
    const uint8_t *g = B + GA_CHAT_SPEAKER_TPL; memset(who, 0, sizeof who); *(uint32_t *)who = *(const uint32_t *)g; who[1] = *(const uint64_t *)(g + 8); who[2] = *(const uint64_t *)(g + 16); who[3] = 0x8000000000000000ull | who[2];
    memset(o1, 0, sizeof o1); memset(o2, 0, sizeof o2); memset(o3, 0, sizeof o3); memset(o4, 0, sizeof o4);
    ((void (*)(uint64_t, void *, int, int, void *, void *, void *, void *, uint64_t, uint64_t))(B + GA_CHAT_ADD_LINE))((uint64_t)(uintptr_t)(B + GA_CHAT_MANAGER), txt, 4, 0, who, o1, o2, o3, (uint64_t)(uintptr_t)o4, 0);
}












static int loc_has(uint64_t map, const char *key, size_t n, uint64_t *bucket) {
    uint64_t cnt = rd(map), bk = rd(map + 8); if (!cnt || !bk || (cnt & (cnt - 1))) return -1;
    uint64_t h[4] = {0}; ((void (*)(uint64_t *, const char *, uint64_t, int))(B + GA_STRING_HASH))(h, key, n, 0); *bucket = bk + 8 * (h[0] & (cnt - 1));
    for (uint64_t nd = rd(*bucket); nd; nd = rd(nd)) if (rd(nd + 0x10) == n && !memcmp((const void *)(uintptr_t)rd(nd + 8), key, n)) return 1;
    return 0;
}
static void loc_add(uint64_t map, const char *key, const char *val) {
    size_t n = strlen(key); uint64_t bucket; if (loc_has(map, key, n, &bucket)) return;
    uint64_t al = rd((uint64_t)(uintptr_t)(B + GA_GAME_ALLOC)); if (!al) return;
    uint64_t *nd = ((uint64_t *(*)(uint64_t, uint64_t, int))(uintptr_t)rd(rd(al) + 0x10))(al, 0x30, 16); if (!nd) return;
    nd[1] = (uint64_t)(uintptr_t)key; nd[2] = n; nd[3] = 0; nd[4] = (uint64_t)(uintptr_t)val; nd[5] = strlen(val); nd[0] = rd(bucket);
    __atomic_store_n((uint64_t *)(uintptr_t)bucket, (uint64_t)(uintptr_t)nd, __ATOMIC_RELEASE);
    LOG("门名牌：串表挂上键 %s → %s", key, val);
}
void dm_door_label(uint8_t *main_base, uint64_t G, int cls, const char *key, int in_town) {
    B = main_base; static struct { int cls; char orig[0x40]; } sv[4]; static int nsv;
    uint64_t T = rd((uint64_t)(uintptr_t)(B + GA_DATA_TABLES + *(uint8_t *)(uintptr_t)(G + 0x106) * 16)); if (!T || (uint64_t)cls >= rd(T + 0x1550)) return;
    char *k = (char *)(uintptr_t)(rd(T + 0x1548) + (uint64_t)cls * 0x168 + 0x42); int i = 0; while (i < nsv && sv[i].cls != cls) i++;
    if (i == nsv) { if (nsv == 4) return; sv[i].cls = cls; memcpy(sv[i].orig, k, sizeof sv[i].orig); nsv++; }
    if (in_town) { uint32_t li = *(volatile uint32_t *)(B + GA_LANG_ID);
        loc_add((uint64_t)(uintptr_t)(B + GA_STRINGS_CUR_LANG), key, dm_ui_group(key, dm_lang_group_of((int)li))); loc_add((uint64_t)(uintptr_t)(B + GA_STRING_TABLE_EN), key, dm_ui_group(key, DM_LG_EN));
        if (strcmp(k, key)) { memset(k, 0, sizeof sv[i].orig); strlcpy(k, key, sizeof sv[i].orig); LOG("门名牌：物件 %d 串键换成 %s", cls, key); } }
    else if (!strcmp(k, key)) { memcpy(k, sv[i].orig, sizeof sv[i].orig); LOG("门名牌：物件 %d 串键写回 %s", cls, sv[i].orig); }
}
static void say(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
static void say(const char *fmt, ...) {
    char line[160], full[180]; va_list ap; va_start(ap, fmt); vsnprintf(line, sizeof line, fmt, ap); va_end(ap);
    snprintf(full, sizeof full, dm_str(B, "dmBrPrefix"), line); dm_game_notice(B, full); LOG("首领连战：提示「%s」", full);
}

enum { RUSH_AFTER = 75 };
int dm_bossrush_context(uint32_t tick) { return R.rush_at && tick - R.rush_at <= RUSH_AFTER; }
int dm_bossrush_owns(uint32_t id) { for (int i = 0; i < R.nboss && i < 3; i++) if (R.boss[i] == id) return 1; return 0; }



static uint64_t free_spot(uint64_t G, uint64_t room, int32_t pos[2]) {
    uint64_t out = 0; ((void (*)(uint64_t, uint64_t, int32_t *, int, int, uint64_t *, int, int))(B + GA_ROOM_FIND_FREE_SPOT))(G, room, pos, 3, 0x400, &out, 4, 100); return out;
}
static uint64_t level_of(uint64_t u) { return dm_unit_level_ptr(u); }


static void level_box(uint64_t lv, int *x0, int *y0, int *x1, int *y1) {
    *x0 = *y0 = 1 << 30; *x1 = *y1 = 0; int n = 0;
    for (uint64_t r = dm_level_rooms(lv); r && n < 2000; r = rd(r + 0x48), n++) { int x = *(int32_t *)(uintptr_t)(r + 0x60) * 5, y = *(int32_t *)(uintptr_t)(r + 0x64) * 5, w = *(int32_t *)(uintptr_t)(r + 0x68) * 5, h = *(int32_t *)(uintptr_t)(r + 0x6c) * 5;
        if (x < *x0) *x0 = x; if (y < *y0) *y0 = y; if (x + w > *x1) *x1 = x + w; if (y + h > *y1) *y1 = y + h; }
}
static uint64_t room_at(uint64_t lv, int px, int py) {
    int n = 0; for (uint64_t r = dm_level_rooms(lv); r && n < 2000; r = rd(r + 0x48), n++) { int x = *(int32_t *)(uintptr_t)(r + 0x60) * 5, y = *(int32_t *)(uintptr_t)(r + 0x64) * 5, w = *(int32_t *)(uintptr_t)(r + 0x68) * 5, h = *(int32_t *)(uintptr_t)(r + 0x6c) * 5;
        if (px >= x && px < x + w && py >= y && py < y + h) return rd(r + 0x58); }
    return 0;
}




static uint64_t clear_spot(uint64_t G, uint64_t lv, int x, int y, int keep) {
    uint64_t rs = room_at(lv, x, y); if (!rs) return 0; int32_t pos[2] = {x, y}; uint64_t rr = free_spot(G, rs, pos);
    if (!rr || abs(pos[0] - x) > 1 || abs(pos[1] - y) > 1) return 0;
    for (int b = 0; b < 128; b++) for (uint64_t u = rd(G + 0x2a30 + 8 * b); u; u = rd(u + 0x158)) { int ox, oy;
        if (unit_xy(u, &ox, &oy) && abs(ox - x) < keep && abs(oy - y) < keep) return 0; }
    return rr;
}
static int teleport(uint64_t G, uint64_t player, int x, int y) {
    return dm_teleport(B, G, player, room_at(level_of(player), x, y), x, y);
}
static int door_ids(uint64_t G, uint32_t *ids, int max) {
    int n = 0; for (int b = 0; b < 128; b++) for (uint64_t u = rd(G + 0x2230 + 2 * 0x400 + 8 * b); u; u = rd(u + 0x158))
        if (*(uint32_t *)(uintptr_t)(u + 4) == DOOR_OBJ && n < max) ids[n++] = *(uint32_t *)(uintptr_t)(u + 8);
    return n;
}


static uint32_t make_door(uint64_t G, uint64_t player, uint64_t room, int32_t pos[2], int target, int xy[2]) {
    uint64_t rr = free_spot(G, room, pos); if (!rr) return 0;
    uint32_t old[32]; int nold = door_ids(G, old, 32);
    int ok = ((int (*)(uint64_t, uint64_t, uint64_t, int, int, int, uint64_t, int, uint64_t, uint64_t))(B + GA_PORTAL_CREATE))(G, player, rr, pos[0], pos[1], target, 0, DOOR_OBJ, 0, 1);
    if (!ok) return 0;
    uint32_t best = 0; int bd = 1 << 30;
    for (int b = 0; b < 128; b++) for (uint64_t u = rd(G + 0x2230 + 2 * 0x400 + 8 * b); u; u = rd(u + 0x158)) {
        int x, y, seen = 0; uint32_t id = *(uint32_t *)(uintptr_t)(u + 8); if (*(uint32_t *)(uintptr_t)(u + 4) != DOOR_OBJ || !unit_xy(u, &x, &y)) continue;
        for (int i = 0; i < nold; i++) seen |= old[i] == id; if (seen) continue;
        int d = abs(x - pos[0]) + abs(y - pos[1]); if (d < bd) { bd = d; best = id; xy[0] = x; xy[1] = y; } }
    return best;
}

static int eligible(uint64_t G, uint64_t player) {


    uint8_t diff = *(uint8_t *)(uintptr_t)(G + 0x104); uint64_t q = rd(rd(rd(player + 0x10) + 0x40 + 8 * diff));
    return q && (*(uint8_t *)(uintptr_t)(q + 0x50) & 1);
}

static void tz_live(void) { atomic_store(&g_dm_tz_live_ns, (uint64_t)clock_gettime_nsec_np(CLOCK_MONOTONIC) + 1000000000ull); }
static void tz_set(uint64_t G, int on) {
    uint64_t (*zone)(uint64_t) = (uint64_t (*)(uint64_t))(B + GA_TZ_ZONE_GET); void (*apply)(uint64_t, uint64_t, int) = (void (*)(uint64_t, uint64_t, int))(B + GA_TZ_APPLY);
    if (on) { tz_live();
        if (!R.tz_time) { int64_t now = time(NULL);
            for (int j = 0; j < 4000 && !R.tz_time; j++) { atomic_store(&g_dm_tz_time, now + j * 1800 + 60); uint64_t rc = zone(0); int hit = 0;
                for (int q = 0; rc && q < 16; q++) hit |= *(int32_t *)(uintptr_t)(rc + 0x20 + 0x40 * (q / 2) + 4 * (q % 2)) == LV_CHAMBER;
                if (hit) R.tz_time = now + j * 1800 + 60; }
            atomic_store(&g_dm_tz_time, 0);
            if (!R.tz_time) { LOG("【红】首领连战：往后 4000 档没找到 WorldstoneKeep 当值的时刻，第 6–10 关不强制地带"); return; } }
        atomic_store(&g_dm_tz_time, R.tz_time);
    } else atomic_store(&g_dm_tz_time, 0);
    uint64_t rc = zone(0); if (rc) apply(G, rc, 1);
    R.tz_on = on; LOG("首领连战：恐怖地带%s（记录 %#llx%s）", on ? "强制为 WorldstoneKeep" : "恢复真实时钟", (unsigned long long)rc, rc ? "，已套用" : "，无记录");
}















static uint8_t *preset_slot(uint8_t ver) { uint64_t T = rd((uint64_t)(uintptr_t)(B + GA_DATA_TABLES + ver * 16)), a5 = T ? rd(T + 0x1060) : 0; return a5 ? (uint8_t *)(uintptr_t)(a5 + 4 * 33) : NULL; }
static void baal_table(uint64_t G, int off) {
    if (off == R.table_off) return; uint8_t ver = off ? *(uint8_t *)(uintptr_t)(G + 0x106) : R.table_ver; uint8_t *r = preset_slot(ver);
    if (!r || r[0] != 5 || *(uint16_t *)(r + 2) != (off ? 544 : 0)) { LOG("【红】首领连战：怪物预设表第五幕第 33 条不是预期值（%s），不动", off ? "巴尔 544" : "place_nothing"); return; }
    r[1] = off ? 0 : 1; *(uint16_t *)(r + 2) = off ? 0 : 544; R.table_off = off; R.table_ver = ver;
    LOG("首领连战：怪物预设表第五幕第 33 条%s", off ? "改成 place_nothing（132 建图时不取巴尔）" : "改回巴尔 544");
}
static uint64_t chamber_rec(uint64_t lv132, int type, int cls) {
    for (uint64_t r = dm_level_rooms(lv132), m = 0; r && m < 400; r = rd(r + 0x48), m++) { int rx = *(int32_t *)(uintptr_t)(r + 0x60) * 5, ry = *(int32_t *)(uintptr_t)(r + 0x64) * 5;
        uint64_t pl = rd(r + 0x98); if (!pl) continue;
        for (uint64_t u = rd(pl + 0x10), k = 0; u && k < 256; u = rd(u + 0x10), k++)
            if (*(int32_t *)(uintptr_t)u == type && *(int32_t *)(uintptr_t)(u + 4) == cls && rx + *(int32_t *)(uintptr_t)(u + 8) == 15135 && ry + *(int32_t *)(uintptr_t)(u + 0xc) == 5920) return u; }
    return 0;
}
static void reset(uint64_t G) {
    if (R.tz_on) atomic_store(&g_dm_tz_time, 0);
    if (R.table_off) { uint8_t *r = preset_slot(R.table_ver); if (r && r[0] == 5 && *(uint16_t *)(r + 2) == 0) { r[1] = 1; *(uint16_t *)(r + 2) = 544; LOG("首领连战：换局，怪物预设表第五幕第 33 条改回巴尔 544"); } }
    memset(&R, 0, sizeof R); R.G = G; R.eligible = -1;
}

void dm_bossrush_c2s(uint64_t G, uint64_t player, const uint8_t *pkt, uint32_t len) {
    (void)player;
    if (G != R.G || len < 9 || pkt[0] != 0x41) return;
    uint32_t id, ty; memcpy(&id, pkt + 1, 4); memcpy(&ty, pkt + 5, 4);
    uint64_t u = ty == 2 ? find_unit(G, 2, id, 0) : 0;
    R.pend_door = u && *(uint32_t *)(uintptr_t)(u + 4) == DOOR_OBJ ? id : 0; R.pend_at = tick_of(G);
}







static const int k_boss[] = { 156, 211, 242, 243, 544 };
static void boss_drop_watch(uint64_t G, uint32_t t) {
    static uint64_t g; static uint32_t id, t_die, max_item; static int cls, x, y;
    if (G != g) { g = G; id = 0; t_die = 0; }
    if (t_die) { if (t - t_die < 25) return;
        int n = 0, nc = 0; char lst[200] = "";
        for (int b = 0; b < 128; b++) for (uint64_t u = rd(G + 0x2e30 + 8 * b); u; u = rd(u + 0x158)) { uint32_t m = *(uint32_t *)(uintptr_t)(u + 0xc); int ix = 0, iy = 0;
            if (*(uint32_t *)(uintptr_t)(u + 8) <= max_item || (m != 3 && m != 5)) continue; uint64_t p = rd(u + 0x38); if (!p) continue; ix = *(int32_t *)(uintptr_t)(p + 0x10); iy = *(int32_t *)(uintptr_t)(p + 0x14);
            if (abs(ix - x) > 20 || abs(iy - y) > 20) continue; n++; uint32_t iid = *(uint32_t *)(uintptr_t)(u + 8);
            int k = 0;
            for (uint64_t c = rd((uint64_t)(uintptr_t)(B + GA_CLIENT_UNIT_TABLE + 4 * 0x400 + (iid & 0x7f) * 8)); c && k < 64; c = rd(c + 0x158), k++) if (*(uint32_t *)(uintptr_t)(c + 8) == iid) { nc++; break; } size_t L = strlen(lst); if (L < 180) snprintf(lst + L, sizeof lst - L, " %u", *(uint32_t *)(uintptr_t)(u + 4)); }
        uint8_t d = *(uint8_t *)(uintptr_t)(G + 0x104), ver = *(uint8_t *)(uintptr_t)(G + 0x106); uint64_t T = rd((uint64_t)(uintptr_t)(B + GA_DATA_TABLES + ver * 16)), mt = T ? rd(T + 0xf90) : 0;
        uint16_t *blk = mt ? (uint16_t *)(uintptr_t)(mt + (uint64_t)cls * 0x1fc + 0xc0 + d * 0x10) : NULL;
        extern _Atomic int g_dm_l_presses; int lp = atomic_load(&g_dm_l_presses);
        LOG("首领掉落读数：类号 %d 拍 %u 死于 %d,%d（难度 %u，宝藏格 普通 %u / 任务 %u），死后 25 拍尸体 20 格内新落地 %d 件%s%s；客户端有其中 %d 件；「显示物品」键本进程交给游戏 %d 次（%s）",
            cls, t_die, x, y, d, blk ? blk[0] : 0, blk ? blk[3] : 0, n, n ? "：类号" : "【零掉落】", lst, nc, lp, lp % 2 ? "单数 = 名字常显" : "双数 = 名字没开，或开了又被关");
        id = 0; t_die = 0; return; }
    if (t % 5) return;
    uint64_t u = 0;
    if (id) { u = find_unit(G, 1, id, 0); if (!u) { id = 0; return; } }
    else { for (unsigned i = 0; i < sizeof k_boss / sizeof *k_boss && !u; i++) { u = find_unit(G, 1, 0, k_boss[i]); if (u) { uint32_t m = *(uint32_t *)(uintptr_t)(u + 0xc); if (m == 0 || m == 12) u = 0; } }
        if (!u) return; id = *(uint32_t *)(uintptr_t)(u + 8); cls = (int)*(uint32_t *)(uintptr_t)(u + 4); }
    uint32_t m = *(uint32_t *)(uintptr_t)(u + 0xc); if (m == 0 || m == 12) { unit_xy(u, &x, &y); t_die = t; return; }


    max_item = 0; for (int b = 0; b < 128; b++) for (uint64_t v = rd(G + 0x2e30 + 8 * b); v; v = rd(v + 0x158)) { uint32_t i = *(uint32_t *)(uintptr_t)(v + 8); if (i > max_item) max_item = i; }
}

static _Atomic int sim_stage;
int dm_bossrush_stage(void) { return atomic_load(&sim_stage); }
void dm_bossrush_tick(uint64_t G, uint64_t player, uint8_t *main_base) {
    atomic_store(&sim_stage,0);
    B = main_base; if (!G || !player || !B) return;




    uint32_t t = tick_of(G);
    if (G != R.G || player != R.player || t < R.last_t) { reset(G); R.player = player; }
    R.last_t = t;
    int lv = unit_level(player); uint64_t room = unit_room(player); if (!lv || !room) return;
    if (!R.done && (R.stage || R.nboss) && (lv == LV_SUMMIT || lv == LV_CHAMBER)) R.rush_at = t ? t : 1;
#ifndef DM_PLAYER_BUILD
    if(t>=30)dm_combat_fixture_speed(B,player,1);
#endif
    atomic_store(&sim_stage,!R.done && R.nboss && lv==k_stage[R.stage].lv?R.stage+1:0);
    boss_drop_watch(G, t);


    { static int at = -1, wl; if (at < 0) { const char *e = getenv("DM_TEST_WARP_LV"); at = 0; if (e && sscanf(e, "%d:%d", &at, &wl) != 2) at = 0; }
      if (at > 0 && t >= (uint32_t)at && lv != wl) { at = 0; LOG("测试 DM_TEST_WARP_LV：拍 %u 从关卡 %d 进 %d", t, lv, wl); ((void (*)(uint64_t, uint64_t, int, int))(B + GA_LEVEL_CHANGE))(G, player, wl, 11); return; } }
    if (R.eligible < 0) { R.eligible = eligible(G, player); R.total = *(uint8_t *)(uintptr_t)(G + 0x104) == 2 ? 12 : 10;
        LOG("首领连战：本局%s资格（难度 %u，共 %d 关）", R.eligible ? "有" : "没有", *(uint8_t *)(uintptr_t)(G + 0x104), R.total); }




#ifndef DM_PLAYER_BUILD
    { static int initialized,at=-1,dx,dy;
      if(!initialized) {
          initialized=1;const char *input=getenv("DM_TEST_COMBAT_POSITION");int used=0;
          if(input && (sscanf(input,"%d:%d,%d%n",&at,&dx,&dy,&used)!=3 || input[used] || at<0 ||
                       dx < -100 || dx > 100 || dy < -100 || dy > 100)) {
              at=-1;LOG("ASSUME combat.fixture: invalid player position");
          }
      }
      if(at>=0 && t>=(unsigned)at) {
          at=-1;int x=0,y=0;
          if(!unit_xy(player,&x,&y))LOG("ASSUME combat.fixture: player position unavailable");
          else {
              int tx=x+dx,ty=y+dy;int32_t pos[2]={tx,ty};uint64_t destination=free_spot(G,room,pos);
              int ok=dm_teleport(B,G,player,destination,pos[0],pos[1]);
              int observed=unit_xy(player,&x,&y);
              LOG("[combat-position] {\"tick\":%u,\"area\":%d,\"requested\":[%d,%d],\"chosen\":[%d,%d],\"position\":[%d,%d],\"moved\":%d}",t,lv,tx,ty,pos[0],pos[1],x,y,ok);

              if(!ok || !observed || !unit_room(player) || unit_level(player)!=lv)LOG("ASSUME combat.fixture: player positioning failed");
          }
          return;
      }
    }
    { static char queue[2048];static int initialized;
      if(!initialized){initialized=1;const char *input=getenv("DM_TEST_SPAWN");
          if(input && strlen(input)>=sizeof queue)LOG("ASSUME combat.fixture: spawn list too long");
          else if(input)memcpy(queue,input,strlen(input)+1);
      }


      static int repeat,rcls,rdx,rdy,rsu;
      if(queue[0] || repeat) {
          int at=0,cls,dx,dy,used=0,times=1,more=0,su=0;
          if(repeat){cls=rcls;dx=rdx;dy=rdy;su=rsu;repeat--;}
          else if((sscanf(queue,"%d:%d:%d,%d%n",&at,&cls,&dx,&dy,&used)!=4 &&
                   (su=1,used=0,sscanf(queue,"%d:s%d:%d,%d%n",&at,&cls,&dx,&dy,&used)!=4)) || at<0 || cls<0 ||
             dx < -100 || dx > 100 || dy < -100 || dy > 100 ||
             (queue[used]=='*' && (sscanf(queue+used,"*%d%n",&times,&more)!=1 || times<1 || times>1000)) ||
             (queue[used+more] && queue[used+more]!=';')) {
              LOG("ASSUME combat.fixture: invalid spawn entry");queue[0]=0;at=-1;
          }
          if(at>=0 && t>=(unsigned)at) {
              if(used){used+=more;const char *rest=queue+used+(queue[used]==';');memmove(queue,rest,strlen(rest)+1);repeat=times-1;rcls=cls;rdx=dx;rdy=dy;rsu=su;}
              int x=0,y=0;unit_xy(player,&x,&y);int32_t pos[2]={x+dx,y+dy};uint64_t rr=free_spot(G,room,pos);

              uint64_t u=!rr?0:su?((uint64_t (*)(uint64_t,uint64_t,int,int,int,int))(B+GA_SUPERUNIQUE_SPAWN))(G,rr,pos[0],pos[1],cls,0)
                  :((uint64_t (*)(uint64_t,uint64_t,int,int,int,int,int,int,uint64_t))(B+GA_UNIT_SPAWN))(G,rr,1,cls,pos[0],pos[1],1,0,0x50);
              LOG("DM_TEST_SPAWN: tick %u at %d,%d (player %d,%d) spawn %s %d -> %s",t,pos[0],pos[1],x,y,su?"superunique row":"class",cls,u?"ok":"failed");
              if(u && su)cls=(int)*(uint32_t *)(uintptr_t)(u+4);
              if(u) {
                  int (*stat)(uint64_t,int,int)=(void *)(B+GA_STAT_GET);
                  LOG("[combat-fixture] {\"tick\":%u,\"unit\":%u,\"class\":%d,\"level\":%d,\"difficulty\":%u,\"life\":%d,\"max_life\":%d,\"defense\":%d,\"ar\":%d,\"position\":[%d,%d],\"resists\":[%d,%d,%d,%d,%d,%d]}",
                      t,*(uint32_t *)(u+8),cls,stat(u,12,0),*(uint8_t *)(G+0x104),stat(u,6,0),stat(u,7,0),stat(u,31,0),stat(u,19,0),pos[0],pos[1],
                      stat(u,36,0),stat(u,39,0),stat(u,43,0),stat(u,41,0),stat(u,45,0),stat(u,37,0));
                  char inventory[4096];size_t length=0;int count=0,bad=0,walked=0;
                  inventory[0]=0;
                  for(int bucket=0;bucket<128 && !bad;bucket++)for(uint64_t unit=rd(G+0x2630+8*bucket);unit;unit=rd(unit+0x158)) {
                      if(++walked>2048){bad=1;break;}
                      if(unit_level(unit)!=lv)continue;
                      int written=snprintf(inventory+length,sizeof inventory-length,"%s[%u,%u]",count?",":"",*(uint32_t *)(unit+4),*(uint32_t *)(unit+8));
                      if(written<0 || (size_t)written>=sizeof inventory-length){bad=1;break;}
                      length+=(size_t)written;count++;
                  }
                  if(bad)LOG("ASSUME combat.fixture: monster inventory overflow");
                  else LOG("[combat-fixture-inventory] {\"tick\":%u,\"area\":%d,\"units\":[%s]}",t,lv,inventory);
              } else LOG("ASSUME combat.fixture: monster spawn failed");
          }
      }
    }
#endif
#ifndef DM_PLAYER_BUILD
    dm_combat_fixture_cast(G,player);
#endif
    if (!R.eligible) return;
    dm_door_label(B, G, DOOR_OBJ, "dmDoorBossRush", lv == LV_TOWN);





    if (lv != LV_TOWN) R.town_tried = 0;
    if (lv == LV_TOWN && !R.door_town && !R.town_tried) { uint64_t lvt = level_of(player); int x0, y0, x1, y1; level_box(lvt, &x0, &y0, &x1, &y1);
        int cx = x0 + 76, cy = y0 + 80; if (room_at(lvt, cx, cy)) { R.town_tried = 1; uint64_t rr = 0; int32_t pos[2] = {cx, cy};
            for (int r = 0; r <= 5 && !rr; r++) for (int dx = -r; dx <= r && !rr; dx++) for (int dy = -r; dy <= r && !rr; dy++) { if (abs(dx) != r && abs(dy) != r) continue; pos[0] = cx + dx; pos[1] = cy + dy; rr = clear_spot(G, lvt, pos[0], pos[1], 3); }
            if (rr) R.door_town = make_door(G, player, rr, pos, LV_SUMMIT, R.town_xy);
            LOG("首领连战：哈洛加斯开门 %s（物件 id %u 在 %d,%d；目标 %d,%d）", R.door_town ? "成功" : "失败", R.door_town, R.town_xy[0], R.town_xy[1], cx, cy); } }



    if (R.pend_door) { uint64_t du = find_unit(G, 2, R.pend_door, 0); int px, py, dx, dy;
        int mine = R.pend_door == R.door_town || R.pend_door == R.door_summit, back = !mine && (lv == LV_SUMMIT || lv == LV_CHAMBER);
        if (!du || (!mine && !back) || t - R.pend_at > 250) R.pend_door = 0;
        else if (unit_xy(player, &px, &py) && unit_xy(du, &dx, &dy) && abs(px - dx) <= 4 && abs(py - dy) <= 4) {
            int to = back ? LV_TOWN : R.pend_door == R.door_summit ? LV_CHAMBER : R.stage < 5 ? LV_SUMMIT : LV_CHAMBER; R.pend_door = 0;
            if (R.done && !back) say("%s", dm_str(B, "dmBrDoneThisGame"));
            else { LOG("首领连战：拍 %u 进门 → %d", t, to); R.tp_due = to != LV_TOWN;  ((void (*)(uint64_t, uint64_t, int, int))(B + GA_LEVEL_CHANGE))(G, player, to, 11); return; } } }







    if (lv != R.lv_prev) { R.lv_prev = lv; R.in_since = t; if (lv != LV_SUMMIT && lv != LV_CHAMBER) R.tp_due = 0;


#ifndef DM_PLAYER_BUILD
        static int lose = -1; if (lose < 0) lose = getenv("DM_TEST_BR_LOSE") != NULL;
        if (lose && R.nboss && lv != k_stage[R.stage].lv) for (int i = 0; i < R.nboss; i++) if (!R.boss_dead[i]) { R.boss[i] = 0x7ffffff0u + i; LOG("首领连战：测试 DM_TEST_BR_LOSE：第 %d 关首领 id 换成不存在的值", R.stage + 1); }
#endif
    }


    if (R.tp_due && lv == R.lv_prev && (lv == LV_SUMMIT || lv == LV_CHAMBER)) { R.tp_due = 0; int px, py, x0, y0, x1, y1; unit_xy(player, &px, &py); level_box(level_of(player), &x0, &y0, &x1, &y1);
        double vx = (x0 + x1) / 2.0 - px, vy = (y0 + y1) / 2.0 - py, vl = vx * vx + vy * vy > 1 ? __builtin_sqrt(vx * vx + vy * vy) : 1;
        int ok = 0; for (int d = 18; d >= 10 && !ok; d -= 4) ok = teleport(G, player, px + (int)(d * vx / vl), py + (int)(d * vy / vl));
        int nx, ny; unit_xy(player, &nx, &ny); R.arena_lv = lv; R.arena_xy[0] = nx; R.arena_xy[1] = ny;
        LOG("首领连战：拍 %u 进关卡 %d 落在入口 %d,%d → 挪进擂台 %s，现在 %d,%d", t, lv, px, py, ok ? "成" : "不成", nx, ny); }





    { static int at = -1; if (at < 0) { const char *e = getenv("DM_TEST_BR_WARP132"); at = e ? atoi(e) : 0; }
      if (at > 0 && (R.done || (!R.stage && !R.nboss)) && lv == LV_TOWN && t >= (uint32_t)at) { at = 0; LOG("首领连战：测试 DM_TEST_BR_WARP132：拍 %u 照原版王座门进 132", t); ((void (*)(uint64_t, uint64_t, int, int))(B + GA_LEVEL_CHANGE))(G, player, LV_CHAMBER, 11); return; } }


    static int tz_off = -1; if (tz_off < 0) { const char *e = getenv("DM_BOSSRUSH_TZ"); tz_off = e && e[0] == '0'; }
    int want = !tz_off && !R.done && R.stage >= 5 && R.stage < 10 && lv == LV_CHAMBER;
    if (want != R.tz_on) tz_set(G, want);




    if (lv == LV_CHAMBER && R.table_off) { uint64_t u = chamber_rec(level_of(player), 1, NOTHING_CLS); baal_table(G, 0);
        LOG("首领连战：132 已建，原生巴尔预设%s", u ? "已取成不刷" : "【红】没取成不刷（站位 (15135,5920) 找不到 {怪, 817} 记录）"); }
    if (R.tz_on) tz_live();

    if (R.loot_at && (int32_t)(t - R.loot_at) >= 0) { R.loot_at = 0; int n1 = 0, n2 = 0, all = 0;
        for (int b = 0; b < 128; b++) for (uint64_t u = rd(G + 0x2e30 + 8 * b); u; u = rd(u + 0x158)) { uint64_t d = rd(u + 0x10);
            if (*(uint32_t *)(uintptr_t)u != 4 || *(uint32_t *)(uintptr_t)(u + 0xc) != 3 || !d) continue; all++; if (*(uint32_t *)(uintptr_t)d != 7) continue;
            uint32_t c = ((uint32_t (*)(uint64_t))(B + GA_ITEM_CODE))(u); n1 += !memcmp(&c, "cm1 ", 4); n2 += !memcmp(&c, "cm2 ", 4); }
        LOG("首领连战：拍 %u 本局地上物品 %d 件，其中暗金小型神符 %d 件、暗金大型神符 %d 件", t, all, n1, n2); }










    if (R.door_at && (int32_t)(t - R.door_at) >= 0 && lv == LV_SUMMIT) { R.door_at = 0; uint64_t lvp = level_of(player), best_rr = 0; int32_t best[2] = {0, 0}; int bx = R.boss_xy[0], by = R.boss_xy[1], bd = -1;
        static int lx[256], ly[256]; int nl = 0;
        for (int b = 0; b < 128; b++) for (uint64_t u = rd(G + 0x2e30 + 8 * b); u && nl < 256; u = rd(u + 0x158)) { uint64_t ip = rd(u + 0x38);
            if (*(uint32_t *)(uintptr_t)(u + 0xc) != 3 || !ip) continue; lx[nl] = *(int32_t *)(uintptr_t)(ip + 0x10); ly[nl++] = *(int32_t *)(uintptr_t)(ip + 0x14); }
        static const int8_t dir[16][2] = {{2, 0}, {2, 1}, {2, 2}, {1, 2}, {0, 2}, {-1, 2}, {-2, 2}, {-2, 1}, {-2, 0}, {-2, -1}, {-2, -2}, {-1, -2}, {0, -2}, {1, -2}, {2, -2}, {2, -1}};


        for (int r = 1; r <= 2 && bd < 3; r++) for (int d = 0; d < 16; d++) { int32_t pos[2] = {bx + dir[d][0] * r + (r == 2 ? dir[d][0] / 2 : 0), by + dir[d][1] * r + (r == 2 ? dir[d][1] / 2 : 0)};
            int dmin = 99; for (int i = 0; i < nl; i++) { int dd = abs(lx[i] - pos[0]) > abs(ly[i] - pos[1]) ? abs(lx[i] - pos[0]) : abs(ly[i] - pos[1]); if (dd < dmin) dmin = dd; }
            if (dmin <= bd) continue; uint64_t rr = clear_spot(G, lvp, pos[0], pos[1], 3); if (rr) { bd = dmin; best_rr = rr; best[0] = pos[0]; best[1] = pos[1]; } }
        if (best_rr) R.door_summit = make_door(G, player, best_rr, best, LV_CHAMBER, R.summit_xy);
        LOG("首领连战：亚瑞特之巅开门 %s（物件 id %u 在 %d,%d；巴尔倒在 %d,%d；地上掉落 %d 件，最近的离门 %d 格）", R.door_summit ? "成功" : "失败", R.door_summit, R.summit_xy[0], R.summit_xy[1], bx, by, nl, bd);
        say("%s", dm_str(B, "dmBrToChamber")); }
    if (R.nboss) { int alive = 0, missing = 0;
        for (int i = 0; i < R.nboss; i++) { if (R.boss_dead[i]) continue; uint64_t u = find_unit(G, 1, R.boss[i], 0);
            if (!u) { missing++; continue; } uint32_t md = *(uint32_t *)(uintptr_t)(u + 0xc);
            if (md == 0 || md == 12) { R.boss_dead[i] = 1; LOG("首领连战：拍 %u 第 %d 关类号 %d 死亡", t, R.stage + 1, k_stage[R.stage].cls[i]); continue; }
            alive++; R.boss_hp[i] = ((int (*)(uint64_t, int, int))(B + GA_STAT_GET))(u, 6, 0); unit_xy(u, &R.boss_xy[0], &R.boss_xy[1]);
            if (t % 125 == 0) LOG("首领连战：拍 %u 第 %d 关类号 %d 生命 %d/%d", t, R.stage + 1, k_stage[R.stage].cls[i], R.boss_hp[i] >> 8, ((int (*)(uint64_t, int, int))(B + GA_STAT_GET))(u, 7, 0) >> 8); }
        if (!alive && !missing) {
            LOG("首领连战：拍 %u 第 %d 关打完", t, R.stage + 1); R.nboss = 0; R.stage++; R.next_at = t + 75;
            if (R.stage >= 11) R.loot_at = t + 50;
            if (R.stage == R.total) { R.done = 1; if (R.tz_on) tz_set(G, 0); say("%s", dm_str(B, "dmBrAllDone")); dm_feats_event(DM_FT_E_RUSH); return; }
            if (R.stage == 5) { R.door_at = t + 50; baal_table(G, 1); }
            return; }
        if (missing && lv == k_stage[R.stage].lv && t - R.in_since >= 50) for (int i = 0; i < R.nboss; i++) {
            if (R.boss_dead[i] || find_unit(G, 1, R.boss[i], 0)) continue;
            int32_t pos[2] = {R.spawn_xy[i][0], R.spawn_xy[i][1]}; uint64_t rs = room_at(level_of(player), pos[0], pos[1]), rr = rs ? free_spot(G, rs, pos) : 0;
            uint64_t u = rr ? ((uint64_t (*)(uint64_t, uint64_t, int, int, int, int, int, int, uint64_t))(B + GA_UNIT_SPAWN))(G, rr, 1, k_stage[R.stage].cls[i], pos[0], pos[1], 1, 0, 0x10) : 0;
            if (!u) continue;
            if (R.boss_hp[i] > 0) ((void (*)(uint64_t, int, int, int))(B + GA_STAT_SET))(u, 6, R.boss_hp[i], 0);
            LOG("首领连战：拍 %u 第 %d 关类号 %d 不在单位表里（人离开过关卡），按原刷点接回：生命 %d → 新单位 id %u", t, R.stage + 1, k_stage[R.stage].cls[i], R.boss_hp[i] >> 8, *(uint32_t *)(uintptr_t)(u + 8));
            R.boss[i] = *(uint32_t *)(uintptr_t)(u + 8); }
        return; }
    if (R.done || lv != k_stage[R.stage].lv) { R.next_at = 0; return; }
    { uint32_t pm = *(uint32_t *)(uintptr_t)(player + 0xc); if (pm == 0 || pm == 17) { R.next_at = 0; return; } }
    if (!R.next_at) R.next_at = t + 25;
    if ((int32_t)(t - R.next_at) < 0) return;




    int px, py, x0, y0, x1, y1; unit_xy(player, &px, &py); uint64_t lvp = level_of(player); level_box(lvp, &x0, &y0, &x1, &y1);
    int ax = R.arena_lv == lv ? R.arena_xy[0] : px, ay = R.arena_lv == lv ? R.arena_xy[1] : py;
    double vx = (x0 + x1) / 2.0 - ax, vy = (y0 + y1) / 2.0 - ay, vl = vx * vx + vy * vy > 1 ? __builtin_sqrt(vx * vx + vy * vy) : 1;
    static const float ring[5][2] = {{8, 0}, {-6.47f, 4.70f}, {2.47f, -7.61f}, {2.47f, 7.61f}, {-6.47f, -4.70f}};
    float rx = ring[R.stage % 5][0], ry = ring[R.stage % 5][1];
    for (int i = 0; i < k_stage[R.stage].n; i++) {
        int32_t pos[2] = {ax + (int)(10 * vx / vl + rx) + (int)(6 * (i - (k_stage[R.stage].n - 1) / 2.0) * -vy / vl), ay + (int)(10 * vy / vl + ry) + (int)(6 * (i - (k_stage[R.stage].n - 1) / 2.0) * vx / vl)};
        uint64_t rs = room_at(lvp, pos[0], pos[1]); if (!rs) { rs = room; pos[0] = px + 8; pos[1] = py; }
        uint64_t rr = free_spot(G, rs, pos);
        uint64_t u = rr ? ((uint64_t (*)(uint64_t, uint64_t, int, int, int, int, int, int, uint64_t))(B + GA_UNIT_SPAWN))(G, rr, 1, k_stage[R.stage].cls[i], pos[0], pos[1], 1, 0, 0x10) : 0;
        if (u) { int k = R.nboss++; R.boss[k] = *(uint32_t *)(uintptr_t)(u + 8); R.boss_dead[k] = 0; R.boss_hp[k] = 0; R.spawn_xy[k][0] = pos[0]; R.spawn_xy[k][1] = pos[1]; }
        LOG("首领连战：拍 %u 第 %d 关刷类号 %d 在 %d,%d → %s（等级 %d）", t, R.stage + 1, k_stage[R.stage].cls[i], pos[0], pos[1], u ? "成功" : "失败",
            u ? ((int (*)(uint64_t, int, int))(B + GA_STAT_GET))(u, 12, 0) : 0); }
    if (!R.nboss) { R.next_at = t + 25; return; }
    char st[160]; snprintf(st, sizeof st, dm_str(B, "dmBrStage"), R.stage + 1, R.total, dm_str(B, k_stage[R.stage].name), R.stage >= 5 && R.stage < 10 ? dm_str(B, "dmBrTerror") : "");
    say("%s", st);
}
