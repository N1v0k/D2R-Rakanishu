package com.xiaoboluo.d2r;

import java.util.Arrays;





final class FeatsLayoutTest {
    private static int checks;
    private static void check(boolean value, String what) { checks++; if (!value) throw new AssertionError(what); }
    private static boolean at(int[] v, int group, int page) { return Arrays.equals(v, new int[]{group, page}); }

    public static void main(String[] args) {
        check(FeatsLayout.perPage(0, 800, 100) == 8 && FeatsLayout.perPage(0, 50, 100) == 1 && FeatsLayout.perPage(100, 0, 100) == 1, "rows per page");
        check(FeatsLayout.pages(7, 8) == 1 && FeatsLayout.pages(9, 8) == 2 && FeatsLayout.pages(0, 8) == 1 && FeatsLayout.pages(3, 0) == 3, "pages");
        check(FeatsLayout.groupPitch(140, 1000, 5, 60) == 140 && FeatsLayout.groupPitch(140, 600, 5, 60) == 120 && FeatsLayout.groupPitch(140, 200, 5, 60) == 60
            && FeatsLayout.groupPitch(140, 600, 0, 60) == 140, "group rows: the wanted pitch while they fit, else the room's share, not below the floor");
        check(FeatsLayout.clamp(5, 3) == 2 && FeatsLayout.clamp(-1, 3) == 0 && FeatsLayout.clamp(2, 0) == 0, "clamped");

        check(at(FeatsLayout.pad(PadNav.DOWN, 0, 3, 1, 2), 1, 0), "down: the next group, on its first page");
        check(at(FeatsLayout.pad(PadNav.DOWN, 2, 3, 1, 2), 2, 1), "down at the last group: stays, the page too");
        check(at(FeatsLayout.pad(PadNav.UP, 0, 3, 0, 1), 0, 0), "up at the first group: stays");
        check(at(FeatsLayout.pad(PadNav.R1, 2, 3, 0, 1), 0, 0) && at(FeatsLayout.pad(PadNav.L1, 0, 3, 0, 1), 2, 0), "LB / RB go round");
        check(at(FeatsLayout.pad(PadNav.RIGHT, 1, 3, 0, 2), 1, 1) && at(FeatsLayout.pad(PadNav.R2, 1, 3, 1, 2), 1, 1), "right / RT: the next page, up to the last");
        check(at(FeatsLayout.pad(PadNav.LEFT, 1, 3, 1, 2), 1, 0) && at(FeatsLayout.pad(PadNav.L2, 1, 3, 0, 2), 1, 0), "left / LT: the page before, down to the first");
        check(at(FeatsLayout.pad(PadNav.A, 1, 3, 1, 2), 1, 1) && at(FeatsLayout.pad(PadNav.Y, 1, 3, 1, 2), 1, 1), "other buttons change nothing");
        check(at(FeatsLayout.pad(PadNav.R1, 0, 0, 0, 1), 0, 0), "no groups: nothing to select");


        java.util.List<Integer> mastery = Arrays.asList(1, 2, 3), noCombat = Arrays.asList(2, 3);
        check(FeatsLayout.sameGroup(mastery, 2, noCombat) == 1 && FeatsLayout.sameGroup(noCombat, 0, mastery) == 1, "the same group in the other view");
        check(FeatsLayout.sameGroup(mastery, 0, noCombat) == 0 && FeatsLayout.sameGroup(mastery, 5, noCombat) == 0
            && FeatsLayout.sameGroup(java.util.Collections.emptyList(), 0, mastery) == 0, "a group the other view lacks, or none: the first");

        java.util.List<String> two = Arrays.asList("Nicht mitgezählt: Ann,", "Bob");
        check(FeatsLayout.cap(two, 2) == two && FeatsLayout.cap(Arrays.asList("a", "b", "c"), 2).equals(Arrays.asList("a", "b …")), "foot lines");
        check(FeatsLayout.cap(two, 0).isEmpty() && FeatsLayout.cap(two, -3).isEmpty(), "no room: no foot lines");


        FeatsLayout.Stamp laid = new FeatsLayout.Stamp();
        check(laid.stale(1, 0, 1920, 1080), "the first frame builds");
        check(!laid.stale(1, 0, 1920, 1080) && !laid.stale(1, 0, 1920, 1080), "further frames of the same window, texts and fonts keep it");
        check(laid.stale(2, 1, 1920, 1080) && !laid.stale(2, 1, 1920, 1080), "another language (its texts and fonts) builds once again");
        check(laid.stale(3, 1, 1920, 1080) && !laid.stale(3, 1, 1920, 1080), "the page opened again with new data builds once again");
        check(laid.stale(3, 2, 1920, 1080), "new fonts alone build again");
        check(laid.stale(3, 2, 1080, 1920) && laid.stale(3, 2, 1080, 1000) && !laid.stale(3, 2, 1080, 1000), "another window size builds again");


        FeatsLayout.Measure m = (s, k) -> s.length() * 10 * k;
        FeatsLayout.Shrink floorHalf = (s, max, k) -> Math.max(k / 2, Math.min(k, max / (s.length() * 10)));
        FeatsLayout.Fit f = FeatsLayout.fit("Hello", 60, 1, floorHalf, m);
        check(f.text.equals("Hello") && f.k == 1, "fits: whole, at its size");
        f = FeatsLayout.fit("Hello", 40, 1, floorHalf, m);
        check(f.text.equals("Hello") && f.k == 0.8f, "a little too wide: shrunk, whole");
        f = FeatsLayout.fit("Hello world", 40, 1, floorHalf, m);
        check(f.k == 0.5f && f.text.endsWith("…") && m.width(f.text, f.k) <= 40 && "Hello world".startsWith(f.text.substring(0, f.text.length() - 1)), "far too wide: at the floor, cut");
        check(FeatsLayout.fit("", 40, 1, floorHalf, m) == FeatsLayout.NONE && FeatsLayout.fit("Hello", 0, 1, floorHalf, m) == FeatsLayout.NONE, "nothing to draw");
        System.out.println("FeatsLayoutTest: " + checks + " checks");
    }
}
