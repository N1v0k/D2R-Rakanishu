package com.xiaoboluo.d2r;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.view.Choreographer;
import android.view.MotionEvent;
import android.view.View;









final class CamButtons extends View implements Choreographer.FrameCallback {
    private static final float RATE = 1.8f;



    private static final float LEFT_X = 0.07f, RIGHT_X = 0.18f, Y = 0.42f, R = 0.06f;
    private final Paint fill = new Paint(Paint.ANTI_ALIAS_FLAG), ring = new Paint(Paint.ANTI_ALIAS_FLAG), glyph = new Paint(Paint.ANTI_ALIAS_FLAG);
    private int leftId = -1, rightId = -1;
    private boolean shown, running;
    private long lastNs;

    CamButtons(Context context) {
        super(context);
        fill.setStyle(Paint.Style.FILL);
        ring.setStyle(Paint.Style.STROKE);
        ring.setColor(0xB0FFFFFF);
        glyph.setColor(0xF0FFFFFF);
        glyph.setTextAlign(Paint.Align.CENTER);
    }

    void start() { if (!running) { running = true; lastNs = 0; Choreographer.getInstance().postFrameCallback(this); } }
    void stop() { running = false; leftId = rightId = -1; invalidate(); }

    @Override public void doFrame(long ns) {
        if (!running) return;


        boolean active = DmActivity.nativeCamActive() != 0 && DmActivity.nativeCamStyle() != 1;
        if (active != shown) { shown = active; if (!active) leftId = rightId = -1; invalidate(); }
        int dir = (rightId >= 0 ? 1 : 0) - (leftId >= 0 ? 1 : 0);
        if (dir != 0 && lastNs != 0) DmActivity.nativeCamTurn(dir * RATE * Math.min(0.1f, (ns - lastNs) * 1e-9f));
        lastNs = ns;
        Choreographer.getInstance().postFrameCallback(this);
    }

    private float cx(int i) { return getWidth() * (i == 0 ? LEFT_X : RIGHT_X); }
    private float cy() { return getHeight() * Y; }
    private float r() { return getHeight() * R; }


    private int hit(float x, float y) {
        for (int i = 0; i < 2; i++) { float dx = x - cx(i), dy = y - cy(); if (dx * dx + dy * dy <= r() * r() * 1.69f) return i; }
        return -1;
    }

    @Override protected void onDraw(Canvas canvas) {
        if (!shown) return;
        float r = r();
        ring.setStrokeWidth(r * 0.08f);
        glyph.setTextSize(r * 1.2f);
        for (int i = 0; i < 2; i++) {
            boolean held = i == 0 ? leftId >= 0 : rightId >= 0;
            fill.setColor(held ? 0xA0FFFFFF : 0x60000000);
            canvas.drawCircle(cx(i), cy(), r, fill);
            canvas.drawCircle(cx(i), cy(), r, ring);
            canvas.drawText(i == 0 ? "⟲" : "⟳", cx(i), cy() + r * 0.42f, glyph);
        }
    }

    @Override public boolean onTouchEvent(MotionEvent e) {
        if (!shown) return false;
        int action = e.getActionMasked(), index = e.getActionIndex(), id = e.getPointerId(index);
        switch (action) {
            case MotionEvent.ACTION_DOWN:
            case MotionEvent.ACTION_POINTER_DOWN: {
                int b = hit(e.getX(index), e.getY(index));
                if (b < 0) return action != MotionEvent.ACTION_DOWN;
                if (b == 0) leftId = id; else rightId = id;
                invalidate();
                return true;
            }
            case MotionEvent.ACTION_UP:
            case MotionEvent.ACTION_POINTER_UP:
                if (id == leftId) leftId = -1;
                if (id == rightId) rightId = -1;
                invalidate();
                return true;
            case MotionEvent.ACTION_CANCEL:
                leftId = rightId = -1;
                invalidate();
                return true;
            default:
                return true;
        }
    }
}
