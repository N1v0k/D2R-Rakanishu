package com.xiaoboluo.d2r;

import java.util.Locale;






public class GameLangTest {
    private static int cases;
    private static void equal(Object want, Object actual) {
        cases++;
        if (!want.equals(actual)) throw new AssertionError("want=" + want + " actual=" + actual);
    }
    public static void main(String[] args) {

        String[] group = {"en", "de", "es", "en", "en", "ko", "en", "ru", "zh", "zh", "mx", "en", "pt"};
        for (int i = 0; i < group.length; i++) equal(group[i], GameLang.groupOf(i));
        equal("en", GameLang.groupOf(-1));
        equal("en", GameLang.groupOf(13));


        equal(1, GameLang.fromTag("de-AT"));
        equal(1, GameLang.fromTag("de"));
        equal(2, GameLang.fromTag("es"));
        equal(2, GameLang.fromTag("es-ES"));
        equal(2, GameLang.fromTag("es-Latn-ES"));
        equal(10, GameLang.fromTag("es-MX"));
        equal(10, GameLang.fromTag("es-419"));
        equal(10, GameLang.fromTag("es-US"));
        equal(10, GameLang.fromTag("es_MX"));
        equal(8, GameLang.fromTag("zh-Hant-TW"));
        equal(8, GameLang.fromTag("yue-Hant-HK"));
        equal(8, GameLang.fromTag("cmn"));
        equal(7, GameLang.fromTag("ru-RU"));
        equal(5, GameLang.fromTag("ko-KR"));
        equal(12, GameLang.fromTag("pt-BR"));
        equal(12, GameLang.fromTag("pt-PT"));
        equal(12, GameLang.fromTag("pt"));
        equal(0, GameLang.fromTag("fr-FR"));
        equal(0, GameLang.fromTag("ja-JP"));
        equal(0, GameLang.fromTag("en-US"));
        equal(0, GameLang.fromTag(""));
        equal(0, GameLang.fromTag(null));


        equal("en", GameLang.code(0, "de-DE"));
        equal("de", GameLang.code(1, "en-US"));
        equal("ru", GameLang.code(7, "de-DE"));
        equal("de", GameLang.code(-1, "de-DE"));
        equal("en", GameLang.code(-1, "en-US"));
        equal("mx", GameLang.code(-1, "es-MX"));
        equal("pt", GameLang.code(12, "en-US"));
        equal("pt", GameLang.code(-1, "pt-BR"));


        GameLang.Watch w = new GameLang.Watch();
        equal(true, w.changed());
        equal(false, w.changed());
        equal(false, GameLang.known());
        int v = GameLang.version();
        equal(true, GameLang.set(1));
        equal(true, GameLang.known());
        equal("de", GameLang.code());
        equal(true, GameLang.german());
        equal(v + 1, GameLang.version());
        equal(false, GameLang.set(1));
        equal(v + 1, GameLang.version());
        equal(true, w.changed());
        equal(false, w.changed());
        w.reset();
        equal(true, w.changed());
        equal(false, w.changed());
        equal(true, GameLang.set(0));
        equal(false, GameLang.german());
        equal("en", GameLang.code());
        equal(true, GameLang.set(7));
        equal(false, GameLang.german());
        equal("ru", GameLang.code());
        equal(true, GameLang.set(99));
        equal(false, GameLang.known());
        equal(false, GameLang.set(-1));
        equal(v + 4, GameLang.version());


        equal(Locale.forLanguageTag("es-419"), GameLang.locale("mx"));
        equal("de", GameLang.locale("de").getLanguage());
        equal("ko", GameLang.locale("ko").getLanguage());
        equal("zh", GameLang.locale("zh").getLanguage());
        System.out.println("GameLang OK (" + cases + " cases)");
    }
}
