

#pragma once
#include <stdint.h>
typedef struct { int tier; float progress; int secs_left, done, guardian, inc, next, overtime, goal, gdead, gtotal, wave, nwave; } dm_rift_hud_t;
void dm_rift_tick(uint64_t G, uint64_t player, uint8_t *main_base);
void dm_rift_c2s(uint64_t G, const uint8_t *pkt, uint32_t len);
void dm_rift_client_pump(uint8_t *main_base);
int dm_rift_hud(dm_rift_hud_t *out);
int dm_rift_active(void);
void dm_rift_gamble_vals(int best, uint32_t *uniq, uint32_t *set);
typedef struct { int menu, best, unlock, def; unsigned ms; int clvl, tier, done, secs_left; float progress; int inc, next, overtime; } dm_rift_menu_t;
int dm_rift_cleared_any(void);
int dm_rift_merc_unlocked(void);
void dm_rift_merc_locked_hint(void);
int dm_rift_tier_level(int tier, int clvl);
int dm_rift_tier_hp_x10(int tier, int clvl);
int dm_rift_menu(dm_rift_menu_t *out);
void dm_rift_menu_close(void);
void dm_rift_menu_start(int tier);
void dm_rift_menu_continue(void);
void dm_rift_menu_abandon(void);
void dm_rift_menu_next(void);
int dm_rift_menu_pending(void); int dm_rift_menu_seq(void); int dm_rift_swept(uint32_t id);
void dm_rift_menu_peek(dm_rift_menu_t *out);
void dm_rift_ui_pump(void);
void dm_rift_ui_on_uimsg(const char *cat, const char *act, const char *arg);
typedef struct { int best, clears, deaths, tier, unlock; unsigned ms; int fast[101]; } dm_rift_stats_t;
int dm_rift_stats(dm_rift_stats_t *out);
