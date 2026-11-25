package com.xiaoboluo.d2r;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;






public class BuildPlanTest {
    static int failed, passed;

    static void check(String what, Object got, Object want) {
        boolean ok = String.valueOf(got).equals(String.valueOf(want));
        if (ok) passed++; else { failed++; System.out.println("FAIL " + what + ": " + got + " (want " + want + ")"); }
    }

    static Map<Integer, Integer> kv(String t) {
        Map<Integer, Integer> m = new HashMap<>();
        if (!t.equals("-")) for (String x : t.split(",")) { String[] p = x.split("="); m.put(Integer.parseInt(p[0]), Integer.parseInt(p[1])); }
        return m;
    }

    static BuildModel.Step step(int lvl, int skill, int to, boolean save) {
        BuildModel.Step s = new BuildModel.Step(); s.lvl = lvl; s.skill = skill; s.to = to; s.save = save; return s;
    }

    static void sharedCases(String dir) throws IOException {
        int n = 0;
        for (String line : Files.readAllLines(Paths.get(dir, "build_plan_cases.tsv"), StandardCharsets.UTF_8)) {
            if (line.startsWith("#") || line.trim().isEmpty()) continue;
            String[] f = line.split("\t");
            List<BuildModel.Step> steps = new ArrayList<>();
            if (!f[3].equals("-")) for (String s : f[3].split(",")) { String[] p = s.split(":"); steps.add(step(Integer.parseInt(p[0]), Integer.parseInt(p[1]), Integer.parseInt(p[2]), p.length > 3 && p[3].equals("s"))); }
            Map<Integer, Integer> base = kv(f[4]), reasons = kv(f[5]);
            BuildPlan.Future future = f.length > 7 ? BuildPlan.tableFuture(table(f[7])) : null;
            BuildPlan.Result r = BuildPlan.next(steps, Integer.parseInt(f[1]), base, skill -> reasons.getOrDefault(skill, 0), Integer.parseInt(f[2]), future);
            check("case " + f[0], r, f[6]);
            n++;
        }
        check("shared cases read", n >= 10, true);
    }


    static Map<Integer, BuildModel.SkillInfo> table(String t) {
        Map<Integer, BuildModel.SkillInfo> m = new HashMap<>();
        for (String x : t.split(",")) {
            String[] p = x.split(":");
            BuildModel.SkillInfo s = new BuildModel.SkillInfo(); s.id = Integer.parseInt(p[0]); s.req = Integer.parseInt(p[1]); s.max = Integer.parseInt(p[2]);
            if (p.length > 3) { String[] q = p[3].split("/"); s.pre = new int[q.length]; for (int k = 0; k < q.length; k++) s.pre[k] = Integer.parseInt(q[k]); }
            m.put(s.id, s);
        }
        return m;
    }

    static Map<Integer, BuildModel.SkillInfo> skills() {
        Map<Integer, BuildModel.SkillInfo> m = new HashMap<>();
        int[][] t = {{10, 1, 20}, {11, 6, 20}, {12, 30, 20}};
        for (int[] x : t) { BuildModel.SkillInfo s = new BuildModel.SkillInfo(); s.id = x[0]; s.req = x[1]; s.max = x[2]; m.put(s.id, s); }
        m.get(11).pre = new int[]{10};
        return m;
    }




    static void laterLevel() {
        Map<Integer, BuildModel.SkillInfo> sk = table("20:1:20,21:6:20,22:18:20");
        BuildModel.Step keep = step(18, 22, 1, true); keep.note = new BuildModel.Text("Punkte aus Stufe 16 und 17 aufheben", "Keep the points from levels 16 and 17");
        List<BuildModel.Step> steps = new ArrayList<>();
        steps.add(step(2, 20, 4, false)); steps.add(step(7, 21, 10, false)); steps.add(keep); steps.add(step(18, 22, 6, false)); steps.add(step(18, 21, 13, false));
        Map<Integer, Integer> base = kv("20=4,21=10");
        BuildPlan.Reason now = BuildPlan.tableReason(sk, 17, base);
        BuildPlan.Future future = BuildPlan.tableFuture(sk);
        BuildPlan.Result r = BuildPlan.next(steps, 17, base, now, 2, future);
        check("later: kept for the save step", r, "SAVE 2 18");
        check("later: a note that only keeps points is not repeated", BuildPlan.saveNote(steps.get(r.index)), null);
        BuildModel.Step why = step(18, 22, 1, true); why.note = new BuildModel.Text("Fortbewegung über Hindernisse", "Movement over obstacles");
        check("later: a reason is shown", BuildPlan.saveNote(why).get(true), "Fortbewegung über Hindernisse");
        check("later: a step without save gives no reason", BuildPlan.saveNote(steps.get(4)), null);
        check("later: no points, next level", BuildPlan.next(steps, 17, base, now, 0, future), "WAIT 2");
        check("later: no points, waits for level 18", BuildPlan.next(steps, 15, base, BuildPlan.tableReason(sk, 15, base), 0, future), "WAIT 2 18");
        check("later: surplus pulled forward", BuildPlan.next(steps, 17, base, now, 4, future), "NEXT 4");
        check("later: without look ahead all kept", BuildPlan.next(steps, 17, base, now, 4, null), "SAVE 2 18");
        check("later: exact need kept", BuildPlan.next(steps, 17, base, now, 3, future), "SAVE 2 18");
    }


    static void titleBreaks() {
        BuildPlan.Width chars = String::length;
        check("title: fits", BuildPlan.breakAtHyphens("Feuer-Hexer", chars, 12), "Feuer-Hexer");
        check("title: hyphen break", BuildPlan.breakAtHyphens("Feuer-Hexenmeister", chars, 12), "Feuer-\nHexenmeister");
        check("title: last hyphen that fits", BuildPlan.breakAtHyphens("Blitz-Speer-Amazone (Javazon)", chars, 12), "Blitz-Speer-\nAmazone (Javazon)");
        check("title: two breaks", BuildPlan.breakAtHyphens("Ab-Cdefgh-Ijklmn-Opq", chars, 9), "Ab-\nCdefgh-\nIjklmn-\nOpq");
        check("title: no hyphen stays whole", BuildPlan.breakAtHyphens("Totenbeschwoerer", chars, 12), "Totenbeschwoerer");
        check("title: slash after the hyphen stays on the line", BuildPlan.breakAtHyphens("Abcdefghi-/Jklmn-Opqrst", chars, 12), "Abcdefghi-/\nJklmn-Opqrst");
        check("title: leading or trailing hyphen ignored", BuildPlan.breakAtHyphens("-Abcdefghijklm-", chars, 12), "-Abcdefghijklm-");
    }

    static BuildModel.Phase phase(String id, int lo, int hi, boolean respec, BuildModel.Step... steps) {
        BuildModel.Phase p = new BuildModel.Phase(); p.id = id; p.lo = lo; p.hi = hi; p.respec = respec;
        for (BuildModel.Step s : steps) p.skills.add(s);
        return p;
    }

    public static void main(String[] args) throws IOException {
        sharedCases(args.length > 0 ? args[0] : "dm/android/tests");
        Map<Integer, BuildModel.SkillInfo> sk = skills();
        Map<Integer, Integer> base = kv("10=1");
        check("reason ok", BuildPlan.tableReason(sk, 6, base).of(11), BuildPlan.OK);
        check("reason level", BuildPlan.tableReason(sk, 5, base).of(11), BuildPlan.R_LEVEL);
        check("reason prereq", BuildPlan.tableReason(sk, 9, kv("-")).of(11), BuildPlan.R_PREREQ);
        check("reason max", BuildPlan.tableReason(sk, 40, kv("10=20")).of(10), BuildPlan.R_MAX);
        check("reason data", BuildPlan.tableReason(sk, 9, base).of(99), BuildPlan.R_DATA);
        check("reason next point needs level", BuildPlan.tableReason(sk, 3, kv("10=3")).of(10), BuildPlan.R_LEVEL);

        BuildModel.Guide g = new BuildModel.Guide();
        g.phases.add(phase("lvl", 1, 29, false, step(1, 10, 3, false)));
        g.phases.add(phase("end", 30, 99, false, step(30, 11, 1, false)));
        check("chain cumulative", BuildPlan.chain(g, 1).size(), 2);
        check("auto phase low", BuildPlan.phase(g, 12, "auto"), 0);
        check("auto phase high", BuildPlan.phase(g, 31, "auto"), 1);
        check("chosen cumulative phase does not pin", BuildPlan.phase(g, 12, "end"), 0);
        g.phases.get(1).respec = true;
        check("respec waits", BuildPlan.phase(g, 31, "auto"), 0);
        check("respec pending", BuildPlan.respecPending(g, 31, "auto"), true);
        check("respec confirmed", BuildPlan.phase(g, 31, "end"), 1);
        check("respec chain starts over", BuildPlan.chain(g, 1).size(), 1);

        g.phases.add(phase("late", 60, 99, false, step(60, 10, 20, false)));
        check("three phases unconfirmed", BuildPlan.phase(g, 70, "auto"), 0);
        check("three phases pending", BuildPlan.pendingRespec(g, 70, "auto"), 1);
        check("three phases confirmed", BuildPlan.phase(g, 70, "end"), 2);
        check("three phases confirmed early", BuildPlan.phase(g, 40, "end"), 1);
        check("three phases chain", BuildPlan.chain(g, 2).size(), 2);
        check("chosen cumulative id is no confirmation", BuildPlan.phase(g, 70, "late"), 0);

        List<BuildModel.StatRule> rules = new ArrayList<>();
        BuildModel.StatRule a = new BuildModel.StatRule(); a.stat = "str"; a.to = 45; rules.add(a);
        BuildModel.StatRule b = new BuildModel.StatRule(); b.stat = "ene"; b.none = true; rules.add(b);
        BuildModel.StatRule c = new BuildModel.StatRule(); c.stat = "vit"; c.rest = true; rules.add(c);
        Map<String, Integer> sb = new HashMap<>(); sb.put("str", 30);
        check("stat next open target", BuildPlan.statNext(rules, sb), 0);
        sb.put("str", 45);
        check("stat next rest", BuildPlan.statNext(rules, sb), 2);
        check("stat next unknown base", BuildPlan.statNext(rules, null), -1);

        List<BuildModel.Step> st = new ArrayList<>();
        st.add(step(1, 10, 4, false)); st.add(step(6, 11, 1, false)); st.add(step(7, 10, 10, false));
        check("distribution", BuildPlan.distribution(st), "{10=10, 11=1}");
        check("outside plan", BuildPlan.outsidePlan(st, kv("10=12,12=2")), 4);

        BuildModel.Follow f = BuildModel.Follow.decode(BuildModel.Follow.encode(new BuildModel.Follow("war-fire", 3, null)));
        check("follow codec", f.guide + "/" + f.version + "/" + f.phase, "war-fire/3/auto");
        check("follow key", BuildModel.Follow.key("Hero", 7), "Hero#7");
        check("follow broken", BuildModel.Follow.decode("x\ty\tz"), null);

        int[] out = new int[BuildModel.SNAP_INTS];
        out[0] = 1; out[1] = 5; out[2] = 7; out[3] = 12; out[7] = 50; out[12] = 2; out[13] = 45; out[14] = 25; out[15] = 60; out[16] = 15;
        int h = BuildModel.SNAP_HEAD; out[h] = 394; out[h + 1] = 3; out[h + 2] = 0; out[h + 3] = 401; out[h + 4] = 0; out[h + 5] = 2;
        byte[] name = "Hero".getBytes(StandardCharsets.UTF_8);
        BuildModel.Snap s = BuildModel.Snap.parse(out, out.length, name, name.length);
        check("snap", s.valid + " " + s.name + " " + s.cls + " " + s.level + " " + s.base + " " + s.reason, "true Hero 7 12 {401=0, 394=3} {401=2, 394=0}");
        check("snap base", s.attr[0] + " " + s.baseMap(), "50 {str=45, vit=60, dex=25, ene=15}");
        check("snap stat next from base", BuildPlan.statNext(rules, s.baseMap()), 2);
        out[16] = -1;
        check("snap base unreadable", BuildModel.Snap.parse(out, out.length, name, name.length).baseMap(), null);
        check("snap invalid", BuildModel.Snap.parse(new int[BuildModel.SNAP_INTS], BuildModel.SNAP_INTS, name, name.length).valid, false);
        check("snap no name", BuildModel.Snap.parse(out, out.length, new byte[0], 0).valid, false);

        laterLevel();
        titleBreaks();
        System.out.println("BuildPlanTest: " + passed + " ok, " + failed + " failed");
        if (failed > 0) System.exit(1);
    }
}
