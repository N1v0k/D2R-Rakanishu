package com.xiaoboluo.d2r;









final class ExpRailGeometry {
    static final int PIECES = 21, GUTTERS = 10;

    private ExpRailGeometry() {}


    static boolean validEdges(int[] e, int sourceWidth) {
        if (e == null || e.length != PIECES + 1 || e[0] != 0 || e[PIECES] != sourceWidth) return false;
        for (int i = 0; i < PIECES; i++) if (e[i + 1] <= e[i]) return false;
        return true;
    }



    static int[] target(int[] e, int width, float pxPerSource) {
        if (e == null || e.length != PIECES + 1 || width <= 0 || !(pxPerSource > 0) || Float.isInfinite(pxPerSource)) return null;
        int[] w = new int[PIECES];
        long fixed = 0, gutterSource = 0;
        for (int i = 0; i < PIECES; i += 2) { w[i] = Math.round((e[i + 1] - e[i]) * pxPerSource); if (w[i] < 1) return null; fixed += w[i]; }
        for (int i = 1; i < PIECES; i += 2) gutterSource += e[i + 1] - e[i];
        long rest = width - fixed;
        if (rest < GUTTERS || gutterSource <= 0) return null;
        long sum = 0, prev = 0;
        for (int i = 1; i < PIECES; i += 2) {
            sum += e[i + 1] - e[i];
            long upTo = Math.round((double) rest * sum / gutterSource);
            w[i] = (int) (upTo - prev); prev = upTo;
            if (w[i] < 1) return null;
        }
        int[] t = new int[PIECES + 1];
        for (int i = 0; i < PIECES; i++) t[i + 1] = t[i] + w[i];
        return t;
    }



    static float clip(int[] t, int permille) {
        if (t == null || permille <= 0) return 0;
        if (permille >= 1000) return t[PIECES];
        int k = (permille - 1) / 100;
        float frac = (permille - 100 * k) / 100f;
        return t[2 * k + 1] + frac * (t[2 * k + 2] - t[2 * k + 1]);
    }
}
