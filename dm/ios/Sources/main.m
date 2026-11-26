

#import <UIKit/UIKit.h>
#include <sys/resource.h>
static int g_nofile_was, g_nofile_now;
#import <Metal/Metal.h>
#import <BackgroundAssets/BackgroundAssets.h>
#import "asset_dl.h"
#import <CommonCrypto/CommonDigest.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <dirent.h>
#include <libkern/OSCacheControl.h>
#include <sys/sysctl.h>
#include <os/proc.h>
#include <mach/mach.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
void dm_host_shutdown(const char *why);
#include "front.h"
#include "../../host/romfs_validate.h"
#include "../../host/dm_i18n.h"
static NSString *T(const char *k) { return @(dm_ui(k)); }
#import <Network/Network.h>
#include "dm_metal_cache_guard.h"

#if __IPHONE_OS_VERSION_MAX_ALLOWED < 260400


@interface BAAssetPackManager (DmIOS26_4Compatibility)
- (BOOL)assetPackIsAvailableLocallyWithIdentifier:(NSString *)assetPackIdentifier API_AVAILABLE(ios(26.4));
- (void)getLocalStatusOfAssetPackWithIdentifier:(NSString *)assetPackIdentifier
                            completionHandler:(void (^)(BAAssetPackStatus status))completionHandler API_AVAILABLE(ios(26.4));
@end
#endif

static NSString *g_mode;
static int g_log = -1;
static int g_log_tee;
static int g_log_stderr = -1;
static int g_log_pipe[2] = {-1, -1};
static pthread_t g_log_thread;
void plog(const char *fmt, ...);

static void metal_cache_stats(NSString *directory, unsigned long *file_count, unsigned long long *byte_count) {
    *file_count = 0; *byte_count = 0;
    NSFileManager *fm = NSFileManager.defaultManager;
    NSDictionary *entry = [fm attributesOfItemAtPath:directory error:nil];
    if ([entry[NSFileType] isEqualToString:NSFileTypeRegular]) {
        *file_count = 1; *byte_count = [entry[NSFileSize] unsignedLongLongValue]; return;
    }
    if (![entry[NSFileType] isEqualToString:NSFileTypeDirectory]) return;
    for (NSString *relative in [fm enumeratorAtPath:directory]) {
        NSString *path = [directory stringByAppendingPathComponent:relative];
        NSDictionary *attributes = [fm attributesOfItemAtPath:path error:nil];
        if ([attributes[NSFileType] isEqualToString:NSFileTypeRegular]) {
            (*file_count)++; *byte_count += [attributes[NSFileSize] unsignedLongLongValue];
        }
    }
}

static BOOL metal_name(NSString *name) {
    return [name rangeOfString:@"metal" options:NSCaseInsensitiveSearch].location != NSNotFound;
}

static void metal_cache_report_and_clean(void) {
    NSFileManager *fm = NSFileManager.defaultManager;
    NSString *caches = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES)[0];


    NSString *app = [caches stringByAppendingPathComponent:NSBundle.mainBundle.bundleIdentifier ?: @"app"];
    NSArray<NSString *> *targets = @[[app stringByAppendingPathComponent:@"com.apple.metal"], [app stringByAppendingPathComponent:@"com.apple.metalfe"],
                                     [caches stringByAppendingPathComponent:@"com.apple.metal"]];
    unsigned long before_files = 0; unsigned long long before_bytes = 0;
    for (NSString *t in targets) {
        unsigned long f = 0; unsigned long long b = 0; metal_cache_stats(t, &f, &b); before_files += f; before_bytes += b;
        plog("METALCACHE 现存 %lu 个 %llu 字节 %s", f, b, t.fileSystemRepresentation);
    }
    for (NSString *base in @[caches, NSTemporaryDirectory()]) {
        for (NSString *relative in [fm enumeratorAtPath:base]) {
            NSString *path = [base stringByAppendingPathComponent:relative];
            if (metal_name(path.lastPathComponent) && ![targets containsObject:path]) {
                unsigned long files = 0; unsigned long long bytes = 0;
                metal_cache_stats(path, &files, &bytes);
                plog("METALCACHE 候选 %s %llu 字节", path.fileSystemRepresentation, bytes);
            }
        }
    }
    const char *cold = getenv("DM_METAL_COLD");
    if (!cold || strcmp(cold, "1")) return;

    NSString *root = NSHomeDirectory();
    for (NSString *t in targets) {
        if (![fm attributesOfItemAtPath:t error:nil]) continue;
        if (!dm_metal_cache_path_allowed(root.fileSystemRepresentation, t.fileSystemRepresentation)) {
            plog("METALCACHE 【红】拒删 %s", t.fileSystemRepresentation); return;
        }
        NSError *error = nil;
        if (![fm removeItemAtPath:t error:&error]) {
            plog("METALCACHE 【红】删除失败 %s：%s", t.fileSystemRepresentation, error.localizedDescription.UTF8String); return;
        }
    }
    unsigned long after_files = 0; unsigned long long after_bytes = 0;
    for (NSString *t in targets) { unsigned long f = 0; unsigned long long b = 0; metal_cache_stats(t, &f, &b); after_files += f; after_bytes += b; }
    plog("METALCACHE 已清 %lu 个 %llu 字节 → 现存 %lu 个 %llu 字节", before_files, before_bytes, after_files, after_bytes);
}

static void write_all(int fd, const char *p, size_t n) {
    while (n) {
        ssize_t w = write(fd, p, n);
        if (w > 0) { p += w; n -= (size_t)w; }
        else if (w < 0 && errno == EINTR) continue;
        else break;
    }
}





static pthread_mutex_t g_log_client_mu = PTHREAD_MUTEX_INITIALIZER;
static int g_log_client = -1;
static char g_log_path[1024];
static void log_client_write(const char *p, size_t n) {
    pthread_mutex_lock(&g_log_client_mu);
    if (g_log_client >= 0) {
        while (n) { ssize_t w = write(g_log_client, p, n); if (w < 0 && errno == EINTR) continue; if (w <= 0) { close(g_log_client); g_log_client = -1; break; } p += w; n -= (size_t)w; }
    }
    pthread_mutex_unlock(&g_log_client_mu);
}
static void *log_port_reader(void *arg) {




    int c = (int)(intptr_t)arg; char b[64], acc[16] = {0}; ssize_t r;
    while ((r = read(c, b, sizeof b - 1)) > 0) {
        b[r] = 0; size_t n = strlen(acc); char cat[sizeof acc + sizeof b]; memcpy(cat, acc, n); memcpy(cat + n, b, (size_t)r + 1);
        if (strstr(cat, "dump")) { void crash_dump_now(const char *why); crash_dump_now("DM_LOG_PORT dump"); cat[0] = 0; }
        if (strstr(cat, "quit")) {
            static const char ack[] = "DM_LOG_PORT：收到 quit，退出\n";
            pthread_mutex_lock(&g_log_client_mu); write_all(c, ack, sizeof ack - 1); pthread_mutex_unlock(&g_log_client_mu);
            fprintf(stderr, "%s", ack); dm_host_shutdown("DM_LOG_PORT quit"); _exit(0);
        }
        size_t m = strlen(cat), keep = m < sizeof acc - 1 ? m : sizeof acc - 1; memcpy(acc, cat + m - keep, keep); acc[keep] = 0;
    }
    return NULL;
}
static int log_port_open(int port) {
    int s = socket(AF_INET, SOCK_STREAM, 0), one = 1;
    struct sockaddr_in a = {.sin_len = sizeof a, .sin_family = AF_INET, .sin_port = htons((uint16_t)port), .sin_addr.s_addr = htonl(INADDR_LOOPBACK)};
    if (s < 0 || setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one) || bind(s, (struct sockaddr *)&a, sizeof a) || listen(s, 1)) {
        fprintf(stderr, "【红】DM_LOG_PORT：监听 127.0.0.1:%d 失败：%s\n", port, strerror(errno)); if (s >= 0) close(s); return -1;
    }
    return s;
}
static void *log_port_main(void *arg) {
    int port = (int)(intptr_t)arg, one = 1, s = log_port_open(port);
    if (s < 0) return NULL;
    fprintf(stderr, "DM_LOG_PORT：监听 127.0.0.1:%d\n", port);
    for (;;) {
        int c = accept(s, NULL, NULL);
        if (c < 0) {
            if (errno == EINTR) continue;


            fprintf(stderr, "DM_LOG_PORT：accept 失败（%s），重建监听\n", strerror(errno)); close(s);
            while ((s = log_port_open(port)) < 0) sleep(1);
            continue;
        }
        setsockopt(c, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof one);
        pthread_mutex_lock(&g_log_client_mu);
        if (g_log_client >= 0) close(g_log_client);
        char lp[PATH_MAX]; int f = fcntl(g_log, F_GETPATH, lp) == 0 ? open(lp, O_RDONLY) : -1; char b[16384]; ssize_t r;
        if (f >= 0) { while ((r = read(f, b, sizeof b)) > 0) { const char *p = b; while (r > 0) { ssize_t w = write(c, p, (size_t)r); if (w <= 0) break; p += w; r -= w; } } close(f); }
        g_log_client = c;
        pthread_mutex_unlock(&g_log_client_mu);
        pthread_t rd; if (!pthread_create(&rd, NULL, log_port_reader, (void *)(intptr_t)c)) pthread_detach(rd);
    }
    return NULL;
}
static void *log_tee_main(void *unused) {
    (void)unused;
    char buf[4096], line[8192];
    size_t used = 0;
    for (;;) {
        ssize_t n = read(g_log_pipe[0], buf, sizeof buf);
        if (n == 0) break;
        if (n < 0) { if (errno == EINTR) continue; break; }
        for (ssize_t i = 0; i < n; i++) {
            if (used < sizeof line) line[used++] = buf[i];
            if (buf[i] == '\n') {
                pthread_mutex_lock(&g_log_client_mu); write_all(g_log, line, used); fsync(g_log); pthread_mutex_unlock(&g_log_client_mu);
                if (g_log_stderr >= 0) write_all(g_log_stderr, line, used);
                log_client_write(line, used);
                used = 0;
            } else if (used == sizeof line) {
                write_all(g_log, line, used);
                used = 0;
            }
        }
    }
    if (used) {
        write_all(g_log, line, used);
        fsync(g_log);
        write_all(g_log_stderr, line, used);
    }
    return NULL;
}

void plog(const char *fmt, ...) {
    char b[1024]; va_list ap; va_start(ap, fmt); int n = vsnprintf(b, sizeof b - 1, fmt, ap); va_end(ap);
    if (n < 0) return; if (n > (int)sizeof b - 2) n = sizeof b - 2; b[n++] = '\n';
    if (g_log_tee) write_all(2, b, (size_t)n);
    else if (g_log >= 0) { write(g_log, b, n); fsync(g_log); } else write(2, b, n);
}



static struct timeval g_pst;
static double g_T[7];
static double since_start(void) {
    if (!g_pst.tv_sec) { struct kinfo_proc kp; size_t n = sizeof kp; int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_PID, getpid()};
                         if (!sysctl(mib, 4, &kp, &n, NULL, 0) && n) g_pst = kp.kp_proc.p_starttime; }
    struct timeval tv; gettimeofday(&tv, NULL);
    return (tv.tv_sec - g_pst.tv_sec) * 1e3 + (tv.tv_usec - g_pst.tv_usec) / 1e3;
}
__attribute__((constructor)) static void boot_t1(void) { g_T[1] = since_start(); }



static const uint32_t k_code[] = { 0xD2800540                 , 0xD65F03C0           };

static void try_call(uint8_t *p, const char *tag) {
    sys_icache_invalidate(p, sizeof k_code);
    plog("%s: 跳入 %p", tag, p);
    long r = ((long (*)(void))p)();
    plog("%s: 返回 %ld（期望 42）", tag, r);
}

static void probe_anon(void) {
    uint8_t *p = mmap(NULL, 0x4000, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    plog("anon mmap RW = %p errno %d", p, p == MAP_FAILED ? errno : 0);
    if (p == MAP_FAILED) return;
    memcpy(p, k_code, sizeof k_code);
    int rc = mprotect(p, 0x4000, PROT_READ | PROT_EXEC);
    plog("anon mprotect RX = %d errno %d", rc, rc ? errno : 0);
    if (!rc) try_call(p, "anon");
}

static void probe_mapjit(void) {
    uint8_t *p = mmap(NULL, 0x4000, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANON | MAP_JIT, -1, 0);
    plog("MAP_JIT mmap RWX = %p errno %d", p, p == MAP_FAILED ? errno : 0);
    if (p == MAP_FAILED) return;
    memcpy(p, k_code, sizeof k_code);
    try_call(p, "mapjit");
}

static void probe_file(void) {
    NSString *f = [NSTemporaryDirectory() stringByAppendingPathComponent:@"code.bin"];
    int fd = open(f.fileSystemRepresentation, O_RDWR | O_CREAT | O_TRUNC, 0644);
    uint8_t pg[0x4000] = {0}; memcpy(pg, k_code, sizeof k_code); write(fd, pg, sizeof pg);
    uint8_t *p = mmap(NULL, 0x4000, PROT_READ | PROT_EXEC, MAP_PRIVATE, fd, 0);
    plog("file mmap RX = %p errno %d", p, p == MAP_FAILED ? errno : 0);
    close(fd);
    if (p != MAP_FAILED) try_call(p, "file");
}




static int claim_key(pthread_key_t want) {
    pthread_key_t got[512]; int n = 0, ok = 0;
    for (;;) {
        pthread_key_t k; if (pthread_key_create(&k, NULL)) break;
        if (k == want) { ok = 1; break; }
        if (k > want || n == 512) { pthread_key_delete(k); break; }
        got[n++] = k;
    }
    plog("claim %lu: %s，途经 %d 个键（首个 %lu）", (unsigned long)want, ok ? "拿到" : "失败", n, n ? (unsigned long)got[0] : 0ul);
    while (n) pthread_key_delete(got[--n]);
    return ok;
}
static void probe_tsd(void) {
    pthread_key_t k0; pthread_key_create(&k0, NULL); plog("当前首个空闲动态键 %lu", (unsigned long)k0); pthread_key_delete(k0);
    for (pthread_key_t want = 508; want < 512; want++) {
        if (!claim_key(want)) continue;
        pthread_setspecific(want, (void *)(0x1234 + want));
        uint64_t base; __asm__ volatile("mrs %0, tpidrro_el0" : "=r"(base));
        uint64_t v = ((uint64_t *)(base & ~7ull))[want];
        plog("槽 %lu 经 tpidrro 读回 %#llx（期望 %#lx）", (unsigned long)want, v, 0x1234 + (unsigned long)want);
    }
}



int dm_main(int argc, char **argv);
UIView *front_ios_view(CGRect frame);
void front_ios_lifecycle_install(void);
void front_ios_focus_event(int ev);
static char *g_boot_argv[8];
static int g_boot_argc = 4;








static NSDictionary *astc_info(void) {
    static NSDictionary *info; static dispatch_once_t once;
    dispatch_once(&once, ^{ NSData *d = [NSData dataWithContentsOfFile:[NSBundle.mainBundle.resourcePath stringByAppendingPathComponent:@"romfs_overlay_astc/pack.json"]];
        NSDictionary *j = d ? [NSJSONSerialization JSONObjectWithData:d options:0 error:nil] : nil;
        if ([j isKindOfClass:NSDictionary.class] && [j[@"name"] isKindOfClass:NSString.class] && [j[@"size"] isKindOfClass:NSNumber.class] && [j[@"sha256"] isKindOfClass:NSString.class]) info = j; });
    return info;
}
static BOOL astc_is_pack_dir(NSString *d) {
    NSString *nm = astc_info()[@"name"] ?: @"00-00000008.data"; NSFileManager *fm = NSFileManager.defaultManager; NSString *data = [d stringByAppendingPathComponent:@"data"];
    return [fm fileExistsAtPath:[data stringByAppendingPathComponent:nm]] || [fm fileExistsAtPath:[data stringByAppendingPathComponent:[nm stringByAppendingString:@".part"]]];
}
static int g_astc_none;
static NSString *g_astc_dir;


static NSString *astc_pack_locate(char *why, size_t n) {
    g_astc_none = 0;
    NSDictionary *info = astc_info(); if (!info) { snprintf(why, n, "%s", dm_ui("dmOldNoSupportPkg")); return nil; }
    NSFileManager *fm = NSFileManager.defaultManager; NSString *doc = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES)[0], *nm = info[@"name"];
    NSMutableArray<NSString *> *cand = [NSMutableArray arrayWithObject:[doc stringByAppendingPathComponent:@"Legacy Device Pack"]];
    for (NSString *a in [[fm contentsOfDirectoryAtPath:doc error:nil] sortedArrayUsingSelector:@selector(compare:)] ?: @[]) {
        NSString *pa = [doc stringByAppendingPathComponent:a]; [cand addObject:pa];
        for (NSString *b in [fm contentsOfDirectoryAtPath:pa error:nil] ?: @[]) [cand addObject:[pa stringByAppendingPathComponent:b]]; }
    for (NSString *d in cand) {
        NSString *f = [[d stringByAppendingPathComponent:@"data"] stringByAppendingPathComponent:nm];
        NSDictionary *at = [fm attributesOfItemAtPath:f error:nil]; if (!at) continue;
        if ((long long)at.fileSize != [info[@"size"] longLongValue]) { snprintf(why, n, dm_ui("dmOldPackSize"), d.lastPathComponent.UTF8String, at.fileSize / 1e9, [info[@"size"] longLongValue] / 1e9); continue; }
        NSString *stamp = [NSString stringWithFormat:@"%@|%llu|%.0f|%@", f, at.fileSize, at.fileModificationDate.timeIntervalSince1970, info[@"sha256"]];
        if ([[NSUserDefaults.standardUserDefaults stringForKey:@"dm_astc_pack_ok"] isEqualToString:stamp]) return d;
        CFAbsoluteTime t0 = CFAbsoluteTimeGetCurrent(); NSFileHandle *h = [NSFileHandle fileHandleForReadingAtPath:f]; CC_SHA256_CTX c; CC_SHA256_Init(&c);
        for (;;) { @autoreleasepool { NSData *x = [h readDataOfLength:8 << 20]; if (!x.length) break; CC_SHA256_Update(&c, x.bytes, (CC_LONG)x.length); } }
        unsigned char dg[32]; CC_SHA256_Final(dg, &c); NSMutableString *hex = [NSMutableString new]; for (int i = 0; i < 32; i++) [hex appendFormat:@"%02x", dg[i]];
        plog("boot: 补充包 %s 校验用时 %.1f 秒：%s", f.UTF8String, CFAbsoluteTimeGetCurrent() - t0, [hex isEqualToString:info[@"sha256"]] ? "对" : "不对");
        if (![hex isEqualToString:info[@"sha256"]]) { snprintf(why, n, dm_ui("dmOldPackBad"), d.lastPathComponent.UTF8String); continue; }
        [NSUserDefaults.standardUserDefaults setObject:stamp forKey:@"dm_astc_pack_ok"]; return d;
    }
    if (!why[0]) { snprintf(why, n, "%s", dm_ui("dmOldPackNone")); g_astc_none = 1; }
    return nil;
}

















static int g_doc_bad;
static int g_pack_wait;
static int g_doc_stray;








static BOOL ba_configured(void) {
    static int v = -1; if (v >= 0) return v;
    NSString *g = [NSBundle.mainBundle objectForInfoDictionaryKey:@"BAAppGroupID"];
    if (![[NSBundle.mainBundle objectForInfoDictionaryKey:@"BAHasManagedAssetPacks"] boolValue]) v = 0;
    else if (!g.length || ![NSFileManager.defaultManager containerURLForSecurityApplicationGroupIdentifier:g]) { v = 0; plog("boot: 本包声明了托管资源包，但签名里没有它要的 app group「%s」（自签 / 重签的包），不走托管资源包", g.UTF8String ?: "未声明"); }
    else v = 1;
    return v;
}



static BOOL g_pack_legacy_available;








int dm_s2a_pack_apply(const char *docs);
static int s2a_pack_pending(void) {
    NSString *inc = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES)[0] stringByAppendingPathComponent:@"资源包/.incoming"];
    int n = 0; for (NSString *f in [NSFileManager.defaultManager contentsOfDirectoryAtPath:inc error:nil]) if ([f hasSuffix:@".tar"]) n++;
    return n;
}

static NSString *romfs_locate(char *why, size_t n) {
    g_doc_bad = 0; g_doc_stray = 0;
    NSString *support = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES)[0];
    NSString *local = [support stringByAppendingPathComponent:@"romfs"];




    { const char *cl = getenv("DM_ROMFS_TEST_CLEAN");
      if (cl && *cl && strcmp(cl, "romfs") && !strchr(cl, '/')) {
          NSString *doc0 = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES)[0];
          BOOL r = [NSFileManager.defaultManager removeItemAtPath:[doc0 stringByAppendingPathComponent:@(cl)] error:nil];
          plog("boot: DM_ROMFS_TEST_CLEAN：Documents/%s %s，退出", cl, r ? "已删" : "不存在"); exit(0); } }
    if (!getenv("DM_ROMFS_SKIP_LOCAL") && dm_romfs_validate([local stringByAppendingPathComponent:@"data"].fileSystemRepresentation, why, n)) return local;









    NSFileManager *fm = NSFileManager.defaultManager; NSString *docdir = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES)[0];



    { const char *td = getenv("DM_ROMFS_TEST_DOC"); if (td && *td && !strchr(td, '/')) { docdir = [docdir stringByAppendingPathComponent:@(td)]; plog("boot: DM_ROMFS_TEST_DOC：按 Documents/%s 找资源", td); } }
    NSArray<NSString *> *(^subdirs)(NSString *) = ^NSArray<NSString *> *(NSString *d) {
        NSMutableArray *pre = [NSMutableArray array], *rest = [NSMutableArray array];
        for (NSString *nm in [[fm contentsOfDirectoryAtPath:d error:nil] sortedArrayUsingSelector:@selector(compare:)] ?: @[]) { BOOL dir = NO;
            if ([nm hasPrefix:@"."] || [nm.lowercaseString containsString:@"save"] || [nm isEqualToString:@"temp"] || [nm isEqualToString:@"Inbox"] || ![fm fileExistsAtPath:[d stringByAppendingPathComponent:nm] isDirectory:&dir] || !dir) continue;
            if (astc_is_pack_dir([d stringByAppendingPathComponent:nm])) {
                NSString *pk = astc_info()[@"name"] ?: @"00-00000008.data"; int other = 0;
                for (NSString *f in [fm contentsOfDirectoryAtPath:[[d stringByAppendingPathComponent:nm] stringByAppendingPathComponent:@"data"] error:nil] ?: @[]) other += [f hasSuffix:@".data"] && ![f isEqualToString:pk];
                if (!other) continue; }
            [([nm isEqualToString:@"D2R Mobile Resources"] || [nm isEqualToString:@"romfs"]) ? pre : rest addObject:[d stringByAppendingPathComponent:nm]]; }
        return [pre arrayByAddingObjectsFromArray:rest]; };



    int (^arcs)(NSString *) = ^int(NSString *d) { int k = 0; for (NSString *f in [fm contentsOfDirectoryAtPath:d error:nil] ?: @[]) k += [f hasSuffix:@".data"]; return k; };
    NSArray<NSString *> *top = subdirs(docdir); NSMutableArray<NSString *> *cand = [NSMutableArray array], *level = [NSMutableArray arrayWithObject:docdir];
    for (int depth = 0; depth <= 4 && level.count; depth++) { NSMutableArray<NSString *> *next = [NSMutableArray array];
        for (NSString *d in level) { if (arcs(d)) [cand addObject:d]; [next addObjectsFromArray:depth ? subdirs(d) : top]; }
        level = next; }
    NSString *arcdir = nil; char w2[512] = "";
    for (NSString *d in cand) if (dm_romfs_validate(d.fileSystemRepresentation, w2, sizeof w2)) { arcdir = d; break; }
    if (arcdir) {



        NSString *root = arcdir.stringByDeletingLastPathComponent;
        if (![arcdir.lastPathComponent isEqualToString:@"data"]) {
            NSString *data = [arcdir stringByAppendingPathComponent:@"data"]; NSError *e = nil; int moved = 0, all = 0; root = arcdir;
            BOOL made = [fm createDirectoryAtPath:data withIntermediateDirectories:YES attributes:nil error:&e];
            for (NSString *f in made ? [fm contentsOfDirectoryAtPath:arcdir error:nil] : @[]) { if (!([f hasSuffix:@".data"] || [f hasSuffix:@".data.holes"])) continue;
                NSError *me = nil; all++; if ([fm moveItemAtPath:[arcdir stringByAppendingPathComponent:f] toPath:[data stringByAppendingPathComponent:f] error:&me]) moved++; else e = me; }
            plog("boot: 档案文件散着放在 %s：挪进它下面的 data 文件夹 %d / %d 个%s%s", arcdir.fileSystemRepresentation, moved, all, e ? "；没成：" : "", e ? e.localizedDescription.UTF8String : "");
            if (!made || moved != all) { snprintf(why, n, dm_ui("dmDlTidyFail"), e.localizedDescription.UTF8String ?: dm_ui("dmDlUnknown")); g_doc_bad = 1; return nil; }
        }
        strlcpy(why, w2, n); plog("boot: 「D2R Mobile」里的资源：%s", root.fileSystemRepresentation); return root;
    }
    if (cand.count) {


        NSString *best = cand[0]; for (NSString *d in cand) if (arcs(d) > arcs(best)) best = d;
        NSString *holder = [best.lastPathComponent isEqualToString:@"data"] ? best.stringByDeletingLastPathComponent : best;
        char rep[1500]; dm_romfs_report(best.fileSystemRepresentation, rep, sizeof rep); NSString *where = [holder isEqualToString:docdir] ? T("dmDlAppFolder") : [NSString stringWithFormat:T("dmDlQuoted"), holder.lastPathComponent];
        snprintf(why, n, dm_ui("dmDlIncomplete"), where.UTF8String, rep);
        plog("boot: 资源不完整（%s）：%s", best.fileSystemRepresentation, rep);
        g_doc_bad = 1; return nil;
    }








    g_pack_wait = 0;
    NSString *pack = nil; BOOL avail = NO; const char *tp = getenv("DM_ROMFS_TEST_PACK");
    if (tp && *tp && !strchr(tp, '/')) { pack = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES)[0] stringByAppendingPathComponent:@(tp)]; avail = YES; }
    else if (!ba_configured()) plog("boot: 本包没有配置托管资源包，不问系统");
    else if (@available(iOS 26.4, *)) {
        NSError *e = nil; pack = [BAAssetPackManager.sharedManager URLForPath:@"romfs" error:&e].path; avail = [BAAssetPackManager.sharedManager assetPackIsAvailableLocallyWithIdentifier:@"romfs"];
        if (e) plog("boot: 托管资源包取路径报错：%s", e.localizedDescription.UTF8String);
    } else if (@available(iOS 26.0, *)) {
        NSError *e = nil; pack = [BAAssetPackManager.sharedManager URLForPath:@"romfs" error:&e].path; avail = g_pack_legacy_available;
        if (e) plog("boot: 托管资源包路径取不到（iOS 26.4 以前的接口）：%s", e.localizedDescription.UTF8String);
    }
    if (pack) {
        NSArray *in = [fm contentsOfDirectoryAtPath:[pack stringByAppendingPathComponent:@"data"] error:nil]; BOOL there = [fm fileExistsAtPath:pack];
        plog("boot: 托管资源包：系统说本地%s；路径 %s（磁盘上%s，data 里 %d 个文件）", avail ? "可用" : "不可用", pack.fileSystemRepresentation, there ? "有" : "没有", (int)in.count);
        if (avail && there) {
            char w3[512];
            NSString *pdir = [pack stringByAppendingPathComponent:@"data"]; const char *pd = pdir.fileSystemRepresentation;
            if (dm_romfs_validate(pd, w3, sizeof w3)) { strlcpy(why, w3, n); return pack; }
            char rep[1500]; dm_romfs_report(pd, rep, sizeof rep);
            snprintf(why, n, dm_ui("dmDlPackBad"), rep);
            plog("boot: 托管资源包校验不过（%s）｜目录里有：%s", w3, [in componentsJoinedByString:@" "].UTF8String);
            return nil;
        }
        if (!avail && there) g_pack_wait = 1;
    }



    if (top.count) {
        NSMutableArray *names = [NSMutableArray array]; for (NSString *d in top) if (names.count < 6) [names addObject:d.lastPathComponent];
        snprintf(why, n, dm_ui("dmDlNoData"), [names componentsJoinedByString:T("dmDlNameSep")].UTF8String, top.count > 6 ? dm_ui("dmDlEtc") : "");
        plog("boot: %s", why); g_doc_stray = 1;
    } else snprintf(why, n, "%s", dm_ui("dmDlNone"));
    return nil;
}
static int g_need_assets;




static int g_no_bc;
static char g_astc_why[512];
static void astc_handoff(UIWindow *w);
static BOOL res_dl_configured(void) {
    if (getenv("DM_TEST_NO_DL_BASE")) return NO;
    const char *e = getenv("DM_ASSET_BASE"); if (e && *e) return YES;
    for (NSString *fn in @[@"asset_base.txt", @"asset_pointers.txt"]) {
        NSString *t = [NSString stringWithContentsOfFile:[[NSBundle.mainBundle.resourcePath stringByAppendingPathComponent:@"dm"] stringByAppendingPathComponent:fn] encoding:NSUTF8StringEncoding error:nil];
        for (NSString *l in [t componentsSeparatedByCharactersInSet:NSCharacterSet.newlineCharacterSet]) { NSString *v = [l stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceCharacterSet]; if (v.length && ![v hasPrefix:@"#"]) return YES; } }
    return NO;
}
static NSString *no_assets_text(void) { return T(res_dl_configured() ? "dmDlNoAssets" : "dmDlCopyIn"); }
static BOOL astc_dl_configured(void) {
    const char *e = getenv("DM_ASTC_PACK_BASE"); if (e && *e) return YES;
    NSString *t = [NSString stringWithContentsOfFile:[[NSBundle.mainBundle.resourcePath stringByAppendingPathComponent:@"dm"] stringByAppendingPathComponent:@"astc_pack_base.txt"] encoding:NSUTF8StringEncoding error:nil];
    for (NSString *l in [t componentsSeparatedByCharactersInSet:NSCharacterSet.newlineCharacterSet]) { NSString *v = [l stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceCharacterSet]; if (v.length && ![v hasPrefix:@"#"]) return YES; }
    return NO;
}






static NSString *g_block_title, *g_block_msg, *g_block_why;
static NSString *g_romfs; static char g_romfs_why[2048];
static UIWindow *g_window;
static NSString *kv_file(NSString *path, NSString *key) {
    NSString *t = [NSString stringWithContentsOfFile:path encoding:NSUTF8StringEncoding error:nil];
    for (NSString *l in [t componentsSeparatedByCharactersInSet:NSCharacterSet.newlineCharacterSet]) if ([l hasPrefix:[key stringByAppendingString:@"="]]) return [l substringFromIndex:key.length + 1];
    return nil;
}
static NSString *pack_check(void) {
    NSString *app = NSBundle.mainBundle.resourcePath; NSFileManager *fm = NSFileManager.defaultManager;


    NSString *(^tpath)(const char *) = ^NSString *(const char *e) { if (!e) return nil; NSString *p = @(e); return [p hasPrefix:@"/"] ? p :
        [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES)[0] stringByAppendingPathComponent:p]; };
    NSString *tm = tpath(getenv("DM_TEST_PACK_MANIFEST")), *ti = tpath(getenv("DM_TEST_PACK_INFO"));
    NSString *man = [NSString stringWithContentsOfFile:tm ?: [app stringByAppendingPathComponent:@"dm/shell_manifest.txt"] encoding:NSUTF8StringEncoding error:nil];
    if (!man) { plog("boot: 安装包自检：没有壳清单（Xcode 出的包），不查"); return nil; }
    NSMutableArray<NSString *> *miss = [NSMutableArray array]; int total = 0;
    for (NSString *l in [man componentsSeparatedByCharactersInSet:NSCharacterSet.newlineCharacterSet]) {
        NSString *m = [l stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceCharacterSet]; if (!m.length) continue; total++;
        BOOL dir = NO; if (![fm fileExistsAtPath:[app stringByAppendingPathComponent:m] isDirectory:&dir] || dir != [m hasSuffix:@"/"]) [miss addObject:m]; }
    if (miss.count) {
        plog("boot: 🔴 安装包自检：壳清单 %d 项缺 %lu 项：%s", total, (unsigned long)miss.count, [miss componentsJoinedByString:@", "].UTF8String);
        return [NSString stringWithFormat:T("dmPackBadMiss"), (int)miss.count, [[miss subarrayWithRange:NSMakeRange(0, MIN(miss.count, (NSUInteger)3))] componentsJoinedByString:@", "]]; }
    NSString *sp = kv_file([app stringByAppendingPathComponent:@"dm/shell_info.txt"], @"PRODUCT") ?: (ti ? kv_file(ti, @"SHELL_PRODUCT") : nil), *pp = kv_file(ti ?: [app stringByAppendingPathComponent:@"dm/pack_info.txt"], @"PRODUCT");
    if (sp && pp && ![sp isEqualToString:pp]) { plog("boot: 🔴 安装包自检：壳的产物代号 %s，打包记录 %s，不一致", sp.UTF8String, pp.UTF8String); return [NSString stringWithFormat:T("dmPackBadProd"), sp, pp]; }
    plog("boot: 安装包自检通过：壳清单 %d 项齐全；产物代号 壳 %s / 打包记录 %s", total, sp.UTF8String ?: "无", pp.UTF8String ?: "无（旧版打包工具）");
    return nil;
}
@interface BlockVC : UIViewController @end
@implementation BlockVC
- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated]; static int shown; if (shown++) return;
    UIAlertController *a = [UIAlertController alertControllerWithTitle:g_block_title message:g_block_msg preferredStyle:UIAlertControllerStyleAlert];
    [a addAction:[UIAlertAction actionWithTitle:T("dmOldQuit") style:UIAlertActionStyleCancel handler:^(UIAlertAction *x) { plog("boot: 玩家点了「退出」（%s）", g_block_why.UTF8String); exit(0); }]];
    [self presentViewController:a animated:YES completion:nil];
    plog("boot: 已弹框（%s）｜屏上标题：%s｜正文：%s", g_block_why.UTF8String, a.title.UTF8String, [a.message stringByReplacingOccurrencesOfString:@"\n" withString:@" ⏎ "].UTF8String);
}
@end
@interface NoBcVC : UIViewController @property (strong) UILabel *label; @property (strong) UIProgressView *bar; @property (assign) BOOL shown; @end
@implementation NoBcVC
- (void)viewDidLoad {
    [super viewDidLoad]; self.view.backgroundColor = UIColor.blackColor; CGRect b = self.view.bounds;
    self.label = [[UILabel alloc] initWithFrame:CGRectMake(40, b.size.height / 2 - 90, b.size.width - 80, 100)]; self.label.textColor = [UIColor colorWithRed:0.85 green:0.72 blue:0.45 alpha:1];
    self.label.textAlignment = NSTextAlignmentCenter; self.label.numberOfLines = 4; self.label.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleTopMargin | UIViewAutoresizingFlexibleBottomMargin;
    self.bar = [[UIProgressView alloc] initWithFrame:CGRectMake(b.size.width * 0.2, b.size.height / 2 + 24, b.size.width * 0.6, 4)]; self.bar.hidden = YES;
    self.bar.progressTintColor = [UIColor colorWithRed:0.80 green:0.25 blue:0.10 alpha:1]; self.bar.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleTopMargin | UIViewAutoresizingFlexibleBottomMargin;
    [self.view addSubview:self.label]; [self.view addSubview:self.bar];
}
- (void)viewDidAppear:(BOOL)animated { [super viewDidAppear:animated]; if (!self.shown) { self.shown = YES; [self ask]; } }
- (void)ask {
    NSDictionary *info = astc_info(); UIAlertController *a;
    if (!info) a = [UIAlertController alertControllerWithTitle:T("dmOldUnsupTitle") message:T("dmOldUnsupMsg") preferredStyle:UIAlertControllerStyleAlert];
    else {
        NSString *m = [NSString stringWithFormat:T("dmOldNeedMsg"),
                       [info[@"size"] longLongValue] / 1e9, !g_astc_none && g_astc_why[0] ? [NSString stringWithFormat:T("dmOldWhySuffix"), g_astc_why] : @""];
        a = [UIAlertController alertControllerWithTitle:T("dmOldNeedTitle") message:m preferredStyle:UIAlertControllerStyleAlert];
        if (astc_dl_configured()) [a addAction:[UIAlertAction actionWithTitle:T("dmOldDl") style:UIAlertActionStyleDefault handler:^(UIAlertAction *x) { [self download]; }]];
        [a addAction:[UIAlertAction actionWithTitle:T("dmOldCopied") style:UIAlertActionStyleDefault handler:^(UIAlertAction *x) { [self recheck]; }]];
    }
    [a addAction:[UIAlertAction actionWithTitle:T("dmOldQuit") style:UIAlertActionStyleCancel handler:^(UIAlertAction *x) { plog("boot: 玩家点了「退出」（本机型不支持 BC 格式，补充包未就绪）"); exit(0); }]];
    [self presentViewController:a animated:YES completion:nil];
    plog("boot: 已弹「%s」｜屏上标题：%s｜正文：%s｜按钮：%s", info ? "需要老机型补充包" : "暂不支持本机型", a.title.UTF8String, [a.message stringByReplacingOccurrencesOfString:@"\n" withString:@" ⏎ "].UTF8String, [[a.actions valueForKey:@"title"] componentsJoinedByString:@" / "].UTF8String);
}
- (void)recheck {
    g_astc_why[0] = 0; NSString *d = astc_pack_locate(g_astc_why, sizeof g_astc_why);
    if (d) { g_astc_dir = d; plog("boot: 补充包到位 %s，接着检查游戏资源", d.UTF8String); astc_handoff(self.view.window); return; }
    plog("boot: 补充包仍不可用：%s", g_astc_why); [self ask];
}
- (void)download {
    self.label.text = T("dmOldConnecting"); self.bar.hidden = NO; self.bar.progress = 0; UIApplication.sharedApplication.idleTimerDisabled = YES;
    [DmAssetDL.astcPack prepare:^(NSString *err, int64_t remain, int64_t total, BOOL cellular) {
        if (err) { plog("boot: 补充包下载走不了：%s", err.UTF8String); g_astc_none = 0; snprintf(g_astc_why, sizeof g_astc_why, dm_ui("dmOldDlFail"), err.UTF8String); self.label.text = @""; self.bar.hidden = YES; [self ask]; return; }
        [DmAssetDL.astcPack run:^(int64_t done, int64_t tot, NSString *note) {
            self.label.text = [NSString stringWithFormat:T("dmOldProgress"), done / 1e9, tot / 1e9, tot ? 100.0 * done / tot : 0, note ? [@"\n" stringByAppendingString:note] : @""];
            self.bar.progress = tot ? (float)done / tot : 0;
        } done:^(NSString *e2) {
            if (e2) { g_astc_none = 0; snprintf(g_astc_why, sizeof g_astc_why, dm_ui("dmOldDlFail"), e2.UTF8String); self.label.text = @""; self.bar.hidden = YES; [self ask]; return; }
            [self recheck];
        }];
    }];
}
@end


static void boot_prepare(void) {
    NSString *res = [NSBundle.mainBundle.resourcePath stringByAppendingPathComponent:@"dm"];
    NSString *doc = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES)[0];
    NSString *support = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES)[0];
    NSString *romfs = [support stringByAppendingPathComponent:@"romfs"];
    NSFileManager *fm = NSFileManager.defaultManager;
    [fm createDirectoryAtPath:romfs withIntermediateDirectories:YES attributes:nil error:nil];
    NSURL *romURL = [NSURL fileURLWithPath:romfs]; NSNumber *excluded = @YES; [romURL setResourceValue:excluded forKey:NSURLIsExcludedFromBackupKey error:nil];
    const char *romwhy = g_romfs_why;
    NSString *found = g_romfs;
    if (!found) {
        plog("boot: 🔴 RomFS 没经过起宿主前的校验（不应发生）：%s", romwhy);
        plog("boot: 请推送原版 RomFS：xcrun devicectl device copy to --device <设备> --domain-type appDataContainer --domain-identifier <包名> --source $DM_ROOT/data/switch/3.4.95035/Update/RomFS --destination 'Library/Application Support/romfs'");
        abort();
    }
    romfs = found;
    plog("boot: RomFS 校验通过：%s，挂载 %s", romwhy, romfs.fileSystemRepresentation);
    plog("boot: 文件描述符软上限 %d → %d", g_nofile_was, g_nofile_now);
    NSString *oldrom = [doc stringByAppendingPathComponent:@"romfs"]; if ([fm fileExistsAtPath:oldrom] && ![romfs isEqualToString:oldrom]) plog("boot: 提示：Documents/romfs 没在用（资源取自 %s），可删除", romfs.fileSystemRepresentation);
    if (getenv("DM_SAVE_FRESH")) {


        const char *sf = getenv("DM_SAVE_FRESH");
        NSString *rn = strcmp(sf, "1") ? [@"save_ref_" stringByAppendingString:@(sf)] : @"save_ref";
        NSString *sv = [doc stringByAppendingPathComponent:@"save.test"], *ref = [NSBundle.mainBundle.resourcePath stringByAppendingPathComponent:rn];
        setenv("DM_SAVE", sv.fileSystemRepresentation, 1);
        NSError *e = nil;




        void (^writable)(void) = ^{
            [fm setAttributes:@{NSFilePosixPermissions: @0755} ofItemAtPath:sv error:nil];
            for (NSString *rel in [fm enumeratorAtPath:sv]) {
                NSString *pth = [sv stringByAppendingPathComponent:rel]; BOOL dir = NO; [fm fileExistsAtPath:pth isDirectory:&dir];
                [fm setAttributes:@{NSFilePosixPermissions: dir ? @0755 : @0644} ofItemAtPath:pth error:nil];
            }
        };
        if ([fm fileExistsAtPath:sv]) writable();


        for (NSString *x in @[@".cloudstate", @".cloudbak"]) { NSString *p = [sv stringByAppendingString:x]; if ([fm fileExistsAtPath:p]) [fm removeItemAtPath:p error:nil]; }
        if ([fm fileExistsAtPath:sv] && ![fm removeItemAtPath:sv error:&e]) plog("boot: 删旧存档失败 %s", e.localizedDescription.UTF8String);
        else if (!strcmp(sf, "empty")) { [fm createDirectoryAtPath:sv withIntermediateDirectories:YES attributes:nil error:nil]; plog("boot: 本局从空存档起（Documents/save.test 已清空 = 首次启动；Documents/save 未动）"); }
        else if (![fm copyItemAtPath:ref toPath:sv error:&e]) plog("boot: 标准存档副本失败 %s", e.localizedDescription.UTF8String);
        else if (writable(), access([sv stringByAppendingPathComponent:@"SharedStashSoftCoreV2.d2i"].fileSystemRepresentation, W_OK) != 0) plog("boot: 🔴 存档副本仍不可写");
        else plog("boot: 本局用标准存档 %s 新副本（Documents/save.test；Documents/save 未动）", rn.UTF8String);
    } else if (getenv("DM_SAVE_TEST")) {
        NSString *sv = [doc stringByAppendingPathComponent:@"save.test"];
        setenv("DM_SAVE", sv.fileSystemRepresentation, 1); plog("boot: 本局沿用测试存档 Documents/save.test（Documents/save 未动）");
    }
    for (NSString *d in @[@"save", @"temp"])
        [fm createDirectoryAtPath:[doc stringByAppendingPathComponent:d] withIntermediateDirectories:YES attributes:nil error:nil];
    { void dm_custom_init(const char *dir); dm_custom_init(doc.fileSystemRepresentation); }
    setenv("DM_ROMFS", romfs.fileSystemRepresentation, 0);
    NSString *bundledOverlay = [NSBundle.mainBundle.resourcePath stringByAppendingPathComponent:g_astc_dir ? @"romfs_overlay_astc" : @"romfs_overlay"];
    {
        int dm_custom_get(const char *key);
        NSString *vdir = [NSBundle.mainBundle.resourcePath stringByAppendingPathComponent:g_astc_dir ? @"romfs_overlay_astc_variants" : @"romfs_overlay_variants"];
        NSArray<NSString *> *vs = [fm contentsOfDirectoryAtPath:vdir error:nil] ?: @[];
        NSMutableSet<NSString *> *all = [NSMutableSet set], *on = [NSMutableSet set];
        for (NSString *v in vs) [all addObjectsFromArray:[v componentsSeparatedByString:@"+"]];
        for (NSString *k in all) if (dm_custom_get(k.UTF8String)) [on addObject:k];
        for (NSString *v in vs) if ([[NSSet setWithArray:[v componentsSeparatedByString:@"+"]] isEqualToSet:on]) { bundledOverlay = [vdir stringByAppendingPathComponent:v]; break; }
        if (on.count && [bundledOverlay.lastPathComponent isEqualToString:@"romfs_overlay"]) plog("boot: 🔴 开关组合 %s 没有对应的覆盖层，按全关启动", [on.allObjects componentsJoinedByString:@"+"].UTF8String);
    }
    if (g_astc_dir) bundledOverlay = [NSString stringWithFormat:@"%@:%@", bundledOverlay, g_astc_dir];
    if (g_astc_dir || [fm fileExistsAtPath:bundledOverlay]) { setenv("DM_ROMFS_OVERLAY", bundledOverlay.fileSystemRepresentation, 1); plog("boot: RomFS 覆盖层 %s", bundledOverlay.fileSystemRepresentation); }
    else plog("boot: 🔴 App 内缺 romfs_overlay（底部 UI v4），按原版 HUD 启动");
    setenv("DM_SAVE", [doc stringByAppendingPathComponent:@"save"].fileSystemRepresentation, 0);
    setenv("DM_TEMP", [doc stringByAppendingPathComponent:@"temp"].fileSystemRepresentation, 0);
    setenv("DM_NVN_SHMAP", [res stringByAppendingPathComponent:@"shader_map.tsv"].fileSystemRepresentation, 0);
    setenv("DM_MSL_DIR", [NSBundle.mainBundle.resourcePath stringByAppendingPathComponent:@"msl"].fileSystemRepresentation, 0);


    if (chdir(doc.fileSystemRepresentation)) plog("boot: 🔴 chdir Documents 失败 %s", strerror(errno));
    g_boot_argv[0] = "dmhost"; g_boot_argv[1] = strdup(res.fileSystemRepresentation); g_boot_argv[2] = "main"; g_boot_argv[3] = "sdk";

}
static void boot_begin(void) {
    double tp0 = since_start(); boot_prepare();
    plog("启动计时 boot_prepare %.1f → %.1f ms（开日志 %.1f ms 起）", tp0, since_start(), g_T[2]);


    { void dm_sharelog_boot(void); dm_sharelog_boot(); }
    { void dm_update_boot(void); dm_update_boot(); }
    if (!getenv("DM_NO_PRELOAD")) dm_host_preload_start(g_boot_argc, g_boot_argv);
    else plog("boot: DM_NO_PRELOAD——宿主装载不提前（对照）");
}
static void *boot_thread(void *arg) {
    (void)arg;
    g_T[5] = since_start();
    char **argv = g_boot_argv;
    { uint64_t r = front_game_res(1), r0 = front_game_res(0);
      plog("boot: 前端给游戏的输出分辨率 主机 %llux%llu / 掌机 %llux%llu", r & 0xffffffff, r >> 32, r0 & 0xffffffff, r0 >> 32); }
    g_T[6] = since_start();
    plog("启动计时 T5 boot_thread 入口 %.1f ms · T6 dm_main 前 %.1f ms", g_T[5], g_T[6]);
    { void dm_cloud_boot(void); dm_cloud_boot(); }
    plog("boot: dm_main %s", argv[1]);
    int rc = dm_main(g_boot_argc, argv);
    plog("boot: dm_main 返回 %d", rc);
    plog("DONE");
    return NULL;
}
static void run_boot(void) {
    pthread_attr_t a; pthread_attr_init(&a); pthread_attr_setstacksize(&a, 64 << 20);
    pthread_t t; pthread_create(&t, &a, boot_thread, NULL);
}









static int g_host_up;
static void log_promote(void) {
    static int done; if (done || ![g_mode isEqualToString:@"boot"]) return; done = 1;
    NSString *doc = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES)[0];
    const char *cur = [doc stringByAppendingPathComponent:@"boot.txt"].fileSystemRepresentation, *prev = [doc stringByAppendingPathComponent:@"boot.previous.txt"].fileSystemRepresentation;




    if (rename(cur, prev) != 0 && errno != ENOENT) { plog("boot: 上一轮日志没能改名保留（%s），本轮日志留在 boot.launch.txt", strerror(errno)); return; }
    if (rename(g_log_path, cur) != 0) plog("boot: 本轮日志没能改名成 boot.txt（%s），留在 boot.launch.txt", strerror(errno));
}
static void host_start(void) {
    if (g_host_up) return;
    if (getenv("DM_TEST_BG_RELAUNCH")) return;
    { NSString *r = romfs_locate(g_romfs_why, sizeof g_romfs_why);
      if (!r) { plog("boot: 起宿主前资源校验没过（%s），回到资源界面", g_romfs_why); if (g_window) astc_handoff(g_window); return; }
      g_romfs = r; }
#ifdef DM_SHELL


    { extern int dm_shader_prep_begin(void (^)(void)); static int asked; if (!asked) { asked = 1; if (dm_shader_prep_begin(^{ host_start(); })) return; } }
#endif
    g_host_up = 1; plog("boot: 场景到前台，起宿主（进程已活 %.1f ms）", since_start());
    { const char *mw = getenv("DM_TEST_MEMWARN");
      if (mw && atof(mw) > 0) dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(atof(mw) * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{ UIApplication *app = UIApplication.sharedApplication; [app.delegate applicationDidReceiveMemoryWarning:app]; }); }
    boot_begin();
    front_ios_lifecycle_install();
    run_boot();
}



static void probe_msl(void) {
    NSString *dir = [NSBundle.mainBundle.resourcePath stringByAppendingPathComponent:@"msl"];
    id<MTLDevice> dev = MTLCreateSystemDefaultDevice();
    NSArray *fs = [[NSFileManager.defaultManager contentsOfDirectoryAtPath:dir error:nil] filteredArrayUsingPredicate:[NSPredicate predicateWithFormat:@"self ENDSWITH '.metallib'"]];
    int lib_ok = 0, fn_ok = 0, n = 0; CFAbsoluteTime t0 = CFAbsoluteTimeGetCurrent();
    plog("设备 %s，metallib %lu 个", dev.name.UTF8String, (unsigned long)fs.count);
    for (NSString *f in fs) {
        n++; NSError *e = nil;
        id<MTLLibrary> lib = [dev newLibraryWithURL:[NSURL fileURLWithPath:[dir stringByAppendingPathComponent:f]] error:&e];
        if (!lib) { if (n - lib_ok <= 5) plog("加载失败 %s：%s", f.UTF8String, e.localizedDescription.UTF8String); continue; }
        lib_ok++;
        NSData *jd = [NSData dataWithContentsOfFile:[dir stringByAppendingPathComponent:[f.stringByDeletingPathExtension stringByAppendingString:@".json"]]];
        NSDictionary *j = jd ? [NSJSONSerialization JSONObjectWithData:jd options:0 error:nil] : nil;
        if ([lib newFunctionWithName:j[@"entry"]]) fn_ok++; else if (n - fn_ok <= 5) plog("取入口失败 %s", f.UTF8String);
    }
    plog("加载成功 %d/%d，取入口成功 %d/%d，用时 %.1f 秒", lib_ok, n, fn_ok, n, CFAbsoluteTimeGetCurrent() - t0);
}

static void run_mode(void) {
    const char *m = g_mode.UTF8String;
    if (!strcmp(m, "anon")) probe_anon();
    else if (!strcmp(m, "mapjit")) probe_mapjit();
    else if (!strcmp(m, "file")) probe_file();
    else if (!strcmp(m, "tsd")) probe_tsd();
    else if (!strcmp(m, "msl")) probe_msl();
    else if (!strcmp(m, "blank")) { plog("blank：空白 Metal 60 帧清屏（发热负对照）"); return; }
    else if (!strcmp(m, "s2a-pack")) {
        int dm_s2a_pack_apply(const char *docs);
        int rc = dm_s2a_pack_apply(NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES)[0].fileSystemRepresentation);
        plog("s2a-pack：%s", rc ? "解包失败（原因见 unpack.log）" : "解开了"); exit(rc ? 1 : 0);
    }
    else plog("未知模式 %s", m);
    plog("DONE");
}

@interface App : UIResponder <UIApplicationDelegate>
@end
static void run_boot(void);


@interface AssetVC : UIViewController <BAManagedAssetPackDownloadDelegate>
@property (strong) UILabel *label; @property (strong) UIProgressView *bar; @property (strong) UIButton *retry;
@property (assign) int64_t last; @property (assign) CFAbsoluteTime lastT; @property (assign) double rate;
@property (strong) nw_path_monitor_t netmon; @property (assign) int netDown, stopped;






@property (strong) UIButton *useDirect, *share; @property (strong) dispatch_source_t poll; @property (assign) CFAbsoluteTime packT, packFull, logT, testAvailAt;
@property (assign) int64_t packBytes, packDone; @property (assign) int gone, slowShown, finished, textKind, packStop;
@property (assign) BOOL legacyStatusPending;
@property (assign) CFAbsoluteTime legacyStatusT;
@property (assign) NSUInteger legacyStatus;
@property (strong) NSError *legacyStatusError;
@property (copy) void (^legacyStatusCompletion)(BOOL available);
@end
@implementation AssetVC
- (void)netUp { if (self.netDown && self.stopped) { plog("assets: 网络通了，自动重试"); self.netDown = 0; [self start]; } else self.netDown = 0; }
- (void)watchNet {
    nw_path_monitor_t m = nw_path_monitor_create(); self.netmon = m; nw_path_monitor_set_queue(m, dispatch_get_main_queue()); __weak AssetVC *w = self;
    nw_path_monitor_set_update_handler(m, ^(nw_path_t pth) { AssetVC *v = w; if (!v) return; if (nw_path_get_status(pth) == nw_path_status_satisfied) [v netUp]; else v.netDown = 1; });
    nw_path_monitor_start(m);
}




- (UIRectEdge)preferredScreenEdgesDeferringSystemGestures { return UIRectEdgeAll; }
- (BOOL)prefersHomeIndicatorAutoHidden { return YES; }
- (BOOL)prefersStatusBarHidden { return YES; }
- (void)viewDidLoad {
    [super viewDidLoad]; self.view.backgroundColor = UIColor.blackColor;
    CGRect b = self.view.bounds;
    self.label = [[UILabel alloc] initWithFrame:CGRectMake(40, b.size.height / 2 - 150, b.size.width - 80, 160)];
    self.label.textColor = [UIColor colorWithRed:0.85 green:0.72 blue:0.45 alpha:1]; self.label.textAlignment = NSTextAlignmentCenter; self.label.numberOfLines = 8; self.label.adjustsFontSizeToFitWidth = YES; self.label.minimumScaleFactor = 0.5;
    self.label.font = [UIFont systemFontOfSize:17 weight:UIFontWeightMedium]; self.label.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleTopMargin | UIViewAutoresizingFlexibleBottomMargin;
    self.bar = [[UIProgressView alloc] initWithFrame:CGRectMake(b.size.width * 0.2, b.size.height / 2 + 24, b.size.width * 0.6, 4)];
    self.bar.progressTintColor = [UIColor colorWithRed:0.80 green:0.25 blue:0.10 alpha:1]; self.bar.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleTopMargin | UIViewAutoresizingFlexibleBottomMargin;
    self.retry = [UIButton buttonWithType:UIButtonTypeSystem]; self.retry.frame = CGRectMake(b.size.width / 2 - 60, b.size.height / 2 + 50, 120, 44);
    [self.retry setTitle:T("dmDlRetry") forState:UIControlStateNormal]; self.retry.hidden = YES; self.retry.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleRightMargin | UIViewAutoresizingFlexibleTopMargin | UIViewAutoresizingFlexibleBottomMargin;
    [self.retry addTarget:self action:@selector(start) forControlEvents:UIControlEventTouchUpInside];
    self.useDirect = [UIButton buttonWithType:UIButtonTypeSystem]; [self.useDirect setTitle:T("dmDlUseDirect") forState:UIControlStateNormal]; self.useDirect.hidden = YES;
    [self.useDirect addTarget:self action:@selector(useDirectTap) forControlEvents:UIControlEventTouchUpInside];
    self.share = [UIButton buttonWithType:UIButtonTypeSystem]; [self.share setTitle:T("dmPauseShareLog") forState:UIControlStateNormal]; self.share.hidden = YES;
    [self.share addTarget:self action:@selector(shareTap) forControlEvents:UIControlEventTouchUpInside];
    [self.view addSubview:self.label]; [self.view addSubview:self.bar]; [self.view addSubview:self.retry]; [self.view addSubview:self.useDirect]; [self.view addSubview:self.share];
    UIApplication.sharedApplication.idleTimerDisabled = YES;
    [self watchNet];
    [self start];
}
- (void)viewDidLayoutSubviews {
    [super viewDidLayoutSubviews]; CGRect b = self.view.bounds; NSMutableArray<UIButton *> *row = [NSMutableArray array]; CGFloat w = 0;
    for (UIButton *x in @[self.retry, self.useDirect, self.share]) if (!x.hidden) { [x sizeToFit]; [row addObject:x]; w += MAX(120, x.bounds.size.width + 24) + (row.count > 1 ? 16 : 0); }
    CGFloat x0 = (b.size.width - w) / 2;
    for (UIButton *x in row) { CGFloat bw = MAX(120, x.bounds.size.width + 24); x.frame = CGRectMake(x0, b.size.height / 2 + 50, bw, 44); x0 += bw + 16; }
}
- (void)showBtn:(UIButton *)x { dispatch_async(dispatch_get_main_queue(), ^{ x.hidden = NO; [self.view setNeedsLayout]; }); }
- (void)importPack:(int)total {
    static int busy; if (busy) return; busy = 1;
    plog("assets: 「D2R Mobile」里有 S2Arm 推来没解开的资源包 %d 批，就地解开", total);
    [self say:[NSString stringWithFormat:T("dmImportPack"), 0, total]]; self.bar.progress = 0;
    dispatch_source_t tm = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_main_queue());
    dispatch_source_set_timer(tm, DISPATCH_TIME_NOW, NSEC_PER_SEC / 2, NSEC_PER_SEC / 10);
    dispatch_source_set_event_handler(tm, ^{ int done = total - s2a_pack_pending(); if (done < 0) done = 0;
        [self say:[NSString stringWithFormat:T("dmImportPack"), done, total]]; self.bar.progress = (float)done / total; });
    dispatch_resume(tm);
    NSString *docs = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES)[0];
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
        int rc = dm_s2a_pack_apply(docs.fileSystemRepresentation);
        dispatch_async(dispatch_get_main_queue(), ^{
            dispatch_source_cancel(tm); busy = 0;
            if (!rc) { plog("assets: 推来的资源包解开了，接着找资源"); self.bar.progress = 1; [self start]; return; }
            NSString *lg = [NSString stringWithContentsOfFile:[docs stringByAppendingPathComponent:@"unpack.log"] encoding:NSUTF8StringEncoding error:nil] ?: @"";
            NSString *last = [[lg stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet] componentsSeparatedByString:@"\n"].lastObject ?: @"";
            NSRange r = [last rangeOfString:@"解包失败："]; NSString *why = r.location == NSNotFound ? last : [last substringFromIndex:NSMaxRange(r)];
            plog("assets: 推来的资源包解不开（返回 %d）：%s", rc, why.UTF8String);
            [self say:[NSString stringWithFormat:T("dmImportFail"), why]]; [self showBtn:self.retry]; [self showBtn:self.share];
        });
    });
}
- (void)shareTap { plog("assets: 玩家点了分享诊断日志"); void dm_sharelog_share(void); dm_sharelog_share(); }
- (void)useDirectTap {
    plog("assets: 玩家点了改用直接下载（托管已等 %.0f 秒，进度 %.2f / %.2f GB%s）", CFAbsoluteTimeGetCurrent() - self.packT, self.packDone / 1e9, self.packBytes / 1e9, self.packFull ? "，已到 100%" : "");
    self.gone = 1; if (self.poll) { dispatch_source_cancel(self.poll); self.poll = nil; }
    self.useDirect.hidden = YES; [self.view setNeedsLayout]; [self direct:nil];
}
- (void)offerDirect:(int)why {
    if (self.gone) return;
    if (!res_dl_configured()) { static int lg; if (!lg++) plog("assets: 托管走不下去（%d：1 安装超时 2 暂停 3 报错），但本包没配资源下载地址，不给「改用直接下载」", why); return; }
    plog("assets: 托管走不下去（%d：1 安装超时 2 暂停 3 报错）→ 露出「改用直接下载」", why); [self showBtn:self.useDirect]; [self showBtn:self.share];
}
static double slow_s(void) { const char *t = getenv("DM_TEST_BA_SLOW"); return t ? atof(t) : 300; }
static int64_t free_bytes(void) {
    const char *t = getenv("DM_TEST_BA_FREE"); if (t) return (int64_t)(atof(t) * 1e9);
    NSURL *u = [NSURL fileURLWithPath:NSHomeDirectory()]; NSNumber *v = nil; [u getResourceValue:&v forKey:NSURLVolumeAvailableCapacityForImportantUsageKey error:nil]; return v.longLongValue;
}
- (void)legacyPackReady {
    if (self.gone || g_host_up) return;
    g_pack_legacy_available = YES;
    [self ready];
}
- (void)queryLegacyPack:(void (^)(BOOL available))completion API_AVAILABLE(ios(26.0)) {


    self.legacyStatusCompletion = completion;
    if (self.legacyStatusPending) return;
    self.legacyStatusPending = YES;
    [BAAssetPackManager.sharedManager getStatusOfAssetPackWithIdentifier:@"romfs" completionHandler:^(BAAssetPackStatus st, NSError *e) {
        dispatch_async(dispatch_get_main_queue(), ^{
            self.legacyStatusPending = NO; self.legacyStatusT = CFAbsoluteTimeGetCurrent(); self.legacyStatus = st; self.legacyStatusError = e;
            void (^done)(BOOL) = self.legacyStatusCompletion; self.legacyStatusCompletion = nil;
            if (self.gone || g_host_up) return;


            if (!e && (st & BAAssetPackStatusDownloaded)) g_pack_legacy_available = YES;
            if (e) plog("assets: 托管资源包状态查询失败（iOS 26.4 以前的接口）：%s", e.localizedDescription.UTF8String);
            if (done) done(g_pack_legacy_available);
        });
    }];
}
- (BOOL)packAvail {
    if (getenv("DM_TEST_BA")) return self.testAvailAt > 0 && CFAbsoluteTimeGetCurrent() >= self.testAvailAt;
    if (@available(iOS 26.4, *)) return [BAAssetPackManager.sharedManager assetPackIsAvailableLocallyWithIdentifier:@"romfs"];
    if (@available(iOS 26.0, *)) {
        if (!g_pack_legacy_available && !self.legacyStatusPending && CFAbsoluteTimeGetCurrent() - self.legacyStatusT >= 30)
            [self queryLegacyPack:^(BOOL available) { if (available) [self ready]; }];
        return g_pack_legacy_available;
    }
    return NO;
}
- (void)logPack:(BOOL)avail free:(int64_t)fr {
    CFAbsoluteTime now = CFAbsoluteTimeGetCurrent(); double w = now - self.packT, f = self.packFull ? now - self.packFull : -1, d = self.packDone / 1e9, t = self.packBytes / 1e9, g = fr / 1e9; int a = avail;
    if (getenv("DM_TEST_BA")) { plog("assets: 托管状态（测试现场）：已等 %.0f 秒，进度 %.2f / %.2f GB（100%% 后 %.0f 秒，-1 = 没到），系统说本地可用 %d，可用空间 %.2f GB", w, d, t, f, a, g); return; }
    if (@available(iOS 26.4, *)) {
        [BAAssetPackManager.sharedManager getLocalStatusOfAssetPackWithIdentifier:@"romfs" completionHandler:^(BAAssetPackStatus st) {
            plog("assets: 托管状态：已等 %.0f 秒，进度 %.2f / %.2f GB（100%% 后 %.0f 秒，-1 = 没到），系统说本地可用 %d，可用空间 %.2f GB；系统状态位 0x%lx（1 可下载 2 有更新 4 最新 8 过期 0x10 已撤 0x20 下载中 0x40 已下完）",
                 w, d, t, f, a, g, (unsigned long)st); }];
    } else if (@available(iOS 26.0, *)) {
        plog("assets: 托管资源包（iOS 26.4 以前的接口）：已等 %.0f 秒，%.2f / %.2f GB，满额已 %.0f 秒，可用 %d，可用空间 %.2f GB，状态 0x%lx，查询在途 %d，错误 %s",
             w, d, t, f, a, g, (unsigned long)self.legacyStatus, self.legacyStatusPending, self.legacyStatusError.localizedDescription.UTF8String ?: "无");
    } else plog("assets: 托管状态：已等 %.0f 秒，进度 %.2f / %.2f GB（100%% 后 %.0f 秒，-1 = 没到），系统说本地可用 %d，可用空间 %.2f GB", w, d, t, f, a, g);
}
- (void)packText:(int64_t)fr {
    NSString *dev = UIDevice.currentDevice.userInterfaceIdiom == UIUserInterfaceIdiomPad ? @"iPad" : @"iPhone";
    int k = self.packBytes > 0 && fr < self.packBytes ? 3 : self.slowShown ? 2 : 1;
    NSString *t = k == 3 ? [NSString stringWithFormat:T("dmDlInstallSpace"), self.packBytes / 1e9, dev, fr / 1e9, (self.packBytes - fr) / 1e9, dev]
                : k == 2 ? [NSString stringWithFormat:T(res_dl_configured() ? "dmDlPackSlow" : "dmDlPackSlowNoDl"), (int)MAX(1, lround(slow_s() / 60))]
                : [NSString stringWithFormat:T("dmDlInstalling"), self.packBytes / 1e9, fr / 1e9];
    if (k != self.textKind) { self.textKind = k; [self say:t]; }
    else if (![self.label.text isEqualToString:t]) dispatch_async(dispatch_get_main_queue(), ^{ self.label.text = t; });
}
- (void)pollPack {
    if (self.gone) return;
    CFAbsoluteTime now = CFAbsoluteTimeGetCurrent(); BOOL avail = [self packAvail]; int64_t fr = free_bytes();
    if (now - self.logT >= 30) { self.logT = now; [self logPack:avail free:fr]; }
    if (avail) { plog("assets: 轮询：系统说资源包本地可用（完成回调%s）→ 进游戏", self.finished ? "也到了" : "没到"); [self ready]; return; }
    if (!self.packFull || self.packStop) return;
    if (!self.slowShown && now - self.packFull >= slow_s()) { self.slowShown = 1; plog("assets: 进度 100%% 后 %.0f 秒系统仍没装好", now - self.packFull); [self offerDirect:1]; }
    [self packText:fr];
}
- (void)watchPack {
    if (self.poll) return; self.packT = CFAbsoluteTimeGetCurrent(); self.logT = 0;
    dispatch_source_t s = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_main_queue()); self.poll = s; __weak AssetVC *w = self;
    dispatch_source_set_timer(s, dispatch_time(DISPATCH_TIME_NOW, 5 * NSEC_PER_SEC), 5 * NSEC_PER_SEC, NSEC_PER_SEC / 2);
    dispatch_source_set_event_handler(s, ^{ [w pollPack]; }); dispatch_resume(s);
}






- (void)fakeBA:(NSString *)sc API_AVAILABLE(ios(26.0)) {
    plog("assets: 测试现场 DM_TEST_BA=%s", sc.UTF8String); self.packBytes = 4130000000LL; [self watchPack]; [self say:T("dmDlDownloading")];
    NSProgress *g = [NSProgress progressWithTotalUnitCount:self.packBytes]; BAAssetPack *np = nil;
    for (int i = 1; i <= 10; i++) dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(i * 0.3 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{ g.completedUnitCount = self.packBytes * i / 10; [self downloadOfAssetPack:np hasProgress:g]; });
    if ([sc isEqualToString:@"silent"]) self.testAvailAt = CFAbsoluteTimeGetCurrent() + 11;
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 6 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
        if ([sc isEqualToString:@"finish"]) [self downloadOfAssetPackFinished:np];
        if ([sc isEqualToString:@"fail"]) [self downloadOfAssetPack:np failedWithError:[NSError errorWithDomain:@"DmTestBA" code:1 userInfo:@{NSLocalizedDescriptionKey: @"test failure"}]]; });
    const char *tp = getenv("DM_TEST_BA_TAP"); if (tp) dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(atof(tp) * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        if (self.useDirect.hidden) plog("【红】DM_TEST_BA_TAP：「改用直接下载」没露出来"); else [self useDirectTap]; });
}
- (void)say:(NSString *)t { plog("assets: %s", t.UTF8String); dispatch_async(dispatch_get_main_queue(), ^{ self.label.text = t; }); }
- (void)fail:(NSString *)what error:(NSError *)e {


    plog("assets: %s失败：%s（%s %ld）", what.UTF8String, e.localizedDescription.UTF8String, e.domain.UTF8String, (long)e.code);
    [self say:no_assets_text()];
    [self showBtn:self.retry];
}
- (void)start {
    self.retry.hidden = YES; self.stopped = 0; self.last = 0; self.lastT = CFAbsoluteTimeGetCurrent(); self.packStop = 0; self.textKind = 0;
    char why[2048];
    if (getenv("DM_TEST_BA") && !self.gone) { if (@available(iOS 26.0, *)) [self fakeBA:@(getenv("DM_TEST_BA"))]; return; }
    { int np = s2a_pack_pending(); if (np) { [self importPack:np]; return; } }
    if (!getenv("DM_ASSETS_TEST") && romfs_locate(why, sizeof why)) { [self ready]; return; }
    if (!getenv("DM_ASSETS_TEST") && g_doc_bad) { [self direct:@(why)]; return; }
    if (self.gone) { [self direct:nil]; return; }
    NSString *stray = !getenv("DM_ASSETS_TEST") && g_doc_stray ? @(why) : nil;
    if (getenv("DM_ROMFS_TEST_PACK")) { [self ready]; return; }
    if (getenv("DM_ASSETS_TEST") && !strcmp(getenv("DM_ASSETS_TEST"), "direct")) { [self direct:nil]; return; }
    if (getenv("DM_ASSETS_TEST")) {
        [self say:T("dmDlTestProgress")];
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 2 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{ [self ready]; });
        return;
    }
    if (!ba_configured()) { [self direct:stray]; return; }
    if (@available(iOS 26.0, *)) {
        void (^download)(void) = ^{
            [self say:T("dmDlConnecting")];
            BAAssetPackManager *m = BAAssetPackManager.sharedManager; m.delegate = self;
            [m getAssetPackWithIdentifier:@"romfs" completionHandler:^(BAAssetPack *pack, NSError *e) {
                if (!pack) { plog("assets: 没有托管资源包（%s），改从下载站直接下", e.localizedDescription.UTF8String); dispatch_async(dispatch_get_main_queue(), ^{ [self direct:stray]; }); return; }
                [self say:T("dmDlDownloading")];
                plog("assets: 托管资源包 版本 %ld、%.2f GB，请系统确保本地可用（可用空间 %.2f GB）", (long)pack.version, pack.downloadSize / 1e9, free_bytes() / 1e9);
                dispatch_async(dispatch_get_main_queue(), ^{ self.packBytes = pack.downloadSize; [self watchPack]; });
                [m ensureLocalAvailabilityOfAssetPack:pack completionHandler:^(NSError *e2) {
                    if (e2) { self.packStop = 1; [self fail:@"下载游戏资源" error:e2]; [self offerDirect:3]; return; }
                    plog("assets: 系统「确保本地可用」完成回调到了");
                    dispatch_async(dispatch_get_main_queue(), ^{
                        if (@available(iOS 26.4, *)) [self ready];
                        else [self legacyPackReady];
                    });
                }];
            }];
        };
        if (@available(iOS 26.4, *)) download();
        else {
            [self say:T("dmDlConnecting")];
            [self queryLegacyPack:^(BOOL available) { if (available) [self ready]; else download(); }];
        }
    } else [self direct:stray];
}


- (void)direct:(NSString *)fallback {
    if (!res_dl_configured()) {
        plog("assets: 本包没配资源下载地址，不走下载"); [self say:fallback ? [NSString stringWithFormat:@"%@\n%@", fallback, T("dmDlCopyInShort")] : no_assets_text()]; self.bar.hidden = YES; [self showBtn:self.retry]; self.stopped = 1; return; }
    [self say:T("dmDlConnecting")];
    [DmAssetDL.shared prepare:^(NSString *err, int64_t remain, int64_t total, BOOL cellular) {
        if (err && DmAssetDL.shared.shortBytes > 0) {
            plog("assets: 直接下载走不了：%s", err.UTF8String);
            NSString *dev = UIDevice.currentDevice.userInterfaceIdiom == UIUserInterfaceIdiomPad ? @"iPad" : @"iPhone";
            NSString *m = [NSString stringWithFormat:T("dmDlSpaceMsg"), DmAssetDL.shared.shortBytes / 1e9, remain / 1e9, dev, DmAssetDL.shared.freeBytes / 1e9, dev];
            NSString *title = [NSString stringWithFormat:T("dmDlSpaceTitle"), dev];
            [self say:[[title stringByAppendingString:@"\n"] stringByAppendingString:m]]; [self showBtn:self.retry];
            UIAlertController *a = [UIAlertController alertControllerWithTitle:title message:m preferredStyle:UIAlertControllerStyleAlert];
            [a addAction:[UIAlertAction actionWithTitle:T("dmOk") style:UIAlertActionStyleDefault handler:nil]];
            [self presentViewController:a animated:YES completion:^{ plog("assets: 已弹系统提示「%s」｜正文：%s", title.UTF8String, [m stringByReplacingOccurrencesOfString:@"\n" withString:@" ⏎ "].UTF8String); }]; return; }



        { static int once; const char *ot = getenv("DM_ASSET_OFFLINE_TEST"); if (ot && !once) { once = 1; err = T("dmDlNoServer"); self.netDown = 1;
            dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(atof(ot) * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{ [self netUp]; }); } }
        if (err) {
            plog("assets: 直接下载走不了：%s", err.UTF8String);
            [self say:[NSString stringWithFormat:T("dmDlNotStarted"), fallback ? [fallback stringByAppendingString:@"\n"] : @"", self.netDown ? T("dmDlNetDown") : err, self.netDown ? T("dmDlNextAuto") : T("dmDlNextRetry")]];
            [self showBtn:self.retry]; self.stopped = 1; return; }
        void (^go)(void) = ^{
            [self say:[NSString stringWithFormat:T("dmDlWill"), remain < 100000000 ? [NSString stringWithFormat:@"%.0f MB", remain / 1e6] : [NSString stringWithFormat:@"%.1f GB", remain / 1e9], total / 1e9]];
            self.last = total - remain; self.lastT = CFAbsoluteTimeGetCurrent(); self.rate = 0;
            [DmAssetDL.shared run:^(int64_t done, int64_t tot, NSString *note) {
                CFAbsoluteTime now = CFAbsoluteTimeGetCurrent();
                if (now - self.lastT >= 1.0) { self.rate = MAX(0, done - self.last) / (now - self.lastT); self.last = done; self.lastT = now; }
                self.label.text = [NSString stringWithFormat:@"正在下载游戏资源（共 %.1f GB）\n%.2f / %.2f GB（%.0f%%）  %.1f MB/s%@", tot / 1e9, done / 1e9, tot / 1e9, tot ? 100.0 * done / tot : 0, self.rate / 1e6, note ? [@"\n" stringByAppendingString:note] : @""];
                self.bar.progress = tot ? (float)done / tot : 0;
            } done:^(NSString *e2) {
                if (e2) { [self say:[NSString stringWithFormat:T("dmDlStopped"), e2, self.last / 1e9, total / 1e9]]; [self showBtn:self.retry]; self.stopped = 1; return; }
                [self ready];
            }];
        };
        if (!cellular || remain <= 0) { go(); return; }
        UIAlertController *a = [UIAlertController alertControllerWithTitle:T("dmDlCellTitle") message:[NSString stringWithFormat:T("dmDlCellMsg"), remain / 1e9] preferredStyle:UIAlertControllerStyleAlert];
        [a addAction:[UIAlertAction actionWithTitle:T("dmDlCellLater") style:UIAlertActionStyleCancel handler:^(UIAlertAction *x) { [self say:T("dmDlCellWait")]; [self showBtn:self.retry]; }]];
        [a addAction:[UIAlertAction actionWithTitle:T("dmDlCellGo") style:UIAlertActionStyleDefault handler:^(UIAlertAction *x) { plog("assets: 玩家同意用流量下载"); go(); }]];
        [self presentViewController:a animated:YES completion:nil];
    }];
}
- (void)downloadOfAssetPack:(BAAssetPack *)p hasProgress:(NSProgress *)g API_AVAILABLE(ios(26.0)) {
    CFAbsoluteTime now = CFAbsoluteTimeGetCurrent(); int64_t done = g.completedUnitCount, total = g.totalUnitCount;
    if (self.gone) return;
    self.packDone = done; if (total > 0) self.packBytes = total;
    if (total > 0 && done >= total) {
        if (!self.packFull) { self.packFull = now; plog("assets: 托管下载进度到 100%%（%.2f GB），等系统装进本地；可用空间 %.2f GB", total / 1e9, free_bytes() / 1e9);
            dispatch_async(dispatch_get_main_queue(), ^{ self.bar.progress = 1; [self packText:free_bytes()]; [self showBtn:self.share]; }); }
        return; }
    if (now - self.lastT >= 1.0) { self.rate = (done - self.last) / (now - self.lastT); self.last = done; self.lastT = now; }
    NSString *t = [NSString stringWithFormat:T("dmDlProgress"), done / 1e9, total / 1e9, total ? 100.0 * done / total : 0, self.rate / 1e6];
    dispatch_async(dispatch_get_main_queue(), ^{ self.label.text = t; self.bar.progress = total ? (float)done / total : 0; });
}
- (void)downloadOfAssetPackBegan:(BAAssetPack *)p API_AVAILABLE(ios(26.0)) { plog("assets: 系统通知托管下载开始 / 继续"); self.packStop = 0; self.textKind = 0; }
- (void)downloadOfAssetPackFinished:(BAAssetPack *)p API_AVAILABLE(ios(26.0)) {
    plog("assets: 系统通知资源包下载完成%s", self.gone ? "（已改走直接下载，不接）" : ""); if (self.gone) return; self.finished = 1;
    dispatch_async(dispatch_get_main_queue(), ^{
        if (@available(iOS 26.4, *)) [self ready];
        else if (!p || [p.identifier isEqualToString:@"romfs"]) [self legacyPackReady];
    }); }
- (void)downloadOfAssetPackPaused:(BAAssetPack *)p API_AVAILABLE(ios(26.0)) {
    plog("assets: 系统通知托管下载暂停%s", self.gone ? "（已改走直接下载）" : ""); if (self.gone) return; self.packStop = 1; [self say:T("dmDlPaused")]; [self offerDirect:2]; }
- (void)downloadOfAssetPack:(BAAssetPack *)p failedWithError:(NSError *)e API_AVAILABLE(ios(26.0)) {
    if (self.gone) { plog("assets: 托管下载失败（已改走直接下载）：%s", e.localizedDescription.UTF8String); return; }
    self.packStop = 1; [self fail:@"下载游戏资源" error:e]; [self offerDirect:3]; }
- (void)ready {
    char why[2048];
    static int started; if (started) return;
    if (!romfs_locate(why, sizeof why)) {
        if (g_pack_wait) { if (!self.packFull) [self say:T("dmDlPackWait")]; return; }
        if (self.poll) { plog("assets: 系统说可用但校验没过：%s", why); }
        [self say:[NSString stringWithFormat:T("dmDlNotReady"), @(why)]]; [self showBtn:self.retry]; return; }
    started = 1; if (self.poll) { dispatch_source_cancel(self.poll); self.poll = nil; }
    plog("assets: 资源就绪，就地起宿主");
    g_need_assets = 0;
    self.view = front_ios_view(self.view.window.windowScene.coordinateSpace.bounds);
    [self setNeedsUpdateOfScreenEdgesDeferringSystemGestures]; [self setNeedsUpdateOfHomeIndicatorAutoHidden];
    host_start();
    front_ios_focus_event(3);
}
@end
static void astc_handoff(UIWindow *w) {
    g_no_bc = 0; g_need_assets = 1; AssetVC *vc = [AssetVC new]; w.rootViewController = vc;
}








@interface DmGameVC : UIViewController
@end
@implementation DmGameVC
- (UIRectEdge)preferredScreenEdgesDeferringSystemGestures { return UIRectEdgeAll; }
- (BOOL)prefersHomeIndicatorAutoHidden { return YES; }
- (BOOL)prefersStatusBarHidden { return YES; }
- (void)viewSafeAreaInsetsDidChange {
    [super viewSafeAreaInsetsDidChange]; UIEdgeInsets i = self.view.safeAreaInsets; CGSize z = self.view.bounds.size;
    plog("安全区 上 %.1f 左 %.1f 下 %.1f 右 %.1f（点）；视图 %.0fx%.0f；朝向 %ld", i.top, i.left, i.bottom, i.right, z.width, z.height, (long)self.view.window.windowScene.interfaceOrientation);
}
@end
@interface SceneDelegate : UIResponder <UIWindowSceneDelegate>
@property (strong, nonatomic) UIWindow *window;
@end
@implementation SceneDelegate
- (void)scene:(UIScene *)scene willConnectToSession:(UISceneSession *)s options:(UISceneConnectionOptions *)o {
    UIWindowScene *ws = (UIWindowScene *)scene;
    self.window = [[UIWindow alloc] initWithWindowScene:ws]; g_window = self.window;
    UIViewController *vc = [DmGameVC new];
    vc.view.backgroundColor = UIColor.blackColor;
    if ([g_mode isEqualToString:@"boot"] && g_block_title) { vc = [BlockVC new]; vc.view.backgroundColor = UIColor.blackColor; }
    else if ([g_mode isEqualToString:@"boot"] && g_no_bc) { vc = [NoBcVC new]; vc.view.backgroundColor = UIColor.blackColor; }
    else if ([g_mode isEqualToString:@"boot"] && g_need_assets) vc = [AssetVC new];
    else if ([g_mode isEqualToString:@"boot"]) vc.view = front_ios_view(ws.coordinateSpace.bounds);
    else if ([g_mode isEqualToString:@"blank"]) { UIView *front_ios_blank_view(CGRect); vc.view = front_ios_blank_view(ws.coordinateSpace.bounds); }
    else {
        UILabel *l = [[UILabel alloc] initWithFrame:vc.view.bounds];
        l.text = [NSString stringWithFormat:@"dm iOS · %@", g_mode]; l.textAlignment = NSTextAlignmentCenter;
        l.textColor = UIColor.whiteColor; [vc.view addSubview:l];
    }
    self.window.rootViewController = vc; [self.window makeKeyAndVisible];
    g_T[4] = since_start();
    plog("启动计时 T4 willConnect 末 %.1f ms", g_T[4]);
}
- (void)sceneWillResignActive:(UIScene *)s { if (g_host_up) front_ios_focus_event(0); }
- (void)sceneDidEnterBackground:(UIScene *)s { if (g_host_up) front_ios_focus_event(1); }
- (void)sceneWillEnterForeground:(UIScene *)s { log_promote(); if ([g_mode isEqualToString:@"boot"] && !g_need_assets && !g_no_bc && !g_block_title) { host_start(); if (g_host_up) front_ios_focus_event(2); } }
- (void)sceneDidBecomeActive:(UIScene *)s { if (g_host_up) front_ios_focus_event(3); }


- (void)sceneDidDisconnect:(UIScene *)s { if (g_host_up) dm_host_shutdown("iOS 场景断开"); }
@end






@interface BgTest : NSObject <NSURLSessionDownloadDelegate> @end
@implementation BgTest
- (void)URLSession:(NSURLSession *)s downloadTask:(NSURLSessionDownloadTask *)t didFinishDownloadingToURL:(NSURL *)u { plog("后台测试会话：下载到手（丢弃）"); }
- (void)URLSession:(NSURLSession *)s task:(NSURLSessionTask *)t didCompleteWithError:(NSError *)e { plog("后台测试会话：任务结束%s", e ? e.localizedDescription.UTF8String : ""); }
@end
static NSURLSession *bg_test_session(void) {
    static NSURLSession *s; static dispatch_once_t once;
    dispatch_once(&once, ^{ NSURLSessionConfiguration *c = [NSURLSessionConfiguration backgroundSessionConfigurationWithIdentifier:[NSBundle.mainBundle.bundleIdentifier stringByAppendingString:@".bgtest"]];
        c.sessionSendsLaunchEvents = YES; c.discretionary = NO; s = [NSURLSession sessionWithConfiguration:c delegate:[BgTest new] delegateQueue:nil]; });
    return s;
}
static void bg_relaunch_test(void) {
    const char *u = getenv("DM_TEST_BG_RELAUNCH"); NSURL *url = u ? [NSURL URLWithString:@(u)] : nil; if (!url) return;
    plog("后台测试会话：6 秒后才许开始取 %s，2 秒后本进程退出，等系统在后台拉起", u);
    NSURLSessionDownloadTask *t = [bg_test_session() downloadTaskWithURL:url]; t.earliestBeginDate = [NSDate dateWithTimeIntervalSinceNow:6]; [t resume];
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 2 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{ exit(0); });
}

@implementation App
- (void)applicationWillTerminate:(UIApplication *)a { if (g_host_up) dm_host_shutdown("iOS App 终止"); }




- (void)applicationDidReceiveMemoryWarning:(UIApplication *)a {
    static int n; if (++n > 50) return;
    task_vm_info_data_t vi; mach_msg_type_number_t cnt = TASK_VM_INFO_COUNT; unsigned long long fp = 0;
    if (task_info(mach_task_self(), TASK_VM_INFO, (task_info_t)&vi, &cnt) == KERN_SUCCESS) fp = vi.phys_footprint >> 20;
    unsigned long long h[3] = {0, 0, 0}, gpu = 0; void dm_guest_heap_last(unsigned long long *); unsigned long long mtl_gpu_allocated_mb(void);
    if (g_host_up) { dm_guest_heap_last(h); gpu = mtl_gpu_allocated_mb(); }
    plog("【内存告警】系统发来内存告警（本局第 %d 次，进程已活 %.0f 秒，applicationState=%ld）：常驻 %llu MB，显卡已分配 %llu MB，游戏堆 %llu MB 余量 %llu MB 最大可分配块 %llu KB，离系统内存上限还有 %llu MB",
         n, since_start() / 1000.0, (long)a.applicationState, fp, gpu, h[0], h[1], h[2], (unsigned long long)(os_proc_available_memory() >> 20));
}
- (void)application:(UIApplication *)a handleEventsForBackgroundURLSession:(NSString *)ident completionHandler:(void (^)(void))done {
    plog("boot: 系统交来后台下载会话 %s 的事件（applicationState=%ld，宿主%s）", ident.UTF8String, (long)a.applicationState, g_host_up ? "在跑" : "没起");
    if ([ident hasSuffix:@".bgtest"]) { plog("后台测试会话：接回"); (void)bg_test_session(); done(); return; }
    dm_asset_dl_bg_events(ident, done);
}
- (BOOL)application:(UIApplication *)a didFinishLaunchingWithOptions:(NSDictionary *)o {
    g_T[3] = since_start();
    if ([g_mode isEqualToString:@"blank"]) {
        UIApplication.sharedApplication.idleTimerDisabled = YES;
        uint64_t front_blank_presents(void); void front_ios_heat_start(uint64_t (*)(void), int); front_ios_heat_start(front_blank_presents, 0);
    }
    const char *us = getenv("DM_TEST_UISHOT");
    if (us && atof(us) > 0 && strchr(us, ':')) { NSString *fn = @(strchr(us, ':') + 1); dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(atof(us) * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{




        NSMutableArray<UIWindow *> *ws = [NSMutableArray array]; for (UIScene *sc in UIApplication.sharedApplication.connectedScenes) if ([sc isKindOfClass:UIWindowScene.class]) [ws addObjectsFromArray:((UIWindowScene *)sc).windows];
        [ws sortUsingComparator:^NSComparisonResult(UIWindow *x, UIWindow *y) { return x.windowLevel < y.windowLevel ? NSOrderedAscending : x.windowLevel > y.windowLevel ? NSOrderedDescending : NSOrderedSame; }];
        if (!ws.count) { plog("【红】DM_TEST_UISHOT：没有窗口"); return; }
        CGRect all = CGRectZero; for (UIWindow *x in ws) all = CGRectUnion(all, x.frame);
        UIImage *im = [[[UIGraphicsImageRenderer alloc] initWithBounds:all] imageWithActions:^(UIGraphicsImageRendererContext *c) { for (UIWindow *x in ws) if (!x.hidden) [x drawViewHierarchyInRect:x.frame afterScreenUpdates:YES]; }];
        for (UIWindow *x in ws) { UIViewController *top = x.rootViewController; while (top.presentedViewController) top = top.presentedViewController;
            plog("DM_TEST_UISHOT：窗口 %s 层级 %.0f %.0fx%.0f%s｜最上层 %s", NSStringFromClass(x.class).UTF8String, x.windowLevel, x.frame.size.width, x.frame.size.height, x.hidden ? "（隐藏）" : "", top ? NSStringFromClass(top.class).UTF8String : "无");
            if (top && top != x.rootViewController) { NSMutableArray<UIView *> *q = [NSMutableArray arrayWithObject:top.view];
                while (q.count) { UIView *v = q.firstObject; [q removeObjectAtIndex:0]; [q addObjectsFromArray:v.subviews];
                    if ([v isKindOfClass:UILabel.class] && ((UILabel *)v).text.length) { UILabel *l = (UILabel *)v; CGFloat need = [l sizeThatFits:CGSizeMake(l.bounds.size.width, CGFLOAT_MAX)].height; BOOL sc = NO; for (UIView *u = l.superview; u; u = u.superview) if ([u isKindOfClass:UIScrollView.class]) sc = YES;
                        plog("DM_TEST_UISHOT：标签 %.0fx%.0f 排完要高 %.0f 行数上限 %ld%s %s｜%.24s", l.bounds.size.width, l.bounds.size.height, need, (long)l.numberOfLines, sc ? " 在滚动区里" : "", need <= l.bounds.size.height + 0.5 ? "不截断" : "【红】高度不够", l.text.UTF8String); } } } }
        NSString *path = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES)[0] stringByAppendingPathComponent:[fn.lastPathComponent stringByAppendingString:@".png"]];
        plog("DM_TEST_UISHOT：%s %s", path.fileSystemRepresentation, [UIImagePNGRepresentation(im) writeToFile:path atomically:YES] ? "已存" : "【红】没存下");
    }); }
    plog("dm iOS 模式 %s · %s %s", g_mode.UTF8String, UIDevice.currentDevice.systemName.UTF8String, UIDevice.currentDevice.systemVersion.UTF8String);
    plog("启动计时 T1 静态构造 %.1f ms · T2 main %.1f ms · T3 didFinishLaunching %.1f ms（零点 = 进程创建）", g_T[1], g_T[2], g_T[3]);
    if ([g_mode isEqualToString:@"boot"]) { plog("boot: 启动完成，宿主等场景到前台再起（applicationState=%ld）", (long)a.applicationState); bg_relaunch_test(); return YES; }
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{ run_mode(); });
    return YES;
}
@end

int main(int argc, char *argv[]) {
    g_T[2] = since_start();






    { struct rlimit rl; if (!getrlimit(RLIMIT_NOFILE, &rl)) { rlim_t was = rl.rlim_cur, want = 4096; if (rl.rlim_max != RLIM_INFINITY && want > rl.rlim_max) want = rl.rlim_max;
        if (was < want) { rl.rlim_cur = want; int rc = setrlimit(RLIMIT_NOFILE, &rl); fprintf(stderr, "boot: 文件描述符软上限 %llu → %llu（%s）\n", (unsigned long long)was, (unsigned long long)want, rc ? "失败" : "已设"); g_nofile_was = (int)was; g_nofile_now = rc ? (int)was : (int)want; } else g_nofile_was = g_nofile_now = (int)was; } }
    g_mode = @"boot";
    for (int i = 1; i + 1 < argc; i++) if (!strcmp(argv[i], "-mode")) g_mode = @(argv[i + 1]);


    { const char *pa = getenv("S2A_PACK_APPLY_ONLY"); if (pa && !strcmp(pa, "1")) g_mode = @"s2a-pack"; }




    if ([g_mode isEqualToString:@"thermal"]) {
        int th = (int)NSProcessInfo.processInfo.thermalState;
        NSString *d0 = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES)[0];
        NSDateFormatter *df = [NSDateFormatter new]; df.dateFormat = @"yyyy-MM-dd HH:mm:ss ZZZ";
        NSString *line = [NSString stringWithFormat:@"温控档位 %d（轻探）时间 %@\n", th, [df stringFromDate:NSDate.date]];
        [line writeToFile:[d0 stringByAppendingPathComponent:@"thermal.txt"] atomically:YES encoding:NSUTF8StringEncoding error:nil];
        fputs(line.UTF8String, stdout); fflush(stdout);
        return 0;
    }
    NSString *doc = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES)[0];
    const char *log_file = strdup([doc stringByAppendingPathComponent:[g_mode stringByAppendingString:@".txt"]].fileSystemRepresentation);






    if ([g_mode isEqualToString:@"boot"]) log_file = strdup([doc stringByAppendingPathComponent:@"boot.launch.txt"].fileSystemRepresentation);
    g_log = open(log_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if ([g_mode isEqualToString:@"boot"]) {


        { char hw[64] = "未知"; size_t hn = sizeof hw; sysctlbyname("hw.machine", hw, &hn, NULL, 0);
          NSDictionary *inf = NSBundle.mainBundle.infoDictionary; NSDateFormatter *df = [NSDateFormatter new]; df.dateFormat = @"yyyy-MM-dd HH:mm:ss ZZZ";
          plog("boot: 版本 %s 构建 %s 机型 %s 系统 %s 时间 %s", [inf[@"CFBundleShortVersionString"] description].UTF8String ?: "未知", [inf[@"CFBundleVersion"] description].UTF8String ?: "未知",
               hw, NSProcessInfo.processInfo.operatingSystemVersionString.UTF8String, [df stringFromDate:NSDate.date].UTF8String);
        }


        setenv("DM_XV_FILE", [doc stringByAppendingPathComponent:@"xv_violation.txt"].fileSystemRepresentation, 0);
        const char *log_port = getenv("DM_LOG_PORT");
        strlcpy(g_log_path, log_file, sizeof g_log_path);
        if ((getenv("DM_LOG_TEE") || log_port) && !pipe(g_log_pipe) && (!getenv("DM_LOG_TEE") || (g_log_stderr = dup(2)) >= 0) &&
            !pthread_create(&g_log_thread, NULL, log_tee_main, NULL)) {
            g_log_tee = 1;
            dup2(g_log_pipe[1], 2);
            close(g_log_pipe[1]);
            g_log_pipe[1] = -1;
            pthread_t lp; if (log_port && !pthread_create(&lp, NULL, log_port_main, (void *)(intptr_t)atoi(log_port))) pthread_detach(lp);
        } else {
            dup2(g_log, 2);
        }
        metal_cache_report_and_clean();
        char why[2048];
        { id<MTLDevice> dev = MTLCreateSystemDefaultDevice(); g_no_bc = !dev.supportsBCTextureCompression || getenv("DM_TEST_NO_BC");




          if (![dev supportsFamily:MTLGPUFamilyMetal3] || getenv("DM_TEST_NO_METAL3")) {
              g_block_title = T("dmGpuOldTitle"); g_block_msg = T("dmGpuOldMsg"); g_block_why = @"图形芯片不支持 Metal 3";
              plog("boot: 🔴 显卡 %s 不支持 Metal 3%s——不起游戏，弹框说明", dev.name.UTF8String, getenv("DM_TEST_NO_METAL3") ? "（测试开关 DM_TEST_NO_METAL3）" : ""); }
          plog("boot: 显卡 %s，BC 压缩格式%s", dev.name.UTF8String, g_no_bc ? (getenv("DM_TEST_NO_BC") ? "按测试开关 DM_TEST_NO_BC 当成不支持" : "不支持——不起游戏，弹框说明后退出") : "支持"); }
        if (g_no_bc) {
            NSString *d = astc_pack_locate(g_astc_why, sizeof g_astc_why);
            if (d) { g_astc_dir = d; g_no_bc = 0; plog("boot: 用老机型补充包 %s", d.UTF8String); }
            else plog("boot: 老机型补充包未就绪：%s", g_astc_why);
        }
        NSString *pb = g_block_title ? nil : pack_check();
        if (pb) { g_block_title = T("dmPackBadTitle"); g_block_msg = [NSString stringWithFormat:@"%@\n\n%@", pb, T("dmPackBadHow")]; g_block_why = @"安装包自检没过"; }
        if (g_block_title) {}
        else if (g_no_bc) {}
        else if (getenv("DM_ASSETS_TEST") || getenv("DM_TEST_BA")) { g_need_assets = 1; plog("boot: DM_ASSETS_TEST——强制走资源下载界面再就地起宿主（测这条路径）"); }
        else if (s2a_pack_pending()) { g_need_assets = 1; plog("boot: 有 S2Arm 推来没解开的资源包 %d 批，先在资源界面解开", s2a_pack_pending()); }
        else if (romfs_locate(why, sizeof why)) plog("boot: 游戏资源就绪，宿主等场景到前台再装载");
        else { g_need_assets = 1; plog("boot: 游戏资源还没到（%s），先显示资源下载界面", why); }
    } else metal_cache_report_and_clean();
    @autoreleasepool { return UIApplicationMain(argc, argv, nil, NSStringFromClass(App.class)); }
}
