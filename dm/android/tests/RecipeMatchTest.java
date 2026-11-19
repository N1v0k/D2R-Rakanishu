package com.xiaoboluo.d2r;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Map;


public class RecipeMatchTest {
    static int failed;

    static void check(String what, Object got, Object want) {
        boolean ok = String.valueOf(got).equals(String.valueOf(want));
        if (!ok) failed++;
        System.out.println((ok ? "ok   " : "FAIL ") + what + ": " + got + (ok ? "" : " (want " + want + ")"));
    }

    static RecipeModel.Input in(String id, String kind, int n, String... flags) {
        RecipeModel.Input i = new RecipeModel.Input();
        i.id = id; i.kind = kind; i.count = n; i.name = id; i.img = id; i.flags.addAll(Arrays.asList(flags));
        return i;
    }

    static RecipeModel.Card card(String cat, String out, int order, RecipeModel.Input... inputs) {
        RecipeModel.Card c = new RecipeModel.Card();
        c.cat = cat; c.out = out; c.title = out; c.order = order; c.in = inputs;
        return c;
    }

    static RecipeModel.Catalog catalog() {
        RecipeModel.Catalog d = new RecipeModel.Catalog();
        for (String n : new String[]{"a", "b", "c", "d", "e"}) { RecipeModel.Item i = new RecipeModel.Item("Stein " + n, "Stone " + n, "tstone", "", 0, ""); i.shortDe = n.toUpperCase(); i.shortEn = n.toUpperCase(); d.items.put("s" + n, i); }
        d.items.put(RecipeMatch.CUBE_CODE, new RecipeModel.Item("Kasten", "Box", "tmisc", "", 0, ""));
        d.items.put("kiw", new RecipeModel.Item("Kiesel", "Pebble", "tpeb", "tsmall", 0, ""));
        d.items.put("hat", new RecipeModel.Item("Hut", "Hat", "thead", "", 0, "hat"));
        d.items.put("hat2", new RecipeModel.Item("Hoher Hut", "Tall Hat", "thead", "", 1, "hat"));
        d.items.put("rod", new RecipeModel.Item("Rute", "Rod", "tlong", "", 0, "rod"));
        d.items.put("nek", new RecipeModel.Item("Kette", "Chain", "tneck", "", 0, ""));
        d.typeUp.put("tlong", Arrays.asList("tarm", "tgear"));
        d.typeUp.put("thead", Arrays.asList("twear"));
        d.typeUp.put("tpeb", Arrays.asList("tsmallish"));
        d.typeUp.put("tneck", new ArrayList<>());
        d.typeUp.put("tstone", Arrays.asList(RecipeMatch.POUCH_TYPES[0]));
        d.vault.put("u7", "nek");
        d.cube.add(card("cata", "sb", 2, in("sa", "i", 3)));
        d.cube.add(card("cata", "sc", 3, in("sb", "i", 3)));
        d.cube.add(card("cata", "sd", 4, in("sc", "i", 3)));
        RecipeModel.Card word = card(null, "", 27, in("sb", "i", 1), in("sa", "i", 1), in("se", "i", 1));
        word.title = "Wort"; word.sockets = 3; word.itype.add("tlong");
        d.words.add(word);
        return d;
    }

    static RecipeStock stock(RecipeStock.Entry... e) {
        RecipeStock s = new RecipeStock();
        s.difficulty = 0; s.ladderFree = true; s.entries.addAll(Arrays.asList(e));
        return s;
    }

    static RecipeStock.Entry stone(String code, int n, int place) { return new RecipeStock.Entry(code, n, place, 2, 0, 0, 0, 0); }

    static String status(RecipeModel.Catalog d, RecipeModel.Card c, RecipeStock s) {
        RecipeMatch.Result r = new RecipeMatch(d, s, true).evaluate(c);
        return r.status + " " + r.text;
    }

    static RecipeStock.Entry at(String code, int place, int x, int y, int id, int quality, int sockets) {
        RecipeStock.Entry e = new RecipeStock.Entry(code, 1, place, quality, sockets, 0, 0, 0);
        e.x = x; e.y = y; e.id = id;
        return e;
    }

    static String plan(RecipeModel.Catalog d, RecipeModel.Card c, RecipeStock s) {
        RecipeInsert.Plan p = RecipeInsert.plan(c, d, s, new RecipeMatch(d, s, true));
        if (p.steps == null) return p.outcome + " -";
        List<Integer> shown = new ArrayList<>();
        for (int i = 0; i < p.steps.length; i++) if (i % RecipeInsert.STEP_INTS < 9) shown.add(p.steps[i]);
        return p.outcome + " " + shown;
    }

    static void insertTests() {
        RecipeModel.Catalog d = catalog();
        RecipeModel.Card makeC = d.cube.get(1), word = d.words.get(0);
        final int INV = RecipeStock.INVENTORY, CUBE = RecipeStock.CUBE, STASH = RecipeStock.STASH;
        d.items.get(RecipeMatch.CUBE_CODE).w = 2; d.items.get(RecipeMatch.CUBE_CODE).h = 2;
        RecipeStock.Entry box = at(RecipeMatch.CUBE_CODE, INV, 8, 0, 1, 2, 0);
        check("three from the inventory", plan(d, makeC, stock(box, at("sb", INV, 0, 0, 11, 2, 0), at("sb", INV, 1, 0, 12, 2, 0), at("sb", INV, 2, 0, 13, 2, 0))),
            RecipeInsert.OK + " [11, 0, 0, 0, 0, 0, 3, 0, 0, 12, 0, 0, 1, 0, 0, 3, 1, 0, 13, 0, 0, 2, 0, 0, 3, 2, 0]");
        check("a rune word goes nowhere", plan(d, word, stock(box)).charAt(0) - '0', RecipeInsert.RUNEWORD);
        check("no cube", plan(d, makeC, stock(at("sb", INV, 0, 0, 11, 2, 0))).charAt(0) - '0', RecipeInsert.NO_CUBE);
        RecipeStock.Entry boxAway = at(RecipeMatch.CUBE_CODE, STASH, 8, 0, 1, 2, 0);
        check("cube in the stash", plan(d, makeC, stock(boxAway)).charAt(0) - '0', RecipeInsert.NO_CUBE);
        RecipeStock far = stock(box, at("sb", STASH, 0, 0, 11, 2, 0), at("sb", STASH, 1, 0, 12, 2, 0), at("sb", INV, 0, 0, 13, 2, 0));
        check("in the stash, too far", plan(d, makeC, far).charAt(0) - '0', RecipeInsert.FAR);
        far.near = true;
        check("in the stash, near", plan(d, makeC, far),
            RecipeInsert.OK + " [13, 0, 0, 0, 0, 0, 3, 0, 0, 11, 0, 4, 0, 0, 0, 3, 1, 0, 12, 0, 4, 1, 0, 0, 3, 2, 0]");
        RecipeStock page = stock(box, stone("sb", 3, RecipeStock.OTHER_PAGE)); page.near = true;
        check("on another page", plan(d, makeC, page).charAt(0) - '0', RecipeInsert.ON_PAGE);
        RecipeStock pouch = stock(box, stone("sb", 3, RecipeStock.POUCH)); pouch.near = true;
        check("in the pouch", plan(d, makeC, pouch).substring(0, 2).trim(), RecipeInsert.ELSEWHERE);
        check("missing", plan(d, makeC, stock(box)).charAt(0) - '0', RecipeInsert.MISSING);
        RecipeStock stale = stock(box, at("sb", INV, 0, 0, 11, 2, 0)); stale.stale = 1;
        check("stale", plan(d, makeC, stale).charAt(0) - '0', RecipeInsert.UNSURE);
        RecipeModel.Card quest = card("x", "", 1, in("sb", "i", 1)); quest.quest = true;
        check("quest", plan(d, quest, stock(box, at("sb", INV, 0, 0, 11, 2, 0))).charAt(0) - '0', RecipeInsert.UNSURE);
        RecipeModel.Card ring = card("x", "", 1, in("tneck", "t", 1, "mag"), in("sa", "i", 1));
        check("the only piece", plan(d, ring, stock(box, at("nek", INV, 0, 0, 21, 4, 0), at("sa", INV, 1, 0, 22, 2, 0))),
            RecipeInsert.OK + " [22, 0, 0, 1, 0, 0, 3, 0, 0, 21, 0, 0, 0, 0, 0, 3, 1, 0]");
        check("two pieces: the player chooses", plan(d, ring, stock(box, at("nek", INV, 0, 0, 21, 4, 0), at("nek", INV, 1, 0, 23, 4, 0), at("sa", INV, 2, 0, 22, 2, 0))).charAt(0) - '0', RecipeInsert.CHOOSE);
        check("the one in the cube counts as chosen", plan(d, ring, stock(box, at("nek", CUBE, 0, 0, 21, 4, 0), at("nek", INV, 1, 0, 23, 4, 0), at("sa", INV, 2, 0, 22, 2, 0))),
            RecipeInsert.OK + " [21, 3, 3, 0, 0, 0, 3, 0, 0, 22, 0, 0, 2, 0, 0, 3, 1, 0]");
        check("two in the cube, one wanted", plan(d, ring, stock(box, at("nek", CUBE, 0, 0, 21, 4, 0), at("nek", CUBE, 1, 0, 23, 4, 0), at("sa", INV, 2, 0, 22, 2, 0))).charAt(0) - '0', RecipeInsert.CHOOSE);
        check("foreign things go out first", plan(d, makeC, stock(box, at("sa", CUBE, 2, 3, 31, 2, 0), at("sb", INV, 0, 0, 11, 2, 0), at("sb", INV, 1, 0, 12, 2, 0), at("sb", INV, 2, 0, 13, 2, 0))),
            RecipeInsert.OK + " [31, 0, 3, 2, 3, 0, 0, 3, 0, 11, 0, 0, 0, 0, 0, 3, 0, 0, 12, 0, 0, 1, 0, 0, 3, 1, 0, 13, 0, 0, 2, 0, 0, 3, 2, 0]");
        List<RecipeStock.Entry> full = new ArrayList<>(Arrays.asList(box, at("sa", CUBE, 2, 3, 31, 2, 0), at("sb", CUBE, 0, 0, 11, 2, 0), at("sb", CUBE, 1, 0, 12, 2, 0), at("sb", CUBE, 2, 0, 13, 2, 0)));
        for (int y = 0; y < RecipeInsert.INV_H; y++) for (int x = 0; x < RecipeInsert.INV_W; x++) if (x < 8) full.add(at("hat", INV, x, y, 100 + x * 10 + y, 2, 0));
        for (int y = 2; y < RecipeInsert.INV_H; y++) for (int x = 8; x < RecipeInsert.INV_W; x++) full.add(at("hat", INV, x, y, 200 + x * 10 + y, 2, 0));
        check("the inventory too full", plan(d, makeC, stock(full.toArray(new RecipeStock.Entry[0]))).charAt(0) - '0', RecipeInsert.INVENTORY_FULL);
        d.items.get("hat").w = 2; d.items.get("hat").h = 3;
        RecipeModel.Card big = card("x", "", 1, in("hat", "i", 3));
        check("does not fit the cube", plan(d, big, stock(box, at("hat", INV, 0, 0, 41, 2, 0), at("hat", INV, 2, 0, 42, 2, 0), at("hat", INV, 4, 0, 43, 2, 0))).charAt(0) - '0', RecipeInsert.CUBE_FULL);
        RecipeStock.Entry st = stone("sb", 3, RecipeStock.STACKS); st.id = 51; st.unit = 9;
        RecipeStock stacks = stock(box, st); stacks.near = true;
        check("from a stack", plan(d, makeC, stacks), RecipeInsert.OK + " [51, 1, 0, 0, 0, 9, 3, 0, 0, 51, 1, 0, 0, 0, 9, 3, 1, 0, 51, 1, 0, 0, 0, 9, 3, 2, 0]");
        RecipeStock oneFree = stock(box, st);
        oneFree.near = true;
        RecipeInsert.Plan sp = RecipeInsert.plan(makeC, d, oneFree, new RecipeMatch(d, oneFree, true));
        check("a stack keeps cells for the way back", sp.steps[RecipeInsert.STEP_INTS - 2] + "," + sp.steps[RecipeInsert.STEP_INTS - 1] + " " + sp.steps[2 * RecipeInsert.STEP_INTS - 2] + "," + sp.steps[2 * RecipeInsert.STEP_INTS - 1], "0,0 1,0");
        check("the host checks code and quality", (sp.steps[9] & 255) + " " + sp.steps[10], (int) 's' + " 2");
        List<RecipeStock.Entry> packed = new ArrayList<>(Arrays.asList(box, st));
        d.items.get("hat").w = 1; d.items.get("hat").h = 1;
        for (int y = 0; y < RecipeInsert.INV_H; y++) for (int x = 0; x < RecipeInsert.INV_W; x++) if (x < 8 || y >= 2) if (!(x == 0 && y == 0)) packed.add(at("hat", INV, x, y, 300 + x * 10 + y, 2, 0));
        RecipeStock noRoom = stock(packed.toArray(new RecipeStock.Entry[0])); noRoom.near = true;
        check("no room for the way back", plan(d, makeC, noRoom).charAt(0) - '0', RecipeInsert.INVENTORY_FULL);
        RecipeModel.Card counted = card("x", "", 1, in("sb", "i", 1)); counted.num = 2;
        check("numinputs must match", plan(d, counted, stock(box, at("sb", INV, 0, 0, 11, 2, 0))).charAt(0) - '0', RecipeInsert.UNSURE);
        RecipeStock done = stock(box, at("sb", CUBE, 0, 0, 11, 2, 0), at("sb", CUBE, 1, 0, 12, 2, 0), at("sb", CUBE, 2, 0, 13, 2, 0));
        RecipeInsert.Plan none = RecipeInsert.plan(makeC, d, done, new RecipeMatch(d, done, true));
        check("nothing to move, all still checked", none.outcome + " " + none.moves + " " + none.steps.length / RecipeInsert.STEP_INTS, RecipeInsert.OK + " 0 3");
        RecipeStock.Entry two = stone("sb", 2, RecipeStock.STACKS); two.id = 52;
        RecipeStock shortStack = stock(box, two); shortStack.near = true;
        check("a stack of two for three", plan(d, makeC, shortStack).charAt(0) - '0', RecipeInsert.MISSING);
        RecipeModel.Card noe = card("x", "", 1, in("thead", "t", 1, "noe"));
        check("ethereal unmeasured", plan(d, noe, stock(box, at("rod", INV, 0, 0, 61, 2, 0))).charAt(0) - '0', RecipeInsert.UNSURE);
    }

    public static void main(String[] args) {
        RecipeModel.Catalog d = catalog();
        RecipeModel.Card makeB = d.cube.get(0), makeC = d.cube.get(1), makeD = d.cube.get(2), word = d.words.get(0);
        final int INV = RecipeStock.INVENTORY, STASH = RecipeStock.STASH;

        check("3 in the inventory", status(d, makeC, stock(stone("sb", 3, INV))), RecipeMatch.READY + " bereit");
        check("3 in the stash", status(d, makeC, stock(stone("sb", 3, STASH))), RecipeMatch.IN_STASH + " im Lager");
        check("3 in the stacks", status(d, makeC, stock(stone("sb", 3, RecipeStock.STACKS))), RecipeMatch.IN_STASH + " im Lager");
        check("in the pouch", status(d, makeC, stock(stone("sb", 3, RecipeStock.POUCH))), RecipeMatch.IN_STASH + " im Lager");
        check("only the level below", status(d, makeC, stock(stone("sa", 9, INV))), RecipeMatch.CHAIN + " C: aus 9 A");
        check("two levels, part held", status(d, makeD, stock(stone("sc", 1, INV), stone("sb", 6, INV))), RecipeMatch.CHAIN + " D: aus 3 C, davon 2 aus 6 B");
        check("one short", status(d, makeC, stock(stone("sb", 2, INV))), RecipeMatch.ALMOST + " fehlt: B");
        check("nothing held", status(d, makeC, stock()), RecipeMatch.NONE + " fehlt: B");
        check("too little for the chain", status(d, makeC, stock(stone("sa", 8, INV))), RecipeMatch.NONE + " fehlt: B");
        check("the chain spends each once", status(d, makeD, stock(stone("sb", 8, INV))), RecipeMatch.NONE + " fehlt: C");
        check("the cube is no ingredient", status(d, makeC, stock(new RecipeStock.Entry(RecipeMatch.CUBE_CODE, 3, INV, 2, 0, 0, 0, 0))), RecipeMatch.NONE + " fehlt: B");

        RecipeStock.Entry base = new RecipeStock.Entry("rod", 1, INV, 2, 3, 0, 0, 0);
        check("word lacks one", status(d, word, stock(stone("sb", 1, INV), stone("sa", 1, INV), base)), RecipeMatch.ALMOST + " fehlt: E");
        check("word all there", status(d, word, stock(stone("sb", 1, INV), stone("sa", 1, INV), stone("se", 1, INV), base)), RecipeMatch.READY + " bereit");
        RecipeMatch.Result r = new RecipeMatch(d, stock(stone("sb", 1, INV), stone("sa", 1, INV), base), true).evaluate(word);
        check("base named", r.where, "Inventar 2, Basis: Rute in Inventar");
        check("input state", Arrays.toString(r.state), "[0, 0, 1]");
        RecipeStock.Entry full = new RecipeStock.Entry("rod", 1, INV, 2, 3, 1, 0, 0), four = new RecipeStock.Entry("rod", 1, INV, 2, 4, 0, 0, 0), magic = new RecipeStock.Entry("rod", 1, INV, 4, 3, 0, 0, 0);
        RecipeStock.Entry formed = new RecipeStock.Entry("rod", 1, INV, 2, 3, 0, RecipeStock.RUNEWORD, 0), barred = new RecipeStock.Entry("rod", 1, INV, 2, 3, 0, RecipeStock.NO_WORD, 0);
        for (RecipeStock.Entry e : new RecipeStock.Entry[]{full, four, magic, formed, barred})
            check("no base", status(d, word, stock(stone("sb", 1, INV), stone("sa", 1, INV), stone("se", 1, INV), e)), RecipeMatch.ALMOST + " fehlt: Basis");

        RecipeMatch m = new RecipeMatch(d, stock(), true);
        RecipeStock.Entry hat = new RecipeStock.Entry("hat", 1, INV, 4, 0, 0, 0, 0), tall = new RecipeStock.Entry("hat2", 1, INV, 4, 0, 0, 0, 0), eth = new RecipeStock.Entry("hat", 1, INV, 4, 0, 0, RecipeStock.ETHEREAL, 0);
        check("exact code only", m.fits(in("hat", "i", 1, "mag"), tall), false);
        check("upg takes the higher tier", m.fits(in("hat", "i", 1, "mag", "upg"), tall), true);
        check("upg not the lower", m.fits(in("hat2", "i", 1, "mag", "upg"), hat), false);
        check("quality", m.fits(in("hat", "i", 1, "rar"), hat), false);
        check("type through its parents", m.fits(in("twear", "t", 1, "mag"), hat), true);
        check("wrong type", m.fits(in("tgear", "t", 1), hat), false);
        check("second type", m.fits(in("tsmall", "t", 1), new RecipeStock.Entry("kiw", 1, INV, 2, 0, 0, 0, 0)), true);
        check("any", m.fits(in("any", "t", 1, "mag"), hat), true);
        check("tier flags", m.fits(in("twear", "t", 1, "exc"), tall) + " " + m.fits(in("twear", "t", 1, "bas"), tall), "true false");
        check("sockets", m.fits(in("twear", "t", 1, "nos"), hat) + " " + m.fits(in("twear", "t", 1, "sock"), hat), "true false");
        check("ethereal", m.fits(in("twear", "t", 1, "noe"), eth) + " " + m.fits(in("twear", "t", 1, "eth"), eth), "false true");
        RecipeModel.Input named = in("x", "u", 1); named.row = 7;
        check("unique by row", m.fits(named, new RecipeStock.Entry("nek", 1, INV, 7, 0, 0, 0, 7)) + " " + m.fits(named, new RecipeStock.Entry("nek", 1, INV, 7, 0, 0, 0, 8)), "true false");
        RecipeStock.Entry kept = new RecipeStock.Entry("", 1, RecipeStock.VAULT, 7, 0, 0, RecipeStock.UNKNOWN, 7);
        check("unique in the chronicle", status(d, card("x", "", 1, named), stock(kept)), RecipeMatch.IN_STASH + " im Lager");
        check("chronicle piece gets its code", status(d, card("x", "", 1, in("tneck", "t", 1, "uni")), stock(new RecipeStock.Entry("", 1, RecipeStock.VAULT, 7, 0, 0, RecipeStock.UNKNOWN, 7))), RecipeMatch.PROBABLE + " wahrscheinlich");

        RecipeModel.Card two = card("x", "", 1, in("tneck", "t", 1, "mag"), in("any", "t", 1, "mag"));
        check("one item fills one input", status(d, two, stock(new RecipeStock.Entry("nek", 1, INV, 4, 0, 0, 0, 0))).charAt(0) - '0', RecipeMatch.ALMOST);
        RecipeModel.Card hell = card("x", "", 1, in("sb", "i", 1)); hell.diff = 2;
        check("hell only", status(d, hell, stock(stone("sb", 1, INV))), RecipeMatch.IN_STASH + " nur in der Hölle");
        RecipeStock s = stock(stone("sb", 1, INV)); s.difficulty = 2;
        check("in hell", status(d, hell, s), RecipeMatch.READY + " bereit");
        RecipeStock.Entry odd = stone("sb", 3, INV); odd.flags = RecipeStock.UNKNOWN;
        check("unknown is never ready", status(d, makeC, stock(odd)), RecipeMatch.PROBABLE + " wahrscheinlich");
        RecipeStock cut = stock(stone("sb", 3, INV)); cut.truncated = true;
        check("cut off is never ready", status(d, makeC, cut), RecipeMatch.PROBABLE + " wahrscheinlich");
        makeC.locked = true;
        RecipeStock ladder = stock(stone("sb", 3, INV)); ladder.ladderFree = false;
        check("ladder rule open", status(d, makeC, ladder), RecipeMatch.PROBABLE + " wahrscheinlich");
        makeC.locked = false;
        RecipeModel.Card quest = card("x", "", 1, in("sb", "i", 1)); quest.quest = true;
        check("quest cannot be read", status(d, quest, stock(stone("sb", 1, INV))), RecipeMatch.PROBABLE + " wahrscheinlich, Quest nötig");
        RecipeModel.Card questHell = card("x", "", 1, in("sb", "i", 1)); questHell.quest = true; questHell.diff = 2;
        check("quest and hell", status(d, questHell, stock(stone("sb", 1, INV))), RecipeMatch.IN_STASH + " nur in der Hölle");
        RecipeModel.Card pure = card("x", "", 1, in("twear", "t", 1, "noe"));
        check("ethereal unmeasured", status(d, pure, stock(new RecipeStock.Entry("hat", 1, INV, 4, 0, 0, 0, 0))), RecipeMatch.PROBABLE + " wahrscheinlich");
        RecipeStock old = stock(stone("sb", 3, RecipeStock.POUCH)); old.stale = 1;
        check("stale pouch", status(d, makeC, old), RecipeMatch.PROBABLE + " wahrscheinlich");
        RecipeStock oldVault = stock(stone("sb", 3, RecipeStock.POUCH)); oldVault.stale = 2;
        check("only the vault stale", status(d, makeC, oldVault), RecipeMatch.IN_STASH + " im Lager");

        RecipeStock emptyOld = stock(); emptyOld.stale = 1;
        check("lacking, may lie in the old pouch", status(d, makeC, emptyOld), RecipeMatch.PROBABLE + " Bestand unvollständig");
        RecipeModel.Card gear = card("x", "", 1, in("thead", "t", 1, "mag"));
        check("gear cannot lie in the pouch", status(d, gear, emptyOld), RecipeMatch.NONE + " fehlt: thead");
        RecipeStock oldKept = stock(); oldKept.stale = 2;
        check("the old vault does not excuse stones", status(d, makeC, oldKept), RecipeMatch.NONE + " fehlt: B");
        check("a unique may lie in the old vault", status(d, card("x", "", 1, named), oldKept), RecipeMatch.PROBABLE + " Bestand unvollständig");
        RecipeStock noPages = stock(); noPages.stale = 4;
        check("pages left out", status(d, gear, noPages), RecipeMatch.PROBABLE + " Bestand unvollständig");

        Map<RecipeModel.Card, RecipeMatch.Result> staleRes = new RecipeMatch(d, emptyOld, true).evaluateAll();
        check("nothing held stays out of Doable", RecipeMatch.doable(new ArrayList<>(d.cube), staleRes, Arrays.asList("cata")).size(), 0);

        RecipeStock mix = stock(stone("sb", 3, INV), stone("sa", 9, STASH));
        Map<RecipeModel.Card, RecipeMatch.Result> res = new RecipeMatch(d, mix, true).evaluateAll();
        List<RecipeModel.Card> cards = new ArrayList<>(d.cube);
        cards.addAll(d.words);
        StringBuilder order = new StringBuilder();
        for (RecipeModel.Card c : RecipeMatch.doable(cards, res, Arrays.asList("cata"))) order.append(c.title).append(' ');
        check("order", order.toString().trim(), "sc sb");

        insertTests();
        check("game lines come bottom up, colours gone", RecipeModel.gameLines("\ue07e3Zeile unten\n\n\ue07eU(1-2)\ue07e3 oben\n'AB'\n\ue07e5\ue07e4Titel", "Titel"), "[(1-2) oben, Zeile unten]");

        ByteBuffer b = ByteBuffer.allocateDirect(44).order(ByteOrder.LITTLE_ENDIAN);
        b.putInt(0, 7); b.putInt(4, 1); b.put(8, (byte) 3); b.put(9, (byte) 2); b.put(10, (byte) 1); b.put(12, (byte) 1); b.put(13, (byte) 5); b.put(14, (byte) 3); b.put(15, (byte) 2);
        byte[] code = {'s', 'b', ' ', ' '};
        b.put(16, (byte) 1); b.put(17, (byte) 1); b.put(18, (byte) 2);
        for (int i = 0; i < 4; i++) b.put(20 + i, code[i]);
        b.putShort(24, (short) 7); b.put(26, (byte) 5); b.put(27, (byte) 2); b.putShort(32, (short) 42); b.put(34, (byte) 8); b.put(35, (byte) 9); b.putInt(36, 77); b.putInt(40, 88);
        RecipeStock p = RecipeStock.parse(b, 44);
        check("parse new fields", p.near + " " + p.stashOpen + " " + p.insert + " " + p.entries.get(0).x + " " + p.entries.get(0).y + " " + p.entries.get(0).id + " " + p.entries.get(0).unit, "true true 2 8 9 77 88");
        check("parse", p.gen + " " + p.dataVersion + " " + p.difficulty + " " + p.ladderFree + " " + p.pages + " " + p.seen + " " + p.stale + " " + p.entries.get(0).code + " " + p.entries.get(0).qty + " " + p.entries.get(0).place + " " + p.entries.get(0).row, "7 3 2 true 5 3 2 sb 7 5 42");
        check("short reading", RecipeStock.parse(b, 30), null);

        System.out.println(failed == 0 ? "all ok" : failed + " FAILED");
        System.exit(failed == 0 ? 0 : 1);
    }
}
