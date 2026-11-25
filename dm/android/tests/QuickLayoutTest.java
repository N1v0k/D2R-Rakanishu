


package com.xiaoboluo.d2r;

public class QuickLayoutTest {
    private static final float FOCUS_RIM = 9 / 60f;
    private static int checks, fails;
    private static void check(boolean ok, String what) { checks++; if (!ok) { fails++; System.out.println("  FAIL " + what); } }

    private static GameStyleGeometry.Panel panel(float w, float h, float ydpi) {
        GameStyleGeometry.Panel p = new GameStyleGeometry.Panel();
        p.compute(w, h, GameStyleData.CONSOLE_UI_SCALE, 7f / 25.4f * ydpi);
        return p;
    }

    private static void inside(QuickLayout l, GameStyleGeometry.Panel p, String what) {
        check(l.bx >= p.contentL && l.bx + l.bw <= p.contentR, what + ": buttons inside the content area horizontally");
        check(l.rowTop(0) >= p.contentT - 0.01f && l.rowTop(l.n - 1) + l.pitch <= p.contentB + 0.01f, what + ": rows inside the content area vertically");
        check(l.bh <= l.pitch + 0.01f, what + ": a button fits its row");
    }

    public static void main(String[] args) {
        QuickLayout l = new QuickLayout();

        for (float[] sz : new float[][]{{2480, 1116, 410}, {2688, 1216, 430}, {1200, 900, 320}}) {
            String what = (int) sz[0] + "x" + (int) sz[1];
            GameStyleGeometry.Panel p = panel(sz[0], sz[1], sz[2]);
            l.compute(p, 7);
            inside(l, p, what);
            check(Math.abs(l.bw - p.buttonW()) < 0.01f && Math.abs(l.bh - p.buttonH()) < 0.01f, what + ": buttons at the game's size " + l.bw + "x" + l.bh);
            check(Math.abs(l.pitch - l.bh - QuickLayout.GAP * p.s) < 0.01f, what + ": gap " + (l.pitch - l.bh) + " = GAP game units");
            check(l.pitch - l.bh > FOCUS_RIM * l.bh + 1, what + ": the highlight (" + FOCUS_RIM * l.bh + ") clear of the next button");
            check(Math.abs(l.listTop + 3.5f * l.pitch - (p.contentT + p.contentB) / 2) < 0.01f && Math.abs(l.bx + l.bw / 2 - (p.contentL + p.contentR) / 2) < 0.01f, what + ": column centred");
        }

        { GameStyleGeometry.Panel p = panel(2480, 1116, 410); l.compute(p, 7);
          check(Math.abs(l.bw - 394) < 1.5f && Math.abs(l.bh - 72) < 1.5f && Math.abs(l.pitch - 86) < 1.5f, "2480x1116: " + l.bw + "x" + l.bh + ", row " + l.pitch); }

        { GameStyleGeometry.Panel p = panel(2688, 1216, 430);
          float room = p.contentB - p.contentT, row = p.buttonH() + QuickLayout.GAP * p.s;
          int n = (int) Math.ceil(room / row) + 1;
          l.compute(p, n);
          inside(l, p, n + " entries");
          check(l.pitch < row && Math.abs(l.pitch * n - room) < 0.01f, n + " entries: the rows close up to fill the content area");
          check(l.bh < p.buttonH() && Math.abs(l.bw / l.bh - p.buttonW() / p.buttonH()) < 1e-3f, n + " entries: smaller buttons of the same shape");
          check(Math.abs((l.pitch - l.bh) / l.bh - QuickLayout.GAP / GameStyleGeometry.Panel.BUTTON_H) < 1e-3f, n + " entries: the gap shrinks with the button"); }

        GameStyleGeometry.Panel p = panel(2688, 1216, 430);
        l.compute(p, 6);
        float cx = l.bx + l.bw / 2;
        for (int i = 0; i < 6; i++) {
            check(l.hit(cx - 5, l.rowTop(i) + 0.5f) == i * 2 && l.hit(cx + 5, l.rowTop(i) + l.pitch - 0.5f) == i * 2 + 1, "entry " + i + " both halves, full row");
            if (i > 0) check(l.hit(cx - 5, l.rowTop(i) - 0.5f) == (i - 1) * 2, "no gap above entry " + i);
        }
        check(l.hit((p.hitCloseL + p.hitCloseR) / 2, (p.hitCloseT + p.hitCloseB) / 2) == QuickLayout.CLOSE && l.hit((p.closeL + p.closeR) / 2, (p.closeT + p.closeB) / 2) == QuickLayout.CLOSE, "the X");
        check(l.hit(p.hitCloseL + 1, 1) == QuickLayout.CLOSE, "the X's touch area above the frame");
        check(l.hit(l.bx - 3, l.rowTop(2) + 10) == QuickLayout.NEUTRAL && l.hit(p.frameL + 2, (p.frameT + p.frameB) / 2) == QuickLayout.NEUTRAL, "frame rim and content beside the buttons neutral");
        check(l.hit(cx, l.listTop - 2) == QuickLayout.NEUTRAL, "above the list neutral");
        check(l.hit(p.frameL - 2, 600) == QuickLayout.OUTSIDE && l.hit(cx, p.frameB + 2) == QuickLayout.OUTSIDE && l.hit(20, 600) == QuickLayout.OUTSIDE, "outside the frame");

        QuickModel menu = new QuickModel();
        menu.setEntries(true, true, true, true, true);
        for (float[] size : new float[][]{{2688, 1216, 520}, {800, 360, 440}}) {
            GameStyleGeometry.Panel q = panel(size[0], size[1], size[2]);
            l.compute(q, menu.count());
            check(l.n == menu.count(), "layout and menu contain the same entries");
            inside(l, q, "complete menu " + (int) size[0] + "x" + (int) size[1]);
            for (int i = 0; i < menu.count(); i++) {
                int hit = l.hit(l.bx + l.bw / 2, l.btnTop(i) + l.bh / 2);
                menu.press(hit);
                check(menu.release(hit) / 2 == menu.id(i), "drawn entry " + i + " receives its action");
            }
            int x = l.hit((q.hitCloseL + q.hitCloseR) / 2, (q.hitCloseT + q.hitCloseB) / 2);
            menu.press(x);
            check(menu.release(x) == QuickModel.TAP_CLOSE, "a tap on the X closes");
        }
        System.out.println("QuickLayoutTest: " + checks + " checks, " + fails + " failed");
        if (fails != 0) System.exit(1);
    }
}
