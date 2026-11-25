package com.xiaoboluo.d2r;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;




final class BookModelTest {
    private static void check(boolean value, String what) { if (!value) throw new AssertionError(what); }
    private static final String HERO = "hero\tTest\t1\t30\t1\t2\t500\t200\t0\t0\t0\t8\t1\t1\t43\t0\t0\n";


    private static String enemy(int id, String name, int kind, int act, int area, double kill, double death, int light, String kindKey) {
        return "enemy\t" + id + "\t" + name + "\t" + kind + "\t" + act + "\t" + area + "\t30\t700\t40.0\t" + kill + "\t" + death + "\t0.2\t10\t" + light
            + "\t0\t0\t0\t0,0,0,0,0,0\t" + name.toLowerCase() + "\t" + kindKey + "\t0\n";
    }
    private static String keys(List<BookModel.Tile> tiles) {
        StringBuilder s = new StringBuilder();
        for (BookModel.Tile t : tiles) s.append(s.length() == 0 ? "" : " ").append(t.kind.key).append(':').append(t.badge);
        return s.toString();
    }

    public static void main(String[] args) {
        String report = HERO
            + enemy(0, "Std", 10, 2, 0, 5, 50, 0, "")
            + enemy(1, "Zombie", 0, 1, 2, 3, 100, 0, "M10")
            + enemy(2, "Zombie", 0, 2, 43, 6, 80, 1, "M10")
            + enemy(3, "Jackal", 0, 2, 43, 2, -1, 0, "M20")
            + enemy(4, "Jackal", 0, 2, 44, 4, 90, -1, "M20")
            + enemy(5, "Radament", 6, 2, 49, 30, 20, 2, "M229")
            + enemy(6, "Créature", 4, 2, 43, 9, 60, 1, "S12")
            + enemy(7, "Duriel", 5, 2, 73, 60, 10, 2, "M211");
        CombatModel m = CombatModel.parse(report);
        BookModel b = BookModel.of(m, Arrays.asList(new BookModel.Listed("M300", "minion1", "Minion"), new BookModel.Listed("M10", "x", "Dup")));
        check(b.kinds.size() == 6, "kinds " + b.kinds.size());
        check(b.find("M10").encounters.size() == 2 && b.find("M10").nameKey.equals("Zombie") && b.find("M10").look.equals("zombie"), "zombie folded");
        check(b.find("M300").encounters.isEmpty() && b.find("M300").look.equals("minion1"), "a listed kind without encounters");
        check(b.find("") == null && b.find(null) == null, "no kind for the standard enemy");
        CombatModel.Names names = (key, lang) -> key.equals("Créature") ? (lang == 1 ? "Kreatur" : "Créature") : key;


        BookModel.Filter f = new BookModel.Filter();
        check(keys(b.tiles(f, names)).equals("M10:1 M20:-1 S12:1 M229:2 M211:2 M300:-1"), "all " + keys(b.tiles(f, names)));

        f.act = 1;
        check(keys(b.tiles(f, names)).equals("M10:0"), "act 1 " + keys(b.tiles(f, names)));

        f.act = 2; f.area = 43;
        check(keys(b.tiles(f, names)).equals("M10:1 M20:0 S12:1"), "area 43 " + keys(b.tiles(f, names)));

        f.area = 0; f.type = CombatModel.SUPERUNIQUE;
        check(keys(b.tiles(f, names)).equals("S12:1 M229:2"), "mini bosses " + keys(b.tiles(f, names)));
        f.type = CombatModel.ACT_BOSS;
        check(keys(b.tiles(f, names)).equals("M211:2"), "bosses");
        f.type = -1;

        f.sort = BookModel.SORT_KILL;
        check(keys(b.tiles(f, names)).equals("M20:-1 M10:1 S12:1 M229:2 M211:2"), "kill " + keys(b.tiles(f, names)));
        f.sort = BookModel.SORT_DEATH;
        check(keys(b.tiles(f, names)).equals("M211:2 M229:2 S12:1 M10:1 M20:-1"), "death " + keys(b.tiles(f, names)));
        f.sort = BookModel.SORT_RATING;
        check(keys(b.tiles(f, names)).equals("M10:1 S12:1 M229:2 M211:2 M20:-1"), "rating " + keys(b.tiles(f, names)));
        f.descending = true;
        check(keys(b.tiles(f, names)).equals("M20:-1 M229:2 M211:2 M10:1 S12:1"), "rating down " + keys(b.tiles(f, names)));
        f.descending = false; f.sort = BookModel.SORT_PLACE;

        f.act = 0;
        check(keys(b.tiles(f, names)).equals("M10:1 M20:-1 S12:1 M229:2 M211:2 M300:-1"), "place " + keys(b.tiles(f, names)));

        f.query = "kreatur";
        check(keys(b.tiles(f, names)).equals("S12:1"), "German name");
        f.query = "CREATURE";
        check(keys(b.tiles(f, names)).equals("S12:1"), "without accents");

        CombatModel.Names many = (key, lang) -> !key.equals("Zombie") ? key : lang == 0 ? "Prüfgegner" : lang == 5 ? "시험 적" : lang == 12 ? "Inimigo Ígneo" : "Test enemy";
        for (String q : new String[]{"pruf", "시험", "igneo", "Ígneo"}) { f.query = q; check(keys(b.tiles(f, many)).equals("M10:1"), "found by " + q); }
        f.query = "minion";
        check(keys(b.tiles(f, names)).equals("M300:-1"), "a listed kind by its name");
        f.query = "";

        BookModel w = BookModel.of(m, Arrays.asList(new BookModel.Listed("M301", "egg", "Egg", Arrays.asList("M10"))));
        BookModel.Filter a1 = new BookModel.Filter(); a1.act = 1;
        check(keys(w.tiles(a1, names)).equals("M10:0 M301:-1"), "a listed kind in its parent's act " + keys(w.tiles(a1, names)));
        a1.act = 3;
        check(keys(w.tiles(a1, names)).isEmpty(), "not where its parent is not");
        check(w.place(w.find("M301"), 43, f).id == 2 && w.place(w.find("M10"), 0, f) == null, "the listed kind's place");

        BookModel.Drops drops = (k, e) -> k.key.equals("M20") ? (e.area == 44 ? 0.5 : 0.1) : k.key.equals("M301") ? 0.2 : k.key.equals("M211") ? 0.05 : 0;
        BookModel.Filter q = new BookModel.Filter(); q.query = "zz";
        List<BookModel.Tile> found = w.tiles(q, names, drops);
        check(keys(found).equals("M20:-1 M301:-1 M211:2") && found.get(0).chance == 0.5, "droppers by chance " + keys(found));
        q.act = 2; q.area = 43;
        List<BookModel.Tile> in43 = w.tiles(q, names, drops);
        check(keys(in43).equals(keys(found)) && in43.get(0).chance == 0.5, "a search looks past the act and area filters " + keys(in43));

        BookModel.Filter zombie = new BookModel.Filter(); zombie.query = "zombie";
        List<BookModel.Tile> both = w.tiles(zombie, names, drops);
        check(keys(both).equals("M10:1 M20:-1 M301:-1 M211:2") && both.get(0).chance == 0 && both.get(1).chance == 0.5, "the named kind first " + keys(both));
        BookModel.Drops rarely = (k, e) -> k.key.equals("M10") ? 0.01 : drops.chance(k, e);
        List<BookModel.Tile> bothRare = w.tiles(zombie, names, rarely);
        check(keys(bothRare).equals(keys(both)) && bothRare.get(0).chance == 0.01, "the named kind first with its chance " + keys(bothRare));
        BookModel.Filter byName = new BookModel.Filter(); byName.act = 1; byName.query = "kreatur";
        check(keys(b.tiles(byName, names)).equals("S12:1"), "a name found in another act than the filter's");


        BookModel.Filter act1 = new BookModel.Filter(); act1.act = 1;
        check(BookModel.encounter(b.find("M10"), 43, act1).id == 2, "the wanted area");
        check(BookModel.encounter(b.find("M10"), 0, act1).id == 1, "the hardest within the filter");
        check(BookModel.encounter(b.find("M10"), 99, f).id == 2, "no such area: the hardest");
        check(BookModel.encounter(b.find("M300"), 0, f) == null, "a listed kind has no encounter");

        String nightmare = HERO + enemy(1, "Jackal", 0, 2, 44, 4, 90, 0, "M20") + enemy(2, "Zombie", 0, 2, 43, 6, 80, 2, "M10");
        BookModel n = BookModel.of(CombatModel.parse(nightmare), new ArrayList<>());
        CombatModel.Enemy again = BookModel.encounter(n.find("M10"), 43, f);
        check(again.id == 2 && again.light == 2 && n.find("M229") == null, "rematch by kind and area");

        String guessed = HERO + enemy(1, "Ghost", 4, 2, 43, 3, 50, 0, "S30").replace("\tS30\t0\n", "\tS30\t1\n") + enemy(2, "Bat", 0, 2, 44, 3, 50, 0, "M30")
            + enemy(3, "Bat", 0, 1, 7, 3, 50, 0, "M30") + enemy(4, "Clone", 9, 0, 0, 3, 50, 0, "M333");
        BookModel gb = BookModel.of(CombatModel.parse(guessed), null);
        BookModel.Filter here = new BookModel.Filter(); here.act = 2; here.area = 43;
        check(b.tiles(here, names).size() == 3 && gb.tiles(here, names).isEmpty(), "a guessed area does not count for the area filter");
        here.area = 0;
        check(keys(gb.tiles(here, names)).equals("S30:0 M30:0"), "the act filter keeps it " + keys(gb.tiles(here, names)));
        List<CombatModel.Enemy> places = BookModel.places(gb.find("M30"));
        check(places.size() == 2 && places.get(0).area == 7 && places.get(1).area == 44, "places by act and area");
        check(BookModel.places(gb.find("M333")).isEmpty() && BookModel.places(null).isEmpty(), "no area, no place");

        BookModel.Kills k = BookModel.kills("book\t0\t1\nkill\tM10\t3\t0\t1\nkill\tS12\t0\t2\t0\n");
        check(k.known() && k.count("M10", 0) == 3 && k.count("M10", 1) == 0 && k.count("S12", 1) == 2 && k.count("M99", 0) == 0 && k.count("M10", 3) == 0, "kills");
        check(!BookModel.kills("book\t1\t0\n").known(), "resting");
        for (String bad : new String[]{"", "kill\tM1\t1\t1\t1\n", "book\t0\t0\nkill\tX1\t1\t1\t1\n", "book\t0\t0\nkill\tM1\t1\t1\n", "book\t0\t0\nkill\tM1\t-1\t0\t0\n", "book\t0\t0\nbook\t0\t0\n", "book\tx\t0\n"})
            try { BookModel.kills(bad); throw new AssertionError("accepted " + bad); } catch (IllegalArgumentException expected) { }

        check(Arrays.toString(BookModel.Where.choices(true)).equals("[6, 1, 2, 3, 4, 5, 0]") && Arrays.toString(BookModel.Where.choices(false)).equals("[1, 2, 3, 4, 5, 0]"), "choices");
        check(BookModel.Where.first(true, 2) == BookModel.Where.AREA && BookModel.Where.first(false, 2) == 2 && BookModel.Where.first(false, 0) == BookModel.Where.ALL, "first choice");
        check(BookModel.Where.step(BookModel.Where.AREA, -1, true) == BookModel.Where.AREA && BookModel.Where.step(BookModel.Where.AREA, 1, true) == 1, "step from the area");
        check(BookModel.Where.step(5, 1, false) == BookModel.Where.ALL && BookModel.Where.step(BookModel.Where.ALL, 1, false) == BookModel.Where.ALL && BookModel.Where.step(1, -1, false) == 1, "the ends stop");
        check(BookModel.Where.step(BookModel.Where.AREA, 1, false) == 2, "a choice the list lacks steps from the first");
        check(BookModel.Where.kept(BookModel.Where.AREA, false, 3) == 3 && BookModel.Where.kept(BookModel.Where.AREA, true, 3) == BookModel.Where.AREA && BookModel.Where.kept(4, false, 3) == 4, "kept");
        BookModel.Filter wf = new BookModel.Filter();
        BookModel.Where.apply(BookModel.Where.AREA, 43, wf);
        check(wf.act == 0 && wf.area == 43 && keys(b.tiles(wf, names)).equals("M10:1 M20:0 S12:1"), "the area " + keys(b.tiles(wf, names)));
        BookModel.Where.apply(1, 43, wf);
        check(wf.act == 1 && wf.area == 0 && keys(b.tiles(wf, names)).equals("M10:0"), "an act " + keys(b.tiles(wf, names)));
        BookModel.Where.apply(BookModel.Where.ALL, 43, wf);
        check(wf.act == 0 && wf.area == 0 && b.tiles(wf, names).size() == 6, "every act");
        check(BookModel.Where.areaHas(m, 43) && !BookModel.Where.areaHas(m, 50) && !BookModel.Where.areaHas(m, 0) && !BookModel.Where.areaHas(null, 43), "an area with enemies");
        check(!BookModel.Where.areaHas(CombatModel.parse(guessed), 43), "a guessed area has none");

        String hellOnly = HERO + enemy(1, "Imp", 0, 5, 107, 3, 50, 0, "M400");
        check(!BookModel.Where.areaHas(m, 107) && BookModel.Where.areaHas(CombatModel.parse(hellOnly), 107), "per difficulty");

        check(BookModel.rank(0) < BookModel.rank(-1) && BookModel.rank(-1) < BookModel.rank(1) && BookModel.rank(1) < BookModel.rank(2), "rank");
        System.out.println("BookModelTest: kinds, filters, orders, badges, search, listed kinds and the kind page's encounter passed");
    }
}
