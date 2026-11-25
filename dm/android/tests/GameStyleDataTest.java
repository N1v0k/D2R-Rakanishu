





package com.xiaoboluo.d2r;

public final class GameStyleDataTest {
    private static int checks, fails;
    private static void check(boolean ok, String what) { checks++; if (!ok) { fails++; System.out.println("FAIL " + what); } }
    private static boolean near(float a, float b) { return Math.abs(a - b) < 1e-4f; }

    private static final String TSV = String.join("\n",
        "# game style",
        "colour\tFontColorWhite\t250\t250\t250\t255",
        "colour\tTooltip.backgroundColor\t0\t0\t0\t191",
        "colour\tSkillTree.granted\t102\t102\t255\t255",
        "colour\tFontColorPartyGreen\t79\t194\t56\t255",
        "shadow\tdefault\t3\t3\t0\t0\t0\t200",
        "shadow\tbutton\t4\t4\t0\t0\t0\t200",
        "size\thd\tMediumFontSize\t26",
        "size\tconsole\tMediumFontSize\t28",
        "size\tconsole\tSmallFontSize\t26",
        "scale\tconsole\t1.16",
        "spacing\tMinimumSpacing\t0.88",
        "pad\ttooltip\t20\t12\t20\t12",
        "tint\thover\t1.5\t1.447\t1.335\t1.5",
        "font\tko\tglobal\tBlizzardGlobalTCUnicode.ttf",
        "font\tde\texocet\tExocetBlizzardOT-Medium.otf",
        "counter\t154\t79\t62\t62\t132\tMediumFontSize\tMinimumSpacing\tFontColorWhite",
        "box\tnumbox\t6",
        "future\twhatever\t1\t2\t3",
        "") + "colour\tFontColorRed\t252\t70\t70\t255\r\n";

    private static void rows() {
        GameStyleData d = GameStyleData.parse(TSV);
        check(d.rows() == 19 && d.bad() == 0, "all rows taken, the unknown kind is not broken: " + d.rows() + "/" + d.bad());
        check(d.colour("FontColorWhite") == 0xFFFAFAFA, "colour from the table wins over the built-in one");
        check(d.colour("FontColorRed") == 0xFFFC4646, "CRLF line");
        check(d.colour("Tooltip.backgroundColor") == 0xBF000000, "alpha kept");
        check(d.colour("SkillTree.granted") == 0xFF6666FF, "granted blue");
        check(d.colour("FontColorPartyGreen") == 0xFF4FC238, "imported positive colour");
        GameStyleData.Shadow s = d.shadow("button");
        check(s != null && near(s.dx, 4) && near(s.dy, 4) && s.argb == 0xC8000000, "button shadow 4/4 alpha 200");
        check(d.shadow(null) == null, "no shadow");
        check(near(d.size("MediumFontSize"), 28) && near(d.size(GameStyleData.HD, "MediumFontSize"), 26), "sizes per profile, console by default");
        check(near(d.size("SmallFontSize"), 26), "console small");
        check(near(d.scale(GameStyleData.CONSOLE), 1.16f) && near(d.scale(GameStyleData.HD), 1f), "scales (hd from the built-in table)");
        check(near(d.spacing("MinimumSpacing"), 0.88f) && near(d.spacing(null), 1), "spacing");
        float[] p = d.pad("tooltip");
        check(p != null && near(p[0], 20) && near(p[1], 12) && near(p[2], 20) && near(p[3], 12), "tooltip padding");
        float[] t = d.tint("hover");
        check(t != null && near(t[1], 1.447f), "hover tint");
        check("BlizzardGlobalTCUnicode.ttf".equals(d.font("ko", GameStyleData.GLOBAL)), "Korean global font from the table");
        check("KoDia.ttf".equals(d.font("ko", GameStyleData.EXOCET)), "Korean Exocet from the built-in table");
        check("ExocetBlizzardOT-Medium.otf".equals(d.font("xx", GameStyleData.EXOCET)), "unknown language takes English");
        GameStyleData.Counter c = d.counter();
        check(c != null && near(c.x, 154) && near(c.w, 62) && near(c.field, 132) && "MediumFontSize".equals(c.size) && "FontColorWhite".equals(c.colour), "counter row");
        check(d.box("numbox") == 6 && d.box("other") == -1, "box ends");
    }

    private static void broken() {
        String bad = String.join("\n",
            "colour\tFontColorWhite\t300\t0\t0\t255",
            "colour\tFontColorBlue\t1\t2\t3",
            "colour\t\t1\t2\t3\t4",
            "shadow\tdefault\tx\t3\t0\t0\t0\t200",
            "size\tconsole\tMediumFontSize\t-1",
            "size\tconsole\tSmallFontSize\tNaN",
            "spacing\tMinimumSpacing\t0",
            "font\ten\texocet\t../evil.ttf",
            "font\ten\tformal\t.hidden.ttf",
            "counter\t154\t79\t0\t62\t132\tMediumFontSize\tMinimumSpacing\tFontColorWhite",
            "counter\t154\t79\t62\t62\t132\t\tMinimumSpacing\tFontColorWhite",
            "box\tnumbox\t-2",
            "colour\tFontColorGreen\t1\t2\t3\t4");
        GameStyleData d = GameStyleData.parse(bad);
        check(d.bad() == 12 && d.rows() == 1, "broken rows counted: " + d.bad() + " broken, " + d.rows() + " taken");
        check(d.colour("FontColorWhite") == 0xFFF0F0F0, "broken colour falls back to the built-in one");
        check(d.colour("FontColorBlue") == 0xFF6E6EFF, "short row falls back");
        check(d.colour("FontColorGreen") == 0x04010203, "the good row is still taken");
        check(near(d.shadow("default").dx, 3) && d.shadow("default").argb == 0xC8000000, "broken shadow falls back to 3/3 alpha 200");
        check(near(d.size("MediumFontSize"), 28) && near(d.size("SmallFontSize"), 26), "broken sizes fall back");
        check(near(d.spacing("MinimumSpacing"), 0.88f), "broken spacing falls back");
        check("ExocetBlizzardOT-Medium.otf".equals(d.font("en", GameStyleData.EXOCET)) && "Formal436BT.ttf".equals(d.font("en", GameStyleData.FORMAL)), "no path in a font name");
        check(near(d.counter().w, 62) && "MediumFontSize".equals(d.counter().size), "broken counter falls back");
        check(d.box("numbox") == -1, "broken box is unknown");
    }

    private static void fallback() {
        GameStyleData d = GameStyleData.parse(null);
        check(d.empty() && d.rows() == 0, "no file: empty");

        check(d.colour("FontColorGoldYellow") == 0xFFC7B377, "gold unified to FontColorGoldYellow");
        check(d.colour("Tooltip.backgroundColor") == 0xBF000000 && d.colour("Tooltip.inGameBackgroundColor") == 0xBF000000, "tooltip fills");
        check(d.colour("Tooltip.UniqueColor") == 0xFFC7B377 && d.colour("Tooltip.substringColor") == 0xFF949494, "tooltip text colours");
        check(d.colour("PanelNoteTextColor") == 0xFFADA894 && d.colour("FontColorLightGold") == 0xFFFFF6E3, "named meanings");
        check(d.shadow("victory").argb == 0xFF000000 && near(d.shadow("victory").dx, 8), "victory shadow");
        check(near(d.size("TooltipFontSize"), 36) && near(d.size(GameStyleData.HD, "SmallFontSize"), 22), "sizes");
        check(near(d.size("NoSuchFontSize"), 0), "unknown size = 0");
        check(near(d.pad("tooltipInGame")[0], 7), "in-game padding");
        check(near(d.counter().field, 132) && "MinimumSpacing".equals(d.counter().spacing), "counter");
        check("IrisL.ttf".equals(d.font("ko", GameStyleData.FORMAL)) && "Philosopher-BoldItalic.ttf".equals(d.font("ru", GameStyleData.FORMAL))
            && "BlizzardGlobal-V5_81.ttf".equals(d.font("zh", GameStyleData.EXOCET)) && "BlizzardGlobal-V5_81.ttf".equals(d.font("ko", GameStyleData.GLOBAL)), "fonts as AppArt.roleFile");

        GameStyleData t = GameStyleData.parse("colour\tFontColorRed\t1\t1\t1\t255\n");
        t.legacy("FontColorRed", 0xFF020202); t.legacy("FontColorYellow", 0xFF030303);
        check(t.colour("FontColorRed") == 0xFF010101, "style.tsv before dmg_colors.txt");
        check(t.colour("FontColorYellow") == 0xFF030303, "dmg_colors.txt before built-in");
        check(t.colour("FontColorPartyGreen") == 0xFF4FC238, "positive fallback");
        check(t.colour("SkillTree.granted") == 0xFF6666FF, "extra skill levels fallback");
        check(t.colour("FontColorGreen") == 0xFF00FC00, "built-in last");

        check(d.colour("NoSuchColour") == 0xFFF0F0F0 && d.colour("NoSuchColour") == 0xFFF0F0F0, "unknown colour = FontColorWhite");
        check(d.unknown().size() == 1 && "NoSuchColour".equals(d.unknown().get(0)), "unknown name noted once");
        check(!d.has("NoSuchColour") && d.has("FontColorBlue"), "has()");
    }


    private static void presets() {
        GameStyleData d = GameStyleData.parse(null);
        for (GameStyleData.Preset p : GameStyleData.PRESETS) {
            check(d.has(p.colour), p.name + " colour " + p.colour);
            check(p.shadow == null || d.shadow(p.shadow) != null, p.name + " shadow " + p.shadow);
            check(p.size != null ? d.size(p.size) > 0 : p.units > 0, p.name + " size");
            check(java.util.Arrays.asList("DefaultSpacing", "ReducedSpacing", "MinimumSpacing").contains(p.spacing) && d.spacing(p.spacing) > 0, p.name + " spacing " + p.spacing);
            check(p.face >= 0 && p.face < GameStyleData.ROLES.length, p.name + " face");
        }
        check(GameStyleData.ICON_NUMBER.colour.equals(d.counter().colour) && GameStyleData.ICON_NUMBER.size.equals(d.counter().size)
            && GameStyleData.ICON_NUMBER.spacing.equals(d.counter().spacing) && "button".equals(GameStyleData.ICON_NUMBER.shadow), "ICON_NUMBER = the skill tree's point box");
        check(GameStyleData.TOOLTIP.shadow == null && GameStyleData.TOOLTIP_TITLE.shadow == null && GameStyleData.TOOLTIP_SUB.shadow == null, "tooltip text has no shadow");
        check(GameStyleData.LEGEND.shadow == null, "legend has no shadow");
    }

    public static void main(String[] a) {
        rows(); broken(); fallback(); presets();
        System.out.println("GameStyleDataTest: " + checks + " checks, " + fails + " failed");
        if (fails != 0) System.exit(1);
    }
}
