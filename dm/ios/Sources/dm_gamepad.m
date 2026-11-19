





















#import <GameController/GameController.h>
#include "dm_input.h"
#include "dm_padmap.h"
#include "hle.h"

static void (*g_on_mode)(int pad);
static int g_pad_mode, g_need_center;
void dm_padmode_request(int pad);
static void set_mode(int pad) { if (g_pad_mode == pad) return; g_pad_mode = pad; dm_padmode_request(pad); LOG("手柄：%s", pad ? "手柄有输入 → 手柄模式" : "碰了屏幕 / 手柄断开 → 触屏模式"); if (g_on_mode) g_on_mode(pad); }
int dm_gamepad_mode(void) { return g_pad_mode; }



enum { NSLOT = 6 };
static struct { const void *who; uint64_t b; int32_t ax[4]; int glyph; } g_s[NSLOT];
static int g_glyph = DM_GLYPH_PAD;
static int slot_of(const void *who, int make) { int fr = -1; for (int i = 0; i < NSLOT; i++) { if (g_s[i].who == who) return i; if (fr < 0 && !g_s[i].who) fr = i; }
    if (make && fr >= 0) { memset(&g_s[fr], 0, sizeof g_s[fr]); g_s[fr].who = who; return fr; } return -1; }

static int32_t axis(float v) { if (v > 1) v = 1; if (v < -1) v = -1; return (int32_t)lroundf(v * DM_STICK_MAX); }
static int stick_on(int was, int32_t x, int32_t y) { double m = hypot(x, y) / DM_STICK_MAX; return was ? m >= 0.15 : m >= 0.25; }


static void apply(void) {
    uint64_t b = 0; int32_t ax[4] = {0}; double best[2] = {0};
    for (int i = 0; i < NSLOT; i++) { if (!g_s[i].who) continue; b |= g_s[i].b;
        for (int k = 0; k < 2; k++) { double m = hypot(g_s[i].ax[2 * k], g_s[i].ax[2 * k + 1]); if (m > best[k]) { best[k] = m; ax[2 * k] = g_s[i].ax[2 * k]; ax[2 * k + 1] = g_s[i].ax[2 * k + 1]; } } }
    static int on[2]; static uint64_t prev_b;
    on[0] = stick_on(on[0], ax[0], ax[1]); on[1] = stick_on(on[1], ax[2], ax[3]);
    if (!on[0] && !on[1]) g_need_center = 0;
    if ((b & ~prev_b) || ((on[0] || on[1]) && !g_need_center)) { dm_padmode_request_glyph(g_glyph); set_mode(1); }
    prev_b = b;
    if (!g_pad_mode || !on[0]) ax[0] = ax[1] = 0; if (!g_pad_mode || !on[1]) ax[2] = ax[3] = 0;










    { static uint64_t prev; static int mine; void dm_tp_request(void); int dm_front_navigation(void), dm_freetp_zr_free(int zl);
      if ((b & DM_BTN_ZR) && !(prev & DM_BTN_ZR)) { mine = !dm_front_navigation() && dm_freetp_zr_free((b & DM_BTN_ZL) != 0); if (mine) { dm_tp_request(); LOG("手柄：ZR（这一格空着）→ 免费回城"); } }
      prev = b; if (mine) b &= ~(uint64_t)DM_BTN_ZR; if (!(b & DM_BTN_ZR) && !(prev & DM_BTN_ZR)) mine = 0; }
    dm_pad_set(b, ax[0], ax[1], ax[2], ax[3]);
}
static void put(const void *who, uint64_t b, int32_t lx, int32_t ly, int32_t rx, int32_t ry, int glyph) {
    int i = slot_of(who, 1); if (i < 0) return;
    if ((b & ~g_s[i].b) || hypot(lx, ly) >= 0.25 * DM_STICK_MAX || hypot(rx, ry) >= 0.25 * DM_STICK_MAX) g_glyph = glyph;
    g_s[i].b = b; g_s[i].ax[0] = lx; g_s[i].ax[1] = ly; g_s[i].ax[2] = rx; g_s[i].ax[3] = ry; g_s[i].glyph = glyph; apply();
}
static void drop(const void *who) {
    int i = slot_of(who, 0); if (i < 0) return; memset(&g_s[i], 0, sizeof g_s[i]); apply();
    for (int k = 0; k < NSLOT; k++) if (g_s[k].who) return;
    set_mode(0);
}
void dm_gamepad_touched(void) { if (g_pad_mode) { g_need_center = 1; set_mode(0); apply(); } }
void dm_gamepad_reset(void) {
    int any = 0; for (int i = 0; i < NSLOT; i++) if (g_s[i].who) { any |= g_s[i].b != 0 || g_s[i].ax[0] || g_s[i].ax[1] || g_s[i].ax[2] || g_s[i].ax[3]; g_s[i].b = 0; memset(g_s[i].ax, 0, sizeof g_s[i].ax); }
    if (any) { LOG("手柄：失活，手柄的键和摇杆清零"); apply(); }
}



void dm_gamepad_inject_n(int n, uint64_t b, int32_t lx, int32_t ly, int32_t rx, int32_t ry) { put((const void *)(intptr_t)(n == 2 ? 2 : 1), b, lx, ly, rx, ry, DM_GLYPH_PAD); }
void dm_gamepad_drop_n(int n) { LOG("手柄：断开（虚拟手柄 %d）", n == 2 ? 2 : 1); drop((const void *)(intptr_t)(n == 2 ? 2 : 1)); }
void dm_gamepad_inject(uint64_t b, int32_t lx, int32_t ly, int32_t rx, int32_t ry) { dm_gamepad_inject_n(1, b, lx, ly, rx, ry); }
static int is_xbox(GCController *c) { return [c.extendedGamepad isKindOfClass:[GCXboxGamepad class]] || [c.productCategory ?: @"" containsString:@"Xbox"]; }
static int is_ps(GCController *c) { NSString *k = c.productCategory; return [k isEqualToString:GCProductCategoryDualSense] || [k isEqualToString:GCProductCategoryDualShock4]; }
static int glyph_of(GCController *c) { return is_xbox(c) ? DM_GLYPH_XBOX : is_ps(c) ? DM_GLYPH_PS : DM_GLYPH_PAD; }
static void publish(GCController *c, GCExtendedGamepad *g) {
    uint64_t b = 0; int xb = is_xbox(c);
    if (xb) b |= dm_padmap_face_xbox(g.buttonA.pressed, g.buttonB.pressed, g.buttonX.pressed, g.buttonY.pressed, 0);
    else b |= dm_padmap_face(g.buttonA.pressed, g.buttonB.pressed, g.buttonX.pressed, g.buttonY.pressed);
    if (g.leftShoulder.pressed) b |= DM_BTN_L; if (g.rightShoulder.pressed) b |= DM_BTN_R; if (g.leftTrigger.pressed) b |= DM_BTN_ZL; if (g.rightTrigger.pressed) b |= DM_BTN_ZR;
    if (g.leftThumbstickButton.pressed) b |= DM_BTN_LS; if (g.rightThumbstickButton.pressed) b |= DM_BTN_RS;
    if (g.buttonMenu.pressed) b |= DM_BTN_PLUS; if (g.buttonOptions.pressed) b |= DM_BTN_MINUS;
    if (g.dpad.left.pressed) b |= DM_BTN_LEFT; if (g.dpad.up.pressed) b |= DM_BTN_UP; if (g.dpad.right.pressed) b |= DM_BTN_RIGHT; if (g.dpad.down.pressed) b |= DM_BTN_DOWN;
    put((__bridge const void *)c, b, axis(g.leftThumbstick.xAxis.value), axis(g.leftThumbstick.yAxis.value), axis(g.rightThumbstick.xAxis.value), axis(g.rightThumbstick.yAxis.value), glyph_of(c));
}
static void attach(GCController *c) {
    GCExtendedGamepad *g = c.extendedGamepad; if (!g) { LOG("手柄：%s 不是扩展手柄，不接", c.vendorName.UTF8String ?: "?"); return; }
    LOG("手柄：连上 %s，类别 %s → %s", c.vendorName.UTF8String ?: "?", c.productCategory.UTF8String ?: "?", is_xbox(c) ? "Xbox 手柄：面键按位置排（下 A 右 B 上 X 左 Y）、Xbox 键帽" : is_ps(c) ? "面键 buttonA/B/X/Y 直接发 A/B/X/Y、PlayStation 键帽" : "面键 buttonA/B/X/Y 直接发 A/B/X/Y");
    __weak GCController *wc = c;
    g.valueChangedHandler = ^(GCExtendedGamepad *gp, GCControllerElement *el) { (void)el; GCController *sc = wc; if (sc) publish(sc, gp); };
}
void dm_gamepad_start(void (*on_mode)(int pad)) {
    g_on_mode = on_mode;
    NSNotificationCenter *nc = [NSNotificationCenter defaultCenter];
    [nc addObserverForName:GCControllerDidConnectNotification object:nil queue:[NSOperationQueue mainQueue] usingBlock:^(NSNotification *n) { attach(n.object); }];
    [nc addObserverForName:GCControllerDidDisconnectNotification object:nil queue:[NSOperationQueue mainQueue] usingBlock:^(NSNotification *n) {
        LOG("手柄：断开 %s", ((GCController *)n.object).vendorName.UTF8String ?: "?"); drop((__bridge const void *)n.object); }];
    for (GCController *c in GCController.controllers) attach(c);
}
