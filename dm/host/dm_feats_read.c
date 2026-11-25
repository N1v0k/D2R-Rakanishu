


#include "dm_feats.h"
#include <string.h>

int dm_guest_heap_readable(uint64_t a, size_t n);

enum {
    PD_WAYPOINTS = 0x58, PD_SIZE = 0x70,
    WP_SIZE = 2 + DM_FT_WP_BYTES,
    T_LEVELS = 0x1480, T_HIRE = 0x558, T_SIZE = 0x1490,
    LEVEL_SIZE = 0x194, LEVEL_WP = 0xf4, LEVELS_MAX = 1024,
    HIRE_SIZE = 0x150, HIRE_ACT = 0xc, HIRES_MAX = 4096,
    INV_MAGIC = 0x01020304, INV_FIRST = 0x10, INV_CURSOR = 0x40, INV_SIZE = 0x48,
    UNIT_SIZE = 0x1c0, UNIT_ITEM = 4,
    D_QUALITY = 0, D_FLAGS = 0x18, D_ROW = 0x34, D_SLOT = 0x54, D_PAGE = 0x55, D_NEXT = 0xb0, D_SIZE = 0xb8,
    U_ID = 8, U_MODE = 0xc, U_DATA = 0x10, U_SOCKETS = 0x90, F_RUNEWORD = 0x04000000, PAGE_CUBE = 3, SOCKETS_MAX = 8, LIST_MAX = 4096,
    U_LINK_TYPE = 0xe8, U_FLAGS = 0x128, F_LINKED = 0x400,
};
static uint64_t rd(uint64_t a) { return *(const uint64_t *)(uintptr_t)a; }
static uint32_t rd32(uint64_t a) { return *(const uint32_t *)(uintptr_t)a; }
static int ok(uint64_t a, size_t n) { return a && !(a & 7) && dm_guest_heap_readable(a, n); }

static int span(uint64_t a, uint64_t n, uint64_t size, uint64_t max) { return n <= max && (!n || (ok(a, (size_t)(n * size)))); }

int dm_ftr_waypoints(uint64_t pd, uint8_t wp[DM_FT_DIFFS][DM_FT_WP_BYTES]) {
    if (!ok(pd, PD_SIZE)) return 0;
    for (int d = 0; d < DM_FT_DIFFS; d++) {
        uint64_t w = rd(pd + PD_WAYPOINTS + 8 * (uint64_t)d);
        if (!w || !dm_guest_heap_readable(w, WP_SIZE)) return 0;
        const uint8_t *p = (const uint8_t *)(uintptr_t)w;
        if (p[0] != 2 || p[1] != 1) return 0;
        memcpy(wp[d], p + 2, DM_FT_WP_BYTES);
    }
    return 1;
}

int dm_ftr_waypoint_count(uint64_t t) {
    if (!ok(t, T_SIZE)) return 0;
    uint64_t levels = rd(t + T_LEVELS), n = rd(t + T_LEVELS + 8); int count = 0;
    if (!n || !span(levels, n, LEVEL_SIZE, LEVELS_MAX)) return 0;
    for (uint64_t i = 0; i < n; i++) count += *(const uint8_t *)(uintptr_t)(levels + LEVEL_SIZE * i + LEVEL_WP) != 255;
    return count <= 8 * DM_FT_WP_BYTES ? count : 0;
}

int dm_ftr_hireling_act(uint64_t t, int at, uint32_t want) {
    if (!ok(t, T_SIZE)) return -1;
    uint64_t rows = rd(t + T_HIRE), n = rd(t + T_HIRE + 8);
    if (!span(rows, n, HIRE_SIZE, HIRES_MAX)) return -1;
    for (uint64_t k = 0; k < n; k++) if (rd32(rows + k * HIRE_SIZE + (uint64_t)at) == want) return (int)rd32(rows + k * HIRE_SIZE + HIRE_ACT);
    return 0;
}

int dm_ftr_unit(uint64_t u, size_t n) { return ok(u, n); }



typedef struct { uint64_t q, held; int cursor, n; } walk;
static int next_item(walk *w, uint64_t *it, uint64_t *d) {
    if (!w->q && (!w->held || w->cursor)) return 0;
    if (!w->q) { w->q = w->held; w->cursor = 1; }
    if (++w->n > LIST_MAX || !ok(w->q, UNIT_SIZE) || rd32(w->q) != UNIT_ITEM) return -1;
    *it = w->q; *d = rd(w->q + U_DATA);
    if (!ok(*d, D_SIZE)) return -1;
    w->q = w->cursor ? 0 : rd(*d + D_NEXT);
    return 1;
}
static int start(uint64_t inv, walk *w) {
    if (!ok(inv, INV_SIZE) || rd32(inv) != INV_MAGIC) return 0;
    *w = (walk){ rd(inv + INV_FIRST), rd(inv + INV_CURSOR), 0, 0 };
    return 1;
}

int dm_ftr_items(uint64_t inv, dm_ft_item *out, int max, dm_ftr_item_fn code, dm_ftr_item_fn two_hands) {
    walk w; uint64_t q, d; int r, n = 0;
    if (!start(inv, &w)) return -1;
    while ((r = next_item(&w, &q, &d)) > 0) {
        if (n == max) return -1;
        dm_ft_item *e = &out[n++]; memset(e, 0, sizeof *e);
        e->code = code(q); e->quality = (uint8_t)rd32(d + D_QUALITY); e->row = (uint16_t)rd32(d + D_ROW); e->set = -1;
        uint8_t slot = *(const uint8_t *)(uintptr_t)(d + D_SLOT);
        e->slot = rd32(q + U_MODE) == 1 && slot >= 1 && slot <= DM_FT_SLOTS ? slot : 0;
        e->two_hands = e->quality == DM_FT_Q_UNIQUE && e->slot ? two_hands(q) != 0 : 0;
    }
    return r < 0 ? -1 : n;
}

int dm_ftr_find(uint64_t inv, uint32_t id, uint64_t *item) {
    walk w; uint64_t it, d; int r;
    if (!start(inv, &w)) return -1;
    while ((r = next_item(&w, &it, &d)) > 0) if (rd32(it + U_ID) == id) { *item = it; return 1; }
    return r;
}


static int socketed(uint64_t it, uint32_t ids[SOCKETS_MAX]) {
    uint64_t c = rd(it + U_SOCKETS), s, sd;
    if (!c) return 0;
    if (!ok(c, INV_SIZE)) return -1;
    int k = 0;
    for (s = rd(c + INV_FIRST); s; s = rd(sd + D_NEXT)) {
        if (k == SOCKETS_MAX || !ok(s, UNIT_SIZE) || !ok(sd = rd(s + U_DATA), D_SIZE)) return -1;
        ids[k++] = rd32(s + U_ID);
    }
    return k;
}

int dm_ftr_socket(uint64_t it, uint32_t id, int *runeword) {
    uint32_t ids[SOCKETS_MAX]; uint64_t d; int k;
    if (!ok(it, UNIT_SIZE) || !ok(d = rd(it + U_DATA), D_SIZE) || (k = socketed(it, ids)) < 0) return -1;
    *runeword = (rd32(d + D_FLAGS) & F_RUNEWORD) != 0;
    for (int i = 0; i < k; i++) if (ids[i] == id) return 1;
    return 0;
}

int dm_ftr_cube(uint64_t inv, dm_ft_cube_item *out, int max, dm_ftr_item_fn code) {
    walk w; uint64_t it, d; int r, n = 0;
    if (!start(inv, &w)) return -1;
    while ((r = next_item(&w, &it, &d)) > 0) {
        if (rd32(it + U_MODE) != 0 || *(const uint8_t *)(uintptr_t)(d + D_PAGE) != PAGE_CUBE) continue;
        uint32_t ids[SOCKETS_MAX]; int f = socketed(it, ids);
        if (n == max || f < 0) return -1;
        out[n++] = (dm_ft_cube_item){ rd32(it + U_ID), code(it), (uint8_t)rd32(d + D_QUALITY), (uint8_t)f };
    }
    return r < 0 ? -1 : n;
}

int dm_ftr_top(uint64_t inv, uint32_t *id) {
    walk w; uint64_t it, d; int r; uint32_t top = 0;
    if (!start(inv, &w)) return -1;
    while ((r = next_item(&w, &it, &d)) > 0) if (rd32(it + U_ID) > top) top = rd32(it + U_ID);
    if (r < 0) return -1;
    *id = top;
    return 1;
}

int dm_ftr_gold_link(uint64_t it, int *linked, uint32_t *type) {
    if (!ok(it, UNIT_SIZE) || rd32(it) != UNIT_ITEM) return -1;
    *linked = (rd32(it + U_FLAGS) & F_LINKED) != 0; *type = rd32(it + U_LINK_TYPE);
    return 1;
}

int dm_ftr_quality(uint64_t it, uint32_t *id, int *quality) {
    uint64_t d;
    if (!ok(it, UNIT_SIZE) || rd32(it) != UNIT_ITEM || !ok(d = rd(it + U_DATA), D_SIZE)) return -1;
    *id = rd32(it + U_ID); *quality = (int)rd32(d + D_QUALITY);
    return 1;
}
