






package com.xiaoboluo.d2r;

import java.util.List;

public final class EffectLayoutTest {
    private static int checks, fails;
    private static void check(boolean ok, String what) { checks++; if (!ok) { fails++; System.out.println("FAIL " + what); } }
    private static boolean near(float a, float b) { return Math.abs(a - b) < 0.01f; }

    public static void main(String[] args) {

        check(near(EffectLayout.scale(826.7f, 372f, false), 372f / 375f), "phone 2480x1116 @3.0: 372 dp high -> 0.992");
        check(near(EffectLayout.scale(1334f, 750f, false), 1f), "a large phone stays at 1");
        check(near(EffectLayout.scale(1334f, 750f, true), 1.15f), "a large tablet grows at most 15 %");
        check(near(EffectLayout.scale(600f, 375f, true), 1f), "a small tablet window stays at 1");
        check(near(EffectLayout.unit(2480, 1116, 3f, false), 3f * 372f / 375f), "unit = density x scale");
        check(EffectLayout.unit(0, 1116, 3f, false) == 0 && EffectLayout.unit(2480, 1116, 0, false) == 0, "no size / density: unit 0");


        check(EffectLayout.rows(0, 34, 1) == 1, "exactly one field fits in 2r");
        check(EffectLayout.rows(0, 33.9f, 1) == 0, "less than 2r: nothing fits");
        check(EffectLayout.rows(0, 74, 1) == 2 && EffectLayout.rows(0, 73.9f, 1) == 1, "2r + pitch fits two");
        check(EffectLayout.rows(100, 50, 1) == 0, "an inverted band holds nothing");
        check(EffectLayout.rows(0, 100000, 1) == EffectLayout.MAX_ROWS, "rows are capped");


        float u = EffectLayout.unit(2480, 1116, 3f, false), bottom = 1116 * 0.86f, r = 17 * u, field = 34 * u;
        float top = StatusPlace.effectsTop(11348, 1116, 0, 3), left = StatusPlace.left(1116, 0), clock = StatusPlace.left(1116, 0);
        int rows = EffectLayout.rows(top, bottom, u);
        float reach = EffectLayout.reach(field, field * 154 / 132 + 40), step = EffectLayout.step(reach, u);
        EffectLayout.Cells into = new EffectLayout.Cells();
        float[] xs = into.x, ys = into.y;
        EffectLayout.Cells one = EffectLayout.place(into, 1, top, bottom, u, left, step, reach, 0, null);
        check(one == into && one.x == xs && one.y == ys, "place writes into the given cells, no new arrays");
        check(one.cells() == 1 && one.shown == 1 && one.more == 0 && near(one.y[0] - r, top) && near(one.x[0] - r, clock), "one effect: its field starts at the top edge, left in line with the clock");
        float inset = 44;
        EffectLayout.Cells cut = EffectLayout.place(into, 1, top, bottom, u, StatusPlace.left(1116, inset), step, reach, 0, null);
        check(near(cut.x[0] - r, clock + inset), "with a cutout: the field moves with the clock (cutout inset added)");
        EffectLayout.Cells five = EffectLayout.place(into, 5, top, bottom, u, left, step, reach, 0, null);
        boolean inside = true; for (int i = 0; i < 5; i++) inside &= near(five.x[i], left + r) && near(five.y[i], top + r + i * 40 * u);
        check(rows >= 5 && inside, "five fill the first column from the top down, 40 units apart (rows " + rows + ")");

        float two = StatusPlace.effectsTop(11348, 1116, 0, 2);
        check(near(top - two, 1116 * StatusPlace.SIZE * StatusPlace.LINE), "one line less moves the top up by one line");

        int n = rows + 2;
        EffectLayout.Cells more = EffectLayout.place(into, n, top, bottom, u, left, step, reach, 0, null);
        check(more.cells() == n && near(more.x[rows] - more.x[0], reach + 4 * u) && near(more.y[rows], top + r), "the second column starts at the top, reach + 4 units further right");
        check(near(EffectLayout.reach(field, field * 154 / 132 + 40), field * 154 / 132 + 40) && near(EffectLayout.reach(field, 0), field), "reach: field to the widest box's right edge; no box = the field");
        check(near(EffectLayout.step(reach, u), field * 154 / 132 + 40 + 4 * u), "step = reach of the widest box + 4 units (\"9\", \"99\" or \"2 Min.\" alike)");
        check(near(EffectLayout.below(field, field * 141 / 132), field * 9 / 132) && EffectLayout.below(field, field * 0.9f) == 0, "below: how far the tallest box reaches under the field");

        float low = 30 * u;
        EffectLayout.Cells tall = EffectLayout.place(into, rows * 2, top, bottom, u, left, step, reach, low, null);
        boolean within = true; for (int i = 0; i < tall.cells(); i++) within &= tall.y[i] + r + low <= bottom + 3 * u + 0.01f;
        check(within && tall.cells() < rows * 2, "boxes reaching far below stay within half a row gap of the bottom bound");
        check(EffectLayout.place(into, rows * 2, top, bottom, u, left, step, reach, 3 * u, null).cells() == rows * 2, "a box hanging no more than half a row gap below costs no row");
        int capacity = rows * 2;
        EffectLayout.Cells full = EffectLayout.place(into, capacity, top, bottom, u, left, step, reach, 0, null);
        check(full.cells() == capacity && full.shown == capacity && full.more == 0, "exactly two full columns: no +N");
        EffectLayout.Cells many = EffectLayout.place(into, capacity + 4, top, bottom, u, left, step, reach, 0, null);
        check(many.cells() == capacity && many.shown == capacity - 1 && many.more == 5, "four more than fit: the last cell is +5");
        check(EffectLayout.place(into, 0, top, bottom, u, left, step, reach, 0, null).cells() == 0 && EffectLayout.place(into, 3, 500, 510, u, left, step, reach, 0, null).cells() == 0, "nothing to draw / no room: no cells");



        float[] keep = {left + step + 120, 900, 90};
        EffectLayout.Cells kept = EffectLayout.place(into, capacity + 4, top, bottom, u, left, step, reach, 0, keep);
        int col1 = 0, col2 = 0; boolean clear = true;
        for (int i = 0; i < kept.cells(); i++) {
            if (kept.x[i] > left + r + 1) col2++; else col1++;
            float x0 = kept.x[i] - r;
            if (EffectLayout.touches(x0, kept.y[i] - r, x0 + reach, kept.y[i] + r, keep)) clear = false;
        }
        check(col1 == rows && col2 > 0 && col2 < rows && clear, "the second column stops above the thumb, the first keeps all rows (" + col1 + "/" + col2 + ")");
        check(kept.more == capacity + 4 - (kept.cells() - 1), "fewer cells: the rest goes into +N");
        check(EffectLayout.touches(0, 0, 10, 10, new float[]{15, 5, 6}) && !EffectLayout.touches(0, 0, 10, 10, new float[]{17, 17, 6}) && !EffectLayout.touches(0, 0, 10, 10, null),
            "touches: a rectangle against a circle");
        check(EffectLayout.bottomFor(0, 100, 900, keep) == 900 && EffectLayout.bottomFor(keep[0] - 50, keep[0] + 50, 1000, keep) == 810 && EffectLayout.bottomFor(0, 1, 900, null) == 900,
            "bottomFor: only a card meeting the thumb horizontally ends above it");


        float[] c = new float[4];
        EffectLayout.card(250, 558, 300, 120, 400, 900, c);
        check(near(c[0], 250) && near(c[1], 498) && near(c[3], 618) && near(c[2], 550), "card centred on the symbol, from the given left edge");
        EffectLayout.card(250, 420, 300, 120, 400, 900, c);
        check(near(c[1], 400), "card pushed down to the top bound");
        EffectLayout.card(250, 880, 300, 120, 400, 900, c);
        check(near(c[3], 900), "card pushed up to the bottom bound");
        EffectLayout.card(250, 558, 300, 700, 400, 900, c);
        check(near(c[1], 400), "a card taller than the band starts at the top bound");
        float cardLeft = more.x[rows] - r + reach + EffectLayout.CARD_GAP * u;
        check(cardLeft > more.x[rows] - r + reach && cardLeft > more.x[0] - r + reach, "the card starts right of the rightmost column's box, clear of every column");


        EffectLayout.Measure m = s -> s.length() * 10f;
        List<String> w = EffectLayout.wrap("ca. 168 Schaden, falls die Rate bleibt", 160, m);
        check(w.size() == 3 && w.get(0).equals("ca. 168 Schaden,") && w.get(1).equals("falls die Rate") && w.get(2).equals("bleibt"), "wrapped into three lines: " + w);
        w = EffectLayout.wrap("Widerstandsschwund", 50, m);
        check(w.size() == 1 && w.get(0).equals("Widerstandsschwund"), "a long word keeps its own line");
        check(EffectLayout.wrap("  ", 50, m).isEmpty(), "blank text: no lines");

        List<String> b = EffectLayout.breakWord("Widerstandsschwund", 50, m);
        check(b.size() == 4 && String.join("", b).equals("Widerstandsschwund") && b.get(0).equals("Wider") && b.get(3).equals("und"), "broken into pieces of at most 5 characters: " + b);
        check(EffectLayout.breakWord("kurz", 50, m).size() == 1, "a word that fits stays whole");
        check(EffectLayout.breakWord("W", 5, m).size() == 1, "a single character wider than max is still one piece");




        float[] h10 = new float[10];
        java.util.Arrays.fill(h10, 10);
        int[] pg = EffectLayout.cardPages(h10, 100, 10, 3);
        check(pg[0] == 1 && pg[1] == 0 && pg[2] == 0 && pg[3] == 10, "100 high in 100: one page with all lines");
        pg = EffectLayout.cardPages(h10, 60, 10, 0);
        check(pg[0] == 2 && pg[2] == 0 && pg[3] == 5, "100 high in 60 with a 10 paging row: 5 lines a page");
        pg = EffectLayout.cardPages(h10, 60, 10, 7);
        check(pg[1] == 1 && pg[2] == 5 && pg[3] == 10, "a page past the end is clamped to the last");
        pg = EffectLayout.cardPages(new float[]{50, 50, 50}, 30, 10, 1);
        check(pg[0] == 3 && pg[2] == 1 && pg[3] == 2, "a line taller than a page gets a page of its own");
        pg = EffectLayout.cardPages(h10, 60, 10, -2);
        check(pg[1] == 0 && pg[2] == 0, "a negative page is clamped to the first");


        EffectLayout.CardKey key = new EffectLayout.CardKey();
        check(key.changed("a", 100, 50, 10, 40, 5, 20, 1, 0, -1), "the first layout");
        check(!key.changed("a", 100, 50, 10, 40, 5, 20, 1, 0, -1), "same inputs: no new layout");
        check(key.changed("b", 100, 50, 10, 40, 5, 20, 1, 0, -1), "other text");
        check(key.changed("b", 120, 50, 10, 40, 5, 20, 1, 0, -1), "other view width");
        check(key.changed("b", 120, 50, 12, 40, 5, 20, 1, 0, -1), "status lines moved the top bound");
        check(key.changed("b", 120, 50, 12, 40, 5, 20, 2, 0, -1), "language / font changed the style generation");
        check(key.changed("b", 120, 50, 12, 40, 5, 20, 2, 1, -1), "another page");
        check(key.changed("b", 120, 50, 12, 40, 5, 24, 2, 1, -1), "another anchor");
        key.page(0);
        check(key.changed("b", 120, 50, 12, 40, 5, 24, 2, 1, -1) && !key.changed("b", 120, 50, 12, 40, 5, 24, 2, 1, -1), "the clamped page is what is compared next");
        key.reset();
        check(key.changed("b", 120, 50, 12, 40, 5, 24, 2, 1, -1), "after a reset (card closed) the next open lays out");
        System.out.println("EffectLayoutTest: " + checks + " checks, " + fails + " failed");
        if (fails > 0) System.exit(1);
    }
}
