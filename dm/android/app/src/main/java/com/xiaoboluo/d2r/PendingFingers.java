package com.xiaoboluo.d2r;

import java.util.Arrays;






final class PendingFingers {
    static final int NONE = 0;
    private static final int FREE = 0, PENDING = 1, VOID = 2;
    private final int[] state, target;
    private final float[] x, y;

    PendingFingers(int slots) { state = new int[slots]; target = new int[slots]; x = new float[slots]; y = new float[slots]; }

    void land(int slot, int t, float px, float py) { state[slot] = PENDING; target[slot] = t; x[slot] = px; y[slot] = py; }
    boolean pending(int slot) { return slot >= 0 && slot < state.length && state[slot] == PENDING; }

    boolean held(int slot) { return slot >= 0 && slot < state.length && state[slot] != FREE; }
    int target(int slot) { return pending(slot) ? target[slot] : NONE; }
    float landX(int slot) { return x[slot]; }
    float landY(int slot) { return y[slot]; }



    int move(int slot, float px, float py, float slop) {
        if (!pending(slot)) return NONE;
        float dx = px - x[slot], dy = py - y[slot];
        if (dx * dx + dy * dy <= slop * slop) return NONE;
        int t = target[slot]; state[slot] = FREE; target[slot] = NONE;
        return t;
    }



    int up(int slot, float px, float py, float slop) {
        if (!held(slot)) return NONE;
        float dx = px - x[slot], dy = py - y[slot];
        int t = state[slot] == PENDING && dx * dx + dy * dy <= slop * slop ? target[slot] : NONE;
        state[slot] = FREE; target[slot] = NONE;
        return t;
    }


    int cancel(int slot) {
        if (!held(slot)) return NONE;
        int t = target(slot); state[slot] = FREE; target[slot] = NONE;
        return t;
    }


    int voidAll() {
        int n = 0;
        for (int i = 0; i < state.length; i++) if (state[i] == PENDING) { state[i] = VOID; target[i] = NONE; n++; }
        return n;
    }

    void clear() { Arrays.fill(state, FREE); Arrays.fill(target, NONE); }
}
