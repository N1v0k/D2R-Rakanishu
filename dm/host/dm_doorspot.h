













#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

enum { DS_R = 40, DS_W = 2 * DS_R + 1, DS_N = DS_W * DS_W };
enum { DS_FAR = 36 };
enum { DS_SELF_GAP = 3 };
struct ds_grid { uint8_t blk[DS_N]; int16_t dist[DS_N]; int reach; };

static inline int ds_blk(const struct ds_grid *g, int ix, int iy) { return ix < 0 || iy < 0 || ix >= DS_W || iy >= DS_W || g->blk[iy * DS_W + ix]; }


static inline int ds_free(const struct ds_grid *g, int ix, int iy) { return !(ds_blk(g, ix, iy) || ds_blk(g, ix + 1, iy) || ds_blk(g, ix - 1, iy) || ds_blk(g, ix, iy + 1) || ds_blk(g, ix, iy - 1)); }
static inline int ds_dist(const struct ds_grid *g, int ix, int iy) { return ix < 0 || iy < 0 || ix >= DS_W || iy >= DS_W ? -1 : g->dist[iy * DS_W + ix]; }




static inline void ds_flood_r(struct ds_grid *g, int relax) {
    static uint16_t q[DS_N]; int h = 0, t = 0; memset(g->dist, 0xff, sizeof g->dist); g->reach = 0;
    g->dist[DS_R * DS_W + DS_R] = 0; q[t++] = (uint16_t)(DS_R * DS_W + DS_R);
    while (h < t) { int c = q[h++], cx = c % DS_W, cy = c / DS_W; g->reach++;
        static const int d4[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
        for (int k = 0; k < 4; k++) { int nx = cx + d4[k][0], ny = cy + d4[k][1]; if (ds_dist(g, nx, ny) != -1 || nx < 0 || ny < 0 || nx >= DS_W || ny >= DS_W || !(ds_free(g, nx, ny) || (g->dist[c] < relax && !ds_blk(g, nx, ny)))) continue;
            g->dist[ny * DS_W + nx] = (int16_t)(g->dist[c] + 1); q[t++] = (uint16_t)(ny * DS_W + nx); } }
}
static inline void ds_flood(struct ds_grid *g) { ds_flood_r(g, 0); }
static inline int ds_clear(const struct ds_grid *g, int ix, int iy, int k) { for (int dy = -k; dy <= k; dy++) for (int dx = -k; dx <= k; dx++) if (ds_dist(g, ix + dx, iy + dy) < 0) return 0; return 1; }








static inline int ds_pick_ring(const struct ds_grid *g, int wx, int wy, int lo, int maxd, int *rx, int *ry) {
    for (int k = 2; k >= 0; k--) { long best = -1; int bx = 0, by = 0;
        for (int y = -DS_R; y <= DS_R; y++) for (int x = -DS_R; x <= DS_R; x++) { int d = ds_dist(g, x + DS_R, y + DS_R), far = abs(x) > abs(y) ? abs(x) : abs(y);
            if (d < 0 || d > maxd || far < lo || !ds_clear(g, x + DS_R, y + DS_R, k)) continue;
            long s = (long)(x - wx) * (x - wx) + (long)(y - wy) * (y - wy); if (best >= 0 && s >= best) continue; best = s; bx = x; by = y; }
        if (best >= 0) { *rx = bx; *ry = by; return k; } }
    return -1;
}
static inline int ds_pick(const struct ds_grid *g, int wx, int wy, int have_o, int ox, int oy, int gap, int *rx, int *ry) {
    for (int tier = 0; tier < 3; tier++) { long best = -1; int bx = 0, by = 0, tx = tier == 2 ? 0 : wx, ty = tier == 2 ? 0 : wy;
        for (int y = -DS_R; y <= DS_R; y++) for (int x = -DS_R; x <= DS_R; x++) { int d = ds_dist(g, x + DS_R, y + DS_R); if (d < 0) continue;
            int far = abs(x) > abs(y) ? abs(x) : abs(y), og = !have_o ? 1 << 20 : abs(x - ox) > abs(y - oy) ? abs(x - ox) : abs(y - oy);
            if (tier < 2 ? (far < DS_SELF_GAP || d > DS_FAR || og < gap || !ds_clear(g, x + DS_R, y + DS_R, 2 - tier)) : (far < 1 || og < 2)) continue;
            long s = (long)(x - tx) * (x - tx) + (long)(y - ty) * (y - ty); if (best >= 0 && s >= best) continue; best = s; bx = x; by = y; }
        if (best >= 0) { *rx = bx; *ry = by; return tier; } }
    *rx = 0; *ry = 0; return 3;
}
