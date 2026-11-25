package com.xiaoboluo.d2r;










final class GlossaryCalc {

    static final int VERSION = 1, VER = 0, VALID = 1, SESSION = 2, CLASS = 3, LEVEL = 4, DIFF = 5, EXPANSION = 6, DEF = 7, BLOCK = 8, AR = 9, AC = 10, DEX = 11,
        ACPCT = 12, ACOVR = 13, STATE101 = 14, SKILLPCT = 15, GEN = 16, ARMOR_SHRINE = 17, STATS = 18;
    static final int[] STAT_IDS = {19, 20, 34, 35, 36, 39, 40, 41, 42, 43, 44, 45, 46, 60, 62, 93, 99, 105, 110, 135, 136, 141};
    static final int INTS = STATS + STAT_IDS.length;
    static final int MAX_MONSTER_LEVEL = 110;

    static final class Snap {
        boolean valid, state101, armorShrine;
        int session, cls = -1, level, diff = -1, expansion = -1, def, block, ar = -1, ac, dex, acpct, acovr, skillPct, gen;
        final int[] stats = new int[STAT_IDS.length];
        int stat(int id) { for (int i = 0; i < STAT_IDS.length; i++) if (STAT_IDS[i] == id) return stats[i]; return 0; }

        static Snap parse(int[] w, int n) {
            Snap s = new Snap();
            if (w == null || n < INTS || w[VER] != VERSION || w[VALID] == 0) return s;
            s.valid = true; s.session = w[SESSION]; s.cls = w[CLASS]; s.level = w[LEVEL]; s.diff = w[DIFF] >= 0 && w[DIFF] <= 2 ? w[DIFF] : -1;
            s.expansion = w[EXPANSION] == 0 || w[EXPANSION] == 1 ? w[EXPANSION] : -1;
            s.def = w[DEF]; s.block = w[BLOCK]; s.ar = w[AR]; s.ac = w[AC]; s.dex = w[DEX]; s.acpct = w[ACPCT]; s.acovr = w[ACOVR]; s.state101 = w[STATE101] != 0;
            s.skillPct = w[SKILLPCT]; s.gen = w[GEN]; s.armorShrine = w[ARMOR_SHRINE] != 0;
            System.arraycopy(w, STATS, s.stats, 0, STAT_IDS.length);
            if (s.level < 1) s.valid = false;
            return s;
        }
    }



    static final class Monsters {
        final int[][][] ar = new int[2][][], def = new int[2][][];
        int[][] leech;
        int ar(int expansion, int diff, int level) { return pick(ar, expansion, diff, level); }
        int def(int expansion, int diff, int level) { return pick(def, expansion, diff, level); }
        private static int pick(int[][][] t, int e, int d, int l) {
            if (e < 0 || e > 1 || d < 0 || t[e] == null || d >= t[e].length || t[e][d] == null || l < 0 || l >= t[e][d].length) return -1;
            return t[e][d][l];
        }
        int leech(int diff, int which) { return leech != null && diff >= 0 && diff < leech.length && leech[diff] != null && which < leech[diff].length ? leech[diff][which] : -1; }
    }


    static int hit(int ar, int def, int attackerLevel, int defenderLevel) {
        if (ar < 0 || def < 0 || attackerLevel < 1 || defenderLevel < 1 || ar + def <= 0) return -1;
        long c = 200L * ar * attackerLevel / ((long) (ar + def) * (attackerLevel + defenderLevel));
        return (int) Math.max(5, Math.min(95, c));
    }


    static int defense(int ac, int dex, int pct, int ovr) {
        int base = ac + dex / 4;
        long q = (long) base * pct / 100;
        long def = base + (base > 0 ? q : -q);
        if (ovr != 0) def = def + def * ovr / 100;
        return (int) Math.max(Integer.MIN_VALUE, Math.min(Integer.MAX_VALUE, def));
    }



    static boolean defenseModelHolds(Snap s) { return s.valid && !s.state101 && defense(s.ac, s.dex, s.acpct, s.acovr) == s.def; }


    static int monsterLevel(int level) { return Math.max(1, Math.min(MAX_MONSTER_LEVEL, level)); }


    static String tenths(int tenths, boolean german) {
        int a = Math.abs(tenths);
        String s = a % 10 == 0 ? Integer.toString(a / 10) : (a / 10) + (german ? "," : ".") + (a % 10);
        return tenths < 0 ? "-" + s : s;
    }


    static String seconds(int frames, boolean german) {
        int hundredths = frames * 4;
        String s = (hundredths / 100) + (german ? "," : ".") + String.format(java.util.Locale.ROOT, "%02d", hundredths % 100);
        return s;
    }
}
