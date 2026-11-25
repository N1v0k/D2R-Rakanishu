#pragma once
#ifdef __cplusplus
extern "C" {
#endif



int front_android_full(void);


int front_android_letterbox(void);

int front_android_window_size(int *w, int *h);



void front_android_view_rect(int win_w, int win_h, int out[8]);



void front_android_drop_haptic(int tier);
#ifdef __cplusplus
}
#endif
