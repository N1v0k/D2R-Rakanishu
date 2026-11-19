















#pragma once
#include <stdint.h>

enum { VK_FPS_UNKNOWN = 0xffff };
#define VK_FPS_WARM_NS 500000000ull
#define VK_FPS_WINDOW_NS 1000000000ull
#define VK_FPS_STALE_MS 500u

typedef struct {
    uint64_t start_ns, last_ns;
    uint32_t frames, tag;
    uint16_t x10;
    uint8_t started, warm;
} vk_fps_window;

static inline uint64_t vk_fps_pack(uint32_t tag, uint16_t x10, uint64_t now_ns) {
    return (uint64_t)(tag & 0xffffu) << 48 | (uint64_t)x10 << 32 | (uint32_t)(now_ns / 1000000u);
}


static inline uint64_t vk_fps_present(vk_fps_window *w, uint64_t now_ns, uint32_t tag) {
    if (!w->started || tag != w->tag || now_ns < w->last_ns || now_ns - w->last_ns > VK_FPS_WARM_NS) {
        w->started = 1; w->tag = tag; w->start_ns = now_ns; w->frames = 0; w->warm = 0; w->x10 = VK_FPS_UNKNOWN;
    } else {
        w->frames++;
        uint64_t span = now_ns - w->start_ns;
        if (span >= (w->warm ? VK_FPS_WINDOW_NS : VK_FPS_WARM_NS)) {
            uint64_t v = ((uint64_t)w->frames * 10000000000ull + span / 2) / span;
            w->x10 = (uint16_t)(v < VK_FPS_UNKNOWN ? v : VK_FPS_UNKNOWN - 1);
            w->warm = 1; w->start_ns = now_ns; w->frames = 0;
        }
    }
    w->last_ns = now_ns;
    return vk_fps_pack(w->tag, w->x10, now_ns);
}


static inline int vk_fps_read(uint64_t packed, uint32_t tag, uint64_t now_ns) {
    if (!packed || (uint32_t)(packed >> 48) != (tag & 0xffffu)) return -1;
    uint32_t x10 = (uint32_t)(packed >> 32) & 0xffffu, at = (uint32_t)packed, now = (uint32_t)(now_ns / 1000000u);
    if (x10 == VK_FPS_UNKNOWN || (uint32_t)(now - at) > VK_FPS_STALE_MS) return -1;
    return (int)((x10 + 5) / 10);
}
