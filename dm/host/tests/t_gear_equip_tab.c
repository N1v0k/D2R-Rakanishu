



#include <stdio.h>
#include <string.h>
#include "../dm_gear_equip_tab.h"

static int checks, fails; static const char *g_case;
#define CHECK(c) do { checks++; if (!(c)) { fails++; printf("FAIL %s line %d: %s\n", g_case, __LINE__, #c); } } while (0)

static dm_gear_snap S; static dm_gtab_show SHOW[16]; static dm_gtab_row R[16]; static int NR;
static void snap(const char *name) {
    g_case = name; memset(&S, 0, sizeof S); memset(SHOW, 0, sizeof SHOW); memset(R, 0, sizeof R); NR = 0;
    S.show = SHOW; S.p.ncand = 16;
    for (int k = 0; k < DM_GEAR_P_N; k++) S.now.at[k] = -1;
}
static void cand(int c, int where, uint32_t id, uint64_t vh) { SHOW[c].where = where; SHOW[c].id = id; SHOW[c].vhash = vh; }
static int row(int place, int c, int charm, int unit) { int i = NR++; R[i].place = place; R[i].cand = c; R[i].charm = charm; R[i].unit = unit; return i; }
static void needs(int i, int unit) { R[i].needs.w[unit >> 6] |= 1ull << (unit & 63); }
static uint32_t stock(int place, int x, int y) { return DM_GTAB_STOCK_ID | (uint32_t)place << 16 | (uint32_t)y << 8 | (uint32_t)x; }

static void t_single(void) {
    snap("an inventory amulet and a stash ring");
    cand(0, DM_GTAB_INV, 77, 0); cand(1, DM_GTAB_STASH, stock(1, 3, 4), 0xabc); cand(2, DM_GTAB_SHARED, stock(6, 0, 9), 0xdef);
    int a = row(DM_GEAR_P_AMULET, 0, 0, 0), r = row(DM_GEAR_P_RING1, 1, 0, 1), r2 = row(DM_GEAR_P_RING2, 2, 0, 2);
    dm_gew_target t;
    CHECK(!dm_gew_target_of(&S, R, NR, a, &t) && t.nunit == 1 && t.nseq == 1 && t.seq[0] == DM_GO_NECK && t.unit_slots[0] == 1u << DM_GO_NECK);
    CHECK(t.slot[DM_GO_NECK].kind == DM_GEW_UNIT && t.slot[DM_GO_NECK].id == 77);
    for (int s = 1; s < DM_GO_SLOTS; s++) if (s != DM_GO_NECK) CHECK(t.slot[s].kind == DM_GEW_KEEP);
    CHECK(!dm_gew_target_of(&S, R, NR, r, &t) && t.slot[DM_GO_RRING].kind == DM_GEW_STOCK && t.slot[DM_GO_RRING].id == stock(1, 3, 4) && t.slot[DM_GO_RRING].vh == 0xabc);
    CHECK(!dm_gew_target_of(&S, R, NR, r2, &t) && t.slot[DM_GO_LRING].kind == DM_GEW_STOCK && t.slot[DM_GO_LRING].vh == 0xdef);
    CHECK(!dm_gew_target_of(&S, R, NR, -1, &t) && t.nunit == 3 && t.seq[0] == DM_GO_NECK && t.seq[1] == DM_GO_RRING && t.seq[2] == DM_GO_LRING);
    CHECK(dm_gew_target_of(&S, R, NR, NR, &t) == -1 && dm_gew_target_of(&S, R, NR, -2, &t) == -1);
}
static void t_hands(void) {
    snap("a two-hander from the stash: both hands one unit, the left hand emptied");
    cand(3, DM_GTAB_STASH, stock(1, 0, 0), 0x55);
    int m = row(DM_GEAR_P_MAIN, 3, 0, 0), o = row(DM_GEAR_P_OFF, -1, 0, 0);
    dm_gew_target t, u;
    CHECK(!dm_gew_target_of(&S, R, NR, m, &t) && t.nunit == 1 && t.unit_slots[0] == (1u << DM_GO_RIGHT | 1u << DM_GO_LEFT) && t.seq[0] == DM_GO_RIGHT);
    CHECK(t.slot[DM_GO_RIGHT].kind == DM_GEW_STOCK && t.slot[DM_GO_LEFT].kind == DM_GEW_EMPTY);
    CHECK(!dm_gew_target_of(&S, R, NR, o, &u) && !memcmp(&t, &u, sizeof t));
}
static void t_suppliers(void) {
    snap("a row that needs others puts them on first, through others too");
    cand(0, DM_GTAB_SHARED, stock(6, 1, 1), 1); cand(1, DM_GTAB_INV, 10, 0); cand(2, DM_GTAB_STASH, stock(0, 2, 2), 2); cand(3, DM_GTAB_INV, 11, 0);

    int ring = row(DM_GEAR_P_RING1, 0, 0, 5), gl = row(DM_GEAR_P_GLOVES, 1, 0, 9), axe = row(DM_GEAR_P_MAIN, 2, 0, 0), off = row(DM_GEAR_P_OFF, -1, 0, 0), boots = row(DM_GEAR_P_BOOTS, 3, 0, 8);
    needs(gl, 5); needs(axe, 9); (void)off;
    R[ring].first = R[gl].first = 1;
    dm_gew_target t;
    CHECK(!dm_gew_target_of(&S, R, NR, axe, &t) && t.nunit == 3 && t.seq[0] == DM_GO_RRING && t.seq[1] == DM_GO_GLOVES && t.seq[2] == DM_GO_RIGHT);
    CHECK(t.slot[DM_GO_FEET].kind == DM_GEW_KEEP && t.slot[DM_GO_LEFT].kind == DM_GEW_EMPTY);
    CHECK(!dm_gew_target_of(&S, R, NR, gl, &t) && t.nunit == 2 && t.seq[0] == DM_GO_RRING && t.seq[1] == DM_GO_GLOVES && t.slot[DM_GO_RIGHT].kind == DM_GEW_KEEP);
    CHECK(!dm_gew_target_of(&S, R, NR, ring, &t) && t.nunit == 1);
    CHECK(!dm_gew_target_of(&S, R, NR, boots, &t) && t.nunit == 1 && t.seq[0] == DM_GO_FEET);
    CHECK(!dm_gew_target_of(&S, R, NR, -1, &t) && t.nunit == 4 && t.seq[0] == DM_GO_RRING && t.seq[3] == DM_GO_FEET);
}
static void t_no_button(void) {
    snap("charm rows, rows that need them, worn items in another slot, items without a name");
    cand(0, DM_GTAB_INV, 20, 0); cand(1, DM_GTAB_STASH, stock(1, 0, 0), 9); cand(2, DM_GTAB_WORN, 30, 0); cand(3, DM_GTAB_STASH, 31, 0); cand(4, DM_GTAB_INV, 32, 0);
    int ch = row(-1, 1, 1, 2), axe = row(DM_GEAR_P_MAIN, 0, 0, 0), amu = row(DM_GEAR_P_AMULET, 4, 0, 4), ring = row(DM_GEAR_P_RING2, 2, 0, 3), helm = row(DM_GEAR_P_HEAD, 3, 0, 1);
    needs(axe, 2);
    S.now.at[DM_GEAR_P_RING1] = 2;
    dm_gew_target t;
    CHECK(dm_gew_target_of(&S, R, NR, ch, &t) == -1);
    CHECK(dm_gew_target_of(&S, R, NR, axe, &t) == -1);
    CHECK(dm_gew_target_of(&S, R, NR, ring, &t) == -1);
    CHECK(dm_gew_target_of(&S, R, NR, helm, &t) == -1);
    CHECK(!dm_gew_target_of(&S, R, NR, amu, &t) && t.nunit == 1);
    CHECK(dm_gew_target_of(&S, R, NR, -1, &t) == -1);
    S.now.at[DM_GEAR_P_RING2] = 2; S.now.at[DM_GEAR_P_RING1] = -1;
    CHECK(!dm_gew_target_of(&S, R, NR, ring, &t) && t.slot[DM_GO_LRING].kind == DM_GEW_KEEP);
}
static void t_all_without_charms(void) {
    snap("all: every unit but the charm rows");
    cand(0, DM_GTAB_INV, 20, 0); cand(1, DM_GTAB_STASH, stock(2, 5, 5), 7); cand(2, DM_GTAB_INV, 21, 0);
    row(DM_GEAR_P_AMULET, 0, 0, 0); row(-1, 1, 1, 1); row(-1, 2, 2, 2);
    dm_gew_target t;
    CHECK(!dm_gew_target_of(&S, R, NR, -1, &t) && t.nunit == 1 && t.seq[0] == DM_GO_NECK);
    snap("all: nothing but charms");
    cand(1, DM_GTAB_STASH, stock(2, 5, 5), 7); row(-1, 1, 1, 0);
    CHECK(dm_gew_target_of(&S, R, NR, -1, &t) == -1);
}
static void t_units_done(void) {
    g_case = "units in place after whole changes";
    dm_gew_target t; memset(&t, 0, sizeof t); t.nunit = 3;
    t.unit_slots[0] = 1u << DM_GO_RRING; t.unit_slots[1] = 1u << DM_GO_RIGHT; t.unit_slots[2] = 1u << DM_GO_NECK;
    dm_go_plan p; memset(&p, 0, sizeof p); p.nchange = 3;
    p.change[0].slots = 1u << DM_GO_RRING; p.change[1].slots = 1u << DM_GO_RIGHT | 1u << DM_GO_LEFT; p.change[2].slots = 1u << DM_GO_NECK;
    CHECK(dm_gew_units_done(&t, &p, 0) == 0 && dm_gew_units_done(&t, &p, 1) == 1 && dm_gew_units_done(&t, &p, 2) == 2 && dm_gew_units_done(&t, &p, 3) == 3);
    CHECK(dm_gew_units_done(&t, &p, 9) == 3 && dm_gew_units_done(NULL, &p, 3) == 0);
    CHECK(dm_gew_slot_of(DM_GEAR_P_MAIN) == DM_GO_RIGHT && dm_gew_slot_of(DM_GEAR_P_RING2) == DM_GO_LRING && dm_gew_slot_of(-1) == 0 && dm_gew_slot_of(DM_GEAR_P_N) == 0);
}
static void t_ring_places(void) {
    snap("the ring places against the ring slots");
    cand(0, DM_GTAB_WORN, 50, 0); cand(1, DM_GTAB_STASH, stock(1, 0, 0), 5);
    S.now.at[DM_GEAR_P_RING1] = 0;
    int r = row(DM_GEAR_P_RING2, 1, 0, 0);
    dm_gew_target t;
    CHECK(!dm_gew_target_of(&S, R, NR, r, &t) && t.ring_was[0] == 50 && t.ring_was[1] == 0 && t.slot[DM_GO_LRING].kind == DM_GEW_STOCK);
    CHECK(dm_gew_ring_swap(&t, 50, 0) == 0);
    CHECK(dm_gew_ring_swap(&t, 0, 50) == 1);
    CHECK(dm_gew_ring_swap(&t, 0, 0) == -1 && dm_gew_ring_swap(&t, 51, 0) == -1 && dm_gew_ring_swap(&t, 50, 51) == -1);

    dm_go_plan p; memset(&p, 0, sizeof p); p.nchange = 2; p.change[0].slots = 1u << DM_GO_RRING; p.change[1].slots = 1u << DM_GO_NECK;
    dm_gew_target u; memset(&u, 0, sizeof u); u.nunit = 3; u.unit_slots[0] = 1u << DM_GO_LRING; u.unit_slots[1] = 1u << DM_GO_RRING; u.unit_slots[2] = 1u << DM_GO_NECK;
    CHECK(dm_gew_units_done(&u, &p, 1) == 1 && dm_gew_units_done(&u, &p, 2) == 2);
}

int main(void) {
    t_single();
    t_hands();
    t_suppliers();
    t_no_button();
    t_all_without_charms();
    t_units_done();
    t_ring_places();
    printf("t_gear_equip_tab: %d checks, %d failures\n", checks, fails);
    return fails != 0;
}
