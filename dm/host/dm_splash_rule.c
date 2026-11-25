#include "dm_splash_rule.h"
#include <math.h>
#include <string.h>

enum { SRV_WHIRLWIND = 76, SRV_SMITE = 150 };

static int is(const char *type, size_t n, const char *code) { return n == strlen(code) && !memcmp(type, code, n); }

dm_splash_group dm_splash_group_of(const char *type, int two_hands) {
    if (!type) return DM_SPLASH_NONE;
    size_t n = 0;
    while (n < 4 && type[n]) n++;
    while (n && type[n - 1] == ' ') n--;
    if (is(type, n, "pole")) return DM_SPLASH_HEAVY;
    if (is(type, n, "axe")) return two_hands ? DM_SPLASH_HEAVY : DM_SPLASH_MEDIUM;
    if (is(type, n, "hamm")) return two_hands ? DM_SPLASH_HEAVY : DM_SPLASH_LIGHT;
    if (is(type, n, "swor")) return two_hands ? DM_SPLASH_MEDIUM : DM_SPLASH_LIGHT;
    static const char *const light[] = { "club", "mace", "scep", "staf", "h2h", "h2h2" };
    for (size_t i = 0; i < sizeof light / sizeof *light; i++) if (is(type, n, light[i])) return DM_SPLASH_LIGHT;
    static const char *const thrust[] = { "spea", "aspe", "jave", "ajav" };
    for (size_t i = 0; i < sizeof thrust / sizeof *thrust; i++) if (is(type, n, thrust[i])) return DM_SPLASH_THRUST;
    static const char *const none[] = { "knif", "wand", "orb", "taxe", "tkni", "tpot", "bow", "abow", "xbow" };
    for (size_t i = 0; i < sizeof none / sizeof *none; i++) if (is(type, n, none[i])) return DM_SPLASH_NONE;
    return DM_SPLASH_UNKNOWN;
}

dm_splash_area dm_splash_area_of(dm_splash_group g, int cls, int str, int dex, int range) {
    static const int value[] = { [DM_SPLASH_HEAVY] = 60, [DM_SPLASH_MEDIUM] = 40, [DM_SPLASH_LIGHT] = 25, [DM_SPLASH_THRUST] = 25 };
    static const int width[] = { [DM_SPLASH_HEAVY] = 45, [DM_SPLASH_MEDIUM] = 35, [DM_SPLASH_LIGHT] = 30, [DM_SPLASH_THRUST] = 20 };
    static const int class_pct[8] = { 85, 0, 50, 85, 100, 85, 85, 50 };
    dm_splash_area a = { 0, 0, 0 };
    if (g != DM_SPLASH_HEAVY && g != DM_SPLASH_MEDIUM && g != DM_SPLASH_LIGHT && g != DM_SPLASH_THRUST) return a;
    if (cls < 0 || cls >= 8) return a;
    int s = str < 75 ? 75 : str > 225 ? 225 : str;
    int share = value[g] * class_pct[cls] * s / (150 * 100) / 5 * 5;
    if (share < 10) return a;
    int extra = dex > 0 ? dex / 4 : 0;
    a.share = share;
    a.width = width[g] + (extra > 45 ? 45 : extra);
    a.depth = range < 0 ? 0 : range > 2 ? 2 : range;
    return a;
}




#define SPLASH_BODY 1.0
#define SPLASH_ROUND 0.5

double dm_splash_cone_factor(const dm_splash_area *a, int ax, int ay, int px, int py, int nx, int ny) {
    double dx = px - ax, dy = py - ay, vx = nx - ax, vy = ny - ay;
    double aa = sqrt(dx * dx + dy * dy), na = sqrt(vx * vx + vy * vy);
    if (aa == 0 || na == 0 || a->width <= 0) return 0;
    double along = (vx * dx + vy * dy) / aa, side = fabs(vx * dy - vy * dx) / aa;
    double half = a->width * 3.14159265358979323846 / 360.0;
    if (along < 1e-9) return 0;
    if (along > aa + a->depth + SPLASH_ROUND) return 0;
    if (side > along * tan(half) + SPLASH_BODY + 1e-9) return 0;
    double angle = atan2(side, along);
    return 1 - 0.5 * (angle > half ? 1 : angle / half);
}

double dm_splash_reach(const dm_splash_area *a, int extra) {
    if (!a->share || extra <= 0) return 1;
    double n = a->width / 45.0 + a->depth / 2.0;
    return 1 + (n > extra ? extra : n) * 0.75 * a->share / 100.0;
}

int dm_splash_skill_excluded(int kick, int srvdofunc) { return kick != 0 || srvdofunc == SRV_WHIRLWIND || srvdofunc == SRV_SMITE; }
