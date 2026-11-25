package com.xiaoboluo.d2r;

import java.util.Arrays;
import java.util.List;
import java.util.Locale;
import java.util.TimeZone;













final class FeatsModelTest {
    private static int checks;
    private static void check(boolean value, String what) { checks++; if (!value) throw new AssertionError(what); }
    private static void bad(String text, String what) {
        checks++;
        try { FeatsModel.parse(text); } catch (IllegalArgumentException e) { return; }
        throw new AssertionError("accepted: " + what);
    }


    private static final String[] ROWS = {
        "row\t1\t1\t1\t500", "row\t2\t1\t1\t5000", "row\t3\t1\t1\t10000", "row\t44\t7\t17\t20", "row\t45\t7\t17\t65", "row\t46\t7\t17\t250",
        "row\t47\t7\t18\t64", "row\t48\t7\t19\t250", "row\t49\t7\t14\t256", "row\t50\t7\t14\t512", "row\t51\t7\t14\t1024", "row\t4\t2\t2\t1", "row\t5\t2\t2\t2", "row\t6\t2\t2\t3", "row\t7\t2\t2\t4",
        "row\t8\t2\t2\t5", "row\t9\t2\t3\t1", "row\t10\t2\t3\t2", "row\t11\t2\t26\t27", "row\t12\t2\t7\t0", "row\t13\t3\t4\t0", "row\t14\t3\t4\t1", "row\t15\t3\t4\t2",
        "row\t16\t3\t4\t3", "row\t17\t3\t4\t4", "row\t18\t3\t4\t5", "row\t19\t3\t4\t6", "row\t43\t3\t4\t7", "row\t21\t3\t5\t99", "row\t23\t3\t9\t0",
        "row\t22\t3\t8\t99", "row\t52\t8\t20\t1", "row\t53\t8\t21\t10", "row\t54\t8\t21\t30", "row\t55\t8\t22\t300", "row\t56\t8\t14\t2048",
        "row\t24\t6\t14\t1", "row\t25\t6\t14\t2", "row\t26\t6\t14\t4", "row\t27\t6\t14\t8", "row\t28\t6\t14\t16",
        "row\t29\t6\t14\t32", "row\t30\t6\t14\t64", "row\t57\t6\t23\t4", "row\t58\t6\t14\t4096", "row\t59\t6\t24\t5", "row\t60\t6\t25\t5",
        "row\t31\t4\t10\t1", "row\t32\t4\t10\t2", "row\t33\t4\t11\t10", "row\t34\t4\t12\t0", "row\t35\t4\t15\t1000",
        "row\t36\t4\t16\t500000", "row\t37\t4\t16\t1000000", "row\t38\t4\t14\t128", "row\t39\t5\t13\t1", "row\t40\t5\t13\t2", "row\t41\t5\t13\t3", "row\t42\t5\t13\t5" };

    private static final String[] ALL_ROWS;
    static {
        java.util.List<String> l = new java.util.ArrayList<>();
        for (String r : ROWS) { l.add("all" + r); if (r.startsWith("row\t43\t")) l.add("allrow\t20\t3\t6\t8"); }
        ALL_ROWS = l.toArray(new String[0]);
    }
    private static final String NAMES = "class\t0\tAmazone\nclass\t1\tZauberin\nclass\t2\tTotenbeschwörer\nclass\t3\tPaladin\nclass\t4\tBarbar\n"
        + "class\t5\tDruide\nclass\t6\tAssassine\nclass\t7\tHexenmeister\ndifficulty\t0\tNormal\ndifficulty\t1\tAlptraum\ndifficulty\t2\tHölle\n"
        + "name\t31\tDer Stein von Jordan\nname\t32\tStandarte der Helden\nname\t39\tJägerinnenspäher\nname\t40\tWüstensöldner\nname\t41\tEisenwolf\nname\t42\tBarbar\n";

    private static final long SALES = 212, GOLD = 212480, LOOKS = 3, DYES = 0;

    private static void rows(StringBuilder b, String[] rows, long... entered) {
        for (String r : rows) {
            int id = Integer.parseInt(r.split("\t")[1]); long when = 0;
            for (int i = 0; i + 1 < entered.length; i += 2) if (entered[i] == id) when = entered[i + 1];
            b.append(r).append(when > 0 ? "\t1\t" + when : "\t0\t0").append('\n');
        }
    }


    private static String text(int mode, long kills, int cls, long... entered) { return text(mode, kills, cls + "\t0\t-1\t-1\t-1", entered); }

    private static String text(int mode, long kills, String hero, long... entered) {
        StringBuilder b = new StringBuilder("feats\t" + mode + "\tHero\ncounter\t0\t" + kills + "\ncounter\t1\t" + SALES + "\ncounter\t2\t" + GOLD + "\ncounter\t3\t" + LOOKS + "\ncounter\t4\t" + DYES + "\n");
        rows(b, ROWS, entered);
        b.append("hero\t").append(hero).append('\n').append(NAMES);
        return b.append(sum(1, kills, "", entered)).toString();
    }

    private static String sum(int heroes, long kills, String names, long... entered) {
        StringBuilder b = new StringBuilder("all\t" + heroes + "\n" + names + "allcounter\t0\t" + kills + "\nallcounter\t1\t" + SALES + "\nallcounter\t2\t" + GOLD + "\nallcounter\t3\t" + LOOKS + "\nallcounter\t4\t" + DYES + "\n");
        rows(b, ALL_ROWS, entered);
        return b.toString();
    }
    private static String withSum(String text, String sum) { return text.substring(0, text.indexOf("all\t")) + sum; }

    public static void main(String[] args) {

        FeatsModel m = FeatsModel.parse(text(FeatsModel.MODE_OWN, 1234, 7, 1, 1763500000L, 4, 1763400000L, 5, 1763450000L));
        check(m.hero.equals("Hero") && m.cls == 7 && m.hardcore == 0 && m.uniques == -1 && m.bosses == -1 && m.quests == -1 && !m.resting() && m.rows.size() == 59, "parsed");
        check(m.groups().equals(Arrays.asList(1, 7, 2, 3, 8, 6, 4, 5)), "the groups in table order");
        List<FeatsModel.Row> mastery = m.rows(3);
        check(mastery.size() == 2 && mastery.get(0).id == 43 && mastery.get(1).id == 21, "mastery of a softcore Warlock: its own class row and level 99");
        check(Arrays.equals(m.count(0), new int[]{3, 50}) && Arrays.equals(m.count(1), new int[]{1, 3}) && Arrays.equals(m.count(2), new int[]{2, 9})
            && Arrays.equals(m.count(3), new int[]{0, 2}) && Arrays.equals(m.count(4), new int[]{0, 8}) && Arrays.equals(m.count(5), new int[]{0, 4}) && Arrays.equals(m.count(6), new int[]{0, 11})
            && Arrays.equals(m.count(7), new int[]{0, 8}) && Arrays.equals(m.count(8), new int[]{0, 5}), "counts of the hero's view");
        FeatsModel.Row k500 = m.row(1), k5000 = m.row(2), k10000 = m.row(3), act3 = m.row(6);
        check(k500.entered && k500.when == 1763500000L && m.progress(k500) == -1, "an entered counting row shows its day, not its count");
        check(m.progress(k5000) == 1234 && m.progress(k10000) == 1234 && m.progress(act3) == -1, "open counting rows show the count; a state row nothing");
        check(Math.abs(FeatsModel.fill(1234, 5000) - 0.2468f) < 1e-4 && FeatsModel.fill(7000, 5000) == 1f && FeatsModel.fill(0, 500) == 0f, "the bar's share");
        check(Arrays.equals(m.gameNames(m.row(43)), new String[]{"Hexenmeister", "Hölle"}) && Arrays.equals(m.gameNames(m.row(9)), new String[]{"", "Alptraum"})
            && Arrays.equals(m.gameNames(k500), new String[]{"", ""}), "the game's names of a class, a difficulty and a kill row");
        check(Arrays.equals(m.gameNames(m.row(31)), new String[]{"Der Stein von Jordan", ""}) && Arrays.equals(m.gameNames(m.row(40)), new String[]{"Wüstensöldner", ""})
            && Arrays.equals(m.gameNames(m.row(12)), new String[]{"", ""}), "the game's names of an item and a mercenary; none for the waypoints");
        check(m.rows(5).size() == 4 && m.rows(5).get(3).id == 42 && m.rows(4).get(2).id == 33 && m.progress(m.row(33)) == -1 && m.progress(m.row(40)) == -1,
            "companions and treasures; no slot count before the items were read, a mercenary row shows none");


        check(m.rows(6).size() == 11 && m.rows(6).get(0).id == 24 && m.rows(6).get(6).id == 30 && m.rows(6).get(10).id == 60 && m.progress(m.row(25)) == -1, "the crafting group with the workshop rows");
        FeatsModel.Row sales = m.row(35);
        check(sales.group == 4 && m.progress(sales) == SALES && FeatsModel.progressText(SALES, sales.arg, Locale.GERMANY).equals("212 / 1.000")
            && Arrays.equals(m.gameNames(sales), new String[]{"", ""}) && Arrays.equals(m.gameNames(m.row(28)), new String[]{"", ""}), "212 / 1 000 items sold, no game names");
        FeatsModel sold = FeatsModel.parse(text(FeatsModel.MODE_OWN, 0, 7, 35, 1763000000L));
        check(sold.progress(sold.row(35)) == -1, "sold enough: its day, no count");

        FeatsModel.Row g500 = m.row(36), g1000 = m.row(37), gamble = m.row(38);
        check(g500.group == 4 && m.progress(g500) == GOLD && m.progress(g1000) == GOLD && m.progress(gamble) == -1
            && FeatsModel.progressText(GOLD, g500.arg, Locale.GERMANY).equals("212.480 / 500.000") && Arrays.equals(m.gameNames(gamble), new String[]{"", ""}),
            "212 480 / 500 000 gold, the gamble row without a count or game names");
        check(m.rows(4).size() == 8 && m.rows(4).get(5).id == 36 && m.rows(4).get(7).id == 38, "the treasures with the gold and gamble rows");
        FeatsModel rich = FeatsModel.parse(text(FeatsModel.MODE_OWN, 0, 7, 36, 1763000000L));
        check(rich.progress(rich.row(36)) == -1 && rich.progress(rich.row(37)) == GOLD, "500 000 entered: its day; the next row still counts");

        check(FeatsModel.widestCount(m.row(37), Locale.GERMANY).equals("999.999 / 1.000.000") && FeatsModel.widestCount(m.row(3), Locale.GERMANY).equals("9.999 / 10.000")
            && FeatsModel.widestCount(m.row(33), Locale.GERMANY).equals("9 / 10") && FeatsModel.widestCount(m.row(38), Locale.GERMANY) == null
            && FeatsModel.widestCount(m.row(4), Locale.GERMANY) == null, "the widest counts");

        FeatsModel hc = FeatsModel.parse(text(FeatsModel.MODE_OWN, 0, "4\t1\t3\t-1\t-1"));
        check(hc.rows(3).size() == 4 && hc.rows(3).get(2).id == 23 && hc.rows(3).get(3).id == 22 && hc.hardcore == 1, "a hardcore Barbarian: its class, 99, both hardcore rows");
        check(FeatsModel.parse(text(FeatsModel.MODE_OWN, 0, "4\t-1\t3\t-1\t-1")).rows(3).size() == 4, "hardcore not known: the hardcore rows shown");
        FeatsModel soft = FeatsModel.parse(text(FeatsModel.MODE_OWN, 0, "4\t0\t3\t-1\t-1", 22, 1763000000L));
        check(soft.rows(3).size() == 3 && soft.rows(3).get(2).id == 22, "softcore: only the entered hardcore row");

        FeatsModel.Row slots = hc.row(33);
        check(hc.uniques == 3 && hc.progress(slots) == 3 && FeatsModel.progressText(3, slots.arg, Locale.GERMANY).equals("3 / 10")
            && Math.abs(FeatsModel.fill(3, slots.arg) - 0.3f) < 1e-6, "3 / 10 unique slots");
        FeatsModel zero = FeatsModel.parse(text(FeatsModel.MODE_OWN, 0, "4\t1\t0\t-1\t-1")), worn = FeatsModel.parse(text(FeatsModel.MODE_OWN, 0, "4\t1\t9\t-1\t-1", 33, 1763000000L));
        check(zero.progress(zero.row(33)) == -1 && worn.progress(worn.row(33)) == -1, "no slot: nothing; entered: its day");

        FeatsModel quester = FeatsModel.parse(text(FeatsModel.MODE_OWN, 0, "7\t0\t-1\t-1\t18"));
        FeatsModel.Row quests = quester.row(11);
        check(quests.group == 2 && quests.rule == FeatsModel.R_QUESTS && quester.quests == 18 && quester.progress(quests) == 18
            && FeatsModel.progressText(18, quests.arg, Locale.GERMANY).equals("18 / 27") && FeatsModel.widestCount(quests, Locale.GERMANY).equals("26 / 27")
            && Arrays.equals(quester.gameNames(quests), new String[]{"", ""}) && quester.rows(2).get(7).id == 11 && quester.rows(2).get(8).id == 12,
            "18 / 27 quests, the row before the waypoints, no game names");
        FeatsModel noQuest = FeatsModel.parse(text(FeatsModel.MODE_OWN, 0, "7\t0\t-1\t-1\t0")), allQuests = FeatsModel.parse(text(FeatsModel.MODE_OWN, 0, "7\t0\t-1\t-1\t27", 11, 1763000000L));
        check(noQuest.progress(noQuest.row(11)) == -1 && allQuests.progress(allQuests.row(11)) == -1
            && quester.all.progress(quester.all.row(11)) == -1, "no quest: nothing; entered: its day; the sum: no count");
        bad(text(FeatsModel.MODE_OWN, 0, 7).replace("row\t11\t2\t26\t27", "row\t11\t2\t26\t26"), "a quests row with another target");


        FeatsModel hunter = FeatsModel.parse(text(FeatsModel.MODE_OWN, 0, "7\t0\t-1\t31\t-1", 44, 1763000000L, 52, 1763000100L));
        List<FeatsModel.Row> hunt = hunter.rows(7), rifts = hunter.rows(8);
        check(hunt.size() == 8 && hunt.get(0).id == 44 && hunt.get(7).id == 51 && rifts.size() == 5 && rifts.get(0).id == 52 && rifts.get(4).id == 56
            && Arrays.equals(hunter.count(7), new int[]{1, 8}) && Arrays.equals(hunter.count(8), new int[]{1, 5}), "the hunt and rift groups");
        FeatsModel.Row bounty = hunter.row(47);
        check(hunter.bosses == 31 && hunter.progress(bounty) == 31 && FeatsModel.progressText(31, bounty.arg, Locale.GERMANY).equals("31 / 64")
            && FeatsModel.widestCount(bounty, Locale.GERMANY).equals("63 / 64"), "31 / 64 bosses of the ranking");
        check(hunter.progress(hunter.row(45)) == -1 && hunter.progress(hunter.row(48)) == -1 && hunter.progress(hunter.row(53)) == -1 && hunter.progress(hunter.row(57)) == -1
            && FeatsModel.widestCount(hunter.row(45), Locale.GERMANY) == null && FeatsModel.widestCount(hunter.row(55), Locale.GERMANY) == null, "streak, fast boss, rift tier and shrine rows show no count");
        check(m.progress(m.row(47)) == -1, "the boss rows not known: no count");
        check(Arrays.equals(hunter.gameNames(hunter.row(50)), new String[]{"", "Alptraum"}) && Arrays.equals(hunter.gameNames(hunter.row(49)), new String[]{"", ""})
            && Arrays.equals(hunter.gameNames(hunter.row(59)), new String[]{"", ""}), "the potion row takes the Nightmare difficulty's name, the others none");
        bad(text(FeatsModel.MODE_OWN, 0, 7).replace("row\t50\t7\t14\t512", "row\t50\t7\t14\t32768"), "an event bit the host does not have");

        FeatsModel.Row looks = hunter.row(59), dyes = hunter.row(60);
        check(hunter.progress(looks) == 3 && FeatsModel.progressText(3, looks.arg, Locale.GERMANY).equals("3 / 5") && hunter.progress(dyes) == -1
            && FeatsModel.widestCount(looks, Locale.GERMANY).equals("4 / 5") && FeatsModel.widestCount(dyes, Locale.GERMANY).equals("4 / 5")
            && Math.abs(FeatsModel.fill(3, looks.arg) - 0.6f) < 1e-6 && hunter.progress(hunter.row(57)) == -1, "3 / 5 items given a look; no count at 0 or for the shrines");
        bad(text(FeatsModel.MODE_OWN, 0, 7).replace("counter\t4\t0\n", ""), "no colour counter");
        bad(text(FeatsModel.MODE_OWN, 0, 7).replace("row\t59\t6\t24\t5", "row\t59\t6\t24\t0"), "a looks row without a target");
        bad(text(FeatsModel.MODE_OWN, 0, 7).replace("row\t53\t8\t21\t10", "row\t53\t8\t21\t0"), "a rift tier of 0");
        bad(text(FeatsModel.MODE_OWN, 0, 7).replace("row\t53\t8\t21\t10", "row\t53\t9\t21\t10"), "a group the host does not have");


        FeatsModel none = FeatsModel.parse(text(FeatsModel.MODE_OWN, 0, 2));
        check(none.progress(none.row(1)) == -1 && Arrays.equals(none.count(1), new int[]{0, 3}), "nothing counted: nothing shown");
        check(none.rows(3).get(0).id == 15, "a Necromancer's class row");

        FeatsModel other = FeatsModel.parse(text(FeatsModel.MODE_FOREIGN, 0, 2, 13, 1763000000L));
        check(other.rows(3).size() == 3 && other.rows(3).get(0).id == 13 && !other.resting(), "an entered row of another class stays");
        FeatsModel unknown = FeatsModel.parse(text(FeatsModel.MODE_OWN, 0, -1));
        check(unknown.rows(3).size() == 9 && unknown.count(0)[1] == 57, "class not read yet: every class row");

        check(FeatsModel.parse(text(FeatsModel.MODE_NEWER, 0, 7)).resting() && FeatsModel.parse(text(FeatsModel.MODE_UNREAD, 0, 7)).resting(), "newer and unread rest");

        check(FeatsModel.parse(text(FeatsModel.MODE_OWN, 0, 7).replace("counter\t0\t0", "counter\t0\t18446744073709551615")).counters[0] == Long.MAX_VALUE, "a saturated counter");


        String good = text(FeatsModel.MODE_OWN, 5, 1);
        bad(good.replace("feats\t0\tHero\n", ""), "no head");
        bad(good + "feats\t0\tHero\n", "two heads");
        bad(good.replace("feats\t0", "feats\t4"), "unknown mode");
        bad(good.replace("counter\t0\t5\n", ""), "no counter");
        bad(good.replace("hero\t1\t0\t-1\t-1\t-1\n", ""), "no hero line");
        bad(good.replace("hero\t1\t0\t-1\t-1\t-1", "hero\t1\t0\t-1\t-1"), "a hero line without the quests");
        bad(good.replace("hero\t1\t0\t-1\t-1\t-1", "hero\t1\t0\t-1\t-1\t28"), "more quests than a difficulty has");
        bad(good.replace("hero\t1\t0\t-1\t-1\t-1", "hero\t1\t0\t-1\t-1\t-2"), "quests out of range");
        bad(good.replace("hero\t1\t0\t-1\t-1\t-1", "hero\t1\t0\t-1\t-2\t-1"), "boss rows out of range");
        bad(good.replace("hero\t1\t0\t-1", "hero\t8\t0\t-1"), "class out of range");
        bad(good.replace("hero\t1\t0\t-1", "hero\t1"), "a hero line without hardcore and slots");
        bad(good.replace("hero\t1\t0\t-1", "hero\t1\t2\t-1"), "hardcore out of range");
        bad(good.replace("hero\t1\t0\t-1", "hero\t1\t0\t11"), "more unique slots than there are");
        bad(good.replace("name\t40\tWüstensöldner\n", ""), "a row's game name missing");
        bad(good + "name\t40\tWüstensöldner\n", "a row's game name twice");
        bad(good.replace("row\t40\t5\t13\t2", "row\t40\t5\t13\t6"), "a mercenary of act 6");
        bad(good.replace("row\t33\t4\t11\t10", "row\t33\t4\t11\t11"), "eleven unique slots");
        bad(good.replace("class\t7\tHexenmeister\n", ""), "a class name missing");
        bad(good.replace("difficulty\t2\tHölle\n", ""), "a difficulty name missing");
        bad(good.replace("row\t2\t1\t1\t5000\t0\t0", "row\t2\t1\t1\t5000\t0\t9"), "a time without the entry");
        bad(good.replace("row\t2\t1\t1\t5000\t0\t0", "row\t1\t1\t1\t5000\t0\t0"), "an id twice");
        bad(good.replace("row\t2\t1\t1\t5000", "row\t2\t9\t1\t5000"), "an unknown group");
        bad(good.replace("row\t2\t1\t1\t5000", "row\t2\t1\t26\t5000"), "an unknown rule");
        bad(good.replace("row\t2\t1\t1\t5000", "row\t2\t1\t6\t5000"), "the account-wide rule in one hero's rows");
        bad(good.replace("row\t4\t2\t2\t1", "row\t4\t2\t2\t6"), "act 6");
        bad(good.replace("row\t26\t6\t14\t4", "row\t26\t6\t14\t6"), "an event row naming two events");
        bad(good.replace("row\t26\t6\t14\t4", "row\t26\t6\t14\t32768"), "an unknown event");
        bad(good.replace("row\t35\t4\t15\t1000", "row\t35\t4\t15\t0"), "a sales row without a target");
        bad(good.replace("counter\t1\t212\n", ""), "no sales counter");
        bad(good.replace("row\t36\t4\t16\t500000", "row\t36\t4\t16\t0"), "a gold row without a target");
        bad(good.replace("counter\t2\t" + GOLD + "\n", ""), "no gold counter");
        bad(good.replace("row\t43\t3\t4\t7", "row\t43\t3\t4\t8"), "class 8");
        bad(good.replace("row\t2\t1\t1\t5000\t0\t0", "row\t2\t1\t1\t5000\t2\t0"), "entered neither 0 nor 1");
        bad(good.replace("row\t2\t1\t1\t5000\t0\t0", "row\t2\t1\t1\t5000\t0"), "a short row");
        bad(good + "more\t1\n", "an unknown line");
        bad(String.join("\n", Arrays.asList(good.split("\n")).subList(0, 2)) + "\nhero\t1\t0\t-1\t-1\t-1\n" + NAMES, "no rows");


        String three = withSum(text(FeatsModel.MODE_OWN, 1234, 7, 1, 1763500000L),
            sum(3, 5234, "skipped\tBob\tunreadable\nskipped\tDan\tnewer\nwaiting\tGus\nwaiting\tHal Two\n", 1, 1763000000L, 4, 1763100000L, 13, 1763200000L, 14, 1763300000L, 43, 1763400000L));
        FeatsModel h = FeatsModel.parse(three), a = h.all;
        check(!h.account && h.all != null && a.account && a.all == null && a.heroes == 3 && a.cls == -1 && !a.resting(), "the hero and the sum");
        check(a.skipped.equals(Arrays.asList("Bob")) && a.newer.equals(Arrays.asList("Dan")) && a.waiting.equals(Arrays.asList("Gus", "Hal Two"))
            && !a.unavailable && !a.incomplete, "the heroes left out by reason and those not counted yet");
        check(h.rows(3).size() == 2 && h.row(20) == null, "this hero's view: its own class row and level 99, not every class's");
        List<FeatsModel.Row> am = a.rows(3);
        check(am.size() == 12 && am.get(7).id == 43 && am.get(8).id == 20 && am.get(9).id == 21 && am.get(10).id == 23 && am.get(11).id == 22,
            "the sum's mastery: every class, then every class's Hell, level 99 and the hardcore rows");
        check(Arrays.equals(a.count(0), new int[]{5, 60}) && Arrays.equals(a.count(3), new int[]{3, 12}) && a.groups().equals(Arrays.asList(1, 7, 2, 3, 8, 6, 4, 5)), "the sum's counts");
        check(Arrays.equals(a.gameNames(a.row(39)), new String[]{"Jägerinnenspäher", ""}) && a.progress(a.row(33)) == -1 && a.progress(a.row(47)) == -1, "the sum: a mercenary's name, no slot count, no boss count");
        FeatsModel.Row every = a.row(20);
        check(a.classesDone() == 3 && a.progress(every) == 3 && Math.abs(FeatsModel.fill(3, every.arg) - 0.375f) < 1e-6, "every class's Hell: 3 / 8 with its bar");
        check(a.progress(a.row(2)) == 5234 && a.progress(a.row(1)) == -1 && a.row(1).when == 1763000000L, "the sum's kills; the earliest day of an entry");
        check(Arrays.equals(a.gameNames(every), new String[]{"", "Hölle"}) && Arrays.equals(a.gameNames(a.row(14)), new String[]{"Zauberin", "Hölle"}), "the sum's game names");
        check(a.classNames == h.classNames && !a.complete(), "the game's names shared; not everything done");

        check(FeatsModel.parse(text(FeatsModel.MODE_OWN, 0, 7)).all.progress(FeatsModel.parse(text(FeatsModel.MODE_OWN, 0, 7)).all.row(20)) == -1, "no class done: nothing shown");

        long[] each = new long[ALL_ROWS.length * 2];
        for (int i = 0; i < ALL_ROWS.length; i++) { each[2 * i] = Integer.parseInt(ALL_ROWS[i].split("\t")[1]); each[2 * i + 1] = 1763000000L + i; }
        FeatsModel done = FeatsModel.parse(withSum(text(FeatsModel.MODE_OWN, 1234, 7), sum(8, 99999, "", each))).all;
        check(done.complete() && done.classesDone() == 8 && done.progress(done.row(20)) == -1 && Arrays.equals(done.count(0), new int[]{60, 60}), "everything done");
        check(!FeatsModel.parse(withSum(text(FeatsModel.MODE_OWN, 1234, 7), sum(0, 0, ""))).all.complete(), "no hero added, nothing done");

        FeatsModel rest = FeatsModel.parse(withSum(text(FeatsModel.MODE_NEWER, 0, 7), sum(2, 10, "skipped\tHero\tnewer\n", 4, 1763100000L)));
        check(rest.resting() && !rest.all.resting() && rest.all.row(4).entered && rest.all.newer.equals(Arrays.asList("Hero")), "resting hero, the sum of the others");

        FeatsModel gone = FeatsModel.parse(withSum(text(FeatsModel.MODE_OWN, 10, 7), sum(1, 10, "incomplete\tunavailable\n"))).all;
        FeatsModel many = FeatsModel.parse(withSum(text(FeatsModel.MODE_OWN, 10, 7), sum(256, 10, "incomplete\tlimit\n"))).all;
        check(gone.unavailable && !gone.incomplete && many.incomplete && !many.unavailable, "unavailable and incomplete");

        bad(three.replace("all\t3\n", ""), "no sum head");
        bad(three + "all\t3\n", "two sum heads");
        bad(three.replace("all\t3\n", "all\t-1\n"), "a negative hero count");
        bad(three.replace("allcounter\t0\t5234\n", ""), "no added counter");
        bad(three + "allcounter\t0\t1\n", "an added counter twice");
        bad(three.replace("skipped\tBob", "skipped\t"), "a hero without a name");
        bad(three.replace("skipped\tBob\tunreadable", "skipped\tBob"), "a skipped hero without a reason");
        bad(three.replace("skipped\tBob\tunreadable", "skipped\tBob\tbroken"), "an unknown reason");
        bad(three.replace("all\t3\n", "all\t3\nincomplete\tsoon\n"), "an unknown incompleteness");
        bad(three.replace("all\t3\n", "all\t3\nincomplete\tlimit\nincomplete\tunavailable\n"), "incomplete twice");
        bad(three.replace("waiting\tGus", "waiting\tGus\tx"), "a waiting line with more");
        bad(three.replace("allrow\t20\t3\t6\t8", "allrow\t20\t3\t6\t9"), "every class's Hell beyond eight");
        bad(three.replace("allrow\t20\t3\t6\t8", "allrow\t20\t3\t26\t8"), "an unknown rule in the sum");
        bad(three.replace("row\t42\t5\t13\t5\t0\t0\nhero", "row\t42\t5\t13\t5\t0\t0\nrow\t20\t3\t6\t8\t0\t0\nhero"), "every class's Hell in one hero's rows");
        String noRows = three.substring(0, three.indexOf("allrow"));
        bad(noRows, "a sum without rows");


        TimeZone utc = TimeZone.getTimeZone("UTC");
        check(FeatsModel.date(1763510400L, Locale.GERMANY, utc).equals("19.11.25"), "German day: " + FeatsModel.date(1763510400L, Locale.GERMANY, utc));
        check(FeatsModel.date(1763510400L, Locale.US, utc).equals("11/19/25"), "English day: " + FeatsModel.date(1763510400L, Locale.US, utc));
        check(FeatsModel.progressText(1234, 5000, Locale.GERMANY).equals("1.234 / 5.000") && FeatsModel.progressText(1234, 5000, Locale.US).equals("1,234 / 5,000"), "grouped counts");
        System.out.println("FeatsModelTest: " + checks + " checks");
    }
}
