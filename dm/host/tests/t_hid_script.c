



#ifndef HLE_HID_SOURCE
#define HLE_HID_SOURCE "../hle_hid.c"
#endif
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include HLE_HID_SOURCE
module_t g_mod[MAX_MOD]; int g_nmod;
static uint64_t clock_ns = 1000000000ull;
static uint64_t guest_frame;
static uint64_t server_tick;
static int server_available;
uint64_t now_ns(void) { return clock_ns; }
uint64_t guest_ns(void) { return clock_ns; }
uint64_t dm_guest_frame(void) { return guest_frame; }
uint64_t dm_server_tick(void) { return server_available ? server_tick : UINT64_MAX; }
int touch_fill(void *out, uint64_t max) { (void)out; (void)max; return 0; }
static int test_loadscreen_up, hold_requests;
int dm_loadscreen_up(void) { return test_loadscreen_up; }
void dm_hold_for_loadscreen(void) { hold_requests++; }
int dm_opmode(void) { return 1; }
void dm_touch_set(const dm_touch *t, int n) { (void)t; (void)n; }
static void (*ui_cb)(const char *, const char *, const char *);
void uimsg_subscribe(void (*f)(const char *, const char *, const char *)) { ui_cb = f; }
static int subscribe_result;
int uimsg_try_subscribe(void (*f)(const char *, const char *, const char *)) { if (subscribe_result) return subscribe_result; ui_cb = f; return 0; }
static int checks, fails;
#define CHECK(c, s) do { checks++; int ok = !!(c); fails += !ok; printf("%s %s\n", ok ? "✓" : "✗", s); } while (0)
static void setup(const char *script) {
    memset(g_steps, 0, sizeof g_steps); memset(g_keys, 0, sizeof g_keys);
    g_nsteps = g_nkeys = g_kcur = g_ktries = g_kbeg = g_kend = 0; g_kstate = KS_PRE; g_krel = 1;
    clock_ns = 1000000000ull; guest_frame = 0; server_tick = 0; server_available = 1; setenv("DM_KEYS", script, 1); keys_parse();
}
static void setup_pregame(const char *script) {
    memset(g_steps, 0, sizeof g_steps); memset(g_keys, 0, sizeof g_keys);
    g_nsteps = g_nkeys = g_kcur = g_ktries = g_kbeg = g_kend = 0; g_kstate = KS_PRE; g_krel = 1;
    clock_ns = 1000000000ull; guest_frame = 0; server_tick = 0; server_available = 0; setenv("DM_KEYS", script, 1); keys_parse();
}
static void ui(const char *act) { ui_cb("InputMessage", act, ""); }
static NpadState read_state(void) { NpadState s; fill(&s, 0, STYLE_FULLKEY); return s; }
static int neutral(NpadState s) { return !s.buttons && !s.lx && !s.ly; }
static void release(void) { guest_frame += ACTION_HOLD_TICKS; server_tick += ACTION_HOLD_TICKS; read_state(); }
static void frames(uint64_t n) { guest_frame += n; server_tick += n; read_state(); }
int main(int argc, char **argv) {
    unsetenv("DM_KEYS"); hid_keys_init();
    setup("LSL@+0.05,LSU@+0.05");
    NpadState s = read_state(); CHECK(s.lx == -32767 && s.ly == 0 && s.buttons == 0, "首步LSL送入真手柄布局，不进按钮位");
    ui("ControllerActionBegin"); ui("ControllerActionBegin"); release();
    CHECK(neutral(read_state()), "两个Begin尚未End时，下一步仍不放行");
    ui("ControllerActionEnd"); s = read_state();
    CHECK(s.ly == 32767 && s.lx == 0, "两个重复Begin/一次End后，下一步LSU放行");

    setup("A@+0.05,B@+0.05"); CHECK(read_state().buttons == DM_BTN_A, "普通A按下");
    ui("ControllerActionBegin"); release();
    CHECK(neutral(read_state()), "单次Begin未End不放下一键");
    ui("ControllerActionEnd"); CHECK(read_state().buttons == DM_BTN_B, "End后才发B，不吞键");

    setup("A@+0.6"); CHECK(read_state().buttons == DM_BTN_A, "按住时长测试先按下A");
    clock_ns += 60000000000ull; guest_frame += 15 - 1; server_tick += 15 - 1;
    CHECK(read_state().buttons == DM_BTN_A, "墙钟跳跃但客体帧未到点时A仍按住");
    guest_frame++; server_tick++; CHECK(neutral(read_state()), "服务端拍到点后A才松开");

    setup("LSL@+0.05,LSU@+0.05"); read_state(); release();
    CHECK(read_state().ly == 32767, "局内未出现Begin：松开即空闲，不等不存在的End");

    setup("A@+0.05,B@+0.05"); read_state(); ui("ControllerActionBegin"); ui("ControllerActionEnd"); ui("ControllerActionBegin"); release();
    CHECK(neutral(read_state()), "End后又有新Begin：重新忙，旧End不替新动作放行");
    ui("ControllerActionEnd"); CHECK(read_state().buttons == DM_BTN_B, "新动作自己的End后放行");

    setup("A@+0.05,B@+0.05"); read_state(); ui("ControllerActionEnd"); ui("ControllerActionEnd"); ui("ControllerActionBegin"); release();
    CHECK(neutral(read_state()), "先到的多余End不能抵消后来Begin");
    ui("ControllerActionEnd"); CHECK(read_state().buttons == DM_BTN_B, "后来Begin结束后可继续");

    setup("A@>OpenPanel:Done,B@+0.05"); read_state(); ui("ControllerActionBegin"); ui("ControllerActionBegin");
    ui_cb("PanelManager", "OpenPanel", "Done");
    CHECK(neutral(read_state()), "按住期间确认会松开，但动作未End仍等空闲");
    ui("ControllerActionEnd"); CHECK(read_state().buttons == DM_BTN_B, "确认后的重复Begin只需一End，再放B");

    setup("A@+0.05,B@%0.2+0.05"); read_state(); ui("ControllerActionBegin"); ui("ControllerActionBegin"); release(); ui("ControllerActionEnd");
    CHECK(neutral(read_state()), "End不跳过脚本原定延时");
    frames(4); CHECK(neutral(read_state()), "服务端延时 4 拍未到 5 拍时不放下一键");
    frames(1); CHECK(read_state().buttons == DM_BTN_B, "服务端延时到 5 拍且动作结束才放下一键");
    release(); CHECK(neutral(read_state()), "末步松开后无额外按键");

    setup("A@>Never"); read_state(); release();
    frames(OK_WAIT_TICKS - ACTION_HOLD_TICKS - 1); CHECK(g_ktries == 1 && neutral(read_state()), "确认等待 150 服务端拍前不重按");
    frames(1); CHECK(g_ktries == 2 && read_state().buttons == DM_BTN_A, "确认等待满 150 服务端拍才重按");

    setup_pregame("A@%0.2"); read_state(); frames(11); CHECK(neutral(read_state()), "主菜单出图帧延时 11/12 帧不提前按键");
    frames(1); CHECK(read_state().buttons == DM_BTN_A, "主菜单出图帧延时满 12 帧才按键");

    pid_t short_hold = fork();
    if (!short_hold) { setup("A@+0.01"); _exit(read_state().buttons == 0 ? 0 : 99); }
    int short_status; waitpid(short_hold, &short_status, 0);
    CHECK(WIFEXITED(short_status) && WEXITSTATUS(short_status) == 0, "反例：按住不足 1 服务端拍，真实读数越线为未采到");

    pid_t child = fork();
    if (!child) { setenv("DM_KEYS_SOFT", "0", 1); setup("A@>Never"); read_state(); release(); clock_ns += 60000000000ull; read_state(); _exit(99); }
    int status; waitpid(child, &status, 0);
    CHECK(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT, "客体帧不动但墙钟 60 秒只走兜底报红");

    setup("A@Start>OpenPanel:Done,B@>OpenPanel:Next");
    ui_cb("PanelManager", "OpenPanel", "Done"); ui_cb("PanelManager", "OpenPanel", "Start");
    CHECK(read_state().buttons == DM_BTN_A && g_kcur == 0, "按下前的旧确认不完成当前步");
    ui("ControllerActionBegin"); ui("ControllerActionBegin"); ui_cb("PanelManager", "OpenPanel", "Done");
    ui_cb("PanelManager", "OpenPanel", "Next");
    CHECK(neutral(read_state()), "下一步尚未按下时的旧确认不能跳步，仍等前一动作End");
    ui("ControllerActionEnd");
    CHECK(read_state().buttons == DM_BTN_B && g_kcur == 1, "前一End后下一步真按B，旧Next确认不算完成");
    ui_cb("PanelManager", "OpenPanel", "Next");
    CHECK(neutral(read_state()) && g_kcur == 2, "下一步按下后的新鲜确认才完成并松手");
    if (argc > 1) {
        FILE *f = fopen(argv[1], "r"); if (!f) { perror("trace"); return 2; }
        setup("LSL@+8,LSU@+8"); read_state(); char line[2048]; int entered = 0, nb = 0, ne = 0;
        while (fgets(line, sizeof line, f)) {
            if (strstr(line, "hid 脚本步 3 按下")) entered = 1;
            if (!entered) continue;
            double t;
            if (sscanf(line, "[dm] UIMSG [%lfs]", &t) != 1) continue;
            int beg = strstr(line, "InputMessage ControllerActionBegin") != NULL;
            int end = strstr(line, "InputMessage ControllerActionEnd") != NULL;
            if (!beg && !end) continue;
            clock_ns = 1000000000ull + (uint64_t)((t - 16.13) * 1e9); read_state();
            ui(beg ? "ControllerActionBegin" : "ControllerActionEnd"); nb += beg; ne += end;
            if (end) break;
        }
        fclose(f); s = read_state();
        CHECK(nb == 73 && ne == 1 && s.ly == 32767, "iOS原始步3轨迹73 Begin/1 End回放后进入LSU（假时钟按消息时间推进）");
    }


    setup("A@OpenPanel:PauseLayout%1>PausePanelMessage:ExitGame"); server_tick = 401; guest_frame = 1000;
    ui_cb("PanelManager", "OpenPanel", "PauseLayout"); CHECK(neutral(read_state()), "冻结前见到前置，延时未到不按");
    server_available = 0; read_state();
    guest_frame += 59; CHECK(neutral(read_state()), "冻结后按帧计：59 帧未到 1 秒不按");
    guest_frame += 1; CHECK(read_state().buttons == DM_BTN_A, "冻结后按帧计：满 60 帧（1 秒）按下A，不因拍号停住死等");


    setup("A@DM:Tick:84#>OpenPanel:NpcDialogPanel"); server_tick = 84; test_loadscreen_up = 1; hold_requests = 0;
    ui_cb("DM", "Tick", "84#"); CHECK(neutral(read_state()) && hold_requests == 1, "读取画面开着：前置到了也不按 A，并请求冻结拍");
    frames(30); CHECK(neutral(read_state()) && hold_requests >= 2, "读取画面仍开着：继续不按");
    test_loadscreen_up = 0; hold_requests = 0; CHECK(read_state().buttons == DM_BTN_A && hold_requests == 0, "读取画面撤下：按下 A，不再请求冻结");
    setup("A@DM:Tick:84#>OpenPanel:NpcDialogPanel"); server_tick = 84; test_loadscreen_up = 0; hold_requests = 0;
    ui_cb("DM", "Tick", "84#"); CHECK(read_state().buttons == DM_BTN_A && hold_requests == 0, "读取画面本来就撤了：到点即按，不请求冻结（时序不变）");


    pid_t hard = fork();
    if (!hard) { setenv("DM_KEYS_SOFT", "0", 1); setup("A@1,Q@2"); _exit(99); }
    int hard_status; waitpid(hard, &hard_status, 0);
    CHECK(WIFSIGNALED(hard_status) && WTERMSIG(hard_status) == SIGABRT, "硬：未知键名照旧终止");
    setenv("DM_KEYS_SOFT", "1", 1);
    setup("A@1,Q@2"); CHECK(g_nkeys == 0 && g_nsteps == 0 && g_kdead, "软：未知键名 → 整段作废，前面已解析的时刻键也清掉");
    clock_ns += 1500000000ull; CHECK(neutral(read_state()), "软：作废后到了原定时刻也不出键");
    setup("A@OpenPanel:X%abc"); CHECK(g_nsteps == 0 && g_kdead, "软：延时不是数 → 作废");
    setup("A@OpenPanel:X+1x"); CHECK(g_nsteps == 0 && g_kdead, "软：按住秒数带垃圾 → 作废");
    setup("A@1x"); CHECK(g_nkeys == 0 && g_kdead, "软：时刻后面带垃圾 → 作废");
    setup("A@1e999"); CHECK(g_nkeys == 0 && g_kdead, "软：时刻溢出 → 作废");
    setup("A@1+-2"); CHECK(g_nkeys == 0 && g_kdead, "软：负的按住秒数 → 作废");
    setup("T1.5:0.5@OpenPanel:X"); CHECK(g_nsteps == 0 && g_kdead, "软：触点坐标超出 0–1 → 作废");
    setup("C0.5:0.5x@OpenPanel:X"); CHECK(g_nsteps == 0 && g_kdead, "软：点击坐标带垃圾 → 作废");
    setup("A@1+0.5,B@OpenPanel:X%0.2+0.35>OpenPanel:Y,T0.5:0.25@OpenPanel:Y"); CHECK(g_nkeys == 1 && g_nsteps == 2 && !g_kdead, "软：合法脚本照常解析（时刻、延时、按住、触点）");
    { static char big[5000]; memset(big, 'A', sizeof big - 1); big[sizeof big - 1] = 0; big[1] = '@'; big[2] = '1';
      memset(g_steps, 0, sizeof g_steps); g_nsteps = g_nkeys = g_kcur = 0; setenv("DM_KEYS", big, 1); keys_parse(); CHECK(g_nkeys == 0 && g_nsteps == 0 && g_kdead, "软：超过 4095 字节 → 作废，不截断"); }
    setenv("DM_TEST_CURSOR", "OpenPanel:Y:5", 1); setup("A@>Never"); CHECK(g_nsteps == 0 && g_kdead, "软：DM_TEST_CURSOR 格式错 → 整段作废"); unsetenv("DM_TEST_CURSOR");
    subscribe_result = -2; setup("A@>Never"); CHECK(g_nsteps == 0 && g_kdead && neutral(read_state()), "软：订阅 UI 消息失败 → 作废，不终止"); subscribe_result = 0;
    setenv("DM_TEST_CURSOR", "OpenPanel:Y:5,5", 1);
    setup("A@>Never,B@0.5+100"); CHECK(read_state().buttons == DM_BTN_A && !g_kdead, "软：按下 A，等确认");
    release(); clock_ns += 60000000000ull; s = read_state();
    CHECK(g_kdead && g_kcur == g_nsteps && g_cursor_cur == g_ncursor_tests, "软：墙钟兜底超时 → 中止全部脚本（步与 DM_TEST_CURSOR），不终止");
    CHECK(neutral(s), "软：中止后按时刻的 B 到点了也不出");
    unsetenv("DM_TEST_CURSOR");
    setup("A@>Never"); read_state();
    for (int i = 0; i < 4 && !g_kdead; i++) { release(); frames(OK_WAIT_TICKS); }
    CHECK(g_kdead && g_ktries == 3 && neutral(read_state()), "软：按 3 次没等到确认 → 中止，不终止、不再按");
    ui_cb("PanelManager", "OpenPanel", "Never"); CHECK(neutral(read_state()) && g_kcur == g_nsteps, "软：中止后迟到的确认不再推进脚本");
    unsetenv("DM_KEYS_SOFT");
    printf("== HID脚本 %d 项，失败 %d ==\n", checks, fails); return fails != 0;
}
