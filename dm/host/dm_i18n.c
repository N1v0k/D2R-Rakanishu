







#include "dm_i18n.h"
#include "game_addrs.h"
#include "dm_i18n_tab.h"
#include <TargetConditionals.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

_Atomic int g_dm_lang = -1;

#if TARGET_OS_IPHONE
#include <CoreFoundation/CoreFoundation.h>
#endif








int dm_lang_from_tag(const char *t) {
    if (!t || !*t) return 8;
    char l[4] = { 0 }; int i = 0; while (i < 3 && t[i] && t[i] != '-' && t[i] != '_') { l[i] = (char)(t[i] | 0x20); i++; }
    static const char *const zh[] = { "zh", "yue", "wuu", "hak", "nan", "cmn", "gan", "hsn", "lzh" };
    for (unsigned k = 0; k < sizeof zh / sizeof *zh; k++) if (!strcmp(l, zh[k])) return 8;
    if (!strcmp(l, "de")) return 1;
    if (!strcmp(l, "ko")) return 5;
    if (!strcmp(l, "ru")) return 7;
    if (!strcmp(l, "pt")) return 12;
    if (!strcmp(l, "es")) {
        const char *r = t + i; while (*r == '-' || *r == '_') { r++; size_t n = strcspn(r, "-_"); if (n == 4) { r += n; continue; }
            return (n == 2 && (r[0] | 0x20) == 'e' && (r[1] | 0x20) == 's') ? 2 : 10; }
        return 2; }
    return 0;
}






int dm_sys_lang(void) {
    const char *e = getenv("DM_LANG"); if (e && *e) return dm_lang_from_tag(e);
#if TARGET_OS_IPHONE
    int r = 8; CFArrayRef a = CFLocaleCopyPreferredLanguages();
    if (a) { if (CFArrayGetCount(a) > 0) { char t[64]; if (CFStringGetCString((CFStringRef)CFArrayGetValueAtIndex(a, 0), t, sizeof t, kCFStringEncodingUTF8)) r = dm_lang_from_tag(t); } CFRelease(a); }
    return r;
#else
    return 8;
#endif
}

static int settings_lang(void) {
    const char *sv = getenv("DM_SAVE"); if (!sv) return dm_sys_lang();
    char path[1200]; snprintf(path, sizeof path, "%s/Settings/Settings.json", sv);
    FILE *f = fopen(path, "rb"); if (!f) return dm_sys_lang();
    char buf[8192]; size_t n = fread(buf, 1, sizeof buf - 1, f); fclose(f); buf[n] = 0;
    const char *k = strstr(buf, "\"Language\""); if (!k) return -1;
    k = strchr(k + 10, ':'); return k ? atoi(k + 1) : -1;
}

int dm_lang_index(void) {
    int l = atomic_load(&g_dm_lang); if (l >= 0) return l;
    static _Atomic int file = -2; int f = atomic_load(&file); if (f == -2) { f = settings_lang(); atomic_store(&file, f); }
    return f < 0 ? 8 : f;
}
int dm_lang_zh(void) { int l = dm_lang_index(); return l == 8 || l == 9; }
int dm_lang_group(void) { return dm_lang_group_of(dm_lang_index()); }

const char *dm_ui(const char *key) { return dm_ui_group(key, dm_lang_group()); }
int dm_lang_group_of(int l) { return l == 8 || l == 9 ? DM_LG_ZH : l == 2 ? DM_LG_ES : l == 10 ? DM_LG_MX : l == 7 ? DM_LG_RU : l == 5 ? DM_LG_KO : l == 1 ? DM_LG_DE : l == 12 ? DM_LG_PT : DM_LG_EN; }
const char *dm_ui_group(const char *key, int group) {
    for (unsigned i = 0; i < sizeof k_dm_i18n / sizeof *k_dm_i18n; i++) if (!strcmp(k_dm_i18n[i].k, key)) {
        switch (group) { case DM_LG_ZH: return k_dm_i18n[i].zh; case DM_LG_ES: return k_dm_i18n[i].es; case DM_LG_MX: return k_dm_i18n[i].mx; case DM_LG_RU: return k_dm_i18n[i].ru; case DM_LG_KO: return k_dm_i18n[i].ko; case DM_LG_DE: return k_dm_i18n[i].de; case DM_LG_PT: return *k_dm_i18n[i].pt ? k_dm_i18n[i].pt : k_dm_i18n[i].en; default: return k_dm_i18n[i].en; } }
    return key;
}





const char *dm_untag(const char *s) {
    if (!s) return s;
    for (;;) {
        if (s[0] == '[' && (((s[1] == 'm' || s[1] == 'f') && (s[2] == 's' || s[2] == 'p')) || (s[1] == 'n' && (s[2] == 's' || s[2] == 'l')) || (s[1] == 'p' && s[2] == 'l')) && s[3] == ']') s += 4;
        else if (s[0] == '[' && s[1] == 'p' && s[2] == ']') s += 3;
        else return s;
    }
}

const char *dm_str(uint8_t *b, const char *key) {
    if (!b) return dm_ui(key);
    struct { const char *k; uint64_t n; } s = { key, strlen(key) };
    const char *r = ((const char *(*)(void *))(b + GA_LOC_STRING_GET))(&s);
    return r && *r && strcmp(r, key) ? r : dm_ui(key);
}

const char *dm_strn(uint8_t *b, const char *key, long long n, char *buf, size_t size) { return dm_plural(dm_str(b, key), n, 0, buf, size); }
