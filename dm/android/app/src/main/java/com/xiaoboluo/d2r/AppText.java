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

    static String get(Context c, int id) { return res(c).getString(id); }
    static String get(Context c, int id, Object... args) { return res(c).getString(id, args); }
}
