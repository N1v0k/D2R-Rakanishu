package com.xiaoboluo.d2r;

import android.content.Context;
import android.graphics.Canvas;
import android.view.MotionEvent;
import android.view.View;




final class PageShade extends View {
    private final View[] pages;
    private final int[] box = new int[4];

    PageShade(Context context, View... pages) {
        super(context);
        this.pages = pages;
        setWillNotDraw(false);
        setFocusable(false);
        setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);
    }

    @Override public boolean onTouchEvent(MotionEvent e) { return false; }

    @Override protected void onDraw(Canvas c) {
        GameRect g = GameRect.ui();
        if (g.whole()) return;
        g.bounds(GameRect.PAGE, box);
        boolean clipped = false;
        for (View v : pages) {
            if (!(v instanceof PadPage) || v.getVisibility() != VISIBLE || !((PadPage) v).padOpen()) continue;
            if (!clipped) { c.save(); c.clipOutRect(box[0], box[1], box[0] + box[2], box[1] + box[3]); clipped = true; }
            c.drawColor(((PadPage) v).backdrop());
        }
        if (clipped) c.restore();
    }
}
