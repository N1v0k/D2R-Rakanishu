package com.xiaoboluo.d2r;

import java.util.List;



final class StreakLogTest {
    private static int checks;
    private static void check(boolean value, String what) { checks++; if (!value) throw new AssertionError(what); }

    static final String LIST = "streaks\t3\n"
        + "best\t0\t47\t1030\t2187\t41\t1\n"
        + "s\t0\t12\t300\t0\t2\t0\t1\n"
        + "s\t1\t47\t1030\t2187\t41\t0\t1\n"
        + "s\t2\t19\t500\t0\t41\t0\t2\n";

    private static void rejected(String text, String what) {
        try { StreakLog.parse(text); } catch (IllegalArgumentException x) { checks++; return; }
        throw new AssertionError("accepted: " + what);
    }

    public static void main(String[] args) {
        StreakLog log = StreakLog.parse(LIST);
        StreakLog.Names names = id -> id == 41 ? "Area F" : id == 2 ? "Area B" : "?";
        check(log.count == 3 && log.streaks.size() == 3 && log.any(), "three streaks");
        check(log.best[0] != null && log.best[0].kills == 47 && log.best[0].area == 41 && log.best[1] == null && log.best[2] == null, "one record");
        StreakLog.Streak newest = log.streaks.get(0), death = log.streaks.get(2);
        check(newest.kills == 12 && Math.abs(newest.seconds() - 12.0) < 1e-9 && death.death() && !newest.death(), "fields");
        check(log.bonusColumn() && !log.mixed(), "bonus column, one difficulty");

        check(log.record(log.streaks.get(1)) && !log.record(newest) && !log.record(death) && log.dropped(0) == null && log.dropped(1) == null, "kept record");

        StreakLog twice = StreakLog.parse("streaks\t2\nbest\t0\t30\t250\t0\t2\t1\ns\t0\t30\t250\t0\t2\t0\t1\ns\t1\t30\t250\t0\t2\t0\t1\n");
        check(twice.record(twice.streaks.get(1)) && !twice.record(twice.streaks.get(0)), "the older of two equal streaks is the record");
        StreakLog gone = StreakLog.parse("streaks\t1\nbest\t0\t30\t250\t0\t2\t1\ns\t0\t12\t250\t0\t2\t0\t1\n");
        check(gone.dropped(0) == gone.best[0] && !gone.record(gone.streaks.get(0)), "dropped record");
        StreakLog other = StreakLog.parse("streaks\t1\nbest\t1\t12\t250\t0\t2\t1\ns\t0\t12\t250\t0\t2\t0\t1\n");
        check(!other.record(other.streaks.get(0)) && other.dropped(1) != null, "a record of another difficulty is not this streak");

        List<StreakLog.Streak> rows = log.rows(StreakLog.SORT_NONE, false, names);
        check(rows.get(0) == newest && rows.get(2) == death, "newest first");
        rows = log.rows(StreakLog.SORT_KILLS, false, names);
        check(rows.get(0).kills == 47 && rows.get(1).kills == 19 && rows.get(2).kills == 12, "most kills first");
        rows = log.rows(StreakLog.SORT_KILLS, true, names);
        check(rows.get(0).kills == 12, "fewest kills first");
        rows = log.rows(StreakLog.SORT_BONUS, false, names);
        check(rows.get(0).bonus == 2187 && rows.get(2) == death, "largest bonus first, deaths last");
        rows = log.rows(StreakLog.SORT_TIME, false, names);
        check(rows.get(0).ticks == 1030 && rows.get(2).ticks == 300, "longest first");
        rows = log.rows(StreakLog.SORT_AREA, false, names);
        check(rows.get(0).area == 2 && rows.get(1).area == 41 && rows.get(1).index == 1, "area by name, ties newest first");

        StreakLog plain = StreakLog.parse("streaks\t1\nbest\t1\t10\t250\t0\t2\t1\ns\t0\t10\t250\t0\t2\t1\t1\n");
        check(!plain.bonusColumn() && !plain.mixed() && plain.best[1].kills == 10, "plain");
        StreakLog mixed = StreakLog.parse("streaks\t1\nbest\t0\t30\t250\t0\t2\t1\nbest\t1\t10\t250\t0\t2\t1\ns\t0\t10\t250\t0\t2\t1\t1\n");
        check(mixed.mixed(), "a record of another difficulty");
        StreakLog empty = StreakLog.parse("streaks\t0\n");
        check(!empty.any() && empty.streaks.isEmpty(), "empty");
        check(empty.tier(500) == 0, "no tier lines: no tier");

        StringBuilder tiers = new StringBuilder("streaks\t1\ns\t0\t6\t300\t0\t2\t0\t1\n");
        int[] from = {8, 12, 20, 30, 45, 65, 100, 150, 250}, pct = {10, 15, 20, 25, 30, 35, 40, 50, 60};
        for (int t = 0; t < 9; t++) tiers.append("tier\t").append(t + 1).append('\t').append(from[t]).append('\t').append(pct[t]).append('\n');
        StreakLog tl = StreakLog.parse(tiers.toString());
        check(tl.streaks.get(0).kills == 6 && tl.tier(6) == 0, "a stored streak of 6 parses with tier 0");
        check(tl.tier(7) == 0 && tl.tier(8) == 1 && tl.tier(11) == 1 && tl.tier(12) == 2 && tl.tier(249) == 8 && tl.tier(250) == 9 && tl.tier(100000) == 9, "tier by kills");
        rejected("streaks\t0\ntier\t0\t6\t10\n", "tier 0");
        rejected("streaks\t0\ntier\t1\t6\t10\tx\n", "a tier line with a word cell");
        rejected("streaks\t0\ntier\t1\t6\t10\ntier\t1\t12\t15\n", "a tier twice");
        rejected("streaks\t0\ntier\t1\t12\t10\ntier\t2\t12\t15\n", "tiers not rising");
        rejected("streaks\t0\ntier\t1\t5\t10\n", "a tier below the kept minimum");

        rejected("", "no head");
        rejected("streaks\t2\ns\t0\t12\t300\t0\t2\t0\t1\n", "count");
        rejected("streaks\t21\n", "past the ring");
        rejected("streaks\t1\ns\t0\t5\t300\t0\t2\t0\t1\n", "below the kept minimum");
        check(StreakLog.parse("streaks\t1\ns\t0\t6\t300\t0\t2\t0\t1\n").streaks.get(0).kills == 6, "a streak of 6");
        rejected("streaks\t1\ns\t0\t12\t300\t0\t2\t3\t1\n", "difficulty");
        rejected("streaks\t1\ns\t0\t12\t300\t0\t2\t0\t0\n", "end");
        rejected("streaks\t1\ns\t1\t12\t300\t0\t2\t0\t1\n", "index");
        rejected("streaks\t0\nbest\t0\t12\t300\t0\t2\t1\nbest\t0\t13\t300\t0\t2\t1\n", "two records of one difficulty");
        rejected("streaks\t0\nbest\t0\tx\t300\t0\t2\t1\n", "not a number");
        System.out.println("StreakLogTest: " + checks + " checks passed");
    }
}
