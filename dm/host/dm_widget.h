







#pragma once
#include <stdint.h>
#include <string.h>

enum {
    DM_VT_TEXT = 0x20ba4c8,
    DM_VT_INPUT = 0x20b8718,
    DM_VT_NUMINPUT = 0x20b91c0,
    DM_VT_TOGGLE = 0x20bc110,
    DM_VT_TABBAR = 0x20ba3a8,
    DM_VT_SLOT = 0x2089fa0,
    DM_VT_SLOT_B = 0x2089ab8,
    DM_VT_SLOT_C = 0x2088fc8,
    DM_VT_IMAGE = 0x20b8650,
    DM_VT_BUTTON = 0x20b7b00,
    DM_VT_CAROUSEL = 0x20b7c28,
    DM_VT_SLIDER = 0x20bad70,
};

static inline uint64_t dm_widget_vt(const uint8_t *base, uint64_t w) { return w && base ? *(const uint64_t *)(uintptr_t)w - (uint64_t)(uintptr_t)base : 0; }
static inline int dm_widget_is(const uint8_t *base, uint64_t w, uint64_t vt) { return w && base && dm_widget_vt(base, w) == vt; }
static inline int dm_widget_is_slot(const uint8_t *base, uint64_t w) { uint64_t v = dm_widget_vt(base, w); return v == DM_VT_SLOT || v == DM_VT_SLOT_B || v == DM_VT_SLOT_C; }



#define DM_WIDGET_OK(base, w, vt, who) (dm_widget_is((base), (w), (vt)) ? 1 : dm_widget_red_((base), (w), (vt), (who)))
#define DM_WIDGET_RED_DEF \
    static int dm_widget_red_(const uint8_t *base, uint64_t w, uint64_t vt, const char *who) { static int n; \
        if (w && base && n++ < 5) { const char *nm = *(const char **)(uintptr_t)(w + 8); \
            LOG("【红】%s：控件「%s」不是预期的类（虚表 main+%#llx，应为 %#llx），不按偏移读写", who, nm ? nm : "?", (unsigned long long)dm_widget_vt(base, w), (unsigned long long)vt); } \
        return 0; }







int dm_ui_top_chk_on(void);
void dm_ui_top_chk(uint64_t root, const char *name, uint64_t got);
static inline uint64_t dm_ui_top(uint64_t root, const char *name) {
    uint64_t r = 0, ch = root ? *(uint64_t *)(uintptr_t)(root + 0x58) : 0, n = root ? *(uint64_t *)(uintptr_t)(root + 0x60) : 0;
    if (ch && n <= 256) for (uint64_t j = 0; j < n && !r; j++) { uint64_t p = *(uint64_t *)(uintptr_t)(ch + 8 * j); const char *nm = p ? *(const char **)(uintptr_t)(p + 8) : 0; if (nm && !strcmp(nm, name)) r = p; }
    if (dm_ui_top_chk_on()) dm_ui_top_chk(root, name, r);
    return r;
}
