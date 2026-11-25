package com.xiaoboluo.d2r;

import java.util.ArrayDeque;






final class FeatQueue {
    static final long SHOW_NS = 4_000_000_000L, FADE_NS = 500_000_000L;
    static final int SEQ = 0, NOTES = 1, KEPT = 8, N = 1 + 3 * KEPT;

    static final class Note {
        final int seq, id, arg;
        Note(int seq, int id, int arg) { this.seq = seq; this.id = id; this.arg = arg; }
    }

    private final ArrayDeque<Note> waiting = new ArrayDeque<>();
    private int seen = -1;
    private Note shown;
    private long visibleNs, lastNs;
    private boolean wasVisible;



    void take(int[] hud, int n) {
        if (n < N) return;
        int newest = hud[SEQ];
        if (seen < 0) { seen = newest; return; }
        for (int q = Math.max(seen + 1, newest - KEPT + 1); q <= newest; q++) {
            int k = NOTES + 3 * (q % KEPT);
            if (hud[k] == q) waiting.add(new Note(q, hud[k + 1], hud[k + 2]));
        }
        if (newest > seen) seen = newest;
    }



    Note at(long ns, boolean visible) {
        if (shown != null && visible && wasVisible) visibleNs += Math.max(0, ns - lastNs);
        wasVisible = visible; lastNs = ns;
        if (shown != null && visibleNs >= SHOW_NS + FADE_NS) shown = null;
        if (shown == null && visible && !waiting.isEmpty()) { shown = waiting.poll(); visibleNs = 0; }
        return visible ? shown : null;
    }

    void hidden() { wasVisible = false; }

    private Note built;
    private boolean relang;



    Note textsDue(Note now, boolean langChanged) {
        relang |= langChanged;
        if (now == null || (now == built && !relang)) return null;
        built = now; relang = false;
        return now;
    }

    int alpha() {
        if (shown == null) return 0;
        return visibleNs < SHOW_NS ? 255 : (int) Math.max(0, 255 * (SHOW_NS + FADE_NS - visibleNs) / FADE_NS);
    }
}
