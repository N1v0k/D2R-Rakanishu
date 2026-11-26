







#pragma once
#include <stdint.h>
#include "game_addrs.h"
#include <string.h>

enum {
    DM_VT_TEXT = GA_VT_TEXTBOX,
    DM_VT_INPUT = GA_VT_INPUT_TEXT,
    DM_VT_NUMINPUT = GA_VT_NUMERIC_INPUT,
    DM_VT_TOGGLE = GA_VT_TOGGLE_BUTTON,
    DM_VT_TABBAR = GA_VT_TABBAR,
    DM_VT_SLOT = GA_VT_EQUIP_SLOT,
    DM_VT_SLOT_B = GA_VT_ITEM_GRID,
    DM_VT_SLOT_C = GA_VT_BELT,
    DM_VT_IMAGE = GA_VT_IMAGE,
    DM_VT_BUTTON = GA_VT_BUTTON,
    DM_VT_CAROUSEL = GA_VT_CAROUSEL,
    DM_VT_SLIDER = GA_VT_SETTINGS_SLIDER,
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
