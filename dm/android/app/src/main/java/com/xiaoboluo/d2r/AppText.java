package com.xiaoboluo.d2r;

import android.content.Context;
import android.content.res.Configuration;
import android.content.res.Resources;
import java.util.HashMap;








final class AppText {
    private static final HashMap<String, Resources> cache = new HashMap<>();
    private AppText() {}

    static Resources res(Context c) { return GameLang.known() ? res(c, GameLang.code()) : c.getResources(); }



    static synchronized Resources res(Context c, String code) {
        Resources r = cache.get(code);
        if (r == null) {
            Context app = c.getApplicationContext() != null ? c.getApplicationContext() : c;
            Configuration cfg = new Configuration(app.getResources().getConfiguration());
            cfg.setLocale(GameLang.locale(code));
            r = app.createConfigurationContext(cfg).getResources();
            cache.put(code, r);
        }
        return r;
    }


    static String get(Context c, int id) { String s = res(c).getString(id); return Plural.has(s) ? Plural.pick(s, -1) : s; }
    static String get(Context c, int id, Object... args) {
        Resources r = res(c);
        String s = r.getString(id);
        if (!Plural.has(s)) return r.getString(id, args);
        return Plural.format(r.getConfiguration().getLocales().get(0), s, args);
    }
}
