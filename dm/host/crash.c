#include <TargetConditionals.h>




#include "dm.h"
#include "dm_uctx.h"
#include <signal.h>
#include <dlfcn.h>
#include <sys/ucontext.h>
#include <unistd.h>
#include <sys/mman.h>
#include <libkern/OSCacheControl.h>
#include <mach/mach.h>
#include <stdatomic.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>













typedef struct { char b[512]; int n; } cw_t;
static void cw_flush(cw_t *w) {
    for (int o = 0; o < w->n; ) {
        ssize_t k = write(2, w->b + o, (size_t)(w->n - o));
        if (k > 0) o += (int)k;
        else if (!(k < 0 && errno == EINTR)) break;
    }
    w->n = 0;
}
static void cw_s(cw_t *w, const char *s) { for (; *s; s++) { if (w->n == (int)sizeof w->b) cw_flush(w); w->b[w->n++] = *s; } }
static void cw_hex(cw_t *w, uint64_t v) {
    char t[19]; int i = 18; t[i] = 0;
    do { t[--i] = "0123456789abcdef"[v & 15]; v >>= 4; } while (v);
    t[--i] = 'x'; t[--i] = '0'; cw_s(w, t + i);
}
static void cw_dec(cw_t *w, long long v) {
    char t[24]; int i = 23; t[i] = 0; unsigned long long u = v < 0 ? 0ull - (unsigned long long)v : (unsigned long long)v;
    do { t[--i] = (char)('0' + u % 10); u /= 10; } while (u);
    if (v < 0) t[--i] = '-';
    cw_s(w, t + i);
}
#if !defined(__APPLE__)



static uint64_t cw_hexval(const char **p) {
    uint64_t v = 0;
    for (;; (*p)++) {
        char c = **p; int d = c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : -1;
        if (d < 0) return v;
        v = v << 4 | (uint64_t)d;
    }
}
static int cw_map(cw_t *w, uint64_t a) {
    int fd = open("/proc/self/maps", O_RDONLY | O_CLOEXEC); if (fd < 0) return 0;
    char buf[4096], line[512]; int ln = 0, found = 0; ssize_t k;
    while (!found && (k = read(fd, buf, sizeof buf)) > 0) {
        for (ssize_t i = 0; i < k && !found; i++) {
            if (buf[i] != '\n') { if (ln < (int)sizeof line - 1) line[ln++] = buf[i]; continue; }
            line[ln] = 0; ln = 0;
            const char *p = line; uint64_t lo = cw_hexval(&p); if (*p++ != '-') continue; uint64_t hi = cw_hexval(&p);
            if (a < lo || a >= hi) continue;
            while (*p == ' ') p++; while (*p && *p != ' ') p++; while (*p == ' ') p++;
            uint64_t fo = cw_hexval(&p);
            const char *path = line, *q = line; for (; *q; q++) if (*q == '/' || *q == '[') { path = q; break; }
            if (path == line) continue;
            const char *base = path; for (q = path; *q; q++) if (*q == '/') base = q + 1;
            cw_s(w, base); cw_s(w, "+"); cw_hex(w, a - lo + fo); found = 1;
        }
    }
    close(fd);
    return found;
}
#endif


static void cw_addr(cw_t *w, const char *tag, uint64_t a) {
    cw_s(w, " "); if (*tag) { cw_s(w, tag); cw_s(w, "="); }
    module_t *m = module_of((void *)a);
    if (m) { cw_s(w, m->name); cw_s(w, "+"); cw_hex(w, a - (uint64_t)m->base); return; }
#if !defined(__APPLE__)
    if (cw_map(w, a)) return;
#endif
    cw_hex(w, a);
}

static _Atomic uint64_t g_watch_last_ns;
static _Atomic uint64_t g_watch_count;
static _Atomic uint32_t g_watch_tick;
static _Atomic int g_watch_pregame = 1;
static _Atomic int g_watch_stage_idx;
static char g_watch_stage[2][256] = {"启动", "启动"};
static unsigned g_watch_stall;
static void (*g_dump_hook)(void);
static void (*g_wait_dump)(void);

static uint64_t watch_now_ns(void) {
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}
uint64_t crash_watch_progress_count(void) { return atomic_load(&g_watch_count); }
void crash_watch_progress(uint32_t tick, const char *stage) {
    int next = 1 - atomic_load(&g_watch_stage_idx);
    if (stage && *stage) snprintf(g_watch_stage[next], sizeof g_watch_stage[next], "%s", stage);
    atomic_store(&g_watch_tick, tick);
    atomic_store(&g_watch_stage_idx, next);
    atomic_fetch_add(&g_watch_count, 1);
    atomic_store(&g_watch_last_ns, watch_now_ns());
}
void crash_watch_frame(void) { if (atomic_load(&g_watch_pregame)) crash_watch_progress(0, "出图帧"); }
void crash_watch_pregame_done(void) { atomic_store(&g_watch_pregame, 0); }
void crash_watch_pregame_again(void) { atomic_store(&g_watch_pregame, 1); }
void crash_dump_now(const char *why);
static void *watchdog_main(void *arg) {
    (void)arg;
    for (;;) {
        usleep(250000);




        uint64_t last = atomic_load(&g_watch_last_ns), now = watch_now_ns();
        if (last > now) continue;




        { static int dumps; static uint64_t last_dump, dumped_for;
          if (g_watch_stall && !atomic_load(&g_watch_pregame) && atomic_load(&g_watch_tick) > 0 && now - last >= 1000000000ull && now - last < (uint64_t)g_watch_stall * 1000000000ull
              && dumps < 3 && dumped_for != last && (!last_dump || now - last_dump >= 10000000000ull)) { dumps++; last_dump = now; dumped_for = last; crash_dump_now("进展停了 1 秒以上"); } }
        if (g_watch_stall && now - last >= (uint64_t)g_watch_stall * 1000000000ull) {
            uint64_t sec = (now - last) / 1000000000ull;
            int idx = atomic_load(&g_watch_stage_idx);
            LOG("【红】看门狗：第 %llu 秒起无进展，卡在 拍 %u / 阶段 %s", (unsigned long long)sec,
                atomic_load(&g_watch_tick), g_watch_stage[idx]);
            if (g_dump_hook) g_dump_hook();
            if (g_wait_dump) g_wait_dump();
#if TARGET_OS_OSX
            { const char *d = getenv("DM_WATCH_SAMPLE");
              if (d) { char c[1200]; snprintf(c, sizeof c, "/usr/bin/sample %d 1 -f '%s' >/dev/null 2>&1", (int)getpid(), d); if (system(c)) LOG("看门狗：采样失败"); else LOG("看门狗：线程栈已写 %s", d); } }
#endif
            _exit(128 + SIGALRM);
        }
    }
    return NULL;
}
void crash_off(const char *tag, uint64_t a) {
#define off crash_off

    module_t *m = module_of((void *)a);
    Dl_info di = {0};
    if (m) fprintf(stderr, " %s%s%s+%#llx", tag, *tag ? "=" : "", m->name, a - (uint64_t)m->base);
    else if (dladdr((void *)a, &di) && di.dli_sname) fprintf(stderr, " %s%s%s+%#lx", tag, *tag ? "=" : "", di.dli_sname, (unsigned long)(a - (uint64_t)di.dli_saddr));



    else if (di.dli_fname && di.dli_fbase) { const char *n = strrchr(di.dli_fname, '/'); fprintf(stderr, " %s%s%s+%#llx", tag, *tag ? "=" : "", n ? n + 1 : di.dli_fname, (unsigned long long)(a - (uint64_t)di.dli_fbase)); }
    else fprintf(stderr, " %s%s%#llx", tag, *tag ? "=" : "", a);
}


void dm_backtrace(uint64_t fp) {
    fprintf(stderr, "[dm] bt:");
    for (int i = 0; i < 32 && fp && !(fp & 7); i++) {
        uint64_t nfp = ((uint64_t *)fp)[0], ra = ((uint64_t *)fp)[1];
        off("", ra);
        if (nfp <= fp) break;
        fp = nfp;
    }
    fputc('\n', stderr);
}






#define MAX_FAULT_HOOK 4
static int (*g_fault_hook[MAX_FAULT_HOOK])(void *addr, int is_write, void *uc);
static void (*g_write_prep[MAX_FAULT_HOOK])(void *addr, size_t len);
static int g_nfault_hook, g_nwrite_prep;
void dm_fault_hook_register(int (*fn)(void *addr, int is_write, void *uc)) {
    if (g_nfault_hook >= MAX_FAULT_HOOK) die("缺页钩子过多");
    g_fault_hook[g_nfault_hook++] = fn;
}
static void (*g_res_report)(uint32_t tick, const char *why);
void dm_res_report_register(void (*fn)(uint32_t tick, const char *why)) { g_res_report = fn; }
void (*dm_res_report_cb(void))(uint32_t, const char *) { return g_res_report; }
void dm_guest_write_prep_register(void (*fn)(void *addr, size_t len)) {
    if (g_nwrite_prep >= MAX_FAULT_HOOK) die("宿主写前钩子过多");
    g_write_prep[g_nwrite_prep++] = fn;
}


void dm_guest_write_prepare(void *addr, size_t len) { for (int i = 0; i < g_nwrite_prep; i++) g_write_prep[i](addr, len); }



static int (*g_trap_hook)(ucontext_t *uc);
void crash_set_trap_hook(int (*fn)(ucontext_t *uc)) { g_trap_hook = fn; }
void crash_set_dump_hook(void (*fn)(void)) { g_dump_hook = fn; }
void crash_set_wait_dump(void (*fn)(void)) { g_wait_dump = fn; }


void crash_dump_now(const char *why) { fprintf(stderr, "[dm] 现场导出（%s）：拍 %u / 阶段 %s\n", why, atomic_load(&g_watch_tick), g_watch_stage[atomic_load(&g_watch_stage_idx)]); if (g_dump_hook) g_dump_hook(); if (g_wait_dump) g_wait_dump(); }





static volatile sig_atomic_t g_crash_sig;
static void crash_deadman(int sig) {
    (void)sig;
    static const char msg[] = "[dm] CRASH：现场导出 3 秒没走完（多半卡在分配器或 stdio 的锁上），直接退出 | the crash dump did not finish in 3 s (likely stuck on an allocator or stdio lock), exiting\n";
    ssize_t k = write(2, msg, sizeof msg - 1); (void)k;
    _exit(128 + g_crash_sig);
}
static void on_crash(int sig, siginfo_t *si, void *uc_) {
    ucontext_t *uc = uc_;
    if (sig == SIGTRAP && g_trap_hook && g_trap_hook(uc)) return;
    if (sig == SIGSEGV || sig == SIGBUS) {
        int is_write = (int)((UC_ESR(uc) >> 6) & 1);
        for (int i = 0; i < g_nfault_hook; i++) if (g_fault_hook[i](si->si_addr, is_write, uc)) return;
    }


    { cw_t w; w.n = 0;
      cw_s(&w, "[dm] CRASH sig="); cw_dec(&w, sig);
      cw_addr(&w, "addr", (uint64_t)si->si_addr); cw_addr(&w, "pc", UC_PC(uc)); cw_addr(&w, "lr", UC_LR(uc));
      cw_s(&w, " sp="); cw_hex(&w, UC_SP(uc)); cw_s(&w, "\n");
      for (int i = 0; i < 29; i++) { cw_s(&w, "x"); cw_dec(&w, i); cw_s(&w, "="); cw_hex(&w, UC_X(uc, i)); cw_s(&w, i % 6 == 5 ? "\n" : " "); }
      cw_s(&w, "\n"); cw_flush(&w);



      cw_s(&w, "[dm] bt（信号安全 | signal-safe）:");
      uint64_t fp = UC_FP(uc);
      for (int i = 0; i < 32 && fp && !(fp & 7); i++) {
          uint64_t nfp = ((uint64_t *)fp)[0], ra = ((uint64_t *)fp)[1];
          cw_addr(&w, "", ra);
          if (nfp <= fp) break;
          fp = nfp;
      }
      cw_s(&w, "\n"); cw_flush(&w); }


    g_crash_sig = sig;
    struct sigaction da = {.sa_handler = crash_deadman}; sigemptyset(&da.sa_mask); sigaction(SIGALRM, &da, 0); alarm(3);
    fprintf(stderr, "[dm] CRASH 符号 | symbols:");
    off("addr", (uint64_t)si->si_addr); off("pc", UC_PC(uc)); off("lr", UC_LR(uc));
    fprintf(stderr, "\n");
    dm_backtrace(UC_FP(uc));
    if (g_dump_hook) g_dump_hook();
    _exit(128 + sig);
}




#if !TARGET_OS_TV
static pthread_key_t g_altstack_key;
static pthread_once_t g_altstack_once = PTHREAD_ONCE_INIT;
static void altstack_free(void *p) { stack_t off = {.ss_flags = SS_DISABLE}; sigaltstack(&off, 0); free(p); }
static void altstack_key_init(void) { if (pthread_key_create(&g_altstack_key, altstack_free)) die("altstack key"); }
void crash_thread_init(void) {
    pthread_once(&g_altstack_once, altstack_key_init);
    if (pthread_getspecific(g_altstack_key)) return;
    stack_t ss = {.ss_sp = malloc(65536), .ss_size = 65536}; sigaltstack(&ss, 0);
    pthread_setspecific(g_altstack_key, ss.ss_sp);
}
#define CRASH_SA_ONSTACK SA_ONSTACK
#else
void crash_thread_init(void) {}
#define CRASH_SA_ONSTACK 0
#endif
void crash_install(void) {
    crash_thread_init();
    struct sigaction sa = {.sa_sigaction = on_crash, .sa_flags = SA_SIGINFO | CRASH_SA_ONSTACK};
    sigaction(SIGALRM, &sa, 0);




    const char *watch = getenv("DM_WATCH"), *stall = getenv("DM_WATCH_STALL");
    if (watch) alarm(atoi(watch));
    if (watch || stall) {
        g_watch_stall = stall && atoi(stall) > 0 ? (unsigned)atoi(stall) : 15;
        atomic_store(&g_watch_last_ns, watch_now_ns());
        pthread_t th; if (pthread_create(&th, NULL, watchdog_main, NULL)) die("看门狗线程");
        pthread_detach(th);
    }
    sigaction(SIGSEGV, &sa, 0); sigaction(SIGBUS, &sa, 0); sigaction(SIGILL, &sa, 0); sigaction(SIGTRAP, &sa, 0);
}
