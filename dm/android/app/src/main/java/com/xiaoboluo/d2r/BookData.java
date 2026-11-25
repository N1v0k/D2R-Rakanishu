package com.xiaoboluo.d2r;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;





final class BookData {
    static final int SCHEMA = 1;
    static final int QUALITY = 1, UBER = 2, CLASS = 4;
    static final int UPGRADES = 1, BOVINE_DROP = 2;
    static final int LANGUAGES = 13;


    static final class Set {

        final int[][] head;
        final int[][] entries;
        final String[] itemCode, itemName;
        final int[] itemLevel, itemFlags;
        final char[] itemCategory;
        final int[] uniqueId, uniqueItem, uniqueLevel, uniqueRarity, uniqueDiffs;
        final String[] uniqueName;
        final int[] setId, setItem, setLevel, setRarity, setBovine;
        final String[] setName;
        final int[][] ratio = new int[4][];
        final Map<Integer, int[]> monster, superunique;
        final Map<Integer, Extra> extra;
        final int[][] uniquesOfItem, setsOfItem;

        Set(Map<String, Object> o) {
            List<Object> tc = list(o, "tc");
            head = new int[tc.size()][];
            entries = new int[tc.size()][];
            List<Object> items = list(o, "item"), uniques = list(o, "unique"), sets = list(o, "set");
            int n = items.size();
            itemCode = new String[n]; itemName = new String[n]; itemLevel = new int[n]; itemFlags = new int[n]; itemCategory = new char[n];
            for (int k = 0; k < n; k++) {
                List<Object> r = row(items.get(k), 5);
                itemCode[k] = text(r.get(0)); itemName[k] = text(r.get(1)); itemLevel[k] = whole(r.get(2)); itemFlags[k] = whole(r.get(4));
                String cat = text(r.get(3));
                if (cat.length() != 1 || "rejcywagpq".indexOf(cat.charAt(0)) < 0) throw new IllegalArgumentException("book.json: item category " + cat);
                itemCategory[k] = cat.charAt(0);
            }
            int nu = uniques.size();
            uniqueId = new int[nu]; uniqueItem = new int[nu]; uniqueLevel = new int[nu]; uniqueRarity = new int[nu]; uniqueDiffs = new int[nu];
            uniqueName = new String[nu];
            List<List<Integer>> byItemU = buckets(n), byItemS = buckets(n);
            for (int k = 0; k < nu; k++) {
                List<Object> r = row(uniques.get(k), 6);
                uniqueId[k] = whole(r.get(0)); uniqueItem[k] = index(r.get(1), n); uniqueLevel[k] = whole(r.get(2)); uniqueRarity[k] = whole(r.get(3));
                uniqueName[k] = text(r.get(4)); uniqueDiffs[k] = whole(r.get(5));
                if (uniqueRarity[k] < 0) throw new IllegalArgumentException("book.json: unique rarity");
                byItemU.get(uniqueItem[k]).add(k);
            }
            int ns = sets.size();
            setId = new int[ns]; setItem = new int[ns]; setLevel = new int[ns]; setRarity = new int[ns]; setBovine = new int[ns]; setName = new String[ns];
            for (int k = 0; k < ns; k++) {
                List<Object> r = row(sets.get(k), 6);
                setId[k] = whole(r.get(0)); setItem[k] = index(r.get(1), n); setLevel[k] = whole(r.get(2)); setRarity[k] = whole(r.get(3));
                setName[k] = text(r.get(4)); setBovine[k] = whole(r.get(5));
                if (setRarity[k] < 0) throw new IllegalArgumentException("book.json: set rarity");
                byItemS.get(setItem[k]).add(k);
            }
            uniquesOfItem = arrays(byItemU); setsOfItem = arrays(byItemS);
            for (int k = 0; k < tc.size(); k++) {
                List<Object> r = row(tc.get(k), 10);
                int[] h = new int[9];
                for (int j = 0; j < 9; j++) h[j] = whole(r.get(j));
                if (h[0] == 0 || h[1] < 0) throw new IllegalArgumentException("book.json: class " + k + " picks / nodrop");
                head[k] = h;
                List<Object> e = list(r.get(9));
                if (e.size() % 3 != 0) throw new IllegalArgumentException("book.json: class " + k + " entries");
                int[] x = new int[e.size()];
                for (int j = 0; j < e.size(); j += 3) {
                    int kind = whole(e.get(j));
                    x[j] = kind;
                    x[j + 1] = index(e.get(j + 1), kind == 0 ? tc.size() : kind == 1 ? n : kind == 2 ? nu : -1);
                    x[j + 2] = whole(e.get(j + 2));
                    if (x[j + 2] <= 0) throw new IllegalArgumentException("book.json: class " + k + " weight");
                }
                entries[k] = x;
            }
            for (Object x : list(o, "ratio")) {
                List<Object> r = row(x, 14);
                int uber = whole(r.get(0)), cls = whole(r.get(1));
                if ((uber | cls) >>> 1 != 0 || ratio[uber * 2 + cls] != null) throw new IllegalArgumentException("book.json: ratio rows");
                int[] v = new int[12];
                for (int j = 0; j < 12; j++) v[j] = whole(r.get(2 + j));
                if (v[1] <= 0 || v[4] <= 0 || v[7] <= 0 || v[10] <= 0) throw new IllegalArgumentException("book.json: ratio divisor");
                ratio[uber * 2 + cls] = v;
            }
            for (int[] r : ratio) if (r == null) throw new IllegalArgumentException("book.json: ratio rows missing");
            monster = classes(map(o, "mon"), tc.size());
            superunique = classes(map(o, "su"), tc.size());
            extra = new HashMap<>();
            for (Map.Entry<String, Object> e : map(o, "extra").entrySet()) {
                List<Object> r = row(e.getValue(), 4);
                List<Object> lv = row(r.get(2), 3), by = list(r.get(3));
                int[] parents = new int[by.size()];
                for (int j = 0; j < parents.length; j++) parents[j] = whole(by.get(j));
                extra.put(key(e.getKey()), new Extra(text(r.get(0)), text(r.get(1)), new int[]{whole(lv.get(0)), whole(lv.get(1)), whole(lv.get(2))}, parents));
            }
            acyclic();
        }

        private void acyclic() {
            int[] state = new int[head.length];
            for (int k = 0; k < head.length; k++) visit(k, state, 0);
        }
        private void visit(int k, int[] state, int depth) {
            if (state[k] == 2) return;
            if (state[k] == 1 || depth > head.length) throw new IllegalArgumentException("book.json: classes call each other");
            state[k] = 1;
            int[] e = entries[k];
            for (int j = 0; j < e.length; j += 3) if (e[j] == 0) visit(e[j + 1], state, depth + 1);
            state[k] = 2;
        }



        int upgrade(int tc, int mlvl) {
            int g = head[tc][6];
            if (g <= 0) return tc;
            while (tc + 1 < head.length && head[tc + 1][6] == g && head[tc + 1][7] <= mlvl) tc++;
            return tc;
        }
    }



    static final class Extra {
        final String name, look;
        final int[] level, parents;
        Extra(String name, String look, int[] level, int[] parents) { this.name = name; this.look = look; this.level = level; this.parents = parents; }
    }

    final Map<String, Set> sets;
    final Map<String, String[]> names;

    private BookData(Map<String, Set> sets, Map<String, String[]> names) { this.sets = sets; this.names = names; }


    static BookData parse(String text) {
        Object root = BookJson.parse(text);
        if (!(root instanceof Map)) throw new IllegalArgumentException("book.json: not an object");
        @SuppressWarnings("unchecked") Map<String, Object> o = (Map<String, Object>) root;
        if (whole(o.get("v")) != SCHEMA) throw new IllegalArgumentException("book.json: schema " + o.get("v"));
        Map<String, Set> sets = new HashMap<>();
        for (Map.Entry<String, Object> e : map(o, "sets").entrySet()) sets.put(e.getKey(), new Set(map(e.getValue())));
        if (!sets.containsKey("rotw") || !sets.containsKey("lod")) throw new IllegalArgumentException("book.json: a table set is missing");
        Map<String, String[]> names = new HashMap<>();
        for (Map.Entry<String, Object> e : map(o, "names").entrySet()) {
            List<Object> l = row(e.getValue(), LANGUAGES);
            String[] a = new String[LANGUAGES];
            for (int j = 0; j < LANGUAGES; j++) a[j] = text(l.get(j));
            names.put(e.getKey(), a);
        }
        return new BookData(Collections.unmodifiableMap(sets), Collections.unmodifiableMap(names));
    }


    String name(String key, int lang) {
        String[] n = names.get(key);
        return n == null ? null : n[lang >= 0 && lang < LANGUAGES ? lang : 0];
    }

    private static Map<Integer, int[]> classes(Map<String, Object> m, int count) {
        Map<Integer, int[]> out = new HashMap<>();
        for (Map.Entry<String, Object> e : m.entrySet()) {
            List<Object> r = row(e.getValue(), 4);
            int[] v = new int[4];
            for (int d = 0; d < 3; d++) { v[d] = whole(r.get(d)); if (v[d] < -1 || v[d] >= count) throw new IllegalArgumentException("book.json: class of " + e.getKey()); }
            v[3] = whole(r.get(3));
            out.put(key(e.getKey()), v);
        }
        return out;
    }
    private static int key(String k) {
        try { return Integer.parseInt(k); } catch (NumberFormatException e) { throw new IllegalArgumentException("book.json: key " + k); }
    }
    private static List<List<Integer>> buckets(int n) { List<List<Integer>> b = new ArrayList<>(n); for (int k = 0; k < n; k++) b.add(new ArrayList<>()); return b; }
    private static int[][] arrays(List<List<Integer>> b) {
        int[][] out = new int[b.size()][];
        for (int k = 0; k < out.length; k++) { out[k] = new int[b.get(k).size()]; for (int j = 0; j < out[k].length; j++) out[k][j] = b.get(k).get(j); }
        return out;
    }
    @SuppressWarnings("unchecked")
    private static Map<String, Object> map(Object o) {
        if (!(o instanceof Map)) throw new IllegalArgumentException("book.json: an object is missing");
        return (Map<String, Object>) o;
    }
    private static Map<String, Object> map(Map<String, Object> o, String k) { return map(o.get(k)); }
    @SuppressWarnings("unchecked")
    private static List<Object> list(Object o) {
        if (!(o instanceof List)) throw new IllegalArgumentException("book.json: a list is missing");
        return (List<Object>) o;
    }
    private static List<Object> list(Map<String, Object> o, String k) { return list(o.get(k)); }
    private static List<Object> row(Object o, int n) {
        List<Object> l = list(o);
        if (l.size() != n) throw new IllegalArgumentException("book.json: a row of " + l.size() + " fields, want " + n);
        return l;
    }
    private static int whole(Object o) {
        if (!(o instanceof Long) || (Long) o != (int) (long) (Long) o) throw new IllegalArgumentException("book.json: not a whole number: " + o);
        return (int) (long) (Long) o;
    }
    private static int index(Object o, int size) {
        int v = whole(o);
        if (v < 0 || v >= size) throw new IllegalArgumentException("book.json: index " + v + " outside " + size);
        return v;
    }
    private static String text(Object o) {
        if (!(o instanceof String)) throw new IllegalArgumentException("book.json: not a text: " + o);
        return (String) o;
    }
}
