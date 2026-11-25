






#pragma once
#include <math.h>
#include <stdint.h>
#include <string.h>



enum { XBG_CAM_BLOCK = 608, XBG_CAM_SCENE = 1440 };











typedef struct { float dist, h, look, lookh, eye, fwd, pitch, fov, tau, near_, reach, pull_pitch; } xbg_cam_params;
static inline xbg_cam_params xbg_cam_default_params(int mode) {
    xbg_cam_params p = { 20.0f, 13.0f, 4.0f, 3.0f, 6.0f, 2.2f, 8.0f, mode == 2 ? 75.0f : 65.0f, 0.35f, 0.3f, 45.0f, 0.0f };
    return p;
}















typedef struct { float vp_prev[16], vp_last[16], fx, fz, px, pz, eye[3], fwd[3], dcur, tilt, tilt_lo, tilt_hi, ahead[2]; int have, have_eye, hold; uint64_t frame; double t; } xbg_cam_state;
static inline void xbg_cam_init(xbg_cam_state *s) {
    memset(s, 0, sizeof *s);


    s->fx = s->fz = -0.70711f;
    s->frame = UINT64_MAX;
}





static inline int xbg_cam_sig(const uint8_t *p, uint64_t avail, uint64_t full) {
    if (full < XBG_CAM_BLOCK || avail < 272) return 0;
    float v[7], pr[16];
    memcpy(v, p + 80, sizeof v);
    memcpy(pr, p + 208, sizeof pr);
    return fabsf(v[0] - 0.70711f) < 2e-3f && fabsf(v[1] + 0.35355f) < 2e-3f && fabsf(v[2] - 0.61237f) < 2e-3f && fabsf(v[5] - 0.86603f) < 2e-3f
        && fabsf(v[6] - 0.5f) < 2e-3f && pr[3] == 0 && pr[7] == 0 && pr[11] == 0 && fabsf(pr[15] - 1) < 1e-4f;
}


static inline int xbg_cam_sig10(const uint8_t *p, uint64_t avail, uint64_t full) {
    if (full != XBG_CAM_SCENE || avail < 44) return 0;
    float v[3];
    memcpy(v, p + 32, sizeof v);
    return fabsf(v[0] + 0.61237f) < 2e-3f && fabsf(v[1] + 0.5f) < 2e-3f && fabsf(v[2] + 0.61237f) < 2e-3f;
}

static inline void xbg_cam_m4_mul(const float *a, const float *b, float *o) {
    for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++) { double s = 0; for (int k = 0; k < 4; k++) s += (double)a[i * 4 + k] * b[k * 4 + j]; o[i * 4 + j] = (float)s; }
}


static inline int xbg_cam_m4_inv(const float *m, float *o) {
    double a[4][8];
    for (int i = 0; i < 4; i++) for (int j = 0; j < 8; j++) a[i][j] = j < 4 ? m[i * 4 + j] : (j - 4 == i);
    for (int c = 0; c < 4; c++) {
        int p = c;
        for (int r = c + 1; r < 4; r++) if (fabs(a[r][c]) > fabs(a[p][c])) p = r;
        if (fabs(a[p][c]) < 1e-12) return 0;
        if (p != c) for (int j = 0; j < 8; j++) { double t = a[c][j]; a[c][j] = a[p][j]; a[p][j] = t; }
        double d = a[c][c];
        for (int j = 0; j < 8; j++) a[c][j] /= d;
        for (int r = 0; r < 4; r++) if (r != c) { double f = a[r][c]; for (int j = 0; j < 8; j++) a[r][j] -= f * a[c][j]; }
    }
    for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++) o[i * 4 + j] = (float)a[i][j + 4];
    return 1;
}

















#define XBG_CAM_ORIG_DIST 884.03f
static inline void xbg_cam_ground(const uint8_t *src, float *px, float *py, float *pz) {
    float iv[16], pr[16];
    memcpy(iv, src + 144, sizeof iv);
    memcpy(pr, src + 208, sizeof pr);
    float d = XBG_CAM_ORIG_DIST, s = 0;
    const float h = pr[5] != 0 ? fabsf(2 / pr[5]) : 0;
    if (h > 10 && h < 500) {
        const float u = h / (0.75f * 70.7107f);
        d = h / (2 * tanf(u * atanf(0.03f)));
        s = 7.25f * (u - 1) + 0.176777f * 8 * u;
    }
    const float hl = sqrtf(iv[8] * iv[8] + iv[10] * iv[10]);
    if (!(hl > 1e-4f)) s = 0;
    *px = iv[12] - d * iv[8] - (s != 0 ? s * iv[8] / hl : 0);
    *py = iv[13] - d * iv[9];
    *pz = iv[14] - d * iv[10] - (s != 0 ? s * iv[10] / hl : 0);
}







static inline int xbg_cam_follow(xbg_cam_state *s, float px, float pz, uint64_t frame, double now, int want, float wx, float wz, float tau) {
    if (frame == s->frame) return 0;
    s->frame = frame;
    double dt = s->t ? now - s->t : 0;
    s->t = now;
    if (dt > 0.25) dt = 0.25;
    float dx = px - s->px, dz = pz - s->pz, l = sqrtf(dx * dx + dz * dz);
    if (!s->have || l >= 20) s->fx = s->fz = -0.70711f;
    else if (!s->hold) {
        if (!want && l > 0.05f) { wx = dx / l; wz = dz / l; want = 1; }
        if (want && dt > 0) {
            float a = atan2f(s->fz, s->fx), d = atan2f(wz, wx) - a;
            while (d > 3.14159265f) d -= 6.2831853f;
            while (d < -3.14159265f) d += 6.2831853f;
            a += d * (float)(1 - exp(-dt / tau));
            s->fx = cosf(a);
            s->fz = sinf(a);
        }
    }
    s->px = px;
    s->pz = pz;
    s->have = 1;
    memcpy(s->vp_prev, s->vp_last, 64);
    return 1;
}
















static inline int xbg_cam_drag(xbg_cam_state *s, float px, float pz, uint64_t frame, double now, float dist, float k) {
    if (frame == s->frame) return 0;
    s->frame = frame;
    s->t = now;
    float dx = px - s->px, dz = pz - s->pz, l = sqrtf(dx * dx + dz * dz);
    if (!s->have || l >= 20) s->fx = s->fz = -0.70711f;
    else if (l > 1e-4f) {
        float ex = px - (s->px - s->fx * dist), ez = pz - (s->pz - s->fz * dist), el = sqrtf(ex * ex + ez * ez);
        if (el > 1e-3f) {
            float a = atan2f(s->fz, s->fx), d = atan2f(ez, ex) - a;
            while (d > 3.14159265f) d -= 6.2831853f;
            while (d < -3.14159265f) d += 6.2831853f;
            a += d * (k < 0 ? 0 : k > 1 ? 1 : k);
            s->fx = cosf(a);
            s->fz = sinf(a);
        }
    }
    s->px = px;
    s->pz = pz;
    s->have = 1;
    memcpy(s->vp_prev, s->vp_last, 64);
    return 1;
}




static inline void xbg_cam_ease(xbg_cam_state *s, float wx, float wz, double dt, float tau) {
    if (!(dt > 0) || !(tau > 0) || (wx == 0 && wz == 0)) return;
    float a = atan2f(s->fz, s->fx), d = atan2f(wz, wx) - a;
    while (d > 3.14159265f) d -= 6.2831853f;
    while (d < -3.14159265f) d += 6.2831853f;
    a += d * (float)(1 - exp(-dt / tau));
    s->fx = cosf(a);
    s->fz = sinf(a);
}




static inline void xbg_cam_turn(xbg_cam_state *s, float rad) {
    float a = atan2f(s->fz, s->fx) + rad;
    s->fx = cosf(a);
    s->fz = sinf(a);
}






static inline float xbg_cam_pull(xbg_cam_state *s, float dist, float clear, double dt) {
    float target = clear > 0 && clear < dist ? clear : dist;
    if (target < 2) target = 2;
    if (s->dcur <= 0 || dt <= 0) s->dcur = target;
    else s->dcur += (target - s->dcur) * (float)(1 - exp(-dt / (target < s->dcur ? 0.08 : 0.4)));
    return s->dcur;
}






static inline void xbg_cam_build(xbg_cam_state *s, const uint8_t *src, uint8_t *dst, int mode, const xbg_cam_params *pp, float vp[16], float ivp[16]) {
    float pr[16], px, py, pz;
    memcpy(pr, src + 208, sizeof pr);
    xbg_cam_ground(src, &px, &py, &pz);
    px -= s->ahead[0];
    pz -= s->ahead[1];
    float fx = s->fx, fz = s->fz, eye[3], at[3], fov = pp->fov;
    if (mode == 2) {


        const float deg = 3.14159265f / 180, p0 = pp->pitch * deg;
        s->tilt_lo = p0 - 60 * deg; s->tilt_hi = p0 + 10 * deg;
        float h = pp->eye, ah = pp->fwd, pitch = p0 - fminf(fmaxf(s->tilt, s->tilt_lo), s->tilt_hi);
        eye[0] = px + fx * ah; eye[1] = py + h; eye[2] = pz + fz * ah;
        at[0] = eye[0] + fx * cosf(pitch); at[1] = eye[1] - sinf(pitch); at[2] = eye[2] + fz * cosf(pitch);
    } else {





        float d = s->dcur > 0 ? s->dcur : pp->dist, lh = pp->lookh, boom = pp->dist * pp->dist + (pp->h - lh) * (pp->h - lh);



        float la = pp->dist > 0 && d < pp->dist ? pp->look * d / pp->dist : pp->look;
        float h = d < pp->dist ? lh + sqrtf(fmaxf(boom - d * d, 0)) : pp->h;



        if (pp->pull_pitch > 0 && d < pp->dist) h = fminf(h, lh + (d + la) * tanf(pp->pull_pitch * 3.14159265f / 180));






        float vhalf = atanf(tanf(fov * 3.14159265f / 360) * (pr[5] != 0 ? fabsf(pr[0] / pr[5]) : 9.0f / 16));
        for (int k = 0; k < 16; k++) {
            float top = fmaxf(3 * 3.14159265f / 180, pp->reach > 0 ? atanf(h / (d + pp->reach)) : 0), hmin = lh + (d + la) * tanf(vhalf + top);
            if (h >= hmin - 1e-3f) break;
            h = hmin;
        }





        const float deg = 3.14159265f / 180, run = d + la, rise = h - lh, R = sqrtf(run * run + rise * rise), e0 = atan2f(rise, run);
        s->tilt_lo = e0 - 75 * deg; s->tilt_hi = e0 - 4 * deg;
        const float e = e0 - fminf(fmaxf(s->tilt, s->tilt_lo), s->tilt_hi);
        const float dwall = d;
        d = fminf(R * cosf(e) - la, dwall); h = lh + R * sinf(e);
        eye[0] = px - fx * d; eye[1] = py + h; eye[2] = pz - fz * d;
        at[0] = px + fx * la; at[1] = py + lh; at[2] = pz + fz * la;
    }


    float b[3] = { eye[0] - at[0], eye[1] - at[1], eye[2] - at[2] }, bl = sqrtf(b[0] * b[0] + b[1] * b[1] + b[2] * b[2]);
    for (int i = 0; i < 3; i++) b[i] /= bl;
    float r[3] = { b[2], 0, -b[0] }, rl = sqrtf(r[0] * r[0] + r[2] * r[2]);
    r[0] /= rl; r[2] /= rl;
    float u[3] = { b[1] * r[2] - b[2] * r[1], b[2] * r[0] - b[0] * r[2], b[0] * r[1] - b[1] * r[0] };
    float V[16] = { r[0], u[0], b[0], 0,  r[1], u[1], b[1], 0,  r[2], u[2], b[2], 0,
        -(eye[0] * r[0] + eye[1] * r[1] + eye[2] * r[2]), -(eye[0] * u[0] + eye[1] * u[1] + eye[2] * u[2]), -(eye[0] * b[0] + eye[1] * b[1] + eye[2] * b[2]), 1 };


    float n = pp->near_, F = 3000, sx = 1.0f / tanf(fov * 3.14159265f / 360), sy = sx * (pr[0] != 0 ? pr[5] / pr[0] : 16.0f / 9);
    float A = n / (F - n), B = n * F / (F - n);
    float P[16] = { sx, 0, 0, 0,  0, sy, 0, 0,  0, 0, A, -1,  0, 0, B, 0 }, iV[16], iP[16];
    xbg_cam_m4_mul(V, P, vp);
    xbg_cam_m4_inv(V, iV);
    xbg_cam_m4_inv(P, iP);
    xbg_cam_m4_inv(vp, ivp);
    memcpy(dst + 80, V, 64); memcpy(dst + 144, iV, 64); memcpy(dst + 208, P, 64); memcpy(dst + 272, iP, 64); memcpy(dst + 336, vp, 64); memcpy(dst + 400, ivp, 64);


    memcpy(dst + 464, s->have && s->vp_prev[15] != 0 ? s->vp_prev : vp, 64);
    memcpy(s->vp_last, vp, 64);



    { int32_t zero = 0; float xy[2] = { B, -A }; memcpy(dst + 592, xy, 8); memcpy(dst + 604, &zero, 4); }
    for (int i = 0; i < 3; i++) { s->eye[i] = eye[i]; s->fwd[i] = -b[i]; }
    s->have_eye = 1;
}



static inline void xbg_cam_build10(const xbg_cam_state *s, uint8_t *dst) {
    if (!s->have_eye) return;
    memcpy(dst + 16, s->eye, 12);
    memcpy(dst + 32, s->fwd, 12);
    float zero = 0;
    memcpy(dst + 848 + 8, &zero, 4);
}
