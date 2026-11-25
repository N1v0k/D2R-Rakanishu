#pragma once


#include <stdint.h>
void dm_bossrush_tick(uint64_t game, uint64_t player, uint8_t *main_base);
void dm_bossrush_c2s(uint64_t game, uint64_t player, const uint8_t *pkt, uint32_t len);
void dm_game_notice(uint8_t *main_base, const char *text);
void dm_game_chat(uint8_t *main_base, const char *text);
int dm_bossrush_stage(void);
int dm_bossrush_context(uint32_t tick);
int dm_bossrush_owns(uint32_t id);
void dm_kills_tick(uint64_t G, uint64_t player, uint8_t *main_base);
void dm_door_label(uint8_t *main_base, uint64_t game, int cls, const char *key, int in_town);
