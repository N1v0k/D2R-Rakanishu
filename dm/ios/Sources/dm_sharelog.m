













#import <UIKit/UIKit.h>
#include <stdatomic.h>
#include <sys/sysctl.h>
#include <pwd.h>
#include <unistd.h>
#include "../../host/dm_i18n.h"
void plog(const char *fmt, ...);
void dm_sharelog_init(void (*cb)(void));



static NSString *last_line(NSData *d, const char *key) {
    if (!d.length) return nil;
    NSData *k = [NSData dataWithBytes:key length:strlen(key)];
    NSRange r = [d rangeOfData:k options:NSDataSearchBackwards range:NSMakeRange(0, d.length)]; if (r.location == NSNotFound) return nil;
    const char *b = d.bytes; NSUInteger s = r.location, e = r.location;
    while (s > 0 && b[s - 1] != '\n') s--;
    while (e < d.length && b[e] != '\n') e++;
    return [[NSString alloc] initWithBytes:b + s length:e - s encoding:NSUTF8StringEncoding] ?: @"（这一行不是合法 UTF-8）";
}
static void perf_block(NSMutableString *h, NSString *title, NSData *d) {
    [h appendFormat:@"\n—— %@ ——\n", title];
    if (!d) { [h appendString:@"（没有这份日志）\n"]; return; }
    static const char *keys[] = { "boot: 版本", "[诊断]", "[perf]", "[nvn] 上屏", "温控档位", "紫闪探针 至出图" };
    for (size_t i = 0; i < sizeof keys / sizeof *keys; i++) { NSString *l = last_line(d, keys[i]); [h appendFormat:@"%@\n", l ?: [NSString stringWithFormat:@"（没有「%@」行）", @(keys[i])]]; }
}


static NSData *scrub_home(NSData *d) {
    struct passwd *pw = getpwuid(getuid()); const char *h = pw ? pw->pw_dir : NULL; size_t hn = h ? strlen(h) : 0;
    if (!d || hn < 2) return d;
    NSMutableData *o = [NSMutableData dataWithCapacity:d.length]; const uint8_t *b = d.bytes; size_t n = d.length, i = 0, from = 0;
    while (i + hn <= n) { if (b[i] == (uint8_t)h[0] && !memcmp(b + i, h, hn)) { [o appendBytes:b + from length:i - from]; [o appendBytes:"~" length:1]; i += hn; from = i; } else i++; }
    [o appendBytes:b + from length:n - from]; return o;
}
static NSDate *g_t0;
static NSURL *build_file(void) {
    NSFileManager *fm = NSFileManager.defaultManager;
    NSString *doc = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES)[0];
    NSData *(^rd)(NSString *) = ^NSData *(NSString *n) { return [NSData dataWithContentsOfFile:[doc stringByAppendingPathComponent:n] options:NSDataReadingMappedIfSafe error:nil]; };
    NSData *cur = rd(@"boot.txt"), *prev = rd(@"boot.previous.txt");
    NSDictionary *inf = NSBundle.mainBundle.infoDictionary; NSString *build = [inf[@"CFBundleVersion"] description] ?: @"未知";
    NSDateFormatter *df = [NSDateFormatter new]; df.dateFormat = @"yyyy-MM-dd HH:mm:ss ZZZ";
    char hw[64] = "未知"; size_t hn = sizeof hw; sysctlbyname("hw.machine", hw, &hn, NULL, 0);
    UIDevice *dev = UIDevice.currentDevice; dev.batteryMonitoringEnabled = YES;
    NSArray<NSString *> *bst = @[ @"未知", @"放电", @"充电中", @"已充满" ]; int bs = (int)dev.batteryState;
    NSDictionary *fsa = [fm attributesOfFileSystemForPath:doc error:nil]; NSProcessInfo *pi = NSProcessInfo.processInfo;
    NSMutableString *h = [NSMutableString string];
    [h appendFormat:@"D2R Mobile Diagnostics\n导出时间 %@\n版本 %@ 构建 %@\n机型 %s 系统 %@\n", [df stringFromDate:NSDate.date], [inf[@"CFBundleShortVersionString"] description] ?: @"未知", build, hw, pi.operatingSystemVersionString];
    [h appendFormat:@"温控档位 %ld 低电量模式 %@ 电量 %.0f%% %@\n", (long)pi.thermalState, pi.lowPowerModeEnabled ? @"开" : @"关", dev.batteryLevel < 0 ? -1.0 : dev.batteryLevel * 100, bs >= 0 && bs <= 3 ? bst[bs] : @"未知"];
    [h appendFormat:@"物理内存 %.1f GB 可用存储 %.1f GB 本次已运行 %.0f 分钟\n", pi.physicalMemory / 1073741824.0, [fsa[NSFileSystemFreeSize] doubleValue] / 1073741824.0, -g_t0.timeIntervalSinceNow / 60];
    perf_block(h, @"本局关键读数（各取最后一条）", cur);
    perf_block(h, @"上一局关键读数（各取最后一条）", prev);
    NSMutableData *out = [[h dataUsingEncoding:NSUTF8StringEncoding] mutableCopy];
    void (^part)(NSString *, NSData *) = ^(NSString *t, NSData *d) {
        [out appendData:[[NSString stringWithFormat:@"\n\n===== %@（%@）=====\n", t, d ? [NSString stringWithFormat:@"%lu 字节", (unsigned long)d.length] : @"没有这份日志"] dataUsingEncoding:NSUTF8StringEncoding]];
        if (d) [out appendData:d]; };
    part(@"本局日志 boot.txt", cur); part(@"上一局日志 boot.previous.txt", prev);
    NSString *dir = [NSTemporaryDirectory() stringByAppendingPathComponent:@"sharelog"];
    [fm removeItemAtPath:dir error:nil]; [fm createDirectoryAtPath:dir withIntermediateDirectories:YES attributes:nil error:nil];
    df.dateFormat = @"yyyyMMdd_HHmmss";
    NSString *p = [dir stringByAppendingPathComponent:[NSString stringWithFormat:@(dm_ui("dmShareLogFile")), build, [df stringFromDate:NSDate.date]]];
    NSData *fin = scrub_home(out);
    if (![fin writeToFile:p atomically:YES]) { plog("诊断日志：【红】导出文件写不进 %s", p.UTF8String); return nil; }
    plog("诊断日志：已打包 %s（%lu 字节：本局 %lu、上一局 %s）", p.lastPathComponent.UTF8String, (unsigned long)fin.length, (unsigned long)cur.length,
         prev ? [NSString stringWithFormat:@"%lu", (unsigned long)prev.length].UTF8String : "没有");
    if (getenv("DM_TEST_SHARELOG_CLOSE")) [fin writeToFile:[doc stringByAppendingPathComponent:@"sharelog_test.txt"] atomically:YES];
    return [NSURL fileURLWithPath:p];
}
static UIViewController *top_vc(void) {
    UIViewController *vc = nil, *alt = nil, *any = nil;
    for (UIScene *s in UIApplication.sharedApplication.connectedScenes)
        if ([s isKindOfClass:UIWindowScene.class]) for (UIWindow *w in ((UIWindowScene *)s).windows) {
            if (!w.rootViewController) continue;
            if (w.isKeyWindow) vc = w.rootViewController;
            else if (s.activationState == UISceneActivationStateForegroundActive && !alt) alt = w.rootViewController;
            else if (!any) any = w.rootViewController; }
    vc = vc ?: alt ?: any; while (vc.presentedViewController) vc = vc.presentedViewController;
    return vc;
}
static _Atomic int g_busy;
static void on_click(void) {
    if (atomic_exchange(&g_busy, 1)) { plog("诊断日志：分享面板还开着，这次点击不再弹"); return; }
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
        NSURL *u = build_file();
        dispatch_async(dispatch_get_main_queue(), ^{
            UIViewController *vc = u ? top_vc() : nil;
            if (!vc) { if (u) plog("诊断日志：【红】没找到可弹分享面板的界面"); atomic_store(&g_busy, 0); return; }
            UIActivityViewController *a = [[UIActivityViewController alloc] initWithActivityItems:@[u] applicationActivities:nil];
            a.completionWithItemsHandler = ^(UIActivityType t, BOOL done, NSArray *items, NSError *e) { (void)items;
                plog("诊断日志：分享面板收起（%s%s%s）", done ? "已发到 " : "没发", done ? (t ?: @"未知").UTF8String : "", e ? [NSString stringWithFormat:@"，错误 %@", e.localizedDescription].UTF8String : "");
                atomic_store(&g_busy, 0); };
            UIPopoverPresentationController *pp = a.popoverPresentationController;
            if (pp) { pp.sourceView = vc.view; pp.sourceRect = CGRectMake(CGRectGetMidX(vc.view.bounds), CGRectGetMidY(vc.view.bounds), 1, 1); pp.permittedArrowDirections = 0; }
            [vc presentViewController:a animated:YES completion:^{
                plog("诊断日志：分享面板已弹出（%s，%s）", pp ? "弹出框" : "底部面板", NSStringFromCGRect(a.view.frame).UTF8String);
                const char *tc = getenv("DM_TEST_SHARELOG_CLOSE"); if (!tc) return;
                dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(atof(tc) * 1e9)), dispatch_get_main_queue(), ^{
                    [a dismissViewControllerAnimated:NO completion:^{ plog("诊断日志：测试开关 DM_TEST_SHARELOG_CLOSE 到点收起分享面板"); atomic_store(&g_busy, 0); }]; });
            }];
        });
    });
}
void dm_sharelog_boot(void) { g_t0 = NSDate.date; dm_sharelog_init(on_click); }
void dm_sharelog_share(void) { on_click(); }
