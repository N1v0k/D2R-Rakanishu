











#include "../dm_rift.c"

void (*g_dm_pre_commit_hook[DM_PRE_COMMIT_HOOKS])(const char *);
uint64_t dm_fs_commit_count(void) { return 0; }

int main(void) {
    static const struct { int st, nd_lv; unsigned next_door; int want; const char *what; } c[] = {
        { ST_IDLE, 0, 0, MENU_SELECT, "没在打" }, { ST_IDLE, 120, 5, MENU_SELECT, "没在打（位置与门号是残留）" },
        { ST_RUN, 0, 0, MENU_CHOICE, "正在打" },
        { ST_DONE, 120, 5, MENU_CHOICE, "打完，位置在，门在" }, { ST_DONE, 120, 0, MENU_CHOICE, "打完，位置在，门没开出来" },
        { ST_DONE, 0, 5, MENU_SELECT, "打完，位置没记，门号非 0" }, { ST_DONE, 0, 0, MENU_SELECT, "打完，位置没记，门也没有" },
    };
    int fails = 0, n = (int)(sizeof c / sizeof *c);
    for (int i = 0; i < n; i++) { atomic_store(&K.st, c[i].st); L.nd_lv = c[i].nd_lv; L.next_door = c[i].next_door; dm_rift_menu_t m = {0}; dm_rift_menu_peek(&m);
        if (town_menu(c[i].st) != c[i].want || m.menu != c[i].want) { fails++; printf("  ✗ %s：应给页 %d，town_menu %d、peek %d\n", c[i].what, c[i].want, town_menu(c[i].st), m.menu); } }
    atomic_store(&M.req_start, 7); atomic_store(&M.req_cont, 1); atomic_store(&M.req_abandon, 1); atomic_store(&M.menu, MENU_CHOICE); menu_reset();
    if (atomic_load(&M.req_start) || atomic_load(&M.req_cont) || atomic_load(&M.req_abandon) || atomic_load(&M.menu) != MENU_NONE) { fails++; printf("  ✗ 新局重置后仍有残留：开始 %d 继续 %d 放弃 %d 页 %d\n", atomic_load(&M.req_start), atomic_load(&M.req_cont), atomic_load(&M.req_abandon), atomic_load(&M.menu)); }


    static const struct { int st; int ago; int want; } oc[] = { { ST_RUN, 100, 0 }, { ST_RUN, 899, 0 }, { ST_RUN, 901, 1 }, { ST_RUN, 5000, 1 }, { ST_DONE, 901, 0 }, { ST_IDLE, 901, 0 } };
    for (unsigned i = 0; i < sizeof oc / sizeof *oc; i++) { atomic_store(&K.st, oc[i].st); K.t0_ns = guest_ns() - (uint64_t)oc[i].ago * 1000000000ull; dm_rift_hud_t h = {0}; dm_rift_menu_t m = {0}; dm_rift_hud(&h); menu_fill(&m, 2);
        if (rift_overtime() != oc[i].want || m.overtime != oc[i].want || (oc[i].st != ST_IDLE && h.overtime != oc[i].want)) { fails++; printf("  ✗ 状态 %d 开局 %d 秒前：超时量应为 %d，得 %d（顶部条 %d、菜单 %d）\n", oc[i].st, oc[i].ago, oc[i].want, rift_overtime(), h.overtime, m.overtime); } }
    printf("rift_overtime %s：6 格\n", fails ? "失败" : "OK");


    static const struct { int ago, guard, dead, total, want; } gc[] = { { 100, 0, 0, 0, 0 }, { 100, 1, 0, 1, 1 }, { 100, 1, 0, 2, 2 }, { 100, 1, 1, 2, 2 }, { 100, 1, 1, 3, 2 }, { 901, 0, 0, 0, 3 }, { 901, 1, 0, 1, 4 }, { 901, 1, 1, 2, 5 } };
    for (unsigned i = 0; i < sizeof gc / sizeof *gc; i++) { atomic_store(&K.st, ST_RUN); K.t0_ns = guest_ns() - (uint64_t)gc[i].ago * 1000000000ull; K.guard = gc[i].guard ? 7 : 0; atomic_store(&K.guard_dead, 0); atomic_store(&K.gdead, gc[i].dead); atomic_store(&K.gtotal, gc[i].total);
        dm_rift_hud_t h = {0}; dm_rift_hud(&h); if (h.goal != gc[i].want || h.gdead != gc[i].dead || h.gtotal != gc[i].total) { fails++; printf("  ✗ 开局 %d 秒前 守护者在场 %d（%d/%d）：第二行应为 %d，得 %d\n", gc[i].ago, gc[i].guard, gc[i].dead, gc[i].total, gc[i].want, h.goal); } }
    K.guard = 0; printf("hud_goal %s：8 格\n", fails ? "失败" : "OK");
    printf("t_rift_oldrun %s：%d 格城里入口给页 + 新局重置清请求\n", fails ? "失败" : "OK", n);
    return fails != 0;
}
