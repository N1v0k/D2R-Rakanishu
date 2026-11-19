



package com.xiaoboluo.d2r;

import java.util.HashMap;
import java.util.Map;

public final class RecipeTermsTest {
    private static int checks, fails;
    private static void check(boolean ok, String what) { checks++; if (!ok) { fails++; System.out.println("FAIL " + what); } }

    public static void main(String[] a) {
        check("Runenworte".equals(RecipeTerms.get(null, "runewords", true)) && "Runewords".equals(RecipeTerms.get(null, "runewords", false)), "built-in = the game's word (Runenworte)");
        check("Edelsteine".equals(RecipeTerms.get(null, "cat.gems", true)) && "Sockelfassungen".equals(RecipeTerms.get(null, "cat.socket", true)), "Edelsteine, Sockelfassungen");
        check("Schatz".equals(RecipeTerms.get(null, "place.stash", true)) && "Stapel-Reiter".equals(RecipeTerms.get(null, "place.stacks", true)), "stash and stack tab");
        Map<String, String[]> pack = new HashMap<>();
        pack.put("runewords", new String[]{"Runenworte X", "Runewords X"});
        pack.put("group.helm", new String[]{"[ms]Helm[fs]Helmin", "Helm"});
        pack.put("cat.gems", new String[]{"  ", "Gems"});
        pack.put("cat.runes", new String[]{"Runen %d", "Runes"});
        pack.put("all", new String[]{"Alle"});
        pack.put("page", new String[]{"Seite %u von %u", "%u"});
        check("Runenworte X".equals(RecipeTerms.get(pack, "runewords", true)) && "Runewords X".equals(RecipeTerms.get(pack, "runewords", false)), "the pack's word first");
        check("Helm".equals(RecipeTerms.get(pack, "group.helm", true)), "grammar marks: the first form");
        check("Edelsteine".equals(RecipeTerms.get(pack, "cat.gems", true)) && "Gems".equals(RecipeTerms.get(pack, "cat.gems", false)), "an empty word falls back");
        check("Runen".equals(RecipeTerms.get(pack, "cat.runes", true)), "a word with a format sign falls back");
        check("Alle".equals(RecipeTerms.get(pack, "all", true)), "a broken pair falls back");
        check("unknown.key".equals(RecipeTerms.get(pack, "unknown.key", true)) && !RecipeTerms.known("unknown.key") && RecipeTerms.known("page"), "unknown keys");
        check("Seite 2 von 7".equals(RecipeTerms.page(pack, true, 2, 7)), "the pack's page format");
        check("Page 2 / 7".equals(RecipeTerms.page(pack, false, 2, 7)), "a page format without two numbers falls back");
        check("Seite 1/1".equals(RecipeTerms.page(null, true, 1, 1)), "built-in page format = the game's SharedStashPage");
        StringBuilder long_ = new StringBuilder();
        for (int i = 0; i < 50; i++) long_.append('w');
        pack.put("cat.quest", new String[]{long_.toString(), "Quest"});
        check("Quest".equals(RecipeTerms.get(pack, "cat.quest", true)), "too long falls back");
        String[] places = RecipeTerms.places(null, true);
        check(places.length == 8 && "Inventar".equals(places[RecipeStock.INVENTORY]) && "Schatz".equals(places[RecipeStock.STASH]) && "Chronik".equals(places[RecipeStock.VAULT])
            && "Stapel-Reiter".equals(places[RecipeStock.STACKS]) && "Beutel".equals(places[RecipeStock.POUCH]), "places in RecipeStock's order");
        check("".equals(RecipeTerms.clean(null)) && "a b".equals(RecipeTerms.clean(" a \t b ")), "clean");
        System.out.println("RecipeTermsTest: " + checks + " checks, " + fails + " failed");
        if (fails != 0) System.exit(1);
    }
}
