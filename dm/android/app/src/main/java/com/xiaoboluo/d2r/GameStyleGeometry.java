package com.xiaoboluo.d2r;








final class GameStyleGeometry {
    static final float SCREEN = 2160f;
    private GameStyleGeometry() {}


    static float k(float canvasHeight) { return canvasHeight > 0 ? canvasHeight / SCREEN : 0; }
    static float kTouch(float canvasHeight, float touchScale) { return k(canvasHeight) * touchScale; }



    static int shadow(float units, float k) {
        if (units == 0 || !(k > 0)) return 0;
        int px = Math.max(1, Math.round(Math.abs(units) * k));
        return units < 0 ? -px : px;
    }



    static float letterSpacing(float kerning, float advanceEm) { return kerning > 0 && advanceEm > 0 ? (kerning - 1f) * advanceEm : 0; }



    static float counterText(GameStyleData.Counter c, float sizeUnits, float field, float smallUnits, float k) {
        return Math.max(sizeUnits / c.field * field, smallUnits * k);
    }






    static void counterBox(GameStyleData.Counter c, float sizeUnits, float cx, float cy, float field, float textSize, float textWidth, float end, float[] out) {
        float s = field / c.field, left = cx - field / 2 + c.x * s, top = cy - field / 2 + c.y * s;
        float height = Math.max(c.h * s, sizeUnits > 0 ? textSize * c.h / sizeUnits : 0);
        float width = Math.max(height * c.w / c.h, textWidth + 2 * end);
        out[0] = left; out[1] = top; out[2] = left + width; out[3] = top + height;
    }






    static void counterBoxAt(GameStyleData.Counter c, float sizeUnits, float right, float bottom, float textSize, float textWidth, float end, float[] out) {
        float height = sizeUnits > 0 ? textSize * c.h / sizeUnits : textSize * 2;
        float width = Math.max(height * c.w / c.h, textWidth + 2 * end);
        out[0] = right - width; out[1] = bottom - height; out[2] = right; out[3] = bottom;
    }









    static final float CORNER_MAX = 0.45f, CORNER_INSET = 0.08f;
    static void corner(float left, float top, float right, float bottom, float px, float textW, float shade, float[] out) {
        float fw = Math.max(0, right - left), fh = Math.max(0, bottom - top), inset = CORNER_INSET * Math.min(fw, fh);
        float p = Math.min(px, CORNER_MAX * fh), perPx = (px > 0 ? textW / px : 0) + shade, room = fw - 2 * inset;
        if (perPx * p > room && perPx > 0) p = Math.max(0, room) / perPx;
        float s = shade * p;
        out[0] = p; out[1] = right - inset - s; out[2] = bottom - inset - s;
    }



    static int digits(int v, char[] out) {
        v = Math.max(0, v);
        int n = 0;
        for (int t = v; ; t /= 10) { n++; if (t < 10) break; }
        n = Math.min(n, out.length);
        for (int i = n - 1; i >= 0; i--) { out[i] = (char) ('0' + v % 10); v /= 10; }
        return n;
    }



    static void threePart(float left, float right, float end, float[] xs) {
        float e = Math.max(0, Math.min(end, (right - left) / 2));
        xs[0] = left; xs[1] = left + e; xs[2] = right - e; xs[3] = right;
    }




    static int tiles(float left, float right, float tile) {
        if (!(right > left) || !(tile > 0)) return 0;
        return Math.max(1, (int) Math.ceil((right - left) / tile - 1e-3f));
    }

    static float fittedTabBaseline(float baseline, float originalSize, float fittedSize) {
        return baseline - (originalSize - fittedSize) * GameStyleData.CAPS_EM / 2;
    }


    static float centreBaseline(float top, float bottom, float ascent, float descent) { return (top + bottom) / 2 - (ascent + descent) / 2; }


    static void tooltipBox(float textW, float textH, float[] pad, float k, float[] out) {
        out[0] = textW + (pad[0] + pad[2]) * k;
        out[1] = textH + (pad[1] + pad[3]) * k;
    }




    static float fit(float width, float max, float k, float units, float floorUnits) {
        if (!(width > max) || !(max > 0) || !(units > floorUnits)) return k;
        return Math.max(k * max / width, k * floorUnits / units);
    }






    static float commonFit(float[] widths, float[] max, int n, float k, float minK) {
        float out = k;
        for (int i = 0; i < n; i++) if (widths[i] > max[i] && max[i] > 0) out = Math.min(out, k * max[i] / widths[i]);
        return Math.max(out, Math.min(k, minK));
    }






    interface Width { float at(float k); }
    static float fitWhole(Width w, float room, float k, float minK) {
        float lo = Math.min(k, minK), out = k, width = w.at(k);
        for (int i = 0; i < FIT_TRIES && room > 0 && width > room && out > lo; i++) {
            out = Math.max(lo, out * room / width * (i == 0 ? 1 : FIT_STEP));
            width = w.at(out);
        }
        return out;
    }
    private static final int FIT_TRIES = 12;
    private static final float FIT_STEP = 0.99f;








    static void pagerHits(float x0, float x1, float y0, float y1, float textW, float prevW, float nextW, float minTap, float minY, float maxX, float maxY,
                          float[] prev, float[] next) {
        float mid = (x0 + x1) / 2, half = Math.max(0, minTap) / 2;
        float prevC = mid - textW / 2 + prevW / 2, nextC = mid + textW / 2 - nextW / 2;
        float bottom = Math.max(y1, Math.min(maxY, y0 + minTap)), top = Math.max(0, Math.min(y0, Math.max(minY, bottom - minTap)));
        prev[0] = Math.max(0, Math.min(Math.min(x0, mid - textW / 2), Math.min(prevC - half, mid - minTap))); prev[1] = top; prev[2] = mid; prev[3] = bottom;
        next[0] = mid; next[1] = top; next[2] = Math.min(maxX, Math.max(Math.max(x1, mid + textW / 2), Math.max(nextC + half, mid + minTap))); next[3] = bottom;
    }











    static final class Panel {


        static final float FRAME_X = -1670, FRAME_Y = 40, FRAME_W = 2878, FRAME_H = 1757, FRAME_SRC_W = 1440, FRAME_SRC_H = 881;


        static final float CONTENT_L = 38, CONTENT_T = 92, CONTENT_R = 1402, CONTENT_B = 794;


        static final float STRIP_X = 75, STRIP_W = 2730, TAB_X = 81.8f, TAB_Y = 9.3f, TAB_H = 89.4f;


        static final float[] TABS5 = {0, 543.5f, 1086.6f, 1630.2f, 2170.8f}, TABS6 = {0, 453.3f, 905.0f, 1356.7f, 1810.8f, 2260.7f};
        static final float TAB5_W = 543.0f, TAB6_W = 451.3f, TAB7_W = 387.6f;


        static final float STRIP_END = Math.max(Math.max(TABS5[4] + TAB5_W, TABS6[5] + TAB6_W), Math.min(7 * TAB7_W, Panel.ENDCAP_R - TAB_X));

        static final float TAB_BODY_H = TAB_H * 36 / 45;
        static final float TAB_BASE = (TAB_BODY_H + GameStyleData.PANEL_TAB.units * GameStyleData.PANEL_EM * GameStyleData.CAPS_EM) / 2;



        static final float ENDCAP_L = 60, ENDCAP_R = 2785.7f, ENDCAP_Y = 7, ENDCAP_W = 30, ENDCAP_H = 82;


        static final float CLOSE_X = 2814.8f, CLOSE_Y = 16.7f, CLOSE_W = 45.9f, CLOSE_H = 65.1f;


        static final float SUBTAB_W = 268, SUBTAB_H = 120, SUBTAB_GAP = 2;
        static final float SUBTAB_BODY_H = SUBTAB_H * 54 / 60;


        static final float BUTTON_W = 658.2f, BUTTON_H = 119.7f, BUTTON_PITCH = 180, BUTTON_BASE = 77.6f;


        static final float MODAL_W = 530, MODAL_H = 102;



        static final float FOOT_W = 470, FOOT_H = 115.6f, FOOT_Y = 1939.2f, FOOT_COL = 500, FOOT_BASE = 73.4f;

        static final int BACKDROP = 0xB3000000;
        static final float TEXT_UNITS = 40;


        static final float SUB_ROOM = 0.9f, MIN_TAB_K = 0.6f;

        float k, s, minTap;
        float frameL, frameT, frameR, frameB;
        float contentL, contentT, contentR, contentB;
        float tabTop, tabBottom, tabBase;
        float closeL, closeT, closeR, closeB;
        float hitCloseL, hitCloseT, hitCloseR, hitCloseB;
        float hitTabBottom;
        float endLl, endLr, endRl, endRr, endT, endB;
        float footT, footB, footW, footBase, footHitT, footHitB;
        float tabX0, tabFit = 1;



        void compute(float w, float h, float consoleScale, float minTap) {
            this.minTap = Math.max(0, minTap);


            k = Math.min(GameStyleGeometry.k(h), w > 0 ? w / (-2 * FRAME_X) : 0); s = k * consoleScale;
            frameL = w / 2 + FRAME_X * k; frameT = FRAME_Y * k; frameR = frameL + FRAME_W * s; frameB = frameT + FRAME_H * s;
            float sx = (frameR - frameL) / FRAME_SRC_W, sy = (frameB - frameT) / FRAME_SRC_H;
            contentL = frameL + CONTENT_L * sx; contentT = frameT + CONTENT_T * sy; contentR = frameL + CONTENT_R * sx; contentB = frameT + CONTENT_B * sy;
            tabTop = frameT + TAB_Y * s; tabBottom = tabTop + TAB_H * s; tabBase = tabTop + TAB_BASE * s;
            closeL = frameL + CLOSE_X * s; closeT = frameT + CLOSE_Y * s; closeR = closeL + CLOSE_W * s; closeB = closeT + CLOSE_H * s;
            endLl = frameL + ENDCAP_L * s; endLr = endLl + ENDCAP_W * s; endRl = frameL + ENDCAP_R * s; endRr = endRl + ENDCAP_W * s;
            endT = frameT + ENDCAP_Y * s; endB = endT + ENDCAP_H * s;
            hitTabBottom = Math.min(Math.max(contentT, this.minTap), h);


            hitCloseL = Math.min(endRl, Math.max(0, w - this.minTap)); hitCloseT = 0;
            hitCloseR = Math.min(w, Math.max(frameR, hitCloseL + this.minTap)); hitCloseB = Math.min(h, Math.max(hitTabBottom, this.minTap));




            tabX0 = frameL + TAB_X * s;
            tabFit = hitCloseL < endRl && s > 0 ? Math.max(0, Math.min(1, (hitCloseL - tabX0) / (STRIP_END * s))) : 1;
            footT = FOOT_Y * k; footB = footT + FOOT_H * k; footW = FOOT_W * k; footBase = footT + FOOT_BASE * k;
            footHitT = contentB; footHitB = h;
        }



        float minTabScale() { return MIN_TAB_K * tabFit; }




        void tab(int n, int i, float[] out) {
            float f = s * tabFit;
            out[0] = tabX0 + tabEdge(n, i, false) * f; out[1] = tabTop; out[2] = tabX0 + tabEdge(n, i, true) * f; out[3] = tabBottom;
        }

        private static float tabEdge(int n, int i, boolean right) {
            if (n == 4 || n == 5) return right ? (i + 1 < 5 ? TABS5[i + 1] : TABS5[4] + TAB5_W) : TABS5[i];
            if (n <= 6) return right ? (i + 1 < 6 ? TABS6[i + 1] : TABS6[5] + TAB6_W) : TABS6[i];
            float each = Math.min(TAB7_W, (ENDCAP_R - TAB_X) / n);
            return (right ? i + 1 : i) * each;
        }



        void searchField(int first, float inset, float[] out) {
            tab(6, first, out);
            float left = out[0] + inset;
            tab(6, 5, out);
            out[0] = left; out[2] = Math.min(out[2], hitCloseL);
        }



        void tabHit(int n, int i, float[] out) {
            tab(n, i, out);
            out[1] = 0; out[3] = hitTabBottom;
            if (out[2] > hitCloseL) out[2] = Math.max(out[0], hitCloseL);
        }



        void subTab(float left, float top, float width, int i, float[] out) {
            float wpx = width > 0 ? width : SUBTAB_W * s, gap = SUBTAB_GAP * s;
            out[0] = left + i * (wpx + gap); out[1] = top; out[2] = out[0] + wpx; out[3] = top + SUBTAB_H * s;
        }

        float subTabWidth(float left, float right, int count) { return count > 0 ? (right - left - (count - 1) * SUBTAB_GAP * s) / count : 0; }
        float subTabH() { return SUBTAB_H * s; }
        float subTabBase(float top, float capsPx) { return top + SUBTAB_BODY_H * s / 2 + capsPx / 2; }



        float subTabK(float boxW, Width w) { float k0 = s * GameStyleData.PANEL_EM; return fitWhole(w, boxW * SUB_ROOM, k0, k0 * MIN_TAB_K); }


        void foot(float w, int col, float[] out) {
            float cx = w / 2 + (col - 1) * FOOT_COL * k;
            out[0] = cx - footW / 2; out[1] = footT; out[2] = cx + footW / 2; out[3] = footB;
        }

        void footHit(float w, int col, float[] out) {
            float cx = w / 2 + (col - 1) * FOOT_COL * k, half = Math.max(FOOT_COL * k, minTap) / 2;
            out[0] = cx - half; out[1] = footHitT; out[2] = cx + half; out[3] = footHitB;
        }

        float buttonW() { return BUTTON_W * s; }
        float buttonH() { return BUTTON_H * s; }
        float buttonPitch() { return BUTTON_PITCH * s; }
        float buttonBase(float top) { return top + BUTTON_BASE * s; }
        float modalW() { return MODAL_W * s; }
        float modalH() { return MODAL_H * s; }


        static float textPx(float unitsPerPx) { return TEXT_UNITS * unitsPerPx * GameStyleData.PANEL_EM; }


        float smallPx(float smallUnits) { return smallUnits * s * GameStyleData.PANEL_EM; }
    }










    static final float CHECK_GAP = 0.3f, CHECK_BOX = 1.05f * 35 / 33f, CHECK_STEP = 1.6f, CHECK_SLACK = 0.98f;
    static float checkRow(float[] labelW, float size, float minSize, float width, float[] maxW) {
        int n = labelW.length;
        if (n == 0 || !(size > 0)) return size;
        float fixed = n * (CHECK_GAP + CHECK_STEP) - (CHECK_STEP - CHECK_BOX), sum = 0;
        for (float x : labelW) sum += Math.max(0, x);
        float sz = size;
        if (sum + fixed * size > width) sz = Math.max(Math.min(minSize, size), size * width * CHECK_SLACK / (sum + fixed * size));
        float f = sz / size, room = Math.max(0, width - fixed * sz), need = sum * f;
        if (need <= room) { for (int i = 0; i < n; i++) maxW[i] = Math.max(0, labelW[i]) * f + (room - need) / n; return sz; }
        boolean[] whole = new boolean[n];
        int left = n; float rest = room;
        for (boolean changed = true; changed && left > 0; ) {
            changed = false; float share = rest / left;
            for (int i = 0; i < n; i++) if (!whole[i] && Math.max(0, labelW[i]) * f <= share) { whole[i] = true; rest -= Math.max(0, labelW[i]) * f; left--; changed = true; }
        }
        for (int i = 0; i < n; i++) maxW[i] = whole[i] ? Math.max(0, labelW[i]) * f : left > 0 ? Math.max(0, rest) / left : 0;
        return sz;
    }







    static final class ItemGrid {
        static final int MAX_ROWS = 2;
        static final float PIC1 = 0.62f, PIC2 = 0.8f, TEXT_AT = 1.05f, TEXT_END = 1.1f;



        static final float WORD_FIT = 0.97f;
        int rows, cols, nameLines;
        float cellW, cellH, pic, textW, nameSize, slotSize;
        float slotAt, nameAt, nameStep;


        void compute(int count, float w, float h, float wordEm, float slotEm, float sizePx, float minPx) {
            float floor = Math.min(minPx, sizePx);
            for (rows = 1; ; rows++) {
                cols = Math.max(1, (count + rows - 1) / rows);
                cellW = w / cols; cellH = h / rows; pic = cellH * (rows == 1 ? PIC1 : PIC2);
                textW = Math.max(0, cellW - pic * TEXT_END);
                slotSize = Math.min(sizePx, rows == 1 ? sizePx : cellH * 0.3f);
                nameSize = wordEm > 0 ? Math.min(sizePx, textW * WORD_FIT / wordEm) : sizePx;
                boolean fits = nameSize >= floor - 0.01f && slotEm * slotSize <= textW + 0.01f;
                if (fits || rows >= MAX_ROWS || rows >= count) break;
            }
            nameSize = Math.max(nameSize, floor);
            nameLines = rows == 1 ? 2 : 1;
            slotAt = rows == 1 ? 0.38f : 0.40f; nameAt = rows == 1 ? 0.62f : 0.82f; nameStep = 0.17f;
        }

        void cell(float left, float top, int i, float[] out) { out[0] = left + (i % cols) * cellW; out[1] = top + (i / cols) * cellH; }
    }







    static final class GearRow {
        static final float SLOT_MIN = 0.085f, SLOT_MAX = 0.13f, PIC = 0.78f, PIC_AT = 0.14f, TEXT_AFTER = 0.05f;
        static final String SEP = " · ";
        interface Measure { float width(String s); }
        float slotW, slotSize, picCx, textX;


        void compute(float left, float w, float h, float pad, float nameEm, float wordEm, float sizePx) {
            float lo = w * SLOT_MIN, hi = w * SLOT_MAX;
            slotW = Math.max(lo, Math.min(hi, nameEm * sizePx / ItemGrid.WORD_FIT));
            slotSize = wordEm > 0 ? Math.min(sizePx, slotW * ItemGrid.WORD_FIT / wordEm) : sizePx;
            float pic = h * PIC;
            picCx = Math.max(left + w * PIC_AT, left + pad + slotW + pic / 2);
            textX = picCx + w * TEXT_AFTER;
        }


        static float nameAt(int bodyLines) { return bodyLines > 2 ? 0.27f : 0.33f; }
        static float bodyAt(int bodyLines, int i) { return bodyLines > 2 ? 0.49f + 0.19f * i : 0.58f + 0.24f * i; }
        static float bodyScale(int bodyLines) { return bodyLines > 2 ? 0.145f : 0.16f; }


        static java.util.List<String> details(String text, float width, int maxLines, Measure m) {
            java.util.List<String> out = new java.util.ArrayList<>();
            if (text == null || text.isEmpty()) return out;
            String line = "";
            for (String part : text.split(java.util.regex.Pattern.quote(SEP))) {
                String next = line.isEmpty() ? part : line + SEP + part;
                if (line.isEmpty() || m.width(next) <= width || out.size() >= maxLines - 1) line = next;
                else { out.add(line); line = part; }
            }
            out.add(line);
            return out;
        }
    }
}
