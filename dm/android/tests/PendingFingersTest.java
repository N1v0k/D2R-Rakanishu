




package com.xiaoboluo.d2r;

public final class PendingFingersTest {
    private static int checks, fails;
    private static void check(boolean ok, String what) { checks++; if (!ok) { fails++; System.out.println("FAIL " + what); } }

    public static void main(String[] args) {
        final int A = 0x10087, B = 0x30000, NONE = PendingFingers.NONE;
        float slop = 24;
        PendingFingers p = new PendingFingers(16);
        check(!p.held(0) && !p.pending(0) && p.target(0) == NONE && !p.held(-1) && !p.held(16), "empty at first, out-of-range slots are never held");

        p.land(2, A, 100, 500);
        check(p.held(2) && p.pending(2) && p.target(2) == A, "landed: pending with its target");
        check(p.move(2, 110, 510, slop) == NONE && p.pending(2), "a small wobble stays pending");
        check(p.move(2, 100 + slop, 500, slop) == NONE, "exactly the slop is still a tap");
        check(p.up(2, 105, 505, slop) == A && !p.held(2), "lifted in place: a tap on its target, slot freed");
        p.land(2, A, 100, 500);
        check(p.move(2, 110, 500, slop) == NONE && p.up(2, 160, 500, slop) == NONE && !p.held(2), "the last stretch only in the up event: past the slop, no tap, slot freed");

        p.land(3, A, 100, 500);
        check(p.move(3, 100, 500 - slop - 1, slop) == A && !p.held(3), "past the slop: handed over with its target, no longer held");
        check(p.landX(3) == 100 && p.landY(3) == 500, "the landing point stays readable for the hand-over");
        check(p.move(3, 300, 300, slop) == NONE && p.up(3, 300, 300, slop) == NONE, "after the hand-over it is an ordinary finger: no second hand-over, no tap");

        p.land(0, A, 100, 500); p.land(5, B, 120, 900);
        check(p.move(0, 200, 400, slop) == A && p.pending(5), "the other finger stays pending");
        check(p.up(5, 120, 900, slop) == B, "and taps its own target");

        p.land(1, A, 100, 500); p.land(4, B, 120, 900);
        check(p.voidAll() == 2, "two pending fingers voided");
        check(p.held(1) && !p.pending(1) && p.target(1) == NONE, "a voided finger is still held (no raw touch, no native event) but not pending");
        check(p.move(1, 400, 100, slop) == NONE && p.held(1), "a voided finger is never handed over");
        check(p.up(1, 100, 500, slop) == NONE && !p.held(1), "a voided finger lifts without a tap");
        p.land(6, A, 10, 10);
        check(p.pending(6) && p.held(4), "a new finger after voiding is pending again, the old voided one still held");

        check(p.cancel(6) == A && !p.held(6), "cancelling a pending finger returns its target (to release the hold), no tap");
        check(p.cancel(4) == NONE && !p.held(4), "cancelling a voided finger returns nothing");
        check(p.cancel(7) == NONE, "cancelling a free slot does nothing");
        p.land(8, A, 1, 1); p.clear();
        check(!p.held(8), "clear frees everything");
        System.out.println("PendingFingersTest: " + checks + " checks, " + fails + " failed");
        if (fails > 0) System.exit(1);
    }
}
