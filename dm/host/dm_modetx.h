









#ifndef DM_MODETX_H
#define DM_MODETX_H
#include <stdint.h>
enum { DM_MODETX_SETTLE = 4, DM_MODETX_TIMEOUT = 240 };
enum { DM_TX_IDLE, DM_TX_REBUILD1, DM_TX_REBUILD2 };
enum { DM_TXA_NONE, DM_TXA_BEGIN, DM_TXA_REPORT_BACK, DM_TXA_DONE };
typedef struct { int st, timeouts; uint64_t at, moved_at; uint32_t tips; } dm_modetx;
static inline int dm_modetx_step(dm_modetx *t, uint64_t frame, int want, int cur, uint32_t tips) {
    if (t->st == DM_TX_IDLE) {
        if (want < 0 || want == cur) return DM_TXA_NONE;
        t->st = DM_TX_REBUILD1; t->at = frame; t->moved_at = 0; t->tips = tips; t->timeouts = 0; return DM_TXA_BEGIN; }
    if (tips != t->tips) { t->tips = tips; t->moved_at = frame; }
    int done = t->moved_at && frame - t->moved_at >= DM_MODETX_SETTLE, late = frame - t->at >= DM_MODETX_TIMEOUT;
    if (!done && !late) return DM_TXA_NONE;
    if (!done) t->timeouts++;
    t->at = frame; t->moved_at = 0;
    if (t->st == DM_TX_REBUILD1) { t->st = DM_TX_REBUILD2; return DM_TXA_REPORT_BACK; }
    t->st = DM_TX_IDLE; return DM_TXA_DONE;
}




typedef struct { uint64_t hold; } dm_modegate;
static inline int dm_modegate_apply(dm_modegate *g, int frozen, uint64_t *buttons) {
    if (frozen) { g->hold |= *buttons; *buttons = 0; return 1; }
    g->hold &= *buttons; *buttons &= ~g->hold; return 0;
}
#endif
