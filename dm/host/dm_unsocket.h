



#pragma once
#include <stdint.h>




#define DM_UNS_PENALTY_INSERT_INDESTRUCTIBLE 1

enum { DM_UNS_ROW = 0x158, DM_UNS_ROW_FLAGS = 0x50, DM_UNS_ROW_INPUTS = 0x14, DM_UNS_INPUT = 8, DM_UNS_INPUTS = 7 };
enum { DM_UNS_OUT_DESTROY = 0x10, DM_UNS_OUT_REMOVE = 0x20 };


typedef struct {
    int items;
    int bases;
    int ral, scroll;
    int base_w, base_h;
    int fillers;
    int fillers_small;
    int cube_w, cube_h;
    int inv_free;
    int job_busy;
} dm_uns_view;

enum { DM_UNS_PASS, DM_UNS_APPLY, DM_UNS_REFUSE_ROOM, DM_UNS_REFUSE_SHAPE, DM_UNS_REFUSE_BUSY, DM_UNS_REFUSE_READ };



int dm_uns_decide(const dm_uns_view *v, int *overflow);


int dm_uns_overflow(int cube_w, int cube_h, int base_w, int base_h, int fillers);



typedef struct { int ethereal, no_durability, indestructible_before, indestructible_after, max_dur; } dm_uns_item;
int dm_uns_penalty(const dm_uns_item *it);


typedef struct {
    int base_found;
    int base_fillers;
    int ral_left, scroll_left;
    int fillers_noted;
    int fillers_home;
} dm_uns_after;
enum { DM_UNS_NOT_RUN, DM_UNS_DONE, DM_UNS_ODD };

int dm_uns_outcome(const dm_uns_after *a);




int dm_uns_row_is_clear(const uint8_t *row, uint16_t hel, uint16_t scroll);
void dm_uns_row_lend(uint8_t *row, uint16_t ral);



enum { DM_UNS_N_UNREAD, DM_UNS_N_REFUSED, DM_UNS_N_ROW, DM_UNS_N_QUEUED, DM_UNS_N_NOT_QUEUED, DM_UNS_N_NOT_RUN, DM_UNS_N_DONE,
       DM_UNS_N_ODD, DM_UNS_N_PENALISED, DM_UNS_N_SPARED };
typedef struct {
    int (*view)(void *ctx, dm_uns_view *v);
    int (*lend)(void *ctx, int spill);
    void (*give_back)(void *ctx);
    uint64_t (*body)(void *ctx);
    void (*after)(void *ctx, dm_uns_after *a, dm_uns_item *it);
    void (*penalty)(void *ctx);
    void (*notice)(void *ctx, int why, int overflow);
    void (*note)(void *ctx, int what, int mode);
} dm_uns_ops;
enum { DM_UNS_QUEUED = 4 };
uint64_t dm_uns_gate(const dm_uns_ops *ops, void *ctx, int mode);
