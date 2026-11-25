#pragma once
#include <stddef.h>
#include <stdint.h>
#include "dm_touch_game_state.h"
typedef struct {
    uint64_t world_epoch, widget[6], hud_identity;
    uint32_t seen;
    int confirmed;
} dm_tg_hud_cohort;
typedef int (*dm_tg_read_fn)(uint64_t address, void *out, size_t bytes);

int dm_tg_vm_read(uint64_t address, void *out, size_t bytes);
void dm_tg_hud_reset(dm_tg_hud_cohort *cohort, uint64_t world_epoch);

extern int dm_tg_identity_reason; extern uint64_t dm_tg_identity_lr; extern uint32_t dm_tg_identity_slot;
int dm_tg_hud_identity(dm_tg_hud_cohort *cohort, const uint64_t *regs,
                       uint64_t main_base, int game_no_panel, uint64_t world_epoch,
                       dm_tg_read_fn read_fn, uint64_t *caller, uint64_t *hud);
