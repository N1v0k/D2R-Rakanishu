



package com.xiaoboluo.d2r;

import java.util.ArrayList;
import java.util.List;

public final class PadNavTest {
    private static int checks, fails;
    private static void check(boolean ok, String what) { checks++; if (!ok) { fails++; System.out.println("FAIL " + what); } }

    private static final List<Integer> got = new ArrayList<>();
    private static PadNav nav() { got.clear(); return new PadNav(got::add); }
    private static boolean only(int... cmds) {
        if (got.size() != cmds.length) return false;
        for (int i = 0; i < cmds.length; i++) if (got.get(i) != cmds[i]) return false;
        return true;
    }

    private static void buttons() {
        PadNav n = nav();
        n.key(1, PadNav.A, true, 0);
        check(only(PadNav.A), "A fires on press");
        n.key(1, PadNav.A, true, 10);
        check(only(PadNav.A), "a second DOWN without UP (repeat) does not fire");
        n.key(1, PadNav.A, false, 20);
        check(only(PadNav.A), "release does not fire");
        n.key(1, PadNav.MENU, true, 30);
        check(only(PadNav.A, PadNav.MENU), "menu fires");
    }

    private static void twoPads() {
        PadNav n = nav();
        n.key(1, PadNav.MENU, true, 0);
        n.begin();
        got.clear();
        n.key(2, PadNav.MENU, true, 10);
        check(only(PadNav.MENU), "pad 2's menu press counts while pad 1 still holds it");
        n.key(2, PadNav.MENU, false, 20);
        n.key(1, PadNav.MENU, false, 30);
        n.key(1, PadNav.MENU, true, 40);
        check(only(PadNav.MENU, PadNav.MENU), "pad 1 released and pressed again: counts");
        n.key(3, PadNav.B, true, 50);
        n.gone(3, 60);
        n.key(3, PadNav.B, true, 70);
        check(only(PadNav.MENU, PadNav.MENU, PadNav.B, PadNav.B), "a pad that went away starts fresh");
    }

    private static void heldAtBegin() {
        PadNav n = nav();
        n.key(1, PadNav.MENU, true, 0);
        n.motion(1, 0f, 0.9f, 0f, 0f, 0f, 0.9f, 0);
        got.clear();
        n.begin();
        n.key(1, PadNav.MENU, false, 100);
        n.key(1, PadNav.MENU, true, 200);
        check(only(PadNav.MENU), "the opening press's release is ignored, a new press counts");
        got.clear();
        n.motion(1, 0f, 0.95f, 0f, 0f, 0f, 0.95f, 300);
        check(got.isEmpty(), "stick and trigger held since begin: nothing");
        check(n.poll(1000) < 0, "no repeat for a blocked direction");
        n.motion(1, 0f, 0.1f, 0f, 0f, 0f, 0.1f, 400);
        n.motion(1, 0f, 0.9f, 0f, 0f, 0f, 0.9f, 500);
        check(only(PadNav.R2, PadNav.DOWN), "after release both count again");
    }

    private static void directions() {
        PadNav n = nav();
        n.key(1, PadNav.DOWN, true, 0);
        check(only(PadNav.DOWN), "D-pad key fires once");
        check(n.poll(399) == 400 && only(PadNav.DOWN), "no repeat before 400 ms");
        n.poll(400);
        check(only(PadNav.DOWN, PadNav.DOWN), "first repeat at 400 ms");
        n.poll(549);
        check(got.size() == 2, "nothing at 549 ms");
        n.poll(550);
        n.poll(700);
        check(got.size() == 4, "repeats every 150 ms (550, 700)");
        long next = n.poll(1500);
        check(got.size() == 5 && next == 1500 + PadNav.REPEAT_NEXT_MS, "a late callback fires once, no burst, and the next one is a full interval from now");
        n.poll(1501);
        check(got.size() == 5, "nothing right after the late one");
        n.key(1, PadNav.DOWN, false, 1510);
        check(n.poll(3000) < 0 && got.size() == 5, "released: repeat stops");

        got.clear();
        n.motion(2, 0f, 0f, 1f, 0f, 0f, 0f, 3000);
        check(only(PadNav.RIGHT), "HAT right fires");
        n.motion(2, 0f, 0f, 1f, 0f, 0f, 0f, 3010);
        check(only(PadNav.RIGHT), "unchanged HAT does not fire again");
        n.motion(2, 0f, 0f, 0f, -1f, 0f, 0f, 3020);
        check(only(PadNav.RIGHT, PadNav.UP), "HAT up after right");
        n.motion(2, 0f, 0f, 0f, 0f, 0f, 0f, 3030);

        got.clear();
        n.motion(3, 0.45f, 0f, 0f, 0f, 0f, 0f, 4000);
        check(got.isEmpty(), "0.45 is not pushed");
        n.motion(3, -0.6f, 0.3f, 0f, 0f, 0f, 0f, 4010);
        check(only(PadNav.LEFT), "-0.6 / 0.3 = left (dominant axis)");
        n.motion(3, -0.42f, 0.1f, 0f, 0f, 0f, 0f, 4020);
        check(only(PadNav.LEFT), "back to 0.42: still held (hysteresis)");
        n.motion(3, -0.3f, 0f, 0f, 0f, 0f, 0f, 4030);
        n.motion(3, -0.7f, 0f, 0f, 0f, 0f, 0f, 4040);
        check(only(PadNav.LEFT, PadNav.LEFT), "released below 0.4 and pushed again: fires again");
        n.gone(3, 4050);
        check(n.poll(5000) < 0, "a pad gone releases its directions");

        got.clear();
        PadNav m = nav();
        m.motion(1, 0f, 0f, 0f, 1f, 0f, 0f, 0);
        m.motion(2, 0f, 0f, 0f, 0f, 0f, 0f, 10);
        check(only(PadNav.DOWN) && m.poll(400) > 400 && got.size() == 2, "pad 2 centred does not release pad 1's direction");
    }



    private static void triggers() {
        PadNav n = nav();
        n.key(1, PadNav.R2, true, 0);
        n.motion(1, 0f, 0f, 0f, 0f, 0f, 0.9f, 10);
        check(only(PadNav.R2), "key then axis of the same press: one R2");
        n.motion(1, 0f, 0f, 0f, 0f, 0f, 0.59f, 20);
        n.motion(1, 0f, 0f, 0f, 0f, 0f, 0.61f, 30);
        check(only(PadNav.R2), "axis wobbling around 0.6 while held: no second R2");
        n.motion(1, 0f, 0f, 0f, 0f, 0f, 0.1f, 40);
        n.motion(1, 0f, 0f, 0f, 0f, 0f, 0.9f, 50);
        check(only(PadNav.R2), "axis released and pressed while the key is still held: no second R2");
        n.key(1, PadNav.R2, false, 60);
        check(only(PadNav.R2), "key released, axis still held: still held");
        n.motion(1, 0f, 0f, 0f, 0f, 0f, 0f, 70);
        n.key(1, PadNav.R2, true, 80);
        check(only(PadNav.R2, PadNav.R2), "both released, pressed again: a new R2");
        n.key(1, PadNav.R2, false, 90);
        got.clear();
        n.motion(2, 0f, 0f, 0f, 0f, 0.9f, 0f, 100);
        n.motion(2, 0f, 0f, 0f, 0f, 0.5f, 0f, 110);
        n.motion(2, 0f, 0f, 0f, 0f, 0.8f, 0f, 120);
        check(only(PadNav.L2), "axis only: back to 0.5 is not a release (hysteresis)");
        n.motion(2, 0f, 0f, 0f, 0f, 0.3f, 0f, 130);
        n.motion(2, 0f, 0f, 0f, 0f, 0.7f, 0f, 140);
        check(only(PadNav.L2, PadNav.L2), "axis only: below 0.4 and past 0.6 again: a new L2");

        n.key(3, PadNav.L2, true, 200);
        got.clear();
        n.begin();
        n.motion(3, 0f, 0f, 0f, 0f, 0.9f, 0f, 210);
        n.key(3, PadNav.L2, false, 220);
        check(got.isEmpty(), "held at begin: the axis and the key release give nothing");
        n.motion(3, 0f, 0f, 0f, 0f, 0f, 0f, 230);
        n.key(3, PadNav.L2, true, 240);
        check(only(PadNav.L2), "after both released a new press counts");
    }


    private static final int PAUSE = 0, LEFT = 1, RIGHT = 2, PLAYERS = 3, FPS = 4, CLOSE = 5, RECIPES = 6, STATUS = 7, BUILDS = 8;
    private static int[][] grid(boolean book, boolean builds) {
        List<int[]> c = new ArrayList<>();
        c.add(new int[]{PAUSE, 0, 0, MenuFocus.ACTION});
        if (book) c.add(new int[]{RECIPES, 0, 1, MenuFocus.ACTION});
        if (builds) c.add(new int[]{BUILDS, 0, 2, MenuFocus.ACTION});
        c.add(new int[]{LEFT, 1, 0, MenuFocus.VALUE});
        c.add(new int[]{RIGHT, 2, 0, MenuFocus.VALUE});
        c.add(new int[]{PLAYERS, 3, 0, MenuFocus.VALUE});
        c.add(new int[]{FPS, 4, 0, MenuFocus.VALUE});
        c.add(new int[]{CLOSE, 5, 0, MenuFocus.ACTION});
        c.add(new int[]{STATUS, 5, 1, MenuFocus.ACTION});
        return c.toArray(new int[0][]);
    }

    private static void focus() {
        MenuFocus f = new MenuFocus();
        f.setCells(grid(true, false));
        f.open(false);
        check(!f.shown() && f.focused() == PAUSE, "opened by touch: hidden, on Pause");
        check(f.handle(PadNav.DOWN) == MenuFocus.R_SHOWN && f.focused() == PAUSE, "first direction only reveals");
        check(f.handle(PadNav.RIGHT) == MenuFocus.R_MOVED && f.focused() == RECIPES, "right: Recipes");
        check(f.handle(PadNav.RIGHT) == MenuFocus.R_NONE && f.focused() == RECIPES, "no wrap to the right");
        check(f.handle(PadNav.DOWN) == MenuFocus.R_MOVED && f.focused() == LEFT, "down from Recipes: Left (one cell)");
        check(f.handle(PadNav.LEFT) == MenuFocus.R_STEP_BACK && f.focused() == LEFT, "left on a value row: step back");
        check(f.handle(PadNav.A) == MenuFocus.R_STEP_FORWARD, "A on a value row: step forward");
        for (int i = 0; i < 3; i++) f.handle(PadNav.DOWN);
        check(f.focused() == FPS, "down three times: frame rate");
        check(f.handle(PadNav.DOWN) == MenuFocus.R_MOVED && f.focused() == STATUS, "down to the last row keeps the column of Recipes: status line");
        check(f.handle(PadNav.DOWN) == MenuFocus.R_NONE, "no wrap at the bottom");
        check(f.handle(PadNav.LEFT) == MenuFocus.R_MOVED && f.focused() == CLOSE, "left: Close");
        check(f.handle(PadNav.A) == MenuFocus.R_ACTIVATE, "A on Close: activate");
        for (int i = 0; i < 5; i++) f.handle(PadNav.UP);
        check(f.focused() == PAUSE, "up five times from Close: Pause (left column kept)");
        check(f.handle(PadNav.UP) == MenuFocus.R_NONE, "no wrap at the top");
        check(f.handle(PadNav.B) == MenuFocus.R_CLOSE && f.handle(PadNav.MENU) == MenuFocus.R_CLOSE, "B and menu close");
        check(f.handle(PadNav.X) == MenuFocus.R_NONE, "other buttons do nothing");

        f.open(true);
        check(f.shown() && f.focused() == CLOSE, "opened by holding: shown at once, on the last chosen cell");
        check(new MenuFocus().handle(PadNav.B) == MenuFocus.R_CLOSE, "an empty menu still closes on B");

        MenuFocus g = new MenuFocus();
        g.setCells(grid(false, false));
        g.open(true);
        check(g.handle(PadNav.RIGHT) == MenuFocus.R_NONE && g.focused() == PAUSE, "Pause alone: right does nothing");

        MenuFocus b = new MenuFocus();
        b.setCells(grid(false, true));
        b.open(true);
        check(b.handle(PadNav.RIGHT) == MenuFocus.R_MOVED && b.focused() == BUILDS, "column gap: right from Pause reaches Builds");
        b.setCells(grid(true, true));
        check(b.focused() == BUILDS, "the table changes (book appears): focus stays on Builds");
        check(b.handle(PadNav.LEFT) == MenuFocus.R_MOVED && b.focused() == RECIPES, "left: Recipes");

        MenuFocus q = new MenuFocus();
        q.setCells(new int[][]{{10, 0, 0, MenuFocus.ACTION}, {11, 1, 0, MenuFocus.ACTION}, {12, 2, 0, MenuFocus.VALUE}, {13, 3, 0, MenuFocus.ACTION}});
        q.open(true);
        q.handle(PadNav.DOWN); q.handle(PadNav.DOWN);
        check(q.focused() == 12 && q.handle(PadNav.RIGHT) == MenuFocus.R_STEP_FORWARD, "a vertical list: value entry steps with right");
        check(q.handle(PadNav.DOWN) == MenuFocus.R_MOVED && q.focused() == 13 && q.handle(PadNav.DOWN) == MenuFocus.R_NONE, "last entry, no wrap");
    }

    public static void main(String[] a) {
        buttons(); twoPads(); heldAtBegin(); directions(); triggers(); focus();
        System.out.println("PadNavTest: " + checks + " checks, " + fails + " failed");
        if (fails > 0) System.exit(1);
    }
}
