














#pragma once
#include <math.h>
#include <stdint.h>

enum { DM_PT_UNKNOWN = -2, DM_PT_EMPTY = -1 };
#define DM_PT_SCALE 100000
#define DM_PT_MAX (3 * DM_PT_SCALE)
#define DM_PT_STALE_MS 500
#define DM_PT_NAME_LINE 1.7f
#define DM_PT_NAME_PT 40

static inline int dm_pt_finite_in(float v, float lo, float hi) { return isfinite(v) && v >= lo && v <= hi; }


typedef struct { int32_t y, h; float scale; int name; int32_t name_y; float name_anchor, name_scale; int32_t name_pt; } dm_pt_icon;




static inline float dm_pt_icon_bottom(int32_t panel_y, float panel_scale, const dm_pt_icon *ic) {
    if (!dm_pt_finite_in(panel_scale, 0.1f, 10) || !dm_pt_finite_in(ic->scale, 0.1f, 10) || ic->h <= 0 || ic->h > 1000) return NAN;
    float top = panel_y + panel_scale * ic->y, k = panel_scale * ic->scale, bottom = top + k * ic->h;
    if (ic->name) {
        if (!dm_pt_finite_in(ic->name_anchor, 0, 2) || !dm_pt_finite_in(ic->name_scale, 0.1f, 10)) return NAN;
        int pt = ic->name_pt >= 8 && ic->name_pt <= 200 ? ic->name_pt : DM_PT_NAME_PT;
        float nb = top + k * (ic->h * ic->name_anchor + ic->name_y) + k * ic->name_scale * DM_PT_NAME_LINE * pt;
        if (nb > bottom) bottom = nb;
    }
    return bottom;
}


static inline int dm_pt_code(float units, int ui_y, int ui_h, int real_h) {
    if (!isfinite(units) || ui_h <= 0 || real_h <= 0) return DM_PT_UNKNOWN;
    double px = units * (double)ui_h / 2160.0 + ui_y, v = px / real_h * DM_PT_SCALE;
    return v <= 0 ? 0 : v >= DM_PT_MAX ? DM_PT_MAX : (int)lround(v);
}


static inline uint64_t dm_pt_pack(uint64_t ms, int code) { if (code < DM_PT_UNKNOWN) code = DM_PT_UNKNOWN; if (code > DM_PT_MAX) code = DM_PT_MAX; return (ms & ((1ull << 44) - 1)) << 20 | (uint64_t)(code + 2); }
static inline int dm_pt_unpack(uint64_t v, uint64_t now_ms) {
    if (!v) return DM_PT_UNKNOWN;
    uint64_t at = v >> 20, now = now_ms & ((1ull << 44) - 1); int code = (int)(v & 0xfffff) - 2;
    if (now < at || now - at > DM_PT_STALE_MS) return DM_PT_UNKNOWN;
    return code < DM_PT_UNKNOWN || code > DM_PT_MAX ? DM_PT_UNKNOWN : code;
}
