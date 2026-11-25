package com.xiaoboluo.d2r;

import java.util.ArrayList;
import java.util.BitSet;
import java.util.HashMap;
import java.util.List;
import java.util.Map;









final class DropCalc {
    static final int CAP = 6;
    private static final int N = CAP + 1;



    static final int ITEM = 0, UNIQUE = 1, SET = 2;

    static final class Drop {
        final int kind, index;
        final double chance;
        Drop(int kind, int index, double chance) { this.kind = kind; this.index = index; this.chance = chance; }
    }

    private final BookData.Set d;
    private final int diff, ilvl, players, mf;
    private final boolean bovine;
    private final HashMap<Long, double[]> plain = new HashMap<>();



    DropCalc(BookData.Set d, int diff, int mlvl, int players, int mf, int monster) {
        this.d = d; this.diff = diff; this.ilvl = mlvl; this.players = Math.max(1, Math.min(8, players)); this.mf = Math.max(0, mf);
        this.bovine = (monster & BookData.BOVINE_DROP) != 0;
    }
    DropCalc(BookData.Set d, int diff, int mlvl, int players, int mf) { this(d, diff, mlvl, players, mf, BookData.UPGRADES); }



    static int root(BookData.Set d, int tc, int diff, int mlvl, int monster) {
        return tc >= 0 && diff > 0 && (monster & BookData.UPGRADES) != 0 ? d.upgrade(tc, mlvl) : tc;
    }



    List<Drop> all(int root) {
        List<Drop> out = new ArrayList<>();
        BitSet items = reach(d, root);
        for (int i = items.nextSetBit(0); i >= 0; i = items.nextSetBit(i + 1)) {
            add(out, ITEM, i, root);
            for (int u : d.uniquesOfItem[i]) add(out, UNIQUE, u, root);
            for (int s : d.setsOfItem[i]) add(out, SET, s, root);
        }
        out.sort((a, b) -> a.chance != b.chance ? Double.compare(b.chance, a.chance) : a.kind != b.kind ? a.kind - b.kind : a.index - b.index);
        return out;
    }


    double chance(int root, int kind, int index) {
        return reachesTarget(root, kind, index) ? found(root, kind, index) : 0;
    }

    private void add(List<Drop> out, int kind, int index, int root) {
        if (kind == UNIQUE && !rollable(index) && !forcedReach(root, index)) return;
        if (kind == SET && !setRollable(index)) return;
        double p = found(root, kind, index);
        if (p > 0) out.add(new Drop(kind, index, p));
    }

    private double found(int root, int kind, int index) {
        double[] m = op(root, 0, 0, new Target(kind, index), new HashMap<>());
        double left = 0;
        for (int k = 0; k < N; k++) left += m[k];
        return Math.max(0, Math.min(1, 1 - left));
    }

    private static final class Target {
        final int kind, index;
        Target(int kind, int index) { this.kind = kind; this.index = index; }
    }




    private double[] op(int tc, int u, int s, Target t, HashMap<Long, double[]> memo) {
        int[] h = d.head[tc];
        if ((h[8] >> diff & 1) == 0) return identity();
        u = Math.max(u, h[2]);
        s = Math.max(s, h[3]);
        if (t != null && !reachesTarget(tc, t.kind, t.index)) t = null;
        long key = (long) tc << 22 | (long) u << 11 | s;
        HashMap<Long, double[]> cache = t == null ? plain : memo;
        double[] done = cache.get(key);
        if (done != null) return done;
        int[] e = d.entries[tc];
        double[] out;
        if (h[0] > 0) {
            long total = 0;
            for (int j = 0; j < e.length; j += 3) total += e[j + 2];
            double nodrop = noDrop(h[1], total, players);
            double sum = nodrop + total;
            if (sum <= 0) out = identity();
            else {
                double[] pick = scaled(identity(), nodrop / sum);
                for (int j = 0; j < e.length; j += 3) addScaled(pick, entry(e[j], e[j + 1], u, s, t, memo), e[j + 2] / sum);
                out = identity();
                for (int k = 0; k < h[0]; k++) out = times(out, pick);
            }
        } else {
            out = identity();
            int left = -h[0];
            for (int j = 0; j < e.length && left > 0; j += 3)
                for (int c = 0; c < e[j + 2] && left > 0; c++, left--) out = times(out, entry(e[j], e[j + 1], u, s, t, memo));
        }
        cache.put(key, out);
        return out;
    }

    private double[] entry(int kind, int index, int u, int s, Target t, HashMap<Long, double[]> memo) {
        if (kind == 0) return op(index, u, s, t, memo);
        double q = 0;
        if (t != null) {
            if (kind == 1) q = leaf(index, u, s, t);
            else if (t.kind == UNIQUE) q = t.index == index ? 1 : 0;
            else if (t.kind == ITEM) q = d.uniqueItem[index] == t.index ? 1 : 0;
        }
        return item(q);
    }


    private double leaf(int item, int u, int s, Target t) {
        if (t.kind == ITEM) return item == t.index ? 1 : 0;
        if (t.kind == UNIQUE) {
            if (d.uniqueItem[t.index] != item || !rollable(t.index)) return 0;
            int share = uniqueShare(item);
            return share == 0 ? 0 : uniqueRoll(item, u) * d.uniqueRarity[t.index] / share;
        }
        if (d.setItem[t.index] != item || !setRollable(t.index)) return 0;
        int share = setShare(item);
        return share == 0 ? 0 : (1 - uniqueRoll(item, u)) * setRoll(item, s) * d.setRarity[t.index] / share;
    }

    private boolean rollable(int k) { return d.uniqueRarity[k] > 0 && d.uniqueLevel[k] <= ilvl && (d.uniqueDiffs[k] >> diff & 1) != 0; }
    private boolean setRollable(int k) { return d.setRarity[k] > 0 && d.setLevel[k] <= ilvl && (d.setBovine[k] == 0 || bovine); }
    private int uniqueShare(int item) { int n = 0; for (int k : d.uniquesOfItem[item]) if (rollable(k)) n += d.uniqueRarity[k]; return n; }
    private int setShare(int item) { int n = 0; for (int k : d.setsOfItem[item]) if (setRollable(k)) n += d.setRarity[k]; return n; }


    double uniqueRoll(int item, int factor) {
        if ((d.itemFlags[item] & BookData.QUALITY) == 0) return 0;
        int[] r = ratio(item);
        return quality(r[0], r[1], r[2], ilvl - d.itemLevel[item], effective(mf, 250), factor);
    }


    double setRoll(int item, int factor) {
        if ((d.itemFlags[item] & BookData.QUALITY) == 0) return 0;
        int[] r = ratio(item);
        return quality(r[6], r[7], r[8], ilvl - d.itemLevel[item], effective(mf, 500), factor);
    }


    static int effective(int mf, int k) { return mf <= 10 ? mf : mf * k / (mf + k); }

    private int[] ratio(int item) {
        int f = d.itemFlags[item];
        return d.ratio[((f & BookData.UBER) != 0 ? 2 : 0) + ((f & BookData.CLASS) != 0 ? 1 : 0)];
    }



    static double quality(int base, int divisor, int min, int levelsAbove, int effectiveMf, int factor) {
        long chance = (long) (base - levelsAbove / divisor) * 128;
        chance = chance * 100 / (100 + effectiveMf);
        if (chance < min) chance = min;
        chance -= chance * factor / 1024;
        return chance <= 128 ? 1 : 128.0 / chance;
    }



    static double noDrop(int nodrop, long total, int players) {
        int n = (players + 1) / 2;
        if (nodrop <= 0 || n <= 1 || total <= 0) return nodrop;
        return Math.floor(total / (Math.pow((nodrop + (double) total) / nodrop, n) - 1));
    }



    private static final Map<BookData.Set, BitSet[]> REACH = new java.util.WeakHashMap<>();
    private static final Map<BookData.Set, BitSet[]> FORCED = new java.util.WeakHashMap<>();

    static BitSet reach(BookData.Set d, int tc) { return tables(d)[0][tc]; }

    private boolean reachesTarget(int tc, int kind, int index) {
        BitSet items = reach(d, tc);
        if (kind == ITEM) return items.get(index);
        if (kind == UNIQUE) return items.get(d.uniqueItem[index]);
        return items.get(d.setItem[index]);
    }

    private boolean forcedReach(int tc, int unique) { return tables(d)[1][tc].get(unique); }

    private static synchronized BitSet[][] tables(BookData.Set d) {
        BitSet[] items = REACH.get(d), forced = FORCED.get(d);
        if (items == null) {
            items = new BitSet[d.head.length];
            forced = new BitSet[d.head.length];
            for (int k = 0; k < items.length; k++) fill(d, k, items, forced);
            REACH.put(d, items);
            FORCED.put(d, forced);
        }
        return new BitSet[][]{items, forced};
    }

    private static void fill(BookData.Set d, int tc, BitSet[] items, BitSet[] forced) {
        if (items[tc] != null) return;
        BitSet a = new BitSet(), f = new BitSet();
        int[] e = d.entries[tc];
        for (int j = 0; j < e.length; j += 3) {
            if (e[j] == 0) { fill(d, e[j + 1], items, forced); a.or(items[e[j + 1]]); f.or(forced[e[j + 1]]); }
            else if (e[j] == 1) a.set(e[j + 1]);
            else { a.set(d.uniqueItem[e[j + 1]]); f.set(e[j + 1]); }
        }
        items[tc] = a;
        forced[tc] = f;
    }



    private static double[] identity() { double[] m = new double[N * N]; for (int k = 0; k < N; k++) m[k * N + k] = 1; return m; }



    private static double[] item(double q) {
        double[] m = new double[N * N];
        for (int k = 0; k < CAP; k++) m[k * N + k + 1] = 1 - q;
        m[CAP * N + CAP] = 1;
        return m;
    }

    private static double[] scaled(double[] m, double f) { for (int k = 0; k < m.length; k++) m[k] *= f; return m; }
    private static void addScaled(double[] to, double[] m, double f) { for (int k = 0; k < to.length; k++) to[k] += m[k] * f; }

    private static double[] times(double[] a, double[] b) {
        double[] out = new double[N * N];
        for (int r = 0; r < N; r++)
            for (int k = r; k < N; k++) {
                double x = a[r * N + k];
                if (x == 0) continue;
                for (int c = k; c < N; c++) out[r * N + c] += x * b[k * N + c];
            }
        return out;
    }
}
