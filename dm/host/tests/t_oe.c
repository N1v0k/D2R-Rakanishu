



#ifndef HLE_OE_SOURCE
#define HLE_OE_SOURCE "../hle_oe.c"
#endif
#define DM_OE_TEST_DEV_S3D 1
#include HLE_OE_SOURCE
#include <signal.h>
#include <sys/wait.h>
uint64_t now_ns(void) { return 0; }
static int t_dev_s3d; int dm_dev_s3d_pct(void) { return t_dev_s3d; }


module_t g_mod[MAX_MOD]; int g_nmod;
void dm_guest_write_prepare(void *a, size_t n) { (void)a; (void)n; }
uint64_t dm_nvn_frame_count(void) { return 0; }
int dm_nvn_ruler_truth(void) { return 0; }
int dm_ingame_now(void) { return 1; }
int mtl_fps_target(void) { return 60; }
double mtl_display_refresh(void) { return 0; }
double dm_pace_hz(double screen_hz, double floor_hz) { return screen_hz > 0 ? screen_hz : floor_hz; }
pthread_key_t g_keyres[2];
uint64_t dm_game_res(int docked) { (void)docked; return 0; }
void dm_uiscreen_res_reread(void) {}
static int last_bump, n_bump;
void dm_ui_gen_bump(int why) { last_bump = why; n_bump++; }
static int checks, fails;
#define CHECK(c, s) do { checks++; int ok = !!(c); fails += !ok; printf("%s %s\n", ok ? "✓" : "✗", s); } while (0)
static void reset(void) { g_mqh = g_mqn = 0; g_focus = 1; g_opmode = 1; g_ntf_opmode = g_ntf_perfmode = 0; g_perf_reload = 0; last_bump = n_bump = 0; }
static int pop_is(uint32_t want) { uint32_t m = 999; return h_oe_TryPopMsg(&m) && m == want; }
static int empty(void) { uint32_t m = 999; return !h_oe_TryPopMsg(&m) && m == 999; }
static int child_kind;
static void abort_exit(int sig) { (void)sig; dprintf(2, "【红】t_oe child kind=%d：10 秒看门狗超时\n", child_kind); _exit(97); }
static void *focus_burst(void *p) { (void)p; for (int i = 0; i < 17; i++) { dm_focus_set(2); dm_focus_set(1); } return NULL; }
static void *mode_burst(void *p) { (void)p; for (int i = 0; i < 17; i++) dm_set_opmode(i % 2); return NULL; }
static pthread_mutex_t stage_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t stage_cond = PTHREAD_COND_INITIALIZER;
static int stage, interleaved_ok;
static void stage_set(int n) { pthread_mutex_lock(&stage_lock); stage = n; pthread_cond_broadcast(&stage_cond); pthread_mutex_unlock(&stage_lock); }
static void stage_wait(int n) { pthread_mutex_lock(&stage_lock); while (stage < n) pthread_cond_wait(&stage_cond, &stage_lock); pthread_mutex_unlock(&stage_lock); }
static void *consume_interleaved(void *p) {
    (void)p; stage_wait(1); int ok = pop_is(15); stage_set(2);
    stage_wait(3); interleaved_ok = ok && h_oe_GetFocus() == 1 && pop_is(15) && pop_is(30) && pop_is(31) && empty() && h_oe_GetOpMode() == 0 && h_oe_GetPerfMode() == 0;
    return NULL;
}
static int child_burst(int kind) {
    fflush(stdout); pid_t p = fork(); if (p < 0) return 0;
    if (!p) {
        child_kind = kind; signal(SIGABRT, abort_exit); signal(SIGALRM, abort_exit); alarm(10); reset();
        g_ntf_opmode = g_ntf_perfmode = 1;
        if (kind == 0) { focus_burst(NULL); _exit(h_oe_GetFocus() == 1 && pop_is(15) && empty() ? 0 : 1); }
        if (kind == 1) { mode_burst(NULL); _exit(h_oe_GetOpMode() == 0 && h_oe_GetPerfMode() == 0 && pop_is(30) && pop_is(31) && empty() ? 0 : 1); }
        if (kind == 3) {
            pthread_t c; pthread_create(&c, NULL, consume_interleaved, NULL);
            dm_focus_set(2); stage_set(1); stage_wait(2);
            dm_focus_set(1); dm_set_opmode(0); stage_set(3); pthread_join(c, NULL);
            _exit(interleaved_ok ? 0 : 1);
        }
        pthread_t f, m; pthread_create(&f, NULL, focus_burst, NULL); pthread_create(&m, NULL, mode_burst, NULL);
        pthread_join(f, NULL); pthread_join(m, NULL);
        uint32_t msg; int nf = 0, no = 0, np = 0, order = 1;
        while (h_oe_TryPopMsg(&msg)) { if (msg == 15) nf++; else if (msg == 30) no++; else if (msg == 31) { order &= no == 1; np++; } else order = 0; }
        _exit(nf == 1 && no == 1 && np == 1 && order && h_oe_GetFocus() == 1 && h_oe_GetOpMode() == 0 ? 0 : 1);
    }
    int st; if (waitpid(p, &st, 0) != p) return 0;
    return WIFEXITED(st) && WEXITSTATUS(st) == 0;
}






static uint64_t s3_now; static scale3d_ctl s3; static int s3_dm;
static int s3_one(double iv) { int b = s3.level; s3_now += (uint64_t)((iv > 0 ? iv : 16.6667) * 1e6); int lv = scale3d_ctl_step(&s3, iv, s3_now); s3_dm += lv != b; return lv; }
static int s3_n(int n, double iv) { int lv = s3.level; for (int i = 0; i < n; i++) lv = s3_one(iv); return lv; }
#define F60 16.6667
#define D60 33.4
static void s3_hold_pass(void) { uint64_t end = s3.hold_until; while (s3_now < end) s3_one(F60); }


static const int s3_rows[18][2] = {{0,1},{2,4},{3,3},{0,0},{10,12},{0,0},{0,0},{0,0},{2,1},{0,0},{6,8},{0,0},{0,0},{0,0},{2,3},{5,4},{0,0},{0,0}};
static int s3_replay(int clustered) {
    s3 = (scale3d_ctl){.max_level = 3}; s3_now = 5000 * S3_SEC; s3_dm = 0;
    for (int r = 0; r < 18; r++) { int ns = s3_rows[r][0] + s3_rows[r][1], every = ns ? 600 / ns : 0;
        for (int i = 0; i < 600; i++) { int slow = clustered ? i < ns : (ns && i % every == 0 && i / every < ns); s3_one(slow ? (i % 2 ? 25.0 : 40.0) : F60); } }
    return s3_dm;
}
static void test_scale3d(void) {
    s3 = (scale3d_ctl){.max_level = 2}; s3_now = 1000 * S3_SEC;
    CHECK(s3_n(1200, F60) == 0, "动态比例：60 帧稳 20 秒 → 保持 0 档");
    CHECK(s3_n(1000, 25.0) == 0, "动态比例：25.0 ms（未超 25.0005）不算掉帧 → 不降");
    s3 = (scale3d_ctl){.max_level = 2}; s3_now = 1100 * S3_SEC;
    CHECK(s3_n(299, D60) == 0, "快降：全掉帧 4 窗多 1 帧不到（不足 5 窗）→ 还没降");
    CHECK(s3_n(1, D60) == 1, "快降：满 5 窗（10 秒）掉帧率 100% > 5% → 降到 1 档");
    s3_hold_pass();
    CHECK(s3.level == 1, "快降：停判 3 秒内的帧不计");
    CHECK(s3_n(299, D60) == 1 && s3_n(1, D60) == 2, "快降：停判后历史清空，再满 5 窗 → 降到 2 档");
    s3_hold_pass();
    CHECK(s3_n(3000, D60) == 2, "下限：已到第 2 档（max_level），再掉也不动");


    CHECK(s3_max_level(0.75f, 0.60f) == 3 && s3_level_scale(0.75f, 0) == 0.75f && s3_level_scale(0.75f, 1) == 0.70f && s3_level_scale(0.75f, 3) == 0.60f, "档位：配置 0.75 → 0.70 / 0.65 / 0.60，最深第 3 档");
    CHECK(s3_max_level(0.85f, 0.60f) == 5 && s3_level_scale(0.85f, 1) == 0.80f && s3_level_scale(0.85f, 2) == 0.75f && s3_level_scale(0.85f, 5) == 0.60f, "档位：配置 0.85 → 0.80 / 0.75 / 0.70 / 0.65 / 0.60，最深第 5 档");
    CHECK(s3_max_level(1.0f, 0.70f) == 6 && s3_level_scale(1.0f, 1) == 0.95f && s3_level_scale(1.0f, 9) == 0.55f && s3_level_scale(0.72f, 1) == 0.70f, "档位：配置 1.0 的第 1 档 0.95；超出取最低；配置 0.72 的第 1 档 0.70");


    s3 = (scale3d_ctl){.max_level = 2}; s3_now = 2000 * S3_SEC;
    for (int w = 0; w < 29; w++) { s3_n(3, D60); s3_n(114, F60); }
    CHECK(s3.level == 0, "慢降：2.56% 的窗 29 个（不到 60 秒；10 秒内 2.56% < 5%）→ 不降");
    s3_n(3, D60); s3_n(114, F60);
    CHECK(s3.level == 1, "慢降：满 30 窗（60 秒）掉帧率 2.56% > 1.5% → 降到 1 档");


    s3 = (scale3d_ctl){.max_level = 2, .level = 2}; s3_now = 3000 * S3_SEC;
    CHECK(s3_n(1799, F60) == 2, "升档：零掉帧 14 窗多 → 还不升");
    CHECK(s3_n(1, F60) == 1, "升档：连续 15 窗（30 秒）零掉帧 → 升到 1 档");
    s3 = (scale3d_ctl){.max_level = 2, .level = 2}; s3_now = 3500 * S3_SEC;
    s3_n(1500, F60); s3_one(D60); 
    CHECK(s3_n(1500, F60) == 2, "升档：中途一帧掉帧把连续计数清零 → 30 秒内不升");


    s3 = (scale3d_ctl){.max_level = 2, .level = 1}; s3_now = 4000 * S3_SEC;
    CHECK(s3_n(1800, F60) == 0, "防抖：30 秒零掉帧 → 升到 0 档");
    s3_hold_pass();
    CHECK(s3_n(300, D60) == 1, "防抖：升上去约 13 秒内满 5 窗全掉帧 → 打回 1 档");
    s3_hold_pass();
    CHECK(s3_n(120 * 43, F60) == 1, "防抖：刚升又掉，之后零掉帧 43 窗（86 秒）→ 不升（要 45 窗）");
    CHECK(s3_n(120 * 3, F60) == 0, "防抖：满 45 窗 → 升到 0 档");


    s3 = (scale3d_ctl){.max_level = 2}; s3_now = 5500 * S3_SEC; int lv = 0;
    for (int i = 0; i < 599; i++) lv = s3_one(i % 2 ? -1 : F60);
    CHECK(lv == 0, "丢弃帧：一半帧被丢弃，不足 5 窗 → 还没降");
    CHECK(s3_one(-1) == 1, "丢弃帧：满 5 窗、掉帧率 50% → 降到 1 档");
    s3 = (scale3d_ctl){.max_level = 2}; s3_now = 5800 * S3_SEC;
    CHECK(s3_n(5000, 0) == 0, "无读数（0）的帧不计 → 不动");


    s3 = (scale3d_ctl){.max_level = 2, .period_ms = 1000.0 / 30}; s3_now = 6000 * S3_SEC;
    CHECK(s3_n(600, 33.34) == 0, "30 帧档：33.34 ms 稳 20 秒 → 保持 0 档");
    CHECK(s3_n(1000, 45.0) == 0, "30 帧档：45 ms（未超 50）不算掉帧");
    s3 = (scale3d_ctl){.max_level = 2, .period_ms = 1000.0 / 30}; s3_now = 6500 * S3_SEC;
    CHECK(s3_n(149, 66.7) == 0 && s3_n(1, 66.7) == 1, "30 帧档：66.7 ms 全掉帧满 5 窗（每窗 30 帧 = 2001 ms，共 150 帧）→ 降到 1 档");


    s3 = (scale3d_ctl){.max_level = 2}; s3_now = 7000 * S3_SEC;
    for (int k = 0; k < 9; k++) { s3_n(20, D60); s3_n(580, F60); }
    CHECK(s3.level == 0, "偶发卡顿：每 10 秒连掉 20 帧、共 9 次 → 不降");


    s3 = (scale3d_ctl){.max_level = 2}; s3_now = 7500 * S3_SEC;
    for (int k = 0; k < 40; k++) { s3_n(6, D60); s3_n(6, F60); }
    CHECK(s3.level == 1, "频繁短卡顿：6 掉 6 好反复 12 秒 → 降到 1 档");


    s3 = (scale3d_ctl){.max_level = 2}; s3_now = 8000 * S3_SEC;
    CHECK(s3_n(299, D60) == 0 && s3_n(1, D60) == 1, "长段掉帧：满 5 窗（10 秒）→ 降到 1 档，时刻不变");
    int a = s3_replay(0), b = s3_replay(1);
    printf("  （实玩第二段回放：慢帧均匀散开换级 %d 次，挤在一起换级 %d 次）\n", a, b);
    CHECK(a == 0 && b == 0, "动态比例：实玩第二段的慢帧序列（帧率 59+）不论散开还是挤在一起都不换级");
}



static void test_scale3d_live(void) {
    unsetenv("DM_3D_SCALE"); unsetenv("DM_3D_SCALE_DYN"); unsetenv("DM_THERMAL_NOFALLBACK");
    CHECK(!dm_3d_scale_pinned() && dm_3d_scale_cfg() == S3_DEF_PCT / 100.0f, "3D 清晰度：没设过 = 平台缺省");
    dm_3d_scale_feed(16.7);
    unsigned r0 = atomic_load(&g_s3_rev);
    dm_3d_scale_menu_set(100);
    CHECK(dm_3d_scale_cfg() == 1.0f && atomic_load(&g_s3_rev) == r0 + 1, "3D 清晰度：选项页设 100 % → 配置值 1.00，修订号 +1");
    dm_3d_scale_menu_set(100);
    CHECK(atomic_load(&g_s3_rev) == r0 + 1, "3D 清晰度：同一个值再设不加修订号");
    atomic_store(&g_scale3d_level, 3);
    dm_3d_scale_feed(16.7);
    CHECK(atomic_load(&g_scale3d_level) == 0 && atomic_load(&g_s3_rev_ack) == atomic_load(&g_s3_rev), "3D 清晰度：出图线程见到新修订号 → 回到第 0 档并确认");
    CHECK(s3_max_level(dm_3d_scale_cfg(), S3_FLOOR_DEF) == 8, "3D 清晰度：配置 1.00 时最深一档按新值重算（0.95 … 0.60 共 8 档）");
    dm_3d_scale_menu_set(75); CHECK(dm_3d_scale_cfg() == 0.75f, "3D 清晰度：75 %");
    dm_3d_scale_menu_set(60); CHECK(dm_3d_scale_cfg() == 0.60f && s3_max_level(dm_3d_scale_cfg(), S3_FLOOR_DEF) == 0, "3D sharpness: the power-saving preset 60 % is taken and sits on the floor (no lower level)");
    dm_3d_scale_menu_set(40); CHECK(dm_3d_scale_cfg() == S3_DEF_PCT / 100.0f, "3D 清晰度：范围外的值 = 平台缺省");
    dm_3d_scale_menu_set(0);

    t_dev_s3d = 90; CHECK(dm_3d_scale_default_pct() == 90 && dm_3d_scale_cfg() == 0.90f, "3D 清晰度：设备缺省 90 %% = 缺省与配置值");
    dm_3d_scale_menu_set(80); CHECK(dm_3d_scale_cfg() == 0.80f, "3D 清晰度：选项页的值压过设备缺省"); dm_3d_scale_menu_set(0);
    t_dev_s3d = 120; CHECK(dm_3d_scale_default_pct() == S3_DEF_PCT, "3D 清晰度：设备缺省不合法 = 平台缺省");
    t_dev_s3d = 0; CHECK(dm_3d_scale_default_pct() == S3_DEF_PCT && dm_3d_scale_cfg() == S3_DEF_PCT / 100.0f, "3D 清晰度：没有设备缺省 = 平台缺省");
    dm_3d_quality_mode_set(1); atomic_store(&g_s3g_scale, 0.5f); dm_3d_scale_feed(16.7); s3g_feed(16.7);
    CHECK(atomic_load(&g_s3g_scale) == 0.0f, "画质优先：GPU 控制器的比例清零（不再覆盖配置值）");
    dm_3d_quality_mode_set(0);
}

static int child_pinned(void) {
    fflush(stdout); pid_t p = fork(); if (p < 0) return 0;
    if (!p) { setenv("DM_3D_SCALE", "0.9", 1); dm_3d_scale_menu_set(75); _exit(dm_3d_scale_pinned() && dm_3d_scale_cfg() == 0.9f ? 0 : 1); }
    int st = 0; waitpid(p, &st, 0); return WIFEXITED(st) && WEXITSTATUS(st) == 0;
}
int main(void) {
    test_scale3d();
    CHECK(child_pinned(), "3D 清晰度：DM_3D_SCALE 钉住时选项页的值不起作用");
    test_scale3d_live();
    unsetenv("DM_OE_SCRIPT"); reset();
    CHECK(empty(), "初始空队列：返回false且不改输出");
    dm_focus_set(1); dm_set_opmode(1); CHECK(empty(), "相同状态不发通知");
    dm_focus_set(3); CHECK(h_oe_GetFocus() == 3, "消费前即可查询后台状态3");
    CHECK(pop_is(15) && empty(), "后台变化通知15恰一次");
    dm_focus_set(2); dm_focus_set(1);
    CHECK(pop_is(15) && empty() && h_oe_GetFocus() == 1, "未消费的失焦/恢复合并，查询最终焦点1");
    reset(); g_ntf_opmode = g_ntf_perfmode = 1;
    dm_focus_set(2); dm_set_opmode(0); dm_focus_set(3); dm_set_opmode(1);
    CHECK(pop_is(15) && pop_is(30) && pop_is(31) && empty(), "三类首次排位FIFO=15,30,31，重复不挪位");
    CHECK(h_oe_GetFocus() == 3 && h_oe_GetOpMode() == 1 && h_oe_GetPerfMode() == 1, "重复通知不保存陈旧值，查询最新3/1/1");
    reset(); g_ntf_opmode = g_ntf_perfmode = 1;
    dm_set_opmode(0); dm_focus_set(2); dm_set_opmode(1);
    CHECK(pop_is(30) && pop_is(31) && pop_is(15) && empty(), "模式先变化时FIFO=30,31,15，成对顺序不变");
    reset(); g_ntf_opmode = g_ntf_perfmode = 1;
    dm_focus_set(2); CHECK(pop_is(15), "先弹出焦点通知");
    dm_set_opmode(0); dm_focus_set(1);
    CHECK(pop_is(30) && pop_is(31) && pop_is(15) && empty(), "弹出后的恢复再次通知并排在待取模式消息之后");
    CHECK(h_oe_GetFocus() == 1, "恢复后的当前焦点=1");
    reset(); dm_set_opmode(0);
    CHECK(empty() && h_oe_GetOpMode() == 0, "通知关闭只更新状态、不入队");
    h_oe_SetOpModeNtf(0x100); h_oe_SetPerfModeNtf(0x101); dm_set_opmode(1);
    CHECK(pop_is(31) && empty(), "通知bool只读低8位，性能单开只发31");
    h_oe_SetOpModeNtf(0x101); h_oe_SetPerfModeNtf(0x100); dm_set_opmode(0);
    CHECK(pop_is(30) && empty(), "运行模式单开只发30");
    reset(); mq_push(16); mq_push(16);
    CHECK(pop_is(16) && pop_is(16) && empty(), "逐次事件16不合并，保留两次FIFO");
    reset(); g_ntf_opmode = g_ntf_perfmode = 1; dm_set_opmode(0);
    CHECK(pop_is(30), "模式对中先消费30，31尚未消费");
    dm_set_opmode(1);
    CHECK(pop_is(31) && pop_is(30) && empty() && h_oe_GetOpMode() == 1, "消费跨新变化：旧31保留排位，已弹30重新排队，最终模式1");
    CHECK(child_burst(0), "34次焦点变化跨旧32槽，不终止且最终恢复1/一条15");
    CHECK(child_burst(1), "17次模式变化跨旧32槽，不终止且最终0/30,31");
    CHECK(child_burst(2), "两生产者并发，各类恰一条、30先于31、最终焦点1/模式0");
    reset(); g_ntf_opmode = g_ntf_perfmode = 1; int rotated = 1;
    for (int i = 0; i < 31; i++) { dm_focus_set(2 - i % 2); rotated &= pop_is(15) && empty(); }
    CHECK(rotated, "31次逐条消费，把环头推进到尾槽");
    dm_focus_set(1); dm_set_opmode(0); dm_focus_set(3); dm_set_opmode(1);
    CHECK(pop_is(15) && pop_is(30) && pop_is(31) && empty(), "跨32槽回绕后仍按15,30,31去重并保持首次顺序");
    CHECK(child_burst(3), "消费者弹出后、查询前发生新变化：新恢复不漏通知，最终焦点1/模式0");


    reset(); g_ntf_opmode = g_ntf_perfmode = 1;
    dm_oe_res_resized(); CHECK(pop_is(31) && empty() && last_bump == DM_UIGEN_RESIZE && n_bump == 1, "原地改尺寸的31：界面代号按RESIZE加一次");
    dm_oe_res_changed(); CHECK(pop_is(31) && empty() && last_bump == DM_UIGEN_PERFMODE, "比例可能变的31（界面画面 / 折叠屏）：照旧按PERFMODE");
    dm_oe_res_resized(); CHECK(pop_is(31) && last_bump == DM_UIGEN_RESIZE, "取走一条重载的31之后，新的原地改尺寸又按RESIZE");
    dm_oe_res_resized(); dm_oe_res_changed(); CHECK(pop_is(31) && empty() && last_bump == DM_UIGEN_PERFMODE, "排着的改尺寸并进一条要重载的：按PERFMODE");
    dm_oe_res_changed(); dm_oe_res_resized(); CHECK(pop_is(31) && empty() && last_bump == DM_UIGEN_PERFMODE, "顺序反过来也按PERFMODE");
    dm_oe_res_resized(); dm_set_opmode(0);
    CHECK(pop_is(31) && last_bump == DM_UIGEN_PERFMODE && pop_is(30) && last_bump == DM_UIGEN_OPMODE && empty(), "改尺寸排着时运行模式变了：31保留排位但按PERFMODE，30按OPMODE");
    n_bump = 0; dm_focus_set(2); CHECK(pop_is(15) && n_bump == 0, "焦点通知不加界面代号");
    g_ntf_perfmode = 0; dm_oe_res_changed(); g_ntf_perfmode = 1; dm_oe_res_resized();
    CHECK(pop_is(31) && empty() && last_bump == DM_UIGEN_RESIZE, "通知关着时被拒的重载31不留下标记：之后的改尺寸仍按RESIZE");
    printf("== oe %d 项，失败 %d ==\n", checks, fails); return fails != 0;
}
