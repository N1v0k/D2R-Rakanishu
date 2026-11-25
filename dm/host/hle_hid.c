



#include "hle.h"
#include "dm_prof.h"
#include "dm_uiscreen.h"
#include "dm_actions.h"
#include "dm_cursor_item.h"
#include <unistd.h>
#include "dm_input.h"
#include "front.h"
#include <stdatomic.h>
#include "dm_cursor.h"
#include <math.h>
#include <errno.h>
_Atomic int g_dm_l_presses;

enum { NPAD_HANDHELD = 0x20 };
enum { STYLE_FULLKEY = 1u << 0, STYLE_HANDHELD = 1u << 1 };
enum { ATTR_CONNECTED = 1u << 0 };



typedef struct {
    int64_t sampling;
    uint64_t buttons;
    int32_t lx, ly, rx, ry;
    uint32_t attributes;
    uint32_t _pad;
} NpadState;
_Static_assert(sizeof(NpadState) == 0x28, "NpadState");

static struct { _Atomic uint64_t buttons; _Atomic int32_t lx, ly, rx, ry; } g_in;
static _Atomic int64_t g_sampling;
static _Atomic uint64_t g_sampled_right;
void dm_input_sampled_right(int32_t *rx,int32_t *ry) {
    uint64_t v=atomic_load_explicit(&g_sampled_right,memory_order_acquire);
    *rx=(int32_t)(uint32_t)v;*ry=(int32_t)(uint32_t)(v>>32);
}





static int g_stall_ms = -1; static _Atomic int g_stall_due;






static _Atomic uint64_t g_press_latch;



static _Atomic uint64_t g_pad_btn, g_pad_axes, g_pad_latch;
void dm_pad_set(uint64_t buttons, int32_t lx, int32_t ly, int32_t rx, int32_t ry) {
    uint64_t old = atomic_exchange(&g_pad_btn, buttons); atomic_fetch_or(&g_pad_latch, buttons & ~old);
    atomic_store(&g_pad_axes, (uint64_t)(uint16_t)lx | (uint64_t)(uint16_t)ly << 16 | (uint64_t)(uint16_t)rx << 32 | (uint64_t)(uint16_t)ry << 48);
}
void dm_pad_unlatch(uint64_t mask) { atomic_fetch_and(&g_pad_latch, ~mask); }

static _Atomic uint64_t g_plus_on, g_plus_off;
void dm_input_plus_reads(uint64_t *on, uint64_t *off) { *off = atomic_load(&g_plus_off); *on = atomic_load(&g_plus_on); }
void dm_input_set(uint64_t buttons, int32_t lx, int32_t ly, int32_t rx, int32_t ry) {
    if (g_stall_ms < 0) { const char *e = getenv("DM_TEST_HID_STALL_MS"); g_stall_ms = e ? atoi(e) : 0; if (g_stall_ms) LOG("DM_TEST_HID_STALL_MS：A 每次按下沿后游戏取手柄先睡 %d ms", g_stall_ms); }
    if (g_stall_ms > 0 && (buttons & DM_BTN_A) && !(g_in.buttons & DM_BTN_A)) atomic_store(&g_stall_due, 1);
    atomic_fetch_or(&g_press_latch, buttons & ~g_in.buttons);
    g_in.buttons = buttons; g_in.lx = lx; g_in.ly = ly; g_in.rx = rx; g_in.ry = ry;
}




static void ui_taps_step(void) {
    static struct { uint64_t fr; float x, y, x2, y2; int st, kind, arg, shift; } tp[128]; static int n = -1;
    if (n < 0) { n = 0; const char *e = getenv("DM_UI_TAPS"); char buf[4096]; if (e) { strlcpy(buf, e, sizeof buf); for (char *t = strtok(buf, ";"); t && n < 128; t = strtok(NULL, ";")) {
        unsigned long long f; int x, y, c = 0; if (sscanf(t, "%llu:%d,%d%n", &f, &x, &y, &c) < 3) continue;
        tp[n].fr = f; tp[n].x = x / 1000.0f; tp[n].y = y / 1000.0f; tp[n].kind = 0; const char *a = t + c;
        if (*a == ':') { a++; if (*a == 's' || *a == 'c') { tp[n].shift = *a == 's' ? 0x10 : 0x11; a++; } tp[n].kind = *a; int p = 0, q = 0;
            if ((*a == 'd' || *a == 'g') && sscanf(a + 1, "%d,%d", &p, &q) == 2) { tp[n].x2 = p / 1000.0f; tp[n].y2 = q / 1000.0f; } else tp[n].arg = atoi(a + 1); }
        n++; } LOG("DM_UI_TAPS：%d 项", n); } }
    uint64_t fr = dm_guest_frame();
    static int rel = -1; if (rel < 0) rel = getenv("DM_UI_TAPS_AT") != NULL;
    if (rel) { uint64_t b = dm_ui_taps_base(); if (!b || fr < b) return; fr -= b; }
    for (int i = 0; i < n; i++) { if (fr < tp[i].fr || tp[i].st < 0) continue; int d = (int)(fr - tp[i].fr), k = tp[i].kind; float x = tp[i].x, y = k == 'W' ? tp[i].y : dm_ui_ny(tp[i].y);
        if (!k || k == 't') { int hold = k ? tp[i].arg : 6; if (tp[i].st == 0) { tp[i].st = 1; dm_ui_touch(0, x, y); } else if (tp[i].st == 1 && d >= hold) { tp[i].st = -1; dm_ui_touch(2, x, y); } continue; }
        if (k == 'N') { void dm_num_set(int); dm_num_set(tp[i].arg); tp[i].st = -1; continue; }
        if (k == 'A') { void dm_shop_buy(void), dm_shop_sell_cursor(void), dm_stat_add(int, int), dm_skill_add_selected(void); int v = tp[i].arg;
            if (v == 1) dm_shop_buy(); else if (v == 2) dm_shop_sell_cursor(); else if (v == 3) dm_skill_add_selected(); else if (v == 4) { void dm_test_hold_first_sellable(void); dm_test_hold_first_sellable(); } else if (v == 6) { void dm_test_sell_first(void); dm_test_sell_first(); } else if (v == 5) { void dm_test_belt_first_potion(void); dm_test_belt_first_potion(); } else if (v == 35 || v == 38 || v == 39) { void dm_inv_sort(int); dm_inv_sort(v == 35 ? 0 : v == 38 ? 3 : 4); } else if (v == 36) { void dm_name_random(void); dm_name_random(); } else if (v >= 44 && v <= 46) { void dm_test_identify(int); dm_test_identify(v - 43); } else if (v == 74) { void dm_held_stow(void); dm_held_stow(); } else if (v == 75) { void dm_test_swap_held(void); dm_test_swap_held(); } else if (v >= 70 && v <= 73) { dm_uiscreen_enable(v - 70); }                                                                                                                                                                                                                     else if (v == 47 || v == 48) { dm_stash_take(v == 47 ? 1 : 3); } else if (v == 43) { dm_cursor_item_info ci; int hw = 0, hh = 0, c = dm_cursor_item(&ci), hd = dm_held_size(&hw, &hh); LOG("测试：光标物品 %s 容器 %d 格 %d,%d 占 %dx%d 叠数 %d；手上 %d（%dx%d）；等目标 %d", c ? ci.code : "无", c ? ci.container : 0, c ? ci.gx : 0, c ? ci.gy : 0, c ? ci.gw : 0, c ? ci.gh : 0, c ? ci.stack : 0, hd, hw, hh, dm_item_targeting()); } else if (v >= 50 && v <= 69) { void dm_test_stash_take(int); dm_test_stash_take(v - 50 ? v - 50 : 1); } else if (v >= 40 && v <= 42) { void dm_test_equip(int); dm_test_equip(v - 39); } else if (v == 33 || v == 34) { void dm_belt_expand(int); dm_belt_expand(v == 33); } else if (v == 31 || v == 32) { void dm_item_compare(int); dm_item_compare(v == 31); } else if (v == 30) { void dm_test_hover_at(float, float); dm_test_hover_at(x, y); } else if (v >= 21 && v <= 26) { void dm_test_use_first(int); dm_test_use_first(v - 20); } else if (v >= 1000) { void dm_belt_put(uint32_t); dm_belt_put((uint32_t)(v - 1000)); } else if (v >= 100) dm_stat_add(v / 10 % 10, v % 10 == 9 ? -1 : v % 10); tp[i].st = -1; continue; }
        if (k == 'W') { unsigned dm_world_tap(float, float); dm_world_tap(x, y); tp[i].st = -1; continue; }
        if (k == 'g') { float t = d / 60.0f; if (t > 1) t = 1;
            if (tp[i].st == 0) { tp[i].st = 1; int hz = fabsf(tp[i].x2 - x) * 16 > fabsf(tp[i].y2 - y) * 9; dm_ui_drag_begin(hz ? 2 : 1, x, y, tp[i].arg ? tp[i].arg / 1000.0f : 0.05f); }
            dm_ui_drag_move(x + (tp[i].x2 - x) * t, y + (tp[i].y2 - y) * t); if (d >= 62) { dm_ui_drag_end(); tp[i].st = -1; } continue; }
        if (k == 'k') { static unsigned ks[128]; if (tp[i].st == 0) { tp[i].st = 1; ks[i] = dm_ui_item_query(x, y); } else if (d >= 3) { tp[i].st = -1; if (dm_ui_item_result(ks[i]) == 2) dm_ui_tab_go(); } continue; }
        if (tp[i].st == 0) { tp[i].st = 1; dm_ui_item_query(x, y); dm_ui_ptr(DM_UP_HOVER, x, y, 0); if (tp[i].shift) dm_ui_ptr(DM_UP_KEYDOWN, x, y, tp[i].shift); continue; }
        int done = 0;
        if (k == 'h') done = d >= tp[i].arg;
        else if (k == 'r' || k == 'l') { if (tp[i].st == 1 && d >= 4) { tp[i].st = 2; dm_ui_ptr(k == 'r' ? DM_UP_RDOWN : DM_UP_LDOWN, x, y, 0); dm_ui_ptr(k == 'r' ? DM_UP_RUP : DM_UP_LUP, x, y, 0); } done = d >= 10; }
        else if (k == 'p') { if (tp[i].st == 1 && d >= 4) { tp[i].st = 2; dm_ui_ptr(DM_UP_LDOWN, x, y, 0); } else if (tp[i].st == 2 && d >= 4 + tp[i].arg) { tp[i].st = 3; dm_ui_ptr(DM_UP_LUP, x, y, 0); } done = d >= 10 + tp[i].arg; }
        else if (k == 'm') { if (tp[i].st == 1) { tp[i].st = 2; dm_ui_map_full(tp[i].arg); } done = d >= 4; }
        else if (k == 'w') { if (tp[i].st == 1 && d >= 4) { tp[i].st = 2; dm_ui_ptr(DM_UP_SCROLL, x, y, tp[i].arg); } done = d >= 8; }
        else if (k == 'd') { if (tp[i].st == 1 && d >= 4) { tp[i].st = 2; dm_ui_ptr(DM_UP_LDOWN, x, y, 0); }
            else if (tp[i].st == 2 && d >= 6) { float t = (d - 6) / 20.0f; if (t > 1) t = 1; dm_ui_ptr(DM_UP_MOVE, x + (tp[i].x2 - x) * t, y + (tp[i].y2 - y) * t, 0); if (d >= 30) { tp[i].st = 3; dm_ui_ptr(DM_UP_LUP, 0, 0, 0); } }
            done = d >= 36; }
        else done = 1;
        if (done) { tp[i].st = -1; if (tp[i].shift) dm_ui_ptr(DM_UP_KEYUP, x, y, tp[i].shift); dm_ui_ptr(DM_UP_END, x, y, 0); } }
}
static _Atomic uint64_t g_stick_ns;
uint64_t dm_stick_last_ns(void) { return atomic_load(&g_stick_ns); }
uint64_t dm_input_samples(void) { return (uint64_t)atomic_load(&g_sampling); }

static uint32_t connected_id(void) { return dm_opmode() == 0 ? NPAD_HANDHELD : 0; }
static uint32_t style_of(uint32_t id) {
    if (id != connected_id()) return 0;
    return id == NPAD_HANDHELD ? STYLE_HANDHELD : STYLE_FULLKEY;
}


























#ifndef DM_PLAYER_BUILD
#define K(n) {DM_BTN_##n, #n}
static const struct { uint32_t mask; const char *name; } k_names[] = {
    K(A), K(B), K(X), K(Y), K(LS), K(RS), K(L), K(R), K(ZL), K(ZR), K(PLUS), K(MINUS), K(LEFT), K(UP), K(RIGHT), K(DOWN)};
#undef K
#endif


enum { PK_LSL = 1u << 24, PK_LSR = 1u << 25, PK_LSU = 1u << 26, PK_LSD = 1u << 27, PK_MASK = 0xFu << 24, PK_TOUCH = 1u << 28, PK_MSG = 1u << 29, PK_CLICK = 1u << 30 };
#ifndef DM_PLAYER_BUILD
static const struct { uint32_t mask; const char *name; } k_stick[] = {{PK_LSL, "LSL"}, {PK_LSR, "LSR"}, {PK_LSU, "LSU"}, {PK_LSD, "LSD"}};
#define KMAX 256
static struct { uint32_t mask; uint64_t t0, t1; } g_keys[KMAX];
static int g_nkeys;
static uint64_t g_kbase;


enum { KS_PRE, KS_HOLD, KS_OK, KS_DONE };
static struct { uint32_t mask; char pre[96], ok[96]; int seen_pre, seen_server, hold_override; double delay_seconds, hold_seconds; uint64_t seen_clock, seen_at; float tx, ty; char msg[96]; } g_steps[KMAX];
static int g_nsteps, g_kcur, g_kstate, g_ktries, g_krel = 1, g_kbeg, g_kend;
static uint64_t g_kpress, g_kpress_clock; static int g_kpress_server;
static pthread_mutex_t g_klock = PTHREAD_MUTEX_INITIALIZER;
static struct { char msg[160]; int32_t x, y; } g_cursor_tests[KMAX];
static int g_ncursor_tests, g_cursor_cur;








static int g_ksoft;
static _Atomic int g_kdead;
static int keys_soft_default(void) {
#ifdef __ANDROID__
    return 1;
#else
    return 0;
#endif
}
#define KEYS_BAD(...) do { if (!g_ksoft) die(__VA_ARGS__); LOG("【红】hid 脚本作废：" __VA_ARGS__); return -1; } while (0)


static int keys_num(const char *s, const char *end, double lo, double hi, double *out) {
    char *e; errno = 0; double v = strtod(s, &e);
    if (e == s || (end ? e != end : *e != 0) || errno == ERANGE || !isfinite(v) || v < lo || v > hi) return -1;
    *out = v; return 0;
}
#define SERVER_TICKS_PER_SECOND 25ull
#define PRODUCT_FRAMES_PER_SECOND 60ull
#define ACTION_HOLD_TICKS 2ull
#define DIRECTION_HOLD_TICKS 3ull
#define OK_WAIT_TICKS 150ull
#define WALL_FALLBACK_NS 60000000000ull

typedef struct { uint64_t value; int server; } script_clock;
static script_clock script_clock_now(void) {
    uint64_t tick = dm_server_tick(); { extern _Atomic int g_dm_frozen; if (atomic_load(&g_dm_frozen)) tick = UINT64_MAX; }
    return tick == UINT64_MAX ? (script_clock){dm_guest_frame(), 0} : (script_clock){tick, 1};
}
static uint64_t script_clock_elapsed(script_clock now, uint64_t start, int start_server) {
    return now.server == start_server && now.value >= start ? now.value - start : 0;
}
static uint64_t script_seconds_to_units(double seconds, int server) {
    return (uint64_t)(seconds * (server ? SERVER_TICKS_PER_SECOND : PRODUCT_FRAMES_PER_SECOND));
}
static uint64_t step_hold_units(const typeof(g_steps[0]) *st, int server) {
    if (st->hold_override) return script_seconds_to_units(st->hold_seconds, server);
    return (st->mask & (DM_BTN_UP | DM_BTN_DOWN | DM_BTN_LEFT | DM_BTN_RIGHT | PK_MASK)) ? DIRECTION_HOLD_TICKS : ACTION_HOLD_TICKS;
}

static int cursor_tests_parse(void) {
    g_ncursor_tests = g_cursor_cur = 0;
    const char *e = getenv("DM_TEST_CURSOR"); if (!e || !*e) return 0;
    char buf[4096]; if (g_ksoft && strlen(e) >= sizeof buf) KEYS_BAD("DM_TEST_CURSOR 超过 %d 字节", (int)sizeof buf - 1);
    strncpy(buf, e, sizeof buf - 1); buf[sizeof buf - 1] = 0;
    for (char *save, *tok = strtok_r(buf, ";", &save); tok; tok = strtok_r(NULL, ";", &save)) {
        char *colon = strrchr(tok, ':'); int x, y; char extra;
        if (!colon || sscanf(colon + 1, "%d,%d%c", &x, &y, &extra) != 2 || g_ncursor_tests == KMAX)
            KEYS_BAD("DM_TEST_CURSOR 项「%s」格式：UI消息触发:x,y", tok);
        *colon = 0;
        if (!*tok) KEYS_BAD("DM_TEST_CURSOR 项触发串为空");
        strncpy(g_cursor_tests[g_ncursor_tests].msg, tok, sizeof g_cursor_tests[0].msg - 1);
        g_cursor_tests[g_ncursor_tests].msg[sizeof g_cursor_tests[0].msg - 1] = 0;
        g_cursor_tests[g_ncursor_tests].x = (int32_t)x;
        g_cursor_tests[g_ncursor_tests].y = (int32_t)y;
        g_ncursor_tests++;
    }
    return 0;
}

static void step_chain(void) {
    if (g_kcur < g_nsteps && !g_steps[g_kcur].pre[0] && !g_steps[g_kcur].seen_pre) { script_clock c = script_clock_now(); g_steps[g_kcur].seen_pre = 1; g_steps[g_kcur].seen_at = now_ns(); g_steps[g_kcur].seen_clock = c.value; g_steps[g_kcur].seen_server = c.server; }
}



static void keys_kill_locked(void) {
    if (g_kcur < g_nsteps && g_kstate == KS_HOLD) {
        if (g_steps[g_kcur].mask & PK_TOUCH) dm_touch_set(NULL, 0);
        if (g_steps[g_kcur].mask & PK_CLICK) dm_ui_touch(2, g_steps[g_kcur].tx, g_steps[g_kcur].ty);
    }
    int at = g_kcur + 1, left = g_nsteps - g_kcur - 1;
    g_krel = 1; g_kstate = KS_PRE; g_kcur = g_nsteps; g_cursor_cur = g_ncursor_tests; atomic_store(&g_kdead, 1);
    LOG("【红】hid 脚本中止：停在第 %d 步，后面 %d 步与按时刻的键不再执行；游戏照常（DM_KEYS_SOFT）", at, left < 0 ? 0 : left);
}
static void keys_on_uimsg(const char *cat, const char *act, const char *arg) {
    char m[256]; snprintf(m, sizeof m, "%s:%s:%s", cat, act, arg);
    pthread_mutex_lock(&g_klock);
    if (g_cursor_cur < g_ncursor_tests && strstr(m, g_cursor_tests[g_cursor_cur].msg)) {
        int i = g_cursor_cur++;
        dm_cursor_set(g_cursor_tests[i].x, g_cursor_tests[i].y);
    }




    if (strstr(m, "InputMessage:ControllerActionBegin") && !g_krel) { g_kbeg = 1; g_kend = 0; }
    if (strstr(m, "InputMessage:ControllerActionEnd")) g_kend = 1;
    if (g_kcur < g_nsteps) {
        if (g_kstate != KS_PRE && g_steps[g_kcur].ok[0] && strstr(m, g_steps[g_kcur].ok)) {
            LOG("hid 脚本步 %d 确认「%s」（第 %d 次按键，等待时钟单位 %llu，墙钟 %llu ms）", g_kcur + 1, m, g_ktries,
                (unsigned long long)(script_clock_now().value - g_kpress_clock), (unsigned long long)((now_ns() - g_kpress) / 1000000ull));
            if (g_kstate == KS_HOLD && (g_steps[g_kcur].mask & PK_TOUCH)) dm_touch_set(NULL, 0);
            if (g_kstate == KS_HOLD && (g_steps[g_kcur].mask & PK_CLICK)) dm_ui_touch(2, g_steps[g_kcur].tx, g_steps[g_kcur].ty);
            g_krel = 1;
            g_kcur++; g_kstate = KS_PRE; g_ktries = 0; step_chain();
        }


        int j = g_kstate == KS_PRE ? g_kcur : g_kcur + 1;
        if (j < g_nsteps && !g_steps[j].seen_pre && g_steps[j].pre[0] && strstr(m, g_steps[j].pre)) { script_clock c = script_clock_now(); g_steps[j].seen_pre = 1; g_steps[j].seen_at = now_ns(); g_steps[j].seen_clock = c.value; g_steps[j].seen_server = c.server; }
    }
    pthread_mutex_unlock(&g_klock);
}
static uint32_t steps_now(void) {
    if (!g_nsteps) return 0;
    uint64_t t = now_ns(); uint32_t m = 0;
    pthread_mutex_lock(&g_klock);
    if (g_kcur < g_nsteps) {
        typeof(g_steps[0]) *st = &g_steps[g_kcur];
        script_clock c = script_clock_now();


        if (st->seen_pre && c.server != st->seen_server) { st->seen_clock = c.value; st->seen_server = c.server; }
        if (g_kstate != KS_PRE && c.server != g_kpress_server) { g_kpress_clock = c.value; g_kpress_server = c.server; }
        uint64_t elapsed = script_clock_elapsed(c, st->seen_clock, st->seen_server);
        int dm_loadscreen_up(void); void dm_hold_for_loadscreen(void);
        if (g_kstate == KS_PRE && st->seen_pre && elapsed >= script_seconds_to_units(st->delay_seconds, c.server) && dm_loadscreen_up()) dm_hold_for_loadscreen();
        else if (g_kstate == KS_PRE && st->seen_pre && g_krel && (g_kend >= g_kbeg || (st->mask & (PK_MSG | PK_CLICK))) && elapsed >= script_seconds_to_units(st->delay_seconds, c.server)) {
            g_kstate = KS_HOLD; g_kpress = t; g_kpress_clock = c.value; g_kpress_server = c.server; g_krel = 0; g_kbeg = g_kend = 0; g_ktries = 1;
            if (st->mask & PK_TOUCH) { dm_touch p = {0, st->tx, st->ty}; dm_touch_set(&p, 1); }
            if (st->mask & PK_MSG) dm_ui_send_queue(st->msg);
            if (st->mask & PK_CLICK) dm_ui_touch(0, st->tx, st->ty);
            LOG("hid 脚本步 %d 按下 mask=%#x（前置「%s」）", g_kcur + 1, st->mask, st->pre);
        } else if (g_kstate == KS_OK && t - g_kpress >= WALL_FALLBACK_NS) {
            if (!g_ksoft) die("【红】hid 脚本步 %d：墙钟兜底超时（等待服务端拍 %llu，墙钟 %llu ms，确认「%s」）", g_kcur + 1,
                (unsigned long long)script_clock_elapsed(c, g_kpress_clock, g_kpress_server), (unsigned long long)((t - g_kpress) / 1000000ull), st->ok);
            LOG("【红】hid 脚本步 %d：墙钟兜底超时（等待服务端拍 %llu，墙钟 %llu ms，确认「%s」）", g_kcur + 1,
                (unsigned long long)script_clock_elapsed(c, g_kpress_clock, g_kpress_server), (unsigned long long)((t - g_kpress) / 1000000ull), st->ok);
            keys_kill_locked();
        } else if (g_kstate == KS_OK && script_clock_elapsed(c, g_kpress_clock, g_kpress_server) >= OK_WAIT_TICKS && g_ktries >= 3) {
            if (!g_ksoft) die("【红】hid 脚本步 %d：按 3 次都没等到确认「%s」（等待服务端拍 %llu，墙钟 %llu ms）", g_kcur + 1, st->ok,
                (unsigned long long)script_clock_elapsed(c, g_kpress_clock, g_kpress_server), (unsigned long long)((t - g_kpress) / 1000000ull));
            LOG("【红】hid 脚本步 %d：按 3 次都没等到确认「%s」（等待服务端拍 %llu，墙钟 %llu ms）", g_kcur + 1, st->ok,
                (unsigned long long)script_clock_elapsed(c, g_kpress_clock, g_kpress_server), (unsigned long long)((t - g_kpress) / 1000000ull));
            keys_kill_locked();
        } else if (g_kstate == KS_OK && script_clock_elapsed(c, g_kpress_clock, g_kpress_server) >= OK_WAIT_TICKS) {
            LOG("hid 脚本步 %d 等待确认：服务端拍 %llu，墙钟 %llu ms；重按（第 %d 次）", g_kcur + 1,
                (unsigned long long)script_clock_elapsed(c, g_kpress_clock, g_kpress_server), (unsigned long long)((t - g_kpress) / 1000000ull), g_ktries + 1);
            g_kstate = KS_HOLD; g_kpress = t; g_kpress_clock = c.value; g_kpress_server = c.server; g_krel = 0; g_kbeg = g_kend = 0; g_ktries++;
            if (st->mask & PK_TOUCH) { dm_touch p = {0, st->tx, st->ty}; dm_touch_set(&p, 1); }
            if (st->mask & PK_MSG) dm_ui_send_queue(st->msg);
            if (st->mask & PK_CLICK) dm_ui_touch(0, st->tx, st->ty);
            LOG("hid 脚本步 %d 重按（第 %d 次）", g_kcur + 1, g_ktries);
        }
        if (g_kstate == KS_HOLD) {
            uint64_t held = script_clock_elapsed(c, g_kpress_clock, g_kpress_server);
            if (held < step_hold_units(st, c.server)) m = st->mask & ~(PK_TOUCH | PK_MSG | PK_CLICK);
            else if (st->ok[0]) { LOG("hid 脚本步 %d 松开（服务端拍 %llu，按住 %llu 拍）", g_kcur + 1, (unsigned long long)c.value, (unsigned long long)held); g_krel = 1; g_kstate = KS_OK; if (st->mask & PK_TOUCH) dm_touch_set(NULL, 0); if (st->mask & PK_CLICK) dm_ui_touch(2, st->tx, st->ty); }
            else {
                LOG("hid 脚本步 %d 松开（时钟 %llu，按住 %llu 拍）", g_kcur + 1, (unsigned long long)c.value, (unsigned long long)held); g_krel = 1; if (st->mask & PK_TOUCH) dm_touch_set(NULL, 0); if (st->mask & PK_CLICK) dm_ui_touch(2, st->tx, st->ty);
                g_kcur++; g_kstate = KS_PRE; g_ktries = 0; step_chain();
            }
        }
    }
    pthread_mutex_unlock(&g_klock);
    return m;
}
static int keys_parse_items(void) {
    const char *e = getenv("DM_KEYS"); if (!e) e = "";
    char buf[4096]; if (g_ksoft && strlen(e) >= sizeof buf) KEYS_BAD("DM_KEYS 超过 %d 字节", (int)sizeof buf - 1);
    strncpy(buf, e, sizeof buf - 1); buf[sizeof buf - 1] = 0;
    for (char *save, *tok = strtok_r(buf, ",", &save); tok; tok = strtok_r(NULL, ",", &save)) {
        char *at = strchr(tok, '@'); if (!at) KEYS_BAD("DM_KEYS 项「%s」缺 @时刻 / @消息", tok);
        *at = 0;
        uint32_t mask = 0; float tx = 0, ty = 0;
        if ((tok[0] == 'T' || tok[0] == 'C') && ((tok[1] >= '0' && tok[1] <= '9') || tok[1] == '.')) {
            int click = tok[0] == 'C'; char *c = strchr(tok, ':');
            if (!c) { if (click) KEYS_BAD("DM_KEYS 点击项「%s」格式：C<x>:<y>@前置…", tok); KEYS_BAD("DM_KEYS 触点项「%s」格式：T<x>:<y>@前置…", tok); }
            double x, y;
            if (g_ksoft && (keys_num(tok + 1, c, 0, 1, &x) || keys_num(c + 1, NULL, 0, 1, &y))) KEYS_BAD("DM_KEYS 坐标「%s」不是 0–1 之间的数", tok);
            tx = (float)atof(tok + 1); ty = (float)atof(c + 1); mask = click ? PK_CLICK : PK_TOUCH;
        }
        const char *msg = NULL;
        if (!strncmp(tok, "MSG:", 4)) { if (!strchr(tok + 4, ':') || strlen(tok + 4) >= sizeof g_steps[0].msg) KEYS_BAD("DM_KEYS 消息项「%s」格式：MSG:<类别>:<动作>[:<参数>]@前置[>确认]", tok); msg = tok + 4; mask = PK_MSG; }
        for (int i = 0; i < (int)(sizeof k_names / sizeof *k_names); i++) if (!strcmp(tok, k_names[i].name)) mask = k_names[i].mask;
        for (int i = 0; i < 4; i++) if (!strcmp(tok, k_stick[i].name)) mask = k_stick[i].mask;
        if (!mask) KEYS_BAD("DM_KEYS 未知键名「%s」", tok);
        const char *v = at + 1;
        if ((*v >= '0' && *v <= '9') || *v == '.') {
            if (mask & (PK_TOUCH | PK_MSG | PK_CLICK)) KEYS_BAD("DM_KEYS 触点项 / 消息项只支持按 UI 消息的步（…@前置…），不支持按时刻");
            char *plus = strchr(v, '+');
            double t = atof(v), d = plus ? atof(plus + 1) : 0.2;
            if (g_ksoft && (keys_num(v, plus, 0, 86400, &t) || (plus && keys_num(plus + 1, NULL, 0, 86400, &d)))) KEYS_BAD("DM_KEYS 时刻项「%s」：时刻 / 按住秒数不是 0–86400 之间的数", v);
            if (g_nkeys == KMAX) KEYS_BAD("DM_KEYS 超过 %d 项", KMAX);
            g_keys[g_nkeys++] = (typeof(g_keys[0])){mask, (uint64_t)(t * 1e9), (uint64_t)((t + d) * 1e9)};
        } else {
            if (g_nsteps == KMAX) KEYS_BAD("DM_KEYS 超过 %d 项", KMAX);
            typeof(g_steps[0]) *st = &g_steps[g_nsteps++]; st->mask = mask; st->tx = tx; st->ty = ty; st->msg[0] = 0; if (msg) strcpy(st->msg, msg);
            const char *gt = strchr(v, '>');
            size_t np = gt ? (size_t)(gt - v) : strlen(v);
            if (np >= sizeof st->pre || (gt && (!gt[1] || strlen(gt + 1) >= sizeof st->ok))) KEYS_BAD("DM_KEYS 消息步「%s」格式：键@前置[%%延时秒][+按住秒][>确认]", v);
            memcpy(st->pre, v, np); st->pre[np] = 0;
            char *pp = strchr(st->pre, '+'); if (pp) { st->hold_override = 1; st->hold_seconds = atof(pp + 1); if (g_ksoft && keys_num(pp + 1, NULL, 0, 86400, &st->hold_seconds)) KEYS_BAD("DM_KEYS 消息步「%s」：按住秒数不是 0–86400 之间的数", v); *pp = 0; }
            char *pc = strchr(st->pre, '%'); if (pc) { st->delay_seconds = atof(pc + 1); if (g_ksoft && keys_num(pc + 1, NULL, 0, 86400, &st->delay_seconds)) KEYS_BAD("DM_KEYS 消息步「%s」：延时秒数不是 0–86400 之间的数", v); *pc = 0; }
            if (gt) strcpy(st->ok, gt + 1);
        }
    }
    return 0;
}
int uimsg_try_subscribe(void (*fn)(const char *, const char *, const char *));
void uimsg_subscribe(void (*fn)(const char *, const char *, const char *));
static void keys_parse(void) {
    g_nkeys = 0; g_kbase = now_ns();
    const char *soft = getenv("DM_KEYS_SOFT"); g_ksoft = soft ? atoi(soft) != 0 : keys_soft_default(); atomic_store(&g_kdead, 0);
    int bad = cursor_tests_parse() || keys_parse_items();
    if (!bad && (g_nsteps || g_ncursor_tests)) {
        g_steps[0].seen_pre = 0; if (g_nsteps && !g_steps[0].pre[0]) { script_clock c = script_clock_now(); g_steps[0].seen_pre = 1; g_steps[0].seen_at = now_ns(); g_steps[0].seen_clock = c.value; g_steps[0].seen_server = c.server; }
        if (!g_ksoft) uimsg_subscribe(keys_on_uimsg);
        else { int r = uimsg_try_subscribe(keys_on_uimsg); if (r) { LOG("【红】hid 脚本作废：订阅 UI 消息失败（%s）", r == -1 ? "产物没有 UI 观测门 OBS_UIMSG" : "UI 消息订阅者已满 8 个"); bad = 1; } }
    }


    if (bad) { g_nkeys = g_nsteps = g_ncursor_tests = g_cursor_cur = g_kcur = 0; atomic_store(&g_kdead, 1); LOG("【红】hid 脚本作废：DM_KEYS / DM_TEST_CURSOR 整段不执行，游戏照常（DM_KEYS_SOFT）"); return; }
    LOG("hid 脚本输入：按时刻 %d 项，按 UI 消息 %d 步", g_nkeys, g_nsteps);
}
static pthread_once_t g_kone = PTHREAD_ONCE_INIT;
void hid_keys_init(void) { pthread_once(&g_kone, keys_parse); }
static uint64_t keys_now(void) {
    pthread_once(&g_kone, keys_parse);
    uint64_t t = now_ns() - g_kbase, m = steps_now();
    if (!atomic_load(&g_kdead)) for (int i = 0; i < g_nkeys; i++) if (t >= g_keys[i].t0 && t < g_keys[i].t1) m |= g_keys[i].mask;
    static _Atomic uint64_t seen;
    uint64_t newbits = m & ~atomic_fetch_or(&seen, m);
    if (newbits) LOG("hid 脚本键首次被读到：mask=%#llx @%.2fs", newbits, t / 1e9);
    return m;
}
#else

void hid_keys_init(void) {}
static uint64_t keys_now(void) { return 0; }
#endif





static FILE *g_receipt_file;
static pthread_once_t g_receipt_once = PTHREAD_ONCE_INIT;
static pthread_mutex_t g_receipt_lock = PTHREAD_MUTEX_INITIALIZER;
static void receipt_init(void) {
    const char *path = getenv("DM_HID_RECEIPT_LOG");
    if (path && *path) {
        g_receipt_file = fopen(path, "w");
        if (!g_receipt_file) LOG("【红】HID返回收样文件打开失败：%s", path);
    }
}
static void receipt(const NpadState *s, uint32_t id, uint32_t style, uint64_t km, uint64_t read_start_ns) {
    if (!g_receipt_file) return;
    static NpadState last; static uint32_t last_id, last_style;
    static uint64_t last_km, last_ns; static unsigned count; static int have, truncated;
    NpadState state = *s; state.sampling = 0; uint64_t t = now_ns();
    pthread_mutex_lock(&g_receipt_lock);
    if (truncated || (have && id == last_id && style == last_style && km == last_km && !memcmp(&state, &last, sizeof state) && t - last_ns < 1000000000ull)) {
        pthread_mutex_unlock(&g_receipt_lock); return;
    }
    if (count == 1024) {
        fprintf(g_receipt_file, "{\"event\":\"limit_reached\",\"limit\":1024}\n"); truncated = 1;
    } else {
        fprintf(g_receipt_file, "{\"event\":\"npad_return\",\"read_start_ns\":%llu,\"time_ns\":%llu,\"sampling\":%lld,\"id\":%u,\"style\":%u,\"buttons\":%llu,\"lx\":%d,\"ly\":%d,\"rx\":%d,\"ry\":%d,\"attributes\":%u,\"script_mask\":%llu}\n",
                (unsigned long long)read_start_ns, (unsigned long long)t, (long long)s->sampling, id, style, (unsigned long long)s->buttons,
                s->lx, s->ly, s->rx, s->ry, s->attributes, (unsigned long long)km);
        last = state; last_id = id; last_style = style; last_km = km; last_ns = t; have = 1; count++;
    }
    if (fflush(g_receipt_file)) { LOG("【红】HID返回收样写入失败"); truncated = 1; }
    pthread_mutex_unlock(&g_receipt_lock);
}

void (*dm_pad_hook)(uint64_t, uint64_t *, int32_t *, int32_t *, int32_t *, int32_t *);




void (*dm_qa_frame_hook)(void);
static void fill(NpadState *s, uint32_t id, uint32_t style) {
    if (dm_qa_frame_hook) dm_qa_frame_hook();
    pthread_once(&g_receipt_once, receipt_init);
    DM_PROF(0, "taps", ui_taps_step());
    DM_PROF(0, "ui_frontend", dm_ui_frontend_pump());
    { void dm_textinput_pump(void); DM_PROF(0, "textinput", dm_textinput_pump()); }
    { void dm_update_pump(void); DM_PROF(0, "update", dm_update_pump()); }
    { void dm_actions_pump(void); DM_PROF(0, "actions", dm_actions_pump()); }
    { void dm_world_pump(void); DM_PROF(0, "world", dm_world_pump()); }
    DM_PROF(0, "ui_mouse", dm_ui_mouse_pump());
    { void dm_aim_cone_pump(void); DM_PROF(0, "aim_cone", dm_aim_cone_pump()); dm_ipf_frame(0); }
    if (atomic_load(&g_stall_due) && atomic_exchange(&g_stall_due, 0)) usleep((useconds_t)g_stall_ms * 1000);
    uint64_t read_start_ns = g_receipt_file ? now_ns() : 0;
    memset(s, 0, sizeof *s);
    s->sampling = ++g_sampling;
    if (!(style_of(id) & style)) { receipt(s, id, style, 0, read_start_ns); return; }
    uint64_t km = keys_now();






    { uint64_t dm_ui_alone_take(uint64_t);
      uint64_t tb = ((g_in.buttons | atomic_load(&g_press_latch)) & ~dm_ui_auto_release()) | (km & ~(uint64_t)PK_MASK) | dm_ui_auto_buttons(), pb = atomic_load(&g_pad_btn) | atomic_load(&g_pad_latch);
      static int t_old = -1; if (t_old < 0) t_old = getenv("DM_TEST_ALONE_NOPAD") != NULL;
      uint64_t alone = dm_ui_alone_take(t_old ? tb : tb | pb);
      if (alone) s->buttons = t_old ? alone | atomic_load(&g_pad_btn) | atomic_exchange(&g_pad_latch, 0) : alone;
      else s->buttons = ((g_in.buttons | atomic_exchange(&g_press_latch, 0)) & ~dm_ui_auto_release()) | (km & ~(uint64_t)PK_MASK) | dm_ui_auto_buttons() | atomic_load(&g_pad_btn) | atomic_exchange(&g_pad_latch, 0); }
    s->lx = g_in.lx; s->ly = g_in.ly; s->rx = g_in.rx; s->ry = g_in.ry;



    { uint64_t ax = atomic_load(&g_pad_axes); int16_t plx = (int16_t)ax, ply = (int16_t)(ax >> 16), prx = (int16_t)(ax >> 32), pry = (int16_t)(ax >> 48);
      if (!(s->lx || s->ly) && (plx || ply)) { s->lx = plx; s->ly = ply; } if (!(s->rx || s->ry) && (prx || pry)) { s->rx = prx; s->ry = pry; } }
    int frozen; { int dm_padmode_input_gate(uint64_t *); if ((frozen = dm_padmode_input_gate(&s->buttons))) s->lx = s->ly = s->rx = s->ry = 0; }
    if (dm_pad_hook) dm_pad_hook(guest_ns(), &s->buttons, &s->lx, &s->ly, &s->rx, &s->ry);







    { void dm_cam_remap_stick(int32_t *, int32_t *, int32_t *, int32_t *); dm_cam_remap_stick(&s->lx, &s->ly, &s->rx, &s->ry); }
    { uint64_t dm_itemcmp_pad_filter(uint64_t); s->buttons = dm_itemcmp_pad_filter(s->buttons); }
    { uint64_t dm_runeword_pad_filter(uint64_t); s->buttons = dm_runeword_pad_filter(s->buttons); }
    { uint64_t dm_padmode_page_filter(uint64_t); s->buttons = dm_padmode_page_filter(s->buttons); }
    { uint64_t dm_swapskill_filter(uint64_t); s->buttons = dm_swapskill_filter(s->buttons); }
    { static int trx, tryy, tdelay, held, tset = -1; if (tset < 0) { const char *e = getenv("DM_TEST_RSTICK"); tset = e && sscanf(e, "%d,%d,%d", &trx, &tryy, &tdelay) >= 2; }
      if (tset > 0) { held = (s->buttons & 0xf) ? held + 1 : 0; if (held > tdelay) { s->rx = trx; s->ry = tryy; } } }
    if (s->lx || s->ly || (km & PK_MASK & ~(uint64_t)PK_TOUCH)) atomic_store(&g_stick_ns, now_ns());
    dm_cursor_apply(&s->buttons, &s->lx, &s->ly);
    { static uint64_t prev; if ((s->buttons & DM_BTN_L) && !(prev & DM_BTN_L)) { int n = atomic_fetch_add(&g_dm_l_presses, 1) + 1; if (n <= 3) LOG("取手柄处：「显示物品」键（L）交给了游戏，本进程第 %d 次", n); } prev = s->buttons; }
    if (km & PK_LSL) s->lx = -32767; if (km & PK_LSR) s->lx = 32767;
    if (km & PK_LSU) s->ly = 32767;  if (km & PK_LSD) s->ly = -32767;



    if (km & PK_MASK & ~(uint64_t)PK_TOUCH) { void dm_cam_remap_stick(int32_t *, int32_t *, int32_t *, int32_t *); int32_t zx = 0, zy = 0; dm_cam_remap_stick(&s->lx, &s->ly, &zx, &zy); }
    if (s->buttons & DM_BTN_PLUS) atomic_fetch_add(&g_plus_on, 1); else if (!frozen) atomic_fetch_add(&g_plus_off, 1);
    s->attributes = ATTR_CONNECTED;
    atomic_store_explicit(&g_sampled_right,(uint32_t)s->rx|((uint64_t)(uint32_t)s->ry<<32),memory_order_release);
    receipt(s, id, style, km, read_start_ns);
}

HLE_FORCE("_ZN2nn3hid14InitializeNpadEv", "连 hid 服务；宿主从 dm_input 取输入", void, h_hid_InitNpad, void) {}
HLE_FORCE("_ZN2nn3hid21InitializeTouchScreenEv", "连 hid 服务；触屏输入由前端另接", void, h_hid_InitTouch, void) {}
HLE_FORCE("_ZN2nn3hid22SetSupportedNpadIdTypeEPKjm", "支持的手柄号；宿主固定", void, h_hid_SetIds, const uint32_t *ids, uint64_t n) { (void)ids; (void)n; }


HLE_FORCE("_ZN2nn3hid24SetSupportedNpadStyleSetENS_4util10BitFlagSetILi32ENS0_12NpadStyleTagEEE", "支持的手柄样式；宿主固定", void, h_hid_SetStyles, uint32_t styles) { (void)styles; }
HLE_FORCE("_ZN2nn3hid29SetNpadHandheldActivationModeENS0_26NpadHandheldActivationModeE", "掌机手柄激活方式；宿主固定", void, h_hid_SetHhMode, int m) { (void)m; }
HLE_FORCE("_ZN2nn3hid15GetNpadStyleSetERKj", "手柄样式查询；跟 DM_OPMODE 走", uint32_t, h_hid_GetStyle, const uint32_t *id) { return style_of(*id); }
HLE_FORCE("_ZN2nn3hid12GetNpadStateEPNS0_16NpadFullKeyStateERKj", "全键手柄状态；取 dm_input", void, h_hid_GetFullKey, NpadState *s, const uint32_t *id) { fill(s, *id, STYLE_FULLKEY); }
HLE_FORCE("_ZN2nn3hid12GetNpadStateEPNS0_17NpadHandheldStateERKj", "掌机手柄状态；取 dm_input", void, h_hid_GetHandheld, NpadState *s, const uint32_t *id) { fill(s, *id, STYLE_HANDHELD); }



HLE_FORCE("_ZN2nn3hid20IsVibrationPermittedEv", "震动许可；宿主不支持（返回 bool）", unsigned, h_hid_VibPermit, void) { return 0; }
HLE_FORCE("_ZN2nn3hid25GetVibrationDeviceHandlesEPNS0_21VibrationDeviceHandleEiRKjNS_4util10BitFlagSetILi32ENS0_12NpadStyleTagEEE",
          "震动设备；宿主 0 个", int, h_hid_GetVibHandles, void *out, int max, const uint32_t *id, uint32_t style) { (void)out; (void)max; (void)id; (void)style; return 0; }
HLE_FORCE("_ZN2nn3hid25InitializeVibrationDeviceERKNS0_21VibrationDeviceHandleE", "震动；宿主不支持", void, h_hid_InitVib, const void *h) { (void)h; }
HLE_FORCE("_ZN2nn3hid18SendVibrationValueERKNS0_21VibrationDeviceHandleERKNS0_14VibrationValueE", "震动；宿主不支持", void, h_hid_SendVib, const void *h, const void *v) { (void)h; (void)v; }





typedef struct { int64_t sampling; int32_t count, _r; uint8_t touches[16][0x28]; } TouchState16;
_Static_assert(sizeof(TouchState16) == 0x290, "TouchScreenState<16>");
int touch_fill(void *out, uint64_t now_ns);
HLE_FORCE("_ZN2nn3hid20GetTouchScreenStatesILm16EEEiPNS0_16TouchScreenStateIXT_EEEi", "触屏状态：前端触点（hle_touch.c）", int, h_hid_GetTouch, TouchState16 *out, int n) {
    if (n < 1) return 0;
    touch_fill(out, guest_ns());
    return 1;
}
