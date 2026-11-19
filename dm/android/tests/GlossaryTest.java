




package com.xiaoboluo.d2r;

import java.util.List;

public class GlossaryTest {
    private static int checks, fails;
    private static void check(boolean ok, String what) { checks++; if (!ok) { fails++; System.out.println("  FAIL " + what); } }

    private static GlossaryModel.Term term(String id, String cat, int stat, String de, String en, int... stats) {
        GlossaryModel.Term t = new GlossaryModel.Term();
        t.id = id; t.cat = cat; t.nameStat = stat; t.fallback = new GlossaryModel.Text(de, en); t.stats = stats; t.keywords = new String[]{id};
        return t;
    }

    private static final GlossaryModel.Names NAMES = new GlossaryModel.Names() {
        public String stat(int id, boolean de) {
            if (id == 16) return de ? "%+d%% verbesserte Verteidigung" : "%+d%% Enhanced Defense";
            if (id == 19) return de ? "%+d zu Angriffswert" : "%+d to Attack Rating";
            return null;
        }
        public String group(int stat, boolean de) { return stat == 39 ? (de ? "Alle Widerstände %+d" : "All Resistances %+d") : null; }
        public String word(String key, boolean de) { return null; }
    };

    private static GlossaryModel model() {
        GlossaryModel m = new GlossaryModel();
        m.terms.add(term("defense", "def", 31, "Verteidigung", "Defense", 31));
        m.terms.add(term("enhdef", "def", 16, "Verbesserte Verteidigung", "Enhanced Defense", 16, 171));
        m.terms.add(term("ar", "off", 19, "Angriffswert", "Attack Rating", 19));
        GlossaryModel.Term r = term("resist", "def", -1, "Widerstände", "Resistances", 39, 41); r.nameGroup = 39; m.terms.add(r);
        m.terms.add(term("fcr", "speed", 105, "Schnellere Zauberrate", "Faster Cast Rate", 105));
        m.terms.add(term("leech", "off", 60, "Lebensentzug", "Life Leech", 60, 62));
        String[][] p = {{"noLive", "kein Spiel", "no game"}, {"def", "Verteidigung: [{0}]", "Defense: [{0}]"}, {"pct", "Prozent [+{0} %]", "percent [+{0}%]"},
            {"defWith", "ohne [{0}]", "without [{0}]"}, {"defWould", "mit +{0} % [{1}]", "with +{0}% [{1}]"}, {"noModel", "geht nicht", "cannot"},
            {"hitYou", "Stufe {0}: [{1} %]", "level {0}: [{1}%]"}, {"hitYouWith", "Stufe {0}: [{1} %] statt {2} %", "level {0}: [{1}%] instead of {2}%"},
            {"hitYouWould", "Stufe {0}: dann [{1} %] statt {2} %", "level {0}: then [{1}%] instead of {2}%"},
            {"diff", "Schwierigkeit {0}", "difficulty {0}"}, {"res", "{0}: [{1} %] ({2}, {3}, {4} %)", "{0}: [{1}%] ({2}, {3}, {4}%)"},
            {"leech", "L [{0} %] M {1} %", "L [{0}%] M {1}%"}, {"leechDiff", "{0}: [{1} %] {2} %", "{0}: [{1}%] {2}%"},
            {"cast", "{0} % → [{1}] ({2} s)", "{0}% → [{1}] ({2} s)"}, {"next", "{0} % → {1} (noch [{2} %])", "{0}% → {1} ([{2}%])"}, {"top", "Ende", "end"},
            {"noClass", "keine", "none"}, {"ar", "AW [{0}]", "AR [{0}]"}, {"hitIt", "triffst Stufe {0} zu [{1} %]", "hit level {0} [{1}%]"},
            {"diff0", "Normal", "Normal"}, {"diff1", "Albtraum", "Nightmare"}, {"diff2", "Hölle", "Hell"}};
        for (String[] x : p) m.phrases.put(x[0], new GlossaryModel.Text(x[1], x[2]));
        m.fcr[7] = new int[][]{{0, 15}, {9, 14}, {18, 13}, {30, 12}, {48, 11}, {75, 10}, {125, 9}};
        return m;
    }

    private static GlossaryCalc.Snap snap(int level, int diff, int expansion) {
        int[] w = new int[GlossaryCalc.INTS];
        w[GlossaryCalc.VER] = GlossaryCalc.VERSION; w[GlossaryCalc.VALID] = 1; w[GlossaryCalc.SESSION] = 3; w[GlossaryCalc.CLASS] = 7; w[GlossaryCalc.LEVEL] = level;
        w[GlossaryCalc.DIFF] = diff; w[GlossaryCalc.EXPANSION] = expansion;

        w[GlossaryCalc.AC] = 29; w[GlossaryCalc.DEX] = 22; w[GlossaryCalc.ACPCT] = 100; w[GlossaryCalc.SKILLPCT] = 100; w[GlossaryCalc.DEF] = 68;
        w[GlossaryCalc.BLOCK] = 30; w[GlossaryCalc.AR] = 133; w[GlossaryCalc.GEN] = 1; w[GlossaryCalc.ARMOR_SHRINE] = 1;
        int[] ids = GlossaryCalc.STAT_IDS;
        for (int i = 0; i < ids.length; i++) {
            int v = ids[i] == 39 ? 5 : ids[i] == 43 ? 19 : ids[i] == 41 ? 0 : ids[i] == 45 ? 5 : ids[i] == 105 ? 20 : ids[i] == 60 ? 6 : ids[i] == 62 ? 3 : 0;
            w[GlossaryCalc.STATS + i] = v;
        }
        return GlossaryCalc.Snap.parse(w, w.length);
    }

    private static GlossaryCalc.Monsters monsters() {
        GlossaryCalc.Monsters m = new GlossaryCalc.Monsters();
        int[] ar = new int[111], def = new int[111];
        for (int l = 0; l <= 110; l++) { ar[l] = 8 * l; def[l] = 6 * l; }
        m.ar[1] = new int[][]{ar, ar, ar}; m.def[1] = new int[][]{def, def, def};
        m.leech = new int[][]{{1, 1}, {2, 2}, {3, 3}};
        return m;
    }

    private static String joined(List<GlossaryLive.Line> l) { StringBuilder b = new StringBuilder(); for (GlossaryLive.Line x : l) b.append(x.text()).append(" | "); return b.toString(); }

    public static void main(String[] args) {
        GlossaryLive.Line loss = new GlossaryLive.Line("before ", "100", " after").compared(200, 100, false);
        check(loss.colour.equals(EffectDetail.WORSE), "lower defense is worse");
        check(loss.compared(20, 10, true).colour.equals(EffectDetail.BETTER), "lower enemy hit chance is better");
        check(loss.compared(10, 10, true).colour.equals(EffectDetail.GREY), "unchanged chance is grey");
        check(loss.slice(8, 12).value.equals("00") && loss.slice(8, 12).colour.equals(loss.colour), "wrapped value keeps comparison colour");

        check(GlossaryModel.clean("%+d%% verbesserte Verteidigung").equals("Verbesserte Verteidigung"), "percent stat -> Verbesserte Verteidigung");
        check(GlossaryModel.clean("%+d to Attack Rating").equals("Attack Rating") && GlossaryModel.clean("%+d zu Angriffswert").equals("Angriffswert"), "zu / to dropped");
        check(GlossaryModel.clean("Feuer-\nWiderstand").equals("Feuer-Widerstand") && GlossaryModel.clean("Herbei-\nrufung").equals("Herbeirufung"), "line breaks after a hyphen");
        check(GlossaryModel.clean("Alle Widerstände %+d").equals("Alle Widerstände") && GlossaryModel.clean("Feuerwiderstand %+d%%").equals("Feuerwiderstand"), "trailing placeholders");
        check(GlossaryModel.clean("Blockchance: %d%%").equals("Blockchance") && GlossaryModel.clean(null).isEmpty(), "colon and null");
        check(GlossaryModel.shown("%+d%% verbesserte Verteidigung").equals("+X% verbesserte Verteidigung") && GlossaryModel.shown("Schaden reduziert um %d").equals("Schaden reduziert um X"), "shown with X");
        GlossaryModel m = model();
        check(GlossaryModel.name(m.byId("enhdef"), NAMES, true).equals("Verbesserte Verteidigung") && GlossaryModel.name(m.byId("enhdef"), NAMES, false).equals("Enhanced Defense"), "game name first");
        check(GlossaryModel.name(m.byId("defense"), NAMES, true).equals("Verteidigung") && GlossaryModel.name(m.byId("defense"), null, false).equals("Defense"), "fallback without the game string");
        check(GlossaryModel.name(m.byId("resist"), NAMES, true).equals("Alle Widerstände"), "group name");

        check(m.filter(null, "", NAMES).size() == 6 && m.filter("def", null, NAMES).size() == 3 && m.filter("speed", "", NAMES).size() == 1, "categories");
        check(m.filter(null, "VERTEIDIGUNG", NAMES).size() == 2 && m.filter(null, "attack", NAMES).size() == 1 && m.filter(null, "enhanced def", NAMES).size() == 1, "search both languages, every word");
        check(m.filter(null, "fcr", NAMES).size() == 1 && m.filter(null, "zzz", NAMES).isEmpty() && m.filter("off", "verteidigung", NAMES).isEmpty(), "keywords, nothing, category and search");
        check(m.byStat(171) == m.byId("enhdef") && m.byStat(62) == m.byId("leech") && m.byStat(999) == null, "card link by stat");

        int[][] t = m.fcr[7];
        check(java.util.Arrays.equals(GlossaryModel.step(t, 0), new int[]{15, 9, 14}) && java.util.Arrays.equals(GlossaryModel.step(t, 20), new int[]{13, 30, 12}), "step and next");
        check(java.util.Arrays.equals(GlossaryModel.step(t, 30), new int[]{12, 48, 11}) && java.util.Arrays.equals(GlossaryModel.step(t, 200), new int[]{9, -1, -1}) && GlossaryModel.step(null, 5) == null, "exact step, top, no table");

        check(GlossaryCalc.hit(56, 68, 7, 7) == 45 && GlossaryCalc.hit(56, 34, 7, 7) == 62, "hit chance, same level (the plan's example)");
        check(GlossaryCalc.hit(10, 100000, 1, 99) == 5 && GlossaryCalc.hit(100000, 0, 99, 1) == 95, "5 and 95 limits");
        check(GlossaryCalc.hit(-1, 5, 1, 1) == -1 && GlossaryCalc.hit(0, 0, 1, 1) == -1 && GlossaryCalc.hit(5, 5, 0, 1) == -1, "unreasonable input");
        check(GlossaryCalc.hit(100000, 100000, 2000000, 2000000) == 50, "no overflow");
        check(GlossaryCalc.defense(29, 22, 100, 0) == 68 && GlossaryCalc.defense(29, 22, 0, 0) == 34 && GlossaryCalc.defense(29, 22, 0, 50) == 51, "the game's defense formula");
        check(GlossaryCalc.defense(-10, 0, 50, 0) == -5, "negative base as the game (def_ adds -q)");
        check(GlossaryCalc.monsterLevel(0) == 1 && GlossaryCalc.monsterLevel(120) == 110 && GlossaryCalc.monsterLevel(30) == 30, "monster level clamped");
        check(GlossaryCalc.tenths(20, true).equals("2") && GlossaryCalc.tenths(15, true).equals("1,5") && GlossaryCalc.tenths(15, false).equals("1.5"), "tenths");
        check(GlossaryCalc.seconds(15, true).equals("0,60") && GlossaryCalc.seconds(9, false).equals("0.36"), "frames to seconds");

        GlossaryCalc.Snap s = snap(7, 0, 1);
        check(s.valid && s.cls == 7 && s.level == 7 && s.def == 68 && s.stat(43) == 19 && s.stat(999) == 0, "snapshot parsed");
        int[] bad = new int[GlossaryCalc.INTS]; bad[GlossaryCalc.VER] = 2; bad[GlossaryCalc.VALID] = 1; bad[GlossaryCalc.LEVEL] = 5;
        check(!GlossaryCalc.Snap.parse(bad, bad.length).valid && !GlossaryCalc.Snap.parse(new int[3], 3).valid, "another version / too short = invalid");
        check(GlossaryCalc.defenseModelHolds(s), "the model matches the game's 68");

        GlossaryLive de = new GlossaryLive(m, true), en = new GlossaryLive(m, false);
        GlossaryCalc.Monsters mon = monsters();
        GlossaryLive.Game game = new GlossaryLive.Game() {
            public int penalty(int diff, boolean classic) { int[][] p = {{0, 0}, {-40, -20}, {-100, -50}}; return p[diff][classic ? 1 : 0]; }
            public String resistName(int stat, boolean de) { return stat == 39 ? "Feuer-Widerstand" : stat == 41 ? "Blitz-Widerstand" : stat == 43 ? "Kälte-Widerstand" : "Gift-Widerstand"; }
        };
        GlossaryLive.Context shrine = new GlossaryLive.Context(128, 171, 100);
        String a = joined(de.lines(m.byId("enhdef"), s, mon, game, shrine));
        check(a.contains("Verteidigung: 68") && a.contains("ohne 34") && a.contains("Stufe 7: 45 % statt 62 %"), "shrine on: 68, without 34, 45 % instead of 62 % — " + a);
        GlossaryCalc.Snap s0 = snap(7, 0, 1);
        s0.acpct = 0; s0.skillPct = 0; s0.def = 34; s0.armorShrine = false;
        String b = joined(de.lines(m.byId("enhdef"), s0, mon, game, null));
        check(b.contains("Verteidigung: 34") && b.contains("mit +100 % 68") && b.contains("dann 45 % statt 62 %"), "imagined shrine — " + b);
        String q = joined(de.lines(m.byId("enhdef"), s, mon, game, null));
        check(q.contains("ohne 34") && !q.contains("mit +100 %"), "shrine on, opened from the Quick Menu: without 34, never a second shrine — " + q);
                String other = joined(de.lines(m.byId("enhdef"), s, mon, game, new GlossaryLive.Context(37, 171, 60)));
        check(other.contains("ohne 34"), "another buff's card does not set the shrine bonus — " + other);
        GlossaryLive.Game none = new GlossaryLive.Game() { public int penalty(int d, boolean c) { return Integer.MIN_VALUE; } public String resistName(int st, boolean d) { return "R" + st; } };
        String unk = joined(de.lines(m.byId("resist"), snap(40, 2, 1), mon, none, null));
        check(unk.contains("R39: ? % (5, ?, 75 %)"), "unknown penalty: no effective value guessed — " + unk);
                GlossaryCalc.Snap sx = snap(7, 0, 1); sx.def = 70;
        check(joined(de.lines(m.byId("enhdef"), sx, mon, game, shrine)).contains("geht nicht"), "no prediction when the model differs from the game");
        GlossaryCalc.Snap sh = snap(40, 2, 1);
        String r = joined(de.lines(m.byId("resist"), sh, mon, game, null));
        check(r.contains("Schwierigkeit Hölle") && r.contains("Feuer-Widerstand: -95 % (5, -100, 75 %)") && r.contains("Kälte-Widerstand: -81 %"), "resistances in Hell — " + r);
        GlossaryCalc.Snap sc = snap(40, 1, 0);
        check(joined(de.lines(m.byId("resist"), sc, mon, game, null)).contains("Feuer-Widerstand: -15 % (5, -20, 75 %)"), "classic Nightmare penalty");
        String l = joined(en.lines(m.byId("leech"), snap(40, 2, 1), mon, game, null));
        check(l.contains("L 6% M 3%") && l.contains("Hell: 2% 1%"), "leech after the Hell divisor — " + l);
        String f = joined(de.lines(m.byId("fcr"), s, mon, game, null));
        check(f.contains("20 % → 13 (0,52 s)") && f.contains("30 % → 12 (noch 10 %)"), "cast rate step and next — " + f);
        String ar = joined(de.lines(m.byId("ar"), s, mon, game, null));
        check(ar.contains("AW 133") && ar.contains("triffst Stufe 7 zu 76 %"), "attack rating and own hit chance — " + ar);
        check(joined(de.lines(m.byId("defense"), new GlossaryCalc.Snap(), mon, game, null)).contains("kein Spiel"), "no snapshot: the note");
        check(joined(de.lines(m.byId("defense"), s, null, game, null)).equals("Verteidigung: 68 | "), "no pack monsters: no hit chance");
        GlossaryLive.Line one = de.line("def", 68);
        check(one.head.equals("Verteidigung: ") && one.value.equals("68") && one.tail.isEmpty(), "value in [ ] split for the blue colour");
        System.out.println("GlossaryTest: " + (checks - fails) + " / " + checks + " ok");
        if (fails > 0) System.exit(1);
    }
}
