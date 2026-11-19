package com.xiaoboluo.d2r;























final class StatusPlace {
    static final int UNKNOWN = -2, EMPTY = -1, SCALE = 100000, MAX = 3 * SCALE, TOL = 300;
    static final long HOLD_NS = 500_000_000L;


    static final float SIZE = 0.024f, LINE = 1.3f, EDGE = 0.02f, TOP = 0.222f, ASCENT = 0.8f, DESCENT = 0.35f, MAX_TOP = 0.34f, MAX_RESERVED = 0.6f;

    private int shown = UNKNOWN, pending = UNKNOWN;
    private long pendingSince, unknownSince = -1;

    int shown() { return shown; }

    private static int level(int v) { return v == EMPTY ? -1 : v; }


    boolean update(int raw, long ns) {
        if (raw < UNKNOWN || raw > MAX) raw = UNKNOWN;
        if (raw == UNKNOWN) {
            pending = UNKNOWN;
            if (shown == UNKNOWN) { unknownSince = -1; return false; }
            if (unknownSince < 0) unknownSince = ns;
            if (ns - unknownSince < HOLD_NS) return false;
            shown = UNKNOWN; unknownSince = -1; return true;
        }
        unknownSince = -1;
        if (shown == UNKNOWN) { shown = raw; pending = UNKNOWN; return true; }
        if (raw == shown) { pending = UNKNOWN; return false; }
        int now = level(raw), cur = level(shown);
        if (now > cur || cur - now <= TOL) { shown = raw; pending = UNKNOWN; return true; }
        if (pending == UNKNOWN) { pending = raw; pendingSince = ns; return false; }
        if (now > level(pending)) pending = raw;
        if (ns - pendingSince < HOLD_NS) return false;
        shown = pending; pending = UNKNOWN; return true;
    }




    static float baseline(int placed, float h, float insetTop) {
        float size = h * SIZE;
        if (placed == UNKNOWN || placed < UNKNOWN || placed > MAX) return h * TOP + size + insetTop;
        float top = placed == EMPTY ? h * EDGE : placed / (float) SCALE * h + 0.5f * size * LINE;
        top = Math.max(h * EDGE, Math.min(top, h * MAX_TOP));
        return top + ASCENT * size;
    }


    static float reserved(int placed, float h, float insetTop, int lines) {
        float size = h * SIZE, r;
        if (lines > 0) r = baseline(placed, h, insetTop) + (lines - 1) * size * LINE + DESCENT * size;
        else if (placed == UNKNOWN || placed < UNKNOWN || placed > MAX) r = h * TOP + insetTop;
        else r = placed == EMPTY ? h * EDGE : placed / (float) SCALE * h;
        return Math.min(r, h * MAX_RESERVED);
    }






    static float effectsTop(int placed, float h, float insetTop, int lines) {
        float size = h * SIZE;
        if (lines > 0) return reserved(placed, h, insetTop, lines) + 0.5f * size * LINE;
        return baseline(placed, h, insetTop) - ASCENT * size;
    }



    static float left(float h, float insetLeft) { return h * EDGE + insetLeft; }
}
