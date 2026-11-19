package com.xiaoboluo.d2r;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Set;

















final class EffectDetail {

    static final int SUB = 1, VALUE = 2, NOTE = 3;

    static final String BETTER = "FontColorPartyGreen", WORSE = "FontColorRed", GRANTED = "SkillTree.granted", WHITE = "FontColorWhite", GREY = "FontColorLightGray";
    static final int SHOWN_SKILLS = 6, FRESH_MS = 1000, SHRINE_PLUS = 2, SKILL_SHRINE = 134;
    static final int ST_LIFE = 6, ST_MAXLIFE = 7, ST_MANA = 8, ST_MAXMANA = 9, ST_STAMINA = 10, ST_MAXSTAMINA = 11, ST_LEVEL = 12, ST_EXP = 13, ST_MANA_RECOVERY = 26,
        ST_MANA_BONUS = 27, ST_STAMINA_BONUS = 28, ST_LIFE_REGEN = 74, ST_EXP_BONUS = 85, ST_DMG_RESIST = 36, ST_THORNS_PCT = 131;
    static final int[] RESISTS = {39, 41, 43, 45}, MAX_RESISTS = {40, 42, 44, 46};
    static final int ATTRIBUTES = 4;
    static final int RES_FLOOR = -100, RES_CAP = 75, RES_HARD_CAP = 95, MAX_LEVEL = 99, RUN_BONUS = 999;

    static final int[][] PAIRS = {{21, 22}, {48, 49}, {50, 51}, {52, 53}, {54, 55}, {57, 58}};
    static final int POISON_LENGTH = 59;



    static final class Line {
        final String text, colour, accentColour; final int kind, accent;
        Line(String text, int kind, String colour) { this(text, kind, colour, -1, null); }
        Line(String text, int kind, String colour, int accent, String accentColour) { this.text = text; this.kind = kind; this.colour = colour; this.accent = accent; this.accentColour = accentColour; }
        @Override public String toString() { return text; }
    }


    static final class Player {
        int flags, session, unit, cls, age = Integer.MAX_VALUE, diff, classic, mode, stand, walk, run;
        final int[] stats = new int[EffectFormat.PLAYER_STATS.length];
        boolean valid() { return (flags & EffectFormat.PF_VALID) != 0; }
        int stat(int id) { for (int i = 0; i < EffectFormat.PLAYER_STATS.length; i++) if (EffectFormat.PLAYER_STATS[i] == id) return stats[i]; return 0; }
        boolean has(int id) { for (int s : EffectFormat.PLAYER_STATS) if (s == id) return true; return false; }
    }


    static final class Skills {
        int flags, session, unit, cls, age = Integer.MAX_VALUE, count, dropped;
        final int[] id = new int[EffectFormat.S_MAX], base = new int[EffectFormat.S_MAX], bonus = new int[EffectFormat.S_MAX];
        int effective(int skill) { for (int i = 0; i < count; i++) if (id[i] == skill) return base[i] + bonus[i]; return -1; }
        int baseOf(int skill) { for (int i = 0; i < count; i++) if (id[i] == skill) return base[i]; return 0; }
    }




    static final class Combat {
        int flags, session, unit, age = Integer.MAX_VALUE, state, slot, v171, v19, v25;
        final int[] values = new int[8];
    }



    static final class Entry {
        int state, skill, level, ticks, lists, n, shown = -1;
        int[] id = new int[EffectFormat.STATS], layer = new int[EffectFormat.STATS], value = new int[EffectFormat.STATS];
        int own(int stat) { int v = 0; for (int k = 0; k < n; k++) if (id[k] == stat && layer[k] == 0) v += value[k]; return v; }
        boolean has(int stat) { for (int k = 0; k < n; k++) if (id[k] == stat && layer[k] == 0) return true; return false; }
    }


    static final class EnemyLine { int f; String a, b, ca, cb; }


    static final class Info {
        String name, kind = "", shortText;
        boolean curse, poison, party, sheet, bad;
        final List<EnemyLine> enemy = new ArrayList<>();
        int[] par = new int[12]; String[] calc = new String[4], ast = new String[6]; String len = "";
        Map<String, Integer> refs = Collections.emptyMap();
        final int[] ownSt = new int[6]; final String[] ownCa = new String[6]; int ownN;
        String slow = "";
        int emin = EffectCalc.NONE; final int[] eminLev = new int[5];
        int ownSkill = -1;
    }



    interface Texts {
        String stat(int id, int value);
        String word(String key);
        String own(String key);
        String skillName(int id);
        int penalty(int diff, boolean classic);
        long threshold(int cls, int level);
        String arrow();
        int[] opSources(int stat);
        int[] group(int stat);
        String groupText(int stat, int value);
        boolean german();
    }

    private EffectDetail() {}


    static String num(double v, boolean de) { return EffectFormat.number(v, de); }
    static String grouped(long v, boolean de) {
        String s = String.format(Locale.ROOT, "%,d", v);
        return de ? s.replace(',', '.') : s;
    }
    static String factor(int percent, boolean de) {
        String s = String.format(Locale.ROOT, "%.2f", percent / 100.0);
        while (s.contains(".") && (s.endsWith("0") || s.endsWith("."))) s = s.substring(0, s.length() - 1);
        return "×" + (de ? s.replace('.', ',') : s);
    }
    static String signedInt(int v) { return (v > 0 ? "+" : "") + v; }




    static int resCap(int maxBonus) { return Math.min(RES_HARD_CAP, RES_CAP + maxBonus); }
    static int resShown(int total, int maxBonus, int penalty) { return Math.max(RES_FLOOR, Math.min(resCap(maxBonus), total + penalty)); }



    static int staminaPerTick(int max, int shift, int bonus) {
        if (shift < 0 || max <= 0) return 0;
        int s = max >> shift;
        return bonus != 0 ? s + s * bonus / 100 : s;
    }


    static int runShift(int bonus, int run) { return bonus > RUN_BONUS ? run : -1; }



    static int secondsToFull(int cur, int max, int perTick) {
        if (perTick <= 0) return -1;
        long ticks = ((long) Math.max(0, max - cur) + perTick - 1) / perTick;
        return EffectFormat.seconds((int) Math.min(Integer.MAX_VALUE, ticks));
    }


    static boolean fresh(Player p) { return p != null && p.valid() && p.age < FRESH_MS; }



    static boolean touches(Entry e, Texts t, int... stats) {
        for (int s : stats) {
            for (int k = 0; k < e.n; k++) if (e.layer[k] == 0 && e.id[k] == s) return true;
            for (int src : t.opSources(s)) for (int k = 0; k < e.n; k++) if (e.id[k] == src) return true;
        }
        return false;
    }


    static boolean touchesOp(Entry e, Texts t, int stat) {
        for (int src : t.opSources(stat)) for (int k = 0; k < e.n; k++) if (e.id[k] == src) return true;
        return false;
    }



    static boolean together(Player p, Skills s, boolean shrineListed) {
        return fresh(p) && s != null && (s.flags & EffectFormat.SF_VALID) != 0 && p.session != 0 && p.session == s.session && p.unit == s.unit
            && p.age < FRESH_MS && s.age < FRESH_MS && ((s.flags & EffectFormat.SF_SHRINE) != 0) == shrineListed;
    }




    static List<Line> lines(Entry e, Info info, Map<Integer, Entry> all, Map<Integer, Info> infos, Player p, Skills sk, int manaBase, Texts t) {
        return lines(e, info, all, infos, p, sk, null, manaBase, t);
    }
    static List<Line> lines(Entry e, Info info, Map<Integer, Entry> all, Map<Integer, Info> infos, Player p, Skills sk, Combat cb, int manaBase, Texts t) {
        List<Line> out = new ArrayList<>();
        boolean de = t.german(), bad = "debuff".equals(info.kind), own = "buff".equals(info.kind) || "aura".equals(info.kind);
        boolean ok = fresh(p);
        if (e.state == SKILL_SHRINE && "shrine".equals(info.kind)) skillShrine(out, all, infos, p, sk, t);




        int[][] extra = ownValues(e, info, p, sk);
        Integer slow = slowValue(e, info);
        boolean enemies = !info.enemy.isEmpty();
        if (own && enemies && (e.n > 0 || info.party || extra.length > 0)) out.add(new Line(t.own("OnYou"), SUB, null));
        Set<Integer> done = new HashSet<>();
        Set<String> groups = new HashSet<>();
        boolean mana = false, sheet = false;
        for (int k = 0; k < e.n; k++) {
            int id = e.id[k], v = e.value[k];
            if (e.layer[k] != 0 || done.contains(id)) continue;
            done.add(id);
            if (pair(out, e, id, done, bad, t)) continue;
            if (id == ST_LIFE_REGEN) { lifeRegen(out, e, info, p, v, bad, t); continue; }
            if (id == ST_MANA_RECOVERY || id == ST_MANA_BONUS) {
                if (id == ST_MANA_BONUS) statLine(out, id, v, bad, t);
                if (!mana) { mana = true; manaLines(out, e, p, manaBase, bad, t); }
                continue;
            }
            String g = whole(e, id, t);
            if (g == null) statLine(out, id, v, bad, t);
            else if (groups.add(g)) out.add(new Line(g, VALUE, statColour(id, v, bad)));
            if (!ok) continue;
            if (id == ST_STAMINA_BONUS) stamina(out, e, p, v, bad, t);
            for (int r = 0; r < RESISTS.length; r++) if (id == RESISTS[r] || id == MAX_RESISTS[r] && !e.has(RESISTS[r])) resist(out, e, p, r, t);
            if (id == ST_EXP_BONUS && !touchesOp(e, t, ST_EXP_BONUS)) experience(out, p, v, t);
            if (id >= 0 && id < ATTRIBUTES && !touchesOp(e, t, id)) attribute(out, p, id, v, t);
        }
        for (int[] x : extra) ownLine(out, x[0], x[1], bad, t);
        boolean sheetDone = "shrine".equals(info.kind) && shrineCombat(out, e, p, cb, t);
        if (info.sheet && !bad && ok && !sheetDone) sheet = true;
        if (sheet) out.add(new Line(t.own("Sheet"), NOTE, GREY));
        if (own && info.party) out.add(new Line(t.own("Party"), NOTE, GREY));
        if (own && (enemies || slow != null)) enemy(out, e, info, p, sk, t, slow);
        if (bad && info.curse && e.n == 0 && extra.length == 0) {
            if (info.shortText != null && !info.shortText.isEmpty()) out.add(new Line(info.shortText, VALUE, WHITE));
            out.add(new Line(t.own("NoNumbers"), NOTE, GREY));
        }
        return out;
    }



    static String whole(Entry e, int id, Texts t) {
        int[] g = t.group(id);
        if (g.length < 2) return null;
        int v = e.own(id);
        for (int m : g) if (!e.has(m) || e.own(m) != v) return null;
        return t.groupText(id, v);
    }

    private static void statLine(List<Line> out, int id, int v, boolean bad, Texts t) {
        String s = t.stat(id, v);
        if (s != null && !s.isEmpty()) out.add(new Line(s, VALUE, statColour(id, v, bad)));
    }


    private static boolean pair(List<Line> out, Entry e, int id, Set<Integer> done, boolean bad, Texts t) {
        for (int[] pr : PAIRS) {
            if (pr[0] != id || !e.has(pr[1])) continue;
            String f = t.word("range" + pr[0]);
            int lo = e.own(pr[0]), hi = e.own(pr[1]);
            String s;
            if (pr[0] == 57) {
                int len = e.own(POISON_LENGTH);
                if (len <= 0) return false;
                s = format3(f, (int) ((long) lo * len / 256), (int) ((long) hi * len / 256), EffectFormat.seconds(len));
                done.add(POISON_LENGTH);
            } else s = format2(f, lo, hi);
            if (s == null) return false;
            done.add(pr[1]);
            if (lo < 0 && hi > 0 || lo > 0 && hi < 0) s += " · " + t.own("Mixed");
            out.add(new Line(s, VALUE, rangeColour(0, 0, lo, hi, bad)));
            return true;
        }
        return false;
    }


    static String format2(String f, int a, int b) { return f != null && count(f) == 2 ? f.replaceFirst("%d", Integer.toString(a)).replaceFirst("%d", Integer.toString(b)).replace("%%", "%") : null; }
    static String format3(String f, int a, int b, int c) { return f != null && count(f) == 3 ? f.replaceFirst("%d", Integer.toString(a)).replaceFirst("%d", Integer.toString(b)).replaceFirst("%d", Integer.toString(c)).replace("%%", "%") : null; }
    private static int count(String f) {
        int n = 0;
        for (int i = 0; i < f.length(); i++) {
            if (f.charAt(i) != '%') continue;
            if (f.startsWith("%d", i)) { n++; i++; }
            else if (f.startsWith("%%", i)) i++;
            else return -1;
        }
        return n;
    }


    private static String ba(String before, String after, Texts t) { return before + " " + t.arrow() + " " + after; }

    private static String colourOf(double before, double after, boolean worse) { return after > before ? (worse ? WORSE : BETTER) : after < before ? (worse ? BETTER : WORSE) : GREY; }

    static String statColour(int id, int value, boolean bad) {
        if (value == 0) return GREY;
        if (bad || value < 0) return WORSE;
        return id == 83 || id == 97 || id == 107 || id == 127 || id == 188 ? GRANTED : BETTER;
    }

    static String rangeColour(int beforeMin, int beforeMax, int afterMin, int afterMax, boolean bad) {
        if (afterMin == beforeMin && afterMax == beforeMax) return GREY;
        if (afterMin >= beforeMin && afterMax >= beforeMax) return bad ? WORSE : BETTER;
        if (afterMin <= beforeMin && afterMax <= beforeMax) return WORSE;
        return GREY;
    }


    private static void lifeRegen(List<Line> out, Entry e, Info info, Player p, int v, boolean bad, Texts t) {
        boolean de = t.german();
        if (info.poison && v < 0) {
            double rate = EffectFormat.perSecond(-v);
            out.add(new Line(String.format(t.own("PoisonRate"), num(rate, de)), VALUE, WORSE));
            int ticks = e.shown >= 0 ? e.shown : e.ticks;
            if (e.lists == 1 && ticks > 0) {
                double left = (double) -v * ticks / 256;
                out.add(new Line(String.format(t.own("PoisonLeft"), num(left, de), EffectFormat.seconds(ticks)), VALUE, WORSE));
            }
            return;
        }
        if (!fresh(p) || touchesOp(e, t, ST_LIFE_REGEN)) {

            out.add(new Line(String.format(t.own("PerSec"), t.word("life"), EffectFormat.signed(EffectFormat.perSecond(v), de)), VALUE, statColour(ST_LIFE_REGEN, v, bad)));
            return;
        }
        int total = p.stat(ST_LIFE_REGEN);
        double before = EffectFormat.perSecond(total - v), after = EffectFormat.perSecond(total);
        out.add(new Line(String.format(t.own("PerSec"), t.word("life"), ba(num(before, de), num(after, de), t)), VALUE, colourOf(before, after, false)));
    }


    private static void manaLines(List<Line> out, Entry e, Player p, int base, boolean bad, Texts t) {
        if (!fresh(p) || base <= 0) return;
        if (touches(e, t, ST_MAXMANA) || touchesOp(e, t, ST_MANA_RECOVERY) || touchesOp(e, t, ST_MANA_BONUS)) return;
        boolean de = t.german();
        int bonus = p.stat(ST_MANA_BONUS), rec = p.stat(ST_MANA_RECOVERY), vb = e.own(ST_MANA_BONUS), vr = e.own(ST_MANA_RECOVERY);
        int tickBefore = EffectFormat.manaPerTick(base, bonus - vb) + rec - vr, tickAfter = EffectFormat.manaPerTick(base, bonus) + rec;
        double before = EffectFormat.perSecond(tickBefore), after = EffectFormat.perSecond(tickAfter);
        out.add(new Line(String.format(t.own("PerSec"), t.word("mana"), ba(num(before, de), num(after, de), t)), VALUE, colourOf(before, after, false)));
        int cur = p.stat(ST_MANA), max = p.stat(ST_MAXMANA);
        int sb = secondsToFull(cur, max, tickBefore), sa = secondsToFull(cur, max, tickAfter);
        if (cur < max && sb > 0 && sa > 0) out.add(new Line(String.format(t.own("FullIn"), t.word("mana"), ba(Integer.toString(sb), Integer.toString(sa), t)), VALUE, colourOf(sa, sb, false)));
    }


    private static void stamina(List<Line> out, Entry e, Player p, int v, boolean bad, Texts t) {
        if ((p.flags & EffectFormat.PF_STAMINA) == 0 || touches(e, t, ST_MAXSTAMINA) || touchesOp(e, t, ST_STAMINA_BONUS)) return;
        boolean de = t.german();
        int max = p.stat(ST_MAXSTAMINA), total = p.stat(ST_STAMINA_BONUS), before = total - v;
        double sb = EffectFormat.perSecond(staminaPerTick(max, p.stand, before)), sa = EffectFormat.perSecond(staminaPerTick(max, p.stand, total));
        double rb = EffectFormat.perSecond(staminaPerTick(max, runShift(before, p.run), before)), ra = EffectFormat.perSecond(staminaPerTick(max, runShift(total, p.run), total));
        String word = t.word("stamina");
        out.add(new Line(String.format(t.own("Stand"), word, ba(num(sb, de), num(sa, de), t)), VALUE, colourOf(sb, sa, false)));
        out.add(new Line(String.format(t.own("Run"), word, ba(num(rb, de), num(ra, de), t)), VALUE, colourOf(rb, ra, false)));
    }


    static String resistLabel(String w) { return w == null ? "" : w.replace("-\n", "-").replace('\n', ' ').trim(); }

    private static void resist(List<Line> out, Entry e, Player p, int r, Texts t) {
        if ((p.flags & EffectFormat.PF_DIFF) == 0 || (p.flags & EffectFormat.PF_CLASSIC) == 0 || touchesOp(e, t, RESISTS[r]) || touchesOp(e, t, MAX_RESISTS[r])) return;
        int pen = t.penalty(p.diff, p.classic != 0);
        if (pen == Integer.MIN_VALUE) return;
        int total = p.stat(RESISTS[r]), max = p.stat(MAX_RESISTS[r]), v = e.own(RESISTS[r]), vm = e.own(MAX_RESISTS[r]);
        int before = resShown(total - v, max - vm, pen), after = resShown(total, max, pen);
        String label = resistLabel(t.word("res" + RESISTS[r]));
        if (label.isEmpty()) return;
        boolean capped = total + pen >= resCap(max) && after == resCap(max);
        String s = label + " " + ba(Integer.toString(before), after + " %", t) + (capped ? " · " + t.own("Cap") : "");
        out.add(new Line(s, VALUE, colourOf(before, after, false)));
        if (capped && v > 0) out.add(new Line(String.format(t.own("Effective"), after - before), NOTE, GREY));
    }


    private static void attribute(List<Line> out, Player p, int id, int v, Texts t) {
        String label = resistLabel(t.word("attr" + id));
        if (label.isEmpty()) return;
        int total = p.stat(id), before = total - v;
        out.add(new Line(label + " " + ba(Integer.toString(before), Integer.toString(total), t), VALUE, colourOf(before, total, false)));
    }


    private static void experience(List<Line> out, Player p, int v, Texts t) {
        boolean de = t.german();
        int total = p.stat(ST_EXP_BONUS), before = total - v;
        out.add(new Line(t.own("ExpBonus") + " " + ba(signedInt(before), signedInt(total) + " %", t), VALUE, colourOf(before, total, false)));


        out.add(new Line(t.own("ExpKill") + " " + ba(factor(100 + before, de), factor(100 + total, de), t), VALUE, colourOf(before, total, false)));
        int level = p.stat(ST_LEVEL);
        long exp = p.stat(ST_EXP) & 0xffffffffL, next = level >= 1 && level < MAX_LEVEL ? t.threshold(p.cls, level) : -1;
        if (next > exp) out.add(new Line(String.format(t.own("NextLevel"), level + 1, grouped(next - exp, de)), NOTE, GREY));
    }


    static final int ARMOR_SHRINE = 128, COMBAT_SHRINE = 129;


    static boolean combatFor(Entry e, Player p, Combat c) {
        if (c == null || (c.flags & EffectFormat.CF_VALID) == 0 || c.age >= FRESH_MS || !fresh(p) || p.session == 0 || c.session != p.session || c.unit != p.unit || c.state != e.state) return false;
        return e.state == ARMOR_SHRINE ? c.v171 == e.own(171) && c.v171 != 0 : e.state == COMBAT_SHRINE && c.v19 == e.own(19) && c.v25 == e.own(25) && (c.v19 != 0 || c.v25 != 0);
    }

    static String plainLine(String w) {
        if (w == null) return "";
        for (String part : w.split("\n")) if (!part.contains("%") && !part.trim().isEmpty()) return part.trim();
        return "";
    }



    static boolean shrineCombat(List<Line> out, Entry e, Player p, Combat c, Texts t) {
        if ((e.state != ARMOR_SHRINE && e.state != COMBAT_SHRINE) || !combatFor(e, p, c)) return false;
        int[] v = c.values; int want = 0, done = 0;
        String attack = resistLabel(t.word("attack")), of = attack.isEmpty() ? "" : " (" + attack + ")";
        if (e.state == ARMOR_SHRINE) {
            want++;
            String label = resistLabel(t.word("defense"));
            if ((c.flags & EffectFormat.CF_DEF) != 0 && !label.isEmpty()) { out.add(new Line(label + " " + ba(Integer.toString(v[0]), Integer.toString(v[1]), t), VALUE, colourOf(v[0], v[1], false))); done++; }
        } else {
            if (c.v19 != 0) {
                want++;
                String label = plainLine(t.word("attackrating"));
                if ((c.flags & EffectFormat.CF_AR) != 0 && !label.isEmpty()) { out.add(new Line(label + of + " " + ba(Integer.toString(v[2]), Integer.toString(v[3]), t), VALUE, colourOf(v[2], v[3], false))); done++; }
            }
            want++;
            String label = resistLabel(t.word("damage"));
            if ((c.flags & EffectFormat.CF_DMG) != 0 && !label.isEmpty()) {
                boolean mixed = v[6] < v[4] && v[7] > v[5] || v[6] > v[4] && v[7] < v[5];
                out.add(new Line(label + of + " " + ba(v[4] + "-" + v[5], v[6] + "-" + v[7], t) + (mixed ? " · " + t.own("Mixed") : ""), VALUE, rangeColour(v[4], v[5], v[6], v[7], false)));
                done++;
            }
        }
        return want > 0 && done == want;
    }


    private static void skillShrine(List<Line> out, Map<Integer, Entry> all, Map<Integer, Info> infos, Player p, Skills sk, Texts t) {
        if (!together(p, sk, all.containsKey(SKILL_SHRINE)) || (sk.flags & EffectFormat.SF_PLUS2) == 0) return;
        String plus = EffectFormat.apply(t.word("allskills"), SHRINE_PLUS);
        if (plus != null && !plus.isEmpty()) out.add(new Line(plus, VALUE, GRANTED));
        Integer[] order = new Integer[sk.count];
        for (int i = 0; i < sk.count; i++) order[i] = i;
        Arrays.sort(order, (a, b) -> sk.base[a] != sk.base[b] ? sk.base[b] - sk.base[a] : (sk.base[b] + sk.bonus[b]) - (sk.base[a] + sk.bonus[a]));
        int shown = 0, hidden = 0;
        for (Integer i : order) {
            String name = t.skillName(sk.id[i]);
            if (name == null || name.isEmpty()) continue;
            if (shown >= SHOWN_SKILLS) { hidden++; continue; }
            int after = sk.base[i] + sk.bonus[i], before = after - SHRINE_PLUS;
            String head = name + " " + Math.max(0, before) + " " + t.arrow() + " ";
            out.add(new Line(head + after, VALUE, WHITE, head.length(), GRANTED));
            shown++;
        }
        if (hidden > 0) out.add(new Line(String.format(t.own("More"), hidden), NOTE, GREY));


        List<Integer> states = new ArrayList<>(all.keySet());
        Collections.sort(states);
        for (int s : states) {
            Entry b = all.get(s); Info bi = infos.get(s);
            if (b == null || bi == null || !"buff".equals(bi.kind) || b.skill <= 0 || b.level <= 0) continue;
            int now = sk.effective(b.skill);
            String name = t.skillName(b.skill);
            if (now > b.level && name != null) out.add(new Line(String.format(t.own("CastAt"), name, b.level) + " · " + String.format(t.own("Recast"), now), NOTE, GREY));
        }
    }





    static boolean skillsTrusted(Player p, Skills sk) {
        return fresh(p) && sk != null && (sk.flags & EffectFormat.SF_VALID) != 0 && p.session != 0 && p.session == sk.session && p.unit == sk.unit && sk.age < FRESH_MS && sk.dropped == 0;
    }



    static EffectCalc.Env formulaEnv(Info info, Player p, Skills sk) {
        boolean trusted = sk != null && skillsTrusted(p, sk);
        return new EffectCalc.Env() {
            public int param(int i) { return i >= 1 && i <= info.par.length ? info.par[i - 1] : 0; }
            public String calc(int i) { return i >= 1 && i <= info.calc.length && info.calc[i - 1] != null ? info.calc[i - 1] : ""; }
            public String ast(int i) { return i >= 1 && i <= info.ast.length && info.ast[i - 1] != null ? info.ast[i - 1] : ""; }
            public String len() { return info.len == null ? "" : info.len; }
            public int baseLevel(String skill) {
                Integer id = info.refs.get(skill);
                return id == null || !trusted ? EffectCalc.NONE : sk.baseOf(id);
            }
            public int emin() { return info.emin; }
            public int eminLev(int i) { return i >= 0 && i < info.eminLev.length ? info.eminLev[i] : 0; }
        };
    }



    static int[][] ownValues(Entry e, Info info, Player p, Skills sk) {
        if (info.ownN == 0 || e.level <= 0 || info.bad || e.skill != info.ownSkill) return new int[0][];
        List<int[]> vals = new ArrayList<>();
        EffectCalc.Env env = formulaEnv(info, p, sk);
        for (int k = 0; k < info.ownN; k++) {
            int st = info.ownSt[k];
            if (st != ST_LIFE && st != ST_THORNS_PCT && e.has(st)) continue;
            int v = st == ST_THORNS_PCT && e.has(st) ? e.own(st) : EffectCalc.field(info.ownCa[k], e.level, env);
            if (v != EffectCalc.NONE && v != 0) vals.add(new int[]{st, v});
        }
        return vals.toArray(new int[0][]);
    }



    static Integer slowValue(Entry e, Info info) {
        if (info.slow == null || info.slow.isEmpty() || e.level <= 0 || info.bad || e.skill != info.ownSkill) return null;
        int v = EffectCalc.field(info.slow, e.level, formulaEnv(info, null, null));
        return v == EffectCalc.NONE || v >= 0 ? null : -v;
    }

    private static void ownLine(List<Line> out, int st, int v, boolean bad, Texts t) {
        if (st == ST_LIFE) {
            out.add(new Line(String.format(t.own("Heals"), num(v / 256.0, t.german())), VALUE, bad ? WORSE : BETTER));
            return;
        }
        if (st == ST_THORNS_PCT && v > 0) {
            out.add(new Line(String.format(t.own("Thorns"), v), VALUE, bad ? WORSE : BETTER));
            return;
        }
        if (st == ST_DMG_RESIST && v < 0) {
            out.add(new Line(String.format(t.own("TakesMore"), -v), VALUE, WORSE));
            return;
        }
        String s = t.stat(st, v);
        if (s != null && !s.isEmpty()) out.add(new Line(s, VALUE, statColour(st, v, bad)));
    }

    private static void enemy(List<Line> out, Entry e, Info info, Player p, Skills sk, Texts t, Integer slow) {
        EffectCalc.Env env = formulaEnv(info, p, sk);
        if (slow != null) out.add(new Line(String.format(t.own("SlowEnemies"), slow.intValue()), VALUE, WHITE));
        if (info.enemy.isEmpty()) return;
        List<Line> lines = new ArrayList<>();
        String duration = null;
        int missing = 0;
        for (EnemyLine l : info.enemy) {
            String s = info.bad || e.skill <= 0 || e.level <= 0 ? null : enemyLine(l, e.skill, e.level, env);
            if (s == null) { missing++; continue; }
            if (duration == null && l.f == 36 && lines.isEmpty() && "clc1".equals(l.ca)) duration = s;
            else lines.add(new Line(s, VALUE, WHITE));
        }
        out.add(new Line(t.own("OnEnemies") + (duration != null ? " · " + duration : ""), SUB, null));
        out.addAll(lines);
        if (missing > 0) out.add(new Line(t.own("EnemyTree"), NOTE, GREY));
    }


    static String enemyLine(EnemyLine l, int skill, int level, EffectCalc.Env env) {
        EffectCalc.Env ea = EffectCalc.gate(skill, l.ca, env);
        if (ea == null) return null;
        int a = EffectCalc.field(l.ca, level, ea);
        if (a == EffectCalc.NONE) return null;
        if (l.f == 36) {
            int d = l.cb == null || l.cb.isEmpty() ? 1 : EffectCalc.field(l.cb, level, env);
            if (d == EffectCalc.NONE || d == 0) return null;
            int v = a / d;
            String f = v == 1 || l.b == null ? l.a : l.b;
            return EffectFormat.apply(f, v);
        }
        if (l.f == 74) return EffectFormat.apply(l.a, a);
        if (l.f == 75) {
            EffectCalc.Env eb = EffectCalc.gate(skill, l.cb, env);
            if (eb == null) return null;
            int b = EffectCalc.field(l.cb, level, eb);
            return b == EffectCalc.NONE ? null : format2(l.a, a, b);
        }
        return null;
    }
}
