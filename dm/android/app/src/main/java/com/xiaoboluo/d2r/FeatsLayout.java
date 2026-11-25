package com.xiaoboluo.d2r;

import java.util.ArrayList;
import java.util.List;









final class FeatsLayout {
    private FeatsLayout() {}

    static final float LIST = 0.30f;


    static int perPage(float top, float bottom, float pitch) { return Math.max(1, (int) (Math.max(0, bottom - top) / Math.max(1, pitch))); }
    static int pages(int rows, int per) { return Math.max(1, (rows + Math.max(1, per) - 1) / Math.max(1, per)); }

    static float groupPitch(float want, float room, int n, float least) { return n <= 0 ? want : Math.max(least, Math.min(want, room / n)); }
    static int clamp(int v, int count) { return Math.max(0, Math.min(Math.max(0, count - 1), v)); }

    interface Measure { float width(String s, float k); }
    interface Shrink { float size(String s, float max, float k); }


    static final class Fit {
        final String text; final float k;
        Fit(String text, float k) { this.text = text; this.k = k; }
    }
    static final Fit NONE = new Fit("", 0);


    static Fit fit(String s, float max, float k, Shrink shrink, Measure m) {
        if (s.isEmpty() || !(max > 0)) return NONE;
        float kk = shrink.size(s, max, k);
        String out = s;
        while (out.length() > 1 && m.width(out, kk) > max) out = out.substring(0, out.length() - 2) + "…";
        return new Fit(out, kk);
    }





    static final class Stamp {
        private boolean built;
        private int texts, fonts, w, h;
        boolean stale(int texts, int fonts, int w, int h) {
            if (built && texts == this.texts && fonts == this.fonts && w == this.w && h == this.h) return false;
            built = true; this.texts = texts; this.fonts = fonts; this.w = w; this.h = h;
            return true;
        }
    }


    static int sameGroup(List<Integer> from, int index, List<Integer> to) {
        return index >= 0 && index < from.size() ? Math.max(0, to.indexOf(from.get(index))) : 0;
    }


    static List<String> cap(List<String> lines, int max) {
        if (lines.size() <= max) return lines;
        List<String> out = new ArrayList<>(lines.subList(0, Math.max(0, max)));
        if (!out.isEmpty()) out.set(out.size() - 1, out.get(out.size() - 1) + " …");
        return out;
    }


    static int[] pad(int cmd, int group, int groups, int page, int pages) {
        int g = group;
        switch (cmd) {
            case PadNav.UP: g = clamp(group - 1, groups); break;
            case PadNav.DOWN: g = clamp(group + 1, groups); break;
            case PadNav.L1: g = groups > 0 ? Math.floorMod(group - 1, groups) : 0; break;
            case PadNav.R1: g = groups > 0 ? Math.floorMod(group + 1, groups) : 0; break;
            case PadNav.LEFT: case PadNav.L2: return new int[]{group, clamp(page - 1, pages)};
            case PadNav.RIGHT: case PadNav.R2: return new int[]{group, clamp(page + 1, pages)};
            default: return new int[]{group, page};
        }
        return new int[]{g, g == group ? page : 0};
    }
}
