package com.xiaoboluo.d2r;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Set;




final class FightLogTest {
    private static int checks;
    private static void check(boolean value, String what) { checks++; if (!value) throw new AssertionError(what); }

    static final String LIST = "log\t3\t1\n"

        + "f\t0\t11\t2\t698\t50\t73\t0\t0\t1\t2530\t0\t0\t0\t1\t1\t0\t0\t0\n"
        + "g\t0\t0\tBossA\t0\t1\t1\t0\t52\t0\t2530\t3000\t0\t0\t0\n"

        + "f\t1\t22\t1\t615\t50\t45\t1\t5\t6\t3000\t900\t100\t79\t3\t0\t1\t3050\t950\n"
        + "g\t1\t0\tLeadB\t4\t0\t1\t1\t50\t665\t1500\t1500\t0\t1\t1200\n"
        + "g\t1\t1\tKindC\t0\t0\t4\t4\t60\t500\t2000\t2000\t0\t0\t1500\n"
        + "g\t1\t2\tOtherD\t2\t0\t1\t0\t70\t0\t500\t600\t1\t0\t350\n"

        + "f\t2\t33\t3\t300\t30\t45\t1\t0\t3\t1230\t0\t0\t133\t1\t0\t0\t0\t0\n"
        + "g\t2\t0\tTrioE\t0\t0\t3\t0\t30\t0\t1230\t5000\t0\t0\t0\n";

    public static void main(String[] args) {
        FightLog log = FightLog.parse(LIST);
        FightLog.Names names = k -> k.equals("@others") ? "weitere" : k.equals("LeadB") ? "Lead name" : k.equals("KindC") ? "Kind name" : k;
        check(log.count == 3 && log.recording && log.entries.size() == 3, "three fights");
        FightLog.Entry death = log.entries.get(0), group = log.entries.get(1), aborted = log.entries.get(2);
        check(death.end == FightLog.END_DEATH && death.lead == 50 && Math.abs(death.seconds() - 27.92) < 1e-9 && death.sim && Math.abs(death.dps() - 2530 / 27.92) < 1e-9, "death entry");
        check(death.minLifePercent() == 0 && group.minLifePercent() == 31 && aborted.aborted() && !death.aborted(), "min life and aborted");

        check(FightLog.label(death, names).equals("BossA") && FightLog.label(group, names).equals("Lead name + 5") && FightLog.label(aborted, names).equals("TrioE ×3"), "labels " + FightLog.label(group, names));
        check(FightLog.label(group.groups.get(1), names).equals("Kind name ×4") && FightLog.label(group.groups.get(2), names).equals("weitere"), "group labels");
        check(group.minionShare() == 23 && death.minionShare() == 0, "minion share " + group.minionShare());

        check(group.known && Math.abs(group.own() - 3050 / 24.6) < 1e-9 && Math.abs(group.companionDps() - 950 / 24.6) < 1e-9, "own and companion DPS");
        check(!death.known && death.own() == death.dps() && death.companionDps() < 0, "an older fight keeps its DPS and has no companion part");
        check(Math.abs(group.groups.get(1).own(group.lead + group.duration, true) - 1500 / 17.6) < 1e-9 && group.groups.get(1).own(0, false) == group.groups.get(1).dps(0), "a group's own part");

        check(aborted.partial() == null && death.partial() == null && group.partial() == null, "no kill, a death, a win");
        FightLog.Entry escaped = FightLog.parse(LIST.replace("f\t2\t33\t3\t300\t30\t45\t1\t0\t3", "f\t2\t33\t3\t300\t30\t45\t1\t2\t3")).entries.get(2);
        check("2 / 3".equals(escaped.partial()), "partial kills " + escaped.partial());

        check(group.groups.get(0).sim && !group.groups.get(1).sim && !group.groups.get(2).sim, "group simulations");

        FightLog.Group invaders = group.groups.get(1), others = group.groups.get(2);
        check(Math.abs(invaders.seconds() - 17.6) < 1e-9 && Math.abs(invaders.dps(group.lead + group.duration) - 2000 / 17.6) < 1e-9, "group time");
        check(others.seconds() < 0 && Math.abs(others.dps(group.lead + group.duration) - 500 / ((665 - 70) / 25.0)) < 1e-9, "open group");


        FightLog one = FightLog.parse("log\t2\t1\nf\t0\t44\t1\t0\t50\t2\t0\t1\t1\t0\t6\t0\t214\t1\t0\t0\t0\t0\ng\t0\t0\tFallen\t0\t0\t1\t1\t50\t50\t6\t6\t0\t0\t0\n"
            + "f\t1\t55\t1\t2\t50\t2\t0\t1\t1\t0\t12\t0\t214\t1\t0\t0\t0\t0\ng\t1\t0\tFallen\t0\t0\t1\t1\t50\t52\t12\t12\t0\t0\t0\n");
        FightLog.Entry instant = one.entries.get(0), twoTicks = one.entries.get(1);
        check(instant.dps() < 0 && FightLog.time(instant.seconds(), CombatModel.L_DE).equals("<0,1 s") && FightLog.time(instant.seconds(), CombatModel.L_EN).equals("<0.1 s"), "one-hit kill " + FightLog.time(instant.seconds(), CombatModel.L_DE));
        check(instant.groups.get(0).dps(instant.lead + instant.duration) < 0 && FightLog.time(instant.groups.get(0).seconds(), CombatModel.L_DE).equals("<0,1 s"), "one-hit group");
        check(FightLog.time(1 / 25.0, CombatModel.L_DE).equals("<0,1 s") && FightLog.time(twoTicks.seconds(), CombatModel.L_DE).equals("0,1 s") && Math.abs(twoTicks.dps() - 12 / 0.08) < 1e-9, "two ticks");
        check(FightLog.time(-1, CombatModel.L_DE).equals(CombatModel.time(-1, CombatModel.L_DE)) && FightLog.time(27.92, CombatModel.L_EN).equals("27.9 s"), "other times unchanged");

        Set<Integer> none = new HashSet<>();
        check(order(log.rows(FightLog.SORT_NONE, false, none, names)).equals(Arrays.asList(0, 1, 2)), "newest first");
        check(order(log.rows(FightLog.SORT_KILL, false, none, names)).equals(Arrays.asList(1, 0, 2)), "fastest kill first");
        check(order(log.rows(FightLog.SORT_KILL, true, none, names)).equals(Arrays.asList(2, 0, 1)), "reversed");
        check(order(log.rows(FightLog.SORT_DPS, false, none, names)).equals(Arrays.asList(1, 2, 0)), "highest DPS first " + order(log.rows(FightLog.SORT_DPS, false, none, names)));
        check(order(log.rows(FightLog.SORT_LIFE, false, none, names)).equals(Arrays.asList(0, 1, 2)), "most dangerous first");
        check(order(log.rows(FightLog.SORT_NAME, false, none, names)).equals(Arrays.asList(0, 1, 2)), "by name");

        Set<Integer> opened = new HashSet<>(Arrays.asList(1, 2));
        List<FightLog.Row> rows = log.rows(FightLog.SORT_NONE, false, opened, names);
        check(rows.size() == 6 && rows.get(1).entry == group && rows.get(1).group == null && rows.get(2).group == group.groups.get(0) && rows.get(5).entry == aborted && rows.get(5).group == null, "opened groups");

        for (String bad : new String[]{"log\t2\t1\n" + LIST.substring(LIST.indexOf("f\t0")), LIST.replace("g\t1\t2\tOtherD", "g\t1\t3\tOtherD"), LIST.replace("\t79\t3\t0\t1\t", "\t799\t3\t0\t1\t"), "f\t0\n"}) {
            boolean thrown = false;
            try { FightLog.parse(bad); } catch (IllegalArgumentException e) { thrown = true; }
            check(thrown, "rejected: " + bad.substring(0, Math.min(30, bad.length())));
        }
        fight();
        System.out.println("FightLogTest OK, checks=" + checks);
    }
    private static List<Integer> order(List<FightLog.Row> rows) { List<Integer> o = new ArrayList<>(); for (FightLog.Row r : rows) o.add(r.entry.index); return o; }

    static void fight() {


        StringBuilder t = new StringBuilder("fight\t2\t740\t25\t30\t-1\nact\t0\t0\nact\t1\t52\n");
        for (int i = 0; i < 30; i++) t.append("trace\t").append(i).append("\t").append(100 - i).append(",100,").append(100 - i).append("\t").append(100 - 3 * i).append(",100,").append(100 - 3 * i).append("\t50,60,55\t0\t").append(i % 3 == 0 ? "2" : "1").append("\n");
        t.append("drink\t300\t2\t40.0\t50.0\n");
        for (int i : new int[]{3, 6, 9, 12, 15}) t.append("en\t").append(i).append("\t3\t0\n");
        t.append("en\t4\t9\t0\nen\t7\t9\t0\nen\t10\t4\t8\nen\t13\t33\t0\n");
        t.append("state\t300\t0\t13\nstate\t400\t1\t32\nstate\t420\t1\t99\n");
        t.append("sim\t2\t800\t32\t25\t40.70\t40\t-1\n");
        for (int i = 0; i < 25; i++) t.append("strace\t").append(i).append("\t").append(100 - i).append(",100,").append(100 - i).append("\t90,95,90\n");
        FightLog.Detail d = FightLog.detail(t.toString());
        check(d.fight.n == 30 && d.enemy[3] == 3 && d.own[10] == 8 && d.states.size() == 3 && d.sim != null && d.sim.n == 25 && Math.abs(d.simDps - 40.7) < 1e-9 && d.simOffset == 40, "detail");
        int[] c = FightLog.counts(d);
        check(c[0] == 5 && c[1] == 2 && c[2] == 1 && c[3] == 1 && c[4] == 1, "counts " + Arrays.toString(c));
        List<FightLog.Band> bands = FightLog.bands(d, id -> id != 99, 3);
        check(bands.size() == 2 && bands.get(0).id == 13 && bands.get(0).from == 0 && bands.get(0).to == 300 && bands.get(0).ended && !bands.get(0).began
              && bands.get(1).id == 32 && bands.get(1).from == 400 && bands.get(1).to == 740 && bands.get(1).began && !bands.get(1).ended, "bands");
        check(FightLog.bands(d, id -> true, 1).size() == 1, "at most max bands");

        FightLog.Detail cut = FightLog.detail(t.toString().replaceAll("strace\t24\t[^\n]*\n", ""));
        check(cut.sim == null && cut.fight.n == 30, "incomplete simulation dropped");

        CombatGraph.Words words = new CombatGraph.Words() {
            public String skill(int id) { return id == 0 ? "Normaler Angriff" : "Skill" + id; }
            public boolean icon(int id) { return false; }
            public String shortage(String skill) { return "-"; }
            public String potion(int group) { return "Trank" + group; }
        };
        CombatGraph.Extra x = new CombatGraph.Extra();
        x.enemy = d.enemy; x.own = d.own; x.zero = 50; x.ownLane = "Du"; x.enemyLane = "BossA"; x.enemyLaneShort = "Gegner";
        x.hit = "Treffer"; x.miss = "verfehlt"; x.dodge = "ausgewichen"; x.block = "geblockt"; x.crit = "Krit";
        x.hitColour = 1; x.missColour = 2; x.blockColour = 3; x.simColour = 4; x.starColour = 5; x.dodgeColour = 6;
        x.bandText.add("Schutz · bis 10 s"); x.bandSpan.add(new int[]{0, 300});
        x.group = new float[30]; Arrays.fill(x.group, 50);
        x.sim = d.sim; x.simOffset = d.simOffset; x.simLabel = "Simulation";
        CombatGraph g = CombatGraph.build(d.fight, 1000, 300, 2600, 1100, 1, 20, s -> s.length() * 10, words, "BossA", "Dein Leben", "Tod 27,6 s", true, new int[]{10, 11, 12, 13}, x);
        List<String> legend = new ArrayList<>(); for (CombatGraph.Entry e : g.legend) legend.add(e.text);
        check(legend.equals(Arrays.asList("BossA", "Dein Leben", "Simulation", "Skill52", "Normaler Angriff", "Krit ×1", "BossA: Treffer ×5", "verfehlt ×2", "ausgewichen ×1", "geblockt ×1", "Trank0 ×1")), "legend " + legend);

        check(g.enemyT > g.laneB && g.enemyB < 1100 && g.bands.size() == 1 && g.bands.get(0).box[3] < g.laneT && g.bands.get(0).box[1] > g.manaB, "lanes and band");
        int enemyTicks = 0; for (float[] r : g.ticks) if (r[1] == g.enemyT) enemyTicks++;
        check(enemyTicks == 9, "one enemy tick per outcome " + enemyTicks);
        check(g.stars.size() == 2 && g.simEnemy.length > 8 && g.simLife.length > 8 && g.group.length > 4, "stars (lane and legend), dashed simulation, group line");

        float deathX = g.endX;
        check(deathX < g.plotR - 10 && g.endBox[2] <= deathX + 1e-3, "end marker at the death " + deathX + " " + g.plotR);
        float zeroX = g.plotL + (g.plotR - g.plotL) * 50 / 840f; boolean zero = false;
        for (CombatGraph.Label l : g.labels) zero |= l.text.equals("0 s") && Math.abs(l.x + 15 - zeroX) < 1e-2;
        check(zero, "the axis counts from the first damage");


        x.groupLabel = "LeadB"; x.simLife = false;
        CombatGraph one = CombatGraph.build(d.fight, 1000, 300, 2600, 1100, 1, 20, s -> s.length() * 10, words, "LeadB + 5", "Dein Leben", "Tod 27,6 s", true, new int[]{10, 11, 12, 13}, x);
        check(one.legend.get(0).text.equals("LeadB + 5") && one.legend.get(0).kind == CombatGraph.LINE_ENEMY && one.legend.get(1).text.equals("LeadB") && one.legend.get(1).kind == CombatGraph.LINE_GROUP
              && one.legend.get(3).kind == CombatGraph.LINE_SIM, "thick and thin legend entries");
        check(one.simEnemy.length > 8 && one.simLife.length == 0, "no simulated own life for one enemy of several");
        x.groupLabel = null; x.simLife = true;
        check(g.legend.stream().noneMatch(e -> e.kind == CombatGraph.LINE_GROUP), "no thin entry without its name");

        x.sim = null;
        CombatGraph plain = CombatGraph.build(d.fight, 1000, 300, 2600, 1100, 1, 20, s -> s.length() * 10, words, "BossA", "Dein Leben", "Tod 27,6 s", true, new int[]{10, 11, 12, 13}, x);
        check(plain.simEnemy == null && !plain.legend.get(2).text.equals("Simulation") && Math.abs(plain.endX - plain.plotR) < 1e-3, "plain");
    }
}
