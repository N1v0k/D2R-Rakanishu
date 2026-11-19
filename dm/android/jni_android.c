













#include "../host/dm.h"
#include "../host/dm_effects.h"
#include "../host/dm_input.h"
#include "../host/dm_res_scale.h"
#include "../host/front.h"
#include "../host/hle.h"
#include <android/log.h>
#include <android/native_window_jni.h>
#include <dirent.h>
#include <errno.h>
#include <jni.h>
#include "vulkan_present.h"
#include "vk_driver.h"
#include "touch_android.h"
#include "lifecycle_dispatch.h"
#include "log_policy.h"
#include <sys/resource.h>
#include <stdatomic.h>
#include "overlay_validate.h"
#include "dm_overlay_contract.h"
#include "cpu_translation.h"
#include <sys/auxv.h>
#ifdef DM_PLAYER_BUILD
#include "player_guard.h"
#endif
#ifdef __aarch64__
#include <asm/hwcap.h>
#endif
#include <sys/stat.h>
#include <sys/system_properties.h>
#include <sys/utsname.h>
#include <unistd.h>



void dm_padmode_request(int pad);
void dm_padmode_request_glyph(int glyph);

void front_android_set_window(void *win, int w, int h);
int dm_custom_get(const char *key); void dm_custom_init(const char *dir);
void front_android_set_refresh(float hz);
int dm_main(int argc, char **argv);
void front_android_boot_failed(int code);

static char g_files[1024], g_ext[1024], g_src[1024], g_runtime[1100];
static char g_data_src[1100];
static FILE *g_logf;




static dm_lifecycle_dispatch g_lifecycle;
static pthread_once_t g_lifecycle_once = PTHREAD_ONCE_INIT;
static int g_lifecycle_error;




















void dm_audio_idle(int on);
void dm_hle_quiet(int on);
#define KA_IDLE_AFTER_NS 10000000000ull
static pthread_mutex_t g_ka_m = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_ka_cv;
static pthread_once_t g_ka_once = PTHREAD_ONCE_INIT;
static int g_ka_want_bg, g_ka_inhibit, g_ka_idle, g_ka_ok;
static unsigned g_ka_gen, g_ka_armed_gen;
static uint64_t g_ka_deadline;
static unsigned g_leave_done; static int g_leave_result;
static uint64_t ka_now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return (uint64_t)t.tv_sec * 1000000000ull + (uint64_t)t.tv_nsec; }

static int ka_unpark(void) { if (!g_ka_idle) return 0; g_ka_idle = 0; dm_hle_quiet(0); dm_audio_idle(0); return 1; }
static void ka_arm(void) { g_ka_armed_gen = g_ka_gen; g_ka_deadline = ka_now() + KA_IDLE_AFTER_NS; pthread_cond_signal(&g_ka_cv); }
static void *ka_thread(void *arg) {
    (void)arg;
    pthread_mutex_lock(&g_ka_m);
    for (;;) {
        if (!g_ka_deadline) { pthread_cond_wait(&g_ka_cv, &g_ka_m); continue; }
        uint64_t d = g_ka_deadline;
        if (ka_now() < d) { struct timespec ts = {(time_t)(d / 1000000000ull), (long)(d % 1000000000ull)}; pthread_cond_timedwait(&g_ka_cv, &g_ka_m, &ts); continue; }
        g_ka_deadline = 0;
        if (!(g_ka_want_bg && g_ka_gen == g_ka_armed_gen && !g_ka_inhibit && !g_ka_idle)) continue;
        g_ka_idle = 1;
        dm_audio_idle(1);
        dm_hle_quiet(1);
        unsigned gen = g_ka_gen;
        pthread_mutex_unlock(&g_ka_m);
        LOG("keepalive: background idle on (gen %u)", gen);
        pthread_mutex_lock(&g_ka_m);
    }
    return NULL;
}
static void ka_init(void) {
    pthread_condattr_t a; pthread_condattr_init(&a); pthread_condattr_setclock(&a, CLOCK_MONOTONIC);
    pthread_cond_init(&g_ka_cv, &a); pthread_condattr_destroy(&a);
    pthread_t th; g_ka_ok = !pthread_create(&th, NULL, ka_thread, NULL);
    if (g_ka_ok) { pthread_setname_np(th, "dm-keepalive"); pthread_detach(th); }
    else LOG("【红】keepalive: idle thread start failed, the audio keeps running in the background");
}
static int ka_ready(void) { pthread_once(&g_ka_once, ka_init); return g_ka_ok; }

static void ka_posted(int ev) {
    if (!ka_ready()) return;
    int woke = 0;
    pthread_mutex_lock(&g_ka_m);
    g_ka_gen++;
    if (ev == 1) { g_ka_want_bg = 1; if (!g_ka_inhibit) ka_arm(); }
    else if (ev == 2 || ev == 3) { g_ka_want_bg = 0; g_ka_deadline = 0; woke = ka_unpark(); }
    pthread_mutex_unlock(&g_ka_m);
    if (woke) LOG("keepalive: background idle off (foreground)");
}


static void ka_inhibit(int d) {
    if (!ka_ready()) return;
    int woke = 0;
    pthread_mutex_lock(&g_ka_m);
    g_ka_inhibit += d;
    if (d > 0) woke = ka_unpark();
    else if (!g_ka_inhibit && g_ka_want_bg && !g_ka_idle) ka_arm();
    pthread_mutex_unlock(&g_ka_m);
    if (woke) LOG("keepalive: background idle off (leave save)");
}
static void ka_leave_result(unsigned covered, int result) {
    pthread_mutex_lock(&g_ka_m);
    if (covered >= g_leave_done) { g_leave_done = covered; g_leave_result = result; }
    pthread_mutex_unlock(&g_ka_m);
}

static void lifecycle_graphics(int on) {
    __android_log_print(ANDROID_LOG_INFO, "dm-lifecycle", "graphics suspend=%d begin", on);
    dm_gfx_suspend(on);
    __android_log_print(ANDROID_LOG_INFO, "dm-lifecycle", "graphics suspend=%d complete", on);
}

static pthread_mutex_t g_silence_lock = PTHREAD_MUTEX_INITIALIZER;
static int g_silence;
static void silence(int who, int on) {
    pthread_mutex_lock(&g_silence_lock);
    g_silence = on ? g_silence | who : g_silence & ~who;
    dm_audio_suspend(g_silence != 0);
    pthread_mutex_unlock(&g_silence_lock);
}
static void lifecycle_audio(int on) { silence(1, on); }


static atomic_uint g_leave_req;
static atomic_int g_leaving;
int dm_leave_save(int wait_book);
void dm_pause_want(int reasons, int on);
int dm_pause_wanted(void);



static void *leave_thread(void *arg) {
    (void)arg;
    for (;;) {
        unsigned seen = atomic_load(&g_leave_req);
        int r = dm_leave_save(1);
        __android_log_print(ANDROID_LOG_INFO, "dm-lifecycle", "leave save result=%d", r);
        LOG("keepalive: leave save request %u result %d", seen, r);
        ka_leave_result(seen, r);
        if (seen != atomic_load(&g_leave_req)) continue;
        atomic_store(&g_leaving, 0);
        if (seen == atomic_load(&g_leave_req) || atomic_exchange(&g_leaving, 1)) { ka_inhibit(-1); return NULL; }
    }
}
static void lifecycle_init(void) {
    dm_lifecycle_ops ops = {dm_focus_set, lifecycle_audio, lifecycle_graphics};
    g_lifecycle_error = dm_lifecycle_start(&g_lifecycle, ops);
    if (!g_lifecycle_error) pthread_setname_np(g_lifecycle.thread, "dm-lifecycle");
    if (g_lifecycle_error) __android_log_print(ANDROID_LOG_ERROR, "dm-lifecycle", "worker start failed rc=%d", g_lifecycle_error);
}
static int lifecycle_ready(void) { pthread_once(&g_lifecycle_once, lifecycle_init); return !g_lifecycle_error; }








static char g_logpath[1200];
static long long g_log_bytes;
static unsigned g_log_gen;
static pthread_mutex_t g_log_mu = PTHREAD_MUTEX_INITIALIZER;
static void log_rotate_session(void) {
    pthread_mutex_lock(&g_log_mu);
    char one[1300]; dm_log_name(one, sizeof one, g_logpath, ".1");
    fclose(g_logf); rename(g_logpath, one); g_log_gen++;
    g_logf = dm_log_open(g_logpath, &g_log_bytes);
    pthread_mutex_unlock(&g_log_mu);
    if (g_logf) { g_log_bytes += fprintf(g_logf, "[dm] boot: 日志轮转：本次会话写满 %d MiB，前面的在 boot.log.1（pid %d）\n", DM_LOG_ROTATE_MAX >> 20, getpid()); fflush(g_logf); }
}
static void log_line(const char *s, size_t n, int newline) {
    if (!g_logf) return;
    fwrite(s, 1, n, g_logf); if (newline) fputc('\n', g_logf); fflush(g_logf);
    if ((g_log_bytes += (long long)n + newline) >= DM_LOG_ROTATE_MAX) log_rotate_session();
}
static void *log_pump(void *arg) {
    int fd = (int)(intptr_t)arg; char buf[4096]; size_t have = 0; ssize_t k;
    while ((k = read(fd, buf + have, sizeof buf - 1 - have)) > 0) {
        have += (size_t)k; buf[have] = 0; char *s = buf, *nl;
        while ((nl = strchr(s, '\n'))) { *nl = 0; __android_log_write(ANDROID_LOG_INFO, "dm", s); log_line(s, (size_t)(nl - s), 1); s = nl + 1; }
        have = strlen(s); memmove(buf, s, have);
        if (have == sizeof buf - 1) { __android_log_write(ANDROID_LOG_INFO, "dm", buf); log_line(buf, have, 0); have = 0; }
    }
    return NULL;
}
static void *log_trim_thread(void *arg) {
    (void)arg; setpriority(PRIO_PROCESS, (id_t)gettid(), 10);
    char tr[1300], one[1300], tmp[1300];
    if (dm_log_name(tr, sizeof tr, g_logpath, ".trim") || dm_log_name(one, sizeof one, g_logpath, ".1") || dm_log_name(tmp, sizeof tmp, g_logpath, ".1.trimmed")) return NULL;
    pthread_mutex_lock(&g_log_mu); unsigned gen = g_log_gen; pthread_mutex_unlock(&g_log_mu);
    int ok = !dm_log_copy_tail(tr, tmp, DM_LOG_KEEP_TAIL); unlink(tr);
    pthread_mutex_lock(&g_log_mu); if (ok && gen == g_log_gen) ok = !rename(tmp, one); else ok = 0; pthread_mutex_unlock(&g_log_mu);
    if (!ok) unlink(tmp);
    LOG("boot: 日志轮转：旧日志%s", ok ? "最后 8 MiB 留在 boot.log.1" : "裁剪没做成，已删掉（腾地方）");
    return NULL;
}
static void log_init(void) {
    static int initialized; if (initialized) return; initialized = 1;
    snprintf(g_logpath, sizeof g_logpath, "%s/boot.log", g_runtime);
    long long old = 0; int trim = 0, rot = dm_log_rotate_at_start(g_logpath, &old, &trim);
    g_logf = dm_log_open(g_logpath, &g_log_bytes);
    int fds[2]; if (pipe(fds)) return;
    dup2(fds[1], 2); close(fds[1]); setvbuf(stderr, NULL, _IOLBF, 0);
    pthread_t th; if (!pthread_create(&th, NULL, log_pump, (void *)(intptr_t)fds[0])) pthread_detach(th);
    if (rot) LOG("boot: 日志轮转：旧的 boot.log %lld 字节%s", old, rot == 1 ? "改名为 boot.log.1" : rot == 2 ? "超过 32 MiB，后台只留最后 8 MiB 到 boot.log.1" : "改名失败，接着写");
    if (trim && !pthread_create(&th, NULL, log_trim_thread, NULL)) pthread_detach(th);
}





void dm_android_perf_sampling(int seconds);
static void log_configure(void) { const char *e = getenv("DM_LOG_PERF_EVERY"); int sec = e ? atoi(e) : 30; dm_android_perf_sampling(sec < 0 ? 30 : sec); }

static void mkdirs(const char *p) {
    char b[1100]; snprintf(b, sizeof b, "%s", p);
    for (char *s = b + 1; *s; s++) if (*s == '/') { *s = 0; mkdir(b, 0700); *s = '/'; }
    mkdir(b, 0700);
}




static int link_romfs(const char *romfs) {
    char data[1100]; snprintf(data, sizeof data, "%s/data", romfs); mkdirs(data);
    DIR *d = opendir(g_src); if (!d) { LOG("【红】资源目录打不开 %s：errno %d（没给「所有文件访问」权限？）", g_src, errno); return -1; }
    struct dirent *e; int n = 0;
    while ((e = readdir(d))) {
        const char *nm = e->d_name; size_t l = strlen(nm);
        if (!(l > 5 && !strcmp(nm + l - 5, ".data")) && !(l > 11 && !strcmp(nm + l - 11, ".data.holes")) && strcmp(nm, ".build.config") && strcmp(nm, "build.config")) continue;
        char from[1100], to[1100]; snprintf(from, sizeof from, "%s/%s", g_src, nm);
        snprintf(to, sizeof to, "%s/%s", data, strcmp(nm, "build.config") ? nm : ".build.config");
        unlink(to); if (!symlink(from, to)) n++;
    }
    closedir(d);
    return n;
}

static void *host_thread(void *arg) {
    (void)arg;
    char out[1200]; snprintf(out, sizeof out, "%s/out", g_runtime);


    char shaders[1200], error[512] = {0};
    snprintf(shaders, sizeof shaders, "%s/shaders", g_runtime);
    int graphics_rc = dm_android_vk_initialize(shaders, error, sizeof error);
    if (graphics_rc) { LOG("boot: Vulkan initialization failed %d: %s", graphics_rc, error); front_android_boot_failed(-6); return NULL; }
    char *argv[] = {"dmhost", out, "main", "sdk", NULL};
    LOG("boot: dm_main %s", out);
    int rc = dm_main(4, argv);
    LOG("boot: dm_main 返回 %d", rc);
    front_android_boot_failed(-7);
    return NULL;
}




#ifdef DM_PLAYER_BUILD
static void env_file_ignored(const char *dir) {
    char path[1300]; snprintf(path, sizeof path, "%s/env", dir);
    if (!access(path, F_OK)) LOG("boot: variant player ignores files/dm/env (%s)", path);
}
#else
static void load_env_file(const char *dir) {
    char path[1300]; snprintf(path, sizeof path, "%s/env", dir);
    FILE *fp = fopen(path, "r"); if (!fp) return;
    char line[512];
    while (fgets(line, sizeof line, fp)) {
        line[strcspn(line, "\r\n")] = 0;
        char *eq = strchr(line, '=');
        if (!line[0] || line[0] == '#' || !eq || eq == line) continue;
        *eq = 0; setenv(line, eq + 1, 1); LOG("boot: 环境 %s=%s（来自 %s）", line, eq + 1, path);
    }
    fclose(fp);
}
#endif













static void cpu_translation_defaults(void) {
    char abilist[PROP_VALUE_MAX] = "", abi[PROP_VALUE_MAX] = "", bridge[PROP_VALUE_MAX] = "", qemu[PROP_VALUE_MAX] = "";
    __system_property_get("ro.product.cpu.abilist", abilist);
    __system_property_get("ro.product.cpu.abi", abi);
    __system_property_get("ro.dalvik.vm.native.bridge", bridge);
    if (__system_property_get("ro.kernel.qemu", qemu) <= 0) __system_property_get("ro.boot.qemu", qemu);
    struct utsname u; if (uname(&u)) snprintf(u.machine, sizeof u.machine, "?");
#ifdef HWCAP_ATOMICS
    int lse = (getauxval(AT_HWCAP) & HWCAP_ATOMICS) != 0;
#else
    int lse = -1;
#endif
    int t = dm_android_cpu_translation(abilist, abi);
    LOG("boot: cpu %s (abilist %s, abi %s, native bridge %s, qemu %s, kernel %s, LSE atomics %d)",
        t == DM_CPU_TRANSLATED ? "translated" : t == DM_CPU_NATIVE ? "native" : "unknown (no change)",
        abilist[0] ? abilist : "-", abi[0] ? abi : "-", bridge[0] ? bridge : "-", qemu[0] ? qemu : "-", u.machine, lse);
    if (t != DM_CPU_TRANSLATED) return;
    dm_android_translator_defaults();
    LOG("boot: arm64 translator: write-protect trackers DM_GFX_VCACHE=%s DM_GFX_TEX_DIRTY=%s DM_GFX_WTRACK=%s (default 0 here; files/dm/env and the marker files still win)",
        getenv("DM_GFX_VCACHE"), getenv("DM_GFX_TEX_DIRTY"), getenv("DM_GFX_WTRACK"));
}










static int overlay_variant(const char *overlay, char *out, size_t n) {
    char vdir[1300]; snprintf(vdir, sizeof vdir, "%s_variants", overlay);
    DIR *d = opendir(vdir); if (!d) return 0;
    char names[16][64]; int nv = 0; struct dirent *e;
    while ((e = readdir(d)) && nv < 16) if (e->d_name[0] != '.' && strlen(e->d_name) < sizeof names[0]) strlcpy(names[nv++], e->d_name, sizeof names[0]);
    closedir(d);




    char keys[16][64]; int nk = 0, on = 0;
    for (int i = 0; i < nv; i++) { char t[64], *sp = NULL; strlcpy(t, names[i], sizeof t);
        for (char *k = strtok_r(t, "+", &sp); k; k = strtok_r(NULL, "+", &sp)) {
            int seen = 0; for (int j = 0; j < nk; j++) seen |= !strcmp(keys[j], k);
            if (!seen && nk < 16) { strlcpy(keys[nk++], k, sizeof keys[0]); on += dm_custom_get(k) != 0; } } }
    if (!on) { LOG("boot: 覆盖层变体 %d 份，开关都关着，用 overlay", nv); return 0; }
    for (int i = 0; i < nv; i++) { char t[64], *sp = NULL; int cnt = 0, all = 1; strlcpy(t, names[i], sizeof t);
        for (char *k = strtok_r(t, "+", &sp); k; k = strtok_r(NULL, "+", &sp)) { cnt++; all &= dm_custom_get(k) != 0; }
        if (!all || cnt != on) continue;
        char path[1400], reason[256]; snprintf(path, sizeof path, "%s/%s", vdir, names[i]);
        dm_overlay_result r = dm_overlay_validate(path, DM_OVERLAY_SOURCE_SHA256, reason, sizeof reason);
        if (r != DM_OVERLAY_OK) { LOG("boot: 【红】覆盖层变体 %s 不能用（%d：%s），按开关全关启动", names[i], r, reason); return 0; }
        if (snprintf(out, n, "%s:%s", path, overlay) >= (int)n) return 0;
        LOG("boot: 覆盖层变体 %s（开关开着，叠在 overlay 前面）", names[i]); return 1; }
    LOG("boot: 【红】开着的 %d 个覆盖层开关没有对应的变体，按开关全关启动", on);
    return 0;
}

const char *dm_android_runtime_dir(void) { return g_runtime; }
JNIEXPORT jint JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeStart(JNIEnv *env, jclass cls, jstring files, jstring ext, jstring src, jstring lang) {
    (void)cls;
    static int started; if (started) return 0;
    const char *f = (*env)->GetStringUTFChars(env, files, NULL), *x = (*env)->GetStringUTFChars(env, ext, NULL), *s = (*env)->GetStringUTFChars(env, src, NULL), *lg = (*env)->GetStringUTFChars(env, lang, NULL);
    snprintf(g_files, sizeof g_files, "%s", f); snprintf(g_ext, sizeof g_ext, "%s", x); snprintf(g_src, sizeof g_src, "%s", s);


    jclass proc = (*env)->FindClass(env, "android/os/Process");
    g_proc_start_ns = (uint64_t)(*env)->CallStaticLongMethod(env, proc, (*env)->GetStaticMethodID(env, proc, "getStartUptimeMillis", "()J")) * 1000000ull;


    char p[1200]; snprintf(g_runtime, sizeof g_runtime, "%s/dm", g_files);
    snprintf(p, sizeof p, "%s/out/main.patched.bin", g_runtime);
    if (access(p, R_OK)) snprintf(g_runtime, sizeof g_runtime, "%s/dm", g_ext);
    mkdirs(g_runtime);
#ifdef DM_PLAYER_BUILD
    dm_player_guard_set_dir(g_runtime);
#endif
    log_init();
#ifndef DM_PLAYER_BUILD
    load_env_file(g_runtime);
#endif
    log_configure();
    LOG("boot: 内部目录 %s，外部目录 %s，资源 %s，系统语言 %s", g_files, g_ext, g_src, lg);




    LOG("boot: variant " DM_VARIANT_NAME);
#ifdef DM_PLAYER_BUILD
    env_file_ignored(g_runtime);
#endif
    cpu_translation_defaults();
    char romfs[1100], save[1100], temp[1100];
    snprintf(romfs, sizeof romfs, "%s/romfs", g_files); snprintf(save, sizeof save, "%s/save", g_files); snprintf(temp, sizeof temp, "%s/temp", g_files);
    mkdirs(save); mkdirs(temp);
    int n = link_romfs(romfs);
    LOG("boot: 资源链接 %d 个文件", n);
    setenv("DM_ROMFS", romfs, 0); setenv("DM_SAVE", save, 0); setenv("DM_TEMP", temp, 0);


    { const char *r = getenv("DM_ROMFS"); snprintf(g_data_src, sizeof g_data_src, "%s", r && strcmp(r, romfs) ? r : g_src); }




    { const char *sv = getenv("DM_SAVE"); char sd[1100]; strlcpy(sd, sv ? sv : g_files, sizeof sd); char *sl = strrchr(sd, '/'); if (sl) *sl = 0; dm_custom_init(sd); }


    { char sw[1200], *sl; snprintf(sw, sizeof sw, "%s", g_src); sl = strrchr(sw, '/'); if (sl) { snprintf(sl + 1, sizeof sw - (size_t)(sl + 1 - sw), "fullscreen-off");
        if (!access(sw, F_OK)) { setenv("DM_ANDROID_FULL", "0", 1); LOG("boot: fullscreen-off in the game folder -> DM_ANDROID_FULL=0"); } } }
    { void front_android_res_latch(void); front_android_res_latch(); }
#ifdef DM_ANDROID_TEST_HOOKS
    { void dm_test_hooks_start(const char *files_dir); dm_test_hooks_start(g_files); }
#endif



    setenv("DM_GFX_WB_SKIP", "1", 0);



    setenv("DM_GFX_PIPE_MEMO", "1", 0);




    setenv("DM_NVN_SYNC", "1", 0);



    setenv("DM_GFX_NATIVE_BC", "1", 0);





    setenv("DM_GFX_QUAD_DERIVATIVES", "1", 0);





    setenv("DM_GFX_WARP32", "1", 0);





    setenv("DM_GFX_LDC_FOLD", "1", 0);
    setenv("DM_GFX_FORWARD_TEMPS", "1", 0);





    setenv("DM_GFX_NAN_MINMAX", "1", 0);





    setenv("DM_GFX_DEFER_EVENTS", "2", 0);





    setenv("DM_GFX_READ_PREFLIGHT", "0", 0);





    setenv("DM_GFX_XVIEW_GUARD", "1", 0);







    setenv("DM_GFX_UPLOAD_PROLOGUE", "1", 0);





    setenv("DM_GFX_SHADER_REUSE", "1", 0);



    setenv("DM_GFX_VCACHE", "1", 0);





    setenv("DM_GFX_TEX_DIRTY", "1", 0);



    setenv("DM_GFX_ASYNC_POOL_RESET", "1", 0);




    setenv("DM_GFX_PERF_SAMPLE", "0", 0);



    setenv("DM_HLE_SPIN_US", "30", 0);
    if (!getenv("DM_LANG") && *lg) setenv("DM_LANG", lg, 0);
    if (chdir(g_files)) LOG("【红】chdir %s 失败 errno %d", g_files, errno);
    (*env)->ReleaseStringUTFChars(env, files, f); (*env)->ReleaseStringUTFChars(env, ext, x); (*env)->ReleaseStringUTFChars(env, src, s); (*env)->ReleaseStringUTFChars(env, lang, lg);
    if (n <= 0) return -2;

    extern int dm_derived_reload(void) __attribute__((weak));
    snprintf(p, sizeof p, "%s/derived.tsv", g_runtime);
    static int derived_pending;
    const char *derived = getenv("DM_DERIVED");
    if (!derived || strcmp(derived, p)) derived_pending = 1;
    if (derived_pending) {
        if (setenv("DM_DERIVED", p, 1) || (dm_derived_reload && dm_derived_reload())) {
            LOG("boot.derived: runtime tables could not be loaded");
            return -11;
        }
        derived_pending = 0;
    }
    char required[1300];
    const char *modules[] = {"main.patched.bin", "main.fixups.tsv", "main.gates.tsv", "sdk.patched.bin", "sdk.fixups.tsv", "sdk.gates.tsv"};
    for (unsigned i = 0; i < sizeof modules / sizeof modules[0]; i++) {
        snprintf(required, sizeof required, "%s/out/%s", g_runtime, modules[i]);
        if (access(required, R_OK)) { LOG("boot: missing %s", required); return -3; }
    }
    snprintf(required, sizeof required, "%s/data/.build.config", romfs);
    if (access(required, R_OK)) { LOG("boot: missing build configuration"); return -5; }
    snprintf(required, sizeof required, "%s/shaders", g_runtime);
    DIR *shaders = opendir(required); int have_shader = 0;
    if (shaders) { struct dirent *entry; while ((entry = readdir(shaders))) {
        size_t len = strlen(entry->d_name);
        if (len > 4 && !strcmp(entry->d_name + len - 4, ".xgs")) { have_shader = 1; break; }
    } closedir(shaders); }
    if (!have_shader) { LOG("boot: no shader bundles in %s", required); return -4; }
    snprintf(required, sizeof required, "%s/overlay", g_runtime);


    char overlay_reason[256];
    dm_overlay_result overlay = dm_overlay_validate(required, DM_OVERLAY_SOURCE_SHA256, overlay_reason, sizeof overlay_reason);
    if (overlay == DM_OVERLAY_OK) overlay = dm_overlay_require_touch_art(required, overlay_reason, sizeof overlay_reason);
    if (overlay != DM_OVERLAY_OK) {
        LOG("boot: overlay rejected (%d): %s", overlay, overlay_reason);
        return overlay == DM_OVERLAY_MISSING ? -9 : -10;
    }
    LOG("boot: overlay verified source=%s", DM_OVERLAY_SOURCE_SHA256);
    char layered[2800];
    setenv("DM_ROMFS_OVERLAY", overlay_variant(required, layered, sizeof layered) ? layered : required, 1);
    if (!lifecycle_ready()) return -8;
    pthread_attr_t a; pthread_attr_init(&a); pthread_attr_setstacksize(&a, 8u << 20);
    pthread_t th; int rc = pthread_create(&th, &a, host_thread, NULL); pthread_attr_destroy(&a);
    if (!rc) { started = 1; pthread_detach(th); }
    return rc ? -1 : n;
}




int dm_lang_index(void);
JNIEXPORT jint JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeGameLang(JNIEnv *env, jclass cls) { (void)env; (void)cls; return dm_lang_index(); }

static ANativeWindow *g_win;



int dm_cam_sensor_mode(void); void dm_cam_turn(float rad); int dm_cam_mode(void); int dm_cam_on(void);
JNIEXPORT jint JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeCamSensor(JNIEnv *env, jclass cls) { (void)env; (void)cls; return dm_cam_sensor_mode(); }



JNIEXPORT jint JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeCamMode(JNIEnv *env, jclass cls) { (void)env; (void)cls; return dm_cam_mode(); }
JNIEXPORT jint JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeCamActive(JNIEnv *env, jclass cls) { (void)env; (void)cls; return dm_cam_on(); }



int dm_cam_style(void);
JNIEXPORT jint JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeCamStyle(JNIEnv *env, jclass cls) { (void)env; (void)cls; return dm_cam_style(); }


int dm_cam_aim(float out[50]);
JNIEXPORT jint JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeCamAim(JNIEnv *env, jclass cls, jfloatArray out) {
    (void)cls; float buf[50]; if ((*env)->GetArrayLength(env, out) < 50) return 0; int n = dm_cam_aim(buf);
    if (n == 25) (*env)->SetFloatArrayRegion(env, out, 0, 50, buf); return n; }



int dm_cam_threats(float *out, int max);
JNIEXPORT jint JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeCamThreats(JNIEnv *env, jclass cls, jfloatArray out) {
    (void)cls; jsize len = (*env)->GetArrayLength(env, out); float buf[16 * 5]; int max = len / 5 < 16 ? len / 5 : 16;
    int n = dm_cam_threats(buf, max); if (n > 0) (*env)->SetFloatArrayRegion(env, out, 0, n * 5, buf); return n; }



int dm_cam_map_heading(float out[4]);
JNIEXPORT jint JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeCamHeading(JNIEnv *env, jclass cls, jfloatArray out) {
    (void)cls; float buf[4]; int map; if ((*env)->GetArrayLength(env, out) < 4 || !(map = dm_cam_map_heading(buf))) return 0;
    (*env)->SetFloatArrayRegion(env, out, 0, 4, buf); return map; }
JNIEXPORT void JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeCamTurn(JNIEnv *env, jclass cls, jfloat rad) { (void)env; (void)cls; dm_cam_turn(rad); }
JNIEXPORT void JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeRefresh(JNIEnv *env, jclass cls, jfloat hz) {
    (void)env; (void)cls;
    front_android_set_refresh(hz);
}


void front_android_set_modes(const float *hz, int n, int pw, int ph);
JNIEXPORT void JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeDisplayModes(JNIEnv *env, jclass cls, jfloatArray hz, jint pw, jint ph) {
    (void)cls; float v[16]; jsize n = hz ? (*env)->GetArrayLength(env, hz) : 0; if (n > 16) n = 16; if (n) (*env)->GetFloatArrayRegion(env, hz, 0, n, v);
    front_android_set_modes(v, (int)n, pw, ph); }
JNIEXPORT void JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeSurface(JNIEnv *env, jclass cls, jobject surface, jint w, jint h) {
    (void)cls;
    ANativeWindow *old = g_win;
    g_win = surface ? ANativeWindow_fromSurface(env, surface) : NULL;
    dm_android_vk_set_window(g_win);
    front_android_set_window(g_win, w, h);
    if (old) ANativeWindow_release(old);
    LOG("前端：画面 %s %dx%d", g_win ? "就绪" : "收回", w, h);
}



JNIEXPORT void JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeTouch(JNIEnv *env, jclass cls, jint n, jintArray ids, jfloatArray xs, jfloatArray ys) {
    (void)cls;
    dm_touch t[16]; if (n > 16) n = 16; if (n < 0) n = 0;
    jint id[16]; jfloat x[16], y[16];
    if (n) { (*env)->GetIntArrayRegion(env, ids, 0, n, id); (*env)->GetFloatArrayRegion(env, xs, 0, n, x); (*env)->GetFloatArrayRegion(env, ys, 0, n, y); }
    for (int i = 0; i < n; i++) t[i] = (dm_touch){(uint32_t)id[i], x[i], y[i]};
    dm_touch_set(t, n);
}




JNIEXPORT void JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativePad(JNIEnv *env, jclass cls, jlong buttons, jint lx, jint ly, jint rx, jint ry) {
    (void)env; (void)cls;

    static uint64_t prev;
    static int logged;
    if (buttons && !prev && logged < 30) {
        LOG("手柄：按下 buttons=%#llx lx=%d ly=%d rx=%d ry=%d", (unsigned long long)buttons, lx, ly, rx, ry);
        ++logged;
    }
    prev = (uint64_t)buttons;
    dm_pad_set((uint64_t)buttons, lx, ly, rx, ry);
}


JNIEXPORT void JNICALL Java_com_xiaoboluo_d2r_AppArt_nativeLog(JNIEnv *env, jclass cls, jstring line) {
    (void)cls; const char *c = line ? (*env)->GetStringUTFChars(env, line, NULL) : NULL; if (!c) return;
    LOG("%s", c); (*env)->ReleaseStringUTFChars(env, line, c); }


int dm_dmgnum_copy(float *out, int max);
JNIEXPORT jint JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeDamageNumbers(JNIEnv *env, jclass cls, jfloatArray out) {
    enum { FIELDS = 8, MAX = 32 };
    (void)cls; jsize len = (*env)->GetArrayLength(env, out); float buf[MAX * FIELDS]; int max = len / FIELDS < MAX ? len / FIELDS : MAX;
    int n = dm_dmgnum_copy(buf, max); if (n > 0) (*env)->SetFloatArrayRegion(env, out, 0, n * FIELDS, buf); return n; }

int dm_ui_sort_buttons(float *out);
void dm_inv_sort(int page);
JNIEXPORT jint JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeSortButtons(JNIEnv *env, jclass cls, jfloatArray out) { (void)cls; float buf[6]; int n = dm_ui_sort_buttons(buf); if (n > 0) (*env)->SetFloatArrayRegion(env, out, 0, n * 3, buf); return n; }
JNIEXPORT void JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeInvSort(JNIEnv *env, jclass cls, jint page) { (void)env; (void)cls; int dm_itemjob_owner(void); if (!dm_itemjob_owner()) dm_inv_sort(page); }


void dm_belt_drink(int kind);
int dm_belt_kinds(void);
int dm_char_data(void);
int dm_recipes_request_stock(void);
int dm_recipes_stock_copy(void *out, int bytes);
int dm_recipes_insert(const int *plan, int steps, uint32_t cube_id, uint32_t gen);
int dm_recipes_insert_state(void);
void dm_recipes_text_start(const char *dir);
int dm_recipes_text(int rec, char *out, int bytes);
int dm_item_text(int kind, int row, int grp, char *out, int bytes);
void dm_recipes_abort(void);
int dm_recipes_busy(void);
void dm_hud_request_players(int n);
extern _Atomic int dm_fps_cap;
void dm_present_vote(float hz);










int dm_display_modes(int *hz, int cap);
int dm_fps_vote_policy(void) {
    static int p = -1; if (p >= 0) return p;
    char path[1200], b[16] = ""; snprintf(path, sizeof path, "%s/gfx-frame-rate-policy", g_runtime); FILE *f = fopen(path, "r"); if (f) { if (!fgets(b, sizeof b, f)) b[0] = 0; fclose(f); }
    p = !strncmp(b, "highest", 7); if (f) LOG("帧率：标记 gfx-frame-rate-policy=%s → 投%s", b[0] ? b : "?", p ? "能整除的最高刷新率" : "能整除的最低刷新率");
    return p; }
void dm_fps_apply(int fps) {
    int m[16], n = dm_display_modes(m, 16), even = 1, pol = dm_fps_vote_policy(), hz = dm_fps_vote(fps, m, n, pol, &even);
    atomic_store(&dm_fps_cap, fps); dm_present_vote((float)hz);
    LOG("帧率：fps=%d vote=%d Hz（%s，policy %s，屏 %d 档刷新率）", fps, hz, even ? "even" : "uneven", pol ? "highest" : "lowest", n); }
int dm_video_caps(void) { return 1 | 2 | 4 | 8; }
void dm_custom_legacy(int fps_cap, int status); int dm_custom_status(void); int dm_fps_setting(void);
JNIEXPORT void JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeLegacySettings(JNIEnv *env, jclass cls, jint fps, jint status) { (void)env; (void)cls; dm_custom_legacy(fps, status); }
JNIEXPORT jint JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeStatusLines(JNIEnv *env, jclass cls) { (void)env; (void)cls; return dm_custom_status(); }
JNIEXPORT jint JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeFpsChoice(JNIEnv *env, jclass cls) { (void)env; (void)cls; return dm_fps_setting(); }


JNIEXPORT jboolean JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeStatusEffectsOn(JNIEnv *env, jclass cls) { (void)env; (void)cls; return dm_custom_get("status_fx") != 0; }





int dm_extras_caps(void) { return 1 | 2; }
void dm_custom_legacy_shoulder(int left, int right); void dm_custom_has_triggers(int on); int dm_custom_tech_take(void); int dm_custom_get(const char *key); int dm_hud_players_wanted(void);
const char *xbg_gpu_label(void);
const char *dm_data_source(void) { return g_data_src; }
JNIEXPORT void JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeLegacyShoulder(JNIEnv *env, jclass cls, jint left, jint right) { (void)env; (void)cls; dm_custom_legacy_shoulder(left, right); }
JNIEXPORT jint JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeShoulderAction(JNIEnv *env, jclass cls, jint hand) { (void)env; (void)cls; return dm_custom_get(hand ? "shoulder_r" : "shoulder_l"); }
JNIEXPORT void JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeSetHasTriggers(JNIEnv *env, jclass cls, jint on) { (void)env; (void)cls; dm_custom_has_triggers(on); }
JNIEXPORT jint JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeTechRequest(JNIEnv *env, jclass cls) { (void)env; (void)cls; return dm_custom_tech_take(); }
JNIEXPORT jint JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativePlayersWanted(JNIEnv *env, jclass cls) { (void)env; (void)cls; return dm_hud_players_wanted(); }
JNIEXPORT jstring JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeDataSource(JNIEnv *env, jclass cls) { (void)cls; return (*env)->NewStringUTF(env, g_data_src); }
JNIEXPORT jstring JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeGpuLabel(JNIEnv *env, jclass cls) { (void)cls; const char *g = xbg_gpu_label(); return g && *g ? (*env)->NewStringUTF(env, g) : NULL; }





JNIEXPORT void JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeVkDriver(JNIEnv *env, jclass cls, jstring dir, jstring lib, jstring hooks, jstring tu, jstring attempt, jstring state, jstring label, jint test_fail) {
    (void)cls; jstring in[7] = {dir, lib, hooks, tu, attempt, state, label}; const char *c[7] = {0};
    for (int i = 0; i < 7; i++) c[i] = in[i] ? (*env)->GetStringUTFChars(env, in[i], NULL) : NULL;
    dm_vk_driver_config(c[0], c[1], c[2], c[3], c[4], c[5], c[6], test_fail);
    for (int i = 0; i < 7; i++) if (c[i]) (*env)->ReleaseStringUTFChars(env, in[i], c[i]);
}
JNIEXPORT jstring JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeDriverState(JNIEnv *env, jclass cls) { (void)cls; char b[200]; dm_vk_driver_state(b, sizeof b); return (*env)->NewStringUTF(env, b); }
int dm_driver_label(char *buf, int cap) { return dm_vk_driver_label(buf, (size_t)(cap > 0 ? cap : 0)); }
static atomic_int g_driver_open;
void dm_driver_open(void) { atomic_store(&g_driver_open, 1); }
JNIEXPORT jint JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeDriverRequest(JNIEnv *env, jclass cls) { (void)env; (void)cls; return atomic_exchange(&g_driver_open, 0); }


int dm_fps_measured(void);
void dm_fps_reset(void);
JNIEXPORT jint JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeFpsMeasured(JNIEnv *env, jclass cls) { (void)env; (void)cls; return dm_fps_measured(); }


int dm_ui_portrait_bottom(void);
JNIEXPORT jint JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativePortraitBottom(JNIEnv *env, jclass cls) { (void)env; (void)cls; return dm_ui_portrait_bottom(); }



int dm_ui_level_word(char *out, int cap);
JNIEXPORT jbyteArray JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeLevelWord(JNIEnv *env, jclass cls) {
    (void)cls; char w[64]; int n = dm_ui_level_word(w, sizeof w); if (n <= 0) return NULL;
    jbyteArray a = (*env)->NewByteArray(env, n); if (a) (*env)->SetByteArrayRegion(env, a, 0, n, (const jbyte *)w); return a;
}




int dm_padmode_busy(void);
JNIEXPORT jint JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeDisplayState(JNIEnv *env, jclass cls) {
    (void)env; (void)cls; float s[DM_ANDROID_TOUCH_DRAW_FLOATS]; dm_android_touch_layout(s);
    return (s[2] == 0 && s[DM_ANDROID_TOUCH_HUD] != 0 ? 1 : 0) | (dm_padmode_busy() ? 2 : 0); }



JNIEXPORT jint JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativePadBusy(JNIEnv *env, jclass cls) { (void)env; (void)cls; return dm_padmode_busy(); }
JNIEXPORT void JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativePlusReads(JNIEnv *env, jclass cls, jlongArray out) {
    (void)cls; uint64_t on = 0, off = 0; dm_input_plus_reads(&on, &off); jlong v[2] = { (jlong)on, (jlong)off }; (*env)->SetLongArrayRegion(env, out, 0, 2, v); }
JNIEXPORT void JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativePadUnlatch(JNIEnv *env, jclass cls, jlong mask) { (void)env; (void)cls; dm_pad_unlatch((uint64_t)mask); }
void dm_ui_rect(int *x, int *y, int *w, int *h);
int dm_screen_real(int *w, int *h);
JNIEXPORT jint JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeUiRect(JNIEnv *env, jclass cls, jintArray out) {
    (void)cls; jint v[6] = {0}; dm_ui_rect(&v[0], &v[1], &v[2], &v[3]); if (!dm_screen_real(&v[4], &v[5]) || v[3] <= 0) return 0;
    (*env)->SetIntArrayRegion(env, out, 0, 6, v); return 1; }

void front_android_view_rect(int win_w, int win_h, int out[8]);
JNIEXPORT void JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeViewRect(JNIEnv *env, jclass cls, jint w, jint h, jintArray out) {
    (void)cls; if (!out || (*env)->GetArrayLength(env, out) < 8) return; int r[8]; jint v[8];
    front_android_view_rect((int)w, (int)h, r); for (int i = 0; i < 8; i++) v[i] = r[i];
    (*env)->SetIntArrayRegion(env, out, 0, 8, v); }
JNIEXPORT void JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativePlayers(JNIEnv *env, jclass cls, jint players) { (void)env; (void)cls; dm_hud_request_players(players); }
JNIEXPORT void JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativePause(JNIEnv *env, jclass cls, jint reasons, jint on) { (void)env; (void)cls; dm_pause_want(reasons, on); silence(2, dm_pause_wanted()); dm_fps_reset(); }


JNIEXPORT void JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeLeave(JNIEnv *env, jclass cls) { (void)env; (void)cls;
    pthread_t t; pthread_attr_t a;
    ka_inhibit(1);
    atomic_fetch_add(&g_leave_req, 1);
    if (atomic_exchange(&g_leaving, 1)) { ka_inhibit(-1); return; }
    pthread_attr_init(&a); pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED);
    int rc = pthread_create(&t, &a, leave_thread, NULL);
    if (rc) {
        unsigned seen;
        do {
            seen = atomic_load(&g_leave_req);
            int r = dm_leave_save(0);
            __android_log_print(ANDROID_LOG_ERROR, "dm-lifecycle", "leave thread failed rc=%d, saving here: result=%d", rc, r);
            ka_leave_result(seen, r);
        } while (seen != atomic_load(&g_leave_req));
        atomic_store(&g_leaving, 0);
        ka_inhibit(-1);
    }
    pthread_attr_destroy(&a); }



JNIEXPORT void JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeLeaveState(JNIEnv *env, jclass cls, jlongArray out) {
    (void)cls; if (!out || (*env)->GetArrayLength(env, out) < 3) return;
    pthread_mutex_lock(&g_ka_m); jlong v[3] = {(jlong)atomic_load(&g_leave_req), (jlong)g_leave_done, (jlong)g_leave_result}; pthread_mutex_unlock(&g_ka_m);
    (*env)->SetLongArrayRegion(env, out, 0, 3, v); }


void dm_resume_app_left(void); int dm_resume_offered(void); int dm_resume_choice(int here); void dm_resume_exit_game(void);
JNIEXPORT void JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeResumeLeft(JNIEnv *env, jclass cls) { (void)env; (void)cls; dm_resume_app_left(); }
JNIEXPORT void JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeResumeDrop(JNIEnv *env, jclass cls) { (void)env; (void)cls; dm_resume_exit_game(); }
JNIEXPORT jint JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeResumeOffer(JNIEnv *env, jclass cls) { (void)env; (void)cls; return dm_resume_offered(); }
JNIEXPORT jint JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeResumeChoose(JNIEnv *env, jclass cls, jint here) { (void)env; (void)cls; return dm_resume_choice(here != 0); }
int dm_pause_reasons(void);
JNIEXPORT jint JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativePauseReasons(JNIEnv *env, jclass cls) { (void)env; (void)cls; return dm_pause_reasons(); }
JNIEXPORT void JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeShutdown(JNIEnv *env, jclass cls) { (void)env; (void)cls; dm_host_shutdown("Android: keepalive quit"); }
int dm_ingame_now(void), dm_uiscreen_ingame(void);
JNIEXPORT jint JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeInGame(JNIEnv *env, jclass cls) { (void)env; (void)cls; return dm_ingame_now() && dm_uiscreen_ingame(); }
JNIEXPORT void JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeBeltDrink(JNIEnv *env, jclass cls, jint kind) { (void)env; (void)cls; dm_belt_drink(kind); }
JNIEXPORT jint JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeBeltKinds(JNIEnv *env, jclass cls) { (void)env; (void)cls; return dm_belt_kinds(); }
JNIEXPORT jint JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeRecipeRequest(JNIEnv *env, jclass cls) { (void)env; (void)cls; return dm_recipes_request_stock(); }
JNIEXPORT jint JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeRecipeInsert(JNIEnv *env, jclass cls, jintArray plan, jint cube, jint gen) { (void)cls; jsize n = (*env)->GetArrayLength(env, plan); if (n <= 0 || n % 13 || n > 13 * 24) return 0; jint buf[13 * 24]; (*env)->GetIntArrayRegion(env, plan, 0, n, buf); return dm_recipes_insert((const int *)buf, n / 13, (uint32_t)cube, (uint32_t)gen); }
JNIEXPORT void JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeRecipeTextStart(JNIEnv *env, jclass cls, jstring dir) { (void)cls; const char *d = (*env)->GetStringUTFChars(env, dir, NULL); if (d) { dm_recipes_text_start(d); (*env)->ReleaseStringUTFChars(env, dir, d); } }
JNIEXPORT jbyteArray JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeRecipeText(JNIEnv *env, jclass cls, jint rec) { (void)cls; static char t[4096]; int n = dm_recipes_text(rec, t, sizeof t); if (n <= 0) return NULL; jbyteArray a = (*env)->NewByteArray(env, n); if (a) (*env)->SetByteArrayRegion(env, a, 0, n, (const jbyte *)t); return a; }



JNIEXPORT jbyteArray JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeItemText(JNIEnv *env, jclass cls, jint kind, jint row, jint grp) {
    (void)cls; char *t = malloc(4096); if (!t) return NULL;
    int n = dm_item_text(kind, row, grp, t, 4096); jbyteArray a = NULL;
    if (n < 0) a = (*env)->NewByteArray(env, 0);
    else if (n > 0 && (a = (*env)->NewByteArray(env, n))) (*env)->SetByteArrayRegion(env, a, 0, n, (const jbyte *)t);
    free(t); return a;
}
JNIEXPORT jint JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeRecipeInsertState(JNIEnv *env, jclass cls) { (void)env; (void)cls; return dm_recipes_insert_state(); }
JNIEXPORT void JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeRecipeAbort(JNIEnv *env, jclass cls) { (void)env; (void)cls; dm_recipes_abort(); }
JNIEXPORT jint JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeRecipeBusy(JNIEnv *env, jclass cls) { (void)env; (void)cls; return dm_recipes_busy(); }
JNIEXPORT jint JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeRecipeStock(JNIEnv *env, jclass cls, jobject buffer) { (void)cls; void *p = (*env)->GetDirectBufferAddress(env, buffer); jlong n = (*env)->GetDirectBufferCapacity(env, buffer); return p && n > 0 ? dm_recipes_stock_copy(p, (int)n) : 0; }


JNIEXPORT jint JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeEffects(JNIEnv *env, jclass cls, jintArray out) {
    (void)cls; if (!out) return 0;
    jsize n = (*env)->GetArrayLength(env, out); jint *p = (*env)->GetIntArrayElements(env, out, NULL); if (!p) return 0;
    int w = dm_effects_copy((int *)p, (int)n); (*env)->ReleaseIntArrayElements(env, out, p, 0); return w; }

#include "../host/dm_streak.h"
JNIEXPORT jint JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeStreak(JNIEnv *env, jclass cls, jintArray out) {
    (void)cls; if (!out) return 0;
    int buf[DM_SK_HUD_N]; jsize n = (*env)->GetArrayLength(env, out);
    int k = dm_streak_hud(buf, n < DM_SK_HUD_N ? n : DM_SK_HUD_N);
    if (k > 0) (*env)->SetIntArrayRegion(env, out, 0, k, buf);
    return k; }
JNIEXPORT jint JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeCharData(JNIEnv *env, jclass cls) { (void)env; (void)cls; return dm_char_data(); }



#include "../host/dm_glossary.h"
JNIEXPORT void JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeGlossaryWant(JNIEnv *env, jclass cls, jint on) { (void)env; (void)cls; dm_glossary_want(on); }
JNIEXPORT jint JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeGlossary(JNIEnv *env, jclass cls, jintArray out) {
    (void)cls; int buf[DM_GLOSS_INTS]; jsize n = (*env)->GetArrayLength(env, out);
    int k = dm_glossary_copy(buf, n < DM_GLOSS_INTS ? n : DM_GLOSS_INTS);
    if (k > 0) (*env)->SetIntArrayRegion(env, out, 0, k, buf);
    return k;
}



#include "../host/dm_build.h"
JNIEXPORT jint JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeBuildState(JNIEnv *env, jclass cls, jintArray out, jbyteArray name) {
    (void)cls; int buf[DM_BUILD_INTS]; char nm[DM_BUILD_NAME]; int nl = 0;
    jsize n = (*env)->GetArrayLength(env, out), nn = (*env)->GetArrayLength(env, name);
    int k = dm_build_state(buf, n < DM_BUILD_INTS ? n : DM_BUILD_INTS, nm, nn < DM_BUILD_NAME ? nn : DM_BUILD_NAME, &nl);
    (*env)->SetIntArrayRegion(env, out, 0, k, buf);
    if (nl > 0) (*env)->SetByteArrayRegion(env, name, 0, nl, (const jbyte *)nm);
    return k << 8 | nl;
}
JNIEXPORT jint JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeBuildSkills(JNIEnv *env, jclass cls, jintArray ids) {
    (void)cls; int buf[DM_BUILD_MAX_SKILLS]; jsize n = (*env)->GetArrayLength(env, ids); if (n > DM_BUILD_MAX_SKILLS) n = DM_BUILD_MAX_SKILLS;
    (*env)->GetIntArrayRegion(env, ids, 0, n, buf); return dm_build_skills(buf, n);
}
JNIEXPORT void JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeBuildTarget(JNIEnv *env, jclass cls, jint session, jint page, jint row, jint col, jint show, jint guide) {
    (void)env; (void)cls; dm_build_target(session, page, row, col, show, guide);
}
JNIEXPORT jint JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeBuildOpenRequest(JNIEnv *env, jclass cls) { (void)env; (void)cls; return dm_build_open_take(); }


JNIEXPORT void JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeBuildCard(JNIEnv *env, jclass cls, jint session, jbyteArray text, jint to, jint at, jint state) {
    (void)cls; char buf[DM_BUILD_CARD_PARTS * DM_BUILD_CARD_TEXT]; jsize n = text ? (*env)->GetArrayLength(env, text) : 0; if (n > (jsize)sizeof buf) n = (jsize)sizeof buf;
    if (n > 0) (*env)->GetByteArrayRegion(env, text, 0, n, (jbyte *)buf);
    dm_build_card(session, buf, n, to, at, state);
}
JNIEXPORT void JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeBuildStat(JNIEnv *env, jclass cls, jint session, jint stat, jint show) { (void)env; (void)cls; dm_build_stat(session, stat, show); }
JNIEXPORT void JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeBuildKeys(JNIEnv *env, jclass cls, jint session, jintArray ids) {
    (void)cls; int buf[DM_BUILD_KEYS]; jsize n = ids ? (*env)->GetArrayLength(env, ids) : 0; if (n > DM_BUILD_KEYS) n = DM_BUILD_KEYS;
    if (n > 0) (*env)->GetIntArrayRegion(env, ids, 0, n, buf); dm_build_keys(session, buf, n);
}
JNIEXPORT jint JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeBuildHints(JNIEnv *env, jclass cls) { (void)env; (void)cls; return dm_build_hints(); }



#include "../host/dm_gear_live.h"
JNIEXPORT jint JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeTipDots(JNIEnv *env, jclass cls, jfloatArray out) {
    (void)cls; float box[DM_GEAR_DOTS_BOX] = { 0 }; int dots = dm_gear_dots_get(dm_guest_frame(), box);
    if (!out || (*env)->GetArrayLength(env, out) < DM_GEAR_DOTS_BOX) return 0;
    (*env)->SetFloatArrayRegion(env, out, 0, DM_GEAR_DOTS_BOX, box);
    return dots;
}
JNIEXPORT void JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeGearGoal(JNIEnv *env, jclass cls, jint session, jint hits, jint boss) { (void)env; (void)cls; dm_gear_goal_set(session, hits, boss); }


#include "../host/dm_gear_tab.h"
JNIEXPORT jbyteArray JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeBestGear(JNIEnv *env, jclass cls, jint on) {
    (void)cls; dm_gear_tab_want(on);
    if (!on) return NULL;
    size_t cap = 64 * 1024; char *out = malloc(cap); if (!out) return NULL;
    int rc = dm_gear_tab_text(out, cap); jbyteArray result = NULL;
    if (rc >= 0) { result = (*env)->NewByteArray(env, rc); if (result) (*env)->SetByteArrayRegion(env, result, 0, rc, (jbyte *)out); }
    free(out); return result;
}

JNIEXPORT void JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeBestGearCharms(JNIEnv *env, jclass cls, jint search) { (void)env; (void)cls; dm_gear_tab_charms(search); }


JNIEXPORT void JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeBestGearTip(JNIEnv *env, jclass cls, jint id, jint place) { (void)env; (void)cls; dm_gear_tab_tip_ask((uint32_t)id, place); }



#include "../host/dm_gear_equip_tab.h"
JNIEXPORT jint JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeBestGearEquip(JNIEnv *env, jclass cls, jstring key, jint row) {
    (void)cls; if (!key) return -1;
    const char *k = (*env)->GetStringUTFChars(env, key, NULL); if (!k) return -1;
    int rc = dm_gear_equip_tab_tap(k, row);
    (*env)->ReleaseStringUTFChars(env, key, k);
    return rc;
}


#include "../host/dm_gear_pages.h"
JNIEXPORT jint JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeBestGearStockAsk(JNIEnv *env, jclass cls) { (void)env; (void)cls; return (jint)dm_gear_pages_ask(); }
JNIEXPORT jint JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeBestGearStockAnswered(JNIEnv *env, jclass cls) { (void)env; (void)cls; return (jint)dm_gear_pages_answered(); }



JNIEXPORT void JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativePadMode(JNIEnv *env, jclass cls, jint pad) {
    (void)env; (void)cls;
    dm_padmode_request(pad ? 1 : 0);
}



JNIEXPORT void JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativePadGlyph(JNIEnv *env, jclass cls, jint glyph) {
    (void)env; (void)cls;
    dm_padmode_request_glyph(glyph);
}



JNIEXPORT void JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeFocus(JNIEnv *env, jclass cls, jint ev) {
    (void)env; (void)cls;
    dm_android_touch_focus(ev == 2 || ev == 3);
    dm_fps_reset();
    if (ev == 1) { void front_android_res_live_stop(void); front_android_res_live_stop(); }
    if (ev == 3) { void dm_resume_app_back(void); dm_resume_app_back(); }
    ka_posted(ev);
    if (lifecycle_ready()) {
        dm_lifecycle_post(&g_lifecycle, ev);
        __android_log_print(ANDROID_LOG_INFO, "dm-lifecycle", "UI posted event=%d", ev);
    }
}

#include "dm_sim_core.h"
JNIEXPORT jint JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeSimEval(JNIEnv *env, jclass cls, jbyteArray profile, jbyteArray out) {
    (void)cls;
    if (!profile || !out) return -1;
    jsize n = (*env)->GetArrayLength(env, profile), cap = (*env)->GetArrayLength(env, out);
    if (n < 1 || n > 65536 || cap < 1 || cap > 1048576) return -1;
    char *input = malloc((size_t)n + 1), *output = malloc((size_t)cap);
    if (!input || !output) { free(input); free(output); return -1; }
    (*env)->GetByteArrayRegion(env, profile, 0, n, (jbyte *)input);
    if ((*env)->ExceptionCheck(env)) { free(input); free(output); return -1; }
    input[n] = 0;
    int rc = memchr(input, 0, (size_t)n) ? -1 : dm_sim_eval_text(input, output, (size_t)cap);
    if (rc >= 0) (*env)->SetByteArrayRegion(env, out, 0, rc, (const jbyte *)output);
    free(input); free(output);
    return rc;
}

#include "dm_sim_ui.h"
JNIEXPORT jbyteArray JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeCombat(JNIEnv *env,jclass cls,jintArray options) {
    (void)cls;
    if(!options)return NULL;
    jsize n=(*env)->GetArrayLength(env,options);if(n<8 || n>8+DM_SIM_MAXBAR)return NULL;
    jint v[8+DM_SIM_MAXBAR];(*env)->GetIntArrayRegion(env,options,0,n,v);
    if((*env)->ExceptionCheck(env))return NULL;
    dm_sim_options opt={.diff=v[0],.players=v[1],.potions=v[2],.terror=v[3],.herald=v[4],.target=v[5],.ordered=v[6],.norder=n-8};
    for(int i=0;i<opt.norder;i++)opt.order[i]=v[8+i];
    size_t cap=2*1024*1024;char *out=malloc(cap);if(!out)return NULL;
    int rc=dm_sim_ui_query(&opt,out,cap);jbyteArray result=NULL;
    if(rc>=0){result=(*env)->NewByteArray(env,rc);if(result)(*env)->SetByteArrayRegion(env,result,0,rc,(jbyte *)out);}
    free(out);return result;
}


#include "dm_fightlog_text.h"
JNIEXPORT jbyteArray JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeFightLog(JNIEnv *env,jclass cls,jint fight,jint group,jint key,jint sim) {
    (void)cls;
    size_t cap=512*1024;char *out=malloc(cap);if(!out)return NULL;
    int rc=dm_fl_ui_query(fight,group,(uint32_t)key,sim,out,cap);jbyteArray result=NULL;
    if(rc>=0){result=(*env)->NewByteArray(env,rc);if(result)(*env)->SetByteArrayRegion(env,result,0,rc,(jbyte *)out);}
    free(out);return result;
}

JNIEXPORT jbyteArray JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeStreaks(JNIEnv *env,jclass cls) {
    (void)cls;
    char out[4096];int rc=dm_streak_text(out,sizeof out);jbyteArray result=NULL;
    if(rc>=0){result=(*env)->NewByteArray(env,rc);if(result)(*env)->SetByteArrayRegion(env,result,0,rc,(jbyte *)out);}
    return result;
}
