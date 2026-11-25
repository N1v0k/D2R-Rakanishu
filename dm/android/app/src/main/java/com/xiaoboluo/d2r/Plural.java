package com.xiaoboluo.d2r;





public final class Plural {
    public static final int ONE = 0, FEW = 1, MANY = 2, OTHER = 3;

    public static final int WHOLE = 0, ZERO_DECIMALS = 1, DECIMALS = 2;
    private static final String[] KINDS = { "one=", "few=", "many=", "other=" };

    private Plural() {}



    static int category(String lang, long n, int dec) {
        if (n < 0) n = -n;
        switch (lang) {
            case "en": case "de": return n == 1 && dec == WHOLE ? ONE : OTHER;
            case "es": return n == 1 && dec != DECIMALS ? ONE : OTHER;
            case "pt": return n <= 1 ? ONE : OTHER;
            case "ru": {
                if (dec != WHOLE) return OTHER;
                long a = n % 10, b = n % 100;
                return a == 1 && b != 11 ? ONE : a >= 2 && a <= 4 && (b < 12 || b > 14) ? FEW : MANY;
            }
            default: return OTHER;
        }
    }


    public static boolean has(String cell) {
        if (cell == null || cell.length() < 4 || cell.charAt(2) != '|') return false;
        for (String k : KINDS) if (cell.startsWith(k, 3)) return true;
        return false;
    }



    public static String pick(String cell, long n) {
        if (!has(cell)) return cell;
        int want = n < 0 ? OTHER : category(cell.substring(0, 2), n, WHOLE);
        String hit = null, other = null;
        for (String part : cell.substring(3).split("\\|", -1)) {
            for (int k = 0; k < KINDS.length; k++) {
                if (!part.startsWith(KINDS[k])) continue;
                String text = part.substring(KINDS[k].length());
                if (k == want) hit = text;
                if (k == OTHER) other = text;
            }
        }
        return hit != null ? hit : other != null ? other : "";
    }


    public static String format(java.util.Locale locale, String cell, Object... args) { return String.format(locale, pick(cell, count(args)), args); }


    public static long count(Object... args) {
        for (Object a : args) if (a instanceof Integer || a instanceof Long || a instanceof Short || a instanceof Byte) return ((Number) a).longValue();
        return -1;
    }
}
