package com.xiaoboluo.d2r;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.RectF;







final class DotRows {
    static final String DOT = "•", DOT_ALT = "·";


    interface Glyphs {
        boolean ready();
        boolean covers(String s);
        float units();
        float measure(String s, float k);
        float baseline(float top, float bottom, float k);
        int generation();
        int colour(String name);
        void drawLabel(Canvas c, String s, float x, float baseline, float k);
        void drawDot(Canvas c, String s, float x, float baseline, float k, int argb);
        void backing(Canvas c, RectF box);
    }
    private static final String GREEN = "FontColorPartyGreen", RED = "FontColorRed", GREY = "FontColorLightGray";
    private final Glyphs glyphs;
    private final String[] words = {"", ""};
    private final RectF backing = new RectF();
    private String glyph;
    private int green, red, grey, generation = -1;

    private String glyphNow() { return DotLayout.dotGlyph(DOT, DOT_ALT, glyphs.covers(DOT), glyphs.covers(DOT_ALT)); }

    DotRows(Glyphs glyphs) {
        this.glyphs = glyphs;
        glyph = glyphNow();
    }



    boolean words(Context c) {
        String a = clean(AppText.get(c, R.string.dmAndroidDotDamage)), d = clean(AppText.get(c, R.string.dmAndroidDotDefense));
        String g = glyphNow();
        if (a.equals(words[0]) && d.equals(words[1]) && (g == null ? glyph == null : g.equals(glyph))) return false;
        words[0] = a; words[1] = d; glyph = g;
        return true;
    }
    private String clean(String s) { return s == null || s.isEmpty() || !glyphs.covers(s) ? "" : s; }

    boolean ready() { return glyphs.ready() && glyph != null; }
    float units() { return glyphs.units(); }


    DotLayout layout(int code, float lineH, float px) {
        if (glyphs.generation() != generation) {
            generation = glyphs.generation();
            green = glyphs.colour(GREEN); red = glyphs.colour(RED); grey = glyphs.colour(GREY);
        }
        float k = px / glyphs.units();
        float w0 = words[0].isEmpty() ? 0 : glyphs.measure(words[0], k), w1 = words[1].isEmpty() ? 0 : glyphs.measure(words[1], k);
        return new DotLayout(code, lineH, px, w0, w1, glyph == null ? 0 : glyphs.measure(glyph, k));
    }


    RectF dots(int code, int r, DotLayout l, RectF out) {
        int n = DotLayout.dots(DotLayout.row(code, r));
        float t = l.rowTop(r);
        out.set(l.dotLeft(0), t, l.dotLeft(n - 1) + l.dotW, t + l.lineH);
        return out;
    }

    RectF block(DotLayout l, RectF out) {
        float pad = l.where == DotLayout.OUTSIDE ? l.pad() : 0;
        out.set(l.left - pad, l.top - pad, l.left + l.width + pad, l.top + l.height + pad);
        return out;
    }

    void draw(Canvas c, int code, DotLayout l) {
        String g = glyph;
        if (code == 0 || l == null || g == null || l.where == DotLayout.NONE || !glyphs.ready()) return;
        if (l.where == DotLayout.OUTSIDE) glyphs.backing(c, block(l, backing));
        float k = l.px / glyphs.units();
        for (int r = 0; r < 2; r++) {
            int v = DotLayout.row(code, r);
            if (v == 0) continue;
            float t = l.rowTop(r), base = glyphs.baseline(t, t + l.lineH, k);
            if (!words[r].isEmpty()) glyphs.drawLabel(c, words[r], l.left, base, k);
            for (int i = 0, n = DotLayout.dots(v); i < n; i++) glyphs.drawDot(c, g, l.dotLeft(i), base, k, v > 4 ? green : v < 4 ? red : grey);
        }
    }
}
