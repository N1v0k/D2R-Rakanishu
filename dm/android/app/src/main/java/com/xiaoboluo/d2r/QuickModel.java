







package com.xiaoboluo.d2r;

final class QuickModel {
    static final int PAUSE = 0, RECIPES = 1, BUILDS = 2, PLAYERS = 3, CLOSE = 4, GLOSSARY = 5, COMBAT = 6;
    static final int ACTION = 0, PAGE = 1, VALUE = 2;
    static final int MIN_PLAYERS = 1, MAX_PLAYERS = 8;

    static final int TAP_NONE = -1, TAP_CLOSE = -2;

    private int[] ids = {PAUSE, BUILDS, PLAYERS};
    private int down = QuickLayout.NEUTRAL, held = QuickLayout.NEUTRAL;
    private boolean cancelled;


    void setEntries(boolean recipes, boolean builds) { setEntries(recipes, builds, false); }
    void setEntries(boolean recipes, boolean builds, boolean glossary) { setEntries(recipes, builds, glossary, false); }
    void setEntries(boolean recipes, boolean builds, boolean glossary, boolean combat) {
        int n = 2 + (recipes ? 1 : 0) + (builds ? 1 : 0) + (glossary ? 1 : 0) + (combat ? 1 : 0), k = 0;
        int[] a = new int[n];
        a[k++] = PAUSE;
        if (recipes) a[k++] = RECIPES;
        if (builds) a[k++] = BUILDS;
        if (glossary) a[k++] = GLOSSARY;
        if (combat) a[k++] = COMBAT;
        a[k] = PLAYERS;
        ids = a;
    }
    int count() { return ids.length; }
    int id(int index) { return ids[index]; }
    int indexOf(int id) { for (int i = 0; i < ids.length; i++) if (ids[i] == id) return i; return -1; }
    static int kind(int id) { return id == PLAYERS ? VALUE : id == RECIPES || id == BUILDS || id == GLOSSARY || id == COMBAT ? PAGE : ACTION; }


    static int players(int want, int dir) {
        int w = Math.max(MIN_PLAYERS, Math.min(MAX_PLAYERS, want));
        if (dir == 0) return w % MAX_PLAYERS + 1;
        return Math.max(MIN_PLAYERS, Math.min(MAX_PLAYERS, w + dir));
    }
    static boolean atEnd(int want, int dir) { return dir < 0 ? want <= MIN_PLAYERS : want >= MAX_PLAYERS; }


    private boolean same(int a, int b) {
        if (a < 0 || b < 0) return a == b;
        if (a / 2 != b / 2) return false;
        return kind(ids[a / 2]) != VALUE || a == b;
    }

    void press(int hit) { down = held = hit; cancelled = false; }

    void move(int hit) { if (!cancelled && down >= 0 && !same(down, hit)) { cancelled = true; held = QuickLayout.NEUTRAL; } }
    int release(int hit) {
        int d = down; boolean c = cancelled;
        down = held = QuickLayout.NEUTRAL; cancelled = false;
        if ((d == QuickLayout.OUTSIDE || d == QuickLayout.CLOSE) && hit == d) return TAP_CLOSE;
        if (c || d < 0 || !same(d, hit)) return TAP_NONE;
        return ids[d / 2] * 2 + hit % 2;
    }
    void cancel() { down = held = QuickLayout.NEUTRAL; cancelled = false; }

    int heldIndex() { return held >= 0 ? held / 2 : -1; }
    int heldHalf() { return held >= 0 ? held % 2 : -1; }
}
