












#pragma once
#if !defined(__cplusplus)
#include <stdlib.h>


#define DM_PLAYER_ENV_KEEP(X) \
    X("DM_ANDROID_FULL") \
    X("DM_DERIVED") \
    X("DM_GFX_ASYNC_POOL_RESET") \
    X("DM_GFX_DEFER_EVENTS") \
    X("DM_GFX_FORWARD_TEMPS") \
    X("DM_GFX_LDC_FOLD") \
    X("DM_GFX_NAN_MINMAX") \
    X("DM_GFX_NATIVE_BC") \
    X("DM_GFX_PERF_SAMPLE") \
    X("DM_GFX_PIPE_MEMO") \
    X("DM_GFX_QUAD_DERIVATIVES") \
    X("DM_GFX_READ_PREFLIGHT") \
    X("DM_GFX_SHADER_REUSE") \
    X("DM_GFX_TEX_DIRTY") \
    X("DM_GFX_UPLOAD_PROLOGUE") \
    X("DM_GFX_VCACHE") \
    X("DM_GFX_WARP32") \
    X("DM_GFX_WB_SKIP") \
    X("DM_GFX_WTRACK") \
    X("DM_GFX_XVIEW_GUARD") \
    X("DM_HLE_SPIN_US") \
    X("DM_LANG") \
    X("DM_NVN_SYNC") \
    X("DM_ROMFS") \
    X("DM_ROMFS_OVERLAY") \
    X("DM_SAVE") \
    X("DM_TEMP")

#define DM_PLAYER_ENV_MATCH(name) || !__builtin_strcmp(k, name)

static inline __attribute__((always_inline)) int dm_player_env_kept(const char *k) {
    if (!k || __builtin_strncmp(k, "DM_", 3)) return 1;
    return 0 DM_PLAYER_ENV_KEEP(DM_PLAYER_ENV_MATCH);
}
static inline __attribute__((always_inline)) char *dm_player_getenv(const char *k) {
    return dm_player_env_kept(k) ? (getenv)(k) : (char *)0;
}
#undef DM_PLAYER_ENV_MATCH
#define getenv(k) dm_player_getenv(k)
#endif
