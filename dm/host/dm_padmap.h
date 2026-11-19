









#pragma once
#include <stdint.h>
#include "dm_input.h"
static inline uint64_t dm_padmap_face(int a, int b, int x, int y) {
    return (a ? DM_BTN_A : 0) | (b ? DM_BTN_B : 0) | (x ? DM_BTN_X : 0) | (y ? DM_BTN_Y : 0);
}






static inline uint64_t dm_padmap_face_xbox(int a, int b, int x, int y, int nintendo) {
    return nintendo ? dm_padmap_face(b, a, x, y) : dm_padmap_face(a, b, y, x);
}
