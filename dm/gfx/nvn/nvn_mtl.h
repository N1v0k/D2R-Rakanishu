





#pragma once
#include <stdint.h>
#include <stddef.h>

void mtl_init(void);
void bf_want_note(const char *key);



void mtl_pool_add(const void *mem, uint64_t size);
void mtl_pool_del(const void *mem);
int mtl_addr(uint64_t addr, uint64_t len, void **buf, uint64_t *off);

typedef struct {
    uint32_t pso, cps, shaders, dss;
    uint32_t view_nonnull, view_slots, live_tex, sview_nonnull;
    uint32_t pools;
    uint64_t pool_adds;
    uint32_t c1_bufs; uint64_t c1_alloc, c1_data;
} mtl_res_count;
void mtl_res_counts(uint32_t tick, const char *why);
void mtl_res_snapshot(mtl_res_count *out);



void mtl_sub_begin(void);
void mtl_sub_copy(uint64_t src, uint64_t dst, uint64_t n);
void mtl_sub_event(volatile uint32_t *val, uint32_t mode, uint32_t v);
void mtl_sub_counter(uint64_t addr, uint32_t type);
uint64_t mtl_sub_commit(void);

uint64_t mtl_last_fence(void);
int mtl_wait(uint64_t fence, uint64_t timeout_ns);
void mtl_finish(void);
void mtl_drain(void);
void mtl_xv_window_tex(uint64_t serial);
void mtl_ui_ghost(int on, float px, float py, const float grid[4], float cx, float cy);
int mtl_ui_ghost_rect(float r[4]);
