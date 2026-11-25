

#include "dm.h"
#include "front.h"
#include <stdatomic.h>
#ifdef __APPLE__
#include <sys/sysctl.h>
#endif
#include <sys/time.h>
#include <unistd.h>
void probe_install(void); void ticks_install(void); void hid_keys_init(void); void tact_prefetch_start(const char *outdir); void tact_chk_init(void); void stubs_report(void); void crash_install(void); void guest_thread_init(void); uint32_t svc_thread_main_init(void);
typedef void (*fn0)(void); typedef void (*fn2)(uint64_t, uint64_t); typedef void (*fnp)(void *);
static void *req(const char *n){ void *p=guest_sym(n); if(!p) die("缺符号 %s", n); return p; }



static char *g_argv[64] = {"D2R"}; static int g_argc = 1;

void crash_sample_self(void);








static void save_throttle_patch(void) {
    const char *v = getenv("DM_SAVE_THROTTLE"); int sec = v ? atoi(v) : 3;
    module_t *m = NULL;
    for (int i = 0; i < g_nmod; i++) if (!strcmp(g_mod[i].name, "main")) m = &g_mod[i];
    if (!m) return;
    int32_t *p = (int32_t *)(m->base + 0x20cefa8);
    if (*p != 30) { LOG("【红】存档落盘节流：main+0x20cefa8 原值 %d ≠ 30（构建号不符？），不改", *p); return; }
    if (sec == 30) return;
    *p = sec < 1 ? 1 : sec;
    LOG("存档落盘节流：main+0x20cefa8 30 → %d 秒", *p);
}






static void game_seed_maybe(void) {
    const char *v = getenv("DM_GAME_SEED");
    if (!v) return;
    module_t *m = NULL;
    for (int i = 0; i < g_nmod; i++) if (!strcmp(g_mod[i].name, "main")) m = &g_mod[i];
    if (!m) die("DM_GAME_SEED：没有 main 模块");
    uint32_t *p = (uint32_t *)(m->base + 0x20a0074);
    if (*p != 0xffffffffu) die("DM_GAME_SEED：main+0x20a0074 原值 %#x ≠ 0xffffffff（构建号不符？）", *p);
    *p = (uint32_t)strtoull(v, NULL, 0);
    LOG("DM_GAME_SEED：main+0x20a0074 ← %#x", *p);
}

static void guest_main(void) {
    guest_thread_init();
    { void inflate_bench_maybe(void); inflate_bench_maybe(); }
    crash_sample_self();
    uint32_t hmain = svc_thread_main_init();




    ((fn0)req("_init_libc0"))();
    LOG(">>> nnosInitialize"); ((fn2)req("nnosInitialize"))(hmain, 0);
    ((fn0)req("_init_libc1"))();
    LOG(">>> nninitInitializeSdkModule"); ((fn0)req("nninitInitializeSdkModule"))();
    LOG(">>> nninitStartup"); ((fn0)req("nninitStartup"))();
    ((fn0)req("_init_libc2"))();
    dm_prof_mark("② SDK 初始化（nnosInitialize → nninitStartup）");
    run_module_inits();
    dm_prof_mark("③ 各模块 init_array");
    ((fnp)req("_ZN2nn2os11SetHostArgcEi"))((void *)(uint64_t)g_argc);
    ((fnp)req("_ZN2nn2os11SetHostArgvEPPc"))(g_argv);
    game_seed_maybe(); save_throttle_patch();
    { void dm_guest_spawn_selftest(void); dm_guest_spawn_selftest(); }
    { void dm_padmode_boot(void); dm_padmode_boot(); }
    LOG(">>> nnMain"); ((fn0)req("nnMain"))();
    LOG("nnMain returned");
}

struct dm_flags g_flag;
uint64_t g_t0_ns;
static void flags_init(void) {
    g_flag.thr = !!getenv("DM_THR"); g_flag.svc = !!getenv("DM_SVC"); g_flag.fs = !!getenv("DM_FS");
    g_flag.movie = !!getenv("DM_MOVIE"); g_flag.dbg = !!getenv("DM_DBG"); g_flag.imports = !!getenv("DM_IMPORTS");
    g_flag.bootprof = !!getenv("DM_BOOTPROF");
}



uint64_t g_proc_start_ns;
static uint64_t proc_start_mono_ns(void) {
#ifndef __APPLE__


    if (g_proc_start_ns) return g_proc_start_ns;


    uint64_t mono = now_ns(); char buf[1024] = ""; FILE *f = fopen("/proc/self/stat", "r");
    if (!f) return mono;
    size_t got = fread(buf, 1, sizeof buf - 1, f); fclose(f); buf[got] = 0;
    const char *p = strrchr(buf, ')'); unsigned long long st = 0;
    if (!p || sscanf(p + 2, "%*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %*u %*u %*d %*d %*d %*d %*d %*d %llu", &st) != 1) return mono;
    long hz = sysconf(_SC_CLK_TCK); struct timespec bt; clock_gettime(CLOCK_BOOTTIME, &bt);
    int64_t age = (int64_t)bt.tv_sec * 1000000000ll + bt.tv_nsec - (int64_t)(st * 1000000000ull / (unsigned long long)(hz > 0 ? hz : 100));
    return age > 0 ? mono - (uint64_t)age : mono;
#else
    struct kinfo_proc kp; size_t n = sizeof kp; int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_PID, getpid()};
    uint64_t mono = now_ns(); struct timeval tv; gettimeofday(&tv, NULL);
    if (sysctl(mib, 4, &kp, &n, NULL, 0) || !n) return mono;
    int64_t age = (int64_t)(tv.tv_sec - kp.kp_proc.p_starttime.tv_sec) * 1000000000ll + (tv.tv_usec - kp.kp_proc.p_starttime.tv_usec) * 1000ll;
    return age > 0 ? mono - (uint64_t)age : mono;
#endif
}





static pthread_key_t g_race_keys[512];
static unsigned g_race_n, g_race_want;
static void *keyrace_thread(void *unused);
static void host_preload(int argc, char **argv) {
    g_t0_ns = proc_start_mono_ns();
    flags_init();
    dm_prof_mark("⓪ 进程创建 → 宿主 main（dyld）");
    if (argc < 2) die("usage: dmhost <dir> [mod...] [-- 游戏参数...]");






    if (getenv("DM_FIXED_TIME")) setenv("TZ", "UTC", 1);
    crash_install();
    const char *race = getenv("DM_TEST_KEYRACE");
    if (race && *race) {
        g_race_want = (unsigned)strtoul(race, NULL, 10);
        if (g_race_want > 512) g_race_want = 512;
        pthread_t th;
        if (pthread_create(&th, NULL, keyrace_thread, NULL)) die("DM_TEST_KEYRACE 起线程失败");
        pthread_detach(th);
    }
    { void dm_term_install(void); dm_term_install(); }
    { void crash_set_dump_hook(void (*)(void)); void svc_thread_dump(void); crash_set_dump_hook(svc_thread_dump); }
    const char *names[MAX_MOD]; int n = 0;
    int i = 2;
    for (; i < argc && strcmp(argv[i], "--") && n < MAX_MOD; i++) names[n++] = argv[i];
    if (i < argc && !strcmp(argv[i], "--")) for (i++; i < argc && g_argc < 63; i++) g_argv[g_argc++] = argv[i];
    if (!n) { names[n++] = "main"; names[n++] = "sdk"; }
    load_modules(argv[1], names, n);
    uint64_t ta = now_ns();
    probe_install();
    ticks_install();
    { void target_rank_install(void); target_rank_install(); }
    hid_keys_init();
    uint64_t tb = now_ns();
    tact_chk_init();
    tact_prefetch_start(argv[1]);
    uint64_t tc = now_ns();
    stubs_report();
    if (g_flag.bootprof) LOG("① 细分（装载后）：探针 / 拍门 / 按键 %.1f ms、预取启动（读清单）%.1f ms、桩报告 %.1f ms", (tb - ta) / 1e6, (tc - tb) / 1e6, (now_ns() - tc) / 1e6);
    dm_prof_mark("① 宿主装载（读 NSO / 重定位 / 强制接管）");
    { void loader_hash_log(void); loader_hash_log(); }
}
static struct { int argc; char **argv; pthread_t th; _Atomic int started; } g_pre;
static void *preload_thread(void *a) { (void)a; host_preload(g_pre.argc, g_pre.argv); return NULL; }
static void *keyrace_thread(void *unused) {
    (void)unused;
    for (unsigned i = 0; i < g_race_want; i++) if (pthread_key_create(&g_race_keys[g_race_n++], NULL)) break;
    return NULL;
}






void dm_host_preload_start(int argc, char **argv) {
    if (atomic_exchange(&g_pre.started, 1)) return;
    g_pre.argc = argc; g_pre.argv = calloc((size_t)argc + 1, sizeof(char *));
    for (int i = 0; i < argc; i++) g_pre.argv[i] = strdup(argv[i]);
    pthread_attr_t a; pthread_attr_init(&a); pthread_attr_setstacksize(&a, 8 << 20);
    if (pthread_create(&g_pre.th, &a, preload_thread, NULL)) die("dm_host_preload_start：起线程失败");
}
#include <TargetConditionals.h>
int dm_app_bootstrap(int *argc, char ***argv);
int main(int argc, char **argv) {
#if TARGET_OS_OSX
    if (dm_app_bootstrap(&argc, &argv)) { void dm_cloud_boot(void); dm_cloud_boot(); }
#endif
    if (getenv("DM_PRELOAD")) dm_host_preload_start(argc, argv);
    if (atomic_load(&g_pre.started)) {
        pthread_join(g_pre.th, NULL);
        int same = argc == g_pre.argc; for (int i = 0; same && i < argc; i++) same = !strcmp(argv[i], g_pre.argv[i]);
        if (!same) die("【红】dm_host_preload_start 的参数与 main 不同");
        void crash_thread_init(void); crash_thread_init();
        LOG("宿主装载已在后台提前完成（dm_host_preload_start）");
    } else host_preload(argc, argv);
    front_run(guest_main);
    return 0;
}
