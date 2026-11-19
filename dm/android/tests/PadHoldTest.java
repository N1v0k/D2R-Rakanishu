




package com.xiaoboluo.d2r;

public final class PadHoldTest {
    private static int checks, fails;
    private static void check(boolean ok, String what) { checks++; if (!ok) { fails++; System.out.println("FAIL " + what); } }



    private static final class Game {
        PadHold h = new PadHold();
        long now = 1000, on, off;
        boolean busy, gateHold, prevPlus, eligible = true;
        int presses, opens, unlatches, dropped;
        boolean exclusive;

        int readEvery = 1, frameNo;
        boolean inflight, liftExclusiveOnWithdraw;
        void frame(int ms) {
            now += ms;
            boolean before = h.plusOut();
            int ev = h.tick(now, busy, on, off, eligible);
            if (liftExclusiveOnWithdraw && before && !h.plusOut()) exclusive = false;
            if ((ev & PadHold.EV_OPEN) != 0) opens++;
            if ((ev & PadHold.EV_UNLATCH) != 0) unlatches++;
            if ((ev & PadHold.EV_DROPPED) != 0) dropped++;
            if (++frameNo % readEvery == 0) read(inflight ? before : h.plusOut());
        }
        void read() { read(h.plusOut()); }

        void read(boolean published) {
            boolean plus = published && !exclusive;
            if (busy) { if (plus) gateHold = true; prevPlus = false; return; }
            if (gateHold) { if (!plus) gateHold = false; plus = false; }
            if (plus) { on++; if (!prevPlus) presses++; } else off++;
            prevPlus = plus;
        }
        void frames(int n) { for (int i = 0; i < n; i++) frame(16); }

        void untilPlus() { for (int i = 0; i < 10 && !h.plusOut(); i++) { now += 16; h.tick(now, busy, on, off, eligible); } }
    }

    private static void shortPress() {
        Game g = new Game();
        check(g.h.down(1, g.now, true), "START owned when a hold is possible");
        check(g.h.masksPlus(), "owned + is masked");
        g.frames(5);
        check(g.h.ring(g.now) < 0, "no ring before 150 ms");
        g.h.up(1, g.now + 10, true, false);
        check(!g.h.masksPlus(), "released: no longer masked");
        g.frames(20);
        check(g.presses == 1, "short press: exactly one press reached the game (" + g.presses + ")");
        check(g.opens == 0, "short press: no app menu");
        check(!g.h.active(), "short press: idle afterwards");
        check(g.on >= PadHold.SEND_READS, "+ held for at least SEND_READS reads");
    }

    private static void hold() {
        Game g = new Game();
        g.h.down(1, g.now, true);
        g.frames(10);
        check(g.h.ring(g.now) >= 0 && g.h.ring(g.now) < 0.1f, "ring starts after 150 ms");
        g.frames(10);
        float mid = g.h.ring(g.now);
        check(mid > 0.4f && mid < 0.6f, "ring half way at ~320 ms (" + mid + ")");
        g.frames(12);
        check(g.opens == 1, "hold: app menu opens once at 500 ms");
        check(g.h.ring(g.now) < 0, "ring gone after opening");
        check(g.h.masksPlus(), "the opening press stays masked while held");
        g.frames(10);
        g.h.up(1, g.now, false, false);
        g.frames(10);
        check(g.opens == 1 && g.presses == 0, "hold: the release is swallowed, no game menu");
        check(!g.h.active() && !g.h.masksPlus(), "hold: idle afterwards");
    }

    private static void releaseAtThresholdBeforeTick() {
        Game g = new Game();
        g.h.down(1, g.now, true);
        g.frames(30);
        g.h.up(1, g.now + 25, true, false);
        g.frames(3);
        check(g.opens == 1 && g.presses == 0, "released at 505 ms before the tick: still a hold");
        check(!g.h.active() && !g.h.masksPlus(), "and idle (the release already came)");
    }

    private static void busyAtOpen() {
        Game g = new Game();
        g.h.down(1, g.now, true);
        g.busy = true;
        g.frames(40);
        check(g.opens == 0 && g.h.ring(g.now) == 1f, "switching at 500 ms: waits with a full ring");
        g.busy = false;
        g.frames(1);
        check(g.opens == 1, "opens when the switch ends");
    }

    private static void shortPressDuringSwitch() {
        Game g = new Game();
        g.busy = true;
        g.h.down(1, g.now, true);
        g.frames(6);
        g.h.up(1, g.now, true, false);
        g.frames(30);
        check(g.presses == 0 && !g.h.plusOut(), "no + while the layout switch runs");
        g.busy = false;
        g.frames(20);
        check(g.presses == 1, "delivered exactly once after the switch (" + g.presses + ")");
    }

    private static void switchStartsBeforeDelivery() {
        Game g = new Game();
        g.h.down(1, g.now, true); g.frames(5); g.h.up(1, g.now, true, false);
        g.untilPlus();
        check(g.h.plusOut(), "+ published");
        g.busy = true;
        g.read();
        g.frames(10);
        check(!g.h.plusOut(), "+ withdrawn during the switch");
        check(g.unlatches == 1, "latch withdrawn once");
        g.busy = false;
        g.frames(30);
        check(g.presses == 1, "sent again after the switch, exactly one press (" + g.presses + ")");
        check(!g.h.active(), "idle afterwards");
    }

    private static void switchAfterPartialDelivery() {
        Game g = new Game();
        g.h.down(1, g.now, true); g.frames(5); g.h.up(1, g.now, true, false);
        g.untilPlus();
        g.frame(16);
        check(g.presses == 1, "first read delivered +");
        g.busy = true;
        g.frames(10);
        g.busy = false;
        g.frames(30);
        check(g.presses == 1, "no resend after a delivered read: still one press (" + g.presses + ")");
        check(!g.h.active(), "idle afterwards");
    }

    private static void exclusivePressStall() {
        Game g = new Game();
        g.exclusive = true;
        g.h.down(1, g.now, true); g.frames(3); g.h.up(1, g.now, true, false);
        g.frames(8);
        check(g.presses == 0 && g.unlatches >= 1, "held back: + withdrawn after STALL_READS");
        g.exclusive = false;
        g.frames(30);
        check(g.presses == 1, "retried and delivered once (" + g.presses + ")");
    }



    private static void withdrawnWhileInFlight() {
        Game g = new Game();
        g.inflight = true; g.exclusive = true; g.liftExclusiveOnWithdraw = true;
        g.h.down(1, g.now, true); g.frames(3); g.h.up(1, g.now, true, false);
        g.frames(60);
        check(g.presses == 1, "withdrawn while a read was in flight: exactly one press (" + g.presses + ")");
        check(!g.h.active() && !g.h.plusOut(), "idle afterwards");
    }

    private static void cancelByTouch() {
        Game g = new Game();
        g.h.down(1, g.now, true);
        g.frames(15);
        check(g.h.cancel() == false, "cancel during a hold: nothing was published");
        check(g.h.ring(g.now) < 0, "ring gone");
        g.frames(40);
        check(g.opens == 0, "no app menu after cancel");
        check(g.h.masksPlus(), "still masked while held");
        g.h.up(1, g.now, true, false);
        g.frames(20);
        check(g.presses == 0 && !g.h.masksPlus() && !g.h.active(), "no game menu either, idle after release");

        Game k = new Game();
        k.h.down(1, k.now, true); k.frames(3); k.h.up(1, k.now, true, false);
        k.untilPlus();
        check(k.h.plusOut() && k.h.cancel(), "cancel while + is published asks for the unlatch");
        check(!k.h.plusOut() && !k.h.active(), "and nothing is left");
    }

    private static void eligibility() {
        Game g = new Game();
        check(!g.h.down(1, g.now, false), "not possible (menu, loading screen …): START not owned, + raw to the game");
        check(!g.h.masksPlus(), "not masked");
        g.h.up(1, g.now + 100, false, false);
        check(!g.h.active(), "nothing queued");

        Game l = new Game();
        l.h.down(1, l.now, true);
        l.frames(10);
        l.eligible = false;
        l.frames(30);
        check(l.opens == 0 && l.h.ring(l.now) < 0, "eligibility lost: cancelled");
        l.h.up(1, l.now, false, false);
        l.eligible = true;
        l.frames(10);
        check(l.presses == 0 && !l.h.active(), "no press after the cancelled hold");

        Game p = new Game();
        p.h.down(1, p.now, true); p.frames(3); p.h.up(1, p.now, true, false);
        p.eligible = false;
        p.frames(10);
        check(p.presses == 0 && !p.h.active(), "queued short press dropped when no longer possible");
    }

    private static void secondPadAndRepeats() {
        Game g = new Game();
        g.h.down(1, g.now, true);
        g.frames(5);
        check(g.h.down(2, g.now, true), "a second pad's START during a hold is owned");
        check(g.h.down(1, g.now, true), "a repeated DOWN of the same pad changes nothing");
        g.h.up(2, g.now, true, false);
        g.frames(5);
        check(g.h.holding(), "pad 2's release does not end pad 1's hold");
        g.h.up(1, g.now, true, false);
        g.frames(20);
        check(g.presses == 1 && g.opens == 0, "pad 1's short press decides: one game menu press");

        Game d = new Game();
        d.h.down(7, d.now, true);
        d.frames(10);
        d.h.deviceGone(7);
        d.frames(40);
        check(d.opens == 0 && d.presses == 0 && !d.h.masksPlus() && !d.h.active(), "pad gone during the hold: nothing, nothing masked");
    }

    private static void timeoutAndBackground() {
        Game g = new Game();
        g.h.down(1, g.now, true); g.frames(3); g.h.up(1, g.now, true, false);
        g.untilPlus();
        check(g.h.plusOut(), "+ published");

        for (int i = 0; i < 140; i++) { g.now += 16; int ev = g.h.tick(g.now, false, g.on, g.off, true); if ((ev & PadHold.EV_DROPPED) != 0) g.dropped++; if ((ev & PadHold.EV_UNLATCH) != 0) g.unlatches++; }
        check(g.dropped == 1 && g.unlatches == 1 && !g.h.plusOut() && !g.h.active(), "timeout drops + and withdraws the latch");

        Game s = new Game();
        s.h.down(1, s.now, true); s.frames(3); s.h.up(1, s.now, true, false);
        s.busy = true;
        for (int i = 0; i < 250; i++) { s.now += 16; s.h.tick(s.now, true, s.on, s.off, true); }
        s.busy = false;
        s.frames(20);
        check(s.presses == 1, "a 4 s layout switch still delivers the press afterwards");

        Game b = new Game();
        b.h.down(1, b.now, true); b.frames(3); b.h.up(1, b.now, true, false); b.untilPlus();
        check(b.h.reset(), "reset with + published asks for the unlatch");
        check(!b.h.active() && !b.h.masksPlus() && !b.h.plusOut(), "reset clears everything");
        Game c = new Game();
        c.h.down(1, c.now, true); c.frames(10);
        c.h.reset();
        c.frames(40);
        check(c.opens == 0 && !c.h.masksPlus(), "reset during a hold: no menu, nothing masked (the release will not come)");
    }


    private static void switchedOff() {
        Game g = new Game();
        g.h.down(4, g.now, true); g.frames(18);
        g.h.up(4, g.now, true, false);
        g.frame(16);
        check(!g.h.plusOut(), "nothing published within PULSE_DELAY_MS");
        g.h.deviceGone(4);
        g.frames(30);
        check(g.presses == 0 && !g.h.active() && !g.h.masksPlus(), "switched off at 300 ms: no game menu, no app menu");
        Game c = new Game();
        c.h.down(4, c.now, true); c.frames(10);
        c.h.up(4, c.now, true, true);
        c.frames(30);
        check(c.presses == 0 && c.opens == 0 && !c.h.active(), "cancelled release: nothing");
        Game o = new Game();
        o.h.down(1, o.now, true); o.frames(3); o.h.up(1, o.now, true, false);
        o.h.deviceGone(9);
        o.frames(30);
        check(o.presses == 1, "another pad gone: the press is still delivered");
    }

    private static void twoQuickPresses() {
        Game g = new Game();
        g.h.down(1, g.now, true); g.frame(16); g.h.up(1, g.now, true, false);
        g.frame(16);
        g.h.down(1, g.now, true); g.frame(16); g.h.up(1, g.now, true, false);
        g.frames(40);
        check(g.presses == 2, "two quick short presses: two separate presses (" + g.presses + ")");

        Game s = new Game();
        s.readEvery = 3;
        s.h.down(1, s.now, true); s.frame(16); s.h.up(1, s.now, true, false);
        s.frame(16);
        s.h.down(1, s.now, true); s.frame(16); s.h.up(1, s.now, true, false);
        s.frames(80);
        check(s.presses == 2, "slow reads: two separate presses (" + s.presses + ")");
    }

    private static void gate() {
        PadGate q = new PadGate();
        final long A = 1, ZR = 1 << 9, X = 1 << 2;
        check(q.apply(false, A, false, false) == A, "free: buttons pass");
        check(q.apply(true, A | ZR, true, false) == 0 && !q.stickL(), "captured: nothing reaches the game");
        long out = q.apply(false, A | ZR, true, false);
        check(out == 0, "closed with A and ZR held: both quarantined");
        check(!q.stickL(), "left stick out at close: quarantined");
        check(q.apply(false, A | ZR | X, true, false) == X, "a new press after closing passes");
        check(q.apply(false, ZR | X, false, false) == X && q.stickL(), "stick back in the centre: free again");
        check(q.apply(false, ZR | X | A, false, false) == (X | A), "A released once, pressed again: passes");
        check(q.apply(false, X, false, false) == X && q.apply(false, X | ZR, false, false) == (X | ZR), "ZR released and pressed again: passes");
        q.apply(true, X, false, true);
        q.reset();
        check(q.apply(false, 0, false, false) == 0 && q.stickR(), "reset clears the quarantine");
    }

    public static void main(String[] a) {
        shortPress(); hold(); releaseAtThresholdBeforeTick(); busyAtOpen(); shortPressDuringSwitch(); switchStartsBeforeDelivery();
        switchAfterPartialDelivery(); exclusivePressStall(); withdrawnWhileInFlight(); cancelByTouch(); eligibility(); secondPadAndRepeats(); timeoutAndBackground();
        twoQuickPresses(); switchedOff(); gate();
        System.out.println("PadHoldTest: " + checks + " checks, " + fails + " failed");
        if (fails > 0) System.exit(1);
    }
}
