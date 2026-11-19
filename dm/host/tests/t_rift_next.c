







#include "../dm_rift.c"

void (*g_dm_pre_commit_hook[DM_PRE_COMMIT_HOOKS])(const char *);
uint64_t dm_fs_commit_count(void) { return 0; }

int main(void) {
    static const struct { int tier, inc, want; } c[] = {
        { 1, 0, 1 }, { 1, 1, 2 }, { 1, 2, 3 }, { 1, 3, 4 },
        { 96, 3, 99 }, { 97, 0, 97 }, { 97, 1, 98 }, { 97, 2, 99 }, { 97, 3, 100 },
        { 98, 0, 98 }, { 98, 1, 99 }, { 98, 2, 100 }, { 98, 3, 100 },
        { 99, 0, 99 }, { 99, 1, 100 }, { 99, 2, 100 }, { 99, 3, 100 },
        { 100, 0, 100 }, { 100, 1, 100 }, { 100, 2, 100 }, { 100, 3, 100 },
    };
    int fails = 0, n = (int)(sizeof c / sizeof *c);
    atomic_store(&K.st, ST_DONE);
    for (int i = 0; i < n; i++) {
        K.tier = c[i].tier; K.inc = c[i].inc; dm_rift_hud_t h = {0}; int ok = dm_rift_hud(&h);
        if (!ok || next_tier() != c[i].want || h.next != c[i].want || h.next > 100 || h.tier != c[i].tier || h.inc != c[i].inc) {
            fails++; printf("  ✗ 第 %d 层 晋级 %d：下一层应为 %d，宿主 %d、顶部条 %d（取到 %d）\n", c[i].tier, c[i].inc, c[i].want, next_tier(), h.next, ok); }
    }






    static const struct { int base, full; long long want; } hc[] = {
        { 2147483392, 2147483392, 2147483392LL }, { 1000, 1770, 1213267453LL }, { 256, 512, 1073741696LL },
        { 6912000, 12234240, 1213267453LL }, { 100, 90, 2147483392LL }, { 0, 500, 2147483392LL },
    };
    int hn = (int)(sizeof hc / sizeof *hc);
    for (int i = 0; i < hn; i++) { long long got = hp_base_limit(hc[i].base, hc[i].full), tot = hc[i].base > 0 ? got * hc[i].full / hc[i].base : got;
        if (got != hc[i].want || (hc[i].full > hc[i].base && hc[i].base > 0 && tot > 2147483392LL)) { fails++; printf("  ✗ 基础 %d 合计 %d：可写上限应为 %lld，得 %lld（写满后合计 %lld）\n", hc[i].base, hc[i].full, hc[i].want, got, tot); } }
    printf("hp_base_limit %s：%d 格，带百分比加成时写满基础项合计不过属性上限\n", fails ? "失败" : "OK", hn);






    static const struct { int tier, s, d; } dc[] = { { 30, 510, 750 }, { 75, 510, 750 }, { 87, 637, 870 }, { 100, 775, 1000 } };
    for (unsigned i = 0; i < sizeof dc / sizeof *dc; i++) { K.tier = dc[i].tier; if (hi_duo_x1000(0) != dc[i].s || hi_duo_x1000(1) != dc[i].d) { fails++; printf("  ✗ 第 %d 层 s / d 应为 %d / %d，得 %d / %d\n", dc[i].tier, dc[i].s, dc[i].d, hi_duo_x1000(0), hi_duo_x1000(1)); } }
    K.tier = 100; if (10819454LL * hi_duo_x1000(0) / 1000 > 8388607LL) { fails++; printf("  ✗ 100 层巴尔 × s 过了属性上限\n"); }
    K.guards[0] = 7; K.guards[1] = 9; K.guards[2] = 0; if (!is_guard(7) || !is_guard(9) || is_guard(0) || is_guard(8)) { fails++; printf("  ✗ is_guard 认错\n"); }
    printf("hi_duo %s：4 格系数 + 巴尔不过上限 + 认守护者\n", fails ? "失败" : "OK");




    K.guards[0] = K.guards[1] = K.guards[2] = 0; K.inv_id[0] = 21; K.inv_id[1] = 0; g_inv_gone[0] = 0; g_inv_gone[1] = 33;
    if (!is_guard(21) || !is_guard(33) || is_guard(22) || is_guard(0)) { fails++; printf("  ✗ is_guard 没认全闯入首领（在场 21、已退场 33）\n"); }
    K.inv_id[0] = 0; g_inv_gone[1] = 0; if (is_guard(21) || is_guard(33)) { fails++; printf("  ✗ 清掉之后 is_guard 还认\n"); }


    { static const struct { uint32_t cls; int want; } bc[] = { { 561, 0 }, { 562, 1 }, { 564, 1 }, { 566, 1 }, { 567, 0 }, { 570, 1 }, { 571, 0 }, { 544, 0 }, { 45, 0 } };
      for (unsigned i = 0; i < sizeof bc / sizeof *bc; i++) if (boss_summon(bc[i].cls) != bc[i].want) { fails++; printf("  ✗ boss_summon(%u) 应为 %d\n", bc[i].cls, bc[i].want); } }
    printf("invader_guard %s：在场与已退场的闯入首领都算特殊单位\n", fails ? "失败" : "OK");


    { static const struct { int tier, nat, quota, cap; } sc[] = { { 1, 100, 0, 2 }, { 74, 100, 0, 2 }, { 75, 80, 2, 2 }, { 79, 80, 2, 2 }, { 80, 55, 5, 2 }, { 84, 55, 5, 2 }, { 85, 30, 7, 2 }, { 89, 30, 7, 2 }, { 90, 0, 1 << 30, 3 }, { 100, 0, 1 << 30, 3 } };
      for (unsigned i = 0; i < sizeof sc / sizeof *sc; i++) if (su_nat_pct(sc[i].tier) != sc[i].nat || su_quota(sc[i].tier) != sc[i].quota || su_cap(sc[i].tier) != sc[i].cap) { fails++; printf("  ✗ 第 %d 层特殊精英梯度应为 %d%% / %d 组 / 同时 %d 组\n", sc[i].tier, sc[i].nat, sc[i].quota, sc[i].cap); }


      static const uint8_t out[] = { 5, 10, 15, 18, 20, 26, 27, 28, 29, 30, 31, 39, 41, 42, 43, 44, 45, 60, 61, 62, 63, 64, 65 }; int seen[66] = { 0 }, bad = sizeof k_su_pool != 43;
      for (unsigned i = 0; i < sizeof k_su_pool; i++) { if (k_su_pool[i] > 65 || seen[k_su_pool[i]]++) bad = 1; } for (unsigned i = 0; i < sizeof out; i++) bad |= seen[out[i]]; bad |= !seen[6] || !seen[24] || !seen[36] || !seen[38];
      if (bad) { fails++; printf("  ✗ 特殊精英的池不对（应 43 行、不重样、不含剔掉的 23 行）\n"); }
      printf("su_tiers %s：10 格梯度 + 池 43 行\n", fails ? "失败" : "OK"); }
    printf("t_rift_next %s：%d 格（层 × 晋级量）下一层 = 手写期望且 ≤ 100，宿主与顶部条同值\n", fails ? "失败" : "OK", n);
    return fails != 0;
}
