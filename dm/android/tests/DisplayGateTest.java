



package com.xiaoboluo.d2r;

public final class DisplayGateTest {
    private static int checks, fails;
    private static void check(boolean ok, String what) { checks++; if (!ok) { fails++; System.out.println("FAIL " + what); } }

    private static int state(boolean show, boolean busy) { return (show ? DisplayGate.SHOW : 0) | (busy ? DisplayGate.BUSY : 0); }

    private static void gate() {
        DisplayGate g = new DisplayGate(); long t = 1000;
        check(!g.shown() && !g.seen(t), "hidden at first");

        check(!g.advance(t += 16, state(true, false)), "frame 1 hidden");
        check(!g.advance(t += 16, state(true, false)), "frame 2 hidden");
        check(g.advance(t += 16, state(true, false)), "frame 3 shown");

        check(g.seen(t) && g.advance(t, state(false, false)), "same frame keeps the value");

        check(!g.advance(t += 16, state(false, false)), "hidden at once");

        for (int i = 0; i < 10; i++) check(!g.advance(t += 16, state(true, true)), "busy holds hidden " + i);
        check(!g.advance(t += 16, state(true, false)) && !g.advance(t += 16, state(true, false)), "two calm frames after busy");
        check(g.advance(t += 16, state(true, false)), "third calm frame after busy shows");

        for (int i = 0; i < 10; i++) check(g.advance(t += 16, state(true, true)), "busy holds shown " + i);

        check(!g.advance(t += 16, state(false, true)), "hide wins over busy");

        g.advance(t += 16, state(true, false)); g.advance(t += 16, state(true, false));
        check(!g.advance(t += 16, state(false, false)), "interrupted");
        check(!g.advance(t += 16, state(true, false)) && !g.advance(t += 16, state(true, false)) && g.advance(t += 16, state(true, false)), "counted again from the start");

        g.reset();
        check(!g.shown() && !g.seen(t), "reset hides and forgets the frame");
        check(!g.advance(t, state(true, false)) && !g.advance(t += 16, state(true, false)) && g.advance(t += 16, state(true, false)), "after reset 3 frames again");
    }

    public static void main(String[] args) {
        gate();
        if (fails > 0) { System.out.println("DisplayGateTest: " + fails + " of " + checks + " failed"); System.exit(1); }
        System.out.println("DisplayGateTest OK (" + checks + " checks)");
    }
}
