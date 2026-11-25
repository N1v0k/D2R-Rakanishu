






#pragma once
#include <stdarg.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../host/dm_hud.h"

enum { QA_PANELS = 64, QA_PANEL_NAME = 64, QA_TIP = 1024, QA_TIPS = 2, QA_OVERLAYS = 48, QA_NAME = 32, QA_OV_STATE = 384 };
enum { QA_SCREEN_BOOT, QA_SCREEN_FRONTEND, QA_SCREEN_LOADING, QA_SCREEN_INGAME };






typedef struct { _Atomic unsigned mid; unsigned back, front; } qa_tri;
enum { QA_TRI_FRESH = 4u };
static inline void qa_tri_init(qa_tri *t) { t->back = 0; t->front = 2; atomic_store(&t->mid, 1u); }
static inline void qa_tri_publish(qa_tri *t) { t->back = atomic_exchange_explicit(&t->mid, t->back | QA_TRI_FRESH, memory_order_acq_rel) & 3u; }
static inline int qa_tri_take(qa_tri *t) {
    if (!(atomic_load_explicit(&t->mid, memory_order_acquire) & QA_TRI_FRESH)) return 0;
    t->front = atomic_exchange_explicit(&t->mid, t->front, memory_order_acq_rel) & 3u; return 1;
}

typedef struct { int n, overflow; char name[QA_PANELS][QA_PANEL_NAME]; } qa_panels;
typedef struct { int n, truncated, overflow, has_id; uint32_t id; uint64_t frame; char text[QA_TIPS][QA_TIP + 1]; } qa_tips;


typedef struct {
    int screen, paused;
    unsigned kills;
    dm_hud_info hud;
    qa_panels panels;
    qa_tips tips;
    int overlays; char ov_name[QA_OVERLAYS][QA_NAME], ov_state[QA_OVERLAYS][QA_OV_STATE];
} qa_snapshot;


static inline int qa_tips_same(const qa_tips *a, const qa_tips *b) {
    if (a->n != b->n || a->truncated != b->truncated || a->overflow != b->overflow || a->id != b->id || a->has_id != b->has_id) return 0;
    for (int i = 0; i < a->n && i < QA_TIPS; i++) if (strcmp(a->text[i], b->text[i])) return 0;
    return 1;
}
static inline int qa_panels_same(const qa_panels *a, const qa_panels *b) {
    if (a->n != b->n || a->overflow != b->overflow) return 0;
    for (int i = 0; i < a->n && i < QA_PANELS; i++) if (strcmp(a->name[i], b->name[i])) return 0;
    return 1;
}

typedef struct { char *p; size_t cap, n; int full; } qa_buf;
static inline void qa_put(qa_buf *b, const char *s, size_t n) {
    if (b->full || n >= b->cap - b->n) { b->full = 1; return; }
    memcpy(b->p + b->n, s, n); b->n += n; b->p[b->n] = 0;
}
__attribute__((format(printf, 2, 3))) static inline void qa_printf(qa_buf *b, const char *fmt, ...) {
    if (b->full) return;
    va_list a; va_start(a, fmt); int w = vsnprintf(b->p + b->n, b->cap - b->n, fmt, a); va_end(a);
    if (w < 0 || (size_t)w >= b->cap - b->n) { b->full = 1; b->p[b->n] = 0; return; }
    b->n += (size_t)w;
}



static inline size_t qa_utf8_len(const unsigned char *s, size_t n) {
    if (!n) return 0;
    unsigned c = s[0];
    if (c < 0x80) return 1;
    size_t k = c >= 0xf0 ? 4 : c >= 0xe0 ? 3 : c >= 0xc2 ? 2 : 0; if (!k || c > 0xf4 || k > n) return 0;
    for (size_t i = 1; i < k; i++) if ((s[i] & 0xc0) != 0x80) return 0;
    if (k == 3 && ((c == 0xe0 && s[1] < 0xa0) || (c == 0xed && s[1] >= 0xa0))) return 0;
    if (k == 4 && ((c == 0xf0 && s[1] < 0x90) || (c == 0xf4 && s[1] >= 0x90))) return 0;
    return k;
}

static inline size_t qa_utf8_cut(const char *s, size_t n, size_t max) {
    if (n <= max) return n;
    size_t i = max; while (i > 0 && ((unsigned char)s[i] & 0xc0) == 0x80) i--;
    return i;
}


static inline void qa_json_str(qa_buf *b, const char *s, size_t n) {
    qa_put(b, "\"", 1);
    for (size_t i = 0; i < n && !b->full;) {
        unsigned char c = (unsigned char)s[i];
        if (c == '"' || c == '\\') { char e[2] = {'\\', (char)c}; qa_put(b, e, 2); i++; continue; }
        if (c == '\n') { qa_put(b, "\\n", 2); i++; continue; }
        if (c < 0x20 || c == 0x7f) { char e[8]; snprintf(e, sizeof e, "\\u%04x", c); qa_put(b, e, 6); i++; continue; }
        size_t k = qa_utf8_len((const unsigned char *)s + i, n - i);
        if (!k) { qa_put(b, "\\ufffd", 6); i++; continue; }
        qa_put(b, s + i, k); i += k;
    }
    qa_put(b, "\"", 1);
}
static inline int qa_act(int area) { return area <= 0 ? 0 : area < 40 ? 1 : area < 75 ? 2 : area < 103 ? 3 : area < 109 ? 4 : 5; }
static inline const char *qa_screen_name(int s) { return s == QA_SCREEN_INGAME ? "ingame" : s == QA_SCREEN_LOADING ? "loading" : s == QA_SCREEN_FRONTEND ? "frontend" : "boot"; }


static inline int qa_render_body(qa_buf *b, const qa_snapshot *s) {
    const dm_hud_info *h = &s->hud;
    qa_printf(b, "\"screen\":\"%s\",\"paused\":%d,", qa_screen_name(s->screen), !!s->paused);
    qa_printf(b, "\"kills\":%u,", s->kills);
    qa_printf(b, "\"hud\":{\"valid\":%d,\"area\":%d,\"act\":%d,\"x\":%d,\"y\":%d,\"level\":%d,\"exp_permille\":%d,\"hp\":[%d,%d],\"mp\":[%d,%d],"
                 "\"belt\":[%d,%d,%d,%d],\"portal\":%d,\"players\":%d,\"points\":%d,\"quest_unread\":%d,\"dead\":%d},",
              !!h->valid, h->area, qa_act(h->area), h->x, h->y, h->level, h->exp_permille, h->hp, h->hp_max, h->mp, h->mp_max,
              h->belt[0], h->belt[1], h->belt[2], h->belt[3], h->portal, h->players, h->points, h->quest_unread, h->dead);
    qa_put(b, "\"panels\":[", 10);
    for (int i = 0; i < s->panels.n && i < QA_PANELS; i++) { if (i) qa_put(b, ",", 1); qa_json_str(b, s->panels.name[i], strnlen(s->panels.name[i], QA_PANEL_NAME - 1)); }
    qa_printf(b, "],\"panels_overflow\":%d,\"tooltip\":", !!s->panels.overflow);
    if (!s->tips.n) qa_put(b, "null", 4);
    else {
        qa_put(b, "{\"texts\":[", 10);
        for (int i = 0; i < s->tips.n && i < QA_TIPS; i++) { if (i) qa_put(b, ",", 1); qa_json_str(b, s->tips.text[i], strnlen(s->tips.text[i], QA_TIP)); }
        qa_printf(b, "],\"truncated\":%d,\"overflow\":%d,\"hovered\":%u}", !!s->tips.truncated, !!s->tips.overflow, s->tips.has_id ? s->tips.id : 0u);
    }
    qa_put(b, ",\"overlays\":{", 13);
    for (int i = 0; i < s->overlays && i < QA_OVERLAYS; i++) {
        if (i) qa_put(b, ",", 1);
        qa_json_str(b, s->ov_name[i], strnlen(s->ov_name[i], QA_NAME - 1)); qa_put(b, ":", 1); qa_json_str(b, s->ov_state[i], strnlen(s->ov_state[i], QA_OV_STATE - 1));
    }
    qa_put(b, "}", 1);
    return b->full ? -1 : 0;
}



typedef struct { int pid; long long start_ms, mono_ms, wall_ms, hud_age_ms, tip_age_ms, tip_frames, overlay_age_ms; unsigned seq; uint64_t frame; } qa_stamps;


static inline int qa_render_file(qa_buf *b, const qa_stamps *t, const char *body) {
    qa_printf(b, "{\"v\":1,\"session\":{\"pid\":%d,\"start_ms\":%lld},\"seq\":%u,\"mono_ms\":%lld,\"wall_ms\":%lld,\"frame\":%llu,",
              t->pid, t->start_ms, t->seq, t->mono_ms, t->wall_ms, (unsigned long long)t->frame);
    qa_put(b, body, strlen(body));
    qa_printf(b, ",\"age_ms\":{\"hud\":%lld,\"tooltip\":%lld,\"tooltip_frames\":%lld,\"overlays\":%lld}}\n", t->hud_age_ms, t->tip_age_ms, t->tip_frames, t->overlay_age_ms);
    return b->full ? -1 : 0;
}




static inline int qa_write_due(int changed, uint64_t now, uint64_t last_write, uint64_t min_gap, uint64_t heartbeat) {
    if (!last_write) return 1;
    if (now - last_write < min_gap) return 0;
    return changed || now - last_write >= heartbeat;
}


static inline int qa_token_ok(const char *s, size_t cap) {
    size_t n = 0; if (!s) return 0;
    for (; s[n]; n++) if (n >= cap - 1 || !((s[n] >= 'a' && s[n] <= 'z') || (s[n] >= '0' && s[n] <= '9') || s[n] == '_')) return 0;
    return n > 0;
}






static inline void qa_tips_add(qa_tips *cur, const char *txt, uint64_t frame, uint32_t id, int has_id) {
    if (cur->frame != frame) { cur->frame = frame; cur->n = cur->truncated = cur->overflow = 0; cur->id = id; cur->has_id = has_id; }
    size_t len = strnlen(txt, QA_TIP + 1); if (len > QA_TIP) { len = qa_utf8_cut(txt, len, QA_TIP); cur->truncated = 1; }
    for (int i = 0; i < cur->n; i++) if (!strncmp(cur->text[i], txt, len) && !cur->text[i][len]) return;
    if (cur->n == QA_TIPS) { cur->overflow = 1; return; }
    memcpy(cur->text[cur->n], txt, len); cur->text[cur->n][len] = 0; cur->n++;
}
