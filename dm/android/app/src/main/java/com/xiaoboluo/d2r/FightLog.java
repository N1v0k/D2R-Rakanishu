package com.xiaoboluo.d2r;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.Set;






final class FightLog {
    static final int END_KILL = 1, END_DEATH = 2;
    static final int RANK_NORMAL = 0, RANK_MINION = 1, RANK_CHAMPION = 2, RANK_UNIQUE = 3, RANK_SUPER = 4;
    static final int EN_ATTACK = 1, EN_HIT = 2, EN_BLOCK = 4, EN_MISS = 8, EN_DODGE = 32;
    static final int ME_CRIT = 8, ME_MINION = 16;
    static final int SORT_NONE = -1, SORT_NAME = 0, SORT_KILL = 1, SORT_DPS = 2, SORT_LIFE = 3;
    static final double TPS = 25;

    static final class Group {
        int index, rank, joined, kills, first, done;
        String key;
        boolean boss, others, sim;
        long dealt, life, mine;

        double seconds() { return done > 0 ? Math.max(0, done - first) / TPS : -1; }


        double dps(int endTick) { return dps(endTick, dealt); }

        double own(int endTick, boolean known) { return dps(endTick, known ? mine : dealt); }
        private double dps(int endTick, long value) { double t = (done > 0 ? done - first : endTick - first) / TPS; return measurable(t) ? value / t : -1; }
        boolean elite() { return rank == RANK_SUPER || rank == RANK_UNIQUE || boss; }
    }
    static final class Entry {
        int index, key, end, duration, lead, area, difficulty, kills, enemies, minLife, ngroups;
        final long[] dealt = new long[3];
        boolean sim, known;
        long mine, companions;
        final List<Group> groups = new ArrayList<>();
        long total() { return dealt[0] + dealt[1] + dealt[2]; }
        double seconds() { return duration / TPS; }


        double dps() { return measurable(seconds()) ? total() / seconds() : -1; }

        double own() { return !known ? dps() : measurable(seconds()) ? mine / seconds() : -1; }

        double companionDps() { return known && companions > 0 && measurable(seconds()) ? companions / seconds() : -1; }
        int minLifePercent() { return Math.round(minLife * 100f / 255f); }
        boolean aborted() { return end != END_KILL && end != END_DEATH; }

        String partial() { return aborted() && kills > 0 ? kills + " / " + enemies : null; }

        int minionShare() { long t = total(); return t > 0 ? (int) Math.round(dealt[1] * 100.0 / t) : 0; }

        Group lead() {
            Group best = null;
            for (Group g : groups) if (best == null || weight(g) > weight(best) || (weight(g) == weight(best) && g.life > best.life)) best = g;
            return best;
        }
        int others() { Group l = lead(); return l == null ? 0 : Math.max(0, enemies - l.joined); }
        private static int weight(Group g) { return g.others ? -1 : g.elite() ? 3 : g.rank == RANK_CHAMPION ? 2 : g.rank == RANK_MINION ? 1 : 0; }
    }

    static boolean measurable(double seconds) { return CombatModel.displayedPositive(seconds); }

    static String time(double seconds, int lang) { return seconds >= 0 && !measurable(seconds) ? (lang == CombatModel.L_EN ? "<0.1 s" : "<0,1 s") : CombatModel.time(seconds, lang); }

    static final class Row { final Entry entry; final Group group; Row(Entry e, Group g) { entry = e; group = g; } }
    static final class State { int tick, id; boolean on; }
    static final class Detail {
        CombatModel.Fight fight;
        int[] enemy, own;
        final List<State> states = new ArrayList<>();
        float[][] groups;
        CombatModel.Fight sim;
        double simDps;
        int simOffset;
        int simGroup = -1;
    }

    int count;
    boolean recording;
    final List<Entry> entries = new ArrayList<>();

    static FightLog parse(String text) {
        FightLog log = new FightLog(); boolean head = false;
        for (String line : text.split("\n")) {
            if (line.isEmpty()) continue;
            String[] v = line.split("\t", -1);
            try {
                switch (v[0]) {
                case "log":
                    if (v.length != 3) throw new IllegalArgumentException();
                    log.count = CombatModel.i(v[1]); log.recording = CombatModel.i(v[2]) != 0; head = true; break;
                case "f": {
                    if (v.length != 19 || CombatModel.i(v[1]) != log.entries.size()) throw new IllegalArgumentException();
                    Entry e = new Entry();
                    e.index = CombatModel.i(v[1]); e.key = CombatModel.i(v[2]); e.end = CombatModel.i(v[3]); e.duration = CombatModel.i(v[4]); e.lead = CombatModel.i(v[5]);
                    e.area = CombatModel.i(v[6]); e.difficulty = CombatModel.i(v[7]); e.kills = CombatModel.i(v[8]); e.enemies = CombatModel.i(v[9]);
                    for (int k = 0; k < 3; k++) e.dealt[k] = Long.parseLong(v[10 + k]);
                    e.minLife = CombatModel.i(v[13]); e.ngroups = CombatModel.i(v[14]); e.sim = CombatModel.i(v[15]) != 0;
                    e.known = CombatModel.i(v[16]) != 0; e.mine = Long.parseLong(v[17]); e.companions = Long.parseLong(v[18]);
                    if (e.duration < 0 || e.lead < 0 || e.minLife < 0 || e.minLife > 255 || e.ngroups < 0 || e.ngroups > 8 || e.kills < 0 || e.kills > e.enemies) throw new IllegalArgumentException();
                    log.entries.add(e); break; }
                case "g": {
                    if (v.length != 15 || log.entries.isEmpty()) throw new IllegalArgumentException();
                    Entry e = log.entries.get(log.entries.size() - 1);
                    Group g = new Group();
                    g.index = CombatModel.i(v[2]); g.key = v[3]; g.rank = CombatModel.i(v[4]); g.boss = CombatModel.i(v[5]) != 0; g.joined = CombatModel.i(v[6]);
                    g.kills = CombatModel.i(v[7]); g.first = CombatModel.i(v[8]); g.done = CombatModel.i(v[9]); g.dealt = Long.parseLong(v[10]); g.life = Long.parseLong(v[11]);
                    g.others = CombatModel.i(v[12]) != 0; g.sim = CombatModel.i(v[13]) != 0; g.mine = Long.parseLong(v[14]);
                    if (CombatModel.i(v[1]) != e.index || g.index != e.groups.size() || g.index >= e.ngroups || g.rank < RANK_NORMAL || g.rank > RANK_SUPER) throw new IllegalArgumentException();
                    e.groups.add(g); break; }
                default: break;
                }
            } catch (RuntimeException x) { throw new IllegalArgumentException("Invalid fight log", x); }
        }
        if (!head || log.count != log.entries.size()) throw new IllegalArgumentException("Incomplete fight log");
        for (Entry e : log.entries) if (e.groups.size() != e.ngroups) throw new IllegalArgumentException("Incomplete fight log");
        return log;
    }


    interface Names { String get(String key); }

    List<Row> rows(int sort, boolean descending, Set<Integer> opened, Names names) {
        List<Entry> list = new ArrayList<>(entries);
        Comparator<Entry> order = null;
        if (sort == SORT_NAME) order = Comparator.comparing((Entry e) -> label(e, names).toLowerCase(java.util.Locale.ROOT));
        else if (sort == SORT_KILL) order = Comparator.comparingInt((Entry e) -> e.end == END_KILL ? 0 : e.end == END_DEATH ? 1 : 2).thenComparingInt(e -> e.duration);
        else if (sort == SORT_DPS) order = Comparator.comparingDouble((Entry e) -> -e.own());
        else if (sort == SORT_LIFE) order = Comparator.comparingInt((Entry e) -> e.minLife);
        if (order != null) list.sort(descending ? order.reversed().thenComparingInt(e -> e.index) : order.thenComparingInt(e -> e.index));
        List<Row> out = new ArrayList<>();
        for (Entry e : list) {
            out.add(new Row(e, null));
            if (e.groups.size() >= 2 && opened.contains(e.index)) for (Group g : e.groups) out.add(new Row(e, g));
        }
        return out;
    }

    static String label(Entry e, Names names) {
        Group g = e.lead();
        if (g == null) return names.get("-");
        int rest = e.others();
        return e.groups.size() == 1 ? label(g, names) : names.get(g.others ? "@others" : g.key) + " + " + rest;
    }

    static String label(Group g, Names names) { return names.get(g.others ? "@others" : g.key) + (g.joined > 1 ? " ×" + g.joined : ""); }

    static Detail detail(String text) {
        Detail d = new Detail(); CombatModel.Fight sim = null; int simFilled = 0;
        for (String line : text.split("\n")) {
            if (line.isEmpty()) continue;
            String[] v = line.split("\t", -1);
            try {
                switch (v[0]) {
                case "fight": case "act": case "trace": case "drink":
                    d.fight = CombatModel.fightLine(d.fight, v);
                    if (v[0].equals("fight")) { d.enemy = new int[d.fight.n]; d.own = new int[d.fight.n]; }
                    break;
                case "en": {
                    int k = CombatModel.i(v[1]);
                    if (v.length != 4 || d.fight == null || k < 0 || k >= d.fight.n) throw new IllegalArgumentException();
                    d.enemy[k] = CombatModel.i(v[2]); d.own[k] = CombatModel.i(v[3]); break; }
                case "state": {
                    if (v.length != 4) throw new IllegalArgumentException();
                    State s = new State(); s.tick = CombatModel.i(v[1]); s.on = CombatModel.i(v[2]) != 0; s.id = CombatModel.i(v[3]);
                    if (s.tick < 0 || s.id < 0) throw new IllegalArgumentException();
                    d.states.add(s); break; }
                case "glife": {
                    int g = CombatModel.i(v[1]); String[] p = v.length == 3 ? v[2].split(",") : new String[0];
                    if (d.fight == null || g < 0 || g >= 8 || p.length != d.fight.n) throw new IllegalArgumentException();
                    if (d.groups == null) d.groups = new float[8][];
                    d.groups[g] = new float[p.length];
                    for (int k = 0; k < p.length; k++) d.groups[g][k] = CombatModel.percent(p[k]);
                    break; }
                case "sim": {
                    if (v.length != 8 || sim != null) throw new IllegalArgumentException();
                    sim = new CombatModel.Fight(); sim.endKind = CombatModel.i(v[1]); sim.end = CombatModel.i(v[2]); sim.width = CombatModel.i(v[3]); sim.n = CombatModel.i(v[4]);
                    d.simDps = CombatModel.d(v[5]); d.simOffset = CombatModel.i(v[6]); d.simGroup = CombatModel.i(v[7]); sim.best = -1;
                    if (sim.n < 1 || sim.n > 256 || sim.width < 1 || sim.end < 0 || d.simOffset < 0 || d.simGroup < -1 || d.simGroup >= 8) throw new IllegalArgumentException();
                    sim.lo = new float[CombatModel.SERIES][sim.n]; sim.hi = new float[CombatModel.SERIES][sim.n]; sim.last = new float[CombatModel.SERIES][sim.n];
                    break; }
                case "strace": {
                    int k = CombatModel.i(v[1]);
                    if (v.length != 4 || sim == null || k != simFilled || k >= sim.n) throw new IllegalArgumentException();
                    for (int q = 0; q < 2; q++) {
                        String[] t = v[2 + q].split(",");
                        if (t.length != 3) throw new IllegalArgumentException();
                        sim.lo[q][k] = CombatModel.percent(t[0]); sim.hi[q][k] = CombatModel.percent(t[1]); sim.last[q][k] = CombatModel.percent(t[2]);
                    }
                    simFilled++; break; }
                default: break;
                }
            } catch (RuntimeException x) { throw new IllegalArgumentException("Invalid fight", x); }
        }
        if (d.fight == null || d.fight.filled != d.fight.n) throw new IllegalArgumentException("Incomplete fight");
        if (sim != null && simFilled == sim.n) d.sim = sim;
        return d;
    }


    static final class Band { int id, from, to; boolean began, ended; }




    static List<Band> bands(Detail d, java.util.function.IntPredicate named, int max) {
        List<Band> out = new ArrayList<>();
        int end = d.fight.end;
        java.util.Map<Integer, Band> open = new java.util.LinkedHashMap<>();
        java.util.Set<Integer> seen = new java.util.HashSet<>();
        for (State s : d.states) {
            if (!named.test(s.id) || s.tick > end) continue;
            if (s.on) {
                if (open.containsKey(s.id)) continue;
                Band b = new Band(); b.id = s.id; b.from = s.tick; b.began = true; open.put(s.id, b); seen.add(s.id);
            } else {
                Band b = open.remove(s.id);
                if (b == null) { if (seen.contains(s.id)) continue; b = new Band(); b.id = s.id; b.from = 0; }
                seen.add(s.id);
                b.to = s.tick; b.ended = true; out.add(b);
            }
        }
        for (Band b : open.values()) { b.to = end; out.add(b); }
        out.sort(Comparator.comparingInt((Band b) -> b.began ? b.from : b.to));
        return out.size() > max ? new ArrayList<>(out.subList(0, max)) : out;
    }

    static int[] counts(Detail d) {
        int[] c = new int[5];
        for (int k = 0; k < d.fight.n; k++) {
            if ((d.enemy[k] & EN_HIT) != 0) c[0]++;
            if ((d.enemy[k] & EN_MISS) != 0) c[1]++;
            if ((d.enemy[k] & EN_BLOCK) != 0) c[2]++;
            if ((d.own[k] & ME_CRIT) != 0) c[3]++;
            if ((d.enemy[k] & EN_DODGE) != 0) c[4]++;
        }
        return c;
    }
}
