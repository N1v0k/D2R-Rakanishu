#include <stdint.h>


#pragma once
void dm_elysium_arm(int on);
int dm_elysium_active(void);
void dm_elysium_pump(void);
void dm_elysium_stash_step(uint64_t game, uint64_t player, uint8_t *main_base);
