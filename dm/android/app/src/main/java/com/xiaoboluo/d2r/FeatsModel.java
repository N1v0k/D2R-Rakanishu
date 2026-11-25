package com.xiaoboluo.d2r;

import java.text.DateFormat;
import java.text.NumberFormat;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.TimeZone;












final class FeatsModel {
    static final int MODE_OWN = 0, MODE_FOREIGN = 1, MODE_NEWER = 2, MODE_UNREAD = 3;
    static final int R_KILLS = 1, R_ACT = 2, R_DIFF = 3, R_CLASS_HELL = 4, R_LEVEL = 5, R_EVERY_CLASS = 6, R_WAYPOINTS = 7, R_HC_LEVEL = 8,
        R_HC_GAME = 9, R_OWN = 10, R_UNIQUES = 11, R_FULL_SET = 12, R_MERC = 13, R_EVENT = 14, R_SALES = 15, R_GOLD = 16, R_STREAK = 17, R_BOSSES = 18,
        R_BOSS_FAST = 19, R_RIFT_CLEAR = 20, R_RIFT_TIER = 21, R_RIFT_FAST = 22, R_SHRINES = 23, R_LOOKS = 24, R_DYES = 25, R_QUESTS = 26, R_LAST = R_QUESTS;
    static final int GROUPS = 8, CLASSES = 8, DIFFICULTIES = 3, COUNTERS = 5, KILLS = 0, SALES = 1, GOLD = 2, LOOKS = 3, DYES = 4, SLOTS = 10, QUESTS = 27;
    static final int EVENTS = 0x7fff, E_DRY = 512;
    static final int TICKS_PER_SECOND = 25, RIFT_FAST_TIER = 10, ESCAPE_PCT = 5, EQUIP_PCT = 25;

    static final class Row {
        final int id, group, rule;
        final long arg;
        final boolean entered;
        final long when;
        Row(int id, int group, int rule, long arg, boolean entered, long when) {
            this.id = id; this.group = group; this.rule = rule; this.arg = arg; this.entered = entered; this.when = when;
        }
    }

    int mode = -1, cls = -2, hardcore = -1, uniques = -1, bosses = -1, quests = -1;
    String hero;
    final long[] counters = new long[COUNTERS];
    final List<Row> rows = new ArrayList<>();
    String[] classNames = new String[CLASSES], difficultyNames = new String[DIFFICULTIES];
    Map<Integer, String> rowNames = new HashMap<>();

    FeatsModel all;
    boolean account;
    int heroes = -1;
    boolean unavailable, incomplete;
    final List<String> skipped = new ArrayList<>(), newer = new ArrayList<>(), waiting = new ArrayList<>();

    static FeatsModel parse(String text) {
        FeatsModel m = new FeatsModel(), a = new FeatsModel();
        a.account = true; a.mode = MODE_OWN; a.cls = -1; a.hero = "";
        boolean[] counted = new boolean[COUNTERS], added = new boolean[COUNTERS];
        for (String line : text.split("\n")) {
            if (line.isEmpty()) continue;
            String[] v = line.split("\t", -1);
            try {
                switch (v[0]) {
                case "feats":
                    if (v.length != 3 || m.mode >= 0) throw new IllegalArgumentException("feats line");
                    m.mode = range(v[1], MODE_OWN, MODE_UNREAD); m.hero = v[2]; break;
                case "counter": counter(m, v, counted); break;
                case "row": row(m, v, false); break;
                case "allrow": row(a, v, true); break;
                case "allcounter": counter(a, v, added); break;
                case "all":
                    if (v.length != 2 || a.heroes >= 0) throw new IllegalArgumentException("all line");
                    a.heroes = range(v[1], 0, Integer.MAX_VALUE); break;
                case "skipped":
                    if (v.length != 3 || v[1].isEmpty() || !(v[2].equals("newer") || v[2].equals("unreadable"))) throw new IllegalArgumentException("skipped line");
                    (v[2].equals("newer") ? a.newer : a.skipped).add(v[1]); break;
                case "waiting":
                    if (v.length != 2 || v[1].isEmpty()) throw new IllegalArgumentException("waiting line");
                    a.waiting.add(v[1]); break;
                case "incomplete":
                    if (v.length != 2 || a.unavailable || a.incomplete || !(v[1].equals("unavailable") || v[1].equals("limit"))) throw new IllegalArgumentException("incomplete line");
                    if (v[1].equals("unavailable")) a.unavailable = true; else a.incomplete = true;
                    break;
                case "hero":
                    if (v.length != 6 || m.cls != -2) throw new IllegalArgumentException("hero line");
                    m.cls = range(v[1], -1, CLASSES - 1); m.hardcore = range(v[2], -1, 1); m.uniques = range(v[3], -1, SLOTS); m.bosses = range(v[4], -1, Integer.MAX_VALUE);
                    m.quests = range(v[5], -1, QUESTS); break;
                case "name": {
                    if (v.length != 3) throw new IllegalArgumentException("name line");
                    int id = range(v[1], 1, 65535);
                    if (m.rowNames.containsKey(id)) throw new IllegalArgumentException("name twice");
                    m.rowNames.put(id, v[2]); break; }
                case "class": {
                    if (v.length != 3) throw new IllegalArgumentException("class line");
                    int c = range(v[1], 0, CLASSES - 1);
                    if (m.classNames[c] != null) throw new IllegalArgumentException("class twice");
                    m.classNames[c] = v[2]; break; }
                case "difficulty": {
                    if (v.length != 3) throw new IllegalArgumentException("difficulty line");
                    int d = range(v[1], 0, DIFFICULTIES - 1);
                    if (m.difficultyNames[d] != null) throw new IllegalArgumentException("difficulty twice");
                    m.difficultyNames[d] = v[2]; break; }
                default: throw new IllegalArgumentException("unknown line " + v[0]);
                }
            } catch (RuntimeException x) { throw new IllegalArgumentException("Invalid achievements list", x); }
        }
        boolean complete = m.mode >= 0 && m.cls != -2 && !m.rows.isEmpty() && a.heroes >= 0 && !a.rows.isEmpty();
        for (boolean c : counted) complete &= c;
        for (boolean c : added) complete &= c;
        for (String n : m.classNames) complete &= n != null;
        for (String n : m.difficultyNames) complete &= n != null;
        for (Row r : a.rows) complete &= !named(r) || m.rowNames.containsKey(r.id);
        if (!complete) throw new IllegalArgumentException("Incomplete achievements list");
        a.classNames = m.classNames; a.difficultyNames = m.difficultyNames; a.rowNames = m.rowNames;
        m.all = a;
        return m;
    }

    private static void counter(FeatsModel m, String[] v, boolean[] seen) {
        if (v.length != 3) throw new IllegalArgumentException(v[0] + " line");
        int c = range(v[1], 0, COUNTERS - 1);
        if (seen[c]) throw new IllegalArgumentException(v[0] + " twice");
        long value = Long.parseUnsignedLong(v[2]);
        m.counters[c] = value < 0 ? Long.MAX_VALUE : value;
        seen[c] = true;
    }

    private static void row(FeatsModel m, String[] v, boolean sum) {
        if (v.length != 7) throw new IllegalArgumentException(v[0] + " line");
        int entered = range(v[5], 0, 1);
        long when = Long.parseLong(v[6]), arg = Long.parseLong(v[4]);
        Row r = new Row(range(v[1], 1, 65535), range(v[2], 1, GROUPS), range(v[3], R_KILLS, R_LAST), arg, entered == 1, when);
        if (r.rule == R_EVERY_CLASS && !sum) throw new IllegalArgumentException("row " + r.id + " rule");
        if (arg < 0 || when < 0 || when > 0xffffffffL || (!r.entered && when != 0) || m.row(r.id) != null) throw new IllegalArgumentException("row " + r.id);
        if ((r.rule == R_KILLS && arg < 1) || (r.rule == R_ACT && (arg < 1 || arg > 5)) || (r.rule == R_DIFF && arg >= DIFFICULTIES)
            || (r.rule == R_CLASS_HELL && arg >= CLASSES) || (r.rule == R_EVERY_CLASS && (arg < 1 || arg > CLASSES)) || (r.rule == R_HC_LEVEL && arg < 1)
            || (r.rule == R_OWN && arg < 1) || (r.rule == R_UNIQUES && (arg < 1 || arg > SLOTS)) || (r.rule == R_QUESTS && arg != QUESTS) || (r.rule == R_MERC && (arg < 1 || arg > 5))
            || (r.rule == R_EVENT && (Long.bitCount(arg) != 1 || (arg & ~EVENTS) != 0)) || (r.rule == R_SALES && arg < 1) || (r.rule == R_GOLD && arg < 1)
            || ((r.rule == R_STREAK || r.rule == R_BOSSES || r.rule == R_BOSS_FAST || r.rule == R_RIFT_TIER || r.rule == R_RIFT_FAST || r.rule == R_SHRINES
                 || r.rule == R_LOOKS || r.rule == R_DYES) && arg < 1))
            throw new IllegalArgumentException("row " + r.id + " arg");
        m.rows.add(r);
    }
    private static int range(String s, int lo, int hi) {
        int v = Integer.parseInt(s);
        if (v < lo || v > hi) throw new IllegalArgumentException(s + " not in " + lo + ".." + hi);
        return v;
    }

    Row row(int id) { for (Row r : rows) if (r.id == id) return r; return null; }


    boolean resting() { return mode == MODE_NEWER || mode == MODE_UNREAD; }

    boolean shown(Row r) {
        if (r.entered) return true;
        if (r.rule == R_CLASS_HELL) return cls < 0 || r.arg == cls;
        if (r.rule == R_HC_LEVEL || r.rule == R_HC_GAME) return hardcore != 0;
        return true;
    }


    List<Row> rows(int group) {
        List<Row> out = new ArrayList<>();
        for (Row r : rows) if (r.group == group && shown(r)) out.add(r);
        return out;
    }

    List<Integer> groups() {
        List<Integer> out = new ArrayList<>();
        for (Row r : rows) if (shown(r) && !out.contains(r.group)) out.add(r.group);
        return Collections.unmodifiableList(out);
    }

    int[] count(int group) {
        int done = 0, all = 0;
        for (Row r : rows) if ((group == 0 || r.group == group) && shown(r)) { all++; if (r.entered) done++; }
        return new int[]{done, all};
    }


    boolean complete() { int[] n = count(0); return n[1] > 0 && n[0] == n[1]; }


    int classesDone() {
        boolean[] done = new boolean[CLASSES]; int n = 0;
        for (Row r : rows) if (r.rule == R_CLASS_HELL && r.entered && !done[(int) r.arg]) { done[(int) r.arg] = true; n++; }
        return n;
    }



    long progress(Row r) {
        if (r.entered) return -1;
        long have = r.rule == R_KILLS ? counters[KILLS] : r.rule == R_SALES ? counters[SALES] : r.rule == R_GOLD ? counters[GOLD]
            : r.rule == R_EVERY_CLASS ? classesDone() : r.rule == R_UNIQUES ? uniques : r.rule == R_BOSSES ? bosses : r.rule == R_QUESTS ? quests
            : r.rule == R_LOOKS ? counters[LOOKS] : r.rule == R_DYES ? counters[DYES] : -1;
        return have > 0 ? have : -1;
    }


    static String widestCount(Row r, Locale language) {
        boolean counting = r.rule == R_KILLS || r.rule == R_SALES || r.rule == R_GOLD || r.rule == R_EVERY_CLASS || r.rule == R_UNIQUES || r.rule == R_BOSSES || r.rule == R_QUESTS
            || r.rule == R_LOOKS || r.rule == R_DYES;
        return counting && r.arg >= 1 ? progressText(r.arg - 1, r.arg, language) : null;
    }

    static float fill(long value, long target) { return target <= 0 ? 1f : (float) Math.min(1.0, Math.max(0.0, (double) value / target)); }


    static String date(long when, Locale language, TimeZone zone) {
        DateFormat f = DateFormat.getDateInstance(DateFormat.SHORT, language);
        f.setTimeZone(zone);
        return f.format(new Date(when * 1000));
    }

    static String progressText(long value, long target, Locale language) {
        NumberFormat n = NumberFormat.getIntegerInstance(language);
        return n.format(value) + " / " + n.format(target);
    }


    static boolean named(Row r) { return r.rule == R_OWN || r.rule == R_MERC; }

    String[] gameNames(Row r) {
        if (named(r)) return new String[]{rowNames.get(r.id), ""};
        if (r.rule == R_CLASS_HELL) return new String[]{classNames[(int) r.arg], difficultyNames[DIFFICULTIES - 1]};
        if (r.rule == R_EVERY_CLASS) return new String[]{"", difficultyNames[DIFFICULTIES - 1]};
        if (r.rule == R_DIFF) return new String[]{"", difficultyNames[(int) r.arg]};
        if (r.rule == R_EVENT && r.arg == E_DRY) return new String[]{"", difficultyNames[1]};
        return new String[]{"", ""};
    }
}
