package com.xiaoboluo.d2r;

import java.util.Locale;














final class GameLang {
    static final String EN = "en", DE = "de", ES = "es", MX = "mx", RU = "ru", KO = "ko", ZH = "zh", PT = "pt";
    static final int LANGS = 13;
    private static volatile int index = -1;
    private static volatile int version;
    private GameLang() {}




    static String groupOf(int lang) {
        switch (lang) {
            case 8: case 9: return ZH;
            case 2: return ES;
            case 10: return MX;
            case 7: return RU;
            case 5: return KO;
            case 1: return DE;
            case 12: return PT;
            default: return EN;
        }
    }





    static int fromTag(String tag) {
        if (tag == null || tag.isEmpty()) return 0;
        String[] parts = tag.replace('_', '-').toLowerCase(Locale.ROOT).split("-");
        String l = parts[0];
        switch (l) {
            case "zh": case "yue": case "wuu": case "hak": case "nan": case "cmn": case "gan": case "hsn": case "lzh": return 8;
            case "de": return 1;
            case "ko": return 5;
            case "ru": return 7;
            case "pt": return 12;
            case "es":
                for (int i = 1; i < parts.length; i++) {
                    if (parts[i].length() == 4) continue;
                    return parts[i].equals("es") ? 2 : 10;
                }
                return 2;
            default: return 0;
        }
    }



    static synchronized boolean set(int lang) {
        int l = lang >= 0 && lang < LANGS ? lang : -1;
        if (l == index) return false;
        index = l;
        version++;
        return true;
    }

    static boolean known() { return index >= 0; }
    static int index() { return index; }


    static int version() { return version; }


    static String code() { return code(index, Locale.getDefault().toLanguageTag()); }
    static String code(int lang, String phoneTag) { return groupOf(lang >= 0 ? lang : fromTag(phoneTag)); }



    static boolean german() { return DE.equals(code()); }



    static Locale locale(String code) { return MX.equals(code) ? Locale.forLanguageTag("es-419") : Locale.forLanguageTag(code); }




    static final class Watch {
        private int seen = -1;
        boolean changed() { int v = version; if (v == seen) return false; seen = v; return true; }


        void reset() { seen = -1; }
    }
}
