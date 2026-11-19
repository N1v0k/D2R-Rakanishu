package com.xiaoboluo.d2r;

import android.view.View;
import java.util.HashMap;

final class TestHooks {
    private TestHooks() {}
    static native int nativeObservationEnabled();
    static native int nativeEnabled();
    static native void nativeOverlay(String name, String state, boolean drawn);

    private static int on = -1;
    private static final HashMap<String, String> sent = new HashMap<>();
    private static final HashMap<String, Integer> generation = new HashMap<>();

    private static boolean enabled() {
        if (on >= 0) return on == 1;
        if (!BuildConfig.DEBUG) { on = 0; return false; }

        if (!DmActivity.hostStarted()) return false;
        int v;
        try { v = nativeEnabled(); } catch (UnsatisfiedLinkError e) { on = 0; return false; }
        if (v < 0) return false;
        on = v;
        if (on == 1) overlay("pad_layout", PadInput.isPadMode() ? "pad" : "touch");
        return on == 1;
    }

    private static boolean send(String name, String state, boolean drawn) {
        try { nativeOverlay(name, state, drawn); return true; } catch (UnsatisfiedLinkError e) { on = 0; return false; }
    }

    static void overlay(String name, String state) { overlay(null, name, state); }




    static void overlay(View view, String name, String state) {
        if (!enabled() || state.equals(sent.get(name)) || !send(name, state, false)) return;
        sent.put(name, state);
        final int gen = generation.merge(name, 1, Integer::sum);
        if (view == null || view.getVisibility() != View.VISIBLE) return;
        view.getViewTreeObserver().registerFrameCommitCallback(() -> {
            if (on == 1 && gen == generation.getOrDefault(name, 0)) send(name, state, true);
        });
    }
    private static int observe = -1;
    private static boolean observing() {
        if (!enabled()) return false;
        if (observe < 0) {
            try { observe = nativeObservationEnabled(); }
            catch (UnsatisfiedLinkError e) { observe = 0; }
        }
        return observe == 1;
    }
    static void text(android.graphics.Canvas c, char[] s, int off, int len, float x, float y, android.graphics.Paint p) { if (observing()) UiObservation.text(c, new String(s, off, len), x, y, p); }
    static void begin(View view, String page) { if (observing()) UiObservation.begin(page); }
    static void text(android.graphics.Canvas c, CharSequence s, float x, float y, android.graphics.Paint p) { if (observing()) UiObservation.text(c, s, x, y, p); }
    static void end(View view) { if (observing()) UiObservation.end(view); }
    static void foreground() { if (observing()) UiObservation.foreground(); }
    static void hit(android.graphics.RectF rect, int kind, int index) { if (observing()) UiObservation.hit(rect, kind, index); }
}
