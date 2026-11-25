








#include <stdint.h>
#include <stddef.h>

int64_t dm_user_clock(void);
uint64_t dm_seq_next(void);
uint64_t dm_fs_d2s_wseq(void);

static int g_ser_infl; static uint64_t g_ser_seq; static int64_t g_ser_sec = -1; static uint32_t g_ser_tq, g_tq_now;

void dm_ser_observe(uint64_t G) {
    static uint64_t pv, last_seq;
    uint32_t tq = *(const uint32_t *)(G + 0x170);
    uint64_t v = *(const uint64_t *)(G + 0x5800), w = dm_fs_d2s_wseq();
    if (v != pv) { if (v > 1 && pv) { g_ser_sec = dm_user_clock(); g_ser_infl = !(w > last_seq); g_ser_seq = dm_seq_next(); g_ser_tq = tq; } pv = v; }
    if (g_ser_infl && w > g_ser_seq) g_ser_infl = 0;
    last_seq = dm_seq_next(); g_tq_now = tq;
}

int dm_ser_state(void) { return !g_ser_infl ? 0 : g_tq_now - g_ser_tq < 50 ? 1 : 2; }

int64_t dm_ser_sec(void) { return g_ser_sec; }

uint64_t dm_fnv64(const uint8_t *d, size_t n) { uint64_t h = 1469598103934665603ull; for (size_t i = 0; i < n; i++) { h ^= d[i]; h *= 1099511628211ull; } return h ? h : 1; }
