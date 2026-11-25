







#import <UIKit/UIKit.h>
#import <Metal/Metal.h>
#import <GameController/GameController.h>
#import <QuartzCore/CAMetalLayer.h>
#import <CommonCrypto/CommonDigest.h>
#include <pthread.h>
#include <signal.h>
#include "front.h"
#include "romfs_validate.h"
#include "dm_i18n.h"
static CAMetalLayer *layer;
static BOOL started;
static __weak UIWindow *tvWindow;
static NSString *SWT(const char *k) { return @(dm_ui(k)); }
void dm_gamepad_start(void (*callback)(int));
void dm_gamepad_reset(void);
void dm_focus_set(int);
void dm_audio_suspend(int);
void dm_gfx_suspend(int);
void guest_clock_freeze_for(int, int);
void dm_padmode_request(int);
int dm_main(int, char **);
int dm_front_navigation(void) { return 1; }
int dm_thermal_now(void) { return (int)NSProcessInfo.processInfo.thermalState; }
void *front_metal_layer(void) { return (__bridge void *)layer; }
uint64_t front_game_res(int docked) { return ((uint64_t)(docked ? 900 : 720) << 32) | (docked ? 1600 : 1280); }
void front_run(void (*guest)(void)) { sigset_t all; sigfillset(&all); pthread_sigmask(SIG_UNBLOCK, &all, NULL); guest(); }
@interface TVView : UIView @end
@implementation TVView
+ (Class)layerClass { return CAMetalLayer.class; }
- (BOOL)canBecomeFirstResponder { return YES; }
- (void)layoutSubviews { [super layoutSubviews]; layer.drawableSize = CGSizeMake(1920, 1080); }
@end
@interface SceneDelegate : UIResponder <UIWindowSceneDelegate>
@property(strong, nonatomic) UIWindow *window;
@end
static UIViewController *top_vc(void) { UIViewController *vc = tvWindow.rootViewController; while (vc.presentedViewController) vc = vc.presentedViewController; return vc; }


int front_text_input(const char *title, const char *sub, const char *guide, const char *initial, int max_chars, char *out, int cap) {
    (void)max_chars;
    if (NSThread.isMainThread || !out || cap < 1) return -1;
    dispatch_semaphore_t done = dispatch_semaphore_create(0); __block int accepted = 0;
    dispatch_async(dispatch_get_main_queue(), ^{
        GCEventViewController *root = (GCEventViewController *)tvWindow.rootViewController;
        root.controllerUserInteractionEnabled = YES; dm_gamepad_reset();
        void (^finish)(void) = ^{ root.controllerUserInteractionEnabled = NO; [root.view becomeFirstResponder]; dm_gamepad_reset(); dispatch_semaphore_signal(done); };
        UIAlertController *a = [UIAlertController alertControllerWithTitle:title && *title ? @(title) : SWT("dmInputTitle") message:@(sub ?: "") preferredStyle:UIAlertControllerStyleAlert];
        [a addTextFieldWithConfigurationHandler:^(UITextField *f) { f.text = @(initial ?: ""); f.placeholder = @(guide ?: ""); }];
        [a addAction:[UIAlertAction actionWithTitle:SWT("dmCancel") style:UIAlertActionStyleCancel handler:^(UIAlertAction *x) { (void)x; finish(); }]];
        [a addAction:[UIAlertAction actionWithTitle:SWT("dmConfirm") style:UIAlertActionStyleDefault handler:^(UIAlertAction *x) { (void)x; snprintf(out, cap, "%s", a.textFields.firstObject.text.UTF8String); accepted = 1; finish(); }]];
        [top_vc() presentViewController:a animated:YES completion:nil];
    });
    dispatch_semaphore_wait(done, DISPATCH_TIME_FOREVER); return accepted;
}


void front_save_failed(int err) {
    NSLog(@"tvOS：存档写失败 errno=%d", err);
    dispatch_async(dispatch_get_main_queue(), ^{
        UIAlertController *a = [UIAlertController alertControllerWithTitle:SWT("dmSaveNoSpace") message:nil preferredStyle:UIAlertControllerStyleAlert];
        [a addAction:[UIAlertAction actionWithTitle:SWT("dmConfirm") style:UIAlertActionStyleDefault handler:nil]];
        [top_vc() presentViewController:a animated:YES completion:nil];
    });
}
static void pathenv(const char *key, NSString *value) { setenv(key, value.fileSystemRepresentation, 0); }
static NSDictionary *read_json(NSString *path) {
    NSData *data = [NSData dataWithContentsOfFile:path];
    id v = data ? [NSJSONSerialization JSONObjectWithData:data options:0 error:NULL] : nil;
    return [v isKindOfClass:NSDictionary.class] ? v : nil;
}


static BOOL checked_file(NSString *path, NSDictionary *info, NSMutableDictionary *stamps) {
    NSDictionary *attr = [NSFileManager.defaultManager attributesOfItemAtPath:path error:NULL];
    if (!attr || ![info[@"sha256"] isKindOfClass:NSString.class] || attr.fileSize != [info[@"size"] unsignedLongLongValue]) return NO;
    NSString *stamp = [NSString stringWithFormat:@"%@|%llu|%.6f|%@", path, attr.fileSize, attr.fileModificationDate.timeIntervalSince1970, info[@"sha256"]];
    if ([stamps[path] isEqualToString:stamp]) return YES;
    FILE *f = fopen(path.fileSystemRepresentation, "rb"); if (!f) return NO;
    CC_SHA256_CTX c; CC_SHA256_Init(&c); unsigned char buf[65536], d[CC_SHA256_DIGEST_LENGTH]; size_t n;
    while ((n = fread(buf, 1, sizeof buf, f)) > 0) CC_SHA256_Update(&c, buf, (CC_LONG)n);
    BOOL ok = !ferror(f); fclose(f); CC_SHA256_Final(d, &c);
    NSMutableString *hex = [NSMutableString new]; for (int i = 0; i < CC_SHA256_DIGEST_LENGTH; i++) [hex appendFormat:@"%02x", d[i]];
    ok = ok && [hex isEqualToString:info[@"sha256"]];
    if (ok) stamps[path] = stamp;
    return ok;
}


static NSString *validate_resources(NSString *base, BOOL needs_astc) {
    char why[1024] = {0};
    if (!dm_romfs_validate([base stringByAppendingPathComponent:@"RomFS/data"].fileSystemRepresentation, why, sizeof why))
        return [NSString stringWithFormat:SWT("dmTvNoRomfs"), why];
    if (!needs_astc) return nil;
    NSString *stamp_path = [base stringByAppendingPathComponent:@"validation-stamps.plist"];
    NSMutableDictionary *stamps = [[NSDictionary dictionaryWithContentsOfFile:stamp_path] mutableCopy] ?: [NSMutableDictionary new];
    NSString *overlay = [base stringByAppendingPathComponent:@"romfs_overlay_astc"];
    NSDictionary *pack = read_json([overlay stringByAppendingPathComponent:@"pack.json"]);
    NSString *name = pack[@"name"];
    if (![name isKindOfClass:NSString.class] || ![name isEqualToString:name.lastPathComponent]
        || !checked_file([[base stringByAppendingPathComponent:@"astc/data"] stringByAppendingPathComponent:name], pack, stamps)) return SWT("dmOldPackNone");
    if (![NSFileManager.defaultManager fileExistsAtPath:[overlay stringByAppendingPathComponent:@"data/.build.config"]]) return SWT("dmTvNoAstcIndex");
    [stamps writeToFile:stamp_path atomically:YES];
    return nil;
}
static void *boot(void *unused) {
    (void)unused;
    @autoreleasepool {
        NSString *res = [NSBundle.mainBundle.resourcePath stringByAppendingPathComponent:@"dm"];
        char *argv[] = {"dmhost", (char *)res.fileSystemRepresentation, "main", "sdk", NULL};
        dm_padmode_request(1);
        NSLog(@"tvOS：dm_main 起跑"); int rc = dm_main(4, argv); NSLog(@"tvOS：dm_main 返回 %d", rc);
    }
    return NULL;
}
@implementation SceneDelegate
- (void)scene:(UIScene *)scene willConnectToSession:(UISceneSession *)session options:(UISceneConnectionOptions *)options {
    (void)session; (void)options;
    self.window = [[UIWindow alloc] initWithWindowScene:(UIWindowScene *)scene]; tvWindow = self.window;
    GCEventViewController *vc = [GCEventViewController new]; vc.controllerUserInteractionEnabled = NO;
    TVView *view = [[TVView alloc] initWithFrame:self.window.bounds]; vc.view = view;
    layer = (CAMetalLayer *)view.layer; layer.device = MTLCreateSystemDefaultDevice(); layer.pixelFormat = MTLPixelFormatBGRA8Unorm; layer.framebufferOnly = YES;
    self.window.rootViewController = vc; [self.window makeKeyAndVisible]; [view becomeFirstResponder]; dm_gamepad_start(NULL);
    NSString *base = [NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES).firstObject stringByAppendingPathComponent:@"D2R"];
    NSString *bundle = NSBundle.mainBundle.resourcePath;
    NSFileManager *fm = NSFileManager.defaultManager;
    for (NSString *p in @[@"save", @"temp", @"RomFS"]) [fm createDirectoryAtPath:[base stringByAppendingPathComponent:p] withIntermediateDirectories:YES attributes:nil error:NULL];
    pathenv("DM_ROMFS", [base stringByAppendingPathComponent:@"RomFS"]);
    pathenv("DM_SAVE", [base stringByAppendingPathComponent:@"save"]);
    pathenv("DM_TEMP", [base stringByAppendingPathComponent:@"temp"]);
    pathenv("DM_MSL_DIR", [bundle stringByAppendingPathComponent:@"msl"]);
    pathenv("DM_NVN_SHMAP", [bundle stringByAppendingPathComponent:@"dm/shader_map.tsv"]);


    BOOL needs_astc = !layer.device.supportsBCTextureCompression;
    pathenv("DM_ROMFS_OVERLAY", needs_astc ? [NSString stringWithFormat:@"%@:%@", [base stringByAppendingPathComponent:@"romfs_overlay_astc"], [base stringByAppendingPathComponent:@"astc"]]
                                           : [bundle stringByAppendingPathComponent:@"romfs_overlay"]);
    { void dm_custom_init(const char *); dm_custom_init(base.fileSystemRepresentation); }
    chdir(base.fileSystemRepresentation);
    NSLog(@"tvOS：外壳就绪 显卡 %@ BC %d 资源目录 %@", layer.device.name, layer.device.supportsBCTextureCompression, base);
    NSError *err = nil;
    id<MTL4CommandQueue> probe = [layer.device newMTL4CommandQueueWithDescriptor:[MTL4CommandQueueDescriptor new] error:&err];
    UILabel *label = [[UILabel alloc] initWithFrame:CGRectInset(view.bounds, 100, 100)];
    label.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight; label.numberOfLines = 0;
    label.textColor = UIColor.whiteColor; label.textAlignment = NSTextAlignmentCenter; label.font = [UIFont systemFontOfSize:30];
    label.text = SWT("dmTvChecking"); [view addSubview:label];
    if (!probe) { label.text = SWT("dmTvGpuFail"); NSLog(@"tvOS：建 Metal 4 命令队列失败 %@", err); return; }
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
        NSString *problem = validate_resources(base, needs_astc);
        dispatch_async(dispatch_get_main_queue(), ^{
            if (problem) { label.text = problem; NSLog(@"tvOS：资源没齐，不起游戏：%@", problem); return; }
            [label removeFromSuperview];
            pthread_attr_t attr; pthread_attr_init(&attr); pthread_attr_setstacksize(&attr, 16 << 20);
            pthread_t th; int rc = pthread_create(&th, &attr, boot, NULL); pthread_attr_destroy(&attr);
            if (rc) { NSLog(@"tvOS：起游戏线程失败 %d", rc); return; }
            pthread_detach(th); started = YES;


            const char *shot = getenv("DM_TV_SHOT_SECONDS"); int sec = shot ? atoi(shot) : 0;
            if (sec > 0) dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)sec * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
                void mtl_grab_next_present(void (^)(CGImageRef)); uint64_t mtl_present_count(void);
                NSLog(@"tvOS：取帧 已上屏 %llu 次", (unsigned long long)mtl_present_count());
                mtl_grab_next_present(^(CGImageRef img) { @autoreleasepool {
                    NSData *png = img ? UIImagePNGRepresentation([UIImage imageWithCGImage:img]) : nil;
                    BOOL ok = [png writeToFile:[base stringByAppendingPathComponent:@"frame.png"] atomically:YES];
                    NSLog(@"tvOS：取帧 写入 %d %lu 字节", ok, (unsigned long)png.length);
                } });
            });
        });
    });
}
- (void)sceneWillResignActive:(UIScene *)scene { (void)scene; dm_gamepad_reset(); if (started) dm_focus_set(2); }
- (void)sceneDidEnterBackground:(UIScene *)scene { (void)scene; if (started) { dm_focus_set(3); dm_audio_suspend(1); guest_clock_freeze_for(1, 1); dm_gfx_suspend(1); } }
- (void)sceneWillEnterForeground:(UIScene *)scene { (void)scene; if (started) { dm_gfx_suspend(0); guest_clock_freeze_for(1, 0); dm_audio_suspend(0); } }
- (void)sceneDidBecomeActive:(UIScene *)scene {
    (void)scene;
    if (!self.window.rootViewController.presentedViewController) { ((GCEventViewController *)self.window.rootViewController).controllerUserInteractionEnabled = NO; [self.window.rootViewController.view becomeFirstResponder]; }
    if (started) dm_focus_set(1);
}
@end
@interface App : UIResponder <UIApplicationDelegate> @end
@implementation App @end
int main(int argc, char **argv) { @autoreleasepool { return UIApplicationMain(argc, argv, nil, NSStringFromClass(App.class)); } }
