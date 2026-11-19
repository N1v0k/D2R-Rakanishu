package com.xiaoboluo.d2r;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Map;








final class EffectOrder {
    private EffectOrder() {}



    static List<Integer> next(List<Integer> prev, Map<Integer, Integer> kinds) {
        List<Integer> out = new ArrayList<>(kinds.size());
        if (prev != null) for (Integer s : prev) if (kinds.containsKey(s) && !out.contains(s)) out.add(s);
        List<Integer> fresh = new ArrayList<>();
        for (Integer s : kinds.keySet()) if (!out.contains(s)) fresh.add(s);
        Collections.sort(fresh, (a, b) -> { int ka = kinds.get(a), kb = kinds.get(b); return ka != kb ? ka - kb : a - b; });
        out.addAll(fresh);
        return out;
    }








    static List<Integer> shown(List<Integer> base, Map<Integer, Integer> kinds, int debuff, int capacity) {
        if (capacity <= 0 || base.size() <= capacity) return base;
        int slots = capacity - 1, debuffs = 0;
        for (Integer s : base) if (isDebuff(kinds, s, debuff)) debuffs++;
        int takeDebuffs = Math.min(debuffs, slots), others = slots - takeDebuffs;
        List<Integer> drawn = new ArrayList<>(base.size()), rest = new ArrayList<>(base.size() - slots);
        for (Integer s : base) {
            boolean d = isDebuff(kinds, s, debuff);
            if (d && takeDebuffs > 0) { takeDebuffs--; drawn.add(s); }
            else if (!d && others > 0) { others--; drawn.add(s); }
            else rest.add(s);
        }
        drawn.addAll(rest);
        return drawn;
    }

    private static boolean isDebuff(Map<Integer, Integer> kinds, Integer s, int debuff) { Integer k = kinds.get(s); return k != null && k == debuff; }
}
