package com.xiaoboluo.d2r;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.RectF;
import android.view.Choreographer;
import android.view.MotionEvent;
import android.view.View;
import java.util.Arrays;









final class PotionButtons extends View implements Choreographer.FrameCallback {
    private static final int MENU = 3, COUNT = 4, DIFF_TARGET = 14;

    private static final float[] DX = {-156, -146, -120}, DY = {10, -60, -118};
    private static final float RADIUS = 24;
    private static final String[] PREFIX = {"hp", "mp", "rv"};
    private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG | Paint.FILTER_BITMAP_FLAG);
    private final TouchOverlay overlay;
    private final AppArt art;
    private final GameStyle style;
    private final char[] number = new char[3];
    static final float CELL = 0.8f;
    private final Runnable openMenu;
    private final float[] centre = new float[3];
    private final int[] finger = {-1, -1, -1, -1};
    private boolean shown, running;
    private int kinds = -1;

    PotionButtons(Context context, TouchOverlay overlay, AppArt art, GameStyle style, Runnable openMenu) {
        super(context);
        this.overlay = overlay;
        this.art = art;
        this.style = style;
        this.openMenu = openMenu;
    }

    void start() { if (!running) { running = true; Choreographer.getInstance().postFrameCallback(this); } }
    void stop() { running = false; Choreographer.getInstance().removeFrameCallback(this); Arrays.fill(finger, -1); invalidate(); }

    @Override public void doFrame(long ns) {
        if (!running) return;
        boolean active = overlay.hudShown();
        int k = active ? DmActivity.nativeBeltKinds() : 0;
        if (active != shown || k != kinds) { shown = active; kinds = k; if (!active) Arrays.fill(finger, -1); invalidate(); }

        else if (active && (place(0) ? centre[0] != anchor[0] || centre[1] != anchor[1] || centre[2] != anchor[2] : anchor[2] != 0)) invalidate();
        if (active && place(0)) System.arraycopy(centre, 0, anchor, 0, 3); else anchor[2] = 0;
        Choreographer.getInstance().postFrameCallback(this);
    }

    private final float[] anchor = new float[3];
    private int count(int i) { return kinds >> (i * 8) & 0xff; }
    private int tier(int i) { return kinds >>> 24 >> (i * 3) & (i == 2 ? 3 : 7); }
    private boolean visible(int i) { return i != 2 || count(2) > 0; }

    private boolean place(int i) {
        if (i == MENU) return overlay.targetBox(DIFF_TARGET, centre);
        if (!overlay.targetCentre(0, centre)) return false;
        float k = centre[2];
        centre[0] += DX[i] * k; centre[1] += DY[i] * k; centre[2] = RADIUS * k;
        return true;
    }

    private int hit(float x, float y) {
        for (int i = 0; i < COUNT; i++) {
            if (!visible(i) || !place(i)) continue;
            float dx = x - centre[0], dy = y - centre[1], r = centre[2] * 1.1f;
            if (dx * dx + dy * dy <= r * r) return i;
        }
        return -1;
    }

    private void bitmap(Canvas c, Bitmap b, float x, float y, float r, int alpha) {
        if (b == null) return;
        float w = b.getWidth() >= b.getHeight() ? r : r * b.getWidth() / b.getHeight(), h = w * b.getHeight() / b.getWidth();
        paint.setAlpha(alpha);
        c.drawBitmap(b, null, new RectF(x - w, y - h, x + w, y + h), paint);
    }



    int circles(float[] out, int at) {
        if (!shown) return at;
        for (int i = 0; i < MENU && at + 3 <= out.length; i++) if (visible(i) && place(i)) { System.arraycopy(centre, 0, out, at, 3); at += 3; }
        return at;
    }

    @Override protected void onDraw(Canvas canvas) {
        if (BuildConfig.DM_QA_BUILD) report();
        if (!shown) return;
        Bitmap socket = overlay.overlayImage("touchart/dm_mobile_socket.png");
        for (int i = 0; i < COUNT; i++) {
            if (!visible(i) || !place(i)) continue;
            float x = centre[0], y = centre[1], r = centre[2];
            if (i == MENU) { bitmap(canvas, art.image(finger[i] >= 0 ? "menu_icon_down" : "menu_icon"), x, y, r * 0.62f, 255); continue; }
            bitmap(canvas, socket, x, y, r, 255);
            int n = count(i);
            if (n > 0) bitmap(canvas, art.image(PREFIX[i] + tier(i)), x, y, r * (finger[i] >= 0 ? 0.62f : 0.72f), 255);
            int len = digits(n);
            style.cornerNumber(canvas, number, 0, len, x - CELL * r, y - CELL * r, x + CELL * r, y + CELL * r, countPx(2 * r), "FontColorWhite");
        }
    }



    private final float[] qaCircles = new float[3 * MENU];
    private final int[] qaAt = new int[2];
    private void report() {
        int n = circles(qaCircles, 0);
        StringBuilder b = new StringBuilder();
        if (n > 0) getLocationInWindow(qaAt);
        for (int i = 0; i < n; i += 3) b.append(b.length() > 0 ? "_" : "").append(Math.round(qaAt[0] + qaCircles[i])).append('_').append(Math.round(qaAt[1] + qaCircles[i + 1])).append('_').append(Math.round(qaCircles[i + 2]));
        TestHooks.overlay(this, "potion_buttons", n > 0 ? b.toString() : "none");
    }



    private float countPx(float field) { return style.counterPx(field, GameStyleGeometry.k(getHeight()) * style.data().scale(GameStyleData.PROFILE)); }



    private int digits(int v) { return GameStyleGeometry.digits(Math.max(0, Math.min(999, v)), number); }

    @Override public boolean onTouchEvent(MotionEvent e) {
        if (!shown) return false;
        int action = e.getActionMasked(), index = e.getActionIndex(), id = e.getPointerId(index);
        switch (action) {
            case MotionEvent.ACTION_DOWN:
            case MotionEvent.ACTION_POINTER_DOWN: {
                int b = hit(e.getX(index), e.getY(index));
                if (b < 0) return action != MotionEvent.ACTION_DOWN;
                finger[b] = id;
                if (b != MENU) DmActivity.nativeBeltDrink(b + 1);
                invalidate();
                return true;
            }
            case MotionEvent.ACTION_UP:
            case MotionEvent.ACTION_POINTER_UP:
                if (finger[MENU] == id) openMenu.run();
                for (int i = 0; i < COUNT; i++) if (finger[i] == id) finger[i] = -1;
                invalidate();
                return true;
            case MotionEvent.ACTION_CANCEL:
                Arrays.fill(finger, -1);
                invalidate();
                return true;
            default:
                return true;
        }
    }
}
