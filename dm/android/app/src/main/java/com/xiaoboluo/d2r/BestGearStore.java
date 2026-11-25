package com.xiaoboluo.d2r;

import android.content.Context;
import android.content.SharedPreferences;




final class BestGearStore {
    static final String PREFS = "dm_bestgear";
    private static final String CHARMS = "searchCharms";

    private BestGearStore() {}


    static int[] fromProfile(String profile) {
        if (profile == null) return null;
        try {
            org.json.JSONObject g = new org.json.JSONObject(profile).optJSONObject("bestgear");
            if (g == null || !g.has("survivalHits") || !g.has("bossShare")) return null;
            return BestGearModel.fromGuide(g.getInt("survivalHits"), g.getDouble("bossShare"));
        } catch (org.json.JSONException e) { return null; }
    }

    private static SharedPreferences prefs(Context c) { return c.getSharedPreferences(PREFS, Context.MODE_PRIVATE); }
    static int[] goal(Context c, String name, int classIndex, String profile) {
        int[] own = name == null || name.isEmpty() ? null : BestGearModel.decode(prefs(c).getString(BestGearModel.key(name, classIndex), null));
        return BestGearModel.pick(own, fromProfile(profile));
    }

    static boolean searchCharms(Context c) { return prefs(c).getBoolean(CHARMS, true); }
    static void searchCharms(Context c, boolean on) { prefs(c).edit().putBoolean(CHARMS, on).apply(); }
    static void put(Context c, String name, int classIndex, int hits, int boss) {
        String v = BestGearModel.encode(hits, boss);
        if (name != null && !name.isEmpty() && BestGearModel.decode(v) != null) prefs(c).edit().putString(BestGearModel.key(name, classIndex), v).apply();
    }
}
