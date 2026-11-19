

#define _POSIX_C_SOURCE 200809L
#include "../dm_gear_compose.h"
#include "../dm_itemcmp_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures, checks;
#define CHECK(cond, ...) do { checks++; if (!(cond)) { failures++; fprintf(stderr, "FAIL %s:%d: ", __FILE__, __LINE__); fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); } } while (0)

enum { STR = 0, ENE = 1, DEX = 2, VIT = 3, HP = 7, MANA = 9, AC = 31, FIRE = 39, LIGHT = 41, COLD = 43, POIS = 45, MAXFIRE = 40, IAS = 93, FCR = 105,
       HP_LVL = 216, VIT_LVL = 223, AC_LVL = 214, HP_PCT = 76, CLASS = 83, TAB = 188 };




static const char *TABLES =
    "DM_GEAR\t1\tsynthetic\n"
    "op\t1\t8\t0\t0\t9\n"
    "op\t3\t9\t0\t0\t7,11\n"
    "op\t76\t11\t0\t0\t7\n"
    "op\t214\t4\t3\t1\t31\n"
    "op\t216\t2\t3\t1\t7\n"
    "op\t220\t2\t3\t1\t0\n"
    "op\t223\t2\t3\t1\t3\n"
    "op\t268\t6\t0\t0\t31\n"
    "shift\t7\t8\n"
    "shift\t9\t8\n"
    "set\t0\t3\n"
    "set\t1\t2\n"
    "setb\t0\t2\t31\t0\t50\n"
    "setb\t0\t0\t93\t0\t20\n"
    "setx\t1\t2\t97\n"
    "setb\t1\t0\t39\t0\t5\n"
    "item\t0\t0\t2\n"
    "item\t1\t0\t2\n"
    "item\t2\t0\t2\n"
    "item\t3\t1\t1\n"
    "item\t4\t1\t0\n"
    "itemb\t0\t1\t39\t0\t10\n"
    "itemb\t0\t1\t41\t0\t10\n"
    "itemb\t0\t1\t43\t0\t10\n"
    "itemb\t0\t1\t45\t0\t10\n"
    "itemb\t0\t2\t9\t0\t5120\n"
    "itemb\t1\t1\t0\t0\t10\n"
    "itemx\t2\t1\t151\n"
    "itemb\t3\t1\t105\t0\t10\n"
    "end\t31\n";

static dm_gear_tables T;
static dm_gear_base base(int str, int dex, int vit, int ene, int clvl) {
    dm_gear_base b; memset(&b, 0, sizeof b); b.cls = 7; b.clvl = clvl; b.lpv = 8; b.mpm = 8;
    dm_gear_list_add(&b.base, STR, 0, str); dm_gear_list_add(&b.base, DEX, 0, dex); dm_gear_list_add(&b.base, VIT, 0, vit); dm_gear_list_add(&b.base, ENE, 0, ene);
    dm_gear_list_add(&b.base, HP, 0, 300 << 8); dm_gear_list_add(&b.base, MANA, 0, 100 << 8); dm_gear_list_add(&b.base, 12, 0, clvl);
    return b;
}
static dm_gear_item item(uint32_t id, int slot, int setitem) {
    dm_gear_item it; memset(&it, 0, sizeof it); it.id = id; it.slot = slot; it.setitem = setitem; return it;
}
static dm_gear_char C;
static int compose(const dm_gear_base *b, const dm_gear_item *items, int n) { return dm_gear_compose(&T, b, items, n, NULL, &C); }

static void parse_tests(void) {
    CHECK(dm_gear_tables_parse(&T, TABLES) == 0 && T.ok, "synthetic tables parse");
    CHECK(T.nset == 2 && T.nitem == 5 && T.nsetb == 4 && T.nitemb == 8 && T.nop == 8 && T.shift[HP] == 8, "table counts %d %d %d %d %d", T.nset, T.nitem, T.nsetb, T.nitemb, T.nop);
    static dm_gear_tables bad;
    static const char *broken[] = {
        "DM_GEAR\t2\tx\nset\t0\t1\nitem\t0\t0\t2\nop\t3\t9\t0\t0\t7\nend\t5\n",
        "DM_GEAR\t1\tx\nset\t0\t1\nitem\t0\t0\t2\nop\t3\t9\t0\t0\t7\nend\t9\n",
        "DM_GEAR\t1\tx\nset\t0\t1\nitem\t0\t1\t2\nop\t3\t9\t0\t0\t7\nend\t5\n",
        "DM_GEAR\t1\tx\nset\t0\t1\nitem\t0\t0\t2\nop\t3\t9\t0\t0\t7\nitemb\t0\t6\t39\t0\t1\nend\t6\n",
        "DM_GEAR\t1\tx\nset\t0\t1\nitem\t0\t0\t2\nop\t3\t9\t0\t0\t7\nsetb\t0\t1\t39\t0\t1\nend\t6\n",
        "DM_GEAR\t1\tx\nset\t0\t1\nitem\t0\t0\t2\nop\t3\t9\t0\t0\tx\nend\t5\n",
        "DM_GEAR\t1\tx\nset\t0\t1\nitem\t0\t0\t2\nop\t3\t9\t0\t0\t7\nwhat\t1\nend\t6\n",
        "DM_GEAR\t1\tx\nset\t0\t1\nitem\t0\t0\t2\nop\t3\t9\t0\t0\t7\n",
    };
    for (size_t i = 0; i < sizeof broken / sizeof *broken; i++) CHECK(dm_gear_tables_parse(&bad, broken[i]) < 0 && !bad.ok, "broken table %zu refused", i);
}

static void plain_tests(void) {
    dm_gear_base b = base(100, 100, 50, 30, 40);
    CHECK(compose(&b, NULL, 0) == 0 && C.value[STR] == 100 && C.value[HP] == 300 << 8 && C.value[MANA] == 100 << 8,
          "naked: the base keeps its own life and mana (life %d)", C.value[HP]);
    dm_gear_item r[3] = { item(1, 6, -1), item(2, 7, -1), item(3, 2, -1) };
    dm_gear_list_add(&r[0].own, FCR, 0, 10); dm_gear_list_add(&r[1].own, FCR, 0, 10); dm_gear_list_add(&r[2].own, FCR, 0, 15);
    dm_gear_list_add(&r[0].own, VIT, 0, 5); dm_gear_list_add(&r[1].own, HP, 0, 20 << 8);
    compose(&b, r, 3);
    CHECK(C.value[FCR] == 35 && C.active[0] && C.active[1] && C.active[2], "ring pair and amulet: cast rate adds over three slots (%d)", C.value[FCR]);
    CHECK(C.value[HP] == (320 << 8) + 5 * 8 * 64, "flat life and the life of the ring's vitality (%d)", C.value[HP]);

    dm_gear_item w2 = item(4, 4, -1), w1[2] = { item(5, 4, -1), item(6, 5, -1) };
    dm_gear_list_add(&w2.own, AC, 0, 0); dm_gear_list_add(&w2.own, 21, 0, 30); dm_gear_list_add(&w1[0].own, 21, 0, 10); dm_gear_list_add(&w1[1].own, AC, 0, 40);
    compose(&b, &w2, 1); int two = C.value[21], ac2 = C.value[AC];
    compose(&b, w1, 2);
    CHECK(two == 30 && ac2 == 0 && C.value[21] == 10 && C.value[AC] == 40, "two-handed %d/%d against one hand and shield %d/%d", two, ac2, C.value[21], C.value[AC]);

    dm_gear_item bq[2] = { item(7, 4, -1), item(8, 5, -1) }; dm_gear_list_add(&bq[0].own, 21, 0, 5); dm_gear_list_add(&bq[1].own, 21, 0, 0); dm_gear_list_add(&bq[1].own, 22, 0, 4);
    compose(&b, bq, 2);
    CHECK(C.value[21] == 5 && C.value[22] == 4 && C.active[1], "bow and quiver");

    dm_gear_item f = item(9, 3, -1); dm_gear_list_add(&f.own, FIRE, 0, 110); dm_gear_list_add(&f.own, MAXFIRE, 0, 5);
    compose(&b, &f, 1);
    CHECK(C.value[FIRE] == 110 && dm_ic_res_shown(C.value[FIRE], C.value[MAXFIRE], -100) == 10 && dm_ic_res_shown(C.value[FIRE] + 100, C.value[MAXFIRE], -100) == 80,
          "resistance under and over the cap of 80 (%d)", dm_ic_res_shown(C.value[FIRE] + 100, C.value[MAXFIRE], -100));
}

static void op_tests(void) {
    dm_gear_base b = base(100, 100, 50, 30, 40);
    dm_gear_item it = item(1, 3, -1);
    dm_gear_list_add(&it.own, HP_LVL, 0, 8 << 8);
    dm_gear_list_add(&it.own, VIT_LVL, 0, 4);
    compose(&b, &it, 1);
    CHECK(C.value[VIT] == 50 + 20 && C.value[HP] == (300 << 8) + (40 << 8) + 20 * 8 * 64 && !(C.flags & DM_GEAR_C_ITEMOP) && !C.value[HP_LVL] && !C.value[VIT_LVL],
          "per level life and vitality, the vitality feeding life (%d %d)", C.value[VIT], C.value[HP]);
    dm_gear_item ac = item(2, 3, -1); dm_gear_list_add(&ac.own, AC_LVL, 0, 8);
    compose(&b, &ac, 1);
    CHECK(C.value[AC] == 0 && !(C.flags & DM_GEAR_C_ITEMOP) && C.value[AC_LVL] == 8, "an item's own defense per level is in its own defense already");
    dm_gear_item pct = item(3, 3, -1); dm_gear_list_add(&pct.own, HP_PCT, 0, 10);
    compose(&b, &pct, 1);
    CHECK((C.flags & DM_GEAR_C_PCT) && C.value[HP] == 300 << 8, "a life percent is marked, life without it");
    dm_gear_base bo = b; dm_gear_list_add(&bo.base, HP_PCT, 0, 35);
    compose(&bo, &it, 1);
    CHECK(C.flags & DM_GEAR_C_PCT, "a life percent in the base (a buff) is marked too");
    dm_gear_item tm = item(4, 3, -1); dm_gear_list_add(&tm.own, 268, 0, 3);
    compose(&b, &tm, 1);
    CHECK(C.flags & DM_GEAR_C_ITEMOP, "a by time value is marked");
    dm_gear_item sk = item(5, 3, -1); dm_gear_list_add(&sk.own, CLASS, 7, 1); dm_gear_list_add(&sk.own, TAB, 57, 2);
    compose(&b, &sk, 1);
    CHECK(dm_gear_list_get(&C.raw, CLASS, 7) == 1 && dm_gear_list_get(&C.raw, TAB, 57) == 2 && C.value[CLASS] == 0, "class and tab skill bonuses keep their layers");
}

static void set_tests(void) {
    dm_gear_base b = base(100, 100, 50, 30, 40);
    dm_gear_item s[3] = { item(10, 3, 0), item(11, 10, 1), item(12, 9, 2) };
    compose(&b, s, 1);
    CHECK(C.value[FIRE] == 0 && C.value[AC] == 0 && C.nworn == 1 && C.worn[0].pieces == 1, "one piece: no bonus");
    compose(&b, s, 2);
    CHECK(C.value[FIRE] == 10 && C.value[COLD] == 10 && C.value[STR] == 110 && C.value[AC] == 50 && C.value[MANA] == 100 << 8 && C.value[IAS] == 0,
          "two pieces: both pieces' first green bonus and the two-piece set bonus (fire %d str %d ac %d)", C.value[FIRE], C.value[STR], C.value[AC]);
    CHECK(!(C.flags & DM_GEAR_C_UNMAPPED), "no unmappable bonus active with two pieces");
    compose(&b, s, 3);
    CHECK(C.value[MANA] == 120 << 8 && C.value[IAS] == 20 && C.value[AC] == 50 && C.worn[0].pieces == 3, "full set: second green bonus and the full set bonus");
    CHECK((C.flags & DM_GEAR_C_UNMAPPED) && C.nx == 1 && C.xstat[0] == 151, "row 2's unmappable green bonus is named");

    dm_gear_item d[2] = { item(10, 6, 0), item(13, 7, 0) };
    compose(&b, d, 2);
    CHECK(C.worn[0].pieces == 1 && C.value[FIRE] == 0, "duplicate piece counts once");

    dm_gear_item r[2] = { item(10, 3, 0), item(11, 10, 1) };
    r[0].green_read = 1; r[0].green[0].n = 1; r[0].green[0].v[0] = (dm_gear_v){ FIRE, 0, 12 };
    compose(&b, r, 2);
    CHECK(C.value[FIRE] == 12 && C.value[COLD] == 0, "read green bonus used (%d)", C.value[FIRE]);

    r[0].green[0].n = 2; r[0].green[0].v[1] = (dm_gear_v){ AC_LVL, 0, 8 };
    compose(&b, r, 2);
    CHECK(C.value[AC] == 50 + 40 && !(C.flags & DM_GEAR_C_ITEMOP), "green defense per level converts (%d)", C.value[AC]);
    compose(&b, r, 1);
    CHECK(C.value[AC] == 0, "green defense per level off with one piece");

    dm_gear_base b9 = base(100, 100, 50, 30, 9);
    dm_gear_item two[3] = { item(10, 3, 0), item(11, 10, 1), item(12, 9, 2) };
    for (int i = 0; i < 2; i++) { two[i].green_read = 1; two[i].green[0].n = 1; two[i].green[0].v[0] = (dm_gear_v){ AC_LVL, 0, 20 }; }
    compose(&b9, two, 2);
    CHECK(C.value[AC] == 50 + 22 + 22 && !(C.flags & DM_GEAR_C_ITEMOP), "two green defenses per level round per item (%d)", C.value[AC]);
    int both = C.value[AC];
    compose(&b9, two, 1);
    CHECK(both - C.value[AC] == 50 + 22 + 22, "taking one piece off drops both and the set bonus (%d)", both - C.value[AC]);
    static dm_gear_tables t2; t2 = T;
    for (int r = 0; r < 2; r++) t2.itemb[t2.nitemb++] = (dm_gear_bonus){ (int16_t)r, 1, AC_LVL, 0, 0, 20 };
    dm_gear_item tb[2] = { item(10, 3, 0), item(11, 10, 1) };
    dm_gear_compose(&t2, &b9, tb, 2, NULL, &C);
    CHECK(C.value[AC] == 50 + 22 + 22, "the tables' green defenses per level round per item too (%d)", C.value[AC]);

    dm_gear_item p[2] = { item(20, 6, 3), item(21, 7, 4) };
    compose(&b, p, 1);
    CHECK(C.value[FCR] == 0 && (C.flags & DM_GEAR_C_FUNC1), "per other piece: alone no bonus, marked");
    compose(&b, p, 2);
    CHECK(C.value[FCR] == 10 && C.value[FIRE] == 5 && (C.flags & DM_GEAR_C_UNMAPPED) && C.xstat[0] == 97, "per other piece: with row 4, full set bonus and the unmappable two-piece one");

    dm_gear_item q[2] = { item(10, 3, 0), item(11, 10, 1) }; q[1].req_str = 500;
    compose(&b, q, 2);
    CHECK(!C.active[1] && C.worn[0].pieces == 1 && C.value[FIRE] == 0, "unusable piece adds no piece");
}

static void requirement_tests(void) {

    dm_gear_base b = base(45, 45, 50, 30, 40);
    dm_gear_item a = item(1, 3, -1); a.req_str = 50; dm_gear_list_add(&a.own, STR, 0, 10);
    compose(&b, &a, 1);

    CHECK(!C.active[0] && C.value[STR] == 45 && (C.flags & DM_GEAR_C_ORDER), "own strength bridging its own requirement: not rateable");
    dm_gear_item a3 = item(1, 3, -1); a3.req_str = 50; dm_gear_list_add(&a3.own, STR, 0, 3);
    compose(&b, &a3, 1);
    CHECK(!C.active[0] && C.value[STR] == 45 && !(C.flags & DM_GEAR_C_ORDER), "own strength short of its own requirement either way");
    dm_gear_item ab[2] = { a, item(2, 6, -1) }; dm_gear_list_add(&ab[1].own, STR, 0, 5);
    compose(&b, ab, 2);
    CHECK(C.active[0] && C.active[1] && C.value[STR] == 60 && !(C.flags & DM_GEAR_C_ORDER), "a ring lifts the strength: chain closes (%d)", C.value[STR]);

    dm_gear_item ch[3] = { item(1, 6, -1), item(2, 7, -1), item(3, 3, -1) };
    dm_gear_list_add(&ch[0].own, STR, 0, 10); ch[1].req_str = 55; dm_gear_list_add(&ch[1].own, DEX, 0, 10); ch[2].req_dex = 55;
    compose(&b, ch, 3);
    CHECK(C.active[0] && C.active[1] && C.active[2] && !(C.flags & DM_GEAR_C_ORDER), "chain of three resolves");
    ch[0].req_lvl = 41;
    compose(&b, ch, 3);
    CHECK(!C.active[0] && !C.active[1] && !C.active[2] && !(C.flags & DM_GEAR_C_ORDER), "the chain falls with its first link (level)");

    dm_gear_item cy[2] = { item(1, 6, -1), item(2, 7, -1) };
    cy[0].req_dex = 50; dm_gear_list_add(&cy[0].own, STR, 0, 10); cy[1].req_str = 50; dm_gear_list_add(&cy[1].own, DEX, 0, 10);
    compose(&b, cy, 2);
    CHECK(C.flags & DM_GEAR_C_ORDER, "cycle: two stable states, not rateable");

    dm_gear_base b2 = base(102, 45, 50, 30, 40);
    dm_gear_item sc[2] = { item(10, 3, 0), item(11, 10, 1) }; sc[0].req_str = 110;
    compose(&b2, sc, 2);
    CHECK(C.flags & DM_GEAR_C_ORDER, "strength from the set bonus the item completes itself: not rateable");

    unsigned char given[2] = { 1, 0 };
    CHECK(dm_gear_compose(&T, &b, ab, 2, given, &C) == 0 && C.active[0] && !C.active[1] && C.value[STR] == 55, "given verdict used");
}



static void capture_tests(void) {
    static const int prof[8][4] = { { 150, 120, 200, 50 }, { 60, 80, 150, 200 }, { 90, 70, 250, 60 }, { 140, 90, 220, 40 }, { 180, 100, 260, 30 },
                                    { 110, 80, 240, 80 }, { 120, 160, 200, 40 }, { 95, 80, 330, 20 } };
    for (int cls = 0; cls < 8; cls++) {
        dm_gear_base naked = base(prof[cls][0], prof[cls][1], prof[cls][2], prof[cls][3], 80); naked.cls = cls;
        dm_gear_item g[6] = { item(1, 3, 0), item(2, 10, 1), item(3, 9, 2), item(4, 6, -1), item(5, 7, 3), item(6, 1, -1) };
        dm_gear_list_add(&g[0].own, AC, 0, 300); dm_gear_list_add(&g[0].own, VIT_LVL, 0, 8); dm_gear_list_add(&g[3].own, FCR, 0, 10);
        dm_gear_list_add(&g[3].own, STR, 0, 5); dm_gear_list_add(&g[4].own, HP, 0, 25 << 8); dm_gear_list_add(&g[5].own, CLASS, cls, 2); g[5].req_str = 1000;
        dm_gear_char capture;
        dm_gear_compose(&T, &naked, g, 6, NULL, &capture);
        CHECK(!capture.active[5] && capture.active[0], "class %d: helm unusable, armour worn", cls);

        dm_gear_base empty; memset(&empty, 0, sizeof empty); empty.cls = cls; empty.clvl = 80; empty.lpv = 8; empty.mpm = 8;
        dm_gear_char worn; dm_gear_compose(&T, &empty, g, 6, capture.active, &worn);

        dm_gear_base back = empty; memset(&back.base, 0, sizeof back.base);
        for (int s = 0; s < DM_GEAR_NSTAT; s++) dm_gear_list_add(&back.base, s, 0, capture.value[s] - worn.value[s]);
        for (int i = 0; i < capture.raw.n; i++) if (capture.raw.v[i].layer) dm_gear_list_add(&back.base, capture.raw.v[i].stat, capture.raw.v[i].layer, capture.raw.v[i].value);
        for (int i = 0; i < worn.raw.n; i++) if (worn.raw.v[i].layer) dm_gear_list_add(&back.base, worn.raw.v[i].stat, worn.raw.v[i].layer, -worn.raw.v[i].value);
        dm_gear_compose(&T, &back, g, 6, NULL, &C);
        int same = !memcmp(C.value, capture.value, sizeof C.value) && !memcmp(C.active, capture.active, sizeof C.active);
        CHECK(same, "class %d: composing the worn gear again gives the capture (str %d/%d life %d/%d)", cls, C.value[STR], capture.value[STR], C.value[HP], capture.value[HP]);
        static const int col[4] = { 0, 3, 1, 2 };
        for (int s = 0; s < 4; s++) CHECK(dm_gear_list_get(&back.base, s, 0) == prof[cls][col[s]], "class %d: base attribute %d back (%d)", cls, s, dm_gear_list_get(&back.base, s, 0));
    }
}

static void pack_tests(const char *path) {
    FILE *f = fopen(path, "rb"); if (!f) { CHECK(0, "cannot read %s", path); return; }
    static char text[1 << 20]; size_t n = fread(text, 1, sizeof text - 1, f); fclose(f); text[n] = 0;
    static dm_gear_tables t;
    CHECK(dm_gear_tables_parse(&t, text) == 0, "pack gear data parses");
    int bad = 0;
    for (int i = 0; i < t.nitem; i++) bad |= t.item[i].set < 0 || t.item[i].set >= t.nset;
    int pieces = 0; for (int s = 0; s < t.nset; s++) pieces += t.pieces[s];
    CHECK(!bad && pieces == t.nitem && t.nsetb > 0 && t.nitemb > 0, "pack: every set item in a set, pieces add up (%d/%d)", pieces, t.nitem);
    int vit = 0, ene = 0; for (int k = 0; k < t.nop; k++) { vit |= t.op[k].stat == 3 && t.op[k].op == 9; ene |= t.op[k].stat == 1 && t.op[k].op == 8; }
    CHECK(vit && ene && t.shift[7] == 8 && t.shift[9] == 8, "pack: vitality / energy ops and the life / mana shift");
    printf("gear data: %d sets, %d set items, %d set bonuses, %d green bonuses, %d ops\n", t.nset, t.nitem, t.nsetb, t.nitemb, t.nop);
}

int main(void) {
    parse_tests();
    if (!T.ok) { fprintf(stderr, "t_gear_compose: synthetic tables did not parse\n"); return 1; }
    plain_tests(); op_tests(); set_tests(); requirement_tests(); capture_tests();
    const char *p = getenv("DM_GEAR_DATA"); if (p && *p) pack_tests(p);
    printf("t_gear_compose: %d checks, %d failed\n", checks, failures);
    return failures != 0;
}
