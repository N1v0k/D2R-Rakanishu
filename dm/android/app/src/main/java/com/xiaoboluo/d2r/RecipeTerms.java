package com.xiaoboluo.d2r;

import java.util.HashMap;
import java.util.Map;








final class RecipeTerms {
    private RecipeTerms() {}

    static final int MAX = 40;

    private static final String[] BUILT_IN = {
        "runewords", "Runenworte", "Runewords",
        "cat.runes", "Runen", "Runes", "cat.gems", "Edelsteine", "Gems", "cat.craft", "Handwerk", "Crafting", "cat.socket", "Sockelfassungen", "Sockets",
        "cat.improve", "Verbessern", "Improve", "cat.quest", "Quest", "Quest", "cat.ext", "Erweiterung", "Expansion",
        "all", "Alle", "All", "group.weap", "Waffen", "Weapons", "group.tors", "Rüstung", "Armor", "group.helm", "Helm", "Helm", "group.shld", "Schild", "Shield",
        "place.inventory", "Inventar", "Inventory", "place.cube", "Würfel", "Cube", "place.stash", "Schatz", "Stash", "place.other", "andere Seite", "other page",
        "place.shared", "Geteilt", "Shared", "place.stacks", "Stapel-Reiter", "Stack tab", "place.pouch", "Beutel", "Pouch", "place.chronicle", "Chronik", "Chronicle",
        "page", "Seite %u/%u", "Page %u / %u"};
    private static final Map<String, String[]> OWN = new HashMap<>();
    static { for (int i = 0; i < BUILT_IN.length; i += 3) OWN.put(BUILT_IN[i], new String[]{BUILT_IN[i + 1], BUILT_IN[i + 2]}); }

    static final String[] PLACES = {"place.inventory", "place.cube", "place.stash", "place.other", "place.shared", "place.stacks", "place.pouch", "place.chronicle"};

    static boolean known(String key) { return OWN.containsKey(key); }



    static String get(Map<String, String[]> pack, String key, boolean de) {
        String[] p = pack == null ? null : pack.get(key);
        if (p != null && p.length == 2) {
            String w = clean(p[de ? 0 : 1]);
            if (!w.isEmpty() && w.length() <= MAX && ("page".equals(key) ? pageFormat(w) : w.indexOf('%') < 0)) return w;
        }
        String[] own = OWN.get(key);
        return own == null ? key : own[de ? 0 : 1];
    }



    static String page(Map<String, String[]> pack, boolean de, int page, int pages) {
        String f = get(pack, "page", de);
        StringBuilder out = new StringBuilder();
        int n = 0;
        for (int i = 0; i < f.length(); i++) {
            char c = f.charAt(i);
            if (c == '%' && i + 1 < f.length() && (f.charAt(i + 1) == 'u' || f.charAt(i + 1) == 'd')) { out.append(n++ == 0 ? page : pages); i++; }
            else out.append(c);
        }
        return out.toString();
    }


    static String[] places(Map<String, String[]> pack, boolean de) {
        String[] out = new String[PLACES.length];
        for (int i = 0; i < out.length; i++) out[i] = get(pack, PLACES[i], de);
        return out;
    }



    static String clean(String s) {
        if (s == null) return "";
        String t = s;
        if (t.startsWith("[") && t.length() > 4 && t.charAt(3) == ']') {
            int next = t.indexOf('[', 4);
            t = t.substring(4, next < 0 ? t.length() : next);
        }
        return t.replaceAll("\\s+", " ").trim();
    }


    private static boolean pageFormat(String f) {
        int n = 0;
        for (int i = 0; i < f.length(); i++) {
            if (f.charAt(i) != '%') continue;
            if (i + 1 >= f.length() || (f.charAt(i + 1) != 'u' && f.charAt(i + 1) != 'd')) return false;
            n++; i++;
        }
        return n == 2;
    }
}
