





















#import <Foundation/Foundation.h>
#import <CommonCrypto/CommonDigest.h>
#import <Network/Network.h>
#import "asset_dl.h"
#include "../../host/dm_i18n.h"
void plog(const char *fmt, ...);

#define DL_CHUNK (16ll << 20)
static NSString *dm_dl_sid(NSString *suffix) { return [NSBundle.mainBundle.bundleIdentifier stringByAppendingString:suffix]; }

@interface DmAssetDL () <NSURLSessionDownloadDelegate>
@property (strong) NSURLSession *session; @property (strong) dispatch_queue_t q;
@property (copy) NSString *base, *dir; @property (strong) NSArray<NSDictionary *> *files;
@property (assign) int64_t total, settled;
@property (assign) int running, retry, cellular; @property (strong) NSMutableDictionary<NSString *, NSNumber *> *redo;
@property (copy) void (^onProgress)(int64_t, int64_t, NSString *); @property (copy) void (^onDone)(NSString *);
@property (copy) NSString *sid, *envBase, *ptrFile, *baseFile, *defKey;
@property (copy) void (^bgDone)(void);




@property (strong) NSURLSession *fg;
@property (assign) int useFg, heal, skipWin;
@property (assign) int64_t curBytes, winBytes;
@property (assign) CFAbsoluteTime winT; @property (assign) double best;
@property (strong) dispatch_source_t watch;
- (NSURLSession *)bg;
@end

@implementation DmAssetDL
+ (instancetype)shared { static DmAssetDL *s; static dispatch_once_t once; dispatch_once(&once, ^{ s = [[DmAssetDL alloc] initDir:@"D2R Mobile Resources/data" sid:dm_dl_sid(@".assets") env:@"DM_ASSET_BASE" ptr:@"asset_pointers.txt" base:@"asset_base.txt" key:@"dm_asset_base"]; }); return s; }
+ (instancetype)astcPack { static DmAssetDL *s; static dispatch_once_t once; dispatch_once(&once, ^{ s = [[DmAssetDL alloc] initDir:@"Legacy Device Pack/data" sid:dm_dl_sid(@".astcpack") env:@"DM_ASTC_PACK_BASE" ptr:nil base:@"astc_pack_base.txt" key:@"dm_astc_pack_base"]; }); return s; }
- (instancetype)initDir:(NSString *)dir sid:(NSString *)sid env:(NSString *)env ptr:(NSString *)ptr base:(NSString *)base key:(NSString *)key {
    if ((self = [super init])) {
        self.q = dispatch_queue_create("dm.assetdl", DISPATCH_QUEUE_SERIAL); self.redo = [NSMutableDictionary new];
        NSString *doc = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES)[0];
        { const char *td = getenv("DM_ROMFS_TEST_DOC"); if (td && *td && !strchr(td, '/')) doc = [doc stringByAppendingPathComponent:@(td)]; }
        __weak DmAssetDL *w = self;
        [NSNotificationCenter.defaultCenter addObserverForName:@"UIApplicationDidEnterBackgroundNotification" object:nil queue:nil usingBlock:^(NSNotification *n) { DmAssetDL *d = w; if (d) dispatch_async(d.q, ^{
            if (d.running && d.useFg) { plog("assets: 进后台，在途这一片交还给系统的后台会话"); d.useFg = 0; [d.fg invalidateAndCancel]; d.fg = nil; [d pump]; } }); }];
        self.dir = [doc stringByAppendingPathComponent:dir]; self.sid = sid; self.envBase = env; self.ptrFile = ptr; self.baseFile = base; self.defKey = key;
    }
    return self;
}
void dm_asset_dl_bg_events(NSString *ident, void (^done)(void)) { DmAssetDL *d = [ident isEqualToString:dm_dl_sid(@".astcpack")] ? DmAssetDL.astcPack : DmAssetDL.shared; d.bgDone = done; dispatch_async(d.q, ^{ (void)[d bg]; }); }
- (NSURLSession *)bg {
    if (!self.session) {
        NSURLSessionConfiguration *c = [NSURLSessionConfiguration backgroundSessionConfigurationWithIdentifier:self.sid];
        c.sessionSendsLaunchEvents = YES; c.discretionary = NO; c.allowsCellularAccess = YES; c.HTTPMaximumConnectionsPerHost = 2;
        c.timeoutIntervalForRequest = 60; c.timeoutIntervalForResource = 7 * 24 * 3600;
        NSOperationQueue *oq = [NSOperationQueue new]; oq.underlyingQueue = self.q; oq.maxConcurrentOperationCount = 1;
        self.session = [NSURLSession sessionWithConfiguration:c delegate:self delegateQueue:oq];
    }
    return self.session;
}
- (NSURLSession *)fgNew {
    [self.fg invalidateAndCancel];
    NSURLSessionConfiguration *c = NSURLSessionConfiguration.ephemeralSessionConfiguration; c.allowsCellularAccess = YES; c.HTTPMaximumConnectionsPerHost = 2;
    c.timeoutIntervalForRequest = 30; c.timeoutIntervalForResource = 3600; c.requestCachePolicy = NSURLRequestReloadIgnoringLocalAndRemoteCacheData;
    NSOperationQueue *oq = [NSOperationQueue new]; oq.underlyingQueue = self.q; oq.maxConcurrentOperationCount = 1;
    self.fg = [NSURLSession sessionWithConfiguration:c delegate:self delegateQueue:oq]; return self.fg;
}




- (void)watchTick {
    if (!self.running) return;
    CFAbsoluteTime now = CFAbsoluteTimeGetCurrent(); const char *ew = getenv("DM_ASSET_STALL_WIN"), *em = getenv("DM_ASSET_STALL_MIN"); double win = ew ? atof(ew) : 20;
    if (!self.winT) { self.winT = now; self.winBytes = self.curBytes; return; }
    if (now - self.winT < win) return;
    double rate = MAX(0, self.curBytes - self.winBytes) / (now - self.winT); self.winT = now; self.winBytes = self.curBytes;
    if (self.skipWin > 0) { self.skipWin--; if (rate > self.best) self.best = rate; return; }
    BOOL slow = em ? rate < atof(em) : (rate < 24 * 1024 || (rate * 4 < self.best && rate < 512 * 1024));
    if (rate > self.best) self.best = rate;
    if (!slow) return;
    self.heal++; self.skipWin = MIN(self.heal, 6); self.useFg = 1; self.winT = 0; self.curBytes = 0;
    plog("assets: 这一片掉速（%.0f KB/s，本轮最高 %.0f KB/s）：取消、换新连接从已下的地方接着取（第 %d 次）", rate / 1024, self.best / 1024, self.heal);
    (void)[self fgNew];
    [[self bg] getTasksWithCompletionHandler:^(NSArray *d0, NSArray *u0, NSArray<NSURLSessionDownloadTask *> *tasks) { for (NSURLSessionTask *t in tasks) if (t.state != NSURLSessionTaskStateCompleted) [t cancel];
        [self report:[self scan] note:[NSString stringWithFormat:@(dm_ui("dmDlReconnect")), self.heal]]; [self pump]; }];
}
static NSData *dl_get(NSString *url, NSTimeInterval timeout) {
    NSURL *u = [NSURL URLWithString:url]; if (!u) return nil;
    NSURLSessionConfiguration *c = NSURLSessionConfiguration.ephemeralSessionConfiguration; c.timeoutIntervalForRequest = timeout; c.timeoutIntervalForResource = timeout * 2;
    c.requestCachePolicy = NSURLRequestReloadIgnoringLocalAndRemoteCacheData;
    NSURLSession *s = [NSURLSession sessionWithConfiguration:c]; dispatch_semaphore_t sem = dispatch_semaphore_create(0); __block NSData *out = nil;
    [[s dataTaskWithURL:u completionHandler:^(NSData *d, NSURLResponse *r, NSError *e) {
        if (!e && [(NSHTTPURLResponse *)r statusCode] == 200) out = d; else plog("assets: 取 %s 失败：%s", url.UTF8String, e ? e.localizedDescription.UTF8String : [NSString stringWithFormat:@"HTTP %ld", (long)[(NSHTTPURLResponse *)r statusCode]].UTF8String);
        dispatch_semaphore_signal(sem); }] resume];
    dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER); [s finishTasksAndInvalidate]; return out;
}
static NSString *dl_trim(NSString *s) { s = [s stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet]; while ([s hasSuffix:@"/"]) s = [s substringToIndex:s.length - 1]; return s; }
static NSArray<NSString *> *dl_lines(NSString *path) {
    NSMutableArray *a = [NSMutableArray new]; NSString *t = [NSString stringWithContentsOfFile:path encoding:NSUTF8StringEncoding error:nil];
    for (NSString *l in [t componentsSeparatedByCharactersInSet:NSCharacterSet.newlineCharacterSet]) { NSString *v = dl_trim(l); if (v.length && ![v hasPrefix:@"#"]) [a addObject:v]; }
    return a;
}


- (NSString *)resolve {
    NSString *res = [NSBundle.mainBundle.resourcePath stringByAppendingPathComponent:@"dm"]; NSMutableArray<NSString *> *cand = [NSMutableArray new];
    if (getenv("DM_TEST_NO_DL_BASE")) return @(dm_ui("dmDlNoBase"));
    const char *e = getenv(self.envBase.UTF8String); if (e && *e) [cand addObject:dl_trim(@(e))];
    else {
        for (NSString *p in self.ptrFile ? dl_lines([res stringByAppendingPathComponent:self.ptrFile]) : @[]) {
            NSData *d = dl_get(p, 8); NSString *t = d ? [[NSString alloc] initWithData:d encoding:NSUTF8StringEncoding] : nil;
            NSString *first = dl_trim([t componentsSeparatedByCharactersInSet:NSCharacterSet.newlineCharacterSet].firstObject ?: @"");
            if ([first hasPrefix:@"https://"]) { plog("assets: 地址文件 %s → %s", p.UTF8String, first.UTF8String); [cand addObject:first]; }
        }
        NSString *last = [NSUserDefaults.standardUserDefaults stringForKey:self.defKey]; if (last.length) [cand addObject:last];
        [cand addObjectsFromArray:dl_lines([res stringByAppendingPathComponent:self.baseFile])];
    }
    if (!cand.count) return @(dm_ui("dmDlNoBase"));
    for (NSString *b in cand) {
        NSData *d = dl_get([b stringByAppendingString:@"/manifest.json"], 15); if (!d) continue;
        NSDictionary *j = [NSJSONSerialization JSONObjectWithData:d options:0 error:nil]; NSArray *fs = [j isKindOfClass:NSDictionary.class] ? j[@"files"] : nil;
        int64_t tot = 0; BOOL ok = [fs isKindOfClass:NSArray.class] && fs.count;
        for (NSDictionary *f in (ok ? fs : @[])) {
            NSString *nm = [f isKindOfClass:NSDictionary.class] ? f[@"name"] : nil, *sha = [f isKindOfClass:NSDictionary.class] ? f[@"sha256"] : nil; NSNumber *sz = [f isKindOfClass:NSDictionary.class] ? f[@"size"] : nil;
            if (![nm isKindOfClass:NSString.class] || !nm.length || [nm containsString:@"/"] || [nm hasPrefix:@"."] || ![sha isKindOfClass:NSString.class] || sha.length != 64 || ![sz isKindOfClass:NSNumber.class] || sz.longLongValue < 0) { ok = NO; break; }
        }
        if (!ok) { plog("assets: %s 的清单格式不对，换下一个地址", b.UTF8String); continue; }



        fs = [fs filteredArrayUsingPredicate:[NSPredicate predicateWithBlock:^BOOL(NSDictionary *f, id _) { return [f[@"name"] hasSuffix:@".data"] || [f[@"name"] hasSuffix:@".data.holes"]; }]];
        for (NSDictionary *f in fs) tot += [f[@"size"] longLongValue];
        if (!fs.count) { plog("assets: %s 的清单里没有档案文件，换下一个地址", b.UTF8String); continue; }
        self.base = b; self.files = fs; self.total = tot; [NSUserDefaults.standardUserDefaults setObject:b forKey:self.defKey];
        plog("assets: 下载地址 %s，清单 %lu 个文件共 %lld 字节", b.UTF8String, (unsigned long)fs.count, tot); return nil;
    }
    return @(dm_ui("dmDlNoServer"));
}
static int64_t dl_size(NSString *p) { NSDictionary *a = [NSFileManager.defaultManager attributesOfItemAtPath:p error:nil]; return a ? (int64_t)[a fileSize] : -1; }
- (NSString *)final:(NSString *)nm { return [self.dir stringByAppendingPathComponent:nm]; }
- (NSString *)part:(NSString *)nm { return [self.dir stringByAppendingPathComponent:[nm stringByAppendingString:@".part"]]; }
- (BOOL)have:(NSDictionary *)f { return dl_size([self final:f[@"name"]]) == [f[@"size"] longLongValue]; }
- (int64_t)scan { int64_t n = 0; for (NSDictionary *f in self.files) { if ([self have:f]) n += [f[@"size"] longLongValue]; else { int64_t p = dl_size([self part:f[@"name"]]); if (p > 0) n += MIN(p, [f[@"size"] longLongValue]); } } return n; }
static NSString *dl_sha256(NSString *path) {
    NSFileHandle *h = [NSFileHandle fileHandleForReadingAtPath:path]; if (!h) return nil; CC_SHA256_CTX c; CC_SHA256_Init(&c);
    for (;;) { @autoreleasepool { NSData *d = [h readDataOfLength:1 << 20]; if (!d.length) break; CC_SHA256_Update(&c, d.bytes, (CC_LONG)d.length); } }
    unsigned char dg[32]; CC_SHA256_Final(dg, &c); NSMutableString *s = [NSMutableString new]; for (int i = 0; i < 32; i++) [s appendFormat:@"%02x", dg[i]]; return s;
}

- (void)prepare:(void (^)(NSString *err, int64_t remain, int64_t total, BOOL cellular))done {
    self.shortBytes = 0; self.freeBytes = 0;
    dispatch_async(self.q, ^{
        NSString *err = getenv("DM_ASSET_DL_OFF") ? @"测试开关 DM_ASSET_DL_OFF：下载站当作走不通" : [self resolve]; if (err) { dispatch_async(dispatch_get_main_queue(), ^{ done(err, 0, 0, NO); }); return; }
        NSFileManager *fm = NSFileManager.defaultManager; [fm createDirectoryAtPath:self.dir withIntermediateDirectories:YES attributes:nil error:nil];
        [[NSURL fileURLWithPath:self.dir.stringByDeletingLastPathComponent] setResourceValue:@YES forKey:NSURLIsExcludedFromBackupKey error:nil];
        self.settled = [self scan]; int64_t remain = self.total - self.settled;
        NSNumber *freeb = nil; [[NSURL fileURLWithPath:self.dir] getResourceValue:&freeb forKey:NSURLVolumeAvailableCapacityForImportantUsageKey error:nil];
        { const char *ft = getenv("DM_ASSET_FREE_TEST"); if (ft) freeb = @((long long)(atof(ft) * 1e9)); }
        self.freeBytes = freeb ? freeb.longLongValue : 0;
        if (freeb && freeb.longLongValue < remain + (200ll << 20)) { self.shortBytes = remain + (200ll << 20) - freeb.longLongValue; NSString *m = [NSString stringWithFormat:@"手机剩余空间不够：还要下 %.2f GB，可用 %.2f GB", remain / 1e9, freeb.longLongValue / 1e9]; dispatch_async(dispatch_get_main_queue(), ^{ done(m, remain, self.total, NO); }); return; }


        nw_path_monitor_t mon = nw_path_monitor_create(); dispatch_semaphore_t sem = dispatch_semaphore_create(0); __block BOOL exp = NO; __block int got = 0;
        nw_path_monitor_set_queue(mon, dispatch_get_global_queue(QOS_CLASS_UTILITY, 0));
        nw_path_monitor_set_update_handler(mon, ^(nw_path_t p) { if (!got) { got = 1; exp = nw_path_is_expensive(p); dispatch_semaphore_signal(sem); } });
        nw_path_monitor_start(mon); dispatch_semaphore_wait(sem, dispatch_time(DISPATCH_TIME_NOW, 2 * NSEC_PER_SEC)); nw_path_monitor_cancel(mon);
        if (getenv("DM_ASSET_CELL_TEST")) exp = YES;
        plog("assets: 已有 %lld / %lld 字节，还要下 %lld；%s", self.settled, self.total, remain, exp ? "当前是按流量计费的网络" : "当前不是按流量计费的网络");
        dispatch_async(dispatch_get_main_queue(), ^{ done(nil, remain, self.total, exp); });
    });
}
- (void)run:(void (^)(int64_t, int64_t, NSString *))progress done:(void (^)(NSString *))done {
    dispatch_async(self.q, ^{ self.onProgress = progress; self.onDone = done; self.running = 1; self.retry = 0; self.heal = 0; self.skipWin = 0; self.best = 0; self.winT = 0; self.curBytes = 0;
        if (!self.watch) { self.watch = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, self.q); dispatch_source_set_timer(self.watch, dispatch_time(DISPATCH_TIME_NOW, 5 * NSEC_PER_SEC), 5 * NSEC_PER_SEC, NSEC_PER_SEC);
            __weak DmAssetDL *w = self; dispatch_source_set_event_handler(self.watch, ^{ [w watchTick]; }); dispatch_resume(self.watch); }
        [self pump]; });
}
- (void)report:(int64_t)done note:(NSString *)note { void (^p)(int64_t, int64_t, NSString *) = self.onProgress; int64_t t = self.total; if (p) dispatch_async(dispatch_get_main_queue(), ^{ p(done, t, note); }); }
- (void)finish:(NSString *)err { self.running = 0; void (^d)(NSString *) = self.onDone; self.onDone = nil; plog("assets: 直接下载结束：%s", err ? err.UTF8String : "全部到齐"); if (d) dispatch_async(dispatch_get_main_queue(), ^{ d(err); }); }


- (void)pump {
    if (!self.running) return;
    self.settled = [self scan];
    for (NSDictionary *f in self.files) {
        if ([self have:f]) continue;
        NSString *nm = f[@"name"], *part = [self part:nm]; int64_t size = [f[@"size"] longLongValue], p = MAX(dl_size(part), 0);
        if (p > size) { [NSFileManager.defaultManager removeItemAtPath:part error:nil]; p = 0; }
        if (p == size) {
            [self report:self.settled note:[NSString stringWithFormat:@(dm_ui("dmDlVerifying")), nm]];
            if (!size) [NSFileManager.defaultManager createFileAtPath:part contents:NSData.data attributes:nil];
            NSString *sha = dl_sha256(part);
            if ([sha isEqualToString:f[@"sha256"]]) { NSError *e = nil; [NSFileManager.defaultManager removeItemAtPath:[self final:nm] error:nil];
                if (![NSFileManager.defaultManager moveItemAtPath:part toPath:[self final:nm] error:&e]) { [self finish:[NSString stringWithFormat:@(dm_ui("dmDlWriteFail")), nm, e.localizedDescription]]; return; }
                plog("assets: %s 到齐（%lld 字节，校验值对）", nm.UTF8String, size); continue; }
            int n = self.redo[nm].intValue + 1; self.redo[nm] = @(n); [NSFileManager.defaultManager removeItemAtPath:part error:nil];
            plog("assets: 🔴 %s 校验值不对（得 %s，应 %s），删掉重下（第 %d 次）", nm.UTF8String, sha.UTF8String, [f[@"sha256"] UTF8String], n);
            if (n >= 3) { [self finish:[NSString stringWithFormat:@(dm_ui("dmDlBad3")), nm]]; return; }
            p = 0; self.settled = [self scan];
        }
        int64_t end = MIN(p + DL_CHUNK, size) - 1; NSString *desc = [NSString stringWithFormat:@"%@\t%lld\t%lld", nm, p, end];
        [[self bg] getTasksWithCompletionHandler:^(NSArray *d0, NSArray *u0, NSArray<NSURLSessionDownloadTask *> *tasks) {
            BOOL alive = NO;
            for (NSURLSessionTask *t in tasks) { if (t.state == NSURLSessionTaskStateCompleted) continue; if (!self.useFg && !alive && [t.taskDescription isEqualToString:desc]) { alive = YES; if (t.state == NSURLSessionTaskStateSuspended) [t resume]; } else [t cancel]; }
            self.winT = 0; self.curBytes = 0;
            if (alive) { plog("assets: 接回系统手里没做完的任务：%s 字节 %lld–%lld", nm.UTF8String, p, end); return; }
            NSString *enc = [nm stringByAddingPercentEncodingWithAllowedCharacters:NSCharacterSet.URLPathAllowedCharacterSet];
            NSMutableURLRequest *rq = [NSMutableURLRequest requestWithURL:[NSURL URLWithString:[NSString stringWithFormat:@"%@/data/%@", self.base, enc]]];
            [rq setValue:[NSString stringWithFormat:@"bytes=%lld-%lld", p, end] forHTTPHeaderField:@"Range"]; rq.cachePolicy = NSURLRequestReloadIgnoringLocalAndRemoteCacheData;
            NSURLSessionDownloadTask *t = [(self.useFg ? (self.fg ?: [self fgNew]) : [self bg]) downloadTaskWithRequest:rq]; t.taskDescription = desc; [t resume];
        }];
        [self report:self.settled note:nil];
        return;
    }
    [self finish:nil];
}
- (void)URLSession:(NSURLSession *)s downloadTask:(NSURLSessionDownloadTask *)t didWriteData:(int64_t)n totalBytesWritten:(int64_t)w totalBytesExpectedToWrite:(int64_t)x {
    self.curBytes = w;
    if (self.running) [self report:self.settled + w note:nil];
}
- (void)URLSession:(NSURLSession *)s downloadTask:(NSURLSessionDownloadTask *)t didFinishDownloadingToURL:(NSURL *)loc {
    NSArray<NSString *> *d = [t.taskDescription componentsSeparatedByString:@"\t"]; if (d.count != 3) return;
    NSString *nm = d[0], *part = [self part:nm]; int64_t off = d[1].longLongValue, end = d[2].longLongValue, got = dl_size(loc.path), have = MAX(dl_size(part), 0);
    NSHTTPURLResponse *r = (NSHTTPURLResponse *)t.response; NSString *cr = [r valueForHTTPHeaderField:@"Content-Range"]; NSString *wantcr = [NSString stringWithFormat:@"bytes %lld-%lld/", off, end];
    if (r.statusCode != 206 || ![cr hasPrefix:wantcr] || got != end - off + 1 || have != off) {
        plog("assets: 丢弃一段 %s 字节 %lld–%lld：HTTP %ld，Content-Range %s，收到 %lld 字节，.part 现有 %lld", nm.UTF8String, off, end, (long)r.statusCode, cr.UTF8String ?: "无", got, have); return; }
    NSFileHandle *in = [NSFileHandle fileHandleForReadingAtPath:loc.path]; if (![NSFileManager.defaultManager fileExistsAtPath:part]) [NSFileManager.defaultManager createFileAtPath:part contents:nil attributes:nil];
    NSFileHandle *out = [NSFileHandle fileHandleForWritingAtPath:part]; if (!in || !out) { plog("assets: 🔴 打不开 %s", part.UTF8String); return; }
    @try { [out seekToEndOfFile]; for (;;) { @autoreleasepool { NSData *b = [in readDataOfLength:1 << 20]; if (!b.length) break; [out writeData:b]; } } [out synchronizeFile]; }
    @catch (NSException *ex) { plog("assets: 🔴 写 %s 出错：%s", part.UTF8String, ex.reason.UTF8String); [out truncateFileAtOffset:(unsigned long long)off]; }
    [out closeFile]; [in closeFile];
}
- (void)URLSession:(NSURLSession *)s task:(NSURLSessionTask *)t didCompleteWithError:(NSError *)e {
    if (e && e.code == NSURLErrorCancelled) return;
    if (!self.running) return;
    if (e) { int wait = MIN(30, 2 << MIN(self.retry, 4)); self.retry++;
        plog("assets: 一段没取成（%s），%d 秒后自动重试（第 %d 次）", e.localizedDescription.UTF8String, wait, self.retry);
        [self report:[self scan] note:[NSString stringWithFormat:@(dm_ui("dmDlNetRetry")), wait]];
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, wait * NSEC_PER_SEC), self.q, ^{ [self pump]; }); return; }
    self.retry = 0; [self pump];
}
- (void)URLSessionDidFinishEventsForBackgroundURLSession:(NSURLSession *)s { void (^d)(void) = self.bgDone; self.bgDone = nil; if (d) dispatch_async(dispatch_get_main_queue(), d); }
@end
