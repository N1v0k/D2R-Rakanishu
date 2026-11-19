

#pragma once
#include <stddef.h>
#include <stdint.h>

void nvn_dirty_add_range(const void *mem, size_t size);
void nvn_dirty_stat(int *live, int *peak, int *cap);
void nvn_dirty_remove_range(const void *mem);
int nvn_dirty_tracked(const void *addr, size_t len);
void nvn_dirty_protect(const void *addr, size_t len);
uint64_t nvn_dirty_now(void);
int nvn_dirty_since(const void *addr, size_t len, uint64_t stamp);
void nvn_dirty_stats(uint64_t *faults, uint64_t *preps);
void nvn_dirty_mp_stats(uint64_t *calls, uint64_t *pages, uint64_t *ns);
void nvn_dirty_slot_stats(int *live, int *used_slots, uint64_t *adds_total);
