



package com.xiaoboluo.d2r;

public final class RecipeStyleTest {
    private static int checks, fails;
    private static void check(boolean ok, String what) { checks++; if (!ok) { fails++; System.out.println("FAIL " + what); } }
    private static boolean near(float a, float b) { return Math.abs(a - b) < 1e-3f; }

    private static void colours() {
        GameStyleData d = GameStyleData.parse(null);
        check("Tooltip.MagicColor".equals(RecipeStyle.quality("magic")) && d.colour(RecipeStyle.quality("magic")) == 0xFF6E6EFF, "magic = blue #6E6EFF");
        check(d.colour(RecipeStyle.quality("crafted")) == 0xFFFFA800, "runes / crafted = orange #FFA800");
        check(d.colour(RecipeStyle.quality("rare")) == 0xFFFFFF64 && d.colour(RecipeStyle.quality("set")) == 0xFF00FC00 && d.colour(RecipeStyle.quality("unique")) == 0xFFC7B377, "rare / set / unique");
        check("Tooltip.DefaultColor".equals(RecipeStyle.quality("white")) && "Tooltip.DefaultColor".equals(RecipeStyle.quality(null)) && "Tooltip.DefaultColor".equals(RecipeStyle.quality("???")), "anything else white");
        check(d.colour(RecipeStyle.status(RecipeMatch.READY)) == 0xFF4FC238, "ready = PartyGreen");
        check(d.colour(RecipeStyle.status(RecipeMatch.NONE)) == 0xFFFC4646 && d.colour(RecipeStyle.status(RecipeMatch.ALMOST)) == 0xFFFC4646, "missing = red #FC4646");
        check(d.colour(RecipeStyle.status(RecipeMatch.IN_STASH)) == 0xFFC7B377 && d.colour(RecipeStyle.status(RecipeMatch.CHAIN)) == 0xFFC7B377, "stored / chain = gold yellow #C7B377");
        check(d.colour(RecipeStyle.status(RecipeMatch.PROBABLE)) == 0xFF949494, "not fully read = grey #949494");
        check("FontColorRed".equals(RecipeStyle.input(RecipeMatch.LACKS, "r01")) && "FontColorGoldYellow".equals(RecipeStyle.input(RecipeMatch.CHAINED, "gcv")), "ingredient missing red, chained gold");
        check("Tooltip.CraftedColor".equals(RecipeStyle.input(RecipeMatch.HAVE, "r33")) && "FontColorWhite".equals(RecipeStyle.input(RecipeMatch.HAVE, "gcv")), "a rune orange, a gem white");
        check(!RecipeStyle.rune("rin") && !RecipeStyle.rune("r1") && !RecipeStyle.rune(null) && RecipeStyle.rune("r10"), "rune codes r01-r99 only");
        String[] all = {RecipeStyle.quality("magic"), RecipeStyle.quality("rare"), RecipeStyle.quality("set"), RecipeStyle.quality("unique"), RecipeStyle.quality("crafted"),
            RecipeStyle.quality("white"), RecipeStyle.status(RecipeMatch.READY), RecipeStyle.status(RecipeMatch.IN_STASH), RecipeStyle.status(RecipeMatch.PROBABLE),
            RecipeStyle.status(RecipeMatch.NONE), RecipeStyle.input(RecipeMatch.HAVE, "x"), "TabsActiveTextColor", "TabsInactiveTextColor", "FontColorDarkGold", "Tooltip.backgroundColor"};
        for (String n : all) check(d.has(n), "built-in has " + n + " (old packs draw too)");
    }

    private static void layout() {
        check(near(RecipeStyle.k(188), 1) && near(RecipeStyle.k(94), 0.5f) && RecipeStyle.k(0) == 0, "k = row height / 188");
        check(RecipeStyle.perPage(843, 158, false) == 5 && RecipeStyle.perPage(100, 158, false) == 4 && RecipeStyle.perPage(2000, 158, false) == 6, "4 to 6 rows");
        check(RecipeStyle.perPage(340, 170, true) == 2 && RecipeStyle.perPage(50, 170, true) == 1, "keyboard up: fewer, at least 1");
        float[] c = new float[6];
        float k = 0.856f, left = 40, right = 2648;
        RecipeStyle.columns(left, right, k, 620 * k, 600 * k, c);
        check(near(c[0], left + 180 * k), "name starts where the Chronicle's text does (x 180)");
        check(near(c[5], right - 30 * k) && near(c[5] - c[4], 600 * k), "status column at the right, as wide as asked");
        check(near(c[1], c[0] + 620 * k) && near(c[2], c[1] + 30 * k) && near(c[3], c[4] - 30 * k), "name | gap | inputs | gap | status");
        RecipeStyle.columns(left, right, k, 300 * k, 430 * k, c);
        check(near(c[1] - c[0], 300 * k) && near(c[5] - c[4], 430 * k), "a short name and a short status leave more room to the inputs");
        RecipeStyle.columns(0, 1200, k, 620 * k, 600 * k, c);
        check(c[1] >= c[0] && c[1] <= c[2] && c[3] - c[2] >= RecipeStyle.MIN_INPUTS * k - 1e-3f && c[4] > c[3], "narrow row still in order");
    }


    private static final RecipeStyle.Measure MONO = s -> s.length() * 10f;

    private static void wrapping() {
        check(RecipeStyle.wrap("lädierter Amethyst", 1000, MONO).equals(java.util.Arrays.asList("lädierter Amethyst")), "fits on one line: not wrapped");
        check(RecipeStyle.wrap("lädierter Amethyst", 100, MONO).equals(java.util.Arrays.asList("lädierter", "Amethyst")), "wraps at the space, nothing cut");
        check(RecipeStyle.wrap("Hel-Rune", 50, MONO).equals(java.util.Arrays.asList("Hel-", "Rune")), "wraps after a hyphen, the hyphen stays");
        check(RecipeStyle.wrap("Bul-Kathos' Nightmare", 120, MONO).equals(java.util.Arrays.asList("Bul-Kathos'", "Nightmare")), "a hyphenated word stays together when it fits");
        check(String.join("", RecipeStyle.wrap("Festering Essence of Destruction", 110, MONO)).replace(" ", "").equals("FesteringEssenceofDestruction"), "wrapping loses no letter");
        check(RecipeStyle.lines("Regenerationstrank", 100, MONO) == Integer.MAX_VALUE && RecipeStyle.lines("Regenerationstrank", 180, MONO) == 1, "a word wider than the room does not fit");
        check(RecipeStyle.wrap("", 100, MONO).isEmpty() && RecipeStyle.wrap(null, 100, MONO).isEmpty(), "empty text, no lines");

        float need = RecipeStyle.cellNeed("lädierter Amethyst", "", MONO, MONO, 1000, 3);
        check(need >= 90 && need <= 91, "the cell needs the longer word's width when it may wrap: " + need);
        need = RecipeStyle.cellNeed("lädierter Amethyst", "", MONO, MONO, 1000, 1);
        check(need >= 180 && need <= 181, "one line: the whole name's width: " + need);
        check(RecipeStyle.cellLines("Kettenhandschuhe", "Magisch, auch höhere Stufe", RecipeStyle.cellNeed("Kettenhandschuhe", "Magisch, auch höhere Stufe", MONO, MONO, 1000, 3), MONO, MONO) <= 3, "name + condition fit in three lines at the needed width");
        check(Float.isInfinite(RecipeStyle.cellNeed("Regenerationstrank", "", MONO, MONO, 100, 3)), "no fit: infinite");

        float nw = RecipeStyle.nameNeed("Aufwerten zu Außergewöhnlich", "Waffen (Einzigartig)", MONO, MONO, 100, 400, 2);
        check(RecipeStyle.nameFits("Aufwerten zu Außergewöhnlich", "Waffen (Einzigartig)", nw, MONO, MONO, 2) && nw < 280, "a long title wraps into two lines with its description: " + nw);
        check(RecipeStyle.nameNeed("Aufwerten zu Außergewöhnlich", "", MONO, MONO, 100, 250, 1) == -1, "one line does not fit in 250");
        check(RecipeStyle.nameNeed("Eld-Rune", "", MONO, MONO, 260, 620, 1) == 260, "a short name takes the narrowest column");
    }



    private static void rows() {
        RecipeStyle.Layout l = new RecipeStyle.Layout();
        float k = 1;
        RecipeStyle.layout("fehlerhafter Amethyst", "", new String[]{"lädierter Amethyst"}, new String[]{""}, MONO, MONO, MONO, 0, 2160, k, 500, l);
        check(l.f == 1 && !l.squeezed && l.cols[1] - l.cols[0] >= 210 && RecipeStyle.cellRoom(l.slots[0], k) >= 180, "a gem row: title and ingredient on one line each");
        String[] four = {"Verdorbene Essenz des Leidens", "Geladene Essenz des Hasses", "Brennende Essenz des Schreckens", "Eiternde Essenz der Zerstörung"};
        RecipeStyle.layout("Zeichen der Absolution", "", four, new String[]{"", "", "", ""}, MONO, MONO, MONO, 0, 2160, k, 500, l);
        check(!l.squeezed, "four long essences fit without squeezing");
        float sum = 0;
        for (int i = 0; i < 4; i++) {
            sum += l.slots[i];
            RecipeStyle.Measure m = RecipeStyle.scaled(MONO, l.f);
            check(RecipeStyle.lines(four[i], RecipeStyle.cellRoom(l.slots[i], k), m) <= RecipeStyle.CELL_LINES, "essence " + i + " whole in at most three lines");
        }
        check(sum <= l.cols[3] - l.cols[2] + 0.5f, "the cells stay inside the input area");
        check(l.cols[1] <= l.cols[2] && l.cols[3] <= l.cols[4], "name | inputs | status in order");
        RecipeStyle.layout("Blut-Helm", "", new String[]{"Helm", "Juwel", "Ral-Rune", "perfekter Rubin"}, new String[]{"Magisch, auch höhere Stufe", "", "", ""}, MONO, MONO, MONO, 0, 2160, k, 500, l);
        for (int i = 0; i < 4; i++) check(l.slots[i] - RecipeStyle.cellRoom(l.slots[i], k) >= RecipeStyle.CELL_GAP * k - 1e-3f, "cell " + i + " keeps a gap to its neighbours");
    }

    private static void sharing() {
        float[] out = new float[6];
        float used = RecipeStyle.share(900, new float[]{100}, 1, out);
        check(near(out[0], 300) && near(used, 300), "one short input: a third of the area, as before");
        used = RecipeStyle.share(900, new float[]{500}, 1, out);
        check(near(out[0], 500), "one long input: as wide as it needs");
        used = RecipeStyle.share(900, new float[]{400, 100, 100, 100}, 4, out);
        check(near(used, 900) && out[0] >= 400 && out[1] >= 100 && near(out[0] - 400, out[1] - 100), "four inputs: each its need, the spare room shared evenly");
        check(RecipeStyle.share(900, new float[]{400, 400, 400}, 3, out) > 900, "no fit: reported so the caller shrinks the text");
        float[] r = new float[8];
        RecipeStyle.cellRows(2, r); check(r[0] == 48 && r[1] == 72 && r[2] == 86, "up to two lines: the picture and lines as before");
        RecipeStyle.cellRows(3, r); check(r[2] + 3 * r[3] <= RecipeStyle.ROW_UNITS && r[0] + r[1] / 2 <= r[2], "three lines fit the row below a smaller picture");
        RecipeStyle.nameRows(1, 1, r); check(r[0] == 20 && r[1] == 94 && r[2] == 94 && r[3] == 160, "title + description: the old positions");
        RecipeStyle.nameRows(2, 1, r); check(r[0] >= 0 && r[5] <= RecipeStyle.ROW_UNITS && r[3] == r[1] && r[4] == r[2], "three name lines stacked inside the row");
    }




    private static void tall() {
        RecipeStyle.Layout l = new RecipeStyle.Layout();
        float k = 1;
        String[] six = {"Latente Schwarze Bresche", "Mal-Rune", "perfekter Diamant", "Südlicher Weltsteinsplitter", "Tiefer Weltsteinsplitter", "Nördlicher Weltsteinsplitter"};
        String[] none = {"", "", "", "", "", ""};
        RecipeStyle.layout("Erneuerte Schwarze Bresche", "", six, none, MONO, MONO, MONO, 0, 1800, k, 500, l);
        check(RecipeStyle.crowded(l) && !l.tall && l.split == 6, "six long ingredients in one row only fit smaller (crowded)");
        RecipeStyle.layout("Erneuerte Schwarze Bresche", "", six, none, MONO, MONO, MONO, 0, 1800, k, 500, true, l);
        check(l.tall && l.split == 3 && l.f == 1 && !l.squeezed && !RecipeStyle.crowded(l), "two bands: three per band, the text at its normal size");
        float area = l.cols[3] - l.cols[2], top = 0, bottom = 0;
        for (int i = 0; i < 6; i++) {
            if (i < 3) top += l.slots[i]; else bottom += l.slots[i];
            check(RecipeStyle.lines(six[i], RecipeStyle.cellRoom(l.slots[i], k), MONO) <= RecipeStyle.CELL_LINES, "ingredient " + i + " whole in its cell: " + six[i]);
        }
        check(top <= area + 0.5f && bottom <= area + 0.5f && near(top, l.used) && near(bottom, l.used2), "each band stays inside the input area");
        check(l.cols[1] <= l.cols[2] && l.cols[3] <= l.cols[4], "two bands: name | inputs | status in order");
        String[] five = {"a", "b", "c", "d", "e"};
        RecipeStyle.layout("x", "", five, new String[]{"", "", "", "", ""}, MONO, MONO, MONO, 0, 2160, k, 500, true, l);
        check(l.split == 3, "five: three on top, two below");
        RecipeStyle.layout("x", "", new String[]{"lädierter Amethyst"}, new String[]{""}, MONO, MONO, MONO, 0, 2160, k, 500, true, l);
        check(!l.tall && l.split == 1, "one ingredient never takes two bands");
        RecipeStyle.layout("fehlerhafter Amethyst", "", new String[]{"lädierter Amethyst", "Juwel"}, new String[]{"", ""}, MONO, MONO, MONO, 0, 2160, k, 500, l);
        check(!RecipeStyle.crowded(l), "an ordinary row is not crowded");

        boolean[] t = {false, true, false, false, false, true, true};
        check(java.util.Arrays.equals(RecipeStyle.pages(t, 7, 4), new int[]{0, 3, 6, 7}), "4 places: [ordinary, tall, ordinary] [ordinary, ordinary, tall] [tall]");
        check(java.util.Arrays.equals(RecipeStyle.pages(t, 7, 1), new int[]{0, 1, 2, 3, 4, 5, 6, 7}), "one place (keyboard up): a tall row takes one place too");
        check(java.util.Arrays.equals(RecipeStyle.pages(new boolean[]{false, false, false, false, false}, 5, 2), new int[]{0, 2, 4, 5}), "no tall rows: as before, perPage each");
        check(java.util.Arrays.equals(RecipeStyle.pages(new boolean[0], 0, 6), new int[]{0, 0}), "no cards: one empty page");
        int[] st = RecipeStyle.pages(t, 7, 4);
        check(RecipeStyle.pageOf(st, 0) == 0 && RecipeStyle.pageOf(st, 2) == 0 && RecipeStyle.pageOf(st, 3) == 1 && RecipeStyle.pageOf(st, 5) == 1 && RecipeStyle.pageOf(st, 6) == 2, "the page of a card");
    }

    public static void main(String[] a) {
        colours(); layout(); wrapping(); sharing(); rows(); tall();
        System.out.println("RecipeStyleTest: " + checks + " checks, " + fails + " failed");
        if (fails != 0) System.exit(1);
    }
}
