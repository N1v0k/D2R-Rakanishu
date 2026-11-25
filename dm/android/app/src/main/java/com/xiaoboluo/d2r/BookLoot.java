package com.xiaoboluo.d2r;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Set;
import java.util.WeakHashMap;






final class BookLoot {
    static final int FOUND = -1, UNIQUES = 0, SETS = 1, RUNES = 2, GEMS = 3, JEWELLERY = 4, WEAPONS = 5, ARMOUR = 6, OTHER = 7, GROUPS = 8;

    static boolean[] defaultOpen() { boolean[] o = new boolean[GROUPS]; o[UNIQUES] = o[SETS] = o[RUNES] = true; return o; }

    static final class Row {
        final int kind, index, group;
        final double chance;
        final String name, code;
        final int id;
        Row(int kind, int index, int group, double chance, String name, String code, int id) {
            this.kind = kind; this.index = index; this.group = group; this.chance = chance; this.name = name; this.code = code; this.id = id;
        }
        String key() { return kind + ":" + index; }
    }


    static final class Line {
        final int group, count;
        final Row row;
        Line(int group, int count, Row row) { this.group = group; this.count = count; this.row = row; }
    }

    static int group(BookData.Set s, DropCalc.Drop d) {
        if (d.kind == DropCalc.UNIQUE) return UNIQUES;
        if (d.kind == DropCalc.SET) return SETS;
        switch (s.itemCategory[d.index]) {
            case 'g': return -2;
            case 'r': return RUNES;
            case 'e': case 'j': return GEMS;
            case 'c': case 'y': return JEWELLERY;
            case 'w': return WEAPONS;
            case 'a': return ARMOUR;
            default: return OTHER;
        }
    }


    static List<Row> rows(BookData data, BookData.Set s, List<DropCalc.Drop> drops, int lang) {
        List<Row> out = new ArrayList<>();
        for (DropCalc.Drop d : drops) {
            int g = group(s, d);
            if (g < 0) continue;
            String key = d.kind == DropCalc.ITEM ? s.itemName[d.index] : d.kind == DropCalc.UNIQUE ? s.uniqueName[d.index] : s.setName[d.index];
            String name = data.name(key, lang);
            if (name == null || name.isEmpty()) continue;
            int item = d.kind == DropCalc.ITEM ? d.index : d.kind == DropCalc.UNIQUE ? s.uniqueItem[d.index] : s.setItem[d.index];
            int id = d.kind == DropCalc.UNIQUE ? s.uniqueId[d.index] : d.kind == DropCalc.SET ? s.setId[d.index] : -1;
            out.add(new Row(d.kind, d.index, g, d.chance, name, s.itemCode[item], id));
        }
        return out;
    }



    static List<Line> lines(List<Row> rows, boolean[] open, Set<String> found) {
        List<Line> out = new ArrayList<>();
        for (Row r : rows) if (found.contains(r.key())) out.add(new Line(FOUND, 0, r));
        for (int g = 0; g < GROUPS; g++) {
            int n = 0;
            for (Row r : rows) if (r.group == g) n++;
            if (n == 0) continue;
            out.add(new Line(g, n, null));
            if (open[g]) for (Row r : rows) if (r.group == g) out.add(new Line(g, 0, r));
        }
        return out;
    }



    static String ratio(double chance, int lang) {
        double n = chance >= 1 ? 1 : 1 / chance;
        boolean comma = lang != CombatModel.L_EN;
        String s;
        if (n < 9.95) s = String.format(Locale.ROOT, "%.1f", n);
        else s = String.format(Locale.US, "%,d", Math.round(n));
        if (n < 9.95) { if (s.endsWith(".0")) s = s.substring(0, s.length() - 2); if (comma) s = s.replace('.', ','); }
        else if (comma) s = s.replace(',', '.');
        return "1 : " + s;
    }


    static int dropClass(BookData.Set s, String kindKey, int diff) {
        int[] tc = entry(s, kindKey);
        return tc == null || diff < 0 || diff > 2 ? -1 : tc[diff];
    }

    static int flags(BookData.Set s, String kindKey) { int[] tc = entry(s, kindKey); return tc == null ? 0 : tc[3]; }

    static int rootClass(BookData.Set s, String kindKey, int diff, int mlvl) {
        return DropCalc.root(s, dropClass(s, kindKey, diff), diff, mlvl, flags(s, kindKey));
    }
    private static int[] entry(BookData.Set s, String kindKey) {
        if (kindKey == null || kindKey.length() < 2) return null;
        int n;
        try { n = Integer.parseInt(kindKey.substring(1)); } catch (NumberFormatException e) { return null; }
        return kindKey.charAt(0) == 'S' ? s.superunique.get(n) : kindKey.charAt(0) == 'M' ? s.monster.get(n) : null;
    }



    static int level(BookData.Set s, String kindKey, int diff, CombatModel.Enemy encounter, CombatModel.Enemy place) {
        if (encounter != null) return encounter.level;
        BookData.Extra x = null;
        try { if (kindKey != null && kindKey.startsWith("M")) x = s.extra.get(Integer.parseInt(kindKey.substring(1))); } catch (NumberFormatException e) { x = null; }
        if (diff > 0 && place != null && place.level > 0) return place.level;
        return x == null || diff < 0 || diff > 2 ? -1 : x.level[diff];
    }






    static java.util.Map<String, Double> chances(BookModel b, BookData.Set s, int diff, int players, int mf, List<int[]> targets) {
        java.util.Map<String, Double> out = new java.util.HashMap<>();
        java.util.Map<String, DropCalc> calcs = new java.util.HashMap<>();
        for (BookModel.Kind k : b.kinds) {
            if (dropClass(s, k.key, diff) < 0) continue;
            if (k.encounters.isEmpty()) {
                List<CombatModel.Enemy> places = b.parentPlaces(k, null);
                if (places.isEmpty()) {
                    int level = level(s, k.key, diff, null, null);
                    if (level >= 0) best(out, k.key + "|-", calcs, s, k.key, diff, level, players, mf, targets);
                } else for (CombatModel.Enemy p : places) {
                    int level = level(s, k.key, diff, null, p);
                    if (level >= 0) best(out, k.key + "|" + p.id, calcs, s, k.key, diff, level, players, mf, targets);
                }
            } else for (CombatModel.Enemy e : k.encounters) best(out, k.key + "|" + e.id, calcs, s, k.key, diff, e.level, players, mf, targets);
        }
        return out;
    }

    private static void best(java.util.Map<String, Double> out, String key, java.util.Map<String, DropCalc> calcs, BookData.Set s, String kindKey,
                             int diff, int level, int players, int mf, List<int[]> targets) {
        int flags = flags(s, kindKey), root = rootClass(s, kindKey, diff, level);
        if (root < 0) return;
        DropCalc c = calcs.computeIfAbsent(level + "/" + flags, x -> new DropCalc(s, diff, level, players, mf, flags));
        double p = 0;
        for (int[] t : targets) p = Math.max(p, c.chance(root, t[0], t[1]));
        if (p > 0) out.put(key, p);
    }

    private static final Map<BookData.Set, List<Object[]>> INDEX = new WeakHashMap<>();



    static List<int[]> match(BookData data, BookData.Set s, String folded) {
        if (folded == null || folded.length() < 2) return Collections.emptyList();
        List<int[]> out = new ArrayList<>();
        for (Object[] e : index(data, s)) if (((String) e[2]).contains(folded)) out.add(new int[]{(Integer) e[0], (Integer) e[1]});
        return out;
    }

    private static synchronized List<Object[]> index(BookData data, BookData.Set s) {
        List<Object[]> idx = INDEX.get(s);
        if (idx != null) return idx;
        idx = new ArrayList<>();
        Set<String> seen = new HashSet<>();
        for (int i = 0; i < s.itemCode.length; i++) if (s.itemCategory[i] != 'g') add(idx, data, DropCalc.ITEM, i, s.itemName[i], seen);
        for (int i = 0; i < s.uniqueName.length; i++) add(idx, data, DropCalc.UNIQUE, i, s.uniqueName[i], seen);
        for (int i = 0; i < s.setName.length; i++) add(idx, data, DropCalc.SET, i, s.setName[i], seen);
        INDEX.put(s, idx);
        return idx;
    }

    private static void add(List<Object[]> idx, BookData data, int kind, int index, String key, Set<String> seen) {
        StringBuilder all = new StringBuilder();
        for (int l = 0; l < BookData.LANGUAGES; l++) {
            String n = data.name(key, l);
            if (n != null && !n.isEmpty()) all.append(CombatModel.fold(n)).append('\n');
        }
        if (all.length() > 0 && seen.add(kind + ":" + index)) idx.add(new Object[]{kind, index, all.toString()});
    }
}
