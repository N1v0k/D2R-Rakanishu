

















#import <Foundation/Foundation.h>
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE
#import <UIKit/UIKit.h>
#endif
#import <Network/Network.h>
#include <signal.h>
#include <sys/clonefile.h>
#include <stdatomic.h>
#include "front.h"
#include "hle.h"

int dm_cloud_sync(const char *save, const char *cloud, const char *bak, const char *statef, int allow_down, uint64_t now_ms, int (*ready)(const char *unit));
int dm_cloud_inbox_add(const char *save, const char *statef, const char *pack); uint64_t dm_cloud_pack_stamp(const char *pack);
int dm_cloud_bak_pack(const char *bak, const char *unit, const char *pack, const char *why); void dm_cloud_char_conflict_note(const char *unit);
int dm_custom_get(const char *key);
extern void (*g_dm_save_commit_hook)(const char *commit_dir);

static NSString *g_cloud, *g_save, *g_bak, *g_state;
static dispatch_queue_t g_q;
static uint64_t now_ms(void) { return (uint64_t)([NSDate date].timeIntervalSince1970 * 1000); }



static int pack_ready(const char *unit) {
    NSString *nm = [NSString stringWithFormat:@"%s.pack", unit];
    if ([NSFileManager.defaultManager fileExistsAtPath:[g_cloud stringByAppendingPathComponent:[NSString stringWithFormat:@".%@.icloud", nm]]]) return 0;
    NSURL *u = [NSURL fileURLWithPath:[g_cloud stringByAppendingPathComponent:nm]]; NSString *st = nil;
    if (![u getResourceValue:&st forKey:NSURLUbiquitousItemDownloadingStatusKey error:nil] || !st) return 1;
    return [st isEqualToString:NSURLUbiquitousItemDownloadingStatusCurrent];
}
static int pending_downloads(int start) {
    int n = 0; NSFileManager *fm = NSFileManager.defaultManager;
    for (NSString *f in [fm contentsOfDirectoryAtPath:g_cloud error:nil] ?: @[]) {
        NSString *nm = f; if ([f hasPrefix:@"."] && [f hasSuffix:@".icloud"]) nm = [f substringWithRange:NSMakeRange(1, f.length - 8)];
        if (![nm hasSuffix:@".pack"]) continue;
        if (pack_ready([nm substringToIndex:nm.length - 5].UTF8String)) continue;
        n++; if (start) [fm startDownloadingUbiquitousItemAtURL:[NSURL fileURLWithPath:[g_cloud stringByAppendingPathComponent:nm]] error:nil];
    }
    return n;
}








static void take_conflicts(NSString *save) {
    NSFileManager *fm = NSFileManager.defaultManager;
    for (NSString *f in [fm contentsOfDirectoryAtPath:g_cloud error:nil] ?: @[]) {
        if ([f hasPrefix:@"."] || ![f hasSuffix:@".pack"]) continue;
        NSURL *u = [NSURL fileURLWithPath:[g_cloud stringByAppendingPathComponent:f]]; NSArray<NSFileVersion *> *vs = [NSFileVersion unresolvedConflictVersionsOfItemAtURL:u];
        if (!vs.count) continue;
        NSString *unit = f.stringByDeletingPathExtension; int bad = 0;
        if ([unit isEqualToString:@"_stash"]) { for (NSFileVersion *v in vs) bad |= dm_cloud_inbox_add(save.fileSystemRepresentation, g_state.fileSystemRepresentation, v.URL.fileSystemRepresentation) != 0; }
        else { uint64_t best = dm_cloud_pack_stamp(u.fileSystemRepresentation); NSFileVersion *win = nil;
            for (NSFileVersion *v in vs) { uint64_t t = dm_cloud_pack_stamp(v.URL.fileSystemRepresentation); if (t > best) { best = t; win = v; } }
            if (win) { NSError *e = nil; bad |= dm_cloud_bak_pack(g_bak.fileSystemRepresentation, unit.UTF8String, u.fileSystemRepresentation, "iCloud 冲突里较旧的现行版") != 0;
                if (!bad && ![win replaceItemAtURL:u options:0 error:&e]) { bad = 1; LOG("【红】iCloud 同步：%s 换上较新的冲突版本失败 %s", unit.UTF8String, e.localizedDescription.UTF8String ?: "?"); } }
            for (NSFileVersion *v in vs) if (v != win) bad |= dm_cloud_bak_pack(g_bak.fileSystemRepresentation, unit.UTF8String, v.URL.fileSystemRepresentation, "iCloud 冲突版本") != 0;
            if (!bad) dm_cloud_char_conflict_note(unit.UTF8String); }
        if (bad) { LOG("【红】iCloud 同步：%s 有 %lu 个冲突版本没收好，留着下次启动再来", unit.UTF8String, (unsigned long)vs.count); continue; }
        for (NSFileVersion *v in vs) v.resolved = YES;
        NSError *e = nil; if (![NSFileVersion removeOtherVersionsOfItemAtURL:u error:&e]) LOG("iCloud 同步：%s 冲突已标解决，删其余版本没成（%s），不影响", unit.UTF8String, e.localizedDescription.UTF8String ?: "?");
        LOG("iCloud 同步：%s 的 %lu 个 iCloud 冲突版本已收好（%s）", unit.UTF8String, (unsigned long)vs.count, [unit isEqualToString:@"_stash"] ? "进待合并收件箱" : "最新的当现行版、其余进备份");
    }
}
static int run_sync(NSString *save, int down) {
    __block int r = -1; NSError *err = nil;
    [[[NSFileCoordinator alloc] initWithFilePresenter:nil] coordinateWritingItemAtURL:[NSURL fileURLWithPath:g_cloud] options:NSFileCoordinatorWritingForMerging error:&err byAccessor:^(NSURL *u) {
        (void)u; if (down) take_conflicts(save);
        r = dm_cloud_sync(save.fileSystemRepresentation, g_cloud.fileSystemRepresentation, g_bak.fileSystemRepresentation, g_state.fileSystemRepresentation, down, now_ms(), pack_ready); }];
    if (err) LOG("【红】iCloud 同步：文件协调失败 %s", err.localizedDescription.UTF8String);
    return r;
}










static NSString *g_pend;
static _Atomic int g_bg;
static _Atomic int g_session;
static void upload_snap(NSString *snap, const char *why) {
    if (dm_custom_get("icloud")) { int n = run_sync(snap, 0); LOG("iCloud 同步：%s上传 %d 个", why, n); }
    [NSFileManager.defaultManager removeItemAtPath:snap error:nil];
}
static void on_commit(const char *commit_dir) {
    static unsigned seq; NSString *snap = [NSString stringWithFormat:@"%@.cloudsnap%u", g_save, ++seq];
    [NSFileManager.defaultManager removeItemAtPath:snap error:nil];
    if (clonefile(commit_dir, snap.fileSystemRepresentation, 0)) { LOG("【红】iCloud 同步：存档快照失败 errno=%d，这次提交不上传", errno); return; }
    int defer = atomic_load(&g_session) && !atomic_load(&g_bg);
    dispatch_async(g_q, ^{
        if (defer) { if (g_pend) [NSFileManager.defaultManager removeItemAtPath:g_pend error:nil]; g_pend = snap; return; }
        if (g_pend) { [NSFileManager.defaultManager removeItemAtPath:g_pend error:nil]; g_pend = nil; }
        upload_snap(snap, atomic_load(&g_bg) ? "后台存档提交后" : "存档提交后（不在局内）");
    });
}


void dm_cloud_flush(int background) {
    if (!g_q) return;
    if (background) atomic_store(&g_bg, 1);
#if TARGET_OS_IPHONE
    __block UIBackgroundTaskIdentifier tk = UIBackgroundTaskInvalid;
    if (background) tk = [UIApplication.sharedApplication beginBackgroundTaskWithName:@"dm.cloud" expirationHandler:^{ LOG("【红】iCloud 同步：后台收尾时间到，系统叫停（没传完的下次启动补传）"); [UIApplication.sharedApplication endBackgroundTask:tk]; tk = UIBackgroundTaskInvalid; }];
#endif
    dispatch_async(g_q, ^{
        NSString *p = g_pend; g_pend = nil;
        if (p) upload_snap(p, background ? "切到后台，" : "退出这一局回到前端，");
        else LOG("iCloud 同步：%s，局内没有待传的存档", background ? "切到后台" : "退出这一局回到前端");
#if TARGET_OS_IPHONE
        if (tk != UIBackgroundTaskInvalid) dispatch_async(dispatch_get_main_queue(), ^{ if (tk != UIBackgroundTaskInvalid) { [UIApplication.sharedApplication endBackgroundTask:tk]; tk = UIBackgroundTaskInvalid; } });
#endif
    });
}


int dm_cloud_flush_wait(double sec) {
    if (!g_q) return 1;
    dispatch_semaphore_t done = dispatch_semaphore_create(0);
    dispatch_async(g_q, ^{ NSString *p = g_pend; g_pend = nil; if (p) upload_snap(p, "退出 App 前，"); else LOG("iCloud 同步：退出 App 前，没有待传的存档"); dispatch_semaphore_signal(done); });
    if (dispatch_semaphore_wait(done, dispatch_time(DISPATCH_TIME_NOW, (int64_t)(sec * NSEC_PER_SEC)))) { LOG("【红】iCloud 同步：退出 App 前 %.0f 秒没传完（没传完的下次启动补传）", sec); return 0; }
    return 1;
}
void dm_cloud_foreground(void) { atomic_store(&g_bg, 0); }
static void cloud_ingame(int on) { if (on) atomic_store(&g_session, 1); }
static void cloud_charsel(void) { if (atomic_exchange(&g_session, 0)) dm_cloud_flush(0); }

static NSURL *container(double limit) {
    __block NSURL *ct = nil; dispatch_semaphore_t got = dispatch_semaphore_create(0);
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{ ct = [NSFileManager.defaultManager URLForUbiquityContainerIdentifier:nil]; dispatch_semaphore_signal(got); });
    if (dispatch_semaphore_wait(got, dispatch_time(DISPATCH_TIME_NOW, (int64_t)(limit * NSEC_PER_SEC)))) { LOG("iCloud 不可用：取容器超过 %.0f 秒，本次只用本地存档", limit); return nil; }
    if (!ct) LOG("iCloud 不可用：没登录 iCloud 或本 App 的 iCloud 被关，只用本地存档");
    return ct;
}

















static NSString *test_path(const char *e) {
    if (!e) return nil; if (e[0] == '/') return @(e);
#if TARGET_OS_OSX
    return [@(getenv("DM_SAVE") ?: ".").stringByDeletingLastPathComponent stringByAppendingPathComponent:@(e)];
#else
    return [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES).firstObject stringByAppendingPathComponent:@(e)];
#endif
}
static int v2_ready(NSString *nw, NSString *okf) {
    NSFileManager *fm = NSFileManager.defaultManager; if ([fm fileExistsAtPath:okf]) return 1;
    NSString *ls = [NSString stringWithContentsOfFile:[nw stringByAppendingPathComponent:@"sw_migrated.txt"] encoding:NSUTF8StringEncoding error:nil];
    if (!ls) { LOG("iCloud 同步：saves_v2 里还没有完成清单（别的设备的迁移还没传完？），本次不同步"); return 0; }
    int miss = 0, n = 0; for (NSString *f in [ls componentsSeparatedByString:@"\n"]) { if (!f.length) continue; n++;
        if (![fm fileExistsAtPath:[nw stringByAppendingPathComponent:f]] && ![fm fileExistsAtPath:[nw stringByAppendingPathComponent:[NSString stringWithFormat:@".%@.icloud", f]]]) miss++; }
    if (miss) { LOG("iCloud 同步：saves_v2 清单 %d 项还缺 %d 项没到本机，本次不同步", n, miss); return 0; }
    [@"" writeToFile:okf atomically:YES encoding:NSUTF8StringEncoding error:nil]; LOG("iCloud 同步：saves_v2 清单 %d 项都已到本机", n); return 1;
}
static int migrate_v2(NSString *docs, double deadline) {
    NSFileManager *fm = NSFileManager.defaultManager; NSString *nw = [docs stringByAppendingPathComponent:@"saves_v2"], *old = [docs stringByAppendingPathComponent:@"saves"], *tmp = [nw stringByAppendingString:@".tmp"];
    if ([fm fileExistsAtPath:nw]) return 1;
    if (![fm fileExistsAtPath:old]) { LOG("iCloud 同步：云端没有旧目录 saves，直接用 saves_v2"); [fm createDirectoryAtPath:nw withIntermediateDirectories:YES attributes:nil error:nil];
        [@"" writeToFile:[nw stringByAppendingPathComponent:@"sw_migrated.txt"] atomically:YES encoding:NSUTF8StringEncoding error:nil]; return 1; }
    g_cloud = old; int pend = pending_downloads(1);
    while (pend && CFAbsoluteTimeGetCurrent() < deadline) { usleep(100000); pend = pending_downloads(0); }
    g_cloud = nil;
    if (pend) { LOG("iCloud 同步：旧目录 saves 还有 %d 个包没下载完，本次不迁移、不同步，下次启动再试", pend); return 0; }
    const char *kill_at = getenv("DM_TEST_ICLOUD_MIGRATE_KILL");
    __block BOOL ok = YES; __block NSError *e2 = nil; __block NSMutableString *list = [NSMutableString string]; __block int copied = 0; NSError *err = nil;
    [[[NSFileCoordinator alloc] initWithFilePresenter:nil] coordinateReadingItemAtURL:[NSURL fileURLWithPath:old] options:0 error:&err byAccessor:^(NSURL *u) {
        if ([fm fileExistsAtPath:tmp]) { LOG("iCloud 同步：上次迁移没做完（有 saves_v2.tmp），删掉重来"); [fm removeItemAtPath:tmp error:nil]; }
        ok = [fm createDirectoryAtPath:tmp withIntermediateDirectories:NO attributes:nil error:&e2];
        for (NSString *f in [fm contentsOfDirectoryAtPath:u.path error:nil] ?: @[]) { if (!ok) break; if ([f hasPrefix:@"."]) continue;
            ok = [fm copyItemAtPath:[u.path stringByAppendingPathComponent:f] toPath:[tmp stringByAppendingPathComponent:f] error:&e2]; if (!ok) break;
            if ([f hasSuffix:@".pack"]) [list appendFormat:@"%@\n", f]; copied++;
            if (kill_at && atoi(kill_at) == copied) { LOG("DM_TEST_ICLOUD_MIGRATE_KILL：复制完第 %d 个文件，自杀", copied); kill(getpid(), SIGKILL); } }
        if (ok) ok = [list writeToFile:[tmp stringByAppendingPathComponent:@"sw_migrated.txt"] atomically:YES encoding:NSUTF8StringEncoding error:&e2];
        if (ok && kill_at && !strcmp(kill_at, "marker")) { LOG("DM_TEST_ICLOUD_MIGRATE_KILL：清单已写、改名之前自杀"); kill(getpid(), SIGKILL); }
        if (ok) ok = [fm moveItemAtPath:tmp toPath:nw error:&e2]; }];
    if (!ok || err) { [fm removeItemAtPath:tmp error:nil];
        if ([fm fileExistsAtPath:nw]) { LOG("iCloud 同步：saves_v2 已由别的设备建好，用它"); return 1; }
        LOG("【红】iCloud 同步：旧目录 saves 复制到 saves_v2 失败（%s），本次不同步", (err ?: e2).localizedDescription.UTF8String ?: "?"); return 0; }
    LOG("iCloud 同步：旧目录 saves 已复制到 saves_v2（%d 个文件，清单 %lu 个包），旧目录留给旧包用", copied, (unsigned long)[list componentsSeparatedByString:@"\n"].count - 1); return 1;
}












static int never_synced(NSString *statef) {
    NSString *t = [NSString stringWithContentsOfFile:statef encoding:NSUTF8StringEncoding error:nil];
    return !t || [t rangeOfString:@"\t"].location == NSNotFound;
}
static int first_listing_ok(double deadline) {
    int got = 0, listed = 0, present = 0, net = 0; const char *e = getenv("DM_TEST_ICLOUD_FIRST");
    if (e) { sscanf(e, "%d,%d,%d,%d", &got, &listed, &present, &net); }
    else {
        __block int g = 0, n = 0, pr = 0; dispatch_semaphore_t sem = dispatch_semaphore_create(0); __block NSMetadataQuery *q = nil; __block id obs = nil;






        BOOL on_main = NSThread.isMainThread;
        void (^startq)(void) = ^{
            q = [NSMetadataQuery new]; q.searchScopes = @[NSMetadataQueryUbiquitousDocumentsScope];
            q.predicate = [NSPredicate predicateWithFormat:@"%K LIKE '*.pack'", NSMetadataItemFSNameKey];
            obs = [NSNotificationCenter.defaultCenter addObserverForName:NSMetadataQueryDidFinishGatheringNotification object:q queue:nil usingBlock:^(NSNotification *note) { (void)note;
                [q disableUpdates]; n = (int)q.resultCount; NSFileManager *fm = NSFileManager.defaultManager;
                for (NSUInteger i = 0; i < q.resultCount; i++) { NSURL *u = [[q resultAtIndex:i] valueForAttribute:NSMetadataItemURLKey]; if (!u) continue;
                    NSString *ph = [u.path.stringByDeletingLastPathComponent stringByAppendingPathComponent:[NSString stringWithFormat:@".%@.icloud", u.lastPathComponent]];
                    if ([fm fileExistsAtPath:u.path] || [fm fileExistsAtPath:ph]) pr++; }
                g = 1; [q stopQuery]; [NSNotificationCenter.defaultCenter removeObserver:obs]; dispatch_semaphore_signal(sem); }];
            if (![q startQuery]) { [NSNotificationCenter.defaultCenter removeObserver:obs]; dispatch_semaphore_signal(sem); } };
        double left = deadline - CFAbsoluteTimeGetCurrent(); if (left < 0.5) left = 0.5;
        if (on_main) {
            startq();
            NSDate *until = [NSDate dateWithTimeIntervalSinceNow:left];
            while (!g && until.timeIntervalSinceNow > 0)
                [NSRunLoop.currentRunLoop runMode:NSDefaultRunLoopMode beforeDate:[NSDate dateWithTimeIntervalSinceNow:MIN(0.05, until.timeIntervalSinceNow)]];
            if (!g) { [q stopQuery]; if (obs) [NSNotificationCenter.defaultCenter removeObserver:obs]; }
        } else {
            dispatch_async(dispatch_get_main_queue(), startq);
            if (dispatch_semaphore_wait(sem, dispatch_time(DISPATCH_TIME_NOW, (int64_t)(left * NSEC_PER_SEC))))
                dispatch_async(dispatch_get_main_queue(), ^{ if (q) { [q stopQuery]; if (obs) [NSNotificationCenter.defaultCenter removeObserver:obs]; } });
        }
        got = g; listed = n; present = pr;
        __block int ns = 0; dispatch_semaphore_t ps = dispatch_semaphore_create(0); nw_path_monitor_t m = nw_path_monitor_create();
        nw_path_monitor_set_queue(m, dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0));
        nw_path_monitor_set_update_handler(m, ^(nw_path_t path) { if (!ns) { ns = nw_path_get_status(path) == nw_path_status_satisfied ? 1 : 2; dispatch_semaphore_signal(ps); } });
        nw_path_monitor_start(m); dispatch_semaphore_wait(ps, dispatch_time(DISPATCH_TIME_NOW, (int64_t)(NSEC_PER_SEC))); nw_path_monitor_cancel(m); net = ns == 1;
    }
    int ok = got && (listed ? present >= listed : net);
    LOG("iCloud 同步：本机从没同步成功过，先看云端条目列表：收集%s、列出 %d 个包、已到本机 %d 个、网络%s → %s", got ? "完成" : "未完成", listed, present, net ? "通" : "不通",
        ok ? "看清了，同步" : "没看清，本次不同步（不上传、不建 saves_v2），下次启动再看");
    return ok;
}
void dm_cloud_boot(void) {
    const char *sv = getenv("DM_SAVE"), *sub = getenv("DM_TEST_ICLOUD_SUB"), *ldir = getenv("DM_TEST_ICLOUD_DIR"), *lroot = getenv("DM_TEST_ICLOUD_ROOT"); if (!sv) return;
    NSString *doc = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES).firstObject;
    NSString *save = @(sv); int real = [save.stringByStandardizingPath isEqualToString:[doc stringByAppendingPathComponent:@"save"].stringByStandardizingPath];
#if TARGET_OS_OSX
    real = getenv("DM_CLOUD_REAL") != NULL; (void)doc;
#endif
    int off = getenv("DM_TEST_ICLOUD_OFF") != NULL;
    if (!real && !sub && !ldir && !lroot) { if (off) LOG("iCloud 同步：测试开关 DM_TEST_ICLOUD_OFF，本局不同步");
#if TARGET_OS_OSX
        else LOG("iCloud 同步：测试用的存档目录（DM_APP_SUPPORT），本局不同步");
#endif
        return; }






    if (NSProcessInfo.processInfo.isiOSAppOnMac && !ldir && !lroot) { const char *r1 = getenv("DM_REAL_ICLOUD"), *r2 = getenv("DM_REAL_ICLOUD_CONFIRM");
        if (!(r1 && !strcmp(r1, "1") && r2 && !strcmp(r2, "写入真实云端"))) { LOG("iCloud 同步：Mac 上运行：缺省不同步 iCloud"); return; }
        LOG("【红】iCloud 同步：Mac 上运行，已显式放开（DM_REAL_ICLOUD）：本局连真实云端"); }
    if (off) { LOG("iCloud 同步：测试开关 DM_TEST_ICLOUD_OFF，本局不同步"); return; }
    if (!dm_custom_get("icloud")) { LOG("iCloud 同步：自定义页已关，不同步"); return; }
    double t0 = CFAbsoluteTimeGetCurrent(), limit = 5.0;
    if (ldir) g_cloud = test_path(ldir);
    else if (sub) { NSURL *ct = container(limit); if (!ct) return; g_cloud = [[ct.path stringByAppendingPathComponent:@"Documents"] stringByAppendingPathComponent:@(sub)]; }
    else { NSString *docs = test_path(lroot); if (!lroot) { NSURL *ct = container(limit); if (!ct) return; docs = [ct.path stringByAppendingPathComponent:@"Documents"]; }
        if (never_synced([save stringByAppendingString:@".cloudstate"]) && (!lroot || getenv("DM_TEST_ICLOUD_FIRST")) && !first_listing_ok(t0 + limit)) return;
        if (!migrate_v2(docs, t0 + limit)) return;
        NSString *nw = [docs stringByAppendingPathComponent:@"saves_v2"]; if (!v2_ready(nw, [save stringByAppendingString:@".cloudv2ok"])) return;
        g_cloud = nw; }
    [NSFileManager.defaultManager createDirectoryAtPath:g_cloud withIntermediateDirectories:YES attributes:nil error:nil];
    g_save = save; g_bak = [save stringByAppendingString:@".cloudbak"]; g_state = [save stringByAppendingString:@".cloudstate"];
    if (ldir) { g_bak = [g_cloud stringByAppendingString:@".cloudbak"]; g_state = [g_cloud stringByAppendingString:@".cloudstate"]; }
    g_q = dispatch_queue_create("dm.cloud", DISPATCH_QUEUE_SERIAL);
    for (NSString *f in [NSFileManager.defaultManager contentsOfDirectoryAtPath:save.stringByDeletingLastPathComponent error:nil] ?: @[])
        if ([f hasPrefix:[save.lastPathComponent stringByAppendingString:@".cloudsnap"]]) [NSFileManager.defaultManager removeItemAtPath:[save.stringByDeletingLastPathComponent stringByAppendingPathComponent:f] error:nil];
    double t1 = CFAbsoluteTimeGetCurrent(); int pend = pending_downloads(1);
    while (pend && CFAbsoluteTimeGetCurrent() - t0 < limit) { usleep(100000); pend = pending_downloads(0); }
    double t2 = CFAbsoluteTimeGetCurrent(); int n = run_sync(save, 1);
    LOG("iCloud 同步：启动分段 取容器与查齐 %.0f ms、等下载 %.0f ms、文件协调与同步 %.0f ms", (t1 - t0) * 1000, (t2 - t1) * 1000, (CFAbsoluteTimeGetCurrent() - t2) * 1000);
    LOG("iCloud 同步：启动 %.0f ms，动作 %d 个%s（云端目录 %s）", (CFAbsoluteTimeGetCurrent() - t0) * 1000, n, pend ? [NSString stringWithFormat:@"，%d 个包没等到下载完、本次跳过", pend].UTF8String : "", ldir ? "本地测试目录" : sub ? sub : "saves_v2");
    g_dm_save_commit_hook = on_commit;
    { extern void (*g_dm_ingame_hook)(int); extern void (*g_dm_charsel_hook)(void); g_dm_ingame_hook = cloud_ingame; g_dm_charsel_hook = cloud_charsel; }
}
