



#pragma once
#include <stdint.h>
#include "dm_tipx_core.h"
struct dm_gtab_pkg;

enum { DM_TIPX_CELL = 1, DM_TIPX_SLOT = 2 };
typedef struct {
    uint8_t *b;
    uint64_t widget, item, player;
    int producer, pad;
    uint32_t ident;
    int slot;
    const struct dm_gtab_pkg *pkg;
    const char *orig;
    int max_chars;
} dm_tipx_ctx;
void dm_tipx_obs(uint64_t *regs);
void dm_tipx_tap(void);
int dm_tipx_pad_page(void);
int dm_tipx_pages(void);

int dm_itemcmp_block(const dm_tipx_ctx *c, dm_tipx_line *ln, int max, uint64_t *key);

const char *dm_itemcmp_marks(const dm_tipx_span **span, int *nspan);
