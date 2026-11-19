package com.xiaoboluo.d2r;





public class DotLayoutTest {
    static int fails;
    static void check(boolean ok, String what) { System.out.println((ok ? "ok   " : "FAIL ") + what); if (!ok) fails++; }
    static boolean near(float a, float b) { return Math.abs(a - b) < 1e-3f; }
    static int code(int damage, int defense) { return (damage == Integer.MIN_VALUE ? 0 : damage + 4) | (defense + 4) << 3; }


    static final float PX = 20, DOT = 8, LINE = 24, DE0 = 46, DE1 = 44, EN0 = 52, EN1 = 41;

    static final float L = 400, T = 200, R = 800, B = 600, IN = 20, TOP = 210, VW = 1600, VH = 900;


    static float[][] lines(float w0, float w1) {
        float mid = (L + R) / 2;
        float[] l = new float[2], r = new float[2];
        float[] w = {w0, w1};
        for (int i = 0; i < 2; i++) if (w[i] > 0) { l[i] = mid - w[i] / 2; r[i] = mid + w[i] / 2; } else { l[i] = 1; r[i] = 0; }
        return new float[][] {l, r};
    }
    static DotLayout place(int code, float w0, float w1, float[] labels, float l, float t, float r, float b) { return place(code, w0, w1, labels, l, t, r, b, false); }
    static DotLayout place(int code, float w0, float w1, float[] labels, float l, float t, float r, float b, boolean besideBoxes) {
        float[][] x = lines(w0, w1);
        if (l != L || r != R) { float mid = (l + r) / 2, off = mid - (L + R) / 2; for (int i = 0; i < 2; i++) if (x[0][i] <= x[1][i]) { x[0][i] += off; x[1][i] += off; } }
        return new DotLayout(code, LINE, PX, labels[0], labels[1], DOT).inTooltip(l, t, r, b, IN, t + 10, x[0], x[1], VW, VH, besideBoxes);
    }
    static DotLayout place(int code, float w0, float w1, float[] labels) { return place(code, w0, w1, labels, L, T, R, B); }

    static boolean clear(DotLayout d, float[] lineL, float[] lineR) {
        for (int k = 0; k < 2; k++) {
            int i = d.line[k];
            if (i < 0 || lineL[i] > lineR[i]) continue;
            float from = d.left, to = d.left + d.rowWidth(k), gap = DotLayout.CLEAR * PX - 1e-3f;
            if (to + gap > lineL[i] && from - gap < lineR[i]) return false;
        }
        return true;
    }

    public static void main(String[] a) {
        float[] de = {DE0, DE1}, en = {EN0, EN1};
        int both = code(3, -2);
        check(DotLayout.rows(both) == 2 && DotLayout.dots(DotLayout.row(both, 0)) == 3 && DotLayout.dots(DotLayout.row(both, 1)) == 2, "three green damage dots, two red defense dots");
        int same = code(0, 0);
        check(DotLayout.rows(same) == 2 && DotLayout.dots(DotLayout.row(same, 0)) == 1 && DotLayout.dots(DotLayout.row(same, 1)) == 1, "a swap without change: one grey dot per row");
        int noDamage = code(Integer.MIN_VALUE, 1);
        check(DotLayout.rows(noDamage) == 1 && DotLayout.row(noDamage, 0) == 0, "no damage base: only the defense row");


        DotLayout m = new DotLayout(both, LINE, PX, DE0, DE1, DOT).at(100, 50);
        check(m.where == DotLayout.FREE && near(m.rowTop(0), 50) && near(m.rowTop(1), 74), "damage on the first line, defense on the next");
        check(near(m.dotLeft(0), 100 + DE0 + DotLayout.GAP * PX) && near(m.dotLeft(1) - m.dotLeft(0), DOT + DotLayout.SPACE * PX), "the dots after the widest label, a space between them");
        check(near(m.width, DE0 + DotLayout.GAP * PX + 3 * DOT + 2 * DotLayout.SPACE * PX) && near(m.height, 2 * LINE), "the block: the label column and the longest row, two lines");
        check(near(m.rowWidth(1), DE0 + DotLayout.GAP * PX + 2 * DOT + DotLayout.SPACE * PX), "each row reaches as far as its own dots");
        DotLayout bare = new DotLayout(noDamage, LINE, PX, DE0, 0, DOT).at(100, 50);
        check(near(bare.dotLeft(0), 100) && near(bare.rowTop(1), 50) && bare.line[0] == -1, "a single row without its label: on the first line, the dots at the left edge");


        for (float[] labels : new float[][] {de, en}) {
            String lang = labels == de ? "DE" : "EN";
            DotLayout d = place(both, 120, 60, labels);
            float[][] x = lines(120, 60);
            check(d.where == DotLayout.LEFT && near(d.left, L + IN) && near(d.rowTop(0), T + 10) && near(d.rowTop(1), T + 10 + LINE) && clear(d, x[0], x[1]),
                lang + " short name: at the inner left edge on the top two lines, clear of their text");

            d = place(both, 340, 60, labels);
            check(d.where == DotLayout.OUTSIDE && near(d.left - d.pad(), L) && near(d.top + d.height + d.pad(), T) && d.top + d.height <= T, lang + " long name: above the box at its top left corner");

            check(place(both, 100, 340, labels).where == DotLayout.OUTSIDE, lang + " long second line: outside");
        }


        float wide = (R - L) - 2 * (IN + new DotLayout(both, LINE, PX, DE0, DE1, DOT).width + DotLayout.CLEAR * PX) - 2;
        check(place(both, wide, 0, de).where == DotLayout.LEFT && place(both, wide, 0, en).where == DotLayout.OUTSIDE, "the same name: German fits at the left, the wider English block goes outside");


        {
            DotLayout d = new DotLayout(both, LINE, PX, DE0, DE1, DOT);
            float[] lineL = {L + IN, L + IN}, lineR = {L + 200, L + 150};
            d.inTooltip(L, T, R, B, IN, T + 10, lineL, lineR, VW, VH, false);
            check(d.where == DotLayout.RIGHT && near(d.left + d.width, R - IN) && clear(d, lineL, lineR), "no room at the left: at the inner right edge, clear of the text");
            float[] tight = {L + IN, L + IN}, far = {R - IN - d.width, L + 150};
            DotLayout e = new DotLayout(both, LINE, PX, DE0, DE1, DOT).inTooltip(L, T, R, B, IN, T + 10, tight, far, VW, VH, false);
            check(e.where == DotLayout.OUTSIDE, "text closer to the right edge than the block plus the clearance: outside");
        }

        check(place(noDamage, 100, 340, de).where == DotLayout.LEFT, "a single row stands on the top line only");

        check(place(both, 0, 0, de).where == DotLayout.LEFT, "empty lines: at the left");

        {
            int threeOne = code(3, 0);
            DotLayout probe = new DotLayout(threeOne, LINE, PX, DE0, DE1, DOT);
            float second = (R - L) - 2 * (IN + probe.rowWidth(1) + DotLayout.CLEAR * PX) - 2;
            check(place(threeOne, 0, second, de).where == DotLayout.LEFT, "the defense row's single dot clears a second line the three-dot row would not");
        }

        check(place(both, 0, 0, de, 400, 200, 470, 600).where == DotLayout.OUTSIDE, "a box narrower than the block: outside");


        DotLayout top = place(both, 340, 0, de, 400, 10, 800, 400);
        check(top.where == DotLayout.OUTSIDE && near(top.left + top.width + top.pad(), 400) && near(top.top - top.pad(), 10), "no room above: left of the box at its top");
        DotLayout corner = place(both, 340, 0, de, 0, 10, 400, 400);
        check(corner.where == DotLayout.OUTSIDE && near(corner.left - corner.pad(), 400) && near(corner.top - corner.pad(), 10), "at the view's top left: right of the box");
        DotLayout full = place(both, 1500, 0, de, 0, 10, 1600, 400);
        check(full.where == DotLayout.OUTSIDE && near(full.left - full.pad(), 0) && near(full.top - full.pad(), 400), "as wide as the view: below the box");
        DotLayout none = place(both, 1500, 0, de, 0, 10, 1600, 890);
        check(none.where == DotLayout.NONE, "no spot in the view: not shown");
        DotLayout[] outside = {top, corner, full};
        float[][] boxes = {{400, 10, 800, 400}, {0, 10, 400, 400}, {0, 10, 1600, 400}};
        for (int i = 0; i < outside.length; i++) {
            DotLayout d = outside[i];
            float[] box = boxes[i];
            float x0 = d.left - d.pad(), y0 = d.top - d.pad(), x1 = d.left + d.width + d.pad(), y1 = d.top + d.height + d.pad();
            check(x1 <= box[0] + 1e-3f || x0 >= box[2] - 1e-3f || y1 <= box[1] + 1e-3f || y0 >= box[3] - 1e-3f, "an outside block never overlaps the box (" + i + ")");
            check(x0 >= 0 && y0 >= 0 && x1 <= VW && y1 <= VH, "an outside block stays in the view (" + i + ")");
        }


        DotLayout above = place(both, 340, 60, de, L, T, R, B, true);
        check(above.where == DotLayout.OUTSIDE && above.top + above.height + above.pad() <= T + 1e-3f, "boxes beside: a long name still goes above the box");
        DotLayout below = place(both, 340, 0, de, 400, 10, 800, 400, true);
        check(below.where == DotLayout.OUTSIDE && below.left - below.pad() <= 800 + 1e-3f && below.left + below.width + below.pad() >= 400 - 1e-3f
            && below.top - below.pad() >= 400 - 1e-3f, "boxes beside: the left spot is skipped, the block stands below the box");
        check(place(both, 340, 0, de, 400, 10, 800, 400).where == DotLayout.OUTSIDE, "without boxes beside the order is unchanged");
        DotLayout right = place(both, 340, 0, de, 0, 10, 400, 400, true);
        check(right.where == DotLayout.OUTSIDE && right.left - right.pad() >= 0 && right.top - right.pad() >= 400 - 1e-3f, "boxes beside: no block right of a box at the view's left edge either");
        check(place(both, 1500, 0, de, 0, 10, 1600, 890, true).where == DotLayout.NONE, "boxes beside and no room above or below: not shown");
        {
            float[] lineL = {L + IN, L + IN}, lineR = {L + 200, L + 150};
            DotLayout inside = new DotLayout(both, LINE, PX, DE0, DE1, DOT).inTooltip(L, T, R, B, IN, T + 10, lineL, lineR, VW, VH, true);
            check(inside.where == DotLayout.RIGHT && near(inside.left + inside.width, R - IN), "boxes beside: the inside right edge stays a place");
        }


        check("•".equals(DotLayout.dotGlyph("•", "·", true, true)) && "•".equals(DotLayout.dotGlyph("•", "·", true, false)), "covered primary: the round dot");
        check("·".equals(DotLayout.dotGlyph("•", "·", false, true)), "only the stand-in covered: the smaller one");
        check(DotLayout.dotGlyph("•", "·", false, false) == null, "neither covered: no glyph, no rows");
        DotLayout noDot = new DotLayout(both, LINE, PX, DE0, DE1, 0).at(100, 50);
        check(near(noDot.width, DE0 + DotLayout.GAP * PX + 2 * DotLayout.SPACE * PX) && near(noDot.dotLeft(0), noDot.left + noDot.labels)
            && near(noDot.dotLeft(1) - noDot.dotLeft(0), DotLayout.SPACE * PX), "no glyph measured: no dot width, only the spaces between them remain");

        if (fails > 0) { System.out.println(fails + " failed"); System.exit(1); }
        System.out.println("DotLayoutTest: all passed");
    }
}
