

package com.xiaoboluo.d2r;

final class AppText {
    static String[] labels = {"DMG", "DEF"};
    private AppText() {}
    static String get(android.content.Context c, int id) { return id == R.string.dmAndroidDotDamage ? labels[0] : labels[1]; }
    static String get(android.content.Context c, int id, String arg) { return get(c, id) + "/" + arg; }
}
