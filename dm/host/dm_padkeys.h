
#pragma once
#include <stdint.h>
typedef struct { uint64_t prev, eat; } dm_padkeys;



static inline uint64_t dm_padkeys_step(dm_padkeys *k, uint64_t bt, uint64_t own, uint64_t *pressed) {
    uint64_t down = bt & ~k->prev; k->prev = bt;
    *pressed = down & own;
    k->eat = (k->eat | (bt & own)) & bt;
    return bt & ~k->eat;
}
