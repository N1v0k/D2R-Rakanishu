package com.xiaoboluo.d2r;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;








public class EffectDetailTest {
    private static int checks, fails;
    private static void check(boolean ok, String what) { checks++; if (!ok) { fails++; System.err.println("FAIL " + what); } }
    private static void equal(Object want, Object actual, String what) { check(want == null ? actual == null : want.equals(actual), what + ": want=" + want + " actual=" + actual); }

    static final Map<Integer, String> STATS = new HashMap<>(), SKILLS = new HashMap<>();
    static final Map<String, String> WORDS = new HashMap<>(), OWN = new HashMap<>();
    static {
        STATS.put(39, "Feuerwiderstand %+d%%"); STATS.put(40, "%+d%% zu maximalem Feuerwiderstand"); STATS.put(27, "Mana regenerieren %d%%"); STATS.put(85, "%+d%% zu gesammelter Erfahrung");
        STATS.put(52, "%+d Magieschaden"); STATS.put(53, "%+d Magieschaden"); STATS.put(119, "%d%% Bonus zu Angriffswert"); STATS.put(28, "Ausdauerheilung plus %d%%");
        STATS.put(171, "%+d%% verbesserte Verteidigung"); STATS.put(36, "%+d%% Schaden Widerstand"); STATS.put(78, "Der Angreifer erleidet %d Schaden"); STATS.put(25, "%+d%% erhöhter Schaden"); STATS.put(999, "Etwas %+d");
        SKILLS.put(394, "Testzauber"); SKILLS.put(395, "Testblitz"); SKILLS.put(149, "Testbefehl");
        for (int i = 0; i < 6; i++) SKILLS.put(300 + i, "Fertigkeit " + i);
        WORDS.put("allskills", "%+d zu allen Fertigkeiten"); WORDS.put("life", "Leben"); WORDS.put("mana", "Mana"); WORDS.put("stamina", "Ausdauer");
        WORDS.put("res39", "Feuer-\nWiderstand"); WORDS.put("attr0", "Stärke"); STATS.put(0, "%+d zu Stärke"); STATS.put(3, "%+d zu Vitalität"); WORDS.put("range52", "Fügt %d-%d Magieschaden hinzu"); WORDS.put("range57", "Fügt %d-%d Giftschaden über %d Sek. hinzu");
        OWN.put("OnYou", "Auf dich"); OWN.put("OnEnemies", "Auf getroffene Gegner"); OWN.put("Party", "Auch auf Gruppe"); OWN.put("Cap", "Obergrenze");
        OWN.put("Effective", "davon %d %% wirksam"); OWN.put("CastAt", "%1$s wirkt mit Stufe %2$d"); OWN.put("Recast", "neu wirken: %d"); OWN.put("More", "+%d weitere");
        OWN.put("NextLevel", "Bis Stufe %1$d: noch %2$s Erfahrung"); OWN.put("Sheet", "Wert mit Bonus im Charakterbogen (blau)"); OWN.put("NoNumbers", "Keine Zahlen im Spiel hinterlegt");
        OWN.put("EnemyTree", "Werte auf Gegnern im Fertigkeitsbaum"); OWN.put("PerSec", "%1$s pro Sek. %2$s"); OWN.put("Stand", "%1$s pro Sek. im Stehen %2$s");
        OWN.put("Run", "%1$s pro Sek. beim Rennen %2$s"); OWN.put("FullIn", "%1$s voll in %2$s Sek."); OWN.put("ExpBonus", "Erfahrungsbonus"); OWN.put("ExpKill", "Erfahrung je Kill");
        OWN.put("PoisonRate", "%s Giftschaden pro Sek."); OWN.put("PoisonLeft", "Noch %1$s Giftschaden in %2$d Sek.");
        OWN.put("Heals", "Heilt: %s Leben"); OWN.put("SlowEnemies", "Verlangsamt Gegner um %d %%");
        OWN.put("Thorns", "Nahkampf-Angreifer erleiden %d %% des Schadens"); OWN.put("TakesMore", "Du erleidest +%d %% physischen Schaden");
    }
    static final int[][] PENALTY = {{0, 0}, {-40, -20}, {-100, -50}};
    static final EffectDetail.Texts T = new EffectDetail.Texts() {
        public String stat(int id, int value) { String f = STATS.get(id); return f == null ? null : EffectFormat.apply(f, value); }
        public String word(String key) { String w = WORDS.get(key); return w == null ? "" : w; }
        public String own(String key) { String w = OWN.get(key); return w == null ? "" : w; }
        public String skillName(int id) { return SKILLS.get(id); }
        public int penalty(int diff, boolean classic) { return diff >= 0 && diff < 3 ? PENALTY[diff][classic ? 1 : 0] : Integer.MIN_VALUE; }
        public long threshold(int cls, int level) { return cls == 7 && level == 8 ? 11234 : -1; }
        public String arrow() { return "›"; }
        public int[] opSources(int stat) { return stat == 11 ? new int[]{3, 162} : stat == 9 ? new int[]{1, 77} : new int[0]; }
        public int[] group(int stat) { return stat == 39 || stat == 41 || stat == 43 || stat == 45 ? new int[]{39, 41, 43, 45} : new int[0]; }
        public String groupText(int stat, int value) { return group(stat).length > 0 ? EffectFormat.apply("Alle Widerstände %+d", value) : null; }
        public boolean german() { return true; }
    };

    static EffectDetail.Entry entry(int state, int ticks, int lists, int... stats) {
        EffectDetail.Entry e = new EffectDetail.Entry();
        e.state = state; e.skill = -1; e.ticks = ticks; e.lists = lists;
        for (int i = 0; i + 1 < stats.length; i += 2) { e.id[e.n] = stats[i]; e.value[e.n] = stats[i + 1]; e.n++; }
        return e;
    }
    static EffectDetail.Info info(String kind) { EffectDetail.Info i = new EffectDetail.Info(); i.kind = kind; i.name = "X"; return i; }
    static EffectDetail.Player player(int diff, int classic) {
        EffectDetail.Player p = new EffectDetail.Player();
        p.flags = EffectFormat.PF_VALID | EffectFormat.PF_DIFF | EffectFormat.PF_CLASSIC | EffectFormat.PF_STAMINA; p.session = 7; p.unit = 42; p.cls = 7; p.age = 100;
        p.diff = diff; p.classic = classic; p.stand = 8; p.walk = 9; p.run = 8;
        return p;
    }
    static void set(EffectDetail.Player p, int stat, int v) { for (int i = 0; i < EffectFormat.PLAYER_STATS.length; i++) if (EffectFormat.PLAYER_STATS[i] == stat) p.stats[i] = v; }
    static List<String> texts(List<EffectDetail.Line> l) { List<String> o = new ArrayList<>(); for (EffectDetail.Line x : l) o.add(x.text); return o; }
    static List<String> run(EffectDetail.Entry e, EffectDetail.Info i, EffectDetail.Player p, EffectDetail.Skills s, int base) {
        Map<Integer, EffectDetail.Entry> all = new HashMap<>(); all.put(e.state, e);
        Map<Integer, EffectDetail.Info> infos = new HashMap<>(); infos.put(e.state, i);
        return texts(EffectDetail.lines(e, i, all, infos, p, s, base, T));
    }

    public static void main(String[] args) {
        equal("FontColorPartyGreen", EffectDetail.BETTER, "positive semantic colour");
        equal(EffectDetail.GREY, EffectDetail.statColour(0, 0, false), "zero effect");
        equal(EffectDetail.GRANTED, EffectDetail.statColour(127, 2, false), "additional skill levels");
        equal(EffectDetail.WORSE, EffectDetail.statColour(127, -1, false), "lost skill levels");
        equal(EffectDetail.GREY, EffectDetail.rangeColour(10, 20, 12, 18, false), "mixed interval is neutral");
        equal(EffectDetail.GREY, EffectDetail.rangeColour(10, 20, 10, 20, false), "equal interval is neutral");

        EffectDetail.Entry fire = entry(131, 3600, 1, 39, 75);
        EffectDetail.Player p = player(1, 0); set(p, 39, 150);
        List<String> l = run(fire, info("shrine"), p, null, 0);
        equal("[Feuerwiderstand +75%, Feuer-Widerstand 35 › 75 % · Obergrenze, davon 40 % wirksam]", l.toString(), "fire shrine in nightmare, total 150");
        set(p, 40, 10);
        equal("Feuer-Widerstand 35 › 85 % · Obergrenze", run(fire, info("shrine"), p, null, 0).get(1), "+10 maximum: cap 85");
        set(p, 40, 0); p.diff = 0;
        l = run(fire, info("shrine"), p, null, 0);
        equal("Feuer-Widerstand 75 › 75 % · Obergrenze", l.get(1), "normal: already at the cap");
        equal("davon 0 % wirksam", l.get(2), "normal: none of it takes effect");
        p.diff = 2;
        l = run(fire, info("shrine"), p, null, 0);
        equal("[Feuerwiderstand +75%, Feuer-Widerstand -25 › 50 %]", l.toString(), "hell: below the cap, no cap note");
        p.classic = 1;
        equal("Feuer-Widerstand 25 › 75 % · Obergrenze", run(fire, info("shrine"), p, null, 0).get(1), "classic hell: penalty -50");
        p.diff = 0; p.classic = 0; set(p, 39, 75);
        l = run(fire, info("shrine"), p, null, 0);
        equal("Feuer-Widerstand 0 › 75 % · Obergrenze", l.get(1), "exactly at the cap counts as the cap");
        equal("davon 75 % wirksam", l.get(2), "all of it takes effect");
        p.diff = 2; set(p, 39, -60);
        EffectDetail.Entry lower = entry(61, 200, 1, 39, -60);
        l = run(lower, info("debuff"), p, null, 0);
        equal("Feuer-Widerstand -100 › -100 %", l.get(1), "lower resist in hell: floor -100");

        STATS.put(41, "Blitzwiderstand %+d%%"); STATS.put(43, "Kältewiderstand %+d%%"); STATS.put(45, "Giftwiderstand %+d%%");
        WORDS.put("res41", "Blitz-\nWiderstand"); WORDS.put("res43", "Kälte-\nWiderstand"); WORDS.put("res45", "Gift-\nWiderstand");
        EffectDetail.Player q = player(0, 0); set(q, 39, 34); set(q, 41, -41); set(q, 43, -22); set(q, 45, 34);
        EffectDetail.Entry allRes = entry(61, 3000, 1, 39, -41, 41, -41, 43, -41, 45, -41);
        equal("[Alle Widerstände -41, Feuer-Widerstand 75 › 34 %, Blitz-Widerstand 0 › -41 %, Kälte-Widerstand 19 › -22 %, Gift-Widerstand 75 › 34 %]", run(allRes, info("debuff"), q, null, 0).toString(),
            "Lower Resist on me: the game's all resistances line, then each resistance before > after");
        EffectDetail.Entry uneven = entry(61, 3000, 1, 39, -41, 41, -41, 43, -40, 45, -41);
        equal("[Feuerwiderstand -41%, Feuer-Widerstand 75 › 34 %, Blitzwiderstand -41%, Blitz-Widerstand 0 › -41 %, Kältewiderstand -40%, Kälte-Widerstand 18 › -22 %, Giftwiderstand -41%, Gift-Widerstand 75 › 34 %]",
            run(uneven, info("debuff"), q, null, 0).toString(), "one value differs: each resistance on its own line");
        EffectDetail.Entry three = entry(61, 3000, 1, 39, -41, 41, -41, 43, -41);
        equal("Feuerwiderstand -41%", run(three, info("debuff"), q, null, 0).get(0), "a member missing: no group line");
        p.flags &= ~EffectFormat.PF_CLASSIC;
        equal(1, run(fire, info("shrine"), p, null, 0).size(), "classic flag unknown: no before > after");

        p = player(0, 0); set(p, 39, 125); set(p, 40, 5);
        EffectDetail.Entry aura = entry(3, -1, 1, 39, 50, 40, 5);
        equal("Feuer-Widerstand 50 › 80 % · Obergrenze", run(fire, info("shrine"), p, null, 0).get(1), "shrine with an aura: before keeps the aura");
        l = run(aura, info("aura"), p, null, 0);
        equal("Feuerwiderstand +50%", l.get(0), "own aura without an enemy section: no on-you heading, the lines stand on their own");
        equal("Feuer-Widerstand 75 › 80 % · Obergrenze", l.get(1), "aura with a shrine: before keeps the shrine, the max bonus goes too");
        check(l.indexOf("davon 5 % wirksam") == 2 && l.indexOf("+5% zu maximalem Feuerwiderstand") == 3 && l.size() == 4, "the max line once, no second resistance line: " + l);


        EffectDetail.Entry shrine = entry(134, 2400, 1);
        EffectDetail.Skills sk = new EffectDetail.Skills();
        sk.flags = EffectFormat.SF_VALID | EffectFormat.SF_SHRINE | EffectFormat.SF_PLUS2; sk.session = 7; sk.unit = 42; sk.age = 200; sk.count = 2;
        sk.id[0] = 395; sk.base[0] = 1; sk.bonus[0] = 2; sk.id[1] = 394; sk.base[1] = 6; sk.bonus[1] = 2;
        p = player(0, 0);
        l = run(shrine, info("shrine"), p, sk, 0);
        equal("[+2 zu allen Fertigkeiten, Testzauber 6 › 8, Testblitz 1 › 3]", l.toString(), "skill shrine: +2, skills by base level");
        EffectDetail.Line first = EffectDetail.lines(shrine, info("shrine"), map(shrine), infos(shrine, info("shrine")), p, sk, 0, T).get(1);
        check(first.accent == "Testzauber 6 › ".length() && "SkillTree.granted".equals(first.accentColour), "the level after is in the skill tree's bonus colour");
        sk.flags &= ~EffectFormat.SF_PLUS2;
        equal(0, run(shrine, info("shrine"), p, sk, 0).size(), "+2 instructions not matched: no lines");
        sk.flags |= EffectFormat.SF_PLUS2; sk.session = 8;
        equal(0, run(shrine, info("shrine"), p, sk, 0).size(), "another session: no lines");
        sk.session = 7; sk.unit = 43;
        equal(0, run(shrine, info("shrine"), p, sk, 0).size(), "another unit: no lines");
        sk.unit = 42; sk.age = 1000;
        equal(0, run(shrine, info("shrine"), p, sk, 0).size(), "a block 1 s old: no lines");
        sk.age = 200; p.age = 1500;
        equal(0, run(shrine, info("shrine"), p, sk, 0).size(), "a stale player block: no lines");
        p.age = 100; sk.flags &= ~EffectFormat.SF_SHRINE;
        equal(0, run(shrine, info("shrine"), p, sk, 0).size(), "the skill block has no shrine bit yet: no lines");
        sk.flags |= EffectFormat.SF_SHRINE; sk.count = 0;
        equal("[+2 zu allen Fertigkeiten]", run(shrine, info("shrine"), p, sk, 0).toString(), "no skills learned: only +2");
        sk.count = 7;
        for (int i = 0; i < 7; i++) { sk.id[i] = 300 + i % 6; sk.base[i] = i + 1; sk.bonus[i] = 2; }
        sk.id[6] = 394;
        l = run(shrine, info("shrine"), p, sk, 0);
        equal(8, l.size(), "7 skills: +2, 6 lines and a more line");
        equal("Testzauber 7 › 9", l.get(1), "highest base first");
        equal("Fertigkeit 5 6 › 8", l.get(2), "then the next base level");
        equal("+1 weitere", l.get(7), "the more line");

        EffectDetail.Entry bo = entry(32, 3000, 1, 77, 35); bo.skill = 149; bo.level = 5;
        sk.count = 1; sk.id[0] = 149; sk.base[0] = 5; sk.bonus[0] = 2;
        Map<Integer, EffectDetail.Entry> all = map(shrine); all.put(32, bo);
        Map<Integer, EffectDetail.Info> infos = infos(shrine, info("shrine")); infos.put(32, info("buff"));
        l = texts(EffectDetail.lines(shrine, info("shrine"), all, infos, p, sk, 0, T));
        equal("Testbefehl wirkt mit Stufe 5 · neu wirken: 7", l.get(l.size() - 1), "battle orders cast at 5, a recast would be 7");
        bo.level = 7;
        l = texts(EffectDetail.lines(shrine, info("shrine"), all, infos, p, sk, 0, T));
        check(!l.get(l.size() - 1).contains("wirkt mit"), "a buff cast at the current level gets no line");


        EffectDetail.Entry mana = entry(135, 2400, 1, 27, 400);
        p = player(0, 0); set(p, 27, 400); set(p, 8, 50 << 8); set(p, 9, 100 << 8);
        l = run(mana, info("shrine"), p, null, 100);
        equal("[Mana regenerieren 400%, Mana pro Sek. 9,8 › 48,8, Mana voll in 6 › 2 Sek.]", l.toString(), "mana shrine, mana half full");
        set(p, 8, 100 << 8);
        equal(2, run(mana, info("shrine"), p, null, 100).size(), "full mana: no time to full");
        equal(1, run(mana, info("shrine"), p, null, 0).size(), "no mana base (class table unread): only the game's line");
        set(p, 8, 50 << 8);
        equal(1, run(entry(135, 2400, 1, 27, 400, 77, 20), info("shrine"), p, null, 100).size(), "a state that also changes maximum mana (op 77): no before > after");
        p.age = 1000;
        equal(1, run(mana, info("shrine"), p, null, 100).size(), "a player block 1 s old: no before > after");
        p.age = 100;


        EffectDetail.Entry stam = entry(136, 4800, 1, 28, 1000);
        p = player(0, 0); set(p, 28, 1000); set(p, 11, 100 << 8);
        l = run(stam, info("shrine"), p, null, 0);
        equal("[Ausdauerheilung plus 1000%, Ausdauer pro Sek. im Stehen 9,8 › 107,4, Ausdauer pro Sek. beim Rennen 0 › 107,4]", l.toString(), "stamina shrine standing and running");
        equal(1, run(entry(136, 4800, 1, 28, 1000, 162, 50), info("shrine"), p, null, 0).size(), "a state that also raises maximum stamina (op 162): only the game's line");
        p.flags &= ~EffectFormat.PF_STAMINA;
        equal(1, run(stam, info("shrine"), p, null, 0).size(), "stamina code not matched: only the game's line");


        EffectDetail.Entry exp = entry(137, 3600, 1, 85, 50);
        p = player(0, 0); set(p, 85, 60); set(p, 12, 8); set(p, 13, 10000);
        l = run(exp, info("shrine"), p, null, 0);
        equal("[+50% zu gesammelter Erfahrung, Erfahrungsbonus +10 › +60 %, Erfahrung je Kill ×1,1 › ×1,6, Bis Stufe 9: noch 1.234 Erfahrung]", l.toString(), "experience shrine");
        set(p, 85, 50); set(p, 12, 99);
        equal("[+50% zu gesammelter Erfahrung, Erfahrungsbonus 0 › +50 %, Erfahrung je Kill ×1 › ×1,5]", run(exp, info("shrine"), p, null, 0).toString(), "level 99: no next level line");
        set(p, 13, -1); set(p, 12, 8);
        equal(3, run(exp, info("shrine"), p, null, 0).size(), "experience above 2^31 read unsigned: past the threshold, no line");


        EffectDetail.Info poison = info("debuff"); poison.poison = true;
        l = run(entry(2, 150, 1, 74, -42), poison, player(0, 0), null, 0);
        equal("[4,1 Giftschaden pro Sek., Noch 24,6 Giftschaden in 6 Sek.]", l.toString(), "poison, one list");
        equal("[4,1 Giftschaden pro Sek.]", run(entry(2, 150, 2, 74, -42), poison, player(0, 0), null, 0).toString(), "poison, two lists: only the rate");
        equal("[4,1 Giftschaden pro Sek.]", run(entry(2, -1, 1, 74, -42), poison, player(0, 0), null, 0).toString(), "poison without a time: only the rate");


        p = player(0, 0); set(p, 0, 60);
        equal("[+10 zu Stärke, Stärke 50 › 60]", run(entry(77, 100, 1, 0, 10), info("buff"), p, null, 0).toString(), "strength before > after with the sheet label");
        equal("[+5 zu Vitalität]", run(entry(77, 100, 1, 3, 5), info("buff"), p, null, 0).toString(), "no label for vitality in this pack: only the game's line");


        p = player(0, 0); set(p, 74, 512);
        equal("[Leben pro Sek. 25 › 50]", run(entry(40, -1, 1, 74, 256), info("aura"), p, null, 0).toString(), "life regeneration before > after");
        p.age = 5000;
        equal("[Leben pro Sek. +25]", run(entry(40, -1, 1, 74, 256), info("aura"), p, null, 0).toString(), "stale player block: only the state's own rate");


        EffectDetail.Info bane = info("buff");
        EffectDetail.Entry hex = entry(198, 2900, 1, 52, 4, 53, 8, 119, 20, 999, 3); hex.skill = 385; hex.level = 1;
        l = run(hex, bane, player(0, 0), null, 0);
        equal("[Fügt 4-8 Magieschaden hinzu, 20% Bonus zu Angriffswert, Etwas +3]", l.toString(), "magic damage once as a range, an unknown stat as game text");
        EffectDetail.Info armor = info("shrine"); armor.sheet = true;
        equal("[+100% verbesserte Verteidigung, Wert mit Bonus im Charakterbogen (blau)]", run(entry(128, 2400, 1, 171, 100), armor, player(0, 0), null, 0).toString(), "armor shrine: the sheet note");
        EffectDetail.Info dim = info("debuff"); dim.curse = true; dim.shortText = "Verflucht eine Gruppe";
        equal("[Verflucht eine Gruppe, Keine Zahlen im Spiel hinterlegt]", run(entry(23, 200, 1), dim, player(0, 0), null, 0).toString(), "a curse without numbers");
        EffectDetail.Info bo2 = info("buff"); bo2.party = true;
        equal("[Auch auf Gruppe]", run(entry(32, 100, 1), bo2, player(0, 0), null, 0).toString(), "a party buff without stats, no enemy section: no heading");


        EffectDetail.Info might = info("aura"); might.party = true; might.ownSt[0] = 25; might.ownCa[0] = "ln34"; might.ownN = 1; might.ownSkill = 98; might.par[2] = 40; might.par[3] = 10;
        EffectDetail.Entry m = entry(33, 50, 1); m.skill = 98; m.level = 1;
        equal("[+40% erhöhter Schaden, Auch auf Gruppe]", run(m, might, player(0, 0), null, 0).toString(), "Might level 1: par3 40");
        m.level = 10;
        equal("[+130% erhöhter Schaden, Auch auf Gruppe]", run(m, might, player(0, 0), null, 0).toString(), "Might level 10: 40 + 9 x 10");
        equal("[+55% erhöhter Schaden, Auch auf Gruppe]", run(entry(33, 50, 1, 25, 55), might, player(0, 0), null, 0).toString(), "the value the state carries wins over the formula");

        EffectDetail.Info prayer = info("aura"); prayer.party = true; prayer.ownSt[0] = 6; prayer.ownCa[0] = "edns"; prayer.ownN = 1; prayer.ownSkill = 99; prayer.emin = 2;
        int[] plev = {1, 1, 2, 2, 3}; for (int i = 0; i < plev.length; i++) prayer.eminLev[i] = plev[i];
        EffectDetail.Entry q2 = entry(34, 50, 1); q2.skill = 99; q2.level = 3;
        equal("[Heilt: 4 Leben, Auch auf Gruppe]", run(q2, prayer, player(0, 0), null, 0).toString(), "Prayer level 3: EMin 2 + 2 x 1, no rate claimed");
        q2.level = 18;
        equal("[Heilt: 21 Leben, Auch auf Gruppe]", run(q2, prayer, player(0, 0), null, 0).toString(), "Prayer level 18: brackets 7 / 8 / rest of the third step");

        EffectDetail.Info thorns = info("aura"); thorns.party = true; thorns.ownSt[0] = 78; thorns.ownCa[0] = "edmn"; thorns.ownSt[1] = 131; thorns.ownCa[1] = "ln34"; thorns.ownN = 2; thorns.ownSkill = 103;
        thorns.par[2] = 250; thorns.par[3] = 40; thorns.emin = 4;
        int[] tlev = {4, 8, 12, 16, 20}; for (int i = 0; i < tlev.length; i++) thorns.eminLev[i] = tlev[i];
        EffectDetail.Entry th = entry(36, 50, 1); th.skill = 103; th.level = 1;
        equal("[Der Angreifer erleidet 4 Schaden, Nahkampf-Angreifer erleiden 250 % des Schadens, Auch auf Gruppe]", run(th, thorns, player(0, 0), null, 0).toString(), "Thorns level 1: EMin and the returned percent (own text, the game has none)");
        th.level = 10;
        equal("[Der Angreifer erleidet 48 Schaden, Nahkampf-Angreifer erleiden 610 % des Schadens, Auch auf Gruppe]", run(th, thorns, player(0, 0), null, 0).toString(), "Thorns level 10: 4 + 7 x 4 + 2 x 8, 250 + 9 x 40");
        EffectDetail.Entry th2 = entry(36, 50, 1, 131, 650); th2.skill = 103; th2.level = 10;
        equal("[Der Angreifer erleidet 48 Schaden, Nahkampf-Angreifer erleiden 650 % des Schadens, Auch auf Gruppe]", run(th2, thorns, player(0, 0), null, 0).toString(),
            "Thorns: the percent the state carries wins and still gets our line");

        EffectDetail.Info defiance = info("aura"); defiance.party = true; defiance.ownSt[0] = 171; defiance.ownCa[0] = "ln34"; defiance.ownN = 1; defiance.ownSkill = 104; defiance.par[2] = 70; defiance.par[3] = 10;
        EffectDetail.Entry df = entry(37, 50, 1); df.skill = 104; df.level = 1;
        equal("[+70% verbesserte Verteidigung, Auch auf Gruppe]", run(df, defiance, player(0, 0), null, 0).toString(), "Defiance level 1");
        df.level = 2;
        equal("[+80% verbesserte Verteidigung, Auch auf Gruppe]", run(df, defiance, player(0, 0), null, 0).toString(), "Defiance level 2: 70 + 10");

        EffectDetail.Info aim = info("aura"); aim.party = true; aim.ownSt[0] = 119; aim.ownCa[0] = "ln34"; aim.ownN = 1; aim.ownSkill = 108; aim.par[2] = 75; aim.par[3] = 15;
        EffectDetail.Entry ba = entry(40, 50, 1); ba.skill = 108; ba.level = 1;
        equal("[75% Bonus zu Angriffswert, Auch auf Gruppe]", run(ba, aim, player(0, 0), null, 0).toString(), "Blessed Aim level 1");
        ba.level = 3;
        equal("[105% Bonus zu Angriffswert, Auch auf Gruppe]", run(ba, aim, player(0, 0), null, 0).toString(), "Blessed Aim level 3: 75 + 2 x 15");

        EffectDetail.Info freeze = info("aura"); freeze.slow = "-dm34"; freeze.ownSkill = 114; freeze.par[2] = 25; freeze.par[3] = 60;
        EffectDetail.Entry hf = entry(43, 50, 1); hf.skill = 114; hf.level = 1;
        equal("[Verlangsamt Gegner um 30 %]", run(hf, freeze, player(0, 0), null, 0).toString(), "Holy Freeze level 1: dm34, one self-describing line, no headings");
        hf.level = 10;
        equal("[Verlangsamt Gegner um 48 %]", run(hf, freeze, player(0, 0), null, 0).toString(), "Holy Freeze level 10: 25 + (1100 / 16 = 68) x 35 / 100");
        hf.level = 8;
        equal("[Verlangsamt Gegner um 46 %]", run(hf, freeze, player(0, 0), null, 0).toString(), "Holy Freeze level 8: the simulation's truncation, not 47");
        hf.skill = 115;
        equal("[]", run(hf, freeze, player(0, 0), null, 0).toString(), "a state from another skill gets no numbers from this skill's formulas");

        EffectDetail.Info amp = info("debuff"); amp.curse = true; amp.ownSt[0] = 36; amp.ownCa[0] = "-par5"; amp.ownN = 1; amp.ownSkill = 66; amp.par[4] = 100;
        EffectDetail.Entry am = entry(9, 375, 1); am.skill = 66; am.level = 5;
        equal("[Du erleidest +100 % physischen Schaden]", run(am, amp, player(0, 0), null, 0).toString(), "Amplify Damage: the extra damage taken at the state's level, no note about missing numbers");


        EffectDetail.EnemyLine d = new EffectDetail.EnemyLine(); d.f = 36; d.a = "Verhexungsdauer: %d Sekunde"; d.b = "Verhexungsdauer: %d Sekunden"; d.ca = "clc1"; d.cb = "25";
        EffectDetail.EnemyLine def = new EffectDetail.EnemyLine(); def.f = 74; def.a = "%+d%% zu Verteidigung des Gegners"; def.ca = "ast1"; def.cb = "";
        bane.enemy.add(d); bane.enemy.add(def);
        bane.par[4] = 30; bane.par[5] = 1; bane.par[8] = 5; bane.calc[0] = "150+(((skill('Blast'.blvl)) *par9))"; bane.ast[0] = "min(ln56,50)*-1";
        Map<String, Integer> refs = new HashMap<>(); refs.put("Blast", 384); bane.refs = refs;
        java.util.Set<String> keepV = new java.util.HashSet<>(EffectCalc.VERIFIED), keepB = new java.util.HashSet<>(EffectCalc.VERIFIED_BASE);
        EffectCalc.VERIFIED.clear(); EffectCalc.VERIFIED_BASE.clear();
        l = run(hex, bane, player(0, 0), sk, 0);
        equal("[Auf getroffene Gegner, Werte auf Gegnern im Fertigkeitsbaum]", l.subList(4, l.size()).toString(), "enemy lines not checked: heading and note, no numbers");
        EffectCalc.VERIFIED.add("385:clc1"); EffectCalc.VERIFIED.add("385:ast1");
        sk.flags = EffectFormat.SF_VALID;
        l = run(hex, bane, player(0, 0), sk, 0);
        equal("[Auf getroffene Gegner · Verhexungsdauer: 6 Sekunden, -30% zu Verteidigung des Gegners]", l.subList(4, l.size()).toString(), "checked enemy lines: duration in the heading, the defense line");
        hex.level = 3;
        l = run(hex, bane, player(0, 0), sk, 0);
        equal("-32% zu Verteidigung des Gegners", l.get(l.size() - 1), "level 3");
        l = run(hex, bane, player(0, 0), null, 0);
        equal("[Auf getroffene Gegner, -32% zu Verteidigung des Gegners, Werte auf Gegnern im Fertigkeitsbaum]", l.subList(4, l.size()).toString(), "no skill block: the duration needing a skill level drops, the rest stays");
        bane.bad = true;
        l = run(hex, bane, player(0, 0), sk, 0);
        equal("[Auf getroffene Gegner, Werte auf Gegnern im Fertigkeitsbaum]", l.subList(4, l.size()).toString(), "a formula the pack marked as not computable: no numbers");
        EffectCalc.VERIFIED.clear(); EffectCalc.VERIFIED.addAll(keepV); EffectCalc.VERIFIED_BASE.addAll(keepB);

        bane.bad = false; hex.level = 5; sk.count = 0; sk.flags = EffectFormat.SF_VALID;
        l = run(hex, bane, player(0, 0), sk, 0);
        equal("[Auf getroffene Gegner · Verhexungsdauer: 6 Sekunden, -34% zu Verteidigung des Gegners]", l.subList(4, l.size()).toString(), "shipped: level 5 as the skill tree (-34 %), duration with Blast at 0");
        sk.count = 1; sk.id[0] = 384; sk.base[0] = 2;
        l = run(hex, bane, player(0, 0), sk, 0);
        equal("[Auf getroffene Gegner, -34% zu Verteidigung des Gegners, Werte auf Gegnern im Fertigkeitsbaum]", l.subList(4, l.size()).toString(), "Blast learned: the synergy term was never checked, no duration");
        sk.count = 0; sk.session = 9;
        l = run(hex, bane, player(0, 0), sk, 0);
        equal("[Auf getroffene Gegner, -34% zu Verteidigung des Gegners, Werte auf Gegnern im Fertigkeitsbaum]", l.subList(4, l.size()).toString(), "skill block of another session: base levels not trusted");
        sk.session = 7; sk.dropped = 1;
        equal("Werte auf Gegnern im Fertigkeitsbaum", run(hex, bane, player(0, 0), sk, 0).get(run(hex, bane, player(0, 0), sk, 0).size() - 1), "a cut skill block: no base level from it");
        sk.dropped = 0;


        WORDS.put("defense", "Verteidigung"); WORDS.put("attackrating", "%s\nAngriffswert"); WORDS.put("damage", "Schaden"); WORDS.put("attack", "Angreifen");
        STATS.put(19, "%+d zu Angriffswert"); STATS.put(25, "%+d%% erhöhter Schaden");
        EffectDetail.Combat cb = new EffectDetail.Combat();
        cb.flags = EffectFormat.CF_VALID | EffectFormat.CF_DEF; cb.session = 7; cb.unit = 42; cb.age = 100; cb.state = 128; cb.v171 = 100;
        int[] vals = {34, 68, 133, 533, 2, 12, 4, 24}; System.arraycopy(vals, 0, cb.values, 0, 8);
        EffectDetail.Entry ae = entry(128, 2400, 1, 171, 100);
        equal("[+100% verbesserte Verteidigung, Verteidigung 34 › 68]", runC(ae, armor, player(0, 0), cb).toString(), "armor shrine with the combat block: before > after, no sheet note");
        cb.age = 1500;
        equal("[+100% verbesserte Verteidigung, Wert mit Bonus im Charakterbogen (blau)]", runC(ae, armor, player(0, 0), cb).toString(), "stale combat block: old card");
        cb.age = 100; cb.v171 = 50;
        equal("[+100% verbesserte Verteidigung, Wert mit Bonus im Charakterbogen (blau)]", runC(ae, armor, player(0, 0), cb).toString(), "block computed with other shrine values: old card");
        cb.v171 = 100; cb.unit = 43;
        equal("[+100% verbesserte Verteidigung, Wert mit Bonus im Charakterbogen (blau)]", runC(ae, armor, player(0, 0), cb).toString(), "another unit: old card");
        cb.unit = 42; cb.flags = EffectFormat.CF_VALID;
        equal("[+100% verbesserte Verteidigung, Wert mit Bonus im Charakterbogen (blau)]", runC(ae, armor, player(0, 0), cb).toString(), "defense not allowed (R0): old card");
        EffectDetail.Info combat = info("shrine"); combat.sheet = true;
        EffectDetail.Entry ce = entry(129, 2400, 1, 19, 276, 25, 200);
        cb.state = 129; cb.v171 = 0; cb.v19 = 276; cb.v25 = 200; cb.flags = EffectFormat.CF_VALID | EffectFormat.CF_AR | EffectFormat.CF_DMG;
        equal("[+276 zu Angriffswert, +200% erhöhter Schaden, Angriffswert (Angreifen) 133 › 533, Schaden (Angreifen) 2-12 › 4-24]", runC(ce, combat, player(0, 0), cb).toString(), "combat shrine: attack rating and damage");
        cb.flags = EffectFormat.CF_VALID | EffectFormat.CF_AR;
        equal("[+276 zu Angriffswert, +200% erhöhter Schaden, Angriffswert (Angreifen) 133 › 533, Wert mit Bonus im Charakterbogen (blau)]", runC(ce, combat, player(0, 0), cb).toString(), "damage not allowed: its line missing, the sheet note stays");
        equal("Angriffswert", EffectDetail.plainLine("%s\nAngriffswert"), "the attack rating label without the %s line");


        EffectDetail.Combat ca = new EffectDetail.Combat(), cc = new EffectDetail.Combat();
        ca.session = cc.session = 7; ca.unit = cc.unit = 42; ca.age = cc.age = 100;
        ca.flags = EffectFormat.CF_VALID | EffectFormat.CF_DEF; ca.state = 128; ca.v171 = 100; System.arraycopy(vals, 0, ca.values, 0, 8);
        cc.flags = EffectFormat.CF_VALID | EffectFormat.CF_AR | EffectFormat.CF_DMG; cc.state = 129; cc.v19 = 276; cc.v25 = 200; System.arraycopy(vals, 0, cc.values, 0, 8);
        equal("[+100% verbesserte Verteidigung, Verteidigung 34 › 68]", runC(ae, armor, player(0, 0), ca).toString(), "stacked: the armor card reads its own slot");
        equal("[+276 zu Angriffswert, +200% erhöhter Schaden, Angriffswert (Angreifen) 133 › 533, Schaden (Angreifen) 2-12 › 4-24]", runC(ce, combat, player(0, 0), cc).toString(), "stacked: the combat card reads its own slot");
        equal("[+100% verbesserte Verteidigung, Wert mit Bonus im Charakterbogen (blau)]", runC(ae, armor, player(0, 0), cc).toString(), "stacked: the combat slot never serves the armor card");
        STATS.put(96, "+%d%% schneller laufen/schreiten");
        EffectDetail.Entry st = entry(136, 2400, 1, 162, 100, 10, 100 << 8, 28, 1000, 96, 50);
        EffectDetail.Info stamina = info("shrine");
        String sl = runC(st, stamina, player(0, 0), null).toString();
        equal(true, sl.contains("+50% schneller laufen/schreiten"), "stamina shrine card shows the +50 % run/walk line: " + sl);


        EffectDetail.Entry pe = entry(2, 30, 1, 74, -100); pe.shown = 20;
        equal("[9,8 Giftschaden pro Sek., Noch 7,8 Giftschaden in 1 Sek.]", run(pe, poison, player(0, 0), null, 0).toString(), "the poison line uses the shown ticks (20 → 1 s, 7,8)");
        pe.shown = 0;
        equal("[9,8 Giftschaden pro Sek.]", run(pe, poison, player(0, 0), null, 0).toString(), "nothing left shown: no remaining line");


        equal("Fügt 4-8 Magieschaden hinzu", EffectDetail.format2("Fügt %d-%d Magieschaden hinzu", 4, 8), "range of two");
        equal(null, EffectDetail.format2("Fügt %d Schaden", 4, 8), "a format with one number is no range");
        equal("Feuer-Widerstand", EffectDetail.resistLabel("Feuer-\nWiderstand"), "the sheet label without its line break (German)");
        equal("Fire Resistance", EffectDetail.resistLabel("Fire\nResistance"), "the sheet label without its line break (English)");
        equal(95, EffectDetail.resCap(40), "the hard cap 95");
        equal(15, EffectDetail.secondsToFull(0, 100 << 8, 70), "ticks rounded up, then seconds");
        equal(-1, EffectDetail.secondsToFull(0, 100, 0), "no regeneration: no time");
        equal(1100, EffectDetail.staminaPerTick(100 << 8, 8, 1000), "stamina per tick, the game's integer arithmetic");
        equal(0, EffectDetail.staminaPerTick(100 << 8, EffectDetail.runShift(999, 8), 999), "running: no regeneration up to a bonus of 999");
        equal("×1,45", EffectDetail.factor(145, true), "factor with two decimals");
        equal("1,234,567", EffectDetail.grouped(1234567, false), "grouping in English");

        System.out.println("EffectDetailTest: " + checks + " checks, " + fails + " failed");
        if (fails > 0) System.exit(1);
    }

    static List<String> runC(EffectDetail.Entry e, EffectDetail.Info i, EffectDetail.Player p, EffectDetail.Combat c) {
        Map<Integer, EffectDetail.Entry> all = new HashMap<>(); all.put(e.state, e);
        Map<Integer, EffectDetail.Info> infos = new HashMap<>(); infos.put(e.state, i);
        return texts(EffectDetail.lines(e, i, all, infos, p, null, c, 0, T));
    }
    static Map<Integer, EffectDetail.Entry> map(EffectDetail.Entry e) { Map<Integer, EffectDetail.Entry> m = new HashMap<>(); m.put(e.state, e); return m; }
    static Map<Integer, EffectDetail.Info> infos(EffectDetail.Entry e, EffectDetail.Info i) { Map<Integer, EffectDetail.Info> m = new HashMap<>(); m.put(e.state, i); return m; }
}
