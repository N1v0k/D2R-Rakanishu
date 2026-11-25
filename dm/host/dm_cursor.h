#pragma once
#include <stdint.h>













uint32_t dm_ui_gen(void);
enum { DM_UIGEN_OPMODE = 1, DM_UIGEN_PERFMODE, DM_UIGEN_LANG, DM_UIGEN_PANEL_GONE, DM_UIGEN_ROOT, DM_UIGEN_RESIZE };
void dm_ui_gen_bump(int why);
int dm_ui_unstable(void);








int dm_ui_reloading(void);
int dm_ui_skill_spend_ok(void);
typedef struct { uint64_t w; uint32_t gen; } dm_wref;
static inline void dm_wref_set(dm_wref *r, uint64_t w) { r->w = w; r->gen = dm_ui_gen(); }
uint64_t dm_wref_get(dm_wref *r);
int dm_ui_gen_revalidate(uint32_t *gen, uint64_t anchor);

void dm_cursor_set(int x, int y);




void dm_cursor_set_screen(float x, float y, float screen_w, float screen_h);
void dm_cursor_clear(void);
int dm_ui_mouse_hover_now(float nx, float ny);








void dm_cursor_apply(uint64_t *buttons, int32_t *lx, int32_t *ly);
int dm_cursor_pending(void);




void dm_ui_show_ours(uint64_t w, int on);
void dm_ui_move_ours(uint64_t w, int x);
void dm_ui_send_now(const char *cat, const char *act, const char *arg);
void dm_ui_send_queue(const char *spec);
int dm_ui_fe_drag_ok(void);
void dm_ui_touch(int phase, float nx, float ny);
void dm_ui_mouse_pump(void);


enum { DM_UP_HOVER = 0, DM_UP_MOVE = 1, DM_UP_END = 2, DM_UP_LDOWN = 3, DM_UP_LUP = 4, DM_UP_RDOWN = 5, DM_UP_RUP = 6, DM_UP_SCROLL = 7, DM_UP_KEYDOWN = 8, DM_UP_KEYUP = 9 };
unsigned dm_ui_item_query(float nx, float ny);
int dm_ui_item_result(unsigned seq);
unsigned dm_ui_drop_query(float nx, float ny_lifted, float ny_finger);
int dm_ui_drop_at_now(float nx, float ny_lifted, float ny_finger);
int dm_ui_origin_point(float *nx, float *ny);
int dm_ui_drop_last(void);
int dm_ui_drop_point(float *nx, float *ny);
int dm_ui_drop_bag(void);
int dm_fe_bliss(void);
int dm_ui_shelf_grid(float g[4]);
int dm_ui_item_rect(unsigned seq, float r[4]);
int dm_ui_stack_left(void);
void dm_ui_world_tap(float ux, float uy, float wx, float wy);
void dm_ui_stack_take_at(float nx, float ny);
void dm_ui_stack_cube_at(float nx, float ny);
void dm_ui_shop_buy_at(float nx, float ny);
void dm_ui_item_tap(void);
int dm_ui_targeting(void);
int dm_ui_panel_resident(const char *name);
enum { DM_PS_BANK = 1, DM_PS_VENDOR = 2, DM_PS_DEAD = 4, DM_PS_TEXT = 8, DM_PS_IMBUE = 16, DM_PS_CUBE = 32, DM_PS_VAULT = 64 };
int dm_ui_panel_state(void);
void dm_ui_lifecycle(int background);
int dm_ui_panel_shown(const char *name);
int dm_ui_tap_box(void);
uint32_t dm_ui_cur_stack_id(void);
int dm_ui_free_cursor(void);
void dm_ui_use_item(void);
void dm_ui_long_press(void);
void dm_ui_tab_go(void);
int dm_ui_item_at_now(float nx, float ny);
void dm_ui_press_new(void);
void dm_ui_watch(const char *spec);
int dm_ui_watch_pos(float *nx, float *ny);
uint64_t dm_ui_taps_base(void);
void dm_ui_ptr(int kind, float nx, float ny, int arg);
int dm_ui_touch_active(void);


void dm_ui_on_uimsg(const char *cat, const char *act, const char *arg);
void dm_ui_frontend_pump(void);
uint64_t dm_ui_auto_buttons(void);
void dm_ui_key_pulse(uint64_t mask, int frames);


void dm_ui_drag_begin(int kind, float ox, float oy, float tol_ny);
void dm_ui_drag_move(float x, float y);
void dm_ui_drag_end(void);
void dm_ui_drag_lift(float ny);


int dm_ui_minimap_rect(float r[4]);






void dm_ui_bind_begin(uint64_t key_mask);
int dm_ui_bind_slot(void);
void dm_ui_safe_left(int units);
void dm_ui_safe_right(int units);
uint32_t dm_ui_hit_id(void);
int dm_ui_bind_active(void); uint64_t dm_ui_auto_release(void);                                                                                                                                                              int dm_ui_bind_skill_hit(float *nx, float *ny);
void dm_ui_bind_cancel(void);
void dm_ui_map_full(int on);
int dm_ui_map_is_full(void);


void dm_ui_map_hold(int on);
void dm_ui_text_entered(const char *text);


int dm_lat_on(void);
#define DM_LAT(fmt, ...) do { if (dm_lat_on()) LOG("LAT %llu " fmt, (unsigned long long)(now_ns() / 1000000), ##__VA_ARGS__); } while (0)
void dm_ui_hand_idle(int idle);
uint32_t dm_ui_hover_unid(void);
void dm_ui_identify_hovered(void);
int dm_ui_map_geom(float *cell);
int dm_ui_portrait_bottom(void);
int dm_ui_level_word(char *out, int cap);


int dm_ui_stash_view(int *tab, uint32_t *unit);


int dm_ui_stash_toward(int tab, uint32_t unit);
