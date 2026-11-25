package com.xiaoboluo.d2r;

import java.util.Locale;



final class BookText {
    interface Source { String get(String key); }
    private final Source source;

    BookText(Source source) { this.source = source; }



    String t(String key, Object... args) {
        String f = source.get(key);
        if (f == null) throw new IllegalArgumentException("no text " + key);
        return String.format(Locale.ROOT, Plural.pick(f, Plural.count(args)), args);
    }
}
