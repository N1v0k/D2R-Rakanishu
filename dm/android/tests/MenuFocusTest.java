



package com.xiaoboluo.d2r;

public class MenuFocusTest {
    private static int checks, fails;
    private static void check(boolean ok, String what) { checks++; if (!ok) { fails++; System.out.println("  FAIL " + what); } }

    private static int[][] column(int... kinds) {
        int[][] c = new int[kinds.length][];
        for (int i = 0; i < kinds.length; i++) c[i] = new int[]{10 + i, i, 0, kinds[i]};
        return c;
    }

    public static void main(String[] args) {
        int A = MenuFocus.ACTION, V = MenuFocus.VALUE;
        MenuFocus f = new MenuFocus();

        f.setCells(column(A, A, A, V, A));
        f.open(true);
        check(f.shown() && f.focused() == 10, "opened by holding: focus shows on the first entry (Pause)");
        check(f.handle(PadNav.UP) == MenuFocus.R_NONE && f.focused() == 10, "up on the first entry: no wrap");
        f.handle(PadNav.DOWN); f.handle(PadNav.DOWN); f.handle(PadNav.DOWN);
        check(f.focused() == 13 && f.kind() == V, "down three times: Players, a value entry");
        check(f.handle(PadNav.RIGHT) == MenuFocus.R_STEP_FORWARD && f.handle(PadNav.LEFT) == MenuFocus.R_STEP_BACK, "left / right on the value entry change the value");
        check(f.handle(PadNav.A) == MenuFocus.R_STEP_FORWARD, "A on the value entry steps forward");
        f.handle(PadNav.DOWN);
        check(f.focused() == 14 && f.handle(PadNav.DOWN) == MenuFocus.R_NONE && f.focused() == 14, "down to Close, no wrap at the bottom");
        check(f.handle(PadNav.LEFT) == MenuFocus.R_NONE && f.focused() == 14, "left / right on an action entry: nothing (one column)");
        check(f.handle(PadNav.B) == MenuFocus.R_CLOSE && f.handle(PadNav.MENU) == MenuFocus.R_CLOSE, "B and the menu button close");

        f.open(true);
        check(f.focused() == 13, "reopened: focus on the last chosen entry (Players)");
        f.handle(PadNav.UP); f.handle(PadNav.UP);
        check(f.handle(PadNav.A) == MenuFocus.R_ACTIVATE && f.focused() == 11, "A on Recipes activates it");

        int[][] four = {{10, 0, 0, A}, {12, 1, 0, A}, {13, 2, 0, V}, {14, 3, 0, A}};
        f.setCells(four); f.open(true);
        check(f.focused() == 10, "last choice gone: first entry");
        f.handle(PadNav.DOWN); f.handle(PadNav.DOWN); f.handle(PadNav.RIGHT);
        f.setCells(column(A, A, A, V, A)); f.open(true);
        check(f.focused() == 13, "the table changed back: Players found again by id");

        f.open(false);
        check(!f.shown(), "opened by touch: focus hidden");
        check(f.handle(PadNav.DOWN) == MenuFocus.R_SHOWN && f.shown() && f.focused() == 13, "first press only reveals the focus (no move)");
        f.hide(); check(!f.shown() && f.handle(PadNav.B) == MenuFocus.R_CLOSE, "hidden by a tap; B still closes");
        System.out.println("MenuFocusTest: " + checks + " checks, " + fails + " failed");
        if (fails != 0) System.exit(1);
    }
}
