package com.xiaoboluo.d2r;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.RectF;
import java.util.ArrayList;
import java.util.List;












final class ItemTooltip {
    private static final float LINE = 1.3f, WIDTH = 0.46f, MARGIN = 0.04f, PICTURE = 0.3f, SHRINK = 0.9f;
    private final GameStyle style;
    private final GameText title, body;
    private final Paint picture = new Paint(Paint.ANTI_ALIAS_FLAG | Paint.FILTER_BITMAP_FLAG);
    private final RectF box = new RectF(), pic = new RectF(), fill = new RectF();

    private final List<String> text = new ArrayList<>(), colour = new ArrayList<>();
    private final List<Boolean> head = new ArrayList<>();
    private List<GearPreview.Line> laidFor;
    private int laidW = -1, laidH = -1, laidGen = -1, laidPage = -1, page, pages = 1, revision;
    private float k, lineH, inset, textTop;

    final float[] lineLeft = new float[2], lineRight = new float[2];
    private String nav;

    ItemTooltip(GameStyle style) {
        this.style = style;
        title = style.text(GameStyleData.TOOLTIP_TITLE);
        body = style.text(GameStyleData.TOOLTIP);
        title.prepare(); body.prepare();
    }


    void reset() { page = 0; laidFor = null; box.setEmpty(); pic.setEmpty(); }



    int revision() { return revision; }
    float inset() { return inset; }
    float textTop() { return textTop; }
    float lineH() { return lineH; }
    float k() { return k; }

    int pages() { return pages; }


    RectF box() { return box; }


    boolean flip(int step) {
        if (pages <= 1) return false;
        page = ((page + step) % pages + pages) % pages;
        laidFor = null;
        return true;
    }


    boolean inside(float x, float y) { return box.contains(x, y) || pic.contains(x, y); }

    private float k0(int h) { return GameStyleGeometry.k(h) * style.data().scale(GameStyleData.PROFILE); }

    private void layout(List<GearPreview.Line> lines, int w, int h) {
        text.clear(); colour.clear(); head.clear();
        float base = k0(h), floor = style.data().size("SmallFontSize") / Math.max(1f, body.units());
        style.tooltipBox(0, 0, base, fill);
        float padX = fill.width() / 2, padY = fill.height() / 2, max = w * WIDTH - 2 * padX, room = h * (1 - 2 * MARGIN) - 2 * padY;
        List<String> t = new ArrayList<>(), c = new ArrayList<>();
        List<Boolean> hd = new ArrayList<>();
        for (float s = 1; ; s *= SHRINK) {
            final float kk = base * Math.max(s, floor);
            t.clear(); c.clear(); hd.clear();
            for (int i = 0; i < lines.size(); i++) {
                GearPreview.Line l = lines.get(i);
                final GameText kind = i == 0 ? title : body;
                if (l.text.isEmpty() || !style.covers(GameStyle.EXOCET, l.text)) continue;
                for (String part : EffectLayout.wrap(l.text, max, x -> kind.measure(x, kk)))
                    for (String piece : EffectLayout.breakWord(part, max, x -> kind.measure(x, kk))) { t.add(piece); c.add(l.colour); hd.add(i == 0); }
            }
            k = kk; lineH = body.size(kk) * LINE;
            if (t.size() * lineH <= room || s <= floor) break;
        }
        float[] heights = new float[t.size()];
        java.util.Arrays.fill(heights, lineH);
        int[] cut = EffectLayout.cardPages(heights, room, lineH, page);
        pages = cut[0]; page = cut[1];
        for (int i = cut[2]; i < cut[3]; i++) { text.add(t.get(i)); colour.add(c.get(i)); head.add(hd.get(i)); }
        float width = 0;
        for (int i = 0; i < text.size(); i++) width = Math.max(width, (head.get(i) ? title : body).measure(text.get(i), k));
        nav = pages > 1 ? "<   " + (page + 1) + " / " + pages + "   >" : null;
        if (nav != null) width = Math.max(width, body.measure(nav, k));
        float boxW = Math.min(width, max) + 2 * padX, boxH = (text.size() + (nav != null ? 1 : 0)) * lineH + 2 * padY;
        float picS = h * PICTURE, gap = h * 0.02f, total = picS + gap + boxW;
        float x0 = Math.max(gap, (w - total) / 2) + picS + gap, y0 = (h - boxH) / 2;
        box.set(x0, y0, x0 + boxW, y0 + boxH);
        pic.set(x0 - gap - picS, h / 2f - picS / 2, x0 - gap, h / 2f + picS / 2);

        style.tooltipBox(0, 0, k, fill);
        inset = fill.width() / 2; textTop = box.top + fill.height() / 2;
        for (int i = 0; i < 2; i++) {
            float lw = i < text.size() ? Math.min((head.get(i) ? title : body).measure(text.get(i), k), box.width() - 2 * inset) : 0;
            if (lw > 0) { lineLeft[i] = box.centerX() - lw / 2; lineRight[i] = box.centerX() + lw / 2; } else { lineLeft[i] = 1; lineRight[i] = 0; }
        }
        revision++;
    }



    void draw(Canvas c, int w, int h, Bitmap img, List<GearPreview.Line> lines) {
        if (lines != laidFor || w != laidW || h != laidH || style.generation() != laidGen || page != laidPage) {
            layout(lines, w, h);
            laidFor = lines; laidW = w; laidH = h; laidGen = style.generation(); laidPage = page;
        }
        c.drawColor(0xD8000000);
        if (img != null) {
            float s = Math.min(pic.width() / img.getWidth(), pic.height() / img.getHeight()), iw = img.getWidth() * s, ih = img.getHeight() * s;
            fill.set(pic.centerX() - iw / 2, pic.centerY() - ih / 2, pic.centerX() + iw / 2, pic.centerY() + ih / 2);
            c.drawBitmap(img, null, fill, picture);
        }
        fill.set(box);
        style.tooltip(c, fill);
        style.tooltipBox(0, 0, k, fill);
        float y = box.top + fill.height() / 2, mid = box.centerX();
        for (int i = 0; i < text.size(); i++, y += lineH) line(c, head.get(i) ? title : body, text.get(i), colour.get(i), mid, y);
        if (nav != null) line(c, body, nav, GearPreview.GREY, mid, y);
    }

    private void line(Canvas c, GameText kind, String s, String colourName, float mid, float y) {
        Paint p = kind.paint(k);
        float baseline = GameStyleGeometry.centreBaseline(y, y + lineH, p.ascent(), p.descent());
        kind.colourArgb(style.colour(colourName));
        kind.draw(c, s, mid, baseline, k, Paint.Align.CENTER, 255);
    }
}
