package com.xiaoboluo.d2r;

import java.util.ArrayList;
import java.util.List;










final class RecipeStyle {
    private RecipeStyle() {}

    static final float ROW_UNITS = 188, BOX_X = 20, BOX_Y = 20, BOX = 140, TEXT_X = 180;

    static final float NAME_W = 620, STATUS_W = 600, GAP = 30;





    static final float TEXT = 1.5f;



    static String quality(String colour) {
        if (colour == null) return "Tooltip.DefaultColor";
        switch (colour) {
            case "magic": return "Tooltip.MagicColor";
            case "rare": return "Tooltip.RareColor";
            case "set": return "Tooltip.SetColor";
            case "unique": return "Tooltip.UniqueColor";
            case "crafted": return "Tooltip.CraftedColor";
            default: return "Tooltip.DefaultColor";
        }
    }



    static String status(int status) {
        switch (status) {
            case RecipeMatch.READY: return "FontColorPartyGreen";
            case RecipeMatch.IN_STASH: case RecipeMatch.CHAIN: return "FontColorGoldYellow";
            case RecipeMatch.PROBABLE: return "FontColorLightGray";
            default: return "FontColorRed";
        }
    }



    static String input(int state, String code) {
        if (state == RecipeMatch.LACKS) return "FontColorRed";
        if (state == RecipeMatch.CHAINED) return "FontColorGoldYellow";
        return rune(code) ? "Tooltip.CraftedColor" : "FontColorWhite";
    }

    static boolean rune(String code) { return code != null && code.length() == 3 && code.charAt(0) == 'r' && Character.isDigit(code.charAt(1)) && Character.isDigit(code.charAt(2)); }


    static float k(float rowHeight) { return rowHeight > 0 ? rowHeight / ROW_UNITS : 0; }



    static int perPage(float available, float rowMin, boolean keyboard) {
        int fit = rowMin > 0 ? (int) (available / rowMin) : 0;
        return keyboard ? Math.max(1, Math.min(6, fit)) : Math.max(4, Math.min(6, fit));
    }






    static void columns(float left, float right, float k, float nameW, float statusW, float[] out) {
        float statusR = right - GAP * k, statusL = Math.max(left, statusR - statusW);
        float nameL = left + TEXT_X * k, nameR = Math.max(nameL, Math.min(nameL + nameW, statusL - 2 * GAP * k - MIN_INPUTS * k));
        out[0] = nameL; out[1] = nameR; out[2] = Math.min(nameR + GAP * k, statusL - GAP * k); out[3] = statusL - GAP * k; out[4] = statusL; out[5] = statusR;
    }






    interface Measure { float width(String s); }

    static Measure scaled(Measure m, float f) { return s -> m.width(s) * f; }




    static final int CELL_LINES = 3, NAME_LINES = 3, TITLE_LINES = 2;
    static final float MIN_SHRINK = 0.6f, SHRINK_STEP = 0.04f;

    static final float NAME_MIN = 260, MIN_INPUTS = 300;



    static final float CELL_TEXT = 0.96f, CELL_GAP = 28;


    static float cellRoom(float slot, float k) { return Math.max(0, slot - CELL_GAP * k) * CELL_TEXT; }
    static float cellWidth(float text, float k) { return text / CELL_TEXT + CELL_GAP * k; }




    static List<String> wrap(String s, float width, Measure m) {
        List<String> out = new ArrayList<>();
        if (s == null || s.isEmpty()) return out;
        StringBuilder cur = new StringBuilder();
        String[] words = s.split(" ");
        for (int w = 0; w < words.length; w++) {
            String word = words[w];
            int at = 0;
            while (at < word.length()) {
                int dash = word.indexOf('-', at);
                int end = dash < 0 || dash == word.length() - 1 ? word.length() : dash + 1;
                String piece = word.substring(at, end);
                boolean space = at == 0 && cur.length() > 0;
                String next = cur + (space ? " " : "") + piece;
                if (cur.length() == 0 || m.width(next) <= width) { cur.setLength(0); cur.append(next); }
                else { out.add(cur.toString()); cur.setLength(0); cur.append(piece); }
                at = end;
            }
        }
        if (cur.length() > 0) out.add(cur.toString());
        return out;
    }


    static int lines(String s, float width, Measure m) {
        List<String> l = wrap(s, width, m);
        for (String x : l) if (m.width(x) > width) return Integer.MAX_VALUE;
        return l.size();
    }


    static int cellLines(String name, String cond, float width, Measure item, Measure desc) {
        int n = lines(name, width, item);
        if (n == Integer.MAX_VALUE) return n;
        if (cond == null || cond.isEmpty()) return n;
        int c = lines(cond, width, desc);
        return c == Integer.MAX_VALUE ? c : n + c;
    }



    static float cellNeed(String name, String cond, Measure item, Measure desc, float limit, int maxLines) {
        if (cellLines(name, cond, limit, item, desc) > maxLines) return Float.POSITIVE_INFINITY;
        float lo = 0, hi = limit;
        for (int i = 0; i < 24 && hi - lo > 0.5f; i++) {
            float mid = (lo + hi) / 2;
            if (cellLines(name, cond, mid, item, desc) <= maxLines) hi = mid; else lo = mid;
        }
        return hi;
    }




    static float nameNeed(String title, String sub, Measure name, Measure desc, float min, float max, int titleLines) {
        if (!nameFits(title, sub, max, name, desc, titleLines)) return -1;
        if (nameFits(title, sub, min, name, desc, titleLines)) return min;
        float lo = min, hi = max;
        for (int i = 0; i < 24 && hi - lo > 0.5f; i++) {
            float mid = (lo + hi) / 2;
            if (nameFits(title, sub, mid, name, desc, titleLines)) hi = mid; else lo = mid;
        }
        return hi;
    }

    static boolean nameFits(String title, String sub, float width, Measure name, Measure desc, int titleLines) {
        int t = lines(title, width, name);
        if (t > titleLines) return false;
        if (sub == null || sub.isEmpty()) return true;
        int d = lines(sub, width, desc);
        return d != Integer.MAX_VALUE && t + d <= NAME_LINES;
    }


    static final class Layout {
        final float[] cols = new float[6];
        float[] slots = new float[0];
        float f = 1, used, used2;
        int split;
        boolean tall;
        boolean squeezed;
    }

    static void layout(String title, String sub, String[] names, String[] conds, Measure name, Measure item, Measure desc,
                       float left, float right, float k, float statusW, Layout out) {
        layout(title, sub, names, conds, name, item, desc, left, right, k, statusW, false, out);
    }







    static void layout(String title, String sub, String[] names, String[] conds, Measure name, Measure item, Measure desc,
                       float left, float right, float k, float statusW, boolean tall, Layout out) {
        int n = names.length;
        if (out.slots.length != n) out.slots = new float[n];
        out.tall = tall && n > 1;
        out.split = out.tall ? (n + 1) / 2 : n;
        float min = NAME_MIN * k, max = NAME_W * k;
        out.f = 1; out.squeezed = false;
        for (int lines = 1; lines <= CELL_LINES; lines++) {
            for (int tl = 1; tl <= TITLE_LINES; tl++) {
                float nw = nameNeed(title, sub, name, desc, min, max, tl);
                if (nw < 0) continue;
                columns(left, right, k, nw, statusW, out.cols);
                if (out.cols[1] - out.cols[0] < nw - 0.5f) continue;
                if (place(names, conds, item, desc, out.cols[3] - out.cols[2], k, lines, out)) return;
            }
        }
        float nw = nameNeed(title, sub, name, desc, min, max, TITLE_LINES);
        columns(left, right, k, nw < 0 ? max : nw, statusW, out.cols);
        float area = out.cols[3] - out.cols[2];
        for (float f = 1; ; f -= SHRINK_STEP) {
            out.f = f;
            if (place(names, conds, scaled(item, f), scaled(desc, f), area, k, 0, out)) return;
            if (f - SHRINK_STEP < MIN_SHRINK - 1e-4f) break;
        }
        out.squeezed = true;
        for (int b = 0; b < 2; b++) {
            int from = b == 0 ? 0 : out.split, to = b == 0 ? out.split : n;
            float sum = 0;
            for (int i = from; i < to; i++) { if (Float.isInfinite(out.slots[i])) out.slots[i] = area / (to - from); sum += out.slots[i]; }
            for (int i = from; i < to; i++) out.slots[i] *= area / sum;
        }
        out.used = area; out.used2 = out.split < n ? area : 0;
    }





    private static boolean place(String[] names, String[] conds, Measure item, Measure desc, float area, float k, int lines, Layout out) {
        int n = names.length;
        boolean ok = true;
        for (int b = 0; b < 2; b++) {
            int from = b == 0 ? 0 : out.split, m = (b == 0 ? out.split : n) - from;
            float[] needs = new float[m], slots = new float[m];
            for (int i = 0; i < m; i++) {
                String cond = conds[from + i];
                int most = lines == 0 ? CELL_LINES : Math.max(lines, cond == null || cond.isEmpty() ? 1 : 2);
                needs[i] = cellWidth(cellNeed(names[from + i], cond, item, desc, area, most), k);
            }
            float used = m == 0 ? 0 : share(area, needs, m, slots);
            System.arraycopy(slots, 0, out.slots, from, m);
            if (b == 0) out.used = used; else out.used2 = used;
            if (used > area + 0.5f) ok = false;
        }
        return ok;
    }



    static boolean crowded(Layout l) { return l.slots.length > 1 && (l.f < 1 || l.squeezed); }




    static int[] pages(boolean[] tall, int n, int slots) {
        int[] starts = new int[n + 2];
        int p = 0, used = 0;
        for (int i = 0; i < n; i++) {
            int span = tall[i] && slots >= 2 ? 2 : 1;
            if (used > 0 && used + span > slots) { starts[++p] = i; used = 0; }
            used += span;
        }
        starts[++p] = n;
        return java.util.Arrays.copyOf(starts, p + 1);
    }


    static int pageOf(int[] starts, int index) {
        for (int p = 0; p + 1 < starts.length; p++) if (index < starts[p + 1]) return p;
        return Math.max(0, starts.length - 2);
    }






    static float share(float area, float[] needs, int n, float[] out) {
        float sum = 0, base = n == 0 ? 0 : area / Math.max(n, 3);
        for (int i = 0; i < n; i++) sum += needs[i];
        if (sum > area) { for (int i = 0; i < n; i++) out[i] = needs[i]; return sum; }
        float wide = 0;
        for (int i = 0; i < n; i++) { out[i] = Math.max(needs[i], base); wide += out[i]; }
        if (wide > area) { float extra = (area - sum) / n; for (int i = 0; i < n; i++) out[i] = needs[i] + extra; return area; }
        return wide;
    }






    static void cellRows(int lines, float[] out) {
        if (lines <= 2) { out[0] = 48; out[1] = 72; out[2] = 86; out[3] = 42; return; }
        out[0] = 40; out[1] = 60; out[2] = 70; out[3] = Math.min(38, (ROW_UNITS - 74) / lines);
    }





    static void nameRows(int titleLines, int subLines, float[] out) {
        int n = titleLines + subLines;
        if (titleLines == 1 && subLines == 0) { out[0] = 0; out[1] = ROW_UNITS; return; }
        if (titleLines == 1 && subLines == 1) { out[0] = 20; out[1] = 94; out[2] = 94; out[3] = 160; return; }
        float tp = n >= 3 ? 52 : 64, sp = n >= 3 ? 46 : 56, h = titleLines * tp + subLines * sp, y = (ROW_UNITS - h) / 2;
        for (int i = 0; i < n; i++) { float p = i < titleLines ? tp : sp; out[i] = y; out[i + n] = y + p; y += p; }
    }
}
