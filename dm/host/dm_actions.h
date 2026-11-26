



#pragma once
#include <stdint.h>
#include "game_addrs.h"
typedef struct {
    int open;
    int gold, stash_gold;
    int sel_valid;
    uint32_t sel_id; char sel_code[4]; char sel_name[64]; int buy_price;
    int sel_gx, sel_gy, sel_gw, sel_gh;
    int can_buy;
    int held_valid;
    uint32_t held_id; char held_name[64]; int sell_price;
} dm_shop_info;
void dm_shop_get(dm_shop_info *out);
int  dm_shop_open(void);
void dm_shop_buy(void);
void dm_shop_sell_cursor(void);

typedef struct {
    int stat_points, skill_points;
    int tree_open;
    int char_open;
    int sel_skill;
    int sel_can_add;
    int sel_block;
    char sel_name[64];
} dm_points_info;
void dm_points_get(dm_points_info *out);
void dm_stat_add(int stat, int n);
void dm_skill_add_selected(void);
void dm_belt_put(uint32_t item_id);
void dm_item_use(uint32_t item_id);
int  dm_ui_hover_now(int x, int y);
void dm_item_compare(int on);


void dm_belt_expand(int on);
void dm_belt_use(int col, int row);
void dm_belt_use_merc(int col, int row);
void dm_belt_drink(int kind);
int  dm_belt_kinds(void);
int  dm_belt_rows(int *expanded);
void dm_held_equip(int bodyloc);
void dm_inv_sort(int page);





void dm_inv_sort_grid(uint32_t owner_id, uint32_t owner_type, int page);
void dm_name_random(void);
int dm_name_taken(const char *name);
int dm_name_gen(char *out, int cap, int (*taken)(const char *));
void dm_item_equip(uint32_t item_id);


void dm_item_unequip(uint32_t item_id);
void dm_item_identify(uint32_t target_id);


void dm_item_repair(uint32_t item_id);
void dm_belt_fill(void);
void dm_item_target_cancel(void);
void dm_stash_take(int count);


void dm_stash_take_id(uint32_t item_id, int count);
void dm_num_set(int n);
void dm_actions_pump(void);
void dm_held_stow(void);
int dm_held_stow_result(void);
int dm_held_stow_why(void);
int dm_held_stow_tick(void);
void dm_item_identify_free(uint32_t item_id);


uint64_t dm_swapskill_filter(uint64_t buttons);
void dm_swapskill_srv_wset(int w);
int dm_swapskill_slot(int slot, int *charges);
int dm_swapskill_equipped(int32_t item_id);
int dm_swapskill_offhand_gives(int skill);
