package com.xiaoboluo.d2r;

import java.nio.charset.StandardCharsets;
import java.util.Collections;
import java.util.List;
import java.util.function.IntFunction;
import java.util.function.Supplier;










final class RecipePreview {
    static final long HOLD_MS = 400, BARE_POLL_MS = 250, POLL_LIMIT_MS = 10000;
    static final int WAIT = 0, BARE = 1, FULL = 2;
    static final long NEXT_FRAME = 0, STOP = -1;




    private Object key;
    private long since;
    private String raw;
    private List<String> lines;

    void reset() { key = null; raw = null; lines = null; }



    int state(Object k, long now, Supplier<byte[]> text) {
        if (k != key) { key = k; since = now; raw = null; lines = null; }
        if (raw == null) {
            byte[] t = text.get();
            if (t != null) raw = new String(t, StandardCharsets.UTF_8);
        }
        if (raw != null) return FULL;
        return now - since < HOLD_MS ? WAIT : BARE;
    }


    String raw(Object k) { return k == key ? raw : null; }



    int state(RecipeModel.Card c, long now, IntFunction<byte[]> text) {
        if (c.cat != null) { if (c != key) { key = c; since = now; raw = null; lines = null; } return FULL; }
        return state(c, now, () -> text.apply(c.row));
    }


    List<String> lines(RecipeModel.Card c) {
        if (c.cat != null) return c.preview;
        if (c != key || raw == null) return Collections.<String>emptyList();
        if (lines == null) lines = RecipeModel.gameLines(raw, c.title);
        return lines;
    }


    long nextPoll(int state, long now) {
        if (state == FULL || key == null) return STOP;
        if (state == WAIT) return NEXT_FRAME;
        return now - since < POLL_LIMIT_MS ? BARE_POLL_MS : STOP;
    }
}
