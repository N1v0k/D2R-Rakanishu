








#include "../../host/dm.h"
#include "../../host/front.h"
#include "../../host/hle.h"
#include "../../host/dm_actions.h"
#include "../../android/vulkan_present.h"
#include "touch_ohos.h"
#include "../../host/dm_i18n.h"
#include "../../host/dm_custom.h"
#include "../../android/lifecycle_dispatch.h"
#include "../../android/overlay_validate.h"
#include "dm_overlay_contract.h"
#include <ace/xcomponent/native_interface_xcomponent.h>
#include <hilog/log.h>
#include <napi/native_api.h>
#include <dirent.h>
#include <errno.h>
#include <stdatomic.h>
#include <sys/stat.h>
#include <unistd.h>

void front_android_set_window(void *win, int w, int h);
void front_android_set_refresh(float hz);
int dm_main(int argc, char **argv);
extern int g_swkbd_name_panel, g_swkbd_numeric_max;
extern _Atomic int g_fe_name_no_accept;

#define HLOG(lvl, tag, ...) do { char b_[1024]; snprintf(b_, sizeof b_, __VA_ARGS__); OH_LOG_Print(LOG_APP, lvl, 0xD2E0, tag, "%{public}s", b_); } while (0)

static char g_files[1024], g_ext[1024], g_src[1024], g_runtime[1100];
static FILE *g_logf;


static dm_lifecycle_dispatch g_lifecycle;
static pthread_once_t g_lifecycle_once = PTHREAD_ONCE_INIT;
static int g_lifecycle_error;
static void lifecycle_graphics(int on) { dm_gfx_suspend(on); HLOG(LOG_INFO, "dm-lifecycle", "graphics suspend=%d", on); }
static void lifecycle_init(void) {
    dm_lifecycle_ops ops = {dm_focus_set, dm_audio_suspend, lifecycle_graphics};
    g_lifecycle_error = dm_lifecycle_start(&g_lifecycle, ops);
    if (!g_lifecycle_error) pthread_setname_np(g_lifecycle.thread, "dm-lifecycle");
}
static int lifecycle_ready(void) { pthread_once(&g_lifecycle_once, lifecycle_init); return !g_lifecycle_error; }


static void *log_pump(void *arg) {
    int fd = (int)(intptr_t)arg; char buf[4096]; size_t have = 0; ssize_t k;
    while ((k = read(fd, buf + have, sizeof buf - 1 - have)) > 0) {
        have += (size_t)k; buf[have] = 0; char *s = buf, *nl;
        while ((nl = strchr(s, '\n'))) { *nl = 0; OH_LOG_Print(LOG_APP, LOG_INFO, 0xD2E0, "dm", "%{public}s", s); if (g_logf) { fputs(s, g_logf); fputc('\n', g_logf); fflush(g_logf); } s = nl + 1; }
        have = strlen(s); memmove(buf, s, have);
        if (have == sizeof buf - 1) { OH_LOG_Print(LOG_APP, LOG_INFO, 0xD2E0, "dm", "%{public}s", buf); if (g_logf) { fputs(buf, g_logf); fflush(g_logf); } have = 0; }
    }
    return NULL;
}
static void log_init(void) {
    static int initialized; if (initialized) return; initialized = 1;
    char p[1200]; snprintf(p, sizeof p, "%s/boot.log", g_runtime); g_logf = fopen(p, "a");
    int fds[2]; if (pipe(fds)) return;
    dup2(fds[1], 2); close(fds[1]); setvbuf(stderr, NULL, _IOLBF, 0);
    pthread_t th; if (!pthread_create(&th, NULL, log_pump, (void *)(intptr_t)fds[0])) pthread_detach(th);
}
static void mkdirs(const char *p) {
    char b[1100]; snprintf(b, sizeof b, "%s", p);
    for (char *s = b + 1; *s; s++) if (*s == '/') { *s = 0; mkdir(b, 0700); *s = '/'; }
    mkdir(b, 0700);
}


static int link_romfs(const char *romfs) {
    char data[1100]; snprintf(data, sizeof data, "%s/data", romfs); mkdirs(data);
    DIR *d = opendir(g_src); if (!d) { LOG("【红】资源目录打不开 %s：errno %d", g_src, errno); return -1; }
    struct dirent *e; int n = 0;
    while ((e = readdir(d))) {
        const char *nm = e->d_name; size_t l = strlen(nm);
        if (!(l > 5 && !strcmp(nm + l - 5, ".data")) && !(l > 11 && !strcmp(nm + l - 11, ".data.holes")) && strcmp(nm, ".build.config") && strcmp(nm, "build.config")) continue;
        char from[1100], to[1100]; snprintf(from, sizeof from, "%s/%s", g_src, nm);
        snprintf(to, sizeof to, "%s/%s", data, strcmp(nm, "build.config") ? nm : ".build.config");

        unlink(to);
        if (!symlink(from, to) || !link(from, to)) n++;
        else if (!n) LOG("【红】链接 %s 失败：errno %d", nm, errno);
    }
    closedir(d);
    return n;
}


static napi_threadsafe_function g_show_input, g_boot_failed;
typedef struct { int64_t id; char *s[4]; int max_chars, name_rule, numeric_max; } input_req;
static pthread_mutex_t input_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t input_done = PTHREAD_COND_INITIALIZER;
static int64_t next_request, active_request;
static int pending, result, output_capacity;
static char *output;

static void call_show_input(napi_env env, napi_value js, void *ctx, void *data) {
    (void)ctx; input_req *r = data; if (!env) goto out;
    napi_value argv[8]; napi_create_int64(env, r->id, &argv[0]);
    for (int i = 0; i < 4; i++) napi_create_string_utf8(env, r->s[i] ? r->s[i] : "", NAPI_AUTO_LENGTH, &argv[1 + i]);
    napi_create_int32(env, r->max_chars, &argv[5]); napi_create_int32(env, r->name_rule, &argv[6]); napi_create_int32(env, r->numeric_max, &argv[7]);
    napi_value undef; napi_get_undefined(env, &undef); napi_call_function(env, undef, js, 8, argv, NULL);
out:
    for (int i = 0; i < 4; i++) free(r->s[i]);
    free(r);
}
static void call_boot_failed(napi_env env, napi_value js, void *ctx, void *data) {
    (void)ctx; if (!env) return;
    napi_value code, undef; napi_create_int32(env, (int)(intptr_t)data, &code); napi_get_undefined(env, &undef);
    napi_call_function(env, undef, js, 1, &code, NULL);
}
void front_android_boot_failed(int code) {
    if (g_boot_failed) napi_call_threadsafe_function(g_boot_failed, (void *)(intptr_t)code, napi_tsfn_nonblocking);
}
int front_android_text_input(const char *title, const char *sub, const char *guide, const char *initial, int max_chars, char *out, int out_cap) {
    if (!out || out_cap <= 0 || !g_show_input) return -1;
    input_req *r = calloc(1, sizeof *r); if (!r) return -1;
    const char *src[] = {title, sub, guide, initial};
    for (int i = 0; i < 4; i++) r->s[i] = strdup(src[i] ? src[i] : "");
    r->max_chars = max_chars; r->name_rule = g_swkbd_name_panel; r->numeric_max = g_swkbd_numeric_max;
    int name_rule = r->name_rule;
    pthread_mutex_lock(&input_lock);
    if (active_request) { pthread_mutex_unlock(&input_lock); for (int i = 0; i < 4; i++) free(r->s[i]); free(r); return -1; }
    r->id = active_request = ++next_request;
    pending = 1; result = 0; output = out; output_capacity = out_cap; out[0] = 0;
    pthread_mutex_unlock(&input_lock);
    if (napi_call_threadsafe_function(g_show_input, r, napi_tsfn_blocking) != napi_ok) {
        pthread_mutex_lock(&input_lock); pending = 0; active_request = 0; pthread_mutex_unlock(&input_lock); return -1;
    }
    pthread_mutex_lock(&input_lock);
    while (pending) pthread_cond_wait(&input_done, &input_lock);
    int accepted = result; output = NULL; active_request = 0;
    pthread_mutex_unlock(&input_lock);

    if (accepted && name_rule && !out[0]) { accepted = dm_name_gen(out, out_cap, dm_name_taken); if (accepted) atomic_store(&g_fe_name_no_accept, 1); }
    return accepted;
}

static int get_str(napi_env env, napi_value v, char *buf, size_t cap) { size_t n = 0; return napi_get_value_string_utf8(env, v, buf, cap, &n) == napi_ok ? (int)n : -1; }
static napi_value ret_int(napi_env env, int v) { napi_value r; napi_create_int32(env, v, &r); return r; }
static napi_value undefined(napi_env env) { napi_value r; napi_get_undefined(env, &r); return r; }
#define ARGS(n) size_t argc = n; napi_value argv[n]; napi_get_cb_info(env, info, &argc, argv, NULL, NULL)

static napi_value js_set_callbacks(napi_env env, napi_callback_info info) {
    ARGS(2); napi_value name;
    napi_create_string_utf8(env, "dmShowInput", NAPI_AUTO_LENGTH, &name);
    napi_create_threadsafe_function(env, argv[0], NULL, name, 0, 1, NULL, NULL, NULL, call_show_input, &g_show_input);
    napi_create_string_utf8(env, "dmBootFailed", NAPI_AUTO_LENGTH, &name);
    napi_create_threadsafe_function(env, argv[1], NULL, name, 0, 1, NULL, NULL, NULL, call_boot_failed, &g_boot_failed);
    napi_unref_threadsafe_function(env, g_show_input); napi_unref_threadsafe_function(env, g_boot_failed);
    return undefined(env);
}
static napi_value js_text_result(napi_env env, napi_callback_info info) {
    ARGS(3); int64_t id = 0; bool accepted = false; napi_get_value_int64(env, argv[0], &id); napi_get_value_bool(env, argv[1], &accepted);
    pthread_mutex_lock(&input_lock);
    if (pending && active_request == id) {
        result = accepted && get_str(env, argv[2], output, (size_t)output_capacity) >= 0;
        pending = 0; pthread_cond_signal(&input_done);
        HLOG(LOG_INFO, "dm-input", "completed id=%lld accepted=%d", (long long)id, result);
    }
    pthread_mutex_unlock(&input_lock);
    return undefined(env);
}

static void *host_thread(void *arg) {
    (void)arg;
    char out[1200]; snprintf(out, sizeof out, "%s/out", g_runtime);
    char shaders[1200], error[512] = {0}; snprintf(shaders, sizeof shaders, "%s/shaders", g_runtime);
    int graphics_rc = dm_android_vk_initialize(shaders, error, sizeof error);
    if (graphics_rc) { LOG("boot: Vulkan initialization failed %d: %s", graphics_rc, error); front_android_boot_failed(-6); return NULL; }
    char *argv[] = {"dmhost", out, "main", "sdk", NULL};
    LOG("boot: dm_main %s", out);
    int rc = dm_main(4, argv);
    LOG("boot: dm_main 返回 %d", rc);
    front_android_boot_failed(-7);
    return NULL;
}





static int pack_info_ok(char *why, size_t n) {
    char p[1200]; snprintf(p, sizeof p, "%s/pack_info.txt", g_runtime);
    FILE *f = fopen(p, "rb"); if (!f) return 1;
    char line[256], product[128] = "", target[32] = "";
    while (fgets(line, sizeof line, f)) {
        line[strcspn(line, "\r\n")] = 0;
        if (!strncmp(line, "PRODUCT=", 8)) snprintf(product, sizeof product, "%s", line + 8);
        else if (!strncmp(line, "TARGET=", 7)) snprintf(target, sizeof target, "%s", line + 7);
    }
    fclose(f);
    if (strcmp(target, "ohos")) { snprintf(why, n, "TARGET=%s, want ohos", target); return 0; }
    if (strcmp(product, DM_PRODUCT)) { snprintf(why, n, "PRODUCT=%s, want %s", product, DM_PRODUCT); return 0; }
    return 1;
}


__attribute__((weak)) int dm_derived_reload(void);



static napi_value js_start(napi_env env, napi_callback_info info) {
    ARGS(4); static int started; if (started) return ret_int(env, 0);
    char lg[64] = {0};
    get_str(env, argv[0], g_files, sizeof g_files); get_str(env, argv[1], g_ext, sizeof g_ext); get_str(env, argv[2], g_src, sizeof g_src); get_str(env, argv[3], lg, sizeof lg);


    dm_custom_init(g_files);
    snprintf(g_runtime, sizeof g_runtime, "%s/dm", g_files); mkdirs(g_runtime);
    log_init();
    LOG("boot: 鸿蒙 内部目录 %s，资源 %s，系统语言 %s", g_files, g_src, lg);
    {
        char line[512] = {0}; int at = 0;
        for (int c = 0; c < 16; c++) { char p[96]; snprintf(p, sizeof p, "/sys/devices/system/cpu/cpu%d/cpufreq/cpuinfo_max_freq", c); FILE *f = fopen(p, "r"); if (!f) continue;
            long khz = 0; if (fscanf(f, "%ld", &khz) == 1) at += snprintf(line + at, sizeof line - at, " %d:%ld", c, khz / 1000); fclose(f); }
        LOG("boot: CPU 各核最高主频 MHz%s", at ? line : "（读不到）");
    }
    char romfs[1100], save[1100], temp[1100];
    snprintf(romfs, sizeof romfs, "%s/romfs", g_files); snprintf(save, sizeof save, "%s/save", g_files); snprintf(temp, sizeof temp, "%s/temp", g_files);
    mkdirs(save); mkdirs(temp);





    char data_dir[1200]; snprintf(data_dir, sizeof data_dir, "%s/data", romfs);
    struct stat st_data;
    rmdir(data_dir);
    if (stat(data_dir, &st_data) &&!strncmp(g_src, g_files, strlen(g_files)) && !stat(g_src, &st_data) && S_ISDIR(st_data.st_mode)) {
        mkdirs(romfs);
        if (!rename(g_src, data_dir)) {
            char rec[1300]; snprintf(rec, sizeof rec, "%s/data-moved.txt", g_runtime);
            FILE *f = fopen(rec, "w"); if (f) { fprintf(f, "%s\n%s\n", g_src, data_dir); fclose(f); }
            LOG("boot: 资源目录 %s 改名为 %s（不复制）", g_src, data_dir);
        } else LOG("【红】资源目录改名失败 errno %d", errno);
    }
    int n = 0;
    if (!stat(data_dir, &st_data)) {
        DIR *dd = opendir(data_dir); struct dirent *e;
        while (dd && (e = readdir(dd))) { size_t l = strlen(e->d_name); if (l > 5 && !strcmp(e->d_name + l - 5, ".data")) n++; }
        if (dd) closedir(dd);

        char a[1300], b[1300]; snprintf(a, sizeof a, "%s/build.config", data_dir); snprintf(b, sizeof b, "%s/.build.config", data_dir);
        if (access(b, R_OK) && !access(a, R_OK)) rename(a, b);
    }
    if (n <= 0) n = link_romfs(romfs);
    LOG("boot: 资源链接 %d 个文件", n);
    setenv("DM_ROMFS", romfs, 0); setenv("DM_SAVE", save, 0); setenv("DM_TEMP", temp, 0);

    { char ef[1200], ln[512]; snprintf(ef, sizeof ef, "%s/env", g_runtime); FILE *f = fopen(ef, "rb");
      while (f && fgets(ln, sizeof ln, f)) { ln[strcspn(ln, "\r\n")] = 0; char *eq = strchr(ln, '='); if (ln[0] == '#' || !eq || eq == ln) continue; *eq = 0; setenv(ln, eq + 1, 1); LOG("boot: env %s=%s", ln, eq + 1); }
      if (f) fclose(f); }


    setenv("DM_3D_SCALE_GPU", "1", 0);
    setenv("DM_3D_SCALE_DYN", "1", 0);
    setenv("DM_NVN_SYNC", "0", 0);
    setenv("DM_GFX_DEFER_EVENTS", "2", 0);
    setenv("DM_GFX_READ_PREFLIGHT", "0", 0);
    setenv("DM_GFX_READBACK_CACHED", "1", 0);
    setenv("DM_GFX_PIPE_MEMO", "1", 0); setenv("DM_GFX_QUAD_DERIVATIVES", "1", 0); setenv("DM_GFX_WARP32", "1", 0); setenv("DM_GFX_NAN_MINMAX", "1", 0);
    setenv("DM_GFX_NATIVE_BC", "0", 0);
    setenv("DM_GFX_ASYNC_POOL_RESET", "0", 0); setenv("DM_GFX_WB_SKIP", "0", 0);
    setenv("DM_GFX_NOLOAD", "1", 0);
    setenv("DM_PACE_ADAPT", "1", 0);


    { char mk[1200]; snprintf(mk, sizeof mk, "%s/nvn-async", g_runtime); FILE *f = fopen(mk, "rb"); if (f) { setenv("DM_NVN_SYNC", fgetc(f) == '0' ? "1" : "0", 1); fclose(f); } }
    if (!getenv("DM_LANG") && *lg) setenv("DM_LANG", lg, 0);
    if (chdir(g_files)) LOG("【红】chdir %s 失败 errno %d", g_files, errno);
    if (n <= 0) return ret_int(env, -2);
    char required[1300];
    const char *modules[] = {"main.patched.bin", "main.fixups.tsv", "main.gates.tsv", "sdk.patched.bin", "sdk.fixups.tsv", "sdk.gates.tsv"};
    for (unsigned i = 0; i < sizeof modules / sizeof modules[0]; i++) {
        snprintf(required, sizeof required, "%s/out/%s", g_runtime, modules[i]);
        if (access(required, R_OK)) { LOG("boot: missing %s", required); return ret_int(env, -3); }
    }
    snprintf(required, sizeof required, "%s/data/.build.config", romfs);
    if (access(required, R_OK)) { LOG("boot: missing build configuration"); return ret_int(env, -5); }
    snprintf(required, sizeof required, "%s/shaders", g_runtime);
    DIR *sh = opendir(required); int have_shader = 0;
    if (sh) { struct dirent *e; while ((e = readdir(sh))) { size_t len = strlen(e->d_name); if (len > 4 && !strcmp(e->d_name + len - 4, ".xgs")) { have_shader = 1; break; } } closedir(sh); }
    if (!have_shader) { LOG("boot: no shader bundles in %s", required); return ret_int(env, -4); }


    { char why[256]; if (!pack_info_ok(why, sizeof why)) { LOG("boot: import pack mismatch: %s", why); return ret_int(env, -11); } }
    snprintf(required, sizeof required, "%s/derived.tsv", g_runtime);
    setenv("DM_DERIVED", required, 1);
    if (dm_derived_reload && dm_derived_reload()) { LOG("boot: derived tables incomplete (%s)", required); return ret_int(env, -12); }
    snprintf(required, sizeof required, "%s/overlay", g_runtime);
    char reason[256];
    dm_overlay_result ov = dm_overlay_validate(required, DM_OVERLAY_SOURCE_SHA256, reason, sizeof reason);
    if (ov == DM_OVERLAY_OK) ov = dm_overlay_require_touch_art(required, reason, sizeof reason);
    if (ov != DM_OVERLAY_OK) { LOG("boot: overlay rejected (%d): %s", ov, reason); return ret_int(env, ov == DM_OVERLAY_MISSING ? -9 : -10); }
    setenv("DM_ROMFS_OVERLAY", required, 1);
    if (!lifecycle_ready()) return ret_int(env, -8);
    pthread_attr_t a; pthread_attr_init(&a); pthread_attr_setstacksize(&a, 8u << 20);
    pthread_t th; int rc = pthread_create(&th, &a, host_thread, NULL); pthread_attr_destroy(&a);
    if (!rc) { started = 1; pthread_detach(th); }
    return ret_int(env, rc ? -1 : n);
}


static napi_value js_focus(napi_env env, napi_callback_info info) {
    ARGS(1); int32_t ev = 0; napi_get_value_int32(env, argv[0], &ev);
    dm_ohos_touch_focus(ev == 2 || ev == 3);
    if (lifecycle_ready()) dm_lifecycle_post(&g_lifecycle, ev);
    return undefined(env);
}
static napi_value js_refresh(napi_env env, napi_callback_info info) { ARGS(1); double hz = 0; napi_get_value_double(env, argv[0], &hz); front_android_set_refresh((float)hz); return undefined(env); }

static napi_value js_touch_event(napi_env env, napi_callback_info info) {
    ARGS(4); int32_t phase = 0, id = 0; double x = 0, y = 0;
    napi_get_value_int32(env, argv[0], &phase); napi_get_value_int32(env, argv[1], &id); napi_get_value_double(env, argv[2], &x); napi_get_value_double(env, argv[3], &y);
    dm_ohos_touch_event(phase, id, (float)x, (float)y); return undefined(env);
}

static napi_value js_touch_screen(napi_env env, napi_callback_info info) {
    ARGS(5); double v[5] = {0};
    for (int i = 0; i < 5; i++) napi_get_value_double(env, argv[i], &v[i]);
    dm_ohos_touch_screen((float)v[0], (float)v[1], (float)v[2], (float)v[3], (float)v[4]); return undefined(env);
}

static napi_value js_touch_tick(napi_env env, napi_callback_info info) {
    ARGS(1); napi_typedarray_type t; size_t len = 0; void *data = NULL; napi_value buf; size_t off = 0;
    if (napi_get_typedarray_info(env, argv[0], &t, &len, &data, &buf, &off) != napi_ok || t != napi_float32_array) return ret_int(env, -1);
    return ret_int(env, dm_ohos_touch_tick((float *)data, (int)len));
}

static napi_value js_ui_text(napi_env env, napi_callback_info info) {
    ARGS(1); char k[96] = {0}; get_str(env, argv[0], k, sizeof k);
    napi_value r; napi_create_string_utf8(env, dm_ui(k), NAPI_AUTO_LENGTH, &r); return r;
}




#include <sys/mman.h>
static napi_value js_exec_probe(napi_env env, napi_callback_info info) {
    (void)info; char r[96];
    uint32_t *pg = mmap(NULL, 0x4000, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (pg == MAP_FAILED) snprintf(r, sizeof r, "mmap errno %d", errno);
    else {
        pg[0] = 0x52800540; pg[1] = 0xd65f03c0;
        __builtin___clear_cache((char *)pg, (char *)(pg + 2));
        if (mprotect(pg, 0x4000, PROT_READ | PROT_EXEC)) snprintf(r, sizeof r, "mprotect errno %d", errno);
        else { int v = ((int (*)(void))pg)(); snprintf(r, sizeof r, v == 42 ? "ok" : "bad result %d", v); }
        munmap(pg, 0x4000);
    }
    LOG("boot: 可执行内存自检 %s", r);
    napi_value out; napi_create_string_utf8(env, r, NAPI_AUTO_LENGTH, &out); return out;
}

uint64_t mtl_present_count(void);
static napi_value js_present_count(napi_env env, napi_callback_info info) {
    (void)info; napi_value r; napi_create_double(env, (double)mtl_present_count(), &r); return r;
}

static void surface_set(OH_NativeXComponent *c, void *window) {
    uint64_t w = 0, h = 0;
    if (window) OH_NativeXComponent_GetXComponentSize(c, window, &w, &h);
    dm_android_vk_set_window((ANativeWindow *)window);
    front_android_set_window(window, (int)w, (int)h);
    LOG("前端：画面 %s %llux%llu", window ? "就绪" : "收回", (unsigned long long)w, (unsigned long long)h);
}
static void on_created(OH_NativeXComponent *c, void *window) { surface_set(c, window); }
static void on_changed(OH_NativeXComponent *c, void *window) { surface_set(c, window); }
static void on_destroyed(OH_NativeXComponent *c, void *window) { (void)window; surface_set(c, NULL); }
static void on_touch(OH_NativeXComponent *c, void *window) { (void)c; (void)window; }
static OH_NativeXComponent_Callback g_xcb = {on_created, on_changed, on_destroyed, on_touch};

static napi_value init(napi_env env, napi_value exports) {
    napi_property_descriptor d[] = {
        {"start", NULL, js_start, NULL, NULL, NULL, napi_default, NULL},
        {"focus", NULL, js_focus, NULL, NULL, NULL, napi_default, NULL},
        {"refresh", NULL, js_refresh, NULL, NULL, NULL, napi_default, NULL},
        {"touchEvent", NULL, js_touch_event, NULL, NULL, NULL, napi_default, NULL},
        {"touchScreen", NULL, js_touch_screen, NULL, NULL, NULL, napi_default, NULL},
        {"touchTick", NULL, js_touch_tick, NULL, NULL, NULL, napi_default, NULL},
        {"uiText", NULL, js_ui_text, NULL, NULL, NULL, napi_default, NULL},
        {"execProbe", NULL, js_exec_probe, NULL, NULL, NULL, napi_default, NULL},
        {"presentCount", NULL, js_present_count, NULL, NULL, NULL, napi_default, NULL},
        {"setCallbacks", NULL, js_set_callbacks, NULL, NULL, NULL, napi_default, NULL},
        {"textResult", NULL, js_text_result, NULL, NULL, NULL, napi_default, NULL},
    };
    napi_define_properties(env, exports, sizeof d / sizeof d[0], d);

    napi_value xobj; OH_NativeXComponent *xc = NULL;
    if (napi_get_named_property(env, exports, OH_NATIVE_XCOMPONENT_OBJ, &xobj) == napi_ok && napi_unwrap(env, xobj, (void **)&xc) == napi_ok && xc)
        OH_NativeXComponent_RegisterCallback(xc, &g_xcb);
    return exports;
}
static napi_module g_module = {1, 0, NULL, init, "dm", NULL, {0}};
__attribute__((constructor)) static void register_module(void) { napi_module_register(&g_module); }
