























#import <UIKit/UIKit.h>
#import <Network/Network.h>
#include <stdatomic.h>
void plog(const char *fmt, ...);
void dm_update_init(int own_build, void (*on_select)(void), void (*on_click)(void));
void dm_update_offer(int new_build, const char *note, int urgent);
void dm_update_safe_left(int units);
void dm_update_fail(int kind);
void dm_update_site(const char *host);

static NSString *g_url, *g_install; static int g_own; static _Atomic int g_inflight;



static void upd_net_why(void) {
    nw_path_monitor_t m = nw_path_monitor_create(); nw_path_monitor_set_queue(m, dispatch_get_global_queue(QOS_CLASS_UTILITY, 0));
    nw_path_monitor_set_update_handler(m, ^(nw_path_t p) {
        static const char *st[] = { "无效", "通", "不通", "要先建连接" }, *why[] = { "系统没给原因", "蜂窝数据许可被拒", "无线局域网数据许可被拒", "本地网络许可被拒", "要先开 VPN" };
        nw_path_status_t s = nw_path_get_status(p); nw_path_unsatisfied_reason_t r = nw_path_get_unsatisfied_reason(p);
        plog("更新：系统网络路径 %s%s%s；无线局域网 %s、蜂窝 %s", s <= 3 ? st[s] : "未知", s == nw_path_status_satisfied ? "" : "，原因：", s == nw_path_status_satisfied ? "" : r <= 4 ? why[r] : "未知",
             nw_path_uses_interface_type(p, nw_interface_type_wifi) ? "在用" : "没用上", nw_path_uses_interface_type(p, nw_interface_type_cellular) ? "在用" : "没用上");


        const char *td = getenv("DM_TEST_UPDATE_DENY");
        int denied = (td && atoi(td)) || (s != nw_path_status_satisfied && (r == nw_path_unsatisfied_reason_cellular_denied || r == nw_path_unsatisfied_reason_wifi_denied));
        dm_update_fail(denied ? 1 : 2); plog("更新：选角页显示%s", denied ? "「无法联网检查更新 · 请在设置里允许无线数据」" : "「检查更新失败 · 浏览器手动更新」");
        nw_path_monitor_cancel(m);
    });
    nw_path_monitor_start(m);
}



static NSString *upd_parse(NSData *d, int *build, NSString **note, int *urgent, NSString **install) {
    NSDictionary *j = d ? [NSJSONSerialization JSONObjectWithData:d options:0 error:nil] : nil;
    if (![j isKindOfClass:NSDictionary.class]) return @"格式不对：不是 JSON 对象";
    NSNumber *b = j[@"build"], *u = j[@"urgent"]; NSString *n = j[@"note"], *i = j[@"install_url"];
    if (![b isKindOfClass:NSNumber.class] || ![u isKindOfClass:NSNumber.class] || ![n isKindOfClass:NSString.class] || ![i isKindOfClass:NSString.class]) return @"格式不对：四个字段缺了或类型不对";
    if (!(b.doubleValue >= 1 && b.doubleValue < 2e9) || b.doubleValue != b.intValue) return @"格式不对：build 不是正整数";
    NSString *sch = [NSURL URLWithString:i].scheme.lowercaseString;
    if (![sch isEqualToString:@"https"] && ![sch isEqualToString:@"itms-services"]) return @"格式不对：install_url 不是 https / itms-services 链接";
    n = [[n componentsSeparatedByCharactersInSet:NSCharacterSet.newlineCharacterSet] componentsJoinedByString:@" "];
    if (n.length > 24) n = [n substringWithRange:[n rangeOfComposedCharacterSequencesForRange:NSMakeRange(0, 24)]];
    *build = b.intValue; *note = n; *urgent = u.boolValue; *install = i; return nil;
}
static void upd_fetch(int n);
static void on_select(void) {
    dispatch_async(dispatch_get_main_queue(), ^{
        UIWindow *w = nil; for (UIScene *s in UIApplication.sharedApplication.connectedScenes) if ([s isKindOfClass:UIWindowScene.class]) { w = ((UIWindowScene *)s).windows.firstObject; if (w) break; }
        const char *tl = getenv("DM_TEST_SAFE_L"); CGFloat l = tl ? atof(tl) : w.safeAreaInsets.left, hh = w.bounds.size.height; if (hh > 0) dm_update_safe_left((int)lround(l / hh * 2160));
    });
    if (!g_url) return;
    if (atomic_exchange(&g_inflight, 1)) { plog("更新：上一次查询还没回来，这次不再发"); return; }
    upd_fetch(1);
}












#define UPD_TRIES 3
static void upd_fetch(int n) {
    static NSURLSession *ses; static dispatch_once_t once;
    dispatch_once(&once, ^{ NSURLSessionConfiguration *c = NSURLSessionConfiguration.ephemeralSessionConfiguration; c.timeoutIntervalForRequest = 10; c.timeoutIntervalForResource = 15;
        c.requestCachePolicy = NSURLRequestReloadIgnoringLocalCacheData; c.waitsForConnectivity = NO; ses = [NSURLSession sessionWithConfiguration:c]; });
    const char *tf = getenv("DM_TEST_UPDATE_FAIL_FIRST"); NSString *us = tf && n <= atoi(tf) ? @"https://nonexistent-dm.invalid/latest.json" : g_url;
    NSURL *url = [NSURL URLWithString:us]; if (!url) { plog("更新：版本文件地址 %s 不是合法地址，不查", us.UTF8String); g_url = nil; atomic_store(&g_inflight, 0); return; }
    [[ses dataTaskWithURL:url completionHandler:^(NSData *d, NSURLResponse *r, NSError *e) {
        int build = 0, urgent = 0; NSString *note = nil, *install = nil, *why = nil;
        if (e) why = [NSString stringWithFormat:@"取不到：%@", e.localizedDescription];
        else if ([r isKindOfClass:NSHTTPURLResponse.class] && [(NSHTTPURLResponse *)r statusCode] != 200) why = [NSString stringWithFormat:@"取不到：HTTP %ld", (long)[(NSHTTPURLResponse *)r statusCode]];
        else if (d.length > 4096) why = @"格式不对：超过 4096 字节";
        else why = upd_parse(d, &build, &note, &urgent, &install);
        if (!e && d.length && d.length <= 4096) {
            NSDictionary *j = [NSJSONSerialization JSONObjectWithData:d options:0 error:nil]; NSNumber *rs = [j isKindOfClass:NSDictionary.class] ? j[@"render_safe"] : nil;
            extern int g_dm_rt_ab; if ([rs isKindOfClass:NSNumber.class] && rs.boolValue) { if (g_dm_rt_ab) { g_dm_rt_ab = 0; plog("画法：版本文件 render_safe = true → 本局退回原画法"); } }
            else plog("画法：版本文件没有 render_safe（或为 false）→ 新画法照常"); }
        if (why) {
            plog("更新：查 %s（第 %d / %d 次）→ %s%s", us.UTF8String, n, UPD_TRIES, why.UTF8String, n < UPD_TRIES ? "，稍后再试" : "，已有的提示不动");
            if (e) plog("更新：第 %d 次取不到的错误码 %s %ld", n, e.domain.UTF8String, (long)e.code);
            if (n < UPD_TRIES) { dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(3 * n) * NSEC_PER_SEC), dispatch_get_global_queue(QOS_CLASS_UTILITY, 0), ^{ upd_fetch(n + 1); }); return; }
            atomic_store(&g_inflight, 0);
            if (e) upd_net_why(); else dm_update_fail(2);
            return; }
        atomic_store(&g_inflight, 0);
        if (n > 1) plog("更新：第 %d 次取到了（前面 %d 次失败）", n, n - 1);
        dm_update_fail(0);
        if (build <= g_own) { plog("更新：查 %s → 最新构建 %d，不比自己（%d）新，不提示（已有的提示撤掉）", us.UTF8String, build, g_own); dm_update_offer(0, "", 0); return; }
        plog("更新：查 %s → 有新构建 %d（%s，自己 %d）：%s", us.UTF8String, build, urgent ? "建议立即更新" : "普通", g_own, note.UTF8String);
        dispatch_async(dispatch_get_main_queue(), ^{ g_install = install; });
        dm_update_offer(build, note.UTF8String, urgent);
    }] resume];
}
static void on_click(void) {
    dispatch_async(dispatch_get_main_queue(), ^{
        NSURL *u = g_install ? [NSURL URLWithString:g_install] : nil; if (!u) { plog("更新：点了「更新」但没有安装链接"); return; }
        const char *to = getenv("DM_TEST_UPDATE_OPEN"); if (to && !atoi(to)) { plog("更新：点了「更新」→ 安装链接 %s（DM_TEST_UPDATE_OPEN=0，不真打开）", g_install.UTF8String); return; }
        plog("更新：点了「更新」→ 交系统打开安装链接");
        [UIApplication.sharedApplication openURL:u options:@{} completionHandler:^(BOOL ok) { plog("更新：系统%s安装链接", ok ? "已接手" : "没能打开"); }];
    });
}
void dm_update_boot(void) {
    const char *tb = getenv("DM_TEST_UPDATE_BUILD"), *tu = getenv("DM_TEST_UPDATE_URL");
    NSString *v = tb ? @(tb) : [NSBundle.mainBundle.infoDictionary[@"CFBundleVersion"] description]; g_own = v.intValue;
    if (![v isEqualToString:[NSString stringWithFormat:@"%d", g_own]] || g_own < 0) g_own = 0;
    NSString *url = tu ? @(tu) : NSBundle.mainBundle.infoDictionary[@"SWUpdateURL"];
    if ([url isKindOfClass:NSString.class] && url.length && g_own > 1) g_url = url;
    if (g_url) dm_update_site([NSURL URLWithString:g_url].host.UTF8String);
    plog("更新：自己的构建 %d，%s", g_own, g_url ? "到选角页后查一次新版本" : g_own > 1 ? "包里没有版本文件地址，不查新版本" : "开发包，不查新版本");
    dm_update_init(g_own, on_select, on_click);
}
