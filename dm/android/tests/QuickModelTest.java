


package com.xiaoboluo.d2r;

public class QuickModelTest {
    private static int checks, fails;
    private static void check(boolean ok, String what) { checks++; if (!ok) { fails++; System.out.println("  FAIL " + what); } }

    public static void main(String[] args) {

        check(QuickModel.players(1, -1) == 1 && QuickModel.players(8, 1) == 8, "left / right stop at 1 and 8");
        check(QuickModel.players(3, 1) == 4 && QuickModel.players(3, -1) == 2, "left / right step by one");
        check(QuickModel.players(8, 0) == 1 && QuickModel.players(5, 0) == 6, "A steps forward, 8 -> 1");
        check(QuickModel.atEnd(1, -1) && QuickModel.atEnd(8, 1) && !QuickModel.atEnd(8, -1) && !QuickModel.atEnd(4, 1), "arrows grey only at the ends");
        { int w = 1; for (int i = 0; i < 12; i++) w = QuickModel.players(w, 1); check(w == 8, "nine taps right from 1 end on 8, never 1"); }

        QuickModel m = new QuickModel();
        m.setEntries(true, true);
        check(m.count() == 4 && m.id(0) == QuickModel.PAUSE && m.id(1) == QuickModel.RECIPES && m.id(2) == QuickModel.BUILDS && m.id(3) == QuickModel.PLAYERS, "four entries in order");
        check(m.indexOf(QuickModel.CLOSE) < 0, "no Close entry: the window's X closes");
        m.setEntries(false, true);
        check(m.count() == 3 && m.indexOf(QuickModel.RECIPES) < 0 && m.indexOf(QuickModel.PLAYERS) == 2, "without the recipe book: 3 entries");
        m.setEntries(false, false);
        check(m.count() == 2 && m.id(1) == QuickModel.PLAYERS, "without recipe book and builds: 2 entries");
        check(QuickModel.kind(QuickModel.PLAYERS) == QuickModel.VALUE && QuickModel.kind(QuickModel.RECIPES) == QuickModel.PAGE && QuickModel.kind(QuickModel.PAUSE) == QuickModel.ACTION, "kinds");

        m.setEntries(true, true);
        int players = m.indexOf(QuickModel.PLAYERS) * 2;
        m.press(players + 1); check(m.release(players + 1) == QuickModel.PLAYERS * 2 + 1, "right half pressed and released = +1");
        m.press(players); check(m.release(players) == QuickModel.PLAYERS * 2, "left half = -1");
        m.press(players); check(m.release(players + 1) == QuickModel.TAP_NONE, "released on the other half: nothing");
        m.press(players); m.move(players + 1); m.move(players); check(m.release(players) == QuickModel.TAP_NONE, "dragged across the middle and back: still cancelled");
        m.press(players + 1); check(m.heldIndex() == players / 2 && m.heldHalf() == 1, "held entry and half for the pressed frame");
        m.move(QuickLayout.NEUTRAL); check(m.heldIndex() < 0, "dragged off: no pressed frame");
        check(m.release(players + 1) == QuickModel.TAP_NONE, "dragged off the entry: nothing");
        m.press(0); m.move(1); check(m.release(1) == QuickModel.PAUSE * 2 + 1, "an action entry ignores the halves");
        m.press(0); check(m.release(2) == QuickModel.TAP_NONE, "released on another entry: nothing");
        m.press(QuickLayout.OUTSIDE); check(m.release(QuickLayout.OUTSIDE) == QuickModel.TAP_CLOSE, "outside down and up: close");
        m.press(QuickLayout.OUTSIDE); check(m.release(0) == QuickModel.TAP_NONE, "outside down, released on an entry: nothing");
        m.press(0); check(m.release(QuickLayout.OUTSIDE) == QuickModel.TAP_NONE, "pressed on an entry, released outside: nothing");
        m.press(QuickLayout.NEUTRAL); check(m.release(QuickLayout.NEUTRAL) == QuickModel.TAP_NONE, "the frame rim does nothing");
        m.press(QuickLayout.CLOSE); check(m.heldIndex() < 0 && m.release(QuickLayout.CLOSE) == QuickModel.TAP_CLOSE, "the X pressed and released: close");
        m.press(QuickLayout.CLOSE); check(m.release(QuickLayout.OUTSIDE) == QuickModel.TAP_NONE, "the X pressed, released beside it: nothing");
        m.press(QuickLayout.OUTSIDE); check(m.release(QuickLayout.CLOSE) == QuickModel.TAP_NONE, "pressed outside, released on the X: nothing");

        QuickModel g = new QuickModel(); g.setEntries(true, true, true);
        check(g.count() == 5 && g.id(3) == QuickModel.GLOSSARY && g.id(4) == QuickModel.PLAYERS && QuickModel.kind(QuickModel.GLOSSARY) == QuickModel.PAGE, "five entries with the glossary");
        g.setEntries(false, true, true); check(g.count() == 4 && g.id(2) == QuickModel.GLOSSARY, "glossary without the recipe book");
        g.press(2 * 2); check(g.release(2 * 2 + 1) == QuickModel.GLOSSARY * 2 + 1, "tap on the glossary");

        QuickModel f = new QuickModel(); f.setEntries(true, true, true, true, true);
        check(f.count() == 7 && f.id(5) == QuickModel.FEATS && f.id(6) == QuickModel.PLAYERS && QuickModel.kind(QuickModel.FEATS) == QuickModel.PAGE, "seven entries with the achievements");
        f.setEntries(false, true, false, false, true); check(f.count() == 4 && f.id(2) == QuickModel.FEATS, "achievements without recipes, glossary and combat");
        f.press(2 * 2); check(f.release(2 * 2) == QuickModel.FEATS * 2, "tap on the achievements");
        f.setEntries(true, true, true, true); check(f.indexOf(QuickModel.FEATS) < 0, "out of a game: no achievements");
        System.out.println("QuickModelTest: " + checks + " checks, " + fails + " failed");
        if (fails != 0) System.exit(1);
    }
}
