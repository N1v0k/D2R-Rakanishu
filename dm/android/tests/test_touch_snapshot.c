






#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "dm/android/touch_android.c"
#include "dm/ios/Sources/dm_touch_controls.c"

_Static_assert(DM_ANDROID_TOUCH_DRAW_FLOATS == 185, "Review the Java/native ABI before changing this test");
enum { DRAW_JOY = 12, DRAW_SWAP = 13, DRAW_DIFF = 14, DRAW_PICK0 = 15 };
static const char *case_name = "startup";
static unsigned checks, cases;
static uint64_t test_clock_ns = UINT64_C(100000000000), test_sample;
static int observed_panel;
static unsigned panel_calls;

#define REQUIRE(expression) do { \
    ++checks; \
    if (!(expression)) { \
        fprintf(stderr, "FAIL %s:%d case=%s expression=%s\n", __FILE__, __LINE__, case_name, #expression); \
        exit(1); \
    } \
} while (0)

static void unexpected_host_call(const char *name) {
    fprintf(stderr, "FAIL case=%s unexpected host path=%s\n", case_name, name);
    exit(2);
}





uint64_t now_ns(void) { return test_clock_ns; }
uint64_t dm_input_samples(void) { return test_sample; }
void dm_cursor_clear(void) {}
int dm_cursor_pending(void) { return 0; }
void dm_ui_hand_idle(int idle) { (void)idle; }
int dm_ui_panel_state(void) { return 0; }
int dm_ui_tap_box(void) { return 0; }
uint32_t dm_ui_cur_stack_id(void) { return 0; }
uint32_t dm_ui_hover_unid(void) { return 0; }
int dm_ui_targeting(void) { return 0; }
int dm_ui_touch_active(void) { return 0; }
int dm_ui_bind_active(void) { return 0; }
int dm_belt_rows(int *expanded) { if (expanded) *expanded = 0; return 1; }
void dm_hud_get(dm_hud_info *out) { memset(out, 0, sizeof *out); }
int dm_custom_get(const char *key) { (void)key; return 0; }

static int seen_safe_left = -1, seen_safe_right = -1;
void dm_ui_safe_left(int units) { seen_safe_left = units; }
void dm_ui_safe_right(int units) { seen_safe_right = units; }
int dm_touch_game_snapshot(dm_tg_snapshot *out) { memset(out,0,sizeof *out); return 0; }
dm_ta_glyph dm_ta_skill(uint32_t id) { (void)id; return (dm_ta_glyph){.status=DM_TA_UNKNOWN,.atlas=-1}; }
dm_ta_glyph dm_ta_current_set(const dm_tg_snapshot *s,uint64_t key,uint64_t now,int set) { (void)s; (void)key; (void)now; (void)set; return dm_ta_skill(0); }
int dm_ta_frame_at(int atlas,int frame,dm_ta_frame *out) { (void)atlas; (void)frame; (void)out; return 0; }
int dm_swapskill_slot(int slot,int *charges) { (void)slot; *charges=0; return 0; }
int dm_ui_minimap_rect(float r[4]) { (void)r; return 0; }
int dm_ui_map_is_full(void) { return 0; }
int dm_cam_mode(void) { return 0; }
int dm_cam_style(void) { return 0; }
void dm_cam_tilt(float rad) { (void)rad; }
void dm_cam_turn(float rad) { (void)rad; }
void dm_ui_map_full(int on) { (void)on; unexpected_host_call(__func__); }

static int test_full, test_real_w = 1000, test_real_h = 500, test_ui_x, test_ui_y = 150, test_ui_w = 1000, test_ui_h = 350;
int front_android_full(void) { return test_full; }

static int test_haptic_pending, test_haptic_handed[4], test_haptic_calls;
int dm_drop_haptic_take(void) { int t = test_haptic_pending; test_haptic_pending = 0; return t; }
void front_android_drop_haptic(int tier) { if (tier >= 0 && tier < 4) test_haptic_handed[tier]++; test_haptic_calls++; }
int dm_screen_real(int *w, int *h) { if (w) *w = test_real_w; if (h) *h = test_real_h; return test_real_h > 0; }
void dm_ui_rect(int *x, int *y, int *w, int *h) { if (x) *x = test_ui_x; if (y) *y = test_ui_y; if (w) *w = test_ui_w; if (h) *h = test_ui_h; }
static int map_tap_ok; static float tap_ux, tap_uy, tap_wx, tap_wy, map_wx, map_wy;
unsigned dm_world_tap(float x,float y) { if (!map_tap_ok) unexpected_host_call(__func__); map_wx = x; map_wy = y; return 0; }
int dm_world_tap_result(unsigned seq) { (void)seq; return -1; }
uint32_t dm_ui_hit_id(void) { return 0; }
void dm_points_get(dm_points_info *out) { memset(out,0,sizeof *out); }
void dm_stat_add(int stat,int n) { (void)stat; (void)n; unexpected_host_call(__func__); }
void dm_skill_add_selected(void) { unexpected_host_call(__func__); }

void dm_cursor_set_screen(float x, float y, float w, float h) { (void)x; (void)y; (void)w; (void)h; unexpected_host_call(__func__); }
void dm_ui_touch(int phase, float x, float y) { (void)phase; (void)x; (void)y; unexpected_host_call(__func__); }
int dm_ui_fe_drag_ok(void) { unexpected_host_call(__func__); return 0; }
void dm_ui_drag_begin(int kind, float x, float y, float tolerance) { (void)kind; (void)x; (void)y; (void)tolerance; unexpected_host_call(__func__); }
void dm_ui_drag_move(float x, float y) { (void)x; (void)y; unexpected_host_call(__func__); }
void dm_ui_drag_end(void) { unexpected_host_call(__func__); }
void dm_ui_drag_lift(float y) { (void)y; unexpected_host_call(__func__); }
void dm_ui_press_new(void) { unexpected_host_call(__func__); }
int dm_ui_item_at_now(float x, float y) { (void)x; (void)y; unexpected_host_call(__func__); return 0; }
int dm_ui_drop_at_now(float x, float lifted, float finger) { (void)x; (void)lifted; (void)finger; unexpected_host_call(__func__); return 0; }
int dm_ui_drop_bag(void) { unexpected_host_call(__func__); return 0; }
int dm_ui_drop_point(float *x, float *y) { (void)x; (void)y; unexpected_host_call(__func__); return 0; }
int dm_ui_origin_point(float *x, float *y) { (void)x; (void)y; unexpected_host_call(__func__); return 0; }
int dm_ui_bind_skill_hit(float *x, float *y) { (void)x; (void)y; unexpected_host_call(__func__); return 0; }
void dm_ui_bind_begin(uint64_t key) { (void)key; unexpected_host_call(__func__); }
static int world_tap_ok; static unsigned world_taps;
void dm_ui_world_tap(float ux, float uy, float wx, float wy) { if (!world_tap_ok) unexpected_host_call(__func__); ++world_taps; tap_ux = ux; tap_uy = uy; tap_wx = wx; tap_wy = wy; }
void dm_ui_stack_take_at(float x, float y) { (void)x; (void)y; unexpected_host_call(__func__); }
void dm_ui_stack_cube_at(float x, float y) { (void)x; (void)y; unexpected_host_call(__func__); }
void dm_ui_shop_buy_at(float x, float y) { (void)x; (void)y; unexpected_host_call(__func__); }
void dm_ui_tab_go(void) { unexpected_host_call(__func__); }
void dm_ui_use_item(void) { unexpected_host_call(__func__); }
void dm_ui_identify_hovered(void) { unexpected_host_call(__func__); }
void dm_ui_long_press(void) { unexpected_host_call(__func__); }
void dm_ui_item_tap(void) { unexpected_host_call(__func__); }
void dm_stash_take(int n) { (void)n; unexpected_host_call(__func__); }
void dm_hud_request_players(int n) { (void)n; unexpected_host_call(__func__); }
void dm_belt_expand(int on) { (void)on; unexpected_host_call(__func__); }
void dm_belt_use(int col, int row) { (void)col; (void)row; unexpected_host_call(__func__); }
void dm_shop_sell_cursor(void) { unexpected_host_call(__func__); }
void dm_tp_request(void) { unexpected_host_call(__func__); }



void dm_uiscreen_set_panel(int panel) {
    REQUIRE(panel==0 || panel==1);
    observed_panel=panel; ++panel_calls;
}



static int map_hold;
int dm_hud_stack_msg(const char *act, const char *arg) { REQUIRE(act && arg); return 0; }
void dm_ui_map_hold(int on) { REQUIRE(on==0 || on==1); map_hold=on; }


static uint32_t test_rebuilds;
static int scene_game = -1;
static uint64_t scene_epoch, scene_world;
static unsigned scene_calls;
uint32_t dm_padmode_rebuilds(void) { return test_rebuilds; }
void dm_touch_game_set_scene(int game, uint64_t scene_tag, uint64_t world_tag) { scene_game = game; scene_epoch = scene_tag; scene_world = world_tag; ++scene_calls; }
static void fixture(int navigation) {
    memset(&tc, 0, sizeof tc);
    memset(&scene, 0, sizeof scene);
    memset(queue, 0, sizeof queue);
    head = tail = 0; ready = 0; focused = 1; cancel = resizing = ui_drag = 0;
    width = 800; height = 375; win_w = 800; win_h = 375; vr_x = vr_y = 0; vr_w = 800; vr_h = 375;
    safe_left = safe_right = safe_bottom = 0; tablet_layout = swap_above = 0;
    test_full = 0; map_tap_ok = 0; tap_ux = tap_uy = tap_wx = tap_wy = map_wx = map_wy = 0; seen_safe_left = seen_safe_right = -1;
    epoch = world = 1; pulse_epoch = pulse_sample = resume_sample = 0; pulse_hold_start = 0;
    seen_world = seen_mouse = seen_tab = seen_use = seen_ident = seen_long = seen_item = seen_stack = 0;
    frontend = 0; fe_slot = -1; fe_drag = 0; fe_x0 = fe_y0 = 0;
    test_clock_ns = UINT64_C(100000000000); test_sample = 0;
    observed_panel=-1; panel_calls=0; map_hold=-1;
    rebuilds_seen = 0; test_rebuilds = 0; scene_game = -1; scene_epoch = scene_world = 0; scene_calls = 0;
    dm_tc_set_safe_insets(0, 0); dm_tc_set_edge_bottom(0); dm_tc_set_safe_bottom(0); dm_tc_set_swap_above(0);
    dm_tc_set_panel_state(0); dm_tc_set_tap_box(0); dm_tc_set_cur_stack(0); dm_tc_set_hover_unid(0);
    init_locked();
    dm_tc_set_navigation_mode(&tc, navigation);
    dm_tc_enable_targets(&tc, navigation ? dm_tc_nav_mask(1) : ((1u << DM_TC_COUNT) - 1u));
}

static int field(int record, int component) { return 4 + record * 5 + component; }
static int lit(const float *s, int record) { return s[field(record, 4)] != 0; }
static int visible(const float *s, int record) { return s[field(record, 3)] != 0; }




static void read_twice(float result[DM_ANDROID_TOUCH_DRAW_FLOATS]) {
    struct guarded { uint32_t left; float values[DM_ANDROID_TOUCH_DRAW_FLOATS]; uint32_t right; } a, b;
    unsigned char before[sizeof tc];
    a.left = b.left = UINT32_C(0x13579bdf); a.right = b.right = UINT32_C(0x2468ace0);
    memset(a.values, 0xa5, sizeof a.values); memset(b.values, 0x5a, sizeof b.values);
    memcpy(before, &tc, sizeof tc);
    dm_android_touch_layout(a.values);
    dm_android_touch_layout(b.values);
    REQUIRE(a.left == UINT32_C(0x13579bdf) && b.left == a.left);
    REQUIRE(a.right == UINT32_C(0x2468ace0) && b.right == a.right);
    REQUIRE(memcmp(a.values, b.values, sizeof a.values) == 0);
    REQUIRE(memcmp(before, &tc, sizeof tc) == 0);
    for (int i = 0; i < DM_ANDROID_TOUCH_DRAW_FLOATS; ++i) REQUIRE(isfinite(a.values[i]));
    memcpy(result, a.values, sizeof a.values);
}

static void unchanged_geometry(const float *a, const float *b) {
    REQUIRE(a[0] == b[0] && a[1] == b[1]);
    for (int i = 0; i < 23; ++i)
        for (int c = 0; c < 3; ++c) REQUIRE(a[field(i, c)] == b[field(i, c)]);
}

static void down_target(int slot, int target, double at) {
    dm_tc_target t = tc.layout.target[target];
    dm_tc_begin_at(&tc, slot, t.x, t.y, at);
    REQUIRE(tc.finger[slot].active && tc.finger[slot].target == target);
}

static void test_idle_and_abi(void) {
    fixture(0); float a[DM_ANDROID_TOUCH_DRAW_FLOATS], b[DM_ANDROID_TOUCH_DRAW_FLOATS]; read_twice(a); read_twice(b);
    REQUIRE(memcmp(a, b, sizeof a) == 0);
    REQUIRE(a[0] == 800 && a[1] == 375 && a[2] == 0 && a[3] == 0);
    REQUIRE(visible(a, DRAW_JOY) && visible(a, DRAW_SWAP) && visible(a, DRAW_DIFF));
    for (int i = 0; i < 23; ++i) REQUIRE(!lit(a, i));
    REQUIRE(a[DM_ANDROID_TOUCH_HUD + 16] == 0 && a[DM_ANDROID_TOUCH_HUD + 17] == 0);
}


static void test_stick_deflection(void) {
    fixture(0); float a[DM_ANDROID_TOUCH_DRAW_FLOATS];
    const float x = tc.layout.joy_x, y = tc.layout.joy_y, r = tc.layout.joy_radius;
    dm_tc_begin_at(&tc, 0, x, y, 100.0);
    REQUIRE(tc.finger[0].active && tc.finger[0].target == DM_TC_JOYSTICK);
    dm_tc_move_at(&tc, 0, x + r, y, 100.05); read_twice(a);
    REQUIRE(tc.lx > 0);
    REQUIRE(a[DM_ANDROID_TOUCH_HUD + 16] == tc.lx && a[DM_ANDROID_TOUCH_HUD + 17] == tc.ly);
    dm_tc_end_at(&tc, 0, 100.1); read_twice(a);
    REQUIRE(tc.lx == 0 && tc.ly == 0);
    REQUIRE(a[DM_ANDROID_TOUCH_HUD + 16] == 0 && a[DM_ANDROID_TOUCH_HUD + 17] == 0);
}

static void test_press_release_and_multitouch(void) {
    fixture(0); float a[DM_ANDROID_TOUCH_DRAW_FLOATS], b[DM_ANDROID_TOUCH_DRAW_FLOATS]; read_twice(a);
    down_target(0, DM_TC_MAIN, 100.0); read_twice(b);
    REQUIRE(lit(b, DM_TC_MAIN)); REQUIRE(memcmp(a, b, sizeof a) != 0);
    unchanged_geometry(a, b);
    down_target(1, DM_TC_SKILL1, 100.01); read_twice(b);
    REQUIRE(lit(b, DM_TC_MAIN) && lit(b, DM_TC_SKILL1));
    dm_tc_cancel_finger(&tc, 0); read_twice(b);
    REQUIRE(!lit(b, DM_TC_MAIN) && lit(b, DM_TC_SKILL1));
    dm_tc_cancel_all(&tc); read_twice(b);
    for (int i = 0; i < 23; ++i) REQUIRE(!lit(b, i));
    down_target(0, DM_TC_MAIN, 101.0);
    dm_tc_end_at(&tc, 0, 101.1); read_twice(b);
    REQUIRE(!lit(b, DM_TC_MAIN) && tc.buttons == 0);
}

static void test_slide_cancel(void) {
    fixture(0); float a[DM_ANDROID_TOUCH_DRAW_FLOATS]; down_target(0, DM_TC_SKILL1, 100.0);
    dm_tc_target map = tc.layout.target[DM_TC_MAP];
    dm_tc_move_at(&tc, 0, map.x, map.y, 100.1);
    REQUIRE(tc.finger[0].active && tc.finger[0].aim_cancelled);
    REQUIRE((tc.buttons & tc.layout.target[DM_TC_SKILL1].button) == 0);
    read_twice(a); REQUIRE(!lit(a, DM_TC_SKILL1));
    dm_tc_end_at(&tc, 0, 100.11); read_twice(a);
    REQUIRE(!lit(a, DM_TC_SKILL1) && tc.pulse_button == 0);
}

static void test_release_pulse(void) {
    fixture(0); float a[DM_ANDROID_TOUCH_DRAW_FLOATS], b[DM_ANDROID_TOUCH_DRAW_FLOATS]; down_target(0, DM_TC_SKILL1, 100.0);
    dm_tc_end_at(&tc, 0, 100.01);
    REQUIRE(!tc.finger[0].active && tc.pulse_button == tc.layout.target[DM_TC_SKILL1].button);
    read_twice(a); REQUIRE(lit(a, DM_TC_SKILL1));
    REQUIRE(dm_tc_pulse_end(&tc, tc.pulse_epoch));
    read_twice(b); REQUIRE(!lit(b, DM_TC_SKILL1));
    REQUIRE(memcmp(a, b, sizeof a) != 0); unchanged_geometry(a, b);
}

static void test_swap_and_delayed_pulse(void) {
    fixture(0); float a[DM_ANDROID_TOUCH_DRAW_FLOATS], b[DM_ANDROID_TOUCH_DRAW_FLOATS]; read_twice(a);
    dm_tc_target swap = tc.layout.swap;
    dm_tc_begin_at(&tc, 0, swap.x, swap.y, 100.0); read_twice(b);
    REQUIRE(tc.finger[0].target == DM_TC_SWAP && b[3] == 0 && lit(b, DRAW_SWAP));
    REQUIRE(memcmp(a, b, sizeof a) != 0); unchanged_geometry(a, b);
    dm_tc_end_at(&tc, 0, 100.01); read_twice(b);
    REQUIRE(b[3] == 1 && !lit(b, DRAW_SWAP));
    down_target(0, DM_TC_SKILL1, 101.0); dm_tc_end_at(&tc, 0, 101.01);
    REQUIRE(!tc.finger[0].active && tc.defer_button != 0 && tc.pulse_button == 0);
    read_twice(a); REQUIRE(a[3] == 1 && !lit(a, DM_TC_SKILL1));
    dm_tc_tick(&tc, 101.0 + DM_TC_ZL_LEAD_S + 0.01); read_twice(b);
    REQUIRE(tc.defer_button == 0 && tc.pulse_button != 0 && lit(b, DM_TC_SKILL1));
    REQUIRE(memcmp(a, b, sizeof a) != 0); unchanged_geometry(a, b);
    REQUIRE(dm_tc_pulse_end(&tc, tc.pulse_epoch)); read_twice(b); REQUIRE(!lit(b, DM_TC_SKILL1));
    dm_tc_begin_at(&tc, 0, swap.x, swap.y, 102.0); dm_tc_end_at(&tc, 0, 102.01);
    read_twice(b); REQUIRE(b[3] == 0);
}

static void test_difficulty_hit_and_close(void) {
    fixture(0); float a[DM_ANDROID_TOUCH_DRAW_FLOATS], b[DM_ANDROID_TOUCH_DRAW_FLOATS]; read_twice(a);
    for (int i = 0; i < 8; ++i) REQUIRE(!visible(a, DRAW_PICK0 + i));
    dm_tc_target diff = tc.layout.diff;
    dm_tc_begin_at(&tc, 0, diff.x, diff.y, 100.0); read_twice(b);
    REQUIRE(tc.diff_open && tc.finger[0].target == DM_TC_DIFF && lit(b, DRAW_DIFF));
    for (int i = 0; i < 8; ++i) { REQUIRE(visible(b, DRAW_PICK0 + i)); REQUIRE(!lit(b, DRAW_PICK0 + i)); }
    REQUIRE(memcmp(a, b, sizeof a) != 0);
    dm_tc_cancel_finger(&tc, 0); REQUIRE(tc.diff_open);
    dm_tc_target pick = tc.layout.pick[3];
    dm_tc_begin_at(&tc, 0, pick.x, pick.y, 100.1); read_twice(b);
    REQUIRE(tc.diff_request == 4 && !tc.diff_open && tc.finger[0].target == DM_TC_DIFF);
    REQUIRE(!lit(b, DRAW_DIFF));
    for (int i = 0; i < 8; ++i) {
        REQUIRE(!visible(b, DRAW_PICK0 + i));
        if (i != 3) REQUIRE(!lit(b, DRAW_PICK0 + i));
    }




    dm_tc_cancel_finger(&tc, 0);
    dm_tc_begin_at(&tc, 0, diff.x, diff.y, 101.0); dm_tc_end_at(&tc, 0, 101.01);
    REQUIRE(tc.diff_open);
    dm_tc_begin_at(&tc, 0, diff.x, diff.y, 101.1); read_twice(b);
    REQUIRE(!tc.diff_open && lit(b, DRAW_DIFF));
    for (int i = 0; i < 8; ++i) REQUIRE(!visible(b, DRAW_PICK0 + i));
}

static void test_belt_diff_target_does_not_light_difficulty(void) {
    fixture(0); float a[DM_ANDROID_TOUCH_DRAW_FLOATS]; tc.belt_rows = 2; tc.belt_open = 1;
    dm_tc_target belt = tc.layout.target[DM_TC_BELT1];
    float row_y = belt.y - 165.0f * tc.layout.height / 1320.0f;
    dm_tc_begin_at(&tc, 0, belt.x, row_y, 100.0);
    REQUIRE(tc.finger[0].active && tc.finger[0].target == DM_TC_DIFF);
    REQUIRE(tc.belt_request != 0); read_twice(a);
    REQUIRE(!lit(a, DRAW_DIFF));
    for (int i = 0; i < 8; ++i) REQUIRE(!visible(a, DRAW_PICK0 + i));
}

static void test_navigation_and_resize(void) {
    fixture(0); float a[DM_ANDROID_TOUCH_DRAW_FLOATS], b[DM_ANDROID_TOUCH_DRAW_FLOATS]; read_twice(a);
    dm_tc_set_navigation_mode(&tc, 1); dm_tc_enable_targets(&tc, dm_tc_nav_mask(1)); read_twice(b);
    REQUIRE(b[2] == 1 && memcmp(a, b, sizeof a) != 0);
    REQUIRE(!visible(b, DRAW_JOY) && !visible(b, DRAW_SWAP) && !visible(b, DRAW_DIFF));
    dm_tc_resize(&tc, 960, 440); read_twice(b);
    REQUIRE(b[0] == 960 && b[1] == 440 && b[2] == 1);
}

static uint64_t queue_sample(void) {
    uint64_t buttons = 0; int32_t lx = 0, ly = 0, rx = 0, ry = 0;
    ++test_sample;
    dm_android_touch_step(test_clock_ns, &buttons, &lx, &ly, &rx, &ry);
    return buttons;
}

static void test_android_queue_ten_short_taps(void) {
    fixture(0); dm_tc_target main = tc.layout.target[DM_TC_MAIN]; unsigned observed = 0;
    for (unsigned i = 0; i < 10; ++i) {
        test_clock_ns += UINT64_C(100000000);
        dm_android_touch_event(0, 0, main.x, main.y);
        test_clock_ns += UINT64_C(5000000);
        dm_android_touch_event(2, 0, main.x, main.y);
        REQUIRE(tail - head == 2);
        uint64_t delivered = queue_sample();
        REQUIRE((delivered & DM_BTN_A) != 0);
        REQUIRE(!tc.finger[0].active && (tc.buttons & DM_BTN_A) == 0);
        ++observed;
        test_clock_ns += UINT64_C(10000000);
        REQUIRE((queue_sample() & DM_BTN_A) == 0);
    }
    REQUIRE(observed == 10);
    printf("DETAIL native_queue_short_taps=%u/10 scope=bridge_samples_not_game_actions\n", observed);
}

static void test_android_focus_cancel(void) {
    fixture(0); float a[DM_ANDROID_TOUCH_DRAW_FLOATS]; dm_tc_target main = tc.layout.target[DM_TC_MAIN];
    dm_android_touch_event(0, 0, main.x, main.y);
    REQUIRE((queue_sample() & DM_BTN_A) != 0); read_twice(a); REQUIRE(lit(a, DM_TC_MAIN));
    dm_android_touch_focus(0); REQUIRE(queue_sample() == 0); read_twice(a);
    REQUIRE(!lit(a, DM_TC_MAIN) && tc.buttons == 0 && head == tail);
    dm_android_touch_focus(1); REQUIRE(queue_sample() == 0); read_twice(a); REQUIRE(!lit(a, DM_TC_MAIN));
}



static void test_android_cancelled_latch(void) {
    fixture(0);
    dm_tc_target main=tc.layout.target[DM_TC_MAIN], map=tc.layout.target[DM_TC_MAP];
    dm_android_touch_event(0,0,main.x,main.y);
    dm_android_touch_event(1,0,map.x,map.y);
    dm_android_touch_event(2,0,map.x,map.y);
    REQUIRE((queue_sample() & DM_BTN_A)==0);
    REQUIRE(tc.buttons==0);
    fixture(0); main=tc.layout.target[DM_TC_MAIN];
    dm_android_touch_event(0,0,main.x,main.y);
    dm_android_touch_event(3,0,main.x,main.y);
    dm_android_touch_event(0,1,main.x,main.y);
    dm_android_touch_event(2,1,main.x,main.y);
    REQUIRE((queue_sample() & DM_BTN_A)!=0);
    REQUIRE((queue_sample() & DM_BTN_A)==0);
}



static void test_android_swap_release_cancel(void) {
    fixture(0); dm_tc_target swap=tc.layout.swap;
    dm_android_touch_event(0,0,swap.x,swap.y); queue_sample(); REQUIRE(tc.skill_set==0);
    dm_android_touch_event(2,0,swap.x,swap.y); queue_sample(); REQUIRE(tc.skill_set==1);
    dm_android_touch_event(0,0,swap.x,swap.y);
    dm_android_touch_event(1,0,swap.x-100,swap.y);
    dm_android_touch_event(2,0,swap.x,swap.y);
    queue_sample(); REQUIRE(tc.skill_set==1);
}




static void test_android_quiet_release(void) {
    fixture(0); world_tap_ok = 1; world_taps = 0;
    const float u = tc.layout.height / 1320, y = 1165 * u, x = tc.layout.width / 2 + (1600 - 1434) * u;
    REQUIRE(dm_tc_hit(&tc.layout, x, y) == DM_TC_NONE && x >= tc.layout.joy_zone_r);
    dm_android_touch_event(0, 0, x, y); test_clock_ns += UINT64_C(50000000); dm_android_touch_event(2, 0, x, y); queue_sample();
    REQUIRE(world_taps == 1 && !tc.finger[0].active);
    test_clock_ns += UINT64_C(500000000);
    dm_android_touch_event(0, 0, x, y); test_clock_ns += UINT64_C(50000000); dm_android_touch_event(4, 0, x, y); queue_sample();
    REQUIRE(world_taps == 1 && !tc.finger[0].active && seen_world == tc.world_tap_seq);


    const float jx = tc.layout.joy_zone_r - 20 * u;
    REQUIRE(dm_tc_hit(&tc.layout, jx, y) == DM_TC_NONE && y > tc.layout.joy_zone_t);
    test_clock_ns += UINT64_C(500000000);
    dm_android_touch_event(0, 1, jx, y); queue_sample(); REQUIRE(tc.joy_slot == 1);
    dm_android_touch_event(1, 1, jx + tc.layout.joy_radius, y); queue_sample(); REQUIRE(tc.lx > 0);
    dm_android_touch_event(4, 1, jx + tc.layout.joy_radius, y); queue_sample();
    REQUIRE(tc.joy_slot == DM_TC_NONE && tc.lx == 0 && !tc.finger[1].active && world_taps == 1);
    test_clock_ns += UINT64_C(500000000);
    dm_android_touch_event(0, 1, jx, y); test_clock_ns += UINT64_C(50000000); dm_android_touch_event(4, 1, jx, y); queue_sample();
    REQUIRE(world_taps == 1 && tc.joy_slot == DM_TC_NONE);
    world_tap_ok = 0;
}



static void test_android_panel_observation(void) {
    fixture(0);
    dm_android_touch_uimsg(NULL,"UnloadPanel","LoadScreenPanel");
    REQUIRE(panel_calls==0 && observed_panel==-1);
    dm_android_touch_uimsg("PanelManager","UnloadPanel","LoadScreenPanel");
    REQUIRE(scene.game==1 && tc.navigation_mode==0 && observed_panel==0 && panel_calls==1);
    dm_tc_target main=tc.layout.target[DM_TC_MAIN];
    dm_android_touch_event(0,0,main.x,main.y);
    REQUIRE((queue_sample()&DM_BTN_A)!=0);
    dm_android_touch_event(2,0,main.x,main.y);
    REQUIRE(head!=tail);
    dm_android_touch_uimsg("PanelManager","OpenPanel","PlayerInventoryExpansionLayout");
    REQUIRE(tc.navigation_mode!=0 && observed_panel==1 && panel_calls==2);
    REQUIRE(tc.buttons==0 && head==tail && !tc.finger[0].active);
    dm_android_touch_uimsg("PanelManager","UnloadPanel","PlayerInventoryExpansionLayout");
    REQUIRE(tc.navigation_mode==0 && observed_panel==0 && panel_calls==3);
    dm_android_touch_uimsg("PanelManager","OpenPanel","NpcDialogPanel");
    REQUIRE(tc.navigation_mode!=0 && dm_tc_scene_dialog_only(&scene));
    REQUIRE(observed_panel==0 && panel_calls==4 && map_hold==1);
    dm_android_touch_uimsg("PanelManager","UnloadAll","");
    REQUIRE(tc.navigation_mode==0 && observed_panel==0 && panel_calls==5 && map_hold==0);
}






static void test_android_padmode_rebuild_world(void) {
    fixture(0);
    dm_android_touch_uimsg("PanelManager","UnloadPanel","LoadScreenPanel");
    REQUIRE(scene.game==1 && scene_calls==1 && scene_game==1);
    uint64_t world0=scene_world, epoch0=scene_epoch;
    dm_android_touch_uimsg("PanelManager","OpenPanel","PlayerInventoryExpansionLayout");
    REQUIRE(scene_calls==2 && scene_game==0 && scene_world==world0);
    dm_android_touch_uimsg("PanelManager","UnloadPanel","PlayerInventoryExpansionLayout");
    REQUIRE(scene_calls==3 && scene_game==1 && scene_world==world0);
    epoch0=scene_epoch;
    test_rebuilds=1;
    dm_android_touch_uimsg("PanelManager","LoadPanel","HUDPanel");
    REQUIRE(scene_calls==4 && scene_world==world0+1 && scene_epoch==epoch0 && scene_game==1);
    dm_android_touch_uimsg("PanelManager","LoadPanel","HUDPanel");
    REQUIRE(scene_calls==5 && scene_world==world0+1);
    test_rebuilds=3;
    dm_android_touch_uimsg("PanelManager","LoadPanel","HUDPanel");
    REQUIRE(scene_calls==6 && scene_world==world0+2 && scene_epoch==epoch0);

    float a[DM_ANDROID_TOUCH_DRAW_FLOATS]; test_rebuilds=4; read_twice(a);
    REQUIRE(scene_calls==6 && rebuilds_seen==3);
    dm_android_touch_uimsg("PanelManager","LoadPanel","HUDPanel");
    REQUIRE(scene_world==world0+3);
}



static void test_android_view_rect_coordinates(void) {
    fixture(0); float a[DM_ANDROID_TOUCH_DRAW_FLOATS];
    dm_android_touch_resize(1000, 500, 0, 0, 0);
    REQUIRE(width == 1000 && height == 500 && vr_x == 0 && vr_y == 0 && vr_w == 1000 && vr_h == 500);
    REQUIRE(resizing == 1 && cancel == 1);
    dm_android_touch_view(100, 150, 800, 350);
    REQUIRE(width == 800 && height == 350 && vr_x == 100 && vr_y == 150 && vr_w == 800 && vr_h == 350 && resizing == 1);
    dm_android_touch_view(100, 150, 0, 350);
    REQUIRE(vr_w == 800 && width == 800);
    queue_sample(); REQUIRE(resizing == 0);
    REQUIRE(tc.layout.width == 800 && tc.layout.height == 350);
    read_twice(a); REQUIRE(a[0] == 800 && a[1] == 350);
    dm_android_touch_event(0, 0, 500, 300);
    REQUIRE(tail - head == 1 && queue[(tail - 1) % 256].x == 400 && queue[(tail - 1) % 256].y == 150);
    dm_android_touch_event(1, 0, 500, 50);
    REQUIRE(tail - head == 2 && queue[(tail - 1) % 256].x == 400 && queue[(tail - 1) % 256].y == -100);
    dm_android_touch_event(3, 0, 500, 50); queue_sample(); REQUIRE(head == tail);
    dm_android_touch_event(0, 1, 300, 480); queue_sample();
    REQUIRE(tc.finger[1].active && tc.finger[1].target == DM_TC_JOYSTICK);
    dm_android_touch_event(3, 1, 300, 480); queue_sample();

    dm_tc_target main0 = tc.layout.target[DM_TC_MAIN]; float joy0 = tc.layout.joy_x;
    dm_android_touch_resize(1000, 500, 60, 150, 40);
    queue_sample();
    REQUIRE(vr_x == 100 && vr_w == 800);
    REQUIRE(seen_safe_left == 0 && seen_safe_right == (int)lroundf(50 * 2160.0f / 350));
    REQUIRE(fabsf(main0.x - tc.layout.target[DM_TC_MAIN].x - 50) < 0.01f);
    REQUIRE(tc.layout.joy_x == joy0);
    dm_android_touch_event(0, 1, 300, 480); queue_sample();
    REQUIRE(tc.finger[1].active && tc.finger[1].target == DM_TC_EDGE);
    dm_android_touch_event(3, 1, 300, 480); queue_sample();

    test_full = 1; dm_android_touch_resize(1000, 500, 0, 0, 0); queue_sample();
    REQUIRE(seen_safe_left == 0 && seen_safe_right == 0);
}

static void test_android_drop_haptic_handoff(void) {
    fixture(0); uint64_t buttons = 0; int32_t lx = 0, ly = 0, rx = 0, ry = 0;
    memset(test_haptic_handed, 0, sizeof test_haptic_handed); test_haptic_calls = 0;
    dm_android_touch_step(test_clock_ns, &buttons, &lx, &ly, &rx, &ry);
    REQUIRE(test_haptic_calls == 0);
    test_haptic_pending = 1;
    dm_android_touch_step(test_clock_ns, &buttons, &lx, &ly, &rx, &ry);
    dm_android_touch_step(test_clock_ns, &buttons, &lx, &ly, &rx, &ry);
    REQUIRE(test_haptic_calls == 1 && test_haptic_handed[1] == 1 && test_haptic_pending == 0);
}


static void test_android_world_tap_fullscreen(void) {
    fixture(0); dm_android_touch_resize(1000, 500, 0, 0, 0); dm_android_touch_view(100, 150, 800, 350); queue_sample();
    test_full = 1; world_tap_ok = 1; map_tap_ok = 1;
    tc.world_tap_x = 500; tc.world_tap_y = 200; ++tc.world_tap_seq; requests();
    REQUIRE(fabsf(tap_ux - 500.0f / 800) < 1e-5f && fabsf(tap_uy - 200.0f / 350) < 1e-5f);
    REQUIRE(fabsf(tap_wx - 600.0f / 1000) < 1e-5f && fabsf(tap_wy - 350.0f / 500) < 1e-5f);
    test_full = 0;
    ++tc.world_tap_seq; requests();
    REQUIRE(fabsf(tap_wx - 500.0f / 800) < 1e-5f && fabsf(tap_wy - 200.0f / 350) < 1e-5f);
    test_full = 1; test_ui_h = test_real_h; ++tc.world_tap_seq; requests();
    REQUIRE(fabsf(tap_wx - 500.0f / 800) < 1e-5f && fabsf(tap_wy - 200.0f / 350) < 1e-5f);
    test_ui_h = 350;
    direct[0] = (direct_gesture){2, 500, 250, 100.0, 0};
    dm_android_touch_event(2, 0, 600, 400); queue_sample();
    REQUIRE(map_world_pending == 1);
    REQUIRE(fabsf(map_wx - 600.0f / 1000) < 1e-5f && fabsf(map_wy - 400.0f / 500) < 1e-5f);
    world_tap_ok = 0; map_tap_ok = 0; test_full = 0;
}

static void run_case(const char *name, void (*body)(void)) {
    case_name = name; body(); ++cases; printf("PASS %s\n", name);
}

int main(void) {
    puts("SCOPE native_layout_and_input_bridge; no_Java_Canvas_no_guest_no_save_access");
    run_case("idle_185_abi", test_idle_and_abi);
    run_case("stick_deflection", test_stick_deflection);
    run_case("press_release_multitouch", test_press_release_and_multitouch);
    run_case("slide_cancelled_finger", test_slide_cancel);
    run_case("release_pulse_highlight", test_release_pulse);
    run_case("swap_and_delayed_pulse", test_swap_and_delayed_pulse);
    run_case("difficulty_hit_and_close", test_difficulty_hit_and_close);
    run_case("belt_diff_target_no_false_highlight", test_belt_diff_target_does_not_light_difficulty);
    run_case("navigation_and_resize", test_navigation_and_resize);
    run_case("android_view_rect_coordinates", test_android_view_rect_coordinates);
    run_case("android_world_tap_fullscreen", test_android_world_tap_fullscreen);
    run_case("android_drop_haptic_handoff", test_android_drop_haptic_handoff);
    run_case("android_queue_ten_short_taps", test_android_queue_ten_short_taps);
    run_case("android_focus_cancel", test_android_focus_cancel);
    run_case("android_cancelled_latch", test_android_cancelled_latch);
    run_case("android_swap_release_cancel", test_android_swap_release_cancel);
    run_case("android_quiet_release", test_android_quiet_release);
    run_case("android_panel_observation", test_android_panel_observation);
    run_case("android_padmode_rebuild_world", test_android_padmode_rebuild_world);
    printf("RESULT PASS cases=%u assertions=%u; game_quick_taps_10_of_10=NOT_TESTED\n", cases, checks);
    return 0;
}
