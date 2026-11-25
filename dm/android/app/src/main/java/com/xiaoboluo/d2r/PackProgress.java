package com.xiaoboluo.d2r;

import java.util.EnumMap;
import java.util.EnumSet;
import java.util.Map;






final class PackProgress {
    enum Step {
        EXTRACT(7), REWRITE(1), ART(8), OVERLAY(1), APP_ART(15), PICTURES(44), SHADERS(23), INSTALL(1);
        final int weight;
        Step(int weight) { this.weight = weight; }
    }

    private final EnumMap<Step, long[]> steps = new EnumMap<>(Step.class);
    private final int weights;
    private int shown;


    PackProgress(Map<Step, Long> totals, int shownBefore) {
        if (totals.isEmpty()) throw new IllegalArgumentException("a build has at least one step");
        if (shownBefore < 0 || shownBefore > 100) throw new IllegalArgumentException("shown value out of range: " + shownBefore);
        int w = 0;
        for (Map.Entry<Step, Long> e : totals.entrySet()) {
            if (e.getValue() <= 0) throw new IllegalArgumentException("step " + e.getKey() + " has no work: " + e.getValue());
            steps.put(e.getKey(), new long[]{0, e.getValue()});
            w += e.getKey().weight;
        }
        weights = w;
        shown = shownBefore;
    }

    EnumSet<Step> steps() { return EnumSet.copyOf(steps.keySet()); }


    void set(Step step, long done) {
        long[] s = steps.get(step);
        if (s == null) throw new IllegalArgumentException("step " + step + " is not part of this build");
        if (done < 0 || done > s[1]) throw new IllegalArgumentException("step " + step + " done " + done + " of " + s[1]);
        s[0] = done;
    }

    boolean complete() {
        for (long[] s : steps.values()) if (s[0] != s[1]) return false;
        return true;
    }


    String timeline() {
        StringBuilder b = new StringBuilder("progress ").append(shown);
        for (Map.Entry<Step, long[]> e : steps.entrySet())
            b.append(' ').append(e.getKey().name().toLowerCase(java.util.Locale.ROOT)).append('=').append(e.getValue()[0]).append('/').append(e.getValue()[1]);
        return b.toString();
    }


    int percent() {
        int now;
        if (complete()) now = 100;
        else {
            double sum = 0;
            for (Map.Entry<Step, long[]> e : steps.entrySet()) sum += e.getKey().weight * ((double) e.getValue()[0] / e.getValue()[1]);
            now = Math.min(99, (int) Math.floor(sum * 100 / weights));
        }
        if (now > shown) shown = now;
        return shown;
    }
}
