




package com.xiaoboluo.d2r;

public final class StatusPlaceTest {
    private static int checks, fails;
    private static void check(boolean ok, String what) { checks++; if (!ok) { fails++; System.out.println("FAIL " + what); } }
    private static boolean near(float a, float b) { return Math.abs(a - b) < 0.01f; }
    private static final long MS = 1_000_000L;
    private static final int U = StatusPlace.UNKNOWN, E = StatusPlace.EMPTY, MERC = 11348, ROW2 = 30000;

    private static void debounce() {
        StatusPlace p = new StatusPlace(); long t = 0;
        check(!p.update(U, t) && p.shown() == U, "starts in the fixed place");
        check(p.update(MERC, t += 16 * MS) && p.shown() == MERC, "first edge is taken at once");
        check(!p.update(MERC, t += 16 * MS), "same edge: no change");

        check(p.update(ROW2, t += 16 * MS) && p.shown() == ROW2, "down at once");
        long gone = t += 16 * MS; check(!p.update(MERC, gone) && p.shown() == ROW2, "up waits");
        check(!p.update(MERC, gone + 499 * MS) && p.shown() == ROW2, "still waits at 499 ms");
        check(p.update(MERC, gone + 500 * MS) && p.shown() == MERC, "up after 500 ms");

        t = gone + 600 * MS; p.update(ROW2, t);
        p.update(MERC, t += 100 * MS); check(p.update(ROW2, t += 100 * MS) || p.shown() == ROW2, "row back");
        long again = t += 16 * MS; p.update(MERC, again);
        check(!p.update(MERC, again + 400 * MS) && p.shown() == ROW2, "timer restarted after the row came back");
        check(p.update(MERC, again + 500 * MS) && p.shown() == MERC, "up 500 ms after the restart");

        check(p.update(MERC - 200, t = again + 600 * MS) && p.shown() == MERC - 200, "small move up at once");
        check(p.update(MERC, t += 16 * MS) && p.shown() == MERC, "small move down at once");

        p.update(ROW2, t += 16 * MS); long w = t += 16 * MS;
        p.update(20000, w); p.update(15000, w + 100 * MS); p.update(18000, w + 200 * MS);
        check(p.update(15000, w + 500 * MS) && p.shown() == 20000, "takes the lowest place seen while waiting");
    }

    private static void unknown() {
        StatusPlace p = new StatusPlace(); long t = 0;
        p.update(MERC, t);

        check(!p.update(U, t += 16 * MS) && p.shown() == MERC, "unknown keeps the place");
        check(!p.update(U, t + 499 * MS) && p.shown() == MERC, "still kept at 499 ms");
        check(p.update(U, t + 500 * MS) && p.shown() == U, "fixed place after 500 ms");
        check(p.update(MERC, t += 600 * MS) && p.shown() == MERC, "edge back: at once");

        p.update(U, t += 16 * MS); check(!p.update(MERC, t += 16 * MS) && p.shown() == MERC, "short unknown spell");
        p.update(ROW2, t += 16 * MS); long w = t += 16 * MS; p.update(MERC, w); p.update(U, w + 100 * MS);
        check(!p.update(MERC, w + 550 * MS) && p.shown() == ROW2, "unknown cancelled the pending move up");
        check(p.update(MERC, w + 1050 * MS) && p.shown() == MERC, "new wait after unknown");

        StatusPlace q = new StatusPlace(); check(!q.update(-7, 0) && q.shown() == U, "bad value is unknown");
        check(!q.update(StatusPlace.MAX + 1, 0) && q.shown() == U, "too large is unknown");
    }

    private static void empty() {
        StatusPlace p = new StatusPlace(); long t = 0;
        p.update(MERC, t);

        check(!p.update(E, t += 16 * MS) && p.shown() == MERC, "empty waits");
        check(p.update(E, t + 500 * MS) && p.shown() == E, "empty after 500 ms");
        check(p.update(MERC, t += 600 * MS) && p.shown() == MERC, "portrait back: at once");

        StatusPlace q = new StatusPlace(); q.update(E, 0); check(q.update(0, 16 * MS) && q.shown() == 0, "0 vs empty");
    }

    private static void layout() {
        float h = 1216, size = h * StatusPlace.SIZE, line = size * StatusPlace.LINE;

        check(near(StatusPlace.baseline(U, h, 7), h * 0.222f + size + 7), "fixed place unchanged");

        float edge = MERC / 100000f * h;
        check(near(StatusPlace.baseline(MERC, h, 7), edge + 0.5f * line + 0.8f * size), "first baseline below the portraits");
        check(StatusPlace.baseline(MERC, h, 0) < StatusPlace.baseline(U, h, 0) - 0.08f * h, "much higher than the fixed place");
        check(near(StatusPlace.baseline(E, h, 0), h * 0.02f + 0.8f * size), "no portraits: top margin");
        check(near(StatusPlace.baseline(0, h, 0), h * 0.02f + 0.8f * size), "edge 0: not above the top margin");
        check(near(StatusPlace.baseline(90000, h, 0), h * 0.34f + 0.8f * size), "a very low edge is clamped");
        check(near(StatusPlace.baseline(-9, h, 3), StatusPlace.baseline(U, h, 3)), "bad value: fixed place");
    }

    private static void reserved() {
        float h = 1116, size = h * StatusPlace.SIZE, line = size * StatusPlace.LINE;

        for (int lines = 0; lines <= 3; lines++) {
            float old = lines > 0 ? h * 0.222f + size + (lines - 1) * line + size * 0.35f + 5 : h * 0.222f + 5;
            check(near(StatusPlace.reserved(U, h, 5, lines), old), "fixed place matches the shipped size for " + lines + " lines");
        }

        check(near(StatusPlace.reserved(MERC, h, 5, 3), StatusPlace.baseline(MERC, h, 5) + 2 * line + 0.35f * size), "three lines below the portraits");
        check(near(StatusPlace.reserved(MERC, h, 5, 0), MERC / 100000f * h), "block off: the portrait edge");
        check(near(StatusPlace.reserved(E, h, 5, 0), h * 0.02f), "block off, no portraits: top margin");
        check(StatusPlace.reserved(MERC, h, 0, 3) < StatusPlace.reserved(U, h, 0, 3), "more room for the player count line");
        check(near(StatusPlace.reserved(200000, h, 0, 0), h * 0.6f), "capped at 60 %");
    }




    private static void effects() {
        float h = 1116, size = h * StatusPlace.SIZE, line = size * StatusPlace.LINE;
        for (int placed : new int[]{MERC, E, U}) {
            for (float inset : new float[]{0, 30}) {
                String at = (placed == MERC ? "portrait" : placed == E ? "empty" : "unknown") + " inset " + inset;
                float first = StatusPlace.baseline(placed, h, inset) - StatusPlace.ASCENT * size;
                check(near(StatusPlace.effectsTop(placed, h, inset, 0), first), at + ": no line = the top of the first line, where the clock would be");
                for (int lines = 1; lines <= 3; lines++)
                    check(near(StatusPlace.effectsTop(placed, h, inset, lines), StatusPlace.reserved(placed, h, inset, lines) + 0.5f * line), at + ": " + lines + " lines = half a line under the block");
                check(near(StatusPlace.effectsTop(placed, h, inset, 3) - StatusPlace.effectsTop(placed, h, inset, 2), line)
                    && near(StatusPlace.effectsTop(placed, h, inset, 2) - StatusPlace.effectsTop(placed, h, inset, 1), line), at + ": each line switched off moves it up by one line");
                check(StatusPlace.effectsTop(placed, h, inset, 1) > StatusPlace.effectsTop(placed, h, inset, 0), at + ": one line is lower than none");
            }
        }
        check(near(StatusPlace.effectsTop(MERC, h, 30, 2), StatusPlace.effectsTop(MERC, h, 0, 2)), "with portraits the cutout top inset does not move it (the host measures picture pixels)");
        check(StatusPlace.effectsTop(U, h, 30, 2) > StatusPlace.effectsTop(U, h, 0, 2), "in the fixed place the cutout top inset moves it down");
        check(near(StatusPlace.effectsTop(E, h, 0, 0), h * StatusPlace.EDGE), "no portraits, no lines: at the top margin");
        check(near(StatusPlace.left(h, 0), h * StatusPlace.EDGE) && near(StatusPlace.left(h, 44), h * StatusPlace.EDGE + 44), "left = EDGE x height + the cutout's left inset");
    }

    public static void main(String[] a) {
        debounce(); unknown(); empty(); layout(); reserved(); effects();
        System.out.println("StatusPlaceTest: " + checks + " checks, " + fails + " failed");
        if (fails != 0) System.exit(1);
    }
}
