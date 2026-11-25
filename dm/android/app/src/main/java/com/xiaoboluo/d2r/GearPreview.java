package com.xiaoboluo.d2r;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;




















final class GearPreview {
    static final String UNIQUE = "Tooltip.UniqueColor", SET = "Tooltip.SetColor", MAGIC = "Tooltip.MagicColor", RARE = "Tooltip.RareColor",
        WHITE = "Tooltip.DefaultColor", GREY = "Tooltip.substringColor", CRAFTED = "Tooltip.CraftedColor", RED = "FontColorRed";
    static final int KIND_SET = 0, KIND_UNIQUE = 1, KIND_WORD = 2;
    static final char CODE = '';




    static final int TOUCH_CLOSE = 0;
    static int touch(float dx, float dy, float swipe) { return Math.abs(dx) > swipe && Math.abs(dx) > Math.abs(dy) * 2 ? (dx < 0 ? 1 : -1) : TOUCH_CLOSE; }

    static final class Line {
        final String text, colour;
        Line(String text, String colour) { this.text = text; this.colour = colour; }
        @Override public String toString() { return colour + ":" + text; }
    }




    static final class Card {
        String title = "", titleColour = WHITE, img = "";
        final List<Line> head = new ArrayList<>(), tail = new ArrayList<>();
        final List<Line> reqs = new ArrayList<>();
        final List<String> reqPrefix = new ArrayList<>();
        final List<String> drop = new ArrayList<>();
        int kind = -1, row = -1, grp = -1;
        boolean word;
    }

    private GearPreview() {}

    static String qualityColour(String q) {
        switch (q) {
            case "unique": return UNIQUE;
            case "set": return SET;
            case "magic": return MAGIC;
            case "rare": return RARE;
            case "crafted": return CRAFTED;
            default: return WHITE;
        }
    }



    static String fill(BuildModel.Names n, String label, boolean de, Object... values) {
        BuildModel.Text t = n == null ? null : n.labels.get(label);
        if (t == null) return null;
        String s = t.get(de);
        for (Object v : values) {
            int at = s.indexOf("%d"), i = s.indexOf("%i");
            if (at < 0 || (i >= 0 && i < at)) at = i;
            if (at < 0) return null;
            s = s.substring(0, at) + v + s.substring(at + 2);
        }
        return s.contains("%d") || s.contains("%i") ? null : s;
    }

    private static String prefix(BuildModel.Names n, String label, boolean de) {
        BuildModel.Text t = n == null ? null : n.labels.get(label);
        if (t == null) return null;
        String s = t.get(de);
        int at = s.indexOf('%');
        return at > 0 ? s.substring(0, at).trim() : null;
    }

    private static void req(Card c, BuildModel.Names n, boolean de, String label, int value) {
        String p = prefix(n, label, de);
        if (p != null && !p.isEmpty()) c.reqPrefix.add(p);
        if (value <= 0) return;
        String s = fill(n, label, de, value);
        if (s != null) c.reqs.add(new Line(s, WHITE));
    }

    private static BuildModel.ItemInfo item(BuildModel.Names n, String key) { return n == null || key == null || key.isEmpty() ? null : n.items.get(key); }


    static Card of(BuildModel.Gear x, BuildModel.Names n, boolean de) {
        Card c = new Card();
        switch (x.kind) {
            case "unique": case "set": {
                BuildModel.ItemInfo it = item(n, (x.kind.equals("unique") ? "u:" : "s:") + x.key), base = it == null ? null : item(n, "i:" + it.code);
                c.titleColour = x.kind.equals("unique") ? UNIQUE : SET;
                c.title = it != null ? it.name(de) : x.key;
                c.img = it != null ? it.img : "";
                if (it != null) { c.drop.add(it.de); c.drop.add(it.en); }
                if (base != null) { c.head.add(new Line(base.name(de), c.titleColour)); c.drop.add(base.de); c.drop.add(base.en); }
                req(c, n, de, "dex", it == null || it.ease || base == null ? 0 : base.dex);
                req(c, n, de, "str", it == null || it.ease || base == null ? 0 : base.str);
                req(c, n, de, "lvl", Math.max(it == null ? 0 : it.lvl, base == null ? 0 : base.lvl));
                if (it != null && it.row >= 0) { c.kind = x.kind.equals("unique") ? KIND_UNIQUE : KIND_SET; c.row = it.row; }
                break;
            }
            case "runeword": {
                BuildModel.ItemInfo it = item(n, "r:" + x.key);
                BuildModel.ItemInfo base = !x.baseItem.isEmpty() ? item(n, "i:" + x.baseItem) : item(n, "t:" + x.baseType);
                boolean single = !x.baseItem.isEmpty() && base != null;
                c.word = true;
                c.titleColour = UNIQUE;
                c.title = it != null ? it.name(de) : x.key;
                c.img = base != null ? base.img : "";
                if (it != null) { c.drop.add(it.de); c.drop.add(it.en); }
                if (base != null) c.head.add(new Line(base.name(de), GREY));
                if (it != null && !it.runes.isEmpty()) {
                    StringBuilder r = new StringBuilder("'");
                    for (String code : it.runes) { BuildModel.ItemInfo rune = item(n, "i:" + code); r.append(rune != null && rune.shortName != null ? rune.shortName.get(de) : code); }
                    c.head.add(new Line(r.append("'").toString(), UNIQUE));
                }
                int eth = x.eth ? 10 : 0;
                req(c, n, de, "dex", single && !(it != null && it.ease) ? base.dex - eth : 0);
                req(c, n, de, "str", single && !(it != null && it.ease) ? base.str - eth : 0);
                req(c, n, de, "lvl", Math.max(it == null ? 0 : it.lvl, single ? base.lvl : 0));
                String sock = x.sockets > 0 ? fill(n, "sock", de, x.sockets) : null;
                if (sock != null) c.tail.add(new Line(sock, MAGIC));
                String e = x.eth ? fill(n, "eth", de) : null;
                if (e != null) c.tail.add(new Line(e, MAGIC));
                if (it != null && it.row >= 0) { c.kind = KIND_WORD; c.row = it.row; c.grp = base != null ? base.gat : -1; }
                break;
            }
            case "item": {
                BuildModel.ItemInfo it = item(n, "i:" + x.key);
                c.title = it != null ? it.name(de) : x.key;
                c.img = it != null ? it.img : x.key;
                if (it != null) base(c, it, n, de);
                break;
            }
            default: {
                BuildModel.ItemInfo t = item(n, "t:" + x.key);
                c.titleColour = qualityColour(x.quality);
                c.title = t != null ? t.name(de) : x.key;
                c.img = t != null ? t.img : "";
                BuildModel.Text q = n == null || x.quality.isEmpty() ? null : n.qualities.get(x.quality);
                if (q != null) c.head.add(new Line(q.get(de), GREY));
            }
        }
        return c;
    }


    private static void base(Card c, BuildModel.ItemInfo it, BuildModel.Names n, boolean de) {
        if (it.ac != null) { String s = fill(n, "def", de, it.ac[0] == it.ac[1] ? String.valueOf(it.ac[0]) : it.ac[0] + "-" + it.ac[1]); if (s != null) c.head.add(new Line(s, WHITE)); }
        if (it.d1 != null) { String s = fill(n, "dmg1", de, it.d1[0], it.d1[1]); if (s != null) c.head.add(new Line(s, WHITE)); }
        if (it.d2 != null) { String s = fill(n, "dmg2", de, it.d2[0], it.d2[1]); if (s != null) c.head.add(new Line(s, WHITE)); }
        req(c, n, de, "dex", it.dex);
        req(c, n, de, "str", it.str);
        req(c, n, de, "lvl", it.lvl);
    }





    static List<Line> gameLines(String raw, Card c) {
        List<Line> out = new ArrayList<>();
        if (raw == null) return out;
        String[] parts = raw.split("\n");
        for (int i = parts.length - 1; i >= 0; i--) {
            String line = parts[i];
            String colour = null;
            StringBuilder text = new StringBuilder();
            String current = null;
            for (int k = 0; k < line.length(); k++) {
                char ch = line.charAt(k);
                if (ch == CODE) { if (k + 1 < line.length()) { current = colourOf(line.charAt(k + 1)); k++; } continue; }
                if (colour == null && !Character.isWhitespace(ch)) colour = current;
                text.append(ch);
            }
            String t = untag(text.toString().trim(), c.drop);
            if (t == null || t.isEmpty() || dropped(t, c.drop) || (c.word && t.startsWith("'"))) continue;
            out.add(new Line(t, colour != null ? colour : MAGIC));
        }
        return out;
    }

    private static boolean dropped(String t, List<String> drop) { for (String d : drop) if (d.equalsIgnoreCase(t)) return true; return false; }




    static String untag(String t, List<String> drop) {
        java.util.regex.Matcher m = java.util.regex.Pattern.compile("\\[[a-z]{2}\\]([^\\[]*)").matcher(t);
        if (!t.startsWith("[") || !m.lookingAt()) return t;
        String first = null;
        m.reset();
        while (m.find()) { String f = m.group(1).trim(); if (first == null) first = f; if (dropped(f, drop)) return null; }
        return first;
    }



    static List<Line> tooltipLines(String text) {
        String[] parts = text.split("\n", -1);
        List<Line> out = new ArrayList<>();
        char colour = '0';
        for (String part : parts) {
            StringBuilder plain = new StringBuilder();
            char first = 0;
            for (int i = 0; i < part.length(); i++) {
                char ch = part.charAt(i);
                int skip = ch == CODE ? 1 : ch == '\u00ff' && i + 1 < part.length() && part.charAt(i + 1) == 'c' ? 2 : 0;
                if (skip > 0 && i + skip < part.length()) { colour = part.charAt(i + skip); i += skip; continue; }
                if (plain.length() == 0 && first == 0) first = colour;
                plain.append(ch);
            }
            String t = plain.toString().trim();
            out.add(0, new Line(t, colourOf(first == 0 ? colour : first)));
        }
        while (!out.isEmpty() && out.get(0).text.isEmpty()) out.remove(0);
        while (!out.isEmpty() && out.get(out.size() - 1).text.isEmpty()) out.remove(out.size() - 1);
        return Collections.unmodifiableList(out);
    }



    static List<Line> blockLines(String block) {
        List<Line> out = new ArrayList<>();
        for (String part : block.split("\n")) {
            StringBuilder plain = new StringBuilder();
            char colour = 0;
            for (int i = 0; i < part.length(); i++) {
                char ch = part.charAt(i);
                int skip = ch == CODE ? 1 : ch == '\u00ff' && i + 1 < part.length() && part.charAt(i + 1) == 'c' ? 2 : 0;
                if (skip > 0 && i + skip < part.length()) { if (colour == 0) colour = part.charAt(i + skip); i += skip; continue; }
                plain.append(ch);
            }
            String t = plain.toString().trim();
            if (!t.isEmpty()) out.add(new Line(t, colour == '3' ? "FontColorPartyGreen" : colour == '1' ? RED : colour == '0' ? WHITE : GREY));
        }
        return Collections.unmodifiableList(out);
    }



    static String colourOf(char ch) {
        switch (ch) {
            case '0': return WHITE;
            case '1': return RED;
            case '2': return SET;
            case '3': return MAGIC;
            case '4': return UNIQUE;
            case '5': return GREY;
            case '8': return CRAFTED;
            case '9': return RARE;
            default: return MAGIC;
        }
    }



    static List<Line> lines(Card c, List<Line> game) {
        List<Line> out = new ArrayList<>();
        out.add(new Line(c.title, c.titleColour));
        out.addAll(c.head);
        boolean gameReqs = false;
        for (Line g : game) for (String p : c.reqPrefix) if (g.text.startsWith(p)) gameReqs = true;
        if (!gameReqs) out.addAll(c.reqs);
        out.addAll(game);
        for (Line t : c.tail) { boolean dup = false; for (Line g : game) if (g.text.equals(t.text)) dup = true; if (!dup) out.add(t); }
        return out;
    }
}
