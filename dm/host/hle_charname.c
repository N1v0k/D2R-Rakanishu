















#include "hle.h"
#include "game_addrs.h"
#include "dm.h"

static uintptr_t main_base(void) {
    static uintptr_t b;
    if (!b) for (int i = 0; i < g_nmod; i++) if (!strcmp(g_mod[i].name, "main")) b = (uintptr_t)g_mod[i].base;
    return b;
}

HLE_FORCE("isalpha", "离线角色名放行非 ASCII：只对角色名三处校验里的调用把 ≥0x80 的字节当字母，其余同 SDK 原实现", int, h_isalpha, int c) {
    if ((unsigned)((c | 0x20) - 'a') < 26) return 1;
    if (c >= 0x80 || c < 0) {
        uintptr_t off = (uintptr_t)__builtin_return_address(0) - main_base();
        if ((off >= GA_CHARNAME_CHECK_JOIN && off < GA_AT_CHARNAME_CHECK_JOIN_END) || (off >= GA_CHARNAME_CHECK_LOAD && off < GA_AT_CHARNAME_CHECK_LOAD_END) ||
            (off >= GA_SRV_SAVE_CHECK_A && off < GA_AT_SRV_SAVE_CHECK_A_END) || (off >= GA_SRV_SAVE_CHECK_B && off < GA_AT_SRV_SAVE_CHECK_B_END)) return 1;
    }
    return 0;
}





int dm_charname_unfilter(void) {
    uintptr_t b = main_base(); if (!b) return 0;
    uint64_t *fn = (uint64_t *)(b + GA_SWKBD_TEXT_FILTER);
    if (fn[0] == 1 || fn[1] != b + GA_CHARNAME_TYPE_FILTER) return 0;
    fn[0] = 1; return 1;
}

__attribute__((constructor)) static void charname_register(void) { extern int (*g_swkbd_unfilter)(void); g_swkbd_unfilter = dm_charname_unfilter; }
