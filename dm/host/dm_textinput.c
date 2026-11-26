













#include "hle.h"
#include "game_addrs.h"
#include "dm.h"
#include <string.h>
#include <stdio.h>
#include <stdatomic.h>

extern int g_swkbd_numeric_max;
static _Atomic int g_ti_open, g_num_set = INT32_MIN;
static uint8_t *tb(void) { static uint8_t *b; if (!b) for (int i = 0; i < g_nmod; i++) if (!strcmp(g_mod[i].name, "main")) b = (uint8_t *)g_mod[i].base; return b; }

void dm_num_set(int n);
void dm_textinput_on_uimsg(const char *cat, const char *act, const char *arg) {


    if (!strcmp(cat, "PanelManager") && !strcmp(act, "ClosePanel") && !strncmp(arg, "TouchNum", 8)) { if (!strcmp(arg + 8, "Half")) dm_num_set(-2); else if (!strcmp(arg + 8, "All")) dm_num_set(-1); }
    if (!strcmp(cat, "PanelManager") && !strcmp(arg, "TextInputModal")) { if (!strcmp(act, "OpenPanel")) atomic_store(&g_ti_open, 1); else if (!strcmp(act, "ClosePanel") || !strcmp(act, "UnloadPanel")) atomic_store(&g_ti_open, 0); }
}


void dm_num_set(int n) { atomic_store(&g_num_set, n); }
static uint64_t ti_find(uint64_t w, const char *name, int dep) {
    if (!w || dep > 14) return 0; const char *nm = *(const char **)(w + 8); if (nm && !strcmp(nm, name)) return w;
    uint64_t ch = *(uint64_t *)(w + 0x58), n = *(uint64_t *)(w + 0x60); if (!ch || n > 256) return 0;
    for (uint64_t i = 0; i < n; i++) { uint64_t r = ti_find(*(uint64_t *)(ch + 8 * i), name, dep + 1); if (r) return r; }
    return 0;
}
void dm_textinput_pump(void) {
    uint8_t *b = tb(); if (!b) return; uint64_t root = *(uint64_t *)(b + GA_UI_ROOT); if (!root) return;
    if (atomic_exchange(&g_ti_open, 0)) {
        uint64_t w = *(uint64_t *)(b + GA_CUR_INPUT_BOX);
        if (w) {
            int num = *(uint64_t *)w - (uint64_t)(uintptr_t)b == GA_VT_NUMERIC_INPUT; g_swkbd_numeric_max = num ? *(int32_t *)(w + 0x12e0) : -1;
            LOG("文本框：TextInputModal 打开 → 弹系统键盘（%s%s%d）", num ? "数字框" : "文字框", num ? "，上限 " : "，类型 ", num ? g_swkbd_numeric_max : *(int32_t *)(w + 0x58c));
            ((void (*)(uint64_t))(b + GA_TEXTINPUT_OPEN_KEYBOARD))(w); *(uint8_t *)(root + 0xb8) = 1;
        }
    }
    int n = atomic_exchange(&g_num_set, INT32_MIN);
    if (n != INT32_MIN) {
        uint64_t modal = ti_find(root, "DropGoldModal", 0), w = modal && *(uint8_t *)(modal + 0x50) ? *(uint64_t *)(modal + 0x210) : 0;
        if (w && *(uint64_t *)w - (uint64_t)(uintptr_t)b == GA_VT_NUMERIC_INPUT) {
            int max = *(int32_t *)(w + 0x12e0), min = *(int32_t *)(w + 0x12e4), v = n == -1 ? max : n == -2 ? max / 2 : n; if (v > max) v = max; if (v < min) v = min;
            char t[16]; struct { const char *p; uint64_t n; } sv = { t, (uint64_t)snprintf(t, sizeof t, "%d", v) };
            ((void (*)(uint64_t, void *))(b + GA_TEXTINPUT_SET_TEXT))(w, &sv);
            ((void (*)(uint64_t))(b + GA_NUMINPUT_PARSE_CLAMP))(w);
            LOG("数量框：设为 %d（范围 %d–%d）→ 控件当前值 %d", v, min, max, *(int32_t *)(w + 0x12fc));
        } else LOG("数量框：没有打开的数量弹窗，忽略设值 %d", n);
    }
}
