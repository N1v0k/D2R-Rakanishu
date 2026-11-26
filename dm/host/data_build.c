#include "data_build.h"
#include "game_addrs.h"
#include <stdio.h>
#include <string.h>

int dm_data_build_ok(const char *config, char *have, size_t have_n, char *want, size_t want_n) {
    const char *dot = strrchr(GAME_VERSION_main, '.');
    snprintf(want, want_n, "%s", dot ? dot + 1 : GAME_VERSION_main);
    if (have_n) have[0] = 0;
    FILE *f = fopen(config, "r");
    if (!f) return 0;
    char line[256];
    while (fgets(line, sizeof line, f)) {
        char key[64], value[64];
        if (sscanf(line, " %63[^= \t] = %63s", key, value) == 2 && !strcmp(key, "build-name")) { snprintf(have, have_n, "%s", value); break; }
    }
    fclose(f);
    return have[0] && !strcmp(have, want);
}
