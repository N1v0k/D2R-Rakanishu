package com.xiaoboluo.d2r;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;






final class StreakLog {
    static final int END_TIME = 1, END_DEATH = 2, DIFFICULTIES = 3, RING = 20, TIERS = 9;
    static final int KEPT_MIN = 6;
    static final int SORT_NONE = -1, SORT_KILLS = 0, SORT_BONUS = 1, SORT_TIME = 2, SORT_AREA = 3;

    static final class Streak {
        int index, kills, ticks, bonus, area, difficulty, end;
        double seconds() { return ticks / 25.0; }
        boolean death() { return end == END_DEATH; }
    }

    int count;
    final Streak[] best = new Streak[DIFFICULTIES];
    final List<Streak> streaks = new ArrayList<>();
    private final Streak[] recordRow = new Streak[DIFFICULTIES];
    private final int[] tierFrom = new int[TIERS];


    int tier(int kills) { int t = 0; for (int i = 0; i < TIERS; i++) if (tierFrom[i] > 0 && kills >= tierFrom[i]) t = i + 1; return t; }


    boolean record(Streak s) { return recordRow[s.difficulty] == s; }

    Streak dropped(int difficulty) { return recordRow[difficulty] == null ? best[difficulty] : null; }

    static StreakLog parse(String text) {
        StreakLog log = new StreakLog(); boolean head = false;
        for (String line : text.split("\n")) {
            if (line.isEmpty()) continue;
            String[] v = line.split("\t", -1);
            try {
                switch (v[0]) {
                case "streaks":
                    if (v.length != 2 || head) throw new IllegalArgumentException();
                    log.count = CombatModel.i(v[1]); head = true;
                    if (log.count < 0 || log.count > RING) throw new IllegalArgumentException();
                    break;
                case "best": {
                    if (v.length != 7) throw new IllegalArgumentException();
                    Streak s = new Streak(); s.difficulty = CombatModel.i(v[1]); s.kills = CombatModel.i(v[2]); s.ticks = CombatModel.i(v[3]); s.bonus = CombatModel.i(v[4]);
                    s.area = CombatModel.i(v[5]); s.end = CombatModel.i(v[6]); s.index = -1;
                    if (s.difficulty < 0 || s.difficulty >= DIFFICULTIES || log.best[s.difficulty] != null) throw new IllegalArgumentException();
                    valid(s); log.best[s.difficulty] = s; break; }
                case "s": {
                    if (v.length != 8 || CombatModel.i(v[1]) != log.streaks.size()) throw new IllegalArgumentException();
                    Streak s = new Streak(); s.index = CombatModel.i(v[1]); s.kills = CombatModel.i(v[2]); s.ticks = CombatModel.i(v[3]); s.bonus = CombatModel.i(v[4]);
                    s.area = CombatModel.i(v[5]); s.difficulty = CombatModel.i(v[6]); s.end = CombatModel.i(v[7]);
                    if (s.difficulty < 0 || s.difficulty >= DIFFICULTIES) throw new IllegalArgumentException();
                    valid(s); log.streaks.add(s); break; }
                case "tier": {
                    if (v.length != 4) throw new IllegalArgumentException();
                    int t = CombatModel.i(v[1]), from = CombatModel.i(v[2]);
                    if (t < 1 || t > TIERS || log.tierFrom[t - 1] != 0 || from < KEPT_MIN || (t > 1 && log.tierFrom[t - 2] >= from)) throw new IllegalArgumentException();
                    log.tierFrom[t - 1] = from; break; }
                default: break;
                }
            } catch (RuntimeException x) { throw new IllegalArgumentException("Invalid streak list", x); }
        }
        if (!head || log.count != log.streaks.size()) throw new IllegalArgumentException("Incomplete streak list");

        for (Streak b : log.best) if (b != null) for (Streak s : log.streaks) if (same(s, b)) log.recordRow[b.difficulty] = s;
        return log;
    }
    private static boolean same(Streak s, Streak b) {
        return s.difficulty == b.difficulty && s.kills == b.kills && s.ticks == b.ticks && s.bonus == b.bonus && s.area == b.area && s.end == b.end;
    }
    private static void valid(Streak s) {
        if (s.kills < KEPT_MIN || s.ticks < 0 || s.bonus < 0 || s.area < 0 || (s.end != END_TIME && s.end != END_DEATH)) throw new IllegalArgumentException();
    }


    boolean any() { if (!streaks.isEmpty()) return true; for (Streak b : best) if (b != null) return true; return false; }

    boolean bonusColumn() { for (Streak s : streaks) if (s.bonus > 0 || s.death()) return true; return false; }

    boolean mixed() {
        int seen = -1;
        for (Streak s : streaks) { if (seen >= 0 && s.difficulty != seen) return true; seen = s.difficulty; }
        for (Streak b : best) if (b != null) { if (seen >= 0 && b.difficulty != seen) return true; seen = b.difficulty; }
        return false;
    }


    interface Names { String area(int id); }

    List<Streak> rows(int sort, boolean descending, Names names) {
        List<Streak> list = new ArrayList<>(streaks);
        Comparator<Streak> order = null;
        if (sort == SORT_KILLS) order = Comparator.comparingInt((Streak s) -> -s.kills);
        else if (sort == SORT_BONUS) order = Comparator.comparingInt((Streak s) -> s.death() ? 1 : 0).thenComparingInt(s -> -s.bonus);
        else if (sort == SORT_TIME) order = Comparator.comparingInt((Streak s) -> -s.ticks);
        else if (sort == SORT_AREA) order = Comparator.comparing((Streak s) -> names.area(s.area).toLowerCase(java.util.Locale.ROOT));
        if (order != null) list.sort(descending ? order.reversed().thenComparingInt(s -> s.index) : order.thenComparingInt(s -> s.index));
        return list;
    }
}
