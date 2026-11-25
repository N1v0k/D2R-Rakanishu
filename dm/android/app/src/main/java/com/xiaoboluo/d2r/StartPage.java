package com.xiaoboluo.d2r;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.os.Handler;
import android.os.Looper;
import android.view.MotionEvent;
import android.view.View;
import java.io.File;





final class StartPage extends View {
    static final String PAGE = "start_page";

    static final int BUTTON = 1, ANYWHERE = 2;

    private final Handler ui = new Handler(Looper.getMainLooper());
    private final PageLayout page = new PageLayout();
    private String title, text, button;
    private Runnable action;
    private boolean anywhere, held, loading;
    private final android.graphics.RectF whole = new android.graphics.RectF();

    StartPage(Context c) {
        super(c);
        page.style(GameStyle.builtIn(c));
        setBackgroundColor(0xFF000000);
    }


    void show(String title, String text, String button, Runnable action) {
        this.title = title; this.text = text; this.button = button; this.action = action;
        anywhere = button == null && action != null;
        held = false;
        if (page.style.builtIn() && !loading) {
            loading = true;
            Context c = getContext();
            new Thread(() -> {
                GameStyle s = GameStyle.of(c, new File(c.getFilesDir(), "dm/appart"));
                ui.post(() -> { if (s != null && page.style.builtIn() && page.style(s)) { relayout(); invalidate(); } });
            }, "dm-start-style").start();
        }
        relayout();
        invalidate();
    }

    void text(String text) { this.text = text; relayout(); invalidate(); }

    @Override protected void onSizeChanged(int w, int h, int ow, int oh) { whole.set(0, 0, w, h); relayout(); }

    private void relayout() {
        int w = getWidth(), h = getHeight();
        if (!page.begin(w, h) || text == null) return;
        float y = h * 0.26f;
        if (title != null) { page.line(page.title, title, w / 2f, h * 0.17f, Paint.Align.CENTER, page.colour("FontColorGoldYellow"), w * 0.8f, 1); y = h * 0.32f; }
        float last = page.lines(page.body, text, w / 2f, y, Paint.Align.CENTER, page.colour("FontColorBeige"), w * 0.8f, 3, page.body.size(page.k) * 1.5f);
        if (button != null) {
            float bw = page.buttonWidth(w * 0.30f, w * 0.6f, button), bh = h * 0.11f;
            page.button(BUTTON, button, w / 2f - bw / 2, Math.max(h * 0.62f, last + h * 0.08f), bw, bh, true);
        }
    }

    @Override protected void onDraw(Canvas c) {
        TestHooks.pageBegin(this, PAGE);
        page.draw(c, held ? BUTTON : 0);
        if (anywhere) TestHooks.pageHit(whole, ANYWHERE, 0);
        TestHooks.pageEnd(this);
    }

    @Override public boolean onTouchEvent(MotionEvent e) {
        int a = e.getActionMasked();
        boolean on = anywhere || button != null && page.hit(e.getX(), e.getY()) == BUTTON;
        if (a == MotionEvent.ACTION_DOWN) { held = on && button != null; invalidate(); }
        else if (a == MotionEvent.ACTION_UP) {
            boolean go = on && (anywhere || held);
            held = false;
            invalidate();
            if (go && action != null) action.run();
        } else if (a == MotionEvent.ACTION_CANCEL) { held = false; invalidate(); }
        return true;
    }
}
