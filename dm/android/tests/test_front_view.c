

#include <stdio.h>
#include <stdlib.h>
#include "dm/android/front_view.h"

static int checks, fails;
#define CHECK(c, ...) do { checks++; if (!(c)) { fails++; printf("FAIL line %d: ", __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)
static int is(const int *o, int a, int b, int c, int d, int e, int f, int g, int h) { return o[0] == a && o[1] == b && o[2] == c && o[3] == d && o[4] == e && o[5] == f && o[6] == g && o[7] == h; }

int main(void) {
    int o[8];

    dm_front_view(2048, 1536, 1, 1280, 960, 0, 240, 1280, 720, o);
    CHECK(is(o, 0, 384, 2048, 1152, 0, 0, 2048, 1536), "4:3 run: %d %d %d %d / %d %d %d %d", o[0], o[1], o[2], o[3], o[4], o[5], o[6], o[7]);

    dm_front_view(2048, 1536, 1, 1280, 960, 0, 120, 1280, 720, o);
    CHECK(is(o, 0, 192, 2048, 1152, 0, 0, 2048, 1536), "4:3 panel: %d %d", o[1], o[3]);

    dm_front_view(2048, 1536, 1, 1280, 720, 0, 0, 1280, 720, o);
    CHECK(is(o, 0, 192, 2048, 1152, 0, 192, 2048, 1152), "4:3 front end: %d %d %d %d / %d %d %d %d", o[0], o[1], o[2], o[3], o[4], o[5], o[6], o[7]);

    dm_front_view(2048, 1536, 0, 0, 0, 0, 0, 0, 0, o);
    CHECK(is(o, 0, 192, 2048, 1152, 0, 192, 2048, 1152), "before the first frame: centred 16:9");

    dm_front_view(2208, 1840, 1, 1280, 1068, 0, 348, 1280, 720, o);
    CHECK(o[4] == 0 && o[5] == 0 && o[6] == 2208 && o[7] == 1840 && o[2] == 2208 && o[3] == 1240 && o[1] + o[3] == 1840 && o[0] == 0, "foldable: %d %d %d %d", o[0], o[1], o[2], o[3]);

    dm_front_view(2208, 1840, 1, 1080, 900, 0, 180, 1080, 720, o);
    int vh = (int)lround(720.0 * 1840 / 900), shift = (int)lround(1173.6 * vh / 1320.0 - 2208 * 0.5);
    CHECK(shift > 0 && o[0] == shift && o[2] == 2208 && o[3] == vh, "HUD centre correction: x %d (want %d), w %d, h %d", o[0], shift, o[2], o[3]);

    dm_front_view(2048, 1536, 1, 1280, 960, 0, 0, 1280, 960, o);
    CHECK(is(o, 0, 0, 2048, 1536, 0, 0, 2048, 1536), "UI = picture: no shift (%d)", o[0]);

    dm_front_view(2048, 1536, 1, 2048, 1536, 0, 384, 2048, 1152, o);
    CHECK(o[0] == 0 && o[2] == 2048, "16:9 UI: no shift");

    dm_front_view(0, 0, 1, 1280, 960, 0, 240, 1280, 720, o);
    CHECK(is(o, 0, 0, 0, 0, 0, 0, 0, 0), "no window");
    printf("test_front_view: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
