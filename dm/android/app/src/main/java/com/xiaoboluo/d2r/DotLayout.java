package com.xiaoboluo.d2r;






final class DotLayout {
    static final float GAP = 0.45f;
    static final float SPACE = 0.2f;
    static final float CLEAR = 0.5f;
    static final float PAD = 0.3f;
    static final int FREE = 0, LEFT = 1, RIGHT = 2, OUTSIDE = 3, NONE = 4;



    static String dotGlyph(String primary, String fallback, boolean primaryOk, boolean fallbackOk) {
        return primaryOk ? primary : fallbackOk ? fallback : null;
    }


    static int row(int code, int r) { return (code >> (3 * r)) & 7; }

    static int dots(int v) { return v == 0 ? 0 : Math.max(1, Math.abs(v - 4)); }

    static int rows(int code) { return (row(code, 0) != 0 ? 1 : 0) + (row(code, 1) != 0 ? 1 : 0); }

    final int code;
    final float px, lineH, labels, dotW, step, width, height;
    final int[] line = new int[2];
    float left, top;
    int where = FREE;


    DotLayout(int code, float lineH, float px, float labelW0, float labelW1, float dotW) {
        this.code = code; this.px = px; this.lineH = lineH; this.dotW = dotW;
        float widest = 0;
        int n = 0, most = 0;
        for (int r = 0; r < 2; r++) {
            if (row(code, r) == 0) { line[r] = -1; continue; }
            line[r] = n++;
            widest = Math.max(widest, r == 0 ? labelW0 : labelW1);
            most = Math.max(most, dots(row(code, r)));
        }
        labels = widest > 0 ? widest + GAP * px : 0;
        step = dotW + SPACE * px;
        width = labels + dotsWidth(most);
        height = n * lineH;
    }

    private float dotsWidth(int n) { return n > 0 ? n * dotW + (n - 1) * SPACE * px : 0; }

    float rowWidth(int r) { return labels + dotsWidth(dots(row(code, r))); }
    float rowTop(int r) { return top + line[r] * lineH; }
    float dotLeft(int i) { return left + labels + i * step; }
    float pad() { return PAD * px; }


    DotLayout at(float left, float top) { this.left = left; this.top = top; where = FREE; return this; }





    DotLayout inTooltip(float l, float t, float r, float b, float in, float textTop, float[] lineL, float[] lineR, float viewW, float viewH,
                        boolean besideBoxes) {
        float clear = CLEAR * px, innerL = l + in, innerR = r - in;
        top = textTop;
        if (width <= innerR - innerL) {
            boolean leftFree = true, rightFree = true;
            for (int k = 0; k < 2; k++) {
                int i = line[k];
                if (i < 0 || i > 1 || lineL[i] > lineR[i]) continue;
                leftFree &= innerL + rowWidth(k) + clear <= lineL[i];
                rightFree &= lineR[i] + clear <= innerR - width;
            }
            if (leftFree) { left = innerL; where = LEFT; return this; }
            if (rightFree) { left = innerR - width; where = RIGHT; return this; }
        }
        float pad = pad(), bw = width + 2 * pad, bh = height + 2 * pad;
        float[][] spots = { { l, t - bh }, { l - bw, t }, { r, t }, { l, b } };
        for (int k = 0; k < spots.length; k++) {
            if (besideBoxes && (k == 1 || k == 2)) continue;
            float[] s = spots[k];
            if (s[0] < 0 || s[1] < 0 || s[0] + bw > viewW || s[1] + bh > viewH) continue;
            left = s[0] + pad; top = s[1] + pad; where = OUTSIDE;
            return this;
        }
        where = NONE;
        return this;
    }
}
