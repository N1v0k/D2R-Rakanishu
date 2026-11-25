







#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *const k_head[] = {
    "Al", "Ar", "Bel", "Bran", "Cael", "Cor", "Dar", "Dra", "El", "Eri", "Fen", "Gal", "Gor", "Hal", "Is", "Jor",
    "Kal", "Kor", "Lor", "Ma", "Mor", "Nar", "Ny", "Or", "Ra", "Rho", "Sar", "Sha", "Tal", "Thar", "Ul", "Val",
    "Vex", "Wy", "Xan", "Yr", "Zar", "Ze",
};
static const char *const k_mid[] = { "", "", "", "a", "e", "i", "o", "u", "an", "en", "ri", "la", "mo", "da", "ve", "th", "go", "ka" };
static const char *const k_tail[] = {
    "ric", "dor", "wyn", "eth", "ias", "mir", "nar", "ros", "ven", "dra", "lia", "ra", "na", "us", "is", "on",
    "gar", "thas", "mund", "ane", "iel", "ok", "ash", "ius", "ard", "ex", "ys", "orn", "ald", "ica",
};
#define N(a) (sizeof(a) / sizeof(a)[0])

int dm_name_gen(char *out, int cap, int (*taken)(const char *)) {
    for (int k = 0; k < 64; k++) {
        snprintf(out, (size_t)cap, "%s%s%s", k_head[arc4random_uniform(N(k_head))], k_mid[arc4random_uniform(N(k_mid))], k_tail[arc4random_uniform(N(k_tail))]);
        if (!taken || !taken(out)) return 1;
    }
    return 0;
}
