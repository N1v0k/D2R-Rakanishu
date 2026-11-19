package com.xiaoboluo.d2r;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;













final class GameStyleData {
    static final int EXOCET = 0, FORMAL = 1, GLOBAL = 2;
    static final String[] ROLES = {"exocet", "formal", "global"};
    static final String HD = "hd", CONSOLE = "console";

    static final String PROFILE = CONSOLE;
    static final float CONSOLE_UI_SCALE = 1.16f;

    static final class Shadow {
        final float dx, dy; final int argb;
        Shadow(float dx, float dy, int argb) { this.dx = dx; this.dy = dy; this.argb = argb; }
    }



    static final class Counter {
        final float x, y, w, h, field; final String size, spacing, colour;
        Counter(float x, float y, float w, float h, float field, String size, String spacing, String colour) {
            this.x = x; this.y = y; this.w = w; this.h = h; this.field = field; this.size = size; this.spacing = spacing; this.colour = colour;
        }
    }



    static final class Preset {
        final String name, colour, size, shadow, spacing; final int face; final float units;
        Preset(String name, int face, String colour, String size, float units, String shadow, String spacing) {
            this.name = name; this.face = face; this.colour = colour; this.size = size; this.units = units; this.shadow = shadow; this.spacing = spacing;
        }
    }


    static final Preset ICON_NUMBER = new Preset("ICON_NUMBER", EXOCET, "FontColorWhite", "MediumFontSize", 0, "button", "MinimumSpacing");
    static final Preset TOOLTIP = new Preset("TOOLTIP", EXOCET, "Tooltip.DefaultColor", "TooltipFontSize", 0, null, "DefaultSpacing");
    static final Preset TOOLTIP_TITLE = new Preset("TOOLTIP_TITLE", EXOCET, "Tooltip.UniqueColor", "TooltipFontSize", 0, null, "DefaultSpacing");
    static final Preset TOOLTIP_SUB = new Preset("TOOLTIP_SUB", EXOCET, "Tooltip.substringColor", "TooltipFontSize", 0, null, "DefaultSpacing");
    static final Preset BUTTON_LABEL = new Preset("BUTTON_LABEL", EXOCET, "FontColorLightGold", "MediumFontSize", 0, "default", "DefaultSpacing");
    static final Preset TITLE = new Preset("TITLE", EXOCET, "FontColorGoldYellow", "LargeFontSize", 0, "default", "ReducedSpacing");
    static final Preset BODY = new Preset("BODY", FORMAL, "FontColorWhite", "MediumFontSize", 0, null, "ReducedSpacing");


    static final Preset MODAL_TEXT = new Preset("MODAL_TEXT", FORMAL, "ModalDescriptionTextColor", "MediumFontSize", 0, "default", "DefaultSpacing");
    static final Preset NOTE = new Preset("NOTE", FORMAL, "PanelNoteTextColor", "SmallFontSize", 0, null, "DefaultSpacing");





    static final Preset LEGEND = new Preset("LEGEND", EXOCET, "FontColorLightGray", null, 60f, null, "DefaultSpacing");

    static final Preset WORLD_NUMBER = new Preset("WORLD_NUMBER", EXOCET, "FontColorWhite", null, 90.72f, "default", "DefaultSpacing");


    static final Preset WORLD_CRIT = new Preset("WORLD_CRIT", EXOCET, "FontColorWhite", null, 90.72f, "button", "DefaultSpacing");

    static final Preset STATUS_VALUE = new Preset("STATUS_VALUE", EXOCET, "FontColorWhite", null, 51.84f, "default", "DefaultSpacing");
    static final Preset STATUS_LABEL = new Preset("STATUS_LABEL", EXOCET, "FontColorGoldYellow", null, 51.84f, "default", "DefaultSpacing");

    static final Preset ANNOUNCE = new Preset("ANNOUNCE", EXOCET, "FontColorDarkGold", null, 96f, "victory", "DefaultSpacing");










    static final Preset HUD_TEXT = new Preset("HUD_TEXT", EXOCET, "FontColorWhite", null, 36f * 1.26f, "default", "DefaultSpacing");
    static final Preset HUD_COUNT = new Preset("HUD_COUNT", EXOCET, "FontColorWhite", "MediumFontSize", 0, "button", "MinimumSpacing");





    static final Preset BOOK_NAME = new Preset("BOOK_NAME", EXOCET, "Tooltip.DefaultColor", "MediumLargeFontSize", 0, "default", "DefaultSpacing");
    static final Preset BOOK_STATUS = new Preset("BOOK_STATUS", EXOCET, "FontColorPartyGreen", "MediumLargeFontSize", 0, "default", "DefaultSpacing");
    static final Preset BOOK_ITEM = new Preset("BOOK_ITEM", EXOCET, "FontColorWhite", "SmallFontSize", 0, null, "DefaultSpacing");
    static final Preset BOOK_INFO = new Preset("BOOK_INFO", EXOCET, "FontColorLightGray", "SmallFontSize", 0, null, "DefaultSpacing");
    static final Preset BOOK_DESC = new Preset("BOOK_DESC", FORMAL, "FontColorLightGray", "SmallFontSize", 0, null, "ReducedSpacing");
    static final Preset BOOK_HEADER = new Preset("BOOK_HEADER", EXOCET, "FontColorGoldYellow", "MediumFontSize", 0, "default", "DefaultSpacing");
    static final Preset BOOK_INPUT = new Preset("BOOK_INPUT", GLOBAL, "FontColorWhite", "MediumLargeFontSize", 0, null, "DefaultSpacing");
    static final Preset BOOK_TAB = new Preset("BOOK_TAB", EXOCET, "TabsActiveTextColor", "MediumFontSize", 0, "default", "DefaultSpacing");
    static final int HINT_ALPHA = 84;





    static final Preset PANEL_TAB = new Preset("PANEL_TAB", EXOCET, "FontColorLightGray", null, 40, "default", "ReducedSpacing");
    static final Preset PANEL_SUBTAB = new Preset("PANEL_SUBTAB", EXOCET, "TabsInactiveTextColor", null, 40, "default", "DefaultSpacing");
    static final Preset PANEL_BUTTON = new Preset("PANEL_BUTTON", EXOCET, "LightButtonTextColor", null, 40, "default", "MinimumSpacing");


    static final String PANEL_TAB_ON = "FontColorVeryLightGray", PANEL_TAB_OFF = "FontColorLightGray";





    static final float PANEL_EM = 1000f / 727f;

    static final float CAPS_EM = 0.588f;
    static final Preset[] PRESETS = {ICON_NUMBER, TOOLTIP, TOOLTIP_TITLE, TOOLTIP_SUB, BUTTON_LABEL, TITLE, BODY, MODAL_TEXT, NOTE, LEGEND, WORLD_NUMBER, WORLD_CRIT, STATUS_VALUE, STATUS_LABEL, ANNOUNCE,
        HUD_TEXT, HUD_COUNT,
        BOOK_NAME, BOOK_STATUS, BOOK_ITEM, BOOK_INFO, BOOK_DESC, BOOK_HEADER, BOOK_INPUT, BOOK_TAB,
        PANEL_TAB, PANEL_SUBTAB, PANEL_BUTTON};


    private static final String[] FALLBACK_COLOURS = {
        "FontColorWhite", "F0F0F0", "FontColorVeryLightGray", "F0F0F0", "FontColorGoldYellow", "C7B377", "FontColorGold", "C7B377", "FontColorLightGold", "FFF6E3",
        "FontColorBeige", "CCC3B0", "FontColorDarkGrayGold", "ADA894", "FontColorLightGray", "949494", "FontColorRed", "FC4646", "FontColorGreen", "00FC00",
        "FontColorPartyGreen", "4FC238", "FontColorDarkGold", "78622F", "FontColorBlue", "6E6EFF", "FontColorYellow", "FFFF64", "FontColorOrange", "FFA800", "FontColorDarkGrayBlue", "7D8D90",
        "PanelTitleColor", "C7B377", "PanelNoteTextColor", "ADA894", "LightButtonTextColor", "FFF6E3", "DarkButtonTextColor", "7D8D90",
        "ModalDescriptionTextColor", "FFF6E3", "TabsActiveTextColor", "F0F0F0", "TabsInactiveTextColor", "7D8D90", "TabsActiveTextColorInGame", "F0F0F0",
        "DescriptionTextColor", "F0F0F0", "DescriptionTitleColor", "F0F0F0",
        "Tooltip.substringColor", "949494", "Tooltip.DefaultColor", "F0F0F0", "Tooltip.MagicColor", "6E6EFF", "Tooltip.RareColor", "FFFF64",
        "Tooltip.SetColor", "00FC00", "Tooltip.UniqueColor", "C7B377", "Tooltip.CraftedColor", "FFA800", "Tooltip.QuestColor", "C7B377"};


    private static final int TOOLTIP_BACK = 0xBF000000;

    private final HashMap<String, Integer> colours = new HashMap<>(), legacy = new HashMap<>();
    private final HashMap<String, Shadow> shadows = new HashMap<>();
    private final HashMap<String, Float> sizes = new HashMap<>(), scales = new HashMap<>(), spacings = new HashMap<>();
    private final HashMap<String, float[]> pads = new HashMap<>(), tints = new HashMap<>();
    private final HashMap<String, String> fonts = new HashMap<>();
    private final HashMap<String, Integer> boxes = new HashMap<>();
    private final List<String> unknown = new ArrayList<>();
    private Counter counter;
    private int rows, bad;
    private static final GameStyleData BUILT_IN = builtIn();

    private GameStyleData() {}


    static GameStyleData parse(String tsv) {
        GameStyleData d = new GameStyleData();
        if (tsv == null) return d;
        for (String raw : tsv.split("\n")) {
            String line = raw.endsWith("\r") ? raw.substring(0, raw.length() - 1) : raw;
            if (line.isEmpty() || line.startsWith("#")) continue;
            if (d.row(line.split("\t", -1))) d.rows++; else d.bad++;
        }
        return d;
    }


    private boolean row(String[] f) {
        try {
            switch (f[0]) {
                case "colour": need(f, 6); colours.put(f[1], argb(f[2], f[3], f[4], f[5])); return true;
                case "shadow": need(f, 8); shadows.put(f[1], new Shadow(num(f[2]), num(f[3]), argb(f[4], f[5], f[6], f[7]))); return true;
                case "size": need(f, 4); sizes.put(f[1] + "\t" + f[2], positive(f[3])); return true;
                case "scale": need(f, 3); scales.put(f[1], positive(f[2])); return true;
                case "spacing": need(f, 3); spacings.put(f[1], positive(f[2])); return true;
                case "pad": need(f, 6); pads.put(f[1], new float[]{nonNegative(f[2]), nonNegative(f[3]), nonNegative(f[4]), nonNegative(f[5])}); return true;
                case "tint": need(f, 6); tints.put(f[1], new float[]{nonNegative(f[2]), nonNegative(f[3]), nonNegative(f[4]), nonNegative(f[5])}); return true;
                case "font": need(f, 4); if (!file(f[3])) return false; fonts.put(f[1] + "\t" + f[2], f[3]); return true;
                case "box": need(f, 3); boxes.put(f[1], (int) nonNegative(f[2])); return true;
                case "counter":
                    need(f, 9);
                    if (f[6].isEmpty() || f[7].isEmpty() || f[8].isEmpty()) return false;
                    counter = new Counter(nonNegative(f[1]), nonNegative(f[2]), positive(f[3]), positive(f[4]), positive(f[5]), f[6], f[7], f[8]);
                    return true;
                default: return true;
            }
        } catch (IllegalArgumentException e) { return false; }
    }

    private static void need(String[] f, int n) { if (f.length != n || f[1].isEmpty()) throw new IllegalArgumentException("fields"); }
    private static float num(String s) { float v = Float.parseFloat(s); if (Float.isNaN(v) || Float.isInfinite(v)) throw new IllegalArgumentException(s); return v; }
    private static float positive(String s) { float v = num(s); if (!(v > 0)) throw new IllegalArgumentException(s); return v; }
    private static float nonNegative(String s) { float v = num(s); if (v < 0) throw new IllegalArgumentException(s); return v; }
    private static int argb(String r, String g, String b, String a) { return channel(a) << 24 | channel(r) << 16 | channel(g) << 8 | channel(b); }
    private static int channel(String s) { int v = Integer.parseInt(s); if (v < 0 || v > 255) throw new IllegalArgumentException(s); return v; }

    private static boolean file(String s) { return !s.isEmpty() && s.indexOf('/') < 0 && s.indexOf('\\') < 0 && !s.startsWith("."); }

    int rows() { return rows; }
    int bad() { return bad; }
    boolean empty() { return rows == 0; }


    void legacy(String name, int argb) { legacy.put(name, argb); }

    boolean has(String name) { return colours.containsKey(name) || legacy.containsKey(name) || BUILT_IN.colours.containsKey(name); }


    int colour(String name) {
        Integer c = colours.get(name);
        if (c == null) c = legacy.get(name);
        if (c == null) c = BUILT_IN.colours.get(name);
        if (c != null) return c;
        if (!unknown.contains(name)) unknown.add(name);
        return BUILT_IN.colours.get("FontColorWhite");
    }


    List<String> unknown() { return unknown; }

    Shadow shadow(String name) {
        if (name == null) return null;
        Shadow s = shadows.get(name);
        return s != null ? s : BUILT_IN.shadows.get(name);
    }


    float size(String name) { return size(PROFILE, name); }
    float size(String profile, String name) {
        Float v = sizes.get(profile + "\t" + name);
        if (v == null) v = BUILT_IN.sizes.get(profile + "\t" + name);
        return v == null ? 0 : v;
    }

    float scale(String profile) { Float v = scales.get(profile); if (v == null) v = BUILT_IN.scales.get(profile); return v == null ? 1 : v; }


    float spacing(String name) {
        if (name == null) return 1;
        Float v = spacings.get(name);
        if (v == null) v = BUILT_IN.spacings.get(name);
        return v == null ? 1 : v;
    }


    float[] pad(String name) { float[] p = pads.get(name); return p != null ? p : BUILT_IN.pads.get(name); }


    float[] tint(String name) { float[] t = tints.get(name); return t != null ? t : BUILT_IN.tints.get(name); }


    String font(String lang, int role) {
        String key = lang + "\t" + ROLES[role], f = fonts.get(key);
        if (f == null) f = BUILT_IN.fonts.get(key);
        if (f == null) f = BUILT_IN.fonts.get("en\t" + ROLES[role]);
        return f;
    }


    int box(String name) { Integer b = boxes.get(name); return b == null ? -1 : b; }

    Counter counter() { return counter != null ? counter : BUILT_IN.counter; }


    private static GameStyleData builtIn() {
        GameStyleData d = new GameStyleData();
        for (int i = 0; i < FALLBACK_COLOURS.length; i += 2) d.colours.put(FALLBACK_COLOURS[i], 0xFF000000 | Integer.parseInt(FALLBACK_COLOURS[i + 1], 16));
        d.colours.put("Tooltip.backgroundColor", TOOLTIP_BACK);
        d.colours.put("Tooltip.inGameBackgroundColor", TOOLTIP_BACK);
        d.colours.put("SkillTree.granted", 0xFF6666FF);
        d.shadows.put("default", new Shadow(3, 3, 0xC8000000));
        d.shadows.put("button", new Shadow(4, 4, 0xC8000000));
        d.shadows.put("victory", new Shadow(8, 8, 0xFF000000));
        String[] names = {"SmallFontSize", "MediumFontSize", "XMediumFontSize", "MediumLargeFontSize", "XMediumLargeFontSize", "LargeFontSize", "TooltipFontSize"};
        float[] hd = {22, 26, 28, 32, 34, 38, 36}, console = {26, 28, 30, 32, 34, 38, 36};
        for (int i = 0; i < names.length; i++) { d.sizes.put(HD + "\t" + names[i], hd[i]); d.sizes.put(CONSOLE + "\t" + names[i], console[i]); }
        d.scales.put(HD, 1f); d.scales.put(CONSOLE, CONSOLE_UI_SCALE);
        d.spacings.put("DefaultSpacing", 1f); d.spacings.put("ReducedSpacing", 0.94f); d.spacings.put("MinimumSpacing", 0.88f);
        d.pads.put("tooltip", new float[]{20, 12, 20, 12}); d.pads.put("tooltipInGame", new float[]{7, 1, 7, 1});
        d.tints.put("hover", new float[]{1.5f, 1.447f, 1.335f, 1.5f});
        d.counter = new Counter(154, 79, 62, 62, 132, "MediumFontSize", "MinimumSpacing", "FontColorWhite");

        for (String lang : new String[]{"en", "de", "es", "mx", "ru", "ko", "zh"}) {
            String exocet = "ko".equals(lang) ? "KoDia.ttf" : "zh".equals(lang) ? "BlizzardGlobal-V5_81.ttf" : "ExocetBlizzardOT-Medium.otf";
            String formal = "ko".equals(lang) ? "IrisL.ttf" : "zh".equals(lang) ? "BlizzardGlobal-V5_81.ttf" : "ru".equals(lang) ? "Philosopher-BoldItalic.ttf" : "Formal436BT.ttf";
            d.fonts.put(lang + "\texocet", exocet); d.fonts.put(lang + "\tformal", formal); d.fonts.put(lang + "\tglobal", "BlizzardGlobal-V5_81.ttf");
        }
        return d;
    }
}
