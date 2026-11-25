

#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
void dm_ohos_touch_uimsg(const char *cat, const char *act, const char *arg);
void dm_ohos_touch_screen(float w, float h, float safe_l, float safe_r, float safe_b);
void dm_ohos_touch_event(int phase, int id, float x, float y);
void dm_ohos_touch_focus(int active);


int dm_ohos_touch_tick(float *out, int cap);
void front_ohos_view_rect(float fw, float fh, float out[4]);
int front_ohos_full(void);


enum {
    DM_OS_VISIBLE, DM_OS_VX, DM_OS_VY, DM_OS_VW, DM_OS_VH, DM_OS_NAV, DM_OS_SNAV, DM_OS_DIALOG, DM_OS_BLOCKED, DM_OS_SET,
    DM_OS_MAPCANCEL, DM_OS_DIFFOPEN, DM_OS_PLUSHELD, DM_OS_BARVIS, DM_OS_DEAD, DM_OS_JOYACT, DM_OS_JOYX, DM_OS_JOYY, DM_OS_JOYR,
    DM_OS_LX, DM_OS_LY, DM_OS_SWX, DM_OS_SWY, DM_OS_SWR, DM_OS_SWHELD, DM_OS_COVER, DM_OS_HEADER = 32,
    DM_OS_TARGETS = DM_OS_HEADER, DM_OS_TSTRIDE = 6,
    DM_OS_GLYPHS = DM_OS_TARGETS + 12 * DM_OS_TSTRIDE, DM_OS_GSTRIDE = 8,
    DM_OS_HUD = DM_OS_GLYPHS + 6 * DM_OS_GSTRIDE,
    DM_OS_FLOATS = DM_OS_HUD + 16
};
#ifdef __cplusplus
}
#endif
