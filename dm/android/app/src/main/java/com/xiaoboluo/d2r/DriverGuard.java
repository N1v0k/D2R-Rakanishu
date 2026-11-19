package com.xiaoboluo.d2r;

import java.util.LinkedHashMap;
import java.util.Map;



















final class DriverGuard {
    static final int FAILS_TO_BLOCK = 2;

    static final String NONE = "", FELL_BACK = "fellback", LOAD_FAILED = "loadfailed", REJECTED = "rejected", SAFE = "safe";

    static final class State {
        String pending = "", pendingZip = "", blocked = "", notice = NONE, noticeArg = "", noticeKey = "";
        int fails;

        State copy() { State s = new State(); s.pending = pending; s.pendingZip = pendingZip; s.blocked = blocked; s.notice = notice; s.noticeArg = noticeArg; s.noticeKey = noticeKey; s.fails = fails; return s; }
        boolean empty() { return pending.isEmpty() && pendingZip.isEmpty() && blocked.isEmpty() && notice.isEmpty() && noticeKey.isEmpty() && fails == 0; }
        @Override public boolean equals(Object o) { return o instanceof State && text((State) o).equals(text(this)); }
        @Override public int hashCode() { return text(this).hashCode(); }
    }




    static final class Input {
        String zip, problem, okId = "", fallbackId = "", fallbackReason = "", attemptId = "";
        boolean safe;
    }

    static final class Decision {
        final State state; final boolean custom;
        Decision(State state, boolean custom) { this.state = state; this.custom = custom; }
    }

    private DriverGuard() {}


    static State settle(State in, String okId, String fallbackId, String fallbackReason) {
        State s = in.copy();
        if (s.pending.isEmpty()) return s;
        if (s.pending.equals(fallbackId)) { block(s, LOAD_FAILED, fallbackReason == null ? "" : fallbackReason); }
        else if (s.pending.equals(okId)) { s.fails = 0; clearNotice(s); s.noticeKey = ""; }
        else if (++s.fails >= FAILS_TO_BLOCK) block(s, FELL_BACK, "");
        s.pending = "";
        return s;
    }










    static State okReached(State in, String attemptId) {
        if (in.pending.isEmpty() || !in.pending.equals(attemptId) || (in.notice.isEmpty() && in.noticeKey.isEmpty())) return in;
        State s = in.copy();
        clearNotice(s); s.noticeKey = "";
        return s;
    }

    private static void block(State s, String notice, String arg) {
        s.blocked = s.pendingZip; s.fails = 0;
        s.notice = notice; s.noticeArg = arg; s.noticeKey = notice + ":" + s.pendingZip;
    }

    static Decision decide(State previous, Input in) {
        State s = settle(previous, in.okId, in.fallbackId, in.fallbackReason);
        if (in.zip == null && in.problem == null) {
            s.blocked = ""; s.fails = 0; s.pendingZip = "";
            if (s.notice.equals(REJECTED) || s.notice.equals(SAFE)) clearNotice(s);
            s.noticeKey = s.notice.isEmpty() ? "" : s.noticeKey;
            return new Decision(s, false);
        }
        if (in.problem != null) {
            String key = REJECTED + ":" + (in.zip == null ? "" : in.zip) + ":" + in.problem;
            if (!key.equals(s.noticeKey)) { s.notice = REJECTED; s.noticeArg = in.problem; s.noticeKey = key; }
            return new Decision(s, false);
        }
        if (in.zip.equals(s.blocked)) return new Decision(s, false);
        if (in.safe) {
            if (s.notice.isEmpty() || s.notice.equals(SAFE)) { s.notice = SAFE; s.noticeArg = ""; s.noticeKey = SAFE + ":" + in.zip; }
            return new Decision(s, false);
        }
        if (!in.zip.equals(s.pendingZip)) s.fails = 0;
        s.blocked = "";
        s.pending = in.attemptId; s.pendingZip = in.zip;
        return new Decision(s, true);
    }



    static State fellBackNow(State in, String attemptId, String reason) {
        if (in.pending.isEmpty() || !in.pending.equals(attemptId)) return in;
        State s = in.copy();
        block(s, LOAD_FAILED, reason == null ? "" : reason);
        s.pending = "";
        return s;
    }



    static State cancel(State in, String attemptId) {
        if (in.pending.isEmpty() || !in.pending.equals(attemptId)) return in;
        State s = in.copy(); s.pending = ""; return s;
    }

    static State noticeShown(State in) { State s = in.copy(); clearNotice(s); return s; }
    private static void clearNotice(State s) { s.notice = NONE; s.noticeArg = ""; }





    static State chosenAgain(State in, String zip) {
        if (zip == null || zip.isEmpty() || !in.blocked.equals(zip)) return in;
        State s = in.copy();
        s.blocked = ""; s.fails = 0;
        if (s.noticeKey.contains(zip)) { clearNotice(s); s.noticeKey = ""; }
        return s;
    }




    static State forgotten(State in, String zip) {
        if (zip == null || zip.isEmpty() || (!in.blocked.equals(zip) && !in.pendingZip.equals(zip) && !in.noticeKey.contains(zip))) return in;
        State s = in.copy();
        if (s.blocked.equals(zip)) s.blocked = "";
        if (s.pendingZip.equals(zip)) { s.pending = ""; s.pendingZip = ""; s.fails = 0; }
        if (s.noticeKey.contains(zip)) { clearNotice(s); s.noticeKey = ""; }
        return s;
    }


    static String text(State s) {
        Map<String, String> m = new LinkedHashMap<>();
        m.put("pending", s.pending); m.put("pendingZip", s.pendingZip); m.put("fails", Integer.toString(s.fails)); m.put("blocked", s.blocked);
        m.put("notice", s.notice); m.put("noticeArg", s.noticeArg); m.put("noticeKey", s.noticeKey);
        StringBuilder b = new StringBuilder();
        for (Map.Entry<String, String> e : m.entrySet()) b.append(e.getKey()).append('=').append(e.getValue().replace('\n', ' ').replace('\r', ' ')).append('\n');
        return b.toString();
    }

    static State parse(String text) {
        State s = new State();
        if (text == null) return s;
        for (String line : text.split("\n")) {
            int eq = line.indexOf('=');
            if (eq <= 0) continue;
            String k = line.substring(0, eq), v = line.substring(eq + 1).trim();
            switch (k) {
                case "pending" -> s.pending = v;
                case "pendingZip" -> s.pendingZip = v;
                case "fails" -> { try { s.fails = Math.max(0, Math.min(FAILS_TO_BLOCK, Integer.parseInt(v))); } catch (NumberFormatException e) { s.fails = 0; } }
                case "blocked" -> s.blocked = v;
                case "notice" -> s.notice = v;
                case "noticeArg" -> s.noticeArg = v;
                case "noticeKey" -> s.noticeKey = v;
                default -> { }
            }
        }
        return s;
    }
}
