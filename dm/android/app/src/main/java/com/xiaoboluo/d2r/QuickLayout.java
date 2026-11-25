




package com.xiaoboluo.d2r;

final class QuickLayout {
    static final int OUTSIDE = -1, NEUTRAL = -2, CLOSE = -3;
    static final float GAP = 24;


    float bx, bw, bh, pitch;
    float listTop;
    int n;
    private float frameL, frameT, frameR, frameB, closeL, closeT, closeR, closeB;

    void compute(GameStyleGeometry.Panel p, int entries) {
        n = Math.max(1, entries);
        float row = p.buttonH() + GAP * p.s, room = p.contentB - p.contentT;
        pitch = Math.min(row, room / n);
        float f = pitch / row;
        bw = p.buttonW() * f; bh = p.buttonH() * f;
        bx = (p.contentL + p.contentR) / 2 - bw / 2;
        listTop = (p.contentT + p.contentB) / 2 - n * pitch / 2;
        frameL = p.frameL; frameT = p.frameT; frameR = p.frameR; frameB = p.frameB;
        closeL = p.hitCloseL; closeT = p.hitCloseT; closeR = p.hitCloseR; closeB = p.hitCloseB;
    }

    float rowTop(int i) { return listTop + i * pitch; }
    float btnTop(int i) { return rowTop(i) + (pitch - bh) / 2; }



    int hit(float x, float y) {
        if (x >= closeL && x < closeR && y >= closeT && y < closeB) return CLOSE;
        if (x < frameL || x >= frameR || y < frameT || y >= frameB) return OUTSIDE;
        if (x < bx || x >= bx + bw || y < listTop || y >= listTop + n * pitch) return NEUTRAL;
        int i = Math.min(n - 1, (int) ((y - listTop) / pitch));
        return i * 2 + (x < bx + bw / 2 ? 0 : 1);
    }
}
