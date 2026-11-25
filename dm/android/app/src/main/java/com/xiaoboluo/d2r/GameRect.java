package com.xiaoboluo.d2r;




final class GameRect {
    private static final GameRect UI = new GameRect();
    static GameRect ui() { return UI; }

    private int winW, winH;
    private final int[] r = new int[8];

    GameRect() { }


    private final int[] n = new int[8];
    boolean update(int w, int h, int[] in) {
        java.util.Arrays.fill(n, 0);
        n[2] = n[6] = Math.max(0, w); n[3] = n[7] = Math.max(0, h);
        if (in != null && in.length >= 8 && in[2] > 0 && in[3] > 0 && in[6] > 0 && in[7] > 0) System.arraycopy(in, 0, n, 0, 8);
        boolean same = !stale && winW == w && winH == h;
        for (int i = 0; i < 8 && same; i++) same = r[i] == n[i];
        if (same) return false;
        winW = w; winH = h; stale = false; System.arraycopy(n, 0, r, 0, 8);
        return true;
    }


    private boolean stale;
    void forget() { stale = true; }

    int winW() { return winW; }
    int winH() { return winH; }
    int x() { return r[0]; }
    int y() { return r[1]; }
    int w() { return r[2]; }
    int h() { return r[3]; }
    int pictureX() { return r[4]; }
    int pictureY() { return r[5]; }
    int pictureW() { return r[6]; }
    int pictureH() { return r[7]; }


    boolean whole() { return r[0] == 0 && r[1] == 0 && r[2] == winW && r[3] == winH && r[4] == 0 && r[5] == 0 && r[6] == winW && r[7] == winH; }


    float pictureAtX(float nx) { return r[4] + nx * r[6]; }
    float pictureAtY(float ny) { return r[5] + ny * r[7]; }



    int topY() { return r[5]; }


    int uiH() { return r[3]; }


    float pictureToBlock(float v) { return r[3] > 0 ? v * r[7] / r[3] : v; }



    static final int WHOLE = 0, BLOCK = 1, TOP = 2, PAGE = 3;
    void bounds(int kind, int[] out) {
        if (kind == BLOCK) { out[0] = r[0]; out[1] = r[1]; out[2] = r[2]; out[3] = r[3]; }
        else if (kind == TOP) { out[0] = r[0]; out[1] = topY(); out[2] = r[2]; out[3] = r[3]; }
        else if (kind == PAGE) { out[0] = r[0]; out[1] = r[5] + (r[7] - r[3]) / 2; out[2] = r[2]; out[3] = r[3]; }
        else { out[0] = 0; out[1] = 0; out[2] = winW; out[3] = winH; }
    }
}
