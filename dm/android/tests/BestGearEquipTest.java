package com.xiaoboluo.d2r;




public class BestGearEquipTest {
    static int fails;
    static void check(boolean ok, String what) { System.out.println((ok ? "ok   " : "FAIL ") + what); if (!ok) fails++; }

    public static void main(String[] a) {
        check(BestGearEquip.button(1, BestGearEquip.EQ_OK, false) == BestGearEquip.BUTTON_EQUIP, "at the stash, the row goes: Equip");
        check(BestGearEquip.button(1, BestGearEquip.EQ_OK, true) == BestGearEquip.BUTTON_TAKE_OFF, "a row that empties its place: Take off");
        check(BestGearEquip.button(1, BestGearEquip.EQ_NO_ROOM, false) == BestGearEquip.BUTTON_NO_ROOM, "no room: the greyed button");
        check(BestGearEquip.button(1, BestGearEquip.EQ_NO_ROOM, true) == BestGearEquip.BUTTON_NO_ROOM_OFF, "no room for an item taken off: its own greyed button");
        check(BestGearEquip.button(1, BestGearEquip.EQ_NONE, false) == BestGearEquip.BUTTON_NONE, "a charm row: no button");
        check(BestGearEquip.button(1, BestGearEquip.EQ_PENDING, false) == BestGearEquip.BUTTON_NONE, "not known yet: no button");
        check(BestGearEquip.button(0, BestGearEquip.EQ_OK, false) == BestGearEquip.BUTTON_NONE && BestGearEquip.button(-1, BestGearEquip.EQ_OK, false) == BestGearEquip.BUTTON_NONE,
            "away from the stash or not known: no button");
        check(BestGearEquip.all(1, true) && !BestGearEquip.all(1, false) && !BestGearEquip.all(0, true), "equip all: at the stash and when every row goes");
        check(BestGearEquip.hint(0, new int[] { -1, 2 }) && !BestGearEquip.hint(0, new int[] { -1, -1 }) && !BestGearEquip.hint(1, new int[] { 1 })
            && !BestGearEquip.hint(-1, new int[] { 1 }) && !BestGearEquip.hint(0, null), "the hint line: away, and only when a row would have a button");
        check(BestGearEquip.running("queued") && BestGearEquip.running("run") && !BestGearEquip.running("done"), "a job under way");
        check(BestGearEquip.result("done", 1, 1) == BestGearEquip.R_DONE_ONE && BestGearEquip.result("done", 3, 3) == BestGearEquip.R_DONE_N, "done: one or several");
        check(BestGearEquip.result("returned", 0, 2) == BestGearEquip.R_BACK && BestGearEquip.result("partial", 1, 2) == BestGearEquip.R_PARTIAL
            && BestGearEquip.result("partial", 0, 2) == BestGearEquip.R_PARTIAL, "returned: everything back; partial: k of n, also none in place but moved");
        check(BestGearEquip.result("in_hand", 0, 1) == BestGearEquip.R_IN_HAND && BestGearEquip.result("lost", 0, 1) == BestGearEquip.R_LOST, "in hand, lost");
        check(BestGearEquip.result("changed", 0, 1) == BestGearEquip.R_CHANGED && BestGearEquip.result("busy", 0, 1) == BestGearEquip.R_BUSY
            && BestGearEquip.result("away", 0, 1) == BestGearEquip.R_AWAY && BestGearEquip.result("", 0, 0) == BestGearEquip.R_NONE, "refused starts");
        check(BestGearEquip.refused(-1) == BestGearEquip.R_CHANGED && BestGearEquip.refused(-2) == BestGearEquip.R_AWAY && BestGearEquip.refused(-3) == BestGearEquip.R_BUSY,
            "refused taps: changed, away, busy");
        check(BestGearEquip.why("fault") == 8 && BestGearEquip.why("changed") == 0 && BestGearEquip.why("back") == BestGearEquip.WHY.length && BestGearEquip.why("") == BestGearEquip.WHY.length,
            "reasons: known ones by name, any other the last text");
        BestGearEquip.Job j = new BestGearEquip.Job(4, "partial", 1, 3, 0, "fault");
        check(BestGearEquip.overlay(1, true, j, BestGearEquip.R_NONE, false).equals("run_1_3") && BestGearEquip.overlay(1, false, j, BestGearEquip.R_PARTIAL, false).equals("partial_1_3")
            && BestGearEquip.overlay(1, false, null, BestGearEquip.R_NONE, false).equals("ready") && BestGearEquip.overlay(0, false, null, BestGearEquip.R_NONE, false).equals("away")
            && BestGearEquip.overlay(1, false, null, BestGearEquip.R_CHANGED, false).equals("changed"), "the qa overlay words");
        check(BestGearEquip.overlay(1, false, null, BestGearEquip.R_NONE, true).equals("wait") && BestGearEquip.overlay(1, false, null, BestGearEquip.R_DONE_N, true).equals("done")
            && BestGearEquip.overlay(1, true, null, BestGearEquip.R_NONE, true).equals("run") && BestGearEquip.overlay(1, false, null, BestGearEquip.R_UNSURE, false).equals("unsure"),
            "the overlay while the stock is read again: wait, unless a job's line or a job is shown");
        int[] none = { BestGearEquip.BUTTON_NONE, BestGearEquip.BUTTON_NO_ROOM }, live = { BestGearEquip.BUTTON_NONE, BestGearEquip.BUTTON_TAKE_OFF };
        check(BestGearEquip.interim(false, live, false) && BestGearEquip.interim(false, new int[] { BestGearEquip.BUTTON_EQUIP }, false)
            && BestGearEquip.interim(false, none, true), "searching with a live button or equip all: the best so far");
        check(!BestGearEquip.interim(true, live, true) && !BestGearEquip.interim(false, none, false) && !BestGearEquip.interim(false, null, false),
            "done, or no live button: no best-so-far line");
        check(BestGearEquip.done(1, 0) == BestGearEquip.DONE_ON && BestGearEquip.done(1, 1) == BestGearEquip.DONE_OFF && BestGearEquip.done(3, 3) == BestGearEquip.DONE_OFF
            && BestGearEquip.done(5, 3) == BestGearEquip.DONE_MIXED && BestGearEquip.done(2, 0) == BestGearEquip.DONE_ON, "a job done: put on, taken off, or both");
        System.out.println(fails == 0 ? "BestGearEquipTest: all passed" : "BestGearEquipTest: " + fails + " failed");
        System.exit(fails == 0 ? 0 : 1);
    }
}
