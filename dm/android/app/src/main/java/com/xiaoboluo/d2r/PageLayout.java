package com.xiaoboluo.d2r;

import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.RectF;
import java.util.ArrayList;
import java.util.List;





final class PageLayout {

    static final float TITLE = 48, BODY = 32, SMALL = 28, LABEL = 36, PERCENT = 44, ID = 21, MIN_SHRINK = 0.75f;
    static final String CUT = "...";

    static final class Txt {
        final GameText t; final String s; final float x, y, k; final Paint.Align a; final int colour;
        Txt(GameText t, String s, float x, float y, float k, Paint.Align a, int colour) { this.t = t; this.s = s; this.x = x; this.y = y; this.k = k; this.a = a; this.colour = colour; }
    }
    static final class Btn {
        final int id; final RectF r; final boolean enabled;
        Btn(int id, RectF r, boolean enabled) { this.id = id; this.r = r; this.enabled = enabled; }
    }

    GameStyle style;
    GameText title, body, small, label, percent, id;
    float k;
    private final List<Txt> texts = new ArrayList<>();
    private final List<Btn> buttons = new ArrayList<>();
    private final List<RectF> rows = new ArrayList<>();
    private int rowKind;
    private final RectF bar = new RectF(), filled = new RectF();

    static final int BOX = 200;
    private boolean barShown;
    private int barPercent, barKind;


    boolean style(GameStyle s) {
        if (s.face(GameStyleData.EXOCET) == null || s.face(GameStyleData.FORMAL) == null) {
            AppArt.log("ASSUME packbuild.style: a page style without a face was refused");
            return false;
        }
        style = s;
        s.preparePlate();
        title = text(GameStyleData.EXOCET, TITLE); body = text(GameStyleData.FORMAL, BODY); small = text(GameStyleData.FORMAL, SMALL);
        label = text(GameStyleData.EXOCET, LABEL); percent = text(GameStyleData.EXOCET, PERCENT); id = text(GameStyleData.FORMAL, ID);
        return true;
    }

    private GameText text(int face, float units) {
        GameText t = style.text(new GameStyleData.Preset("PAGE", face, "FontColorWhite", null, units, "default", "DefaultSpacing"));
        t.prepare();
        return t;
    }

    int colour(String name) { return style.colour(name); }


    boolean begin(int w, int h) {
        texts.clear(); buttons.clear(); rows.clear();
        barShown = false;
        if (w == 0 || h == 0 || style == null) return false;
        k = h / 2160f * GameStyleData.CONSOLE_UI_SCALE * GameStyleData.PANEL_EM;
        return true;
    }


    float line(GameText t, String s, float x, float y, Paint.Align a, int colour, float max, int maxRows) {
        if (s == null || s.isEmpty()) return y;
        if (t.measure(s, k) <= max) { texts.add(new Txt(t, s, x, y, k, a, colour)); return y; }
        for (float kk = k; kk >= k * MIN_SHRINK - 1e-4f; kk -= k * 0.05f) {
            if (maxRows >= 2) {
                String[] two = split(t, s, max, kk);
                if (two != null) {
                    float y2 = y + t.size(kk) * 1.25f;
                    texts.add(new Txt(t, two[0], x, y, kk, a, colour));
                    texts.add(new Txt(t, two[1], x, y2, kk, a, colour));
                    return y2;
                }
            }
            if (t.measure(s, kk) <= max) { texts.add(new Txt(t, s, x, y, kk, a, colour)); return y; }
        }
        float kk = k * MIN_SHRINK;
        String cut = s;
        while (cut.length() > 1 && t.measure(cut + CUT, kk) > max) cut = cut.substring(0, cut.length() - 1);
        texts.add(new Txt(t, cut.trim() + CUT, x, y, kk, a, colour));
        return y;
    }


    float lines(GameText t, String s, float x, float y, Paint.Align a, int colour, float max, int rowsEach, float gap) {
        float at = y - gap;
        for (String p : s.split("\n")) at = line(t, p, x, at + gap, a, colour, max, rowsEach);
        return at;
    }

    int mark() { return texts.size(); }


    void lift(int mark, float y) {
        if (texts.size() <= mark) return;
        float dy = texts.get(texts.size() - 1).y - y;
        for (int i = mark; i < texts.size(); i++) { Txt o = texts.get(i); texts.set(i, new Txt(o.t, o.s, o.x, o.y - dy, o.k, o.a, o.colour)); }
    }


    private static String[] split(GameText t, String s, float max, float kk) {
        boolean spaces = s.indexOf(' ') > 0;
        for (int i = s.length() - 1; i > 0; i--) {
            if (spaces ? s.charAt(i) != ' ' : Character.isLowSurrogate(s.charAt(i))) continue;
            String a = s.substring(0, i).trim(), b = s.substring(i).trim();
            if (a.isEmpty() || b.isEmpty() || t.measure(a, kk) > max) continue;
            return t.measure(b, kk) <= max ? new String[]{a, b} : null;
        }
        return null;
    }


    String middle(GameText t, String name, String tail, float width) {
        if (t.measure(name + tail, k) <= width) return name + tail;
        float room = width - t.measure(CUT + tail, k);
        for (int keep = name.length() - 1; keep > 2; keep--) {
            String head = name.substring(0, (keep + 1) / 2), end = name.substring(name.length() - keep / 2);
            if (t.measure(head + end, k) <= room) return head + CUT + end + tail;
        }
        return CUT + tail;
    }


    float buttonWidth(float min, float max, String... labels) {
        float w = 0;
        for (String l : labels) w = Math.max(w, label.measure(l, k));
        return Math.max(min, Math.min(max, w + label.size(k) * 1.6f));
    }

    void button(int idKind, String text, float x, float y, float bw, float bh, boolean enabled) {
        RectF r = new RectF(x, y, x + bw, y + bh);
        buttons.add(new Btn(idKind, r, enabled));
        line(label, text, r.centerX(), r.centerY() + label.size(k) * 0.36f, Paint.Align.CENTER, colour(enabled ? "LightButtonTextColor" : "DarkButtonTextColor"), bw - label.size(k) * 0.8f, 1);
    }

    void row(int kind, RectF r) { rowKind = kind; rows.add(r); }

    void bar(int kind, float left, float top, float right, float bottom, int percent) {
        bar.set(left, top, right, bottom); barShown = true; barPercent = percent; barKind = kind;
        filled.set(left, top, left + (right - left) * Math.max(0, Math.min(100, percent)) / 100f, bottom);
        style.prepareBar(bar);
    }

    RectF bar() { return bar; }

    int hit(float x, float y) {
        for (Btn b : buttons) if (b.enabled && b.r.contains(x, y)) return b.id;
        return 0;
    }


    void draw(Canvas c, int pressed) {
        if (barShown) {
            style.bar(c, bar, filled);
            TestHooks.pageHit(bar, barKind, barPercent);
        }
        for (Btn b : buttons) {
            style.pageButton(c, b.r, !b.enabled ? GameStyle.BUTTON_DISABLED : pressed == b.id ? GameStyle.BUTTON_DOWN : GameStyle.BUTTON_NORMAL);
            if (b.enabled) TestHooks.pageHit(b.r, b.id, 0);
            TestHooks.pageHit(b.r, BOX + b.id, 0);
        }
        for (int i = 0; i < rows.size(); i++) TestHooks.pageHit(rows.get(i), rowKind + i, 0);
        for (Txt t : texts) {
            t.t.colourArgb(t.colour);
            t.t.draw(c, t.s, t.x, t.y, t.k, t.a, 255);
            TestHooks.pageText(c, t.s, t.x, t.y, t.t.paint(t.k));
        }
    }
}
