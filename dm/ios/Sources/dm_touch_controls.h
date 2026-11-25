#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum { DM_TC_MAX_TOUCH = 16, DM_TC_COUNT = 12 };
enum {
    DM_TC_NONE = -1, DM_TC_JOYSTICK = -2, DM_TC_SWAP = -3, DM_TC_DIFF = -4,
    DM_TC_EDGE = -5,
    DM_TC_MAIN = 0,
    DM_TC_SKILL1, DM_TC_SKILL2, DM_TC_SKILL3, DM_TC_SKILL4,
    DM_TC_PORTAL, DM_TC_BELT1, DM_TC_BELT2, DM_TC_BELT3, DM_TC_BELT4,
    DM_TC_MENU, DM_TC_MAP
};
typedef struct { float x, y, r; uint64_t button; } dm_tc_target;
typedef struct {
    float width, height;
    int tablet, swap_above;
    dm_tc_target target[DM_TC_COUNT];
    float joy_x, joy_y, joy_radius;
    float joy_zone_r, joy_zone_t, joy_min_x, joy_max_y;
    dm_tc_target swap;
    dm_tc_target diff;
    dm_tc_target pick[8];
} dm_tc_layout;
typedef struct {
    int active, target;
    float x, y;
    float aim_origin_x, aim_origin_y;
    uint64_t aim_move_sequence;
    int aim_cancelled;
    int key_down; double key_down_s;
    double start_time_s;
    int pointer_dragged, pointer_long;




    int ui_drag;
    int shop_drag;
    int no_tap;
    int cam_drag; float cam_x, cam_y;
    int locked_drag;
} dm_tc_finger;
typedef struct {
    dm_tc_layout layout;
    dm_tc_finger finger[DM_TC_MAX_TOUCH];
    int joy_slot;
    float joy_origin_x, joy_origin_y;
    uint32_t enabled_targets;
    uint64_t buttons;
    int32_t lx, ly, rx, ry;
    int aim_slot, navigation_mode, map_cancel_active;
    uint64_t aim_move_sequence;
    int portal_pending, portal_phase;
    uint64_t gesture_epoch, portal_binding_identity, portal_scene_epoch;

    uint64_t pulse_button, pulse_epoch;
    int32_t pulse_rx, pulse_ry;
    uint64_t defer_button; double defer_at_s;

    int skill_set;




    int pointer_slot;
    double last_time_s;
    int pointer_pickup_done;
    int pointer_pulse_active, pointer_pulse_pending;
    float pointer_pending_x, pointer_pending_y;
    int pointer_valid;
    float pointer_x, pointer_y;






    int pointer_item;








    int pointer_release_item, holding; float release_x, release_y;




    unsigned world_tap_seq; float world_tap_x, world_tap_y;



    int cam_on; float cam_pitch, cam_yaw;



    int cam_orbit;
    int close_left; double close_next_s;




    int pointer_shop; unsigned shop_drop_seq;
    int pointer_lock;

    int pointer_stack; unsigned stack_take_seq;

    int pointer_merc;
    unsigned ident_seq;
    unsigned use_seq;
    unsigned item_tap_seq;
    unsigned long_press_seq;
    unsigned tab_tap_seq;
    int mouse_click_mode; unsigned mouse_click_seq; float mouse_click_x, mouse_click_y;

    uint64_t tap_button; float tap_x, tap_y; int tap_move; unsigned tap_move_seq;
    double pulse_hold_s;
    double last_tap_s; float last_tap_x, last_tap_y;
    double stash_x_check_s; float stash_x_x, stash_x_y; unsigned stash_x_id;
    uint64_t pointer_pending_button; double pointer_pending_hold;

    int diff_open, diff_request;






    int belt_rows, belt_open, belt_request;

    int belt_gap_cell; double belt_gap_at;
    double joy_lost_s; float joy_lost_x, joy_lost_y, joy_lost_ox, joy_lost_oy;
    int joy_block;
    float joy_touch_x, joy_touch_y, joy_lost_tx, joy_lost_ty;
    double belt_pulse_s;
    int belt_q;
} dm_tc_state;


typedef struct { int game; uint64_t panels; } dm_tc_scene;
int dm_tc_navigation(dm_tc_scene *scene, const char *cat, const char *act, const char *arg);
int dm_tc_scene_dialog_only(const dm_tc_scene *scene);
int dm_tc_scene_dead(void);




int dm_tc_panel_mode(int nav, const dm_tc_scene *scene, int blocked);



void dm_tc_set_safe_insets(float left, float right);
void dm_tc_set_edge_bottom(float bottom);
void dm_tc_layout_make(dm_tc_layout *layout, float width, float height);
int dm_tc_hit(const dm_tc_layout *layout, float x, float y);
#define DM_TC_POINTER_LIFT_PT 40.0f
void dm_tc_set_swap_above(int above);
void dm_tc_set_safe_bottom(float bottom);
void dm_tc_set_tall_scale(int on);
float dm_tc_scale_for_size(float width, float height);
void dm_tc_init(dm_tc_state *state, float width, float height);

void dm_tc_resize(dm_tc_state *state, float width, float height);

void dm_tc_enable_targets(dm_tc_state *state, uint32_t mask);








static inline uint32_t dm_tc_nav_mask(int pad) { return pad ? ((1u<<DM_TC_MAIN)|(1u<<DM_TC_SKILL1)|(1u<<DM_TC_SKILL2)|(1u<<DM_TC_SKILL3)|(1u<<DM_TC_SKILL4)) : 0; }

void dm_tc_set_navigation_mode(dm_tc_state *state, int navigation_mode);
void dm_tc_set_joy_block(dm_tc_state *state, int block);

void dm_tc_set_portal_button(dm_tc_state *state, uint64_t mask);
void dm_tc_set_portal_binding(dm_tc_state *state, uint64_t mask, uint64_t binding_identity, uint64_t scene_epoch);

int dm_tc_portal_ack(dm_tc_state *state, uint64_t gesture_epoch, uint64_t binding_identity, uint64_t scene_epoch);
void dm_tc_cancel_portal(dm_tc_state *state);
void dm_tc_begin(dm_tc_state *state, int slot, float x, float y);
void dm_tc_move(dm_tc_state *state, int slot, float x, float y);
void dm_tc_end(dm_tc_state *state, int slot);
void dm_tc_begin_at(dm_tc_state *state, int slot, float x, float y, double now_s);





void dm_tc_begin_stick_at(dm_tc_state *state, int slot, float x, float y, double now_s);
void dm_tc_move_at(dm_tc_state *state, int slot, float x, float y, double now_s);
void dm_tc_end_at(dm_tc_state *state, int slot, double now_s);
void dm_tc_tick(dm_tc_state *state, double now_s);
void dm_tc_cancel_all(dm_tc_state *state);
void dm_tc_cancel_finger(dm_tc_state *state, int slot);

int dm_tc_pulse_end(dm_tc_state *state, uint64_t pulse_epoch);


int dm_tc_pulse_pending(const dm_tc_state *state);
void dm_tc_pointer_press(dm_tc_state *state, float x, float y, uint64_t button);
void dm_tc_pointer_item(dm_tc_state *state, int kind);
void dm_tc_pointer_release(dm_tc_state *state, int kind);
void dm_tc_pointer_release_at(dm_tc_state *state, float x, float y);
void dm_tc_set_holding(dm_tc_state *state, int holding);
void dm_tc_set_cur_stack(unsigned id);
void dm_tc_set_hover_unid(unsigned id);
void dm_tc_set_tap_box(int on);
void dm_tc_set_panel_state(int bits);
void dm_tc_set_iso_fix(int on);
int dm_tc_belt_outside(const dm_tc_state *state, float x, float y);
#define DM_TC_AIM_SETTLE_S 0.08
#define DM_TC_BELT_SWIPE_PT 24
#define DM_TC_KEY_MIN_S 0.035
#define DM_TC_JOY_RESUME_S 0.35
#define DM_TC_JOY_GRAB_K 1.5f
#define DM_TC_ZL_LEAD_S 0.17
#define DM_TC_DOUBLE_TAP_S 0.40
#define DM_TC_HOLD_USE_S 0.60
int dm_tc_pulse_resume(dm_tc_state *state);

#ifdef __cplusplus
}
#endif
