package com.xiaoboluo.d2r;

import java.nio.charset.StandardCharsets;
import java.util.HashMap;
import java.util.Map;
import java.util.function.IntFunction;





public class RecipePreviewTest {
    static int failed;

    static void check(String what, Object got, Object want) {
        boolean ok = String.valueOf(got).equals(String.valueOf(want));
        if (!ok) failed++;
        System.out.println((ok ? "ok   " : "FAIL ") + what + ": " + got + (ok ? "" : " (want " + want + ")"));
    }

    static RecipeModel.Card word(String title, int row) {
        RecipeModel.Card c = new RecipeModel.Card();
        c.title = title; c.row = row; c.preview.add("Helme");
        return c;
    }


    static final class Host implements IntFunction<byte[]> {
        final Map<Integer, String> texts = new HashMap<>();
        int asks;
        @Override public byte[] apply(int row) { asks++; String t = texts.get(row); return t == null ? null : t.getBytes(StandardCharsets.UTF_8); }
    }

    static final String[] NAMES = {"WAIT", "BARE", "FULL"};
    static String name(int state) { return NAMES[state]; }

    public static void main(String[] args) {
        RecipePreview p = new RecipePreview();
        Host host = new Host();
        RecipeModel.Card nadir = word("Nadir", 86), stahl = word("Stahl", 132);
        String raw = "Nadir\n3+30 zu Stärke\n+10 Verteidigung\n'Nef + Tir'";


        check("missing at first: WAIT", name(p.state(nadir, 1000, host)), "WAIT");
        check("WAIT asks again next frame", p.nextPoll(RecipePreview.WAIT, 1000), RecipePreview.NEXT_FRAME);
        check("WAIT draws no lines", p.lines(nadir).size(), 0);
        check("still WAIT just before the hold ends", name(p.state(nadir, 1000 + RecipePreview.HOLD_MS - 1, host)), "WAIT");
        host.texts.put(86, raw);
        check("text arrives: FULL", name(p.state(nadir, 1100, host)), "FULL");
        check("FULL = the game's lines, top down, without name and rune line", p.lines(nadir), "[+10 Verteidigung, +30 zu Stärke]");
        check("FULL stops polling", p.nextPoll(RecipePreview.FULL, 1100), RecipePreview.STOP);
        int before = host.asks;
        p.state(nadir, 1200, host);
        check("FULL is kept, the host is not asked again", host.asks, before);



        p.reset();
        check("other card: WAIT", name(p.state(stahl, 5000, host)), "WAIT");
        check("after the hold: BARE", name(p.state(stahl, 5000 + RecipePreview.HOLD_MS, host)), "BARE");
        check("BARE draws no lines at all", p.lines(stahl).size(), 0);
        check("BARE asks every 250 ms", p.nextPoll(RecipePreview.BARE, 5000 + RecipePreview.HOLD_MS), RecipePreview.BARE_POLL_MS);
        check("BARE stops asking after the limit", p.nextPoll(RecipePreview.BARE, 5000 + RecipePreview.POLL_LIMIT_MS), RecipePreview.STOP);
        host.texts.put(132, "Stahl\n+20% Angriffsgeschwindigkeit");
        check("late text after BARE: FULL", name(p.state(stahl, 7000, host)), "FULL");
        check("late text lines", p.lines(stahl), "[+20% Angriffsgeschwindigkeit]");


        Host none = new Host();
        p.reset();
        check("reopen, nothing there: WAIT", name(p.state(nadir, 20000, none)), "WAIT");
        check("still WAIT later (BARE only once the hold passed)", name(p.state(nadir, 20000 + RecipePreview.HOLD_MS - 50, none)), "WAIT");
        p.reset();
        check("reopened same card: WAIT again, not BARE", name(p.state(nadir, 20000 + RecipePreview.HOLD_MS + 10, none)), "WAIT");
        check("nextPoll after reset: no card, stop", new RecipePreview().nextPoll(RecipePreview.WAIT, 0), RecipePreview.STOP);


        p.state(nadir, 30000, none);
        check("card switch without reset: WAIT", name(p.state(stahl, 30000 + RecipePreview.HOLD_MS + 1, none)), "WAIT");
        check("lines of a card not asked about: none", p.lines(nadir).size(), 0);


        RecipeModel.Card cube = new RecipeModel.Card();
        cube.cat = "gems"; cube.title = "3 Steine"; cube.preview.add("ergibt einen besseren Stein"); cube.row = 7;
        Host h2 = new Host();
        check("cube recipe: FULL at once", name(p.state(cube, 0, h2)), "FULL");
        check("cube recipe lines", p.lines(cube), "[ergibt einen besseren Stein]");
        check("cube recipe: host not asked", h2.asks, 0);

        System.out.println(failed == 0 ? "all ok" : failed + " FAILED");
        System.exit(failed == 0 ? 0 : 1);
    }
}
