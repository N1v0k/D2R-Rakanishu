package com.xiaoboluo.d2r;

import android.graphics.Canvas;
import android.graphics.RectF;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;






public class DotRowsTest {
    static int fails;
    static void check(boolean ok, String what) { System.out.println((ok ? "ok   " : "FAIL ") + what); if (!ok) fails++; }

    static final class Fake implements DotRows.Glyphs {
        final Set<String> covered = new HashSet<>();
        final Map<String,Float> sizes = new HashMap<>();
        boolean ready = true;
        int generation = 1;
        final List<String> measured = new ArrayList<>();
        final List<String> drawn = new ArrayList<>();
        final List<float[]> placed = new ArrayList<>();
        final List<Integer> dotColours = new ArrayList<>();
        int backings, colours;
        public boolean ready() { return ready; }
        public boolean covers(String s) { return covered.contains(s); }
        public float units() { return 10; }
        public float measure(String s, float k) { measured.add(s); float w = sizes.containsKey(s) ? sizes.get(s) : s.length(); return w * 4 * k; }
        public float baseline(float top, float bottom, float k) { return (top + bottom) / 2; }
        public int generation() { return generation; }
        public int colour(String name) { colours++; return name.hashCode(); }
        public void drawLabel(Canvas c, String s, float x, float base, float k) { drawn.add("L" + s); placed.add(new float[] {x, base, k}); }
        public void drawDot(Canvas c, String s, float x, float base, float k, int argb) {
            drawn.add("D" + s); placed.add(new float[] {x, base, k}); dotColours.add(argb);
        }
        public void backing(Canvas c, RectF box) { backings++; }
    }


    static final int CODE = (3 + 4) | (4 - 2) << 3;

    static DotRows rows(Fake f) { AppText.labels = new String[] {"DMG", "DEF"}; DotRows r = new DotRows(f); r.words(null); return r; }

    public static void main(String[] a) {

        Fake f = new Fake();
        f.covered.add(DotRows.DOT); f.covered.add(DotRows.DOT_ALT); f.covered.add("DMG"); f.covered.add("DEF");
        DotRows r = rows(f);
        check(r.ready(), "a font covering the primary: rows are ready");
        DotLayout l = r.layout(CODE, 24, 20).at(100, 50);
        r.draw(new Canvas(), CODE, l);
        check(f.drawn.equals(java.util.Arrays.asList("LDMG", "D•", "D•", "D•", "LDEF", "D•", "D•")),
            "the primary covered draws five \"•\" dots and the two labels, the damage row first");
        check(f.measured.contains(DotRows.DOT) && !f.measured.contains(DotRows.DOT_ALT), "measured the primary glyph, not the stand-in");
        check(f.backings == 0, "a block standing on its own needs no tooltip backing");


        check(l.dotW == 1 * 4 * 2f, "the layout's dot width is the measured width of the drawn glyph");



        int green = "FontColorPartyGreen".hashCode(), red = "FontColorRed".hashCode();
        check(f.placed.size() == 7 && f.dotColours.size() == 5, "seven draw calls, five of them dots");
        check(f.placed.get(0)[0] == 100f && f.placed.get(0)[1] == 62f && f.placed.get(0)[2] == 2f &&
              f.placed.get(4)[0] == 100f && f.placed.get(4)[1] == 86f, "the labels stand at the block's left, one line apart, k = px / units");
        check(f.placed.get(1)[0] == 133f && f.placed.get(2)[0] == 145f && f.placed.get(3)[0] == 157f && f.placed.get(1)[1] == 62f,
            "the damage dots start after the label column, step by dot width plus space, on the row's baseline");
        check(f.placed.get(5)[0] == 133f && f.placed.get(5)[1] == 86f, "the defence dots line up under the damage dots");
        check(f.dotColours.equals(java.util.Arrays.asList(green, green, green, red, red)), "the better row draws green dots, the worse row red ones");


        Fake t = new Fake();
        t.covered.add(DotRows.DOT); t.covered.add("DMG"); t.covered.add("DEF");
        DotRows r7 = rows(t);
        int code = 4 | 4 << 3;
        r7.draw(new Canvas(), code, r7.layout(code, 24, 20).at(0, 0));
        check(t.drawn.equals(java.util.Arrays.asList("LDMG", "D" + DotRows.DOT, "LDEF", "D" + DotRows.DOT)), "a row without change shows one dot");
        check(t.dotColours.equals(java.util.Arrays.asList("FontColorLightGray".hashCode(), "FontColorLightGray".hashCode())),
            "both no-change dots carry the grey style colour");


        Fake g = new Fake();
        g.covered.add(DotRows.DOT_ALT); g.covered.add("DMG"); g.covered.add("DEF");
        DotRows r2 = rows(g);
        check(r2.ready(), "only the stand-in covered: still ready");
        r2.draw(new Canvas(), CODE, r2.layout(CODE, 24, 20).at(0, 0));
        check(g.measured.contains(DotRows.DOT_ALT) && !g.measured.contains(DotRows.DOT), "measured the stand-in");
        boolean allDots = true;
        for (String s : g.drawn) if (s.startsWith("D") && !s.equals("D·")) allDots = false;
        check(allDots && g.drawn.stream().filter(s -> s.startsWith("D")).count() == 5, "draws only \"·\" dots");


        Fake h = new Fake();
        DotRows r3 = rows(h);
        check(!r3.ready(), "a font covering neither glyph: the rows are not ready");
        r3.draw(new Canvas(), CODE, r3.layout(CODE, 24, 20).at(0, 0));
        check(h.measured.isEmpty() && h.drawn.isEmpty(), "nothing measured, nothing drawn");


        Fake m = new Fake();
        m.covered.add("DMG"); m.covered.add("DEF");
        DotRows r8 = rows(m);
        check(!r8.ready(), "covered labels without a dot glyph: the rows are not ready");
        DotLayout lm = r8.layout(CODE, 24, 20).at(0, 0);
        check(m.measured.equals(java.util.Arrays.asList("DMG", "DEF")), "with no dot glyph the layout measures the covered labels and no dot");
        check(lm.dotW == 0f, "no dot glyph: the layout carries a zero dot width");
        r8.draw(new Canvas(), CODE, lm);
        check(m.drawn.isEmpty(), "with no dot glyph nothing is drawn, not even the covered labels");


        Fake n = new Fake();
        DotRows r5 = rows(n);
        check(!r5.ready(), "no glyph at all is never ready");
        r5.draw(new Canvas(), CODE, new DotLayout(CODE, 24, 20, 10, 10, 5).at(0, 0));
        check(n.drawn.isEmpty(), "draw with no glyph draws nothing even with a layout in hand");


        Fake p = new Fake();
        p.covered.add(DotRows.DOT); p.covered.add("DMG"); p.covered.add("DEF"); p.ready = false;
        DotRows r6 = rows(p);
        check(!r6.ready(), "a font that is not ready gives no rows");
        r6.draw(new Canvas(), CODE, new DotLayout(CODE, 24, 20, 10, 10, 5).at(0, 0));
        check(p.drawn.isEmpty(), "draw with an unready font draws nothing");


        Fake q = new Fake();
        q.covered.add(DotRows.DOT); q.covered.add(DotRows.DOT_ALT); q.covered.add("DMG"); q.covered.add("DEF");
        DotRows r4 = rows(q);
        check(!r4.words(null), "nothing changed: words() stays false");
        q.covered.remove(DotRows.DOT);
        check(r4.words(null), "the primary glyph went away: words() says the rows changed");
        q.measured.clear(); q.drawn.clear();
        r4.draw(new Canvas(), CODE, r4.layout(CODE, 24, 20).at(0, 0));
        check(q.measured.contains(DotRows.DOT_ALT) && !q.measured.contains(DotRows.DOT), "the new layout measures the new glyph");
        check(q.drawn.stream().allMatch(s -> !s.equals("D•")), "the new draw never shows the old glyph");
        q.sizes.put(DotRows.DOT_ALT, 3f);
        check(r4.layout(CODE, 24, 20).dotW == 3 * 4 * 2f, "the layout rebuilt after the glyph change carries the new glyph's measured width");


        int before = q.colours;
        r4.layout(CODE, 24, 20); r4.layout(CODE, 24, 20);
        check(q.colours == before, "the colours stay cached while the style generation does not change");
        q.generation++;
        r4.layout(CODE, 24, 20);
        check(q.colours == before + 3, "a new style generation re-asks the three colours");

        System.out.println(fails == 0 ? "DotRowsTest: all good" : "DotRowsTest: " + fails + " FAILED");
        if (fails > 0) System.exit(1);
    }
}
