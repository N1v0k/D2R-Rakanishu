

#pragma once
#include <malloc.h>
#include <stddef.h>
typedef struct { unsigned blocks_in_use; size_t size_in_use, max_size_in_use, size_allocated; } malloc_statistics_t;
static inline void malloc_zone_statistics(void *zone, malloc_statistics_t *s) {
    (void)zone; struct mallinfo mi = mallinfo();
    s->blocks_in_use = 0; s->size_in_use = mi.uordblks; s->max_size_in_use = 0; s->size_allocated = mi.arena;
}
