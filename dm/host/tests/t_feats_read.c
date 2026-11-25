








#include "../dm_feats_read.c"
#include <stdio.h>
#include <stdlib.h>

static _Alignas(16) uint8_t arena[1 << 16];
static size_t top;
int dm_guest_heap_readable(uint64_t a, size_t n) { uint64_t b = (uint64_t)(uintptr_t)arena; return a >= b && n <= sizeof arena && a - b <= sizeof arena - n; }
static uint64_t alloc(size_t n) { uint64_t a = (uint64_t)(uintptr_t)(arena + top); top += (n + 15) & ~(size_t)15; memset((void *)(uintptr_t)a, 0, n); return a; }
static void put64(uint64_t a, uint64_t v) { memcpy((void *)(uintptr_t)a, &v, 8); }
static void put32(uint64_t a, uint32_t v) { memcpy((void *)(uintptr_t)a, &v, 4); }
static const uint64_t OUTSIDE = 0x10000;

static int checks, fails;
#define CHECK(c, ...) do { checks++; if (!(c)) { fails++; fprintf(stderr, "FAIL line %d: ", __LINE__); fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); } } while (0)

static int code_calls, two_calls;
static uint32_t code(uint64_t it) { code_calls++; return 0x20000000u | (uint32_t)(it & 0xffffff); }
static uint32_t two(uint64_t it) { (void)it; two_calls++; return 1; }

static uint64_t item(int quality, int row, int slot, int mode, uint64_t *data) {
    uint64_t u = alloc(UNIT_SIZE), d = alloc(D_SIZE);
    put32(u, UNIT_ITEM); put32(u + 0xc, (uint32_t)mode); put64(u + 0x10, d);
    put32(d + D_QUALITY, (uint32_t)quality); put32(d + D_ROW, (uint32_t)row); *(uint8_t *)(uintptr_t)(d + D_SLOT) = (uint8_t)slot;
    if (data) *data = d;
    return u;
}

int main(void) {
    { uint64_t pd = alloc(PD_SIZE); uint8_t wp[DM_FT_DIFFS][DM_FT_WP_BYTES];
      for (int d = 0; d < DM_FT_DIFFS; d++) { uint64_t w = alloc(16); uint8_t rec[10] = { 2, 1, (uint8_t)(0x3f + d) }; memcpy((void *)(uintptr_t)w, rec, 10); put64(pd + PD_WAYPOINTS + 8 * d, w); }
      CHECK(dm_ftr_waypoints(pd, wp) && wp[0][0] == 0x3f && wp[2][0] == 0x41 && !wp[1][1], "three records, their bits");
      uint64_t w1 = rd(pd + PD_WAYPOINTS + 8);
      *(uint8_t *)(uintptr_t)w1 = 3; CHECK(!dm_ftr_waypoints(pd, wp), "a record without its marker"); *(uint8_t *)(uintptr_t)w1 = 2;
      put64(pd + PD_WAYPOINTS + 8, OUTSIDE); CHECK(!dm_ftr_waypoints(pd, wp), "a record outside the heap");
      put64(pd + PD_WAYPOINTS + 8, (uint64_t)(uintptr_t)(arena + sizeof arena - 4)); CHECK(!dm_ftr_waypoints(pd, wp), "a record cut by the heap's end");
      CHECK(!dm_ftr_waypoints(OUTSIDE, wp) && !dm_ftr_waypoints(0, wp), "player data outside the heap or none"); }

    { uint64_t t = alloc(T_SIZE), lv = alloc(3 * LEVEL_SIZE);
      for (int i = 0; i < 3; i++) *(uint8_t *)(uintptr_t)(lv + LEVEL_SIZE * i + LEVEL_WP) = i == 1 ? 255 : (uint8_t)i;
      put64(t + T_LEVELS, lv); put64(t + T_LEVELS + 8, 3);
      CHECK(dm_ftr_waypoint_count(t) == 2, "two levels with a waypoint");
      put64(t + T_LEVELS + 8, 1000); CHECK(dm_ftr_waypoint_count(t) == 0, "a count whose records leave the heap");
      put64(t + T_LEVELS + 8, 3); put64(t + T_LEVELS, OUTSIDE); CHECK(dm_ftr_waypoint_count(t) == 0, "records outside the heap");
      CHECK(dm_ftr_waypoint_count(OUTSIDE) == 0, "a table outside the heap");
      uint64_t rows = alloc(2 * HIRE_SIZE);
      put32(rows + 4, 1); put32(rows + 8, 271); put32(rows + HIRE_ACT, 1); put32(rows + HIRE_SIZE + 4, 6); put32(rows + HIRE_SIZE + 8, 338); put32(rows + HIRE_SIZE + HIRE_ACT, 2);
      put64(t + T_HIRE, rows); put64(t + T_HIRE + 8, 2);
      CHECK(dm_ftr_hireling_act(t, 8, 338) == 2 && dm_ftr_hireling_act(t, 4, 1) == 1 && dm_ftr_hireling_act(t, 8, 359) == 0, "act by class, by hireling id; none");
      put64(t + T_HIRE + 8, 200); CHECK(dm_ftr_hireling_act(t, 8, 338) == -1, "rows leaving the heap: not readable");
      put64(t + T_HIRE + 8, 2); put64(t + T_HIRE, OUTSIDE); CHECK(dm_ftr_hireling_act(t, 8, 338) == -1, "rows outside the heap"); }

    { uint64_t inv = alloc(INV_SIZE), d1, d2, d3; dm_ft_item it[8];
      uint64_t ring = item(7, 122, 0, 0, &d1), axe = item(7, 9, 4, 1, &d2), cur = item(2, 0, 0, 4, &d3), cap = item(7, 3, 1, 1, NULL);
      put32(inv, INV_MAGIC); put64(inv + INV_FIRST, ring); put64(d1 + D_NEXT, axe); put64(d2 + D_NEXT, cap); put64(inv + INV_CURSOR, cur);
      *(uint8_t *)(uintptr_t)(d1 + D_SLOT) = 3;
      int n = dm_ftr_items(inv, it, 8, code, two);
      CHECK(n == 4 && it[0].quality == 7 && it[0].row == 122 && it[0].slot == 0 && it[1].slot == 4 && it[1].two_hands && it[2].slot == 1 && it[3].slot == 0
            && it[3].code == code(cur) && it[0].set == -1, "list and cursor item: %d", n);
      CHECK(two_calls == 2, "the two-hander getter for worn uniques only (%d)", two_calls);
      CHECK(dm_ftr_items(inv, it, 3, code, two) == -1, "more items than room");
      put64(axe + 0x10, OUTSIDE); code_calls = 0;
      CHECK(dm_ftr_items(inv, it, 8, code, two) == -1 && code_calls == 1, "a data pointer outside the heap: ends before the getter reads the unit");
      put64(axe + 0x10, d2); put64(d2 + D_NEXT, OUTSIDE); code_calls = 0;
      CHECK(dm_ftr_items(inv, it, 8, code, two) == -1 && code_calls == 2, "a next pointer outside the heap: ends there");
      put64(d2 + D_NEXT, cap); put32(axe, 1);
      CHECK(dm_ftr_items(inv, it, 8, code, two) == -1, "a unit that is not an item");
      put32(axe, UNIT_ITEM); put32(inv, 0);
      CHECK(dm_ftr_items(inv, it, 8, code, two) == -1 && dm_ftr_items(OUTSIDE, it, 8, code, two) == -1, "no inventory magic; an inventory outside the heap"); }

    { uint64_t inv = alloc(INV_SIZE), d1, d2, d3, d4, d5;
      uint64_t helm = item(2, 0, 0, 0, &d1), gem = item(2, 0, 0, 6, &d2), rune = item(2, 0, 0, 0, &d3), held = item(2, 0, 0, 4, &d4), amu = item(4, 0, 0, 0, &d5);
      put32(helm + U_ID, 11); put32(gem + U_ID, 12); put32(rune + U_ID, 13); put32(held + U_ID, 14); put32(amu + U_ID, 15);
      put32(inv, INV_MAGIC); put64(inv + INV_FIRST, helm); put64(d1 + D_NEXT, rune); put64(d3 + D_NEXT, amu); put64(inv + INV_CURSOR, held);
      uint64_t found = 0;
      CHECK(dm_ftr_find(inv, 13, &found) == 1 && found == rune && dm_ftr_find(inv, 14, &found) == 1 && found == held, "an item of the list, the cursor's");
      CHECK(dm_ftr_find(inv, 12, &found) == 0 && dm_ftr_find(inv, 99, &found) == 0, "the socketed gem is not in the list; an id nobody has");

      uint64_t own = alloc(INV_SIZE); put64(helm + U_SOCKETS, own); put64(own + INV_FIRST, gem);
      int rw = -1;
      CHECK(dm_ftr_socket(helm, 12, &rw) == 1 && rw == 0 && dm_ftr_socket(helm, 13, &rw) == 0, "the gem in the helm's sockets, the rune not");
      put32(d1 + D_FLAGS, 0x04000000); CHECK(dm_ftr_socket(helm, 12, &rw) == 1 && rw == 1, "the runeword flag");
      CHECK(dm_ftr_socket(rune, 12, &rw) == 0 && rw == 0, "an item without sockets");
      put64(d2 + D_NEXT, OUTSIDE); CHECK(dm_ftr_socket(helm, 12, &rw) == -1, "a socket list leaving the heap");
      put64(d2 + D_NEXT, 0); put64(helm + U_SOCKETS, OUTSIDE); CHECK(dm_ftr_socket(helm, 12, &rw) == -1, "a socket inventory outside the heap");
      put64(helm + U_SOCKETS, own);

      *(uint8_t *)(uintptr_t)(d1 + D_PAGE) = 3; *(uint8_t *)(uintptr_t)(d3 + D_PAGE) = 3; *(uint8_t *)(uintptr_t)(d4 + D_PAGE) = 3;
      put32(d3 + D_QUALITY, 2); put32(d1 + D_QUALITY, 3);
      dm_ft_cube_item c[4];
      int n = dm_ftr_cube(inv, c, 4, code);
      CHECK(n == 2 && c[0].id == 11 && c[0].filled == 1 && c[0].quality == 3 && c[0].code == code(helm) && c[1].id == 13 && c[1].filled == 0, "two in the cube: %d", n);
      *(uint8_t *)(uintptr_t)(d5 + D_PAGE) = 3; CHECK(dm_ftr_cube(inv, c, 4, code) == 3, "the amulet put in");
      CHECK(dm_ftr_cube(inv, c, 2, code) == -1, "more than room");
      uint32_t top = 0; CHECK(dm_ftr_top(inv, &top) == 1 && top == 15, "the highest id held, the cursor's included: %u", top);
      put64(d1 + D_NEXT, OUTSIDE); CHECK(dm_ftr_cube(inv, c, 4, code) == -1 && dm_ftr_find(inv, 15, &found) == -1 && dm_ftr_top(inv, &top) == -1, "a list leaving the heap: not readable"); }

    CHECK(dm_ftr_unit(alloc(0x10), 0x10) && !dm_ftr_unit(OUTSIDE, 0x10) && !dm_ftr_unit(0, 0x10), "a unit record");

    { uint64_t d, pile = item(2, 0, 0, 3, &d); int linked = -1; uint32_t type = 9;
      CHECK(dm_ftr_gold_link(pile, &linked, &type) == 1 && linked == 0, "a pile naming nobody");
      put32(pile + U_FLAGS, 0x400 | 0x10); put32(pile + U_LINK_TYPE, 0); put32(pile + U_LINK_TYPE + 4, 7);
      CHECK(dm_ftr_gold_link(pile, &linked, &type) == 1 && linked == 1 && type == 0, "a pile a player dropped");
      put32(pile + U_LINK_TYPE, 1); CHECK(dm_ftr_gold_link(pile, &linked, &type) == 1 && linked == 1 && type == 1, "a pile naming a monster");
      put32(pile + U_FLAGS, 0x10); CHECK(dm_ftr_gold_link(pile, &linked, &type) == 1 && linked == 0, "the flag clear: naming nobody");
      put32(pile, 1); CHECK(dm_ftr_gold_link(pile, &linked, &type) == -1, "not an item"); put32(pile, UNIT_ITEM);
      CHECK(dm_ftr_gold_link(OUTSIDE, &linked, &type) == -1 && dm_ftr_gold_link(0, &linked, &type) == -1, "outside the heap, none");
      CHECK(dm_ftr_gold_link((uint64_t)(uintptr_t)(arena + sizeof arena - 0x100), &linked, &type) == -1, "a record cut by the heap's end");
      put32(pile + U_ID, 77); put32(d + D_QUALITY, 7); uint32_t id = 0; int q = -1;
      CHECK(dm_ftr_quality(pile, &id, &q) == 1 && id == 77 && q == 7, "id and quality");
      put64(pile + U_DATA, OUTSIDE); CHECK(dm_ftr_quality(pile, &id, &q) == -1, "data outside the heap");
      put32(pile, 0); put64(pile + U_DATA, d); CHECK(dm_ftr_quality(pile, &id, &q) == -1, "not an item"); }
    printf("t_feats_read: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
