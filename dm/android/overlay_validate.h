#pragma once
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif





typedef enum {
    DM_OVERLAY_OK = 0,
    DM_OVERLAY_MISSING = 1,
    DM_OVERLAY_INVALID = 2
} dm_overlay_result;

dm_overlay_result dm_overlay_validate(const char *directory,
                                      const char *expected_source_sha256,
                                      char *reason, size_t reason_capacity);


dm_overlay_result dm_overlay_require_touch_art(const char *directory, char *reason, size_t reason_capacity);

#ifdef __cplusplus
}
#endif
