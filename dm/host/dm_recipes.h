
#pragma once
#include <stddef.h>
#include <stdint.h>

enum { DM_RECIPE_INV, DM_RECIPE_CUBE, DM_RECIPE_STASH, DM_RECIPE_OTHER_PAGE, DM_RECIPE_SHARED, DM_RECIPE_STACKS, DM_RECIPE_POUCH, DM_RECIPE_VAULT };
enum { DM_RECIPE_UNSOCKET_READY = 1 };
enum { DM_RECIPE_ETHEREAL = 1, DM_RECIPE_RUNEWORD = 2, DM_RECIPE_UNKNOWN = 4, DM_RECIPE_NO_WORD = 8 };
typedef struct { uint32_t code; uint16_t qty; uint8_t place, quality, sockets, filled, flags, level; uint16_t row; uint8_t x, y; uint32_t id, unit; } dm_recipe_item;
typedef struct { uint32_t gen, n; uint8_t data_version, difficulty, ladder_free, truncated, page, pages, seen, stale; uint8_t near, stash_open, insert, flags; } dm_recipe_head;

void dm_recipes_pump(uint8_t *b, uint64_t pl);
typedef struct { int id, kind, page, x, y; uint32_t unit; int to_page, to_x, to_y, code, quality, back_x, back_y; } dm_recipe_step;
enum { DM_RECIPE_IDLE, DM_RECIPE_DONE, DM_RECIPE_FAILED, DM_RECIPE_HELD, DM_RECIPE_RUNNING };
enum { DM_RECIPE_WHY_NONE, DM_RECIPE_WHY_STALE, DM_RECIPE_WHY_GONE, DM_RECIPE_WHY_FAR, DM_RECIPE_WHY_HAND, DM_RECIPE_WHY_PANEL, DM_RECIPE_WHY_ABORT, DM_RECIPE_WHY_RUN, DM_RECIPE_WHY_LOST, DM_RECIPE_WHY_HELD };
int dm_recipes_insert(const int *plan, int steps, uint32_t cube_id, uint32_t gen);
int dm_recipes_insert_state(void);
void dm_recipes_abort(void);
int dm_recipes_busy(void);
void dm_recipes_text_start(const char *dir);
int dm_recipes_text(int rec, char *out, int bytes);








#define DM_ITEM_TEXT_NONE (-1)
#define DM_ITEM_DESC_LATER (-2)
int dm_item_text(int kind, int row, int grp, char *out, int bytes);
int dm_item_desc(uint8_t *b, int kind, int row, int grp, char *out, size_t n);
int dm_recipes_request_stock(void);
int dm_recipes_stock_copy(void *out, int bytes);
