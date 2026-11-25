

#pragma once
#include <stdint.h>




int dm_rw_find_lvoff(const uint8_t *rows, uint32_t n, uint32_t rowsize);


int dm_rw_need_level(int base_req, const int *rune_req, int n);












int dm_rw_allowed(uint8_t bits, int16_t first, int16_t last, int known, int offline, int ladder);
