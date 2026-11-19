

#include "dm_prof.h"
#include "dm.h"
#include "dm_uctx.h"
#include "dm_hud.h"
#include "dm_cursor.h"
#include "dm_hud_exp.h"
#include "dm_elysium.h"
#include "dm_bossrush.h"
#include "dm_loadwatch.h"
#include "dm_fightlog.h"
#include "dm_streak.h"
#include <TargetConditionals.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdatomic.h>
#include "dm_i18n.h"
#include "dm_itemjob.h"
#include <signal.h>
#include <dlfcn.h>
#include <sys/ucontext.h>
#include <unistd.h>
#include <malloc/malloc.h>
#include <sys/mman.h>
#include <libkern/OSCacheControl.h>
#include <mach/mach.h>
#if defined(__APPLE__)
#include <pthread/qos.h>
#endif
void crash_off(const char *tag, uint64_t a);
#define off crash_off
void dm_backtrace(uint64_t fp);
uint64_t dm_mtl_l3_sample(uint32_t tick) __attribute__((weak));
void dm_mtl_l3_tick(uint32_t tick) __attribute__((weak));







static int l3_fd_count(void) {
    static int said; int n = 0;
    DIR *d = opendir("/dev/fd");
    if (d) {
        int self = dirfd(d); struct dirent *e;
        while ((e = readdir(d))) if (e->d_name[0] != '.' && atoi(e->d_name) != self) n++;
        closedir(d);
        if (!said++) LOG("L3MEM 句柄计数：/dev/fd 目录");
        return n;
    }
    int top = getdtablesize(); if (top > 4096) top = 4096;
    for (int fd = 0; fd < top; fd++) if (fcntl(fd, F_GETFD) != -1) n++;
    if (!said++) LOG("L3MEM 句柄计数：/dev/fd 打不开，fcntl 探测前 %d 个", top);
    return n;
}

static pthread_mutex_t g_l3mem_mu = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_l3mem_cv = PTHREAD_COND_INITIALIZER;
static pthread_t g_l3mem_thread;
static int g_l3mem_thread_started;
static int g_l3mem_pending;
static uint32_t g_l3mem_pending_tick;
static uint64_t g_l3mem_seq;

static void l3mem_report(uint32_t tick, uint64_t seq) {
    uint64_t started = now_ns();
    malloc_statistics_t ms;
    task_vm_info_data_t vi;
    mach_msg_type_number_t count = TASK_VM_INFO_COUNT;
    uint64_t t = now_ns();
    malloc_zone_statistics(NULL, &ms);
    uint64_t malloc_ns = now_ns() - t;
    t = now_ns();
    uint64_t metal = dm_mtl_l3_sample ? dm_mtl_l3_sample(tick) : 0;
    uint64_t metal_ns = now_ns() - t;
    t = now_ns();
    int handles = l3_fd_count();
    uint64_t fd_ns = now_ns() - t;
    uint64_t footprint = 0;
    t = now_ns();
    if (task_info(mach_task_self(), TASK_VM_INFO, (task_info_t)&vi, &count) == KERN_SUCCESS)
        footprint = vi.phys_footprint;
    uint64_t task_ns = now_ns() - t;
    LOG("[dm] L3MEM 拍 %u 堆在用 %llu Metal已分配 %llu 句柄 %d phys_footprint %llu 耗时_ns %llu malloc_ns %llu metal_ns %llu fd_ns %llu task_ns %llu 线程 dm-l3mem 序号 %llu",
        tick,
        (unsigned long long)ms.size_in_use, (unsigned long long)metal, handles,
        (unsigned long long)footprint, (unsigned long long)(now_ns() - started),
        (unsigned long long)malloc_ns, (unsigned long long)metal_ns,
        (unsigned long long)fd_ns, (unsigned long long)task_ns,
        (unsigned long long)seq);
}

static void *l3mem_main(void *arg) {
    (void)arg;
#if defined(__APPLE__)
    pthread_setname_np("dm-l3mem");
    pthread_set_qos_class_self_np(QOS_CLASS_UTILITY, 0);
#endif
    for (;;) {
        pthread_mutex_lock(&g_l3mem_mu);
        while (!g_l3mem_pending) pthread_cond_wait(&g_l3mem_cv, &g_l3mem_mu);
        uint32_t tick = g_l3mem_pending_tick;
        uint64_t seq = ++g_l3mem_seq;
        g_l3mem_pending = 0;
        pthread_mutex_unlock(&g_l3mem_mu);
        l3mem_report(tick, seq);
    }
    return NULL;
}

static void l3mem_request(uint32_t tick) {
    pthread_mutex_lock(&g_l3mem_mu);
    if (!g_l3mem_thread_started) {
        if (pthread_create(&g_l3mem_thread, NULL, l3mem_main, NULL)) {
            pthread_mutex_unlock(&g_l3mem_mu);
            LOG("【红】L3MEM 采样线程创建失败");
            return;
        }
        pthread_detach(g_l3mem_thread);
        g_l3mem_thread_started = 1;
    }
    g_l3mem_pending_tick = tick;
    g_l3mem_pending = 1;
    pthread_cond_signal(&g_l3mem_cv);
    pthread_mutex_unlock(&g_l3mem_mu);
}







#if !TARGET_OS_IPHONE




#define MAX_PROBE 16
typedef void (*probe_fn)(ucontext_t *uc);
static struct { uint64_t at; uint32_t *slot; unsigned hits; char name[40]; long dump; probe_fn fn; } g_probe[MAX_PROBE];
static int g_nprobe;
static void probe_str(uint64_t p) {
    char buf[80]; vm_size_t got = 0;
    if (p < 0x1000 || vm_read_overwrite(mach_task_self(), p, sizeof buf - 1, (vm_address_t)buf, &got) || !got) return;
    buf[got] = 0; int n = 0; while (buf[n] >= 0x20 && buf[n] < 0x7f) n++;
    if (n >= 3 && (buf[n] == 0 || n == (int)got)) { fprintf(stderr, " \"%.*s\"", n, buf); return; }


    if (got >= 24) {
        uint64_t ptr, len; memcpy(&ptr, buf, 8); memcpy(&len, buf + 8, 8);
        char t[96]; got = 0;
        if (len >= 3 && len < sizeof t && !vm_read_overwrite(mach_task_self(), ptr, len, (vm_address_t)t, &got) && got == len) {
            int k = 0; while (k < (int)len && t[k] >= 0x20 && t[k] < 0x7f) k++;
            if (k == (int)len) fprintf(stderr, " S\"%.*s\"", (int)len, t);
        }
    }
}


static void probe_dump_once(void) {
    static int done; const char *e = getenv("DM_PROBE_DUMP"); if (done || !e) return; done = 1;
    char spec[512]; strlcpy(spec, e, sizeof spec);
    char *mod = strtok(spec, ":"), *o = strtok(NULL, ":"), *n = strtok(NULL, ":"), *f = strtok(NULL, ":");
    module_t *m = NULL; for (int i = 0; i < g_nmod; i++) if (mod && !strcmp(g_mod[i].name, mod)) m = &g_mod[i];
    if (!m || !o || !n) { LOG("DM_PROBE_DUMP 格式：模块:偏移:长度[:文件]"); return; }
    char def[512]; snprintf(def, sizeof def, "%s%s/run/platform/probe_dump.bin", getenv("DM_ROOT") ?: getenv("HOME"), getenv("DM_ROOT") ? "" : "/dm-work");
    FILE *fp = fopen(f ? f : def, "wb"); if (!fp) return;
    fwrite(m->base + strtoull(o, 0, 16), 1, strtoull(n, 0, 16), fp); fclose(fp);
    LOG("探针转储 %s+%s 长 %s → %s", mod, o, n, f ? f : def);
}
static int probe_hit(ucontext_t *uc) {
    uint64_t pc = UC_PC(uc);
    for (int i = 0; i < g_nprobe; i++) if (g_probe[i].at == pc) {
        probe_dump_once();
        if (g_probe[i].fn) { g_probe[i].hits++; g_probe[i].fn(uc); UC_PC(uc) = (uint64_t)g_probe[i].slot; return 1; }
        { extern uint64_t g_t0_ns; uint64_t now_ns(void); fprintf(stderr, "[dm] PROBE %s #%u [%.4fs]", g_probe[i].name, ++g_probe[i].hits, (now_ns() - g_t0_ns) / 1e9); }
        for (int r = 0; r < 4; r++) { uint64_t v = UC_X(uc, r); fprintf(stderr, " x%d=%#llx", r, v); probe_str(v); }
        for (int r = 19; r < 24; r++) fprintf(stderr, " x%d=%#llx", r, UC_X(uc, r));
        { uint64_t x8 = UC_X(uc, 8); Dl_info di; fprintf(stderr, " x8=%#llx", x8);
          if (dladdr((void *)x8, &di) && di.dli_sname) fprintf(stderr, "（%s+%#lx）", di.dli_sname, (unsigned long)(x8 - (uint64_t)di.dli_saddr)); }
        if (g_probe[i].dump >= 0) {
            uint8_t b[16]; vm_size_t got = 0;
            uint64_t at = UC_X(uc, 0) + (g_probe[i].dump & 0xffffff);
            if (g_probe[i].dump & 0x1000000) { uint64_t p = 0; vm_size_t g = 0; if (!vm_read_overwrite(mach_task_self(), at, 8, (vm_address_t)&p, &g)) at = p; else at = 0; }
            if (at && !vm_read_overwrite(mach_task_self(), at, 16, (vm_address_t)b, &got) && got == 16) {
                fprintf(stderr, " [%s%#lx]=", (g_probe[i].dump & 0x1000000) ? "*x0+" : "x0+", g_probe[i].dump & 0xffffff); for (int k = 0; k < 16; k++) fprintf(stderr, "%02x", b[k]); }
        }
        fputc('\n', stderr);
        dm_backtrace(UC_FP(uc));
        UC_PC(uc) = (uint64_t)g_probe[i].slot;
        return 1;
    }
    return 0;
}
static uint32_t *g_slots; static int g_slots_sealed;
static void probe_add(module_t *m, uint64_t off, long dump, probe_fn fn) {
    if (g_nprobe >= MAX_PROBE) die("探针过多");
    if (!g_slots) g_slots = mmap(NULL, 0x4000, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    uint64_t at = (uint64_t)m->base + off;
    uint32_t *w = (uint32_t *)at, *sl = g_slots + g_nprobe * 8;
    sl[0] = *w;
    sl[1] = 0x58000050;
    sl[2] = 0xD61F0200;
    *(uint64_t *)(sl + 3) = at + 4;
    uint64_t pg = at & ~0x3FFFull;
    if (mprotect((void *)pg, 0x4000, PROT_READ | PROT_WRITE)) die("probe mprotect");
    *w = 0xD43E0000;
    mprotect((void *)pg, 0x4000, PROT_READ | PROT_EXEC);
    sys_icache_invalidate(w, 4);
    g_probe[g_nprobe].at = at; g_probe[g_nprobe].slot = sl; g_probe[g_nprobe].dump = dump; g_probe[g_nprobe].fn = fn;
    snprintf(g_probe[g_nprobe].name, sizeof g_probe[0].name, "%s+%#llx", m->name, off);
    LOG("探针 %s（原指令 %08x）", g_probe[g_nprobe].name, sl[0]);
    g_nprobe++;
}

#endif





#define UIH_BITS 18
static struct { uint64_t h; const char *s; } *g_uih;
static uint64_t fnv1a(const char *p, size_t n) { uint64_t h = 0xcbf29ce484222325ull; for (size_t i = 0; i < n; i++) h = (h ^ (uint8_t)p[i]) * 0x100000001b3ull; return h; }
static void uih_build(void) {
    module_t *m = &g_mod[0];
    g_uih = calloc(1u << UIH_BITS, sizeof *g_uih);
    const char *p = (const char *)m->base + m->ro_off, *e = (const char *)m->base + m->data_off;
    while (p < e) {
        const char *q = p; while (q < e && *q >= 0x20 && *q < 0x7f) q++;
        if (q < e && !*q && q - p >= 3) {
            uint64_t h = fnv1a(p, q - p); unsigned i = (unsigned)(h >> (64 - UIH_BITS));
            while (g_uih[i].s && g_uih[i].h != h) i = (i + 1) & ((1u << UIH_BITS) - 1);
            g_uih[i].h = h; g_uih[i].s = p;
        }
        p = q + 1;
    }
}
static const char *uih_name(uint64_t h, char *buf) {
    if (g_uih) for (unsigned i = (unsigned)(h >> (64 - UIH_BITS)); g_uih[i].s; i = (i + 1) & ((1u << UIH_BITS) - 1))
        if (g_uih[i].h == h) return g_uih[i].s;
    snprintf(buf, 24, "%#llx", h); return buf;
}




static FILE *g_uifp; static unsigned g_uiseq;
static int in_game(void); static uint32_t cur_tick(void);
static void res_report(const char *why) { void (*f)(uint32_t, const char *) = dm_res_report_cb(); if (f) f(in_game() ? cur_tick() : 0, why); }
static pthread_mutex_t g_uilock = PTHREAD_MUTEX_INITIALIZER;
static void json_str(FILE *f, const char *s) {
    fputc('"', f);
    for (; *s; s++) { if (*s == '"' || *s == '\\') fputc('\\', f); fputc(*s, f); }
    fputc('"', f);
}






static _Atomic uint64_t g_pns[PROF_N], g_pby[PROF_N];
static const char *const g_pname[PROF_N] = {"RomFS读", "锁等", "条件变量等", "地址等", "同步对象等", "Sleep", "主线程等", "出图等(图形)", "着色器/管线(图形)", "TACT解码", "BC软解(图形)"};
#define MAX_SEG 12
static struct { const char *name; uint64_t t, ns[PROF_N], by[PROF_N]; } g_seg[MAX_SEG];
static int g_nseg, g_prof_done;
static pthread_mutex_t g_proflock = PTHREAD_MUTEX_INITIALIZER;
void dm_prof_add(int id, uint64_t ns, uint64_t bytes) {
    if (!g_flag.bootprof || (unsigned)id >= PROF_N) return;
    atomic_fetch_add(&g_pns[id], ns); if (bytes) atomic_fetch_add(&g_pby[id], bytes);
}
static void prof_table(void) {
    extern uint64_t g_t0_ns;
    fprintf(stderr, "[dm] ===== 启动时间线（DM_BOOTPROF；等待类为全线程累加的线程·秒）=====\n");
    fprintf(stderr, "[dm] %-40s %8s %8s", "分段", "墙钟s", "累计s");
    for (int k = 0; k < PROF_N; k++) fprintf(stderr, " %10s", g_pname[k]);
    fprintf(stderr, " %9s %9s\n", "读MiB", "解码MiB");
    uint64_t prev_t = g_t0_ns, prev_ns[PROF_N] = {0}, prev_by[PROF_N] = {0};
    for (int j = 0; j < g_nseg; j++) {
        fprintf(stderr, "[dm] %-40s %8.3f %8.3f", g_seg[j].name, (g_seg[j].t - prev_t) / 1e9, (g_seg[j].t - g_t0_ns) / 1e9);
        for (int k = 0; k < PROF_N; k++) fprintf(stderr, " %10.3f", (g_seg[j].ns[k] - prev_ns[k]) / 1e9);
        fprintf(stderr, " %9.1f %9.1f\n", (g_seg[j].by[PROF_FS_READ] - prev_by[PROF_FS_READ]) / 1048576.0, (g_seg[j].by[PROF_TACT] - prev_by[PROF_TACT]) / 1048576.0);
        prev_t = g_seg[j].t; memcpy(prev_ns, g_seg[j].ns, sizeof prev_ns); memcpy(prev_by, g_seg[j].by, sizeof prev_by);
    }
}
void dm_prof_mark(const char *seg) {
    if (!g_flag.bootprof) return;
    uint64_t now_ns(void);
    pthread_mutex_lock(&g_proflock);
    if (!g_prof_done && g_nseg < MAX_SEG) {
        g_seg[g_nseg].name = seg; g_seg[g_nseg].t = now_ns();
        for (int k = 0; k < PROF_N; k++) { g_seg[g_nseg].ns[k] = g_pns[k]; g_seg[g_nseg].by[k] = g_pby[k]; }
        g_nseg++;
        if (strstr(seg, "MainMenuButtonRibbon")) { g_prof_done = 1; prof_table(); }
    }
    pthread_mutex_unlock(&g_proflock);
}
static _Atomic uint64_t g_ribbon_ns;


static struct { uint64_t t, ns[PROF_N], by[PROF_N]; int on; } g_ld;
static void prof_load_begin(void) {
    if (!g_flag.bootprof || g_ld.on) return; uint64_t now_ns(void);
    g_ld.t = now_ns(); for (int k = 0; k < PROF_N; k++) { g_ld.ns[k] = g_pns[k]; g_ld.by[k] = g_pby[k]; } g_ld.on = 1;
}
static void prof_load_end(void) {
    if (!g_flag.bootprof || !g_ld.on) return; uint64_t now_ns(void); g_ld.on = 0;
    char line[1024]; size_t o = snprintf(line, sizeof line, "进局装载 墙钟 %.3f s：", (now_ns() - g_ld.t) / 1e9);
    for (int k = 0; k < PROF_N && o < sizeof line; k++) o += snprintf(line + o, sizeof line - o, " %s %.3f", g_pname[k], (g_pns[k] - g_ld.ns[k]) / 1e9);
    if (o < sizeof line) snprintf(line + o, sizeof line - o, " 读 %.1f MiB 解码 %.1f MiB", (g_pby[PROF_FS_READ] - g_ld.by[PROF_FS_READ]) / 1048576.0, (g_pby[PROF_TACT] - g_ld.by[PROF_TACT]) / 1048576.0);
    LOG("%s", line);
    FILE *f = fopen("loadprof.txt", "a"); if (f) { time_t w = time(NULL); struct tm tmv; localtime_r(&w, &tmv); char ts[16]; strftime(ts, sizeof ts, "%H:%M:%S", &tmv); fprintf(f, "%s\t%s\n", ts, line); fclose(f); }
}
static void prof_on_uimsg(uint64_t act, const char *arg) {
    if (!g_flag.bootprof) return;
    static int seen_fe, seen_open, seen_close;
    if (act == 0x61b685f774a14250ull && !seen_fe) { seen_fe = 1; dm_prof_mark("④ nnMain → ToFrontEnd"); }
    else if (!strcmp(arg, "MainMenuLoadingPanel") && act == 0x723d40e52f74e229ull && !seen_open) { seen_open = 1; dm_prof_mark("④b ToFrontEnd → 装载面板打开"); }
    else if (!strcmp(arg, "MainMenuLoadingPanel") && act != 0x723d40e52f74e229ull && seen_open && !seen_close) { seen_close = 1; dm_prof_mark("⑤ 装载期（装载面板开 → 关）"); }
    if (!strcmp(arg, "LoadScreenPanel") && act == 0x723d40e52f74e229ull) prof_load_begin();
    else if (!strcmp(arg, "MainMenuButtonRibbon")) { if (!g_ribbon_ns) { uint64_t now_ns(void); g_ribbon_ns = now_ns(); } dm_prof_mark("⑥ → MainMenuButtonRibbon"); }
}
uint64_t dm_prof_ribbon_ns(void) { return g_ribbon_ns; }
static int g_ui_print;
static void uimsg_print(const uint64_t *regs);

static void (*g_ui_sub[8])(const char *cat, const char *act, const char *arg); static _Atomic int g_nui_sub;


void uimsg_emit(const char *cat, const char *act, const char *arg) {
    for (int k = 0, n = atomic_load(&g_nui_sub); k < n; k++) g_ui_sub[k](cat, act, arg);
}





static pthread_mutex_t g_ui_sub_mu = PTHREAD_MUTEX_INITIALIZER;
int uimsg_try_subscribe(void (*fn)(const char *cat, const char *act, const char *arg)) {
    if (!gate_obs_present(OBS_UIMSG)) return -1;
    pthread_mutex_lock(&g_ui_sub_mu);
    int k = atomic_load(&g_nui_sub); if (k == 8) { pthread_mutex_unlock(&g_ui_sub_mu); return -2; }
    g_ui_sub[k] = fn; atomic_store(&g_nui_sub, k + 1);
    pthread_mutex_unlock(&g_ui_sub_mu);
    gate_obs_set(OBS_UIMSG, uimsg_print); return 0;
}
void uimsg_subscribe(void (*fn)(const char *cat, const char *act, const char *arg)) {
    int r = uimsg_try_subscribe(fn);
    if (r == -1) die("产物没有 UI 观测门 OBS_UIMSG（需 r6 起的产物）：UI 消息订阅无法工作");
    if (r == -2) die("UI 消息订阅者超过 8 个");
}















#define UIH_MONSTERHEALTH 0xd3a07a5ecad1a9a3ull
#define UIH_SETINFO       0x40ffba2d98a3ff97ull
static void monres_on_setinfo(const uint64_t *regs, uint64_t payload) {
    int dm_custom_get(const char *key), lang = dm_lang_index(); if (!dm_custom_get("item_notes") || (lang != 8 && lang != 2 && lang != 10)) return;
    module_t *mm = NULL; for (int i = 0; i < g_nmod; i++) if (!strcmp(g_mod[i].name, "main")) mm = &g_mod[i];
    uint64_t unit = regs[19], name = 0; uint32_t type = 0; vm_size_t got = 0;
    if (!mm || !unit || vm_read_overwrite(mach_task_self(), unit, 4, (vm_address_t)&type, &got) || type != 1) return;
    if (vm_read_overwrite(mach_task_self(), payload, 8, (vm_address_t)&name, &got) || !name) return;
    int (*get)(uint64_t, int, int) = (int (*)(uint64_t, int, int))(mm->base + 0x5b9290);
    static const int id[6] = {36, 37, 39, 41, 43, 45};
    static const char *lb[6] = {"物", "魔", "火", "电", "冰", "毒"}, *lbes[6] = {"Fís", "Mág", "Fue", "Ray", "Frí", "Ven"}, *cc[6] = {"\xee\x81\xbe" "4", "\xee\x81\xbe" "8", "\xee\x81\xbe" "1", "\xee\x81\xbe" "9", "\xee\x81\xbe" "3", "\xee\x81\xbe" "2"};
    char add[160] = ""; size_t n = 0;
    for (int i = 0; i < 6; i++) { int v = get(unit, id[i], 0); if (!v) continue;
        int w = lang != 8 ? (v >= 100 ? snprintf(add + n, sizeof add - n, " %s%s inm", cc[i], lbes[i]) : snprintf(add + n, sizeof add - n, " %s%s %d", cc[i], lbes[i], v))
                          : v >= 100 ? snprintf(add + n, sizeof add - n, " %s%s免", cc[i], lb[i]) : snprintf(add + n, sizeof add - n, " %s%s%d", cc[i], lb[i], v);
        if (w < 0 || (size_t)w >= sizeof add - n) return; n += (size_t)w; }
#ifndef DM_PLAYER_BUILD
    { static int lg = -1; static uint64_t last; if (lg < 0) lg = getenv("DM_TEST_MONRES_LOG") != NULL;
      if (lg && unit != last) { last = unit; int v[6]; for (int i = 0; i < 6; i++) v[i] = get(unit, id[i], 0);
          LOG("DM_TEST_MONRES_LOG：单位 %#llx 属性 36/37/39/41/43/45 = %d/%d/%d/%d/%d/%d → 追加「%s」", (unsigned long long)unit, v[0], v[1], v[2], v[3], v[4], v[5], n ? add : "（全 0 不加）"); } }
#endif
    if (!n) return;
    char *buf = (char *)name; size_t len = strnlen(buf, 0x800); if (len >= 0x800) return;
    size_t keep = len; while (keep && buf[keep - 1] == ' ') keep--;
    if (keep + n + 5 + (len - keep) >= 0x800) return;
    memcpy(buf + keep, add, n); memcpy(buf + keep + n, "\xee\x81\xbe" "0", 4); memset(buf + keep + n + 4, ' ', len - keep); buf[keep + n + 4 + (len - keep)] = 0;
}

static void uimsg_print(const uint64_t *regs) {
    if ((g_ui_print || atomic_load(&g_nui_sub)) && !g_uih) uih_build();
    uint64_t msg = regs[1], f[3] = {0}; vm_size_t got = 0;
    if (vm_read_overwrite(mach_task_self(), msg, sizeof f, (vm_address_t)f, &got) || got != sizeof f) return;
    char b0[24], b1[24], nm[65] = ""; int have_arg = 0; got = 0;
    if (f[2] && !vm_read_overwrite(mach_task_self(), f[2], 64, (vm_address_t)nm, &got)) {
        nm[got] = 0; have_arg = 1;
        size_t k = 0; for (; k < got && nm[k]; k++) if ((uint8_t)nm[k] < 0x20 || (uint8_t)nm[k] >= 0x7f) { have_arg = 0; break; }


        if (k == got) have_arg = 0;
    }
    if (f[0] == UIH_MONSTERHEALTH && f[1] == UIH_SETINFO && f[2]) monres_on_setinfo(regs, f[2]);
    prof_on_uimsg(f[1], have_arg ? nm : "");
    char c0[24], c1[24], stage[256];
    const char *n0 = uih_name(f[0], c0), *n1 = uih_name(f[1], c1);
    snprintf(stage, sizeof stage, "%s:%s:%s", n0, n1, have_arg ? nm : "");
    crash_watch_progress(cur_tick(), stage);




    { static int played, back; if (!strcmp(n0, "FrontEndNavigation") && !strcmp(n1, "ToGame")) { played = 1; back = 0; crash_watch_pregame_done(); }
      else if (!strcmp(n0, "FrontEndNavigation") && !strcmp(n1, "ToFrontEnd") && played) { back = 1; void crash_watch_pregame_again(void); crash_watch_pregame_again(); }
      else if (have_arg && !strcmp(nm, "MainMenuButtonRibbon") && !back) crash_watch_pregame_done(); }
    for (int k = 0, n = atomic_load(&g_nui_sub); k < n; k++) { char s0[24], s1[24]; g_ui_sub[k](uih_name(f[0], s0), uih_name(f[1], s1), have_arg ? nm : ""); }
    if (!g_ui_print) return;
    const char *cat = uih_name(f[0], b0), *act = uih_name(f[1], b1);
    pthread_mutex_lock(&g_uilock);
    if (g_uifp) {
        fprintf(g_uifp, "{\"seq\":%u,\"cat_h\":\"0x%016llx\",\"act_h\":\"0x%016llx\",\"arg\":", g_uiseq++, f[0], f[1]);
        if (have_arg) json_str(g_uifp, nm); else fputs("null", g_uifp);
        if (cat != b0) { fputs(",\"cat\":", g_uifp); json_str(g_uifp, cat); }
        if (act != b1) { fputs(",\"act\":", g_uifp); json_str(g_uifp, act); }
        if (in_game()) fprintf(g_uifp, ",\"tick\":%u", cur_tick());
        fputs("}\n", g_uifp); fflush(g_uifp);
    }
    if (!strcmp(cat, "PanelManager") && have_arg) {
        extern char g_lf_panel[48];
        if (!strcmp(act, "OpenPanel")) strlcpy(g_lf_panel, nm, sizeof g_lf_panel); else if (!strcmp(act, "ClosePanel") && !strcmp(g_lf_panel, nm)) g_lf_panel[0] = 0;
    }
    extern uint64_t g_t0_ns; uint64_t now_ns(void);
    static uint64_t last[2]; static char lastnm[65]; static unsigned rep;
    if (f[0] == last[0] && f[1] == last[1] && !strcmp(have_arg ? nm : "", lastnm)) { rep++; pthread_mutex_unlock(&g_uilock); return; }
    if (rep) fprintf(stderr, "[dm] UIMSG   （上一条又重复 %u 次）\n", rep);
    rep = 0; last[0] = f[0]; last[1] = f[1]; strlcpy(lastnm, have_arg ? nm : "", sizeof lastnm);
    fprintf(stderr, "[dm] UIMSG [%.2fs] %s %s %s\n", (now_ns() - g_t0_ns) / 1e9, cat, act, have_arg ? nm : "");
    pthread_mutex_unlock(&g_uilock);
}

void crash_set_trap_hook(int (*fn)(ucontext_t *uc));












static _Atomic int g_quiet_diff;
void dm_quiet_difficulty_msg(int on) { atomic_store(&g_quiet_diff, on); }
static void event_msg_obs(const uint64_t *regs) {
    uint8_t *pkt = (uint8_t *)(uintptr_t)regs[0]; if (!atomic_load(&g_quiet_diff) || !pkt || pkt[1] != 0x17) return;
    static int n; if (n++ < 8) LOG("秘境：拦下原版「游戏难度设为 N」提示（事件子类型 0x17）"); pkt[1] = 0xff;
}
static void setlang_obs(const uint64_t *regs) {
    dm_ui_gen_bump(DM_UIGEN_LANG);
    { void dm_padmode_lang_reset(void); dm_padmode_lang_reset(); }
    module_t *m = NULL; for (int i = 0; i < g_nmod; i++) if (!strcmp(g_mod[i].name, "main")) m = &g_mod[i];
    uint32_t idx = 0; const char *nm = NULL; vm_size_t got = 0;
    if (regs[1] && !vm_read_overwrite(mach_task_self(), regs[1], 4, (vm_address_t)&idx, &got) && got == 4 && idx < 13) {
        uint64_t p = 0; memcpy(&p, m->base + 0x20ae478 + 8ull * idx, 8); nm = (const char *)p; }
    static uint32_t last = 0xffffffff; if (idx != last) { last = idx; LOG("SetLanguage：语言 %s（号 %u），重装字符串表", nm ? nm : "?", idx); }
    if (nm) atomic_store(&g_dm_lang, (int)idx);
}
void probe_install(void) {
    if (gate_obs_present(OBS_SETLANG)) gate_obs_set(OBS_SETLANG, setlang_obs);
    if (gate_obs_present(OBS_EVENT_MSG)) gate_obs_set(OBS_EVENT_MSG, event_msg_obs);
    { void dm_ui_tooltip_obs(const uint64_t *regs); if (gate_obs_present(OBS_TOOLTIP)) gate_obs_set(OBS_TOOLTIP, dm_ui_tooltip_obs); }
    const char *ui = getenv("DM_UIMSG");
    if (ui && strcmp(ui, "1") && !(g_uifp = fopen(ui, "w"))) die("DM_UIMSG 打不开 %s", ui);
    g_ui_print = ui != NULL;
    if (ui || g_flag.bootprof) {
        if (!gate_obs_present(OBS_UIMSG)) LOG("【红】产物没有 UI 观测门 OBS_UIMSG（需 r6 起的产物）：DM_UIMSG / 启动分段 ④–⑥ 无输出");
        else gate_obs_set(OBS_UIMSG, uimsg_print);
    }
#if !TARGET_OS_IPHONE
    crash_set_trap_hook(probe_hit);
    const char *e = getenv("DM_PROBE");
    if (e) {
        char spec[512]; strlcpy(spec, e, sizeof spec);
        for (char *t = strtok(spec, ","); t; t = strtok(NULL, ",")) {
            char *c = strchr(t, ':'); if (!c) die("DM_PROBE 格式：模块:偏移");
            *c = 0; module_t *m = NULL;
            for (int i = 0; i < g_nmod; i++) if (!strcmp(g_mod[i].name, t)) m = &g_mod[i];
            if (!m) die("DM_PROBE 无此模块 %s", t);
            char *at_ = strchr(c + 1, '@');
            long dump = -1;
            if (at_) dump = at_[1] == '*' ? (0x1000000 | strtol(at_ + 2, 0, 16)) : strtol(at_ + 1, 0, 16);
            probe_add(m, strtoull(c + 1, 0, 16), dump, NULL);
        }
    }
    if (g_slots && !g_slots_sealed) { mprotect(g_slots, 0x4000, PROT_READ | PROT_EXEC); sys_icache_invalidate(g_slots, 0x4000); g_slots_sealed = 1; }
#endif
}



static pthread_t g_sample_pt;




#define SRING 1024
static struct { uint64_t t, pc, lr, bt[16]; int n; uint32_t h; } g_sring[SRING];
static _Atomic unsigned g_shead;
static void on_sample(int sig, siginfo_t *si, void *uc_) {
    (void)sig; (void)si; ucontext_t *uc = uc_;
    unsigned i = atomic_load(&g_shead) % SRING;
    extern uint64_t now_ns(void);
    uint32_t thr_cur_handle(void);
    g_sring[i].h = thr_cur_handle(); g_sring[i].t = now_ns(); g_sring[i].pc = UC_PC(uc); g_sring[i].lr = UC_LR(uc);
    uint64_t fp = UC_FP(uc); int n = 0;


    for (; n < 16 && fp && !(fp & 7); n++) {
        uint64_t fr[2]; vm_size_t got = 0;
        if (vm_read_overwrite(mach_task_self(), fp, sizeof fr, (vm_address_t)fr, &got) || got != sizeof fr) break;
        g_sring[i].bt[n] = fr[1];
        if (fr[0] <= fp) { n++; break; }
        fp = fr[0];
    }
    g_sring[i].n = n;
    atomic_fetch_add(&g_shead, 1);
}
int thr_find_by_name(const char *name, pthread_t *out);
static const char *g_sample_name;
static void *sampler(void *p) {
    useconds_t us = (useconds_t)(uintptr_t)p; unsigned printed = 0;
    for (;;) {
        usleep(us);
        if (g_sample_name && !strcmp(g_sample_name, "*")) {
            int thr_list(pthread_t *out, int max); pthread_t all[64]; int n = thr_list(all, 64);
            for (int k = 0; k < n; k++) { pthread_kill(all[k], SIGUSR2); usleep(200); }
        } else {
            if (g_sample_name) { pthread_t t; if (!thr_find_by_name(g_sample_name, &t)) continue; g_sample_pt = t; }
            pthread_kill(g_sample_pt, SIGUSR2);
        }
        for (unsigned h = atomic_load(&g_shead); printed < h; printed++) {
            unsigned i = printed % SRING;
            extern uint64_t g_t0_ns;
            fprintf(stderr, "[dm] SAMPLE #%u [%.3fs] thr=%#x", printed + 1, (g_sring[i].t - g_t0_ns) / 1e9, g_sring[i].h); off("pc", g_sring[i].pc); off("lr", g_sring[i].lr);
            fprintf(stderr, "\n[dm] bt:"); for (int k = 0; k < g_sring[i].n; k++) off("", g_sring[i].bt[k]); fputc('\n', stderr);
        }
    }
}
void crash_sample_self(void) {
    const char *e = getenv("DM_SAMPLE"); if (!e) return;
    g_sample_pt = pthread_self();
    const char *c = strchr(e, ':'); if (c) g_sample_name = strdup(c + 1);
    struct sigaction sa = {.sa_sigaction = on_sample, .sa_flags = SA_SIGINFO | SA_ONSTACK | SA_RESTART};
    sigaction(SIGUSR2, &sa, 0);
    pthread_t t; pthread_create(&t, NULL, sampler, (void *)(uintptr_t)(atoi(e) * 1000)); pthread_detach(t);
}







#include <CommonCrypto/CommonDigest.h>
static void *g_zarena[64]; static int g_nzarena;
static void *bench_zalloc(void *o, unsigned items, unsigned size) { (void)o; void *p = calloc(items, size); if (g_nzarena < 64) g_zarena[g_nzarena++] = p; return p; }
static void bench_zfree(void *o, void *p) { (void)o; (void)p; }
void inflate_bench_maybe(void) {
    const char *path = getenv("DM_INFLATE_BENCH"); if (!path) return;
    FILE *f = fopen(path, "rb"); if (!f) die("打不开 %s", path);
    fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
    uint8_t *buf = malloc(sz); if (fread(buf, 1, sz, f) != (size_t)sz) die("读样本失败"); fclose(f);
    int (*init)(void *, const char *, int) = (void *)(g_mod[0].base + 0xf3a930);
    int (*inflate)(void *, int) = (void *)(g_mod[0].base + 0xf3aa90);
    uint8_t *out = malloc(64 << 20); uint64_t now_ns(void);
    int rounds = getenv("DM_INFLATE_ROUNDS") ? atoi(getenv("DM_INFLATE_ROUNDS")) : 3;
    double best = 1e9; long nfile = 0, bad = 0; uint64_t zin = 0, zout = 0;
    for (int r = 0; r < rounds; r++) {
        uint64_t t = 0; nfile = bad = 0; zin = zout = 0;
        for (uint8_t *p = buf; p < buf + sz;) {
            uint8_t ckey[16]; memcpy(ckey, p, 16); uint32_t cs, nc; memcpy(&cs, p + 16, 4); memcpy(&nc, p + 20, 4); p += 24;
            size_t pos = 0; int ok = 1;
            for (uint32_t k = 0; k < nc; k++) {
                uint8_t mode = p[0]; uint32_t n, dl; memcpy(&n, p + 1, 4); memcpy(&dl, p + 5, 4); p += 9;
                if (mode == 'Z') {
                    uint8_t zs[112] = {0};
                    *(uint8_t **)(zs + 0) = p; *(uint32_t *)(zs + 8) = n; *(uint8_t **)(zs + 24) = out + pos; *(uint32_t *)(zs + 32) = dl;
                    *(void **)(zs + 64) = (void *)bench_zalloc; *(void **)(zs + 72) = (void *)bench_zfree;
                    uint64_t t0 = now_ns();
                    int rc = init(zs, "1.2.11", 112); if (!rc) rc = inflate(zs, 4);
                    t += now_ns() - t0;
                    if (rc != 1 || *(uint64_t *)(zs + 40) != dl) ok = 0;
                    for (int i = 0; i < g_nzarena; i++) free(g_zarena[i]);
                    g_nzarena = 0; zin += n; zout += dl;
                } else if (mode == 'N') memcpy(out + pos, p, n);
                else ok = 0;
                pos += dl; p += n;
            }
            uint8_t md[16]; CC_MD5(out, (CC_LONG)pos, md);
            if (!ok || pos != cs || memcmp(md, ckey, 16)) bad++;
            nfile++;
        }
        if (t / 1e9 < best) best = t / 1e9;
    }
    fprintf(stderr, "[dm] 游戏 zlib     文件 %ld 不符 %ld | zlib 输入 %.1f MiB 输出 %.1f MiB | 解码最小 %.3f s（%d 轮）= %.0f MiB/s 输出\n",
            nfile, bad, zin / 1048576.0, zout / 1048576.0, best, rounds, zout / 1048576.0 / best);
    exit(bad != 0);
}









static int tk_rd(uint64_t a, void *buf, size_t n);
static void cam_threats_tick(uint64_t G, uint64_t pl);
static _Atomic uint64_t g_player_path;


int dm_diag_player_fixed(uint32_t *x, uint32_t *y) { uint64_t pp = atomic_load(&g_player_path); uint32_t c[2]; if (!pp || !tk_rd(pp, c, 8)) return 0; *x = c[0]; *y = c[1]; return 1; }
static FILE *g_tkfp; static int g_tkfull; static long g_tkleft = -1; static int64_t g_tklast = -1; static uint64_t g_tkt0; static unsigned g_tkreads;
static FILE *g_spfp;


static uint64_t g_players_seq, g_players_ack; static dm_hud_info g_hud;
static module_t *main_mod(void) { for (int i = 0; i < g_nmod; i++) if (!strcmp(g_mod[i].name, "main")) return &g_mod[i]; return NULL; }
static void *players_obj(module_t *main) { return ((void *(*)(void))(main->base + 0x3102c0))(); }

static void players_set(int want) {
    module_t *main = main_mod();
    if (!main) { LOG("【红】DM_PLAYERS：没有 main 模块"); return; }
    uint32_t opmode = 0;
    if (!tk_rd((uint64_t)main->base + 0x2d20b8c, &opmode, 4)) { LOG("【红】DM_PLAYERS：读 main+0x2d20b8c 失败"); return; }
    if (opmode != 0 && opmode != 5 && opmode != 7) { LOG("DM_PLAYERS：模式 %u 不在原版允许集合 {0,5,7}，不改档", opmode); return; }
    void *obj = players_obj(main);
    if (!obj) { LOG("【红】DM_PLAYERS：0x3102c0 返回空对象"); return; }
    ((void (*)(void *, uint32_t))(main->base + 0x1199c50))(obj, (uint32_t)want);
    LOG("DM_PLAYERS：原版路径 main+0x3102c0() → main+0x1199c50(obj,%d)（模式值 %u）", want, opmode);
}
static void players_apply(void) {




    int want = 0; uint64_t seq = dm_hud_players_take(&want);
    if (!seq || seq == g_players_seq) return;
    g_players_seq = g_players_ack = seq;
    players_set(want);
}

static int stat_read(uint64_t unit, uint32_t want, uint32_t *value) {
    uint64_t sl = 0, sb = 0; uint32_t sc = 0;
    if (!tk_rd(unit + 0x88, &sl, 8) || !sl || !tk_rd(sl + 0x30, &sb, 8) || !tk_rd(sl + 0x38, &sc, 4)) return 0;
    if (sc > 512) sc = 512;
    uint32_t pair[2];
    for (uint32_t i = 0; i < sc; i++) if (tk_rd(sb + 8 * i, pair, sizeof pair) && pair[0] == want << 16) { *value = pair[1]; return 1; }
    return 0;
}



static uint32_t g_hp1_to = 256;
static void test_hp1(uint64_t unit) {
    uint64_t sl = 0, sb = 0; uint32_t sc = 0;
    if (!tk_rd(unit + 0x88, &sl, 8) || !sl || !tk_rd(sl + 0x30, &sb, 8) || !tk_rd(sl + 0x38, &sc, 4)) return;
    if (sc > 512) sc = 512; uint32_t pair[2];
    for (uint32_t i = 0; i < sc; i++) if (tk_rd(sb + 8 * i, pair, sizeof pair) && pair[0] == 6u << 16) { uint32_t to = g_hp1_to; if (pair[1] > to) *(uint32_t *)(sb + 8 * i + 4) = to; return; }
}
static void state_dump_monsters(uint64_t G, char *out, size_t cap, size_t *used) {
    int count = 0;
    int w = snprintf(out + *used, cap - *used, ",\"monsters\":["); if (w < 0 || (size_t)w >= cap - *used) return; *used += (size_t)w;
    for (int b = 0; b < 128 && count < 64; b++) {
        uint64_t u = 0; if (!tk_rd(G + 0x2630 + 8 * b, &u, 8)) continue;
        for (int k = 0; u && k < 4096 && count < 64; k++) {
            uint32_t id = 0, cls = 0, stat100 = 0, stat7 = 0;
            if (tk_rd(u + 4, &cls, 4) && tk_rd(u + 8, &id, 4) && stat_read(u, 100, &stat100) && stat_read(u, 7, &stat7)) {
                w = snprintf(out + *used, cap - *used, "%s{\"id\":%u,\"cls\":%u,\"stat100\":%u,\"stat7\":%u}", count ? "," : "", id, cls, stat100, stat7);
                if (w < 0 || (size_t)w >= cap - *used) return; *used += (size_t)w; count++;
            }
            if (!tk_rd(u + 0x158, &u, 8)) break;
        }
    }
    w = snprintf(out + *used, cap - *used, "]"); if (w >= 0 && (size_t)w < cap - *used) *used += (size_t)w;
}
static int tk_rd(uint64_t a, void *buf, size_t n) {
    if (!a) return 0;
    g_tkreads++; vm_size_t got = 0;
    { int dm_guest_heap_readable(uint64_t, size_t); if (dm_guest_heap_readable(a, n)) { memcpy(buf, (const void *)(uintptr_t)a, n); return 1; } }
    return !vm_read_overwrite(mach_task_self(), a, n, (vm_address_t)buf, &got) && got == n;
}
typedef struct { uint32_t type, cls, id, mode, act, seed, rlo, rhi, x, y, xf, yf, it[5], has_it, nst; uint32_t st[96][2]; unsigned ord; } tk_unit;
static int tk_cmp(const void *a, const void *b) {
    const tk_unit *u = a, *v = b;
    if (u->type != v->type) return u->type < v->type ? -1 : 1;
    if (u->id != v->id) return u->id < v->id ? -1 : 1;
    return u->ord < v->ord ? -1 : u->ord > v->ord;
}
static uint64_t tk_fnv(uint64_t h, const void *p, size_t n) { for (size_t i = 0; i < n; i++) h = (h ^ ((const uint8_t *)p)[i]) * 0x100000001b3ull; return h; }

typedef struct { uint32_t id, cls, mode, page, x, y; unsigned ord; } state_item;
static int state_item_cmp(const void *a, const void *b) {
    const state_item *u = a, *v = b;
    if (u->id != v->id) return u->id < v->id ? -1 : 1;
    return u->ord < v->ord ? -1 : u->ord > v->ord;
}
static int state_read_item(uint64_t u, state_item *it, size_t ord, uint64_t *next) {
    uint32_t id = 0, cls = 0, mode = 0, x = 0, y = 0; uint64_t d = 0, pp = 0; uint8_t page = 0;
    if (!tk_rd(u + 8, &id, 4) || !tk_rd(u + 4, &cls, 4) || !tk_rd(u + 0xC, &mode, 4) || !tk_rd(u + 0x10, &d, 8) || !d) return 0;
    if (!tk_rd(d + 0x55, &page, 1)) return 0;
    if (tk_rd(u + 0x38, &pp, 8) && pp) { tk_rd(pp + 0x10, &x, 4); tk_rd(pp + 0x14, &y, 4); }
    *it = (state_item){id, cls, mode, page, x, y, (unsigned)ord};
    *next = 0; tk_rd(d + 0xB0, next, 8);
    return 1;
}
enum { CLS_TBK = 533, CLS_TSC = 544 };
static uint64_t g_player_unit;










_Atomic int g_dm_seek_tp; int g_dm_seek_tp_x, g_dm_seek_tp_y;
static void seek_tp_step(uint64_t G, uint64_t pl, module_t *main) {
    static struct { uint64_t r; int x, y, w, h; uint8_t st, miss; } rm[400]; static int n, hops, bias_on = -1, bias_in, bx, by; static uint64_t lv0; static uint32_t next_tk;
    int mode = atomic_load(&g_dm_seek_tp); uint32_t tk = 0; if (!mode || mode == 2 || !tk_rd(G + 0x170, &tk, 4) || tk < next_tk) return; next_tk = tk + 6;
    int (*tp)(uint64_t, uint64_t, uint64_t, int, int, int, int) = (int (*)(uint64_t, uint64_t, uint64_t, int, int, int, int))(main->base + 0x7cc530);
    uint64_t pp = 0, ar = 0, dr = 0, lv = 0; uint16_t c[4] = {0}; if (!tk_rd(pl + 0x38, &pp, 8) || !pp || !tk_rd(pp, c, 8) || !tk_rd(pp + 0x20, &ar, 8) || !ar || !tk_rd(ar + 0x18, &dr, 8) || !dr || !tk_rd(dr + 0x90, &lv, 8) || !lv) return;
    int px = c[1], py = c[3];
    if (mode == 3) { int ok = tp(G, pl, 0, g_dm_seek_tp_x, g_dm_seek_tp_y, 0, 0); tk_rd(pp, c, 8); LOG("传送探路：拍 %u 传到目标 %d,%d 旁：%s，现在 %d,%d（共跳 %d 次）", tk, g_dm_seek_tp_x, g_dm_seek_tp_y, ok ? "成" : "不成（目标不在当前或相邻房间 / 旁边没空位）", c[1], c[3], hops); atomic_store(&g_dm_seek_tp, 2); return; }
    if (bias_on < 0) { const char *e = getenv("DM_TEST_SEEK_BIAS"); bias_on = e && sscanf(e, "%d,%d", &bx, &by) == 2; }
    if (lv != lv0) { lv0 = lv; n = 0; hops = 0; uint64_t r = 0; tk_rd(lv + 0x10, &r, 8); while (r && n < 400) { int q[4] = {0}; if (!tk_rd(r + 0x60, q, 16)) break; rm[n].r = r; rm[n].x = q[0] * 5; rm[n].y = q[1] * 5; rm[n].w = q[2] * 5; rm[n].h = q[3] * 5; rm[n].miss = 0; rm[n++].st = 0; if (!tk_rd(r + 0x48, &r, 8)) break; }
        bias_in = 0; for (int i = 0; i < n && bias_on; i++) bias_in |= bx >= rm[i].x && bx < rm[i].x + rm[i].w && by >= rm[i].y && by < rm[i].y + rm[i].h;
        uint32_t id = 0; tk_rd(lv + 0x1f8, &id, 4); LOG("传送探路：拍 %u 关卡 %u，关卡房间 %d 间，玩家 %d,%d%s", tk, id, n, px, py, bias_in ? "，按偏向点选房" : ""); }
    int ci = -1; for (int i = 0; i < n; i++) if (rm[i].r == dr) ci = i; if (ci < 0) return; if (rm[ci].st != 2) rm[ci].st = 2;


    static short par[400], que[400]; int qh = 0, qt = 0, best = -1; long bd = -1; for (int i = 0; i < n; i++) par[i] = -1; par[ci] = (short)ci; que[qt++] = (short)ci;
    while (qh < qt) { int a = que[qh++]; if (rm[a].st == 0) { long d = bias_in ? (long)(rm[a].x + rm[a].w / 2 - bx) * (rm[a].x + rm[a].w / 2 - bx) + (long)(rm[a].y + rm[a].h / 2 - by) * (rm[a].y + rm[a].h / 2 - by) : qh; if (bd < 0 || d < bd) { bd = d; best = a; } }
        for (int j = 0; j < n; j++) { if (par[j] >= 0 || rm[j].st == 3) continue; int ox = rm[a].x < rm[j].x + rm[j].w && rm[j].x < rm[a].x + rm[a].w, oy = rm[a].y < rm[j].y + rm[j].h && rm[j].y < rm[a].y + rm[a].h;
            int tx = rm[a].x + rm[a].w == rm[j].x || rm[j].x + rm[j].w == rm[a].x, ty = rm[a].y + rm[a].h == rm[j].y || rm[j].y + rm[j].h == rm[a].y; if ((tx && oy) || (ty && ox)) { par[j] = (short)a; que[qt++] = (short)j; } } }
    if (best < 0) { int v = 0, bad = 0; for (int i = 0; i < n; i++) { v += rm[i].st == 2; bad += rm[i].st == 3; } LOG("传送探路：拍 %u 跳得到的房间都去过了（%d 间里去过 %d、跳不进 %d，共跳 %d 次），没等到目标", tk, n, v, bad, hops); atomic_store(&g_dm_seek_tp, 0); return; }
    int step = best; while (par[step] != ci) step = par[step];
    static const int8_t at[5][2] = {{2, 2}, {1, 1}, {3, 1}, {1, 3}, {3, 3}}; int ok = 0, hx = 0, hy = 0; for (int k = 0; k < 5 && !ok; k++) { hx = rm[step].x + rm[step].w * at[k][0] / 4; hy = rm[step].y + rm[step].h * at[k][1] / 4; ok = tp(G, pl, 0, hx, hy, 0, 0); }
    if (!ok) { rm[step].st = 3; LOG("传送探路：拍 %u 跳不进房 (%d,%d) %dx%d（五个落点都不成），记成跳不进", tk, rm[step].x, rm[step].y, rm[step].w, rm[step].h); return; }
    hops++; tk_rd(pp, c, 8);


    int inroom = c[1] >= rm[step].x && c[1] < rm[step].x + rm[step].w && c[3] >= rm[step].y && c[3] < rm[step].y + rm[step].h; if (!inroom && ++rm[step].miss >= 2) rm[step].st = 3;
    if (hops <= 40 || hops % 20 == 0 || !inroom) LOG("传送探路：拍 %u 第 %d 跳 → 房 (%d,%d)，落在 %d,%d%s（目标房 (%d,%d)）", tk, hops, rm[step].x, rm[step].y, c[1], c[3], inroom ? "" : rm[step].st == 3 ? "，两次都没落进去，记成跳不进" : "，没落进这间", rm[best].x, rm[best].y);
}












static struct { uint64_t G; uint32_t at; int n0; char before[400]; } g_cube;
static int cube_items(uint64_t player, char *o, int cap) {
    module_t *mm = main_mod(); uint64_t inv = 0, it = 0; int n = 0, k = 0; o[0] = 0; if (!mm || !tk_rd(player + 0x90, &inv, 8) || !inv || !tk_rd(inv + 0x10, &it, 8)) return -1;
    for (int c = 0; it && c < 400; c++) { uint64_t d = 0; if (!tk_rd(it + 0x10, &d, 8) || !d) break;
        if (*(uint8_t *)(uintptr_t)(d + 0x55) == 3) { uint32_t code = ((uint32_t (*)(uint64_t))(mm->base + 0x570260))(it); char cs[5]; memcpy(cs, &code, 4); cs[4] = 0; for (int i = 3; i >= 0 && cs[i] == ' '; i--) cs[i] = 0;
            uint64_t pp = 0; int32_t gx = -1, gy = -1; uint32_t id = 0, md = 0; tk_rd(it + 8, &id, 4); tk_rd(it + 0xc, &md, 4); if (tk_rd(it + 0x38, &pp, 8) && pp) { tk_rd(pp + 0x10, &gx, 4); tk_rd(pp + 0x14, &gy, 4); }
            k += snprintf(o + k, cap - k, "%sid%u@%d,%d/%s/品质%u/来源难度%d/模式%u", n ? " " : "", id, gx, gy, cs, *(uint32_t *)(uintptr_t)d, ((int (*)(uint64_t, int, int))(mm->base + 0x5b9290))(it, 356, 0), md); if (k >= cap) k = cap - 1; n++; }
        if (!tk_rd(d + 0xb0, &it, 8)) break; }
    return n;
}
static void cube_diag_req(uint64_t G, uint64_t player, const uint8_t *pkt, uint32_t len) {
    if (!G || !player) return;
    uint64_t inv = 0, cur = 0, pd = 0, qr = 0, q = 0, ca = 0, cb = 0, cc = 0; uint32_t pm = 0, tq = 0, cn = 0; uint8_t diff = 0, xp = 0, opened = 0xff;
    tk_rd(G + 0x170, &tq, 4); tk_rd(G + 0x104, &diff, 1); tk_rd(G + 0x106, &xp, 1); tk_rd(player + 0xc, &pm, 4);
    if (tk_rd(player + 0x90, &inv, 8) && inv) tk_rd(inv + 0x40, &cur, 8);
    if (tk_rd(player + 0x10, &pd, 8) && pd && tk_rd(pd + 0x40 + 8 * diff, &qr, 8) && qr) tk_rd(qr, &q, 8);
    uint16_t w4 = 0, w26 = 0, w40 = 0; if (q) { tk_rd(q + 8, &w4, 2); tk_rd(q + 52, &w26, 2); tk_rd(q + 80, &w40, 2); }
    if (tk_rd(G + 0x21d8, &ca, 8) && ca && tk_rd(ca + 0xc, &cn, 4) && cn && tk_rd(ca + 0x10, &cb, 8) && cb && tk_rd(cb, &cc, 8) && cc) tk_rd(cc + 9, &opened, 1);
    char items[400]; int n = cube_items(player, items, sizeof items), lv = 0; { uint64_t pp = 0, r = 0, r2 = 0, l = 0; if (tk_rd(player + 0x38, &pp, 8) && pp && tk_rd(pp + 0x20, &r, 8) && r && tk_rd(r + 0x18, &r2, 8) && r2 && tk_rd(r2 + 0x90, &l, 8) && l) tk_rd(l + 0x1f8, &lv, 4); }


    char hx[160]; int k = 0; uint32_t bid = 0; if (len >= 8) memcpy(&bid, pkt + 1, 4);
    k += snprintf(hx + k, sizeof hx - k, "魔盒 id%u 原料 %d 项：", bid, len >= 8 ? pkt[7] + 1 : -1);
    for (uint32_t i = 8; i + 5 <= len && k < (int)sizeof hx - 24; i += 5) { uint32_t id; memcpy(&id, pkt + i, 4); k += snprintf(hx + k, sizeof hx - k, " id%u@%d,%d", id, pkt[i + 4] & 15, pkt[i + 4] >> 4); }
    LOG("合成读数：拍 %u 服务端收到合成请求（%s；长 %u）｜关卡 %d 难度 %u 版本字节 %u 玩家模式 %u｜光标物品 %#llx%s｜魔盒 %d 件：%s｜任务字 4=%#06x 26=%#06x 40=%#06x｜本局已开门字节 %s%#x",
        tq, hx, len, lv, diff, xp, pm, (unsigned long long)cur, cur ? "（非空：原版 0x6e8740 会丢掉这次合成）" : "（空）", n, items, w4, w26, w40, opened == 0xff ? "读不到 " : "", opened == 0xff ? 0 : opened);
    g_cube.G = G; g_cube.at = tq + 50; g_cube.n0 = n; strlcpy(g_cube.before, items, sizeof g_cube.before);
}
static void cube_diag_tick(uint64_t G, uint64_t player) {
    uint32_t tq = 0; if (!g_cube.at || G != g_cube.G || !player || !tk_rd(G + 0x170, &tq, 4) || (int32_t)(tq - g_cube.at) < 0) return;
    g_cube.at = 0; char items[400]; int n = cube_items(player, items, sizeof items), red = 0;
    for (int b = 0; b < 128; b++) { uint64_t u = 0; if (!tk_rd(G + 0x2a30 + 8 * b, &u, 8)) continue; for (int c = 0; u && c < 512; c++) { uint32_t cl = 0; tk_rd(u + 4, &cl, 4); red += cl == 60; if (!tk_rd(u + 0x158, &u, 8)) break; } }
    LOG("合成读数：拍 %u 合成后 2 秒｜魔盒 %d → %d 件%s：%s｜本局红门（物件 60）%d 扇", tq, g_cube.n0, n, strcmp(items, g_cube.before) ? "（变了）" : "（没变：服务端没执行配方）", items, red);
}







static uint32_t local_player_id(void) { uint32_t slot = 8, lid = 0xffffffffu; module_t *mm = main_mod(); if (mm && tk_rd((uint64_t)mm->base + 0x2d20e4c, &slot, 4) && slot < 8) tk_rd((uint64_t)mm->base + 0x2d60380 + 4 * slot, &lid, 4); return lid; }
#ifndef DM_PLAYER_BUILD





#define GIVE_STATS_MAX 16
struct give_stat { int st, ly, v, read; };
static struct { uint32_t id; struct give_stat e[GIVE_STATS_MAX]; int n, rec; } give_stats[4];
static int give_stats_n;
static uint32_t give_stats_next[4];
static int give_stats_parse(const char *list, struct give_stat *e) {
    char sb[200]; strlcpy(sb, list, sizeof sb); int n = 0;
    for (char *sv, *t = strtok_r(sb, "+", &sv); t; t = strtok_r(NULL, "+", &sv)) { int st = -1, ly = 0, v = 0;
        if (sscanf(t, "%d/%d=%d", &st, &ly, &v) != 3 && (ly = 0, sscanf(t, "%d=%d", &st, &v) != 2)) { LOG("DM_TEST_GIVE_STATS: bad entry %s", t); continue; }
        if (n < GIVE_STATS_MAX) { e[n].st = st; e[n].ly = ly; e[n].v = v; n++; } }
    return n;
}
static void give_stats_apply(uint8_t *b, uint64_t unit, struct give_stat *e, int n, const char *side) {
    for (int i = 0; i < n; i++) {
        ((void (*)(uint64_t, int, int, int))(b + 0x5bc1f0))(unit, e[i].st, e[i].v, e[i].ly);
        e[i].read = ((int (*)(uint64_t, int, int))(b + 0x5b9290))(unit, e[i].st, e[i].ly);
        LOG("DM_TEST_GIVE_STATS: %s item %u stat %d/%d = %d (reads %d)", side, *(uint32_t *)(unit + 8), e[i].st, e[i].ly, e[i].v, e[i].read); }
}
void dm_test_give_stats_server(uint8_t *b, uint64_t unit, const char *code) {
    const char *env = getenv("DM_TEST_GIVE_STATS"); char sb[256]; size_t n = strlen(code);
    if (!env || __atomic_load_n(&give_stats_n, __ATOMIC_ACQUIRE) >= 4) return;
    strlcpy(sb, env, sizeof sb);
    for (char *sv, *e = strtok_r(sb, ";", &sv); e; e = strtok_r(NULL, ";", &sv)) {
        if (strncmp(e, code, n) || e[n] != ':') continue;
        int k = give_stats_n;
        if (!(give_stats[k].n = give_stats_parse(e + n + 1, give_stats[k].e))) return;
        give_stats_apply(b, unit, give_stats[k].e, give_stats[k].n, "server");
        give_stats[k].id = *(uint32_t *)(unit + 8); give_stats[k].rec = 0;
        __atomic_store_n(&give_stats_n, k + 1, __ATOMIC_RELEASE); return;
    }
}

void dm_test_give_stats_client(uint8_t *b) {
    int n = __atomic_load_n(&give_stats_n, __ATOMIC_ACQUIRE); uint32_t now = cur_tick();
    for (int k = 0; k < n; k++) {
        if ((int32_t)(now - give_stats_next[k]) < 0) continue;
        give_stats_next[k] = now + 25;
        uint32_t id = give_stats[k].id;
        uint64_t u = ((uint64_t (*)(uint64_t, uint32_t, uint32_t, uint32_t))(b + 0x45f5f0))((uint64_t)(uintptr_t)(b + 0x2d603a0 + 4 * 0x400), id & 0x7f, id, 4);
        if (!u || *(uint32_t *)(u + 0xc) != 0) continue;
        int drift = !give_stats[k].rec;
        for (int i = 0; !drift && i < give_stats[k].n; i++)
            if (((int (*)(uint64_t, int, int))(b + 0x5b9290))(u, give_stats[k].e[i].st, give_stats[k].e[i].ly) != give_stats[k].e[i].read) drift = 1;
        if (drift) { give_stats_apply(b, u, give_stats[k].e, give_stats[k].n, "client"); give_stats[k].rec = 1; } }
}
#endif

static void hud_tick(uint64_t G) {
    dm_hud_info h = {0};
    uint64_t belt_item[4] = { 0 };
    uint64_t player = 0, inv = 0, u = 0, next = 0;










    uint32_t lid = local_player_id();
    for (int b = 0; b < 128 && !player; b++) {
        if (!tk_rd(G + 0x2230 + 8 * b, &u, 8)) continue;
        for (int k = 0; u && k < 4096; k++) {
            uint32_t mode = 0, id = 0; uint64_t pp = 0; uint16_t c[4] = {0};
            if (tk_rd(u + 0xC, &mode, 4) && mode && tk_rd(u + 0x38, &pp, 8) && pp && tk_rd(pp, c, 8) && (c[1] || c[3]) && (lid == 0xffffffffu || (tk_rd(u + 8, &id, 4) && id == lid))) { player = u; break; }
            if (!tk_rd(u + 0x158, &u, 8)) break;
        }
    }
    uint32_t sig = 0;
    { void dm_looks_tick(uint64_t, uint64_t); dm_looks_tick(G, player); }
    if (player && tk_rd(player + 0x90, &inv, 8) && tk_rd(inv, &sig, 4) && sig == 0x01020304) {
        { static int hp1 = -1; if (hp1 < 0) { const char *e = getenv("DM_TEST_HP1"); hp1 = e != NULL; if (e && e[0] == '0') g_hp1_to = 0; } if (hp1 && cur_tick() > 200) test_hp1(player); }
        uint32_t lv = 0, xp = 0;
        stat_read(player, 12, &lv); stat_read(player, 13, &xp);




        module_t *mm = main_mod();
        if (mm) { int (*get)(uint64_t, int, int) = (int (*)(uint64_t, int, int))(mm->base + 0x5b9290);
            h.hp = get(player, 6, 0) >> 8; h.hp_max = get(player, 7, 0) >> 8; h.mp = get(player, 8, 0) >> 8; h.mp_max = get(player, 9, 0) >> 8; }
        uint32_t sp = 0, kp = 0, pm = 0; stat_read(player, 4, &sp); stat_read(player, 5, &kp); h.points = (int)(sp + kp);
        if (tk_rd(player + 0xC, &pm, 4)) h.dead = dm_hud_player_mode_dead(pm);




        { uint64_t cur = 0; tk_rd(inv + 0x40, &cur, 8);
          static uint64_t lcur; if (cur != lcur) { uint32_t id = 0; if (cur) tk_rd(cur + 8, &id, 4); LOG("手上物品（服务端）：%s（物品 id %u）", cur ? "拿着" : "空了", id); lcur = cur; }
          static uint64_t mcur; static int mn; if (cur && !dm_hud_held()) { if (++mn == 50 && cur != mcur) { uint32_t id = 0; tk_rd(cur + 8, &id, 4); mcur = cur; LOG("【红】手上物品：服务端光标挂着物品（id %u）、客户端手上已空 2 秒——两端不一致，按客户端（手上空）处理", id); } } else mn = 0; }
        h.level = (int)lv;
        { uint64_t pp = 0, r1 = 0, r2 = 0, lvl = 0; uint16_t c[4] = {0}; uint32_t no = 0;
          if (tk_rd(player + 0x38, &pp, 8) && tk_rd(pp, c, 8)) { h.x = c[1]; h.y = c[3]; }
          atomic_store(&g_player_path, pp);


          if (pp && tk_rd(pp + 0x20, &r1, 8) && tk_rd(r1 + 0x18, &r2, 8) && tk_rd(r2 + 0x90, &lvl, 8) && tk_rd(lvl + 0x1f8, &no, 4) && no < 200) h.area = (int)no; }
        if (lv >= 99) h.exp_permille = 1000;
        else if (lv >= 1) {
            uint32_t lo = k_exp[lv - 1], hi = k_exp[lv];
            h.exp_permille = hi > lo && xp > lo ? (int)((uint64_t)(xp - lo) * 1000 / (hi - lo)) : 0;
            if (h.exp_permille > 1000) h.exp_permille = 1000;
        }
        state_item it;
        if (tk_rd(inv + 0x10, &u, 8))
            for (unsigned k = 0; u && k < 1024 && state_read_item(u, &it, 0, &next); k++) {
                if (it.mode == 2) { h.belt[it.x % 4]++; if (it.x < 4) belt_item[it.x] = u; }
                if (it.cls == CLS_TSC) h.portal++;
                if (it.cls == CLS_TBK) { uint32_t q = 0; if (stat_read(u, 70, &q)) h.portal += (int)q; }
                u = next;
            }
        h.valid = 1;
    }
    module_t *main = main_mod(); uint32_t pl = 0;
    if (main) { void *obj = players_obj(main); if (obj && tk_rd((uint64_t)obj + 0xc, &pl, 4)) h.players = (int)pl; }
    static int logged;
    { static uint64_t eg; static uint32_t et; uint32_t t = 0; tk_rd(G + 0x170, &t, 4); if (G != eg || t < et) { void dm_diag_new_run(void); dm_diag_new_run(); } eg = G; et = t; }
    g_player_unit = player; { uint8_t gv; extern _Atomic int g_dm_game_ver; if (tk_rd(G + 0x106, &gv, 1)) atomic_store(&g_dm_game_ver, gv); }
    { uint32_t tq = 0; static int once; const char *tg = getenv("DM_TEST_GOLD"); if (tg && main && player && tk_rd(G + 0x170, &tq, 4) && tq >= 30 && !once++) ((void (*)(uint64_t, int, int, int))(main->base + 0x5bc1f0))(player, 14, atoi(tg), 0); }




#ifndef DM_PLAYER_BUILD
    { static char q[300]; static int init; uint32_t tq = 0; if (!init) { init = 1; const char *e = getenv("DM_TEST_GOTO_LEVEL"); if (e) strlcpy(q, e, sizeof q); }
      if (q[0] && main && player && tk_rd(G + 0x170, &tq, 4)) { unsigned at = 0, to = 0; int nn = 0;
          if (sscanf(q, "%u:%u%n", &at, &to, &nn) >= 2 && tq >= at) { const char *rest = q + nn + (q[nn] == ';'); memmove(q, rest, strlen(rest) + 1);
              ((void (*)(uint64_t, uint64_t, int, int))(main->base + 0x634000))(G, player, (int)to, 11); LOG("DM_TEST_GOTO_LEVEL：拍 %u 换到关卡 %u", tq, to); } } }
#endif


    { static int at = -1; uint32_t tq = 0; if (at < 0) { const char *e = getenv("DM_TEST_MAP_FULL"); at = e ? atoi(e) : 0; }
      if (at > 0 && main && player && tk_rd(G + 0x170, &tq, 4) && tq >= (uint32_t)at) { at = 0; void dm_ui_map_full(int); dm_ui_map_full(1); LOG("DM_TEST_MAP_FULL：拍 %u 切全屏地图", tq); } }









#ifndef DM_PLAYER_BUILD
    { static uint32_t at[8]; static int n = -1, used; uint32_t tq = 0, pid = 0, mode = 0; const char *tk = getenv("DM_TEST_KILL");
      if (n < 0) { n = 0; char buf[128]; if (tk) { strlcpy(buf, tk, sizeof buf); for (char *sv, *t = strtok_r(buf, ",", &sv); t && n < 8; t = strtok_r(NULL, ",", &sv)) { char *e; unsigned long v = strtoul(t, &e, 10); if (e != t && (!n || v >= at[n - 1])) at[n++] = (uint32_t)v; } } }
      if (used < n && main && player && tk_rd(G + 0x170, &tq, 4) && tq >= at[used] && tk_rd(player + 0xc, &mode, 4) && !dm_hud_player_mode_dead(mode) && tk_rd(player + 8, &pid, 4)) { used++;
          int r = ((int (*)(uint64_t, uint64_t, uint64_t, int, int, uint32_t, int, int))(main->base + 0x6cd2c0))(G, player, 0, 0, 0, pid, 0, 0); LOG("DM_TEST_KILL：拍 %u 对玩家 id %u 开始死亡模式 → %d（第 %d / %d 次）", tq, pid, r, used, n); } }
#endif





#ifndef DM_PLAYER_BUILD
    { static uint32_t at[8], follow; static int amount[8], n = -1, used; uint32_t tq = 0;
      if (n < 0) { n = 0; const char *e = getenv("DM_TEST_XP_GRANT"); char buf[160];
          if (e) { strlcpy(buf, e, sizeof buf); for (char *sv, *t = strtok_r(buf, ",", &sv); t && n < 8; t = strtok_r(NULL, ",", &sv)) { unsigned a = 0; int v = 0;
              if (sscanf(t, "%u:%d", &a, &v) == 2 && v > 0 && (!n || a > at[n - 1])) { at[n] = a; amount[n++] = v; } else LOG("ASSUME xp.fixture: invalid entry %s", t); } } }
      if ((used < n || follow) && main && player && tk_rd(G + 0x170, &tq, 4)) {
          int (*get)(uint64_t, int, int) = (int (*)(uint64_t, int, int))(main->base + 0x5b9290);
          if (used < n && tq >= at[used]) {
              int b[5] = { get(player, 12, 0), get(player, 13, 0), get(player, 4, 0), get(player, 5, 0), get(player, 7, 0) };
              ((void (*)(uint64_t, uint64_t, int))(main->base + 0x7ddc40))(G, player, amount[used]);
              int a[6] = { get(player, 12, 0), get(player, 13, 0), get(player, 4, 0), get(player, 5, 0), get(player, 7, 0), get(player, 29, 0) };
              LOG("[xp-grant] {\"tick\":%u,\"amount\":%d,\"level\":[%d,%d],\"exp\":[%u,%u],\"statpts\":[%d,%d],\"skillpts\":[%d,%d],\"maxlife\":[%d,%d],\"lastexp\":%u}",
                  tq, amount[used], b[0], a[0], (uint32_t)b[1], (uint32_t)a[1], b[2], a[2], b[3], a[3], b[4] >> 8, a[4] >> 8, (uint32_t)a[5]);
              used++; follow = tq + 25; }
          else if (follow && tq >= follow) { follow = 0; uint32_t lid = local_player_id(); uint64_t c = 0;
              if (lid != 0xffffffffu) for (uint64_t v = *(volatile uint64_t *)(main->base + 0x2d603a0 + 8 * (lid & 0x7f)); v && !c; v = *(volatile uint64_t *)(v + 0x158)) if (*(volatile uint32_t *)(v + 8) == lid) c = v;
              if (c) LOG("[xp-grant-client] {\"tick\":%u,\"level\":%d,\"exp\":%u}", tq, get(c, 12, 0), (uint32_t)get(c, 13, 0)); else LOG("[xp-grant-client] {\"tick\":%u,\"missing\":1}", tq); } } }
#endif
    { uint32_t tq = 0; static int once2; const char *ts = getenv("DM_TEST_STAT");
      if (ts && main && player && tk_rd(G + 0x170, &tq, 4) && tq >= 30 && !once2++) { char buf[128]; strlcpy(buf, ts, sizeof buf); for (char *sv, *t = strtok_r(buf, ",", &sv); t; t = strtok_r(NULL, ",", &sv)) { int a = 0, v = 0; if (sscanf(t, "%d=%d", &a, &v) == 2) ((void (*)(uint64_t, int, int, int))(main->base + 0x5bc1f0))(player, a, v, 0); } } }




    { static int set_done, lhp = -1, lmx = -1; static uint32_t lt; static const char *e; static int e_got; if (!e_got) { e_got = 1; e = getenv("DM_TEST_MERC_HP"); }                                                                                           uint32_t tq = 0;
      if (e && main && player && tk_rd(G + 0x170, &tq, 4)) { int pct = 50, at = 60; sscanf(e, "%d@%d", &pct, &at); uint64_t merc = 0;
          for (int b = 0; b < 128 && !merc; b++) { uint64_t u = 0; if (!tk_rd(G + 0x2630 + 8 * b, &u, 8)) continue;
              for (int k = 0; u && k < 4096; k++) { uint32_t cls = 0, mode = 0; if (tk_rd(u + 4, &cls, 4) && tk_rd(u + 0xC, &mode, 4) && (cls == 271 || cls == 338 || cls == 359 || cls == 560 || cls == 561) && mode != 0 && mode != 12) { merc = u; break; } if (!tk_rd(u + 0x158, &u, 8)) break; } }
          if (merc) { int (*get)(uint64_t, int, int) = (int (*)(uint64_t, int, int))(main->base + 0x5b9290); int hp = get(merc, 6, 0), mx = get(merc, 7, 0);
              if (!set_done && tq >= (uint32_t)at) { set_done = 1; ((void (*)(uint64_t, int, int, int))(main->base + 0x5bc1f0))(merc, 6, (int)((long long)mx * pct / 100), 0); hp = get(merc, 6, 0); LOG("DM_TEST_MERC_HP：拍 %u 把佣兵生命设成上限的 %d%% → %d/%d", tq, pct, hp >> 8, mx >> 8); lt = 0; }




              { static int32_t lc = -12345; uint64_t md = 0; int32_t cv = 0; if (tk_rd(merc + 0x10, &md, 8) && md && tk_rd(md + 0x54, &cv, 4) && cv != lc) { if (lc != -12345) LOG("DM_TEST_MERC_HP：拍 %u 佣兵「上次发过的百分比」%d → %d", tq, lc, cv); lc = cv; } }
              if ((hp != lhp || mx != lmx) && (!lt || tq - lt >= 25)) { uint32_t cls = 0, id = 0; tk_rd(merc + 4, &cls, 4); tk_rd(merc + 8, &id, 4); LOG("DM_TEST_MERC_HP：拍 %u 佣兵 类号 %u id %u 生命 %d/%d", tq, cls, id, hp >> 8, mx >> 8); lhp = hp; lmx = mx; lt = tq; } } } }


#ifndef DM_PLAYER_BUILD
    { static int done; static uint64_t km; static uint32_t kid; static int ls = -1, lc = -1; static const char *e; static int e_got; if (!e_got) { e_got = 1; e = getenv("DM_TEST_MERC_KILL"); }                                                                                           uint32_t tq = 0;
      if (done && main && tk_rd(G + 0x170, &tq, 4)) { uint32_t id = 0, sm = 99, cm = 99;
          if (tk_rd(km + 8, &id, 4) && id == kid) tk_rd(km + 0xC, &sm, 4); else sm = 99;
          for (uint64_t u = *(uint64_t *)(main->base + 0x2d603a0 + 1 * 0x400 + (kid & 0x7f) * 8); u; u = *(uint64_t *)(u + 0x158)) if (*(uint32_t *)(u + 8) == kid) { cm = *(uint32_t *)(u + 0xC); break; }
          if ((int)sm != ls || (int)cm != lc) { LOG("DM_TEST_MERC_KILL：拍 %u 佣兵 id %u 模式 服务端 %u 客户端 %u", tq, kid, sm, cm); ls = (int)sm; lc = (int)cm; } }
      if (e && !done && main && player && tk_rd(G + 0x170, &tq, 4) && tq >= (uint32_t)atoi(e)) { uint64_t merc = 0;
          for (int b = 0; b < 128 && !merc; b++) { uint64_t u = 0; if (!tk_rd(G + 0x2630 + 8 * b, &u, 8)) continue;
              for (int k = 0; u && k < 4096; k++) { uint32_t cls = 0, mode = 0; if (tk_rd(u + 4, &cls, 4) && tk_rd(u + 0xC, &mode, 4) && (cls == 271 || cls == 338 || cls == 359 || cls == 560 || cls == 561) && mode != 0 && mode != 12) { merc = u; break; } if (!tk_rd(u + 0x158, &u, 8)) break; } }
          if (merc) { done = 1; uint32_t cls = 0, id = 0, m0 = 0, m1 = 0; tk_rd(merc + 4, &cls, 4); tk_rd(merc + 8, &id, 4); km = merc; kid = id; tk_rd(merc + 0xC, &m0, 4);
              ((void (*)(uint64_t, uint64_t, uint64_t, int))(main->base + 0x7d82b0))(G, merc, 0, 1); tk_rd(merc + 0xC, &m1, 4);
              LOG("DM_TEST_MERC_KILL：拍 %u 杀佣兵 类号 %u id %u，模式 %u → %u（0 = 死亡中，12 = 已死）", tq, cls, id, m0, m1); } } }
#endif




    { static int n = -1, delay = 50, ns; static uint32_t cls[16]; static struct { uint32_t id, t0; } seen[64]; static int rr; uint32_t tq = 0;
      if (n < 0) { n = 0; const char *e = getenv("DM_TEST_MON_KILL"); char buf[160]; if (e) { strlcpy(buf, e, sizeof buf); char *at = strchr(buf, '@'); if (at) { *at = 0; delay = atoi(at + 1); } for (char *sv, *t = strtok_r(buf, ",", &sv); t && n < 16; t = strtok_r(NULL, ",", &sv)) cls[n++] = (uint32_t)atoi(t); } }
      if (n && main && player && tk_rd(G + 0x170, &tq, 4) && tq % 5 == 0)
          for (int b = 0; b < 128; b++) { uint64_t u = 0; if (!tk_rd(G + 0x2630 + 8 * b, &u, 8)) continue;
              for (int k = 0; u && k < 4096; k++) { uint32_t c = 0, mode = 0, id = 0; uint64_t nx = 0; tk_rd(u + 0x158, &nx, 8); int hit = 0; if (tk_rd(u + 4, &c, 4) && tk_rd(u + 0xC, &mode, 4) && tk_rd(u + 8, &id, 4)) for (int i = 0; i < n; i++) hit |= cls[i] == c;
                  if (hit && mode != 0 && mode != 12) { int si = -1; for (int i = 0; i < ns; i++) if (seen[i].id == id) si = i; if (si < 0) { si = ns < 64 ? ns++ : rr++ % 64; seen[si].id = id; seen[si].t0 = tq; }
                      if (si >= 0 && tq - seen[si].t0 >= (uint32_t)delay) { ((void (*)(uint64_t, uint64_t, uint64_t, int))(main->base + 0x7d82b0))(G, u, player, 1); uint32_t m1 = 99; tk_rd(u + 0xC, &m1, 4); LOG("DM_TEST_MON_KILL：拍 %u 杀怪 类号 %u id %u，模式 %u → %u", tq, c, id, mode, m1); seen[si].t0 = tq; } }
                  u = nx; } } }





    { static int cls_n = -1; static uint32_t cls[8], seen[32]; static uint8_t full[8]; static int ns; uint32_t tq = 0;
      if (cls_n < 0) { cls_n = 0; const char *e = getenv("DM_TEST_MON_HP"); char buf[96]; if (e) { strlcpy(buf, e, sizeof buf); for (char *sv, *t = strtok_r(buf, ",", &sv); t && cls_n < 8; t = strtok_r(NULL, ",", &sv)) { full[cls_n] = strchr(t, '+') != NULL; cls[cls_n++] = (uint32_t)atoi(t); } } }
      if (cls_n > 0 && main && player && tk_rd(G + 0x170, &tq, 4) && tq % 5 == 0) { int (*get)(uint64_t, int, int) = (int (*)(uint64_t, int, int))(main->base + 0x5b9290);
          for (int b = 0; b < 128; b++) { uint64_t u = 0; if (!tk_rd(G + 0x2630 + 8 * b, &u, 8)) continue;
              for (int k = 0; u && k < 4096; k++) { uint32_t c = 0, mode = 0, id = 0; int hit = 0, top = 0; if (tk_rd(u + 4, &c, 4) && tk_rd(u + 0xC, &mode, 4) && tk_rd(u + 8, &id, 4)) { for (int i = 0; i < cls_n; i++) if (cls[i] == c) { hit = 1; top = full[i]; }
                      if (hit && mode != 0 && mode != 12) { int hp = get(u, 6, 0), want = top ? 5000 << 8 : 256; if (top ? hp < want : hp > 256) { if (top && get(u, 7, 0) < want) ((void (*)(uint64_t, int, int, int))(main->base + 0x5bc1f0))(u, 7, want, 0); ((void (*)(uint64_t, int, int, int))(main->base + 0x5bc1f0))(u, 6, want, 0);
                              int old = 0; for (int i = 0; i < ns; i++) old |= seen[i] == id; if (!old) { if (ns < 32) seen[ns++] = id; LOG("DM_TEST_MON_HP：拍 %u 怪 类号 %u id %u 生命 %d/%d → %d", tq, c, id, hp >> 8, get(u, 7, 0) >> 8, want >> 8); } } } }
                  if (!tk_rd(u + 0x158, &u, 8)) break; } } } }




    { static int set_done; static struct { uint32_t id; int dur; } last[16]; static int nl; static const char *e; static int e_got; if (!e_got) { e_got = 1; e = getenv("DM_TEST_DUR"); }                                                                                           uint32_t tq = 0; uint64_t iv = 0, u = 0, next = 0; uint32_t sg = 0;
      if (e && main && player && tk_rd(G + 0x170, &tq, 4) && tk_rd(player + 0x90, &iv, 8) && tk_rd(iv, &sg, 4) && sg == 0x01020304 && tk_rd(iv + 0x10, &u, 8)) { int v = 1, at = 60; sscanf(e, "%d@%d", &v, &at);
          int (*get)(uint64_t, int, int) = (int (*)(uint64_t, int, int))(main->base + 0x5b9290); int doset = !set_done && tq >= (uint32_t)at; extern _Atomic int g_dm_dur_sync_n; extern uint32_t g_dm_dur_sync[16][2]; int ns = 0; state_item it;
          for (unsigned k = 0; u && k < 1024 && state_read_item(u, &it, 0, &next); k++, u = next) { if (it.mode != 1) continue; int mx = get(u, 73, 0), dur = get(u, 72, 0); if (mx <= 0) continue;
              if (doset) { int nv = v < mx ? v : mx; ((void (*)(uint64_t, int, int, int))(main->base + 0x5bc1f0))(u, 72, nv, 0); dur = get(u, 72, 0); if (ns < 16) { g_dm_dur_sync[ns][0] = it.id; g_dm_dur_sync[ns][1] = (uint32_t)nv; ns++; } }
              int li = -1; for (int i = 0; i < nl; i++) if (last[i].id == it.id) li = i; if (li < 0 && nl < 16) { li = nl++; last[li].id = it.id; last[li].dur = -1; }
              if (li >= 0 && last[li].dur != dur) { LOG("DM_TEST_DUR：拍 %u 物品 id %u 类号 %u 耐久 %d/%d%s", tq, it.id, it.cls, dur, mx, doset ? "（本拍设的）" : ""); last[li].dur = dur; } }
          if (doset) { set_done = 1; atomic_store(&g_dm_dur_sync_n, ns); } } }


#ifndef DM_PLAYER_BUILD
    { static int on = -1, lx, ly; static uint32_t lt; uint32_t tq = 0; if (on < 0) on = getenv("DM_TEST_POS_LOG") != NULL;
      if (on && main && player && tk_rd(G + 0x170, &tq, 4) && tq - lt >= 25) { uint64_t pp = 0; tk_rd(player + 0x38, &pp, 8); if (pp) { int x = *(uint16_t *)(pp + 2), y = *(uint16_t *)(pp + 6); if (x != lx || y != ly) { LOG("DM_TEST_POS_LOG：拍 %u 位置 %d,%d", tq, x, y); lx = x; ly = y; lt = tq; } } } }
#endif
    { void dm_freetp_server_step(uint8_t *, uint64_t, uint64_t); if (main) dm_freetp_server_step((uint8_t *)main->base, G, player); }
    { void dm_respec_server_step(uint8_t *, uint64_t, uint64_t); if (main) dm_respec_server_step((uint8_t *)main->base, G, player); }
    { void dm_sim_quest_server_step(uint64_t); dm_sim_quest_server_step(player); }


    { static int ids[8], n = -1; static int last[8]; if (n < 0) { n = 0; const char *e = getenv("DM_TEST_STAT_LOG"); char buf[64]; if (e) { strlcpy(buf, e, sizeof buf); for (char *sv, *t = strtok_r(buf, ",", &sv); t && n < 8; t = strtok_r(NULL, ",", &sv)) { ids[n] = atoi(t); last[n++] = -1; } } }
      uint32_t tq = 0; if (n > 0 && main && player && tk_rd(G + 0x170, &tq, 4)) { int v[8], ch = 0; for (int i = 0; i < n; i++) { v[i] = ((int (*)(uint64_t, int, int))(main->base + 0x5b9290))(player, ids[i], 0); ch |= v[i] != last[i]; }
          if (ch) { char b[200]; int k = 0; for (int i = 0; i < n; i++) { k += snprintf(b + k, sizeof b - k, " %d=%d", ids[i], v[i]); if (k >= (int)sizeof b) k = (int)sizeof b - 1; last[i] = v[i]; } LOG("DM_TEST_STAT_LOG：拍 %u%s", tq, b); } } }


    { static int at[8], n = -1, done; uint32_t tq = 0; if (n < 0) { n = 0; const char *e = getenv("DM_TEST_SAVE_AT"); char buf[96]; if (e) { strlcpy(buf, e, sizeof buf); for (char *sv, *t = strtok_r(buf, ",", &sv); t && n < 8; t = strtok_r(NULL, ",", &sv)) at[n++] = atoi(t); } }
      if (n > 0 && done < n && tk_rd(G + 0x170, &tq, 4) && tq >= (uint32_t)at[done]) { int dm_request_save(void); LOG("DM_TEST_SAVE_AT：拍 %u 请游戏存一次盘 → %d", tq, dm_request_save()); done++; } }


    { void dm_swapskill_srv_wset(int); uint64_t pd = 0, q = 0; uint8_t w = 0; if (player && tk_rd(player + 0x10, &pd, 8) && pd && tk_rd(pd + 0xf0, &q, 8) && q && tk_rd(q + 0x5a0, &w, 1)) dm_swapskill_srv_wset(w); }








    { static int at = -1, ni, ki; static uint32_t iids[8]; uint32_t tq = 0;
      if (at < 0) { const char *e = getenv("DM_TEST_IDENT_FREE"), *c = e ? strchr(e, ':') : NULL; at = c ? atoi(e) : 0; for (char *q; c && ni < 8; c = *q == '+' ? q : NULL) { iids[ni++] = (uint32_t)strtoul(c + 1, &q, 10); } }
      if (at > 0 && ki < ni && main && player && tk_rd(G + 0x170, &tq, 4) && tq >= (uint32_t)at) { void dm_item_identify_free(uint32_t); dm_item_identify_free(iids[ki]); LOG("测试 DM_TEST_IDENT_FREE：拍 %u 鉴定物品 id %u", tq, iids[ki]); ki++; } }
    { uint32_t dm_item_identify_free_take(void); uint64_t dm_diag_srv_item(uint32_t); uint32_t fid = main && player ? dm_item_identify_free_take() : 0;
      if (fid) { uint64_t si = dm_diag_srv_item(fid); int r = si ? ((int (*)(uint64_t, uint64_t, uint64_t))(main->base + 0x78acf0))(G, player, si) : -1;
          LOG("免费鉴定：物品 id %u → %s", fid, r < 0 ? "服务端找不到这件，没调" : r ? "已调原版鉴定（服务端置已鉴定并广播更新）" : "原版判已鉴定，没变"); } }






    { uint32_t tq = 0; static int given; const char *tg = getenv("DM_TEST_GIVE");
      if (tg && main && player && !given && tk_rd(G + 0x170, &tq, 4)) { char buf[256]; strlcpy(buf, tg, sizeof buf); int at = 40; char *ap = strrchr(buf, '@'); if (ap) { *ap = 0; at = atoi(ap + 1); }
          if (tq >= (uint32_t)at) { given = 1; for (char *sv, *t = strtok_r(buf, ",", &sv); t; t = strtok_r(NULL, ",", &sv)) { char code[8] = ""; int q = 2, n = 1; if (sscanf(t, "%4[^:]:%d:%d", code, &q, &n) < 1) continue; uint32_t c = 0x20202020; memcpy(&c, code, strlen(code) > 4 ? 4 : strlen(code)); int ok = 0, inv = 0;
                  uint32_t cls = 0, id0 = 0; for (int i = 0; i < n && i < 64; i++) { uint64_t u = ((uint64_t (*)(uint64_t, uint64_t, uint32_t, int))(main->base + 0x7c0b20))(G, player, c, q); if (u) { ok++; tk_rd(u + 4, &cls, 4); if (!id0) tk_rd(u + 8, &id0, 4);
#ifndef DM_PLAYER_BUILD
                          if (!i) dm_test_give_stats_server((uint8_t *)main->base, u, code);
                          if (getenv("DM_TEST_GIVE_IDENT")) {
                              int result=((int (*)(uint64_t,uint64_t,uint64_t))(main->base+0x78acf0))(G,player,u);
                              LOG("DM_TEST_GIVE_IDENT: item %u result %d", *(uint32_t *)(u+8), result);
                          }
#endif
                          if (getenv("DM_TEST_GIVE_INV")) {
                              uint64_t ip = 0; uint8_t pk[17] = { 0x16 }; uint32_t one = 1; tk_rd(u + 0x38, &ip, 8); memcpy(pk + 1, (void *)(u + 8), 4); if (ip) { memcpy(pk + 5, (void *)(ip + 0x10), 4); memcpy(pk + 9, (void *)(ip + 0x14), 4); } memcpy(pk + 13, &one, 4);
                              *(uint32_t *)(G + 0x19050) = 1; ((void (*)(uint64_t, uint64_t, uint8_t *, uint32_t))(main->base + 0x6e5b40))(G, player, pk, 17); *(uint32_t *)(G + 0x19050) = 0; inv += *(uint32_t *)(u + 0xc) != 3; } } }
                  LOG("DM_TEST_GIVE：拍 %u 在玩家旁生成「%s」品质 %d：%d / %d 件成功（物品类号 %u，第一件单位 id %u）%s", tq, code, q, ok, n, cls, id0, !getenv("DM_TEST_GIVE_INV") ? "" : inv == ok ? "，已全部捡进背包" : "，【注意】有没捡进背包的（留在地上）"); } } } }
    if (main && player) DM_PROF(1, "s.elysium", dm_elysium_stash_step(G, player, (uint8_t *)main->base));
    { void dm_townwp_tick(uint64_t, uint64_t, uint8_t *); if (main && player) DM_PROF(1, "s.townwp", dm_townwp_tick(G, player, (uint8_t *)main->base)); }
    { int dm_cloud_conflict_take(void); static int shown;
      if (main && player && !shown && dm_cloud_conflict_take()) { shown = 1; dm_game_notice((uint8_t *)main->base, dm_str((uint8_t *)main->base, "dmCloudConflict")); LOG("iCloud 同步：弹了共享仓库收进收件箱的提示"); } }
    { int dm_cloud_char_conflict_take(char *, size_t); char nm[256], msg[400];
      if (main && player && dm_cloud_char_conflict_take(nm, sizeof nm)) { snprintf(msg, sizeof msg, dm_str((uint8_t *)main->base, "dmCloudCharConflict"), nm); dm_game_notice((uint8_t *)main->base, msg); LOG("iCloud 同步：弹了同名角色用云端那份的提示（%s）", nm); } }
    if (main && player) DM_PROF(1, "s.bossrush", dm_bossrush_tick(G, player, (uint8_t *)main->base));
    if (main && player) { void dm_rift_tick(uint64_t, uint64_t, uint8_t *); DM_PROF(1, "s.rift", dm_rift_tick(G, player, (uint8_t *)main->base)); }
    if (main && player) { void dm_resume_tick(uint64_t, uint64_t, uint8_t *); DM_PROF(1, "s.resume", dm_resume_tick(G, player, (uint8_t *)main->base)); }
    { void dm_pages_tick(uint64_t, uint64_t, uint8_t *); if (main) DM_PROF(1, "s.pages", dm_pages_tick(G, player, (uint8_t *)main->base)); }
    { void dm_gear_pages_tick(uint64_t, uint64_t, uint8_t *); if (main) DM_PROF(1, "s.gearpages", dm_gear_pages_tick(G, player, (uint8_t *)main->base)); }
    { void dm_itemcmp_srv_tick(uint64_t, uint64_t, uint8_t *); if (main && player) dm_itemcmp_srv_tick(G, player, (uint8_t *)main->base); }
    { void dm_vault_tick(uint64_t, uint64_t, uint8_t *); if (main) DM_PROF(1, "s.vault", dm_vault_tick(G, player, (uint8_t *)main->base)); }
    { void dm_effects_tick(uint64_t, uint64_t); dm_effects_tick(G, player); }
    { void dm_autobuff_srv_tick(uint8_t *, uint64_t, uint64_t); if (main && player) DM_PROF(1, "s.autobuff", dm_autobuff_srv_tick((uint8_t *)main->base, G, player)); }
    { void dm_pouch_srv_tick(uint8_t *, uint64_t, uint64_t); if (main) DM_PROF(1, "s.pouch", dm_pouch_srv_tick((uint8_t *)main->base, G, player)); }
    { void dm_cloudmerge_srv_tick(uint8_t *, uint64_t, uint64_t); if (main) DM_PROF(1, "s.cloudmerge", dm_cloudmerge_srv_tick((uint8_t *)main->base, G, player)); }
    { void dm_dmgnum_tick(uint64_t); if (main && player) DM_PROF(1, "s.dmgnum", dm_dmgnum_tick(G)); }
    if (main && player && h.valid) {
        dm_fl_player fp = { h.hp, h.hp_max, h.mp, h.mp_max, h.dead, dm_hud_area_is_town(h.area), h.area, *(volatile uint8_t *)(uintptr_t)(G + 0x104) };
        DM_PROF(1, "s.fightlog", dm_fightlog_tick(G, player, (uint8_t *)main->base, &fp, h.belt, belt_item));
        DM_PROF(1, "s.streak", dm_streak_tick(G, player, (uint8_t *)main->base, h.dead, fp.area, fp.difficulty)); }
    if (main && player) DM_PROF(1, "s.kills", dm_kills_tick(G, player, (uint8_t *)main->base)); dm_ipf_frame(1);
    cube_diag_tick(G, player);
    cam_threats_tick(G, player);
    if (main && player) seek_tp_step(G, player, main);
    if (main && h.valid) h.quest_unread = *(volatile uint8_t *)(main->base + 0x2d5ff58) != 0;
    dm_hud_info ha = h, hb = g_hud; ha.x = ha.y = hb.x = hb.y = 0;
    if (memcmp(&ha, &hb, sizeof ha) && logged < 200) { logged++;
        LOG("dm_hud：区域 %d (%d,%d) Lv %d 经验 %d‰ 生命 %d/%d 法力 %d/%d 腰带 %d/%d/%d/%d 回城 %d 人数档 %d 点数 %d 新任务 %d%s%s", h.area, h.x, h.y, h.level, h.exp_permille, h.hp, h.hp_max, h.mp, h.mp_max, h.belt[0], h.belt[1], h.belt[2], h.belt[3], h.portal, h.players, h.points, h.quest_unread, h.dead ? " 已死亡" : "", h.valid ? "" : "（未读到玩家）"); }
    g_hud = h; dm_hud_publish(&h);
    if (g_players_ack) { dm_hud_players_done(g_players_ack); g_players_ack = 0; }
}

static void state_dump(const uint64_t *regs) {
    static char *prev;
    uint64_t G = regs[0], player = 0, inv = 0, cursor = 0;
    int players = 0; uint32_t lid = local_player_id(); uint64_t mine = 0;
    for (int b = 0; b < 128; b++) {
        uint64_t u = 0;
        if (!tk_rd(G + 0x2230 + 8 * b, &u, 8)) continue;
        for (int k = 0; u && k < 4096; k++) {
            uint32_t mode = 0, id = 0;
            uint64_t pp = 0;
            uint16_t c[4] = {0};
            if (tk_rd(u + 0xC, &mode, 4) && mode && tk_rd(u + 0x38, &pp, 8) && pp && tk_rd(pp, c, 8) && (c[1] || c[3])) {
                if (!players++) player = u;
                if (lid != 0xffffffffu && tk_rd(u + 8, &id, 4) && id == lid) mine = u;
            }
            if (!tk_rd(u + 0x158, &u, 8)) break;
        }
    }
    if (lid != 0xffffffffu) { player = mine; players = mine ? 1 : 0; }
    static int player_warned;
    if (players == 0) return;
    if (players != 1) {
        if (!player_warned) { LOG("【红】DM_STATE：受控玩家不唯一（在场 %d 个）", players); player_warned = 1; }
        if (!player) return;
    }
    if (!tk_rd(player + 0x90, &inv, 8)) return;
    uint32_t sig = 0;
    if (!tk_rd(inv, &sig, 4) || sig != 0x01020304) return;
    tk_rd(inv + 0x40, &cursor, 8);
    static state_item items[1025]; size_t n = 0; uint64_t u = 0, next = 0;
    if (!tk_rd(inv + 0x10, &u, 8)) return;
    for (unsigned k = 0; u && k < 1024 && state_read_item(u, &items[n], n, &next); k++) { n++; u = next; }


    if (cursor && state_read_item(cursor, &items[n], n, &next)) {
        int dup = 0; for (size_t i = 0; i < n; i++) dup |= items[i].id == items[n].id;
        if (!dup) n++;
    }
    qsort(items, n, sizeof *items, state_item_cmp);
    uint32_t cursor_id = 0; int has_cursor = 0;
    if (cursor && tk_rd(cursor + 8, &cursor_id, 4)) has_cursor = 1;
    static char cur[131072]; size_t used = 0;
    int w = snprintf(cur, sizeof cur, "\"items\":[");
    if (w < 0 || (size_t)w >= sizeof cur) return; used = (size_t)w;
    for (size_t i = 0; i < n; i++) {
        state_item *it = &items[i];
        w = snprintf(cur + used, sizeof cur - used, "%s{\"id\":%u,\"cls\":%u,\"mode\":%u,\"page\":%u,\"x\":%u,\"y\":%u}",
                     i ? "," : "", it->id, it->cls, it->mode, it->page, it->x, it->y);
        if (w < 0 || (size_t)w >= sizeof cur - used) return; used += (size_t)w;
    }
    int pl = g_hud.players;
    w = has_cursor ? snprintf(cur + used, sizeof cur - used, "],\"cursor\":%u,\"players\":%d", cursor_id, pl)
                   : snprintf(cur + used, sizeof cur - used, "],\"cursor\":null,\"players\":%d", pl);
    if (w < 0 || (size_t)w >= sizeof cur - used) return; used += (size_t)w;
    state_dump_monsters(G, cur, sizeof cur, &used);
    uint32_t tick = 0;
    if (!tk_rd(G + 0x170, &tick, 4)) return;
    if (prev && !strcmp(prev, cur)) return;
    free(prev); prev = strdup(cur);
    if (!prev) return;
    fprintf(g_spfp, "{\"tick\":%u,%s}\n", tick, cur);
    fflush(g_spfp);
}
static void ticks_hash(const uint64_t *regs) {
    static const uint32_t TB[5] = {0x2230, 0x2630, 0x2a30, 0x3230, 0x2e30};
    static tk_unit *us; static size_t cap;
    uint64_t G = regs[0]; uint32_t tick;
    g_tkreads = 0;
    if (!tk_rd(G + 0x170, &tick, 4)) return;
    if (tick == g_tklast) return;


    if (g_tklast >= 0 && tick != g_tklast + 1) { if (tick <= 1 && (long long)tick < g_tklast) LOG("DM_TICKS 新一局：拍号 %lld → %u", g_tklast, tick); else LOG("【红】DM_TICKS 拍号断档 %lld → %u", g_tklast, tick); }
    g_tkreads = 1;
    static uint8_t tab[0x1400]; unsigned bad = 0; size_t n = 0;
    if (!tk_rd(G + TB[0], tab, sizeof tab)) memset(tab, 0, sizeof tab);
    for (int t = 0; t < 5; t++)
        for (int b = 0; b < 128; b++) {
            uint64_t u; memcpy(&u, tab + TB[t] - TB[0] + 8 * b, 8);
            for (int k = 0; u && k < 4096; k++) {
                uint8_t hd[0x160];
                if (!tk_rd(u, hd, sizeof hd)) { bad++; break; }
                if (n == cap) { cap = cap ? cap * 2 : 1024; us = realloc(us, cap * sizeof *us); }
                tk_unit *x = &us[n]; memset(x, 0, offsetof(tk_unit, st)); x->ord = (unsigned)n;
                memcpy(&x->type, hd, 4); memcpy(&x->cls, hd + 4, 4); memcpy(&x->id, hd + 8, 4); memcpy(&x->mode, hd + 12, 4);
                x->act = hd[0x18]; memcpy(&x->seed, hd + 0x30, 4); memcpy(&x->rlo, hd + 0x28, 4); memcpy(&x->rhi, hd + 0x2c, 4);
                uint64_t q;
                if (t == 4) { uint8_t d[0x1c]; memcpy(&q, hd + 0x10, 8);
                    if (tk_rd(q, d, sizeof d)) { memcpy(&x->it[0], d, 4); memcpy(&x->it[1], d + 4, 4); memcpy(&x->it[2], d + 8, 4); memcpy(&x->it[3], d + 0x18, 4); memcpy(&x->it[4], d + 0x10, 4); x->has_it = 1; } }
                if (t == 0 || t == 1 || t == 3) { uint16_t p[4]; memcpy(&q, hd + 0x38, 8);
                    if (tk_rd(q, p, sizeof p)) { x->xf = p[0]; x->x = p[1]; x->yf = p[2]; x->y = p[3]; } }
                uint8_t sl[0x40]; memcpy(&q, hd + 0x88, 8);
                if (tk_rd(q, sl, sizeof sl)) {
                    uint64_t sb; uint32_t sc; memcpy(&sb, sl + 0x30, 8); memcpy(&sc, sl + 0x38, 4);
                    uint32_t m = sc < 96 ? sc : 96;
                    if (sb && sc && tk_rd(sb, x->st, 8 * m)) x->nst = m;
                }
                if (x->type != (uint32_t)t) bad++;
                n++;
                memcpy(&u, hd + 0x158, 8);
            }
        }
    qsort(us, n, sizeof *us, tk_cmp);
    uint64_t h[6]; for (int i = 0; i < 6; i++) h[i] = 0xcbf29ce484222325ull;
    uint8_t gs[8] = {0}; if (!tk_rd(G + 0x1a8, gs, 8)) memset(gs, 0, 8);
    h[5] = tk_fnv(h[5], gs, 8);
    unsigned cnt[5] = {0};
    for (size_t i = 0; i < n; i++) {
        tk_unit *u = &us[i]; uint32_t t = u->type;
        if (t >= 5) continue;
        cnt[t]++;
        uint32_t f9[9] = {t, u->cls, u->id, u->mode, u->act, u->x, u->y, u->xf, u->yf}, f2[2] = {u->rlo, u->rhi};
        uint64_t hh = tk_fnv(h[t], f9, sizeof f9); hh = tk_fnv(hh, f2, sizeof f2);
        if (t == 4) hh = tk_fnv(hh, u->it, sizeof u->it);
        if (t <= 1 || t == 4) { hh = tk_fnv(hh, &u->nst, 4); hh = tk_fnv(hh, u->st, 8 * (size_t)u->nst); }
        h[t] = hh;
    }
    if (!g_tkt0) g_tkt0 = now_ns();
    fprintf(g_tkfp, "{\"tick\": %u, \"n\": [%u, %u, %u, %u, %u], \"frames\": %llu, \"h\": [", tick, cnt[0], cnt[1], cnt[2], cnt[3], cnt[4], (unsigned long long)dm_nvn_frame_count());
    for (int i = 0; i < 6; i++) fprintf(g_tkfp, "%s\"%016llx\"", i ? ", " : "", h[i]);
    fprintf(g_tkfp, "], \"bad\": %u, \"reads\": %u, \"game\": \"%#llx\", \"dt\": %.3f}\n", bad, g_tkreads, G, (now_ns() - g_tkt0) / 1e9);
    if (g_tkfull)
        for (size_t i = 0; i < n; i++) {
            tk_unit *u = &us[i];
            fprintf(g_tkfp, "{\"t\": %u, \"u\": {\"type\": %u, \"cls\": %u, \"id\": %u, \"mode\": %u, \"act\": %u, \"seed\": %u, \"rlo\": %u, \"rhi\": %u, "
                    "\"x\": %u, \"y\": %u, \"xf\": %u, \"yf\": %u, \"it\": ", tick, u->type, u->cls, u->id, u->mode, u->act, u->seed, u->rlo, u->rhi, u->x, u->y, u->xf, u->yf);
            if (u->has_it) fprintf(g_tkfp, "[%u, %u, %u, %u, %u]", u->it[0], u->it[1], u->it[2], u->it[3], u->it[4]); else fputs("null", g_tkfp);
            fputs(", \"st\": [", g_tkfp);
            for (uint32_t j = 0; j < u->nst; j++) fprintf(g_tkfp, "%s[%u, %u]", j ? ", " : "", u->st[j][0], u->st[j][1]);
            fputs("]}}\n", g_tkfp);
        }
    fflush(g_tkfp);
    g_tklast = tick;
    if (g_tkleft > 0 && --g_tkleft == 0) { fclose(g_tkfp); res_report("退出"); LOG("DM_TICKS 记满，最后拍号 %u，退出", tick); _exit(0); }
}













static FILE *g_c2sfp;
static struct c2s_ent { uint32_t tick, unit, len; int has_unit, sys; uint8_t *pkt; } *g_rp; static size_t g_nrp, g_irp;
static _Atomic uint64_t g_lastG; static uint32_t g_sysclient; static int g_sysclient_set; static unsigned g_sysdrop;
static int64_t g_rplast = -1; static unsigned g_rpdrop;







static void cam_c2s(uint64_t G, uint64_t pl, const uint8_t *b, uint32_t n) {
    int dm_cam_mode(void); void dm_cam_attack(float dx, float dz);
    if (!n || !pl || dm_cam_mode() != 1) return;
    uint8_t op = b[0]; float tx, ty;
    if ((op == 0x05 || op == 0x08 || op == 0x0c || op == 0x0f) && n >= 5) {





        float lx = (float)(b[1] | b[2] << 8), ly = (float)(b[3] | b[4] << 8), best = 4; uint64_t u = 0; int found = 0;
        for (int bk = 0; bk < 128; bk++) if (tk_rd(G + 0x2230 + 1 * 0x400 + 8 * bk, &u, 8)) for (int k = 0; u && k < 4096; k++) {
            uint32_t mode = 0; uint64_t tp = 0; uint32_t xy[2];
            if (tk_rd(u + 0xC, &mode, 4) && mode != 0 && mode != 12 && tk_rd(u + 0x38, &tp, 8) && tp && tk_rd(tp, xy, 8)) {
                float mx = xy[0] / 65536.0f, my = xy[1] / 65536.0f, d2 = (mx - lx) * (mx - lx) + (my - ly) * (my - ly);
                if (d2 < best) { best = d2; tx = mx; ty = my; found = 1; } }
            if (!tk_rd(u + 0x158, &u, 8)) break; }
        if (!found) return;
    }
    else if ((op == 0x06 || op == 0x07 || op == 0x09 || op == 0x0a || op == 0x0d || op == 0x0e || op == 0x10 || op == 0x11) && n >= 9) {
        uint32_t type = b[1] | b[2] << 8 | b[3] << 16 | (uint32_t)b[4] << 24, id = b[5] | b[6] << 8 | b[7] << 16 | (uint32_t)b[8] << 24; if (type > 1) return;
        uint64_t u = 0, hit = 0;
        for (int bk = 0; bk < 128 && !hit; bk++) if (tk_rd(G + 0x2230 + type * 0x400 + 8 * bk, &u, 8)) for (int k = 0; u && k < 4096; k++) { uint32_t i2 = 0; if (tk_rd(u + 8, &i2, 4) && i2 == id) { hit = u; break; } if (!tk_rd(u + 0x158, &u, 8)) break; }
        uint64_t tp = 0; uint32_t xy[2]; if (!hit || !tk_rd(hit + 0x38, &tp, 8) || !tp || !tk_rd(tp, xy, 8)) return;
        tx = xy[0] / 65536.0f; ty = xy[1] / 65536.0f;
    } else return;
    uint64_t pp = 0; uint32_t pxy[2]; if (!tk_rd(pl + 0x38, &pp, 8) || !pp || !tk_rd(pp, pxy, 8)) return;
    dm_cam_attack(tx - pxy[0] / 65536.0f, ty - pxy[1] / 65536.0f);
}










static void cam_threats_tick(uint64_t G, uint64_t pl) {
    int dm_cam_mode(void); void dm_cam_set_threats(const float *xy, const int *tier, int n); int dm_threat_tier(uint64_t unit);
    if (dm_cam_mode() <= 0 || !pl) return;
    static float range = -1; if (range < 0) { const char *e = getenv("DM_CAM_THREAT_RANGE"); range = e ? (float)atof(e) : 30; }
    dm_hud_info h; dm_hud_get(&h); if (h.valid && dm_hud_area_is_town(h.area)) { dm_cam_set_threats(NULL, NULL, 0); return; }
    uint64_t pp = 0; uint32_t pxy[2]; if (!tk_rd(pl + 0x38, &pp, 8) || !pp || !tk_rd(pp, pxy, 8)) return;
    float px = pxy[0] / 65536.0f, py = pxy[1] / 65536.0f, xy[64]; int tier[32], n = 0; uint64_t u = 0;
    uint32_t me = 0, pet[64]; int np = 0; module_t *m = main_mod(); uint64_t e = 0;
    if (m && tk_rd(pl + 8, &me, 4) && tk_rd((uint64_t)m->base + 0x2d33528, &e, 8))
        for (int k = 0; e && k < 256 && np < 64; k++) { uint32_t id = 0, owner = 0; if (tk_rd(e + 8, &id, 4) && tk_rd(e + 0xc, &owner, 4) && owner == me) pet[np++] = id; if (!tk_rd(e + 0x30, &e, 8)) break; }
    for (int bk = 0; bk < 128 && n < 32; bk++) if (tk_rd(G + 0x2230 + 1 * 0x400 + 8 * bk, &u, 8)) for (int k = 0; u && k < 4096 && n < 32; k++) {
        uint32_t cls = 0, mode = 0, mxy[2]; uint64_t tp = 0;
        uint32_t uid = 0; int mine = 0; if (np && tk_rd(u + 8, &uid, 4)) for (int k = 0; k < np && !mine; k++) mine = pet[k] == uid;
        if (!mine && tk_rd(u + 4, &cls, 4) && tk_rd(u + 0xC, &mode, 4) && mode != 0 && mode != 12 && cls != 271 && cls != 338 && cls != 359 && cls != 560 && cls != 561
            && tk_rd(u + 0x38, &tp, 8) && tp && tk_rd(tp, mxy, 8)) {
            float mx = mxy[0] / 65536.0f, my = mxy[1] / 65536.0f, dx = mx - px, dy = my - py;
            if (dx * dx + dy * dy <= range * range) { int r = dm_threat_tier(u); xy[2 * n] = mx; xy[2 * n + 1] = my; tier[n++] = r < 0 ? 0 : r; } }
        if (!tk_rd(u + 0x158, &u, 8)) break; }
    dm_cam_set_threats(xy, tier, n);
}
static void c2s_obs(const uint64_t *regs) {
    uint32_t tick = 0, uid = 0, len = (uint32_t)regs[3]; int hu = 0;
    tk_rd(regs[0] + 0x170, &tick, 4);
    if (regs[1] && tk_rd(regs[1] + 8, &uid, 4)) hu = 1;
    uint8_t b[512]; uint32_t n = len < 512 ? len : 512; if (!tk_rd(regs[2], b, n)) n = 0;
    if (n >= 1 && b[0] == 0x20) cube_diag_req(regs[0], regs[1], b, n);
    dm_bossrush_c2s(regs[0], regs[1], b, n);
    cam_c2s(regs[0], regs[1], b, n);
    { void dm_rift_c2s(uint64_t, const uint8_t *, uint32_t); dm_rift_c2s(regs[0], b, n); }
    { void dm_pouch_c2s(uint64_t *); dm_pouch_c2s((uint64_t *)(uintptr_t)regs); }
    { void dm_shoprefresh_c2s(uint64_t, uint64_t, const uint8_t *, uint32_t); dm_shoprefresh_c2s(regs[0], regs[1], b, n); }
    if (!g_c2sfp) return;




    if (n >= 5 && b[0] == 0x40 && regs[1] && getenv("DM_TEST_WARP_DIAG")) { module_t *mm = main_mod(); uint32_t id = b[1] | b[2] << 8 | b[3] << 16 | (uint32_t)b[4] << 24, pm = 99; uint64_t G = regs[0], pl = regs[1], tile = 0, u = 0; tk_rd(pl + 0xC, &pm, 4);
        for (int bk = 0; bk < 128 && !tile; bk++) if (tk_rd(G + 0x2230 + 5 * 0x400 + 8 * bk, &u, 8)) for (int k = 0; u && k < 4096; k++) { uint32_t i2 = 0; if (tk_rd(u + 8, &i2, 4) && i2 == id) { tile = u; break; } if (!tk_rd(u + 0x158, &u, 8)) break; }
        int chk = mm ? ((int (*)(uint64_t, uint64_t, int, uint64_t, int))((uintptr_t)mm->base + 0x6c5730))(G, pl, 2, 0, 0) : -1, usable = mm && tile ? ((int (*)(uint64_t))((uintptr_t)mm->base + 0x5592f0))(tile) : -1, dist = mm && tile ? ((int (*)(uint64_t, uint64_t))((uintptr_t)mm->base + 0x5c7050))(pl, tile) : -1;
        LOG("DM_TEST_WARP_DIAG：拍 %u 收到 0x40 砖 id %u%s：玩家模式 %u，能否打断 %d（0 = 放行），出口可用 %d（非 0 = 放行），距离 %d（≤4 当场换）", tick, id, tile ? "" : "（服务端单位表里没有这块砖）", pm, chk, usable, dist); }
    fprintf(g_c2sfp, "{\"tick\":%u,\"unit\":", tick);
    if (hu) fprintf(g_c2sfp, "%u", uid); else fputs("null", g_c2sfp);
    fprintf(g_c2sfp, ",\"len\":%u,\"pkt\":\"", len);
    for (uint32_t i = 0; i < n; i++) fprintf(g_c2sfp, "%02x", b[i]);
    fputs("\"}\n", g_c2sfp); fflush(g_c2sfp);
}




static int game_live(void) {
    static const uint64_t *slots;
    if (!slots) { for (int i = 0; i < g_nmod; i++) if (!strcmp(g_mod[i].name, "main")) slots = (const uint64_t *)(g_mod[i].base + 0x2d665d8); if (!slots) return 1; }
    for (int i = 0; i < 0x400; i++) if (slots[i] + 1 >= 2) return 1;
    return 0;
}

static _Atomic uint32_t g_run_epoch;
uint32_t dm_diag_epoch(void) { return atomic_load(&g_run_epoch); }


static uint64_t live_game(void) { uint64_t G = atomic_load(&g_lastG); if (G && !game_live()) { if (atomic_compare_exchange_strong(&g_lastG, &G, 0)) atomic_fetch_add(&g_run_epoch, 1); return 0; } return G; }
static int in_game(void) { return live_game() != 0; }







#define SAVE_ASKED ((uint64_t)-1000000)
int dm_request_save(void) {
    uint64_t G = live_game(); if (!G) return 0;
    uint64_t last = 0; if (!tk_rd(G + 0x5800, &last, 8) || !last) return 0;
    *(volatile uint64_t *)(uintptr_t)(G + 0x5800) = SAVE_ASKED;
    return 1;
}


int dm_game_paused(void) { uint8_t *b = NULL; for (int i = 0; i < g_nmod; i++) if (!strcmp(g_mod[i].name, "main")) b = (uint8_t *)g_mod[i].base; return b && in_game() && (*(volatile uint8_t *)(b + 0x2d5ff51) | *(volatile uint8_t *)(b + 0x2d5ff67)); }


void dm_diag_new_run(void) { atomic_fetch_add(&g_run_epoch, 1); }
uint64_t dm_diag_player(void) { return in_game() ? g_player_unit : 0; }
uint64_t dm_diag_game(void) { return live_game(); }
uint64_t dm_diag_srv_item(uint32_t id) { if (!in_game()) return 0; uint64_t u = 0; tk_rd(g_lastG + 0x2e30 + 8 * (id & 0x7f), &u, 8); for (int k = 0; u && k < 4096; k++) { uint32_t ty = 0, uid = 0; if (!tk_rd(u, &ty, 4) || !tk_rd(u + 8, &uid, 4)) return 0; if (ty == 4 && uid == id) return u; if (!tk_rd(u + 0x158, &u, 8)) return 0; } return 0; }








int dm_diag_srv_merc(uint32_t id, int *hp, int *mx, int *healing) {
    module_t *m = main_mod(); if (!in_game() || !m) return 0; uint64_t u = 0; tk_rd(g_lastG + 0x2630 + 8 * (id & 0x7f), &u, 8);
    for (int k = 0; u && k < 4096; k++) { uint32_t ty = 0, uid = 0, mode = 0; if (!tk_rd(u, &ty, 4) || !tk_rd(u + 8, &uid, 4) || !tk_rd(u + 0xc, &mode, 4)) return 0;
        if (ty == 1 && uid == id) { if (mode == 0 || mode == 12) return 0;
            int (*get)(uint64_t, int, int) = (int (*)(uint64_t, int, int))(m->base + 0x5b9290); *hp = get(u, 6, 0) >> 8; *mx = get(u, 7, 0) >> 8;
            uint64_t sl = 0; uint32_t fl = 0, w = 0; *healing = tk_rd(u + 0x88, &sl, 8) && sl && tk_rd(sl + 0x1c, &fl, 4) && fl >> 31 && tk_rd(sl + 0xb30 + 4 * (100 >> 5), &w, 4) && (w >> (100 & 31) & 1);
            return 1; }
        if (!tk_rd(u + 0x158, &u, 8)) return 0; }
    return 0;
}
int dm_diag_difficulty(void) { uint8_t d = 0; return in_game() && tk_rd(g_lastG + 0x104, &d, 1) && d < 3 ? d : -1; }


int dm_diag_ladder(int *offline, int *ladder) { uint8_t off = 0; uint32_t lad = 0; if (!in_game() || !tk_rd(g_lastG + 0x101, &off, 1) || !tk_rd(g_lastG + 0x108, &lad, 4)) return 0; *offline = off != 0; *ladder = lad != 0; return 1; }
uint32_t dm_cur_tick(void) { uint32_t t = 0; if (in_game()) tk_rd(g_lastG + 0x170, &t, 4); return t; }
static uint32_t cur_tick(void) { return dm_cur_tick(); }
static _Atomic int g_holding;


uint64_t dm_server_tick(void) { return in_game() && !atomic_load(&g_holding) ? dm_cur_tick() : UINT64_MAX; }
static void c2s_sys_obs(const uint64_t *regs) {
    uint32_t len = (uint32_t)regs[1], cid; if (tk_rd(regs[0], &cid, 4) && !g_sysclient_set) { g_sysclient = cid; g_sysclient_set = 1; }
    if (!g_c2sfp) return;
    uint8_t b[512]; uint32_t n = len < 512 ? len : 512; if (!tk_rd(regs[0] + 4, b, n)) n = 0;
    fprintf(g_c2sfp, "{\"tick\":%u,\"q\":\"sys\",\"unit\":null,\"len\":%u,\"pkt\":\"", cur_tick(), len);
    for (uint32_t i = 0; i < n; i++) fprintf(g_c2sfp, "%02x", b[i]);
    fputs("\"}\n", g_c2sfp); fflush(g_c2sfp);
}
static uint32_t g_rpfirst, g_rplast_tick;
static int rp_window(void) { uint32_t t = cur_tick(); return g_rp && g_nrp && g_lastG && t >= g_rpfirst && t <= g_rplast_tick; }








static int sys_clock_msg(uint8_t id) { return id == 0x6f || id == 0x6d; }
void c2s_deliver_sys(uint64_t *regs) {
    uint8_t id = 0; tk_rd(regs[0] + 4, &id, 1);
    if (rp_window() && !sys_clock_msg(id)) { if (!g_sysdrop++) LOG("DM_C2S_REPLAY：拍 %u 起丢弃客户端自发的系统类消息（首条）", cur_tick()); return; }
    static void (*sysm)(uint64_t, uint64_t);
    if (!sysm) { module_t *m = NULL; for (int i = 0; i < g_nmod; i++) if (!strcmp(g_mod[i].name, "main")) m = &g_mod[i];
                 sysm = (void (*)(uint64_t, uint64_t))(m->base + 0x6310b0); }
    sysm(regs[0], regs[1]);
}










typedef struct { uint64_t a; uint32_t b, c; uint8_t ok; uint8_t pad[47]; } autoplace_res;
static int cube_pickup_rewrite(const uint64_t *regs, uint8_t out[4 + 25]) {
    static int on = -1; if (on < 0) { const char *e = getenv("DM_CUBE_PICKUP"); on = !(e && !strcmp(e, "0")); }
    uint8_t in[4 + 17]; uint32_t id, flag;
    if (!on || regs[1] != 17 || !tk_rd(regs[0], in, sizeof in) || in[4] != 0x16) return 0;
    memcpy(&id, in + 5, 4); memcpy(&flag, in + 17, 4); if (!flag) return 0;
    module_t *m = main_mod(); uint64_t G = g_lastG, p = g_player_unit, u = 0, game = 0, inv = 0, pp = 0;
    if (!m || !in_game() || !p || !tk_rd(G + 0x2e30 + 8 * (id & 0x7f), &u, 8)) return 0;
    for (int k = 0; u && k < 4096; k++) {
        uint32_t t = 0, i = 0; if (!tk_rd(u, &t, 4) || !tk_rd(u + 8, &i, 4)) return 0;
        if (t == 4 && i == id) break;
        if (!tk_rd(u + 0x158, &u, 8)) return 0;
    }
    uint32_t mode = 0; if (!u || !tk_rd(u + 0xC, &mode, 4) || mode != 3) return 0;
    int (*is_type)(uint64_t, int) = (void *)(m->base + 0x574bc0); int (*stackable)(uint64_t) = (void *)(m->base + 0x5722f0);
    if (is_type(u, 0x16) || is_type(u, 0x12) || stackable(u)) return 0;
    if (!tk_rd(p + 0xd8, &game, 8) || !game || !tk_rd(p + 0x90, &inv, 8) || !inv) return 0;
    if (((autoplace_res (*)(uint64_t, uint64_t, uint64_t))(m->base + 0x646840))(game, p, u).ok) return 0;
    uint64_t cube = ((uint64_t (*)(uint64_t))(m->base + 0x565e50))(p); if (!cube) return 0;
    uint32_t tx = 0, ty = 0, cid = 0, cx = 0, cy = 0;
    if (!((int (*)(uint64_t, uint64_t, int, uint32_t *, uint32_t *, int))(m->base + 0x55f620))(inv, u, 9, &tx, &ty, 3)) return 0;
    if (!tk_rd(cube + 8, &cid, 4) || !tk_rd(cube + 0x38, &pp, 8) || !pp || !tk_rd(pp + 0x10, &cx, 4) || !tk_rd(pp + 0x14, &cy, 4)) return 0;
    uint32_t cxy = (cx & 0xffff) | cy << 16, txy = (tx & 0xffff) | ty << 16;
    memcpy(out, in, 4); out[4] = 0x5f; memcpy(out + 5, &id, 4); memcpy(out + 9, &cid, 4); memcpy(out + 13, in + 9, 8); memcpy(out + 21, &cxy, 4); memcpy(out + 25, &txy, 4);
    static int logged; if (logged++ < 40) { uint32_t code = ((uint32_t (*)(uint64_t))(m->base + 0x570260))(u); int dm_potpick_auto(uint32_t);
        LOG("规则：背包放不下，拾取物品 %u（%.4s，%s）改进方块 %u（方块格 %u,%u）", id, (const char *)&code, dm_potpick_auto(id) ? "药水自动拾取发的" : "玩家点的", cid, tx & 0xffff, ty); }
    return 1;
}




static int potpick_gate(const uint64_t *regs) {
    int dm_potpick_auto(uint32_t); uint8_t in[4 + 17]; uint32_t id;
    if (regs[1] != 17 || !tk_rd(regs[0], in, sizeof in) || in[4] != 0x16) return 0;
    memcpy(&id, in + 5, 4); if (!dm_potpick_auto(id)) return 0;
    module_t *m = main_mod(); uint64_t G = g_lastG, p = g_player_unit, u = 0, inv = 0;
    if (!m || !in_game() || !p || !tk_rd(G + 0x2e30 + 8 * (id & 0x7f), &u, 8)) return 0;
    for (int k = 0; u && k < 4096; k++) { uint32_t t = 0, i = 0; if (!tk_rd(u, &t, 4) || !tk_rd(u + 8, &i, 4)) return 0; if (t == 4 && i == id) break; if (!tk_rd(u + 0x158, &u, 8)) return 0; }
    uint32_t mode = 0, slot = 0; if (!u || !tk_rd(u + 0xC, &mode, 4) || mode != 3 || !tk_rd(p + 0x90, &inv, 8) || !inv) return 0;
    uint32_t code = ((uint32_t (*)(uint64_t))(m->base + 0x570260))(u);
    if (((int (*)(uint64_t, uint64_t, uint32_t *, int))(m->base + 0x561de0))(inv, u, &slot, 1)) { static int ok; if (ok++ < 40) LOG("药水自动拾取：物品 %u（%.4s）到服务端时腰带放得进（槽 %u），放行", id, (const char *)&code, slot); return 0; }
    static int dr; if (dr++ < 40) LOG("药水自动拾取：物品 %u（%.4s）到服务端时腰带已放不下 → 不捡，留在地上（丢包第 %d 次）", id, (const char *)&code, dr);
    return 1;
}
static int dm_move_allowed(void) { extern _Atomic uint64_t g_dm_move_allow_until_ns; struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return (uint64_t)t.tv_sec * 1000000000ull + (uint64_t)t.tv_nsec < atomic_load(&g_dm_move_allow_until_ns); }
void c2s_deliver(uint64_t *regs) {
    { uint8_t lop = 0xff; if (dm_lat_on() && regs[0] && regs[1] >= 1 && tk_rd(regs[0] + 4, &lop, 1) && lop != 0x6d) DM_LAT("c2s %02x", lop); }






    if (g_rp && (!g_nrp || rp_window())) { if (!g_rpdrop++) LOG("DM_C2S_REPLAY：拍 %u 起丢弃客户端自发命令（首条）", cur_tick()); return; }








    { uint64_t dm_stick_last_ns(void); uint8_t op = 0xff; if (regs[0] && regs[1] >= 1 && tk_rd(regs[0] + 4, &op, 1) && (op == 0x01 || op == 0x03) && now_ns() - dm_stick_last_ns() > 700000000ull && !dm_move_allowed()) {
        static int dropped; if (dropped++ < 10) LOG("产品规则：丢弃非摇杆来源的点地移动包 0x%02x（第 %d 条）", op, dropped); return; } }
    static void (*recv)(uint64_t, uint64_t, uint64_t, uint64_t);
    if (!recv) { module_t *m = NULL; for (int i = 0; i < g_nmod; i++) if (!strcmp(g_mod[i].name, "main")) m = &g_mod[i];
                 recv = (void (*)(uint64_t, uint64_t, uint64_t, uint64_t))(m->base + 0x632510); }
    if (potpick_gate(regs)) return;
    uint8_t cube[4 + 25]; if (cube_pickup_rewrite(regs, cube)) { recv((uint64_t)(uintptr_t)cube, 25, regs[2], regs[3]); return; }








    uint32_t pk_id = 0, pk_code = 0; int pk_auto = 0;
    { uint8_t in[4 + 17]; int dm_potpick_auto(uint32_t); module_t *m = main_mod();
      if (m && in_game() && regs[1] == 17 && tk_rd(regs[0], in, sizeof in) && in[4] == 0x16) { memcpy(&pk_id, in + 5, 4); uint64_t u = dm_diag_srv_item(pk_id);
          if (u) { pk_code = ((uint32_t (*)(uint64_t))(m->base + 0x570260))(u); pk_auto = dm_potpick_auto(pk_id); } else pk_id = 0; } }
    recv(regs[0], regs[1], regs[2], regs[3]);
    if (pk_id) { static uint32_t win, used, skipped; uint32_t t = cur_tick(); if (t - win >= 25) { win = t; used = 0; }
        if (used >= 4) skipped++;
        else { used++; uint64_t u = dm_diag_srv_item(pk_id), d = 0, pp = 0; uint32_t mode = 0, x = 0, y = 0; uint8_t page = 0; char where[64];
            if (!u || !tk_rd(u + 0xc, &mode, 4) || !tk_rd(u + 0x10, &d, 8) || !d) snprintf(where, sizeof where, "服务端已没有这件（叠进了别的一叠 / 金币等）");
            else if (mode == 3) snprintf(where, sizeof where, "还在地上（没捡成）");
            else { tk_rd(d + 0x55, &page, 1); if (tk_rd(u + 0x38, &pp, 8) && pp) { tk_rd(pp + 0x10, &x, 4); tk_rd(pp + 0x14, &y, 4); } snprintf(where, sizeof where, "容器 %u 页 %u 格 %d,%d", mode, page, (int)x, (int)y); }
            if (skipped) LOG("拾取：物品 %u（%.4s，%s）→ %s（此前另有 %u 条没记）", pk_id, (const char *)&pk_code, pk_auto ? "药水自动拾取发的" : "玩家点的", where, skipped);
            else LOG("拾取：物品 %u（%.4s，%s）→ %s", pk_id, (const char *)&pk_code, pk_auto ? "药水自动拾取发的" : "玩家点的", where);
            skipped = 0; } }
}
static uint64_t unit_by_id(uint64_t G, uint32_t id) {
    for (int b = 0; b < 128; b++) {
        uint64_t u = 0; tk_rd(G + 0x2230 + 8 * b, &u, 8);
        for (int k = 0; u && k < 4096; k++) { uint32_t x; if (!tk_rd(u + 8, &x, 4)) break; if (x == id) return u; tk_rd(u + 0x158, &u, 8); }
    }
    return 0;
}
static void c2s_inject(uint64_t G) {
    uint32_t tick; if (!tk_rd(G + 0x170, &tick, 4) || tick == g_rplast) return;
    g_rplast = tick;
    {
        char a[16]; snprintf(a, sizeof a, "%u#", tick);
        for (int i = 0, n = atomic_load(&g_nui_sub); i < n; i++) g_ui_sub[i]("DM", "Tick", a);
    }
    while (g_irp < g_nrp && g_rp[g_irp].tick < tick) { LOG("【红】DM_C2S_REPLAY：第 %zu 条拍号 %u 已过（当前 %u），丢弃", g_irp + 1, g_rp[g_irp].tick, tick); g_irp++; }
    static void (*disp)(uint64_t, uint64_t, uint64_t, uint32_t);
    if (!disp) { module_t *m = NULL; for (int i = 0; i < g_nmod; i++) if (!strcmp(g_mod[i].name, "main")) m = &g_mod[i];
                 disp = (void (*)(uint64_t, uint64_t, uint64_t, uint32_t))(m->base + 0x6e5b40); }
    static void (*sysm)(uint64_t, uint64_t);
    if (!sysm) { module_t *m = NULL; for (int i = 0; i < g_nmod; i++) if (!strcmp(g_mod[i].name, "main")) m = &g_mod[i];
                 sysm = (void (*)(uint64_t, uint64_t))(m->base + 0x6310b0); }
    size_t end = g_irp; while (end < g_nrp && g_rp[end].tick == tick) end++;
    for (size_t k = g_irp; k < end; k++) if (g_rp[k].sys && g_rp[k].len && !sys_clock_msg(g_rp[k].pkt[0])) {
        struct c2s_ent *e = &g_rp[k]; uint8_t *buf = malloc(4 + e->len);
        memcpy(buf, &g_sysclient, 4); memcpy(buf + 4, e->pkt, e->len); sysm((uint64_t)buf, e->len); free(buf);
    }
    for (; g_irp < end; g_irp++) {
        struct c2s_ent *e = &g_rp[g_irp]; if (e->sys) continue;
        uint64_t u = e->has_unit ? unit_by_id(G, e->unit) : 0;
        if (e->has_unit && !u) die("【红】DM_C2S_REPLAY 第 %zu 条：拍 %u 找不到单位 id %u", g_irp + 1, tick, e->unit);
        volatile uint32_t *busy = (volatile uint32_t *)(G + 0x19050);
        *busy = 1; disp(G, u, (uint64_t)e->pkt, e->len); *busy = 0;
    }
    if (g_irp == g_nrp && g_nrp) { static int once; if (!once++) LOG("DM_C2S_REPLAY：%zu 条全部注入（最后拍 %u）", g_nrp, tick); }
    if (g_nrp && tick > g_rplast_tick) {
        static int done; if (!done++) {
            LOG("DM_C2S_REPLAY：拍 %u 已过文件末拍 %u，放行客户端自发消息（合成 UI 消息 DM:ReplayDone）", tick, g_rplast_tick);
            for (int i = 0, n = atomic_load(&g_nui_sub); i < n; i++) g_ui_sub[i]("DM", "ReplayDone", "");
        }
    }
}
static void c2s_load(const char *path) {
    FILE *f = fopen(path, "r"); if (!f) die("DM_C2S_REPLAY 打不开 %s", path);
    char line[2048]; size_t cap = 0;
    while (fgets(line, sizeof line, f)) {
        if (!strchr(line, '{')) continue;
        struct c2s_ent e = {0}; char *p;
        if (!(p = strstr(line, "\"tick\":"))) die("DM_C2S_REPLAY 行缺 tick：%s", line); e.tick = (uint32_t)strtoul(p + 7, NULL, 10);
        if ((p = strstr(line, "\"q\":"))) { p += 4; while (*p == ' ') p++; e.sys = !strncmp(p, "\"sys\"", 5); }
        if (!(p = strstr(line, "\"unit\":"))) die("DM_C2S_REPLAY 行缺 unit：%s", line); p += 7; while (*p == ' ') p++;
        if (strncmp(p, "null", 4)) { e.has_unit = 1; e.unit = (uint32_t)strtoul(p, NULL, 10); }
        if (!(p = strstr(line, "\"len\":"))) die("DM_C2S_REPLAY 行缺 len：%s", line); e.len = (uint32_t)strtoul(p + 6, NULL, 10);
        if (!(p = strstr(line, "\"pkt\":"))) die("DM_C2S_REPLAY 行缺 pkt：%s", line); p = strchr(p + 6, '"') + 1;
        size_t nh = strcspn(p, "\""); if (nh / 2 != e.len) die("【红】DM_C2S_REPLAY 拍 %u：包长 %u 与字节数 %zu 不符（> 512 字节的包录制时被截断，无法回放）", e.tick, e.len, nh / 2);
        e.pkt = malloc(e.len ? e.len : 1);
        for (uint32_t i = 0; i < e.len; i++) { unsigned v; sscanf(p + 2 * i, "%2x", &v); e.pkt[i] = (uint8_t)v; }
        if (g_nrp && e.tick < g_rp[g_nrp - 1].tick) die("【红】DM_C2S_REPLAY 拍号倒退 %u < %u", e.tick, g_rp[g_nrp - 1].tick);
        if (g_nrp == cap) { cap = cap ? cap * 2 : 256; g_rp = realloc(g_rp, cap * sizeof *g_rp); }
        g_rp[g_nrp++] = e;
    }
    fclose(f);
    if (!g_rp) g_rp = malloc(1);
    g_rpfirst = g_nrp ? g_rp[0].tick : 0; g_rplast_tick = g_nrp ? g_rp[g_nrp - 1].tick : 0;
    LOG("DM_C2S_REPLAY ← %s：%zu 条%s", path, g_nrp, g_nrp ? "" : "（空：只关自发命令）");
}

















static uint32_t g_hold_tick; static char g_hold_ui[96] = "PausePanelMessage:ExitGame"; static _Atomic int g_holding; static uint64_t g_hold_t0;
static volatile uint32_t *g_hold_dl; static uint32_t g_hold_dl0; static int32_t g_hold_org; static int g_hold_org_ok; static uint32_t g_hold_dl1;
static uint32_t guest_ms(void) { return (uint32_t)(guest_ns() / 1000000); }
static volatile uint32_t *hold_dl(void) {
    if (!g_hold_dl) for (int i = 0; i < g_nmod; i++) if (!strcmp(g_mod[i].name, "main")) g_hold_dl = (volatile uint32_t *)(g_mod[i].base + 0x2d685d8);
    return g_hold_dl;
}




static void hold_track_origin(void) {
    int32_t d = (int32_t)(guest_ms() - *hold_dl());
    if (!g_hold_org_ok || d < g_hold_org) { g_hold_org = d; g_hold_org_ok = 1; }
}
static void hold_freeze(void) {
    hold_dl();
    g_hold_dl0 = *g_hold_dl; *g_hold_dl = 0xF0000000u;
    guest_clock_freeze(1);
}
static void hold_unfreeze(void) {
    guest_clock_freeze(0);
    if (!g_hold_dl) return;
    uint32_t t = guest_ms() - (uint32_t)g_hold_org, d = g_hold_dl0 + 40;
    *g_hold_dl = g_hold_dl1 = (int32_t)(d - (t + 40)) < 0 ? t + 40 : d;
}







static pthread_mutex_t g_pause_lock = PTHREAD_MUTEX_INITIALIZER;
static int g_pause_wants, g_pause_saving;
static dm_pause_state g_pause_st;
static void pause_sync(int loading) {
    static int held_off;
    int on = dm_pause_holds(g_pause_wants, g_pause_saving, loading);
    if (!loading) held_off = 0;
    else if (g_pause_wants && !g_pause_saving && !held_off) { held_off = 1; LOG("pause: held off while the loading screen is up (reasons %#x)", g_pause_wants); }
    switch (dm_pause_next(&g_pause_st, on)) {
    case DM_PAUSE_APPLY: hold_dl(); *g_hold_dl = dm_pause_applied(&g_pause_st, *g_hold_dl, guest_ms()); guest_clock_freeze_for(3, 1); LOG("暂停：拍 %u", cur_tick()); break;
    case DM_PAUSE_RELEASE: guest_clock_freeze_for(3, 0); *g_hold_dl = dm_pause_released(&g_pause_st, guest_ms()); LOG("暂停结束：拍 %u", cur_tick()); break;
    }
}
void dm_pause_want(int reasons, int on) {
    pthread_mutex_lock(&g_pause_lock);
    g_pause_wants = on ? g_pause_wants | reasons : g_pause_wants & ~reasons;
    pthread_mutex_unlock(&g_pause_lock);
}
int dm_pause_wanted(void) { pthread_mutex_lock(&g_pause_lock); int w = g_pause_wants != 0; pthread_mutex_unlock(&g_pause_lock); return w; }


int dm_pause_reasons(void) { pthread_mutex_lock(&g_pause_lock); int w = g_pause_wants; pthread_mutex_unlock(&g_pause_lock); return w; }
static void pause_saving(int on) { pthread_mutex_lock(&g_pause_lock); g_pause_saving = on; pthread_mutex_unlock(&g_pause_lock); }





extern _Atomic int g_dm_frozen; static _Atomic int g_bgsave; int dm_loadscreen_up(void);
static dm_loadwatch g_loadwatch = { .first_ns = 30000000000ull, .again_ns = 60000000000ull, .max = 5 };
static void loadwatch_pump(int loading, int wants, int saving, int held) {
#ifndef DM_PLAYER_BUILD
    { static int init; if (!init) { init = 1; const char *e = getenv("DM_TEST_LOADWATCH_S"); int sec = e ? atoi(e) : 0;
        if (sec > 0) { g_loadwatch.first_ns = g_loadwatch.again_ns = (uint64_t)sec * 1000000000ull; LOG("DM_TEST_LOADWATCH_S: loading screen watch every %d s", sec); } } }
#endif
    if (!loading && !g_loadwatch.since_ns) return;
    uint64_t since = g_loadwatch.since_ns, now = now_ns();
    int r = dm_loadwatch_step(&g_loadwatch, loading, now, loading && !since ? cur_tick() : 0);
    if (r == DM_LOADWATCH_REPORT) {
        unsigned dm_guest_freeze_reasons(void); int dm_hle_quiet_on(void), dm_focus_now(void), dm_uiscreen_ingame(void), dm_ui_panel_shown(const char *), dm_ui_load_tracked(void);
        dm_hud_info h; dm_hud_get(&h);
        LOG("【红】loading screen up for %.0f s: pause reasons=%#x saving=%d held=%d, guest freeze reasons=%#x, world frozen=%d bgsave=%d, quiet=%d, focus=%d, "
            "server tick %u -> %u, in game %d, panel shown %d tracked %d, player valid %d dead %d area %d",
            (now - g_loadwatch.since_ns) / 1e9, wants, saving, held, dm_guest_freeze_reasons(), atomic_load(&g_dm_frozen), atomic_load(&g_bgsave), dm_hle_quiet_on(),
            dm_focus_now(), g_loadwatch.tick0, cur_tick(), dm_uiscreen_ingame(), dm_ui_panel_shown("LoadScreenPanel"), dm_ui_load_tracked(), h.valid, h.dead, h.area);
    } else if (r == DM_LOADWATCH_GONE) LOG("loading screen gone after %.0f s (server tick now %u)", (now - since) / 1e9, cur_tick());
}
void dm_pause_pump(void) {
    int loading = dm_loadscreen_up();
    pthread_mutex_lock(&g_pause_lock); pause_sync(loading); int wants = g_pause_wants, saving = g_pause_saving, held = g_pause_st.paused; pthread_mutex_unlock(&g_pause_lock);
    loadwatch_pump(loading, wants, saving, held);
}
static void hold_release(const char *why) {
    if (!atomic_exchange(&g_holding, 0)) return;
    uint32_t last = (uint32_t)g_rplast;
    hold_unfreeze();
    LOG("DM_TICK_HOLD：「%s」→ 解冻（冻结 %.3f s）；冻结拍 %u（目标 %u）%s；下一拍时限 %u → %u", why, (now_ns() - g_hold_t0) / 1e9, last, g_hold_tick,
        last == g_hold_tick ? "" : "【红】不等", g_hold_dl0 + 40, g_hold_dl1);
}




static _Atomic int g_lholding, g_lhold_giveup;
static void lhold_release(void);
void dm_hold_for_loadscreen(void) {
    if (!g_hold_org_ok || atomic_load(&g_lhold_giveup)) return;


    if (atomic_load(&g_lholding)) { if (now_ns() - g_hold_t0 > 8000000000ull) { atomic_store(&g_lhold_giveup, 1); LOG("读取画面冻结 8 秒没撤：这次装载要服务端走拍，解冻放行（本次读取画面不再冻结）"); lhold_release(); } return; }
    if (atomic_load(&g_holding) || atomic_exchange(&g_lholding, 1)) return;
    hold_freeze(); g_hold_t0 = now_ns(); atomic_store(&g_holding, 1);
    LOG("读取画面未撤：拍 %u 冻结客体时钟，撤下后放行", cur_tick());
}
static void lhold_release(void) {
    if (!atomic_exchange(&g_lholding, 0)) return;
    if (atomic_exchange(&g_holding, 0)) { hold_unfreeze(); LOG("读取画面已撤 → 解冻（冻结 %.3f s，拍 %u）", (now_ns() - g_hold_t0) / 1e9, cur_tick()); }
}
static void hold_on_uimsg(const char *cat, const char *act, const char *arg) {
    char m[256]; snprintf(m, sizeof m, "%s:%s:%s", cat, act, arg);
    if (strcmp(g_hold_ui, "SHOT") && atomic_load(&g_holding) && strstr(m, g_hold_ui)) hold_release(m);
}








#include <TargetConditionals.h>
#include <sys/stat.h>
#include <time.h>
int dm_thermal_now(void);
int dm_ingame_now(void) { dm_hud_info h; dm_hud_get(&h); return h.valid; }




char g_lf_panel[48];
static int g_loadscreen_seen;
int dm_loadscreen_up(void);
void dm_longframe_note(const char *what) {
    static const char *path; static int init; static unsigned n;
    if (!init) { init = 1; const char *e = getenv("DM_PERF_FILE"); path = e ? (strcmp(e, "0") ? e : NULL) : (DM_PHONE ? "perf.txt" : NULL); }
    dm_hud_info h; dm_hud_get(&h); char ctx[160];
    snprintf(ctx, sizeof ctx, "读取画面 %s，面板 %s，区域 %d%s", g_loadscreen_seen ? (dm_loadscreen_up() ? "盖着" : "已撤") : "未见过", g_lf_panel[0] ? g_lf_panel : "无", h.area, h.valid ? "" : "（未读到玩家）");
    LOG("[长帧] %s；%s", what, ctx);
    if (!path || n++ >= 2000) return;
    FILE *fp = fopen(path, "a"); if (!fp) return;
    time_t w = time(NULL); struct tm tmv; localtime_r(&w, &tmv); char ts[16]; strftime(ts, sizeof ts, "%H:%M:%S", &tmv);
    fprintf(fp, "长帧\t%s\t%s；%s\n", ts, what, ctx); fclose(fp);
}
void dm_perf_present(double iv) {
    static const char *path; static int init; static unsigned c16, c25, c33, disc; static double sum; static uint64_t t0, f0, cpu0, tstart;
    if (!init) { init = 1; const char *e = getenv("DM_PERF_FILE"); path = e ? (strcmp(e, "0") ? e : NULL) : (DM_PHONE ? "perf.txt" : NULL); t0 = tstart = now_ns(); f0 = dm_nvn_frame_count(); cpu0 = dm_nvn_sub_cpu_ns(); }
    dm_3d_scale_feed(iv);
    if (!path) return;
    if (iv < 0) disc++; else if (iv > 0) { sum += iv; if (iv <= 20.8) c16++; else if (iv <= 29.2) c25++; else c33++; }
    uint64_t now = now_ns(); if (now - t0 < 10000000000ull) return;
    unsigned n = c16 + c25 + c33; uint64_t f = dm_nvn_frame_count(), cpu = dm_nvn_sub_cpu_ns();
    struct stat st; if (!stat(path, &st) && st.st_size > 256 * 1024) rename(path, "perf.old.txt");
    FILE *fp = fopen(path, "a");
    if (fp) {
        if (ftell(fp) == 0) fprintf(fp, "# 时刻\t进程秒\t温控\t3D比例\t上屏帧率\t16.7ms档%%\t25ms档%%\t≥33ms档%%\t丢弃帧\t提交CPUms\t场景\n");
        time_t w = time(NULL); struct tm tmv; localtime_r(&w, &tmv); char ts[16]; strftime(ts, sizeof ts, "%H:%M:%S", &tmv);
        dm_hud_info h; dm_hud_get(&h); char scene[64]; if (h.valid) snprintf(scene, sizeof scene, "局内 区域%d (%d,%d) Lv%d%s", h.area, h.x, h.y, h.level, h.dead ? "(死亡)" : ""); else snprintf(scene, sizeof scene, "菜单");
        fprintf(fp, "%s\t%.0f\t%d\t%.2f\t%.1f\t%.1f\t%.1f\t%.1f\t%u\t%.2f\t%s\n", ts, (now - tstart) / 1e9, dm_thermal_now(), dm_3d_scale_cur(),
                sum > 0 ? n * 1000.0 / sum : (f - f0) / ((now - t0) / 1e9), n ? c16 * 100.0 / n : 0, n ? c25 * 100.0 / n : 0, n ? c33 * 100.0 / n : 0, disc, f > f0 ? (cpu - cpu0) / 1e6 / (double)(f - f0) : 0, scene);
        fclose(fp);
    }
    LOG("[perf] 温控 %d 3D %.2f 帧率 %.1f 三档 %u/%u/%u 丢弃 %u", dm_thermal_now(), dm_3d_scale_cur(), sum > 0 ? n * 1000.0 / sum : (f - f0) / ((now - t0) / 1e9), c16, c25, c33, disc);
    c16 = c25 = c33 = disc = 0; sum = 0; t0 = now; f0 = f; cpu0 = cpu;
}
void dm_shot_written(const char *path, int ok) {
    if (g_hold_tick && !strcmp(g_hold_ui, "SHOT") && atomic_load(&g_holding)) {
        static int udc_ab_shots;
        int ab = (getenv("DM_NVN_UDC_AB") && !strcmp(getenv("DM_NVN_UDC_AB"), "1")) || (getenv("DM_NVN_DEFER_AB") && !strcmp(getenv("DM_NVN_DEFER_AB"), "1")) || (getenv("DM_NVN_RULER") && !strcmp(getenv("DM_NVN_RULER"), "1"));
        if (ab && ok && ++udc_ab_shots < 3) return;
        char m[600]; snprintf(m, sizeof m, "截图写盘%s %s", ok ? "完成" : "失败", path); hold_release(m);
    }
}
static void *hold_guard(void *a) {
    (void)a; uint64_t seen = crash_watch_progress_count(), deadline = now_ns() + 10000000000ull;
    for (;;) {
        sleep(1);
        uint64_t count = crash_watch_progress_count();
        if (count != seen) { seen = count; deadline = now_ns() + 10000000000ull; }
        if (now_ns() < deadline) continue;
        if (atomic_exchange(&g_holding, 0)) { hold_unfreeze(); LOG("【红】DM_TICK_HOLD：连续 10 秒无进展，强制解冻；卡在「%s」", !strcmp(g_hold_ui, "SHOT") ? "截图写盘" : g_hold_ui); }
        return NULL;
    }
}




static uint32_t g_pos_tick, g_pos_want[4]; static int g_pos_on, g_pos_bad;
static void shotui_cancel_all(void);
static void poschk(uint64_t G) {




    int n = 0, hit = 0, mine = 0; uint16_t q[4] = {0}, qm[4] = {0}; uint32_t lid = local_player_id();
    for (int b = 0; b < 128; b++) { uint64_t u = 0; tk_rd(G + 0x2230 + 8 * b, &u, 8);
        for (int k = 0; u && k < 4096; k++) {
            uint32_t mode = 0, id = 0; uint64_t pp = 0; uint16_t c[4] = {0}; n++;
            if (tk_rd(u + 12, &mode, 4) && mode && tk_rd(u + 0x38, &pp, 8) && pp && tk_rd(pp, c, 8) && (c[1] || c[3])) { if (!hit++) memcpy(q, c, 8);
                if (lid != 0xffffffffu && tk_rd(u + 8, &id, 4) && id == lid) { mine = 1; memcpy(qm, c, 8); } }
            if (!tk_rd(u + 0x158, &u, 8)) break; } }
    if (lid != 0xffffffffu) { hit = mine; memcpy(q, qm, 8); }
    int ok = hit == 1;
    uint32_t xf = q[0], x = q[1], yf = q[2], y = q[3];
    int same = ok && x == g_pos_want[0] && y == g_pos_want[1] && xf == g_pos_want[2] && yf == g_pos_want[3];
    if (!same) { g_pos_bad = 1; shotui_cancel_all(); }
    if (!ok) LOG("【红】DM_POSCHK 拍 %u：受控玩家不唯一（玩家表 %d 个单位，在场 %d 个）", g_pos_tick, n, hit);
    else LOG("%sDM_POSCHK 拍 %u 玩家 (%u,%u)/(%u,%u) 真值 (%u,%u)/(%u,%u) %s", same ? "" : "【红】", g_pos_tick, x, y, xf, yf,
             g_pos_want[0], g_pos_want[1], g_pos_want[2], g_pos_want[3], same ? "一致" : "不一致");
}














_Atomic int g_dm_frozen;
_Atomic int g_dm_frz_want;
void mtl_world_frozen(int on) __attribute__((weak));
static uint32_t frz_snapshot(uint64_t G, int *nmon, int *php) {
    module_t *main = main_mod(); int (*get)(uint64_t, int, int) = (int (*)(uint64_t, int, int))(main->base + 0x5b9290); uint32_t h = 2166136261u; int n = 0;
    for (int b = 0; b < 128; b++) { uint64_t u = 0; if (!tk_rd(G + 0x2630 + 8 * b, &u, 8)) continue;
        for (int k = 0; u && k < 4096; k++) { uint32_t v[3] = {0}; uint64_t pp = 0; uint16_t c[4] = {0}; tk_rd(u + 8, &v[0], 4); tk_rd(u + 0xC, &v[1], 4); v[2] = (uint32_t)get(u, 6, 0); if (tk_rd(u + 0x38, &pp, 8) && pp) tk_rd(pp, c, 8);
            for (int i = 0; i < 3; i++) h = (h ^ v[i]) * 16777619u; h = (h ^ c[1]) * 16777619u; h = (h ^ c[3]) * 16777619u; n++; if (!tk_rd(u + 0x158, &u, 8)) break; } }
    *nmon = n; *php = g_player_unit ? get(g_player_unit, 6, 0) >> 8 : -1; return h;
}
static void freeze_world_obs(const uint64_t *rc) { uint64_t *r = (uint64_t *)rc; static uint64_t fq[0x1500 / 8], fg[0x200 / 8]; if (atomic_load(&g_dm_frozen)) { fg[0x170 / 8] = 0; fg[0x188 / 8] = (uint64_t)(uintptr_t)fq; r[0] = (uint64_t)(uintptr_t)fg; } }
static void freeze_client_obs(const uint64_t *rc) { uint64_t *r = (uint64_t *)rc; static uint32_t fu[0x200 / 4]; if (atomic_load(&g_dm_frozen) && r[0]) { fu[0x124 / 4] = 1u << 23; r[0] = (uint64_t)(uintptr_t)fu; } }
static void freeze_anim_obs(const uint64_t *rc) { uint64_t *r = (uint64_t *)rc; static uint32_t fu[0x200 / 4]; if (atomic_load(&g_dm_frozen) && r[0]) { fu[0] = 9; r[0] = (uint64_t)(uintptr_t)fu; } }
static void freeze_render_obs(const uint64_t *rc) { (void)rc; static uint64_t t0; static int was; module_t *m = main_mod(); if (!m) return; { void dm_itemcmp_render_probe(void); dm_itemcmp_render_probe(); }                                                                                                                                                         uint32_t *dt = (uint32_t *)((uintptr_t)m->base + 0x20cebb8); uint64_t *wt = (uint64_t *)((uintptr_t)m->base + 0x3114208);
    if (atomic_load(&g_dm_frozen)) { if (!was) { was = 1; t0 = *wt; } *dt = 0; *wt = t0; } else was = 0; }
static _Atomic int g_frz_by_panel;




void dm_freeze_frame(int want) { static int idle;
    if (!atomic_load(&g_dm_frozen) || !atomic_load(&g_frz_by_panel) || want) { idle = 0; return; }
    if (++idle < 10) return; idle = 0; atomic_store(&g_dm_frozen, 0); atomic_store(&g_frz_by_panel, 0); if (mtl_world_frozen) mtl_world_frozen(0);
    LOG("面板冻结：解冻（面板已不在屏上而拍入口没在走：原版暂停 / 退出 / 换角色）"); }
static int g_frz_on;
static int freeze_attach(void) {
    static int tried; if (tried) return g_frz_on; tried = 1;
    g_frz_on = gate_obs_present(OBS_FREEZE_WORLD) && gate_obs_present(OBS_FREEZE_CLIENT) && gate_obs_present(OBS_FREEZE_ANIM) && gate_obs_present(OBS_FREEZE_RENDER);
    if (g_frz_on) { gate_obs_set(OBS_FREEZE_WORLD, freeze_world_obs); gate_obs_set(OBS_FREEZE_CLIENT, freeze_client_obs); gate_obs_set(OBS_FREEZE_ANIM, freeze_anim_obs); gate_obs_set(OBS_FREEZE_RENDER, freeze_render_obs); }
    return g_frz_on;
}
int dm_freeze_attach_probe(void) { return freeze_attach(); }






static uint8_t g_bg_pause[2];
static int save_unpause_begin(void) {
    uint8_t *b = NULL; for (int i = 0; i < g_nmod; i++) if (!strcmp(g_mod[i].name, "main")) b = (uint8_t *)g_mod[i].base;
    if (!b || !in_game()) return 0; volatile uint8_t *f = b + 0x2d5ff51, *g = b + 0x2d5ff67; if (!*f && !*g) return 0;
    if (!freeze_attach()) { LOG("【红】停着存盘：游戏停着，但产物里没有冻结门 10–13，不放开暂停（这次存不了）"); return -1; }
    atomic_store(&g_frz_by_panel, 0); atomic_store(&g_bgsave, 1); atomic_store(&g_dm_frozen, 1);
    int nm = 0, hp = 0; uint32_t h = g_lastG ? frz_snapshot(g_lastG, &nm, &hp) : 0;
    g_bg_pause[0] = *f; g_bg_pause[1] = *g; *f = 0; *g = 0;
    LOG("停着存盘：游戏停着 → 冻住世界、临时放开原版暂停（怪 / 佣兵 %d 个，快照 %08x，玩家生命 %d）", nm, h, hp);
    return 1;
}
static void save_unpause_end(void) {
    if (!atomic_load(&g_bgsave)) return;
    uint8_t *b = NULL; for (int i = 0; i < g_nmod; i++) if (!strcmp(g_mod[i].name, "main")) b = (uint8_t *)g_mod[i].base;
    if (b) { *(volatile uint8_t *)(b + 0x2d5ff51) = g_bg_pause[0]; *(volatile uint8_t *)(b + 0x2d5ff67) = g_bg_pause[1]; }
    int nm = 0, hp = 0; uint32_t h = g_lastG ? frz_snapshot(g_lastG, &nm, &hp) : 0;
    atomic_store(&g_bgsave, 0); atomic_store(&g_dm_frozen, 0);
    LOG("停着存盘：恢复原版暂停、解冻（怪 / 佣兵 %d 个，快照 %08x，玩家生命 %d）", nm, h, hp);
}








uint64_t dm_fs_commit_count(void);
int dm_save_wait(double max_s, uint64_t *commits, int *froze, double *ms) {
    uint64_t c0 = dm_fs_commit_count(), t0 = now_ns(), last = c0, last_t = 0, lim = (uint64_t)(max_s * 1e9); int asked = 0, bg = 0, taken = 0;
    for (;;) {
        if (!bg && in_game()) bg = save_unpause_begin();
        if (!asked) asked = dm_request_save();
        usleep(taken ? 20000 : 2000); uint64_t c = dm_fs_commit_count(), t = now_ns();
        if (c != last) { last = c; last_t = t; }
        if (asked && !taken) { uint64_t stamp = SAVE_ASKED; uint64_t G = live_game(); taken = G && tk_rd(G + 0x5800, &stamp, 8) && stamp != SAVE_ASKED; }
        if ((taken && last != c0 && t - last_t >= 200000000ull) || t - t0 >= lim) break;
    }
    if (bg > 0) save_unpause_end();
    *commits = last - c0; *froze = bg > 0; *ms = (now_ns() - t0) / 1e6;
    return asked;
}
#ifdef __ANDROID__



int dm_leave_save(int wait_book) {
    int dm_uiscreen_ingame(void); dm_hud_info h; dm_hud_get(&h);
    if (!h.valid || h.dead || !dm_uiscreen_ingame()) return 0;
    if (!wait_book && dm_itemjob_owner()) return -2;
    unsigned dm_resume_leave_save_begin(void); void dm_resume_leave_save_end(unsigned); unsigned resume_gen = dm_resume_leave_save_begin();
    pause_saving(1);
    uint64_t t0 = now_ns(); int told = 0;
    while (dm_itemjob_owner()) { usleep(20000); if (!told && now_ns() - t0 > 5000000000ull) { told = 1; LOG("Android: leaving the app: item job %d is not done after 5 s, the save waits for it", dm_itemjob_owner()); } }
    uint64_t commits = 0; int froze = 0; double ms = 0; int asked = dm_save_wait(2.5, &commits, &froze, &ms);
    pause_saving(0);
    dm_resume_leave_save_end(resume_gen);
    LOG("Android: leaving the app: save asked %d, %llu commits, pause menu lifted %d, %.0f ms", asked, (unsigned long long)commits, froze, ms);
    return asked && commits ? 1 : -1;
}
#endif
static void freeze_tick(uint64_t G) {
    static struct { uint32_t at, n; int st; } w[8]; static int nw = -1, panel, cur = -1; static uint32_t left, calls;
    if (nw < 0) { nw = 0; const char *e = getenv("DM_TEST_FREEZE"); char buf[160]; if (e) { strlcpy(buf, e, sizeof buf); for (char *sv, *t = strtok_r(buf, ";", &sv); t && nw < 8; t = strtok_r(NULL, ";", &sv)) if (sscanf(t, "%u:%u", &w[nw].at, &w[nw].n) == 2) nw++; }
        e = getenv("DM_PANEL_FREEZE"); panel = e && e[0] != '0';
        if (nw || panel) { if (freeze_attach()) LOG("面板冻结：四个入口门已挂（按面板 %d，时间窗 %d 个）", panel, nw); else LOG("【红】面板冻结：产物里没有观测门 10–13，不生效"); } }
    if (!g_frz_on) return;
    uint32_t t = 0; if (!tk_rd(G + 0x170, &t, 4)) return; int frozen = atomic_load(&g_dm_frozen);
    if (cur < 0 && !frozen) for (int i = 0; i < nw; i++) if (!w[i].st && t >= w[i].at) { cur = i; w[i].st = 1; left = w[i].n; break; }
    if (cur >= 0 && frozen && left && !--left) { w[cur].st = 2; cur = -1; }
    int want = cur >= 0 || (panel && atomic_load(&g_dm_frz_want)) || atomic_load(&g_bgsave);
    if (want && !frozen) { if (t % 20 == 0 || t % 12 == 0 || t % 11 == 0 || t % 1500 == 0) return;
        int nm = 0, hp = 0; uint32_t h = frz_snapshot(G, &nm, &hp); atomic_store(&g_frz_by_panel, cur < 0); atomic_store(&g_dm_frozen, 1); frozen = 1; calls = 0; if (mtl_world_frozen) mtl_world_frozen(1); LOG("面板冻结：拍 %u 冻结（%s）；怪 %d 只 校验 %08x 玩家生命 %d", t, cur >= 0 ? "时间窗" : "面板开着", nm, h, hp); }
    else if (!want && frozen) { int nm = 0, hp = 0; uint32_t h = frz_snapshot(G, &nm, &hp); atomic_store(&g_dm_frozen, 0); frozen = 0; if (mtl_world_frozen) mtl_world_frozen(0); LOG("面板冻结：拍 %u 解冻（冻了 %u 次拍入口）；怪 %d 只 校验 %08x 玩家生命 %d", t, calls, nm, h, hp); }
    if (frozen) { calls++; uint32_t t1 = t - 1; memcpy((void *)(uintptr_t)(G + 0x170), &t1, 4); }
}
static void tick_gate_product(const uint64_t *regs) { g_lastG = regs[0]; players_apply(); hud_tick(regs[0]); freeze_tick(regs[0]); }
static void tick_gate(const uint64_t *regs) {
    {
        static _Atomic uintptr_t owner; uintptr_t me = (uintptr_t)pthread_self(), exp = 0;
        if (!atomic_compare_exchange_strong(&owner, &exp, me) && exp != me) { LOG("【红】拍入口换了线程（首个 %#lx，本次 %#lx）：static 缓冲会竞争", (unsigned long)exp, (unsigned long)me); abort(); }
    }
    g_lastG = regs[0];
    players_apply();
    hud_tick(regs[0]);
    crash_watch_progress(cur_tick(), "拍入口");
    if (getenv("DM_TEST_LEAK")) {
        static void **leaks; static size_t nleaks;
        void *p = malloc(64 * 1024);
        if (!p) die("DM_TEST_LEAK：64 KiB 分配失败");
        memset(p, 0xa5, 64 * 1024);
        void **next = realloc(leaks, (nleaks + 1) * sizeof(*leaks));
        if (!next) die("DM_TEST_LEAK：泄漏表扩容失败");
        leaks = next; leaks[nleaks++] = p;
    }
    if (g_pos_on && cur_tick() == g_pos_tick) { g_pos_on = 0; poschk(regs[0]); }
    if (dm_mtl_l3_tick) dm_mtl_l3_tick(cur_tick());
    { static uint32_t last; uint32_t t = cur_tick(); if (t && t % 250 == 0 && t != last) { last = t; l3mem_request(t); res_report("每250拍"); } }
    {
        static uint32_t lt, ms; static int st;
        if (!st) { const char *e = getenv("DM_TEST_LAG"); st = e ? 1 : 2; if (e) { lt = (uint32_t)strtoul(e, 0, 0); const char *c = strchr(e, ':'); ms = c ? (uint32_t)strtoul(c + 1, 0, 0) : 300; } }
        if (st == 1 && cur_tick() == lt) { st = 2; usleep(ms * 1000); LOG("DM_TEST_LAG：拍 %u 入口睡 %u ms", lt, ms); } }
    hold_track_origin();




    { int dm_loadscreen_up(void); if (g_hold_tick && !strcmp(g_hold_ui, "SHOT") && cur_tick() == 5 && dm_loadscreen_up()) dm_hold_for_loadscreen(); }
    if (g_hold_tick && g_pos_bad && !strcmp(g_hold_ui, "SHOT") && cur_tick() == g_hold_tick) { static int o2; if (!o2++) LOG("DM_TICK_HOLD：坐标核对不一致，不截图也不冻结"); }
    else if (g_hold_tick) { uint32_t t = cur_tick(); if (t == g_hold_tick) { static int once; if (!once++) {
        hold_freeze();
        g_hold_t0 = now_ns(); atomic_store(&g_holding, 1); LOG("DM_TICK_HOLD：拍 %u 冻结客体时钟，等「%s」", t, g_hold_ui);
        pthread_t th; if (!pthread_create(&th, NULL, hold_guard, NULL)) pthread_detach(th); } } }
    if (g_rp) c2s_inject(regs[0]);
    if (g_spfp) state_dump(regs);
    if (g_tkfp) ticks_hash(regs);
    { void dm_bright_server_tick(uint64_t G); dm_bright_server_tick(regs[0]); }
    freeze_tick(regs[0]);
}




static void *soak_main(void *arg) {
    FILE *f = arg; int iv = 60; const char *e = strchr(getenv("DM_SOAK"), ','); if (e) iv = atoi(e + 1);
    fprintf(f, "秒\t拍号\t句柄\t存活线程\t线程对象\tmalloc_MiB\tfootprint_MiB\n"); fflush(f);
    for (;;) {
        sleep(iv);
        void thr_counts(int *, int *, int *); int h, l, o; thr_counts(&h, &l, &o);
        malloc_statistics_t ms; malloc_zone_statistics(NULL, &ms);
        task_vm_info_data_t vi; mach_msg_type_number_t c = TASK_VM_INFO_COUNT; uint64_t fp = 0;
        if (task_info(mach_task_self(), TASK_VM_INFO, (task_info_t)&vi, &c) == KERN_SUCCESS) fp = vi.phys_footprint;
        long long tk = in_game() ? (long long)cur_tick() : -1;
        fprintf(f, "%.0f\t%lld\t%d\t%d\t%d\t%.1f\t%.1f\n", (now_ns() - g_t0_ns) / 1e9, tk, h, l, o, ms.size_in_use / 1048576.0, fp / 1048576.0);
        fflush(f);
    }
    return NULL;
}
static void soak_install(void) {
    const char *e = getenv("DM_SOAK"); if (!e) return;
    char p[1024]; snprintf(p, sizeof p, "%.*s", (int)strcspn(e, ","), e);
    FILE *f = fopen(p, "w"); if (!f) die("DM_SOAK 打不开 %s", p);
    pthread_t t; if (pthread_create(&t, NULL, soak_main, f)) die("DM_SOAK 起线程失败"); pthread_detach(t);
    LOG("DM_SOAK → %s", p);
}




#define SHOTUI_MAX 8
static struct { char trig[96], path[512]; long off; int done; } g_shotui[SHOTUI_MAX]; static int g_nshotui;
static pthread_mutex_t g_shotgate_mu = PTHREAD_MUTEX_INITIALIZER;
static int g_loadscreen_open, g_loadscreen_unload_pending;
static unsigned g_loadscreen_generation;
static int g_loadscreen_delay_ms;



int dm_loadscreen_up(void) { pthread_mutex_lock(&g_shotgate_mu); int u = g_loadscreen_seen && g_loadscreen_open; pthread_mutex_unlock(&g_shotgate_mu); return u; }
int dm_loadscreen_down(void) { pthread_mutex_lock(&g_shotgate_mu); int d = g_loadscreen_seen && !g_loadscreen_open; pthread_mutex_unlock(&g_shotgate_mu); return d; }
static void loadscreen_close_now(unsigned generation) {
    pthread_mutex_lock(&g_shotgate_mu);
    if (generation == g_loadscreen_generation && g_loadscreen_unload_pending) {
        g_loadscreen_unload_pending = 0; g_loadscreen_open = 0;
        LOG("读取画面已撤"); prof_load_end();
    }
    pthread_mutex_unlock(&g_shotgate_mu);
    if (!dm_loadscreen_up()) { lhold_release(); atomic_store(&g_lhold_giveup, 0); }
}
static void *loadscreen_delayed_close(void *arg) {
    unsigned generation = *(unsigned *)arg; free(arg);
    usleep((useconds_t)g_loadscreen_delay_ms * 1000);
    loadscreen_close_now(generation);
    return NULL;
}
static void (*g_sharelog_cb)(void);
void dm_sharelog_init(void (*cb)(void)) { g_sharelog_cb = cb; }
static void loadscreen_on_uimsg(const char *cat, const char *act, const char *arg) {




    if (!strcmp(cat, "PanelManager") && !strcmp(arg, "QuestLogPanelExpansion") && (!strcmp(act, "OpenPanel") || !strcmp(act, "OpenExclusivePanel"))) { module_t *qm = main_mod(); if (qm) *(volatile uint8_t *)(qm->base + 0x2d5ff58) = 0; }
    { void dm_textinput_on_uimsg(const char *, const char *, const char *); dm_textinput_on_uimsg(cat, act, arg); }
    { void dm_update_on_uimsg(const char *, const char *, const char *); dm_update_on_uimsg(cat, act, arg); }
    { void dm_kills_on_uimsg(const char *, const char *, const char *); dm_kills_on_uimsg(cat, act, arg); } { void dm_rift_ui_on_uimsg(const char *, const char *, const char *); dm_rift_ui_on_uimsg(cat, act, arg); }
    { void dm_vault_on_uimsg(const char *, const char *, const char *); dm_vault_on_uimsg(cat, act, arg); }
    { void dm_padmode_on_uimsg(const char *, const char *, const char *); dm_padmode_on_uimsg(cat, act, arg); }
    { void dm_pouch_on_uimsg(const char *, const char *, const char *); dm_pouch_on_uimsg(cat, act, arg); }
    { void dm_looks_ui_on_uimsg(const char *, const char *, const char *); dm_looks_ui_on_uimsg(cat, act, arg); }
    { void dm_shoprefresh_on_uimsg(const char *, const char *, const char *, uint8_t *); module_t *sm = main_mod(); dm_shoprefresh_on_uimsg(cat, act, arg, sm ? (uint8_t *)sm->base : NULL); }
    { void dm_runeword_on_uimsg(const char *, const char *, const char *); dm_runeword_on_uimsg(cat, act, arg); }
    { void dm_build_on_uimsg(const char *, const char *, const char *); dm_build_on_uimsg(cat, act, arg); }
    { void dm_beltauto_on_uimsg(const char *, const char *, const char *); dm_beltauto_on_uimsg(cat, act, arg); }
    { extern void (*g_dm_charsel_hook)(void); if (g_dm_charsel_hook && !strcmp(cat, "FrontEndNavigation") && !strcmp(act, "ToFrontEnd")) g_dm_charsel_hook(); }
    if (!strcmp(cat, "PanelManager") && !strcmp(act, "ClosePanel") && !strcmp(arg, "TouchShareLog")) {
        LOG("诊断日志：点了「分享诊断日志」%s", g_sharelog_cb ? "" : "（外壳没登记，不弹）"); if (g_sharelog_cb) g_sharelog_cb(); }
    {
        static const char *on, *off; static int rd; void mtl_world_frozen(int);
        if (!rd) { rd = 1; const char *e = getenv("DM_TEST_FRZ"); if (e && *e) { char *d = strdup(e), *g = strchr(d, '>'); if (g) { *g = 0; off = g + 1; } on = d; } }
        if (on) { char m[256]; snprintf(m, sizeof m, "%s:%s:%s", cat, act, arg);
            if (strstr(m, on)) { LOG("DM_TEST_FRZ：「%s」命中 → 世界冻结状态位 开", m); mtl_world_frozen(1); }
            else if (off && strstr(m, off)) { LOG("DM_TEST_FRZ：「%s」命中 → 世界冻结状态位 关", m); mtl_world_frozen(0); } }
    }
    if (!strcmp(cat, "PanelManager") && !strcmp(arg, "CharacterCreateNamePanel")) { extern int g_swkbd_name_panel; g_swkbd_name_panel = !strcmp(act, "OpenPanel") || !strcmp(act, "OpenExclusivePanel"); }
    { void dm_ui_on_uimsg(const char *, const char *, const char *); dm_ui_on_uimsg(cat, act, arg); }
    if (strcmp(arg, "LoadScreenPanel")) return;
    pthread_mutex_lock(&g_shotgate_mu);
    if (!strcmp(act, "OpenPanel") || !strcmp(act, "OpenExclusivePanel")) {
        g_loadscreen_seen = 1; g_loadscreen_open = 1; g_loadscreen_unload_pending = 0; g_loadscreen_generation++; prof_load_begin();
        pthread_mutex_unlock(&g_shotgate_mu); return;
    }
    if (strcmp(act, "UnloadPanel") && strcmp(act, "ClosePanel")) { pthread_mutex_unlock(&g_shotgate_mu); return; }
    g_loadscreen_seen = 1; g_loadscreen_generation++;
    if (!g_loadscreen_delay_ms) {
        g_loadscreen_open = 0; g_loadscreen_unload_pending = 0;
        pthread_mutex_unlock(&g_shotgate_mu); LOG("读取画面已撤"); prof_load_end(); lhold_release(); return;
    }
    g_loadscreen_unload_pending = 1;
    unsigned *generation = malloc(sizeof *generation); *generation = g_loadscreen_generation;
    pthread_mutex_unlock(&g_shotgate_mu);
    LOG("DM_TEST_LOADSCREEN_DELAY_MS：读取画面撤下延迟 %d ms", g_loadscreen_delay_ms);
    pthread_t th; if (!pthread_create(&th, NULL, loadscreen_delayed_close, generation)) pthread_detach(th); else free(generation);
}


static int shotui_wait_needed(const char *trigger) {
    int seen;
    pthread_mutex_lock(&g_shotgate_mu); seen = g_loadscreen_seen; pthread_mutex_unlock(&g_shotgate_mu);
    if (strstr(trigger, "LoadScreenPanel")) { LOG("SHOT 前置：不适用（截读取画面本身）"); return 0; }
    if (strstr(trigger, "MainMenuButtonRibbon")) { LOG("SHOT 前置：不适用（主菜单页）"); return 0; }
    if (!seen) { LOG("SHOT 前置：本局无读取画面"); return 0; }
    return 1;
}
struct shotui_wait_arg { int index; char label[96]; };
static void *shotui_wait_loadscreen(void *arg) {
    struct shotui_wait_arg *a = arg; uint64_t start = now_ns();
    uint64_t not_before = start + (uint64_t)g_loadscreen_delay_ms * 1000000ull;
    for (;;) {
        pthread_mutex_lock(&g_shotgate_mu); int closed = !g_loadscreen_open && !g_loadscreen_unload_pending; pthread_mutex_unlock(&g_shotgate_mu);
        if (closed && now_ns() >= not_before) {
            LOG("SHOT 前置：读取画面已撤 拍 %s 等待 %llu ms", a->label, (unsigned long long)((now_ns() - start) / 1000000));
            { module_t *mm = main_mod(); if (mm) LOG("SHOT 时原版手柄光标 (%d,%d)（0,0 = 该界面没有自由光标，走焦点导航）", *(int32_t *)(mm->base + 0x2d128f8), *(int32_t *)(mm->base + 0x2d128fc)); }
            dm_gfx_shot_at(g_shotui[a->index].off, g_shotui[a->index].path); free(a); return NULL;
        }
        if (now_ns() - start >= 15ull * 1000000000ull) { LOG("读取画面未撤（等待 15 秒）"); free(a); return NULL; }
        usleep(10000);
    }
}
static void shotui_on_uimsg(const char *cat, const char *act, const char *arg) {
    char m[256]; snprintf(m, sizeof m, "%s:%s:%s", cat, act, arg);
    for (int i = 0; i < g_nshotui; i++) if (!g_shotui[i].done && strstr(m, g_shotui[i].trig)) {
        g_shotui[i].done = 1; LOG("DM_NVN_SHOT_UI：「%s」命中 → 第 +%ld 帧截图 %s", m, g_shotui[i].off, g_shotui[i].path);
        if (!shotui_wait_needed(g_shotui[i].trig)) dm_gfx_shot_at(g_shotui[i].off, g_shotui[i].path);
        else {
            struct shotui_wait_arg *w = calloc(1, sizeof *w); w->index = i; strlcpy(w->label, g_shotui[i].trig, sizeof w->label);
            pthread_t th; if (!pthread_create(&th, NULL, shotui_wait_loadscreen, w)) pthread_detach(th); else free(w);
        }
    }
}
static void shotui_cancel_all(void) {
    for (int i = 0; i < g_nshotui; i++) if (!g_shotui[i].done) { g_shotui[i].done = 1; LOG("DM_NVN_SHOT_UI：坐标核对不一致，撤掉截图 %s", g_shotui[i].path); }
}
static void shotui_install(void) {


    void uimsg_subscribe(void (*)(const char *, const char *, const char *));
    const char *delay = getenv("DM_TEST_LOADSCREEN_DELAY_MS"); g_loadscreen_delay_ms = delay ? atoi(delay) : 0;
    uimsg_subscribe(loadscreen_on_uimsg);
    { extern uint32_t (*dm_thread_tick_fn)(void); dm_thread_tick_fn = dm_cur_tick; }
    { extern int (*g_nvn_loadscreen_up)(void); g_nvn_loadscreen_up = dm_loadscreen_up; }
    const char *e = getenv("DM_NVN_SHOT_UI"); if (!e) return;
    char buf[4096]; strlcpy(buf, e, sizeof buf);
    for (char *sv, *tok = strtok_r(buf, ",", &sv); tok; tok = strtok_r(NULL, ",", &sv)) {
        char *plus = strchr(tok, '+'), *colon = plus ? strchr(plus, ':') : NULL;
        if (!plus || !colon || plus == tok || !colon[1]) die("DM_NVN_SHOT_UI 项「%s」格式：<触发>+<帧偏移>:<路径>", tok);
        if (g_nshotui == SHOTUI_MAX) die("DM_NVN_SHOT_UI 超过 %d 项", SHOTUI_MAX);
        typeof(g_shotui[0]) *x = &g_shotui[g_nshotui++];
        snprintf(x->trig, sizeof x->trig, "%.*s", (int)(plus - tok), tok); x->off = atol(plus + 1); strlcpy(x->path, colon + 1, sizeof x->path);
    }
    uimsg_subscribe(shotui_on_uimsg);
    LOG("DM_NVN_SHOT_UI：%d 项", g_nshotui);
}


void dm_note_loadscreen_unload(void);
static void res_on_uimsg(const char *cat, const char *act, const char *arg) { (void)cat; if (!strcmp(act, "UnloadPanel") && !strcmp(arg, "LoadScreenPanel")) { dm_note_loadscreen_unload(); l3mem_request(in_game() ? cur_tick() : 0); res_report("读取画面已撤"); } }
void ticks_install(void) {
    const char *e = getenv("DM_TICKS"), *rp = getenv("DM_C2S_REPLAY"), *lg = getenv("DM_C2S_LOG"), *sp = getenv("DM_STATE");
    const char *pn = getenv("DM_PLAYERS");
    if (pn && *pn) { dm_hud_request_players(atoi(pn)); LOG("DM_PLAYERS：起局请求档位 %d", dm_hud_requested_players()); }
    if ((e || rp || lg) && gate_obs_present(OBS_UIMSG)) { void uimsg_subscribe(void (*)(const char *, const char *, const char *)); uimsg_subscribe(res_on_uimsg); }
    soak_install();
    shotui_install();
    if (lg) {
        if (!gate_obs_present(OBS_C2S)) die("DM_C2S_LOG：产物没有命令分派观测门 OBS_C2S（需 r9 起的产物）");
        if (!(g_c2sfp = fopen(lg, "w"))) die("DM_C2S_LOG 打不开 %s", lg);
        gate_obs_set(OBS_C2S, c2s_obs); LOG("DM_C2S_LOG → %s", lg);
        if (gate_obs_present(OBS_C2S_SYS)) gate_obs_set(OBS_C2S_SYS, c2s_sys_obs); else LOG("【红】DM_C2S_LOG：产物没有系统类消息观测门 OBS_C2S_SYS（需 r10 起），系统类消息不记");
    }
    if (rp) {
        if (!gate_svc_present(0x81)) die("DM_C2S_REPLAY：产物没有收包投递剪口（svc #0x81，需 r9 起的产物）");
        if (!gate_svc_present(0x82)) LOG("【红】DM_C2S_REPLAY：产物没有系统类消息剪口（svc #0x82，需 r10 起），宿主客户端自发的系统类消息无法丢弃");
        if (gate_obs_present(OBS_C2S_SYS) && !g_c2sfp) gate_obs_set(OBS_C2S_SYS, c2s_sys_obs);
        c2s_load(rp);
    }
    if (e) {
        if (!(g_tkfp = fopen(e, "w"))) die("DM_TICKS 打不开 %s", e);
        const char *nn = getenv("DM_TICKS_N"); if (nn) g_tkleft = atol(nn);
        g_tkfull = getenv("DM_TICKS_FULL") != NULL;
        LOG("DM_TICKS → %s%s%s", e, nn ? "，拍数 " : "", nn ? nn : "");
    }
    if (sp) {
        if (!(g_spfp = fopen(sp, "w"))) die("DM_STATE 打不开 %s", sp);
        LOG("DM_STATE → %s", sp);
    }






    if (gate_obs_present(OBS_C2S)) gate_obs_set(OBS_C2S, c2s_obs);
    else LOG("【红】产物没有命令分派观测门 OBS_C2S：首领连战的门点了不换关");
#ifndef DM_PLAYER_BUILD
    { void dm_combat_probe_install(void); dm_combat_probe_install(); }
#endif
    if (gate_obs_present(OBS_TICK)) gate_obs_set(OBS_TICK, tick_gate_product);
    else LOG("【红】产物没有拍入口观测门 OBS_TICK：底栏数字 / 难度条 / 动态 3D 比例（dm_ingame_now）都不工作");
    { module_t *tm = main_mod(); void dm_textbox_install(uint8_t *); if (tm) dm_textbox_install((uint8_t *)tm->base); }
    { module_t *tm = main_mod(); void dm_turnsmooth_install(uint8_t *); if (tm) dm_turnsmooth_install((uint8_t *)tm->base); }
    if (e || rp || lg || sp) {
        if (!gate_obs_present(OBS_TICK)) die("DM_TICKS / DM_STATE / DM_C2S_REPLAY / DM_C2S_LOG：产物没有拍入口观测门 OBS_TICK（需 r8 起的产物）");
        gate_obs_set(OBS_TICK, tick_gate);
        void uimsg_subscribe(void (*)(const char *, const char *, const char *));
        const char *pc = getenv("DM_POSCHK");
        if (pc) { if (sscanf(pc, "%u:%u:%u:%u:%u", &g_pos_tick, &g_pos_want[0], &g_pos_want[1], &g_pos_want[2], &g_pos_want[3]) != 5) die("DM_POSCHK 格式：E:x:y:xf:yf（得到「%s」）", pc);
                  g_pos_on = 1; LOG("DM_POSCHK：拍 %u 核玩家坐标 (%u,%u)/(%u,%u)", g_pos_tick, g_pos_want[0], g_pos_want[1], g_pos_want[2], g_pos_want[3]); }
        const char *h = getenv("DM_TICK_HOLD");
        if (h) { g_hold_tick = (uint32_t)strtoul(h, 0, 0); const char *c = strchr(h, ':'); if (c) snprintf(g_hold_ui, sizeof g_hold_ui, "%s", c + 1);
                 uimsg_subscribe(hold_on_uimsg); LOG("DM_TICK_HOLD：拍 %u 冻结客体时钟直到「%s」", g_hold_tick, g_hold_ui); }
    }
}
