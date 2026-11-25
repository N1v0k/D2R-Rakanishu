







#pragma once
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

enum { DM_CPU_UNKNOWN = -1, DM_CPU_NATIVE = 0, DM_CPU_TRANSLATED = 1 };



static void dm_android_primary_abi(const char *abilist, const char *abi, char *out, size_t bytes) {
    if (!out || !bytes) return;
    out[0] = 0;
    const char *src = abilist && *abilist ? abilist : abi;
    if (!src) return;
    size_t n = strcspn(src, ",");
    if (n >= bytes) n = bytes - 1;
    memcpy(out, src, n); out[n] = 0;
}



static int dm_android_cpu_translation(const char *abilist, const char *abi) {
    char primary[96];
    dm_android_primary_abi(abilist, abi, primary, sizeof primary);
    if (!strcmp(primary, "x86_64") || !strcmp(primary, "x86")) return DM_CPU_TRANSLATED;
    if (!strcmp(primary, "arm64-v8a")) return DM_CPU_NATIVE;
    return DM_CPU_UNKNOWN;
}







static void dm_android_translator_defaults(void) {
    setenv("DM_GFX_VCACHE", "0", 0);
    setenv("DM_GFX_TEX_DIRTY", "0", 0);
    setenv("DM_GFX_WTRACK", "0", 0);
}
