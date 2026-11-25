#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif


void dm_android_touch_resize(float width_dp, float height_dp, float left_dp, float right_dp, float bottom_dp);


void dm_android_touch_view(float x_dp, float y_dp, float width_dp, float height_dp);
void dm_android_touch_tablet(int tablet);





void dm_android_touch_event(int phase, int slot, float x, float y);
void dm_android_touch_focus(int active);
void dm_android_touch_uimsg(const char *cat, const char *act, const char *arg);
void dm_android_touch_step(uint64_t guest_now, uint64_t *buttons, int32_t *lx, int32_t *ly, int32_t *rx, int32_t *ry);




enum { DM_ANDROID_TOUCH_GLYPHS = 4 + 23 * 5, DM_ANDROID_TOUCH_HUD = DM_ANDROID_TOUCH_GLYPHS + 6 * 8,
       DM_ANDROID_TOUCH_DRAW_FLOATS = DM_ANDROID_TOUCH_HUD + 18 };
void dm_android_touch_layout(float out[DM_ANDROID_TOUCH_DRAW_FLOATS]);
#ifdef __cplusplus
}
#endif
