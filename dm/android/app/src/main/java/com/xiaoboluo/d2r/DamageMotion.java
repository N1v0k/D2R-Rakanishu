package com.xiaoboluo.d2r;































final class DamageMotion {
    static final int F_DOT = 1, F_MINION = 2, F_WHITE = 4, F_CRIT = 8, F_SPLASH = 16;
    static final int FIELDS = 8, MAX = 32, LANES = 4, CAP = 64, LIFTS = 8;
    static final float[] LANE = {-1.5f, -0.5f, 0.5f, 1.5f, -2.2f, 2.2f};
    static final float TEXT = 0.042f, SMALL = 0.8f, CRIT = 1.3f, POP_T = 0.18f, POP = 0.35f, RISE = 0.10f, DOT_RISE = 0.55f, HEAD_GAP = 0.02f,
        PITCH = 1.12f, MIN_PITCH = 1.6f, JITTER = 0.12f, ARC_MIN = 0.3f, ARC_SPAN = 0.6f, SPEED_MIN = 0.85f, SPEED_SPAN = 0.3f, STACK = 1.05f, FADE_FROM = 0.6f, DOT_DROP = 1.2f,
        EDGE = 0.01f, CAP_H = 0.75f, PAD = 0.06f, STEP = 0.04f, RELAX = 0.05f;



    private final int[] seqs = new int[CAP], lanes = new int[CAP];
    private final float[] x0 = new float[CAP], lift = new float[CAP], widths = new float[CAP], held = new float[CAP];
    private final boolean[] used = new boolean[CAP], seen = new boolean[CAP];
    private final int[] slot = new int[MAX];
    private final int[] nearLane = new int[MAX], order = new int[LANES];
    private final float[] nearAge = new float[MAX], shift = new float[MAX], tmp = new float[3], a = new float[4], b = new float[4];

    static int seq(float[] d, int i) { return (int) d[i * FIELDS + 4]; }
    static int unit(float[] d, int i) { return (int) d[i * FIELDS + 7]; }
    static int flags(float[] d, int i) { return (int) d[i * FIELDS + 3] >> 5; }
    static float age(float[] d, int i) { return d[i * FIELDS + 2]; }


    static int hash(int seq) {
        int h = seq * 0x9E3779B1;
        h ^= h >>> 15; h *= 0x85EBCA77; h ^= h >>> 13; h *= 0xC2B2AE3D; h ^= h >>> 16;
        return h;
    }
    static float rnd(int h, int k) { return (h >>> (k * 8) & 0xff) / 255f; }
    static float ease(float t) { t = Math.max(0, Math.min(1, t)); return 1 - (1 - t) * (1 - t); }


    static float scale(int flags) { return (flags & F_SPLASH) != 0 ? SMALL : (flags & F_CRIT) != 0 ? CRIT : (flags & (F_DOT | F_MINION)) != 0 ? SMALL : 1f; }

    static float pop(int flags, float age) {
        if ((flags & F_CRIT) == 0 || !(age >= 0) || age >= POP_T) return 1f;
        return 1f + POP * (float) Math.sin(Math.PI * age / POP_T);
    }
    static int alpha(float age) { return age < FADE_FROM ? 255 : Math.max(0, (int) (255 * (1 - age) / (1 - FADE_FROM))); }



    static int rank(int seq, boolean dot, int[] lane, float[] age, int n, int[] out) {
        int count = dot ? 2 : LANES, first = dot ? LANES : 0, start = hash(seq) >>> 30;
        for (int k = 0; k < count; k++) {
            int l = first + (start + k) % count, c = k;
            float youngest = youngest(l, lane, age, n);
            while (c > 0 && youngest(out[c - 1], lane, age, n) < youngest) { out[c] = out[c - 1]; c--; }
            out[c] = l;
        }
        return count;
    }
    static int lane(int seq, boolean dot, int[] lane, float[] age, int n) { int[] o = new int[LANES]; rank(seq, dot, lane, age, n, o); return o[0]; }
    private static float youngest(int l, int[] lane, float[] age, int n) { float y = Float.MAX_VALUE; for (int i = 0; i < n; i++) if (lane[i] == l) y = Math.min(y, age[i]); return y; }


    static void path(int s, int f, int l, float x0, float lift, float t, float size, float h, float[] out) {
        int hs = hash(s);
        float e = ease(t), dir = LANE[l] < 0 ? -1 : 1, arc = (ARC_MIN + ARC_SPAN * rnd(hs, 1)) * size;
        float rise = RISE * h * (SPEED_MIN + SPEED_SPAN * rnd(hs, 2)) * ((f & F_DOT) != 0 ? DOT_RISE : 1);
        out[0] = x0 + dir * arc * e;
        out[1] = ((f & F_DOT) != 0 ? DOT_DROP * size : -HEAD_GAP * h) - lift - rise * e;
        out[2] = size * scale(f) * pop(f, t);
    }


    static float fit(float x0, int s, int f, int l, float ax, float wd, float size, float w) {
        float arc = (ARC_MIN + ARC_SPAN * rnd(hash(s), 1)) * size * (LANE[l] < 0 ? -1 : 1), hw = wd * ((f & F_CRIT) != 0 ? 1 + POP : 1) / 2, m = EDGE * w;
        float lo = ax + x0 + Math.min(0, arc) - hw - m, hi = ax + x0 + Math.max(0, arc) + hw - (w - m);
        if (lo < 0 && hi < 0) return x0 - lo;
        if (hi > 0 && lo > 0) return x0 - hi;
        return x0;
    }





    void track(float[] d, int n, float[] width, float size, float w, float h) {
        n = Math.min(n, MAX);
        for (int r = 0; r < CAP; r++) seen[r] = false;
        for (int i = 0; i < n; i++) {
            slot[i] = -1;
            int s = seq(d, i);
            for (int r = 0; r < CAP; r++) if (used[r] && seqs[r] == s) { slot[i] = r; seen[r] = true; break; }
        }
        for (int r = 0; r < CAP; r++) if (!seen[r]) used[r] = false;
        for (int i = 0; i < n; i++) shift[i] = slot[i] >= 0 ? held[slot[i]] : 0;
        while (true) {
            int pick = -1;
            for (int i = 0; i < n; i++) if (slot[i] < 0 && (pick < 0 || seq(d, i) - seq(d, pick) < 0)) pick = i;
            if (pick < 0) break;
            int s = seq(d, pick), u = unit(d, pick), f = flags(d, pick), near = 0;
            float widest = width[pick];
            for (int j = 0; j < n; j++) {
                if (slot[j] < 0 || unit(d, j) != u) continue;
                nearLane[near] = lanes[slot[j]]; nearAge[near++] = age(d, j);
                widest = Math.max(widest, widths[slot[j]]);
            }
            int count = rank(s, (f & F_DOT) != 0, nearLane, nearAge, near, order), r = 0;
            while (r < CAP - 1 && used[r]) r++;
            used[r] = true; seen[r] = true; seqs[r] = s; widths[r] = width[pick]; slot[pick] = r;
            shift[pick] = 0;
            for (int j = 0; j < n; j++) if (j != pick && slot[j] >= 0 && unit(d, j) == u) { shift[pick] = shift[j]; break; }
            held[r] = shift[pick];
            float pitch = Math.max(MIN_PITCH * size, PITCH * widest), jitter = (rnd(hash(s), 0) * 2 - 1) * JITTER * size, ax = d[pick * FIELDS] * w + shift[pick];
            float best = Float.MAX_VALUE; int bestLane = order[0]; float bestX = 0, bestLift = 0;
            search:
            for (int k = 0; k <= LIFTS; k++) for (int c = 0; c < count; c++) {
                int l = order[c];
                lanes[r] = l; lift[r] = k * STACK * size * scale(f);
                x0[r] = fit(LANE[l] * pitch + jitter, s, f, l, ax, width[pick], size, w);
                float o = clash(d, n, pick, size, w, h);
                if (o < best) { best = o; bestLane = l; bestX = x0[r]; bestLift = lift[r]; }
                if (o <= 0) break search;
            }
            lanes[r] = bestLane; x0[r] = bestX; lift[r] = bestLift;
        }
        shifts(d, n, size, w, h);
    }


    void place(float[] d, int i, float size, float w, float h, float[] out) { at(d, i, slot[i], 0, size, w, h, out); }

    private void at(float[] d, int i, int r, float dt, float size, float w, float h, float[] out) {
        int o = i * FIELDS;
        if (r >= 0) path(seqs[r], flags(d, i), lanes[r], x0[r], lift[r], age(d, i) + dt, size, h, out);
        else path(seq(d, i), flags(d, i), 0, 0, 0, age(d, i) + dt, size, h, out);
        out[0] += d[o] * w + (r >= 0 ? shift[i] : 0);
        out[1] = Math.max(out[2], Math.min(h - EDGE * h, d[o + 1] * h + out[1]));

    }

    private boolean box(float[] d, int i, float dt, float size, float w, float h, float[] out) {
        int r = slot[i];
        if (r < 0 || !(age(d, i) + dt < FADE_FROM)) return false;
        at(d, i, r, dt, size, w, h, tmp);
        float hw = widths[r] * pop(flags(d, i), age(d, i) + dt) / 2;
        float pad = PAD * tmp[2];
        out[0] = tmp[0] - hw - pad; out[1] = tmp[1] - (CAP_H + PAD) * tmp[2]; out[2] = tmp[0] + hw + pad; out[3] = tmp[1] + pad;
        return true;
    }
    static float overlap(float[] a, float[] b) {
        float w = Math.min(a[2], b[2]) - Math.max(a[0], b[0]), h = Math.min(a[3], b[3]) - Math.max(a[1], b[1]);
        if (w <= 0 || h <= 0) return 0;
        return w * h / Math.min((a[2] - a[0]) * (a[3] - a[1]), (b[2] - b[0]) * (b[3] - b[1]));
    }


    private float clash(float[] d, int n, int p, float size, float w, float h) {
        float worst = 0;
        for (int j = 0; j < n; j++) {
            if (j == p || slot[j] < 0) continue;
            for (float dt = 0; box(d, p, dt, size, w, h, a) && box(d, j, dt, size, w, h, b); dt += STEP) worst = Math.max(worst, overlap(a, b));
        }
        return worst;
    }




    private void shifts(float[] d, int n, float size, float w, float h) {
        for (int i = 0; i < n; i++) {
            if (slot[i] < 0) continue;
            int u = unit(d, i), j = 0;
            while (j < i && (slot[j] < 0 || unit(d, j) != u)) j++;
            if (j < i) { shift[i] = shift[j]; held[slot[i]] = shift[j]; continue; }
            float want = groupShift(d, n, u, -1, size, w, h), was = held[slot[i]], step = RELAX * size, now;
            if (want > 0 ? was < want : want < 0 ? was > want : false) now = want;
            else now = was > want ? Math.max(want, was - step) : Math.min(want, was + step);
            shift[i] = now; held[slot[i]] = now;
        }
    }
    private float groupShift(float[] d, int n, int u, int skip, float size, float w, float h) {
        float left = 0, right = 0, m = EDGE * w;
        for (int j = 0; j < n; j++) {
            int r = slot[j];
            if (j == skip || r < 0 || unit(d, j) != u) continue;
            float t = age(d, j);
            path(seqs[r], flags(d, j), lanes[r], x0[r], lift[r], t, size, h, tmp);
            float x = d[j * FIELDS] * w + tmp[0], hw = widths[r] * pop(flags(d, j), t) / 2;
            left = Math.max(left, m + hw - x); right = Math.max(right, x + hw - (w - m));
        }
        return left > 0 ? left : -right;
    }
}
