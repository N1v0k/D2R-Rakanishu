package com.xiaoboluo.d2r;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.view.Choreographer;
import android.view.MotionEvent;
import android.view.View;















final class DamageNumbers extends View implements Choreographer.FrameCallback {
    private static final int F = DamageMotion.FIELDS, MAX = DamageMotion.MAX, DIGITS = 7;

    private static final String[] COLOUR = {"FontColorWhite", "FontColorRed", "FontColorBlue", "FontColorYellow", "FontColorGreen", "FontColorOrange"};
    private final TouchOverlay overlay;
    private final GameText text, crit;
    private final int[] colour = new int[COLOUR.length];
    private final int white;
    private final float[] data = new float[MAX * F];
    private final char[] digits = new char[MAX * DIGITS];
    private final int[] from = new int[MAX];
    private final float[] width = new float[MAX], at = new float[3];
    private final DamageMotion motion = new DamageMotion();
    private int count;
    private boolean running;

    DamageNumbers(Context context, TouchOverlay overlay, GameStyle style) {
        super(context);
        this.overlay = overlay;
        text = style.text(GameStyleData.WORLD_NUMBER);
        crit = style.text(GameStyleData.WORLD_CRIT);
        text.prepare(); crit.prepare();
        for (int e = 0; e < COLOUR.length; e++) colour[e] = style.colour(COLOUR[e]);
        white = style.colour("FontColorWhite");
    }

    void start() { if (!running) { running = true; Choreographer.getInstance().postFrameCallback(this); } }
    void stop() { running = false; count = 0; Choreographer.getInstance().removeFrameCallback(this); invalidate(); }

    @Override public void doFrame(long ns) {
        if (!running) return;
        int n = DmActivity.nativeDamageNumbers(data);
        if (n > 0 && !overlay.displaysShown(ns)) n = 0;
        if (n != count || n > 0) { count = n; invalidate(); }
        Choreographer.getInstance().postFrameCallback(this);
    }

    @Override public boolean onTouchEvent(MotionEvent e) { return false; }

    @Override protected void onDraw(Canvas canvas) {
        if (BuildConfig.DM_QA_BUILD) TestHooks.begin(this, "damage_numbers");
        try {
        if (BuildConfig.DM_QA_BUILD) TestHooks.overlay(this, "damage_numbers", count > 0 && text.ready() ? "visible" : "empty");
        if (count <= 0 || !text.ready()) return;

        GameRect g = GameRect.ui(); boolean whole = g.whole();
        float w = whole ? getWidth() : g.pictureW(), h = whole ? getHeight() : g.pictureH(), size = (whole ? getHeight() : g.uiH()) * DamageMotion.TEXT;
        canvas.save(); if (!whole) canvas.translate(g.pictureX(), g.pictureY());

        for (int i = 0; i < count; i++) {
            int value = Math.max(0, (int) data[i * F + 6]), end = (i + 1) * DIGITS, k = end;
            do { digits[--k] = (char) ('0' + value % 10); value /= 10; } while (value > 0 && k > i * DIGITS);
            from[i] = k;
            int f = DamageMotion.flags(data, i);
            width[i] = kind(f).measurePx(digits, k, end - k, size * DamageMotion.scale(f));
        }


        motion.track(data, count, width, size, w, h);
        for (int i = 0; i < count; i++) {
            int o = i * F, f = DamageMotion.flags(data, i), kind = (int) data[o + 5], len = (i + 1) * DIGITS - from[i];
            motion.place(data, i, size, w, h, at);
            GameText t = kind(f);
            t.colourArgb((f & DamageMotion.F_WHITE) != 0 || kind < 0 || kind >= colour.length ? white : colour[kind]);
            t.drawPx(canvas, digits, from[i], len, at[0], at[1], at[2], Paint.Align.CENTER, DamageMotion.alpha(data[o + 2]));
        }
        canvas.restore();

        } finally { if (BuildConfig.DM_QA_BUILD) TestHooks.end(this); }
    }

    private GameText kind(int flags) { return (flags & DamageMotion.F_CRIT) != 0 ? crit : text; }
}
