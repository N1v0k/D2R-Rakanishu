









#import <Cocoa/Cocoa.h>
#include <sys/stat.h>
#include "hle.h"
#include "dm_i18n.h"
#include "dm_custom.h"
#include "app_mac_overlay.h"
#define T(k) (@(dm_ui(k)))

#ifndef APP_NAME
#define APP_NAME "D2R Mobile"
#endif
static NSString *const kRomfsKey = @"RomfsDir";





static NSString *fmt1(const char *key, NSString *arg) {
    const char *dm_ui(const char *key); char b[1024]; snprintf(b, sizeof b, dm_ui(key), arg.UTF8String ?: ""); return @(b) ?: @"";
}
static NSString *g_sup;








static NSString *romfs_root(NSString *dir, NSString **why) {
    NSFileManager *fm = NSFileManager.defaultManager;
    for (NSString *sub in @[@"", @"RomFS", @"romfs", @"Update/RomFS"]) {
        NSString *d = sub.length ? [dir stringByAppendingPathComponent:sub] : dir;
        if ([fm fileExistsAtPath:[d stringByAppendingPathComponent:@"data/build_manifest.psv"]]) {
            if (![fm isReadableFileAtPath:[d stringByAppendingPathComponent:@"data/build_manifest.psv"]]) { if (why) *why = T("dmAppNoRead"); return nil; }
            return d;
        }
    }
    NSString *m = [dir stringByAppendingPathComponent:@"build_manifest.psv"];
    if ([fm fileExistsAtPath:m]) {
        if (![fm isReadableFileAtPath:m]) { if (why) *why = T("dmAppNoRead"); return nil; }
        if ([dir.lastPathComponent isEqualToString:@"data"]) return dir.stringByDeletingLastPathComponent;
        if (g_sup) { NSString *shim = [g_sup stringByAppendingPathComponent:@"romfs_flat"], *link = [shim stringByAppendingPathComponent:@"data"];
            [fm createDirectoryAtPath:shim withIntermediateDirectories:YES attributes:nil error:nil];
            NSString *cur = [fm destinationOfSymbolicLinkAtPath:link error:nil];
            if (![cur isEqualToString:dir]) { [fm removeItemAtPath:link error:nil]; [fm createSymbolicLinkAtPath:link withDestinationPath:dir error:nil]; }
            if ([fm fileExistsAtPath:[link stringByAppendingPathComponent:@"build_manifest.psv"]]) { LOG("App：选的是 data 那一层（%s），经壳目录 %s 当资源根", dir.UTF8String, shim.UTF8String); return shim; } }
    }
    if (why) *why = fmt1("dmAppNotRomfs", dir.lastPathComponent);
    return nil;
}
static void alert(NSString *title, NSString *text) {
    NSAlert *a = [NSAlert new]; a.messageText = title; a.informativeText = text; [a addButtonWithTitle:T("dmAppOK")]; [a runModal];
}


static NSString *pick_romfs(NSString *first_why) {
    NSString *why = first_why;
    for (;;) {
        NSOpenPanel *p = [NSOpenPanel openPanel];
        p.canChooseDirectories = YES; p.canChooseFiles = NO; p.allowsMultipleSelection = NO; p.prompt = T("dmAppPickUse");
        p.message = why ? fmt1("dmAppPickAgain", why) : T("dmAppPickMsg");
        if ([p runModal] != NSModalResponseOK) return nil;
        NSString *w = nil, *r = romfs_root(p.URL.path, &w);
        if (r) return r;
        alert(T("dmAppNotRomfsTitle"), w); why = w;
    }
}





static NSString *res_item(NSString *name) {
    NSFileManager *fm = NSFileManager.defaultManager; NSString *rp = NSBundle.mainBundle.resourcePath, *d = [rp stringByAppendingPathComponent:name];
    if ([fm fileExistsAtPath:d]) return d;
    NSString *txt = [NSString stringWithContentsOfFile:[rp stringByAppendingPathComponent:@"paths.txt"] encoding:NSUTF8StringEncoding error:nil];
    for (NSString *ln in [txt componentsSeparatedByString:@"\n"]) { NSString *pre = [name stringByAppendingString:@"="]; if ([ln hasPrefix:pre]) { d = [ln substringFromIndex:pre.length]; return [fm fileExistsAtPath:d] ? d : nil; } }
    return nil;
}
static void upd_start(void);


int dm_app_bootstrap(int *argc, char ***argv) {
    @autoreleasepool {
        NSString *bp = NSBundle.mainBundle.bundlePath;
        if (*argc > 1 || ![bp.pathExtension isEqualToString:@"app"]) return 0;
        NSFileManager *fm = NSFileManager.defaultManager;
        if (!getenv("DM_LANG")) setenv("DM_LANG", [NSLocale.preferredLanguages.firstObject hasPrefix:@"zh"] ? "zh-Hans" : "en", 1);
        NSString *sup = [[NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES).firstObject stringByAppendingPathComponent:@APP_NAME] copy];
        if (getenv("DM_APP_SUPPORT")) sup = @(getenv("DM_APP_SUPPORT"));
        NSString *logd = [[NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSUserDomainMask, YES).firstObject stringByAppendingPathComponent:@"Logs/" APP_NAME] copy];
        NSString *save = [sup stringByAppendingPathComponent:@"save"], *temp = [sup stringByAppendingPathComponent:@"temp"];
        for (NSString *d in @[sup, logd, save, temp]) [fm createDirectoryAtPath:d withIntermediateDirectories:YES attributes:nil error:nil];
        g_sup = sup;


        NSString *lf = [logd stringByAppendingPathComponent:@"host.log"];
        [fm removeItemAtPath:[logd stringByAppendingPathComponent:@"host.prev.log"] error:nil];
        [fm moveItemAtPath:lf toPath:[logd stringByAppendingPathComponent:@"host.prev.log"] error:nil];
        if (!isatty(2)) { freopen(lf.fileSystemRepresentation, "w", stderr); setvbuf(stderr, NULL, _IOLBF, 0); }




        [NSUserDefaults.standardUserDefaults setVolatileDomain:@{ @"ApplePersistenceIgnoreState": @YES } forName:NSArgumentDomain];
        [NSApplication sharedApplication]; [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular]; [NSApp activateIgnoringOtherApps:YES];
        NSString *prod = res_item(@"product");
        if (!prod || ![fm fileExistsAtPath:[prod stringByAppendingPathComponent:@"main.patched.bin"]]) {
            alert(T("dmAppBrokenTitle"), T("dmAppBroken")); exit(1);
        }
        NSUserDefaults *ud = NSUserDefaults.standardUserDefaults;


        if (![ud stringForKey:kRomfsKey] && ![NSBundle.mainBundle.bundleIdentifier isEqualToString:@"org.d2rmobile.mac"]) { NSUserDefaults *old = [[NSUserDefaults alloc] initWithSuiteName:@"org.d2rmobile.mac"]; NSString *r = [old stringForKey:kRomfsKey];
            if (r) { [ud setObject:r forKey:kRomfsKey]; if ([old boolForKey:@"KeysShown"]) [ud setBool:YES forKey:@"KeysShown"]; LOG("App：从旧包标识的偏好里接过资源文件夹 %s", r.UTF8String); } }
        NSString *saved = [ud stringForKey:kRomfsKey], *why = nil, *bundled = res_item(@"romfs");
        NSString *rom = bundled ? romfs_root(bundled, &why) : saved ? romfs_root(saved, &why) : nil;
        if (bundled && !rom) { alert(T("dmAppNotRomfsTitle"), why ?: T("dmAppNoRead")); exit(2); }
        if (getenv("DM_APP_ROMFS")) {
            rom = romfs_root(@(getenv("DM_APP_ROMFS")), &why);
            if (!rom) { LOG("【红】App：DM_APP_ROMFS 给的不是游戏资源文件夹：%s", why.UTF8String); exit(2); }
        } else if (!rom) {
            rom = pick_romfs(saved ? fmt1("dmAppRomfsGone", why) : nil);
            if (!rom) { LOG("App：没有选资源文件夹，退出"); exit(0); }
            [ud setObject:rom forKey:kRomfsKey];
        }
        { void dm_custom_init(const char *dir); dm_custom_init(sup.fileSystemRepresentation); }
        if (!getenv("DM_TOUCH_DEFAULTS")) setenv("DM_TOUCH_DEFAULTS", "1", 1);
        if (!getenv("DM_APP_SUPPORT")) setenv("DM_CLOUD_REAL", "1", 1);
        setenv("DM_WINDOW", "1", 1); setenv("DM_APP", "1", 1);
        if (!getenv("DM_AUDIO")) setenv("DM_AUDIO", "1", 1);
        setenv("DM_SAVE", save.fileSystemRepresentation, 1); setenv("DM_TEMP", temp.fileSystemRepresentation, 1);
        setenv("DM_ROMFS", rom.fileSystemRepresentation, 1); setenv("DM_ROOT", sup.fileSystemRepresentation, 1);
        NSString *shmap = [NSBundle.mainBundle.resourcePath stringByAppendingPathComponent:@"shader_map.tsv"];
        if ([fm fileExistsAtPath:shmap]) setenv("DM_NVN_SHMAP", shmap.fileSystemRepresentation, 1);
        NSString *msl = res_item(@"msl");
        if (msl) setenv("DM_MSL_DIR", msl.fileSystemRepresentation, 1);
        BOOL missing_overlay = NO;
        NSString *ov = dm_mac_overlay(res_item(@"romfs_overlay"), res_item(@"romfs_overlay_variants"), dm_custom_get, &missing_overlay);
        if (missing_overlay) LOG("App：缺少已启用自定义功能的覆盖层组合，暂用基础覆盖层；请重新生成并打包覆盖层");
        if (ov) setenv("DM_ROMFS_OVERLAY", ov.fileSystemRepresentation, 1);
        static char *av[5]; av[0] = (*argv)[0]; av[1] = strdup(prod.fileSystemRepresentation); av[2] = "main"; av[3] = "sdk"; av[4] = NULL;
        *argv = av; *argc = 4;
        upd_start();
        LOG("App：存档 %s；资源 %s；程序文件 %s；界面覆盖层 %s；日志 %s", save.UTF8String, rom.UTF8String, prod.UTF8String, getenv("DM_ROMFS_OVERLAY") ?: "（没有，原版界面）", lf.UTF8String);
        return 1;
    }
}











void dm_update_init(int own_build, void (*on_select)(void), void (*on_click)(void)); void dm_update_offer(int new_build, const char *note, int urgent); void dm_update_safe_left(int units);
static NSString *g_upd_url, *g_upd_page; static int g_upd_own; static double g_upd_last;
static void upd_check(void) {
    double now = CFAbsoluteTimeGetCurrent(); if (g_upd_last && now - g_upd_last < 60) return; g_upd_last = now;
    NSURL *u = [NSURL URLWithString:g_upd_url]; if (!u) return;
    NSURLSessionConfiguration *cf = NSURLSessionConfiguration.ephemeralSessionConfiguration; cf.timeoutIntervalForRequest = 2; cf.timeoutIntervalForResource = 2; cf.requestCachePolicy = NSURLRequestReloadIgnoringLocalCacheData;
    [[[NSURLSession sessionWithConfiguration:cf] dataTaskWithURL:u completionHandler:^(NSData *d, NSURLResponse *r, NSError *e) {
        NSInteger code = [r isKindOfClass:NSHTTPURLResponse.class] ? ((NSHTTPURLResponse *)r).statusCode : 0;
        NSDictionary *j = (!e && code == 200 && d) ? [NSJSONSerialization JSONObjectWithData:d options:0 error:nil] : nil;
        NSNumber *b = [j isKindOfClass:NSDictionary.class] ? j[@"build"] : nil;
        if (![b isKindOfClass:NSNumber.class] || b.intValue < 1) { LOG("更新：读版本文件没成（%s，HTTP %ld），本次不提示", e ? e.localizedDescription.UTF8String : "格式不对", (long)code); return; }
        NSString *zh = [j[@"note"] isKindOfClass:NSString.class] ? j[@"note"] : @"", *en = [j[@"note_en"] isKindOfClass:NSString.class] ? j[@"note_en"] : zh, *pg = [j[@"url"] isKindOfClass:NSString.class] ? j[@"url"] : nil;
        NSURL *pu = pg ? [NSURL URLWithString:pg] : nil; BOOL okpg = pu && ([pu.scheme isEqualToString:@"https"] || [pu.scheme isEqualToString:@"http"]) && [pu.host isEqualToString:u.host];
        g_upd_page = okpg ? [pg copy] : nil;
        if (b.intValue > g_upd_own) { const char *l = getenv("DM_LANG"); NSString *note = (l && !strncmp(l, "zh", 2)) ? zh : en; LOG("更新：最新构建 %d 比自己（%d）新 → 主菜单提示（下载页 %s）", b.intValue, g_upd_own, okpg ? "有" : "没给或不同主机，不可点"); dm_update_offer(b.intValue, note.UTF8String, 0); }
        else { LOG("更新：最新构建 %d，不比自己（%d）新，不提示", b.intValue, g_upd_own); dm_update_offer(0, "", 0); }
    }] resume];
}
static void upd_click(void) { NSString *pg = g_upd_page; if (!pg) return; dispatch_async(dispatch_get_main_queue(), ^{ LOG("更新：打开下载页"); if (!getenv("DM_TEST_UPDATE_NOOPEN")) [NSWorkspace.sharedWorkspace openURL:[NSURL URLWithString:pg]]; }); }
static void upd_start(void) {
    NSDictionary *inf = NSBundle.mainBundle.infoDictionary; const char *te = getenv("DM_TEST_UPDATE_URL");
    g_upd_url = te ? @(te) : [inf[@"SWUpdateURL"] description]; g_upd_own = [[inf[@"CFBundleVersion"] description] intValue];
    if (!g_upd_url.length || g_upd_own < 1) { LOG("更新：包里没配版本文件地址，不检查"); return; }
    dm_update_init(g_upd_own, upd_check, upd_click); dm_update_safe_left(0);
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_UTILITY, 0), ^{ upd_check(); });
}





int dm_front_navigation(void) { return 1; }
