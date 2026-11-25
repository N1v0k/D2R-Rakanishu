






package com.xiaoboluo.d2r;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.RectF;

final class PadLegend {
    private PadLegend() {}

    private static final float GLYPH = 0.95f, GLYPH_GAP = 0.16f, GROUP_GAP = 0.9f;
    private static final RectF r = new RectF();


    static float width(float h, Paint text, Object... parts) {
        float w = 0;
        Object prev = null;
        for (Object p : parts) {
            if (p == null) w += h * GROUP_GAP;
            else if (p instanceof Bitmap) { if (prev instanceof Bitmap) w += h * GLYPH_GAP * 0.5f; w += glyphW((Bitmap) p, h); }
            else { if (prev instanceof Bitmap) w += h * GLYPH_GAP; w += text.measureText((String) p); }
            prev = p;
        }
        return w;
    }

    private static final android.graphics.Rect src = new android.graphics.Rect();
    private static final RectF dst = new RectF();



    static void strip(Canvas c, Bitmap b, RectF bar, Paint p) {
        int cap = Math.min(b.getWidth() / 3, b.getHeight());
        float cw = Math.min(bar.width() / 2, cap * bar.height() / b.getHeight());
        src.set(0, 0, cap, b.getHeight()); dst.set(bar.left, bar.top, bar.left + cw, bar.bottom); c.drawBitmap(b, src, dst, p);
        src.set(b.getWidth() - cap, 0, b.getWidth(), b.getHeight()); dst.set(bar.right - cw, bar.top, bar.right, bar.bottom); c.drawBitmap(b, src, dst, p);
        if (bar.width() > 2 * cw) { src.set(cap, 0, b.getWidth() - cap, b.getHeight()); dst.set(bar.left + cw, bar.top, bar.right - cw, bar.bottom); c.drawBitmap(b, src, dst, p); }
    }


    static void draw(Canvas c, RectF bar, Bitmap footer, Paint bitmap, Paint text, Object... parts) {
        if (footer != null) strip(c, footer, bar, bitmap);
        float h = bar.height(), x = bar.centerX() - width(h, text, parts) / 2, cy = bar.centerY();
        Object prev = null;
        for (Object p : parts) {
            if (p == null) x += h * GROUP_GAP;
            else if (p instanceof Bitmap) {
                Bitmap g = (Bitmap) p;
                if (prev instanceof Bitmap) x += h * GLYPH_GAP * 0.5f;
                float gw = glyphW(g, h), gh = h * GLYPH;
                r.set(x, cy - gh / 2, x + gw, cy + gh / 2);
                c.drawBitmap(g, null, r, bitmap);
                x += gw;
            } else {
                if (prev instanceof Bitmap) x += h * GLYPH_GAP;
                Paint.Align a = text.getTextAlign();
                text.setTextAlign(Paint.Align.LEFT);
                c.drawText((String) p, x, cy + text.getTextSize() * 0.36f, text);
                text.setTextAlign(a);
                x += text.measureText((String) p);
            }
            prev = p;
        }
    }

    private static float glyphW(Bitmap g, float h) { return g.getWidth() * h * GLYPH / Math.max(1, g.getHeight()); }
}
