package com.xiaoboluo.d2r;



final class BestGearModel {
    static final int DEFAULT_HITS = 15, DEFAULT_BOSS = 50, MAX_HITS = 200;

    private BestGearModel() {}

    static int[] defaults() { return new int[] { DEFAULT_HITS, DEFAULT_BOSS }; }

    static int[] fromGuide(Integer hits, Double bossShare) {
        if (hits == null || bossShare == null || hits < 0 || hits > MAX_HITS || !(bossShare >= 0 && bossShare <= 1)) return null;
        return new int[] { hits, (int) Math.round(bossShare * 100) };
    }

    static int[] pick(int[] own, int[] guide) { return own != null ? own : guide != null ? guide : defaults(); }

    static String key(String name, int classIndex) { return name + "#" + classIndex; }
    static String encode(int hits, int boss) { return hits + "," + boss; }

    static int[] decode(String s) {
        if (s == null) return null;
        String[] f = s.split(",", -1);
        if (f.length != 2) return null;
        try {
            int hits = Integer.parseInt(f[0]), boss = Integer.parseInt(f[1]);
            return hits >= 0 && hits <= MAX_HITS && boss >= 0 && boss <= 100 ? new int[] { hits, boss } : null;
        } catch (NumberFormatException e) { return null; }
    }
}
