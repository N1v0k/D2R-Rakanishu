package com.xiaoboluo.d2r;



final class BestGearEquip {

    static final int EQ_NO_ROOM = 0, EQ_OK = 1, EQ_NONE = -1, EQ_PENDING = 2;

    static final int BUTTON_NONE = 0, BUTTON_EQUIP = 1, BUTTON_TAKE_OFF = 2, BUTTON_NO_ROOM = 3, BUTTON_NO_ROOM_OFF = 4;


    static final int R_NONE = 0, R_DONE_ONE = 1, R_DONE_N = 2, R_BACK = 3, R_PARTIAL = 4, R_IN_HAND = 5, R_LOST = 6, R_CHANGED = 7, R_BUSY = 8, R_AWAY = 9,
        R_UNSURE = 10;

    static final int DONE_ON = 0, DONE_OFF = 1, DONE_MIXED = 2;

    static final String[] WHY = { "changed", "code", "late", "panel", "page", "save", "hand", "run", "fault", "time" };

    private BestGearEquip() {}



    static final class Job {
        final int seq, k, n, off; final String st, why;
        Job(int seq, String st, int k, int n, int off, String why) { this.seq = seq; this.st = st; this.k = k; this.n = n; this.off = off; this.why = why; }
    }


    static int button(int stash, int eq, boolean empties) {
        if (stash != 1) return BUTTON_NONE;
        if (eq == EQ_OK) return empties ? BUTTON_TAKE_OFF : BUTTON_EQUIP;
        return eq == EQ_NO_ROOM ? (empties ? BUTTON_NO_ROOM_OFF : BUTTON_NO_ROOM) : BUTTON_NONE;
    }

    static boolean all(int stash, boolean eqall) { return stash == 1 && eqall; }

    static boolean hint(int stash, int[] eq) {
        if (stash != 0 || eq == null) return false;
        for (int v : eq) if (v != EQ_NONE) return true;
        return false;
    }

    static boolean interim(boolean done, int[] buttons, boolean all) {
        if (done) return false;
        if (all) return true;
        for (int b : buttons == null ? new int[0] : buttons) if (b == BUTTON_EQUIP || b == BUTTON_TAKE_OFF) return true;
        return false;
    }

    static int done(int n, int off) { return off <= 0 ? DONE_ON : off >= n ? DONE_OFF : DONE_MIXED; }

    static boolean running(String st) { return "queued".equals(st) || "run".equals(st); }


    static int result(String st, int k, int n) {
        switch (st == null ? "" : st) {
        case "done": return n > 1 ? R_DONE_N : R_DONE_ONE;
        case "returned": return R_BACK;
        case "partial": return R_PARTIAL;
        case "in_hand": return R_IN_HAND;
        case "lost": return R_LOST;
        case "changed": return R_CHANGED;
        case "busy": return R_BUSY;
        case "away": return R_AWAY;
        default: return R_NONE;
        }
    }

    static int refused(int rc) { return rc == -1 ? R_CHANGED : rc == -2 ? R_AWAY : rc == -3 ? R_BUSY : R_NONE; }

    static int why(String name) {
        for (int i = 0; i < WHY.length; i++) if (WHY[i].equals(name)) return i;
        return WHY.length;
    }

    static String overlay(int stash, boolean busy, Job job, int result, boolean reading) {
        if (busy && job != null) return "run_" + job.k + "_" + job.n;
        if (busy) return "run";
        switch (result) {
        case R_DONE_ONE: case R_DONE_N: return "done";
        case R_BACK: return "returned";
        case R_PARTIAL: return job == null ? "partial" : "partial_" + job.k + "_" + job.n;
        case R_IN_HAND: return "in_hand";
        case R_LOST: return "lost";
        case R_CHANGED: return "changed";
        case R_BUSY: return "busy";
        case R_AWAY: return "away";
        case R_UNSURE: return "unsure";
        default: return reading ? "wait" : stash == 1 ? "ready" : stash == 0 ? "away" : "none";
        }
    }
}
