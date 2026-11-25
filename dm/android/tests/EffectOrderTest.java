package com.xiaoboluo.d2r;

import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;





public class EffectOrderTest {
    private static int checks, fails;
    private static void check(boolean ok, String what) { checks++; if (!ok) { fails++; System.err.println("FAIL " + what); } }
    private static void equal(Object want, Object actual, String what) { checks++; if (!want.equals(actual)) { fails++; System.err.println("FAIL " + what + ": want=" + want + " actual=" + actual); } }

    static Map<Integer, Integer> now(int... stateKind) { Map<Integer, Integer> m = new LinkedHashMap<>(); for (int i = 0; i + 1 < stateKind.length; i += 2) m.put(stateKind[i], stateKind[i + 1]); return m; }

    public static void main(String[] args) {
        List<Integer> o = EffectOrder.next(null, now(198, 2, 2, 0, 128, 1, 100, 3));
        equal(Arrays.asList(2, 128, 198, 100), o, "first: by kind, then state id");


        o = EffectOrder.next(null, now(2, 0, 198, 2));
        equal(Arrays.asList(2, 198), o, "start: poison above the hex");
        o = EffectOrder.next(o, now(198, 2));
        equal(Arrays.asList(198), o, "poison ends: the hex moves up once");
        o = EffectOrder.next(o, now(198, 2, 2, 0));
        equal(Arrays.asList(198, 2), o, "poison again: it goes last, the hex stays in slot 1");
        o = EffectOrder.next(o, now(198, 2));
        equal(Arrays.asList(198), o, "poison ends again: the hex does not move");
        o = EffectOrder.next(o, now(2, 0, 198, 2));
        equal(Arrays.asList(198, 2), o, "and back: still slot 1 for the hex");

        o = EffectOrder.next(null, now(128, 1, 100, 3));
        for (int i = 0; i < 6; i++) {
            o = EffectOrder.next(o, i % 2 == 0 ? now(128, 1, 100, 3, 2, 0, 11, 0) : now(128, 1, 100, 3));
            equal(128, o.get(0), "shrine stays first, round " + i); equal(100, o.get(1), "potion stays second, round " + i);
        }
        o = EffectOrder.next(Arrays.asList(128, 100), now(128, 1, 100, 3, 11, 0, 2, 0));
        equal(Arrays.asList(128, 100, 2, 11), o, "two new at once: among themselves by kind, then state id");
        equal(Arrays.asList(), EffectOrder.next(Arrays.asList(1, 2), now()), "everything gone: empty");
        o = EffectOrder.next(Arrays.asList(5, 6, 7), now(5, 2, 7, 2));
        equal(Arrays.asList(5, 7), o, "an effect above ends: the ones below move up one place, nothing else");



        float u = 1, top = 0, bottom = 34 + 2 * 40, reach = 34, step = EffectLayout.step(reach, u);
        int cap = EffectLayout.capacity(top, bottom, u, 0, step, reach, 0, null);
        equal(6, cap, "test layout: two columns of three");
        EffectLayout.Cells cells = new EffectLayout.Cells();

        Map<Integer, Integer> buffs = now(30, 2, 31, 2, 32, 2, 33, 2, 34, 4, 35, 4);
        List<Integer> stable = EffectOrder.next(null, buffs);
        List<Integer> drawn = EffectOrder.shown(stable, buffs, 0, cap);
        equal(Arrays.asList(30, 31, 32, 33, 34, 35), drawn, "full but fitting: drawn as the stable order");
        equal(0, EffectLayout.place(cells, drawn.size(), top, bottom, u, 0, step, reach, 0, null).more, "full but fitting: no +N");
        for (int round = 0; round < 3; round++) {


            Map<Integer, Integer> hit = now(30, 2, 31, 2, 32, 2, 33, 2, 34, 4, 35, 4, 2, 0);
            stable = EffectOrder.next(stable, hit);
            equal(6, stable.indexOf(2), "round " + round + ": the stable order still puts the poison last");
            drawn = EffectOrder.shown(stable, hit, 0, cap);
            EffectLayout.Cells c = EffectLayout.place(cells, drawn.size(), top, bottom, u, 0, step, reach, 0, null);
            equal(5, c.shown, "round " + round + ": overflow draws five");
            check(drawn.indexOf(2) >= 0 && drawn.indexOf(2) < c.shown, "round " + round + ": the poison is drawn, not behind +N (at " + drawn.indexOf(2) + ")");
            equal(Arrays.asList(30, 31, 32, 33), drawn.subList(0, 4), "round " + round + ": the buffs above keep their cells");
            equal(Arrays.asList(34, 35), drawn.subList(c.shown, drawn.size()), "round " + round + ": the pushed-out buffs behind +N in stable order");

            stable = EffectOrder.next(stable, buffs);
            equal(Arrays.asList(30, 31, 32, 33, 34, 35), EffectOrder.shown(stable, buffs, 0, cap), "round " + round + ": poison gone, every buff back in its cell");
        }

        Map<Integer, Integer> both = now(30, 2, 31, 2, 32, 2, 33, 2, 34, 4, 35, 4, 9, 0, 2, 0);
        stable = EffectOrder.next(Arrays.asList(30, 31, 32, 33, 34, 35), both);
        drawn = EffectOrder.shown(stable, both, 0, cap);
        equal(Arrays.asList(30, 31, 32, 2, 9, 33, 34, 35), drawn, "two debuffs: both drawn after the first three buffs, the rest behind +N");

        Map<Integer, Integer> curses = now(30, 2, 9, 0, 10, 0, 11, 0, 12, 0, 13, 0, 14, 0);
        drawn = EffectOrder.shown(Arrays.asList(30, 9, 10, 11, 12, 13, 14), curses, 0, cap);
        equal(Arrays.asList(9, 10, 11, 12, 13, 30, 14), drawn, "only debuffs fit: the first five debuffs drawn, the buff and the last debuff behind +N");

        List<Integer> few = Arrays.asList(30, 2);
        check(EffectOrder.shown(few, now(30, 2, 2, 0), 0, cap) == few, "fits: the stable order itself");
        check(EffectOrder.shown(few, now(30, 2, 2, 0), 0, 0) == few, "no cells: the stable order itself");

        equal(cap - 1, EffectLayout.place(cells, cap + 3, top, bottom, u, 0, step, reach, 0, null).shown, "capacity matches place");
        System.out.println("EffectOrderTest: " + checks + " checks, " + fails + " failed");
        if (fails > 0) System.exit(1);
    }
}
