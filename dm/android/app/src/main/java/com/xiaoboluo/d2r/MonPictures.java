package com.xiaoboluo.d2r;

import java.util.Map;
import java.util.Set;
import java.util.TreeSet;
import java.util.function.Predicate;
import java.util.regex.Pattern;




final class MonPictures {
    static final Pattern NAME = Pattern.compile("[a-z0-9_]{1,40}");

    static final int BIG_MAX = 512, SMALL_MAX = 192;
    private final Map<String, String> pictures;
    private final Set<String> none;



    MonPictures(Map<String, String> pictures, Set<String> none) {
        for (Map.Entry<String, String> e : pictures.entrySet()) {
            if (e.getValue() == null || !NAME.matcher(e.getValue()).matches()) throw new IllegalArgumentException("picture name of " + e.getKey());
            if (none.contains(e.getKey())) throw new IllegalArgumentException("look key " + e.getKey() + " both with and without a picture");
        }
        this.pictures = pictures; this.none = none;
    }


    String name(String look) { return look == null ? null : pictures.get(look); }


    boolean known(String look) { return look != null && (look.isEmpty() || pictures.containsKey(look) || none.contains(look)); }



    String missing(Predicate<String> has) {
        if (pictures.isEmpty()) return "mon.json";
        for (String n : new TreeSet<>(pictures.values())) {
            for (String f : new String[]{"mon/" + n + ".webp", "mon/s/" + n + ".webp"}) if (!has.test(f)) return f;
        }
        return null;
    }

    static boolean fits(int width, int height, boolean small) {
        int max = small ? SMALL_MAX : BIG_MAX;
        return width > 0 && height > 0 && width <= max && height <= max;
    }
}
