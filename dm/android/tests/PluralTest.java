package com.xiaoboluo.d2r;



public class PluralTest {
    static int fails;
    static void check(boolean ok, String what) { System.out.println((ok ? "ok   " : "FAIL ") + what); if (!ok) fails++; }

    public static void main(String[] a) {
        Object[][] c = { { "en", 0L, Plural.WHOLE, Plural.OTHER }, { "en", 1L, Plural.WHOLE, Plural.ONE }, { "en", 2L, Plural.WHOLE, Plural.OTHER }, { "en", 1L, Plural.ZERO_DECIMALS, Plural.OTHER },
            { "en", 21L, Plural.WHOLE, Plural.OTHER }, { "de", 1L, Plural.WHOLE, Plural.ONE }, { "de", 11L, Plural.WHOLE, Plural.OTHER }, { "es", 1L, Plural.WHOLE, Plural.ONE },
            { "es", 0L, Plural.WHOLE, Plural.OTHER }, { "es", 1000000L, Plural.WHOLE, Plural.OTHER }, { "pt", 0L, Plural.WHOLE, Plural.ONE }, { "pt", 1L, Plural.WHOLE, Plural.ONE },
            { "pt", 1L, Plural.ZERO_DECIMALS, Plural.ONE }, { "pt", 2L, Plural.WHOLE, Plural.OTHER }, { "ru", 0L, Plural.WHOLE, Plural.MANY }, { "ru", 1L, Plural.WHOLE, Plural.ONE },
            { "ru", 2L, Plural.WHOLE, Plural.FEW }, { "ru", 4L, Plural.WHOLE, Plural.FEW }, { "ru", 5L, Plural.WHOLE, Plural.MANY }, { "ru", 11L, Plural.WHOLE, Plural.MANY },
            { "ru", 12L, Plural.WHOLE, Plural.MANY }, { "ru", 14L, Plural.WHOLE, Plural.MANY }, { "ru", 21L, Plural.WHOLE, Plural.ONE }, { "ru", 22L, Plural.WHOLE, Plural.FEW },
            { "ru", 25L, Plural.WHOLE, Plural.MANY }, { "ru", 101L, Plural.WHOLE, Plural.ONE }, { "ru", 111L, Plural.WHOLE, Plural.MANY }, { "ru", 112L, Plural.WHOLE, Plural.MANY },
            { "ru", 1L, Plural.ZERO_DECIMALS, Plural.OTHER }, { "zh", 1L, Plural.WHOLE, Plural.OTHER }, { "ko", 1L, Plural.WHOLE, Plural.OTHER },
            { "es", 1L, Plural.ZERO_DECIMALS, Plural.ONE }, { "es", 1L, Plural.DECIMALS, Plural.OTHER }, { "de", 1L, Plural.ZERO_DECIMALS, Plural.OTHER },
            { "en", 1L, Plural.DECIMALS, Plural.OTHER }, { "pt", 1L, Plural.DECIMALS, Plural.ONE }, { "ru", 2L, Plural.ZERO_DECIMALS, Plural.OTHER } };
        for (Object[] x : c)
            check(Plural.category((String) x[0], (Long) x[1], (Integer) x[2]) == (Integer) x[3], "category " + x[0] + " " + x[1] + new String[] { "", ",0", ",3" }[(Integer) x[2]]);
        String de = "de|one=%d Gegenstand nicht bewertet|other=%d Gegenstände nicht bewertet";
        check(Plural.has(de) && !Plural.has("%d Treffer") && !Plural.has("ab|x") && !Plural.has(null), "a stamped cell is recognised, a plain one not");
        check(Plural.pick(de, 1).equals("%d Gegenstand nicht bewertet"), "de one");
        check(Plural.pick(de, 2).equals("%d Gegenstände nicht bewertet"), "de other");
        check(Plural.pick(de, -1).equals("%d Gegenstände nicht bewertet"), "no count: other");
        check(Plural.pick("%d Treffer", 1).equals("%d Treffer"), "a plain text stays");
        check(Plural.pick("ru|one=a|few=b|other=c", 5).equals("c"), "ru many falls back to other");
        check(Plural.pick("ru|one=a|few=b|other=c", 3).equals("b"), "ru few");
        check(Plural.pick("en|one=%d point|other=%d points", 0).equals("%d points"), "English fallback text: 0 is other");
        check(Plural.pick("pt|one=%d ponto|other=%d pontos", 0).equals("%d ponto"), "pt rules: 0 is one");
        check(Plural.count("a", 3, 4L) == 3 && Plural.count(2L) == 2 && Plural.count("x") == -1 && Plural.count() == -1 && Plural.count(1.5) == -1, "the count: the first integer argument");
        check(String.format(java.util.Locale.GERMAN, Plural.pick(de, Plural.count(1)), 1).equals("1 Gegenstand nicht bewertet"), "formatted: 1 Gegenstand nicht bewertet");
        check(Plural.format(java.util.Locale.ENGLISH, "en|one=%1$s more in %2$d second|other=%1$s more in %2$d seconds", "12", 1).equals("12 more in 1 second"), "format picks by the first integer argument, not the first argument");
        check(Plural.format(java.util.Locale.GERMAN, "%d Treffer", 3).equals("3 Treffer"), "format: a plain cell as before");
        if (fails > 0) { System.out.println(fails + " failed"); System.exit(1); }
        System.out.println("PluralTest passed");
    }
}
