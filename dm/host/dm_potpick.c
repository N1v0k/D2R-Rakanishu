



























#include "hle.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int dm_custom_get(const char *key);
uint64_t dm_guest_frame(void);
#define PICK_R 3

static uint32_t g_tried[8], g_tried_fr[8];


#include <stdatomic.h>
static _Atomic uint64_t g_auto[4];
static void auto_note(uint32_t id, uint64_t fr) { static unsigned k; atomic_store(&g_auto[k++ % 4], (uint64_t)id << 32 | (uint32_t)fr); }
int dm_potpick_auto(uint32_t id) { uint32_t fr = (uint32_t)dm_guest_frame(); for (int i = 0; i < 4; i++) { uint64_t v = atomic_load(&g_auto[i]); if (v && (uint32_t)(v >> 32) == id && fr - (uint32_t)v < 300) return 1; } return 0; }
uint32_t dm_cur_tick(void);





static int pot_rank(uint8_t *b, uint64_t it) {
    uint32_t c = ((uint32_t (*)(uint64_t))(b + 0x570260))(it); const char *k = (const char *)&c;
    if (k[0] == 'r' && k[1] == 'v') return k[2] == 'l' ? 20 : 19;
    return (k[0] == 'h' || k[0] == 'm') && k[1] == 'p' && k[2] >= '1' && k[2] <= '5' ? k[2] - '0' : 1;
}
static void pick_send(uint8_t *b, uint64_t it, uint32_t id, int x, int y, int px, int py, uint64_t fr) {
    int slot = 0; for (int i = 0; i < 8; i++) { if (g_tried[i] == id) { slot = i; break; } if (g_tried_fr[i] < g_tried_fr[slot]) slot = i; }
    g_tried[slot] = id; g_tried_fr[slot] = (uint32_t)fr; auto_note(id, fr);
    uint8_t pk[17] = { 0x16 }; uint32_t one = 1; memcpy(pk + 1, &id, 4); memcpy(pk + 5, &x, 4); memcpy(pk + 9, &y, 4); memcpy(pk + 13, &one, 4);
    ((void (*)(const uint8_t *, int))(b + 0x2ceab0))(pk, 17);
    uint32_t c = ((uint32_t (*)(uint64_t))(b + 0x570260))(it);
    LOG("药水自动拾取：物品 id %u（%.3s）类号 %u 在 %d,%d（玩家 %d,%d）→ 发捡起包（帧 %llu 拍 %u）", id, (const char *)&c, *(uint32_t *)(it + 4), x, y, px, py, (unsigned long long)fr, dm_cur_tick());
}

void dm_potpick_step(uint8_t *b) {
    { int dm_itemjob_owner(void); if (dm_itemjob_owner()) return; }


    if (b) { void dm_lootfilter_pump(uint8_t *, uint64_t); void dm_lootfilter_test_dump(uint8_t *); dm_lootfilter_pump(b, ((uint64_t (*)(uint32_t))(b + 0x45f9d0))(*(uint32_t *)(b + 0x2d20e4c))); dm_lootfilter_test_dump(b); }
    if (b) { void dm_autobuff_pump(uint8_t *, uint64_t); void dm_autobuff_test_drop(uint64_t); uint64_t pl = ((uint64_t (*)(uint32_t))(b + 0x45f9d0))(*(uint32_t *)(b + 0x2d20e4c)); dm_autobuff_test_drop(pl); dm_autobuff_pump(b, pl); }
    static int env = -2; if (env == -2) { const char *e = getenv("DM_POT_PICK"); env = e ? e[0] != '0' : -1; }
    if (!b || !(env >= 0 ? env : dm_custom_get("pot_pick"))) return;
    static uint64_t next; uint64_t fr = dm_guest_frame(); if (fr < next) return;




    static long t_from = -1; static int t_burst; if (t_from < 0) { const char *e = getenv("DM_TEST_POTPICK_FROM"); t_from = e ? atol(e) : 0; t_burst = getenv("DM_TEST_POTPICK_BURST") != NULL; }
    if (fr < (uint64_t)t_from) return;
    uint64_t pl = ((uint64_t (*)(uint32_t))(b + 0x45f9d0))(*(uint32_t *)(b + 0x2d20e4c)); if (!pl) return;
    uint32_t pm = *(uint32_t *)(pl + 0xc); if (pm == 0 || pm == 17) return;
    uint64_t inv = *(uint64_t *)(pl + 0x90); if (!inv || *(uint32_t *)inv != 0x01020304 || *(uint64_t *)(inv + 0x40)) return;
    uint64_t pp = *(uint64_t *)(pl + 0x38); if (!pp) return; int px = *(uint16_t *)(pp + 2), py = *(uint16_t *)(pp + 6);
    uint64_t best = 0; uint32_t bid = 0; int brank = 0, bx = 0, by = 0;
    for (int k = 0; k < 128; k++) for (uint64_t it = *(uint64_t *)(b + 0x2d603a0 + 4 * 0x400 + k * 8); it; it = *(uint64_t *)(it + 0x158)) {
        if (*(uint32_t *)(it + 0xc) != 3) continue;
        uint64_t ip = *(uint64_t *)(it + 0x38); if (!ip) continue; int x = *(int32_t *)(ip + 0x10), y = *(int32_t *)(ip + 0x14);
        if (abs(x - px) > PICK_R || abs(y - py) > PICK_R) continue;
        { int dm_lootfilter_hidden(uint8_t *, uint64_t); if (dm_lootfilter_hidden(b, it)) continue; }
        int (*is)(uint64_t, int) = (int (*)(uint64_t, int))(b + 0x574bc0);
        if (!(is(it, 77) || is(it, 78)) || !(((int (*)(uint64_t))(b + 0x3c1b28))(it) & 1)) continue;
        uint32_t id = *(uint32_t *)(it + 8); int seen = 0;
        for (int i = 0; i < 8; i++) if (g_tried[i] == id && fr - g_tried_fr[i] < 120) seen = 1;
        if (seen) continue;
        if (t_burst) { pick_send(b, it, id, x, y, px, py, fr); continue; }
        int r = pot_rank(b, it); if (r > brank) { brank = r; best = it; bid = id; bx = x; by = y; }
    }
    if (best) { pick_send(b, best, bid, bx, by, px, py, fr); next = fr + 10; }
}
