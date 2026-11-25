package com.xiaoboluo.d2r;

import java.util.Map;
import java.util.TreeSet;
import java.util.function.Predicate;




final class InvPictures {
    private final Map<String, String> base, unique, set;
    private final Map<String, Integer> variants;


    InvPictures(Map<String, String> base, Map<String, String> unique, Map<String, String> set, Map<String, Integer> variants) {
        this.base = base; this.unique = unique; this.set = set; this.variants = variants;
    }



    String name(String code, int quality, int row, int gfx) {
        String own = row < 0 ? null : quality == 7 ? unique.get(Integer.toString(row)) : quality == 5 ? set.get(Integer.toString(row)) : null;
        if (own != null && !own.isEmpty()) return own;
        String b = base.get(code);
        if (b == null || b.isEmpty()) return null;
        Integer n = variants.get(b);
        return gfx >= 0 && n != null && gfx < n ? b + (gfx + 1) : b;
    }



    String missing(Predicate<String> has) {
        if (base.isEmpty()) return "inv.json";
        TreeSet<String> names = new TreeSet<>(base.values());
        names.addAll(unique.values()); names.addAll(set.values());
        for (Map.Entry<String, Integer> v : variants.entrySet()) {
            if (v.getValue() < 1) return "inv.json";
            for (int k = 1; k <= v.getValue(); k++) names.add(v.getKey() + k);
        }
        for (String n : names) if (n.isEmpty() || !has.test(n)) return "inv/" + n + ".png";
        return null;
    }
}
