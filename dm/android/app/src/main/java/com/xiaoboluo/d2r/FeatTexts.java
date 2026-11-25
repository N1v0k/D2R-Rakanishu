package com.xiaoboluo.d2r;

import android.content.Context;
import java.text.NumberFormat;
import java.util.Locale;




final class FeatTexts {
    private FeatTexts() {}

    private static final int[] NAMES = { 0, R.string.dmAndroidFeatName1, R.string.dmAndroidFeatName2, R.string.dmAndroidFeatName3, R.string.dmAndroidFeatName4,
        R.string.dmAndroidFeatName5, R.string.dmAndroidFeatName6, R.string.dmAndroidFeatName7, R.string.dmAndroidFeatName8, R.string.dmAndroidFeatName9,
        R.string.dmAndroidFeatName10 };
    private static final int[] MERCS = { R.string.dmAndroidFeatName39, R.string.dmAndroidFeatName40, R.string.dmAndroidFeatName41, R.string.dmAndroidFeatName42 };
    private static final int[] CRAFTS = { R.string.dmAndroidFeatName24, R.string.dmAndroidFeatName25, R.string.dmAndroidFeatName26, R.string.dmAndroidFeatName27,
        R.string.dmAndroidFeatName28, R.string.dmAndroidFeatName29, R.string.dmAndroidFeatName30 };
    private static final int[] CRAFT_CONDITIONS = { R.string.dmAndroidFeatGem, R.string.dmAndroidFeatRuneword, R.string.dmAndroidFeatRuneUp, R.string.dmAndroidFeatPerfect,
        R.string.dmAndroidFeatPrism, R.string.dmAndroidFeatRecharge, R.string.dmAndroidFeatUnsocket };
    private static final int[] ACTS = { R.string.dmAndroidFeatAct1, R.string.dmAndroidFeatAct2, R.string.dmAndroidFeatAct3, R.string.dmAndroidFeatAct4, R.string.dmAndroidFeatAct5 };

    private static final int[] OWN = { R.string.dmAndroidFeatName44, R.string.dmAndroidFeatName45, R.string.dmAndroidFeatName46, R.string.dmAndroidFeatName47,
        R.string.dmAndroidFeatName48, R.string.dmAndroidFeatName49, R.string.dmAndroidFeatName50, R.string.dmAndroidFeatName51, R.string.dmAndroidFeatName52,
        R.string.dmAndroidFeatName53, R.string.dmAndroidFeatName54, R.string.dmAndroidFeatName55, R.string.dmAndroidFeatName56, R.string.dmAndroidFeatName57,
        R.string.dmAndroidFeatName58, R.string.dmAndroidFeatName59, R.string.dmAndroidFeatName60 };

    private static boolean classRow(int id) { return (id >= 13 && id <= 19) || id == 43; }



    static String[] of(Context c, int id, long arg, String name, String diff) {
        Locale language = AppText.res(c).getConfiguration().getLocales().get(0);
        NumberFormat num = NumberFormat.getIntegerInstance(language);
        if (id >= 44 && id <= 60) {
            String own = AppText.get(c, OWN[id - 44]);
            switch (id) {
                case 44: case 45: case 46: return new String[]{own, AppText.get(c, R.string.dmAndroidFeatStreak, num.format(arg))};
                case 47: return new String[]{own, AppText.get(c, R.string.dmAndroidFeatBosses, num.format(arg))};
                case 48: return new String[]{own, AppText.get(c, R.string.dmAndroidFeatBossFast, num.format(arg / FeatsModel.TICKS_PER_SECOND))};
                case 49: return new String[]{own, AppText.get(c, R.string.dmAndroidFeatCalm)};
                case 50: return new String[]{own, AppText.get(c, R.string.dmAndroidFeatDry, diff)};
                case 51: return new String[]{own, AppText.get(c, R.string.dmAndroidFeatEscape, num.format(FeatsModel.ESCAPE_PCT))};
                case 52: return new String[]{own, AppText.get(c, R.string.dmAndroidFeatRiftClear)};
                case 53: case 54: return new String[]{own, AppText.get(c, R.string.dmAndroidFeatRiftTier, num.format(arg))};
                case 55: return new String[]{own, AppText.get(c, R.string.dmAndroidFeatRiftFast, num.format(FeatsModel.RIFT_FAST_TIER), num.format(arg / 60))};
                case 56: return new String[]{own, AppText.get(c, R.string.dmAndroidFeatRush)};
                case 57: return new String[]{own, AppText.get(c, R.string.dmAndroidFeatShrines, num.format(arg))};
                case 58: return new String[]{own, AppText.get(c, R.string.dmAndroidFeatEquip, num.format(FeatsModel.EQUIP_PCT))};
                case 59: return new String[]{own, AppText.get(c, R.string.dmAndroidFeatLook, num.format(arg))};
                default: return new String[]{own, AppText.get(c, R.string.dmAndroidFeatDye, num.format(arg))};
            }
        }
        if (id >= 1 && id <= 3) return new String[]{AppText.get(c, NAMES[id]), AppText.get(c, R.string.dmAndroidFeatKills, num.format(arg))};
        if (id >= 4 && id <= 8) return new String[]{AppText.get(c, NAMES[id]), AppText.get(c, ACTS[id - 4])};
        if (id == 9 || id == 10) return new String[]{AppText.get(c, NAMES[id]), AppText.get(c, R.string.dmAndroidFeatDifficulty, diff)};
        if (classRow(id)) return new String[]{AppText.get(c, R.string.dmAndroidFeatNameClass, name), AppText.get(c, R.string.dmAndroidFeatDifficulty, diff)};
        if (id == 20) return new String[]{AppText.get(c, R.string.dmAndroidFeatName20), AppText.get(c, R.string.dmAndroidFeatEveryClass, diff)};
        if (id == 21) return new String[]{AppText.get(c, R.string.dmAndroidFeatName21), AppText.get(c, R.string.dmAndroidFeatLevel, num.format(arg))};
        if (id == 12) return new String[]{AppText.get(c, R.string.dmAndroidFeatName12), AppText.get(c, R.string.dmAndroidFeatWaypoints)};
        if (id == 11) return new String[]{AppText.get(c, R.string.dmAndroidFeatName11), AppText.get(c, R.string.dmAndroidFeatQuests)};
        if (id == 22) return new String[]{AppText.get(c, R.string.dmAndroidFeatName22), AppText.get(c, R.string.dmAndroidFeatHcLevel, num.format(arg))};
        if (id == 23) return new String[]{AppText.get(c, R.string.dmAndroidFeatName23), AppText.get(c, R.string.dmAndroidFeatHcGame)};
        if (id == 31) return new String[]{AppText.get(c, R.string.dmAndroidFeatName31), AppText.get(c, R.string.dmAndroidFeatOwn, name)};
        if (id == 32) return new String[]{AppText.get(c, R.string.dmAndroidFeatName32), AppText.get(c, R.string.dmAndroidFeatOwn, name)};
        if (id == 33) return new String[]{AppText.get(c, R.string.dmAndroidFeatName33), AppText.get(c, R.string.dmAndroidFeatUniques, num.format(arg))};
        if (id == 34) return new String[]{AppText.get(c, R.string.dmAndroidFeatName34), AppText.get(c, R.string.dmAndroidFeatFullSet)};
        if (id >= 39 && id <= 42) return new String[]{AppText.get(c, MERCS[id - 39]), AppText.get(c, R.string.dmAndroidFeatMerc, name)};
        if (id >= 24 && id <= 30) return new String[]{AppText.get(c, CRAFTS[id - 24]), AppText.get(c, CRAFT_CONDITIONS[id - 24])};
        if (id == 35) return new String[]{AppText.get(c, R.string.dmAndroidFeatName35), AppText.get(c, R.string.dmAndroidFeatSales, num.format(arg))};
        if (id == 36 || id == 37) return new String[]{AppText.get(c, id == 36 ? R.string.dmAndroidFeatName36 : R.string.dmAndroidFeatName37), AppText.get(c, R.string.dmAndroidFeatGold, num.format(arg))};
        if (id == 38) return new String[]{AppText.get(c, R.string.dmAndroidFeatName38), AppText.get(c, R.string.dmAndroidFeatGamble)};
        return null;
    }









    static String picture(int id) {
        switch (id) {
            case 1: return "sku"; case 2: return "skl"; case 3: return "skz";
            case 4: return "hdm"; case 5: return "hst"; case 6: return "mss"; case 7: return "hfh"; case 8: return "fed";
            case 9: return "tes"; case 10: return "bet";
            case 13: return "am1"; case 14: return "ob1"; case 15: return "ne1"; case 16: return "pa1"; case 17: return "ba1"; case 18: return "dr1"; case 19: return "ktr";
            case 43: return "wa5";
            case 20: return "toa";
            case 21: return "ass";
            case 11: return "bbb"; case 12: return "tbk"; case 23: return "rvl"; case 22: return "ear";
            case 31: return "rin"; case 32: return "std"; case 33: return "crn"; case 34: return "ful";
            case 39: return "sbw"; case 40: return "spr"; case 41: return "sst"; case 42: return "bsd";
            case 24: return "gsv"; case 25: return "r26"; case 26: return "r10"; case 27: return "gpr"; case 28: return "amu"; case 29: return "lsd"; case 30: return "r15";
            case 35: case 36: case 37: return "gld"; case 38: return "ci0";
            case 44: return "axe"; case 45: return "btx"; case 46: return "gix"; case 47: return "jaw"; case 48: return "scy"; case 49: return "pa1";
            case 50: return "hp5"; case 51: return "ice";
            case 52: return "xa1"; case 53: return "xa2"; case 54: return "xa3"; case 55: return "xa4"; case 56: return "pk1";
            case 57: return "tch"; case 58: return "ghm"; case 59: return "aar"; case 60: return "gpv";
            default: return null;
        }
    }

    static int pictureUnique(int id) { return id == 31 ? 122 : -1; }
}
