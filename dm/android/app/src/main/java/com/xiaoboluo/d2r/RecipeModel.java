package com.xiaoboluo.d2r;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;


final class RecipeModel {
    private RecipeModel() {}

    static final class Input {
        String id, kind, img, name, cond = "";
        int count = 1, row = -1;
        List<String> flags = new ArrayList<>();
    }

    static final class Card {
        String cat, title, colour, img, sub = "", search, out = "";
        String app = "";
        String key = "";
        Input[] in;
        int order, sockets, diff, num, row;
        List<String> preview = new ArrayList<>();
        boolean locked;
        boolean quest;
        List<String> groups = new ArrayList<>(), itype = new ArrayList<>(), etype = new ArrayList<>();
    }


    static List<String> gameLines(String raw, String title) {
        List<String> out = new ArrayList<>();
        for (String line : raw.split("\n")) {
            String clean = line.replaceAll("\ue07e.", "").trim();
            if (!clean.isEmpty() && !clean.equals(title) && !clean.startsWith("'")) out.add(0, clean);
        }
        return out;
    }

    static final class Item {
        final String de, en, type, type2, norm;
        final int tier;
        int w = 1, h = 1;
        String shortDe, shortEn;
        Item(String de, String en, String type, String type2, int tier, String norm) { this.de = de; this.en = en; this.type = type; this.type2 = type2; this.tier = tier; this.norm = norm; }
    }

    static final class Catalog {
        final List<Card> cube = new ArrayList<>(), words = new ArrayList<>();
        final Map<String, Item> items = new HashMap<>();
        final Map<String, List<String>> typeUp = new HashMap<>();
        final Map<String, String> vault = new HashMap<>();
        final Map<String, String[]> terms = new HashMap<>();


        boolean isType(Item item, String want) { return under(item.type, want) || under(item.type2, want); }
        private boolean under(String type, String want) {
            if (type == null || type.isEmpty()) return false;
            if (type.equals(want)) return true;
            List<String> up = typeUp.get(type);
            return up != null && up.contains(want);
        }
    }
}
