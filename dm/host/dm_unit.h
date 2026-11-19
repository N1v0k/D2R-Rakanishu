









#ifndef DM_UNIT_H
#define DM_UNIT_H
#include <stdint.h>
static inline uint64_t dm_unit_rd64(uint64_t a) { return a ? *(volatile uint64_t *)(uintptr_t)a : 0; }


static inline uint64_t dm_unit_level_ptr(uint64_t u) {
    uint64_t path = u ? dm_unit_rd64(u + 0x38) : 0, room = path ? dm_unit_rd64(path + 0x20) : 0, ex = room ? dm_unit_rd64(room + 0x18) : 0;
    return ex ? dm_unit_rd64(ex + 0x90) : 0;
}


static inline int dm_unit_level(uint64_t u) { uint64_t lv = dm_unit_level_ptr(u); return lv ? *(volatile int32_t *)(uintptr_t)(lv + 0x1f8) : 0; }




static inline uint64_t dm_level_rooms(uint64_t lv) { return lv ? dm_unit_rd64(lv + 0x10) : 0; }











static inline int dm_teleport(const uint8_t *main_base, uint64_t G, uint64_t unit, uint64_t room, int x, int y) {
    if (!main_base || !G || !unit || !room) return 0;
    return ((int (*)(uint64_t, uint64_t, uint64_t, int, int, int, int))(uintptr_t)(main_base + 0x7cc530))(G, unit, room, x, y, 0, 0);
}

static inline uint64_t dm_free_spot(const uint8_t *main_base, uint64_t G, uint64_t room, int32_t pos[2]) {
    uint64_t out = 0; ((void (*)(uint64_t, uint64_t, int32_t *, int, int, uint64_t *, int, int))(uintptr_t)(main_base + 0x7160d0))(G, room, pos, 3, 0x400, &out, 4, 100); return out;
}


static inline uint64_t dm_level_room_load(const uint8_t *main_base, uint64_t G, uint64_t lv, int px, int py) {
    int n = 0;
    for (uint64_t r = dm_level_rooms(lv); r && n < 2000; r = dm_unit_rd64(r + 0x48), n++) {
        int x = *(int32_t *)(uintptr_t)(r + 0x60) * 5, y = *(int32_t *)(uintptr_t)(r + 0x64) * 5, w = *(int32_t *)(uintptr_t)(r + 0x68) * 5, h = *(int32_t *)(uintptr_t)(r + 0x6c) * 5;
        if (px >= x && px < x + w && py >= y && py < y + h) { if (!dm_unit_rd64(r + 0x58)) ((void (*)(int, uint64_t))(uintptr_t)(main_base + 0x52dcc0))(*(uint8_t *)(uintptr_t)(G + 0x106), r); return dm_unit_rd64(r + 0x58); }
    }
    return 0;
}

static inline void dm_warp(const uint8_t *main_base, uint64_t G, uint64_t player, int level) { ((void (*)(uint64_t, uint64_t, int, int))(uintptr_t)(main_base + 0x634000))(G, player, level, 11); }
#endif
