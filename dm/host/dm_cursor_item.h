



#pragma once
#include <stdint.h>
enum { DM_ITEM_AT_NONE, DM_ITEM_AT_INVENTORY, DM_ITEM_AT_BELT, DM_ITEM_AT_EQUIP, DM_ITEM_AT_STASH, DM_ITEM_AT_CUBE, DM_ITEM_AT_SHOP, DM_ITEM_AT_TRADE };
typedef struct {
    uint32_t id, cls;
    char code[4];
    int container;
    int potion, scroll, book;
    int beltable;
    int belt_room;
    int equippable;
    int stack;
    int unid;
    int gx, gy, gw, gh;
} dm_cursor_item_info;
int dm_cursor_item(dm_cursor_item_info *out);
int dm_held_size(int *w, int *h);
int dm_held_pending(void);
int dm_vendor_repair_mode(void);
int dm_item_targeting(void);
