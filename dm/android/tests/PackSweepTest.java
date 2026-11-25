package com.xiaoboluo.d2r;

import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;




public class PackSweepTest {
    private static int checks, fails;
    private static void check(boolean ok, String what) { checks++; if (!ok) { fails++; System.err.println("FAIL " + what); } }


    static class Table implements PackSweep.Table {
        final Map<Integer, Integer> procs = new HashMap<>();
        final List<Integer> stubborn = new ArrayList<>();
        final List<String> signals = new ArrayList<>();
        final int own;
        long clock;
        Table(int own) { this.own = own; }
        @Override public List<PackSweep.Proc> launchers() throws IOException {
            List<PackSweep.Proc> out = new ArrayList<>();
            for (Map.Entry<Integer, Integer> e : procs.entrySet()) out.add(new PackSweep.Proc(e.getKey(), e.getValue()));
            return out;
        }
        @Override public int ownGroup() { return own; }
        @Override public void kill(int pid) { signals.add("kill " + pid); if (!stubborn.contains(pid)) procs.remove(pid); }
        @Override public void killGroup(int group) {
            signals.add("group " + group);
            if (group == own) throw new IllegalStateException("the builder's own group was signalled");
            procs.entrySet().removeIf(e -> e.getValue() == group && !stubborn.contains(e.getKey()));
        }
        @Override public void pause(long ms) { clock += ms; }
        @Override public long now() { return clock; }
    }

    public static void main(String[] args) throws Exception {
        Table none = new Table(100);
        check(PackSweep.sweep(none, "t") && none.signals.isEmpty(), "nothing found: nothing signalled");


        Table t = new Table(100);
        t.procs.put(200, 200); t.procs.put(300, 300); t.procs.put(400, 400); t.procs.put(401, 400);
        check(PackSweep.sweep(t, "t") && t.procs.isEmpty(), "every launcher process is gone: " + t.procs);
        check(t.signals.contains("group 200") && t.signals.contains("group 300") && t.signals.contains("group 400") && t.signals.contains("kill 401"),
            "leaders with their groups, the others by pid: " + t.signals);


        Table s = new Table(100);
        s.procs.put(150, 100); s.procs.put(300, 300);
        boolean swept;
        try { swept = PackSweep.sweep(s, "t"); } catch (IllegalStateException e) { swept = false; }
        check(swept && s.procs.isEmpty() && !s.signals.contains("group 100") && s.signals.contains("kill 150"), "the runner in the builder's group dies by its pid alone: " + s.signals);


        Table h = new Table(100);
        h.procs.put(500, 500); h.stubborn.add(500);
        check(!PackSweep.sweep(h, "t") && h.clock >= PackSweep.WAIT_MS, "a process that stays makes the sweep fail after " + h.clock + " ms");


        PackSweep.Table broken = new Table(100) { @Override public List<PackSweep.Proc> launchers() throws IOException { throw new IOException("no /proc"); } };
        check(PackSweep.sweep(broken, "t"), "an unreadable process table does not block the build");


        String status = "Name:\tlibpylauncher.s\nUid:\t10234\t10234\t10234\t10234\nGid:\t10234\n";
        check(PackSweep.owner(status) == 10234, "the real user id");
        check(PackSweep.group("4242 (a (b) c) S 1 4200 4200 0 -1") == 4200, "the group after a command name with spaces and parentheses");
        String lib = "/data/app/~~x/io.r4k4n1shu.d2r-y/lib/arm64/libpylauncher.so";
        PackSweep.Proc p = PackSweep.launcher(4242, status, lib + "\0-X\0utf8\0pack_steps.py\0", "4242 (libpylauncher.s) S 1 4242 4242", 10234);
        check(p != null && p.pid == 4242 && p.group == 4242, "a launcher process of the app's user");
        check(PackSweep.launcher(4242, status, "/system/bin/sh\0-c\0", "4242 (sh) S 1 4242 4242", 10234) == null, "another program is left alone");
        check(PackSweep.launcher(4242, status, lib + "\0", "4242 (x) S 1 4242 4242", 10235) == null, "another user's process is left alone");
        check(PackSweep.launcher(4242, status, "/x/libpylauncher.so.bak\0", "4242 (x) S 1 4242 4242", 10234) == null, "only the launcher itself");

        System.out.println("PackSweepTest: " + checks + " checks, " + fails + " failed");
        if (fails > 0) System.exit(1);
    }
}
