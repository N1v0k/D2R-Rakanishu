package com.xiaoboluo.d2r;

import java.util.Arrays;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;




public class BookLootTest {
    static int failed;
    static void check(String what, Object got, Object want) {
        if (String.valueOf(got).equals(String.valueOf(want))) return;
        failed++;
        System.out.println("FAIL " + what + ": got " + got + ", want " + want);
    }

    static String names(String key, String en, String de) {
        StringBuilder b = new StringBuilder("\"" + key + "\":[\"" + en + "\",\"" + de + "\"");
        for (int i = 2; i < 13; i++) b.append(",\"").append(en).append("\"");
        return b.append("]").toString();
    }

    public static void main(String[] args) {
        String set = "{\"tc\":[[1,0,0,0,0,0,0,0,7,[1,0,1]]],"
            + "\"item\":[[\"gld\",\"gld\",0,\"g\",0],[\"r01\",\"r01\",1,\"r\",0],[\"axe\",\"axe\",4,\"w\",1],[\"cap\",\"cap\",1,\"a\",1],[\"rin\",\"rin\",1,\"y\",1],"
            + "[\"hp1\",\"hp1\",1,\"p\",0],[\"gem\",\"gem\",1,\"e\",0],[\"zzz\",\"nameless\",1,\"q\",0]],"
            + "\"unique\":[[7,2,1,1,\"Gnasher\",7]],\"set\":[[9,3,1,1,\"Capset\",0]],"
            + "\"ratio\":[[0,0,400,1,6400,100,2,3200,160,2,5600,34,3,192],[0,1,240,3,6400,80,3,3200,120,3,5600,17,6,192],"
            + "[1,0,400,1,6400,100,2,3200,160,2,5600,34,3,192],[1,1,240,3,6400,80,3,3200,120,3,5600,17,6,192]],"
            + "\"mon\":{\"10\":[0,-1,0,1]},\"su\":{\"3\":[-1,0,0,1]},\"extra\":{\"11\":[\"Egg\",\"egg\",[5,40,70],[10]]}}";
        String json = "{\"v\":1,\"game\":\"x\",\"names\":{" + String.join(",", names("r01", "El Rune", "El-Rune"), names("axe", "Axe", "Axt"),
            names("cap", "Cap", "Kappe"), names("rin", "Ring", "Ring"), names("hp1", "Minor Healing", "Schwacher Heiltrank"), names("gem", "Chipped Ruby", "Rubinsplitter"),
            names("Gnasher", "The Gnasher", "Der Nager"), names("Capset", "Sigon's Visor", "Sigons Visier"), names("gld", "Gold", "Gold"))
            + "},\"sets\":{\"rotw\":" + set + ",\"lod\":" + set + "}}";
        BookData data = BookData.parse(json);
        BookData.Set s = data.sets.get("rotw");
        List<DropCalc.Drop> drops = Arrays.asList(new DropCalc.Drop(DropCalc.ITEM, 0, 0.9), new DropCalc.Drop(DropCalc.ITEM, 2, 0.5),
            new DropCalc.Drop(DropCalc.ITEM, 1, 0.25), new DropCalc.Drop(DropCalc.ITEM, 3, 0.2), new DropCalc.Drop(DropCalc.ITEM, 4, 0.1),
            new DropCalc.Drop(DropCalc.ITEM, 5, 0.08), new DropCalc.Drop(DropCalc.ITEM, 6, 0.05), new DropCalc.Drop(DropCalc.ITEM, 7, 0.04),
            new DropCalc.Drop(DropCalc.UNIQUE, 0, 0.001), new DropCalc.Drop(DropCalc.SET, 0, 0.0005));
        List<BookLoot.Row> rows = BookLoot.rows(data, s, drops, 1);
        StringBuilder b = new StringBuilder();
        for (BookLoot.Row r : rows) b.append(r.group).append(':').append(r.name).append(r.id >= 0 ? "#" + r.id : "").append(' ');
        check("rows: groups, German names, no gold, no nameless item", b.toString().trim(),
            "5:Axt 2:El-Rune 6:Kappe 4:Ring 7:Schwacher_Heiltrank 3:Rubinsplitter 0:Der_Nager#7 1:Sigons_Visier#9".replace('_', ' '));
        check("picture code of a unique is its base", rows.get(6).code, "axe");

        List<BookLoot.Line> lines = BookLoot.lines(rows, BookLoot.defaultOpen(), Collections.emptySet());
        b.setLength(0);
        for (BookLoot.Line l : lines) b.append(l.row == null ? "[" + l.group + "x" + l.count + "]" : l.row.name).append('|');
        check("lines: headers, uniques / sets / runes open", b.toString(), "[0x1]|Der Nager|[1x1]|Sigons Visier|[2x1]|El-Rune|[3x1]|[4x1]|[5x1]|[6x1]|[7x1]|");
        boolean[] open = new boolean[BookLoot.GROUPS];
        open[BookLoot.WEAPONS] = true;
        lines = BookLoot.lines(rows, open, new HashSet<>(Arrays.asList(DropCalc.UNIQUE + ":0")));
        check("found first, a group opened", lines.get(0).row.name + "|" + lines.size(), "Der Nager|10");

        check("ratio small", BookLoot.ratio(0.75, CombatModel.L_DE), "1 : 1,3");
        check("ratio English", BookLoot.ratio(0.75, CombatModel.L_EN), "1 : 1.3");
        check("ratio sure", BookLoot.ratio(1, CombatModel.L_DE), "1 : 1");
        check("ratio whole", BookLoot.ratio(0.5, CombatModel.L_EN), "1 : 2");
        check("ratio grouping German", BookLoot.ratio(1.0 / 12345.4, CombatModel.L_DE), "1 : 12.345");
        check("ratio grouping English", BookLoot.ratio(1.0 / 1234567, CombatModel.L_EN), "1 : 1,234,567");

        check("class of a monster", BookLoot.dropClass(s, "M10", 0), 0);
        check("class of a monster, none in nightmare", BookLoot.dropClass(s, "M10", 1), -1);
        check("class of a superunique", BookLoot.dropClass(s, "S3", 2), 0);
        check("no such kind", BookLoot.dropClass(s, "M99", 0), -1);
        check("bad key", BookLoot.dropClass(s, "X", 0), -1);
        CombatModel.Enemy place = new CombatModel.Enemy();
        place.level = 66;
        check("listed kind in normal: its own level", BookLoot.level(s, "M11", 0, null, place), 5);
        check("listed kind in hell: the place's", BookLoot.level(s, "M11", 2, null, place), 66);
        check("listed kind in hell without a place", BookLoot.level(s, "M11", 2, null, null), 70);
        CombatModel.Enemy e = new CombatModel.Enemy();
        e.level = 33;
        check("an encounter's level", BookLoot.level(s, "M10", 1, e, place), 33);

        List<int[]> m = BookLoot.match(data, s, CombatModel.fold("nager"));
        check("search in German", m.size() + ":" + m.get(0)[0] + ":" + m.get(0)[1], "1:" + DropCalc.UNIQUE + ":0");
        check("search in English, accents aside", BookLoot.match(data, s, CombatModel.fold("SIGON")).size(), 1);
        check("no gold in the search", BookLoot.match(data, s, "gold").size(), 0);
        check("one letter searches nothing", BookLoot.match(data, s, "a").size(), 0);

        if (failed > 0) { System.out.println("BookLootTest: " + failed + " failed"); System.exit(1); }
        System.out.println("BookLootTest OK");
    }
}
