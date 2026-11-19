package com.xiaoboluo.d2r;

import java.nio.charset.StandardCharsets;
import java.util.IllegalFormatException;
import java.util.Locale;












final class LevelLabel {
    static final int MAX_CHARS = 24, MAX_BYTES = 64;
    private LevelLabel() {}


    static String word(byte[] utf8) {
        if (utf8 == null || utf8.length == 0 || utf8.length > MAX_BYTES) return null;
        String s = new String(utf8, StandardCharsets.UTF_8).trim();
        if (s.isEmpty() || s.length() > MAX_CHARS || s.indexOf('�') >= 0) return null;
        for (int i = 0; i < s.length(); i++) if (Character.isISOControl(s.charAt(i))) return null;
        return s;
    }



    static String text(String word, String fallbackWord, String format, int level, int percent) {
        String w = word != null ? word : fallbackWord != null ? fallbackWord : "";
        if (format != null) {
            try { return String.format(Locale.ROOT, format, w, level, percent); }
            catch (IllegalFormatException e) {                                                                       }
        }
        return (w.isEmpty() ? "" : w + " ") + level + " · " + percent + "%";
    }
}
