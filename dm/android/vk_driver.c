












#include "vk_driver.h"
#include "vk_loader.h"
#include <dlfcn.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

enum { MODE_SYSTEM, MODE_CUSTOM, MODE_FALLBACK };
static struct { char dir[600], lib[80], hooks[600], tu_debug[64], attempt[40], state[600], label[100]; int test_fail; } g_cfg;
static int g_mode = MODE_SYSTEM;
static char g_reason[160];
static pthread_mutex_t g_state_mu = PTHREAD_MUTEX_INITIALIZER;
static atomic_uint g_frames;
static atomic_int g_ok;


int dm_vk_driver_tu_debug_ok(const char *v) {
    static const char *const allowed[] = {"sysmem", "gmem", "nolrz", "noubwc", "flushall", "syncdraw"};
    if (!v || !*v || strlen(v) > 60) return 0;
    char copy[64]; snprintf(copy, sizeof copy, "%s", v);
    for (char *save = NULL, *t = strtok_r(copy, ",", &save); t; t = strtok_r(NULL, ",", &save)) {
        int hit = 0;
        for (size_t i = 0; i < sizeof allowed / sizeof *allowed; i++) if (!strcmp(t, allowed[i])) hit = 1;
        if (!hit) return 0;
    }
    return v[strlen(v) - 1] != ',' && v[0] != ',' && !strstr(v, ",,");
}

void dm_vk_driver_config(const char *dir, const char *lib, const char *hooks, const char *tu_debug, const char *attempt, const char *state_dir, const char *label, int test_fail) {
    memset(&g_cfg, 0, sizeof g_cfg);
    if (!dir || !*dir || !lib || !*lib || !hooks || !*hooks || !state_dir || !*state_dir) return;
    snprintf(g_cfg.dir, sizeof g_cfg.dir, "%s%s", dir, dir[strlen(dir) - 1] == '/' ? "" : "/");
    snprintf(g_cfg.lib, sizeof g_cfg.lib, "%s", lib);
    snprintf(g_cfg.hooks, sizeof g_cfg.hooks, "%s", hooks);
    if (tu_debug && dm_vk_driver_tu_debug_ok(tu_debug)) snprintf(g_cfg.tu_debug, sizeof g_cfg.tu_debug, "%s", tu_debug);
    snprintf(g_cfg.attempt, sizeof g_cfg.attempt, "%s", attempt ? attempt : "");
    snprintf(g_cfg.state, sizeof g_cfg.state, "%s", state_dir);
    snprintf(g_cfg.label, sizeof g_cfg.label, "%s", label ? label : "");
#ifdef DM_DRIVER_TESTS
    g_cfg.test_fail = test_fail;
#else
    (void)test_fail;
#endif
}

static int configured(void) { return g_cfg.dir[0] != 0; }


static void put_file(const char *name, const char *text) {
    if (!g_cfg.state[0]) return;
    char path[700], tmp[720]; snprintf(path, sizeof path, "%s/%s", g_cfg.state, name); snprintf(tmp, sizeof tmp, "%s.tmp", path);
    int fd = open(tmp, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0600); if (fd < 0) { fprintf(stderr, "vk driver: cannot write %s\n", tmp); return; }
    size_t n = strlen(text); ssize_t w = write(fd, text, n); fsync(fd); close(fd);
    if (w != (ssize_t)n || rename(tmp, path)) { unlink(tmp); fprintf(stderr, "vk driver: cannot write %s\n", path); return; }
    int d = open(g_cfg.state, O_RDONLY | O_DIRECTORY | O_CLOEXEC); if (d >= 0) { fsync(d); close(d); }
}

int dm_vk_driver_fall_back(const char *reason) {
    if (g_mode != MODE_CUSTOM) return 0;
    char error[160];
    unsetenv("TU_DEBUG");
    if (dm_vk_loader_use_system(error, sizeof error)) { fprintf(stderr, "vk driver: system loader unavailable after fallback: %s\n", error); abort(); }
    pthread_mutex_lock(&g_state_mu);
    g_mode = MODE_FALLBACK;
    snprintf(g_reason, sizeof g_reason, "%s", reason ? reason : "unknown");
    for (char *c = g_reason; *c; c++) if (*c == '\n' || *c == '\r') *c = ' ';
    pthread_mutex_unlock(&g_state_mu);
    char text[256]; snprintf(text, sizeof text, "%s\n%s\n", g_cfg.attempt, g_reason);
    put_file("attempt.fallback", text);
    fprintf(stderr, "vk driver: own driver %s not used (%s), falling back to the system driver in this process\n", g_cfg.lib, g_reason);
    return 1;
}

int dm_vk_driver_open(void) {
    char error[200];
    pthread_mutex_lock(&g_state_mu); g_mode = MODE_SYSTEM; g_reason[0] = 0; pthread_mutex_unlock(&g_state_mu);
    atomic_store(&g_frames, 0); atomic_store(&g_ok, 0);
    if (!configured()) {
        if (dm_vk_loader_use_system(error, sizeof error)) fprintf(stderr, "vk driver: %s\n", error);
        return 0;
    }
    pthread_mutex_lock(&g_state_mu); g_mode = MODE_CUSTOM; pthread_mutex_unlock(&g_state_mu);
    fprintf(stderr, "vk driver: own driver %s%s attempt %s%s%s\n", g_cfg.dir, g_cfg.lib, g_cfg.attempt, g_cfg.tu_debug[0] ? " TU_DEBUG=" : "", g_cfg.tu_debug);
#ifdef DM_DRIVER_TESTS
    if (g_cfg.test_fail == 1) { dm_vk_driver_fall_back("open failed (test switch)"); return 0; }
#endif
    void *h = dlopen("libdm_driverload.so", RTLD_NOW | RTLD_LOCAL);
    void *(*open_fn)(const char *, const char *, const char *) = h ? (void *(*)(const char *, const char *, const char *))dlsym(h, "dm_driverload_open") : NULL;
    if (!open_fn) { snprintf(error, sizeof error, "libdm_driverload.so: %s", h ? "no dm_driverload_open" : dlerror()); dm_vk_driver_fall_back(error); return 0; }
    if (g_cfg.tu_debug[0]) setenv("TU_DEBUG", g_cfg.tu_debug, 1); else unsetenv("TU_DEBUG");
    void *vk = open_fn(g_cfg.hooks, g_cfg.dir, g_cfg.lib);
    if (!vk) { dm_vk_driver_fall_back("adrenotools could not open the loader"); return 0; }
    char what[120]; snprintf(what, sizeof what, "own driver %s", g_cfg.lib);
    if (dm_vk_loader_use_handle(vk, what, error, sizeof error)) { dm_vk_driver_fall_back(error); return 0; }
    return 1;
}



static int driver_mapped(void) {
    char want[PATH_MAX + 100], dir[PATH_MAX];
    if (!realpath(g_cfg.dir, dir)) return 0;
    snprintf(want, sizeof want, "%s/%s", dir, g_cfg.lib);
    FILE *m = fopen("/proc/self/maps", "re"); if (!m) return 0;
    char line[PATH_MAX + 200]; int hit = 0;
    while (!hit && fgets(line, sizeof line, m)) {
        char *p = strchr(line, '/'); if (!p) continue;
        p[strcspn(p, "\n")] = 0;
        if (!strcmp(p, want)) hit = 1;
    }
    fclose(m);
    return hit;
}

const char *dm_vk_driver_verify(VkPhysicalDevice physical, uint32_t devices) {
    if (g_mode != MODE_CUSTOM) return NULL;
    if (!devices || !physical) return "no Vulkan device";
    if (!driver_mapped()) return "driver not loaded (system driver answered)";
#ifdef DM_DRIVER_TESTS
    if (g_cfg.test_fail == 2) return "identity rejected (test switch)";
#endif
    VkPhysicalDeviceProperties p; vkGetPhysicalDeviceProperties(physical, &p);
    if (p.apiVersion < VK_API_VERSION_1_1) return "Vulkan below 1.1";
    return NULL;
}

void dm_vk_driver_identity(VkPhysicalDevice physical) {
    if (!configured()) return;
    VkPhysicalDeviceDriverProperties drv = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES};
    VkPhysicalDeviceProperties2 p2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
    VkPhysicalDeviceProperties p; vkGetPhysicalDeviceProperties(physical, &p);
    int have_drv = p.apiVersion >= VK_API_VERSION_1_2;
    if (!have_drv) {
        uint32_t n = 0; vkEnumerateDeviceExtensionProperties(physical, NULL, &n, NULL);
        VkExtensionProperties *e = n ? calloc(n, sizeof *e) : NULL;
        if (e && !vkEnumerateDeviceExtensionProperties(physical, NULL, &n, e))
            for (uint32_t i = 0; i < n; i++) if (!strcmp(e[i].extensionName, VK_KHR_DRIVER_PROPERTIES_EXTENSION_NAME)) have_drv = 1;
        free(e);
    }
    if (have_drv) { p2.pNext = &drv; vkGetPhysicalDeviceProperties2(physical, &p2); }
    const char *mode = g_mode == MODE_CUSTOM ? "custom" : g_mode == MODE_FALLBACK ? "fallback" : "system";
    char text[1400];
    snprintf(text, sizeof text, "mode=%s\nattempt=%s\nlib=%s\ndir=%s\nlabel=%s\ntu_debug=%s\nreason=%s\ndevice=%s\napi=%u.%u.%u\ndriverVersion=%#x\ndriverID=%d\ndriverName=%s\ndriverInfo=%s\n",
             mode, g_cfg.attempt, g_cfg.lib, g_cfg.dir, g_cfg.label, g_mode == MODE_CUSTOM ? g_cfg.tu_debug : "", g_reason, p.deviceName,
             VK_VERSION_MAJOR(p.apiVersion), VK_VERSION_MINOR(p.apiVersion), VK_VERSION_PATCH(p.apiVersion), p.driverVersion,
             have_drv ? (int)drv.driverID : -1, have_drv ? drv.driverName : "", have_drv ? drv.driverInfo : "");
    put_file("active.txt", text);
    fprintf(stderr, "vk driver: %s device=%s api=%u.%u.%u driverVersion=%#x driverID=%d driverName=%s driverInfo=%s\n", mode, p.deviceName,
            VK_VERSION_MAJOR(p.apiVersion), VK_VERSION_MINOR(p.apiVersion), VK_VERSION_PATCH(p.apiVersion), p.driverVersion,
            have_drv ? (int)drv.driverID : -1, have_drv ? drv.driverName : "-", have_drv ? drv.driverInfo : "-");
}

void dm_vk_driver_frame(void) {
    if (g_mode != MODE_CUSTOM || atomic_load_explicit(&g_ok, memory_order_relaxed)) return;
    if (atomic_fetch_add(&g_frames, 1) + 1 != DM_VK_DRIVER_OK_FRAMES) return;
    atomic_store(&g_ok, 1);
    char text[64]; snprintf(text, sizeof text, "%s\n", g_cfg.attempt);
    put_file("attempt.ok", text);
    fprintf(stderr, "vk driver: own driver %s presented %d frames, start counts as finished\n", g_cfg.lib, DM_VK_DRIVER_OK_FRAMES);
}


void dm_vk_driver_state(char *out, size_t bytes) {
    if (!out || !bytes) return;
    pthread_mutex_lock(&g_state_mu);
    if (g_mode == MODE_FALLBACK) snprintf(out, bytes, "fallback:%s", g_reason);
    else if (g_mode == MODE_CUSTOM) snprintf(out, bytes, "%s", atomic_load(&g_ok) ? "custom-ok" : "custom");
    else snprintf(out, bytes, "system");
    pthread_mutex_unlock(&g_state_mu);
}



int dm_vk_driver_label(char *out, size_t bytes) {
    if (!out || !bytes) return 0;
    pthread_mutex_lock(&g_state_mu);
    int own = g_mode == MODE_CUSTOM && g_cfg.label[0];
    if (own) snprintf(out, bytes, "%s", g_cfg.label);
    pthread_mutex_unlock(&g_state_mu);
    return own;
}
