















#include "hle.h"
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
        if ((off >= 0x7fe8a8 && off < 0x7feeec) || (off >= 0x807fd8 && off < 0x809d60) ||
            (off >= 0x6f0c70 && off < 0x6f1220) || (off >= 0x6fe640 && off < 0x702e40)) return 1;
    }
    return 0;
}





int dm_charname_unfilter(void) {
    uintptr_t b = main_base(); if (!b) return 0;
    uint64_t *fn = (uint64_t *)(b + 0x3213dd8);
    if (fn[0] == 1 || fn[1] != b + 0xe09190) return 0;
    fn[0] = 1; return 1;
}

__attribute__((constructor)) static void charname_register(void) { extern int (*g_swkbd_unfilter)(void); g_swkbd_unfilter = dm_charname_unfilter; }
