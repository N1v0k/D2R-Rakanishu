

#include "dm_swapskill_core.h"
#include <stdio.h>

typedef struct { int n; struct { int stat, layer, value; } s[8]; } item;
static int get(void *ctx, int stat, int layer) {
    const item *it = ctx; int v = 0;
    for (int i = 0; i < it->n; i++) if (it->s[i].stat == stat && it->s[i].layer == layer) v += it->s[i].value;
    return v;
}

static int skill_class(int sk) { return sk >= 66 && sk <= 95 ? 2 : sk == 149 ? 4 : sk == 36 ? 1 : sk == 0 ? -1 : -2; }

static int fails;
static void check(const char *what, int got, int want) { if (got != want) { fails++; printf("FAIL %s: %d, want %d\n", what, got, want); } }


static int gives(const item *items, int n, int char_class, const int *present, int np, int skill) {
    uint64_t os[8] = {0}, si[8] = {0}, pr[8] = {0}, out[8];
    for (int i = 0; i < n; i++) dm_ss_scan_item(get, (void *)&items[i], char_class, skill_class, os, si);
    for (int i = 0; i < np; i++) pr[present[i] >> 6] |= 1ull << (present[i] & 63);
    dm_ss_given(os, si, pr, out);
    return dm_ss_has(out, skill);
}

int main(void) {
    const int NEC = 2, none[1] = {0};
    item cta = {2, {{97, 149, 3}, {97, 155, 1}}};
    item head = {1, {{107, 74, 2}}};
    item allsk = {3, {{127, 0, 2}, {83, 2, 1}, {188, 17, 3}}};
    item fire = {1, {{107, 36, 3}}};
    item odd = {2, {{107, 5, 1}, {107, 0, 1}}};
    const int ce_node[1] = {74};

    check("stat 97 on the swap set", gives(&cta, 1, NEC, none, 0, 149), 1);
    check("stat 97 with a node on the current set (the pump keeps it as before)", gives(&cta, 1, NEC, (const int[]){149}, 1, 149), 1);
    check("stat 107 at 0 hard points, no node on the current set", gives(&head, 1, NEC, none, 0, 74), 1);
    check("stat 107 with hard points (node on the current set)", gives(&head, 1, NEC, ce_node, 1, 74), 0);
    check("stat 107 of another class", gives(&fire, 1, NEC, none, 0, 36), 0);
    check("stat 107 for the owning class", gives(&fire, 1, 1, none, 0, 36), 1);
    check("stat 107 for a skill without pack data", gives(&odd, 1, NEC, none, 0, 5), 0);
    check("stat 107 for a generic skill", gives(&odd, 1, NEC, none, 0, 0), 0);
    check("stat 107 with an unknown character class", gives(&head, 1, -1, none, 0, 74), 0);
    for (int sk = 0; sk < DM_SS_SKILLS; sk++) if (gives(&allsk, 1, NEC, none, 0, sk)) { check("+all / class / tab skills give no skill", sk, -1); break; }
    { item set[2] = {cta, head};
      check("two items: stat 97", gives(set, 2, NEC, none, 0, 155), 1);
      check("two items: stat 107", gives(set, 2, NEC, none, 0, 74), 1);
      check("two items: an unrelated skill", gives(set, 2, NEC, none, 0, 75), 0); }
    {

      typedef struct { int skill; int32_t item; int level; } node;
      const node charged_only[2] = {{74, -1, 0}, {74, 13, 3}}, hard_points[2] = {{74, -1, 2}, {74, 13, 3}};
      const node *cases[2] = {charged_only, hard_points}; const int want[2] = {1, 0};
      for (int c = 0; c < 2; c++) {
          uint64_t os[8] = {0}, si[8] = {0}, pr[8] = {0}, out[8]; int learned = 0;
          dm_ss_scan_item(get, (void *)&head, NEC, skill_class, os, si);
          for (int i = 0; i < 2; i++) { dm_ss_mark_present(pr, si, cases[c][i].skill, cases[c][i].item, cases[c][i].level); if (cases[c][i].item == -1) learned = cases[c][i].level; }
          dm_ss_given(os, si, pr, out);
          check(c ? "stat 107 and charges, hard points: no swap" : "stat 107 and charges: the swap set still gives it", dm_ss_has(out, 74), want[c]);
          check(c ? "stat 107 and charges, hard points: button casts on the current set" : "stat 107 and charges: button bound with item -1 swaps",
                dm_ss_learned_offhand(learned, dm_ss_has(out, 74)), want[c]); }
      uint64_t os[8] = {0}, si[8] = {0}, pr[8] = {0}, out[8];
      dm_ss_mark_present(pr, si, 74, 13, 3); dm_ss_given(os, si, pr, out);
      check("charges only, nothing on the swap set", dm_ss_learned_offhand(0, dm_ss_has(out, 74)), 0);
      dm_ss_scan_item(get, (void *)&cta, NEC, skill_class, os, si);
      dm_ss_given(os, si, pr, out);
      check("stat 97 and charges of the same skill: button bound with item -1 swaps", dm_ss_learned_offhand(0, dm_ss_has(out, 149)), 1); }
    check("skill id out of range", dm_ss_has((uint64_t[8]){~0ull, ~0ull, ~0ull, ~0ull, ~0ull, ~0ull, ~0ull, ~0ull}, 512), 0);
    if (fails) return 1;
    printf("t_swapskill_core: OK\n");
    return 0;
}
