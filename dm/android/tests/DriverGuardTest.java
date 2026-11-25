package com.xiaoboluo.d2r;


public class DriverGuardTest {
    static int fails;
    static void check(boolean ok, String what) { System.out.println((ok ? "ok   " : "FAIL ") + what); if (!ok) fails++; }

    static DriverGuard.Input in(String zip, String attempt) { DriverGuard.Input i = new DriverGuard.Input(); i.zip = zip; i.attemptId = attempt; return i; }

    public static void main(String[] a) {
        DriverGuard.State s = new DriverGuard.State();

        DriverGuard.Decision d = DriverGuard.decide(s, in(null, "a0"));
        check(!d.custom && d.state.empty(), "no package: system driver, state stays empty");


        d = DriverGuard.decide(s, in("Z1", "a1"));
        check(d.custom && d.state.pending.equals("a1") && d.state.pendingZip.equals("Z1") && d.state.fails == 0, "package: own driver, attempt a1 pending");


        DriverGuard.Input i = in("Z1", "a2"); i.okId = "a1";
        DriverGuard.Decision d2 = DriverGuard.decide(d.state, i);
        check(d2.custom && d2.state.fails == 0 && d2.state.pending.equals("a2") && d2.state.notice.isEmpty(), "finished start: count zero, next attempt a2");


        DriverGuard.Decision d3 = DriverGuard.decide(d2.state, in("Z1", "a3"));
        check(d3.custom && d3.state.fails == 1 && d3.state.notice.isEmpty(), "one unfinished start: count 1, own driver again");

        DriverGuard.Decision d4 = DriverGuard.decide(d3.state, in("Z1", "a4"));
        check(!d4.custom && d4.state.blocked.equals("Z1") && d4.state.notice.equals(DriverGuard.FELL_BACK) && d4.state.pending.isEmpty() && d4.state.fails == 0,
            "second unfinished start in a row: blocked, system driver, notice 'fell back'");

        DriverGuard.Decision d5 = DriverGuard.decide(d4.state, in("Z1", "a5"));
        check(!d5.custom && d5.state.notice.equals(DriverGuard.FELL_BACK), "blocked package stays on the system driver, notice still due");

        DriverGuard.State shown = DriverGuard.noticeShown(d5.state);
        DriverGuard.Decision d6 = DriverGuard.decide(shown, in("Z1", "a6"));
        check(!d6.custom && d6.state.notice.isEmpty() && d6.state.blocked.equals("Z1"), "after the notice was shown: no second notice, still blocked");

        DriverGuard.Decision d7 = DriverGuard.decide(d6.state, in(null, "a7"));
        check(!d7.custom && d7.state.blocked.isEmpty() && d7.state.fails == 0, "package removed: block cleared");

        DriverGuard.Decision d8 = DriverGuard.decide(d7.state, in("Z1", "a8"));
        check(d8.custom && d8.state.pending.equals("a8"), "same package put back: tried again");


        DriverGuard.Input f = in("Z1", "a9"); f.fallbackId = "a8"; f.fallbackReason = "adrenotools could not open the loader";
        DriverGuard.Decision d9 = DriverGuard.decide(d8.state, f);
        check(!d9.custom && d9.state.blocked.equals("Z1") && d9.state.notice.equals(DriverGuard.LOAD_FAILED) && d9.state.noticeArg.startsWith("adrenotools"),
            "early failure marker: blocked at once, notice 'could not be loaded' with reason");

        DriverGuard.State live = DriverGuard.fellBackNow(d8.state, "a8", "no Vulkan device");
        check(live.pending.isEmpty() && live.blocked.equals("Z1") && live.notice.equals(DriverGuard.LOAD_FAILED), "live fallback: blocked, attempt settled");
        DriverGuard.Input g = in("Z1", "b1"); g.fallbackId = "a8";
        DriverGuard.Decision d10 = DriverGuard.decide(live, g);
        check(!d10.custom && d10.state.fails == 0 && d10.state.notice.equals(DriverGuard.LOAD_FAILED), "the marker of a live fallback is not counted twice");
        check(DriverGuard.fellBackNow(d8.state, "other", "x").equals(d8.state), "a live fallback of another attempt changes nothing");


        DriverGuard.Decision d11 = DriverGuard.decide(d4.state, in("Z2", "c1"));
        check(d11.custom && d11.state.blocked.isEmpty() && d11.state.fails == 0 && d11.state.pendingZip.equals("Z2"), "a different package: tried, old block lifted");
        DriverGuard.Decision mid = DriverGuard.decide(d.state, in("Z1", "a1b"));
        DriverGuard.Decision swap = DriverGuard.decide(mid.state, in("Z2", "c2"));
        check(swap.custom && swap.state.fails == 0, "count does not carry over to a different package");


        DriverGuard.Input sf = in("Z1", "s1"); sf.safe = true;
        DriverGuard.Decision ds = DriverGuard.decide(d.state, sf);
        check(!ds.custom && ds.state.pending.isEmpty() && ds.state.fails == 1 && ds.state.notice.equals(DriverGuard.SAFE), "safe start: system, previous attempt settled (1), notice 'safe'");
        DriverGuard.Decision after = DriverGuard.decide(DriverGuard.noticeShown(ds.state), in("Z1", "s2"));
        check(after.custom && after.state.fails == 1, "after a safe start the package is tried again with the count kept");


        DriverGuard.Input bad = in("Z3", "r1"); bad.problem = "elf-arch";
        DriverGuard.Decision r1 = DriverGuard.decide(new DriverGuard.State(), bad);
        check(!r1.custom && r1.state.notice.equals(DriverGuard.REJECTED) && r1.state.noticeArg.equals("elf-arch"), "rejected package: system, notice with reason");
        DriverGuard.Decision r2 = DriverGuard.decide(DriverGuard.noticeShown(r1.state), bad);
        check(!r2.custom && r2.state.notice.isEmpty(), "same rejected package again: no second notice");
        DriverGuard.Input many = new DriverGuard.Input(); many.problem = "many"; many.attemptId = "m1";
        DriverGuard.Decision r3 = DriverGuard.decide(DriverGuard.noticeShown(r1.state), many);
        check(!r3.custom && r3.state.notice.equals(DriverGuard.REJECTED) && r3.state.noticeArg.equals("many"), "more than one ZIP: system, its own notice");
        DriverGuard.Decision r4 = DriverGuard.decide(r1.state, in(null, "n1"));
        check(r4.state.notice.isEmpty() && r4.state.noticeKey.isEmpty(), "package gone: its 'rejected' notice goes too");
        DriverGuard.Decision r5 = DriverGuard.decide(d4.state, in(null, "n2"));
        check(r5.state.notice.equals(DriverGuard.FELL_BACK), "package gone: a 'fell back' notice not yet shown stays");


        DriverGuard.State cancelled = DriverGuard.cancel(d.state, "a1");
        DriverGuard.Decision c1 = DriverGuard.decide(cancelled, in("Z1", "a2"));
        check(c1.custom && c1.state.fails == 0, "cancelled attempt is not counted");



        DriverGuard.State shown4 = DriverGuard.noticeShown(d4.state);
        DriverGuard.State forgot = DriverGuard.forgotten(shown4, "Z1");
        check(forgot.blocked.isEmpty() && forgot.pendingZip.isEmpty() && forgot.fails == 0, "a deleted package: block and count are gone");
        DriverGuard.Decision again = DriverGuard.decide(forgot, in("Z1", "f1"));
        check(again.custom && again.state.pending.equals("f1") && again.state.fails == 0, "the same ZIP after delete and re-add starts with a clean count");
        check(DriverGuard.forgotten(shown4, "Z9").equals(shown4), "deleting another ZIP leaves the state alone");
        DriverGuard.State pending1 = DriverGuard.forgotten(d.state, "Z1");
        check(pending1.pending.isEmpty() && pending1.pendingZip.isEmpty(), "a deleted ZIP takes its open attempt out of the accounting too");
        check(DriverGuard.forgotten(d4.state, "Z1").notice.isEmpty(), "a deleted ZIP takes its not-yet-shown notice with it");
        check(DriverGuard.forgotten(r1.state, "Z3").notice.isEmpty() && DriverGuard.forgotten(r1.state, "Z3").noticeKey.isEmpty(), "a deleted ZIP clears the 'rejected' notice that named it");



        DriverGuard.State re = DriverGuard.chosenAgain(d5.state, "Z1");
        check(re.blocked.isEmpty() && re.fails == 0 && re.notice.isEmpty() && re.noticeKey.isEmpty(), "chosen again: block, count and notice are gone");
        DriverGuard.Decision retry = DriverGuard.decide(re, in("Z1", "h1"));
        check(retry.custom && retry.state.pending.equals("h1") && retry.state.fails == 0, "chosen again: the next start tries the package with a clean count");
        check(DriverGuard.chosenAgain(shown4, "Z9").equals(shown4), "choosing another package leaves the block alone");
        check(DriverGuard.chosenAgain(d.state, "Z1").equals(d.state), "choosing a package that is not blocked changes nothing");
        check(DriverGuard.chosenAgain(shown4, "").equals(shown4), "choosing the system driver changes nothing");





        DriverGuard.State blkZ1 = DriverGuard.fellBackNow(d8.state, "a8", "no Vulkan device");
        DriverGuard.State choseZ2 = DriverGuard.chosenAgain(blkZ1, "Z2");
        check(choseZ2 == blkZ1, "choosing another package leaves the old notice and block alone");
        DriverGuard.Decision run2 = DriverGuard.decide(choseZ2, in("Z2", "ok1"));
        check(run2.custom && run2.state.notice.equals(DriverGuard.LOAD_FAILED) && run2.state.blocked.isEmpty(), "the new driver starts, the old block lifts, the notice is kept until the new one works");
        DriverGuard.State okLive = DriverGuard.okReached(run2.state, "ok1");
        check(okLive.notice.isEmpty() && okLive.noticeKey.isEmpty() && okLive.pending.equals("ok1"), "OK frames: the notice goes, the attempt stays open for the count");
        check(DriverGuard.okReached(okLive, "ok1") == okLive, "okReached with no notice changes nothing");
        check(DriverGuard.okReached(run2.state, "other").equals(run2.state), "OK frames of another attempt leave the notice alone");
        DriverGuard.Input okSettle = in("Z2", "ok2"); okSettle.okId = "ok1";
        DriverGuard.Decision run3 = DriverGuard.decide(run2.state, okSettle);
        check(run3.custom && run3.state.notice.isEmpty() && run3.state.fails == 0, "the OK marker settles the attempt and takes the notice with it");
        DriverGuard.Decision backToSys = DriverGuard.decide(choseZ2, in(null, "z1"));
        check(!backToSys.state.notice.isEmpty(), "running the system driver again keeps the notice of the driver that failed");


        DriverGuard.Input pi = in("Z1", "p1"); pi.pid = 4242;
        DriverGuard.Decision p1 = DriverGuard.decide(new DriverGuard.State(), pi);
        check(p1.custom && p1.state.pendingPid == 4242, "the attempt records the pid of its process");

        DriverGuard.Input x1 = in("Z1", "p2"); x1.endedOutside = true; x1.pid = 5000;
        DriverGuard.Decision p2 = DriverGuard.decide(p1.state, x1);
        check(p2.custom && p2.state.fails == 0 && p2.state.pending.equals("p2") && p2.state.pendingPid == 5000, "a start ended from outside: not counted, own driver again");
        DriverGuard.Input x2 = in("Z1", "p3"); x2.endedOutside = true;
        DriverGuard.Decision p3 = DriverGuard.decide(p2.state, x2);
        check(p3.custom && p3.state.fails == 0 && p3.state.blocked.isEmpty() && p3.state.notice.isEmpty(), "two starts in a row ended from outside never block the driver");

        DriverGuard.Decision q1 = DriverGuard.decide(p3.state, in("Z1", "q1"));
        check(q1.custom && q1.state.fails == 1, "an unfinished start without an excuse counts");
        DriverGuard.Input qx = in("Z1", "q2"); qx.endedOutside = true;
        DriverGuard.Decision q2 = DriverGuard.decide(q1.state, qx);
        check(q2.custom && q2.state.fails == 1, "an excused start leaves the count where it was");
        DriverGuard.Decision q3 = DriverGuard.decide(q2.state, in("Z1", "q3"));
        check(!q3.custom && q3.state.blocked.equals("Z1") && q3.state.notice.equals(DriverGuard.FELL_BACK) && q3.state.pendingPid == 0, "the second unexcused start blocks as before");

        DriverGuard.Input fx = in("Z1", "r2"); fx.endedOutside = true; fx.fallbackId = "r1"; fx.fallbackReason = "vkCreateInstance failed";
        DriverGuard.State r1s = DriverGuard.decide(new DriverGuard.State(), in("Z1", "r1")).state;
        DriverGuard.Decision rf = DriverGuard.decide(r1s, fx);
        check(!rf.custom && rf.state.notice.equals(DriverGuard.LOAD_FAILED), "a fallback marker blocks even when the process ended cleanly");
        DriverGuard.Input ox = in("Z1", "r3"); ox.endedOutside = true; ox.okId = "r1";
        DriverGuard.State r1f = r1s.copy(); r1f.fails = 1;
        check(DriverGuard.decide(r1f, ox).state.fails == 0, "an OK marker still resets the count");

        check(DriverGuard.endedOutsideDriver(DriverGuard.EXIT_SELF, 0), "own exit with status 0 is outside the driver");
        check(!DriverGuard.endedOutsideDriver(DriverGuard.EXIT_SELF, 1), "own exit with another status counts");
        check(DriverGuard.endedOutsideDriver(DriverGuard.USER_REQUESTED, 0) && DriverGuard.endedOutsideDriver(DriverGuard.USER_STOPPED, 0), "force stop and user stop are outside the driver");
        check(DriverGuard.endedOutsideDriver(DriverGuard.LOW_MEMORY, 0) && DriverGuard.endedOutsideDriver(DriverGuard.FREEZER, 0), "background kills are outside the driver");
        check(DriverGuard.endedOutsideDriver(DriverGuard.PACKAGE_UPDATED, 0) && DriverGuard.endedOutsideDriver(DriverGuard.PACKAGE_STATE_CHANGE, 0)
            && DriverGuard.endedOutsideDriver(DriverGuard.PERMISSION_CHANGE, 0), "package and permission changes are outside the driver");
        for (int r : new int[]{0, 2, 4, 5, 6, 7, 9, 12, 13, 99}) check(!DriverGuard.endedOutsideDriver(r, 0), "exit reason " + r + " counts");

        check(DriverGuard.parse(DriverGuard.text(p2.state)).pendingPid == 5000, "the pid survives the text round trip");
        check(DriverGuard.parse("pending=a\npendingZip=Z1\nfails=1").pendingPid == 0, "a guard file without a pid reads as pid 0");
        check(DriverGuard.parse("pendingPid=-3").pendingPid == 0 && DriverGuard.parse("pendingPid=x").pendingPid == 0, "a bad pid reads as 0");


        DriverGuard.State round = DriverGuard.parse(DriverGuard.text(d9.state));
        check(round.equals(d9.state), "text round trip");
        check(DriverGuard.parse("fails=99\ngarbage\n=x\nnotice=fellback").fails == DriverGuard.FAILS_TO_BLOCK, "parse clamps the count and skips bad lines");
        check(DriverGuard.parse(null).empty(), "missing file = empty state");
        DriverGuard.State nl = new DriverGuard.State(); nl.noticeArg = "a\nb";
        check(!DriverGuard.text(nl).contains("a\nb"), "line breaks in values are flattened");

        if (fails > 0) { System.out.println(fails + " failure(s)"); System.exit(1); }
        System.out.println("DriverGuardTest: all passed");
    }
}
