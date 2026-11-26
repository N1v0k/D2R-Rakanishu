






#pragma once
#include <stdint.h>
#include "../game_addrs.h"
#include <string.h>
static int car_sets, car_steps, car_refreshes;
static int32_t car_get32(uint64_t w, int at) { int32_t v; memcpy(&v, (const void *)(uintptr_t)(w + at), 4); return v; }
static void car_put32(uint64_t w, int at, int32_t v) { memcpy((void *)(uintptr_t)(w + at), &v, 4); }
static int car_selectable(uint64_t w, int k) { return ((const uint8_t *)(uintptr_t)w)[0x7d8 + k] != 0; }
static void test_car_set(uint64_t w, int k) { car_sets++; if (k >= 0 && k < car_get32(w, 0x7e8) && car_selectable(w, k)) car_put32(w, 0x7ec, k); }
static void test_car_step(uint64_t w, int fwd) {
    int n = car_get32(w, 0x7e8), k = car_get32(w, 0x7ec), gap = ((const uint8_t *)(uintptr_t)w)[0x971]; car_steps++;
    if (gap) { int any = 0; for (int j = k + (fwd ? 1 : -1); j >= 0 && j < n && !any; j += fwd ? 1 : -1) any = car_selectable(w, j); if (!any) return; }
    for (int s = 0; s < n; s++) { k = (k + (fwd ? 1 : n - 1)) % n; if (car_selectable(w, k)) { car_put32(w, 0x7ec, k); return; } } }
static void test_car_refresh(uint64_t t) { (void)t; car_refreshes++; }
#define DM_CUSTOM_CAROUSEL_SET test_car_set
#define DM_CUSTOM_CAROUSEL_STEP test_car_step
#define DM_CUSTOM_CAROUSEL_REFRESH test_car_refresh

static void car_fake(uint8_t *w, uint64_t vt, int n, int gap, uint64_t text) {
    memcpy(w, &vt, 8); memset(w + 0x7d8, 0, 16);
    memset(w + 0x7d8, 1, (size_t)n); memcpy(w + 0x7e8, &n, 4); memset(w + 0x7ec, 0, 4); memcpy(w + 0x7f8, &text, 8); memset(w + 0x800, 0, 4); w[0x971] = (uint8_t)gap; }
static int car_index(const uint8_t *w) { int32_t v; memcpy(&v, w + 0x7ec, 4); return v; }

static void car_tap(uint8_t *w, int fwd) { test_car_step((uint64_t)(uintptr_t)w, fwd); }

int dm_overlay_file(const char *rel, char *out, size_t n) { (void)rel; (void)out; (void)n; return 0; }
int dm_lang_index(void) { return 1; }
