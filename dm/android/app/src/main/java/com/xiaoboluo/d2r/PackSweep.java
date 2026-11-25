package com.xiaoboluo.d2r;

import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.List;






final class PackSweep {
    static final String LAUNCHER = "libpylauncher.so";
    static final long WAIT_MS = 5000, POLL_MS = 50;

    static final class Proc {
        final int pid, group;
        Proc(int pid, int group) { this.pid = pid; this.group = group; }
    }

    interface Table {
        List<Proc> launchers() throws IOException;
        int ownGroup() throws IOException;
        void kill(int pid);
        void killGroup(int group);
        void pause(long ms);
        long now();
    }

    private PackSweep() {}


    static boolean sweep(Table t, String why) {
        long end = t.now() + WAIT_MS;
        boolean logged = false;
        try {
            int own = t.ownGroup();
            while (true) {
                List<Proc> found = t.launchers();
                if (found.isEmpty()) {
                    if (logged) AppArt.log("pack build: sweep " + why + ": every step process is gone");
                    return true;
                }
                if (!logged) { AppArt.log("pack build: sweep " + why + ": " + found.size() + " step processes left, killing them"); logged = true; }
                if (t.now() >= end) {
                    AppArt.log("pack build: sweep " + why + ": " + found.size() + " step processes still run");
                    return false;
                }
                for (Proc p : found) {
                    if (p.group == p.pid && p.group != own) t.killGroup(p.group);
                    t.kill(p.pid);
                }
                t.pause(POLL_MS);
            }
        } catch (IOException e) {

            AppArt.log("ASSUME packbuild.sweep: the process table cannot be read (" + e.getClass().getSimpleName() + ")");
            return true;
        }
    }

    static String read(File f) throws IOException { return new String(Files.readAllBytes(f.toPath()), StandardCharsets.UTF_8); }


    static Proc launcher(int pid, String status, String cmdline, String stat, int uid) {
        if (owner(status) != uid) return null;
        int zero = cmdline.indexOf('\0');
        String argv0 = zero < 0 ? cmdline : cmdline.substring(0, zero);
        if (!argv0.equals(LAUNCHER) && !argv0.endsWith("/" + LAUNCHER)) return null;
        return new Proc(pid, group(stat));
    }


    static int owner(String status) {
        for (String line : status.split("\n")) if (line.startsWith("Uid:")) return Integer.parseInt(line.substring(4).trim().split("\\s+")[0]);
        throw new IllegalArgumentException("status without Uid");
    }


    static int group(String stat) {
        String[] f = stat.substring(stat.lastIndexOf(')') + 2).split(" ");
        return Integer.parseInt(f[2]);
    }
}
