



#pragma once
#include <stddef.h>
#include <string.h>

enum { DM_PL_ONE, DM_PL_FEW, DM_PL_MANY, DM_PL_OTHER };

enum { DM_PL_WHOLE, DM_PL_ZERO_DECIMALS, DM_PL_DECIMALS };




static inline int dm_plural_cat(const char *lang, long long n, int dec) {
    if (n < 0) n = -n;
    if (!strcmp(lang, "en") || !strcmp(lang, "de")) return n == 1 && dec == DM_PL_WHOLE ? DM_PL_ONE : DM_PL_OTHER;
    if (!strcmp(lang, "es")) return n == 1 && dec != DM_PL_DECIMALS ? DM_PL_ONE : DM_PL_OTHER;
    if (!strcmp(lang, "pt")) return n <= 1 ? DM_PL_ONE : DM_PL_OTHER;
    if (!strcmp(lang, "ru")) {
        if (dec != DM_PL_WHOLE) return DM_PL_OTHER;
        long long a = n % 10, b = n % 100;
        return a == 1 && b != 11 ? DM_PL_ONE : a >= 2 && a <= 4 && (b < 12 || b > 14) ? DM_PL_FEW : DM_PL_MANY;
    }
    return DM_PL_OTHER;
}


static inline const char *dm_plural_forms(const char *cell, char lang[3]) {
    static const char *const kinds[] = { "one=", "few=", "many=", "other=" };
    if (!cell || !cell[0] || !cell[1] || cell[2] != '|') return NULL;
    const char *f = cell + 3; int ok = 0;
    for (int k = 0; k < 4 && !ok; k++) ok = !strncmp(f, kinds[k], strlen(kinds[k]));
    if (!ok) return NULL;
    lang[0] = cell[0]; lang[1] = cell[1]; lang[2] = 0;
    return f;
}


static inline int dm_plural_dec(const char *num) {
    const char *p = num ? strpbrk(num, ".,") : NULL;
    if (!p) return DM_PL_WHOLE;
    for (p++; *p >= '0' && *p <= '9'; p++) if (*p != '0') return DM_PL_DECIMALS;
    return DM_PL_ZERO_DECIMALS;
}



static inline const char *dm_plural(const char *cell, long long n, int dec, char *buf, size_t size) {
    char lang[3]; const char *f = dm_plural_forms(cell, lang);
    if (!f || !buf || !size) return cell;
    static const char *const kinds[] = { "one=", "few=", "many=", "other=" };
    int want = dm_plural_cat(lang, n, dec);
    const char *hit = NULL, *other = NULL;
    for (const char *p = f; p && *p;) {
        const char *end = strchr(p, '|');
        for (int k = 0; k < 4; k++) { size_t L = strlen(kinds[k]);
            if (!strncmp(p, kinds[k], L)) { if (k == want) hit = p + L; if (k == DM_PL_OTHER) other = p + L; } }
        p = end ? end + 1 : NULL;
    }
    const char *use = hit ? hit : other;
    if (!use) { buf[0] = 0; return buf; }
    const char *end = strchr(use, '|'); size_t L = end ? (size_t)(end - use) : strlen(use);
    if (L >= size) L = size - 1;
    memcpy(buf, use, L); buf[L] = 0;
    return buf;
}
