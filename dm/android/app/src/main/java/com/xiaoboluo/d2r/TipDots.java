package com.xiaoboluo.d2r;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.RectF;
import android.view.Choreographer;
import android.view.View;
import java.util.Arrays;





final class TipDots extends View implements Choreographer.FrameCallback {
    static final int BOX = 10;
    static final int HIT_BOX = 1, HIT_BLOCK = 2;
    private final GameStyle style;
    private final DotRows rows;
    private final float[] box = new float[BOX], drawn = new float[BOX], view = new float[4], seen = new float[4];
    private final int[] origin = new int[2];
    private final RectF place = new RectF(), tip = new RectF();
    private final float[] lineL = new float[2], lineR = new float[2];
    private int code;
    private boolean running;
    private long wordsAt;
    private DotLayout layout;
    private String state = "";

    TipDots(Context context, GameStyle style) {
        super(context);
        this.style = style;
        rows = new DotRows(style.rowGlyphs());
        setClickable(false);
        setFocusable(false);
    }

    void start() {
        if (!running) { running = true; wordsAt = 0; report(); Choreographer.getInstance().postFrameCallback(this); }
    }
    void stop() { running = false; Choreographer.getInstance().removeFrameCallback(this); if (code != 0) { code = 0; layout = null; invalidate(); } report(); }


    private boolean words(long now) {
        if (now - wordsAt < 1_000_000_000L) return false;
        wordsAt = now;
        return rows.words(getContext());
    }

    @Override public void doFrame(long ns) {
        if (!running) return;
        int n = DmActivity.nativeTipDots(box);

        view[0] = x(0); view[1] = y(0); view[2] = x(1); view[3] = y(1);
        boolean changed = (words(ns) || !Arrays.equals(view, seen)) && n != 0;
        System.arraycopy(view, 0, seen, 0, 4);
        if (n != code || (n != 0 && !Arrays.equals(box, drawn)) || changed) {
            code = n; System.arraycopy(box, 0, drawn, 0, BOX);
            layout = n == 0 ? null : place();
            invalidate(); report();
        }
        Choreographer.getInstance().postFrameCallback(this);
    }



    private float uiH() { GameRect g = GameRect.ui(); return g.whole() ? getHeight() : g.uiH(); }
    private float x(float n) { GameRect g = GameRect.ui(); return g.whole() ? n * getWidth() : g.pictureAtX(n); }
    private float y(float n) { GameRect g = GameRect.ui(); return g.whole() ? n * getHeight() : g.pictureAtY(n); }
    private DotLayout place() {
        if (!rows.ready()) return null;
        tip.set(x(drawn[0]), y(drawn[1]), x(drawn[2]), y(drawn[3]));
        float lineH = y(drawn[1] + drawn[4]) - y(drawn[1]), middle = tip.centerX();
        float px = Math.min(lineH, rows.units() * GameStyleGeometry.k(uiH()) * style.data().scale(GameStyleData.PROFILE));
        for (int i = 0; i < 2; i++) {
            float half = (x(drawn[0] + drawn[7 + i]) - tip.left) / 2;
            if (half > 0) { lineL[i] = middle - half; lineR[i] = middle + half; } else { lineL[i] = 1; lineR[i] = 0; }
        }
        return rows.layout(code, lineH, px).inTooltip(tip.left, tip.top, tip.right, tip.bottom, x(drawn[0] + drawn[6]) - tip.left, y(drawn[1] + drawn[5]),
            lineL, lineR, getWidth(), getHeight(), drawn[9] > 0.5f);
    }




    private void report() {
        StringBuilder s = new StringBuilder();
        if (code != 0 && running && layout != null && layout.where != DotLayout.NONE) {
            getLocationOnScreen(origin);
            for (int r = 0; r < 2; r++) {
                int v = DotLayout.row(code, r);
                if (v == 0) continue;
                rows.dots(code, r, layout, place);
                if (s.length() > 0) s.append('_');
                s.append(r == 0 ? 'd' : 's').append(v > 4 ? 'g' : v < 4 ? 'r' : 'n').append(DotLayout.dots(v))
                    .append('_').append(Math.round(origin[0] + place.left)).append('_').append(Math.round(origin[1] + place.top))
                    .append('_').append(Math.round(origin[0] + place.right)).append('_').append(Math.round(origin[1] + place.bottom));
            }
            s.append(layout.where == DotLayout.LEFT ? "_left" : layout.where == DotLayout.RIGHT ? "_right" : "_outside");
        }
        String t = s.length() == 0 ? "none" : s.toString();
        if (!t.equals(state)) { state = t; TestHooks.overlay(this, "tip_dots", t); }
    }

    @Override protected void onDraw(Canvas c) {
        if (code == 0 || layout == null) return;
        if (BuildConfig.DM_QA_BUILD) TestHooks.begin(this, "tip_dots");
        try {
            rows.draw(c, code, layout);
            if (BuildConfig.DM_QA_BUILD) { TestHooks.hit(tip, HIT_BOX, 0); TestHooks.hit(rows.block(layout, place), HIT_BLOCK, 0); }
        } finally { if (BuildConfig.DM_QA_BUILD) TestHooks.end(this); }
    }
}
