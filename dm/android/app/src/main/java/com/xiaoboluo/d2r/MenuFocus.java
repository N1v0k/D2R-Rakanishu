









package com.xiaoboluo.d2r;

final class MenuFocus {
    static final int ACTION = 0, VALUE = 1;

    static final int R_NONE = 0, R_MOVED = 1, R_STEP_BACK = 2, R_STEP_FORWARD = 3, R_ACTIVATE = 4, R_CLOSE = 5, R_SHOWN = 6;

    private int[] ids = new int[0], rows = new int[0], cols = new int[0], kinds = new int[0];
    private int at = -1;
    private int want = -1;
    private float x = -1;
    private boolean shown;


    void setCells(int[][] cells) {
        int keep = at >= 0 ? ids[at] : want, n = cells.length;
        ids = new int[n]; rows = new int[n]; cols = new int[n]; kinds = new int[n];
        for (int i = 0; i < n; i++) { ids[i] = cells[i][0]; rows[i] = cells[i][1]; cols[i] = cells[i][2]; kinds[i] = cells[i][3]; }
        at = indexOf(keep);
        if (at < 0) at = indexOf(want);
        if (at < 0 && n > 0) at = 0;
    }


    void open(boolean padOpened) {
        at = indexOf(want);
        if (at < 0 && ids.length > 0) at = 0;
        x = -1;
        shown = padOpened && at >= 0;
    }

    void hide() { shown = false; }
    boolean shown() { return shown && at >= 0; }
    int focused() { return at >= 0 ? ids[at] : -1; }
    int kind() { return at >= 0 ? kinds[at] : ACTION; }

    int handle(int cmd) {
        if (cmd == PadNav.B || cmd == PadNav.MENU) return R_CLOSE;
        if (at < 0) return R_NONE;
        if (!shown) {
            if (cmd >= PadNav.UP && cmd <= PadNav.RIGHT || cmd == PadNav.A) { shown = true; return R_SHOWN; }
            return R_NONE;
        }
        switch (cmd) {
            case PadNav.UP: return moveRow(-1);
            case PadNav.DOWN: return moveRow(1);
            case PadNav.LEFT: case PadNav.RIGHT: {
                int step = cmd == PadNav.LEFT ? -1 : 1;
                if (kinds[at] == VALUE) { want = ids[at]; return step < 0 ? R_STEP_BACK : R_STEP_FORWARD; }
                int next = neighbour(step);
                if (next < 0) return R_NONE;
                at = next; x = center(at);
                return R_MOVED;
            }
            case PadNav.A: want = ids[at]; return kinds[at] == VALUE ? R_STEP_FORWARD : R_ACTIVATE;
            default: return R_NONE;
        }
    }

    private int moveRow(int step) {
        int row = rows[at], target = Integer.MIN_VALUE;
        for (int i = 0; i < ids.length; i++) {
            int r = rows[i];
            if (step < 0 ? r < row && (target == Integer.MIN_VALUE || r > target) : r > row && (target == Integer.MIN_VALUE || r < target)) target = r;
        }
        if (target == Integer.MIN_VALUE) return R_NONE;
        float from = x >= 0 ? x : center(at);
        int best = -1; float dist = Float.MAX_VALUE;
        for (int i = 0; i < ids.length; i++) {
            if (rows[i] != target) continue;
            float d = Math.abs(center(i) - from);
            if (d < dist - 1e-4f) { dist = d; best = i; }
        }
        if (x < 0) x = from;
        at = best;
        return R_MOVED;
    }


    private float center(int i) {
        int n = 0, k = 0;
        for (int j = 0; j < ids.length; j++) if (rows[j] == rows[i]) { if (cols[j] < cols[i]) k++; n++; }
        return (k + 0.5f) / n;
    }

    private int indexOf(int id) { for (int i = 0; i < ids.length; i++) if (ids[i] == id) return i; return -1; }

    private int neighbour(int step) {
        int best = -1;
        for (int i = 0; i < ids.length; i++) {
            if (rows[i] != rows[at] || (cols[i] - cols[at]) * step <= 0) continue;
            if (best < 0 || Math.abs(cols[i] - cols[at]) < Math.abs(cols[best] - cols[at])) best = i;
        }
        return best;
    }
}
