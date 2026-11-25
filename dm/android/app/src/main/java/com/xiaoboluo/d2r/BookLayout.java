package com.xiaoboluo.d2r;




final class BookLayout {
    static final int COLS = 5, ROWS = 3, PER_PAGE = COLS * ROWS;
    static final float BAND = 0.22f;

    static int pages(int count) { return Math.max(1, (count + PER_PAGE - 1) / PER_PAGE); }
    static int clamp(int page, int pages) { return Math.max(0, Math.min(pages - 1, page)); }

    static int onPage(int count, int page) { return Math.max(0, Math.min(PER_PAGE, count - page * PER_PAGE)); }


    static void tile(float left, float top, float right, float bottom, float gap, int i, float[] out) {
        float w = (right - left - (COLS - 1) * gap) / COLS, h = (bottom - top - (ROWS - 1) * gap) / ROWS;
        int col = i % COLS, row = i / COLS;
        out[0] = left + col * (w + gap); out[1] = top + row * (h + gap); out[2] = out[0] + w; out[3] = out[1] + h;
    }

    static void picture(float[] tile, float pad, float[] out) {
        float band = (tile[3] - tile[1]) * BAND, side = Math.max(0, Math.min(tile[2] - tile[0] - 2 * pad, tile[3] - tile[1] - band - 2 * pad));
        float cx = (tile[0] + tile[2]) / 2, top = tile[1] + pad;
        out[0] = cx - side / 2; out[1] = top; out[2] = cx + side / 2; out[3] = top + side;
    }

    static float bandTop(float[] tile) { return tile[3] - (tile[3] - tile[1]) * BAND; }




    static int near(float[][] rects, int from, int dx, int dy) {
        if (from < 0 || from >= rects.length) return rects.length > 0 ? 0 : -1;
        float ax = (rects[from][0] + rects[from][2]) / 2, ay = (rects[from][1] + rects[from][3]) / 2, best = Float.MAX_VALUE;
        int out = from;
        for (int i = 0; i < rects.length; i++) {
            if (i == from) continue;
            float ox = (rects[i][0] + rects[i][2]) / 2 - ax, oy = (rects[i][1] + rects[i][3]) / 2 - ay;
            float along = dx != 0 ? ox * dx : oy * dy, across = dx != 0 ? Math.abs(oy) : Math.abs(ox);
            if (along <= 1) continue;
            float score = along + across * 2.5f;
            if (score < best) { best = score; out = i; }
        }
        return out;
    }
}
