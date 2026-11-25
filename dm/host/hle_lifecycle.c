





#include "hle.h"
#ifdef __APPLE__
#include <dispatch/dispatch.h>
#else
#include <semaphore.h>
#endif
#include <stdatomic.h>
#include <signal.h>
#include <unistd.h>
void tact_prefetch_stop(void);
void granny_prof_dump(void);
void hlecount_flush(void);
#ifndef __APPLE__


static void *shutdown_fallback(void *arg) { (void)arg; usleep(1500 * 1000); LOG("【红】优雅退出 1.5 秒未完成，直接退出"); _exit(0); }
static sem_t g_term_sem;
static void on_term(int sig) { (void)sig; sem_post(&g_term_sem); }
static void *term_main(void *arg) { (void)arg; while (sem_wait(&g_term_sem)) {} dm_host_shutdown("SIGTERM"); return NULL; }
#endif
void dm_host_shutdown(const char *why) {
    static _Atomic int once; if (atomic_exchange(&once, 1)) return;
    uint64_t t0 = now_ns(); LOG("收到 %s：优雅退出", why);
#ifdef __APPLE__
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 1500 * NSEC_PER_MSEC), dispatch_get_global_queue(QOS_CLASS_USER_INTERACTIVE, 0), ^{
        LOG("【红】优雅退出 1.5 秒未完成，直接退出"); _exit(0); });
#else
    { pthread_t th; if (!pthread_create(&th, NULL, shutdown_fallback, NULL)) pthread_detach(th); }
#endif
    tact_prefetch_stop();
    granny_prof_dump();
    hlecount_flush();
    dm_audio_suspend(1);
    dm_gfx_suspend(1);
    LOG("优雅退出完成（%.3f 秒）", (now_ns() - t0) / 1e9);
    _exit(0);
}
void dm_term_install(void) {
#ifndef __APPLE__
    sem_init(&g_term_sem, 0, 0);
    struct sigaction sa = {.sa_handler = on_term, .sa_flags = SA_ONSTACK | SA_RESTART}; sigaction(SIGTERM, &sa, NULL);
    pthread_t th; if (!pthread_create(&th, NULL, term_main, NULL)) pthread_detach(th);
#else
    signal(SIGTERM, SIG_IGN);
    static dispatch_source_t src;
    src = dispatch_source_create(DISPATCH_SOURCE_TYPE_SIGNAL, SIGTERM, 0, dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0));
    dispatch_source_set_event_handler(src, ^{ dm_host_shutdown("SIGTERM"); });
    dispatch_resume(src);
#endif
}
