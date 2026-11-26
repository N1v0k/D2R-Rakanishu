#pragma once
#include <stdint.h>
#include "game_addrs.h"






typedef struct {
    int valid;
    int level;
    int exp_permille;
    int belt[4];
    int portal;
    int players;
    int hp, hp_max, mp, mp_max;
    int area, x, y;
    int points;
    int holding;
    int quest_unread;
    int dead;
} dm_hud_info;




static inline int dm_hud_player_mode_dead(unsigned mode) { return mode == 0 || mode == 17; }




static inline int dm_hud_area_is_town(int area) { return area == 1 || area == 40 || area == 75 || area == 103 || area == 109; }
static inline int dm_hud_bar_visible(const dm_hud_info *h, int navigation, int blocked) { return h->valid && !h->dead && !navigation && !blocked; }
void dm_hud_get(dm_hud_info *out);
void dm_hud_get_stamped(dm_hud_info *out, uint64_t *pub_ns);
uint32_t dm_hud_held(void);
void dm_hud_set_held(uint32_t id);
void dm_hud_request_players(int n);



int dm_hud_players_wanted(void);



uint64_t dm_hud_players_take(int *n);
void dm_hud_players_done(uint64_t seq);
int dm_hud_requested_players(void);
void dm_hud_publish(const dm_hud_info *in);
