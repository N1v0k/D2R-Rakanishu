


#include "../dm_itemcmp_core.h"
#include "../dm_tipx_core.h"
#include "../dm_splash_rule.h"
#include <stdio.h>
#include <string.h>

static int fails, checks;
#define CHECK(c, ...) do { checks++; if (!(c)) { fails++; printf("FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)
static const char *const LB[IC_M_N] = { "FCR", "FHR", "FBR", "IAS", "Fire res", "Cold res", "Light res", "Poison res", "Life", "Mana", "All skills", "Class skills",
                                        "Str", "Dex", "Vit", "Ene", "MF", "GF", "FRW", "LL", "ML" };
#define CC "\xee\x81\xbe"
static const char TO[] = "%s auf %s";
static void zero(dm_ic_in *in) { memset(in, 0, sizeof *in); in->lpv = 7; in->mpm = 5; }
static const dm_ic_line *find(const dm_ic_line *l, int n, int m) { for (int i = 0; i < n; i++) if (l[i].metric == m) return &l[i]; return NULL; }

static void t_math(void) {
    dm_ic_line l[IC_M_N]; dm_ic_in in;

    zero(&in); in.tot[IC_S_VIT] = 30; in.tot[IC_S_ENE] = 20; in.tot[IC_S_STR] = 40; in.tot[IC_S_HP] = 25600; in.tot[IC_S_MANA] = 15360; in.tot[IC_S_FBR] = 25;
    in.old_[IC_S_VIT] = 6; in.old_[IC_S_ENE] = 4; in.old_[IC_S_STR] = 3; in.old_[IC_S_FBR] = 25;
    int n = dm_ic_compute(&in, LB, "Cap", TO, l, IC_M_N); const dm_ic_line *x;
    CHECK((x = find(l, n, IC_M_LIFE)) && x->before == 100 && x->after == 89, "Life from Vitality (op 9): %d > %d", x ? x->before : -1, x ? x->after : -1);
    CHECK((x = find(l, n, IC_M_MANA)) && x->before == 60 && x->after == 55, "Mana from Energy (op 8)");
    CHECK((x = find(l, n, IC_M_FBR)) && x->after == 0 && strstr(x->text, CC "1" "-" "25 (25 % auf 0 %)"), "FBR red with a minus: %s", x ? x->text : "");
    CHECK(find(l, n, IC_M_FCR) == NULL, "unchanged values are absent");
    CHECK(l[0].metric == IC_M_FBR, "Order: speeds first");

    for (int i = 1; i < n; i++) CHECK(l[i].metric > l[i - 1].metric, "order %d", i);

    CHECK(!find(l, n, IC_M_VIT) && !find(l, n, IC_M_ENE) && !find(l, n, IC_M_STR), "no base-value lines");
    zero(&in); in.new_[IC_S_ENE] = 10; in.new_[IC_S_ALLSK] = 1; in.new_[IC_S_CLSK] = 2; in.new_[IC_S_DEX] = 5; in.tot[IC_S_MANA] = 29 * 256;
    n = dm_ic_compute(&in, LB, "Cap", TO, l, IC_M_N);
    CHECK(n == 1 && l[0].metric == IC_M_MANA && l[0].before == 29 && l[0].after == 29 + 10 * 5 / 4 && strstr(l[0].text, CC "0Mana " CC "3+12 (29 auf 41)"), "mana only, blue: %s", n ? l[0].text : "");
    for (int m = 0; m < IC_M_N; m++) if (IC_M_HIDDEN & 1u << m) { zero(&in); in.new_[IC_S_STR] = in.new_[IC_S_DEX] = in.new_[IC_S_VIT] = in.new_[IC_S_ENE] = 3; in.new_[IC_S_ALLSK] = in.new_[IC_S_CLSK] = 1; in.lpv = in.mpm = 0;
        CHECK(dm_ic_compute(&in, LB, "Cap", TO, l, IC_M_N) == 0, "hidden metric %d without a line", m); }

    CHECK(dm_ic_res_shown(95, 25, 0) == 95 && dm_ic_res_shown(95, 0, 0) == 75 && dm_ic_res_shown(95, 30, 0) == 95, "Cap 75 + Max, höchstens 95");
    CHECK(dm_ic_res_shown(50, 0, -100) == -50 && dm_ic_res_shown(140, 0, -40) == 75, "Hölle −100, Albtraum −40");
    CHECK(dm_ic_res_shown(-30, 0, -100) == -100 && dm_ic_res_shown(-10, 0, -100) == -100, "lower bound -100");
    zero(&in); in.tot[IC_S_LIGHT] = 64; in.new_[IC_S_LIGHT] = 11;
    n = dm_ic_compute(&in, LB, "Cap", TO, l, IC_M_N);
    CHECK(n == 1 && l[0].cap == 1 && strstr(l[0].text, CC "3+11 (64 % auf 75 %) " CC "4Cap"), "cap reached: %s", n ? l[0].text : "");
    zero(&in); in.tot[IC_S_FIRE] = 95; in.tot[IC_S_MAXFIRE] = 25; in.old_[IC_S_FIRE] = 45; in.old_[IC_S_MAXFIRE] = 15;
    n = dm_ic_compute(&in, LB, "Cap", TO, l, IC_M_N);
    CHECK(n == 1 && l[0].before == 95 && l[0].after == 50 && l[0].cap == -1, "Cap verlassen: %d > %d", n ? l[0].before : 0, n ? l[0].after : 0);
    CHECK(!strstr(l[0].text, "Cap") && !strstr(l[0].text, CC "4"), "leaving cap has no gold label");
    zero(&in); in.tot[IC_S_LIGHT] = 64; in.new_[IC_S_LIGHT] = 11; in.hypothetical = 1;
    n = dm_ic_compute(&in, LB, "Cap", TO, l, IC_M_N);
    CHECK(n == 1 && l[0].cap == 1 && !strstr(l[0].text, "Cap") && !strstr(l[0].text, CC "4"), "hypothetical resistance stays grey");
    zero(&in); in.tot[IC_S_FIRE] = 120; in.new_[IC_S_FIRE] = 10;
    CHECK(dm_ic_compute(&in, LB, "Cap", TO, l, IC_M_N) == 0, "above the cap: no line");
    zero(&in); in.tot[IC_S_FIRE] = 30; in.new_[IC_S_FIRE] = 20; in.penalty = -100;
    n = dm_ic_compute(&in, LB, "Cap", TO, l, IC_M_N);
    CHECK(n == 1 && l[0].before == -70 && l[0].after == -50 && strstr(l[0].text, "(-" "70 % auf -" "50 %)"), "negative at Hell difficulty: %s", n ? l[0].text : "");

    zero(&in); in.tot[IC_S_HP] = 14080 + 100; in.new_[IC_S_HP] = 100;
    CHECK(dm_ic_compute(&in, LB, "Cap", TO, l, IC_M_N) == 0, "life below 1 point: no line");

    zero(&in); in.new_[IC_S_HP] = 256 * 20; in.new_[IC_S_FCR] = 10; in.blocked = 1u << IC_M_LIFE; in.hypothetical = 1;
    n = dm_ic_compute(&in, LB, "Cap", TO, l, IC_M_N);
    CHECK(n == 1 && l[0].metric == IC_M_FCR && strstr(l[0].text, CC "5FCR " CC "5+10"), "locked + hypothetical grey: %s", n ? l[0].text : "");

    CHECK(dm_ic_req_value(80, 0, 0) == 80 && dm_ic_req_value(80, 50, 0) == 120 && dm_ic_req_value(80, -20, 1) == 54, "requirement with %% and ethereal");

    CHECK(dm_ic_req_outcome(50, 60, 55, 0) == IC_REQ_SAME && dm_ic_req_outcome(50, 40, 45, 0) == IC_REQ_SAME, "no flip");
    CHECK(dm_ic_req_outcome(50, 52, 45, 0) == IC_REQ_LOSS && dm_ic_req_outcome(50, 50, 49, 0) == IC_REQ_LOSS, "not met afterwards (boundary exact)");
    CHECK(dm_ic_req_outcome(50, 45, 60, 0) == IC_REQ_GAIN && dm_ic_req_outcome(50, 49, 50, 0) == IC_REQ_GAIN, "met afterwards (boundary exact)");

    CHECK(!dm_ic_req_kept(25, 24, 0) && dm_ic_req_kept(25, 25, 0) && dm_ic_req_kept(25, 26, 0), "weapon with +10 dexterity: own bonus does not count");

    CHECK(!dm_ic_req_kept(25, 26, 2) && dm_ic_req_kept(25, 27, 2) && dm_ic_req_kept(0, 0, 5), "displaced pieces do not count afterwards");
    CHECK(dm_ic_req_outcome(50, 52, 55, 5) == IC_REQ_AMBIG, "own bonus: one computation flips, the other does not → ambiguous");
    CHECK(dm_ic_req_outcome(50, 58, 49, 5) == IC_REQ_LOSS && dm_ic_req_outcome(50, 40, 60, 5) == IC_REQ_GAIN, "both computations agree despite the own bonus");
    CHECK(dm_ic_req_outcome(50, 52, 54, 3) == IC_REQ_AMBIG, "with bonus: 49/52 before, 51/54 after → ambiguous");
}

static void t_compose(void) {
    static char out[16384]; size_t len;
    const char *orig = CC "3Magieschaden reduziert um 2\n" CC "0Ben\xc3\xb6tigte Stufe: 12\n" CC "3Ring des W\xc3\xa4" "chters";
    dm_tipx_line ln[] = { { CC "0Beim Ausr\xc3\xbcsten " CC "5statt Ring rechts", DM_TIPX_HEAD }, { CC "2Leben +35", 0 }, { CC "1Feuer -6", 0 }, { CC "5Keine weiteren", 0 } };
    dm_tipx_cfg c = { .max_lines = 28, .max_chars = 60, .name_lines = 1, .hint = CC "5... nochmal tippen: Seite %d/%d", .props = CC "5Eigenschaften: Seite 1" };
    int p = dm_tipx_compose(orig, ln, 4, &c, 0, out, sizeof out, &len);
    CHECK(p == 1, "one page: %d", p);
    const char *want = CC "5Keine weiteren\n" CC "1Feuer -6\n" CC "2Leben +35\n" CC "0Beim Ausr\xc3\xbcsten " CC "5statt Ring rechts\n";
    CHECK(!strncmp(out, want, strlen(want)) && !strcmp(out + strlen(want), orig), "umgekehrt vor dem Original, Original unverändert");
    CHECK(len == strlen(out) && dm_tipx_text_lines(out) == 7, "lines %d", dm_tipx_text_lines(out));

    c.max_lines = 3 + 1 + 3;
    p = dm_tipx_compose(orig, ln, 4, &c, 0, out, sizeof out, &len);
    CHECK(p == 2, "two pages: %d", p);
    CHECK(dm_tipx_text_lines(out) == 3 + 3 && strstr(out, "Seite 1/2") && strstr(out, "Leben +35") && !strstr(out, "Feuer -6"), "page 1 full with the note: %s", out);
    p = dm_tipx_compose(orig, ln, 4, &c, 1, out, sizeof out, &len);
    CHECK(p == 2 && strstr(out, "Seite 2/2") && strstr(out, "Keine weiteren") && strstr(out, "Feuer -6") && !strstr(out, "Leben") && strstr(out, "Ring des W") && !strstr(out, "Magieschaden")
          && strstr(out, "Eigenschaften: Seite 1") && strstr(out, "Beim Ausr"), "detail page: name + rest + head: %s", out);
    p = dm_tipx_compose(orig, ln, 4, &c, 5, out, sizeof out, &len);
    CHECK(p == 2 && strstr(out, "Seite 1/2"), "page out of range → page 1");

    dm_tipx_line lw[] = { { CC "0Beim Ausr\xc3\xbcsten", DM_TIPX_HEAD }, { CC "1Nicht nutzbar: Stufe 62 (du 30)", DM_TIPX_WARN }, { CC "2A +1", 0 }, { CC "2B +2", 0 }, { CC "2C +3", 0 } };
    static char longo[2048]; int lo = 0; for (int i = 0; i < 20; i++) lo += snprintf(longo + lo, sizeof longo - lo, "%sEigenschaft %d", i ? "\n" : "", i);
    c.max_lines = 20 + 1 + 1;
    p = dm_tipx_compose(longo, lw, 5, &c, 0, out, sizeof out, &len);
    CHECK(p >= 2 && dm_tipx_text_lines(out) == 21 && strstr(out, "Nicht nutzbar") && strstr(out, "Seite 1/"), "1 line: warning + page: %s", out);
    for (int k = 1; k < p; k++) { dm_tipx_compose(longo, lw, 5, &c, k, out, sizeof out, &len); CHECK(strstr(out, "Nicht nutzbar") != NULL && dm_tipx_text_lines(out) <= c.max_lines, "warning on page %d", k + 1); }
    c.max_lines = 21;
    p = dm_tipx_compose(longo, lw, 5, &c, 0, out, sizeof out, &len);
    CHECK(p >= 2 && !strcmp(out, longo), "0 lines: page 1 = original");

    dm_tipx_line lr[] = { { CC "1Danach fehlt St\xc3\xa4rke f\xc3\xbcr: Kampfschild und Ring links", 0 }, { CC "2Abcdefghijklmnopqrstuvwxyz0123456789", 0 } };
    c.max_lines = 28; c.max_chars = 20;
    p = dm_tipx_compose(orig, lr, 2, &c, 0, out, sizeof out, &len);
    CHECK(p == 1 && strstr(out, CC "1Danach fehlt St\xc3\xa4rke\n") && strstr(out, CC "1f\xc3\xbcr: Kampfschild und\n") && strstr(out, CC "1Ring links\n"), "word wrap with colour code: %s", out);
    CHECK(strstr(out, "...") && strstr(out, CC "2Abcdefghijklmnopq...\n"), "truncating with ...: %s", out);

    const char *u = CC "2\xc3\xa4\xc3\xb6\xc3\xbc"; CHECK(dm_tipx_utf8_cut(u, strlen(u), 2) == 4 + 4, "UTF-8 boundary");
    CHECK(dm_tipx_utf8_cut(CC "2", 4, 0) == 4, "the colour code does not count");

    CHECK(dm_tipx_compose(orig, ln, 4, &c, 0, out, 40, &len) == 0, "Puffer zu klein → 0");

    static dm_tipx_line many[DM_TIPX_MAXL]; static char mtxt[DM_TIPX_MAXL][16]; for (int i = 0; i < DM_TIPX_MAXL; i++) { snprintf(mtxt[i], sizeof mtxt[i], "Line %d", i); many[i].s = mtxt[i]; many[i].flags = 0; }
    c.max_lines = 10; c.max_chars = 40;
    p = dm_tipx_compose(orig, many, DM_TIPX_MAXL, &c, 0, out, sizeof out, &len);
    CHECK(p > 1 && p <= 32, "many lines: %d pages", p);
    int seen = 0; for (int k = 0; k < p; k++) { dm_tipx_compose(orig, many, DM_TIPX_MAXL, &c, k, out, sizeof out, &len); CHECK(dm_tipx_text_lines(out) <= c.max_lines, "Seite %d passt (%d Zeilen)", k + 1, dm_tipx_text_lines(out));
        for (int i = 0; i < DM_TIPX_MAXL; i++) { char w[24]; snprintf(w, sizeof w, "Line %d\n", i); if (strstr(out, w)) seen++; } }
    CHECK(seen == DM_TIPX_MAXL, "every line reachable on exactly one page (%d)", seen);

    dm_tipx_line lt[] = { { CC "0Kopf", DM_TIPX_HEAD }, { CC "2aaa bbb ccc ddd eee fff ggg hhh iii jjj", 0 }, { CC "2B +2", 0 } };
    c.max_lines = 3 + 1 + 3; c.max_chars = 8; c.name_lines = 1;
    p = dm_tipx_compose(orig, lt, 3, &c, 0, out, sizeof out, &len);
    CHECK(p >= 2, "long line: %d pages", p);
    for (int k = 0; k < p; k++) { dm_tipx_compose(orig, lt, 3, &c, k, out, sizeof out, &len); CHECK(dm_tipx_text_lines(out) <= c.max_lines, "detail page %d within the budget (%d lines)", k + 1, dm_tipx_text_lines(out)); }
    dm_tipx_line lf[] = { { CC "0Kopf", DM_TIPX_HEAD }, { CC "1W1", DM_TIPX_WARN }, { CC "1W2", DM_TIPX_WARN }, { CC "2A +1", 0 } };
    c.max_lines = 4; c.max_chars = 40;
    CHECK(dm_tipx_compose(orig, lf, 4, &c, 0, out, sizeof out, &len) == 0, "no room for text on the detail pages → original");

    dm_tipx_line lw2[] = { { CC "0Kopf", DM_TIPX_HEAD }, { CC "1Warnung eins mit vielen langen Woertern hier", DM_TIPX_WARN }, { CC "1Warnung zwei mit vielen langen Woertern hier", DM_TIPX_WARN },
                           { CC "3A +1", 0 }, { CC "3B +2", 0 }, { CC "5Ende", 0 } };
    c.max_lines = 3 + 1 + 9; c.max_chars = 12;
    p = dm_tipx_compose(orig, lw2, 6, &c, 0, out, sizeof out, &len);
    CHECK(p > 1, "warnings too tall: the block stays (%d pages)", p);
    { int w2 = 0; for (int k = 0; k < p; k++) { dm_tipx_compose(orig, lw2, 6, &c, k, out, sizeof out, &len); CHECK(dm_tipx_text_lines(out) <= c.max_lines, "page %d within the budget", k + 1);
          CHECK(k == 0 || strstr(out, "Warnung eins") != NULL, "first warning on page %d", k + 1); w2 += strstr(out, "Warnung zwei") != NULL; }
      CHECK(w2 == 1, "the second warning exactly once: %d", w2); }
}


static void t_hands(void) {
    {
        int both[16] = { 0 }, none[16] = { 0 }, right[16] = { 0 };
        both[6] = 11; both[7] = 12; right[6] = 11;
        CHECK(dm_ic_slot(7, 6, 7, both) == 7 && dm_ic_slot(6, 6, 7, both) == 6, "ring: the named slot, both worn");
        CHECK(dm_ic_slot(0, 6, 7, both) == 6 && dm_ic_slot(0, 6, 7, right) == 7 && dm_ic_slot(0, 6, 7, none) == 6, "ring without wish: empty slot, otherwise right");
        CHECK(dm_ic_slot(5, 4, 5, none) == 5 && dm_ic_slot(0, 4, 5, none) == 0, "weapon: off hand on wish, otherwise the caller decides");
        CHECK(dm_ic_slot(7, 1, 1, both) == 1 && dm_ic_slot(4, 6, 7, both) == 6 && dm_ic_slot(0, 2, 2, none) == 2, "a wish that does not fit: same as none");
    }
    CHECK(dm_ic_other_hand(4) == 5 && dm_ic_other_hand(5) == 4 && dm_ic_other_hand(11) == 12 && dm_ic_other_hand(12) == 11 && dm_ic_other_hand(1) == -1, "andere Hand");
    CHECK(dm_ic_displaced(1, 4, 0, 1) == 0, "1: into the empty right hand, the shield stays");
    CHECK(dm_ic_displaced(5, 4, 1, 1) == 1 << 4, "5: one-hander against one-hander, the shield stays");
    CHECK(dm_ic_displaced(7, 4, 1, 1) == (1 << 4 | 1 << 5), "7: two-hander instead of one-hander + shield");
    CHECK(dm_ic_displaced(2, 5, 0, 1) == 1 << 4, "2: shield into the empty left hand, the two-hander goes");
    CHECK(dm_ic_displaced(2, 4, 0, 1) == 1 << 5, "2: two-hander into the empty right hand, the shield goes");
    CHECK(dm_ic_displaced(7, 11, 1, 1) == (1 << 11 | 1 << 12) && dm_ic_displaced(2, 12, 0, 1) == 1 << 11, "weapon set slots 11/12 like 4/5");
    CHECK(dm_ic_displaced(5, 6, 1, 0) == 1 << 6 && dm_ic_displaced(1, 9, 0, 0) == 0, "andere Plätze: 1 leer, 5 tauschen");
    CHECK(dm_ic_displaced(5, 4, 0, 1) == -1 && dm_ic_displaced(1, 4, 1, 0) == -1 && dm_ic_displaced(7, 4, 1, 0) == -1 && dm_ic_displaced(2, 5, 0, 0) == -1, "conflict with what is worn → no numbers");
    CHECK(dm_ic_displaced(2, 2, 0, 1) == -1 && dm_ic_displaced(7, 6, 1, 1) == -1, "2/7 only on the hands");
    CHECK(dm_ic_displaced(0, 4, 0, 0) == -1 && dm_ic_displaced(3, 4, 1, 0) == -1 && dm_ic_displaced(6, 4, 1, 0) == -1, "0 / pick up / stack → no numbers");


    { const uint64_t weapon = 0x1000, shield = 0x2000, ring = 0x3000, other = 0x4000; uint64_t h[2];
      dm_ic_hands_after(6, 1 << 6 | 1 << 4, ring, weapon, shield, h);
      CHECK(h[0] == 0 && h[1] == shield && dm_ic_hands_change(6, 1 << 6 | 1 << 4, ring, weapon, shield), "ring swap, weapon loses its requirement: empty right hand");
      dm_ic_hands_after(6, 1 << 6, ring, weapon, shield, h);
      CHECK(h[0] == weapon && h[1] == shield && !dm_ic_hands_change(6, 1 << 6, ring, weapon, shield), "ring swap alone: hands unchanged");
      CHECK(dm_ic_hands_change(4, 1 << 4, other, weapon, shield) && dm_ic_hands_change(4, 1 << 4, 0, weapon, shield), "weapon swapped or taken off");
      dm_ic_hands_after(5, 1 << 5 | 1 << 4, other, weapon, shield, h);
      CHECK(h[0] == 0 && h[1] == other, "shield in, the other hand's item out"); }

    dm_ic_line l[IC_M_N]; dm_ic_in in; zero(&in); in.tot[IC_S_FIRE] = 30; in.tot[IC_S_FBR] = 20; in.old_[IC_S_FIRE] = 10 + 12; in.old_[IC_S_FBR] = 20; in.new_[IC_S_FIRE] = 5;
    int n = dm_ic_compute(&in, LB, "Cap", TO, l, IC_M_N); const dm_ic_line *x;
    CHECK((x = find(l, n, IC_M_RES_FIRE)) && x->before == 30 && x->after == 13 && (x = find(l, n, IC_M_FBR)) && x->after == 0, "zwei Teile abgezogen");
}


static void t_order(void) {
    dm_ic_sk s[] = { { 10, 3, -1 }, { 11, 5, -1 }, { 12, 1, 1 }, { 13, 5, -1 }, { 14, 0, -1 }, { 15, 9, 0 }, { 16, 3, -1 } };
    dm_ic_skill_order(s, 7);
    static const int want[] = { 15, 12, 11, 13, 10, 16, 14 };
    int ok = 1; for (int i = 0; i < 7; i++) ok &= s[i].id == want[i];
    CHECK(ok, "order %d %d %d %d %d %d %d", s[0].id, s[1].id, s[2].id, s[3].id, s[4].id, s[5].id, s[6].id);
    dm_ic_sk one[] = { { 7, 2, -1 } }; dm_ic_skill_order(one, 1); CHECK(one[0].id == 7, "ein Eintrag");
    dm_ic_skill_order(s, 0); CHECK(1, "leer");
}


static void t_capacity(void) {



    static int kind[128]; static char keep[128]; int at;
    for (int nmust = 1; nmust <= 5; nmust += 2) for (int nfix = 0; nfix <= 44; nfix += 4) for (int nwhy = 0; nwhy <= 12; nwhy += 4) for (int nval = 0; nval <= 50; nval += 7) for (int lost = 0; lost <= 2; lost += 2) {
        int n = 0;
        for (int i = 0; i < nmust; i++) kind[n++] = IC_K_MUST;
        for (int i = 0; i < nfix; i++) kind[n++] = IC_K_FIXED;
        for (int i = 0; i < nval; i++) kind[n++] = IC_K_VAL;
        for (int i = 0; i < nwhy; i++) kind[n++] = IC_K_WHY;
        kind[n++] = IC_K_END;
        int more = dm_ic_pack(kind, n, DM_TIPX_MAXL, lost, keep, &at), kept = 0, km = 0, kf = 0, kw = 0, kv = 0, end_kept = 0;
        for (int i = 0; i < n; i++) if (keep[i]) { kept++; km += kind[i] == IC_K_MUST; kf += kind[i] == IC_K_FIXED; kw += kind[i] == IC_K_WHY; kv += kind[i] == IC_K_VAL; end_kept |= kind[i] == IC_K_END; }
        char tag[48]; snprintf(tag, sizeof tag, "%d/%d/%d/%d/%d", nmust, nfix, nwhy, nval, lost);
        CHECK(kept + (more > 0) <= DM_TIPX_MAXL, "Budget %s: %d + %d", tag, kept, more > 0);
        CHECK(kept + more == n + lost, "nothing lost %s: %d + %d != %d", tag, kept, more, n + lost);
        CHECK(km == nmust && end_kept, "required lines + closing line stay %s", tag);
        CHECK(kf == nfix || (kw == 0 && kv == 0), "fixed lines before reasons and values %s", tag);
        CHECK(kw == nwhy || kv == 0, "reasons before values %s", tag);
        CHECK(!more == (n + lost <= DM_TIPX_MAXL && !lost), "overflow exactly when it does not fit %s", tag);
        if (more) CHECK(at >= 0 && at <= n && (at == n || !keep[at] || kind[at] == IC_K_END), "\"+n\" an der ersten fehlenden Stelle %s", tag);
        for (int i = 0, seen_drop = 0; i < n; i++) if (kind[i] == IC_K_VAL) { if (!keep[i]) seen_drop = 1; else CHECK(!seen_drop, "Werte vorne behalten"); } }
    { int k2[] = { IC_K_MUST, IC_K_VAL, IC_K_VAL, IC_K_WHY, IC_K_END }; char kp[5];
      CHECK(dm_ic_pack(k2, 5, DM_TIPX_MAXL, 0, kp, &at) == 0 && kp[0] && kp[1] && kp[2] && kp[3] && kp[4] && at == 5, "alles passt"); }



    static char txt[DM_TIPX_MAXL][DM_TIPX_LINEB]; static dm_tipx_line ln[DM_TIPX_MAXL + 1]; static char out[65536]; size_t len;
    for (int i = 0; i < DM_TIPX_MAXL; i++) { int k = snprintf(txt[i], sizeof txt[i], CC "3L%02d", i); while (k < DM_TIPX_LINEB - 8) k += snprintf(txt[i] + k, sizeof txt[i] - k, " wort%d", k % 10);
        ln[i].s = txt[i]; ln[i].flags = i == 0 ? DM_TIPX_HEAD : i == 1 ? DM_TIPX_WARN : 0; }
    static char orig[2048]; int ol = 0; for (int i = 0; i < 20; i++) ol += snprintf(orig + ol, sizeof orig - ol, "%sEigenschaft %d", i ? "\n" : "", i);
    for (int f = -1; f <= 3; f++) {
        dm_tipx_cfg c = { .max_lines = f < 0 ? 5 : 21 + f, .max_chars = 24, .name_lines = 1, .hint = CC "5... Seite %d/%d", .props = CC "5Eigenschaften: Seite 1" };
        int p = dm_tipx_compose(orig, ln, DM_TIPX_MAXL, &c, 0, out, sizeof out, &len);
        if (f < 0) { CHECK(p == 0, "detail pages without room → original: %d", p); continue; }
        CHECK(p > 1, "boundary: %d pages (%d free)", p, f);
        int seen[DM_TIPX_MAXL] = { 0 };
        for (int k = 0; k < p; k++) { int q = dm_tipx_compose(orig, ln, DM_TIPX_MAXL, &c, k, out, sizeof out, &len); CHECK(q == p, "the page count stays stable");
            CHECK(dm_tipx_text_lines(out) <= c.max_lines, "page %d within the budget: %d > %d (%d free)", k + 1, dm_tipx_text_lines(out), c.max_lines, f);
            if (k > 0 || f >= 1) CHECK(strstr(out, "L01") != NULL, "warning on page %d (%d free)", k + 1, f);
            if (k == 0 && f == 0) CHECK(!strcmp(out, orig), "0 free: page 1 = original");
            for (int i = 2; i < DM_TIPX_MAXL; i++) { char w[8]; snprintf(w, sizeof w, "L%02d ", i); if (strstr(out, w)) seen[i]++; } }
        int all = 1; for (int i = 2; i < DM_TIPX_MAXL; i++) all &= seen[i] == 1; CHECK(all, "every line on exactly one page (%d free)", f); }

    dm_tipx_cfg c = { .max_lines = 28, .max_chars = 40, .name_lines = 1, .hint = CC "5%d/%d", .props = NULL };
    static char big[DM_TIPX_LINEB + 8]; memset(big, 'x', sizeof big - 1); big[sizeof big - 1] = 0; ln[DM_TIPX_MAXL].s = big; ln[DM_TIPX_MAXL].flags = 0;
    CHECK(dm_tipx_compose(orig, ln, DM_TIPX_MAXL + 1, &c, 0, out, sizeof out, &len) == 0, "more than DM_TIPX_MAXL lines → 0");
    dm_tipx_line one[] = { { big, 0 } }; CHECK(dm_tipx_compose(orig, one, 1, &c, 0, out, sizeof out, &len) == 0, "line too long → 0");
}


static void t_combat(void) {
    dm_ic_cb c; int v[IC_C_N + 1];
#define Z() memset(&c, 0, sizeof c)
    Z(); c.ac = 100; c.dex = 41; c.acpct = 25; CHECK(dm_ic_cb_eval(&c, v) && v[IC_C_DEF] == 137, "defence (100 + 41/4) * 1.25 truncated: %d", v[IC_C_DEF]);
    Z(); c.ac = -20; c.acpct = 50; dm_ic_cb_eval(&c, v); CHECK(v[IC_C_DEF] == -10, "base <= 0: the percentage part with a flipped sign: %d", v[IC_C_DEF]);
    Z(); c.ac = 10; c.dex = -7; dm_ic_cb_eval(&c, v); CHECK(v[IC_C_DEF] == 9, "Geschick/4 gegen null gerundet: %d", v[IC_C_DEF]);
    Z(); c.ac = 200; c.acovr = -50; dm_ic_cb_eval(&c, v); CHECK(v[IC_C_DEF] == 100, "defence percent override: %d", v[IC_C_DEF]);
    Z(); c.ac = 30001; c.acpct = 4000; c.acovr = 99; CHECK(!dm_ic_cb_eval(&c, v), "override above 0x100000 defence: different division order in the game → no value");
    Z(); c.toblock = 30; c.bf = 20; c.dex = 75; c.lvl = 10; c.expansion = 1; dm_ic_cb_eval(&c, v); CHECK(v[IC_C_BLK] == 0, "no shield, no block: %d", v[IC_C_BLK]);
    c.can_block = 1; dm_ic_cb_eval(&c, v); CHECK(v[IC_C_BLK] == 75, "block above 74 → 75: %d", v[IC_C_BLK]);
    c.lvl = 30; dm_ic_cb_eval(&c, v); CHECK(v[IC_C_BLK] == 50, "block (75-15)*50/(2*30): %d", v[IC_C_BLK]);
    c.dex = 10; c.lvl = 20; dm_ic_cb_eval(&c, v); CHECK(v[IC_C_BLK] == -6, "block rounds towards zero when dexterity < 15: %d", v[IC_C_BLK]);
    c.lvl = 0; c.dex = 17; dm_ic_cb_eval(&c, v); CHECK(v[IC_C_BLK] == 50, "Stufe 0 zählt als 1: %d", v[IC_C_BLK]);
    c.expansion = 0; dm_ic_cb_eval(&c, v); CHECK(v[IC_C_BLK] == 50, "classic: the block value directly: %d", v[IC_C_BLK]);
    Z(); c.tohit = 100; c.dex = 50; c.thf = 10; c.ar_mast = 20; c.tohitpct = 30; dm_ic_cb_eval(&c, v); CHECK(v[IC_C_AR] == 487, "attack rating 325 * 1.5 truncated: %d", v[IC_C_AR]);
    c.ar_zero = 1; dm_ic_cb_eval(&c, v); CHECK(v[IC_C_AR] == 0, "throwing potion: attack rating 0: %d", v[IC_C_AR]);
    Z(); c.armed = 1; c.mn[0] = 2; c.mx[0] = 8; c.bmn = 1; c.bmx = 2; c.srcdam = 128; c.dpct = 20; c.str = 100; c.strb = 75; c.dex = 60; c.dexb = 1; c.edmin = 10; c.edmax = 30; c.flat = 2;
    dm_ic_cb_eval(&c, v); CHECK(v[IC_C_DMG] == 9 && v[IC_C_N] == 30, "damage weapon + strength/dexterity bonus (1 = 100) + ED + flat: %d-%d", v[IC_C_DMG], v[IC_C_N]);
    c.el[0] = 5; c.el[1] = 10; c.mast[0] = 50; c.el[6] = 8; c.el[7] = 4; dm_ic_cb_eval(&c, v); CHECK(v[IC_C_DMG] == 20 && v[IC_C_N] == 49, "fire with Mastery, cold min > max: %d-%d", v[IC_C_DMG], v[IC_C_N]);
    c.el[9] = 256; c.el[10] = 512; c.plen = 50; c.pcnt = 2; dm_ic_cb_eval(&c, v); CHECK(v[IC_C_DMG] == 45 && v[IC_C_N] == 99, "Gift Länge/Anzahl >> 8: %d-%d", v[IC_C_DMG], v[IC_C_N]);
    c.plen_ovr = 100; dm_ic_cb_eval(&c, v); CHECK(v[IC_C_DMG] == 120 && v[IC_C_N] == 249, "poison length overridden: %d-%d", v[IC_C_DMG], v[IC_C_N]);
    c.mode2 = 1; c.mn[1] = 0; c.mx[1] = 0; memset(c.el, 0, sizeof c.el); c.plen_ovr = 0; dm_ic_cb_eval(&c, v);
    CHECK(v[IC_C_DMG] == 4 && v[IC_C_N] == 7, "two-handed: 23/24 instead of 21/22 (lower bounds 1/2): %d-%d", v[IC_C_DMG], v[IC_C_N]);
    Z(); c.str = 50; c.srcdam = 128; dm_ic_cb_eval(&c, v); CHECK(v[IC_C_DMG] == 1 && v[IC_C_N] == 3, "unbewaffnet: 1/2 + volle Stärke: %d-%d", v[IC_C_DMG], v[IC_C_N]);
    c.srcdam = 0; c.str = 0; dm_ic_cb_eval(&c, v); CHECK(v[IC_C_DMG] == 0 && v[IC_C_N] == 1, "SrcDam 0: no base damage, the character sheet shows max = min + 1: %d-%d", v[IC_C_DMG], v[IC_C_N]);
    Z(); c.armed = 1; c.mn[0] = 2; c.mx[0] = 8; c.srcdam = 128; c.xpct = -30; c.xflat = -1; c.sadd_mn = 2; c.sadd_mx = 3; dm_ic_cb_eval(&c, v);
    CHECK(v[IC_C_DMG] == 3 && v[IC_C_N] == 7, "Fertigkeitsbeschreibung: Zusatzprozent/-wert + eigener Schaden: %d-%d", v[IC_C_DMG], v[IC_C_N]);
    Z(); c.tohit = 100; c.dex = 50; c.thf = 10; c.ar_prog = 50; dm_ic_cb_eval(&c, v); CHECK(v[IC_C_AR] == 487, "attack rating with a progressive bonus (flag 3): %d", v[IC_C_AR]);
    Z(); c.armed = 1; c.mn[0] = 10; c.mx[0] = 20; c.srcdam = 64; dm_ic_cb_eval(&c, v); CHECK(v[IC_C_DMG] == 5 && v[IC_C_N] == 10, "SrcDam 64/128: %d-%d", v[IC_C_DMG], v[IC_C_N]);
    c.srcdam = 128; c.dpct = -200; dm_ic_cb_eval(&c, v); CHECK(v[IC_C_DMG] == 1 && v[IC_C_N] == 2, "percent never below -90: %d-%d", v[IC_C_DMG], v[IC_C_N]);
    c.dpct = 0; c.mn[0] = 10; c.mx[0] = 3; dm_ic_cb_eval(&c, v); CHECK(v[IC_C_DMG] == 10 && v[IC_C_N] == 11, "max > min erzwungen: %d-%d", v[IC_C_DMG], v[IC_C_N]);
    c.mn[0] = 0x8000; CHECK(!dm_ic_cb_eval(&c, v), "input outside |x| < 0x8000 → no value");
    c.mn[0] = 30000; c.mx[0] = 30000; c.str = 30000; c.strb = 30000; CHECK(!dm_ic_cb_eval(&c, v), "intermediate above 32 bits → no value");
    Z(); c.srcdam = 300; CHECK(!dm_ic_cb_eval(&c, v), "SrcDam outside 0..255 → no value");

    { dm_ic_cb n, a; memset(&n, 0, sizeof n); n.can_block = 1; n.expansion = 1; n.dex = 75; n.lvl = 10; n.bf = 20; a = n;
      n.toblock = 4; a.toblock = 10; dm_ic_cb_eval(&a, v); CHECK(v[IC_C_BLK] == 75 && dm_ic_cb_new_cap(&n, &a) == 1u << IC_C_BLK, "72 → 90 (75): the cap is new → own combination");
      a.toblock = 5; CHECK(dm_ic_cb_new_cap(&n, &a) == 1u << IC_C_BLK, "72 → exactly 75: the game already takes the branch above 74");
      a.toblock = 4; CHECK(dm_ic_cb_new_cap(&n, &a) == 0, "72 → 72: no new branch");
      n.toblock = 12; a.toblock = 15; CHECK(dm_ic_cb_new_cap(&n, &a) == 0, "96 (75) → 105 (75): already capped now (R0 checked 75), unchanged");
      a.toblock = 2; CHECK(dm_ic_cb_new_cap(&n, &a) == 0, "96 (75) → 66: normal formula afterwards, no new branch");
      n.toblock = 4; a.toblock = 30; a.can_block = 0; CHECK(dm_ic_cb_new_cap(&n, &a) == 0, "no shield afterwards: no block, no branch");
      n.can_block = 0; a.can_block = 1; CHECK(dm_ic_cb_new_cap(&n, &a) == 1u << IC_C_BLK, "shield into the empty hand up to the cap: new branch");
      n.can_block = 1; n.expansion = a.expansion = 0; n.toblock = 50; a.toblock = 60; CHECK(dm_ic_cb_new_cap(&n, &a) == 1u << IC_C_BLK, "classic 70 → 80: new branch");
      a.dex = 0x8000; CHECK(dm_ic_cb_new_cap(&n, &a) == 1u << IC_C_BLK, "input out of range: treated like a new branch");
      n.expansion = a.expansion = 1; n.dex = a.dex = 35; n.lvl = a.lvl = 10; n.bf = a.bf = 0;
      n.toblock = 74; a.toblock = 75; dm_ic_cb_eval(&n, v); CHECK(v[IC_C_BLK] == 74 && dm_ic_cb_new_cap(&n, &a) == 1u << IC_C_BLK, "74 → 75: not capped now, capped afterwards");
      n.toblock = 73; a.toblock = 74; CHECK(dm_ic_cb_new_cap(&n, &a) == 0, "73 → 74: under the boundary");
      n.toblock = 74; a.toblock = 74; CHECK(dm_ic_cb_new_cap(&n, &a) == 0, "74 → 74: under the boundary");
      n.toblock = 75; a.toblock = 74; CHECK(dm_ic_cb_new_cap(&n, &a) == 0, "75 → 74: the normal formula afterwards"); }
#undef Z
    char o[200], lb[64];
    dm_ic_cb_line(IC_C_DEF, 210, 0, 254, 0, "Defense", "MAX", "mixed", TO, 0, o, sizeof o); CHECK(!strcmp(o, CC "0Defense " CC "3+44 (210 auf 254)"), "defence line: %s", o);
    dm_ic_cb_line(IC_C_BLK, 70, 0, 75, 0, "Block", "MAX", "mixed", TO, 0, o, sizeof o); CHECK(!strcmp(o, CC "0Block " CC "3+5 (70 % auf 75 %) " CC "4MAX"), "block capped at 75: gold + MAX: %s", o);
    dm_ic_cb_line(IC_C_BLK, 70, 0, 75, 0, "Block", "MAX", "mixed", TO, 1, o, sizeof o);
    CHECK(!strstr(o, "MAX") && !strstr(o, CC "4") && strstr(o, CC "5+5"), "hypothetical block stays grey");
    dm_ic_cb_line(IC_C_BLK, 75, 0, 70, 0, "Block", "MAX", "mixed", TO, 0, o, sizeof o);
    CHECK(!strstr(o, "MAX") && !strstr(o, CC "4"), "leaving block cap has no label");
    dm_ic_cb_line(IC_C_BLK, 42, 0, 0, 0, "Block", "MAX", "mixed", TO, 0, o, sizeof o); CHECK(!strcmp(o, CC "0Block " CC "1-" "42 (42 % auf 0 %)"), "block down: red: %s", o);
    dm_ic_cb_line(IC_C_DMG, 12, 30, 18, 41, "Dmg (Atk)", "MAX", "mixed", TO, 0, o, sizeof o); CHECK(!strcmp(o, CC "0Dmg (Atk) " CC "312-30 auf 18-41"), "damage better: blue: %s", o);
    dm_ic_cb_line(IC_C_DMG, 12, 30, 10, 41, "D", "MAX", "mixed", TO, 0, o, sizeof o); CHECK(strstr(o, CC "512-30") && strstr(o, "mixed"), "mixed damage is neutral and labelled: %s", o);
    dm_ic_cb_line(IC_C_DMG, 12, 30, 12, 29, "D", "MAX", "mixed", TO, 0, o, sizeof o); CHECK(strstr(o, CC "112-30") != NULL, "Schaden schlechter: rot: %s", o);
    dm_ic_cb_line(IC_C_AR, 480, 0, 530, 0, "AR", "MAX", "mixed", TO, 1, o, sizeof o); CHECK(!strcmp(o, CC "5AR " CC "5+50 (480 auf 530)"), "hypothetical grey: %s", o);
    CHECK(dm_ic_label("%s\nAttack Rating", 1, lb, sizeof lb) && !strcmp(lb, "Attack Rating"), "label without a %%s: %s", lb);
    CHECK(dm_ic_label("\xc3\x8dndice de ataque\nde %s", 1, lb, sizeof lb) && !strcmp(lb, "\xc3\x8dndice de ataque"), "label, Spanish: first line: %s", lb);
    CHECK(dm_ic_label("Chance to Block: %d%%", 2, lb, sizeof lb) && !strcmp(lb, "Chance to Block"), "label up to the colon: %s", lb);
    CHECK(dm_ic_label("\xe6\xa0\xbc\xe6\x8c\xa1\xe5\x87\xa0\xe7\x8e\x87\xef\xbc\x9a%d%%", 2, lb, sizeof lb) && !strcmp(lb, "\xe6\xa0\xbc\xe6\x8c\xa1\xe5\x87\xa0\xe7\x8e\x87"), "voller Doppelpunkt: %s", lb);
    CHECK(dm_ic_label("Defense", 0, lb, sizeof lb) && !strcmp(lb, "Defense"), "the whole string");
    CHECK(!dm_ic_label("Def %d", 0, lb, sizeof lb), "%% in the label → own key");
    CHECK(!dm_ic_label("%s\n%d", 1, lb, sizeof lb), "both lines with %% → own key");
    CHECK(!dm_ic_label("abcdefghijklmnopqrstuvwxyz", 0, lb, sizeof lb), "over 24 characters → own key");
    CHECK(!dm_ic_label("", 0, lb, sizeof lb) && !dm_ic_label(NULL, 0, lb, sizeof lb), "empty or missing → own key");
    CHECK(!dm_ic_label("Block", 2, lb, sizeof lb), "no colon → own key");
}


static void t_join(void) {
    static const struct { const char *fmt, *want; } L[] = {
        { "%s auf %s", "53 % auf 36 %" }, { "%s to %s", "53 % to 36 %" }, { "de %s a %s", "de 53 % a 36 %" }, { "\xd1\x81 %s \xd0\xbd\xd0\xb0 %s", "\xd1\x81 53 % \xd0\xbd\xd0\xb0 36 %" },
        { "%s \xe5\x8f\x98\xe4\xb8\xba %s", "53 % \xe5\x8f\x98\xe4\xb8\xba 36 %" }, { "%s\xec\x97\x90\xec\x84\x9c %s\xeb\xa1\x9c", "53 %\xec\x97\x90\xec\x84\x9c 36 %\xeb\xa1\x9c" } };
    char o[96];
    for (unsigned i = 0; i < sizeof L / sizeof *L; i++) { int w = dm_ic_join(o, sizeof o, L[i].fmt, "53 %", "36 %"); CHECK(!strcmp(o, L[i].want) && w == (int)strlen(o), "Wendung %u: %s", i, o); }
    static const char *const bad[] = { "", NULL, "%s", "%s %s %s", "%d auf %d", "%s auf %s %%", "%s%" };
    for (unsigned i = 0; i < sizeof bad / sizeof *bad; i++) { dm_ic_join(o, sizeof o, bad[i], "1", "2"); CHECK(!strcmp(o, "1 to 2"), "kaputte Wendung %u -> englisch: %s", i, o); }
    char s[8]; int w = dm_ic_join(s, sizeof s, "%s auf %s", "12345", "678"); CHECK(w == 7 && !strcmp(s, "12345 a"), "truncated, NUL byte: %s", s);
    CHECK(dm_ic_join(s, 0, "%s auf %s", "1", "2") == 0, "n = 0");

    dm_ic_in in; dm_ic_line l[IC_M_N]; memset(&in, 0, sizeof in); in.lpv = 7; in.mpm = 5; in.tot[IC_S_FIRE] = 53; in.old_[IC_S_FIRE] = 17;
    int n = dm_ic_compute(&in, LB, "Cap", "%s to %s", l, IC_M_N);
    CHECK(n == 1 && !strcmp(l[0].text, CC "0Fire res " CC "1-" "17 (53 % to 36 %)"), "English line: %s", n ? l[0].text : "");
    char x[200]; dm_ic_cb_line(IC_C_DMG, 12, 30, 18, 41, "Schaden", "MAX", "mixed", "%s 变为 %s", 0, x, sizeof x);
    CHECK(!strcmp(x, CC "0Schaden " CC "312-30 \xe5\x8f\x98\xe4\xb8\xba 18-41"), "Schaden chinesisch: %s", x);
    for (unsigned i = 0; i < strlen(x); i++) CHECK(!(x[i] == '\xe2' && x[i + 1] == '\x80' && x[i + 2] == '\xba'), "no arrow");
}


static void t_hints(void) {
    char d[16];
    { int k[] = { IC_K_MUST, IC_K_VAL, IC_K_WHY, IC_K_WHY, IC_K_WHY, IC_K_END }; int n = dm_ic_hints(k, 6, d);
      CHECK(n == 3 && !d[0] && !d[1] && !d[2] && d[3] && d[4] && d[5], "three reasons: only the first stays, closing line gone (%d)", n); }
    { int k[] = { IC_K_MUST, IC_K_VAL, IC_K_END }; CHECK(dm_ic_hints(k, 3, d) == 0 && !d[0] && !d[1] && !d[2], "no reason: the closing line stays"); }
    { int k[] = { IC_K_MUST, IC_K_FIXED, IC_K_WHY, IC_K_END }; CHECK(dm_ic_hints(k, 4, d) == 1 && !d[1] && !d[2] && d[3], "one reason: it stays, the closing line goes"); }
    { int k[] = { IC_K_MUST, IC_K_FIXED, IC_K_FIXED, IC_K_VAL }; CHECK(dm_ic_hints(k, 4, d) == 0, "note and requirement lines are not hints"); }
    { int k[] = { IC_K_WHY, IC_K_MUST, IC_K_WHY }; CHECK(dm_ic_hints(k, 3, d) == 1 && !d[0] && !d[1] && d[2], "erster gesammelter Grund gewinnt"); }
    CHECK(dm_ic_hints(NULL, 0, d) == 0, "leer");
}


static void t_unproven(void) {
    static const char *const labels[IC_C_N] = { "Verteidigung", "Blockchance", "Angriffswert", "Schaden" };
    char o[DM_TIPX_LINEB];
    int w = dm_ic_unproven_text(o, sizeof o, "%s: nicht berechnet", 1u << IC_C_DEF | 1u << IC_C_AR | 1u << IC_C_DMG, 0, labels);
    CHECK(w == (int)strlen(o) && !strcmp(o, "Verteidigung, Angriffswert, Schaden: nicht berechnet"), "all unverified values named: %s", o);
    dm_ic_unproven_text(o, sizeof o, "%s: nicht berechnet", 1u << IC_C_DEF | 1u << IC_C_AR | 1u << IC_C_DMG, 1, labels);
    CHECK(!strcmp(o, "Verteidigung: nicht berechnet"), "a simulated attack covers attack rating and damage: %s", o);
    CHECK(dm_ic_unproven_text(o, sizeof o, "%s: nicht berechnet", 1u << IC_C_AR | 1u << IC_C_DMG, 1, labels) == 0 && !o[0], "only attack rating values, attack simulated: no line");
    CHECK(dm_ic_unproven_text(o, sizeof o, "%s: nicht berechnet", 0, 0, labels) == 0, "nothing unverified: no line");
    dm_ic_unproven_text(o, sizeof o, "%s：没算", 1u << IC_C_BLK, 0, labels); CHECK(!strcmp(o, "Blockchance：没算"), "the phrase follows the language: %s", o);
    static const char *const bad[] = { NULL, "", "%s %s", "%d", "%s %%" };
    for (unsigned i = 0; i < sizeof bad / sizeof *bad; i++) { dm_ic_unproven_text(o, sizeof o, bad[i], 1u << IC_C_DEF, 0, labels); CHECK(!strcmp(o, "Verteidigung: not computed"), "kaputte Wendung %u -> englisch: %s", i, o); }
    char s[8]; CHECK(dm_ic_unproven_text(s, sizeof s, "%s: nicht berechnet", 1u << IC_C_DEF, 0, labels) == 7 && !strcmp(s, "Verteid"), "truncated: %s", s);

    char lines[6][24] = { "Testhelm", "Leben +35", "Kampfsimulation", "Testzauber: 1 > 2" };
    int n = dm_ic_insert(lines, sizeof lines[0], 4, 6, 2); snprintf(lines[2], sizeof lines[2], "Schaden: ungeprueft");
    CHECK(n == 5 && !strcmp(lines[0], "Testhelm") && !strcmp(lines[1], "Leben +35") && !strcmp(lines[2], "Schaden: ungeprueft") && !strcmp(lines[3], "Kampfsimulation") && !strcmp(lines[4], "Testzauber: 1 > 2"),
          "before the combat-simulation block: %s / %s / %s", lines[2], lines[3], lines[4]);
    n = dm_ic_insert(lines, sizeof lines[0], n, 6, n); CHECK(n == 6 && !strcmp(lines[4], "Testzauber: 1 > 2"), "without the combat-simulation block: at the end");
    CHECK(dm_ic_insert(lines, sizeof lines[0], 6, 6, 2) == 6 && !strcmp(lines[2], "Schaden: ungeprueft"), "full: nothing moves");
    CHECK(dm_ic_insert(lines, sizeof lines[0], 3, 6, 4) == 3 && dm_ic_insert(lines, sizeof lines[0], 3, 6, -1) == 3, "position out of range: nothing moves");
}


static void t_shrine(void) {
    dm_ic_cb c; int g[IC_C_N + 1], b[IC_C_N + 1], a[IC_C_N + 1]; memset(&c, 0, sizeof c);
    c.ac = 30; c.dex = 16; c.acpct = 100;
    c.tohit = 300; c.thf = 15; c.ar_mast = 20;
    c.armed = 1; c.mn[0] = 2; c.mx[0] = 8; c.srcdam = 128; c.dpct = 200;
    dm_ic_cb_eval(&c, g);
    unsigned ok = dm_ic_cb_without(&c, g, 100, 276, 200, b, a);
    CHECK(ok == 0xf, "alle Werte mit R0: %x", ok);
    CHECK(a[IC_C_DEF] == 68 && b[IC_C_DEF] == 34, "defence 34 > 68: %d > %d", b[IC_C_DEF], a[IC_C_DEF]);
    CHECK(a[IC_C_AR] == 432 && b[IC_C_AR] == (84 + 84 * 20 / 100), "attack rating without +276: %d > %d", b[IC_C_AR], a[IC_C_AR]);
    CHECK(b[IC_C_DMG] == 2 && b[IC_C_N] == 8 && a[IC_C_DMG] == 6 && a[IC_C_N] == 24, "Schaden 2-8 > 6-24: %d-%d > %d-%d", b[IC_C_DMG], b[IC_C_N], a[IC_C_DMG], a[IC_C_N]);
    g[IC_C_DEF]++; ok = dm_ic_cb_without(&c, g, 100, 0, 0, b, a); CHECK(!(ok & 1u << IC_C_DEF) && (ok & 1u << IC_C_AR), "R0 misses only with defence: %x", ok);
    g[IC_C_DEF]--; g[IC_C_N]++; ok = dm_ic_cb_without(&c, g, 0, 0, 0, b, a); CHECK(!(ok & 1u << IC_C_DMG), "damage max differs: no damage line: %x", ok);

    g[IC_C_N]--; dm_ic_veto v; memset(&v, 0, sizeof v); dm_ic_veto_add(&v, 1u << IC_C_DMG, "dmCmpUnproven", 1);
    ok = dm_ic_cb_without(&c, g, 100, 276, 200, b, a) & ~v.bad; CHECK(ok == (0xf & ~(1u << IC_C_DMG)), "display path: damage line shown: %x", ok);
    c.ac = 0x9000; CHECK(dm_ic_cb_without(&c, g, 100, 0, 0, b, a) == 0, "outside the written-off branches: nothing");
}



static void t_state_off(void) {
    dm_ic_cb c; memset(&c, 0, sizeof c); int off[512] = { 0 }, v[IC_C_N + 1], w[IC_C_N + 1];
    c.ac = 30; c.dex = 16; c.acpct = 100; c.tohit = 300; c.thf = 15; c.tohitpct = 50; c.armed = 1; c.mn[0] = 2; c.mx[0] = 8; c.srcdam = 128; c.dpct = 200;
    dm_ic_cb x = c; off[171] = 100; off[119] = 50; off[25] = 100; dm_ic_cb_minus(&x, off);
    CHECK(dm_ic_cb_eval(&c, v) && dm_ic_cb_eval(&x, w), "auswertbar");
    CHECK(w[IC_C_DEF] == 34 && v[IC_C_DEF] == 68, "defence without the state percentage: %d (before %d)", w[IC_C_DEF], v[IC_C_DEF]);
    CHECK(w[IC_C_AR] < v[IC_C_AR] && w[IC_C_DMG] == 4 && w[IC_C_N] == 16, "attack rating and damage without the state: %d / %d-%d", w[IC_C_AR], w[IC_C_DMG], w[IC_C_N]);

    static const int read[] = { 0, 2, 31, 171, 16, 182, 20, 19, 119, 21, 22, 23, 24, 185, 186, 25, 17, 18, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58,
                                329, 330, 331, 332, 101, 59, 326 };
    for (int st = 0; st < 512; st++) {
        int r = 0; for (unsigned k = 0; k < sizeof read / sizeof *read; k++) r |= read[k] == st;
        memset(off, 0, sizeof off); off[st] = 1; dm_ic_cb y; memset(&y, 0, sizeof y); dm_ic_cb_minus(&y, off);
        int sum = 0; const int *p = (const int *)&y; for (size_t k = 0; k < sizeof y / sizeof *p; k++) sum += p[k];
        CHECK(sum == (r ? -1 : 0), "Wert %d: Summe %d", st, sum);
    }
}

static void comparison_colours(void) {
    const char *source = CC "3Better word word word\n" CC "5Same\n" CC "6Skill\n" CC "1Worse\n";
    const char *original = CC "3Magic\n" CC "2Set";
    char text[512], out[1024]; snprintf(text, sizeof text, "%s%s", source, original);
    size_t n = dm_tipx_colour_text(text, strlen(source), out, sizeof out);
    CHECK(n && !strcmp(out + n - strlen(original), original), "original item colours preserved");
    const uint32_t palette[4][3] = {{79,194,56},{252,70,70},{148,148,148},{102,102,255}};
    const char codes[] = "3156";
    for (int i = 0; i < 4; i++) {
        char tag[13] = DM_TIPX_COLOUR_TAG CC "3"; tag[11] = codes[i];
        uint64_t regs[32] = {0};
        CHECK(dm_tipx_colour_rgb(tag + 11, 8, palette, regs), "tag recognized");
        CHECK(regs[8] == palette[i][0] && regs[10] == palette[i][1] && regs[9] == palette[i][2], "exact comparison RGB");
        CHECK(!dm_tipx_colour_rgb(tag + 11, 0, palette, regs), "no read before input");
        tag[3] = '0';
        CHECK(!dm_tipx_colour_rgb(tag + 11, 8, palette, regs), "ordinary game controls unchanged");
    }
    CHECK(!dm_tipx_colour_text(text, strlen(source), out, 12), "bounded colour expansion");
    const char *skill = CC "6Skill 5 > 6\n";
    const char *game = CC "6Original\n" CC "3Magic\n" CC "2Set";
    for (int ready = 0; ready <= 1; ready++) {
        snprintf(text, sizeof text, "%s%s", skill, game);
        const char *rendered = dm_tipx_render_text(text, strlen(skill), ready, out, ready ? 12 : sizeof out);
        CHECK(rendered == text && !strncmp(rendered, CC "3Skill 5 > 6", 15), "missing hook or expansion failure uses blue");
        CHECK(!strcmp(rendered + strlen(skill), game), "fallback preserves all original controls");
        CHECK(!strstr(rendered, DM_TIPX_COLOUR_TAG), "fallback contains no tags");
    }
    snprintf(text, sizeof text, "%s%s", skill, game);
    CHECK(dm_tipx_render_text(text, strlen(skill), 1, out, sizeof out) == out &&
          strstr(out, DM_TIPX_COLOUR_TAG CC "6Skill"), "active hook keeps skill colour");
    CHECK(!strcmp(text + strlen(skill), game), "active hook preserves source");
    dm_tipx_render_text(text, 0, 0, out, sizeof out);
    CHECK(!strncmp(text, skill, strlen(skill)), "empty own segment is untouched");
    dm_tipx_line line = {CC "3one two three four five six seven eight", 0};
    dm_tipx_cfg cfg = {.max_lines=6, .max_chars=12, .name_lines=1};
    size_t len; int pages = dm_tipx_compose(original, &line, 1, &cfg, 0, text, sizeof text, &len);
    CHECK(pages > 1, "comparison wraps onto pages");
    for (int page = 0; page < pages; page++) {
        dm_tipx_compose(original, &line, 1, &cfg, page, text, sizeof text, &len);
        const char *suffix = page ? CC "2Set" : original;
        n = dm_tipx_colour_text(text, len - strlen(suffix), out, sizeof out);
        CHECK(n && (len == strlen(suffix) || strstr(out, DM_TIPX_COLOUR_TAG CC "3")), "wrapped comparison keeps its colour");
        CHECK(!strcmp(out + n - strlen(suffix), suffix), "page leaves original colours intact");
    }
}

static void bracket_width(void) {
    dm_tipx_line lines[]={{"Normaler Angriff (123 auf 456)",0}};
    dm_tipx_cfg cfg={.max_lines=20,.max_chars=24,.name_lines=1};
    char out[1024];size_t length;
    CHECK(dm_tipx_compose("Testring",lines,1,&cfg,0,out,sizeof out,&length)==1,"bracketed value fits one page");
    CHECK(strstr(out,"(123 auf 456)")!=NULL,"numeric group stays on one line: %s",out);
}

static void label_value_wrap(void) {
    dm_tipx_line lines[]={{CC "3Fertigkeit mit langem Namen: 19999.9 \xe2\x80\xba 29999.9 DPS",0}};
    dm_tipx_cfg cfg={.max_lines=20,.max_chars=42,.name_lines=1};
    char out[1024];size_t length;
    CHECK(dm_tipx_compose("Testring",lines,1,&cfg,0,out,sizeof out,&length)==1,"long label fits one page");

    CHECK(strstr(out,CC "319999.9 \xe2\x80\xba 29999.9 DPS\n" CC "3Fertigkeit mit langem Namen:\n")!=NULL,"value group stays whole after the label: %s",out);
}



static void t_hand_panel(void) {
    static const struct { const char *what; dm_ic_panel_in in; int hide; } T[] = {
        { "class 7: knif over swor + grim, the grim panel goes",          { 4, 5, 0, 4, 5, 7, 1, 0 }, 5 },
        { "class 4: one-hand weapon over two one-hand weapons, both stay", { 4, 5, 0, 4, 5, 5, 1, 1 }, 0 },
        { "class 6: h2h over h2h + h2h, both stay",                        { 4, 5, 0, 4, 5, 5, 1, 1 }, 0 },
        { "class 6: swor over h2h + shield, the shield panel goes",        { 4, 5, 0, 4, 5, 7, 1, 0 }, 5 },
        { "shield over swor + shield, the weapon panel goes",              { 4, 5, 0, 5, 5, 7, 1, 0 }, 4 },
        { "body locations in the other order, the same hand goes",         { 5, 4, 0, 4, 5, 7, 1, 0 }, 5 },
        { "two-handed weapon, inventory full (code 0): game's panels",     { 4, 5, 0, 4, 0, 0, 1, 0 }, 0 },
        { "two-handed weapon takes both hands off (code 7): both stay",    { 4, 5, 0, 4, 7, 7, 1, 0 }, 0 },
        { "two-handed weapon into an empty hand, the other goes (code 2)", { 4, 5, 0, 4, 2, 7, 1, 0 }, 0 },
        { "other hand empty: nothing to hide",                             { 4, 5, 0, 4, 5, 1, 0, 1 }, 0 },
        { "class 7, right hand empty: the grim may be swapped, it stays",  { 4, 5, 0, 4, 1, 5, 1, 1 }, 0 },
        { "other hand stacks (code 6): it stays",                          { 4, 5, 0, 4, 5, 6, 1, 0 }, 0 },
        { "auto slot stacks (code 6): game's panels",                      { 4, 5, 0, 4, 6, 7, 1, 0 }, 0 },
        { "auto-slot pick failed: game's panels",                          { 4, 5, 0, 0, 0, 0, 1, 0 }, 0 },
        { "inactive weapon set slot: game's panels",                       { 4, 5, 0, 11, 5, 7, 1, 0 }, 0 },
        { "mercenary's gear shown: game's panels",                         { 4, 5, 1, 4, 5, 7, 1, 0 }, 0 },
        { "pairing rule not asked: game's panels",                         { 4, 5, 0, 4, 5, 7, 1, -1 }, 0 },
        { "rings (6 / 7): game's panels",                                  { 6, 7, 0, 6, 5, 7, 1, 0 }, 0 },
        { "one body location only: game's panels",                         { 3, 3, 0, 3, 5, 0, 1, 0 }, 0 },
    };
    for (size_t i = 0; i < sizeof T / sizeof *T; i++) { int got = dm_ic_panel_hide(&T[i].in); CHECK(got == T[i].hide, "%s: %d, want %d", T[i].what, got, T[i].hide); }
}


static void t_hand_panel_memo(void) {
    dm_ic_panel_memo m = { 0 };
    CHECK(dm_ic_panel_memo_get(&m, 0, 0) == -1, "nothing kept yet");
    dm_ic_panel_memo_set(&m, 77, 1000, 5);
    CHECK(dm_ic_panel_memo_get(&m, 77, 1000) == 5, "same submission");
    CHECK(dm_ic_panel_memo_get(&m, 77, 1000 + DM_IC_PANEL_FRAMES - 1) == 5, "still kept on the last frame");
    CHECK(dm_ic_panel_memo_get(&m, 77, 1000 + DM_IC_PANEL_FRAMES) == -1, "decided again once the frames ran out");
    CHECK(dm_ic_panel_memo_get(&m, 78, 1001) == -1, "another item or hand: decided again");
    CHECK(dm_ic_panel_memo_get(&m, 77, 999) == -1, "frame counter behind the kept frame: decided again");
    dm_ic_panel_memo_set(&m, 78, 1001, 0);
    CHECK(dm_ic_panel_memo_get(&m, 78, 1002) == 0, "a kept 'leave the boxes' result is reused too");
    CHECK(dm_ic_panel_memo_get(&m, 77, 1002) == -1, "the older key is gone");
}


static void t_place(void) {
    int x, y;

    dm_tipx_place_in dock = { .tx = 594, .ty = 142, .w = 741, .h = 768, .centre = 1, .above = 1, .pad = 106, .screen_w = 2688, .screen_h = 1216, .scale = 0.563f };
    dm_tipx_place(&dock, &x, &y); CHECK(x == 224 && y == 142, "docked box below its point: %d,%d", x, y);

    dm_tipx_place_in cell = { .tx = 1478, .ty = 705, .tw = 128, .th = 192, .w = 640, .h = 477, .centre = 1, .above = 1, .pad = 106, .screen_w = 2688, .screen_h = 1216, .scale = 0.563f };
    dm_tipx_place(&cell, &x, &y); CHECK(x == 1222 && y == 228, "box above the cell: %d,%d", x, y);

    cell.ox = 3; cell.oy = -9; dm_tipx_place(&cell, &x, &y); CHECK(x == 1224 && y == 223, "offset above: %d,%d", x, y);
    cell.ty = 300; dm_tipx_place(&cell, &x, &y); CHECK(x == 1220 && y == 497, "offset below: %d,%d", x, y);

    dm_tipx_place_in pad = { .tx = 1346, .ty = 701, .tw = 128, .th = 128, .w = 741, .h = 768, .centre = 1, .above = 1, .pad = 28, .screen_w = 2688, .screen_h = 1216, .scale = 0.563f };
    dm_tipx_place(&pad, &x, &y); CHECK(x == 1040 && y == 420, "pushed back inside: %d,%d", x, y);

    dock.w = 1055; dm_tipx_place(&dock, &x, &y); CHECK(x == 106 && y == 142, "left edge: %d,%d", x, y);
    cell.centre = 0; cell.ox = cell.oy = 0; cell.ty = 705; dm_tipx_place(&cell, &x, &y); CHECK(x == 1478 && y == 228, "not centred: %d,%d", x, y);
}



static void t_top_line(void) {
    const char *text = "\xee\x81\xbe" "2own line\nBase\n\xee\x81\xbe" "4Name";
    size_t at, n;
    CHECK(dm_tipx_top_line(text, 0, &at, &n) && n == 8 && !memcmp(text + at, "\xee\x81\xbe" "4Name", n), "line 0 is the last line: %zu+%zu", at, n);
    CHECK(dm_tipx_top_line(text, 1, &at, &n) && n == 4 && !memcmp(text + at, "Base", n), "line 1 the one before: %zu+%zu", at, n);
    CHECK(dm_tipx_top_line(text, 2, &at, &n) && at == 0 && n == 12, "line 2 the first: %zu+%zu", at, n);
    CHECK(!dm_tipx_top_line(text, 3, &at, &n), "no fourth line");
    CHECK(dm_tipx_top_line("Name", 0, &at, &n) && at == 0 && n == 4 && !dm_tipx_top_line("Name", 1, &at, &n), "one line: only line 0");
    CHECK(dm_tipx_top_line("a\n", 0, &at, &n) && at == 2 && n == 0 && dm_tipx_top_line("a\n", 1, &at, &n) && at == 0 && n == 1, "an empty top line has no width");
    CHECK(dm_tipx_top_line("", 0, &at, &n) && n == 0 && !dm_tipx_top_line(NULL, 0, &at, &n) && !dm_tipx_top_line("a", -1, &at, &n), "empty text, no text, bad index");
}


static void t_dot_line(void) {
    CHECK(!dm_tipx_dot_line("Long Sword\nQuality\n45", 0), "no vendor: the number stays a text line");
    CHECK(dm_tipx_dot_line("Long Sword\nQuality\n45", 1), "the price as the top line is skipped");
    CHECK(dm_tipx_dot_line("Schwert\nQualit\xee\x81\xbe" "4\nt 45 ", 1), "colour code inside, trailing space");
    CHECK(!dm_tipx_dot_line("Langschwert\nQualit\xee\x81\xbe" "4\nt", 1), "vendor without a number: the game's top line is the text's last line");
    CHECK(!dm_tipx_dot_line("Preis: 45\nName", 1), "a line above the price: the price is not the top line");
    CHECK(!dm_tipx_dot_line("45\n\xee\x81\xbe" "9", 1) && !dm_tipx_dot_line("a\n", 1), "a colour code or a space is no number");
    CHECK(!dm_tipx_dot_line(NULL, 1) && !dm_tipx_dot_line("", 1) && !dm_tipx_dot_line("45", 0), "no text, empty text, no vendor");
}


static void t_side_boxes(void) {
    static uint64_t blob[0x100]; uint64_t *regs = blob;
    uint64_t frame = (uint64_t)(uintptr_t)blob + 0x330;
    regs[29] = frame + 0x320;
    uint64_t *l1 = (uint64_t *)((uint8_t *)blob + 0x330 + 0x40), *l2 = (uint64_t *)((uint8_t *)blob + 0x330 + 0x18);
    #define OBJ(o, sz) do { o[0] = (uint64_t)(uintptr_t)((uint8_t *)o + 0x18); o[1] = (sz); o[2] = (1ull << 63) | 8; } while (0)
    OBJ(l1, 0); OBJ(l2, 0);
    CHECK(dm_tipx_side_boxes(regs[29], regs) == 0, "both texts empty: no box beside");
    OBJ(l2, 5);
    CHECK(dm_tipx_side_boxes(regs[29], regs) == 1, "one text with a size: a box stands beside");
    OBJ(l1, 99);
    CHECK(dm_tipx_side_boxes(regs[29], regs) == -1, "a text over its capacity: unreadable");
    OBJ(l1, 0);
    CHECK(dm_tipx_side_boxes(regs[29] + 8, regs) == -1, "frame behind x29 not the builder's: unreadable");
    l2[2] = 4;
    CHECK(dm_tipx_side_boxes(regs[29], regs) == -1, "an invalid text object: unreadable");
    #undef OBJ
}



static void t_package(void) {
    int before[IC_S_N] = { 0 }, after[IC_S_N] = { 0 };
    before[IC_S_FIRE] = 30; after[IC_S_FIRE] = 45;
    before[IC_S_HP] = 500 * 256; after[IC_S_HP] = 620 * 256; before[IC_S_STR] = 52; after[IC_S_STR] = 52; before[IC_S_MF] = 20; after[IC_S_MF] = 35;
    dm_ic_pkg_sk sk[10];
    static const char *const names[10] = { "Key A", "Key B", "S1", "S2", "S3", "S4", "S5", "S6", "S7", "S8" };
    for (int i = 0; i < 10; i++) sk[i] = (dm_ic_pkg_sk){ names[i], 5, 6, i < 2 ? i : -1 };
    sk[9].after = 5;
    int cbb[IC_C_N + 1] = { 900, 0, 1200, 10, 20 }, cba[IC_C_N + 1] = { 900, 0, 1350, 30, 60 };
    dm_ic_pkg_in in = { before, after, 0, "Ring of Strength + Large Charm of Strength", sk, 10, 1, cbb, cba };
    dm_ic_pkg_words w = { LB, "Cap", TO, "mixed", "wearable with %s from this recommendation", "%s: level %d to %d", "(gone)", "+%d more skills",
                          { "Defense", "Block", "Attack Rating (Attack)", "Damage (Attack)" } };
    dm_ic_pkg_line l[32]; int n = dm_ic_pkg_lines(&in, &w, l, 32);
    CHECK(n == 15, "package lines: %d", n);
    CHECK(n > 0 && l[0].kind == IC_PKG_NOTE && !strcmp(l[0].text, CC "5wearable with Ring of Strength + Large Charm of Strength from this recommendation"), "note: %s", n ? l[0].text : "");
    CHECK(n > 2 && strstr(l[1].text, "Fire res") && strstr(l[1].text, CC "3+15 (30 % auf 45 %)") && strstr(l[2].text, "Life") && strstr(l[2].text, "+120 (500 auf 620)"),
          "values: %s | %s", n > 2 ? l[1].text : "", n > 2 ? l[2].text : "");
    CHECK(n > 11 && strstr(l[3].text, CC "6Key A: level 5 to 6") && strstr(l[4].text, "Key B") && strstr(l[10].text, "S6") && l[11].kind == IC_PKG_MORE && !strcmp(l[11].text, CC "5+1 more skills"),
          "skills: %s .. %s | %s", n > 11 ? l[3].text : "", n > 11 ? l[10].text : "", n > 11 ? l[11].text : "");
    CHECK(n > 13 && strstr(l[12].text, "Attack Rating (Attack)") && strstr(l[12].text, "+150") && strstr(l[13].text, "Damage (Attack)") && strstr(l[13].text, CC "310-20 auf 30-60"),
          "combat: %s | %s", n > 13 ? l[12].text : "", n > 13 ? l[13].text : "");
    CHECK(n == 15 && strstr(l[14].text, "MF") && l[14].kind == IC_PKG_VAL, "find last: %s", n == 15 ? l[14].text : "");
    for (int i = 1; i < n; i++) CHECK(!strstr(l[i].text, "Defense") && !strstr(l[i].text, "Block") && !strstr(l[i].text, "Str"), "an unchanged value has a line: %s", l[i].text);

    sk[0].after = 0; in.with = ""; in.nsk = 1; in.cb = 0;
    n = dm_ic_pkg_lines(&in, &w, l, 32);
    CHECK(n == 4 && l[0].kind == IC_PKG_VAL && strstr(l[2].text, CC "1Key A: level 5 to 0 (gone)"), "gone: %d %s", n, n > 2 ? l[2].text : "");

    CHECK(dm_ic_pkg_lines(&in, &w, l, 2) == 2, "more lines than room");
}



static void t_sim_veto(void) {
    const unsigned AR = 1u << IC_C_AR, DMG = 1u << IC_C_DMG, ARD = AR | DMG; const char *why;

    dm_ic_veto v; memset(&v, 0, sizeof v); dm_ic_veto_add(&v, DMG, "dmCmpUnproven", 1);
    CHECK(v.bad == DMG && !strcmp(v.why[IC_C_DMG], "dmCmpUnproven") && !(v.sim & ARD), "display path only: bad %x sim %x", v.bad, v.sim);

    CHECK(dm_ic_sim_swap_attack(&v, 0, &why) == 0 && !why, "display only swap withheld");

    dm_ic_veto_add(&v, DMG, "dmCmpLive", 0);
    CHECK(v.sim == DMG && !strcmp(v.sim_why[IC_C_DMG], "dmCmpLive") && !strcmp(v.why[IC_C_DMG], "dmCmpUnproven"), "mismatch hidden: sim %x %s", v.sim, v.sim_why[IC_C_DMG] ? v.sim_why[IC_C_DMG] : "-");
    CHECK(dm_ic_sim_swap_attack(&v, 0, &why) == 2 && why && !strcmp(why, "dmGearNoLive"), "mismatch not rejected");

    memset(&v, 0, sizeof v); dm_ic_veto_add(&v, DMG, "dmCmpUnproven", 1); dm_ic_veto_add(&v, ARD, "dmCmpPassive", 0);
    CHECK(dm_ic_sim_swap_attack(&v, 0, &why) == 1 && !why, "mastery swap not the passive model's: sim %x", v.sim);
    dm_ic_veto_add(&v, AR, "dmCmpLive", 0);
    CHECK(dm_ic_sim_swap_attack(&v, 0, &why) == 1 && !why, "a later reason replaced the passive one");

    memset(&v, 0, sizeof v); dm_ic_veto_add(&v, DMG, "dmCmpUnproven", 1); dm_ic_veto_add(&v, AR, "dmCmpPassive", 0); dm_ic_veto_add(&v, DMG, "dmCmpLive", 0);
    CHECK(dm_ic_sim_swap_attack(&v, 0, &why) == 3 && why && !strcmp(why, "dmGearNoLive"), "passive + mismatch");

    memset(&v, 0, sizeof v); dm_ic_veto_add(&v, ARD, "dmCmpUnproven", 0);
    CHECK(v.bad == ARD && v.sim == ARD && dm_ic_sim_swap_attack(&v, 0, &why) == 2 && !why, "unreadable inputs");
    CHECK(dm_ic_sim_swap_attack(&v, 16, &why) == 2 && why && !strcmp(why, "dmGearNoRanged"), "ranged after");

    CHECK(dm_ic_mastery_moves(28, 30, 1) && dm_ic_mastery_moves(-1, 28, 1) && dm_ic_mastery_moves(28, -1, 1), "type change with mastery");
    CHECK(!dm_ic_mastery_moves(28, 28, 1) && !dm_ic_mastery_moves(28, 30, 0) && !dm_ic_mastery_moves(-1, -1, 0), "no change or no mastery");

    const int both[2] = { 1, 1 }, none[2] = { 0, 0 }, keep_left[2] = { 0, 1 }, axe_sword[2] = { 28, 30 }, mastery_right[2] = { 1, 0 };
    CHECK(!dm_ic_one_after_mastery(6, both, none, axe_sword, none, 31, 0), "a mace for both, no mastery: the model's");
    CHECK(dm_ic_one_after_mastery(6, both, none, axe_sword, mastery_right, 31, 0), "a mace for both, the axe hand's mastery: the passive model's");
    CHECK(dm_ic_one_after_mastery(6, both, none, axe_sword, none, 28, 1), "a new axe with a mastery, the sword hand changes type: the passive model's");
    CHECK(dm_ic_one_after_mastery(6, both, keep_left, axe_sword, mastery_right, 30, 0), "the sword kept, the axe hand goes with its mastery: the passive model's");
    CHECK(!dm_ic_one_after_mastery(6, both, keep_left, axe_sword, none, 30, 0), "the sword kept, no mastery: the model's");
    const int one[2] = { 1, 0 }, axe_only[2] = { 28, -1 };
    CHECK(!dm_ic_one_after_mastery(6, one, none, axe_only, mastery_right, 28, 1), "the same type with a mastery: the model's");

    CHECK(!dm_ic_one_after_mastery(4, both, keep_left, axe_sword, mastery_right, 30, 0), "class 4, the sword kept, the axe hand goes with its mastery: the model's");
    CHECK(!dm_ic_one_after_mastery(4, both, keep_left, axe_sword, none, 30, 1), "class 4, the sword kept with its mastery: the model's");
    CHECK(dm_ic_one_after_mastery(4, both, none, axe_sword, mastery_right, 31, 0), "class 4, a mace for both, the axe hand's mastery: the passive model's");

    CHECK(!dm_ic_dual_types_passive(4, 28, 32, 1), "class 4, an axe and a club with a mace mastery: the model's");
    CHECK(!dm_ic_dual_types_passive(4, 32, 28, 1), "class 4, the club swinging: the model's");
    CHECK(dm_ic_dual_types_passive(6, 50, 51, 1), "class 6, two hand-to-hand bases with a mastery: the passive model's");
    CHECK(!dm_ic_dual_types_passive(6, 50, 50, 1) && !dm_ic_dual_types_passive(6, 50, 51, 0), "class 6, one base or no mastery: the model's");
}


static void t_ranged(void) {
    int none[2] = { 0, 0 }, right[2] = { 1, 0 }, left[2] = { 0, 1 };
    CHECK(!dm_ic_ranged_unproven(none, 0), "bow taken off: unproven");
    CHECK(dm_ic_ranged_unproven(right, 0) && dm_ic_ranged_unproven(left, 0), "bow in hand after: proven");
    CHECK(!dm_ic_ranged_unproven(right, 1), "gear worn now: unproven");
    CHECK(!strcmp(dm_ic_unrated_key(0, 1, 0, 1, "dmGearNoRanged"), "dmGearNoRanged"), "after side only: not named");
    CHECK(!strcmp(dm_ic_unrated_key(0, 1, 1, 1, "dmGearNoRanged"), "dmGearNoSim"), "now side bad: named");
    CHECK(!strcmp(dm_ic_unrated_key(-1, 1, 0, 0, "dmGearNoRanged"), "dmGearNoSim"), "read failed: named");
    CHECK(!strcmp(dm_ic_unrated_key(0, 0, 0, 1, "dmGearNoRanged"), "dmGearNoSim"), "now invalid: named");
    CHECK(!strcmp(dm_ic_unrated_key(0, 1, 0, 1, NULL), "dmGearNoSim"), "no name: not the general key");
}


static void t_charm(void) {



    CHECK(dm_ic_charm_move(1, 1, 0, 0) == IC_CHARM_OUT, "own charm in the inventory: removal");
    CHECK(dm_ic_charm_move(1, 1, 0, 3) == IC_CHARM_IN && dm_ic_charm_move(1, 1, 0, 4) == IC_CHARM_IN, "own charm in cube / stash: addition");
    CHECK(dm_ic_charm_move(1, 0, 0, 0) == IC_CHARM_IN, "foreign charm seen on page 0 (vendor): addition");
    CHECK(dm_ic_charm_move(1, 1, 1, 0) == IC_CHARM_NONE && dm_ic_charm_move(1, 1, 4, 0) == IC_CHARM_NONE, "worn or on the cursor: no direction");
    CHECK(dm_ic_charm_move(0, 1, 0, 0) == IC_CHARM_NONE, "not a charm: no direction");


    dm_ic_line l[IC_M_N]; dm_ic_in in;
    zero(&in); in.tot[IC_S_HP] = 300 * 256; in.new_[IC_S_HP] = 20 * 256;
    int n = dm_ic_compute(&in, LB, "Cap", TO, l, IC_M_N);
    CHECK(n == 1 && !strcmp(l[0].text, CC "0Life " CC "3+20 (300 auf 320)"), "charm addition green: %s", n ? l[0].text : "");
    zero(&in); in.tot[IC_S_HP] = 320 * 256; in.old_[IC_S_HP] = 20 * 256;
    n = dm_ic_compute(&in, LB, "Cap", TO, l, IC_M_N);
    CHECK(n == 1 && !strcmp(l[0].text, CC "0Life " CC "1-" "20 (320 auf 300)"), "charm removal red: %s", n ? l[0].text : "");
    zero(&in); in.tot[IC_S_FIRE] = 64; in.new_[IC_S_FIRE] = 11;
    n = dm_ic_compute(&in, LB, "Cap", TO, l, IC_M_N);
    CHECK(n == 1 && l[0].cap == 1 && !strcmp(l[0].text, CC "0Fire res " CC "3+11 (64 % auf 75 %) " CC "4Cap"), "resistance reaching the cap: %s", n ? l[0].text : "");
    zero(&in); in.tot[IC_S_HP] = 200 * 256; in.new_[IC_S_VIT] = 5;
    n = dm_ic_compute(&in, LB, "Cap", TO, l, IC_M_N);
    CHECK(n == 1 && l[0].metric == IC_M_LIFE && l[0].before == 200 && l[0].after == 208 && !find(l, n, IC_M_VIT), "vitality converted: %d > %d", n ? l[0].before : 0, n ? l[0].after : 0);
    zero(&in); in.tot[IC_S_HP] = 300 * 256; in.new_[IC_S_HP] = 20 * 256; in.hypothetical = 1;
    n = dm_ic_compute(&in, LB, "Cap", TO, l, IC_M_N);
    CHECK(n == 1 && !strcmp(l[0].text, CC "5Life " CC "5+20 (300 auf 320)"), "unusable charm grey: %s", n ? l[0].text : "");


    CHECK(dm_ic_slot_group(4, 0) == 0 && dm_ic_slot_group(4, 1) == 0, "right hand is the weapon group either way");
    CHECK(dm_ic_slot_group(5, 1) == 0 && dm_ic_slot_group(5, 0) == 1, "left hand: a weapon joins the weapon group, a shield is the off hand");
    CHECK(dm_ic_slot_group(0, 0) == 2 && dm_ic_slot_group(0, 1) == 2 && dm_ic_slot_group(1, 0) == 2 && dm_ic_slot_group(6, 0) == 3, "other slots keep their groups (slot 0 = armour)");


    static const uint64_t rows[] = { 0xe11a0001ull, 0xe2a00001ull, 0xe4a00001ull, 0xe8002000ull, 0xea000010ull, 0xec000040ull, 0xf09c4004ull, 0xf09e4004ull,
                                     0xf11a4004ull, 0xf4a00040ull, 0xf6a00004ull, 0xf89c4880ull, 0xf89e4880ull, 0xf91ac880ull, 0xfaa00800ull, 0xfea00100ull, 0xfea00120ull,
                                     0x0a000010ull, 0x2a000010ull, 0x4a000010ull, 0x6a000010ull, 0x8a000010ull, 0xaa000010ull, 0xca000010ull, 0x8c000040ull };
    for (size_t i = 0; i < sizeof rows / sizeof *rows; i++) {
        uint64_t r = rows[i];
        int cls = (int)(r >> 29 & 7), mt = (int)(r >> 27 & 3), gp = (int)(r >> 25 & 3), md = (int)(r >> 23 & 3), wt = (int)(r >> 16 & 0x7f);
        uint32_t src = (uint32_t)(r & 0xffff);
        CHECK(mt < IC_C_N && wt <= 127, "row %zu in the table's format", i);
        CHECK(!(r & (DM_IC_CELL_CAP_NEW | DM_IC_CELL_SET_COMP | DM_IC_CELL_CHARM)), "row %zu carries no extra bit yet", i);
        CHECK(dm_ic_cell_key(cls, mt, gp, 0, md, wt, src, 0, 0) == r, "row %zu rebuilt bit-identically", i);
        CHECK(dm_ic_cell_key(cls, mt, gp, IC_CHARM_IN, md, wt, src, 0, 0) == (r | DM_IC_CELL_CHARM), "row %zu as a charm cell differs from it", i);
        CHECK(dm_ic_cell_key(cls, mt, gp, 0, md, wt, src, 1, 0) == (r | DM_IC_CELL_CAP_NEW), "row %zu with the new cap branch", i);
        CHECK(dm_ic_cell_key(cls, mt, gp, 0, md, wt, src, 0, 1) == (r | DM_IC_CELL_SET_COMP), "row %zu with set composition", i);
    }
    CHECK(dm_ic_cell_key(7, IC_C_DEF, 0, IC_CHARM_OUT, 1, 32, 1, 0, 0) != 0xe4a00001ull, "a charm cell never equals the armour defence row");
    CHECK(dm_ic_cell_key(8, 0, 0, 1, 1, 0, 1, 0, 0) == 0 && dm_ic_cell_key(7, 0, 0, 1, 1, 128, 1, 0, 0) == 0, "outside the format: 0 (never proven)");
}



static void t_block_rows(void) {
    int v[IC_C_N + 1];


    for (int cls = 0; cls < 7; cls++) for (int wt = 28; wt <= 32; wt += 2)
        CHECK(dm_ic_cell_key(cls, IC_C_BLK, 1, 0, 1, wt, 0x10, 0, 0) == (0x0a000010ull | (uint64_t)cls << 29), "class %d off hand block row, weapon type %d", cls, wt);
    CHECK(dm_ic_cell_key(7, IC_C_BLK, 0, 0, 2, 26, 0x2000, 0, 0) == dm_ic_cell_key(7, IC_C_BLK, 0, 0, 1, 127, 0x2000, 0, 0), "block cell ignores hand mode");
    CHECK(dm_ic_cell_key(7, IC_C_AR, 0, 0, 1, 28, 4, 0, 0) != dm_ic_cell_key(7, IC_C_AR, 0, 0, 1, 30, 4, 0, 0), "attack rating cell keeps the weapon type");
    CHECK(dm_ic_cell_key(4, IC_C_BLK, 1, 0, 1, -1, 0x10, 0, 0) == 0, "an unknown weapon record stays outside the table");
    { static const int bench[][3] = { { 20, 7, 16 }, { 25, 9, 18 }, { 30, 11, 20 } };
      for (size_t i = 0; i < sizeof bench / sizeof *bench; i++) {
          dm_ic_cb c; memset(&c, 0, sizeof c); c.can_block = 1; c.expansion = 1; c.dex = 60; c.lvl = 60; c.bf = bench[i][0];
          c.toblock = 0; dm_ic_cb_eval(&c, v); int before = v[IC_C_BLK]; c.toblock = 24; dm_ic_cb_eval(&c, v);
          CHECK(before == bench[i][1] && v[IC_C_BLK] == bench[i][2], "block factor %d: %d -> %d", bench[i][0], before, v[IC_C_BLK]); }
      dm_ic_cb c; memset(&c, 0, sizeof c); c.can_block = 1; c.expansion = 1; c.dex = 80; c.lvl = 60; c.bf = 25; dm_ic_cb_eval(&c, v);
      CHECK(v[IC_C_BLK] == 13, "Barbarian buckler with +20 dexterity: %d", v[IC_C_BLK]);

      for (int w = 0; w < 3; w++) { memset(&c, 0, sizeof c); c.can_block = 1; c.expansion = 1; c.dex = 60; c.lvl = 60; c.bf = 25; c.toblock = 24; c.armed = 1;
          c.mn[0] = w == 0 ? 1 : w == 1 ? 4 : 2; c.mx[0] = w == 0 ? 4 : w == 1 ? 11 : 7; dm_ic_cb_eval(&c, v); CHECK(v[IC_C_BLK] == 18, "weapon %d beside the tower shield: %d", w, v[IC_C_BLK]); }

      for (int two = 0; two < 2; two++) { memset(&c, 0, sizeof c); c.can_block = 0; c.expansion = 1; c.dex = 60; c.lvl = 60; c.bf = 25; c.toblock = 24; c.armed = 1;
          c.mode2 = !two; dm_ic_cb_eval(&c, v); CHECK(v[IC_C_BLK] == 0, "%s without a shield: %d", two ? "dual wield" : "two-handed", v[IC_C_BLK]); } }
}



static void t_insert_under(void) {
    char out[256]; const char *cc = "\xee\x81\xbe";
    char text[96]; snprintf(text, sizeof text, "%s3Defense: 3\n%s4Biggin's Bonnet\nCap", cc, cc);
    size_t n = dm_tipx_insert_under(text, 2, "Look: Great Helm", out, sizeof out);
    char want[160]; snprintf(want, sizeof want, "%s3Defense: 3\nLook: Great Helm\n%s3%s4Biggin's Bonnet\nCap", cc, cc, cc);
    CHECK(n == strlen(want) && !strcmp(out, want), "under a two-line name, the defense colour restored: %s", out);
    n = dm_tipx_insert_under("Durability: 9\nCap", 1, "Look: Great Helm", out, sizeof out);
    snprintf(want, sizeof want, "Durability: 9\nLook: Great Helm\n%s0Cap", cc);
    CHECK(n && !strcmp(out, want), "white when the text set no colour: %s", out);
    n = dm_tipx_insert_under("Cap", 1, "Look: Great Helm", out, sizeof out);
    snprintf(want, sizeof want, "Look: Great Helm\n%s0Cap", cc);
    CHECK(n && !strcmp(out, want), "a name alone: the line goes under it: %s", out);
    CHECK(dm_tipx_insert_under("Durability: 9\nCap", 1, "Look: Great Helm", out, 20) == 0, "too small a buffer is refused");
}


static const char *fx_fmt(int m) {
    static const char *const f[IC_M_N + 1] = { [IC_M_FCR] = "%s FCR", [IC_M_RES_FIRE] = "%s Feuerschaden", [IC_M_RES_COLD] = "%s Kälteschaden", [IC_M_LIFE] = "%s Leben",
        [IC_M_MANA] = "%s Mana", [IC_M_MF] = "%s Unique", [IC_M_GF] = "%s Gold", [IC_M_FRW] = "%s Lauftempo", [IC_X_BLOCK] = "Block: %s Treffer" };
    return f[m];
}
static void t_effect(void) {
    int p = 0;
    CHECK(dm_ic_effect(IC_M_RES_FIRE, 41, 66, &p) && p == -42, "resistance 41 > 66: %d", p);
    CHECK(dm_ic_effect(IC_M_RES_FIRE, -55, -30, &p) && p == -16, "the same +25 in Hell: %d", p);
    CHECK(dm_ic_effect(IC_M_RES_COLD, 41, 75, &p) && p == -58, "up to the cap: %d", p);
    CHECK(dm_ic_effect(IC_M_RES_FIRE, 66, 41, &p) && p == 74, "a loss: %d", p);
    CHECK(!dm_ic_effect(IC_M_LIFE, 120, 121, &p), "+1 life of 120 is under 1 %%");
    CHECK(dm_ic_effect(IC_M_MANA, 85, 55, &p) && p == -35, "mana 85 > 55: %d", p);
    CHECK(dm_ic_effect(IC_M_MF, 0, 25, &p) && p == 23 && dm_ic_effect(IC_M_MF, 100, 150, &p) && p == 13, "magic find flattens: %d", p);
    CHECK(dm_ic_effect(IC_M_GF, 100, 150, &p) && p == 25, "gold find: %d", p);
    CHECK(!dm_ic_effect(IC_M_FRW, 0, 20, &p), "run speed has no effect measure (running adds its own modifier): points only");
    CHECK(dm_ic_effect(IC_X_BLOCK, 42, 51, &p) && p == -16, "block 42 > 51: %d", p);
    CHECK(!dm_ic_effect(IC_M_FCR, 0, 20, &p) && !dm_ic_effect(IC_M_LL, 0, 5, &p) && !dm_ic_effect(IC_M_MANA, 0, 20, &p), "no measure for speeds, leech, an empty pool");

    CHECK(dm_ic_line_says(IC_M_FCR, 0, 20, 20) && dm_ic_line_says(IC_M_FRW, 10, 30, 20), "the item's line says it: nothing it displaces had the stat");
    CHECK(!dm_ic_line_says(IC_M_FCR, 10, 0, 10) && !dm_ic_line_says(IC_M_FCR, 20, 10, 10) && !dm_ic_line_says(IC_M_FHR, 20, 10, 10), "a removal and a swap 20 > 10 keep their loss line");
    CHECK(!dm_ic_line_says(IC_M_MANA, 0, 20, 20) && !dm_ic_line_says(IC_M_FCR, 0, 20, 0), "only points values, only with a line");

    char o[96];
    CHECK(dm_ic_fill("Feuerwiderstand %+d%%", 25, o, sizeof o) && !strcmp(o, "Feuerwiderstand +25%"), "%%+d: %s", o);
    CHECK(dm_ic_fill("Feuerwiderstand %+d%%", -30, o, sizeof o) && !strcmp(o, "Feuerwiderstand -30%"), "negative: %s", o);
    CHECK(dm_ic_fill("%d%% bessere Chance", 100, o, sizeof o) && !strcmp(o, "100% bessere Chance"), "%%d: %s", o);
    CHECK(!dm_ic_fill("%s zu Mana", 5, o, sizeof o) && !dm_ic_fill("%5d zu Mana", 5, o, sizeof o) && !dm_ic_fill("%+d zu Mana", 5, o, 4), "other conversions or no room: no line");

    dm_ic_fx_words w = { .brief = "%s Schaden", .cap = "Grenze", .over = "%d Punkte über Grenze" }; dm_ic_fx x;
    for (int m = 0; m <= IC_M_N; m++) w.fmt[m] = fx_fmt(m);
    CHECK(dm_ic_fx_text(IC_M_RES_FIRE, 41, 66, 0, 0, 0, &w, &x) && !strcmp(x.bracket, CC "3(-42 % Feuerschaden" CC "3)") && !strcmp(x.brief, CC "3(-42 % Schaden" CC "3)")
          && !strcmp(x.rest, CC "3-42 % Feuerschaden"), "resistance gain green: %s / %s / %s", x.bracket, x.brief, x.rest);
    CHECK(dm_ic_fx_text(IC_M_RES_COLD, 41, 75, 1, 0, 0, &w, &x) && !strcmp(x.bracket, CC "3(-58 % Kälteschaden" CC "4 \xc2\xb7 Grenze" CC "3)"), "the cap in gold: %s", x.bracket);
    CHECK(dm_ic_fx_text(IC_M_RES_FIRE, 75, 75, 0, 10, 0, &w, &x) && !strcmp(x.bracket, CC "5(10 Punkte über Grenze)") && !x.rest[0], "above the cap: grey, only at the line: %s", x.bracket);
    CHECK(!dm_ic_fx_text(IC_M_LIFE, 120, 121, 0, 0, 0, &w, &x), "under 1 %%: nothing");
    CHECK(dm_ic_fx_text(IC_M_MANA, 85, 55, 0, 0, 0, &w, &x) && !strcmp(x.bracket, CC "1(-35 % Mana" CC "1)") && !strcmp(x.brief, x.bracket), "a pool loss red, no short form: %s", x.bracket);
    CHECK(dm_ic_fx_text(IC_X_BLOCK, 42, 75, 1, 0, 0, &w, &x) && !strcmp(x.bracket, CC "3(Block: -57 % Treffer" CC "4 \xc2\xb7 Grenze" CC "3)"), "block to its cap: %s", x.bracket);
    CHECK(dm_ic_fx_text(IC_M_FCR, 20, 0, 0, 0, 0, &w, &x) && !x.bracket[0] && !strcmp(x.rest, CC "1-20 % FCR"), "a speed only under the heading: %s", x.rest);
    CHECK(dm_ic_fx_text(IC_M_FCR, 20, 10, 0, 0, 0, &w, &x) && !strcmp(x.rest, CC "1-10 % FCR"), "a swap 20 > 10 is a red loss: %s", x.rest);
    CHECK(dm_ic_fx_text(IC_M_FRW, 0, 20, 0, 0, 0, &w, &x) && !x.bracket[0] && !strcmp(x.rest, CC "3+20 % Lauftempo"), "run speed in points: %s", x.rest);
    CHECK(dm_ic_fx_text(IC_M_RES_FIRE, 41, 75, 1, 0, 1, &w, &x) && !strcmp(x.bracket, CC "5(-58 % Feuerschaden" CC "5)"), "an unusable item: grey, no cap label: %s", x.bracket);
    CHECK(!dm_ic_fx_text(IC_M_IAS, 0, 20, 0, 0, 0, &w, &x) && !dm_ic_fx_text(IC_M_LL, 0, 5, 0, 0, 0, &w, &x), "attack speed and leech have no phrase");

    dm_ic_fx f4[4]; const dm_ic_fx *pf[4]; int v[4] = { 25, 25, 25, 25 };
    for (int k = 0; k < 4; k++) { dm_ic_fx_text(IC_M_RES_FIRE + (k & 1), 41, 66, 0, 0, 0, &w, &f4[k]); pf[k] = &f4[k]; }
    CHECK(dm_ic_res_group(v, pf), "all four the same effect: the all resistances line carries it");
    dm_ic_fx_text(IC_M_RES_COLD, 36, 61, 0, 0, 0, &w, &f4[1]);
    CHECK(!dm_ic_res_group(v, pf), "the same points, another effect (-39 %%): each goes under the heading");
    dm_ic_fx_text(IC_M_RES_COLD, 41, 66, 0, 0, 0, &w, &f4[1]); v[2] = 20;
    CHECK(!dm_ic_res_group(v, pf), "different values on the item: no group line");
    v[2] = 25; pf[3] = NULL;
    CHECK(!dm_ic_res_group(v, pf), "one element without an effect: no group line");
}
static void t_frames(void) {
    dm_ic_frame_words w = { "%d auf %d Frames", "weiter %d Frames", "weiter %d Frames, nächster ab %d %%" }; dm_ic_fx x;
    CHECK(dm_ic_frames_text(13, 11, 0, 0, NULL, &w, &x) && !strcmp(x.bracket, CC "3(13 auf 11 Frames)") && !strcmp(x.rest, CC "313 auf 11 Frames"), "fewer frames green: %s", x.bracket);
    CHECK(dm_ic_frames_text(9, 10, 0, 0, "Feuerball", &w, &x) && !strcmp(x.bracket, CC "1(Feuerball: 9 auf 10 Frames)") && !strcmp(x.brief, CC "1(9 auf 10 Frames)"),
          "more frames red, the skill named, the short form without it: %s", x.bracket);
    CHECK(dm_ic_frames_text(11, 11, 48, 0, NULL, &w, &x) && !strcmp(x.bracket, CC "5(weiter 11 Frames, nächster ab 48 %)"), "the same frames grey with the next step: %s", x.bracket);
    CHECK(dm_ic_frames_text(7, 7, 0, 0, NULL, &w, &x) && !strcmp(x.bracket, CC "5(weiter 7 Frames)"), "the last step: no next: %s", x.bracket);
    CHECK(dm_ic_frames_text(13, 11, 0, 1, NULL, &w, &x) && x.bracket[3] == '5', "an unusable item grey");

    static const char tail[] = "weiter %d Frames, nächster ab %d %%\0%s %d %d";
    dm_ic_frame_words t2 = { w.to, w.keep, tail };
    CHECK(dm_ic_frames_text(11, 11, 48, 0, NULL, &t2, &x) && !strcmp(x.bracket, CC "5(weiter 11 Frames, nächster ab 48 %)"), "a trailing %% ends the format: %s", x.bracket);
    dm_ic_frame_words bad = { "%d auf Frames", "weiter %s", "%d %d %d" };
    CHECK(!dm_ic_frames_text(13, 11, 0, 0, NULL, &bad, &x) && !dm_ic_frames_text(7, 7, 0, 0, NULL, &bad, &x) && !dm_ic_frames_text(7, 7, 9, 0, NULL, &bad, &x), "a broken format: no text");
}
static void t_annotate(void) {

    const char *orig = CC "3Feuerwiderstand +25%\n-5 zu Mana\n" CC "3+10 zu Geschicklichkeit\n" CC "0Haltbarkeit: 10 von 16\n" CC "4Dolch\nMöwe";
    static char out[2048]; dm_tipx_span sp[8]; int ns = 0, pl[8];
    dm_tipx_ann a[] = { { "Feuerwiderstand +25%", CC "3(-42 % Feuerschaden" CC "3)", CC "3(-42 % Schaden" CC "3)" }, { "-5 zu Mana", CC "1(-17 % Mana" CC "1)", NULL },
                        { "Kältewiderstand +25%", CC "3(x)", NULL } };
    size_t n = dm_tipx_annotate(orig, a, 3, 0, out, sizeof out, sp, 8, &ns, pl);
    CHECK(n == strlen(out) && pl[0] == DM_TIPX_ANN_PLACED && pl[1] == DM_TIPX_ANN_PLACED && pl[2] == DM_TIPX_ANN_NONE && ns == 2, "two placed, one without a line: %d %d %d", pl[0], pl[1], pl[2]);
    CHECK(!strncmp(out, CC "3Feuerwiderstand +25% " CC "3(-42 % Feuerschaden" CC "3)" CC "3\n-5 zu Mana " CC "1(-17 % Mana" CC "1)" CC "3\n", strlen(CC "3Feuerwiderstand +25% " CC "3(-42 % Feuerschaden" CC "3)" CC "3\n-5 zu Mana " CC "1(-17 % Mana" CC "1)" CC "3\n")),
          "the bracket after the line, then the game's colour again (the mana line's own colour comes from the line before): %s", out);
    CHECK(!strcmp(strstr(out, CC "3+10"), strstr(orig, CC "3+10")), "the rest of the text unchanged");
    CHECK(sp[0].at == strlen(CC "3Feuerwiderstand +25%") && sp[0].len == strlen(" " CC "3(-42 % Feuerschaden" CC "3)"), "our bytes: the space and the bracket, not the colour after it");
    static char col[4096]; CHECK(dm_tipx_colour_spans(out, sp, ns, col, sizeof col), "colour spans fit");
    CHECK(strstr(col, DM_TIPX_COLOUR_TAG CC "3(-42") && strstr(col, DM_TIPX_COLOUR_TAG CC "1(-17") && !strstr(col, DM_TIPX_COLOUR_TAG CC "3Feuer") && !strstr(col, DM_TIPX_COLOUR_TAG CC "3\n"),
          "only our codes are tagged, the game's and the restored one are not: %s", col);

    const char *twice = CC "3+10 zu Leben\n+10 zu Leben\nRing";
    dm_tipx_ann b1 = { "+10 zu Leben", CC "3(+8 % Leben" CC "3)", NULL };
    dm_tipx_annotate(twice, &b1, 1, 0, out, sizeof out, sp, 8, &ns, pl);
    CHECK(pl[0] == DM_TIPX_ANN_MANY && ns == 0 && !strcmp(out, twice), "two equal lines: no bracket, text unchanged");
    dm_tipx_annotate("+10 zu Leben\nRing", &b1, 1, 0, out, sizeof out, sp, 8, &ns, pl);
    CHECK(pl[0] == DM_TIPX_ANN_COLOUR, "no colour code to restore: no bracket");

    dm_tipx_annotate(orig, a, 1, 41, out, sizeof out, sp, 8, &ns, pl);
    CHECK(pl[0] == DM_TIPX_ANN_PLACED && strstr(out, "(-42 % Feuerschaden"), "fits at 41 (20 + 1 + 20)");
    dm_tipx_annotate(orig, a, 1, 36, out, sizeof out, sp, 8, &ns, pl);
    CHECK(pl[0] == DM_TIPX_ANN_PLACED && strstr(out, "(-42 % Schaden") && !strstr(out, "Feuerschaden"), "the short form at 36: %s", out);
    dm_tipx_annotate(orig, a, 1, 30, out, sizeof out, sp, 8, &ns, pl);
    CHECK(pl[0] == DM_TIPX_ANN_WIDE && !strcmp(out, orig), "too wide even short: no bracket");

    dm_tipx_ann c1 = { "Feuerwiderstand +2", CC "3(x)", NULL };
    dm_tipx_annotate(orig, &c1, 1, 0, out, sizeof out, sp, 8, &ns, pl);
    CHECK(pl[0] == DM_TIPX_ANN_NONE, "a prefix of a line is not the line");
    dm_tipx_annotate(CC "3  Feuerwiderstand +25% \nX", a, 1, 0, out, sizeof out, sp, 8, &ns, pl);
    CHECK(pl[0] == DM_TIPX_ANN_PLACED, "outer spaces do not count");
    CHECK(!dm_tipx_annotate(orig, a, 2, 0, out, strlen(orig) + 8, sp, 8, &ns, pl) && ns == 0 && pl[0] != DM_TIPX_ANN_PLACED, "no room: nothing placed");

    const char *parts[] = { CC "1-35 % Mana", CC "1+18 % Blitzschaden", CC "1-20 % FCR" };
    char line[256];
    int k = dm_tipx_join(parts, 3, CC "5 \xc2\xb7 ", 34, line, sizeof line);
    CHECK(k == 2 && !strcmp(line, CC "1-35 % Mana" CC "5 \xc2\xb7 " CC "1+18 % Blitzschaden"), "two fit in 34: %d %s", k, line);
    CHECK(dm_tipx_join(parts + 2, 1, " ", 3, line, sizeof line) == 1, "the first always goes in");
    CHECK(dm_tipx_width(CC "1-35 % Mana") == 10, "width without colour codes");
}


static const char *strip(const char *s) {
    static char o[256]; size_t k = 0;
    for (const char *p = s; *p && k + 1 < sizeof o; p++) { if (!strncmp(p, CC, 3) && p[3]) { p += 3; continue; } o[k++] = *p; }
    o[k] = 0; return o;
}
static int splash_line_known(dm_splash_area a0, dm_splash_area a1, int weapon, int change_ok, int hypo, int known, const char *lang, char *o, size_t n) {
    int de = !strcmp(lang, "de"), pt = !strcmp(lang, "pt");
    return dm_ic_splash_line(a0.share, a0.width, a1.share, a1.width, weapon, change_ok, hypo, known, de ? "Fl\xc3\xa4" "chenschaden" : pt ? "Dano em \xc3\xa1rea" : "Splash damage",
                             de ? "Kegel" : "cone", de ? TO : "%s to %s", o, n);
}
static int splash_line(dm_splash_area a0, dm_splash_area a1, int weapon, int change_ok, int hypo, const char *lang, char *o, size_t n) {
    return splash_line_known(a0, a1, weapon, change_ok, hypo, 1, lang, o, n);
}


static dm_splash_area hands_area(const char *right, const char *left, int mode, int slot, int cls, int str, int dex) {
    const char *t[2] = { right, left }; int w[2] = { right != NULL, left != NULL }, k = dm_ic_splash_hand(w, slot, -1);
    dm_splash_area none = { 0, 0, 0 };
    return k < 0 ? none : dm_splash_area_of(dm_splash_group_of(t[k], mode == 2), cls, str, dex, 0);
}
static void t_splash(void) {
    char o[256]; int k;

    dm_splash_area axe = hands_area("axe", NULL, 2, 4, 4, 150, 80);
    CHECK(axe.share == 60 && axe.width == 65, "two-hand axe: %d %% %d", axe.share, axe.width);
    k = splash_line(axe, axe, 1, 1, 0, "de", o, sizeof o);
    CHECK(k == IC_SPLASH_OWN && !strcmp(o, CC "0Fl\xc3\xa4" "chenschaden: " CC "560 % \xc2\xb7 Kegel\xc2\xa0" "65\xc2\xb0"), "own line DE: %s", o);
    CHECK(!strcmp(strip(o), "Fl\xc3\xa4" "chenschaden: 60 % \xc2\xb7 Kegel\xc2\xa0" "65\xc2\xb0"), "own line DE as shown: %s", strip(o));
    dm_splash_area s75 = { 75, 152, 3 };
    splash_line(s75, s75, 1, 1, 0, "en", o, sizeof o); CHECK(!strcmp(strip(o), "Splash damage: 75 % \xc2\xb7 cone\xc2\xa0" "152\xc2\xb0"), "own line EN: %s", strip(o));
    splash_line(s75, s75, 1, 1, 0, "pt", o, sizeof o); CHECK(!strcmp(strip(o), "Dano em \xc3\xa1rea: 75 % \xc2\xb7 cone\xc2\xa0" "152\xc2\xb0"), "own line PT: %s", strip(o));


    dm_splash_area s30 = { 30, 122, 2 };
    k = splash_line_known(s30, axe, 1, 0, 0, 0, "de", o, sizeof o); CHECK(k == IC_SPLASH_NONE && !o[0], "unknown after-swap attributes: no line (%s)", o);
    k = splash_line_known(s30, s75, 1, 1, 0, 0, "de", o, sizeof o); CHECK(k == IC_SPLASH_NONE, "unknown after-swap attributes: no change line either");
    k = splash_line_known(axe, axe, 1, 0, 0, 1, "de", o, sizeof o); CHECK(k == IC_SPLASH_OWN, "the worn weapon keeps its own line");

    dm_splash_area a30 = { 30, 122, 2 }, a50 = { 50, 137, 2 }, zero = { 0, 0, 0 };
    k = splash_line(a30, a50, 1, 1, 0, "de", o, sizeof o);
    CHECK(k == IC_SPLASH_CHANGE && !strcmp(o, CC "0Fl\xc3\xa4" "chenschaden " CC "3+20 (30 % auf 50 %) \xc2\xb7 Kegel\xc2\xa0" "122\xc2\xb0 auf 137\xc2\xb0"), "change DE: %s", o);
    splash_line(a30, a50, 1, 1, 0, "en", o, sizeof o); CHECK(!strcmp(strip(o), "Splash damage +20 (30 % to 50 %) \xc2\xb7 cone\xc2\xa0" "122\xc2\xb0 to 137\xc2\xb0"), "change EN: %s", strip(o));
    splash_line(a30, a50, 1, 1, 0, "pt", o, sizeof o); CHECK(!strcmp(strip(o), "Dano em \xc3\xa1rea +20 (30 % to 50 %) \xc2\xb7 cone\xc2\xa0" "122\xc2\xb0 to 137\xc2\xb0"), "change PT: %s", strip(o));
    splash_line(a50, a30, 1, 1, 0, "de", o, sizeof o); CHECK(strstr(o, CC "1" "-" "20 (50 % auf 30 %)") != NULL, "share falls: red: %s", o);
    splash_line(a30, a50, 1, 1, 1, "de", o, sizeof o); CHECK(!strncmp(o, CC "5", 3) && strstr(o, CC "5+20") != NULL, "not usable: grey: %s", o);

    dm_splash_area n90 = { 25, 90, 2 }, n105 = { 25, 105, 2 };
    k = splash_line(n90, n105, 1, 1, 0, "de", o, sizeof o);
    CHECK(k == IC_SPLASH_CHANGE && !strcmp(o, CC "0Fl\xc3\xa4" "chenschaden " CC "325 % \xc2\xb7 Kegel\xc2\xa0" "90\xc2\xb0 auf 105\xc2\xb0"), "width only: %s", o);
    splash_line(n105, n90, 1, 1, 0, "de", o, sizeof o); CHECK(!strncmp(o + 3 + strlen("0Fl\xc3\xa4" "chenschaden "), CC "1", 3), "narrower: red: %s", o);
    CHECK(splash_line(n90, n105, 0, 1, 0, "de", o, sizeof o) == IC_SPLASH_NONE && !o[0], "a ring moving dexterity only: no line");

    k = splash_line(a30, a50, 0, 1, 0, "de", o, sizeof o); CHECK(k == IC_SPLASH_CHANGE && strstr(o, "Kegel\xc2\xa0" "122\xc2\xb0 auf 137\xc2\xb0"), "ring with strength and dexterity: %s", o);

    k = splash_line(a50, zero, 1, 1, 0, "de", o, sizeof o);
    CHECK(k == IC_SPLASH_CHANGE && !strcmp(o, CC "0Fl\xc3\xa4" "chenschaden " CC "1" "-" "50 (50 % auf 0 %)"), "dagger for an axe: %s", o);
    splash_line(zero, axe, 1, 1, 0, "de", o, sizeof o); CHECK(!strcmp(strip(o), "Fl\xc3\xa4" "chenschaden +60 (0 % auf 60 %) \xc2\xb7 Kegel\xc2\xa0" "65\xc2\xb0"), "axe for a dagger: %s", strip(o));

    dm_splash_area dagger = hands_area("knif", NULL, 1, 4, 4, 150, 80), sorc = hands_area("axe", NULL, 2, 4, 1, 150, 80), weak = hands_area("swor", NULL, 1, 4, 2, 75, 0);
    CHECK(dagger.share == 0 && sorc.share == 0 && weak.share == 0, "dagger, sorceress, share below 10 %%: %d %d %d", dagger.share, sorc.share, weak.share);
    CHECK(splash_line(dagger, dagger, 1, 1, 0, "de", o, sizeof o) == IC_SPLASH_NONE && !o[0], "dagger: no line");
    CHECK(splash_line(sorc, sorc, 1, 1, 0, "de", o, sizeof o) == IC_SPLASH_NONE, "sorceress: no line");
    CHECK(splash_line(a30, a30, 0, 1, 0, "de", o, sizeof o) == IC_SPLASH_NONE, "armour without change: no line");

    k = splash_line(a30, a50, 1, 0, 0, "de", o, sizeof o); CHECK(k == IC_SPLASH_OWN && !strcmp(strip(o), "Fl\xc3\xa4" "chenschaden: 50 % \xc2\xb7 Kegel\xc2\xa0" "137\xc2\xb0"), "comparison off: own line: %s", strip(o));
    CHECK(splash_line(a30, a50, 0, 0, 0, "de", o, sizeof o) == IC_SPLASH_NONE, "comparison off: no line for a ring");

    CHECK(dm_ic_splash_hand((int[2]){ 1, 0 }, 0, -1) == 0 && dm_ic_splash_hand((int[2]){ 0, 1 }, 4, -1) == 1 && dm_ic_splash_hand((int[2]){ 0, 0 }, 4, 0) == -1, "one or no weapon");
    CHECK(dm_ic_splash_hand((int[2]){ 1, 1 }, 5, 0) == 1 && dm_ic_splash_hand((int[2]){ 1, 1 }, 4, 1) == 0, "two weapons: the hand of the slot");
    CHECK(dm_ic_splash_hand((int[2]){ 1, 1 }, 6, 1) == 1 && dm_ic_splash_hand((int[2]){ 1, 1 }, 6, -1) == -1, "two weapons, a ring: the swinging hand");

    dm_splash_area alone = hands_area("swor", NULL, 2, 4, 4, 150, 80), shield = hands_area("swor", NULL, 1, 5, 4, 150, 80), two = hands_area("swor", "axe", 1, 4, 4, 150, 80);
    CHECK(alone.share == 40 && alone.width == 55 && shield.share == 25 && shield.width == 50, "sword alone %d / with shield %d", alone.share, shield.share);
    CHECK(two.share == 25 && two.width == 50 && hands_area("swor", "axe", 1, 5, 4, 150, 80).share == 40, "second weapon: the hand's own weapon in one hand");
    k = splash_line(alone, shield, 0, 1, 0, "de", o, sizeof o);
    CHECK(k == IC_SPLASH_CHANGE && !strcmp(strip(o), "Fl\xc3\xa4" "chenschaden " "-" "15 (40 % auf 25 %) \xc2\xb7 Kegel\xc2\xa0" "55\xc2\xb0 auf 50\xc2\xb0"), "shield beside the sword: %s", strip(o));

    CHECK(dm_ic_block_on(0, 0, 1, 1) && !dm_ic_block_on(0, 0, 1, 0) && !dm_ic_block_on(0, 0, 0, 1) && dm_ic_block_on(1, 0, 0, 0) && dm_ic_block_on(0, 1, 0, 0), "block gate");

    uint64_t key = 0x1234567890abcdefull;
    CHECK(dm_ic_splash_key(key, 0, IC_SPLASH_NONE, 0, 0) != dm_ic_splash_key(key, 1, IC_SPLASH_NONE, 0, 0), "splash toggled: another key");
    CHECK(dm_ic_splash_key(key, 1, IC_SPLASH_OWN, 60, 140) != dm_ic_splash_key(key, 1, IC_SPLASH_OWN, 60, 141) &&
          dm_ic_splash_key(key, 1, IC_SPLASH_OWN, 60, 140) != dm_ic_splash_key(key, 1, IC_SPLASH_OWN, 55, 140) &&
          dm_ic_splash_key(key, 1, IC_SPLASH_OWN, 60, 140) != dm_ic_splash_key(key, 1, IC_SPLASH_CHANGE, 60, 140), "share, cone and kind move the key");

    int before[IC_S_N] = { 0 }, after[IC_S_N] = { 0 }, cbb[IC_C_N + 1] = { 100, 0, 200, 5, 9 }, cba[IC_C_N + 1] = { 100, 0, 200, 7, 12 };
    const char *labels[IC_M_N]; for (int m = 0; m < IC_M_N; m++) labels[m] = LB[m];
    dm_ic_pkg_words w = { labels, "MAX", TO, "mixed", "with %s", "%s %d > %d", "gone", "+%d more", { "Def", "Block", "AR", "Damage" } };
    splash_line(a30, a50, 1, 1, 0, "de", o, sizeof o);
    dm_ic_pkg_in in = { before, after, 0, "", NULL, 0, 1, cbb, cba, o }; dm_ic_pkg_line out[8]; int n = dm_ic_pkg_lines(&in, &w, out, 8);
    CHECK(n == 2 && strstr(out[0].text, "Damage") && !strcmp(out[1].text, o) && out[1].kind == IC_PKG_VAL, "package: damage, then splash (%d lines)", n);
}

int main(void) {
    t_insert_under();
    t_effect(); t_frames(); t_annotate();
    t_splash();
    t_charm();
    t_block_rows();
    t_package(); t_ranged();
    t_hand_panel();
    t_hand_panel_memo();
    bracket_width();
    label_value_wrap();
    comparison_colours();
    t_join(); t_math(); t_compose(); t_hands(); t_order(); t_capacity(); t_combat(); t_hints(); t_unproven(); t_sim_veto(); t_shrine(); t_state_off(); t_place(); t_top_line(); t_dot_line(); t_side_boxes();
    printf("t_itemcmp: %d checks, %d failed\n", checks, fails);
    return fails ? 1 : 0;
}
