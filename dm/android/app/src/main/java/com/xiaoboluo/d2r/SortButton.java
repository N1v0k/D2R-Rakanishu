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






final class SortButton extends View implements Choreographer.FrameCallback {
    private static final int FIELDS = 3, MAX = 2, STASH = 4, BEST_ONLY = 5;
    private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG | Paint.FILTER_BITMAP_FLAG), label = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final AppArt art;
    private String text = "", bestText = "";
    private final Runnable openBest;
    private final GameLang.Watch lang = new GameLang.Watch();
    private final float[] data = new float[MAX * FIELDS], drawn = new float[MAX * FIELDS];
    private final RectF box = new RectF(), other = new RectF();
    private int count, down = -1;
    private boolean running;

    SortButton(Context context, AppArt art, Runnable openBest) {
        super(context);
        this.art = art;
        this.openBest = openBest;
        label.setColor(0xFFE8DCC0);
        label.setTextAlign(Paint.Align.CENTER);
    }

    void start() { if (!running) { running = true; Choreographer.getInstance().postFrameCallback(this); } }
    void stop() { running = false; Choreographer.getInstance().removeFrameCallback(this); down = -1; }

    @Override public void doFrame(long ns) {
        if (!running) return;
        int n = PadInput.isPadMode() ? 0 : DmActivity.nativeSortButtons(data);
        if (n != count || !Arrays.equals(data, drawn)) { count = n; System.arraycopy(data, 0, drawn, 0, data.length); if (n == 0) down = -1; invalidate(); }
        if (BuildConfig.DM_QA_BUILD) TestHooks.overlay(this, "sort_buttons", n > 0 ? "open" : "closed");
        Choreographer.getInstance().postFrameCallback(this);
    }


    private float uiH() { GameRect g = GameRect.ui(); return g.whole() ? getHeight() : g.uiH(); }

    private boolean place(int i) { return place(i, box); }
    private boolean place(int i, RectF out) {
        int k = i % MAX, page = k < count ? (int) drawn[k * FIELDS] : -1;
        if (page < 0 || (i < MAX ? page == BEST_ONLY : page != STASH && page != BEST_ONLY)) return false;
        GameRect g = GameRect.ui(); boolean whole = g.whole();
        float nx = drawn[k * FIELDS + 1], ny = drawn[k * FIELDS + 2];
        float h = uiH() * 0.055f, w = h * 3.1f, x = whole ? nx * getWidth() : g.pictureAtX(nx), y = whole ? ny * getHeight() : g.pictureAtY(ny);
        out.set(x - w / 2, y - h / 2, x + w / 2, y + h / 2);
        if (i < MAX) return true;
        label.setTextSize(uiH() * 0.024f);
        float bw = Math.max(w, label.measureText(bestText) + h * 1.2f), gap = h * 0.4f, left = out.left, right = out.right;
        if (page == BEST_ONLY) { out.set(x - bw / 2, y - h / 2, x + bw / 2, y + h / 2); return fits(out, k); }
        out.set(right + gap, y - h / 2, right + gap + bw, y + h / 2);
        if (fits(out, k)) return true;
        out.set(left - gap - bw, y - h / 2, left - gap, y + h / 2);
        return fits(out, k);
    }

    private boolean fits(RectF r, int self) {
        if (r.left < 0 || r.right > getWidth()) return false;
        for (int j = 0; j < count; j++) if (j != self && place(j, other) && RectF.intersects(r, other)) return false;
        return true;
    }

    private int hit(float x, float y) {
        for (int i = 0; i < 2 * MAX; i++) if (place(i) && box.contains(x, y)) return i;
        return -1;
    }

    @Override protected void onDraw(Canvas c) {
        if (lang.changed()) { text = ShoulderKeys.german() ? "Sortieren" : "Sort"; bestText = AppText.get(getContext(), R.string.dmAndroidBestTab); label.setTypeface(art.role(AppArt.EXOCET)); }
        if (BuildConfig.DM_QA_BUILD) TestHooks.begin(this, "sort_buttons");
        try {
            for (int i = 0; i < 2 * MAX; i++) {
                if (!place(i)) continue;
                label.setTextSize(uiH() * 0.024f);
                Bitmap b = art.image(i == down ? "button_down" : "button");
                if (b != null) c.drawBitmap(b, null, box, paint);
                String s = i < MAX ? text : bestText;
                float ty = box.centerY() + label.getTextSize() * 0.36f;
                c.drawText(s, box.centerX(), ty, label);
                if (BuildConfig.DM_QA_BUILD) TestHooks.text(c, s, box.centerX(), ty, label);
            }
        } finally { if (BuildConfig.DM_QA_BUILD) TestHooks.end(this); }
    }

    @Override public boolean onTouchEvent(MotionEvent e) {
        int action = e.getActionMasked();
        if (action == MotionEvent.ACTION_DOWN) { down = hit(e.getX(), e.getY()); if (down < 0) return false; invalidate(); return true; }
        if (down < 0) return false;
        if (action == MotionEvent.ACTION_UP) { if (hit(e.getX(), e.getY()) == down) { if (down >= MAX) openBest.run(); else DmActivity.nativeInvSort((int) drawn[down * FIELDS]); } down = -1; invalidate(); }
        else if (action == MotionEvent.ACTION_CANCEL) { down = -1; invalidate(); }
        return true;
    }
}
