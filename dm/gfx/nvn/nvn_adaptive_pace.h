

#pragma once
#include <stdint.h>

static inline int nvn_adaptive_min_divisor(double hz) {
    double ratio = hz / 60.0;
    int k = (int)ratio;
    if (k < ratio) ++k;
    return k > 0 ? k : 1;
}
static inline int nvn_adaptive_max_divisor(double hz) {
    int k = (int)(hz / 20.0 + 0.02), lo = nvn_adaptive_min_divisor(hz);
    return k < lo ? lo : k;
}
static inline int nvn_adaptive_divisor(double hz, int current, double old_hz) {
    int lo = nvn_adaptive_min_divisor(hz), hi = nvn_adaptive_max_divisor(hz);
    int k = current > 0 ? current : lo;
    if (current > 0 && old_hz > 0 && hz != old_hz) k = (int)(current * hz / old_hz + 0.5);
    return k < lo ? lo : k > hi ? hi : k;
}



typedef struct { uint64_t previous, sum; unsigned count; } nvn_frame_cpu_window;
static inline int nvn_frame_cpu_sample(nvn_frame_cpu_window *s, uint64_t cpu_ns, double *ms) {
    if (!s->previous || cpu_ns < s->previous) { s->previous = cpu_ns; s->sum = 0; s->count = 0; return 0; }
    s->sum += cpu_ns - s->previous; s->previous = cpu_ns;
    if (++s->count < 60) return 0;
    *ms = s->sum / 60000000.0; s->sum = 0; s->count = 0; return 1;
}
