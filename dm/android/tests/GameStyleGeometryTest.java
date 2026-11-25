






package com.xiaoboluo.d2r;

public final class GameStyleGeometryTest {
    private static int checks, fails;
    private static void check(boolean ok, String what) { checks++; if (!ok) { fails++; System.out.println("FAIL " + what); } }
    private static boolean near(float a, float b) { return Math.abs(a - b) < 1e-3f; }

    private static void tabBodyCenter() {
        for (float[] size : new float[][]{{2000, 900}, {2480, 1116}, {2688, 1216}, {1200, 900}}) {
            GameStyleGeometry.Panel panel = new GameStyleGeometry.Panel();
            panel.compute(size[0], size[1], 1.16f, 48);
            float caps = GameStyleGeometry.Panel.textPx(panel.s) * GameStyleData.CAPS_EM;
            float inkCenter = panel.tabBase - caps / 2;
            float bodyCenter = panel.tabTop + (panel.tabBottom - panel.tabTop) * 36 / 45 / 2;
            check(Math.abs(inkCenter - bodyCenter) < .01f, "tab text centered in the rectangular body: " + size[0]);
            for (float factor : new float[]{1, .8f, .6f}) {
                float originalSize = GameStyleGeometry.Panel.textPx(panel.s), fittedSize = originalSize * factor;
                float baseline = GameStyleGeometry.fittedTabBaseline(panel.tabBase, originalSize, fittedSize);
                check(near(baseline - fittedSize * GameStyleData.CAPS_EM / 2, bodyCenter), "fit preserves tab center");
            }
            float[] four = new float[4], five = new float[4];
            for (int index = 0; index < 4; index++) {
                panel.tab(4, index, four); panel.tab(5, index, five);
                check(near(four[0], five[0]) && near(four[2], five[2]), "four tabs use the available wider slots");
            }
            float subBodyCenter = 100 + GameStyleGeometry.Panel.SUBTAB_H * panel.s * 54 / 60 / 2;
            check(near(panel.subTabBase(100, caps) - caps / 2, subBodyCenter), "subtab body center: " + size[0]);
        }
    }

    private static void scale() {
        check(near(GameStyleGeometry.k(2160), 1) && near(GameStyleGeometry.k(1080), 0.5f) && near(GameStyleGeometry.k(1216), 1216 / 2160f), "k = height / 2160");
        check(GameStyleGeometry.k(0) == 0 && GameStyleGeometry.k(-5) == 0, "no canvas, no k");
        check(near(GameStyleGeometry.kTouch(1080, 0.9f), 0.45f), "kTouch = k × touch scale");

        check(GameStyleGeometry.shadow(3, 1216 / 2160f) == 2, "3 units on a 1216 screen = 2 px");
        check(GameStyleGeometry.shadow(4, 1116 / 2160f) == 2, "4 units on a 1116 screen = 2 px");
        check(GameStyleGeometry.shadow(3, 0.1f) == 1, "never below 1 px");
        check(GameStyleGeometry.shadow(8, 1) == 8 && GameStyleGeometry.shadow(0, 1) == 0 && GameStyleGeometry.shadow(-3, 0.1f) == -1, "8 stays 8, 0 stays 0, sign kept");
        check(GameStyleGeometry.shadow(3, 0) == 0, "no k, no shadow");
        check(near(GameStyleGeometry.letterSpacing(0.88f, 0.6f), -0.072f) && near(GameStyleGeometry.letterSpacing(1, 0.6f), 0), "spacing factor to em");
        check(GameStyleGeometry.letterSpacing(0.88f, 0) == 0, "unknown advance: no spacing");
    }

    private static final GameStyleData.Counter C = GameStyleData.parse(null).counter();

    private static void counter() {
        float k = 0.5f, field = 66, cx = 100, cy = 200, end = 3, size = GameStyleGeometry.counterText(C, 28, field, 26, k);
        check(near(size, 14), "text 28/132 of the field: " + size);
        check(near(GameStyleGeometry.counterText(C, 28, 30, 26, k), 13), "small field: SmallFontSize × k floor");
        float[] one = new float[4], two = new float[4], three = new float[4], min = new float[4];
        GameStyleGeometry.counterBox(C, 28, cx, cy, field, size, 9, end, one);
        GameStyleGeometry.counterBox(C, 28, cx, cy, field, size, 18, end, two);
        GameStyleGeometry.counterBox(C, 28, cx, cy, field, size, 27, end, three);
        GameStyleGeometry.counterBox(C, 28, cx, cy, field, size, 52, end, min);

        check(near(one[0], cx - 33 + 77) && near(one[1], cy - 33 + 39.5f), "box placed as CounterTemplate");
        check(near(one[2] - one[0], 31) && near(one[3] - one[1], 31), "one digit: the square box");
        check(near(two[2] - two[0], 31), "two digits still fit");
        check(near(three[2] - three[0], 33), "three digits widen it (27 + 2 × 3)");
        check(near(min[2] - min[0], 58), "\"2 Min.\" widens it more");
        check(near(one[0], min[0]) && near(one[1], min[1]) && near(one[3], min[3]), "left, top and height never move");


        float[] at1 = new float[4], at2 = new float[4], at0 = new float[4];
        GameStyleGeometry.counterBoxAt(C, 28, 200, 300, 14, 9, end, at1);
        GameStyleGeometry.counterBoxAt(C, 28, 200, 300, 14, 40, end, at2);
        check(near(at1[2], 200) && near(at1[3], 300) && near(at1[3] - at1[1], 31) && near(at1[2] - at1[0], 31), "pinned box: lower right fixed, 31 × 31 for one digit");
        check(near(at2[2], 200) && near(at2[3], 300) && near(at2[2] - at2[0], 46) && near(at2[1], at1[1]), "pinned box: a wide number grows it to the left only (40 + 2 × 3)");
        GameStyleGeometry.counterBoxAt(C, 0, 200, 300, 14, 9, end, at0);
        check(near(at0[3] - at0[1], 28), "pinned box without size units: twice the text size");

        float[] small = new float[4];
        float s2 = GameStyleGeometry.counterText(C, 28, 30, 26, k);
        GameStyleGeometry.counterBox(C, 28, cx, cy, 30, s2, 5, end, small);
        check(near(small[3] - small[1], 13 * 62 / 28f) && near(small[2] - small[0], small[3] - small[1]), "floored text keeps the box square and in proportion");
    }

    private static void parts() {
        float[] xs = new float[4];
        GameStyleGeometry.threePart(10, 50, 6, xs);
        check(near(xs[0], 10) && near(xs[1], 16) && near(xs[2], 44) && near(xs[3], 50), "three parts");
        GameStyleGeometry.threePart(10, 18, 6, xs);
        check(near(xs[1], 14) && near(xs[2], 14), "too narrow: the ends share it");
        GameStyleGeometry.threePart(10, 50, -1, xs);
        check(near(xs[1], 10) && near(xs[2], 50), "no ends");
        check(near(GameStyleGeometry.centreBaseline(0, 40, -10, 4), 23), "centred baseline");
        float[] box = new float[2];
        GameStyleGeometry.tooltipBox(100, 30, new float[]{20, 12, 20, 12}, 0.5f, box);
        check(near(box[0], 120) && near(box[1], 42), "tooltip box = text + padding × k");

        check(near(GameStyleGeometry.fit(80, 100, 0.5f, 36, 26), 0.5f), "fits: unchanged");
        check(near(GameStyleGeometry.fit(120, 100, 0.5f, 36, 26), 0.5f * 100 / 120), "shrunk to fit");
        check(near(GameStyleGeometry.fit(300, 100, 0.5f, 36, 26), 0.5f * 26 / 36), "never below SmallFontSize");
        check(near(GameStyleGeometry.fit(300, 100, 0.5f, 26, 26), 0.5f), "already at the floor: unchanged");
        check(near(GameStyleGeometry.fit(300, 0, 0.5f, 36, 26), 0.5f), "no room given: unchanged");
    }



    private static void corner() {
        float[] out = new float[3];
        float left = 100, top = 200, side = 120, right = left + side, bottom = top + side, shade = 4f / 28f, cap = 0.72f;
        String[] texts = {"7", "42", "120", "2 Min."};
        float[] em = {0.62f, 1.24f, 1.86f, 3.4f};
        float lastRight = -1;
        for (int i = 0; i < texts.length; i++) {
            float px = 34, w = em[i] * px;
            GameStyleGeometry.corner(left, top, right, bottom, px, w, shade, out);
            float p = out[0], tw = em[i] * p, x1 = out[1], base = out[2];
            check(p > 0 && p <= px && p < GameStyleGeometry.CORNER_MAX * side + 1e-3f, texts[i] + ": size " + p + " smaller than the cell");
            check(x1 - tw >= left - 1e-3f && x1 + shade * p <= right + 1e-3f, texts[i] + ": inside left / right with the shadow");
            check(base - cap * p >= top - 1e-3f && base + shade * p <= bottom + 1e-3f, texts[i] + ": inside top / bottom with the shadow");
            check(right - (x1 + shade * p) < 0.15f * side && bottom - (base + shade * p) < 0.15f * side, texts[i] + ": in the lower right corner");
            if (lastRight >= 0) check(near(x1, lastRight) || p < px, texts[i] + ": right-aligned like the others");
            lastRight = x1;
        }
        GameStyleGeometry.corner(left, top, right, bottom, 200, 124, shade, out);
        check(near(out[0], GameStyleGeometry.CORNER_MAX * side), "a big size is capped at CORNER_MAX of the cell");
        GameStyleGeometry.corner(left, top, right, bottom, 34, 3.4f * 34, shade, out);
        check(out[0] < 34 && near(3.4f * out[0] + shade * out[0], side - 2 * GameStyleGeometry.CORNER_INSET * side), "too wide: shrunk to the room");
        char[] d = new char[3];
        check(GameStyleGeometry.digits(0, d) == 1 && d[0] == '0', "digits 0");
        check(GameStyleGeometry.digits(120, d) == 3 && new String(d, 0, 3).equals("120"), "digits 120");
        check(GameStyleGeometry.digits(16, d) == 2 && new String(d, 0, 2).equals("16"), "digits 16");
        check(GameStyleGeometry.digits(-5, d) == 1 && d[0] == '0', "negative digits 0");
    }


    private static void tiles() {
        check(GameStyleGeometry.tiles(0, 100, 50) == 2 && GameStyleGeometry.tiles(0, 101, 50) == 3, "two whole tiles, then a short third");
        check(GameStyleGeometry.tiles(10, 20, 50) == 1, "narrower than one tile = one cut tile");
        check(GameStyleGeometry.tiles(0, 0, 50) == 0 && GameStyleGeometry.tiles(5, 0, 50) == 0 && GameStyleGeometry.tiles(0, 10, 0) == 0, "nothing to tile");
        check(GameStyleGeometry.tiles(0, 100.00001f, 50) == 2, "a rounding crumb makes no extra tile");
    }




    private static boolean px(float got, float want) { return Math.abs(got - want) <= 1f; }
    private static void panel() {
        GameStyleGeometry.Panel p = new GameStyleGeometry.Panel();
        float tap = 7f / 25.4f * 520f;
        p.compute(2688, 1216, 1.16f, tap);
        check(px(p.frameL, 403.7f) && px(p.frameT, 22.9f) && px(p.frameR - p.frameL, 1879.4f) && px(p.frameB - p.frameT, 1147.3f), "frame 403.7 / 22.9 / 1879.4 x 1147.3: " + p.frameL + " " + p.frameT + " " + (p.frameR - p.frameL) + " " + (p.frameB - p.frameT));
        float[] b = new float[4];
        float[] six = {457.3f, 753.3f, 1048.3f, 1343.3f, 1639.8f, 1933.6f}, five = {457.7f, 812.6f, 1167.3f, 1522.3f, 1875.3f};
        for (int i = 0; i < 6; i++) { p.tab(6, i, b); check(px(b[0], six[i]) && px(b[1], 29.0f) && px(b[3] - b[1], 58.4f), "six tabs: tab " + i + " at " + six[i] + ": " + b[0]); }
        p.tab(6, 5, b); check(px(b[2] - b[0], 294.7f), "six tabs: the last 294.7 wide");
        for (int i = 0; i < 5; i++) { p.tab(5, i, b); check(px(b[0], five[i]), "five tabs: tab " + i + " at " + five[i] + ": " + b[0]); }
        p.tab(5, 4, b); check(px(b[2] - b[0], 354.6f), "five tabs: the last 354.6 wide");
        for (int n = 1; n <= 8; n++) {
            float last = -1;
            for (int i = 0; i < n; i++) {
                p.tab(n, i, b);
                if (last >= 0) check(Math.abs(b[0] - last) < 0.01f, n + " tabs: no gap / overlap before tab " + i);
                check(b[2] > b[0] && b[0] >= p.frameL + 75 * p.s - 0.01f && b[2] <= p.endRr, n + " tabs: tab " + i + " inside the strip (the game's own last tab reaches over the right end cap)");
                last = b[2];
            }
        }
        check(px(p.tabBase, 62.5f), "tab text baseline inside the body: " + p.tabBase);
        check(px(GameStyleGeometry.Panel.textPx(p.s) * GameStyleData.CAPS_EM, 22) || px(GameStyleGeometry.Panel.textPx(p.s) * GameStyleData.CAPS_EM, 21), "tab capitals 21-22 px: " + GameStyleGeometry.Panel.textPx(p.s) * GameStyleData.CAPS_EM);
        check(px(GameStyleGeometry.Panel.textPx(p.k) * GameStyleData.CAPS_EM, 17.5f), "footer capitals 17-18 px: " + GameStyleGeometry.Panel.textPx(p.k) * GameStyleData.CAPS_EM);
        check(px(p.closeL, 2242.1f) && px(p.closeT, 33.8f) && px(p.closeR - p.closeL, 30f) && px(p.closeB - p.closeT, 42.5f), "X 2242.1 / 33.8 / 30 x 42.5: " + p.closeL + " " + p.closeT);
        check(px(p.endLl, 443.0f) && px(p.endRl, 2223.0f) && px(p.endT, 27.1f), "end caps 443 / 2223.0 (measured), y 27.1");
        check(px(p.subTabH(), 78.3f), "small tab 78.3 high");
        p.subTab(790.9f, 144.6f, 0, 1, b); check(px(b[0], 790.9f + 175.1f + 1.3f) && px(b[2] - b[0], 175.1f), "small tabs 175.1 wide, 2 units apart");
        check(px(p.buttonW(), 429.8f) && px(p.buttonH(), 78.2f) && px(p.buttonPitch(), 117.5f), "options button 429.8 x 78.2, pitch 117.5");
        check(px(p.buttonBase(328.3f), 328.3f + 50.7f), "options button baseline");
        float[] cols = {931.1f, 1211.7f, 1492.5f};
        for (int i = 0; i < 3; i++) { p.foot(2688, i, b); check(px(b[0], cols[i]) && px(b[1], 1091.7f) && px(b[2] - b[0], 264.5f) && px(b[3] - b[1], 65.1f), "footer button " + i + " at " + cols[i] + ": " + b[0] + " " + b[1]); }
        check(px(p.footBase, 1133), "footer text baseline 1133: " + p.footBase);
        check(px(p.contentT, 142.3f) && px(p.contentB, 1056.5f) && p.contentL > p.frameL && p.contentR < p.frameR, "content area inside the frame");
        check(near(p.modalW(), 530 * p.s) && near(p.modalH(), 102 * p.s), "dialog button 530 x 102 units");


        check(p.hitCloseR - p.hitCloseL >= tap - 0.01f && p.hitCloseB - p.hitCloseT >= tap - 0.01f && p.hitCloseL <= p.closeL && p.hitCloseR >= p.closeR && p.hitCloseB >= p.closeB, "X touch area >= 7 mm around the picture");
        check(p.hitCloseR <= 2688 && p.hitCloseB <= 1216, "X touch area on screen");
        for (int n : new int[]{3, 5, 6, 8}) { p.tabHit(n, n - 1, b); check(b[2] <= p.hitCloseL + 0.01f && b[3] >= tap - 0.01f && b[1] == 0, n + " tabs: the last touch area clear of the X, >= 7 mm"); }
        p.footHit(2688, 0, b); check(b[1] <= 1091.7f && b[3] == 1216 && b[3] - b[1] >= tap, "footer touch area >= 7 mm to the bottom");

        GameStyleGeometry.Panel q = new GameStyleGeometry.Panel();
        for (float[] wh : new float[][]{{1600, 1200}, {1200, 900}, {1080, 1080}}) {
            q.compute(wh[0], wh[1], 1.16f, tap);
            check(q.frameL >= 0 && q.frameR <= wh[0] && q.closeR <= wh[0] && q.hitCloseL < q.hitCloseR && q.hitCloseL <= q.closeL, wh[0] + " x " + wh[1] + ": frame and X on screen: " + q.frameL + " " + q.frameR);
            float[] fb = new float[4]; q.foot(wh[0], 2, fb); check(fb[2] <= wh[0] && fb[3] <= wh[1], wh[0] + " x " + wh[1] + ": footer on screen");
        }
        for (float[] wh : new float[][]{{2480, 1116}, {1920, 1080}, {2400, 1080}}) {
            q.compute(wh[0], wh[1], 1.16f, 0);
            float f = wh[1] / 1216f;
            check(px(q.frameR - q.frameL, 1879.4f * f) && px(wh[0] / 2 - q.frameL, (1344 - 403.7f) * f) && px(q.closeL - wh[0] / 2, (2242.1f - 1344) * f), wh[0] + " x " + wh[1] + ": frame and X scale with the height");
            check(q.frameL >= 0 && q.frameR <= wh[0] && q.frameB <= wh[1], wh[0] + " x " + wh[1] + ": frame on screen");
        }
    }





    private static boolean in(float[] r, float x, float y) { return x >= r[0] && x < r[2] && y >= r[1] && y < r[3]; }
    private static void narrowTabs() {
        GameStyleGeometry.Panel p = new GameStyleGeometry.Panel();
        float[] b = new float[4], h = new float[4];
        p.compute(2688, 1216, 1.16f, 7f / 25.4f * 520f);
        check(p.tabFit == 1, "2688 x 1216: the tabs keep the measured places");
        for (float dpi : new float[]{520, 420, 320}) for (float[] wh : new float[][]{{1200, 900}, {1600, 1200}, {1080, 1080}, {1344, 1216}, {960, 1216}, {2000, 1600}}) {
            float tap = 7f / 25.4f * dpi; p.compute(wh[0], wh[1], 1.16f, tap);
            String at = wh[0] + " x " + wh[1] + " @" + dpi + ": ";
            float[] close = {p.hitCloseL, p.hitCloseT, p.hitCloseR, p.hitCloseB};
            for (int n = 1; n <= 8; n++) {
                for (int i = 0; i < n; i++) {
                    p.tab(n, i, b); p.tabHit(n, i, h);
                    float cx = (b[0] + b[2]) / 2, cy = (b[1] + b[3]) / 2;
                    check(b[2] > b[0], at + n + " tabs: tab " + i + " visible");
                    check(in(h, cx, cy), at + n + " tabs: the centre of tab " + i + " selects it");
                    check(!in(close, cx, cy), at + n + " tabs: the centre of tab " + i + " (" + cx + ") is not on the X (" + p.hitCloseL + ")");
                    if (p.hitCloseL < p.endRl) check(b[2] <= p.hitCloseL + 0.01f, at + n + " tabs: tab " + i + " drawn clear of the X's touch area: " + b[2] + " > " + p.hitCloseL);
                    if (i > 0) { float r0 = b[0]; p.tab(n, i - 1, b); check(Math.abs(b[2] - r0) < 0.01f, at + n + " tabs: no gap / overlap before tab " + i); }
                }
            }
            p.tab(6, 3, b); float l63 = b[0]; p.tab(3, 2, b);
            check(Math.abs(b[2] - l63) < 0.01f, at + "the recipe book's search field (place of tab 4 of 6) starts where its third tab ends");
        }


        p.compute(1200, 900, 1.16f, 7f / 25.4f * 520f);
        check(!in(new float[]{p.hitCloseL, p.hitCloseT, p.hitCloseR, p.hitCloseB}, 1090.56f, 36.88f) || !in(tabAt(p, 8, 7), 1090.56f, 36.88f), "1200 x 900: (1090.56, 36.88) is not both the eighth tab and the X");
    }

    private static void narrowTabFloor() {
        GameStyleGeometry.Panel wide = new GameStyleGeometry.Panel(), narrow = new GameStyleGeometry.Panel();
        float[] a = new float[4], b = new float[4];
        wide.compute(2688, 1216, 1.16f, 7f / 25.4f * 520f);
        check(near(wide.minTabScale(), GameStyleGeometry.Panel.MIN_TAB_K), "wide strips keep the floor");
        for (float dpi : new float[]{520, 420}) for (float[] wh : new float[][]{{1200, 900}, {1080, 1080}, {960, 1216}}) {
            narrow.compute(wh[0], wh[1], 1.16f, 7f / 25.4f * dpi);
            check(narrow.tabFit < 1, wh[0] + " x " + wh[1] + ": the strip is narrowed");
            for (int n = 4; n <= 8; n++) for (int i = 0; i < n; i++) {
                wide.tab(n, i, a); narrow.tab(n, i, b);

                float em = (a[2] - a[0]) / (wide.s * wide.minTabScale());
                check(em * narrow.s * narrow.minTabScale() <= (b[2] - b[0]) + 0.01f, wh[0] + " x " + wh[1] + " @" + dpi + ": tab " + i + " of " + n + " keeps a floor-size label inside its rims");
            }
        }
    }

    private static float[] tabAt(GameStyleGeometry.Panel p, int n, int i) { float[] b = new float[4]; p.tab(n, i, b); return b; }



    private static float rowEnd(float[] w, float size, float sz, float[] max, float x) {
        float at = x, f = sz / size, boxR = x;
        for (int i = 0; i < w.length; i++) {
            at += Math.min(w[i] * f, max[i]) + sz * GameStyleGeometry.CHECK_GAP;
            boxR = at + sz * GameStyleGeometry.CHECK_BOX;
            at += sz * GameStyleGeometry.CHECK_STEP;
        }
        return boxR;
    }
    private static void checkRow() {
        float[] max = new float[3];
        float[] w = {113, 98, 120};
        float sz = GameStyleGeometry.checkRow(w, 30.5f, 23.4f, 1000, max);
        check(sz == 30.5f && max[0] >= 113 && max[1] >= 98 && max[2] >= 120, "wide row: nothing changes");
        sz = GameStyleGeometry.checkRow(w, 30.5f, 23.4f, 381, max);
        check(sz < 30.5f && sz >= 23.4f, "card row shrinks within the floor: " + sz);
        check(rowEnd(w, 30.5f, sz, max, 0) <= 381 + 0.01f, "card row: the last box inside the width: " + rowEnd(w, 30.5f, sz, max, 0));
        for (int i = 0; i < 3; i++) check(max[i] >= w[i] * sz / 30.5f, "card row: label " + i + " whole");
        sz = GameStyleGeometry.checkRow(w, 30.5f, 28f, 300, max);
        check(sz == 28f, "floor kept: " + sz);
        check(rowEnd(w, 30.5f, sz, max, 0) <= 300 + 0.01f, "cut row: the last box inside the width: " + rowEnd(w, 30.5f, sz, max, 0));
        float[] w2 = {40, 200, 200}; sz = GameStyleGeometry.checkRow(w2, 30, 30, 300, max);
        check(near(max[0], 40) && near(max[1], max[2]) && max[1] < 200, "a short label stays whole, the long ones share: " + max[0] + " " + max[1] + " " + max[2]);
        sz = GameStyleGeometry.checkRow(w, 30, 30, 50, max);
        check(max[0] == 0 && max[1] == 0 && max[2] == 0, "no room: the labels are empty, not negative");
    }



    private static void itemGrid() {
        GameStyleGeometry.ItemGrid g = new GameStyleGeometry.ItemGrid();
        float w = 1450, h = 167, size = h * 0.15f, min = 23.4f;
        g.compute(6, w, h, 11.5f, 5.4f, size, min);
        check(g.rows == 2 && g.cols == 3 && g.nameSize >= min && g.nameLines == 1, "German: two rows of three, names at least the game's small text: " + g.rows + " " + g.nameSize);
        check(g.textW >= 11.5f * g.nameSize && g.textW >= 5.4f * g.slotSize, "German: the longest word and slot name fit");
        g.compute(6, w, h, 4.5f, 3.5f, size, min);
        check(g.rows == 1 && g.cols == 6 && near(g.nameSize, size) && g.nameLines == 2, "English: one row of six at full size: " + g.rows + " " + g.nameSize);
        g.compute(6, w, h, 16.5f, 4f, size, min);
        check(g.nameSize < size && g.nameSize >= min && 16.5f * g.nameSize <= g.textW * 0.971f, "the longest word keeps a 3 % margin: " + g.textW + " " + g.nameSize);
        g.compute(6, w, h, 40f, 5f, size, min);
        check(g.rows == 2 && g.nameSize >= min, "never below the small text, two rows at most (long words get cut)");
        g.compute(2, w, h, 11.5f, 5.4f, size, min);
        check(g.rows == 1 && g.cols == 2, "two items: one row of two wide cells");
        g.compute(6, w, h, 11.5f, 5.4f, size, min);
        float[] c0 = new float[2], c5 = new float[2];
        g.cell(100, 50, 0, c0); g.cell(100, 50, 5, c5);
        check(near(c0[0], 100) && near(c0[1], 50) && near(c5[0] + g.cellW, 100 + w) && near(c5[1] + g.cellH, 50 + h), "cells fill the area");
        check(g.cellH * g.nameAt <= g.cellH && g.cellH * g.slotAt + g.nameSize * 0.2f < g.cellH * g.nameAt, "slot line above the name line inside the cell");
    }


    private static void commonFit() {
        float[] w = {100, 180, 120}, room = {150, 150, 150};
        float k = GameStyleGeometry.commonFit(w, room, 3, 1, 0.6f);
        check(Math.abs(k - 150f / 180f) < 1e-4f, "the widest name sets the size for all: " + k);
        for (int i = 0; i < 3; i++) check(w[i] * k <= room[i] + 1e-3f, "name " + i + " fits whole at the shared size");
        check(GameStyleGeometry.commonFit(new float[]{100, 90}, room, 2, 1, 0.6f) == 1, "all fit: the game's size");
        check(GameStyleGeometry.commonFit(new float[]{1000}, room, 1, 1, 0.6f) == 0.6f, "never below the minimum");
    }





    private static final String EXOCET_CHARS = "CEGHIQRSVabcdefgiklmnoprstuvwx‹› /12";
    private static final int[] EXOCET_ADV = {699, 669, 720, 713, 443, 838, 692, 590, 796, 613, 537, 562, 627, 550, 502, 576, 366, 564, 516, 835, 684, 652, 510, 574,
        448, 568, 656, 572, 794, 618, 440, 439, 408, 403, 463, 599};
    private static float exocet(String s, float px, boolean ceil) {
        float w = 0;
        for (int i = 0; i < s.length(); i++) {
            int at = EXOCET_CHARS.indexOf(s.charAt(i));
            if (at < 0) throw new IllegalArgumentException("no advance for " + s.charAt(i));
            float a = EXOCET_ADV[at] / 1000f * px;
            w += ceil ? (float) Math.ceil(a) : Math.round(a);
        }
        return w;
    }



    private static void fitWhole() {
        GameStyleGeometry.Width lin = k -> 100 * k;
        check(GameStyleGeometry.fitWhole(lin, 120, 1, 0.6f) == 1, "fits: the game's size");
        check(near(GameStyleGeometry.fitWhole(lin, 80, 1, 0.6f), 0.8f), "proportional text: the proportional size");
        check(GameStyleGeometry.fitWhole(lin, 10, 1, 0.6f) == 0.6f, "never below the floor");
        GameStyleGeometry.Width stepped = k -> (float) Math.ceil(100 * k / 7) * 7;
        float k = GameStyleGeometry.fitWhole(stepped, 80, 1, 0.6f);
        check(stepped.at(k) <= 80 && k >= 0.6f, "not proportional: measured again until it fits: " + k + " -> " + stepped.at(k));
        check(GameStyleGeometry.fitWhole(lin, 0, 1, 0.6f) == 1, "no room: left alone (nothing to fit into)");
    }






    private static void cubeTabs() {
        String[][] names = {{"Runen", "Edelsteine", "Handwerk", "Sockelfassungen", "Verbessern", "Quest", "Erweiterung"},
                            {"Runes", "Gems", "Crafting", "Sockets", "Improve", "Quest", "Expansion"}};
        float[][] screens = {{2000, 900}, {2288, 1034}, {2688, 1216}, {2480, 1116}, {1600, 1200}, {1200, 900}, {1080, 1080}};
        float margin = 20;
        GameStyleGeometry.Panel p = new GameStyleGeometry.Panel();
        float[] box = new float[4];
        boolean oldCut = false;
        for (float[] wh : screens) {
            p.compute(wh[0], wh[1], 1.16f, 7f / 25.4f * 400f);
            float x = p.contentL + margin * p.s, right = p.contentR - margin * p.s, each = p.subTabWidth(x, right, 7), k0 = p.s * GameStyleData.PANEL_EM;
            for (String[] lang : names) for (int i = 0; i < 7; i++) {
                p.subTab(x, p.contentT + margin * p.s, each, i, box);
                float boxW = box[2] - box[0], room = boxW * GameStyleGeometry.Panel.SUB_ROOM;
                check(box[2] <= right + 0.01f, wh[0] + " x " + wh[1] + ": tab " + i + " inside the content");
                for (boolean ceil : new boolean[]{false, true}) {
                    final String name = lang[i];
                    final boolean up = ceil;
                    GameStyleGeometry.Width w = kk -> exocet(name, 40 * kk, up);
                    float k = p.subTabK(boxW, w);
                    String at = wh[0] + " x " + wh[1] + (ceil ? " (rounded up)" : "") + ": \"" + name + "\" ";
                    check(w.at(k) <= room + 1e-3f, at + "measures whole: " + w.at(k) + " <= " + room);
                    check(k >= k0 * GameStyleGeometry.Panel.MIN_TAB_K - 1e-6f, at + "not below the floor");
                    if (w.at(k0) <= room) check(k == k0, at + "fits at the game's size and keeps it");
                    else check(k < k0, at + "shrinks only because it does not fit");
                    float[] wd = {w.at(k0)}, rm = {room};
                    float linear = GameStyleGeometry.commonFit(wd, rm, 1, k0, k0 * GameStyleGeometry.Panel.MIN_TAB_K);
                    if (!ceil && name.equals("Erweiterung") && wh[0] == 2000 && w.at(linear) > room) oldCut = true;
                }
            }
        }
        check(oldCut, "the old proportional size leaves \"Erweiterung\" too wide at 2000 x 900");
    }




    private static void pagerHits() {
        float[][] screens = {{2480, 1116, 400}, {2000, 900, 400}, {2688, 1216, 520}, {2288, 1034, 450}, {1600, 1200, 220}};
        float[] prev = new float[4], next = new float[4];
        for (float[] sc : screens) {
            float w = sc[0], h = sc[1], tap = 7f / 25.4f * sc[2], g = h / 1080f, m = 0.03f * h;

            float x1c = w - m - 60 * g, bot = h - m - 60 * g, y0 = bot - 44 * g, y1 = bot - 6 * g, pw = Math.min(260 * g, (x1c - (w * 0.36f + 24 * g)) * 0.3f);
            float x0 = x1c - 26 * g - pw, x1 = x1c - 26 * g, px = 60 * g;
            String text = "‹   2 / 2   ›";
            float tw = exocet(text, px, false), aw = exocet("›", px, false), mid = (x0 + x1) / 2;
            GameStyleGeometry.pagerHits(x0, x1, y0, y1, tw, exocet("‹", px, false), aw, tap, y0, w, h, prev, next);
            String at = w + " x " + h + ": ";
            check(tw > x1 - x0, at + "the drawn text is wider than its slot (measured): " + tw + " > " + (x1 - x0));
            float r = mid + tw / 2, l = mid - tw / 2, cy = (y0 + y1) / 2;
            check(next[0] <= r - aw && next[2] >= r && next[1] <= y0 && next[3] >= y1, at + "\"›\" inside next: " + (r - aw) + ".." + r + " in " + next[0] + ".." + next[2]);
            check(prev[0] <= l && prev[2] >= l + aw && prev[1] <= y0 && prev[3] >= y1, at + "\"‹\" inside previous");
            check(prev[2] == mid && next[0] == mid, at + "split at the centre");
            for (float[] a : new float[][]{prev, next}) check(a[2] - a[0] >= tap - 0.01f && a[3] - a[1] >= tap - 0.01f, at + "touch area >= 7 mm: " + (a[2] - a[0]) + " x " + (a[3] - a[1]));
            check(next[2] <= w && next[3] <= h && prev[0] >= 0 && prev[1] >= y0, at + "on screen, nothing above the page line (list rows)");
            check(next[0] <= r - aw / 2 && next[2] >= r - aw / 2 && cy >= next[1] && cy <= next[3], at + "a tap on the middle of \"›\" pages");
        }

        GameStyleGeometry.pagerHits(0, 60, 100, 120, 40, 5, 5, 110, 100, 100, 150, prev, next);
        check(prev[0] == 0 && next[2] == 100 && next[3] == 150 && prev[1] == 100, "clamped to the screen: " + prev[0] + " " + next[2] + " " + next[3]);
        GameStyleGeometry.pagerHits(0, 60, 100, 120, 40, 5, 5, 110, 0, 100, 150, prev, next);
        check(prev[3] - prev[1] >= 110 - 0.01f && prev[3] == 150, "short of room below: grows up as far as allowed");
    }


    private static void gearRow() {
        GameStyleGeometry.GearRow g = new GameStyleGeometry.GearRow();
        float w = 1715, h = 168, size = h * 0.19f, pad = h * 0.16f;
        g.compute(0, w, h, pad, 5.2f, 5.2f, size);
        check(5.2f * g.slotSize <= g.slotW * 0.971f && near(g.slotSize, size), "German off-hand: whole at full size: " + g.slotW);
        check(g.picCx - h * GameStyleGeometry.GearRow.PIC / 2 >= pad + g.slotW - 0.01f, "the picture starts after the slot column");
        check(g.textX >= g.picCx + h * GameStyleGeometry.GearRow.PIC / 2, "the text starts after the picture");
        g.compute(0, w, h, pad, 2f, 2f, size);
        check(near(g.slotW, w * 0.085f) && near(g.picCx, w * 0.14f) && near(g.textX, w * 0.19f), "short slot names: the old layout");
        g.compute(0, w, h, pad, 8.6f, 4.8f, size);
        check(near(g.slotW, w * 0.13f) && near(g.slotSize, size) && 4.8f * g.slotSize <= g.slotW, "two words: the widest column, full size (two lines)");
        g.compute(0, w, h, pad, 9f, 9f, size);
        check(9f * g.slotSize <= g.slotW * 0.971f && g.slotSize < size, "a single long word shrinks to fit whole");
        GameStyleGeometry.GearRow.Measure m = s -> s.length() * 10f;
        String d = "Runenwort · Kristallschwert · 4 Fassungen · benötigt Stufe 25";
        java.util.List<String> one = GameStyleGeometry.GearRow.details(d, 1000, 2, m);
        check(one.size() == 1 && one.get(0).equals(d), "fits: one line");
        java.util.List<String> two = GameStyleGeometry.GearRow.details(d, 450, 2, m);
        check(two.size() == 2 && two.get(1).endsWith("benötigt Stufe 25") && 10f * two.get(0).length() <= 450 && 10f * two.get(1).length() <= 450,
            "too long: two lines split at a dot, the level whole on the second: " + two);
        java.util.List<String> cap = GameStyleGeometry.GearRow.details(d, 150, 2, m);
        check(cap.size() == 2 && cap.get(0).equals("Runenwort") && cap.get(1).endsWith("benötigt Stufe 25"), "never more than two lines, the rest joins the last");
        check(GameStyleGeometry.GearRow.details("", 100, 2, m).isEmpty(), "no details, no lines");
        for (int n = 1; n <= 3; n++) {
            float last = GameStyleGeometry.GearRow.bodyAt(n, n - 1) + GameStyleGeometry.GearRow.bodyScale(n) * 0.3f;
            check(last <= 0.92f && GameStyleGeometry.GearRow.nameAt(n) < GameStyleGeometry.GearRow.bodyAt(n, 0) - GameStyleGeometry.GearRow.bodyScale(n) * 0.75f, n + " text lines stay inside the row");
        }
    }

    public static void main(String[] a) {
        tabBodyCenter(); scale(); counter(); parts(); corner(); tiles(); panel(); narrowTabs(); narrowTabFloor(); checkRow(); itemGrid(); commonFit(); fitWhole(); cubeTabs(); pagerHits(); gearRow();
        System.out.println("GameStyleGeometryTest: " + checks + " checks, " + fails + " failed");
        if (fails != 0) System.exit(1);
    }
}
