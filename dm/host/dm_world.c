







#include "touch_mac.h"
#include "dm_uiscreen.h"
#include "dm_world.h"
#include "hle.h"
#include "dm.h"
#include <string.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <math.h>
#include <time.h>
#include "dm_unit.h"
#include "dm_hud.h"
#include "dm_prof.h"
#include <TargetConditionals.h>

static uint8_t *wb(void) { static uint8_t *b; if (!b) for (int i = 0; i < g_nmod; i++) if (!strcmp(g_mod[i].name, "main")) b = (uint8_t *)g_mod[i].base; return b; }
static uint64_t w_player(void) { uint8_t *b = wb(); return b ? ((uint64_t (*)(uint32_t))(b + 0x45f9d0))(*(uint32_t *)(b + 0x2d20e4c)) : 0; }
static int w_unit_xy(uint64_t u, int *x, int *y) {
    uint64_t p = *(uint64_t *)(u + 0x38); if (!p) return 0; uint32_t t = *(uint32_t *)u;
    if (t == 2 || t == 4 || t == 5) { *x = *(int32_t *)(p + 0x10); *y = *(int32_t *)(p + 0x14); } else { *x = *(uint16_t *)(p + 2); *y = *(uint16_t *)(p + 6); }
    return 1;
}
_Atomic uint64_t g_dm_move_allow_until_ns;
static uint64_t w_now_ns(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return (uint64_t)t.tv_sec * 1000000000ull + (uint64_t)t.tv_nsec; }
static uint64_t g_release_at; static int g_rel_x, g_rel_y;




static struct { uint64_t seq, frame; uint32_t player; int active, stage, last_lv; } g_it;
static uint64_t w_interaction_begin(uint64_t pl, uint64_t u, uint64_t fr) {
    if (*(uint32_t *)(u) != 2) return 0;
    if (g_it.active) LOG("物件交互诊断：序号 %llu 被新交互替代", (unsigned long long)g_it.seq);
    g_it.seq++; g_it.frame = fr; g_it.player = *(uint32_t *)(pl + 8); g_it.active = 1; g_it.stage = 0; g_it.last_lv = dm_unit_level(pl);
    int x = 0, y = 0; w_unit_xy(u, &x, &y);
    LOG("物件交互诊断：序号 %llu 帧 %llu 玩家 %u 客户端关卡 %d 目标类号 %u id %u 坐标 %d,%d", (unsigned long long)g_it.seq, (unsigned long long)fr, g_it.player, g_it.last_lv, *(uint32_t *)(u + 4), *(uint32_t *)(u + 8), x, y);
    return g_it.seq;
}
static void w_interaction_poll(uint64_t pl, uint64_t fr) {
    if (!g_it.active) return;
    if (!pl || *(uint32_t *)(pl + 8) != g_it.player || fr < g_it.frame) { LOG("物件交互诊断：序号 %llu 玩家离局或切换，结束", (unsigned long long)g_it.seq); g_it.active = 0; return; }
    int lv = dm_unit_level(pl); uint64_t age = fr - g_it.frame;
    static const uint64_t at[] = { 3, 30, 120 };
    if (lv == g_it.last_lv && (g_it.stage >= 3 || age < at[g_it.stage])) return;
    while (g_it.stage < 3 && age >= at[g_it.stage]) g_it.stage++;
    int x = 0, y = 0; w_unit_xy(pl, &x, &y); dm_hud_info hi; dm_hud_get(&hi);
    LOG("物件交互诊断：序号 %llu 后 %llu 帧 客户端关卡 %d 坐标 %d,%d 模式 %u 服务端最近快照 %d 有效 %d（样本非换图成功判据）", (unsigned long long)g_it.seq, (unsigned long long)age, lv, x, y, *(uint32_t *)(pl + 0xc), hi.area, hi.valid);
    g_it.last_lv = lv; if (age >= 120) g_it.active = 0;
}
static int w_interact(uint64_t u) {
    uint8_t *b = wb(); uint64_t pl = w_player(); int x = 0, y = 0; if (!b || !pl || !u || !w_unit_xy(u, &x, &y)) return 0;
    uint64_t mgr = ((uint64_t (*)(void))(b + 0x2acecc))(), keep = *(uint64_t *)(mgr + 0xd10);
    atomic_store(&g_dm_move_allow_until_ns, w_now_ns() + 500000000ull);
    uint64_t trace = w_interaction_begin(pl, u, dm_guest_frame());
    *(uint64_t *)(mgr + 0xd10) = u;
    int r = ((int (*)(uint64_t, int, int, int, uint64_t))(b + 0x4ace20))(pl, 7, x, y, 0);
    *(uint64_t *)(mgr + 0xd10) = keep;
    if (trace) LOG("物件交互诊断：序号 %llu 点击入口返回 %d（尚未确认换图）", (unsigned long long)trace, r);
    g_release_at = dm_guest_frame() + 3; g_rel_x = x; g_rel_y = y;
    return r;
}






#define W_NEAR 2




#define W_FAR 4
#if TARGET_OS_OSX
#define W_GROUND 99
#define W_NEAREST_OK(t) ((t) == 2 || (t) == W_GROUND)
static int g_hit_mon;
#define W_MON_OK(cat) (g_hit_mon && ((cat) == 1 || (cat) == 0xa))
static int g_gnd_x, g_gnd_y;
static uint64_t g_gnd_stick_seen, g_gnd_stick_at;
#else
#define W_NEAREST_OK(t) ((t) == 2)
#define W_MON_OK(cat) 0
#endif
static struct { uint32_t type, id; uint64_t until_ns, stick, next_fr, still_fr; int on, lx, ly; int n, i, tx, ty, replans, stop_at, empty, direct, alt, alt_on, ax, ay, nearest; uint16_t node[24][2]; } g_go;
static uint64_t w_find(uint32_t type, uint32_t id) { uint8_t *b = wb(); return type <= 5 ? ((uint64_t (*)(uint64_t, uint32_t, uint32_t, uint32_t))(b + 0x45f5f0))((uint64_t)(uintptr_t)(b + 0x2d603a0 + type * 0x400), id & 0x7f, id, type) : 0; }
static void w_go(uint64_t u) { uint64_t dm_stick_last_ns(void); g_go.type = *(uint32_t *)u; g_go.nearest = 0; g_go.id = *(uint32_t *)(u + 8); g_go.until_ns = w_now_ns() + 15000000000ull; g_go.stick = dm_stick_last_ns(); g_go.next_fr = 0; g_go.still_fr = 0; g_go.n = g_go.i = g_go.replans = g_go.empty = g_go.direct = g_go.alt = g_go.alt_on = 0; g_go.stop_at = 0; g_go.on = 1; }














#define WP_W 192
#define WP_CAP 24000
#define WP_SEG 30
static uint64_t g_wp_room[96]; static int g_wp_nroom;
static int wp_room_has(uint64_t r, int x, int y) { int rx = *(int *)(r + 0x80), ry = *(int *)(r + 0x84); return x >= rx && y >= ry && x < rx + *(int *)(r + 0x88) && y < ry + *(int *)(r + 0x8c); }
static void wp_rooms(uint64_t r0) {
    g_wp_nroom = 0; if (!r0) return; g_wp_room[g_wp_nroom++] = r0;
    for (int i = 0; i < g_wp_nroom; i++) { uint64_t r = g_wp_room[i], *adj = *(uint64_t **)r; uint32_t n = *(uint32_t *)(r + 0x40); for (uint32_t k = 0; adj && k < n && k < 32; k++) { uint64_t a = adj[k]; int seen = 0; if (!a) continue; for (int q = 0; q < g_wp_nroom; q++) if (g_wp_room[q] == a) { seen = 1; break; } if (!seen && g_wp_nroom < 96) g_wp_room[g_wp_nroom++] = a; } }
}
static int wp_cell(int x, int y) {
    static int last; for (int t = 0; t < g_wp_nroom; t++) { int i = (last + t) % g_wp_nroom; uint64_t r = g_wp_room[i]; if (!wp_room_has(r, x, y)) continue; last = i;
        uint64_t g = *(uint64_t *)(r + 0x38); if (!g) return 0xffff; uint16_t *a = *(uint16_t **)(g + 0x20); int gw = *(int *)(g + 8); return a ? a[(y - *(int *)(r + 0x84)) * gw + (x - *(int *)(r + 0x80))] : 0xffff; }
    return 0xffff; }










#define WP_MASK_DEF 0x1c09
static uint32_t g_wp_mask = WP_MASK_DEF; static int g_wp_sx, g_wp_sy;
static uint8_t g_wp_blk[WP_W * WP_W]; static int g_wp_ox, g_wp_oy;
static int wp_blk(int x, int y) { int ix = x - g_wp_ox, iy = y - g_wp_oy; if (ix < 0 || iy < 0 || ix >= WP_W || iy >= WP_W) return 1; uint8_t *m = &g_wp_blk[iy * WP_W + ix];
    if (!*m) { uint32_t v = (uint32_t)wp_cell(x, y) & g_wp_mask; if (x == g_wp_sx && y == g_wp_sy) v &= ~0x1000u; *m = v ? 2 : 1; } return *m == 2; }
static int wp_free(int x, int y) { return !(wp_blk(x, y) || wp_blk(x + 1, y) || wp_blk(x - 1, y) || wp_blk(x, y + 1) || wp_blk(x, y - 1)); }
static int wp_line(int x0, int y0, int x1, int y1) {
    int dx = abs(x1 - x0), dy = abs(y1 - y0), sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1, e = dx - dy;
    for (;;) { if (!wp_free(x0, y0)) return 0; if (x0 == x1 && y0 == y1) return 1; int e2 = 2 * e, mx = 0, my = 0; if (e2 > -dy) { e -= dy; mx = 1; } if (e2 < dx) { e += dx; my = 1; }
        if (mx && my && (!wp_free(x0 + sx, y0) || !wp_free(x0, y0 + sy))) return 0; if (mx) x0 += sx; if (my) y0 += sy; } }










static void w_cam_clear(void) {
    int dm_cam_mode(void); void dm_cam_set_clear(const float d[32], float px, float pz);
    if (dm_cam_mode() != 1) return;
    uint64_t pl = w_player(), pp = pl ? *(uint64_t *)(pl + 0x38) : 0; if (!pp || !*(uint64_t *)(pp + 0x20)) return;
    static float sub = -1; static uint32_t mask;
    if (sub < 0) { const char *e = getenv("DM_CAM_SUB"); sub = e ? (float)atof(e) : 2.0f; e = getenv("DM_CAM_WALL"); mask = e ? (uint32_t)strtoul(e, NULL, 0) : 0x801; }
    float x0 = *(uint32_t *)pp / 65536.0f, y0 = *(uint32_t *)(pp + 4) / 65536.0f, d[32];
    wp_rooms(*(uint64_t *)(pp + 0x20));
    for (int k = 0; k < 32; k++) {
        float ux = cosf(k * 6.2831853f / 32), uy = sinf(k * 6.2831853f / 32), t = 0.5f;
        for (; t <= 12; t += 0.25f) if ((uint32_t)wp_cell((int)floorf(x0 + ux * t), (int)floorf(y0 + uy * t)) & mask) break;
        t -= 0.75f; d[k] = (t < 1 ? 1 : t) * sub; }
    dm_cam_set_clear(d, x0 * sub, y0 * sub);
}
static int w_plan(uint8_t *b, uint64_t pl, int x, int y) {
    (void)b; uint64_t t0 = w_now_ns(); uint64_t pp = *(uint64_t *)(pl + 0x38); g_go.n = g_go.i = 0; int r = 0; uint32_t nn = 0; if (!pp) return 0;
    int px = *(uint16_t *)(pp + 2), py = *(uint16_t *)(pp + 6); wp_rooms(*(uint64_t *)(pp + 0x20));
    { uint32_t mk = *(uint32_t *)(pp + 0x64); g_wp_mask = (mk & 1) && !(mk & ~0xffffu) ? mk : WP_MASK_DEF; g_wp_sx = px; g_wp_sy = py; static uint32_t said; if (g_wp_mask != said) { said = g_wp_mask; LOG("世界目标：规划用的碰撞掩码 %#x（玩家路径结构 +0x64 读到 %#x）", g_wp_mask, mk); } }
    static uint8_t from[WP_W * WP_W]; static uint32_t q[WP_W * WP_W]; static const int8_t D[8][2] = {{1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1}};
    int ox = (px + x) / 2 - WP_W / 2, oy = (py + y) / 2 - WP_W / 2, sxi = px - ox, syi = py - oy, goal = -1;
    if (abs(x - px) >= WP_W - 8 || abs(y - py) >= WP_W - 8) LOG("世界目标：目标离得太远（%d,%d 格），超出规划窗口，不规划", abs(x - px), abs(y - py));
    if (sxi >= 0 && syi >= 0 && sxi < WP_W && syi < WP_W && abs(x - px) < WP_W - 8 && abs(y - py) < WP_W - 8) {
        memset(from, 0, sizeof from); memset(g_wp_blk, 0, sizeof g_wp_blk); g_wp_ox = ox; g_wp_oy = oy; int h = 0, t = 0; q[t++] = (uint32_t)(syi * WP_W + sxi); from[syi * WP_W + sxi] = 9;
        int best = -1, bestd = 1 << 30;
        while (h < t && h < WP_CAP) { int c = (int)q[h++], cx = c % WP_W + ox, cy = c / WP_W + oy; if (abs(cx - x) <= W_NEAR && abs(cy - y) <= W_NEAR) { goal = c; break; }
            { int ddx = abs(cx - x), ddy = abs(cy - y), dd = (ddx > ddy ? ddx : ddy) * 64 + ddx + ddy; if (dd < bestd) { bestd = dd; best = c; } }
            for (int d = 0; d < 8; d++) { int nx = cx + D[d][0], ny = cy + D[d][1], ix = nx - ox, iy = ny - oy; if (ix < 0 || iy < 0 || ix >= WP_W || iy >= WP_W || from[iy * WP_W + ix]) continue;
                if (!wp_free(nx, ny)) continue; if (d >= 4 && (!wp_free(cx + D[d][0], cy) || !wp_free(cx, cy + D[d][1]))) continue; from[iy * WP_W + ix] = (uint8_t)(d + 1); q[t++] = (uint32_t)(iy * WP_W + ix); } }
        r = goal >= 0; nn = (uint32_t)t;




        g_go.nearest = 0; if (goal < 0 && h >= t && best >= 0 && W_NEAREST_OK(g_go.type)) { g_go.nearest = 1; if (best != syi * WP_W + sxi) goal = best; }
        if (goal >= 0) {
            int n = 0, c = goal; while (from[c] != 9 && n < WP_W * WP_W / 2) { q[WP_W * WP_W - 1 - n++] = (uint32_t)c; int d = from[c] - 1; c -= D[d][1] * WP_W + D[d][0]; }
            uint32_t *path = q + WP_W * WP_W - n; int ax = px, ay = py, k = 0;
            while (k < n && g_go.n < 24) { int best = k; for (int m = (n - 1 < k + WP_SEG ? n - 1 : k + WP_SEG); m > k; m--) if (wp_line(ax, ay, (int)(path[m] % WP_W) + ox, (int)(path[m] / WP_W) + oy)) { best = m; break; }
                ax = (int)(path[best] % WP_W) + ox; ay = (int)(path[best] / WP_W) + oy; g_go.node[g_go.n][0] = (uint16_t)ax; g_go.node[g_go.n][1] = (uint16_t)ay; g_go.n++; k = best + 1; }
            if (k < n) g_go.n = 0; } }


    if ((nn <= 1 && !r) || getenv("DM_TEST_WP_DUMP")) { static int dl; int R = getenv("DM_TEST_WP_DUMP") ? 5 : 2; if (dl++ < 12) { char o[800]; size_t L = 0; o[0] = 0;
        for (int yy = py - R; yy <= py + R; yy++) { for (int xx = px - R; xx <= px + R; xx++) L += (size_t)snprintf(o + L, L < sizeof o ? sizeof o - L : 0, "%4x ", wp_cell(xx, yy)); L += (size_t)snprintf(o + L, L < sizeof o ? sizeof o - L : 0, "| "); }
        LOG("世界目标：起点 %d,%d 周围碰撞位 %s", px, py, o); } }
    if (getenv("DM_TEST_WP_DUMP") && g_go.n) { char o[300]; size_t L = 0; o[0] = 0; for (int i = 0; i < g_go.n; i++) L += (size_t)snprintf(o + L, L < sizeof o ? sizeof o - L : 0, " %d,%d", g_go.node[i][0], g_go.node[i][1]); LOG("世界目标：路线各段终点%s", o); }
    g_go.tx = x; g_go.ty = y;
    static int lg; if (lg++ < 60) LOG("世界目标：路线 %d 段（找到 %d、搜过 %u 格、%.2f ms；末点 %d,%d；目标 %d,%d）", g_go.n, r, nn, (double)(w_now_ns() - t0) / 1e6, g_go.n ? g_go.node[g_go.n - 1][0] : 0, g_go.n ? g_go.node[g_go.n - 1][1] : 0, x, y);
    return g_go.n;
}




static int w_same_room(uint64_t pl, uint64_t u) { uint64_t a = *(uint64_t *)(pl + 0x38), c = *(uint64_t *)(u + 0x38); return a && c && *(uint64_t *)c && *(uint64_t *)(a + 0x20) == *(uint64_t *)c; }




static int w_needs_room(uint64_t u) { uint8_t *b = wb(); if (*(uint32_t *)u != 2) return 0; uint8_t ver = *(uint8_t *)(u + 0x1bd); if (ver < 1 || ver > 3) return 0;
    uint64_t t = *(uint64_t *)(b + 0x2d64a18 + (uint64_t)ver * 16); uint32_t cls = *(uint32_t *)(u + 4); if (!t || cls >= *(uint64_t *)(t + 0x1550)) return 0;
    return *(uint8_t *)(*(uint64_t *)(t + 0x1548) + (uint64_t)cls * 0x168 + 0x15e) == 16; }
static struct { uint32_t id; uint64_t lv, at; int x, y, tries; } g_tile_retry;
static uint64_t w_level(uint64_t pl) { uint64_t pp = pl ? *(uint64_t *)(pl + 0x38) : 0, r1 = pp ? *(uint64_t *)(pp + 0x20) : 0, r2 = r1 ? *(uint64_t *)(r1 + 0x18) : 0; return r2 ? *(uint64_t *)(r2 + 0x90) : 0; }
static void w_tile_retry_step(void) {
    if (!g_tile_retry.at || dm_guest_frame() < g_tile_retry.at) return; g_tile_retry.at = 0; uint8_t *b = wb(); uint64_t pl = w_player(); int px = 0, py = 0;
    if (!b || !pl || g_go.on || w_level(pl) != g_tile_retry.lv || !w_unit_xy(pl, &px, &py) || abs(px - g_tile_retry.x) > W_FAR || abs(py - g_tile_retry.y) > W_FAR) return;
    uint64_t act = 0; long bd = -1; int ax = 0, ay = 0, ox = g_tile_retry.x, oy = g_tile_retry.y;
    for (int k = 0; k < 128; k++) for (uint64_t c = *(uint64_t *)(b + 0x2d603a0 + 2 * 0x400 + k * 8); c; c = *(uint64_t *)(c + 0x158)) { int cx = 0, cy = 0;
        if (!w_unit_xy(c, &cx, &cy) || abs(cx - ox) > 2 || abs(cy - oy) > 2) continue; if (!(((int (*)(uint64_t, int, int, int, int))(b + 0x464a70))(c, 0, 0, -1, -1) & 1) || ((int (*)(int, uint64_t, uint64_t))(b + 0x2f7b7c))(0x165, pl, c) != 2) continue;
        long d = (long)(cx - ox) * (cx - ox) + (long)(cy - oy) * (cy - oy); if (bd < 0 || d < bd) { bd = d; act = c; ax = cx; ay = cy; } }


    if (!act) { uint64_t tu = w_find(5, g_tile_retry.id); if (!tu || g_tile_retry.tries >= 2) { LOG("世界目标：出口砖 id %u 交互 %d 次后仍在原关卡，2 格内没有可交互的物件，不再试", g_tile_retry.id, g_tile_retry.tries + 1); return; }
        g_tile_retry.tries++; g_tile_retry.at = dm_guest_frame() + 45; int r = w_interact(tu); LOG("世界目标：出口砖 id %u 交互后 45 帧仍在原关卡，旁边没有把守物件 → 再交互一次（第 %d 次补发）→ 交互 %d", g_tile_retry.id, g_tile_retry.tries, r); return; }
    int r = w_interact(act); LOG("世界目标：出口砖 id %u 交互后 45 帧仍在原关卡 → 改交互把守它的物件 cls %u id %u（%d,%d）→ 交互 %d", g_tile_retry.id, *(uint32_t *)(act + 4), *(uint32_t *)(act + 8), ax, ay, r);
}


static int w_arrive(uint64_t pl, uint64_t u, int px, int py, int ox, int oy, const char *how) {
    static const int off[4][2] = {{0, 3}, {0, -3}, {-3, 0}, {3, 0}}; int ok = g_go.stop_at || !w_needs_room(u) || w_same_room(pl, u);
    if (!ok && g_go.alt < 4) { g_go.ax = ox + off[g_go.alt][0]; g_go.ay = oy + off[g_go.alt][1]; g_go.alt++; g_go.alt_on = 1; g_go.n = g_go.i = g_go.replans = g_go.empty = g_go.direct = 0; g_go.next_fr = 0; g_go.still_fr = 0; g_go.until_ns = w_now_ns() + 8000000000ull;
        LOG("世界目标：类型 %u id %u 在（玩家 %d,%d 目标 %d,%d）和物件不在同一房间，换到 (%d,%d) 再试（第 %d 侧）", g_go.type, g_go.id, px, py, ox, oy, g_go.ax, g_go.ay, g_go.alt); return 0; }




    uint64_t act = u; uint8_t *b = wb();
    if (!g_go.stop_at && b && *(uint32_t *)u == 5) { uint64_t nt = ((uint64_t (*)(uint64_t, uint32_t))(b + 0x2f7a0c))(((uint64_t (*)(void))(b + 0x2acecc))(), *(uint32_t *)(b + 0x2d20e4c)); int nx = 0, ny = 0;
        if (nt && nt != u && *(uint32_t *)nt == 2 && w_unit_xy(nt, &nx, &ny) && abs(nx - ox) <= 3 && abs(ny - oy) <= 3) act = nt;




        if (act == u) { g_tile_retry.id = g_go.id; g_tile_retry.lv = w_level(pl); g_tile_retry.at = dm_guest_frame() + 45; g_tile_retry.x = ox; g_tile_retry.y = oy; g_tile_retry.tries = 0; }
        if (act != u) LOG("世界目标：出口砖 id %u 旁有把守它的物件 cls %u id %u（%d,%d，游戏自己选中的）→ 交互它", g_go.id, *(uint32_t *)(act + 4), *(uint32_t *)(act + 8), nx, ny); }
    g_go.on = 0; int r = w_interact(act); LOG("世界目标：类型 %u id %u %s（玩家 %d,%d 目标 %d,%d%s）→ 交互 %d", g_go.type, g_go.id, how, px, py, ox, oy, ok ? "" : "，四侧都进不了同一房间", r); return 1;
}
#if TARGET_OS_OSX












static _Atomic uint64_t g_walk;
void dm_world_walk(float nx, float ny) { if (nx < 0) nx = 0; if (nx > 1) nx = 1; if (ny < 0) ny = 0; if (ny > 1) ny = 1; atomic_store(&g_walk, 1ull << 32 | (uint64_t)(uint32_t)(nx * 65535) << 16 | (uint32_t)(ny * 65535)); }
int dm_world_walking(void) { return g_go.on && g_go.type == W_GROUND; }
static void w_ground_end(const char *how, int px, int py) { g_go.on = 0; static int lg; if (lg++ < 400) LOG("点地面走：%s（玩家 %d,%d 目标 %d,%d）", how, px, py, g_gnd_x, g_gnd_y); }
static void w_ground_step(uint8_t *b, uint64_t pl) {
    uint64_t dm_stick_last_ns(void); uint32_t dm_cur_tick(void); int px = 0, py = 0;
    if (!pl || !w_unit_xy(pl, &px, &py)) { g_go.on = 0; return; }
    if (dm_stick_last_ns() != g_go.stick) { w_ground_end("取消（摇杆 / W A S D 动了）", px, py); return; }
    if (w_now_ns() > g_go.until_ns) { w_ground_end("停下（15 秒没到）", px, py); return; }
    int x = g_gnd_x, y = g_gnd_y, dx = abs(x - px), dy = abs(y - py); uint64_t f = dm_cur_tick(), fr = dm_guest_frame();
    if (dx <= 1 && dy <= 1) { w_ground_end("到点", px, py); return; }
    if (px != g_go.lx || py != g_go.ly || !g_go.still_fr) { g_go.lx = px; g_go.ly = py; g_go.still_fr = f; }
    else if (g_go.next_fr && f - g_go.still_fr > 25) {
        if (g_go.direct || g_go.replans++ >= 1) { w_ground_end(g_go.nearest ? "停在能到的最近处" : "停下（走不动了）", px, py); return; }
        g_go.n = 0; g_go.next_fr = 0; g_go.still_fr = f; }
    if (!g_go.direct && !g_go.n) { if (!w_plan(b, pl, x, y)) { w_ground_end("不走（找不到路）", px, py); return; } g_go.next_fr = 0; }
    while (g_go.i < g_go.n && abs((int)g_go.node[g_go.i][0] - px) <= 1 && abs((int)g_go.node[g_go.i][1] - py) <= 1) { g_go.i++; g_go.next_fr = 0; }
    int gx, gy;
    if (g_go.i >= g_go.n) {
        if (g_go.nearest || !wp_line(px, py, x, y)) { w_ground_end(g_go.nearest ? "停在能到的最近处" : "到点（目标格旁）", px, py); return; }
        if (g_go.direct && fr < g_go.next_fr) return; g_go.direct = 1; gx = x; gy = y; }
    else { gx = g_go.node[g_go.i][0]; gy = g_go.node[g_go.i][1]; }
    if (fr < g_go.next_fr) return; g_go.next_fr = fr + 20;
    atomic_store(&g_dm_move_allow_until_ns, w_now_ns() + 500000000ull);
    ((void (*)(int, uint64_t, int, int))(b + 0x4ad870))(3, pl, gx, gy);
}
#endif
static void w_go_step(void) {
    if (!g_go.on) return; uint64_t dm_stick_last_ns(void); uint8_t *b = wb(); uint64_t pl = w_player(), u = pl ? w_find(g_go.type, g_go.id) : 0; int px, py, x, y;
#if TARGET_OS_OSX
    if (g_go.type == W_GROUND) { w_ground_step(b, pl); return; }
#endif
    const char *why = !u ? "目标没了" : w_now_ns() > g_go.until_ns ? "15 秒没到" : dm_stick_last_ns() != g_go.stick ? "摇杆动了" : !w_unit_xy(pl, &px, &py) || !w_unit_xy(u, &x, &y) ? "读不到坐标" : (g_go.type == 4 && *(uint32_t *)(u + 0xc) != 3) ? "物品已不在地上" : NULL;
    if (why) { g_go.on = 0; LOG("世界目标：类型 %u id %u 取消（%s）", g_go.type, g_go.id, why); return; }
    int ox = x, oy = y; if (g_go.alt_on) { x = g_go.ax; y = g_go.ay; }
    int dx = x - px, dy = y - py; if (dx < 0) dx = -dx; if (dy < 0) dy = -dy;


    { uint32_t dm_cur_tick(void); uint64_t f = dm_cur_tick(); if (px != g_go.lx || py != g_go.ly || !g_go.still_fr) { g_go.lx = px; g_go.ly = py; g_go.still_fr = f; } else if (g_go.next_fr && f - g_go.still_fr > 25) { if (!g_go.stop_at && dx <= W_FAR && dy <= W_FAR) { w_arrive(pl, u, px, py, ox, oy, "走不进 2 格，就地"); return; } if (g_go.replans++ < 3) { g_go.n = 0; g_go.next_fr = 0; g_go.still_fr = f; } else if (g_go.alt_on) { w_arrive(pl, u, px, py, ox, oy, "换侧走不到，就地"); return; } else { g_go.on = 0; LOG("世界目标：类型 %u id %u 取消（走不过去，停在 %d,%d）", g_go.type, g_go.id, px, py); return; } } }
    if (g_go.stop_at > 0 && dx <= g_go.stop_at && dy <= g_go.stop_at) { g_go.on = 0; ((void (*)(int, uint64_t, int, int))(b + 0x4ad870))(3, pl, px, py); LOG("世界目标：类型 %u id %u 走到旁边（玩家 %d,%d 目标 %d,%d），不交互", g_go.type, g_go.id, px, py, x, y); return; }




    int near = g_go.type == 5 ? 1 : W_NEAR;
    if (!g_go.stop_at && dx <= near && dy <= near) { w_arrive(pl, u, px, py, ox, oy, "到跟前"); return; }
    if (!g_go.stop_at && g_go.type == 5 && dx <= W_NEAR && dy <= W_NEAR && !g_go.direct) { g_go.n = g_go.i = 0; g_go.direct = 1; g_go.still_fr = 0; g_go.next_fr = dm_guest_frame() + 20; atomic_store(&g_dm_move_allow_until_ns, w_now_ns() + 500000000ull); ((void (*)(int, uint64_t, int, int))(b + 0x4ad870))(3, pl, x, y); return; }
    if (!g_go.stop_at && g_go.type == 5 && g_go.direct) return;


    if (!g_go.direct && (!g_go.n || abs(x - g_go.tx) > 6 || abs(y - g_go.ty) > 6)) { w_plan(b, pl, x, y); g_go.next_fr = 0; }
    while (g_go.i < g_go.n && abs((int)g_go.node[g_go.i][0] - px) <= 1 && abs((int)g_go.node[g_go.i][1] - py) <= 1) { g_go.i++; g_go.next_fr = 0; }


    if (g_go.i >= g_go.n) {
        if (g_go.n && g_go.empty++ < 8) { g_go.n = 0; g_go.next_fr = 0; return; }




        if (!g_go.stop_at && dx <= W_FAR && dy <= W_FAR) { if (!g_go.direct) { g_go.direct = 1; g_go.still_fr = 0; g_go.next_fr = dm_guest_frame() + 20; atomic_store(&g_dm_move_allow_until_ns, w_now_ns() + 500000000ull); ((void (*)(int, uint64_t, int, int))(b + 0x4ad870))(3, pl, x, y); } return; }
        if (g_go.alt_on) { w_arrive(pl, u, px, py, ox, oy, "换侧走不到，就地"); return; }
        if (g_go.nearest && !g_go.stop_at && g_go.type == 2 && dx <= 8 && dy <= 8) { w_arrive(pl, u, px, py, ox, oy, "已在能到的最近处，就地"); return; }
        g_go.on = 0;
        LOG("世界目标：类型 %u id %u 取消（找不到路，停在 %d,%d，目标 %d,%d）", g_go.type, g_go.id, px, py, x, y);
        return; }
    g_go.empty = 0; int gx = g_go.node[g_go.i][0], gy = g_go.node[g_go.i][1];
    uint64_t fr = dm_guest_frame(); if (fr < g_go.next_fr) return; g_go.next_fr = fr + 20;
    atomic_store(&g_dm_move_allow_until_ns, w_now_ns() + 500000000ull);
    ((void (*)(int, uint64_t, int, int))(b + 0x4ad870))(3, pl, gx, gy);
}


















int dm_cam_on(void); int dm_cam_tap_for_unit(float tx, float ty, float fx, float fy, float *ox, float *oy); int dm_cam_unit_to_screen(float fx, float fy, float hx, float hy, float *ox, float *oy);





int dm_ui_proj_gate(void);
static int w_cam_convert(void) { return dm_cam_on() && !dm_ui_proj_gate(); }
static int w_project(uint8_t *b, uint64_t scene, uint64_t u, float *out, int top) { int r = ((int (*)(uint64_t, uint64_t, float *, int))(b + 0x90d110))(scene, u, out, top); if (r) out[1] += dm_ui_proj_dy(); return r; }
static int w_in_rect(float px, float py, float cx, float cy, float w, float h, float min, float *d) {
    if (w < min) w = min; if (h < min) h = min; float dx = px - cx, dy = py - cy; if (dx < 0) dx = -dx; if (dy < 0) dy = -dy;
    if (dx > w / 2 || dy > h / 2) return 0; *d = sqrtf(dx * dx + dy * dy); return 1;
}




static int w_sprite_box(uint8_t *b, uint64_t u, int out[4]) {
    uint64_t tb = *(uint64_t *)(u + 0x78); if (!tb) return 0; uint64_t arr = *(uint64_t *)(tb + 0x28), cnt = *(uint64_t *)(tb + 0x30); if (!arr || !cnt || cnt > 4096) return 0;
    uint32_t type = *(uint32_t *)u, cls = *(uint32_t *)(u + 4), mode = (type <= 1 && *(uint64_t *)(u + 0x40)) ? *(uint32_t *)(u + 0x58) : *(uint32_t *)(u + 0xc);
    ((void (*)(uint64_t, uint32_t *, uint32_t *, uint32_t *, uint64_t, int))(b + 0x5b23f0))(u, &type, &cls, &mode, (uint64_t)(uintptr_t)(b + 0x1d5a900), 0);
    for (uint64_t i = 0; i < cnt; i++) { uint64_t e = arr + i * 16; if (*(uint32_t *)e != mode) continue; uint64_t q = *(uint64_t *)(e + 8), r = q ? *(uint64_t *)(q + 0x18) : 0; if (!r) return 0;
        out[0] = *(int32_t *)(r + 8); out[1] = *(int32_t *)(r + 0xc); out[2] = *(int32_t *)(r + 0x10); out[3] = *(int32_t *)(r + 0x14); return out[1] > out[0] && out[3] > out[2] && out[1] - out[0] < 2000 && out[3] - out[2] < 2000; }
    return 0;
}
static uint64_t w_hit(uint8_t *b, float sx, float sy, float min, float *dist, int *by_label) {
    uint64_t pl = w_player(), scene = *(uint64_t *)(b + 0x2e2a698), best = 0; float bd = 1e9f, d; int brank = 2; *by_label = 0; if (!pl || !scene) return 0;
    int RWi = 0, RHi = 0; dm_screen_real(&RWi, &RHi); float RW = RWi > 0 ? RWi : 1, RH = RHi > 0 ? RHi : 1; int cam = w_cam_convert();
    int persp = !cam && dm_cam_on();
    static int dumps; int dump = getenv("DM_WORLD_DUMP") && dumps++ < 3;
    uint64_t L = (uint64_t)(uintptr_t)(b + 0x2f0fee0), rec = *(uint64_t *)L, n = *(uint64_t *)(L + 8);


    for (uint64_t k = 0; rec && k < n && k < 512; k++) { int32_t *r = (int32_t *)(rec + k * 0x148); if (r[2] <= 0 || r[3] <= 0 || r[4] == -1) continue;
        if (dump) LOG("世界目标：标签 物品 id %d 矩形 (%d,%d %dx%d)", r[4], r[0], r[1], r[2], r[3]);
        if (w_in_rect(sx, sy, r[0] + r[2] / 2.0f, r[1] + dm_ui_proj_dy() + r[3] / 2.0f, (float)r[2], (float)r[3], min, &d) && d < bd) { uint64_t u = w_find(4, (uint32_t)r[4]); if (u && *(uint32_t *)(u + 0xc) == 3) { bd = d; best = u; brank = 0; *by_label = 1; } } }
    for (int t = 0; t <= 5; t++) { if (t == 3) continue;
        for (int k = 0; k < 128; k++) for (uint64_t u = *(uint64_t *)(b + 0x2d603a0 + t * 0x400 + k * 8); u; u = *(uint64_t *)(u + 0x158)) {
            if (t == 4 && *(uint32_t *)(u + 0xc) != 3) continue;
            if (!(((int (*)(uint64_t, int, int, int, int))(b + 0x464a70))(u, 0, 0, -1, -1) & 1)) continue;
            if (t == 0 && u == pl) continue;
            int cat = ((int (*)(int, uint64_t, uint64_t))(b + 0x2f7b7c))(0x165, pl, u); if (t == 0 ? cat != 9 : t == 1 ? (cat != 5 && !W_MON_OK(cat)) : t == 2 ? cat != 2 : 0) continue;
            float f[2], h[2]; if (!w_project(b, scene, u, f, 0)) continue;
            if (t == 4 || !w_project(b, scene, u, h, 1)) { h[0] = f[0]; h[1] = f[1]; }






            if (persp) { float bx = (f[0] + h[0]) / 2, by = (f[1] + h[1]) / 2; if (bx < -RW / 4 || bx > RW * 1.25f || by < -RH / 4 || by > RH * 1.25f || fabsf(f[1] - h[1]) > 2 * RH) continue; }


            float csx = sx, csy = sy; if (cam) { float ox, oy; if (!dm_cam_tap_for_unit(sx / RW, sy / RH, f[0] / RW, f[1] / RH, &ox, &oy)) continue; csx = ox * RW; csy = oy * RH; }
            float hh = fabsf(f[1] - h[1]), ww = hh * (t == 1 ? 0.44f : 1.0f), cx = (f[0] + h[0]) / 2, cy = (f[1] + h[1]) / 2, dhw = 0, dhv = 0, sbx = 0, sby = 0, sbw = 0, sbh = 0;
            if (t == 0) { cx = f[0]; cy = f[1]; ww = hh; hh = 0; }
            if (t == 2) {




                uint64_t dd = *(uint64_t *)(u + 0x10), rec = dd ? *(uint64_t *)dd : 0; uint32_t szx = rec ? *(uint32_t *)(rec + 0x90) : 0, szy = rec ? *(uint32_t *)(rec + 0x94) : 0;
                if (szx >= 1 && szx <= 32 && szy >= 1 && szy <= 32) { float n = (szx + szy) / 2.0f + 2, k = min / 0.10f * 0.00206f; dhw = n * 16 * k; dhv = n * 8 * k; }










                if (rec) { int32_t *ri = (int32_t *)rec; float k = min / 0.10f * 0.00206f; int sb[4];
                    if (ri[85] > 0 && ri[86] > 0 && ri[85] <= 400 && ri[86] <= 400 && abs(ri[83]) <= 400 && abs(ri[84]) <= 400) { sbw = ri[85] * k; sbh = ri[86] * k; sbx = f[0] + ri[83] * k + sbw / 2; sby = f[1] + ri[84] * k + sbh / 2; }
                    else if (w_sprite_box(b, u, sb)) { int l = sb[0] - 16, r = sb[1] + 16, tp = sb[2] - 16, bt = sb[3] + 16; if (l < -40) l = -40; if (r > 40) r = 40; if (tp < -110) tp = -110;
                        if (r > l) { sbw = (r - l) * k; sbh = (bt - tp) * k; sbx = f[0] + (l + r) / 2.0f * k; sby = f[1] + (tp + bt) / 2.0f * k; } } } }
            if (dump) { int sb[4] = {0, 0, 0, 0}; int okb = w_sprite_box(b, u, sb); LOG("世界目标：  [诊断] 类型 %d cls %u 游戏图形范围 %s 左 %d 右 %d 上 %d 下 %d（经典像素）", t, *(uint32_t *)(u + 4), okb ? "有" : "没有", sb[0], sb[1], sb[2], sb[3]); }
            if (dump) LOG("世界目标：候选 类型 %d cls %u id %u 身体框 中心 (%.0f,%.0f) %.0fx%.0f（不足 %.0f 的边补到 %.0f）；占地菱形 中心 (%.0f,%.0f) 半宽 %.0f 半高 %.0f；选择框 中心 (%.0f,%.0f) %.0fx%.0f", t, *(uint32_t *)(u + 4), *(uint32_t *)(u + 8), cx, cy, ww, hh, min, min, f[0], f[1], dhw, dhv, sbx, sby, sbw, sbh);
            int in = w_in_rect(csx, csy, cx, cy, ww, hh, min, &d);
            if (!in && dhw > 0) { float ax = fabsf(csx - f[0]), ay = fabsf(csy - f[1]); if (ax / dhw + ay / dhv <= 1) { in = 1; d = sqrtf(ax * ax + ay * ay); } }
            int rank = 0; if (!in && sbw > 0) { in = w_in_rect(csx, csy, sbx, sby, sbw, sbh, min, &d); rank = 1; }
            if (in && (rank < brank || (rank == brank && d < bd))) { bd = d; best = u; brank = rank; *by_label = 0; } } }
    *dist = best ? bd : 0; return best;
}


static _Atomic uint64_t g_tap; static _Atomic unsigned g_tap_seq, g_tap_done; static _Atomic int g_tap_res, g_has_target;
unsigned dm_world_tap(float nx, float ny) { if (nx < 0) nx = 0; if (nx > 1) nx = 1; if (ny < 0) ny = 0; if (ny > 1) ny = 1;
    unsigned seq = atomic_fetch_add(&g_tap_seq, 1) + 1; atomic_store(&g_tap, (uint64_t)seq << 32 | (uint64_t)(uint32_t)(nx * 65535) << 16 | (uint32_t)(ny * 65535)); return seq; }
int dm_world_tap_result(unsigned seq) { return atomic_load(&g_tap_done) == seq ? atomic_load(&g_tap_res) : -1; }
int dm_world_has_target(void) { return atomic_load(&g_has_target); }
#if TARGET_OS_OSX
static int w_hostile(uint8_t *b, uint64_t u) { if (*(uint32_t *)u != 1) return 0; uint64_t pl = w_player(); int c = pl ? ((int (*)(int, uint64_t, uint64_t))(b + 0x2f7b7c))(0x165, pl, u) : 0; return c == 1 || c == 0xa; }
static uint64_t g_mouse_attack_at;
static void w_attack(uint64_t u, const char *how) { g_mouse_attack_at = w_now_ns(); if (g_go.on) g_go.on = 0; int r = w_interact(u); static int lg; if (lg++ < 300) LOG("鼠标攻击：%s 类型 1 cls %u id %u → 点击入口 %d", how, *(uint32_t *)(u + 4), *(uint32_t *)(u + 8), r); }
static void w_walk_step(uint8_t *b) {
    uint64_t t = atomic_exchange(&g_walk, 0); if (!t) return; uint64_t dm_stick_last_ns(void); uint64_t pl = w_player(), scene = *(uint64_t *)(b + 0x2e2a698); int W = 0, H = 0, px = 0, py = 0; float f[2];
    dm_screen_real(&W, &H); if (!pl || !scene || W <= 0 || H <= 0 || !w_unit_xy(pl, &px, &py) || !w_project(b, scene, pl, f, 0)) return;
    { float d0 = 0; int lab0 = 0; g_hit_mon = 1; uint64_t mu = w_hit(b, ((t >> 16) & 0xffff) / 65535.0f * W, (t & 0xffff) / 65535.0f * H, H * 0.10f, &d0, &lab0); g_hit_mon = 0;
      if (mu && w_hostile(b, mu)) { if (w_now_ns() - g_gnd_stick_at >= 300000000ull && w_now_ns() - g_mouse_attack_at >= 200000000ull) w_attack(mu, "按住"); return; } }
    if (g_go.on && g_go.type != W_GROUND) return;
    if (w_now_ns() - g_gnd_stick_at < 300000000ull) return;
    float sx = ((t >> 16) & 0xffff) / 65535.0f * W, sy = (t & 0xffff) / 65535.0f * H, k = H * 0.00206f, a = (sx - f[0]) / k / 16, c = (sy - f[1]) / k / 8;
    int x = px + (int)lroundf((a + c) / 2), y = py + (int)lroundf((c - a) / 2);
    if (getenv("DM_TEST_WALK_CAL")) { static int n; if (n++ < 3)
        for (int ty = 1; ty <= 2; ty++) for (int kk = 0; kk < 128; kk++) for (uint64_t u = *(uint64_t *)(b + 0x2d603a0 + ty * 0x400 + kk * 8); u; u = *(uint64_t *)(u + 0x158)) { float g[2]; int ux, uy;
            if (!w_unit_xy(u, &ux, &uy) || !w_project(b, scene, u, g, 0) || g[0] < 0 || g[1] < 0 || g[0] > W || g[1] > H) continue; float a2 = (g[0] - f[0]) / k / 16, c2 = (g[1] - f[1]) / k / 8;
            LOG("点地面走：[校] 类型 %d cls %u 脚底 (%.0f,%.0f) 真格 %d,%d 反算 %d,%d 差 %d,%d（离人 %d,%d 格）", ty, *(uint32_t *)(u + 4), g[0], g[1], ux, uy, px + (int)lroundf((a2 + c2) / 2), py + (int)lroundf((c2 - a2) / 2), px + (int)lroundf((a2 + c2) / 2) - ux, py + (int)lroundf((c2 - a2) / 2) - uy, ux - px, uy - py); } }
    if (g_go.on && abs(x - g_gnd_x) <= 1 && abs(y - g_gnd_y) <= 1) return;
    static int lg; if (lg++ < 400) LOG("点地面走：点 (%.0f,%.0f)，人脚底在 (%.0f,%.0f) → 格 %d,%d（玩家 %d,%d）", sx, sy, f[0], f[1], x, y, px, py);
    g_go.type = W_GROUND; g_go.id = 0; g_go.nearest = 0; g_go.until_ns = w_now_ns() + 15000000000ull; g_go.stick = dm_stick_last_ns(); g_go.next_fr = 0; g_go.still_fr = 0;
    g_go.n = g_go.i = g_go.replans = g_go.empty = g_go.direct = g_go.alt = g_go.alt_on = 0; g_go.stop_at = 0; g_gnd_x = x; g_gnd_y = y; g_go.on = 1;
}
#endif
#if TARGET_OS_OSX






static _Atomic int g_tap_mouse;
unsigned dm_world_click(float nx, float ny) { atomic_store(&g_tap_mouse, 1); return dm_world_tap(nx, ny); }


static void w_mouse_cancel_pending(void) {
    if (dm_touch_host_world_active()) return;
    if (atomic_exchange(&g_tap_mouse, 0)) atomic_store(&g_tap, 0);
    atomic_store(&g_walk, 0);
}
#endif
static void w_tap_step(uint8_t *b) {
    uint64_t t = atomic_exchange(&g_tap, 0); if (!t) return; unsigned seq = (unsigned)(t >> 32); int res = 0;
    int W = 0, H = 0; dm_screen_real(&W, &H);
    float sx = ((t >> 16) & 0xffff) / 65535.0f * W, sy = (t & 0xffff) / 65535.0f * H, d = 0; int lab = 0;
#if TARGET_OS_OSX
    g_hit_mon = atomic_exchange(&g_tap_mouse, 0);
#endif
    uint64_t u = W > 0 && H > 0 ? w_hit(b, sx, sy, H * 0.10f, &d, &lab) : 0;
#if TARGET_OS_OSX
    g_hit_mon = 0;
    if (u && w_hostile(b, u)) { w_attack(u, "点"); u = 0; res = 6; }
#endif
    if (u) { uint32_t ty = *(uint32_t *)u; res = ty == 4 ? 1 : ty == 1 ? 2 : ty == 2 ? 3 : ty == 0 ? 5 : 4; w_go(u); }
    static int logged; if (logged++ < 200) { if (u) LOG("世界目标：点 (%.0f,%.0f) → 类型 %u cls %u id %u（%s，离框中心 %.0f 像素）", sx, sy, *(uint32_t *)u, *(uint32_t *)(u + 4), *(uint32_t *)(u + 8), lab ? "名字标签" : "命中框", d); else if (res != 6) LOG("世界目标：点 (%.0f,%.0f) → 没有目标，不发令", sx, sy); }
    atomic_store(&g_tap_res, res); atomic_store(&g_tap_done, seq);
}




static _Atomic uint32_t g_watch; static _Atomic uint64_t g_watch_pos;
void dm_world_watch(int type, uint32_t cls) { atomic_store(&g_watch_pos, 0); atomic_store(&g_watch, type < 0 ? 0 : 0x80000000u | (uint32_t)type << 24 | (cls & 0xffffff)); }
int dm_world_watch_pos(float *nx, float *ny) { uint64_t v = atomic_load(&g_watch_pos); if (!v) return 0; *nx = ((v >> 16) & 0xffff) / 65535.0f; *ny = (v & 0xffff) / 65535.0f; return 1; }
static void w_watch_step(uint8_t *b) {
    uint32_t w = atomic_load(&g_watch); if (!w) return; int t = (w >> 24) & 0x7f; uint32_t cls = w & 0xffffff; uint64_t scene = *(uint64_t *)(b + 0x2e2a698), v = 0;
    int W = 0, H = 0; dm_screen_real(&W, &H);
    for (int k = 0; scene && W > 0 && H > 0 && t <= 5 && k < 128 && !v; k++) for (uint64_t u = *(uint64_t *)(b + 0x2d603a0 + t * 0x400 + k * 8); u; u = *(uint64_t *)(u + 0x158)) {
#if TARGET_OS_OSX
        if (cls == 0xfffffe) { if (!(((int (*)(uint64_t, int, int, int, int))(b + 0x464a70))(u, 0, 0, -1, -1) & 1) || !w_hostile(b, u)) continue; } else
#endif
        if (cls != 0xffffff ? *(uint32_t *)(u + 4) != cls : (t == 4 && *(uint32_t *)(u + 0xc) != 3)) continue;
        float f[2], h[2]; if (!w_project(b, scene, u, f, 0)) continue;
        if (!w_project(b, scene, u, h, 1)) { h[0] = f[0]; h[1] = f[1]; }
        float cx = (f[0] + h[0]) / 2 / W, cy = (f[1] + h[1]) / 2 / H;
        if (w_cam_convert()) { float ox, oy; if (!dm_cam_unit_to_screen(f[0] / W, f[1] / H, h[0] / W, h[1] / H, &ox, &oy)) continue; cx = ox; cy = oy; }
        if (cx < 0 || cx > 1 || cy < 0 || cy > 1) continue;
        v = 1ull << 32 | (uint64_t)(uint32_t)(cx * 65535) << 16 | (uint32_t)(cy * 65535); break; }
    atomic_store(&g_watch_pos, v);
}




static void w_test_xy_step(void) {
    static struct { uint32_t at; int x, y, st; uint64_t until; } q[12]; static int n = -1, cur = -1; static uint32_t lt[8]; static int nl = -1; uint8_t *b = wb(); uint64_t pl = w_player(); if (!b || !pl) return;
    if (n < 0) { n = 0; const char *e = getenv("DM_TEST_WORLD_XY"); char buf[400]; if (e) { strlcpy(buf, e, sizeof buf); for (char *sv, *t = strtok_r(buf, ";", &sv); t && n < 12; t = strtok_r(NULL, ";", &sv)) if (sscanf(t, "%u:%d,%d", &q[n].at, &q[n].x, &q[n].y) == 3) n++; } }
    if (nl < 0) { nl = 0; const char *e = getenv("DM_TEST_WORLD_LIST"); char buf[100]; if (e) { strlcpy(buf, e, sizeof buf); for (char *sv, *t = strtok_r(buf, ",", &sv); t && nl < 8; t = strtok_r(NULL, ",", &sv)) lt[nl++] = (uint32_t)atoi(t); } }
    uint32_t dm_cur_tick(void); uint32_t tk = dm_cur_tick(); int px = 0, py = 0; w_unit_xy(pl, &px, &py);
    for (int i = 0; i < nl; i++) if (lt[i] && tk >= lt[i]) { lt[i] = 0; LOG("单位列表（拍 %u，玩家 %d,%d）：", tk, px, py);
        { const char *e = getenv("DM_TEST_OBJREC"); char buf[80]; if (e) { strlcpy(buf, e, sizeof buf); uint64_t t = *(uint64_t *)(b + 0x2d64a18 + (uint64_t)*(uint8_t *)(pl + 0x1bd) * 16);
            for (char *sv, *tkn = strtok_r(buf, ",", &sv); tkn && t; tkn = strtok_r(NULL, ",", &sv)) { uint32_t c = (uint32_t)atoi(tkn); if (c >= *(uint64_t *)(t + 0x1550)) continue; int32_t *r = (int32_t *)(*(uint64_t *)(t + 0x1548) + (uint64_t)c * 0x168); char o[1400]; int k = 0;
                for (int q = 0; q < 0x168 / 4; q++) k += snprintf(o + k, sizeof o - k, " %d", r[q]); if (k >= (int)sizeof o) k = (int)sizeof o - 1; LOG("物件表记录 cls %u（每格 4 字节，十进制）：%s", c, o); } } }
        for (int gt = 1; gt <= 5; gt += gt == 2 ? 3 : 1) { char o[1600]; int c2 = 0, cnt = 0; o[0] = 0; for (int k = 0; k < 128; k++) for (uint64_t c = *(uint64_t *)(b + 0x2d603a0 + gt * 0x400 + k * 8); c && cnt < 60; c = *(uint64_t *)(c + 0x158)) { int x = 0, y = 0; w_unit_xy(c, &x, &y); int sel = ((int (*)(uint64_t, int, int, int, int))(b + 0x464a70))(c, 0, 0, -1, -1) & 1, cat = ((int (*)(int, uint64_t, uint64_t))(b + 0x2f7b7c))(0x165, pl, c);
            c2 += snprintf(o + c2, sizeof o - c2, " c%u/模式%u%s(%d,%d)", *(uint32_t *)(c + 4), *(uint32_t *)(c + 0xc), !sel ? "/不可选" : cat == 2 ? "/物" : cat == 4 ? "/出口" : cat == 5 ? "/人" : cat ? "/别" : "/无类", x, y); if (c2 >= (int)sizeof o) c2 = (int)sizeof o - 1; cnt++; } LOG("  类型 %d：%s", gt, o); } }
    if (cur < 0) { for (int i = 0; i < n; i++) if (!q[i].st && tk >= q[i].at) { cur = i; q[i].st = 1; q[i].until = w_now_ns() + 12000000000ull; g_go.on = 0; g_go.n = g_go.i = 0; LOG("按坐标走：→ %d,%d（从 %d,%d）", q[i].x, q[i].y, px, py); break; } if (cur < 0) return; }
    int dx = abs(q[cur].x - px), dy = abs(q[cur].y - py); static uint64_t nf;
    if ((dx <= 2 && dy <= 2) || w_now_ns() > q[cur].until) { LOG("按坐标走：%s，停在 %d,%d（目标 %d,%d）", dx <= 2 && dy <= 2 ? "到了" : "12 秒没到", px, py, q[cur].x, q[cur].y); q[cur].st = 2; cur = -1; return; }
    while (g_go.i < g_go.n && abs((int)g_go.node[g_go.i][0] - px) <= 1 && abs((int)g_go.node[g_go.i][1] - py) <= 1) { g_go.i++; nf = 0; }
    uint64_t fr = dm_guest_frame(); if (fr < nf) return; nf = fr + 20;
    if (g_go.i >= g_go.n && !w_plan(b, pl, q[cur].x, q[cur].y)) return;
    atomic_store(&g_dm_move_allow_until_ns, w_now_ns() + 500000000ull); ((void (*)(int, uint64_t, int, int))(b + 0x4ad870))(3, pl, g_go.node[g_go.i][0], g_go.node[g_go.i][1]);
}












static void w_test_seek_step(void) {
    extern _Atomic int g_dm_seek_tp; extern int g_dm_seek_tp_x, g_dm_seek_tp_y;
    static struct { uint32_t at, type, cls, lvl; int st; } q[8]; static int n = -1, cur = -1; static uint32_t it[6]; static int ni = -1; static uint64_t lv0, settle;
    uint8_t *b = wb(); uint64_t pl = w_player(); if (!b || !pl) return; uint32_t dm_cur_tick(void); uint32_t tk = dm_cur_tick();
    if (n < 0) { n = 0; const char *e = getenv("DM_TEST_WORLD_SEEK"); char buf[300]; if (e) { strlcpy(buf, e, sizeof buf); for (char *sv, *t = strtok_r(buf, ";", &sv); t && n < 8; t = strtok_r(NULL, ";", &sv)) { q[n].lvl = 0; q[n].st = 0; if (sscanf(t, "%u:%u:c%u:L%u", &q[n].at, &q[n].type, &q[n].cls, &q[n].lvl) >= 3 && q[n].type <= 5) n++; } } }
    if (ni < 0) { ni = 0; const char *e = getenv("DM_TEST_LEVEL_INFO"); char buf[80]; if (e) { strlcpy(buf, e, sizeof buf); for (char *sv, *t = strtok_r(buf, ",", &sv); t && ni < 6; t = strtok_r(NULL, ",", &sv)) it[ni++] = (uint32_t)atoi(t); } }
    if (!n && !ni) return;
    uint64_t pp = *(uint64_t *)(pl + 0x38), r1 = pp ? *(uint64_t *)(pp + 0x20) : 0, r2 = r1 ? *(uint64_t *)(r1 + 0x18) : 0, lv = r2 ? *(uint64_t *)(r2 + 0x90) : 0; int px = 0, py = 0; if (!lv || !w_unit_xy(pl, &px, &py)) return;
    for (int i = 0; i < ni; i++) if (it[i] && tk >= it[i]) { it[i] = 0; int cnt = 0, ld = 0, x0 = 1 << 30, y0 = 1 << 30, x1 = 0, y1 = 0;
        for (uint64_t r = *(uint64_t *)(lv + 0x10); r && cnt < 2000; r = *(uint64_t *)(r + 0x48)) { int x = *(int *)(r + 0x60) * 5, y = *(int *)(r + 0x64) * 5, w = *(int *)(r + 0x68) * 5, h = *(int *)(r + 0x6c) * 5; cnt++; ld += *(uint64_t *)(r + 0x58) != 0; if (x < x0) x0 = x; if (y < y0) y0 = y; if (x + w > x1) x1 = x + w; if (y + h > y1) y1 = y + h; }
        uint64_t am = *(uint64_t *)(lv + 0x1c8); LOG("关卡信息（拍 %u）：关卡号 %u，关卡房间 %d 间（已载入 %d），范围 (%d,%d)–(%d,%d)，玩家 %d,%d；真墓关卡号 %u", tk, *(uint32_t *)(lv + 0x1f8), cnt, ld, x0, y0, x1, y1, px, py, am ? *(uint32_t *)(am + 0x120) : 0); }
    if (cur < 0) { for (int i = 0; i < n; i++) if (!q[i].st && tk >= q[i].at && (!q[i].lvl || q[i].lvl == *(uint32_t *)(lv + 0x1f8))) { cur = i; q[i].st = 1; lv0 = lv; settle = 0; atomic_store(&g_dm_seek_tp, 1); LOG("探路：在关卡 %u 里找 类型 %u 类号 %u", *(uint32_t *)(lv + 0x1f8), q[i].type, q[i].cls); break; } if (cur < 0) return; }
    if (lv != lv0) { LOG("探路：关卡变了（%u），这一条结束", *(uint32_t *)(lv + 0x1f8)); q[cur].st = 2; cur = -1; atomic_store(&g_dm_seek_tp, 0); return; }
    for (int k = 0; k < 128; k++) for (uint64_t c = *(uint64_t *)(b + 0x2d603a0 + q[cur].type * 0x400 + k * 8); c; c = *(uint64_t *)(c + 0x158)) if (*(uint32_t *)(c + 4) == q[cur].cls) { int x = 0, y = 0; w_unit_xy(c, &x, &y);
        { uint32_t ty = *(uint32_t *)c; uint64_t cp = *(uint64_t *)(c + 0x38), cr = cp ? *(uint64_t *)(cp + (ty == 2 || ty == 4 || ty == 5 ? 0 : 0x20)) : 0, cd = cr ? *(uint64_t *)(cr + 0x18) : 0; if (cd && *(uint64_t *)(cd + 0x90) != lv) continue; }
        int s3 = atomic_load(&g_dm_seek_tp); if (s3 == 3) return;
        if (s3 == 1) { g_dm_seek_tp_x = x; g_dm_seek_tp_y = y; atomic_store(&g_dm_seek_tp, 3); LOG("探路：看见 类型 %u 类号 %u 在 %d,%d（玩家 %d,%d）→ 先传到它旁边", q[cur].type, q[cur].cls, x, y, px, py); return; }
        if (s3 == 2) { if (!settle) settle = dm_guest_frame() + 90; if (dm_guest_frame() < settle) return; }
        atomic_store(&g_dm_seek_tp, 0); { const char *na = getenv("DM_TEST_SEEK_NOACT"); int hit = 0; for (const char *t = na; t && *t; ) { if ((uint32_t)atoi(t) == q[cur].cls) hit = 1; t = strchr(t, ','); if (t) t++; }
          if (hit) { LOG("探路：找到了 类型 %u 类号 %u 在 %d,%d（玩家 %d,%d），DM_TEST_SEEK_NOACT：只到旁边不交互", q[cur].type, q[cur].cls, x, y, px, py); q[cur].st = 2; cur = -1; return; } }
        LOG("探路：找到了 类型 %u 类号 %u 在 %d,%d（玩家 %d,%d）→ 走过去交互", q[cur].type, q[cur].cls, x, y, px, py); w_go(c); q[cur].st = 2; cur = -1; return; }
    if (!atomic_load(&g_dm_seek_tp)) { LOG("探路：跳得到的房间都去过了，没找到 类型 %u 类号 %u", q[cur].type, q[cur].cls); q[cur].st = 2; cur = -1; }
}
_Atomic int g_dm_dur_sync_n; uint32_t g_dm_dur_sync[16][2];














_Atomic int g_dm_game_ver = -1;










#define MX_MAX 12
static struct { uint64_t lv; int n; int32_t x[MX_MAX], y[MX_MAX]; uint16_t art[MX_MAX]; } g_mx;
static void mx_add(int x, int y, int art) {
    for (int i = 0; i < g_mx.n; i++) if (g_mx.art[i] == art && abs(g_mx.x[i] - x) < 25 && abs(g_mx.y[i] - y) < 25) return;
    if (g_mx.n < MX_MAX) { g_mx.x[g_mx.n] = x; g_mx.y[g_mx.n] = y; g_mx.art[g_mx.n] = (uint16_t)art; g_mx.n++; }
}
static void w_map_mark_step(void) {


    static int env = -2; static uint64_t lv_seen, lv_done, at; if (env == -2) { const char *e = getenv("DM_MAP_MARK"); env = e ? e[0] != '0' : -1; }
    int dm_custom_get(const char *key); if (!(env >= 0 ? env : dm_custom_get("map_mark"))) return;
    uint8_t *b = wb(); uint64_t pl = b ? w_player() : 0, lv = pl ? w_level(pl) : 0; int ver = atomic_load(&g_dm_game_ver); if (!lv || ver < 0) return;
    if (lv != lv_seen) { lv_seen = lv; at = dm_guest_frame() + 30; return; } if (lv == lv_done || dm_guest_frame() < at) return; lv_done = lv;
    memset(&g_mx, 0, sizeof g_mx); g_mx.lv = lv;
    uint32_t lvno = *(uint32_t *)(lv + 0x1f8); if (lvno == 1 || lvno == 40 || lvno == 75 || lvno == 103 || lvno == 109) return;
    uint64_t map = *(uint64_t *)(lv + 0x1c8), cb = map ? *(uint64_t *)(map + 0x838) : 0; if (!cb) return;






    int total = 0, nwp = 0, nex = 0, ninit = 0, cnt[8] = {0}; uint32_t rare = 0; uint64_t t0 = w_now_ns();
    for (uint64_t r = *(uint64_t *)(lv + 0x10); r && total < 4096; r = *(uint64_t *)(r + 0x48), total++) { uint32_t fl = *(uint32_t *)(r + 0x50) >> 4 & 0xff; for (int i = 0; i < 8; i++) cnt[i] += fl >> i & 1; }
    for (int i = 0; i < 8; i++) if (cnt[i] && cnt[i] <= 8) rare |= 0x10u << i;
    total = 0;
    for (uint64_t r = *(uint64_t *)(lv + 0x10); r && total < 4096; r = *(uint64_t *)(r + 0x48), total++) { uint32_t fl = *(uint32_t *)(r + 0x50); int wp = (fl & 0x30000) != 0; if (!wp && !(fl & rare)) continue;
        ninit += !*(uint64_t *)(r + 0x58); ((void (*)(int, uint64_t))(b + 0x52dcc0))(ver, r); uint64_t ar = *(uint64_t *)(r + 0x58); if (!ar || (!wp && !*(uint64_t *)(r + 0x78))) continue;
        ((void (*)(uint64_t))(uintptr_t)cb)(ar); if (wp) nwp++; else nex++;
        int cx = *(int32_t *)(r + 0x60) * 5 + *(int32_t *)(r + 0x68) * 5 / 2, cy = *(int32_t *)(r + 0x64) * 5 + *(int32_t *)(r + 0x6c) * 5 / 2;
        if (wp) mx_add(cx, cy, DM_MG_WP);
        for (uint64_t e = *(uint64_t *)(r + 0x78), k = 0; e && k < 16; e = *(uint64_t *)(e + 8), k++) { uint64_t tr = *(uint64_t *)e, tl = tr ? *(uint64_t *)(tr + 0x90) : 0; uint32_t to = tl ? *(uint32_t *)(tl + 0x1f8) : 0;
            if (to && to != lvno && to < 256) mx_add(cx, cy, DM_MG_LVL + (int)to); } }
    double ms = (w_now_ns() - t0) / 1e6;
    if (getenv("DM_TEST_MAP_MARK_CHECK")) { int miss = 0, all = 0, k = 0; for (uint64_t r = *(uint64_t *)(lv + 0x10); r && k < 4096; r = *(uint64_t *)(r + 0x48), k++) { uint32_t fl = *(uint32_t *)(r + 0x50); if (!(fl & 0xff0) || (fl & 0x30000)) continue;
            ((void (*)(int, uint64_t))(b + 0x52dcc0))(ver, r); if (*(uint64_t *)(r + 0x78)) { all++; if (!(fl & rare)) { miss++; LOG("【红】小地图核对：关卡 %u 房间 (%d,%d) 标志 %#x 有出口链表却没被筛中（少见位 %#x）", lvno, *(int32_t *)(r + 0x60) * 5, *(int32_t *)(r + 0x64) * 5, fl, rare); } } }
        LOG("小地图核对：关卡 %u 逐间全筛有出口链表的房间 %d 间，筛法漏 %d 间（少见位 %#x，各位间数 %d/%d/%d/%d/%d/%d/%d/%d）", lvno, all, miss, rare, cnt[0], cnt[1], cnt[2], cnt[3], cnt[4], cnt[5], cnt[6], cnt[7]); }
    LOG("小地图：关卡 %u 进图标出 小站房间 %d 间、出入口房间 %d 间（共 %d 间房，提前初始化 %d 间，%.2f ms）", lvno, nwp, nex, total, ninit, ms);
    { char o[200] = ""; int k = 0; for (int i = 0; i < g_mx.n && k < (int)sizeof o - 24; i++) k += snprintf(o + k, sizeof o - k, " %s%d@%d,%d", g_mx.art[i] == DM_MG_WP ? "传送点" : "区域", g_mx.art[i] == DM_MG_WP ? 0 : g_mx.art[i] - DM_MG_LVL, g_mx.x[i], g_mx.y[i]);
      LOG("小地图：关卡 %u 出入口 / 传送点名牌 %d 个%s", lvno, g_mx.n, o); }
}


























typedef struct { uint8_t lv, mon; uint16_t cls; uint8_t art; } mg_def;
static const mg_def k_mg[] = {
    { 17, 1, 822, 1 }, { 4, 0, 17, 2 }, { 4, 0, 18, 2 }, { 4, 0, 19, 2 }, { 4, 0, 20, 2 }, { 4, 0, 21, 2 }, { 4, 0, 22, 2 }, { 5, 0, 30, 3 },
    { 25, 1, 757, 4 }, { 28, 0, 108, 5 },
    { 49, 1, 761, 6 }, { 60, 0, 354, 7 }, { 64, 0, 356, 8 }, { 61, 0, 149, 9 }, { 74, 1, 250, 10 },
    { 66, 0, 152, 23 }, { 67, 0, 152, 23 }, { 68, 0, 152, 23 }, { 69, 0, 152, 23 }, { 70, 0, 152, 23 }, { 71, 0, 152, 23 }, { 72, 0, 152, 23 },
    { 78, 0, 252, 24 }, { 85, 0, 407, 11 }, { 91, 0, 406, 12 }, { 93, 0, 405, 13 }, { 83, 0, 404, 14 }, { 94, 0, 193, 15 },
    { 105, 1, 256, 17 }, { 107, 0, 376, 18 }, { 108, 0, 392, 19 }, { 108, 0, 393, 19 }, { 108, 0, 394, 19 }, { 108, 0, 395, 19 }, { 108, 0, 396, 19 },
    { 110, 1, 793, 20 }, { 111, 0, 473, 21 }, { 114, 0, 460, 22 },
    { 37, 1, 156, 25 }, { 73, 1, 211, 26 }, { 102, 1, 242, 27 }, { 131, 1, 543, 28 }, { 120, 0, 546, 29 }, { 124, 0, 462, 30 },
};
static const int8_t k_tomb_at[7][2] = { { 35, 35 }, { 29, 6 }, { 30, 6 }, { 24, 5 }, { 13, 32 }, { 14, 25 }, { 15, 32 } };
#define MG_SYM 64
#define MG_MAX 12
static struct { uint64_t lv, room, ns; uint32_t lvno; int rooms, frames, n; int32_t x[MG_MAX], y[MG_MAX]; uint8_t art[MG_MAX]; } g_mg;
static _Atomic uint32_t g_mg_seq; static dm_map_goal_view g_mg_pub;
static int mg_wanted(uint32_t lvno) { if (lvno == 46 || lvno == 80) return 1; for (unsigned i = 0; i < sizeof k_mg / sizeof *k_mg; i++) if (k_mg[i].lv == lvno) return 1; return 0; }
static void mg_add(int x, int y, int art) {
    for (int i = 0; i < g_mg.n; i++) if (g_mg.art[i] == art && abs(g_mg.x[i] - x) < 25 && abs(g_mg.y[i] - y) < 25) return;
    if (g_mg.n < MG_MAX) { g_mg.x[g_mg.n] = x; g_mg.y[g_mg.n] = y; g_mg.art[g_mg.n] = (uint8_t)art; g_mg.n++; }
}
static void mg_scan_room(uint64_t lv, uint64_t r) {
    int rx = *(int *)(r + 0x60) * 5, ry = *(int *)(r + 0x64) * 5;
    uint64_t pl = *(uint64_t *)(r + 0x98);
    for (uint64_t u = pl, k = 0; u && k < 256; u = *(uint64_t *)(u + 0x10), k++)
        for (unsigned i = 0; i < sizeof k_mg / sizeof *k_mg; i++)
            if (k_mg[i].lv == g_mg.lvno && k_mg[i].mon == *(uint32_t *)u && k_mg[i].cls == *(uint32_t *)(u + 4)) mg_add(rx + *(int32_t *)(u + 8), ry + *(int32_t *)(u + 0x24), k_mg[i].art);
    if (g_mg.lvno != 46 && g_mg.lvno != 80) return;
    uint64_t am = g_mg.lvno == 80 ? 0 : *(uint64_t *)(lv + 0x1c8); if (g_mg.lvno != 80 && !am) return;
    uint32_t want = g_mg.lvno == 80 ? 94 : *(uint32_t *)(am + 0x120);
    for (uint64_t e = *(uint64_t *)(r + 0x78), k = 0; e && k < 16; e = *(uint64_t *)(e + 8), k++) {
        uint64_t tr = *(uint64_t *)e, tl = tr ? *(uint64_t *)(tr + 0x90) : 0; uint32_t to = tl ? *(uint32_t *)(tl + 0x1f8) : 0;
        if (!want || to != want) continue;
        if (to >= 66 && to <= 72) mg_add(rx + k_tomb_at[to - 66][0], ry + k_tomb_at[to - 66][1], MG_SYM + (int)(to - 66));
        else mg_add(rx + *(int *)(r + 0x68) * 5 / 2, ry + *(int *)(r + 0x6c) * 5 / 2, 16);
    }
}
static void w_map_goal_step(void) {
    static int env = -2; if (env == -2) { const char *e = getenv("DM_MAP_MARK"); env = e ? e[0] != '0' : -1; }
    int dm_custom_get(const char *key); int on = env >= 0 ? env : dm_custom_get("map_mark");
    uint8_t *b = wb(); uint64_t pl = on && b ? w_player() : 0, lv = pl ? w_level(pl) : 0; int ver = atomic_load(&g_dm_game_ver);
    if (lv != g_mg.lv) { memset(&g_mg, 0, sizeof g_mg); g_mg.lv = lv; if (lv) { g_mg.lvno = *(uint32_t *)(lv + 0x1f8); g_mg.room = mg_wanted(g_mg.lvno) ? *(uint64_t *)(lv + 0x10) : 0; } }
    if (g_mg.room && ver >= 0) {
        uint64_t t0 = w_now_ns(); g_mg.frames++;
        while (g_mg.room && w_now_ns() - t0 < 1500000) { uint64_t r = g_mg.room; if (!*(uint64_t *)(r + 0x58)) ((void (*)(int, uint64_t))(b + 0x52dcc0))(ver, r); mg_scan_room(lv, r); g_mg.rooms++; g_mg.room = *(uint64_t *)(r + 0x48); }
        g_mg.ns += w_now_ns() - t0;
        if (!g_mg.room) { char o[160] = ""; int k = 0; for (int i = 0; i < g_mg.n; i++) k += snprintf(o + k, sizeof o - k, " %d@%d,%d", g_mg.art[i], g_mg.x[i], g_mg.y[i]);
            LOG("小地图终点：关卡 %u 扫了 %d 间房，分 %d 帧共 %.2f ms，终点 %d 个（图样号@格）%s", g_mg.lvno, g_mg.rooms, g_mg.frames, g_mg.ns / 1e6, g_mg.n, o); } }


    dm_map_goal_view v; memset(&v, 0, sizeof v);
    int nx = lv && g_mx.lv == lv ? g_mx.n : 0, ng = !g_mg.room ? g_mg.n : 0;
    if (lv && ng + nx) { int px = 0, py = 0; w_unit_xy(pl, &px, &py); v.px = px; v.py = py; int W = 0, H = 0; dm_screen_real(&W, &H); uint64_t scene = *(uint64_t *)(b + 0x2e2a698); float f[2];
        if (W > 0 && H > 0 && scene && w_project(b, scene, pl, f, 0)) { v.sx = f[0] / W; v.sy = f[1] / H; for (int i = 0; i < ng && v.n < DM_MAP_GOAL_MAX; i++, v.n++) { v.x[v.n] = g_mg.x[i]; v.y[v.n] = g_mg.y[i]; v.art[v.n] = g_mg.art[i]; }
            for (int i = 0; i < nx && v.n < DM_MAP_GOAL_MAX; i++, v.n++) { v.x[v.n] = g_mx.x[i]; v.y[v.n] = g_mx.y[i]; v.art[v.n] = g_mx.art[i]; } } }
    atomic_fetch_add(&g_mg_seq, 1); g_mg_pub = v; atomic_fetch_add(&g_mg_seq, 1);
}
int dm_map_goals(dm_map_goal_view *v) {
    for (int t = 0; t < 4; t++) { uint32_t a = atomic_load(&g_mg_seq); if (a & 1) continue; *v = g_mg_pub; if (atomic_load(&g_mg_seq) == a) return v->n; }
    v->n = 0; return 0;
}





















float dm_cam_zoom_want(void) {
    static _Atomic int cz = -1;
    int v = atomic_load(&cz);
    if (v < 0) { const char *e = getenv("DM_CAM_ZOOM"); float w = e ? (float)atof(e) : 0.6f; if (!(w >= 0.5f)) w = 0.5f; if (w > 2.25f) w = 2.25f; v = (int)lroundf(w * 100); atomic_store(&cz, v); }
    return v / 100.0f;
}

int dm_cam_zoom_on(void) {
    int dm_cam_style(void); static _Atomic int env = -1;
    int e = atomic_load(&env); if (e < 0) { e = getenv("DM_CAM_ZOOM") != NULL; atomic_store(&env, e); }
    return e || dm_cam_style() == 1;
}
static void w_cam_zoom(uint8_t *b) {
    int dm_cam_mode(void);
    static float orig = -1;
    const float want = dm_cam_zoom_want();
    if (!(*(volatile uint8_t *)(b + 0x2d20948) & 1)) return;
    float *z = (float *)(b + 0x2d129d0 + 0x1348);





    static int panel = -1;
    const int pv = *(volatile int *)(b + 0x2d47640);
    if (pv != panel) { if (dm_cam_mode() > 0) LOG("镜头缩放：面板横移状态 %d → %d", panel, pv); panel = pv; }
    if (dm_cam_mode() > 0 && dm_cam_zoom_on()) {
        if (orig < 0) { orig = *z >= 1.0f && *z <= 2.25f ? *z : 1.2f; LOG("镜头缩放：开镜头，cameraZoomSetting %.2f → %.2f（原版画面放大，画的范围跟着变大）", orig, want); }
        if (*z != want) *z = want;
    } else if (orig >= 0) { *z = orig; LOG("镜头缩放：关镜头，cameraZoomSetting 写回 %.2f", orig); orig = -1; }
    else if (*z < 1.0f) { LOG("镜头缩放：cameraZoomSetting %.2f 低于原版下限，写回 1.20", *z); *z = 1.2f; }
}

int dm_world_unit_head(uint32_t id, float *xy) {
    uint8_t *b = wb(); uint64_t scene = b ? *(uint64_t *)(b + 0x2e2a698) : 0, u = scene ? w_find(1, id) : 0; int W = 0, H = 0; dm_screen_real(&W, &H);
    if (!u || W <= 0 || H <= 0 || !w_project(b, scene, u, xy, 1)) return 0;
    xy[0] /= W; xy[1] /= H;
    return xy[0] > -0.05f && xy[0] < 1.05f && xy[1] > -0.05f && xy[1] < 1.05f;
}
void dm_world_pump(void) {
#if TARGET_OS_OSX
    w_mouse_cancel_pending();
#endif
    { void dm_rift_client_pump(uint8_t *); DM_PROF(0, "w.rift_client", dm_rift_client_pump(wb())); }
    { void dm_resume_client_pump(uint8_t *); dm_resume_client_pump(wb()); }
    { int nd = atomic_load(&g_dm_dur_sync_n); uint8_t *b0 = nd ? wb() : NULL; if (b0) { atomic_store(&g_dm_dur_sync_n, 0); for (int i = 0; i < nd && i < 16; i++) { uint64_t c = w_find(4, g_dm_dur_sync[i][0]); if (c) ((void (*)(uint64_t, int, int, int))(b0 + 0x5bc1f0))(c, 72, (int)g_dm_dur_sync[i][1], 0); } } }
    { void dm_dmgnum_pump(void); DM_PROF(0, "w.dmgnum", dm_dmgnum_pump()); }
    DM_PROF(0, "w.test_seek", w_test_seek_step());
    w_test_xy_step();
    uint8_t *b = wb(); if (!b) return;
    DM_PROF(0, "w.cam", w_cam_clear());
    w_cam_zoom(b);
    if (g_it.active) w_interaction_poll(w_player(), dm_guest_frame());
    DM_PROF(0, "w.watch", w_watch_step(b));
    { void dm_potpick_step(uint8_t *); DM_PROF(0, "w.potpick", dm_potpick_step(b)); }
    { void dm_padmode_step(uint8_t *, uint64_t); DM_PROF(0, "w.padmode", dm_padmode_step(b, dm_guest_frame())); }
    { void dm_beltauto_step(uint8_t *); DM_PROF(0, "w.beltauto", dm_beltauto_step(b)); }
    { void dm_pouch_step(uint8_t *); DM_PROF(0, "w.pouch", dm_pouch_step(b)); }
    if (g_release_at && dm_guest_frame() >= g_release_at) { g_release_at = 0; uint64_t pl = w_player(); if (pl) { atomic_store(&g_dm_move_allow_until_ns, w_now_ns() + 500000000ull); ((int (*)(uint64_t, int, int, int, uint64_t))(b + 0x4ace20))(pl, 2, g_rel_x, g_rel_y, 0); } }
    DM_PROF(0, "w.go", w_go_step());
    DM_PROF(0, "w.tile_retry", w_tile_retry_step());
    DM_PROF(0, "w.map_mark", w_map_mark_step());
    DM_PROF(0, "w.map_goal", w_map_goal_step());




#ifndef DM_PLAYER_BUILD
    for (int tk = 0; tk < 4; tk++) { static int st[4]; static uint32_t t0[4]; static const char *tn[4] = {"DM_TEST_WORLD_TAP", "DM_TEST_WORLD_TAP2", "DM_TEST_WORLD_TAP3", "DM_TEST_WORLD_TAP4"};
        const char *e = st[tk] == 2 ? NULL : getenv(tn[tk]); int ty = 0, at = 0; unsigned cl = 0; uint32_t dm_cur_tick(void);
        if (!e || (sscanf(e, "%d:c%u@%d", &ty, &cl, &at) != 3 && !(sscanf(e, "%d:any@%d", &ty, &at) == 2 && (cl = 0xffffff)))) { st[tk] = 2; continue; }
        uint32_t now = dm_cur_tick(); if (now < (uint32_t)at) continue;
        if (!st[tk]) { if (atomic_load(&g_watch)) continue; dm_world_watch(ty, cl); st[tk] = 1; t0[tk] = now; continue; }
        float nx, ny; if (dm_world_watch_pos(&nx, &ny)) { dm_world_watch(-1, 0); st[tk] = 2; LOG("世界目标：%s %s → 在画面 (%.3f,%.3f) 落点按", tn[tk], e, nx, ny); dm_world_tap(nx, ny);
            const char *sp = getenv("DM_TEST_WORLD_TAP_SHOT"); if (sp) { void dm_gfx_shot_at(long, const char *); char pth[512]; snprintf(pth, sizeof pth, "%s%d.png", sp, tk + 1); dm_gfx_shot_at(1, pth); } }
        else if (now - t0[tk] > 125) { dm_world_watch(-1, 0); st[tk] = 2; LOG("世界目标：%s %s → 5 秒内这类单位没进画面，放弃", tn[tk], e); } }
#endif






#ifndef DM_PLAYER_BUILD
    for (int gk = 0; gk < 4; gk++) { static int tgs[4]; static const char *gn[4] = {"DM_TEST_WORLD_GO", "DM_TEST_WORLD_GO2", "DM_TEST_WORLD_GO3", "DM_TEST_WORLD_GO4"}; const char *e = tgs[gk] ? NULL : getenv(gn[gk]); uint32_t dm_cur_tick(void); int gt = 0, gi = 0, at = 200, bycls = 0;
      int nearonly = 0; if (e && !strncmp(e, "near", 4) && (e[4] == ':' || (e[4] >= '1' && e[4] <= '9' && e[5] == ':'))) { nearonly = e[4] == ':' ? W_NEAR : e[4] - '0'; e += e[4] == ':' ? 5 : 6; }
      int kind = 0; if (e && !strncmp(e, "monster", 7)) { kind = 1; gt = 1; sscanf(e + 7, "@%d", &at); } else if (e && !strncmp(e, "exit", 4)) { kind = 2; gt = 5; sscanf(e + 4, "@%d", &at); }
      if (e && (kind || (sscanf(e, "%d:c%d@%d", &gt, &gi, &at) >= 2 ? (bycls = 1) : sscanf(e, "%d:%d@%d", &gt, &gi, &at) >= 2)) && dm_cur_tick() >= (uint32_t)at && w_player() && gt >= 0 && gt <= 5) { tgs[gk] = 1; uint64_t u = 0;
          if (kind) { uint64_t pl2 = w_player(); int px = 0, py = 0, x, y; long bd = -1; w_unit_xy(pl2, &px, &py);
              for (int k = 0; k < 128; k++) for (uint64_t c = *(uint64_t *)(b + 0x2d603a0 + gt * 0x400 + k * 8); c; c = *(uint64_t *)(c + 0x158)) {
                  if (!(((int (*)(uint64_t, int, int, int, int))(b + 0x464a70))(c, 0, 0, -1, -1) & 1) || !w_unit_xy(c, &x, &y)) continue;
                  if (kind == 1) { int ct = ((int (*)(int, uint64_t, uint64_t))(b + 0x2f7b7c))(0x165, pl2, c); if (ct != 1 && ct != 0xa) continue; }
                  long dd = (long)(x - px) * (x - px) + (long)(y - py) * (y - py); if (bd < 0 || dd < bd) { bd = dd; u = c; } }
              if (!u) { int shown = 0; for (int k = 0; k < 128 && shown < 8; k++) for (uint64_t c = *(uint64_t *)(b + 0x2d603a0 + gt * 0x400 + k * 8); c && shown < 8; c = *(uint64_t *)(c + 0x158)) { int cx2 = 0, cy2 = 0; w_unit_xy(c, &cx2, &cy2); shown++;
                  LOG("世界目标：  在场 类型 %d 类号 %u 模式 %u 可选 %d 类别 %d 距 (%d,%d)", gt, *(uint32_t *)(c + 4), *(uint32_t *)(c + 0xc), ((int (*)(uint64_t, int, int, int, int))(b + 0x464a70))(c, 0, 0, -1, -1) & 1, ((int (*)(int, uint64_t, uint64_t))(b + 0x2f7b7c))(0x165, pl2, c), cx2 - px, cy2 - py); } }
              if (u) { w_go(u); if (kind == 1) g_go.stop_at = 5; else if (nearonly) g_go.stop_at = nearonly; } LOG("世界目标：%s %s → %s", gn[gk], e, u ? "已发" : "没找到（附近已载入的房间里没有）"); if (u) LOG("世界目标：%s 目标 类型 %u 类号 %u id %u", gn[gk], *(uint32_t *)u, *(uint32_t *)(u + 4), *(uint32_t *)(u + 8)); continue; }
          if (!bycls) u = w_find((uint32_t)gt, (uint32_t)gi);
          else { int px = 0, py = 0, x, y; long bd = -1; w_unit_xy(w_player(), &px, &py);
              for (int k = 0; k < 128; k++) for (uint64_t c = *(uint64_t *)(b + 0x2d603a0 + gt * 0x400 + k * 8); c; c = *(uint64_t *)(c + 0x158)) if (*(uint32_t *)(c + 4) == (uint32_t)gi && w_unit_xy(c, &x, &y)) { long dd = (long)(x - px) * (x - px) + (long)(y - py) * (y - py); if (bd < 0 || dd < bd) { bd = dd; u = c; } } }
          if (u) { w_go(u); if (nearonly) g_go.stop_at = nearonly; } LOG("世界目标：DM_TEST_WORLD_GO %s → %s", e, u ? "已发" : "没找到"); if (u) LOG("世界目标：DM_TEST_WORLD_GO 目标 类型 %u 类号 %u id %u", *(uint32_t *)u, *(uint32_t *)(u + 4), *(uint32_t *)(u + 8)); } }
#endif
    w_tap_step(b);
#if TARGET_OS_OSX
    { uint64_t dm_stick_last_ns(void); uint64_t v = dm_stick_last_ns(); if (v != g_gnd_stick_seen) { g_gnd_stick_seen = v; g_gnd_stick_at = w_now_ns(); } }
    w_walk_step(b);
#endif
    { uint64_t pl = w_player(); atomic_store(&g_has_target, pl && ((uint64_t (*)(uint64_t, uint32_t))(b + 0x2f7a0c))(((uint64_t (*)(void))(b + 0x2acecc))(), *(uint32_t *)(b + 0x2d20e4c)) != 0); }
}
