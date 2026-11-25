package com.xiaoboluo.d2r;

import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.List;
import java.util.Random;





public class DropCalcTest {
    static int failed;

    static void near(String what, double got, double want, double tol) {
        if (Math.abs(got - want) <= tol) return;
        failed++;
        System.out.println("FAIL " + what + ": got " + got + ", want " + want + " (± " + tol + ")");
    }

    static void check(String what, Object got, Object want) {
        if (String.valueOf(got).equals(String.valueOf(want))) return;
        failed++;
        System.out.println("FAIL " + what + ": got " + got + ", want " + want);
    }


    static BookData book(String tc, String items, String uniques, String sets) {
        String set = "{\"tc\":[" + tc + "],\"item\":[" + items + "],\"unique\":[" + uniques + "],\"set\":[" + sets + "],"
                + "\"ratio\":[[0,0,400,1,6400,100,2,3200,160,2,5600,34,3,192],[0,1,240,3,6400,80,3,3200,120,3,5600,17,6,192],"
                + "[1,0,400,1,6400,100,2,3200,160,2,5600,34,3,192],[1,1,240,3,6400,80,3,3200,120,3,5600,17,6,192]],"
                + "\"mon\":{\"1\":[0,0,0,1]},\"su\":{},\"extra\":{\"7\":[\"Egg\",\"egg\",[1,2,3],[1]]}}";
        return BookData.parse("{\"v\":1,\"game\":\"x\",\"names\":{\"a\":[\"A\",\"A\",\"A\",\"A\",\"A\",\"A\",\"A\",\"A\",\"A\",\"A\",\"A\",\"A\",\"A\"]},"
                + "\"sets\":{\"rotw\":" + set + ",\"lod\":" + set + "}}");
    }

    static final String ITEMS = "[\"aaa\",\"a\",1,\"r\",0],[\"bbb\",\"b\",1,\"q\",0],[\"ccc\",\"c\",1,\"r\",0],[\"wpn\",\"w\",10,\"w\",1]";

    public static void main(String[] args) throws Exception {

        BookData b = book("[2,1,0,0,0,0,0,0,7,[1,0,1]]", ITEMS, "", "");
        BookData.Set s = b.sets.get("rotw");
        near("two picks of 1/2", new DropCalc(s, 0, 1, 1, 0).chance(0, DropCalc.ITEM, 0), 0.75, 1e-12);


        double q = 0.25;
        b = book("[-2,0,0,0,0,0,0,0,7,[0,1,1,0,2,1]],[-5,0,0,0,0,0,0,0,7,[1,1,5]],[3,0,0,0,0,0,0,0,7,[1,0,1,1,2,3]]", ITEMS, "", "");
        s = b.sets.get("rotw");
        near("cap reached after five items", new DropCalc(s, 0, 1, 1, 0).chance(0, DropCalc.ITEM, 0), q, 1e-12);
        near("cap: the other rune", new DropCalc(s, 0, 1, 1, 0).chance(0, DropCalc.ITEM, 2), 1 - q, 1e-12);

        b = book("[-2,0,0,0,0,0,0,0,7,[0,1,1,0,2,1]],[-4,0,0,0,0,0,0,0,7,[1,1,5]],[3,0,0,0,0,0,0,0,7,[1,0,1,1,2,3]]", ITEMS, "", "");
        near("two places left", new DropCalc(b.sets.get("rotw"), 0, 1, 1, 0).chance(0, DropCalc.ITEM, 0), q + (1 - q) * q, 1e-12);

        b = book("[7,0,0,0,0,0,0,0,7,[1,1,1]],[1,0,0,0,0,0,0,0,7,[1,0,1]]", ITEMS, "", "");
        check("cap: items of a kill", dropped(b.sets.get("rotw"), 0), 6);


        b = book("[-3,0,0,0,0,0,0,0,7,[1,1,2,1,0,5]]", ITEMS, "", "");
        near("negative picks reach the third", new DropCalc(b.sets.get("rotw"), 0, 1, 1, 0).chance(0, DropCalc.ITEM, 0), 1, 1e-12);
        b = book("[-2,0,0,0,0,0,0,0,7,[1,1,2,1,0,5]]", ITEMS, "", "");
        near("negative picks stop at the count", new DropCalc(b.sets.get("rotw"), 0, 1, 1, 0).chance(0, DropCalc.ITEM, 0), 0, 1e-12);


        check("nodrop 1 player", DropCalc.noDrop(100, 50, 1), 100.0);
        check("nodrop 2 players = 1", DropCalc.noDrop(100, 50, 2), 100.0);
        check("nodrop 3 players", DropCalc.noDrop(100, 50, 3), Math.floor(50 / (1.5 * 1.5 - 1)));
        check("nodrop 8 players", DropCalc.noDrop(19, 48, 8), 0.0);
        b = book("[1,1,0,0,0,0,0,0,7,[1,0,1]]", ITEMS, "", "");
        near("players lower nodrop", new DropCalc(b.sets.get("rotw"), 0, 1, 3, 0).chance(0, DropCalc.ITEM, 0), 1.0 / (1 + DropCalc.noDrop(1, 1, 3)), 1e-12);


        b = book("[1,0,0,0,0,0,4,10,7,[1,0,1]],[1,0,0,0,0,0,4,20,7,[1,1,1]],[1,0,0,0,0,0,4,30,7,[1,2,1]]", ITEMS, "", "");
        s = b.sets.get("rotw");
        check("no upgrade at its level", s.upgrade(0, 10), 0);
        check("upgrade to the highest not above", s.upgrade(0, 25), 1);
        check("upgrade to the top", s.upgrade(0, 40), 2);
        check("no downgrade", s.upgrade(1, 5), 1);
        near("upgraded class drops its item", new DropCalc(s, 1, 25, 1, 0).chance(DropCalc.root(s, 0, 1, 25, BookData.UPGRADES), DropCalc.ITEM, 1), 1, 1e-12);
        check("no upgrade in normal", DropCalc.root(s, 0, 0, 25, BookData.UPGRADES), 0);
        check("no upgrade for a boss or noRatio monster", DropCalc.root(s, 0, 2, 25, 0), 0);
        check("no class", DropCalc.root(s, -1, 2, 25, BookData.UPGRADES), -1);


        b = book("[1,0,0,0,0,0,0,0,4,[1,0,1]]", ITEMS, "", "");
        near("hell only, in normal", new DropCalc(b.sets.get("rotw"), 0, 1, 1, 0).chance(0, DropCalc.ITEM, 0), 0, 0);
        near("hell only, in hell", new DropCalc(b.sets.get("rotw"), 2, 1, 1, 0).chance(0, DropCalc.ITEM, 0), 1, 0);


        b = book("[2,0,0,0,0,0,0,0,7,[0,1,1,0,2,1]],[1,1,0,0,0,0,0,0,7,[1,0,1]],[1,3,0,0,0,0,0,0,7,[1,0,1]]", ITEMS, "", "");

        near("two ways", new DropCalc(b.sets.get("rotw"), 0, 1, 1, 0).chance(0, DropCalc.ITEM, 0), 1 - 25.0 / 64, 1e-12);


        check("unique roll at qlvl", DropCalc.quality(400, 1, 6400, 0, 0, 0), 128.0 / 51200);
        check("unique roll with factor 983", DropCalc.quality(400, 1, 6400, 0, 0, 983), 128.0 / (51200 - 51200 * 983 / 1024));
        check("min holds", DropCalc.quality(400, 1, 6400, 390, 0, 0), 128.0 / 6400);
        check("magic find", DropCalc.quality(400, 1, 6400, 0, 300 * 250 / 550, 0), 128.0 / (51200 * 100 / (100 + 136)));
        check("magic find up to 10 counts whole", DropCalc.effective(10, 250) + "/" + DropCalc.effective(1, 250) + "/" + DropCalc.effective(11, 250), "10/1/10");
        b = book("[1,0,0,0,0,0,0,0,7,[1,3,1]]", ITEMS, "[11,3,5,3,\"u1\",7],[12,3,5,1,\"u2\",7],[13,3,50,9,\"u3\",7],[14,3,5,2,\"u4\",4]",
                "[21,3,5,1,\"s1\",0]");
        s = b.sets.get("rotw");
        DropCalc c = new DropCalc(s, 0, 10, 1, 0);
        double pu = DropCalc.quality(400, 1, 6400, 0, 0, 0), ps = DropCalc.quality(160, 2, 5600, 0, 0, 0);
        near("unique by rarity", c.chance(0, DropCalc.UNIQUE, 0), pu * 3 / 4, 1e-15);
        near("unique above the level", c.chance(0, DropCalc.UNIQUE, 2), 0, 0);
        near("unique of another difficulty", c.chance(0, DropCalc.UNIQUE, 3), 0, 0);
        near("set after the unique roll", c.chance(0, DropCalc.SET, 0), (1 - pu) * ps, 1e-15);
        b = book("[1,0,0,0,0,0,0,0,7,[1,3,1]]", ITEMS, "", "[21,3,5,1,\"s1\",0],[22,3,5,3,\"cow\",1]");
        BookData.Set cows = b.sets.get("rotw");
        near("the bovine set: not from others, no share", new DropCalc(cows, 0, 10, 1, 0, BookData.UPGRADES).chance(0, DropCalc.SET, 0), (1 - pu) * ps, 1e-15);
        near("not the bovine set from others", new DropCalc(cows, 0, 10, 1, 0, BookData.UPGRADES).chance(0, DropCalc.SET, 1), 0, 0);
        near("the bovine set from a bovine", new DropCalc(cows, 0, 10, 1, 0, BookData.BOVINE_DROP).chance(0, DropCalc.SET, 1), (1 - pu) * ps * 3 / 4, 1e-15);
        DropCalc rich = new DropCalc(s, 0, 10, 1, 300);
        near("magic find raises the unique", rich.chance(0, DropCalc.UNIQUE, 0), DropCalc.quality(400, 1, 6400, 0, 136, 0) * 3 / 4, 1e-15);
        List<DropCalc.Drop> all = c.all(0);
        check("all: rows", all.size(), 4);
        check("all: first the weapon", all.get(0).kind + ":" + all.get(0).index, DropCalc.ITEM + ":3");


        b = book("[1,1,0,0,0,0,0,0,7,[2,0,1]]", ITEMS, "[11,2,99,0,\"u1\",0]", "");
        s = b.sets.get("rotw");
        near("forced unique", new DropCalc(s, 0, 1, 1, 0).chance(0, DropCalc.UNIQUE, 0), 0.5, 1e-12);
        near("forced unique counts as its item", new DropCalc(s, 0, 1, 1, 0).chance(0, DropCalc.ITEM, 2), 0.5, 1e-12);
        check("forced unique listed", new DropCalc(s, 0, 1, 1, 0).all(0).size(), 2);


        String tree = "[-3,0,0,0,0,0,0,0,7,[0,1,2,0,2,1]]," +
                "[2,2,512,512,0,0,0,0,7,[1,3,3,1,1,1]]," +
                "[3,1,0,0,0,0,0,0,7,[1,0,1,1,2,2]]";
        b = book(tree, ITEMS, "[11,3,5,3,\"u1\",7],[12,3,8,1,\"u2\",7]", "[21,3,5,1,\"s1\",0]");
        s = b.sets.get("rotw");
        for (int players : new int[]{1, 5}) {
            c = new DropCalc(s, 0, 10, players, 150);
            Dice dice = new Dice(s, 0, 10, players, 150, 1234 + players);
            int kills = 400000;
            int[][] hits = dice.run(0, kills);
            for (DropCalc.Drop r : c.all(0)) {
                double seen = (double) hits[r.kind][r.index] / kills, sigma = Math.sqrt(r.chance * (1 - r.chance) / kills);
                near("dice " + players + " players kind " + r.kind + " #" + r.index, seen, r.chance, 5 * sigma + 1e-9);
            }
        }

        String real = System.getenv("DM_BOOK_JSON");
        if (real == null || real.isEmpty()) System.out.println("DropCalcTest: DM_BOOK_JSON not set, the real tables are skipped");
        else realTables(BookData.parse(new String(Files.readAllBytes(Paths.get(real)), StandardCharsets.UTF_8)));

        if (failed > 0) { System.out.println("DropCalcTest: " + failed + " failed"); System.exit(1); }
        System.out.println("DropCalcTest OK");
    }


    static void realTables(BookData b) {
        int[][] cases = {{156, 2, 75}, {156, 0, 15}, {211, 2, 75}, {544, 2, 99}, {5, 2, 85}};
        for (String set : new String[]{"rotw", "lod"}) {
            BookData.Set s = b.sets.get(set);
            for (int[] k : cases) {
                int[] tcs = s.monster.get(k[0]);
                if (tcs == null || tcs[k[1]] < 0) { failed++; System.out.println("FAIL " + set + " monster " + k[0] + " has no class"); continue; }
                long t0 = System.nanoTime();
                int root = DropCalc.root(s, tcs[k[1]], k[1], k[2], tcs[3]);
                List<DropCalc.Drop> all = new DropCalc(s, k[1], k[2], 1, 200, tcs[3]).all(root);
                long ms = (System.nanoTime() - t0) / 1000000;
                System.out.println("real " + set + " monster " + k[0] + " diff " + k[1] + ": " + all.size() + " rows in " + ms + " ms; first "
                        + describe(s, all.get(0)));
                Dice dice = new Dice(s, k[1], k[2], 1, 200, k[0], tcs[3]);
                int kills = 200000;
                int[][] hits = dice.run(root, kills);
                for (int i = 0; i < Math.min(40, all.size()); i++) {
                    DropCalc.Drop r = all.get(i);
                    double seen = (double) hits[r.kind][r.index] / kills, sigma = Math.sqrt(r.chance * (1 - r.chance) / kills);
                    near("real dice " + set + " " + k[0] + " " + describe(s, r), seen, r.chance, 5 * sigma + 1e-9);
                }
            }
        }
    }

    static String describe(BookData.Set s, DropCalc.Drop r) {
        String what = r.kind == DropCalc.ITEM ? s.itemCode[r.index] : r.kind == DropCalc.UNIQUE ? s.uniqueName[r.index] : s.setName[r.index];
        return what + " 1:" + String.format("%.1f", 1 / r.chance);
    }

    static int dropped(BookData.Set s, int tc) {
        Dice d = new Dice(s, 0, 1, 1, 0, 1);
        d.count = 0;
        d.got = new boolean[][]{new boolean[s.itemCode.length], new boolean[s.uniqueId.length], new boolean[s.setId.length]};
        d.cls(tc, 0, 0);
        return d.count;
    }


    static final class Dice {
        final BookData.Set s;
        final int diff, ilvl, players, mf;
        final Random rnd;
        final DropCalc formulas;
        int count;
        boolean[][] got;

        final boolean bovine;
        Dice(BookData.Set s, int diff, int ilvl, int players, int mf, long seed) { this(s, diff, ilvl, players, mf, seed, BookData.UPGRADES); }
        Dice(BookData.Set s, int diff, int ilvl, int players, int mf, long seed, int flags) {
            this.s = s; this.diff = diff; this.ilvl = ilvl; this.players = players; this.mf = mf; this.rnd = new Random(seed);
            this.formulas = new DropCalc(s, diff, ilvl, players, mf, flags);
            this.bovine = (flags & BookData.BOVINE_DROP) != 0;
        }

        int[][] run(int root, int kills) {
            int[][] hits = {new int[s.itemCode.length], new int[s.uniqueId.length], new int[s.setId.length]};
            for (int n = 0; n < kills; n++) {
                count = 0;
                got = new boolean[][]{new boolean[s.itemCode.length], new boolean[s.uniqueId.length], new boolean[s.setId.length]};
                cls(root, 0, 0);
                for (int k = 0; k < 3; k++) for (int i = 0; i < got[k].length; i++) if (got[k][i]) hits[k][i]++;
            }
            return hits;
        }

        void cls(int tc, int u, int st) {
            int[] h = s.head[tc];
            if ((h[8] >> diff & 1) == 0) return;
            u = Math.max(u, h[2]);
            st = Math.max(st, h[3]);
            int[] e = s.entries[tc];
            if (h[0] > 0) {
                long total = 0;
                for (int j = 0; j < e.length; j += 3) total += e[j + 2];
                double nodrop = DropCalc.noDrop(h[1], total, players);
                for (int p = 0; p < h[0]; p++) {
                    double r = rnd.nextDouble() * (nodrop + total);
                    if (r < nodrop) continue;
                    r -= nodrop;
                    for (int j = 0; j < e.length; j += 3) {
                        if (r < e[j + 2]) { entry(e[j], e[j + 1], u, st); break; }
                        r -= e[j + 2];
                    }
                }
            } else {
                int left = -h[0];
                for (int j = 0; j < e.length && left > 0; j += 3)
                    for (int c = 0; c < e[j + 2] && left > 0; c++, left--) entry(e[j], e[j + 1], u, st);
            }
        }

        void entry(int kind, int index, int u, int st) {
            if (kind == 0) { cls(index, u, st); return; }
            if (count >= DropCalc.CAP) return;
            count++;
            if (kind == 2) { got[1][index] = true; got[0][s.uniqueItem[index]] = true; return; }
            got[0][index] = true;
            if (rnd.nextDouble() < formulas.uniqueRoll(index, u)) {
                int k = pick(s.uniquesOfItem[index], true);
                if (k >= 0) got[1][k] = true;
                return;
            }
            if (rnd.nextDouble() < formulas.setRoll(index, st)) {
                int k = pick(s.setsOfItem[index], false);
                if (k >= 0) got[2][k] = true;
            }
        }

        int pick(int[] of, boolean unique) {
            int sum = 0;
            for (int k : of) sum += weight(k, unique);
            if (sum == 0) return -1;
            int r = rnd.nextInt(sum);
            for (int k : of) { r -= weight(k, unique); if (r < 0) return k; }
            return -1;
        }

        int weight(int k, boolean unique) {
            if (unique) return s.uniqueRarity[k] > 0 && s.uniqueLevel[k] <= ilvl && (s.uniqueDiffs[k] >> diff & 1) != 0 ? s.uniqueRarity[k] : 0;
            return s.setRarity[k] > 0 && s.setLevel[k] <= ilvl && (s.setBovine[k] == 0 || bovine) ? s.setRarity[k] : 0;
        }
    }
}
