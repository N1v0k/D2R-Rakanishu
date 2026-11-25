





#import <Cocoa/Cocoa.h>
#include <sys/sysctl.h>
#import <QuartzCore/QuartzCore.h>
#import <Metal/Metal.h>
#import <ImageIO/ImageIO.h>
#include "dm.h"
#include "dm_input.h"
#include "dm_uiscreen.h"
#include "touch_mac.h"
#import "../ios/Sources/dm_rift_hud.h"
void mtl_set_ui_rect_fn(void (*fn)(double *y, double *h));
static void ui_rect_frac(double *fy, double *fh) { int x, y, w, h, rw = 0, rh = 0; dm_ui_rect(&x, &y, &w, &h); if (dm_screen_real(&rw, &rh) && rh > 0) { *fy = (double)y / rh; *fh = (double)h / rh; } }
__attribute__((constructor)) static void ui_shift_reg(void) { mtl_set_ui_rect_fn(ui_rect_frac); }
#include "front.h"
#include "hle.h"
#include <signal.h>

void dm_touch_host_live_enable(void); void dm_touch_host_live(int op, float nx, float ny);
static int g_live;
static CAMetalLayer *g_layer;
static NSWindow *g_win;
static NSView *g_view;








static double g_aspect = 16.0 / 9;
static int g_full;
static void full_setup(void) {
    if (!getenv("DM_APP") || getenv("DM_GAME_RES")) return;
    NSScreen *sc = NSScreen.mainScreen; double w = sc.frame.size.width, h = sc.frame.size.height - sc.safeAreaInsets.top; const char *e = getenv("DM_TEST_SCREEN_ASPECT"); unsigned tw, th;
    if (e && sscanf(e, "%ux%u", &tw, &th) == 2 && tw && th) { w = tw; h = th; }
    if (h <= 0 || w / h >= 16.0 / 9 - 0.01) { LOG("画面：主屏 %.0fx%.0f 不比 16:9 窄 → 全程 16:9", w, h); return; }
    g_aspect = w / h; g_full = 1; dm_uiscreen_enable(DM_UISCREEN_BOTTOM);
    LOG("画面：主屏全屏内容区 %.0fx%.0f（比例 %.3f）比 16:9 窄 → 真全屏：窗口锁这个比例，局内铺满、界面贴底，前端 16:9 留边", w, h, g_aspect);
}


static NSRect game_rect(void) {
    NSSize b = g_view.bounds.size; CGFloat w = b.width, h = b.height;
    if (fabs(w / h / g_aspect - 1) < 0.003) { }
    else if (w > h * g_aspect) w = round(h * g_aspect); else h = round(w / g_aspect);
    return NSMakeRect(floor((b.width - w) / 2), floor((b.height - h) / 2), w, h);
}
static void layout_game(void) {
    NSRect r = game_rect(); if (NSEqualRects(r, g_layer.frame)) return;
    CGFloat sc = g_view.window.backingScaleFactor ?: 2;
    [CATransaction begin]; [CATransaction setDisableActions:YES]; g_layer.frame = r; g_layer.contentsScale = sc; g_layer.drawableSize = CGSizeMake(round(r.size.width * sc), round(r.size.height * sc)); [CATransaction commit];
    LOG("窗口：内容区 %.0f×%.0f，游戏画面 %.0f×%.0f 放在 (%.0f,%.0f)", g_view.bounds.size.width, g_view.bounds.size.height, r.size.width, r.size.height, r.origin.x, r.origin.y);
}




static void app_quit(const char *why) {
    static int once; if (once++) return;
    { uint64_t dm_audio_render_calls(void); LOG("窗口：退出（%s）；声卡回调累计 %llu（0 = 这次没出声）", why, (unsigned long long)dm_audio_render_calls()); }
    int dm_ingame_now(void); int dm_save_wait(double max_s, uint64_t *commits, int *froze, double *ms);
    if (!dm_ingame_now() && !dm_uiscreen_ingame()) { LOG("窗口：退出（不在局内，没有待存的进度）"); dm_host_shutdown(why); return; }
    uint64_t n = 0; int froze = 0; double ms = 0; int asked = dm_save_wait(3, &n, &froze, &ms);
    if (n && asked) LOG("窗口：退出前等游戏存盘 %.0f ms，期间存档提交 %llu 次%s", ms, (unsigned long long)n, froze ? "（原先暂停着：冻结世界存盘、已恢复暂停）" : "");
    else LOG("【红】窗口：未存盘退出——3 秒内没等到存档提交（%s%s）", asked ? "已请游戏存盘" : "一直没到能请存盘的状态（读取画面？）", froze ? "，原先暂停着、冻结放开过" : "");
    { int dm_cloud_flush_wait(double sec); dm_cloud_flush_wait(10); }
    dm_host_shutdown(why);
}
static const char *ui(const char *k) { const char *dm_ui(const char *key); return dm_ui(k); }
static void show_keys(void) { NSAlert *a = [NSAlert new]; a.messageText = @(ui("dmAppKeysTitle")); a.informativeText = @(ui("dmAppKeysText")); [a addButtonWithTitle:@(ui("dmAppOK"))]; [a runModal]; }






static NSString *share_logs(NSString *to) {
    NSFileManager *fm = NSFileManager.defaultManager; NSString *home = NSHomeDirectory(), *logd = [home stringByAppendingPathComponent:@"Library/Logs/D2R Mobile"];
    NSDictionary *inf = NSBundle.mainBundle.infoDictionary; NSString *build = [inf[@"CFBundleVersion"] description] ?: @"0", *ver = [inf[@"CFBundleShortVersionString"] description] ?: @"?";
    NSDateFormatter *df = [NSDateFormatter new]; df.dateFormat = @"yyyyMMdd-HHmm"; NSString *name = [NSString stringWithFormat:@"D2RMobile-logs-%@-%@", build, [df stringFromDate:NSDate.date]];
    NSString *tmp = [NSTemporaryDirectory() stringByAppendingPathComponent:[NSString stringWithFormat:@"dm_share_%d", getpid()]], *dir = [tmp stringByAppendingPathComponent:name];
    [fm removeItemAtPath:tmp error:nil]; if (![fm createDirectoryAtPath:dir withIntermediateDirectories:YES attributes:nil error:nil]) return @"mkdir";
    fflush(stderr);
    NSData *hb = [home dataUsingEncoding:NSUTF8StringEncoding]; NSData *tilde = [@"~" dataUsingEncoding:NSUTF8StringEncoding];
    NSData *(^scrub)(NSData *) = ^NSData *(NSData *d) { if (!d || hb.length < 2) return d; NSMutableData *o = [NSMutableData dataWithCapacity:d.length]; NSRange all = NSMakeRange(0, d.length), r;
        while ((r = [d rangeOfData:hb options:0 range:all]).location != NSNotFound) { [o appendData:[d subdataWithRange:NSMakeRange(all.location, r.location - all.location)]]; [o appendData:tilde]; all = NSMakeRange(NSMaxRange(r), d.length - NSMaxRange(r)); }
        [o appendData:[d subdataWithRange:all]]; return o; };
    int n = 0; for (NSString *f in @[@"host.log", @"host.prev.log"]) { NSData *d = [NSData dataWithContentsOfFile:[logd stringByAppendingPathComponent:f] options:NSDataReadingMappedIfSafe error:nil]; if (d) { [scrub(d) writeToFile:[dir stringByAppendingPathComponent:f] atomically:YES]; n++; } }
    NSString *bt = [NSString stringWithContentsOfFile:[NSBundle.mainBundle.resourcePath stringByAppendingPathComponent:@"build.txt"] encoding:NSUTF8StringEncoding error:nil] ?: @"";
    char hw[64] = "?"; size_t hn = sizeof hw; sysctlbyname("hw.model", hw, &hn, NULL, 0); NSProcessInfo *pi = NSProcessInfo.processInfo;
    NSString *info = [NSString stringWithFormat:@"%@\nD2R Mobile (macOS) %@ (%@)\n%@\n%@\nmacOS %@\n%@  %.0f GB\n%@\n", @(ui("dmAppShareInfo")), ver, build, inf[@"CFBundleIdentifier"] ?: @"", [bt stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet],
        pi.operatingSystemVersionString, @(hw), pi.physicalMemory / 1073741824.0, [NSLocale.preferredLanguages componentsJoinedByString:@","]];
    [[scrub([info dataUsingEncoding:NSUTF8StringEncoding]) copy] writeToFile:[dir stringByAppendingPathComponent:@"info.txt"] atomically:YES];
    NSString *zip = [tmp stringByAppendingPathComponent:[name stringByAppendingString:@".zip"]];
    NSTask *t = [NSTask new]; t.executableURL = [NSURL fileURLWithPath:@"/usr/bin/ditto"]; t.arguments = @[@"-c", @"-k", @"--keepParent", dir, zip]; NSError *err = nil;
    if (![t launchAndReturnError:&err]) { [fm removeItemAtPath:tmp error:nil]; return err.localizedDescription ?: @"ditto"; } [t waitUntilExit];
    if (t.terminationStatus != 0 || ![fm fileExistsAtPath:zip]) { [fm removeItemAtPath:tmp error:nil]; return [NSString stringWithFormat:@"ditto %d", t.terminationStatus]; }
    NSURL *dst = to ? [NSURL fileURLWithPath:to] : nil;
    if (!dst) { NSSavePanel *sp = [NSSavePanel savePanel]; sp.nameFieldStringValue = zip.lastPathComponent; sp.directoryURL = [fm URLsForDirectory:NSDesktopDirectory inDomains:NSUserDomainMask].firstObject; sp.canCreateDirectories = YES;
        if ([sp runModal] != NSModalResponseOK) { [fm removeItemAtPath:tmp error:nil]; LOG("分享日志：玩家取消"); return nil; } dst = sp.URL; }
    [fm removeItemAtURL:dst error:nil]; BOOL ok = [fm moveItemAtPath:zip toPath:dst.path error:&err]; [fm removeItemAtPath:tmp error:nil];
    if (!ok) return err.localizedDescription ?: @"move";
    LOG("分享日志：已存（日志 %d 份 + 版本信息，%llu 字节）", n, [[fm attributesOfItemAtPath:dst.path error:nil] fileSize]);
    if (!to) [NSWorkspace.sharedWorkspace activateFileViewerSelectingURLs:@[dst]];
    return nil;
}
static void share_logs_ui(void) { NSString *e = share_logs(nil); if (e) { LOG("【红】分享日志：打包失败 %s", e.UTF8String); NSAlert *a = [NSAlert new]; a.messageText = @(ui("dmAppShareFail")); a.informativeText = e; [a addButtonWithTitle:@(ui("dmAppOK"))]; [a runModal]; } }
@interface DmWinCtl : NSObject <NSWindowDelegate> @end




static pthread_mutex_t g_scr_mu = PTHREAD_MUTEX_INITIALIZER; static double g_scr_hz; static char g_scr_name[128];
static void screen_note(void) {
    NSScreen *sc = g_win.screen ?: NSScreen.mainScreen; if (!sc) return;
    CGDirectDisplayID id_ = [sc.deviceDescription[@"NSScreenNumber"] unsignedIntValue];
    CGDisplayModeRef m = CGDisplayCopyDisplayMode(id_); double hz = m ? CGDisplayModeGetRefreshRate(m) : 0; if (m) CGDisplayModeRelease(m);
    if (hz <= 0) hz = (double)sc.maximumFramesPerSecond;
    pthread_mutex_lock(&g_scr_mu); int same = hz == g_scr_hz && !strcmp(g_scr_name, sc.localizedName.UTF8String ?: "");
    g_scr_hz = hz; snprintf(g_scr_name, sizeof g_scr_name, "%s", sc.localizedName.UTF8String ?: "?"); pthread_mutex_unlock(&g_scr_mu);
    if (!same) LOG("屏幕：%s，刷新率 %.2f Hz（最高 %ld 帧）", g_scr_name, hz, (long)sc.maximumFramesPerSecond);
}
double front_display_refresh(char *name, size_t n) {
    pthread_mutex_lock(&g_scr_mu); double hz = g_scr_hz; if (name && n) snprintf(name, n, "%s", g_scr_name); pthread_mutex_unlock(&g_scr_mu); return hz;
}
@implementation DmWinCtl
- (BOOL)windowShouldClose:(NSWindow *)w { (void)w; app_quit("关窗口"); return NO; }
- (void)windowDidResize:(NSNotification *)n { (void)n; layout_game(); }
- (void)windowDidChangeBackingProperties:(NSNotification *)n { (void)n; layout_game(); }
- (void)windowDidChangeScreen:(NSNotification *)n { (void)n; screen_note(); }
- (void)quit:(id)s { (void)s; app_quit("菜单退出"); }
- (void)keys:(id)s { (void)s; show_keys(); }
- (void)share:(id)s { (void)s; share_logs_ui(); }
@end
static uint64_t g_btn;
static int g_w, g_a, g_s, g_d;

void *front_metal_layer(void) { return (__bridge void *)g_layer; }


uint64_t front_game_res(int docked) {
    unsigned w, h; const char *e = getenv("DM_GAME_RES");
    if (e && sscanf(e, "%ux%u", &w, &h) == 2) return docked ? (uint64_t)h << 32 | w : (uint64_t)(h * 4 / 5) << 32 | (w * 4 / 5 + 3) / 4 * 4;
    if (g_full) return docked ? (uint64_t)(llround(1600 / g_aspect / 2) * 2) << 32 | 1600 : 720ull << 32 | (uint64_t)(llround(720 * g_aspect / 8) * 8);
    return docked ? 900ull << 32 | 1600 : 720ull << 32 | 1280;
}

static NSPoint g_test_last;
static int g_wheel;
static void push(void) {
    int32_t lx = (g_d - g_a) * 32767, ly = (g_w - g_s) * 32767;
    dm_input_set(g_btn, lx, ly, 0, g_wheel * 32767);
}
static int key_bit(unsigned short code) {
    switch (code) {
        case 38: return 0;  case 40: return 1;  case 32: return 2;  case 34: return 3;
        case 3: return 4;   case 5: return 5;
        case 12: return 6;  case 14: return 7;  case 6: return 8;   case 8: return 9;
        case 36: return g_live ? 40 : 10; case 48: return 11;
        case 24: case 69: return 10;
        case 49: return 0;
        case 53: return g_live ? 41 : -1;
        case 123: return 12; case 126: return 13; case 124: return 14; case 125: return 15;
    }
    return -1;
}
static void on_key(NSEvent *e, int down) {
    unsigned short c = e.keyCode;
    switch (c) { case 13: g_w = down; break; case 0: g_a = down; break; case 1: g_s = down; break; case 2: g_d = down; break; }
    int b = key_bit(c);
    if (down && (b >= 0 || c == 13 || c <= 2)) dm_padmode_request_glyph(DM_GLYPH_KBD);
    if (b >= 0) g_btn = down ? (g_btn | 1ull << b) : (g_btn & ~(1ull << b));
    push();
}

typedef struct { void (*fn)(void); } guest_arg;
static void *guest_tramp(void *p) {
    static char alt[65536];
    stack_t ss = {.ss_sp = alt, .ss_size = sizeof alt}; sigaltstack(&ss, 0);
    ((guest_arg *)p)->fn(); LOG("客体线程结束"); exit(0);
}

void front_run(void (*guest)(void)) {
    if (!getenv("DM_WINDOW")) { guest(); return; }
    @autoreleasepool {
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        full_setup();
        NSRect r = NSMakeRect(0, 0, 1280, floor(1280 / g_aspect));
        NSWindow *win = [[NSWindow alloc] initWithContentRect:r
                                                    styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
                                                      backing:NSBackingStoreBuffered defer:NO];
        win.title = @"D2R Mobile"; win.restorable = NO; g_win = win;
        NSView *v = win.contentView; g_view = v;
        v.wantsLayer = YES; v.layer.backgroundColor = CGColorGetConstantColor(kCGColorBlack);
        win.collectionBehavior = NSWindowCollectionBehaviorFullScreenPrimary; win.contentMinSize = NSMakeSize(640, floor(640 / g_aspect)); if (g_full) win.contentAspectRatio = NSMakeSize(1280, 1280 / g_aspect);
        static DmWinCtl *ctl; ctl = [DmWinCtl new]; win.delegate = ctl;
        g_layer = [CAMetalLayer layer];
        g_layer.device = MTLCreateSystemDefaultDevice();
        g_layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
        g_layer.contentsScale = win.backingScaleFactor;
        [v.layer addSublayer:g_layer]; layout_game();




        NSTimer *riftHUDTimer = [NSTimer timerWithTimeInterval:1.0 / 30 repeats:YES block:^(NSTimer *timer) {
            (void)timer;
            BOOL hide = !dm_uiscreen_ingame() || (g_live && !dm_touch_host_world_active());
            dm_rift_hud_draw(v, NSRectToCGRect(game_rect()), hide);
        }];
        [NSRunLoop.mainRunLoop addTimer:riftHUDTimer forMode:NSRunLoopCommonModes];
        [win center];
        g_live = getenv("DM_APP") || getenv("DM_TOUCH_LIVE"); if (g_live) dm_touch_host_live_enable();
        if (g_live) {
            win.acceptsMouseMovedEvents = YES;
            [v addTrackingArea:[[NSTrackingArea alloc] initWithRect:NSZeroRect options:NSTrackingMouseEnteredAndExited | NSTrackingActiveInKeyWindow | NSTrackingInVisibleRect owner:v userInfo:nil]];
        }
        {
          NSMenu *bar = [NSMenu new];
          NSMenu *(^add)(const char *) = ^NSMenu *(const char *t) { NSMenuItem *it = [NSMenuItem new]; [bar addItem:it]; NSMenu *m = [[NSMenu alloc] initWithTitle:@(t)]; it.submenu = m; return m; };
          [[add("") addItemWithTitle:@(ui("dmAppQuit")) action:@selector(quit:) keyEquivalent:@"q"] setTarget:ctl];
          NSMenuItem *fs = [add(ui("dmAppMenuView")) addItemWithTitle:@(ui("dmAppFullscreen")) action:@selector(toggleFullScreen:) keyEquivalent:@"f"]; fs.keyEquivalentModifierMask = NSEventModifierFlagControl | NSEventModifierFlagCommand;
          { NSMenu *hm = add(ui("dmAppMenuHelp")); [[hm addItemWithTitle:@(ui("dmAppKeysTitle")) action:@selector(keys:) keyEquivalent:@""] setTarget:ctl]; [[hm addItemWithTitle:@(ui("dmAppShareLog")) action:@selector(share:) keyEquivalent:@""] setTarget:ctl]; }
          NSApp.mainMenu = bar; }
        { void dm_gamepad_start(void (*on_mode)(int pad)); dm_gamepad_start(NULL); }
        [win makeKeyAndOrderFront:nil];
        screen_note();
        [NSNotificationCenter.defaultCenter addObserverForName:NSApplicationDidChangeScreenParametersNotification object:nil queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification *n) { (void)n; screen_note(); }];
        [NSApp activateIgnoringOtherApps:YES];
        [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskKeyDown | NSEventMaskKeyUp handler:^NSEvent *(NSEvent *e) {
            if (e.window != win) return e;
            if (e.modifierFlags & NSEventModifierFlagCommand) return e;
            if (!e.isARepeat) on_key(e, e.type == NSEventTypeKeyDown);
            return nil;
        }];


        [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskLeftMouseDown | NSEventMaskLeftMouseDragged | NSEventMaskLeftMouseUp | NSEventMaskMouseMoved | NSEventMaskMouseExited handler:^NSEvent *(NSEvent *e) {
            if (e.window != win) return e;
            BOOL hover = e.type == NSEventTypeMouseMoved || e.type == NSEventTypeMouseExited;
            if (hover && getenv("DM_TEST_HOVER_LOG")) LOG("鼠标悬停输入：类型 %ld", (long)e.type);
            if (hover && !g_live) return e;
            if (e.type == NSEventTypeMouseExited) { dm_touch_host_live(3, -1, -1); return e; }
            if (e.type == NSEventTypeLeftMouseUp) { if (g_live) dm_touch_host_live(2, 0, 0); else dm_touch_set(NULL, 0); return nil; }
            NSPoint p = [v convertPoint:e.locationInWindow fromView:nil]; NSRect gr = game_rect();
            if (hover && getenv("DM_TEST_HOVER_LOG")) LOG("鼠标悬停坐标：窗口 %.1f,%.1f 视图 %.1f,%.1f 画面 %.1f,%.1f %.1fx%.1f", e.locationInWindow.x, e.locationInWindow.y, p.x, p.y, gr.origin.x, gr.origin.y, gr.size.width, gr.size.height);
            if (gr.size.width <= 0 || gr.size.height <= 0) return e;
            float nx = (float)((p.x - gr.origin.x) / gr.size.width), ny = (float)(1.0 - (p.y - gr.origin.y) / gr.size.height);
            if (e.type == NSEventTypeLeftMouseDown && (nx < 0 || nx > 1 || ny < 0 || ny > 1)) return nil;
            if (g_full && g_live) {




                int rw = 0, rh = 0; double cx = 0, cy = 0, cw = 1, ch = 1;
                if (dm_screen_real(&rw, &rh) && rw > 0 && rh > 0) { double ca = (double)rw / rh, ga = gr.size.width / gr.size.height;
                    if (fabs(ga / ca - 1) >= 0.01) { if (ga > ca) { cw = ca / ga; cx = (1 - cw) / 2; } else { ch = ga / ca; cy = (1 - ch) / 2; } } }
                float vx = (float)((nx - cx) / cw), vy = (float)((ny - cy) / ch);
                double fy = 0, fh = 1; ui_rect_frac(&fy, &fh); float uy = fh > 0 ? (float)((vy - fy) / fh) : vy;
                if (e.type == NSEventTypeLeftMouseDown && !dm_uiscreen_ingame() && (vx < 0 || vx > 1 || uy < 0 || uy > 1)) return nil;
                dm_touch_host_live(hover ? 3 : e.type == NSEventTypeLeftMouseDown ? 0 : 1, vx, uy); return hover ? e : nil; }
            if (hover) { dm_touch_host_live(3, nx, ny); return e; }
            dm_touch t = {0, nx < 0 ? 0 : nx > 1 ? 1 : nx, ny < 0 ? 0 : ny > 1 ? 1 : ny};
            if (g_live) { dm_touch_host_live(e.type == NSEventTypeLeftMouseDown ? 0 : 1, t.x, t.y); return nil; }
            dm_touch_set(&t, 1); return nil;
        }];


        if (g_live) {
            [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskRightMouseDown | NSEventMaskRightMouseUp handler:^NSEvent *(NSEvent *e) {
                if (e.window != win) return e;
                if (e.type == NSEventTypeRightMouseDown) g_btn |= DM_BTN_B; else g_btn &= ~(uint64_t)DM_BTN_B;
                push(); return nil;
            }];
            [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskScrollWheel handler:^NSEvent *(NSEvent *e) {
                if (e.window != win || e.scrollingDeltaY == 0) return e;
                static int gen; int my = ++gen; g_wheel = e.scrollingDeltaY > 0 ? 1 : -1; push();
                dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.12 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{ if (my == gen) { g_wheel = 0; push(); } });
                return nil;
            }];






            { id pref = [NSUserDefaults.standardUserDefaults objectForKey:@"CursorHideSeconds"];
              double idle = [pref respondsToSelector:@selector(doubleValue)] ? [pref doubleValue] : 3.0;
              if (idle > 0 && isfinite(idle)) { __block NSPoint last = NSMakePoint(NAN, NAN); __block double since = 0; __block BOOL hid = NO;
                  LOG("鼠标指针：静止 %.1f 秒自动隐藏 | pointer auto-hides after %.1f s at rest", idle, idle);
                  [NSTimer scheduledTimerWithTimeInterval:0.25 repeats:YES block:^(NSTimer *t) { (void)t;
                      NSPoint m = NSEvent.mouseLocation; double now = NSProcessInfo.processInfo.systemUptime;
                      if (!NSEqualPoints(m, last) || NSEvent.pressedMouseButtons || !NSApp.isActive || !win.isKeyWindow) { last = m; since = now; hid = NO; return; }
                      if (hid || now - since < idle || !NSPointInRect(m, [win convertRectToScreen:v.frame])) return;
                      [NSCursor setHiddenUntilMouseMoves:YES]; hid = YES; }]; } }
        }
        if (getenv("DM_APP") && !getenv("DM_APP_ROMFS") && ![NSUserDefaults.standardUserDefaults boolForKey:@"KeysShown"]) { show_keys(); [NSUserDefaults.standardUserDefaults setBool:YES forKey:@"KeysShown"]; }
        { const char *e;
          int w, h; if ((e = getenv("DM_TEST_WINSIZE")) && sscanf(e, "%dx%d", &w, &h) == 2) { [win setContentSize:NSMakeSize(w, h)]; layout_game(); }
          if ((e = getenv("DM_TEST_FULLSCREEN_AT"))) for (const char *q = e; q && *q; q = strchr(q, ';') ? strchr(q, ';') + 1 : NULL)
              dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(atof(q) * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{ [win toggleFullScreen:nil]; });




          if ((e = getenv("DM_TEST_PRESENT_SHOT"))) { void mtl_grab_next_present(void (^cb)(CGImageRef img));
              for (NSString *it in [@(e) componentsSeparatedByString:@";"]) { NSRange c = [it rangeOfString:@":"]; if (c.location == NSNotFound) continue;
                  double at = [it substringToIndex:c.location].doubleValue; NSString *path = [it substringFromIndex:c.location + 1];
                  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(at * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
                      NSSize vb = v.bounds.size; NSRect gr = game_rect(); double fy = 0, fh = 1; ui_rect_frac(&fy, &fh);
                      mtl_grab_next_present(^(CGImageRef img) { if (!img) { LOG("【红】DM_TEST_PRESENT_SHOT：%s 没取到", path.UTF8String); return; }
                          CGImageDestinationRef d = CGImageDestinationCreateWithURL((__bridge CFURLRef)[NSURL fileURLWithPath:path], (__bridge CFStringRef)@"public.png", 1, NULL);
                          if (d) { CGImageDestinationAddImage(d, img, NULL); CGImageDestinationFinalize(d); CFRelease(d); }
                          LOG("DM_TEST_PRESENT_SHOT：%s %zux%zu（内容区 %.0fx%.0f，游戏画面 %.0fx%.0f 在 (%.0f,%.0f)，界面矩形纵向 %.3f 高 %.3f，%s）", path.UTF8String, CGImageGetWidth(img), CGImageGetHeight(img),
                              vb.width, vb.height, gr.size.width, gr.size.height, gr.origin.x, gr.origin.y, fy, fh, dm_uiscreen_ingame() ? "局内" : "局外"); }); }); } }








          if ((e = getenv("DM_TEST_NSMOUSE"))) { NSString *txt = [NSString stringWithContentsOfFile:@(e) encoding:NSUTF8StringEncoding error:nil]; NSTimeInterval t0 = NSProcessInfo.processInfo.systemUptime; int n = 0;
              dispatch_queue_t tq = dispatch_queue_create("dm.test.nsevent", DISPATCH_QUEUE_SERIAL);
              for (NSString *ln in [txt componentsSeparatedByString:@"\n"]) { double sec, x = 0, y = 0; char op[8];
                  if (sscanf(ln.UTF8String, "%lf %7s %lf,%lf", &sec, op, &x, &y) < 2 || ln.UTF8String[0] == '#') continue; n++;
                  const char *at = strchr(ln.UTF8String, '@'); double anc = at && at[1] == 'c' ? 0.5 : at && at[1] == 'b' ? 1 : 0;
                  if (!strcmp(op, "key") || !strcmp(op, "type")) {


                      int ty = !strcmp(op, "type"); char word[64] = ""; if (ty) sscanf(ln.UTF8String, "%*lf %*7s %63s", word); NSString *chars = ty ? @(word) : (x == 36 ? @"\r" : @"");
                      if (ty) {
                          dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(sec * NSEC_PER_SEC)), tq, ^{ CFRunLoopPerformBlock(CFRunLoopGetMain(), kCFRunLoopCommonModes, ^{
                              NSWindow *kw = NSApp.keyWindow; id fr = kw.firstResponder; BOOL ok = [NSApp sendAction:@selector(insertText:) to:nil from:chars];
                              LOG("DM_TEST_NSMOUSE：%.2f 秒打字「%s」（键窗口%s，第一响应者 %s，%s）", NSProcessInfo.processInfo.systemUptime - t0, chars.UTF8String, kw == win ? "是游戏窗口" : kw ? "是弹框" : "没有", fr ? NSStringFromClass([fr class]).UTF8String : "无", ok ? "有人收" : "没人收"); });
                              CFRunLoopWakeUp(CFRunLoopGetMain()); });
                          continue; }
                      for (NSUInteger ci = 0; ci < (ty ? chars.length : 1); ci++) { NSString *one = ty ? [chars substringWithRange:NSMakeRange(ci, 1)] : chars; double at = sec + ci * 0.15;
                          for (int dn = 1; dn >= 0; dn--) dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)((at + (dn ? 0 : 0.08)) * NSEC_PER_SEC)), tq, ^{ NSWindow *kw = NSApp.keyWindow;
                              if (dn) LOG("DM_TEST_NSMOUSE：%.2f 秒投 键盘 %s%s（键窗口%s）", NSProcessInfo.processInfo.systemUptime - t0, ty ? "字 " : "键码 ", ty ? one.UTF8String : [NSString stringWithFormat:@"%d", (int)x].UTF8String, kw == win ? "是游戏窗口" : kw ? "是别的窗口（弹框）" : "没有：App 里没有键窗口");
                              [NSApp postEvent:[NSEvent keyEventWithType:dn ? NSEventTypeKeyDown : NSEventTypeKeyUp location:NSZeroPoint modifierFlags:0 timestamp:NSProcessInfo.processInfo.systemUptime windowNumber:(kw ?: win).windowNumber context:nil characters:one charactersIgnoringModifiers:one isARepeat:NO keyCode:ty ? 0 : (unsigned short)x] atStart:NO]; }); }
                      continue; }
                  if (!strcmp(op, "unit") || !strcmp(op, "unith")) {
                      double hold = !strcmp(op, "unith") ? 2.5 : 0.1;
                      dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(sec * NSEC_PER_SEC)), tq, ^{ void dm_world_watch(int, uint32_t); int dm_world_watch_pos(float *, float *); float nx = 0, ny = 0; int ok = 0;
                          dm_world_watch((int)x, (uint32_t)y); for (int i = 0; i < 60 && !(ok = dm_world_watch_pos(&nx, &ny)); i++) usleep(50000); dm_world_watch(-1, 0);
                          if (!ok) { LOG("DM_TEST_NSMOUSE：unit %d,%u 3 秒内没进画面，不点", (int)x, (unsigned)y); return; }
                          __block NSPoint pt; dispatch_sync(dispatch_get_main_queue(), ^{ pt = [v convertPoint:NSMakePoint(nx * v.bounds.size.width, (1 - ny) * v.bounds.size.height) toView:nil]; });
                          LOG("DM_TEST_NSMOUSE：%.2f 秒 unit %d,%u 在画面 (%.3f,%.3f) → 投按下，%.1f 秒后抬起", NSProcessInfo.processInfo.systemUptime - t0, (int)x, (unsigned)y, nx, ny, hold);
                          for (int dn = 1; dn >= 0; dn--) { if (!dn) usleep((useconds_t)(hold * 1e6));
                              [NSApp postEvent:[NSEvent mouseEventWithType:dn ? NSEventTypeLeftMouseDown : NSEventTypeLeftMouseUp location:pt modifierFlags:0 timestamp:NSProcessInfo.processInfo.systemUptime windowNumber:win.windowNumber context:nil eventNumber:0 clickCount:1 pressure:dn ? 1 : 0] atStart:NO]; } });
                      continue; }
                  if (!strcmp(op, "dbl") || !strcmp(op, "dragto")) {


                      int dbl = !strcmp(op, "dbl");
                      dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(sec * NSEC_PER_SEC)), tq, ^{ __block NSPoint from, to; dispatch_sync(dispatch_get_main_queue(), ^{ from = g_test_last; to = NSMakePoint(x, v.bounds.size.height - (y + anc * (v.bounds.size.height - 720))); if (dbl) from = to; g_test_last = to; });
                          NSEvent *(^mk)(NSEventType, NSPoint, int) = ^(NSEventType t, NSPoint p, int cc) { __block NSPoint wp; dispatch_sync(dispatch_get_main_queue(), ^{ wp = [v convertPoint:p toView:nil]; });
                              return [NSEvent mouseEventWithType:t location:wp modifierFlags:0 timestamp:NSProcessInfo.processInfo.systemUptime windowNumber:win.windowNumber context:nil eventNumber:0 clickCount:cc pressure:t == NSEventTypeLeftMouseUp ? 0 : 1]; };
                          LOG("DM_TEST_NSMOUSE：%.2f 秒 %s 内容区 (%.0f,%.0f)", NSProcessInfo.processInfo.systemUptime - t0, dbl ? "双击" : "拖到", to.x, v.bounds.size.height - to.y);
                          if (dbl) { for (int i = 0; i < 2; i++) { [NSApp postEvent:mk(NSEventTypeLeftMouseDown, to, i + 1) atStart:NO]; usleep(60000); [NSApp postEvent:mk(NSEventTypeLeftMouseUp, to, i + 1) atStart:NO]; usleep(150000); } }
                          else { for (int i = 1; i <= 12; i++) { [NSApp postEvent:mk(NSEventTypeLeftMouseDragged, NSMakePoint(from.x + (to.x - from.x) * i / 12, from.y + (to.y - from.y) * i / 12), 1) atStart:NO]; usleep(50000); } usleep(200000); [NSApp postEvent:mk(NSEventTypeLeftMouseUp, to, 1) atStart:NO]; } });
                      continue; }
                  NSEventType ty = !strcmp(op, "hover") ? NSEventTypeMouseMoved : !strcmp(op, "down") ? NSEventTypeLeftMouseDown : !strcmp(op, "up") ? NSEventTypeLeftMouseUp : NSEventTypeLeftMouseDragged; int has = !strcmp(op, "up") ? 0 : 1;
                  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(sec * NSEC_PER_SEC)), tq, ^{ static NSPoint last;
                      if (has) { last = NSMakePoint(x, v.bounds.size.height - (y + anc * (v.bounds.size.height - 720))); g_test_last = last; }
                      NSEvent *ev = [NSEvent mouseEventWithType:ty location:[v convertPoint:last toView:nil] modifierFlags:0 timestamp:NSProcessInfo.processInfo.systemUptime windowNumber:win.windowNumber context:nil eventNumber:0 clickCount:1 pressure:ty == NSEventTypeLeftMouseUp ? 0 : 1];
                      LOG("DM_TEST_NSMOUSE：%.2f 秒投 %s 内容区 (%.0f,%.0f)（窗口%s是主窗口，App %s在前台，背板缩放 %.1f）", NSProcessInfo.processInfo.systemUptime - t0, ty == NSEventTypeMouseMoved ? "悬停" : ty == NSEventTypeLeftMouseDown ? "按下" : ty == NSEventTypeLeftMouseUp ? "抬起" : "拖动", last.x, v.bounds.size.height - last.y, win.isKeyWindow ? "" : "不", NSApp.isActive ? "" : "不", win.backingScaleFactor);
                      [NSApp postEvent:ev atStart:NO]; }); }
              LOG("DM_TEST_NSMOUSE：%d 条排好", n); }
          if ((e = getenv("DM_TEST_SHARELOG"))) { NSString *to = @(e); dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(8 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{ NSString *er = share_logs(to); LOG("DM_TEST_SHARELOG：第 8 秒打包到 %s → %s", to.lastPathComponent.UTF8String, er ? er.UTF8String : "OK"); }); }
          if ((e = getenv("DM_TEST_QUIT_AT"))) dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(atof(e) * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{ app_quit("DM_TEST_QUIT_AT"); }); }


        NSNotificationCenter *nc = NSNotificationCenter.defaultCenter;








        { const char *e = getenv("DM_OCCL"); int on = !(e && !strcmp(e, "0"));
          [nc addObserverForName:NSWindowDidChangeOcclusionStateNotification object:win queue:nil usingBlock:^(NSNotification *n) { (void)n;
              int hidden = !(win.occlusionState & NSWindowOcclusionStateVisible); void mtl_present_discard(int on);
              LOG("前端：窗口%s（遮挡处理%s）", hidden ? "整个被盖住 / 不可见" : "露出来了", on ? "开" : "关 DM_OCCL=0");
              if (on) mtl_present_discard(hidden); }];
          double at = 0, dur = 0;
          if ((e = getenv("DM_TEST_COVER")) && sscanf(e, "%lf,%lf", &at, &dur) == 2 && at > 0 && dur > 0)
              dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(at * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
                  NSWindow *c = [[NSWindow alloc] initWithContentRect:NSInsetRect(win.frame, -40, -40) styleMask:NSWindowStyleMaskBorderless backing:NSBackingStoreBuffered defer:NO];
                  c.opaque = YES; c.backgroundColor = NSColor.grayColor; c.releasedWhenClosed = NO; [c orderFrontRegardless];
                  LOG("DM_TEST_COVER：盖上 %.1f 秒，游戏窗口%s是主窗口", dur, win.isKeyWindow ? "" : "不");
                  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(dur * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{ [c orderOut:nil]; LOG("DM_TEST_COVER：拿开"); });
              }); }
        [nc addObserverForName:NSWindowDidResignKeyNotification object:win queue:nil usingBlock:^(NSNotification *n) { (void)n; if (!win.miniaturized) dm_focus_set(2); g_btn = 0; g_w = g_a = g_s = g_d = 0; push(); { void dm_gamepad_reset(void); dm_gamepad_reset(); } }];
        [nc addObserverForName:NSWindowDidBecomeKeyNotification object:win queue:nil usingBlock:^(NSNotification *n) { (void)n; dm_focus_set(1); }];
        [nc addObserverForName:NSWindowDidMiniaturizeNotification object:win queue:nil usingBlock:^(NSNotification *n) { (void)n; dm_focus_set(3); dm_audio_suspend(1); dm_gfx_suspend(1); }];
        [nc addObserverForName:NSWindowDidDeminiaturizeNotification object:win queue:nil usingBlock:^(NSNotification *n) { (void)n; dm_gfx_suspend(0); dm_audio_suspend(0); dm_focus_set(2); }];


        static guest_arg ga; ga.fn = guest;
        pthread_attr_t a; pthread_attr_init(&a); pthread_attr_setstacksize(&a, 64u << 20);
        pthread_t th; if (pthread_create(&th, &a, guest_tramp, &ga)) die("pthread_create guest");
        [NSApp run];
    }
}



int front_text_input(const char *title, const char *sub, const char *guide, const char *initial, int max_chars, char *out, int out_cap) {
    if (!g_layer) return -1;
    __block int r = 0;
    __block NSString *text = nil;
    dispatch_sync(dispatch_get_main_queue(), ^{
        NSAlert *a = [NSAlert new];
        a.messageText = title && *title ? @(title) : @"输入";
        NSString *info = [NSString stringWithFormat:@"%s%s%s", sub ? sub : "", sub && *sub && guide && *guide ? "\n" : "", guide ? guide : ""];
        a.informativeText = info;
        [a addButtonWithTitle:@"确定"]; [a addButtonWithTitle:@"取消"];
        NSTextField *f = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 300, 24)];
        f.stringValue = initial ? @(initial) : @"";
        if (guide && *guide) f.placeholderString = @(guide);
        a.accessoryView = f; [a layout]; a.window.initialFirstResponder = f;
        f.target = a.buttons.firstObject; f.action = @selector(performClick:);


        CFRunLoopPerformBlock(CFRunLoopGetMain(), kCFRunLoopCommonModes, ^{ [a.window makeFirstResponder:f]; });
        r = [a runModal] == NSAlertFirstButtonReturn;
        [g_win makeKeyAndOrderFront:nil];
        text = f.stringValue;
    });
    if (!r) return 0;
    (void)max_chars;
    snprintf(out, out_cap, "%s", text.UTF8String);
    return 1;
}
