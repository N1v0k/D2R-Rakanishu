



#pragma once
#if defined(__ANDROID__) || defined(__OHOS__)
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>



#ifndef CLOCK_UPTIME_RAW
#define CLOCK_UPTIME_RAW CLOCK_MONOTONIC
#endif
static inline uint64_t clock_gettime_nsec_np(clockid_t c) {
    struct timespec ts; if (clock_gettime(c, &ts)) return 0;
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}
#ifndef __OHOS__
static inline int getdtablesize(void) { return (int)sysconf(_SC_OPEN_MAX); }
#endif
static inline size_t pthread_get_stacksize_np(pthread_t t) {
    pthread_attr_t a; size_t s = 0;
    if (!pthread_getattr_np(t, &a)) { pthread_attr_getstacksize(&a, &s); pthread_attr_destroy(&a); }
    return s;
}


static inline void *pthread_get_stackaddr_np(pthread_t t) {
    pthread_attr_t a; void *lo = NULL; size_t s = 0;
    if (!pthread_getattr_np(t, &a)) { pthread_attr_getstack(&a, &lo, &s); pthread_attr_destroy(&a); }
    return (char *)lo + s;
}


#define st_mtimespec st_mtim
#define st_atimespec st_atim
#define st_ctimespec st_ctim



int clonefile(const char *src, const char *dst, int flags);
#ifndef RENAME_SWAP
#define RENAME_SWAP (1u << 1)
#endif
static inline int renamex_np(const char *from, const char *to, unsigned flags) {
    return (int)syscall(__NR_renameat2, AT_FDCWD, from, AT_FDCWD, to, flags & RENAME_SWAP ? (1u << 1) : 0u);
}
#endif


#if defined(__ANDROID__) && defined(DM_PLAYER_BUILD)
#include "dm_player_env.h"
#endif
