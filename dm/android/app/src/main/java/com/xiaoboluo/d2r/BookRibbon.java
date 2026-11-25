package com.xiaoboluo.d2r;




final class BookRibbon {
    private BookRibbon() {}

    static final String EDGE = "FontColorDarkGold", SHINE = "FontColorGoldYellow";

    static final float[] SHADE = {0.42f, 0.86f, 0.66f, 0.38f}, AT = {0f, 0.38f, 0.72f, 1f};
    static final float GREY = 0.18f;

    static final float NEAR = 0.20f, FAR = 0.40f;


    static int cloth(int argb, float share) {
        int r = argb >> 16 & 255, g = argb >> 8 & 255, b = argb & 255;
        float grey = 0.3f * r + 0.59f * g + 0.11f * b;
        return 0xFF000000 | channel((r + (grey - r) * GREY) * share) << 16 | channel((g + (grey - g) * GREY) * share) << 8 | channel((b + (grey - b) * GREY) * share);
    }
    private static int channel(float v) { return Math.max(0, Math.min(255, Math.round(v))); }


    static int[] shades(int argb) {
        int[] out = new int[SHADE.length];
        for (int i = 0; i < out.length; i++) out[i] = cloth(argb, SHADE[i]);
        return out;
    }


    static void sash(float side, float[] out) { out[0] = side * NEAR; out[1] = side * FAR; }
}
