#include "dm_touch_controls.h"
#include "../../host/dm_input.h"
#include <math.h>
#include <string.h>

#include <stdatomic.h>
#define DM_TC_POINTER_MOVE_PT 10.0f
#define DM_TC_POINTER_HOLD_S 0.5
#define DM_TC_POINTER_TAP_S 0.35







static int g_tall_scale, g_swap_above;
void dm_tc_set_tall_scale(int on) { g_tall_scale = !!on; }
void dm_tc_set_swap_above(int above) { g_swap_above = !!above; }
float dm_tc_scale_for_size(float width, float height) {
    float fit=fminf(width/667.0f,height/375.0f);
    return g_tall_scale ? fminf(1.15f,fmaxf(1.0f,fit)) : fminf(1.0f,fit);
}
static float scale_for(const dm_tc_layout *l) { return dm_tc_scale_for_size(l->width, l->height); }





static float g_safe_l, g_safe_r, g_safe_b;
void dm_tc_set_safe_bottom(float bottom) { g_safe_b=isfinite(bottom) && bottom>0 ? bottom : 0; }




static float g_edge_b;
void dm_tc_set_edge_bottom(float bottom) { g_edge_b = isfinite(bottom) && bottom > 0 ? bottom : 0; }
void dm_tc_set_safe_insets(float left, float right) { g_safe_l = isfinite(left) && left > 0 ? left : 0; g_safe_r = isfinite(right) && right > 0 ? right : 0; }

void dm_tc_layout_make(dm_tc_layout *l, float width, float height) {
    memset(l, 0, sizeof(*l));
    l->width = isfinite(width) && width > 0 ? width : 0;
    l->height = isfinite(height) && height > 0 ? height : 0;
    float k = scale_for(l);
    l->tablet=g_tall_scale; l->swap_above=g_swap_above;




    float main_x=l->width-g_safe_r-86*k, main_y=l->height-88*k-fmaxf(24*k,g_safe_b+8*k);
    if (g_tall_scale && height<340) main_y=fmaxf(main_y,190*k);


    main_x+=12*k;
    if (!(g_tall_scale && height<340)) main_y+=28*k;
    l->target[DM_TC_MAIN]=(dm_tc_target){main_x,main_y,40*k,DM_BTN_A};
    const float dx[4]={-96,-84,-34,34},dy[4]={18,-50,-92,-92};
    const uint64_t skill[4]={DM_BTN_B,DM_BTN_X,DM_BTN_Y,DM_BTN_R};
    for(int i=0;i<4;i++)
        l->target[DM_TC_SKILL1+i]=(dm_tc_target){main_x+dx[i]*k,main_y+dy[i]*k,26*k,skill[i]};







    const uint64_t bottom[5] = {DM_BTN_LEFT, DM_BTN_UP, DM_BTN_DOWN, DM_BTN_RIGHT, DM_BTN_PLUS};
    const int cell[5] = {1, 2, 3, 4, 5};
    float u = l->height / 1320.0f;
    for (int i = 0; i < 5; ++i)
        l->target[DM_TC_BELT1+i] = (dm_tc_target){l->width/2+(1090+120*cell[i]-1434)*u, 1241.5f*u, 57*u, bottom[i]};
    { const dm_tc_target *s4 = &l->target[DM_TC_SKILL4]; l->target[DM_TC_PORTAL] = (dm_tc_target){s4->x, s4->y-s4->r-12*k-26*k, 26*k, 0}; }


    if (g_tall_scale && height<340)
        l->target[DM_TC_PORTAL]=(dm_tc_target){main_x-34*k,main_y-156*k,26*k,0};

    l->diff = (dm_tc_target){l->width/2+(1090-1434)*u, 1241.5f*u, 57*u, 0};
    for (int i = 0; i < 8; ++i) l->pick[i] = (dm_tc_target){l->width/2+(1390+(i-3.5f)*120-1434)*u, 1051.5f*u, 57*u, 0};
    l->target[DM_TC_MAP] = (dm_tc_target){l->width-30*k, 30*k, 22*k, DM_BTN_MINUS};


    const dm_tc_target *portal=&l->target[DM_TC_PORTAL];
    float px=portal->x-main_x,py=portal->y-main_y,cs=cosf(.46f),sn=sinf(.46f);
    l->swap=(dm_tc_target){main_x+px*cs+py*sn,main_y-px*sn+py*cs,22*k,0};
    if (g_swap_above) l->swap=(dm_tc_target){main_x-124*k,main_y-114*k,22*k,0};











    l->joy_radius = 60*k;
    l->joy_x = fmaxf(90*k, g_safe_l + 60*k) + 30*k;
    l->joy_y = l->height - 115*k;
    l->joy_zone_r = l->width*0.40f; l->joy_zone_t = l->height*0.45f;
    l->joy_min_x = l->joy_radius + 8*k; l->joy_max_y = l->height - l->joy_radius - 8*k;
}

int dm_tc_hit(const dm_tc_layout *l, float x, float y) {
    if (!isfinite(x) || !isfinite(y) || x < 0 || y < 0 || x > l->width || y > l->height)
        return DM_TC_NONE;
    for (int i = 0; i < DM_TC_COUNT; ++i) {
        const dm_tc_target *t = &l->target[i];
        float dx = x-t->x, dy = y-t->y;
        if (t->r > 0 && dx*dx+dy*dy <= t->r*t->r) return i;
    }
    float sx = x-l->swap.x, sy = y-l->swap.y;
    if (l->swap.r > 0 && sx*sx+sy*sy <= l->swap.r*l->swap.r) return DM_TC_SWAP;
    return DM_TC_NONE;
}

static int skill_target(int target) {
    return target >= DM_TC_MAIN && target <= DM_TC_SKILL4;
}


static int aim_dir(const dm_tc_state *s, const dm_tc_finger *f, float *ux, float *uy) {
    float dx = f->x-f->aim_origin_x, dy = f->y-f->aim_origin_y, d = hypotf(dx, dy);
    if (!(d > 10 * scale_for(&s->layout))) return 0;
    *ux = dx/d; *uy = dy/d; return 1;
}

static int inside_map_cancel(const dm_tc_state *s, const dm_tc_finger *f) {
    const dm_tc_target *map = &s->layout.target[DM_TC_MAP];
    return map->r > 0 && fabsf(f->x-map->x) <= map->r && fabsf(f->y-map->y) <= map->r;
}

static int g_iso_fix = 1;
void dm_tc_set_iso_fix(int on) { g_iso_fix = !!on; }
static void update_aim(dm_tc_state *s) {
    s->rx = s->ry = 0;
    s->aim_slot = DM_TC_NONE;
    s->map_cancel_active = 0;
    if (s->navigation_mode) return;
    uint64_t latest = 0;
    for (int i = 0; i < DM_TC_MAX_TOUCH; ++i) {
        const dm_tc_finger *f = &s->finger[i];
        if (!f->active || !skill_target(f->target)) continue;
        if (inside_map_cancel(s, f)) s->map_cancel_active = 1;
        if (!f->aim_cancelled && f->aim_move_sequence > latest) {
            latest = f->aim_move_sequence;
            s->aim_slot = i;
        }
    }
    if (s->pulse_button) { s->rx = s->pulse_rx; s->ry = s->pulse_ry; return; }
    float radius = 60 * scale_for(&s->layout);
    if (s->aim_slot == DM_TC_NONE || radius <= 0) return;
    const dm_tc_finger *f = &s->finger[s->aim_slot];
    if (hypotf(f->x-f->aim_origin_x, f->y-f->aim_origin_y) <= 10 * scale_for(&s->layout)) return;
    float dx = (f->x-f->aim_origin_x)/radius;
    float dy = (f->aim_origin_y-f->y)/radius;
    s->rx = (int32_t)lroundf(fmaxf(-1, fminf(1, dx))*DM_STICK_MAX);
    s->ry = (int32_t)lroundf(fmaxf(-1, fminf(1, dy))*DM_STICK_MAX);
}

static void update(dm_tc_state *s) {
    s->buttons = 0;
    for (int i = 0; i < DM_TC_MAX_TOUCH; ++i) {
        dm_tc_finger *f = &s->finger[i];
        if (f->active && !f->aim_cancelled && f->target >= 0 && f->target < DM_TC_COUNT) {
            uint64_t b = f->target==DM_TC_PORTAL && s->portal_pending ? DM_BTN_ZL : s->layout.target[f->target].button;








            if (!s->navigation_mode && f->target >= DM_TC_SKILL1 && f->target <= DM_TC_SKILL4) {
                float ux, uy;
                if (!f->key_down && !aim_dir(s, f, &ux, &uy) && s->last_time_s - f->start_time_s < DM_TC_AIM_SETTLE_S) b = 0;
                else if (!f->key_down) { f->key_down = 1; f->key_down_s = s->last_time_s; }
            }





            if (!s->navigation_mode && f->target >= DM_TC_BELT1 && f->target <= DM_TC_BELT4) b = 0;
            s->buttons |= b;
        }
    }
    s->buttons |= s->pulse_button;

    if (s->navigation_mode && s->pointer_merc && s->pointer_slot >= 0 && s->pointer_slot < DM_TC_MAX_TOUCH) {
        const dm_tc_finger *pf = &s->finger[s->pointer_slot];
        if (pf->active && pf->pointer_long && !pf->ui_drag) s->buttons |= DM_BTN_X;
    }






    if (s->skill_set && !s->navigation_mode) {





        int use = s->defer_button != 0 && !s->pulse_button;
        for (int t = DM_TC_MAIN; t <= DM_TC_SKILL4 && !use; ++t) if (s->pulse_button && s->pulse_button == s->layout.target[t].button) use = 1;








        int belt_pulse = s->belt_gap_cell != 0;
        for (int t = DM_TC_BELT1; t <= DM_TC_BELT4; ++t) if (s->pulse_button && s->pulse_button == s->layout.target[t].button) belt_pulse = 1;
        if (belt_pulse) s->belt_pulse_s = s->last_time_s;
        for (int i = 0; i < DM_TC_MAX_TOUCH; ++i) {
            const dm_tc_finger *f = &s->finger[i];
            if (!f->active || f->aim_cancelled || f->target < DM_TC_MAIN || f->target > DM_TC_SKILL4) continue;
            if (!belt_pulse) use = 1;
            if (belt_pulse || s->last_time_s - f->start_time_s < DM_TC_ZL_LEAD_S || (s->belt_pulse_s > 0 && s->last_time_s - s->belt_pulse_s < DM_TC_ZL_LEAD_S)) s->buttons &= ~s->layout.target[f->target].button;
        }
        if (use) s->buttons |= DM_BTN_ZL;
    }
    update_aim(s);
    s->lx = s->ly = 0;
    if (s->joy_slot < 0) return;
    dm_tc_finger *f = &s->finger[s->joy_slot];
    float dx = f->x-s->joy_touch_x, dy = s->joy_touch_y-f->y;




    float in_k = fminf(1.0f, scale_for(&s->layout));
    float distance = hypotf(dx, dy), radius = 60 * in_k;
    float dead = 6 * in_k;
    if (distance <= dead || radius <= dead) return;
    float strength = fminf(1, (distance-dead)/(radius-dead));






    float ex = dx/distance, ey = dy/distance;
    if (g_iso_fix) { float n = hypotf(ex, 2*ey); ex /= n; ey = 2*ey/n; }
    s->lx = (int32_t)lroundf(ex * strength * DM_STICK_MAX);
    s->ly = (int32_t)lroundf(ey * strength * DM_STICK_MAX);
}

static int pointer_finger(const dm_tc_state *s, const dm_tc_finger *f) {
    return s->navigation_mode && f->active && s->pointer_slot >= 0 &&
        &s->finger[s->pointer_slot] == f;
}

static void update_pointer(dm_tc_state *s) {
    if (!s->pointer_valid || s->pointer_slot < 0 || s->pointer_slot >= DM_TC_MAX_TOUCH) return;
    dm_tc_finger *f = &s->finger[s->pointer_slot];
    if (f->pointer_dragged && s->pointer_pickup_done && !f->shop_drag && !f->locked_drag) {
        s->pointer_x = f->x;
        s->pointer_y = f->y - DM_TC_POINTER_LIFT_PT * scale_for(&s->layout);
    }
}





static void pointer_pulse_btn(dm_tc_state *s, float x, float y, int kind, uint64_t button, double hold_s) {
    if (!s->pulse_button) {
        s->pointer_x = x; s->pointer_y = y;
        s->pulse_button = button;
        s->pulse_hold_s = hold_s;
        s->pulse_rx = s->pulse_ry = 0;
        s->pointer_pulse_active = kind;
        ++s->pulse_epoch;
    } else {
        s->pointer_pulse_pending = kind;
        s->pointer_pending_x = x;
        s->pointer_pending_y = y;
        s->pointer_pending_button = button; s->pointer_pending_hold = hold_s;
    }
}
static void pointer_pulse_start(dm_tc_state *s, float x, float y, int kind) { pointer_pulse_btn(s, x, y, kind, DM_BTN_A, 0); }
void dm_tc_pointer_release(dm_tc_state *s, int kind) { s->pointer_release_item = kind == 1 || kind == 3 || kind == 4 || kind == 8 ? kind : 0; }
void dm_tc_pointer_release_at(dm_tc_state *s, float x, float y) { s->pointer_release_item = 5; s->release_x = x; s->release_y = y; }
void dm_tc_set_holding(dm_tc_state *s, int holding) { s->holding = !!holding; }
void dm_tc_pointer_item(dm_tc_state *s, int kind) {
    if (kind == 7) { s->pointer_shop = 1; kind = 1; }
    s->pointer_lock = kind == 10; if (kind == 10) kind = 1;

    s->pointer_merc = kind == 12; if (kind == 12) kind = 0;
    s->pointer_stack = kind == 8; if (kind == 8) kind = 1;
    s->pointer_item = (kind >= 1 && kind <= 3) || kind == 6 || kind == 11 ? kind : 0;
    if (s->mouse_click_mode && s->pointer_slot >= 0 && s->pointer_slot < DM_TC_MAX_TOUCH && s->finger[s->pointer_slot].active && !s->finger[s->pointer_slot].pointer_dragged) {
        if (kind == 1) { if (!s->pulse_button) { s->pointer_valid = 1; s->pointer_x = s->finger[s->pointer_slot].aim_origin_x; s->pointer_y = s->finger[s->pointer_slot].aim_origin_y; } }
        else s->pointer_valid = 0;
    }
}
static _Atomic int g_dead_modal;
static _Atomic int g_vendor_open;
static _Atomic int g_close_stop;
static _Atomic int g_tap_box;
void dm_tc_set_tap_box(int on) { atomic_store(&g_tap_box, on != 0); }
static _Atomic unsigned g_cur_stack;
void dm_tc_set_cur_stack(unsigned id) { atomic_store(&g_cur_stack, id); }
static _Atomic unsigned g_hover_unid;
void dm_tc_set_hover_unid(unsigned id) { atomic_store(&g_hover_unid, id); }
static _Atomic int g_imbue_open;
static _Atomic int g_cube_open;
static _Atomic int g_vault_open;
static _Atomic int g_bank_open;
int dm_tc_panel_mode(int nav, const dm_tc_scene *scene, int blocked) {
    return nav ? (dm_tc_scene_dead() ? 2 : dm_tc_scene_dialog_only(scene) && blocked <= 0 ? 3 : 1) : 0;
}
void dm_tc_set_panel_state(int bits) { atomic_store(&g_bank_open, (bits & 1) != 0); atomic_store(&g_vendor_open, (bits & 2) != 0); atomic_store(&g_dead_modal, (bits & 4) != 0); atomic_store(&g_imbue_open, (bits & 16) != 0); atomic_store(&g_cube_open, (bits & 32) != 0); atomic_store(&g_vault_open, (bits & 64) != 0); }

static void pointer_drag_begin(dm_tc_state *s, dm_tc_finger *f) {
    f->pointer_dragged = 1;
    if (s->pointer_lock) { f->locked_drag = 1; return; }
    if (s->pointer_shop || s->pointer_stack) { f->shop_drag = 1; s->pointer_x = 2; s->pointer_y = 2; update(s); return; }
    s->pointer_pickup_done = 0;
    pointer_pulse_start(s, f->aim_origin_x, f->aim_origin_y, 1);
}

void dm_tc_cancel_all(dm_tc_state *s) {
    ++s->gesture_epoch; s->portal_pending=s->portal_phase=0; s->diff_open=0;
    memset(s->finger, 0, sizeof(s->finger));
    for (int i = 0; i < DM_TC_MAX_TOUCH; ++i) s->finger[i].target = DM_TC_NONE;
    s->joy_slot = DM_TC_NONE;
    s->joy_origin_x = s->joy_origin_y = 0;
    s->buttons = 0;
    s->lx = s->ly = s->rx = s->ry = 0;
    s->aim_slot = DM_TC_NONE;
    s->aim_move_sequence = 0;
    s->map_cancel_active = 0;
    s->pulse_button = 0; s->pulse_rx = s->pulse_ry = 0;
    s->belt_q = 0; s->belt_gap_cell = 0;
    s->defer_button = 0;
    s->pointer_slot = DM_TC_NONE;
    s->pointer_valid = 0; s->pointer_x = s->pointer_y = 0;
    s->pointer_pickup_done = 0;
    s->pointer_pulse_active = s->pointer_pulse_pending = 0;
    s->pointer_pending_x = s->pointer_pending_y = 0;
}

int dm_tc_pulse_end(dm_tc_state *s, uint64_t epoch) {
    if (!s->pulse_button || s->pulse_epoch != epoch) return 0;
    int ended_pointer = s->pointer_pulse_active;
    if (s->defer_button && s->defer_at_s < s->last_time_s + DM_TC_ZL_LEAD_S) s->defer_at_s = s->last_time_s + DM_TC_ZL_LEAD_S;
    s->pulse_button = 0; s->pulse_rx = s->pulse_ry = 0; s->pulse_hold_s = 0;
    s->pointer_pulse_active = 0;
    if (ended_pointer == 1) {
        s->pointer_pickup_done = 1;
        update_pointer(s);
    }
    update(s);
    return 1;
}

int dm_tc_pulse_pending(const dm_tc_state *s) { return !s->pulse_button && s->pointer_pulse_pending; }

int dm_tc_pulse_resume(dm_tc_state *s) {
    if (s->pulse_button || !s->pointer_pulse_pending) return 0;
    int kind = s->pointer_pulse_pending;
    s->pointer_pulse_pending = 0;
    pointer_pulse_btn(s, s->pointer_pending_x, s->pointer_pending_y, kind, s->pointer_pending_button ? s->pointer_pending_button : DM_BTN_A, s->pointer_pending_hold);
    s->pointer_pending_button = 0; s->pointer_pending_hold = 0;
    update(s);
    return 1;
}

void dm_tc_init(dm_tc_state *s, float width, float height) {
    memset(s, 0, sizeof(*s));
    s->pointer_release_item = 1;
    dm_tc_layout_make(&s->layout, width, height);
    s->enabled_targets = (1u << DM_TC_COUNT) - 1;
    dm_tc_cancel_all(s);
}

void dm_tc_resize(dm_tc_state *s, float width, float height) {
    uint64_t portal = s->layout.target[DM_TC_PORTAL].button;
    dm_tc_cancel_all(s);
    dm_tc_layout_make(&s->layout, width, height);
    s->layout.target[DM_TC_PORTAL].button = portal;
}

void dm_tc_enable_targets(dm_tc_state *s, uint32_t mask) {
    mask &= (1u << DM_TC_COUNT) - 1;
    if (mask == s->enabled_targets) return;
    dm_tc_cancel_all(s);
    s->enabled_targets = mask;
}

void dm_tc_set_joy_block(dm_tc_state *s, int block) { s->joy_block = block != 0; }
void dm_tc_set_navigation_mode(dm_tc_state *s, int navigation_mode) {
    navigation_mode = !!navigation_mode;
    if (navigation_mode == s->navigation_mode) return;
    dm_tc_cancel_all(s);
    s->navigation_mode = navigation_mode;
}


void dm_tc_cancel_finger(dm_tc_state *s, int slot) {
    if (slot < 0 || slot >= DM_TC_MAX_TOUCH) return;
    ++s->gesture_epoch;
    if (s->finger[slot].active && s->finger[slot].target == DM_TC_PORTAL) { dm_tc_cancel_portal(s); }
    if (slot == s->joy_slot && s->finger[slot].active) { s->joy_lost_s = s->last_time_s; s->joy_lost_x = s->finger[slot].x; s->joy_lost_y = s->finger[slot].y; s->joy_lost_ox = s->joy_origin_x; s->joy_lost_oy = s->joy_origin_y; s->joy_lost_tx = s->joy_touch_x; s->joy_lost_ty = s->joy_touch_y; }
    s->finger[slot] = (dm_tc_finger){.target=DM_TC_NONE};
    if (slot == s->joy_slot) { s->joy_slot = DM_TC_NONE; s->joy_origin_x = s->joy_origin_y = 0; }
    if (slot == s->aim_slot) s->aim_slot = DM_TC_NONE;
    if (slot == s->pointer_slot) { s->pointer_item = 0; s->pointer_shop = 0; s->pointer_stack = 0; s->pointer_lock = 0; s->pointer_merc = 0; s->pointer_slot = DM_TC_NONE; s->pointer_valid = 0; }
    update(s);
}
void dm_tc_cancel_portal(dm_tc_state *s) {
    ++s->gesture_epoch; s->portal_pending=s->portal_phase=0;
    for (int i=0;i<DM_TC_MAX_TOUCH;++i)
        if(s->finger[i].active && s->finger[i].target==DM_TC_PORTAL)
            s->finger[i]=(dm_tc_finger){.target=DM_TC_NONE};
    update(s);
}
void dm_tc_set_portal_binding(dm_tc_state *s,uint64_t mask,uint64_t identity,uint64_t scene) {
    if(s->layout.target[DM_TC_PORTAL].button==mask && s->portal_binding_identity==identity && s->portal_scene_epoch==scene)return;
    dm_tc_cancel_portal(s);
    s->portal_binding_identity=identity;s->portal_scene_epoch=scene;
    s->layout.target[DM_TC_PORTAL].button=mask;update(s);
}
void dm_tc_set_portal_button(dm_tc_state *s,uint64_t mask) {
    dm_tc_set_portal_binding(s,mask,s->portal_binding_identity,s->portal_scene_epoch);
}
int dm_tc_portal_ack(dm_tc_state *s,uint64_t epoch,uint64_t identity,uint64_t scene) {
    if(!s->portal_pending || s->gesture_epoch!=epoch || s->portal_binding_identity!=identity || s->portal_scene_epoch!=scene)return 0;
    int active=0;for(int i=0;i<DM_TC_MAX_TOUCH;i++)active|=s->finger[i].active && s->finger[i].target==DM_TC_PORTAL;
    if(!active)return 0;
    s->portal_pending=0;s->portal_phase=2;update(s);return 1;
}


int dm_tc_belt_outside(const dm_tc_state *s, float x, float y) {
    if (s->navigation_mode || !s->belt_open || s->belt_rows <= 1) return 0;
    int hit = dm_tc_hit(&s->layout, x, y); if (hit >= DM_TC_BELT1 && hit <= DM_TC_BELT4) return 0;
    float u = s->layout.height / 1320.0f;
    for (int r = 1; r < s->belt_rows && r < 4; ++r) for (int c = 0; c < 4; ++c) {
        const dm_tc_target *t = &s->layout.target[DM_TC_BELT1+c];
        if (fabsf(x - t->x) <= 60*u && fabsf(y - (t->y - (165.0f + 162.0f*(r-1))*u)) <= 53.75f*u) return 0;
    }
    return 1;
}




static void belt_fire(dm_tc_state *s, int cell) {
    if (cell >= 0x100) { s->belt_request = cell; return; }
    uint64_t btn = s->layout.target[DM_TC_BELT1 + (cell & 0xf)].button; if (!btn) return;
    if (s->pulse_button || s->defer_button || s->belt_gap_cell) { s->belt_q = cell; return; }
    if (s->skill_set && !s->navigation_mode) for (int i = 0; i < DM_TC_MAX_TOUCH; ++i) {
        const dm_tc_finger *f = &s->finger[i]; if (!f->active || f->aim_cancelled || f->target < DM_TC_MAIN || f->target > DM_TC_SKILL4) continue;
        s->belt_gap_cell = 0x1000 | cell; s->belt_gap_at = s->last_time_s + DM_TC_ZL_LEAD_S; update(s); return; }
    s->pulse_rx = s->pulse_ry = 0; s->pulse_button = btn; ++s->pulse_epoch;
    update(s);
}
void dm_tc_begin_at(dm_tc_state *s, int slot, float x, float y, double now_s) {
    if (slot < 0 || slot >= DM_TC_MAX_TOUCH || s->finger[slot].active || !isfinite(x) || !isfinite(y)) return;
    s->last_time_s = now_s;
    dm_tc_finger *f = &s->finger[slot];
    if (!s->navigation_mode) {
        const dm_tc_target *d = &s->layout.diff;
        int on_diff = d->r > 0 && hypotf(x-d->x, y-d->y) <= d->r, pick = -1;
        for (int i = 0; s->diff_open && i < 8; ++i) if (hypotf(x-s->layout.pick[i].x, y-s->layout.pick[i].y) <= s->layout.pick[i].r) pick = i;
        if (on_diff || pick >= 0) {
            if (pick >= 0) s->diff_request = pick + 1;
            s->diff_open = on_diff ? !s->diff_open : 0;
            *f = (dm_tc_finger){.active=1, .target=DM_TC_DIFF, .x=x, .y=y}; update(s); return;
        }
        s->diff_open = 0;
    }
    int hit = dm_tc_hit(&s->layout, x, y);
    if (!s->navigation_mode && s->belt_open && s->belt_rows > 1) {
        float u = s->layout.height / 1320.0f;
        for (int r = 1; r < s->belt_rows && r < 4; ++r) for (int c = 0; c < 4; ++c) {
            const dm_tc_target *t = &s->layout.target[DM_TC_BELT1+c];
            if (fabsf(x - t->x) <= 60*u && fabsf(y - (t->y - (165.0f + 162.0f*(r-1))*u)) <= 53.75f*u) {
                belt_fire(s, 0x100 | r << 4 | c);
                *f = (dm_tc_finger){.active=1, .target=DM_TC_DIFF, .x=x, .y=y}; update(s); return;
            }
        }
        if (hit < DM_TC_BELT1 || hit > DM_TC_BELT4) {
            s->belt_request = 2; *f = (dm_tc_finger){.active=1, .target=DM_TC_DIFF, .x=x, .y=y}; update(s); return;
        }
    }
    if (hit == DM_TC_SWAP) {
        if (s->navigation_mode) hit = DM_TC_NONE;
        else { *f = (dm_tc_finger){.active=1, .target=DM_TC_SWAP, .x=x, .y=y}; update(s); return; }
    }
    int enabled = hit >= 0 && (s->enabled_targets & (1u << hit));




    if (s->navigation_mode && hit >= 0 && !enabled) hit = DM_TC_NONE;
    if(enabled && hit==DM_TC_PORTAL && s->layout.target[hit].button && s->portal_phase==0) {
        ++s->gesture_epoch;
        uint64_t mask=s->layout.target[hit].button;
        s->portal_pending=(mask&DM_BTN_ZL) && (mask&~DM_BTN_ZL);
        s->portal_phase=s->portal_pending?1:2;
    }
    *f = (dm_tc_finger){.active=1, .target=enabled ? hit : DM_TC_NONE, .x=x, .y=y,
                           .aim_origin_x=x, .aim_origin_y=y,
                           .start_time_s=now_s};

    if (s->navigation_mode && hit == DM_TC_NONE && s->pointer_slot == DM_TC_NONE) {
        s->pointer_slot = slot;
        s->pointer_item = 0; s->pointer_merc = 0;




        if (!s->mouse_click_mode) { s->pointer_valid = 1; s->pointer_x = x; s->pointer_y = y; }
    } else if (!s->navigation_mode && hit == DM_TC_NONE && g_edge_b > 0 && x >= 0 && x < s->layout.width*0.43f && y > s->layout.height - g_edge_b) {
        f->target = DM_TC_EDGE;
    } else if (!s->navigation_mode && !s->joy_block && hit == DM_TC_NONE && s->joy_slot == DM_TC_NONE && x >= 0 && y <= s->layout.height &&
        ((x < s->layout.joy_zone_r && y > s->layout.joy_zone_t) ||
         (x < s->layout.width*0.43f && y > s->layout.height*0.25f && s->joy_lost_s > 0 && now_s - s->joy_lost_s <= DM_TC_JOY_RESUME_S && hypotf(x - s->joy_lost_x, y - s->joy_lost_y) <= DM_TC_JOY_GRAB_K * s->layout.joy_radius))) {








        int resume = s->joy_lost_s > 0 && now_s - s->joy_lost_s <= DM_TC_JOY_RESUME_S && hypotf(x - s->joy_lost_x, y - s->joy_lost_y) <= DM_TC_JOY_GRAB_K * s->layout.joy_radius;
        s->joy_lost_s = 0;
        f->target = DM_TC_JOYSTICK;
        s->joy_slot = slot;

        s->joy_origin_x = resume ? s->joy_lost_ox : fmaxf(x, s->layout.joy_min_x);
        s->joy_origin_y = resume ? s->joy_lost_oy : fminf(y, s->layout.joy_max_y);
        s->joy_touch_x = resume ? s->joy_lost_tx : x; s->joy_touch_y = resume ? s->joy_lost_ty : y;
    }
    update(s);
}

void dm_tc_begin_stick_at(dm_tc_state *s, int slot, float x, float y, double now_s) {
    if (slot < 0 || slot >= DM_TC_MAX_TOUCH || s->finger[slot].active || !isfinite(x) || !isfinite(y)) return;
    s->last_time_s = now_s;

    if (s->navigation_mode || s->joy_block) { s->finger[slot] = (dm_tc_finger){.active=1, .target=DM_TC_DIFF, .x=x, .y=y, .no_tap=1}; update(s); return; }
    if (s->joy_slot != DM_TC_NONE) {
        dm_tc_begin_at(s, slot, x, y, now_s);
        if (s->finger[slot].active) s->finger[slot].no_tap = 1;
        return;
    }
    s->joy_lost_s = 0;

    s->finger[slot] = (dm_tc_finger){.active=1, .target=DM_TC_JOYSTICK, .x=x, .y=y, .aim_origin_x=x, .aim_origin_y=y, .start_time_s=now_s, .no_tap=1};
    s->joy_slot = slot;
    s->joy_origin_x = fmaxf(x, s->layout.joy_min_x);
    s->joy_origin_y = fminf(y, s->layout.joy_max_y);
    s->joy_touch_x = x; s->joy_touch_y = y;
    update(s);
}

void dm_tc_move_at(dm_tc_state *s, int slot, float x, float y, double now_s) {
    if (slot < 0 || slot >= DM_TC_MAX_TOUCH || !s->finger[slot].active || !isfinite(x) || !isfinite(y)) return;
    s->last_time_s = now_s;
    dm_tc_finger *f = &s->finger[slot];
    f->x = x;
    f->y = y;
    if (pointer_finger(s, f)) {
        float distance = hypotf(f->x-f->aim_origin_x, f->y-f->aim_origin_y);
        if (!f->pointer_dragged && distance > DM_TC_POINTER_MOVE_PT * scale_for(&s->layout) && (!s->mouse_click_mode || s->pointer_item == 1))
            pointer_drag_begin(s, f);

        else if (s->mouse_click_mode && s->pointer_item != 1 && s->pointer_item != 3 && s->pointer_item != 6 && !f->ui_drag && !f->pointer_dragged && distance > DM_TC_POINTER_MOVE_PT * scale_for(&s->layout))
            f->ui_drag = fabsf(f->x - f->aim_origin_x) > fabsf(f->y - f->aim_origin_y) ? 2 : 1;
        update_pointer(s);
    } else if (!s->navigation_mode && f->target == DM_TC_NONE && s->cam_on > 0 && f->aim_origin_x >= s->layout.width * 0.5f) {







        float ax = fabsf(x - f->aim_origin_x), ay = fabsf(y - f->aim_origin_y), th = DM_TC_POINTER_MOVE_PT * scale_for(&s->layout);

        const int yaw = s->cam_on == 2 || (s->cam_on == 1 && s->cam_orbit);
        if (!f->cam_drag && (ay > th || (yaw && ax > th)) && (yaw || ay > ax)) { f->cam_drag = 1; f->cam_x = f->aim_origin_x; f->cam_y = f->aim_origin_y; }
        if (f->cam_drag && s->layout.height > 0 && s->layout.width > 0) {



            if (!(s->cam_on == 1 && s->cam_orbit)) s->cam_pitch += (f->cam_y - y) / s->layout.height * 1.5707963f;
            f->cam_y = y;
            if (yaw) s->cam_yaw += (x - f->cam_x) / s->layout.width * 3.1415927f;
            f->cam_x = x; }
    } else if (f->target==DM_TC_SWAP) {
        if (dm_tc_hit(&s->layout,x,y)!=DM_TC_SWAP) f->aim_cancelled=1;
    } else if (!s->navigation_mode && skill_target(f->target)) {
        if (inside_map_cancel(s, f)) f->aim_cancelled = 1;
        if (!f->aim_cancelled) f->aim_move_sequence = ++s->aim_move_sequence;
    }
    update(s);
}

void dm_tc_end_at(dm_tc_state *s, int slot, double now_s) {
    if (slot < 0 || slot >= DM_TC_MAX_TOUCH) return;
    s->last_time_s = now_s;
    if(s->finger[slot].active && s->finger[slot].target==DM_TC_PORTAL) {
        s->finger[slot]=(dm_tc_finger){.target=DM_TC_NONE};
        for(int i=0;i<DM_TC_MAX_TOUCH;i++)if(s->finger[i].active && s->finger[i].target==DM_TC_PORTAL) {update(s);return;}
        dm_tc_cancel_portal(s);return;
    }
    dm_tc_finger done = s->finger[slot];
    if (done.target==DM_TC_SWAP) {
        s->finger[slot]=(dm_tc_finger){.target=DM_TC_NONE};
        if (!s->navigation_mode && !done.aim_cancelled && dm_tc_hit(&s->layout,done.x,done.y)==DM_TC_SWAP)
            s->skill_set=!s->skill_set;
        update(s);return;
    }
    if (s->navigation_mode && slot == s->pointer_slot) {
        float distance = hypotf(done.x-done.aim_origin_x, done.y-done.aim_origin_y);
        double elapsed = now_s - done.start_time_s;

        if (done.pointer_long && !done.pointer_dragged && !done.shop_drag && s->pointer_item == 1 && s->pointer_shop) pointer_pulse_btn(s, done.aim_origin_x, done.aim_origin_y, 2, DM_BTN_Y, 1.3);
        else if (done.pointer_long && !done.pointer_dragged && !done.shop_drag && s->pointer_item == 1 && !s->pointer_lock) ++s->long_press_seq;
        if (done.locked_drag) { }
        else if (done.shop_drag) { ++s->shop_drop_seq; }
        else if (done.pointer_dragged) {
            if (s->pointer_release_item == 1) pointer_pulse_start(s, done.x, done.y - DM_TC_POINTER_LIFT_PT * scale_for(&s->layout), 2);
            else if (s->pointer_release_item == 5) pointer_pulse_start(s, s->release_x, s->release_y, 2);
            else if (s->pointer_release_item == 4) pointer_pulse_start(s, done.x, done.y, 2);
            else if (s->pointer_release_item == 3) pointer_pulse_btn(s, done.aim_origin_x, done.aim_origin_y, 2, DM_BTN_Y, 0.15);
            else if (s->pointer_release_item == 8) {
                pointer_pulse_start(s, done.aim_origin_x, done.aim_origin_y, 2); pointer_pulse_btn(s, done.aim_origin_x, done.aim_origin_y, 2, DM_BTN_Y, 1.3); }
            else pointer_pulse_start(s, done.aim_origin_x, done.aim_origin_y, 2);
        } else if (!done.ui_drag && !done.pointer_long && elapsed < DM_TC_POINTER_TAP_S && distance <= DM_TC_POINTER_MOVE_PT * scale_for(&s->layout)) {

            if (s->pointer_lock) { s->pointer_x = done.aim_origin_x; s->pointer_y = done.aim_origin_y; }
            else if (s->holding && s->pointer_item != 0 && !(atomic_load(&g_imbue_open) && s->pointer_item != 1)) { if (s->pointer_item == 1) pointer_pulse_start(s, done.aim_origin_x, done.aim_origin_y, 2);
                              else if (s->pointer_item == 3) pointer_pulse_btn(s, s->pointer_x, s->pointer_y, 2, DM_BTN_Y, 0.15); }



            else if (s->pointer_item == 2) { ++s->tab_tap_seq; }
            else if (s->pointer_item == 6 || s->pointer_item == 11) { }
            else if (s->pointer_item == 3) { if (!atomic_load(&g_dead_modal)) { s->close_left = 3; s->close_next_s = now_s; atomic_store(&g_close_stop, 0); } }
            else if (s->pointer_item) {
                float ox = done.aim_origin_x, oy = done.aim_origin_y;
                int dbl = now_s - s->last_tap_s < DM_TC_DOUBLE_TAP_S && hypotf(ox - s->last_tap_x, oy - s->last_tap_y) <= 2 * DM_TC_POINTER_MOVE_PT * scale_for(&s->layout);
                if (dbl) { if (atomic_load(&g_hover_unid) && !s->pointer_stack && !s->pointer_shop) ++s->ident_seq;
                    else if (s->pointer_stack) ++s->stack_take_seq;
                    else if (atomic_load(&g_imbue_open)) pointer_pulse_btn(s, ox, oy, 2, DM_BTN_Y, 1.3);
                    else if (atomic_load(&g_bank_open) && !atomic_load(&g_tap_box) && !atomic_load(&g_vault_open)) {




                        if (atomic_load(&g_cur_stack)) { pointer_pulse_btn(s, ox, oy, 2, DM_BTN_X, 0.15); s->stash_x_check_s = now_s + 0.4; s->stash_x_x = ox; s->stash_x_y = oy; s->stash_x_id = atomic_load(&g_cur_stack); }
                        else pointer_pulse_btn(s, ox, oy, 2, DM_BTN_Y, DM_TC_HOLD_USE_S); }
                    else if (atomic_load(&g_cube_open) && !atomic_load(&g_bank_open)) pointer_pulse_btn(s, ox, oy, 2, DM_BTN_Y, DM_TC_HOLD_USE_S);
                    else ++s->use_seq;
                    s->last_tap_s = -1e9; }
                else {
                    int again = now_s - s->last_tap_s < 8.0 && hypotf(ox - s->last_tap_x, oy - s->last_tap_y) <= 2 * DM_TC_POINTER_MOVE_PT * scale_for(&s->layout);
                    if (again && !atomic_load(&g_bank_open) && !atomic_load(&g_vendor_open)) pointer_pulse_btn(s, ox, oy, 2, DM_BTN_ZL, 0);
                    else if (!s->pulse_button) { s->pointer_x = ox; s->pointer_y = oy; }
                    ++s->item_tap_seq;                     s->last_tap_s = now_s; s->last_tap_x = ox; s->last_tap_y = oy; }
            } else if (s->tap_button) {
                s->pointer_valid = 1; pointer_pulse_btn(s, s->tap_x, s->tap_y, 2, s->tap_button, 0);
            } else if (s->tap_move) {
                s->pointer_valid = 1; s->pointer_x = s->tap_x; s->pointer_y = s->tap_y; ++s->tap_move_seq;
            } else if (s->mouse_click_mode) {
                s->mouse_click_x = done.aim_origin_x; s->mouse_click_y = done.aim_origin_y; ++s->mouse_click_seq;
            } else pointer_pulse_start(s, done.aim_origin_x, done.aim_origin_y, 2);
        }
        s->pointer_item = 0; s->pointer_release_item = 1; s->pointer_shop = 0; s->pointer_stack = 0; s->pointer_lock = 0; s->pointer_merc = 0;
        s->pointer_slot = DM_TC_NONE;
        s->finger[slot] = (dm_tc_finger){.target=DM_TC_NONE};
        update(s);
        return;
    }






    if (done.active && done.target >= DM_TC_SKILL1 && done.target <= DM_TC_SKILL4 && !done.aim_cancelled && !s->navigation_mode
        && (!done.key_down || now_s - done.key_down_s < DM_TC_KEY_MIN_S || (s->skill_set && now_s - done.start_time_s < DM_TC_ZL_LEAD_S))) {
        float ux = 0, uy = 0; int has = aim_dir(s, &done, &ux, &uy);
        s->pulse_rx = has ? (int32_t)lroundf(ux*DM_STICK_MAX) : 0; s->pulse_ry = has ? (int32_t)lroundf(-uy*DM_STICK_MAX) : 0;
        if (s->skill_set && now_s - done.start_time_s < DM_TC_ZL_LEAD_S) { s->defer_button = s->layout.target[done.target].button; s->defer_at_s = done.start_time_s + DM_TC_ZL_LEAD_S; }
        else { s->pulse_button = s->layout.target[done.target].button; ++s->pulse_epoch; }
    }
    if (done.active && !s->navigation_mode && done.target >= DM_TC_BELT1 && done.target <= DM_TC_BELT4) {

        float th = DM_TC_BELT_SWIPE_PT * scale_for(&s->layout), up = done.aim_origin_y - done.y, dx = done.x - done.aim_origin_x;
        if (s->belt_rows > 1 && up > th) s->belt_request = 1;
        else if (s->belt_rows > 1 && up < -th) s->belt_request = 2;
        else if (fabsf(dx) <= th && fabsf(up) <= th) belt_fire(s, 0x10 | (done.target - DM_TC_BELT1));
    }
    if (done.active && !s->navigation_mode && !done.cam_drag && !done.no_tap && (done.target == DM_TC_NONE || done.target == DM_TC_JOYSTICK) && now_s - done.start_time_s < DM_TC_POINTER_TAP_S &&
        hypotf(done.x - done.aim_origin_x, done.y - done.aim_origin_y) <= DM_TC_POINTER_MOVE_PT * scale_for(&s->layout)) {
        s->world_tap_x = done.aim_origin_x; s->world_tap_y = done.aim_origin_y; ++s->world_tap_seq;
    }
    s->finger[slot] = (dm_tc_finger){.target=DM_TC_NONE};
    if (slot == s->joy_slot) {
        s->joy_slot = DM_TC_NONE;
        s->joy_origin_x = s->joy_origin_y = 0;
    }
    update(s);
}

void dm_tc_begin(dm_tc_state *s, int slot, float x, float y) { dm_tc_begin_at(s, slot, x, y, s->last_time_s); }
void dm_tc_move(dm_tc_state *s, int slot, float x, float y) { dm_tc_move_at(s, slot, x, y, s->last_time_s); }
void dm_tc_end(dm_tc_state *s, int slot) { dm_tc_end_at(s, slot, s->last_time_s); }

void dm_tc_pointer_press(dm_tc_state *s, float x, float y, uint64_t button) {
    if (s->pointer_slot >= 0 && s->pointer_slot < DM_TC_MAX_TOUCH) s->finger[s->pointer_slot].pointer_long = 1;
    s->pointer_valid = 1; pointer_pulse_btn(s, x, y, 2, button, 0); update(s);
}
void dm_tc_tick(dm_tc_state *s, double now_s) {
    s->last_time_s = now_s;

    if (s->belt_gap_cell && now_s >= s->belt_gap_at && !s->pulse_button) { uint64_t btn = s->layout.target[DM_TC_BELT1 + (s->belt_gap_cell & 0xf)].button; s->belt_gap_cell = 0;
        if (btn && !s->navigation_mode) { s->pulse_rx = s->pulse_ry = 0; s->pulse_button = btn; ++s->pulse_epoch; } update(s); }
    if (s->belt_q && !s->pulse_button && !s->defer_button && !s->belt_gap_cell) { int q = s->belt_q; s->belt_q = 0; if (!s->navigation_mode) belt_fire(s, q); }
    if (s->defer_button && !s->pulse_button && now_s >= s->defer_at_s) { s->pulse_button = s->defer_button; s->defer_button = 0; ++s->pulse_epoch; update(s); }
    else if (!s->navigation_mode) {
        for (int i = 0; i < DM_TC_MAX_TOUCH; ++i) if (s->finger[i].active && (s->finger[i].target >= DM_TC_MAIN && s->finger[i].target <= DM_TC_SKILL4)) { update(s); break; }
    }
    if (s->stash_x_check_s > 0 && now_s >= s->stash_x_check_s) {
        s->stash_x_check_s = 0;
        if (atomic_load(&g_bank_open) && atomic_load(&g_cur_stack) == s->stash_x_id && !s->holding && !s->pulse_button) { pointer_pulse_btn(s, s->stash_x_x, s->stash_x_y, 2, DM_BTN_Y, DM_TC_HOLD_USE_S); update(s); }
    }
    if (s->close_left > 0 && atomic_exchange(&g_close_stop, 0)) s->close_left = 0;
    if (s->close_left > 0) {
        if (!s->navigation_mode) s->close_left = 0;
        else if (!s->pulse_button && now_s >= s->close_next_s) { s->pulse_button = DM_BTN_B; s->pulse_hold_s = 0; s->pulse_rx = s->pulse_ry = 0; s->pointer_pulse_active = 2; ++s->pulse_epoch; --s->close_left; s->close_next_s = now_s + 0.3; update(s); }
    }
    if (s->pointer_slot < 0 || s->pointer_slot >= DM_TC_MAX_TOUCH) return;
    dm_tc_finger *f = &s->finger[s->pointer_slot];
    if (!f->active || f->pointer_dragged || f->pointer_long) return;




    if (s->pointer_merc) {
        if (!f->ui_drag && now_s - f->start_time_s >= DM_TC_POINTER_HOLD_S &&
            hypotf(f->x-f->aim_origin_x, f->y-f->aim_origin_y) <= DM_TC_POINTER_MOVE_PT * scale_for(&s->layout)) { f->pointer_long = 1; update(s); }
        return;
    }
    if (s->mouse_click_mode && s->pointer_item != 1) return;
    if (now_s - f->start_time_s >= DM_TC_POINTER_HOLD_S &&
        hypotf(f->x-f->aim_origin_x, f->y-f->aim_origin_y) <= DM_TC_POINTER_MOVE_PT * scale_for(&s->layout)) {
        f->pointer_long = 1;
        s->pointer_x = f->aim_origin_x;
        s->pointer_y = f->aim_origin_y;
        update(s);
    }
}

int dm_tc_navigation(dm_tc_scene *s, const char *cat, const char *act, const char *arg) {
    static const char *names[]={"PlayerInventoryExpansionLayout","SkillsTreePanel","PauseLayout","PauseLayoutGarden","CharacterStatsPanel","QuestLogPanelExpansion","BankExpansionLayout","HoradricCubeLayout","WaypointsPanelExpansion","NpcDialogPanel","NpcTradePanel","ButtonBindingModal","SkillSelect","YouDiedModal"};
    if(!strcmp(cat,"FrontEndNavigation")&&!strcmp(act,"ToFrontEnd")) { s->game=0;s->panels=0; }
    if(!strcmp(cat,"PanelManager")) {
        if(!strcmp(arg,"LoadScreenPanel")&&!strcmp(act,"OpenPanel"))s->game=0;
        if(!strcmp(arg,"LoadScreenPanel")&&!strcmp(act,"UnloadPanel"))s->game=1;
        if(!strcmp(act,"UnloadAll"))s->panels=0;
        { size_t al=strlen(arg); if((!strcmp(act,"OpenPanel")||!strcmp(act,"OpenExclusivePanel"))&&al>5&&!strcmp(arg+al-5,"Modal"))atomic_store(&g_close_stop,1); }
        for(unsigned i=0;i<sizeof(names)/sizeof(*names);i++)if(!strcmp(arg,names[i])) {
            if(!strcmp(act,"OpenPanel")||!strcmp(act,"OpenExclusivePanel"))s->panels|=1ull<<i;
            if(!strcmp(act,"ClosePanel")||!strcmp(act,"UnloadPanel"))s->panels&=~(1ull<<i);
        }
    }
    return !s->game||s->panels!=0;
}

int dm_tc_scene_dead(void) { return atomic_load(&g_dead_modal); }
int dm_tc_scene_dialog_only(const dm_tc_scene *s) { return s->game && s->panels == (1ull << 9); }

