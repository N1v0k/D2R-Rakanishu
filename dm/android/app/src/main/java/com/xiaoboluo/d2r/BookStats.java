package com.xiaoboluo.d2r;

import java.util.ArrayList;
import java.util.List;






final class BookStats {
    static final class Line {
        final String text, colour;
        Line(String text, String colour) { this.text = text; this.colour = colour; }
    }

    static String modKey(int mod) {
        switch (mod) {
            case 5: return "uniquextrastrong";
            case 6: return "uniqueextrafast";
            case 7: return "uniquecursed";
            case 8: return "uniquemagicresistance";
            case 9: return "uniquefireenchanted";
            case 17: return "monsteruniqueprop2";
            case 18: return "monsteruniqueprop1";
            case 25: return "monsteruniqueprop3";
            case 26: return "monsteruniqueprop5";
            case 27: return "monsteruniqueprop4";
            case 28: return "monsteruniqueprop6";
            case 29: return "monsteruniqueprop7";
            case 30: return "monsteruniqueprop9";
            default: return null;
        }
    }
    interface Names { String get(String key); boolean has(String key); }

    static final String[] ELEMENTS = {"dmAndroidBookPhysical", "dmAndroidBookFire", "dmAndroidBookLightning", "dmAndroidBookCold", "dmAndroidBookPoison",
        "dmAndroidBookMagic"};
    static String element(int e, BookText w) { return e < 0 || e > 5 ? "?" : w.t(ELEMENTS[e]); }
    static String range(int min, int max) { return min == max ? Integer.toString(max) : min + "–" + max; }
    private static String seconds(int frames, int lang) { return CombatModel.number(frames / 25.0, lang).replaceAll("[.,]0$", "") + " s"; }
    private static String attack(int kind, BookText w) {
        return w.t(kind == CombatModel.ATTACK_A1 ? "dmAndroidBookAttack1" : kind == CombatModel.ATTACK_A2 ? "dmAndroidBookAttack2"
            : kind == CombatModel.ATTACK_SKILL ? "dmAndroidBookAttackSkill" : "dmAndroidBookAttackOther");
    }


    static List<Line> values(CombatModel.Stats s, int[] resistance, int lang, BookText w) {
        List<Line> out = new ArrayList<>();
        if (s == null) return out;
        out.add(new Line(w.t("dmAndroidBookStatLevelLife", s.level, range(s.lifeMin, s.lifeMax)), "FontColorWhite"));
        String def = s.defense > 0 ? w.t("dmAndroidBookStatDefense", s.defense) : "";
        if (s.block > 0) def += (def.isEmpty() ? "" : " · ") + w.t("dmAndroidBookStatBlock", s.block);
        if (!def.isEmpty()) out.add(new Line(def, "FontColorWhite"));
        for (int[] a : s.attacks) {
            String value = w.t("dmAndroidBookStatDamage", range(a[1], a[2])) + (a[3] > 0 ? " · " + w.t("dmAndroidBookStatRating", a[3]) : "");
            out.add(new Line(w.t("dmAndroidBookLabel", attack(a[0], w), value), "FontColorWhite"));
            for (int[] e : s.elements) if (e[0] == a[0] && e[4] > 0) out.add(elemental(e, lang, w, "  + "));
        }
        for (int[] e : s.elements) {
            if (e[4] <= 0) continue;
            boolean riding = false;
            for (int[] a : s.attacks) riding |= a[0] == e[0];
            if (!riding) out.add(elemental(e, lang, w, attack(e[0], w) + ": "));
        }
        StringBuilder res = new StringBuilder(), immune = new StringBuilder();
        for (int i = 0; resistance != null && i < 6 && i < resistance.length; i++) {
            if (resistance[i] >= 100) { immune.append(immune.length() == 0 ? "" : ", ").append(element(i, w)); continue; }
            if (resistance[i] != 0) res.append(res.length() == 0 ? "" : " · ").append(element(i, w)).append(' ').append(resistance[i]).append(" %");
        }
        if (res.length() > 0) out.add(new Line(w.t("dmAndroidBookStatResist", res), "FontColorWhite"));
        if (immune.length() > 0) out.add(new Line(w.t("dmAndroidBookImmune", immune), "FontColorWhite"));
        if (s.drain < 100) out.add(new Line(w.t("dmAndroidBookStatLeech", s.drain), "FontColorLightGray"));
        return out;
    }

    private static Line elemental(int[] e, int lang, BookText w, String prefix) {
        String text = prefix + element(e[1], w) + " " + range(e[2], e[3]);
        if (e[1] == 4 && e[5] > 0) text += " " + w.t("dmAndroidBookStatOver", seconds(e[5], lang));
        if (e[1] == 3 && e[5] > 0) text += " · " + w.t("dmAndroidBookStatChill", seconds(e[5], lang));
        if (e[4] > 0 && e[4] < 100) text += " · " + w.t("dmAndroidBookStatChance", e[4]);
        return new Line(text, "FontColorWhite");
    }


    static List<String> abilities(CombatModel.Stats s, BookText w, Names names) {
        List<String> out = new ArrayList<>();
        if (s == null) return out;
        for (int id : s.skills) {
            String key = "@skill:" + id;
            if (id > 0 && names.has(key) && !out.contains(names.get(key))) out.add(names.get(key));
        }
        for (int mod : s.mods) {
            String key = modKey(mod);
            if (key != null && names.has(key) && !out.contains(names.get(key))) out.add(names.get(key));
        }
        if (s.ranged) out.add(w.t("dmAndroidBookRanged"));
        if (s.undead) out.add(w.t("dmAndroidBookUndead"));
        if (s.demon) out.add(w.t("dmAndroidBookDemon"));
        return out;
    }
}
