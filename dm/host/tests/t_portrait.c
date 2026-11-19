

#include "dm_portrait.h"
#include <stdio.h>

static int checks, fails;
#define CHECK(c) do { checks++; if (!(c)) { fails++; printf("FAIL line %d: %s\n", __LINE__, #c); } } while (0)
#define NEAR(a, b, e) (fabs((double)(a) - (double)(b)) <= (e))

int main(void) {

    dm_pt_icon merc = { -206, 120, 1.0f, 1, 0, 0.98f, 1.0f, 40 };
    float b = dm_pt_icon_bottom(266, 1.0f, &merc);
    CHECK(NEAR(b, 60 + 117.6 + 68, 0.01));
    CHECK(dm_pt_code(b, 0, 1216, 1216) == (int)lround(b * 1216 / 2160 / 1216 * DM_PT_SCALE));
    CHECK(NEAR(dm_pt_code(b, 0, 1216, 1216) / (double)DM_PT_SCALE * 1216, 138, 1));

    dm_pt_icon bare = merc; bare.name = 0;
    CHECK(NEAR(dm_pt_icon_bottom(266, 1.0f, &bare), 180, 0.01));

    dm_pt_icon odd = merc; odd.name_pt = 30184;
    CHECK(NEAR(dm_pt_icon_bottom(266, 1.0f, &odd), b, 0.01));

    dm_pt_icon col = merc; col.y = 0;
    CHECK(dm_pt_icon_bottom(266, 1.0f, &col) > b + 200);

    dm_pt_icon big = merc; big.scale = 2.0f;
    CHECK(NEAR(dm_pt_icon_bottom(266, 1.0f, &big), 60 + 2 * (120 * 0.98f) + 2 * 68, 0.01));
    dm_pt_icon nm2 = merc; nm2.name_scale = 0.5f;
    CHECK(NEAR(dm_pt_icon_bottom(266, 1.0f, &nm2), 177.6 + 34, 0.01));

    dm_pt_icon bad = merc; bad.scale = NAN; CHECK(isnan(dm_pt_icon_bottom(266, 1.0f, &bad)));
    bad = merc; bad.h = 0; CHECK(isnan(dm_pt_icon_bottom(266, 1.0f, &bad)));
    bad = merc; bad.h = 5000; CHECK(isnan(dm_pt_icon_bottom(266, 1.0f, &bad)));
    CHECK(isnan(dm_pt_icon_bottom(266, 0.0f, &merc)));
    bad = merc; bad.name_anchor = INFINITY; CHECK(isnan(dm_pt_icon_bottom(266, 1.0f, &bad)));
    bad = merc; bad.name_scale = 0; CHECK(isnan(dm_pt_icon_bottom(266, 1.0f, &bad)));
    bad = merc; bad.name = 0; bad.name_anchor = NAN; CHECK(NEAR(dm_pt_icon_bottom(266, 1.0f, &bad), 180, 0.01));

    CHECK(dm_pt_code(1080, 100, 900, 1000) == (int)lround((1080 * 900 / 2160.0 + 100) / 1000 * DM_PT_SCALE));
    CHECK(dm_pt_code(-50, 0, 1080, 1080) == 0);
    CHECK(dm_pt_code(1e9f, 0, 1080, 1080) == DM_PT_MAX);
    CHECK(dm_pt_code(NAN, 0, 1080, 1080) == DM_PT_UNKNOWN);
    CHECK(dm_pt_code(100, 0, 0, 1080) == DM_PT_UNKNOWN);

    CHECK(dm_pt_unpack(0, 1000) == DM_PT_UNKNOWN);
    CHECK(dm_pt_unpack(dm_pt_pack(1000, 11348), 1000) == 11348);
    CHECK(dm_pt_unpack(dm_pt_pack(1000, 0), 1200) == 0);
    CHECK(dm_pt_unpack(dm_pt_pack(1000, DM_PT_EMPTY), 1499) == DM_PT_EMPTY);
    CHECK(dm_pt_unpack(dm_pt_pack(1000, DM_PT_UNKNOWN), 1000) == DM_PT_UNKNOWN);
    CHECK(dm_pt_unpack(dm_pt_pack(1000, DM_PT_MAX), 1000) == DM_PT_MAX);
    CHECK(dm_pt_unpack(dm_pt_pack(1000, 11348), 1000 + DM_PT_STALE_MS) == 11348);
    CHECK(dm_pt_unpack(dm_pt_pack(1000, 11348), 1001 + DM_PT_STALE_MS) == DM_PT_UNKNOWN);
    CHECK(dm_pt_unpack(dm_pt_pack(1000, 11348), 999) == DM_PT_UNKNOWN);
    CHECK(dm_pt_unpack(dm_pt_pack(5000, 123456789), 5000) == DM_PT_MAX);
    CHECK(dm_pt_unpack(dm_pt_pack(5000, -77), 5000) == DM_PT_UNKNOWN);
    printf("t_portrait: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
