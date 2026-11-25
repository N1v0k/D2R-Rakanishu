package com.xiaoboluo.d2r;

import java.util.EnumMap;
import java.util.Map;


public class PackProgressTest {
    private static int checks, fails;
    private static void check(boolean ok, String what) { checks++; if (!ok) { fails++; System.err.println("FAIL " + what); } }

    private static Map<PackProgress.Step, Long> all(long n) {
        Map<PackProgress.Step, Long> m = new EnumMap<>(PackProgress.Step.class);
        for (PackProgress.Step s : PackProgress.Step.values()) m.put(s, n);
        return m;
    }

    public static void main(String[] args) {
        int sum = 0;
        for (PackProgress.Step s : PackProgress.Step.values()) sum += s.weight;
        check(sum == 100, "the weights of all steps add up to 100: " + sum);

        PackProgress p = new PackProgress(all(1000), 0);
        check(p.percent() == 0, "nothing done: 0");
        p.set(PackProgress.Step.EXTRACT, 1000);
        int total = 0;
        for (PackProgress.Step st : PackProgress.Step.values()) total += st.weight;
        check(total == 100, "the weights add up to 100: " + total);
        int ex = PackProgress.Step.EXTRACT.weight, sh = PackProgress.Step.SHADERS.weight, pi = PackProgress.Step.PICTURES.weight, in = PackProgress.Step.INSTALL.weight;
        check(p.percent() == ex, "extraction done = its weight: " + p.percent());
        p.set(PackProgress.Step.SHADERS, 500);
        p.set(PackProgress.Step.PICTURES, 500);
        check(p.percent() == (int) Math.floor(ex + sh / 2.0 + pi / 2.0), "lanes side by side add up: " + p.percent());

        Map<PackProgress.Step, Long> rebuild = new EnumMap<>(PackProgress.Step.class);
        rebuild.put(PackProgress.Step.SHADERS, 6927L);
        rebuild.put(PackProgress.Step.INSTALL, 7L);
        PackProgress r = new PackProgress(rebuild, 0);
        r.set(PackProgress.Step.SHADERS, 6927);
        check(r.percent() == (int) Math.floor(sh * 100.0 / (sh + in)), "a rebuild rescales the remaining steps to 100: " + r.percent());
        r.set(PackProgress.Step.INSTALL, 6);
        check(r.percent() == (int) Math.floor((sh + in * 6.0 / 7) * 100 / (sh + in)), "most of the install: " + r.percent());
        r.set(PackProgress.Step.INSTALL, 7);
        check(r.percent() == 100, "everything done: 100");

        Map<PackProgress.Step, Long> one = new EnumMap<>(PackProgress.Step.class);
        one.put(PackProgress.Step.EXTRACT, 1_000_000L);
        PackProgress e = new PackProgress(one, 0);
        e.set(PackProgress.Step.EXTRACT, 999_999);
        check(e.percent() == 99, "one byte short of the end never shows 100: " + e.percent());
        e.set(PackProgress.Step.EXTRACT, 1_000_000);
        check(e.percent() == 100, "the last byte: 100");

        PackProgress resumed = new PackProgress(one, 42);
        resumed.set(PackProgress.Step.EXTRACT, 300_000);
        check(resumed.percent() == 42, "after a resume the repeated unit keeps the bar where it was: " + resumed.percent());
        resumed.set(PackProgress.Step.EXTRACT, 500_000);
        check(resumed.percent() == 50, "and moves on once the work passes it");
        resumed.set(PackProgress.Step.EXTRACT, 400_000);
        check(resumed.percent() == 50, "a step reporting less than before does not move the bar back");

        boolean refused = false;
        try { new PackProgress(one, 0).set(PackProgress.Step.SHADERS, 1); } catch (IllegalArgumentException x) { refused = true; }
        check(refused, "a step outside the build is refused");
        refused = false;
        try { new PackProgress(one, 0).set(PackProgress.Step.EXTRACT, 1_000_001); } catch (IllegalArgumentException x) { refused = true; }
        check(refused, "more work than the step has is refused");
        refused = false;
        Map<PackProgress.Step, Long> empty = new EnumMap<>(PackProgress.Step.class);
        empty.put(PackProgress.Step.EXTRACT, 0L);
        try { new PackProgress(empty, 0); } catch (IllegalArgumentException x) { refused = true; }
        check(refused, "a step without work is refused");

        System.out.println("PackProgressTest: " + checks + " checks, " + fails + " failed");
        if (fails > 0) System.exit(1);
    }
}
