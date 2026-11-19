

#pragma once
enum { DM_UISCREEN_OFF, DM_UISCREEN_TOP, DM_UISCREEN_CENTER, DM_UISCREEN_BOTTOM };
void dm_uiscreen_enable(int mode);
void dm_uiscreen_pump(void);
int dm_uiscreen_res_live(void);
int dm_screen_real(int *w, int *h);
void dm_ui_rect(int *x, int *y, int *w, int *h);
float dm_ui_ny(float ny_real);
float dm_ui_ny_back(float ny_ui);
int dm_ui_proj_dy(void);
int dm_uiscreen_ingame(void);
void dm_uiscreen_set_ingame(int on);
void dm_uiscreen_set_panel(int on);
int dm_uiscreen_panel(void);
#include <stdint.h>
uint64_t dm_game_res(int docked);
void dm_uiscreen_res_reread(void);
