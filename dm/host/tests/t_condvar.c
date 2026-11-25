











#include "../dm.h"
#include <pthread.h>
#include <stdatomic.h>
#include <unistd.h>
int svc_thread(int num, uint64_t *regs);
uint32_t svc_thread_main_init(void);
void (*dm_test_cv_after_release)(void);

static _Atomic uint32_t g_mutex, g_key;
static uint32_t g_wtag, g_rtag = 0x2002;
static _Atomic int g_rstate;
static pthread_mutex_t g_m = PTHREAD_MUTEX_INITIALIZER; static pthread_cond_t g_c = PTHREAD_COND_INITIALIZER;
static int g_go, g_wdone; static int64_t g_wret = -1; static uint32_t g_wlock_after;
static void state_set(int state) {
    atomic_store(&g_rstate, state);
    pthread_mutex_lock(&g_m); pthread_cond_broadcast(&g_c); pthread_mutex_unlock(&g_m);
}

static void *reader(void *a) {
    (void)a;
    pthread_mutex_lock(&g_m); while (!g_go) pthread_cond_wait(&g_c, &g_m); pthread_mutex_unlock(&g_m);
    uint32_t z = 0; while (!atomic_compare_exchange_strong(&g_mutex, &z, g_rtag)) { z = 0; sched_yield(); }
    state_set(1);


    if (atomic_load(&g_key)) { state_set(2); uint64_t r[8] = {(uint64_t)&g_key, 1}; svc_thread(0x1D, r); }
    uint32_t v = g_rtag;
    if (!atomic_compare_exchange_strong(&g_mutex, &v, 0)) { uint64_t r[8] = {(uint64_t)&g_mutex}; svc_thread(0x1B, r); }
    state_set(3);
    return NULL;
}
static void hook(void) {
    dm_test_cv_after_release = NULL;
    pthread_mutex_lock(&g_m); g_go = 1; pthread_cond_broadcast(&g_c); pthread_mutex_unlock(&g_m);
    pthread_mutex_lock(&g_m); while (atomic_load(&g_rstate) != 2 && atomic_load(&g_rstate) != 3) pthread_cond_wait(&g_c, &g_m); pthread_mutex_unlock(&g_m);
}
static void *waiter(void *a) {
    (void)a;
    g_wtag = svc_thread_main_init();
    uint32_t z = 0; atomic_compare_exchange_strong(&g_mutex, &z, g_wtag);
    dm_test_cv_after_release = hook;
    uint64_t r[8] = {(uint64_t)&g_mutex, (uint64_t)&g_key, g_wtag, (uint64_t)-1};
    svc_thread(0x1C, r);
    pthread_mutex_lock(&g_m); g_wret = (int64_t)r[0]; g_wlock_after = atomic_load(&g_mutex); g_wdone = 1; pthread_cond_broadcast(&g_c); pthread_mutex_unlock(&g_m);
    return NULL;
}
int main(void) {
    pthread_t w, rd; pthread_create(&rd, NULL, reader, NULL); pthread_create(&w, NULL, waiter, NULL);
    struct timespec dl; clock_gettime(CLOCK_REALTIME, &dl); dl.tv_sec += 10;
    pthread_mutex_lock(&g_m); while (!g_wdone) if (pthread_cond_timedwait(&g_c, &g_m, &dl)) break; int done = g_wdone; pthread_mutex_unlock(&g_m);
#ifdef DM_TEST_CV_OLD_ORDER


    int ok = !done && atomic_load(&g_rstate) == 3;
    printf("%s 旧顺序负对照：W %s，R 状态 %d（期望 W 不醒、R 未发 0x1D 即完成）\n", ok ? "✓" : "✗", done ? "被唤醒" : "10 秒未醒", atomic_load(&g_rstate));
    printf(ok ? "全过：1/1\n" : "失败：负对照没判出丢唤醒（判据无效）\n");
#else
    int ok = done && g_wret == 0 && g_wlock_after == g_wtag && atomic_load(&g_key) == 0;
    printf("%s 现行顺序：W %s 返回 %lld，锁字 %#x（期望 W 标签 %#x），条件键 %u（期望 0），R 状态 %d\n", ok ? "✓" : "✗",
           done ? "被唤醒" : "2 秒未醒", (long long)g_wret, g_wlock_after, g_wtag, atomic_load(&g_key), atomic_load(&g_rstate));
    printf(ok ? "全过：1/1\n" : "失败：1/1\n");
#endif
    fflush(stdout); _exit(ok ? 0 : 1);
}
