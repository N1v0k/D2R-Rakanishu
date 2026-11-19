









#include "../dm_rift.c"

void (*g_dm_pre_commit_hook[DM_PRE_COMMIT_HOOKS])(const char *);
uint64_t dm_fs_commit_count(void) { return 0; }

static int fails, checks;
#define CHECK(c, ...) do { checks++; if (!(c)) { fails++; printf("  ✗ "); printf(__VA_ARGS__); printf("\n"); } } while (0)
static void nap(long ms) { struct timespec d = { ms / 1000, (ms % 1000) * 1000000L }, r; while (nanosleep(&d, &r)) d = r; }
static void left(int *hud, int *menu) { dm_rift_hud_t h = {0}; dm_rift_menu_t m = {0}; if (!dm_rift_hud(&h)) h.secs_left = -9999; menu_fill(&m, 2); *hud = h.secs_left; *menu = m.secs_left; }

int main(void) {
    unsetenv("DM_DETCLOCK");
    int h0, m0, h, m;
    K.t0_ns = guest_ns() - 120ull * 1000000000ull; atomic_store(&K.st, ST_RUN);
    left(&h0, &m0); CHECK((h0 == 780 || h0 == 779) && m0 == h0, "开局 120 秒后剩余应为 780（差 1 秒内），HUD %d 菜单 %d", h0, m0);


    guest_clock_freeze_for(1, 1); left(&h0, &m0); nap(1300); left(&h, &m);
    CHECK(h == h0 && m == m0, "后台 1.3 秒，剩余不该变：HUD %d → %d，菜单 %d → %d", h0, h, m0, m);


    guest_clock_freeze_for(2, 1); guest_clock_freeze_for(1, 0); nap(1300); left(&h, &m);
    CHECK(h == h0 && m == m0, "回前台但键盘还弹着 1.3 秒，剩余不该变：HUD %d → %d，菜单 %d → %d", h0, h, m0, m);


    guest_clock_freeze_for(2, 0); left(&h, &m); CHECK(h == h0 && m == m0, "放开那一刻不该跳：HUD %d → %d", h0, h);
    nap(2200); left(&h, &m); CHECK(h0 - h >= 2 && h0 - h <= 3 && m == h, "放开后 2.2 秒应少 2 秒（差 1 秒内）：HUD %d → %d，菜单 %d", h0, h, m);


    g_left_done = 321; atomic_store(&K.st, ST_DONE); left(&h, &m); CHECK(h == 321 && m == 321, "结算后应停在 321：HUD %d 菜单 %d", h, m);
    printf("t_rift_clock：%d 项，%d 项不通\n", checks, fails); return fails != 0;
}
