





















#include "hle.h"
#include "game_addrs.h"
#include "dm_unit.h"
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static const struct { int lv, cls; } k_town[] = { {1, 119}, {40, 156}, {75, 237}, {103, 398}, {109, 429} };
enum { TILE_WAYPOINT = 13, CHECK_AFTER = 5, SPAWN_GAP = 10, NPC_GAP = 6, OBJ_GAP = 3 };

static uint8_t *main_base(void) { static uint8_t *b; if (!b) for (int i = 0; i < g_nmod; i++) if (!strcmp(g_mod[i].name, "main")) b = (uint8_t *)g_mod[i].base; return b; }
static uint64_t rd(uint64_t a) { return a ? *(volatile uint64_t *)(uintptr_t)a : 0; }
static uint64_t level_of(uint64_t u) { return dm_unit_level_ptr(u); }
static void unit_xy(uint64_t u, int *x, int *y) { uint64_t p = rd(u + 0x38); *x = p ? *(uint16_t *)(uintptr_t)(p + 2) : 0; *y = p ? *(uint16_t *)(uintptr_t)(p + 6) : 0; }
static int town_cls(int lv) { for (unsigned i = 0; i < sizeof k_town / sizeof k_town[0]; i++) if (k_town[i].lv == lv) return k_town[i].cls; return 0; }




static uint64_t preset_find(uint64_t lvt, int cls, int *x, int *y) { int n = 0;
    for (uint64_t r = dm_level_rooms(lvt); r && n < 2000; r = rd(r + 0x48), n++) for (uint64_t u = rd(r + 0x98), k = 0; u && k < 256; u = rd(u + 0x10), k++)
        if (*(int32_t *)(uintptr_t)(u + 0x20) == 2 && *(int32_t *)(uintptr_t)(u + 4) == cls) { *x = *(int32_t *)(uintptr_t)(r + 0x60) * 5 + *(int32_t *)(uintptr_t)(u + 8); *y = *(int32_t *)(uintptr_t)(r + 0x64) * 5 + *(int32_t *)(uintptr_t)(u + 0x24); return u; }
    return 0; }
static int preset_gap(uint64_t lvt, int type, int x, int y, uint64_t skip) { int best = 1 << 30, n = 0;
    for (uint64_t r = dm_level_rooms(lvt); r && n < 2000; r = rd(r + 0x48), n++) for (uint64_t u = rd(r + 0x98), k = 0; u && k < 256; u = rd(u + 0x10), k++) { if (u == skip || *(int32_t *)(uintptr_t)(u + 0x20) != type) continue;
        int dx = abs(*(int32_t *)(uintptr_t)(r + 0x60) * 5 + *(int32_t *)(uintptr_t)(u + 8) - x), dy = abs(*(int32_t *)(uintptr_t)(r + 0x64) * 5 + *(int32_t *)(uintptr_t)(u + 0x24) - y), d = dx > dy ? dx : dy; if (d < best) best = d; }
    return best; }

static struct { int on, lv, wx, wy, sx, sy, calls; uint64_t room; } g_sp;


void svc_join_spawn(uint64_t *regs, int corpse) {
    uint8_t *B = main_base(); typedef uint64_t (*spot_fn)(int, uint64_t, int, int, int32_t *, int32_t *, int);
    spot_fn orig = (spot_fn)(B + GA_LEVEL_SPAWN_POINT); int ver = (int)(uint32_t)regs[0], lv = (int)(uint32_t)regs[2], type = (int)(uint32_t)regs[3], size = (int)(uint32_t)regs[6]; uint64_t act = regs[1];
    int32_t *px = (int32_t *)(uintptr_t)regs[4], *py = (int32_t *)(uintptr_t)regs[5]; int cls = town_cls(lv);
    const char *off = getenv("DM_TOWNWP_OFF"); int no = off && *off && strcmp(off, "0");
    if (type != 0 || !cls || no || getenv("DM_C2S_REPLAY")) { regs[0] = orig(ver, act, lv, type, px, py, size);
        if (!corpse && !g_sp.calls++) LOG("城镇小站落点：%s，原版出生点 %d,%d（关卡 %d）", !cls ? "进局不在城镇" : no ? "DM_TOWNWP_OFF" : "回放原版录制", *px, *py, lv);
        return; }
    if (corpse && g_sp.on && g_sp.lv == lv && g_sp.room) {
        int32_t pos[2] = { g_sp.sx, g_sp.sy }; uint64_t r2 = ((uint64_t (*)(uint64_t, int32_t *, uint64_t, int, int, int, int, int, int))(B + GA_COLL_FIND_FREE))(g_sp.room, pos, 0, size, 0x1c09, 0x1c09, 0, 3, 1);
        if (r2) { *px = pos[0]; *py = pos[1]; regs[0] = r2; g_sp.calls++; LOG("城镇小站落点：给尸体取点 → %d,%d（人的落点 %d,%d）", pos[0], pos[1], g_sp.sx, g_sp.sy); return; }
        LOG("【红】城镇小站落点：人的落点 %d,%d 旁 3 格内给尸体找不到空格，尸体按小站另取", g_sp.sx, g_sp.sy); }
    int32_t sx = 0, sy = 0; uint64_t room = orig(ver, act, lv, TILE_WAYPOINT, &sx, &sy, size);
    if (!room) { regs[0] = orig(ver, act, lv, type, px, py, size); LOG("【红】城镇小站落点：关卡 %d 取不到小站格，退回原版出生点 %d,%d", lv, *px, *py); return; }
    uint64_t lvt = rd(rd(room + 0x18) + 0x90); int wx = 0, wy = 0; uint64_t wrec = lvt ? preset_find(lvt, cls, &wx, &wy) : 0;
    int32_t bx = sx, by = sy; uint64_t broom = room; int found = 0;
    if (wrec) {
        float vx = (float)(sx - wx), vy = (float)(sy - wy), n = sqrtf(vx * vx + vy * vy); if (n < 0.5f) { vx = 1; vy = 1; n = sqrtf(2); } vx /= n; vy /= n;
        for (int k = 0; k < 8 && !found; k++) { float a = (float)(k % 2 ? -(k + 1) / 2 : k / 2) * 0.7853982f, cx = vx * cosf(a) - vy * sinf(a), cy = vx * sinf(a) + vy * cosf(a);
            int32_t pos[2] = { wx + (int)lroundf(cx * SPAWN_GAP), wy + (int)lroundf(cy * SPAWN_GAP) };
            uint64_t r2 = ((uint64_t (*)(uint64_t, int32_t *, uint64_t, int, int, int, int, int, int))(B + GA_COLL_FIND_FREE))(room, pos, 0, size, 0x1c09, 0x1c09, 0, 2, 1); if (!r2) continue;
            int dw = abs(pos[0] - wx) > abs(pos[1] - wy) ? abs(pos[0] - wx) : abs(pos[1] - wy);
            if (dw < SPAWN_GAP - 1 || preset_gap(lvt, 1, pos[0], pos[1], 0) < NPC_GAP || preset_gap(lvt, 2, pos[0], pos[1], wrec) < OBJ_GAP) continue;
            bx = pos[0]; by = pos[1]; broom = r2; found = 1; } }
    *px = bx; *py = by; regs[0] = broom;
    g_sp.calls++; if (!corpse) { g_sp.on = 1; g_sp.lv = lv; g_sp.wx = wx; g_sp.wy = wy; g_sp.sx = bx; g_sp.sy = by; g_sp.room = broom;
        if (found) LOG("城镇小站落点：关卡 %d 小站 %d,%d，进局落点 %d,%d（离小站横 %d 纵 %d 格；小站格旁原版点 %d,%d）", lv, wx, wy, bx, by, bx - wx, by - wy, sx, sy);
        else LOG("【红】城镇小站落点：关卡 %d 小站 %s，安全距离 %d 格处八个方向都不合规，落在小站格旁 %d,%d", lv, wrec ? "找到了" : "的预设记录没找到", SPAWN_GAP, bx, by); }
    else LOG("城镇小站落点：给尸体取点 → %d,%d", bx, by);
}

void dm_townwp_tick(uint64_t G, uint64_t player, uint8_t *B) {
    static uint64_t g0, p0; static uint32_t last_t, join_t; static int logged; (void)B;
    uint32_t t = *(volatile uint32_t *)(uintptr_t)(G + 0x170);
    if (G != g0 || player != p0 || t < last_t) { g0 = G; p0 = player; join_t = t; logged = 0; }
    last_t = t; if (logged || t - join_t < CHECK_AFTER) return; logged = 1;
    uint64_t lv = level_of(player); int id = lv ? *(int32_t *)(uintptr_t)(lv + 0x1f8) : 0, x, y; unit_xy(player, &x, &y);
    if (!g_sp.on) { if (town_cls(id) && !g_sp.calls) LOG("城镇小站落点：这个产物不带进局出生点的剪口（要 r37 起），人在原版出生点 %d,%d", x, y); g_sp.calls = 0; return; }
    LOG("城镇小站落点：拍 %u 人在 %d,%d，小站 %d,%d（差 %d,%d）", t, x, y, g_sp.wx, g_sp.wy, x - g_sp.wx, y - g_sp.wy);
    for (int b = 0; b < 128; b++) for (uint64_t u = rd(G + 0x2230 + 8 * b); u; u = rd(u + 0x158)) { if (u == player || *(uint32_t *)(uintptr_t)(u + 0xc) != 17) continue;
        int cx, cy; unit_xy(u, &cx, &cy); LOG("城镇小站落点：拍 %u 尸体（单位 id %u）在 %d,%d，离人 %d,%d、离小站 %d,%d", t, *(uint32_t *)(uintptr_t)(u + 8), cx, cy, cx - x, cy - y, cx - g_sp.wx, cy - g_sp.wy); }
    g_sp.on = 0; g_sp.calls = 0; g_sp.room = 0;
}
