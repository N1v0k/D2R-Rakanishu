package com.xiaoboluo.d2r;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;










final class GlossaryModel {
    static final String[] CLASSES = {"ama", "sor", "nec", "pal", "bar", "dru", "ass", "war"};

    static final class Text {
        final String de, en;
        Text(String de, String en) { this.de = de == null ? "" : de; this.en = en == null ? "" : en; }
        String get(boolean german) { return german ? de : en; }
        boolean isEmpty() { return de.isEmpty() && en.isEmpty(); }
    }

    static final class Source { String id = "", site = "", title = "", url = "", seen = ""; }

    static final class Term {
        String id = "", cat = "";
        int nameStat = -1, nameGroup = -1;
        String nameWord;
        Text fallback = new Text("", ""), what, how, helps, not, rule, limits = new Text("", "");
        String[] keywords = new String[0], sources = new String[0];
        int[] stats = new int[0];
        boolean has(int stat) { for (int s : stats) if (s == stat) return true; return false; }
    }


    interface Names {
        String stat(int id, boolean german);
        String group(int stat, boolean german);
        String word(String key, boolean german);
    }

    final List<String> catIds = new ArrayList<>();
    final List<Text> cats = new ArrayList<>();
    final List<Term> terms = new ArrayList<>();
    final Map<String, Source> sources = new HashMap<>();
    final Map<String, Text> phrases = new HashMap<>();
    final int[][][] fcr = new int[CLASSES.length][][], fhr = new int[CLASSES.length][][];
    String gameVersion = "";

    static int classIndex(String code) { for (int i = 0; i < CLASSES.length; i++) if (CLASSES[i].equals(code)) return i; return -1; }

    Term byId(String id) { for (Term t : terms) if (t.id.equals(id)) return t; return null; }


    Term byStat(int stat) { for (Term t : terms) if (t.has(stat)) return t; return null; }

    String phrase(String key, boolean german) { Text t = phrases.get(key); return t == null ? "" : t.get(german); }


    static String clean(String format) {
        if (format == null) return "";
        String s = format.replaceAll("-\\s*\\n\\s*(?=\\p{Lu})", "-").replaceAll("-\\s*\\n\\s*", "").replace('\n', ' ');
        s = s.replaceAll("%[+]?d%%", " ").replaceAll("%[+]?[ds]", " ").replace("%%", " ").replace("%", " ");
        s = s.replaceAll("\\s+", " ").trim();
        s = s.replaceAll("^(zu|to)\\s+", "").replaceAll("[:：]$", "").trim();
        return s.isEmpty() ? "" : s.substring(0, 1).toUpperCase(Locale.ROOT) + s.substring(1);
    }


    static String shown(String format) {
        if (format == null) return "";
        return format.replace('\n', ' ').replace("%+d", "+X").replace("%d", "X").replace("%s", "X").replace("%%", "%").replaceAll("\\s+", " ").trim();
    }


    static String format(Term t, Names n, boolean german) {
        if (n == null) return null;
        String f = t.nameStat >= 0 ? n.stat(t.nameStat, german) : t.nameGroup >= 0 ? n.group(t.nameGroup, german) : t.nameWord != null ? n.word(t.nameWord, german) : null;
        return f == null || f.trim().isEmpty() ? null : f;
    }

    static String name(Term t, Names n, boolean german) {
        String c = clean(format(t, n, german));
        return c.isEmpty() ? t.fallback.get(german) : c;
    }



    List<Term> filter(String cat, String query, Names n) {
        List<Term> out = new ArrayList<>();
        String[] words = query == null ? new String[0] : query.trim().toLowerCase(Locale.ROOT).split("\\s+");
        for (Term t : terms) {
            if (cat != null && !cat.equals(t.cat)) continue;
            StringBuilder hay = new StringBuilder();
            hay.append(name(t, n, true)).append(' ').append(name(t, n, false)).append(' ').append(t.fallback.de).append(' ').append(t.fallback.en).append(' ');
            for (String k : t.keywords) hay.append(k).append(' ');
            String h = hay.toString().toLowerCase(Locale.ROOT);
            boolean all = true;
            for (String w : words) if (!w.isEmpty() && !h.contains(w)) { all = false; break; }
            if (all) out.add(t);
        }
        return out;
    }



    static int[] step(int[][] table, int value) {
        if (table == null || table.length == 0) return null;
        int i = 0;
        while (i + 1 < table.length && value >= table[i + 1][0]) i++;
        return i + 1 < table.length ? new int[]{table[i][1], table[i + 1][0], table[i + 1][1]} : new int[]{table[i][1], -1, -1};
    }
}
