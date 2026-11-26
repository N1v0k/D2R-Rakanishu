









#include "../dm_i18n.h"
#include "../game_addrs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
static int fails;
static void expect(const char *what, const char *got, const char *want) { int ok = !strcmp(got, want); if (!ok) fails++; printf("%s %s：期望「%s」→ 真结果「%s」\n", ok ? "✓" : "✗", what, want, got); }
int main(int argc, char **argv) {
    const char *mode = argc > 1 ? argv[1] : "";
    char dir[512]; snprintf(dir, sizeof dir, "%s/i18n_%s", getenv("TMPDIR") ? getenv("TMPDIR") : "/tmp", mode);
    char sd[600], f[700]; snprintf(sd, sizeof sd, "%s/Settings", dir); snprintf(f, sizeof f, "%s/Settings.json", sd);
    mkdir(dir, 0755); mkdir(sd, 0755); remove(f); setenv("DM_SAVE", dir, 1);
    if (!strcmp(mode, "en") || !strcmp(mode, "game")) { FILE *o = fopen(f, "w"); fputs("{\n    \"Item Name Display\": 1,\n    \"Language\": 0,\n    \"AutoMapMode\": 2\n}\n", o); fclose(o); }
    if (!strcmp(mode, "zh")) { FILE *o = fopen(f, "w"); fputs("{ \"Language\": 8 }", o); fclose(o); }
    if (!strcmp(mode, "en")) expect("设置文件 Language 0、游戏还没设", dm_ui("dmConfirm"), "OK");
    else if (!strcmp(mode, "zh")) expect("设置文件 Language 8", dm_ui("dmConfirm"), "确定");
    else if (!strcmp(mode, "none")) { unsetenv("DM_LANG"); expect("没有设置文件、没给系统语言（Mac 缺省 zh-Hans）", dm_ui("dmConfirm"), "确定"); }
    else if (!strcmp(mode, "sysen")) { setenv("DM_LANG", "en-US", 1); expect("没有设置文件（首次安装）、系统语言 en-US", dm_ui("dmConfirm"), "OK"); }
    else if (!strcmp(mode, "syses")) { setenv("DM_LANG", "es-ES", 1); expect("没有设置文件（首次安装）、系统语言 es-ES", dm_ui("dmConfirm"), "Aceptar"); }
    else if (!strcmp(mode, "systw")) { setenv("DM_LANG", "zh-Hant-TW", 1); expect("没有设置文件、系统语言 zh-Hant-TW（繁体用中文）", dm_ui("dmConfirm"), "确定"); }
    else if (!strcmp(mode, "sysru")) { setenv("DM_LANG", "ru-RU", 1); expect("没有设置文件（首次安装）、系统语言 ru-RU", dm_ui("dmConfirm"), "ОК"); }
    else if (!strcmp(mode, "sysko")) { setenv("DM_LANG", "ko-KR", 1); expect("没有设置文件（首次安装）、系统语言 ko-KR", dm_ui("dmConfirm"), "확인"); }
    else if (!strcmp(mode, "syspt")) { setenv("DM_LANG", "pt-BR", 1); expect("没有设置文件（首次安装）、系统语言 pt-BR", dm_ui("dmConfirm"), "OK"); }
    else if (!strcmp(mode, "tags")) {
        static const struct { const char *t; int want; } c[] = { { "en-US", 0 }, { "en-GB", 0 }, { "zh-Hans-CN", 8 }, { "zh-Hans", 8 }, { "zh", 8 }, { "zh-Hans-HK", 8 }, { "zh-Hans-TW", 8 }, { "zh-Hans-MO", 8 },
            { "zh-Hant-TW", 8 }, { "zh-Hant-HK", 8 }, { "zh-Hant", 8 }, { "zh-HK", 8 }, { "zh_TW", 8 }, { "yue-Hant-HK", 8 }, { "yue-Hans-CN", 8 }, { "yue", 8 }, { "wuu-Hans-CN", 8 },
            { "de-DE", 1 }, { "de-AT", 1 }, { "de", 1 }, { "ru-RU", 7 }, { "ru", 7 }, { "ru-UA", 7 }, { "be-BY", 0 }, { "uk-UA", 0 }, { "es-ES", 2 }, { "es", 2 }, { "es_ES", 2 }, { "es-Latn-ES", 2 }, { "es-419", 10 }, { "es-MX", 10 }, { "es-US", 10 }, { "es-AR", 10 }, { "fr-CA", 0 }, { "it-IT", 0 }, { "ko-KR", 5 }, { "ko", 5 }, { "ko_KR", 5 }, { "ko-Kore-KR", 5 }, { "pl-PL", 0 }, { "ja-JP", 0 }, { "pt-BR", 12 }, { "pt", 12 }, { "pt-PT", 12 }, { "pt_BR", 12 }, { "nl-NL", 0 }, { "ar", 0 }, { "zu-ZA", 0 } };
        for (unsigned i = 0; i < sizeof c / sizeof *c; i++) { char w[8], g[8]; snprintf(w, sizeof w, "%d", c[i].want); snprintf(g, sizeof g, "%d", dm_lang_from_tag(c[i].t)); expect(c[i].t, g, w); }
        expect("性别标记：[ms] 去掉", dm_untag("[ms]Gorro"), "Gorro"); expect("性别标记：连写去掉", dm_untag("[fs][fp]Botas"), "Botas");
        expect("性别标记：注释方括号不动", dm_untag("[Exc] Botas"), "[Exc] Botas"); expect("性别标记：中文原样", dm_untag("谐角之冠"), "谐角之冠"); expect("性别标记：只有标记 = 空串", dm_untag("[ms]"), "");
        expect("grammar marker: [ns] removed", dm_untag("[ns]Abc"), "Abc"); expect("grammar marker: [pl] removed", dm_untag("[pl]Abc"), "Abc");
        expect("grammar marker: Polish [nl] removed", dm_untag("[nl]Abc"), "Abc"); expect("grammar marker: Polish [p] removed", dm_untag("[p]Abc"), "Abc");
        expect("grammar marker: [nl][p] chained", dm_untag("[nl][p]Abc"), "Abc"); expect("grammar marker: upper case is text", dm_untag("[PH]Abc"), "[PH]Abc");
        expect("grammar marker: [n] is text", dm_untag("[n]Abc"), "[n]Abc"); expect("grammar marker: [%s] is text", dm_untag("[%s] Abc"), "[%s] Abc"); }
    else if (!strcmp(mode, "game")) {
        expect("游戏没设前按设置文件", dm_ui("dmConfirm"), "OK");
        atomic_store(&g_dm_lang, 8); expect("游戏切到 zhCN", dm_ui("dmConfirm"), "确定");
        atomic_store(&g_dm_lang, 9); expect("游戏切到 zhTW（繁体用中文）", dm_ui("dmConfirm"), "确定");
        atomic_store(&g_dm_lang, 1); expect("游戏切到 deDE（德语）", strcmp(dm_ui("dmConfirm"), dm_ui_group("dmConfirm", DM_LG_DE)) ? "不同" : "相同", "相同");
        atomic_store(&g_dm_lang, 2); expect("游戏切到 esES（西语）", dm_ui("dmConfirm"), "Aceptar");
        atomic_store(&g_dm_lang, 10); expect("游戏切到 esMX（拉美西语，这条没有单独的拉美写法 = 同西语）", dm_ui("dmConfirm"), "Aceptar");
        expect("游戏切到 esMX：有拉美写法的条目取拉美写法（储藏箱 Arcón）", strstr(dm_ui("dmStoreStash"), "Arcón") ? "有" : "没有", "有");
        atomic_store(&g_dm_lang, 2); expect("游戏切到 esES：同一条取西班牙写法（储藏箱 Alijo）", strstr(dm_ui("dmStoreStash"), "Alijo") ? "有" : "没有", "有");
        atomic_store(&g_dm_lang, 7); expect("游戏切到 ruRU（俄语）", dm_ui("dmConfirm"), "ОК");
        expect("游戏切到 ruRU：表里的条目按俄语取（储藏箱 Тайник）", strstr(dm_ui("dmStoreStash"), "Тайник") ? "有" : "没有", "有");
        atomic_store(&g_dm_lang, 5); expect("游戏切到 koKR（韩语）", dm_ui("dmConfirm"), "확인");
        expect("游戏切到 koKR：表里的条目按韩语取（储藏箱 창고）", strstr(dm_ui("dmStoreStash"), "창고") ? "有" : "没有", "有");
        atomic_store(&g_dm_lang, 12); expect("游戏切到 ptBR（葡语）", dm_ui("dmConfirm"), "OK");
        expect("游戏切到 ptBR：表里的条目按葡语取（储藏箱 Baú）", strstr(dm_ui("dmStoreStash"), "Baú") ? "有" : "没有", "有");
        expect("ptBR: an empty Portuguese cell falls back to English", dm_ui("dmAndroidHudLevelWord"), "Level");
        expect("表里没有的键原样返回", dm_ui("dmNoSuchKey"), "dmNoSuchKey"); }
    else if (!strcmp(mode, "plural")) {

        static const struct { const char *lang; long long n; int frac; int want; } c[] = {
            { "en", 0, 0, DM_PL_OTHER }, { "en", 1, 0, DM_PL_ONE }, { "en", 2, 0, DM_PL_OTHER }, { "en", 1, 1, DM_PL_OTHER }, { "en", 21, 0, DM_PL_OTHER },
            { "de", 1, 0, DM_PL_ONE }, { "de", 11, 0, DM_PL_OTHER }, { "es", 1, 0, DM_PL_ONE }, { "es", 0, 0, DM_PL_OTHER }, { "es", 1000000, 0, DM_PL_OTHER },
            { "pt", 0, 0, DM_PL_ONE }, { "pt", 1, 0, DM_PL_ONE }, { "pt", 1, 1, DM_PL_ONE }, { "pt", 2, 0, DM_PL_OTHER },
            { "ru", 0, 0, DM_PL_MANY }, { "ru", 1, 0, DM_PL_ONE }, { "ru", 2, 0, DM_PL_FEW }, { "ru", 4, 0, DM_PL_FEW }, { "ru", 5, 0, DM_PL_MANY },
            { "ru", 11, 0, DM_PL_MANY }, { "ru", 12, 0, DM_PL_MANY }, { "ru", 14, 0, DM_PL_MANY }, { "ru", 21, 0, DM_PL_ONE }, { "ru", 22, 0, DM_PL_FEW },
            { "ru", 25, 0, DM_PL_MANY }, { "ru", 101, 0, DM_PL_ONE }, { "ru", 111, 0, DM_PL_MANY }, { "ru", 112, 0, DM_PL_MANY }, { "ru", 1, 1, DM_PL_OTHER },
            { "zh", 1, 0, DM_PL_OTHER }, { "ko", 1, 0, DM_PL_OTHER },

            { "es", 1, DM_PL_ZERO_DECIMALS, DM_PL_ONE }, { "es", 1, DM_PL_DECIMALS, DM_PL_OTHER }, { "de", 1, DM_PL_ZERO_DECIMALS, DM_PL_OTHER },
            { "en", 1, DM_PL_DECIMALS, DM_PL_OTHER }, { "pt", 1, DM_PL_DECIMALS, DM_PL_ONE }, { "ru", 2, DM_PL_ZERO_DECIMALS, DM_PL_OTHER } };
        for (unsigned i = 0; i < sizeof c / sizeof *c; i++) { char what[48], w[8], g[8]; snprintf(what, sizeof what, "category %s %lld%s", c[i].lang, c[i].n, c[i].frac == DM_PL_DECIMALS ? ",3" : c[i].frac ? ",0" : "");
            snprintf(w, sizeof w, "%d", c[i].want); snprintf(g, sizeof g, "%d", dm_plural_cat(c[i].lang, c[i].n, c[i].frac)); expect(what, g, w); }
        char buf[64];
        { char d[8]; snprintf(d, sizeof d, "%d%d%d%d%d", dm_plural_dec("1"), dm_plural_dec("1,0"), dm_plural_dec("1.00"), dm_plural_dec("1,3"), dm_plural_dec("9.04"));
          expect("how a number is shown: 1 / 1,0 / 1.00 / 1,3 / 9.04", d, "01122"); }
        expect("es 1,0: one", dm_plural("es|one=de %s a %s golpe|other=de %s a %s golpes", 1, dm_plural_dec("1,0"), buf, sizeof buf), "de %s a %s golpe");
        expect("es 1,3: other", dm_plural("es|one=de %s a %s golpe|other=de %s a %s golpes", 1, dm_plural_dec("1,3"), buf, sizeof buf), "de %s a %s golpes");
        expect("a plain text stays", dm_plural("%d Treffer", 1, 0, buf, sizeof buf), "%d Treffer");
        expect("de one", dm_plural("de|one=%d Punkt|other=%d Punkte", 1, 0, buf, sizeof buf), "%d Punkt");
        expect("de other", dm_plural("de|one=%d Punkt|other=%d Punkte", 7, 0, buf, sizeof buf), "%d Punkte");
        expect("ru many falls back to other", dm_plural("ru|one=a|few=b|other=c", 5, 0, buf, sizeof buf), "c");
        expect("ru few", dm_plural("ru|one=a|few=b|other=c", 3, 0, buf, sizeof buf), "b");
        expect("ru decimals: other", dm_plural("ru|one=a|few=b|many=d|other=c", 1, 1, buf, sizeof buf), "c");
        expect("form cut to the buffer", dm_plural("en|one=abcdef|other=x", 1, 0, buf, 4), "abc");
        expect("en rules on English fallback text: 0 is other", dm_plural("en|one=%d point|other=%d points", 0, 0, buf, sizeof buf), "%d points");
        expect("pt rules: 0 is one", dm_plural("pt|one=%d ponto|other=%d pontos", 0, 0, buf, sizeof buf), "%d ponto");
        expect("a text that only looks stamped stays", dm_plural("ab|x", 1, 0, buf, sizeof buf), "ab|x");

        expect("table de one", dm_plural(dm_ui_group("dmAndroidBestUnrated", DM_LG_DE), 1, 0, buf, sizeof buf), "%d Gegenstand nicht bewertet");
        expect("table de other", dm_plural(dm_ui_group("dmAndroidBestUnrated", DM_LG_DE), 2, 0, buf, sizeof buf), "%d Gegenstände nicht bewertet");
        expect("table pt one", dm_plural(dm_ui_group("dmAndroidBestUnrated", DM_LG_PT), 1, 0, buf, sizeof buf), "%d item não avaliado");
        expect("table ru: plain text", dm_plural(dm_ui_group("dmAndroidBestUnrated", DM_LG_RU), 1, 0, buf, sizeof buf), "%d предм. без оценки");
        expect("table ru few", dm_plural(dm_ui_group("dmRiftPtsStat", DM_LG_RU), 2, 0, buf, sizeof buf), "+%d очка характеристик");
        expect("table ru many", dm_plural(dm_ui_group("dmRiftPtsStat", DM_LG_RU), 5, 0, buf, sizeof buf), "+%d очков характеристик");
        expect("table zh: plain", dm_plural(dm_ui_group("dmRiftPtsStat", DM_LG_ZH), 1, 0, buf, sizeof buf), "%d 属性点");
        expect("table pt empty -> English with English rules (0 other)", dm_plural(dm_ui_group("dmCmpMore", DM_LG_PT), 0, 0, buf, sizeof buf), "+%d more changes");
        expect("table pt empty -> English (1 one)", dm_plural(dm_ui_group("dmCmpMore", DM_LG_PT), 1, 0, buf, sizeof buf), "+%d more change");
        expect("table esMX empty -> Spanish forms", dm_plural(dm_ui_group("dmCmpMore", DM_LG_MX), 1, 0, buf, sizeof buf), "+%d cambio más");
        atomic_store(&g_dm_lang, 1); expect("dm_strn without the game: dm_ui in the game language", dm_strn(NULL, "dmImpOver", 1, buf, sizeof buf), "%d Punkt über Grenze"); }
    else { fprintf(stderr, "用法：t_i18n en|zh|none|sysen|syses|systw|sysru|sysko|syspt|tags|game|plural\n"); return 2; }
    return fails ? 1 : 0;
}
