



















#include "dm.h"
#include <errno.h>
#include <sched.h>
#include <signal.h>
#include <sys/mman.h>
#include <time.h>
#include <mach/mach.h>
#include <pthread/qos.h>
#include <dlfcn.h>
#include <setjmp.h>

#define R_TIMEOUT     0xEA01u
#define R_CANCELLED   0xEC01u
#define R_BAD_HANDLE  0xE401u
#define R_BAD_STATE   0xFA01u
#define R_OUT_OF_RES  0xCE01u
#define WAITERS_BIT   0x40000000u
#define H_CUR_THREAD  0xFFFF8000u

enum { O_FREE, O_THREAD, O_EVENT_R, O_EVENT_W };
typedef struct event { int signaled; int refs; } event_t;
typedef struct thr {
    uint32_t handle; uint64_t id;
    uint64_t entry, arg, stack_top; int prio, core; uint64_t mask;
    uint64_t born_ns, exit_ns; uint32_t born_tick, exit_tick;
    int started, exited, cancel;
    pthread_t pt; pthread_cond_t cv;


    int wkind, woke; uint64_t waddr, wkey; uint32_t wtag; int64_t wres;


    int spinning;
    uint64_t wseq;
    struct thr *next;
    struct thr *all;
    int last_svc; uint64_t last_lr;
    int closed;
    uint64_t nsvc;
    mach_port_t mport;
    uint32_t qos;
} thr_t;
static thr_t *g_all;

static pthread_mutex_t G = PTHREAD_MUTEX_INITIALIZER;
#define MAX_H 4096
static struct { int type; void *obj; } g_h[MAX_H];
static pthread_key_t g_keythr;








static pthread_key_t g_keyhome;
static uint64_t g_next_id = 0x51;
static thr_t *g_wait;

static uint32_t h_new(int type, void *obj) {
    for (int i = 1; i < MAX_H; i++) if (!g_h[i].type) { g_h[i].type = type; g_h[i].obj = obj; return 0x1000 + i; }
    die("句柄表满");
}
static void *h_peek(uint32_t h, int type) {
    if (h == H_CUR_THREAD && type == O_THREAD) return pthread_getspecific(g_keythr);
    unsigned i = h - 0x1000;
    if (i == 0 || i >= MAX_H || g_h[i].type != type) return NULL;
    return g_h[i].obj;
}
static void *h_get(uint32_t h, int type) {
    void *o = h_peek(h, type);
    if (!o) LOG("句柄无效 %#x（要类型 %d）", h, type);
    return o;
}
static thr_t *cur(void) {
    thr_t *t = pthread_getspecific(g_keythr);
    if (!t) die("非客体线程调用了线程 SVC");
    return t;
}



static void wait_unlink(thr_t *t) {
    for (thr_t **p = &g_wait; *p; p = &(*p)->next) if (*p == t) { *p = t->next; break; }
    t->next = NULL;
}


static uint64_t g_wseq;
static thr_t *pick(int kind, uint64_t a) {
    thr_t *b = NULL;
    for (thr_t *t = g_wait; t; t = t->next) {
        if (t->wkind != kind || (kind == 2 ? t->wkey : t->waddr) != a) continue;
        if (!b || t->prio < b->prio || (t->prio == b->prio && t->wseq < b->wseq)) b = t;
    }
    return b;
}





static uint64_t g_wakes, g_wake_skips;
void dm_hle_wake_counts(uint64_t *wakes, uint64_t *skips) { *wakes = __atomic_load_n(&g_wakes, __ATOMIC_RELAXED); *skips = __atomic_load_n(&g_wake_skips, __ATOMIC_RELAXED); }
static void wake(thr_t *t, int64_t res) {
    wait_unlink(t); t->wkind = 0; t->wres = res;


    __atomic_store_n(&t->woke, 1, __ATOMIC_RELEASE);
    __atomic_store_n(&g_wakes, g_wakes + 1, __ATOMIC_RELAXED);
    if (t->spinning) __atomic_store_n(&g_wake_skips, g_wake_skips + 1, __ATOMIC_RELAXED);
    else pthread_cond_signal(&t->cv);
}


static int64_t block_inner(thr_t *t, int kind, int64_t ns);
static int64_t block(thr_t *t, int kind, int64_t ns) {
    if (!g_flag.bootprof) return block_inner(t, kind, ns);
    uint64_t t0 = now_ns(); int64_t r = block_inner(t, kind, ns); uint64_t d = now_ns() - t0;
    static const int id[] = {0, PROF_WAIT_LOCK, PROF_WAIT_CV, PROF_WAIT_ADDR, PROF_WAIT_SYNC};
    dm_prof_add(id[kind & 7], d, 0);
    if (t->handle == 0x1001) dm_prof_add(PROF_MAIN_WAIT, d, 0);
    return r;
}











static uint64_t g_spin_ns; static int g_spin_init; static int64_t g_spin_ab;
static int spin_phase_on(void) {
    if (g_spin_ab <= 0) return 1;
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); return ((ts.tv_sec / g_spin_ab) & 1) == 0;
}











static int g_quiet;
static int g_quiet_sleep;
static pthread_mutex_t g_quiet_m = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_quiet_cv = PTHREAD_COND_INITIALIZER;
void dm_hle_quiet(int on) {
    pthread_mutex_lock(&g_quiet_m); __atomic_store_n(&g_quiet_sleep, on != 0, __ATOMIC_SEQ_CST); pthread_cond_broadcast(&g_quiet_cv); pthread_mutex_unlock(&g_quiet_m);
    pthread_mutex_lock(&G);
    g_quiet = on != 0;
    if (!on) for (thr_t *t = g_wait; t; t = t->next) if (t->wkind) pthread_cond_signal(&t->cv);
    pthread_mutex_unlock(&G);
}
int dm_hle_quiet_on(void) { return __atomic_load_n(&g_quiet_sleep, __ATOMIC_RELAXED); }
static void quiet_sleep_gate(void) {
    if (!__atomic_load_n(&g_quiet_sleep, __ATOMIC_RELAXED)) return;
    pthread_mutex_lock(&g_quiet_m);
    while (__atomic_load_n(&g_quiet_sleep, __ATOMIC_SEQ_CST)) pthread_cond_wait(&g_quiet_cv, &g_quiet_m);
    pthread_mutex_unlock(&g_quiet_m);
}
static int64_t block_inner(thr_t *t, int kind, int64_t ns) {
    t->wkind = kind; t->woke = 0; t->next = g_wait; g_wait = t; t->wseq = ++g_wseq;
    if (ns == 0) { wait_unlink(t); t->wkind = 0; return R_TIMEOUT; }
    struct timespec dl;
    int det_wait = ns > 0 && dm_detclock_enabled();
    uint64_t guest_deadline = det_wait ? guest_ns() + (uint64_t)ns : 0;
    if (ns > 0 && !det_wait) { clock_gettime(CLOCK_REALTIME, &dl); dl.tv_sec += ns / 1000000000; dl.tv_nsec += ns % 1000000000;
                  if (dl.tv_nsec >= 1000000000) { dl.tv_sec++; dl.tv_nsec -= 1000000000; } }
    if (!g_spin_init) { const char *e = getenv("DM_HLE_SPIN_US"); g_spin_ns = e ? (uint64_t)atoi(e) * 1000 : 0; e = getenv("DM_HLE_SPIN_AB"); g_spin_ab = e ? atoi(e) : 0; g_spin_init = 1; }
    if (g_spin_ns && t->handle == 0x1001 && (ns < 0 || (uint64_t)ns > g_spin_ns) && spin_phase_on()) {
        t->spinning = 1;
        pthread_mutex_unlock(&G);
        uint64_t end = now_ns() + g_spin_ns;
        while (!__atomic_load_n(&t->woke, __ATOMIC_ACQUIRE) && now_ns() < end) {
#if defined(__aarch64__)
            __asm__ volatile("yield");
#endif
        }
        pthread_mutex_lock(&G);
        t->spinning = 0;
        if (t->woke) return t->wres;
    }
    while (!t->woke) {
        int rc;
        if (det_wait) {
            uint64_t now = guest_ns();
            if (now >= guest_deadline) { wait_unlink(t); t->wkind = 0; return R_TIMEOUT; }
            (void)(guest_deadline - now);
            rc = pthread_cond_wait(&t->cv, &G);
        } else rc = ns > 0 && !g_quiet ? pthread_cond_timedwait(&t->cv, &G, &dl) : pthread_cond_wait(&t->cv, &G);
        if (rc == ETIMEDOUT && !t->woke && (!det_wait || guest_ns() >= guest_deadline)) { wait_unlink(t); t->wkind = 0; return R_TIMEOUT; }
    }
    return t->wres;
}



static inline uint32_t ld32(uint64_t a) { return __atomic_load_n((uint32_t *)a, __ATOMIC_SEQ_CST); }
static inline void st32(uint64_t a, uint32_t v) { __atomic_store_n((uint32_t *)a, v, __ATOMIC_SEQ_CST); }
static inline int cas32(uint64_t a, uint32_t *exp, uint32_t v) { return __atomic_compare_exchange_n((uint32_t *)a, exp, v, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); }



static void lock_handoff(uint64_t addr) {
    thr_t *w = pick(1, addr); int more = 0;
    if (!w) { st32(addr, 0); return; }
    for (thr_t *t = g_wait; t; t = t->next) if (t != w && t->wkind == 1 && t->waddr == addr) { more = 1; break; }
    st32(addr, w->wtag | (more ? WAITERS_BIT : 0));
    wake(w, 0);
}


static void lock_acquire_for(thr_t *t, uint64_t addr, uint32_t tag) {
    uint32_t v = ld32(addr);
    for (;;) {
        if (v == 0) { if (cas32(addr, &v, tag)) { wake(t, 0); return; } continue; }
        if (cas32(addr, &v, v | WAITERS_BIT)) break;
    }
    t->wkind = 1; t->waddr = addr; t->wtag = tag;
    t->wseq = ++g_wseq;
}



extern void dm_thread_enter(uint64_t entry, uint64_t arg, uint64_t sp);
uint32_t (*dm_thread_tick_fn)(void);
void crash_thread_init(void);
void guest_thread_init(void);
static void *thread_main(void *p) {
    thr_t *t = p;
    pthread_setspecific(g_keythr, t);
    pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);
    qos_class_t qc = QOS_CLASS_UNSPECIFIED; int rp = 0;
    pthread_get_qos_class_np(pthread_self(), &qc, &rp); t->qos = (uint32_t)qc;
    t->mport = pthread_mach_thread_np(pthread_self());
    crash_thread_init();
    guest_thread_init();
    pthread_mutex_lock(&G); while (!t->started) pthread_cond_wait(&t->cv, &G); pthread_mutex_unlock(&G);



    sigjmp_buf home;
    pthread_setspecific(g_keyhome, &home);
    if (sigsetjmp(home, 0) == 0) {

        dm_thread_enter(t->entry, t->arg, t->stack_top);
        die("客体线程入口返回");
    }



    { static int n; int k = __atomic_add_fetch(&n, 1, __ATOMIC_RELAXED); if (k <= 20 || k % 50 == 0) LOG("客体线程回到宿主栈退出（第 %d 次）| guest thread exits on the host stack (#%d)", k, k); }
    pthread_exit(NULL);
}
static void thr_free(thr_t *t) {
    for (thr_t **p = &g_all; *p; p = &(*p)->all) if (*p == t) { *p = t->all; break; }
    pthread_cond_destroy(&t->cv); free(t);
}
static const char *thr_name(thr_t *t, char *buf, size_t n);
static void thread_exit(thr_t *t) {
    t->exit_ns = now_ns(); t->exit_tick = dm_thread_tick_fn ? dm_thread_tick_fn() : 0;


    pthread_mutex_lock(&G);
    t->exited = 1;
    for (thr_t *w = g_wait; w; ) { thr_t *n = w->next; if (w->wkind == 4) wake(w, -1); w = n; }
    pthread_setspecific(g_keythr, NULL);
    if (t->closed) thr_free(t);
    pthread_mutex_unlock(&G);


    sigjmp_buf *home = pthread_getspecific(g_keyhome);
    if (home) { pthread_setspecific(g_keyhome, NULL); siglongjmp(*home, 1); }
    pthread_exit(NULL);
}
void dm_thread_returned(void) { die("客体线程入口函数返回（Horizon 要求 ExitThread）"); }



uint32_t svc_thread_main_init(void) {
    pthread_key_create(&g_keythr, NULL);
    pthread_key_create(&g_keyhome, NULL);
    thr_t *t = calloc(1, sizeof *t);
    pthread_cond_init(&t->cv, NULL);
    t->started = 1; t->prio = 44; t->core = 0; t->mask = 7; t->pt = pthread_self(); t->mport = pthread_mach_thread_np(t->pt);
    pthread_mutex_lock(&G); t->handle = h_new(O_THREAD, t); t->id = g_next_id++; t->all = g_all; g_all = t; pthread_mutex_unlock(&G);
    pthread_setspecific(g_keythr, t);
    qos_class_t qc = QOS_CLASS_UNSPECIFIED; int rp = 0;
    pthread_get_qos_class_np(pthread_self(), &qc, &rp); t->qos = (uint32_t)qc;
    return t->handle;
}

static uint64_t g_main_yield_n, g_main_yield_ns;
void dm_main_yield_stats(uint64_t *n, uint64_t *ns) { *n = g_main_yield_n; *ns = g_main_yield_ns; }
uint32_t dm_guest_work_qos(unsigned *count) {
    uint32_t qos = QOS_CLASS_UNSPECIFIED; unsigned n = 0;
    pthread_mutex_lock(&G);
    for (thr_t *t = g_all; t; t = t->all) {
        if (t->handle == 0x1001) continue;
        if (!n) qos = t->qos; else if (qos != t->qos) qos = UINT32_MAX;
        n++;
    }
    pthread_mutex_unlock(&G);
    if (count) *count = n;
    return qos;
}



static int sig_obj(uint32_t h, int *bad) {
    unsigned i = h - 0x1000;
    if (h == H_CUR_THREAD) return 0;
    if (i == 0 || i >= MAX_H) { *bad = 1; return 0; }
    switch (g_h[i].type) {
    case O_THREAD: return ((thr_t *)g_h[i].obj)->exited;
    case O_EVENT_R: return ((event_t *)g_h[i].obj)->signaled;
    default: *bad = 1; return 0;
    }
}
static void wake_sync_waiters(void) {
    for (thr_t *w = g_wait; w; ) { thr_t *n = w->next; if (w->wkind == 4) wake(w, -1); w = n; }
}

static int64_t now_mono_ns(void) { struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); return ts.tv_sec * 1000000000ll + ts.tv_nsec; }
void dm_guest_clock_advanced(void) {
    if (!dm_detclock_enabled()) return;
    pthread_mutex_lock(&G);
    for (thr_t *t = g_wait; t; t = t->next) if (t->wkind) pthread_cond_signal(&t->cv);
    pthread_mutex_unlock(&G);
}



static const char *thr_name(thr_t *t, char *nb, size_t n) {
    if (!t->arg) return t->handle == 0x1001 ? "主线程" : "?";
    uint64_t np = 0; vm_size_t got = 0;
    if (vm_read_overwrite(mach_task_self(), t->arg + 0x1a0, 8, (vm_address_t)&np, &got) || !np ||
        vm_read_overwrite(mach_task_self(), np, n - 1, (vm_address_t)nb, &got)) return "?";
    nb[got] = 0; return nb;
}


int thr_list(pthread_t *out, int max) {
    int n = 0;
    pthread_mutex_lock(&G);
    for (thr_t *t = g_all; t && n < max; t = t->all) if (!t->exited && t->started) out[n++] = t->pt;
    pthread_mutex_unlock(&G);
    return n;
}


void thr_counts(int *handles, int *live, int *objs) {
    int nh = 0, nl = 0, no = 0;
    pthread_mutex_lock(&G);
    for (int i = 1; i < MAX_H; i++) nh += g_h[i].type != O_FREE;
    for (thr_t *t = g_all; t; t = t->all) { no++; nl += t->started && !t->exited; }
    pthread_mutex_unlock(&G);
    *handles = nh; *live = nl; *objs = no;
}


int thr_name_of(pthread_t pt, char *out, size_t n) {
    int ok = 0; char nb[64];
    pthread_mutex_lock(&G);
    for (thr_t *t = g_all; t; t = t->all) if (t->pt == pt && !t->exited) { snprintf(out, n, "%#x %s", t->handle, thr_name(t, nb, sizeof nb)); ok = 1; break; }
    pthread_mutex_unlock(&G);
    return ok;
}
uint32_t thr_cur_handle(void) { thr_t *t = pthread_getspecific(g_keythr); return t ? t->handle : 0; }


int thr_find_by_name(const char *name, pthread_t *out) {
    char nb[32]; int ok = 0;
    pthread_mutex_lock(&G);
    for (thr_t *t = g_all; t; t = t->all) if (!t->exited && !strcmp(thr_name(t, nb, sizeof nb), name)) { *out = t->pt; ok = 1; break; }
    pthread_mutex_unlock(&G);
    return ok;
}




static void thr_bt(thr_t *t) {
    if (!t->mport || t->exited || t->mport == mach_thread_self()) return;
    if (thread_suspend(t->mport) != KERN_SUCCESS) return;
    arm_thread_state64_t st; mach_msg_type_number_t n = ARM_THREAD_STATE64_COUNT;
    if (thread_get_state(t->mport, ARM_THREAD_STATE64, (thread_state_t)&st, &n) == KERN_SUCCESS) {




        if (t->wkind == 0) { char rl[400]; int q = snprintf(rl, sizeof rl, "[dm]   %#x 寄存器 pc=", t->handle); uint64_t pc = __darwin_arm_thread_state64_get_pc(st); module_t *pm = module_of((void *)pc);
            q += pm ? snprintf(rl + q, sizeof rl - q, "%s+%#llx", pm->name, pc - (uint64_t)pm->base) : snprintf(rl + q, sizeof rl - q, "%#llx", pc);
            for (int i = 19; i <= 28; i++) q += snprintf(rl + q, sizeof rl - q, " x%d=%#llx", i, st.__x[i]);
            fprintf(stderr, "%s\n", rl);
            for (int i = 19; i <= 20; i++) { uint8_t raw[128]; vm_size_t g = 0; uint64_t a = st.__x[i];
                if (a < 0x100000000ull || vm_read_overwrite(mach_task_self(), a, sizeof raw, (vm_address_t)raw, &g) || g != sizeof raw) continue;
                char hx[420]; int h = 0; for (int k = 0; k < 128; k += 4) h += snprintf(hx + h, sizeof hx - h, "%08x ", *(uint32_t *)(raw + k));
                uint64_t vt = *(uint64_t *)raw; module_t *vm_ = module_of((void *)vt); char vs[64] = ""; if (vm_) snprintf(vs, sizeof vs, "（首 8 字节 = %s+%#llx）", vm_->name, vt - (uint64_t)vm_->base);
                fprintf(stderr, "[dm]   %#x x%d → %#llx 起 128 字节%s: %s\n", t->handle, i, a, vs, hx); } }
        uint64_t fp = __darwin_arm_thread_state64_get_fp(st), lr = __darwin_arm_thread_state64_get_lr(st);


        if (t->wkind == 0 && module_of((void *)__darwin_arm_thread_state64_get_pc(st))) { uint64_t sp = __darwin_arm_thread_state64_get_sp(st), stk[128]; vm_size_t g = 0;
            if (sp > 0x100000000ull && !vm_read_overwrite(mach_task_self(), sp, sizeof stk, (vm_address_t)stk, &g) && g == sizeof stk)
                for (int r = 0; r < 128; r += 8) fprintf(stderr, "[dm]   %#x 栈 sp+%#05x: %016llx %016llx %016llx %016llx %016llx %016llx %016llx %016llx\n", t->handle, r * 8,
                        stk[r], stk[r + 1], stk[r + 2], stk[r + 3], stk[r + 4], stk[r + 5], stk[r + 6], stk[r + 7]); }
        char line[1024]; int p = snprintf(line, sizeof line, "[dm]   %#x 回溯:", t->handle), nf = 0; uint64_t wait_fp = 0;
        for (int k = 0; k < 64 && fp && nf < 20 && p < (int)sizeof line - 40; k++) {
            module_t *m = module_of((void *)lr);
            if (m) { p += snprintf(line + p, sizeof line - p, " %s+%#llx", m->name, lr - (uint64_t)m->base); nf++; }
            uint64_t fr[2]; vm_size_t g2 = 0;
            if (vm_read_overwrite(mach_task_self(), fp, 16, (vm_address_t)fr, &g2) || g2 != 16 || fr[0] <= fp) break;
            if (m && !strcmp(m->name, "main") && lr - (uint64_t)m->base == 0x132db30) wait_fp = fp;
            fp = fr[0]; lr = fr[1];
        }
        fprintf(stderr, "%s\n", line);






        if (wait_fp) { uint64_t wo[2] = {0, 0}, res[3] = {0, 0, 0}, wjob = 0; vm_size_t g = 0;
            #define RD(addr, buf) ((addr) > 0x100000000ull && !vm_read_overwrite(mach_task_self(), (addr), sizeof(buf), (vm_address_t)(buf), &g) && g == sizeof(buf))
            #define HEX(tag, addr, nbytes) { uint32_t raw[32]; vm_size_t gg = 0; if ((addr) > 0x100000000ull && !vm_read_overwrite(mach_task_self(), (addr), (nbytes), (vm_address_t)raw, &gg) && gg == (nbytes)) { \
                char hx[300]; int h = 0; for (unsigned q = 0; q < (nbytes) / 4; q++) h += snprintf(hx + h, sizeof hx - h, "%08x ", raw[q]); fprintf(stderr, "[dm]   %#x 作业池等待：%s %#llx: %s\n", t->handle, tag, (unsigned long long)(addr), hx); } \
                else fprintf(stderr, "[dm]   %#x 作业池等待：%s %#llx 读不到\n", t->handle, tag, (unsigned long long)(addr)); }
            if (RD(wait_fp - 0x40, wo)) { module_t *vm_ = module_of((void *)wo[0]);
                fprintf(stderr, "[dm]   %#x 作业池等待：等待对象 %#llx 虚表 %s+%#llx（应为 main+0x20f2408），所指 %#llx\n", t->handle, (unsigned long long)(wait_fp - 0x40), vm_ ? vm_->name : "?", vm_ ? wo[0] - (uint64_t)vm_->base : wo[0], wo[1]);
                if (RD(wo[1] - 0x10, res)) {
                    fprintf(stderr, "[dm]   %#x 作业池等待：资源等待对象 %#llx：在途作业 %#llx（%s）、互斥锁 %#llx、事件 %#llx。判别：在途作业为空 = 信号已被某次等待消费；非空且事件未置位 = 完成信号没来；事件已置位 = 等待循环的问题\n",
                            t->handle, (unsigned long long)(wo[1] - 0x10), res[0], res[0] ? "非空" : "为空", res[1], res[2]);
                    HEX("事件本体 64 字节", res[2], 64);
                    HEX("互斥锁本体 32 字节（+8 重入计数、+0x10 持有线程对象、+0x18 锁字 = 持有者句柄）", res[1], 32);


                    if (res[0]) { HEX("在途作业对象 +0 起 128 字节", res[0], 128); HEX("在途作业对象 +0x80 起 128 字节", res[0] + 0x80, 128); } wjob = res[0]; } }


            { uint64_t pc = __darwin_arm_thread_state64_get_pc(st); module_t *pm = module_of((void *)pc); uint64_t off = pm && !strcmp(pm->name, "main") ? pc - (uint64_t)pm->base : 0;


              if (off >= 0x14f0900 && off < 0x14f09b8) { uint64_t qa = st.__x[20] + 0x410, nd = 0; HEX("作业队列头（池 = x20，+0x410 起 32 字节）", qa, 32); int in = 0, cnt = 0;
                  if (RD(qa + 8, &nd)) for (; nd && cnt < 8; cnt++) { uint64_t nb[5] = {0}; if (!RD(nd, nb)) break; fprintf(stderr, "[dm]   %#x 作业池等待：  队列第 %d 个 %#llx 标志 %#x%s\n", t->handle, cnt, (unsigned long long)nd, (uint32_t)nb[2], nd == wjob ? "（就是被等的作业）" : ""); in |= nd == wjob; nd = nb[4]; }
                  fprintf(stderr, "[dm]   %#x 作业池等待：被等的作业%s x20 这个池的队列前 %d 个里\n", t->handle, in ? "在" : "不在", cnt); }
              else fprintf(stderr, "[dm]   %#x 作业池等待：线程不在等待循环自己的指令上，作业池没取\n", t->handle); }
            #undef RD
            #undef HEX
        }
    }
    thread_resume(t->mport);
}
void svc_thread_dump(void) {
    static const char *kinds[] = {"运行/未在等", "锁", "条件变量", "地址", "同步对象"};
    int nh = 0; for (int i = 1; i < MAX_H; i++) nh += g_h[i].type != O_FREE;
    fprintf(stderr, "[dm] 句柄表占用 %d / %d，累计建线程 %llu\n", nh, MAX_H, g_next_id - 0x51);
    for (thr_t *t = g_all; t; t = t->all) {
        module_t *m = module_of((void *)t->last_lr);


        char nb[32]; const char *nm = thr_name(t, nb, sizeof nb);
        double cpu = -1;
        if (t->mport && !t->exited) { thread_basic_info_data_t bi; mach_msg_type_number_t n = THREAD_BASIC_INFO_COUNT;
            if (thread_info(t->mport, THREAD_BASIC_INFO, (thread_info_t)&bi, &n) == KERN_SUCCESS)
                cpu = bi.user_time.seconds + bi.user_time.microseconds / 1e6 + bi.system_time.seconds + bi.system_time.microseconds / 1e6; }
        char own[96] = ""; if (t->wkind == 1 && t->waddr) { uint32_t w = 0; vm_size_t g2 = 0;
            if (!vm_read_overwrite(mach_task_self(), t->waddr, 4, (vm_address_t)&w, &g2)) snprintf(own, sizeof own, " 持有者 %#x", w & ~WAITERS_BIT); }
        if (t->wkind == 2 && t->waddr) { uint32_t w = 0, k = 0; vm_size_t g2 = 0;
            vm_read_overwrite(mach_task_self(), t->waddr, 4, (vm_address_t)&w, &g2); vm_read_overwrite(mach_task_self(), t->wkey, 4, (vm_address_t)&k, &g2);
            snprintf(own, sizeof own, " 锁字 %#x 条件键 %#llx=%#x", w, t->wkey, k); }
        uint8_t raw[64]; vm_size_t g3 = 0;
        int have_raw = (t->wkind == 1 || t->wkind == 2 || t->wkind == 3) && t->waddr && !vm_read_overwrite(mach_task_self(), t->waddr & ~0xfull, 64, (vm_address_t)raw, &g3) && g3 == 64;
        char life[96]; uint64_t nowns = now_ns();
        if (t->exited) snprintf(life, sizeof life, " 建于拍 %u、退于拍 %u（%.1f 秒前）", t->born_tick, t->exit_tick, (nowns - t->exit_ns) / 1e9); else if (!t->born_ns) snprintf(life, sizeof life, " 进程启动时就在"); else snprintf(life, sizeof life, " 建于拍 %u（%.1f 秒前）", t->born_tick, (nowns - t->born_ns) / 1e9);
        fprintf(stderr, "[dm] 线程 %#x 「%s」优先级 %d 核 %d 掩码 %#llx CPU %.2fs svc %llu %s%s%s 等:%s addr=%#llx%s 最近 svc %#x lr=%s+%#llx\n", t->handle, nm, t->prio, t->core, (unsigned long long)t->mask,
                cpu, t->nsvc, t->exited ? "已退出" : "", t->started ? "" : "未启动", life, kinds[t->wkind & 7], t->waddr, own, t->last_svc,
                m ? m->name : "?", m ? t->last_lr - (uint64_t)m->base : t->last_lr);
        if (have_raw) { char hx[200]; int q = 0; for (int i = 0; i < 64; i += 4) q += snprintf(hx + q, sizeof hx - q, "%08x ", *(uint32_t *)(raw + i));
                        fprintf(stderr, "[dm]   %#x 所等对象 %#llx 起 64 字节: %s\n", t->handle, t->waddr & ~0xfull, hx); }
        thr_bt(t);
    }


    thread_act_array_t ths; mach_msg_type_number_t nth = 0;
    if (task_threads(mach_task_self(), &ths, &nth) == KERN_SUCCESS) {
        for (mach_msg_type_number_t i = 0; i < nth; i++) {
            int guest = 0; for (thr_t *t = g_all; t; t = t->all) if (t->mport == ths[i]) { guest = 1; break; }
            if (!guest && ths[i] != mach_thread_self() && thread_suspend(ths[i]) == KERN_SUCCESS) {
                arm_thread_state64_t st; mach_msg_type_number_t n = ARM_THREAD_STATE64_COUNT;
                if (thread_get_state(ths[i], ARM_THREAD_STATE64, (thread_state_t)&st, &n) == KERN_SUCCESS) {
                    char line[1200]; int p = snprintf(line, sizeof line, "[dm] 宿主线程 %#x:", ths[i]);
                    uint64_t fp = __darwin_arm_thread_state64_get_fp(st), pc = __darwin_arm_thread_state64_get_pc(st);
                    for (int k = 0; k < 12 && p < (int)sizeof line - 80; k++) {
                        Dl_info di; module_t *m = module_of((void *)pc);
                        if (m) p += snprintf(line + p, sizeof line - p, " %s+%#llx", m->name, pc - (uint64_t)m->base);
                        else if (dladdr((void *)pc, &di) && di.dli_sname) p += snprintf(line + p, sizeof line - p, " %s+%#lx", di.dli_sname, (unsigned long)(pc - (uint64_t)di.dli_saddr));
                        else p += snprintf(line + p, sizeof line - p, " %#llx", pc);
                        uint64_t fr[2]; vm_size_t g2 = 0;
                        if (!fp || vm_read_overwrite(mach_task_self(), fp, 16, (vm_address_t)fr, &g2) || g2 != 16 || fr[0] <= fp) break;
                        fp = fr[0]; pc = fr[1];
                    }
                    fprintf(stderr, "%s\n", line);
                }
                thread_resume(ths[i]);
            }
            mach_port_deallocate(mach_task_self(), ths[i]);
        }
        vm_deallocate(mach_task_self(), (vm_address_t)ths, nth * sizeof *ths);
    }
}

int svc_thread(int num, uint64_t *regs) {
    { thr_t *me = pthread_getspecific(g_keythr); if (me) { me->last_svc = num; me->last_lr = regs[30]; me->nsvc++; } }
    if (g_flag.thr && num != 0x0B) { thr_t *me = pthread_getspecific(g_keythr); LOG("[%#x] svc %#x x0=%#llx x1=%#llx x2=%#llx x3=%#llx", me ? me->handle : 0, num, regs[0], regs[1], regs[2], regs[3]); }
    switch (num) {
    case 0x08: {
        thr_t *t = calloc(1, sizeof *t);
        pthread_cond_init(&t->cv, NULL);
        t->entry = regs[1]; t->arg = regs[2]; t->stack_top = regs[3] & ~0xFull;
        t->prio = (int)regs[4]; t->core = (int)regs[5]; t->mask = t->core >= 0 ? 1ull << t->core : 7;
        t->born_ns = now_ns(); t->born_tick = dm_thread_tick_fn ? dm_thread_tick_fn() : 0;
        pthread_mutex_lock(&G); t->handle = h_new(O_THREAD, t); t->id = g_next_id++; t->all = g_all; g_all = t; pthread_mutex_unlock(&G);
        pthread_attr_t a; pthread_attr_init(&a); pthread_attr_setstacksize(&a, 1 << 20);
        pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED);
        if (pthread_create(&t->pt, &a, thread_main, t)) die("pthread_create");
        pthread_attr_destroy(&a);
        if (g_flag.thr) LOG("CreateThread h=%#x entry=%#llx arg=%#llx sp=%#llx prio=%d core=%d", t->handle, t->entry, t->arg, t->stack_top, t->prio, t->core);
        regs[0] = 0; regs[1] = t->handle; return 1;
    }
    case 0x09: {
        pthread_mutex_lock(&G);
        thr_t *t = h_get((uint32_t)regs[0], O_THREAD);
        if (t) { t->started = 1; pthread_cond_signal(&t->cv); }
        pthread_mutex_unlock(&G);
        regs[0] = t ? 0 : R_BAD_HANDLE; return 1;
    }
    case 0x0A: thread_exit(cur()); return 1;
    case 0x0B: {
        int64_t ns = (int64_t)regs[0];
        uint64_t t0 = g_flag.bootprof ? now_ns() : 0;
        if (ns > 0 && dm_detclock_enabled()) {
            uint64_t deadline = guest_ns() + (uint64_t)ns;
            while (guest_ns() < deadline) { struct timespec ts = {0, 1000000}; nanosleep(&ts, 0); }
        } else if (ns > 0) { quiet_sleep_gate(); struct timespec ts = {ns / 1000000000, ns % 1000000000}; nanosleep(&ts, 0); }
        else {
            thr_t *me = pthread_getspecific(g_keythr);
            if (me && me->handle == 0x1001) { uint64_t y0 = now_ns(); sched_yield(); g_main_yield_ns += now_ns() - y0; g_main_yield_n++; }
            else sched_yield();
        }
        if (g_flag.bootprof) { uint64_t d = now_ns() - t0; dm_prof_add(PROF_SLEEP, d, 0); thr_t *me = pthread_getspecific(g_keythr); if (me && me->handle == 0x1001) dm_prof_add(PROF_MAIN_WAIT, d, 0); }
        return 1;
    }
    case 0x0C: { thr_t *t = h_get((uint32_t)regs[1], O_THREAD); regs[0] = t ? 0 : R_BAD_HANDLE; regs[1] = t ? (uint64_t)t->prio : 0; return 1; }
    case 0x0D: { thr_t *t = h_get((uint32_t)regs[0], O_THREAD); if (t) t->prio = (int)regs[1]; regs[0] = t ? 0 : R_BAD_HANDLE; return 1; }
    case 0x0E: { thr_t *t = h_get((uint32_t)regs[2], O_THREAD);
                 regs[0] = t ? 0 : R_BAD_HANDLE; regs[1] = t ? (uint64_t)(uint32_t)t->core : 0; regs[2] = t ? t->mask : 0; return 1; }
    case 0x0F: {
        thr_t *t = h_get((uint32_t)regs[0], O_THREAD);
        if (t) { int c = (int)regs[1]; if (c >= 0) t->core = c; else if (c == -1) t->core = 0; if (regs[2]) t->mask = regs[2]; }
        regs[0] = t ? 0 : R_BAD_HANDLE; return 1;
    }
    case 0x10: regs[0] = (uint64_t)(uint32_t)cur()->core; return 1;
    case 0x25: { thr_t *t = h_get((uint32_t)regs[1], O_THREAD); regs[0] = t ? 0 : R_BAD_HANDLE; regs[1] = t ? t->id : 0; return 1; }

    case 0x1A: {
        thr_t *t = cur(); uint64_t addr = regs[1];
        pthread_mutex_lock(&G);
        int64_t r = 0;
        if (ld32(addr) == ((uint32_t)regs[0] | WAITERS_BIT)) { t->waddr = addr; t->wtag = (uint32_t)regs[2]; r = block(t, 1, -1); }
        pthread_mutex_unlock(&G);
        regs[0] = (uint64_t)r; return 1;
    }
    case 0x1B: pthread_mutex_lock(&G); lock_handoff(regs[0]); pthread_mutex_unlock(&G); regs[0] = 0; return 1;
    case 0x1C: {
        thr_t *t = cur(); uint64_t addr = regs[0], key = regs[1];
        pthread_mutex_lock(&G);








#ifdef DM_TEST_HOOKS
#define CV_TEST_HOOK() { extern void (*dm_test_cv_after_release)(void); if (dm_test_cv_after_release) dm_test_cv_after_release(); }
#else
#define CV_TEST_HOOK()
#endif
#ifndef DM_TEST_CV_OLD_ORDER
        st32(key, 1);
        t->wkey = key; t->waddr = addr; t->wtag = (uint32_t)regs[2];
        lock_handoff(addr); CV_TEST_HOOK();
#else
        lock_handoff(addr); CV_TEST_HOOK();
        st32(key, 1);
        t->wkey = key; t->waddr = addr; t->wtag = (uint32_t)regs[2];
#endif
        int64_t r = block(t, 2, (int64_t)regs[3]);
        pthread_mutex_unlock(&G);
        regs[0] = (uint64_t)r; return 1;
    }
    case 0x1D: {
        uint64_t key = regs[0]; int cnt = (int)regs[1], n = 0;
        pthread_mutex_lock(&G);
        for (thr_t *w; (cnt <= 0 || n < cnt) && (w = pick(2, key)); n++) lock_acquire_for(w, w->waddr, w->wtag);
        int left = 0; for (thr_t *w = g_wait; w; w = w->next) if (w->wkind == 2 && w->wkey == key) { left = 1; break; }
        if (!left) st32(key, 0);
        pthread_mutex_unlock(&G);
        regs[0] = 0; return 1;
    }

    case 0x34: {
        thr_t *t = cur(); uint64_t addr = regs[0]; int type = (int)regs[1]; int32_t val = (int32_t)regs[2];
        pthread_mutex_lock(&G);
        int64_t r; uint32_t v = ld32(addr);
        int ok = type == 2 ? (int32_t)v == val : (int32_t)v < val;
        if (ok && type == 1) { while (!cas32(addr, &v, v - 1)) if (!((int32_t)v < val)) { ok = 0; break; } }
        if (!ok) r = R_BAD_STATE;
        else { t->waddr = addr; r = block(t, 3, (int64_t)regs[3]); }
        pthread_mutex_unlock(&G);
        regs[0] = (uint64_t)r; return 1;
    }
    case 0x35: {
        uint64_t addr = regs[0]; int type = (int)regs[1]; int32_t val = (int32_t)regs[2]; int cnt = (int)regs[3];
        pthread_mutex_lock(&G);
        int nw = 0; for (thr_t *w = g_wait; w; w = w->next) if (w->wkind == 3 && w->waddr == addr) nw++;
        int64_t r = 0;
        if (type != 0) {
            int32_t nv = val + 1;
            if (type == 2) {
                if (cnt <= 0) nv = nw > 0 ? val - 1 : val + 1;
                else nv = nw == 0 ? val + 1 : nw <= cnt ? val - 1 : val;
            }
            uint32_t e = (uint32_t)val;
            if (!cas32(addr, &e, (uint32_t)nv)) r = R_BAD_STATE;
        }
        if (!r) { int n = 0;
            for (thr_t *w; (cnt <= 0 || n < cnt) && (w = pick(3, addr)); n++) wake(w, 0); }
        pthread_mutex_unlock(&G);
        regs[0] = (uint64_t)r; return 1;
    }

    case 0x45: {
        event_t *e = calloc(1, sizeof *e); e->refs = 2;
        pthread_mutex_lock(&G); uint32_t hw = h_new(O_EVENT_W, e), hr = h_new(O_EVENT_R, e); pthread_mutex_unlock(&G);
        regs[0] = 0; regs[1] = hw; regs[2] = hr; return 1;
    }
    case 0x11: case 0x12: case 0x17: {
        pthread_mutex_lock(&G);


        uint32_t h = (uint32_t)regs[0];
        event_t *e = num == 0x11 ? h_peek(h, O_EVENT_W) : num == 0x17 ? h_peek(h, O_EVENT_R)
                   : (h_peek(h, O_EVENT_W) ? h_peek(h, O_EVENT_W) : h_peek(h, O_EVENT_R));
        int64_t r = 0;
        if (!e) { r = (num == 0x17 && h_peek(h, O_THREAD)) ? R_BAD_STATE : R_BAD_HANDLE;
                  if (r == R_BAD_HANDLE) LOG("事件 SVC %#x 句柄无效 %#x", num, h); }
        else if (num == 0x11) { e->signaled = 1; wake_sync_waiters(); }
        else if (num == 0x17 && !e->signaled) r = R_BAD_STATE;
        else e->signaled = 0;
        pthread_mutex_unlock(&G);
        regs[0] = (uint64_t)r; return 1;
    }
    case 0x18: {
        thr_t *t = cur(); const uint32_t *hs = (const uint32_t *)regs[1]; int n = (int)regs[2]; int64_t ns = (int64_t)regs[3];
        int64_t deadline = ns > 0 ? (dm_detclock_enabled() ? (int64_t)guest_ns() : now_mono_ns()) + ns : 0;
        pthread_mutex_lock(&G);
        int64_t r; int idx = -1;
        for (;;) {
            if (t->cancel) { t->cancel = 0; r = R_CANCELLED; break; }
            int bad = 0;
            for (int i = 0; i < n && idx < 0; i++) if (sig_obj(hs[i], &bad)) idx = i;
            if (bad) { r = R_BAD_HANDLE; break; }
            if (idx >= 0) { r = 0; break; }
            int64_t left = ns < 0 ? -1 : ns == 0 ? 0 : deadline - (dm_detclock_enabled() ? (int64_t)guest_ns() : now_mono_ns());
            if (ns > 0 && left <= 0) { r = R_TIMEOUT; break; }
            if (block(t, 4, left) == R_TIMEOUT) { r = R_TIMEOUT; break; }
        }
        pthread_mutex_unlock(&G);
        regs[0] = (uint64_t)r; regs[1] = idx >= 0 ? (uint64_t)idx : 0; return 1;
    }
    case 0x19: {
        pthread_mutex_lock(&G);
        thr_t *t = h_get((uint32_t)regs[0], O_THREAD);
        if (t) { t->cancel = 1; if (t->wkind == 4) wake(t, -1); }
        pthread_mutex_unlock(&G);
        regs[0] = t ? 0 : R_BAD_HANDLE; return 1;
    }
    case 0x16: {
        pthread_mutex_lock(&G);
        unsigned i = (uint32_t)regs[0] - 0x1000;
        if (i > 0 && i < MAX_H && (g_h[i].type == O_EVENT_R || g_h[i].type == O_EVENT_W)) {
            event_t *e = g_h[i].obj; if (--e->refs == 0) free(e);
            g_h[i].type = O_FREE; g_h[i].obj = NULL;
        } else if (i > 0 && i < MAX_H && g_h[i].type == O_THREAD) {
            thr_t *t = g_h[i].obj;
            g_h[i].type = O_FREE; g_h[i].obj = NULL;
            t->closed = 1;
            if (t->exited) thr_free(t);
        }
        pthread_mutex_unlock(&G);
        regs[0] = 0; return 1;
    }
    }
    return 0;
}







static struct { void *ev; uint32_t wh; } g_se[512];
static int g_nse;
static pthread_mutex_t g_se_lock = PTHREAD_MUTEX_INITIALIZER;
static uint32_t se_find(void *ev) {
    for (int i = 0; i < g_nse; i++) if (g_se[i].ev == ev) return g_se[i].wh;
    return 0;
}
void hle_bind_system_event(void *ev, int autoclear) {
    pthread_mutex_lock(&g_se_lock); uint32_t wh = se_find(ev); pthread_mutex_unlock(&g_se_lock);
    if (wh) return;
    if (!pthread_getspecific(g_keythr)) die("hle_bind_system_event 须在客体线程上调用");
    static uint32_t (*create)(void *, int, int); static uint32_t (*getw)(const void *);
    if (!create) { create = guest_sym("_ZN2nn2os17CreateSystemEventEPNS0_15SystemEventTypeENS0_14EventClearModeEb");
                   getw = guest_sym("_ZN2nn2os30GetWritableHandleOfSystemEventEPKNS0_15SystemEventTypeE"); }
    uint32_t r = create(ev, autoclear, 1);
    if (r) die("CreateSystemEvent 失败 %#x", r);
    wh = getw(ev);
    pthread_mutex_lock(&g_se_lock);
    if (g_nse >= (int)(sizeof g_se / sizeof g_se[0])) die("SystemEvent 绑定表满");
    g_se[g_nse].ev = ev; g_se[g_nse++].wh = wh;
    pthread_mutex_unlock(&g_se_lock);
}
void hle_signal_system_event(void *ev) {
    pthread_mutex_lock(&g_se_lock); uint32_t wh = se_find(ev); pthread_mutex_unlock(&g_se_lock);
    if (!wh) { hle_bind_system_event(ev, 0); pthread_mutex_lock(&g_se_lock); wh = se_find(ev); pthread_mutex_unlock(&g_se_lock); }
    pthread_mutex_lock(&G);
    event_t *e = h_get(wh, O_EVENT_W);
    if (!e) die("SystemEvent 写端句柄失效 %#x", wh);
    e->signaled = 1; wake_sync_waiters();
    pthread_mutex_unlock(&G);
}
