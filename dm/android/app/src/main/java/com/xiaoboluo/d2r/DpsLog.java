package com.xiaoboluo.d2r;

import java.util.ArrayList;
import java.util.List;






final class DpsLog {
    static final int CHANGE_LEVEL = 1, CHANGE_POINTS = 2, CHANGE_ATTRS = 4, CHANGE_GEAR = 8, CHANGE_DIFFICULTY = 16;
    static final int MIN_FIGHTS = 3, MIN_RANGE = 5;

    static final class Source {
        char kind;
        int id; String key; long dealt; double dps, lo, hi, share;
        boolean range() { return hi > 0; }
    }
    static final class Segment { int index, ticks, fights, changed, level, difficulty; double value, lo, hi; boolean range() { return hi > 0; } }

    int fights; long ticks; double own, lo, hi, team, teamLo, teamHi;
    boolean sim; double simDps, reached; int simFights;
    final List<Source> sources = new ArrayList<>();
    final List<Segment> segments = new ArrayList<>();

    boolean enough() { return fights >= MIN_FIGHTS; }
    boolean range() { return fights >= MIN_RANGE && hi > 0; }

    boolean companions() { return team > own + 1e-9; }

    boolean table() { return sources.size() >= 2; }
    double seconds() { return ticks / 25.0; }

    static DpsLog parse(String text) {
        DpsLog d = new DpsLog(); boolean head = false;
        for (String line : text.split("\n")) {
            if (line.isEmpty()) continue;
            String[] v = line.split("\t", -1);
            try {
                switch (v[0]) {
                case "dps":
                    if (v.length != 9) throw new IllegalArgumentException();
                    d.fights = CombatModel.i(v[1]); d.ticks = Long.parseLong(v[2]); d.own = h(v[3]); d.lo = h(v[4]); d.hi = h(v[5]); d.team = h(v[6]); d.teamLo = h(v[7]); d.teamHi = h(v[8]);
                    if (d.fights < 0 || d.ticks < 0 || d.lo > d.hi) throw new IllegalArgumentException();
                    head = true; break;
                case "sim":
                    if (v.length != 4) throw new IllegalArgumentException();
                    d.sim = true; d.simDps = h(v[1]); d.reached = CombatModel.d(v[2]) / 10; d.simFights = CombatModel.i(v[3]); break;
                case "src": {
                    if (v.length != 9 || v[1].length() != 1 || "scmruno".indexOf(v[1].charAt(0)) < 0) throw new IllegalArgumentException();
                    Source s = new Source();
                    s.kind = v[1].charAt(0); s.id = CombatModel.i(v[2]); s.key = v[3]; s.dealt = Long.parseLong(v[4]); s.dps = h(v[5]); s.lo = h(v[6]); s.hi = h(v[7]); s.share = CombatModel.d(v[8]) / 10;
                    d.sources.add(s); break; }
                case "seg": {
                    if (v.length != 10 || CombatModel.i(v[1]) != d.segments.size()) throw new IllegalArgumentException();
                    Segment g = new Segment();
                    g.index = CombatModel.i(v[1]); g.ticks = CombatModel.i(v[2]); g.fights = CombatModel.i(v[3]); g.value = h(v[4]); g.lo = h(v[5]); g.hi = h(v[6]);
                    g.changed = CombatModel.i(v[7]); g.level = CombatModel.i(v[8]); g.difficulty = CombatModel.i(v[9]);
                    if (g.ticks < 0 || g.lo > g.hi) throw new IllegalArgumentException();
                    d.segments.add(g); break; }
                default: break;
                }
            } catch (RuntimeException x) { throw new IllegalArgumentException("Invalid DPS view", x); }
        }
        if (!head) throw new IllegalArgumentException("Incomplete DPS view");
        return d;
    }
    private static double h(String v) { return CombatModel.d(v) / 100; }


    double change(int s) {
        if (s <= 0 || s >= segments.size() || segments.get(s - 1).value <= 0) return Double.NaN;
        return (segments.get(s).value / segments.get(s - 1).value - 1) * 100;
    }





    static final class Curve {
        int from, to; double top;
        float[] stepX0, stepX1, stepY, bandY0, bandY1;
        float[] markX;
        final List<float[]> ticks = new ArrayList<>();
    }
    Curve curve(int from, int to, float left, float top, float right, float bottom) {
        Curve c = new Curve(); c.from = from; c.to = to; int n = to - from + 1;
        c.stepX0 = new float[n]; c.stepX1 = new float[n]; c.stepY = new float[n]; c.bandY0 = new float[n]; c.bandY1 = new float[n]; c.markX = new float[n];
        long span = 0; double max = 0;
        for (int i = from; i <= to; i++) { Segment g = segments.get(i); span += Math.max(1, g.ticks); max = Math.max(max, Math.max(g.value, g.hi)); }
        double step = CombatGraph.step(max > 0 ? max : 1); c.top = Math.ceil((max > 0 ? max : 1) / step) * step;
        long at = 0;
        for (int i = from; i <= to; i++) {
            Segment g = segments.get(i); int k = i - from;
            c.stepX0[k] = left + (right - left) * at / (float) span; at += Math.max(1, g.ticks); c.stepX1[k] = left + (right - left) * at / (float) span;
            c.stepY[k] = y(g.value, c.top, top, bottom);
            c.bandY0[k] = g.range() ? y(g.hi, c.top, top, bottom) : Float.NaN; c.bandY1[k] = g.range() ? y(g.lo, c.top, top, bottom) : Float.NaN;
            c.markX[k] = k == 0 ? Float.NaN : c.stepX0[k];
        }
        for (double v = step; v <= c.top + 1e-9; v += step) c.ticks.add(new float[]{y(v, c.top, top, bottom), (float) v});
        return c;
    }
    private static float y(double v, double max, float top, float bottom) { return bottom - (bottom - top) * (float) (v / max); }
}
