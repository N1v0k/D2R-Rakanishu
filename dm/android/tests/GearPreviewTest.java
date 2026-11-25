package com.xiaoboluo.d2r;

import java.util.ArrayList;
import java.util.List;






public class GearPreviewTest {
    static int failed;

    static void check(String what, Object got, Object want) {
        boolean ok = String.valueOf(got).equals(String.valueOf(want));
        if (!ok) failed++;
        System.out.println((ok ? "ok   " : "FAIL ") + what + ": " + got + (ok ? "" : " (want " + want + ")"));
    }

    static BuildModel.ItemInfo item(BuildModel.Names n, String key, String de, String en) {
        BuildModel.ItemInfo i = new BuildModel.ItemInfo(); i.de = de; i.en = en; n.items.put(key, i); return i;
    }

    static BuildModel.Gear gear(String kind, String key) { BuildModel.Gear g = new BuildModel.Gear(); g.kind = kind; g.key = key; return g; }

    static String texts(List<GearPreview.Line> lines) {
        List<String> out = new ArrayList<>();
        for (GearPreview.Line l : lines) out.add(l.toString());
        return out.toString();
    }

    public static void main(String[] args) {


        check("tap on the card closes", GearPreview.touch(3, -2, 60), GearPreview.TOUCH_CLOSE);
        check("still tap: closes", GearPreview.touch(0, 0, 60), GearPreview.TOUCH_CLOSE);
        check("swipe left: next page", GearPreview.touch(-200, 20, 60), 1);
        check("swipe right: previous page", GearPreview.touch(200, -20, 60), -1);
        check("mostly vertical drag: closes", GearPreview.touch(80, 200, 60), GearPreview.TOUCH_CLOSE);
        BuildModel.Names n = new BuildModel.Names();
        n.labels.put("lvl", new BuildModel.Text("Stufe: %d", "Level: %d"));
        n.labels.put("str", new BuildModel.Text("Stärke: %d", "Strength: %d"));
        n.labels.put("dex", new BuildModel.Text("Geschick: %d", "Dexterity: %d"));
        n.labels.put("def", new BuildModel.Text("Verteidigung: %d", "Defense: %d"));
        n.labels.put("dmg2", new BuildModel.Text("Zweihand: %d bis %d", "Two-Hand: %d to %d"));
        n.labels.put("sock", new BuildModel.Text("Gesockelt (%i)", "Socketed (%i)"));
        n.labels.put("eth", new BuildModel.Text("Ätherisch", "Ethereal"));
        n.qualities.put("magic", new BuildModel.Text("Magisch", "Magic"));
        BuildModel.ItemInfo shako = item(n, "i:uap", "Tschako", "Shako"); shako.lvl = 43; shako.str = 50; shako.img = "uap"; shako.ac = new int[]{98, 141}; shako.gat = 1;
        BuildModel.ItemInfo crest = item(n, "u:Harlequin Crest", "Harlekinskrone", "Harlequin Crest"); crest.code = "uap"; crest.lvl = 62; crest.row = 248; crest.img = "uap";
        BuildModel.ItemInfo claws = item(n, "s:Trang-Oul's Claws", "Trang-Ouls Krallen", "Trang-Oul's Claws"); claws.code = "xmg"; claws.lvl = 45; claws.row = 88; claws.img = "xmg";
        BuildModel.ItemInfo gaunt = item(n, "i:xmg", "Schwere Armschienen", "Heavy Bracers"); gaunt.lvl = 29; gaunt.str = 58; gaunt.gat = 1;
        BuildModel.ItemInfo gore = item(n, "u:Gorerider", "Blutreiter", "Gore Rider"); gore.code = "xhb"; gore.lvl = 47; gore.row = 241; gore.ease = true;
        BuildModel.ItemInfo boots = item(n, "i:xhb", "Kampfstiefel", "War Boots"); boots.lvl = 34; boots.str = 94;
        BuildModel.ItemInfo inf = item(n, "r:Runeword62", "Unendlichkeit", "Infinity"); inf.lvl = 63; inf.row = 60;
        inf.runes.add("r30"); inf.runes.add("r23"); inf.runes.add("r30"); inf.runes.add("r24");
        for (String[] r : new String[][]{{"r30", "Ber"}, {"r23", "Mal"}, {"r24", "Ist"}}) { BuildModel.ItemInfo x = item(n, "i:" + r[0], r[1] + "-Rune", r[1] + " Rune"); x.shortName = new BuildModel.Text(r[1], r[1]); }
        BuildModel.ItemInfo thresher = item(n, "i:7wc", "Riesendrescher", "Giant Thresher"); thresher.lvl = 66; thresher.str = 188; thresher.dex = 140; thresher.img = "7wc"; thresher.d2 = new int[]{40, 114}; thresher.gat = 0;
        BuildModel.ItemInfo pole = item(n, "t:pole", "Stangenwaffe", "Polearm"); pole.img = "pax"; pole.gat = 0;
        BuildModel.ItemInfo bootType = item(n, "t:boot", "Stiefel", "Boots"); bootType.img = "lbt";



        GearPreview.Card c = GearPreview.of(gear("unique", "Harlequin Crest"), n, true);
        check("unique title", c.title + " " + c.titleColour, "Harlekinskrone Tooltip.UniqueColor");
        check("unique head", texts(c.head), "[Tooltip.UniqueColor:Tschako]");
        check("unique requirements", texts(c.reqs), "[Tooltip.DefaultColor:Stärke: 50, Tooltip.DefaultColor:Stufe: 62]");
        check("unique host ask", c.kind + " " + c.row + " " + c.grp, "1 248 -1");
        check("unique picture", c.img, "uap");

        String raw = "3+2 zu allen Fertigkeiten\n3+50% Chance, magische Gegenstände zu finden\nTschako\n4Harlekinskrone";
        List<GearPreview.Line> game = GearPreview.gameLines(raw, c);
        check("unique game lines top down, name/base dropped", texts(game), "[Tooltip.MagicColor:+50% Chance, magische Gegenstände zu finden, Tooltip.MagicColor:+2 zu allen Fertigkeiten]");
        check("unique whole card", texts(GearPreview.lines(c, game)), "[Tooltip.UniqueColor:Harlekinskrone, Tooltip.UniqueColor:Tschako, Tooltip.DefaultColor:Stärke: 50, Tooltip.DefaultColor:Stufe: 62, "
            + "Tooltip.MagicColor:+50% Chance, magische Gegenstände zu finden, Tooltip.MagicColor:+2 zu allen Fertigkeiten]");

        List<GearPreview.Line> withReq = GearPreview.gameLines("3+2 zu allen Fertigkeiten\n0Stufe: 62", c);
        check("game requirement lines replace ours", texts(GearPreview.lines(c, withReq)), "[Tooltip.UniqueColor:Harlekinskrone, Tooltip.UniqueColor:Tschako, Tooltip.DefaultColor:Stufe: 62, Tooltip.MagicColor:+2 zu allen Fertigkeiten]");
        check("english card", GearPreview.of(gear("unique", "Harlequin Crest"), n, false).title, "Harlequin Crest");


        c = GearPreview.of(gear("set", "Trang-Oul's Claws"), n, false);
        check("set title", c.title + " " + c.titleColour, "Trang-Oul's Claws Tooltip.SetColor");
        check("set host ask", c.kind + " " + c.row, "0 88");
        check("set requirements", texts(c.reqs), "[Tooltip.DefaultColor:Strength: 58, Tooltip.DefaultColor:Level: 45]");
        check("set bonus lines keep the game's green", texts(GearPreview.gameLines("2+25% Giftschaden (2 Gegenstände)\n3+30 Verteidigung", c)),
            "[Tooltip.MagicColor:+30 Verteidigung, Tooltip.SetColor:+25% Giftschaden (2 Gegenstände)]");


        c = GearPreview.of(gear("unique", "Gorerider"), n, true);
        check("ease: no str/dex", texts(c.reqs), "[Tooltip.DefaultColor:Stufe: 47]");



        BuildModel.Gear w = gear("runeword", "Runeword62"); w.baseItem = "7wc"; w.sockets = 4; w.eth = true;
        c = GearPreview.of(w, n, true);
        check("rune word title", c.title + " " + c.titleColour, "Unendlichkeit Tooltip.UniqueColor");
        check("rune word head", texts(c.head), "[Tooltip.substringColor:Riesendrescher, Tooltip.UniqueColor:'BerMalBerIst']");
        check("rune word requirements on an ethereal base", texts(c.reqs), "[Tooltip.DefaultColor:Geschick: 130, Tooltip.DefaultColor:Stärke: 178, Tooltip.DefaultColor:Stufe: 66]");
        check("rune word tail", texts(c.tail), "[Tooltip.MagicColor:Gesockelt (4), Tooltip.MagicColor:Ätherisch]");
        check("rune word host ask (group of the base)", c.kind + " " + c.row + " " + c.grp, "2 60 0");
        check("rune word picture = base", c.img, "7wc");
        List<GearPreview.Line> wg = GearPreview.gameLines("Unendlichkeit\n3Stufe 12 Überzeugung-Aura\n'BerMalBerIst'", c);
        check("rune word game lines without name and rune line", texts(wg), "[Tooltip.MagicColor:Stufe 12 Überzeugung-Aura]");
        check("rune word whole card", texts(GearPreview.lines(c, wg)), "[Tooltip.UniqueColor:Unendlichkeit, Tooltip.substringColor:Riesendrescher, Tooltip.UniqueColor:'BerMalBerIst', "
            + "Tooltip.DefaultColor:Geschick: 130, Tooltip.DefaultColor:Stärke: 178, Tooltip.DefaultColor:Stufe: 66, Tooltip.MagicColor:Stufe 12 Überzeugung-Aura, Tooltip.MagicColor:Gesockelt (4), Tooltip.MagicColor:Ätherisch]");
        check("gender-tagged name line dropped", texts(GearPreview.gameLines("\ue07e3+2 zu allen Fertigkeiten\n\ue07e5\ue07e4[ms]unendlicher[fs]Unendlichkeit[ns]unendliches", c)), "[Tooltip.MagicColor:+2 zu allen Fertigkeiten]");
        check("gender-tagged other line: first form", texts(GearPreview.gameLines("[ms]leerer[fs]leere", c)), "[Tooltip.MagicColor:leerer]");
        check("tail not repeated when the game writes it", texts(GearPreview.lines(c, GearPreview.gameLines("3Gesockelt (4)", c))).split("Gesockelt").length - 1, 1);



        w = gear("runeword", "Runeword62"); w.baseType = "pole"; w.sockets = 4;
        c = GearPreview.of(w, n, false);
        check("rune word on a type: head", texts(c.head), "[Tooltip.substringColor:Polearm, Tooltip.UniqueColor:'BerMalBerIst']");
        check("rune word on a type: level only", texts(c.reqs), "[Tooltip.DefaultColor:Level: 63]");
        check("rune word on a type: group", c.grp, 0);
        check("rune word on a type: no ethereal line", texts(c.tail), "[Tooltip.MagicColor:Socketed (4)]");


        c = GearPreview.of(gear("item", "7wc"), n, true);
        check("base item card", texts(GearPreview.lines(c, GearPreview.gameLines("", c))), "[Tooltip.DefaultColor:Riesendrescher, Tooltip.DefaultColor:Zweihand: 40 bis 114, "
            + "Tooltip.DefaultColor:Geschick: 140, Tooltip.DefaultColor:Stärke: 188, Tooltip.DefaultColor:Stufe: 66]");
        check("base item: host not asked", c.kind, -1);
        c = GearPreview.of(gear("item", "uap"), n, false);
        check("armor base: defense range", texts(c.head), "[Tooltip.DefaultColor:Defense: 98-141]");


        BuildModel.Gear t = gear("type", "boot"); t.quality = "magic";
        c = GearPreview.of(t, n, true);
        check("type card", texts(GearPreview.lines(c, GearPreview.gameLines(null, c))), "[Tooltip.MagicColor:Stiefel, Tooltip.substringColor:Magisch]");
        check("type: host not asked, picture", c.kind + " " + c.img, "-1 lbt");


        BuildModel.Names old = new BuildModel.Names();
        BuildModel.ItemInfo oc = item(old, "u:Harlequin Crest", "Harlekinskrone", "Harlequin Crest"); oc.code = "uap"; oc.lvl = 62;
        item(old, "i:uap", "Tschako", "Shako");
        c = GearPreview.of(gear("unique", "Harlequin Crest"), old, true);
        check("old pack: no host ask", c.kind, -1);
        check("old pack: no requirement lines without labels", texts(c.reqs), "[]");
        check("old pack: name and base", texts(GearPreview.lines(c, new ArrayList<>())), "[Tooltip.UniqueColor:Harlekinskrone, Tooltip.UniqueColor:Tschako]");

        c = GearPreview.of(gear("set", "Trang-Oul's Claws"), null, true);
        check("no names: key as title", c.title + " " + c.kind, "Trang-Oul's Claws -1");


        check("fill two numbers", GearPreview.fill(n, "dmg2", false, 3, 9), "Two-Hand: 3 to 9");
        check("fill too many numbers", GearPreview.fill(n, "lvl", false, 3, 9), "null");
        check("fill too few numbers", GearPreview.fill(n, "dmg2", false, 3), "null");
        check("colour codes", GearPreview.colourOf('2') + " " + GearPreview.colourOf('1') + " " + GearPreview.colourOf('Z'), "Tooltip.SetColor FontColorRed Tooltip.MagicColor");



        String tip = "\ue07e5grey note\n\ue07e3blue line +4.5 %\nstill blue\n\n\u00ffc0white line: 8\n\ue07e3title line\n";
        check("tooltip lines top first", texts(GearPreview.tooltipLines(tip)),
            "[Tooltip.MagicColor:title line, Tooltip.DefaultColor:white line: 8, Tooltip.MagicColor:, Tooltip.MagicColor:still blue, "
            + "Tooltip.MagicColor:blue line +4.5 %, Tooltip.substringColor:grey note]");
        check("tooltip without codes is white", texts(GearPreview.tooltipLines("b\na")), "[Tooltip.DefaultColor:a, Tooltip.DefaultColor:b]");
        check("empty tooltip", texts(GearPreview.tooltipLines("")), "[]");
        check("comparison block", texts(GearPreview.blockLines("\ue07e0heading\n\ue07e3better +4.5 %\n\ue07e1worse -11.3 %\n\ue07e5note \ue07e3x\n")),
            "[Tooltip.DefaultColor:heading, FontColorPartyGreen:better +4.5 %, FontColorRed:worse -11.3 %, Tooltip.substringColor:note x]");

        System.out.println(failed == 0 ? "all ok" : failed + " FAILED");
        System.exit(failed == 0 ? 0 : 1);
    }
}
