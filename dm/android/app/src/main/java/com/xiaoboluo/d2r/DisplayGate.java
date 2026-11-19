package com.xiaoboluo.d2r;








final class DisplayGate {
    static final int SHOW = 1, BUSY = 2, SHOW_AFTER = 3;
    private long frame;
    private boolean framed, shown;
    private int calm;

    boolean seen(long frameNanos) { return framed && frame == frameNanos; }
    boolean shown() { return shown; }


    boolean advance(long frameNanos, int state) {
        if (seen(frameNanos)) return shown;
        framed = true; frame = frameNanos;
        if ((state & SHOW) == 0) { shown = false; calm = 0; }
        else if ((state & BUSY) != 0) calm = 0;
        else if (!shown && ++calm >= SHOW_AFTER) shown = true;
        return shown;
    }


    void reset() { framed = false; shown = false; calm = 0; }
}
