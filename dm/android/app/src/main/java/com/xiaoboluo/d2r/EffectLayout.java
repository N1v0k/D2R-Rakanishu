package com.xiaoboluo.d2r;

import java.util.ArrayList;
import java.util.List;















final class EffectLayout {


    static final float RADIUS = 17, PITCH = 40, COLUMN_GAP = 4, CARD_GAP = 6, BOTTOM = 0.86f;
    static final int COLUMNS = 2, MAX_ROWS = 32;
    private EffectLayout() {}



    static float scale(float widthDp, float heightDp, boolean tablet) {
        float fit = Math.min(widthDp / 667f, heightDp / 375f);
        return tablet ? Math.min(1.15f, Math.max(1f, fit)) : Math.min(1f, fit);
    }


    static float unit(float widthPx, float heightPx, float density, boolean tablet) {
        if (!(density > 0) || !(widthPx > 0) || !(heightPx > 0)) return 0;
        return density * scale(widthPx / density, heightPx / density, tablet);
    }


    static int rows(float top, float bottom, float unit) {
        float r = RADIUS * unit, room = bottom - top - 2 * r;
        if (!(unit > 0) || room < 0) return 0;
        return Math.min(MAX_ROWS, 1 + (int) Math.floor(room / (PITCH * unit) + 1e-4f));
    }





    static float reach(float field, float boxRight) { return Math.max(field, boxRight); }
    static float below(float field, float boxBottom) { return Math.max(0, boxBottom - field); }



    static final class Cells {
        final float[] x = new float[COLUMNS * MAX_ROWS], y = new float[COLUMNS * MAX_ROWS];
        int cells, shown, more;
        int cells() { return cells; }
    }



    static float bottomFor(float x0, float x1, float bottom, float[] keep) {
        return keep != null && x1 > keep[0] - keep[2] && x0 < keep[0] + keep[2] ? Math.min(bottom, keep[1] - keep[2]) : bottom;
    }


    static boolean touches(float x0, float y0, float x1, float y1, float[] keep) {
        if (keep == null) return false;
        float dx = Math.max(Math.max(x0 - keep[0], 0), keep[0] - x1), dy = Math.max(Math.max(y0 - keep[1], 0), keep[1] - y1);
        return dx * dx + dy * dy < keep[2] * keep[2];
    }



    private static int column(float x, float top, float bottom, float unit, float reach, float below, float[] keep) {
        int rows = rows(top, bottom - Math.max(0, below - (PITCH - 2 * RADIUS) * unit / 2), unit), n = 0;
        float side = 2 * RADIUS * unit, pitch = PITCH * unit;
        while (n < rows && !touches(x, top + n * pitch, x + reach, top + n * pitch + side + below, keep)) n++;
        return n;
    }





    static Cells place(Cells into, int count, float top, float bottom, float unit, float left, float step, float reach, float below, float[] keep) {
        into.cells = into.shown = into.more = 0;
        if (count <= 0 || !(unit > 0)) return into;
        int capacity = capacity(top, bottom, unit, left, step, reach, below, keep);
        if (capacity <= 0) return into;
        int cells = Math.min(count, capacity), shown = count <= capacity ? count : capacity - 1;
        float r = RADIUS * unit, pitch = PITCH * unit;
        for (int c = 0, i = 0; c < COLUMNS && i < cells; c++) {
            float x = left + step * c;
            for (int row = 0, rows = column(x, top, bottom, unit, reach, below, keep); row < rows && i < cells; row++, i++) {
                into.x[i] = x + r;
                into.y[i] = top + r + row * pitch;
            }
        }
        into.cells = cells; into.shown = shown; into.more = count - shown;
        return into;
    }




    static int capacity(float top, float bottom, float unit, float left, float step, float reach, float below, float[] keep) {
        if (!(unit > 0)) return 0;
        int capacity = 0;
        for (int c = 0; c < COLUMNS; c++) capacity += column(left + step * c, top, bottom, unit, reach, below, keep);
        return capacity;
    }


    static float step(float reach, float unit) { return reach + COLUMN_GAP * unit; }




    static void card(float left, float cy, float width, float height, float top, float bottom, float[] out) {
        float y = cy - height / 2;
        y = Math.max(top, Math.min(y, bottom - height));
        out[0] = left; out[1] = y; out[2] = left + width; out[3] = y + height;
    }

    interface Measure { float width(String s); }


    static List<String> wrap(String text, float max, Measure m) {
        List<String> out = new ArrayList<>();
        StringBuilder line = new StringBuilder();
        for (String word : text.trim().split("\\s+")) {
            if (word.isEmpty()) continue;
            String next = line.length() == 0 ? word : line + " " + word;
            if (line.length() > 0 && m.width(next) > max) { out.add(line.toString()); line.setLength(0); line.append(word); }
            else { line.setLength(0); line.append(next); }
        }
        if (line.length() > 0) out.add(line.toString());
        return out;
    }



    static List<String> breakWord(String text, float max, Measure m) {
        List<String> out = new ArrayList<>();
        if (m.width(text) <= max) { out.add(text); return out; }
        for (int start = 0; start < text.length(); ) {
            int end = text.offsetByCodePoints(start, 1);
            while (end < text.length()) {
                int next = text.offsetByCodePoints(end, 1);
                if (m.width(text.substring(start, next)) > max) break;
                end = next;
            }
            out.add(text.substring(start, end));
            start = end;
        }
        return out;
    }




    static int[] cardPages(float[] heights, float room, float paging, int page) {
        float total = 0;
        for (float h : heights) total += h;
        if (total <= room || heights.length == 0) return new int[]{1, 0, 0, heights.length};
        List<Integer> starts = new ArrayList<>();
        float used = 0, fit = room - paging;
        for (int i = 0; i < heights.length; i++) {
            if (starts.isEmpty() || used > 0 && used + heights[i] > fit) { starts.add(i); used = 0; }
            used += heights[i];
        }
        int p = Math.max(0, Math.min(page, starts.size() - 1));
        return new int[]{starts.size(), p, starts.get(p), p + 1 < starts.size() ? starts.get(p + 1) : heights.length};
    }




    static final class CardKey {
        private String text; private float w, h, top, bottom, left, cy, keep; private int generation = -1, page = -1;
        boolean changed(String text, float w, float h, float top, float bottom, float left, float cy, int generation, int page, float keep) {
            boolean same = text.equals(this.text) && w == this.w && h == this.h && top == this.top && bottom == this.bottom && left == this.left && cy == this.cy
                && generation == this.generation && page == this.page && keep == this.keep;
            this.text = text; this.w = w; this.h = h; this.top = top; this.bottom = bottom; this.left = left; this.cy = cy; this.generation = generation; this.page = page; this.keep = keep;
            return !same;
        }
        void page(int page) { this.page = page; }
        void reset() { text = null; }
    }
}
