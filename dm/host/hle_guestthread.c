









#include "hle.h"
#include <stdatomic.h>
#include <unistd.h>
typedef int (*create_fn)(void *tt, void (*fn)(void *), void *arg, void *stack, size_t size, int prio);
typedef void (*tt_fn)(void *tt);
static void *need(const char *n) { void *p = guest_sym(n); if (!p) die("宿主客体线程：缺符号 %s", n); return p; }
void *dm_guest_thread_spawn(void (*fn)(void *), void *arg, int prio, size_t stack_size) {
    stack_size = (stack_size + 0xfff) & ~(size_t)0xfff;
    void *tt = calloc(1, 0x200);
    void *stack = aligned_alloc(0x1000, stack_size);
    if (!tt || !stack) die("宿主客体线程：分配失败");
    int rc = ((create_fn)need("_ZN2nn2os12CreateThreadEPNS0_10ThreadTypeEPFvPvES3_S3_mi"))(tt, fn, arg, stack, stack_size, prio);
    if (rc) die("宿主客体线程：CreateThread 失败 %#x", rc);
    ((tt_fn)need("_ZN2nn2os11StartThreadEPNS0_10ThreadTypeE"))(tt);
    return tt;
}
void dm_guest_thread_join(void *tt) { ((tt_fn)need("_ZN2nn2os10WaitThreadEPNS0_10ThreadTypeE"))(tt); ((tt_fn)need("_ZN2nn2os13DestroyThreadEPNS0_10ThreadTypeE"))(tt); }

static struct { int malloc_ok, lock_owned, handle; } g_st;
static void selftest_body(void *arg) {
    (void)arg;
    void *(*gmalloc)(size_t) = need("malloc"); void (*gfree)(void *) = need("free");
    void *p = gmalloc(4096); if (p) { memset(p, 0x5a, 4096); gfree(p); g_st.malloc_ok = 1; }
    uint8_t mtx[0x40] __attribute__((aligned(16))) = {0};
    ((void (*)(void *, int, int))need("_ZN2nn2os15InitializeMutexEPNS0_9MutexTypeEbi"))(mtx, 0, 0);
    ((void (*)(void *))need("_ZN2nn2os9LockMutexEPNS0_9MutexTypeE"))(mtx);
    g_st.lock_owned = ((int (*)(const void *))need("_ZN2nn2os28IsMutexLockedByCurrentThreadEPKNS0_9MutexTypeE"))(mtx) & 1;
    ((void (*)(void *))need("_ZN2nn2os11UnlockMutexEPNS0_9MutexTypeE"))(mtx);
    uint32_t thr_cur_handle(void); g_st.handle = (int)thr_cur_handle();
}
static void park_body(void *arg) { (void)arg; for (;;) sleep(3600); }
void dm_guest_spawn_selftest(void) {
    const char *e = getenv("DM_GUEST_SPAWN_TEST"); if (!e) return;
    if (atoi(e) == 2) { dm_guest_thread_spawn(park_body, NULL, 16, 16 << 10); LOG("宿主客体线程自检：另起常驻线程（之后游戏线程句柄顺延）"); }
    void *tt = dm_guest_thread_spawn(selftest_body, NULL, 16, 64 << 10);
    dm_guest_thread_join(tt);
    uint32_t thr_cur_handle(void); int mine = (int)thr_cur_handle();
    int ok = g_st.malloc_ok && g_st.lock_owned && g_st.handle && g_st.handle != mine;
    LOG("%s宿主客体线程自检：SDK malloc/free %s；互斥锁持有者是本线程 %s；线程句柄 %#x（主线程 %#x）", ok ? "" : "【红】",
        g_st.malloc_ok ? "通" : "不通", g_st.lock_owned ? "是" : "否", g_st.handle, mine);
}
