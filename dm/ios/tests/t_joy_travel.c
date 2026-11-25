
#include "dm_touch_controls.h"
#include "../../host/dm_input.h"
#include <stdio.h>
#include <stdlib.h>
static int checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr,"FAIL line=%d %s\n",__LINE__,#x); exit(1); } } while (0)
static int push(dm_tc_state *s, float w, float h, float d) {
    dm_tc_init(s, w, h); float cx = s->layout.joy_x, cy = s->layout.joy_y; dm_tc_begin(s, 0, cx, cy); CHECK(s->joy_slot == 0);
    dm_tc_move(s, 0, cx + d, cy); return s->lx;
}
int main(void) {
    dm_tc_state s; dm_tc_set_tall_scale(1);
    static const float size[][2] = {{956, 440}, {1194, 672}, {1366, 768}};
    for (int i = 0; i < 3; ++i) {
        float w = size[i][0], h = size[i][1];
        CHECK(push(&s, w, h, 6) == 0);
        CHECK(abs(push(&s, w, h, 20) - 8495) <= 1);
        CHECK(abs(push(&s, w, h, 33) - 16384) <= 1);
        CHECK(push(&s, w, h, 60) == DM_STICK_MAX);
        CHECK(push(&s, w, h, 105) == DM_STICK_MAX);
        CHECK(abs(push(&s, w, h, -20) + 8495) <= 1);
    }
    dm_tc_init(&s, 1366, 768); CHECK(s.layout.joy_radius > 68 && s.layout.joy_radius < 70);


    dm_tc_init(&s, 956, 440); dm_tc_begin(&s, 0, 90, 425); CHECK(s.joy_slot == 0); dm_tc_cancel_all(&s);

    dm_tc_set_edge_bottom(20); dm_tc_init(&s, 1366, 768);
    dm_tc_begin(&s, 0, 100, 760); CHECK(s.joy_slot == DM_TC_NONE && s.finger[0].target == DM_TC_EDGE);
    dm_tc_move(&s, 0, 100, 700); CHECK(s.lx == 0 && s.ly == 0 && s.joy_slot == DM_TC_NONE);
    dm_tc_end(&s, 0); CHECK(s.world_tap_seq == 0 && s.buttons == 0);
    dm_tc_begin(&s, 0, 100, 762); dm_tc_end(&s, 0); CHECK(s.world_tap_seq == 0);
    dm_tc_begin(&s, 0, 150, 745); CHECK(s.joy_slot == 0);
    dm_tc_move(&s, 0, 160, 766); CHECK(s.joy_slot == 0 && s.lx > 0 && s.ly < 0);
    dm_tc_begin(&s, 1, 200, 765); CHECK(s.joy_slot == 0 && s.finger[1].target == DM_TC_EDGE);
    dm_tc_end(&s, 1); CHECK(s.joy_slot == 0 && s.lx > 0);
    dm_tc_end(&s, 0); CHECK(s.lx == 0 && s.ly == 0 && s.joy_slot == DM_TC_NONE);
    dm_tc_begin(&s, 0, 800, 760); CHECK(s.finger[0].target != DM_TC_EDGE); dm_tc_cancel_all(&s);
    dm_tc_set_edge_bottom(0);
    printf("JOY_TRAVEL OK %d 项\n", checks); return 0;
}
