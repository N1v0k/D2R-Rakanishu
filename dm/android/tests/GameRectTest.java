


package com.xiaoboluo.d2r;

public final class GameRectTest {
    private static int checks, fails;
    private static void check(boolean ok, String what) { checks++; if (!ok) { fails++; System.out.println("FAIL " + what); } }
    private static boolean box(int[] b, int x, int y, int w, int h) { return b[0] == x && b[1] == y && b[2] == w && b[3] == h; }

    public static void main(String[] args) {
        GameRect g = new GameRect(); int[] b = new int[4];

        check(g.update(2480, 1116, null) && g.whole(), "no native reading: whole window");
        check(!g.update(2480, 1116, null), "same window: no change");

        check(!g.update(2480, 1116, new int[] {0, 0, 2480, 1116, 0, 0, 2480, 1116}) && g.whole(), "wide screen reading equals the whole window: no change");
        for (int k : new int[] {GameRect.WHOLE, GameRect.BLOCK, GameRect.TOP, GameRect.PAGE}) { g.bounds(k, b); check(box(b, 0, 0, 2480, 1116), "wide screen: every kind takes the whole window (" + k + ")"); }
        check(g.uiH() == 1116 && g.pictureAtX(0.5f) == 1240 && g.pictureAtY(1f) == 1116 && g.pictureToBlock(0.25f) == 0.25f, "wide screen: picture = window, ui height = window height");

        check(g.update(2048, 1536, new int[] {0, 384, 2048, 1152, 0, 0, 2048, 1536}) && !g.whole(), "4:3 in a run: not whole");
        g.bounds(GameRect.BLOCK, b); check(box(b, 0, 384, 2048, 1152), "block at the bottom");
        g.bounds(GameRect.TOP, b); check(box(b, 0, 0, 2048, 1152), "top: the block at the top of the picture");
        g.bounds(GameRect.PAGE, b); check(box(b, 0, 192, 2048, 1152), "page: the block centred in the picture");
        g.bounds(GameRect.WHOLE, b); check(box(b, 0, 0, 2048, 1536), "whole: the window");
        check(g.uiH() == 1152 && g.pictureAtY(0.5f) == 768 && Math.abs(g.pictureToBlock(30000) - 40000) < 0.5f, "4:3: ui height 1152, picture y, the portrait edge scaled by 1536 / 1152");

        check(g.update(2048, 1536, new int[] {0, 192, 2048, 1152, 0, 192, 2048, 1152}), "front end: changed");
        g.bounds(GameRect.TOP, b); check(box(b, 0, 192, 2048, 1152), "front end: top = the picture");
        g.bounds(GameRect.PAGE, b); check(box(b, 0, 192, 2048, 1152), "front end: page = the picture");
        check(g.pictureAtX(0) == 0 && g.pictureAtY(0) == 192 && g.pictureToBlock(0.3f) == 0.3f, "front end: picture coordinates start at the bar");

        check(g.update(2208, 1840, new int[] {150, 1232, 2208, 608, 0, 0, 2208, 1840}), "foldable with a raised UI: changed");
        g.bounds(GameRect.BLOCK, b); check(box(b, 150, 1232, 2208, 608), "the corrected block is kept as given");

        g.forget(); check(g.update(2208, 1840, new int[] {150, 1232, 2208, 608, 0, 0, 2208, 1840}) && !g.update(2208, 1840, new int[] {150, 1232, 2208, 608, 0, 0, 2208, 1840}), "forget: one change, then quiet");

        check(g.update(2208, 1840, new int[] {0, 0, 0, 0, 0, 0, 0, 0}) && g.whole(), "zero reading: whole window");
        check(g.update(100, 100, new int[] {1, 2, 3}) && g.whole(), "short reading: whole window");
        System.out.println("GameRectTest: " + checks + " checks, " + fails + " failed");
        if (fails != 0) System.exit(1);
    }
}
