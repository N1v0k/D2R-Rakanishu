





















package com.xiaoboluo.d2r;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

final class BuildPlan {
    private BuildPlan() {}

    static final int OK = 0, R_LEVEL = 2, R_PREREQ = 3, R_CAP = 5, R_MAX = 6, R_DATA = 7;
    static final int NEXT = 0, UPCOMING = 1, SAVE = 2, WAIT = 3, BLOCKED = 4, DONE = 5;
    static final String[] STATE_NAMES = {"NEXT", "UPCOMING", "SAVE", "WAIT", "BLOCKED", "DONE"};
    static final int SKILL_MAX = 20;
    static final int MAX_LEVEL = 99;

    interface Reason { int of(int skill); }

    interface Future { Reason at(int level, Map<Integer, Integer> base); }

    static final class Result {
        final int state, index;
        final int at;
        Result(int state, int index) { this(state, index, 0); }
        Result(int state, int index, int at) { this.state = state; this.index = index; this.at = at; }
        @Override public String toString() { return STATE_NAMES[state] + " " + index + (at > 0 ? " " + at : ""); }
    }

    static boolean permanent(int r) { return r == R_CAP || r == R_MAX || r == R_DATA; }

    static Result next(List<BuildModel.Step> steps, int level, Map<Integer, Integer> base, Reason reason, int points) {
        return next(steps, level, base, reason, points, null);
    }


    static Result next(List<BuildModel.Step> steps, int level, Map<Integer, Integer> base, Reason reason, int points, Future future) {
        int firstOpen = -1;
        for (int i = 0; i < steps.size(); i++) {
            BuildModel.Step s = steps.get(i);
            if (get(base, s.skill) >= s.to) continue;
            if (firstOpen < 0) firstOpen = i;
            if (s.lvl > level) return hold(steps, i, firstOpen, level, base, reason, points, future);
            int r = reason.of(s.skill);
            if (r == OK) return new Result(points > 0 ? NEXT : UPCOMING, i);
            if (permanent(r)) return new Result(BLOCKED, i);
            if (s.save) return hold(steps, i, firstOpen, level, base, reason, points, future);
        }
        return firstOpen < 0 ? new Result(DONE, -1) : hold(steps, firstOpen, firstOpen, level, base, reason, points, future);
    }


    static int plain(List<BuildModel.Step> steps, int level, Map<Integer, Integer> base, Reason reason) {
        for (int i = 0; i < steps.size(); i++) {
            BuildModel.Step s = steps.get(i);
            if (get(base, s.skill) >= s.to) continue;
            if (s.lvl > level) return -1;
            int r = reason.of(s.skill);
            if (r == OK) return i;
            if (permanent(r) || s.save) return -1;
        }
        return -1;
    }


    static final class Demand {
        final Map<Integer, Integer> need = new LinkedHashMap<>();
        final Map<Integer, Integer> first = new HashMap<>();
    }

    static Demand demand(List<BuildModel.Step> steps, int level, Map<Integer, Integer> base, Future future) {
        Demand d = new Demand();
        Map<Integer, Integer> b = new HashMap<>(base);
        int spent = 0;
        for (int lv = level + 1; lv <= MAX_LEVEL; lv++) {
            Reason r = future.at(lv, b);
            for (int k = plain(steps, lv, b, r); k >= 0; k = plain(steps, lv, b, r)) {
                int skill = steps.get(k).skill;
                b.put(skill, get(b, skill) + 1);
                spent++;
                if (!d.first.containsKey(k)) d.first.put(k, lv);
            }
            d.need.put(lv, spent - (lv - level));
            boolean all = true;
            for (BuildModel.Step s : steps) if (get(b, s.skill) < s.to) { all = false; break; }
            if (all) break;
        }
        return d;
    }


    private static Result hold(List<BuildModel.Step> steps, int i, int firstOpen, int level, Map<Integer, Integer> base, Reason reason, int points, Future future) {
        BuildModel.Step s = steps.get(i);
        boolean learnable = s.lvl > level && reason.of(s.skill) == OK;
        if (future == null) {
            if (points <= 0) return new Result(WAIT, firstOpen);
            return learnable ? new Result(NEXT, i) : new Result(SAVE, i, s.lvl > level ? s.lvl : 0);
        }
        if (points <= 0) {
            for (int lv = level + 1; lv <= MAX_LEVEL; lv++) {
                Result x = next(steps, lv, base, future.at(lv, base), lv - level, future);
                if (x.state == NEXT) return new Result(WAIT, x.index, lv > level + 1 ? lv : 0);
                if (x.state == BLOCKED || x.state == DONE) break;
            }
            return new Result(WAIT, firstOpen);
        }
        Demand d = demand(steps, level, base, future);
        Integer spendsOn = d.first.get(i);
        int until = spendsOn == null ? MAX_LEVEL + 1 : spendsOn, keep = 0;
        boolean safe = learnable;
        for (Map.Entry<Integer, Integer> e : d.need.entrySet()) {
            if (e.getKey() < until && e.getValue() >= points) safe = false;
            keep = Math.max(keep, e.getValue());
        }
        if (safe) return new Result(NEXT, i);
        if (keep > 0 && points > keep) {
            int top = MAX_LEVEL + 1;
            for (Map.Entry<Integer, Integer> e : d.need.entrySet()) if (e.getValue() == keep) { top = e.getKey(); break; }
            for (int j = i + 1; j < steps.size(); j++) {
                BuildModel.Step t = steps.get(j);
                if (t.lvl <= top && get(base, t.skill) < t.to && reason.of(t.skill) == OK) return new Result(NEXT, j);
            }
        }
        return new Result(SAVE, i, spendsOn != null ? spendsOn : s.lvl > level ? s.lvl : 0);
    }



    static BuildModel.Text saveNote(BuildModel.Step st) {
        if (!st.save || st.note == null) return null;
        return st.note.de.toLowerCase(java.util.Locale.ROOT).contains("aufheben") || st.note.en.toLowerCase(java.util.Locale.ROOT).startsWith("keep") ? null : st.note;
    }

    interface Width { float of(String text); }





    static String breakAtHyphens(String title, Width width, float line) {
        StringBuilder out = new StringBuilder();
        for (String word : title.split(" ")) {
            if (out.length() > 0) out.append(' ');
            String rest = word;
            while (width.of(rest) > line) {
                int cut = -1;
                for (int k = rest.indexOf('-'); k > 0 && k < rest.length() - 1; k = rest.indexOf('-', k + 1)) {
                    int end = rest.charAt(k + 1) == '/' ? k + 2 : k + 1;
                    if (end < rest.length() && width.of(rest.substring(0, end)) <= line) cut = end;
                }
                if (cut < 0) break;
                out.append(rest, 0, cut).append('\n');
                rest = rest.substring(cut);
            }
            out.append(rest);
        }
        return out.toString();
    }

    static int get(Map<Integer, Integer> m, int k) { Integer v = m.get(k); return v == null ? 0 : v; }


    static Future tableFuture(final Map<Integer, BuildModel.SkillInfo> skills) { return (lv, b) -> tableReason(skills, lv, b); }


    static Reason tableReason(final Map<Integer, BuildModel.SkillInfo> skills, final int level, final Map<Integer, Integer> base) {
        return skill -> {
            BuildModel.SkillInfo s = skills.get(skill);
            if (s == null) return R_DATA;
            int b = get(base, skill);
            if (b >= Math.min(s.max, SKILL_MAX)) return R_MAX;
            if (level < s.req + b) return R_LEVEL;
            for (int p : s.pre) if (get(base, p) < 1) return R_PREREQ;
            return OK;
        };
    }


    static Reason snapReason(final BuildModel.Snap snap, final Map<Integer, BuildModel.SkillInfo> skills) {
        final Reason table = tableReason(skills, snap.level, snap.base);
        return skill -> { Integer r = snap.reason.get(skill); return r != null ? r : table.of(skill); };
    }


    static List<BuildModel.Step> chain(BuildModel.Guide g, int phase) {
        int from = phase;
        while (from > 0 && !g.phases.get(from).respec) from--;
        List<BuildModel.Step> out = new ArrayList<>();
        for (int i = from; i <= phase && i < g.phases.size(); i++) out.addAll(g.phases.get(i).skills);
        return out;
    }






    static int phase(BuildModel.Guide g, int level, String chosen) {
        int at = levelPhase(g, level), confirmed = confirmed(g, chosen);
        for (int r = latestRespec(g, at); r > 0 && r > confirmed; r = latestRespec(g, at)) at = r - 1;
        return at;
    }

    private static int confirmed(BuildModel.Guide g, String chosen) {
        int c = chosen == null ? -1 : g.phaseIndex(chosen);
        return c >= 0 && g.phases.get(c).respec ? c : -1;
    }


    private static int latestRespec(BuildModel.Guide g, int at) { for (int i = at; i > 0; i--) if (g.phases.get(i).respec) return i; return -1; }

    static int levelPhase(BuildModel.Guide g, int level) {
        int at = 0;
        for (int i = 0; i < g.phases.size(); i++) if (level >= g.phases.get(i).lo) at = i;
        return at;
    }


    static int pendingRespec(BuildModel.Guide g, int level, String chosen) {
        int confirmed = confirmed(g, chosen), at = levelPhase(g, level), first = -1;
        for (int r = latestRespec(g, at); r > 0 && r > confirmed; r = latestRespec(g, r - 1)) first = r;
        return first;
    }

    static boolean respecPending(BuildModel.Guide g, int level, String chosen) { return pendingRespec(g, level, chosen) >= 0; }



    static int statNext(List<BuildModel.StatRule> rules, Map<String, Integer> base) {
        if (base == null) return -1;
        for (int i = 0; i < rules.size(); i++) {
            BuildModel.StatRule r = rules.get(i);
            if (r.rest) return i;
            if (r.to >= 0 && base.containsKey(r.stat) && base.get(r.stat) < r.to) return i;
        }
        return -1;
    }


    static Map<Integer, Integer> distribution(List<BuildModel.Step> steps) {
        Map<Integer, Integer> out = new LinkedHashMap<>();
        for (BuildModel.Step s : steps) out.put(s.skill, Math.max(get(out, s.skill), s.to));
        return out;
    }


    static int outsidePlan(List<BuildModel.Step> steps, Map<Integer, Integer> base) {
        Map<Integer, Integer> d = distribution(steps);
        int n = 0;
        for (Map.Entry<Integer, Integer> e : base.entrySet()) n += Math.max(0, e.getValue() - get(d, e.getKey()));
        return n;
    }
}
