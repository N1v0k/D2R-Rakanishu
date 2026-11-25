



package com.xiaoboluo.d2r;

public final class StatusModelTest {
    private static int checks, fails;
    private static void check(boolean ok, String what) { checks++; if (!ok) { fails++; System.out.println("FAIL " + what); } }


    private static final class Counter implements StatusModel.Receiver {
        int registers, unregisters; boolean fail;
        public boolean register() { registers++; return !fail; }
        public void unregister() { unregisters++; }
    }

    private static void receiver() {
        StatusModel m = new StatusModel(); Counter c = new Counter();

        m.reconcile(true, true, true, true, c); m.reconcile(true, true, true, true, c);
        check(c.registers == 1 && c.unregisters == 0 && m.registered(), "start/start registers once");

        m.reconcile(false, false, false, false, c); m.reconcile(false, false, false, false, c);
        check(c.registers == 1 && c.unregisters == 1 && !m.registered(), "stop/stop unregisters once");

        m.reconcile(true, true, true, true, c);
        check(c.registers == 2 && m.registered(), "stop/start registers again");

        m.reconcile(true, false, true, true, c);
        check(c.unregisters == 2 && !m.registered(), "switch off unregisters");
        m.reconcile(true, true, true, true, c);
        check(c.registers == 3 && m.registered(), "switch on registers");

        m.reconcile(true, true, false, false, c);
        check(c.unregisters == 3 && !m.registered(), "fps only needs no receiver");

        m.reconcile(true, true, true, false, c);
        check(c.registers == 4 && m.registered(), "clock only registers");
        m.reconcile(true, true, false, true, c);
        check(c.registers == 4 && c.unregisters == 3 && m.registered(), "switching clock to battery keeps the registration");

        for (int i = 0; i < 10; i++) m.reconcile(true, true, false, true, c);
        check(c.registers == 4 && c.unregisters == 3, "hidden by a panel keeps the registration");
        m.reconcile(false, true, false, true, c);
        check(c.unregisters == 4, "stop at the end unregisters");

        StatusModel f = new StatusModel(); Counter k = new Counter(); k.fail = true;
        for (int i = 0; i < 5; i++) f.reconcile(true, true, true, true, k);
        check(k.registers == 1 && k.unregisters == 0 && !f.registered(), "failed registration is not retried every frame");
        f.reconcile(false, true, true, true, k);
        check(k.unregisters == 0, "a failed registration is never unregistered");
        k.fail = false; f.reconcile(true, true, true, true, k);
        check(k.registers == 2 && f.registered(), "retried after stop / start");
    }

    private static void values() {
        StatusModel m = new StatusModel();
        check(m.percent == -1 && !m.plugged && !m.low, "battery unknown at first");
        check(m.battery(78, 100, 0, false) && m.percent == 78 && !m.plugged && !m.low, "78 %");
        check(!m.battery(78, 100, 0, false), "same reading: no change");

        check(m.batteryKind() == StatusModel.BATTERY_PLAIN, "78 % not on power: the plain sentence");
        check(m.battery(78, 100, 2, false) && m.plugged, "plugged with the same percentage is a change");
        check(m.batteryKind() == StatusModel.BATTERY_CHARGING, "plugged: the charging sentence, text instead of a bolt");
        check(!m.battery(78, 100, 2, false), "still plugged: no change");

        check(m.battery(78, 100, 2, true) && m.low && m.plugged, "low with the same percentage is a change");
        check(m.battery(78, 100, 0, false) && !m.low && !m.plugged, "unplugged and not low again");

        check(m.battery(150, 200, 0, false) && m.percent == 75, "level / scale 150 / 200 = 75");
        check(m.battery(101, 100, 0, false) && m.percent == 100, "capped at 100");

        check(m.battery(-1, 100, 1, true) && m.percent == -1 && !m.plugged && !m.low, "level -1 is unknown");
        check(m.batteryKind() == StatusModel.BATTERY_NONE, "unknown: no battery sentence, also when plugged");
        check(!m.battery(50, 0, 1, true) && m.percent == -1 && !m.plugged && !m.low, "scale 0 is unknown (no change from unknown)");

        check(m.clock(7, 5) && m.clock.equals("07:05"), "07:05");
        check(!m.clock(7, 5), "same minute: no change");
        check(m.clock(23, 59) && m.clock.equals("23:59"), "23:59");
        check(m.clock(0, 0) && m.clock.equals("00:00"), "00:00");
        check(m.clock(13, 10) && m.clock.equals("13:10"), "13:10 (never 1:10 PM)");

        check(!m.fps(-1) && m.fps == -1, "unknown at first");
        check(m.fps(60) && m.fps == 60, "60");
        check(!m.fps(60), "same: no change");
        check(m.fps(-7) && m.fps == -1, "negative is unknown");
    }

    public static void main(String[] args) {
        receiver(); values();
        if (fails > 0) { System.out.println("StatusModelTest: " + fails + " of " + checks + " failed"); System.exit(1); }
        System.out.println("StatusModelTest OK (" + checks + " checks)");
    }
}
