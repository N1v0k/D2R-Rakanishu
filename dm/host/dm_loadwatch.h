














#ifndef DM_LOADWATCH_H
#define DM_LOADWATCH_H
#include <stdint.h>

static inline int dm_pause_holds(int wants, int saving, int loading) { return wants != 0 && !saving && !loading; }


#define DM_PAUSE_DEADLINE_HOLD 0xF0000000u
typedef struct { int paused; uint32_t dl0, t0; } dm_pause_state;
enum { DM_PAUSE_KEEP = 0, DM_PAUSE_APPLY = 1, DM_PAUSE_RELEASE = 2 };
static inline int dm_pause_next(const dm_pause_state *s, int on) { return on && !s->paused ? DM_PAUSE_APPLY : !on && s->paused ? DM_PAUSE_RELEASE : DM_PAUSE_KEEP; }

static inline uint32_t dm_pause_applied(dm_pause_state *s, uint32_t deadline, uint32_t now_ms) { s->dl0 = deadline; s->t0 = now_ms; s->paused = 1; return DM_PAUSE_DEADLINE_HOLD; }

static inline uint32_t dm_pause_released(dm_pause_state *s, uint32_t now_ms) { s->paused = 0; return s->dl0 + (now_ms - s->t0); }


typedef struct { uint64_t since_ns, next_ns, first_ns, again_ns; uint32_t tick0; int reports, max; } dm_loadwatch;
enum { DM_LOADWATCH_NONE = 0, DM_LOADWATCH_REPORT = 1, DM_LOADWATCH_GONE = 2 };



static inline int dm_loadwatch_step(dm_loadwatch *w, int up, uint64_t now, uint32_t tick) {
    if (!up) {
        int gone = w->since_ns && w->reports ? DM_LOADWATCH_GONE : DM_LOADWATCH_NONE;
        w->since_ns = 0; w->reports = 0;
        return gone;
    }
    if (!w->since_ns) { w->since_ns = now ? now : 1; w->tick0 = tick; w->reports = 0; w->next_ns = now + w->first_ns; return DM_LOADWATCH_NONE; }
    if (w->reports >= w->max || now < w->next_ns) return DM_LOADWATCH_NONE;
    w->reports++; w->next_ns = now + w->again_ns;
    return DM_LOADWATCH_REPORT;
}
#endif
