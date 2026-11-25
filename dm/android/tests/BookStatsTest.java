package com.xiaoboluo.d2r;

import java.util.List;




final class BookStatsTest {
    private static void check(boolean value, String what) { if (!value) throw new AssertionError(what); }
    private static String text(List<BookStats.Line> lines) { StringBuilder s = new StringBuilder(); for (BookStats.Line l : lines) s.append(l.text).append('\n'); return s.toString(); }


    static BookText texts(String root, String column) throws java.io.IOException {
        java.util.List<String> lines = java.nio.file.Files.readAllLines(java.nio.file.Paths.get(root, "dm/i18n/ui_text.tsv"), java.nio.charset.StandardCharsets.UTF_8);
        int col = java.util.Arrays.asList(lines.get(0).split("\t", -1)).indexOf(column);
        java.util.Map<String, String> m = new java.util.HashMap<>();
        for (String l : lines) { String[] f = l.split("\t", -1); if (f.length > col && !f[col].isEmpty()) m.put(f[0], f[col]); }
        return new BookText(m::get);
    }

    public static void main(String[] args) throws Exception {
        BookText de = texts(args[0], "Deutsch"), en = texts(args[0], "English");
        String hero = "hero\tTest\t1\t30\t1\t2\t500\t200\t0\t0\t0\t8\t1\t1\t43\t0\t0\n";
        String row = "enemy\t4\tkey\t0\t1\t5\t30\t700\t40.0\t18\t25\t0.2\t10\t1\t0\t1\t2\t0,100,0,0,0,0\tsu:12\tS12\t0\n";
        String stats = "stats\t45\t900\t1200\t0\t0\t100\t0,10,20,300;2,30,40,0\t0,4,90,90,100,75;2,3,5,9,50,50;3,1,7,7,0,0\t50,50,62\t9,2\tur\n";
        CombatModel.Stats s = CombatModel.parse(hero + row + stats).stats;
        String v = text(BookStats.values(s, new int[]{0, 50, 0, -25, 100, 0}, CombatModel.L_DE, de));
        check(v.startsWith("Stufe 45 · Leben 900–1200\n"), v);
        check(!v.contains("Verteidigung") && !v.contains("Blocken"), "no zero defense or block: " + v);
        check(v.contains("Angriff 1: 10–20 Schaden · Angriffswert 300\n  + Gift 90 über 3 s\n"), "poison under attack 1: " + v);
        check(v.contains("Fertigkeit: 30–40 Schaden\n  + Kälte 5–9 · verlangsamt 2 s · Chance 50 %\n"), "cold with its chance under the skill: " + v);
        check(!v.contains("Feuer 7"), "a slot without chance is no line: " + v);
        check(v.contains("Widerstände: Feuer 50 % · Kälte -25 %\n") && v.contains("Immun: Gift\n"), "resistances: " + v);
        check(!v.contains("raub"), "full leech is no line: " + v);
        stats = stats.replace("\t0\t0\t100\t", "\t350\t20\t75\t");
        s = CombatModel.parse(hero + row + stats).stats;
        v = text(BookStats.values(s, new int[6], CombatModel.L_EN, en));
        check(v.contains("Defense 350 · Block 20 %\n") && v.contains("Life and mana leech work at 75 %\n") && !v.contains("Resistances"), v);
        BookStats.Names names = new BookStats.Names() {
            public String get(String key) { return key.equals("@skill:50") ? "Frost Nova" : key.equals("uniquefireenchanted") ? "Fire Enchanted" : key; }
            public boolean has(String key) { return key.equals("@skill:50") || key.equals("uniquefireenchanted"); }
        };
        List<String> a = BookStats.abilities(s, en, names);
        check(a.toString().equals("[Frost Nova, Fire Enchanted, Ranged attacker, Undead]"), a.toString());
        check(BookStats.values(null, null, CombatModel.L_DE, de).isEmpty() && BookStats.abilities(null, en, names).isEmpty(), "nothing without values");
        System.out.println("BookStatsTest: values, elemental lines, resistances, leech and abilities passed");
    }
}
