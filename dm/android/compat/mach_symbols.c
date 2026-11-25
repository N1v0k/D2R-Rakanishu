



#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

uint64_t mach_absolute_time(void) {
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}


int thread_selfcounts(int type, void *buf, size_t nbytes) { (void)type; memset(buf, 0, nbytes); return -1; }
