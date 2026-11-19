

#pragma once
#include <math.h>
#include <stdint.h>







static inline void dm_front_view(int ww, int wh, int known, int rw, int rh, int ux, int uy, int uw, int uh, int out[8]) {
    out[0] = out[1] = out[4] = out[5] = 0; out[2] = out[6] = ww; out[3] = out[7] = wh;
    if (ww <= 0 || wh <= 0) return;
    if (!known || rw <= 0 || rh <= 0) { known = 0; rw = 16; rh = 9; }
    int px = 0, py = 0, pw = ww, ph = wh;
    if (!(fabs((double)ww / wh / ((double)rw / rh) - 1) < 0.01)) {
        double sc = fmin((double)ww / rw, (double)wh / rh); pw = (int)(rw * sc); ph = (int)(rh * sc); if (pw < 1) pw = 1; if (ph < 1) ph = 1; px = (ww - pw) / 2; py = (wh - ph) / 2; }
    out[4] = out[0] = px; out[5] = out[1] = py; out[6] = out[2] = pw; out[7] = out[3] = ph;
    if (!known || uw <= 0 || uh <= 0) return;
    double sx = (double)pw / rw, sy = (double)ph / rh;
    int vx = px + (int)lround(ux * sx), vy = py + (int)lround(uy * sy), vw = (int)lround(uw * sx), vh = (int)lround(uh * sy);
    if (uh < rh && (int64_t)uw * 9 < (int64_t)uh * 16) { int shift = (int)lround(1173.6 * vh / 1320.0 - vw * 0.5); if (shift > 0) vx += shift; }
    out[0] = vx; out[1] = vy; out[2] = vw; out[3] = vh;
}
