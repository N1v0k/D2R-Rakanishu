#include "../data_build.h"
#include "../game_addrs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
static int fails;
static void check(int ok, const char *what) { if (!ok) { fprintf(stderr, "FAIL: %s\n", what); fails++; } }
static void put(const char *path, const char *text) { FILE *f = fopen(path, "w"); if (!f) exit(2); fputs(text, f); fclose(f); }
int main(void) {
    char dir[] = "/tmp/dm-data-build.XXXXXX", cfg[256], have[64], want[64], text[512];
    if (!mkdtemp(dir)) return 2;
    snprintf(cfg, sizeof cfg, "%s/.build.config", dir);
    const char *build = strrchr(GAME_VERSION_main, '.') + 1;
    snprintf(text, sizeof text, "# Static Build Configuration\n\nroot = 00\nbuild-name = %s\nbuild-uid = x\n", build); put(cfg, text);
    check(dm_data_build_ok(cfg, have, sizeof have, want, sizeof want) && !strcmp(want, build) && !strcmp(have, build), "the table's build is accepted");
    put(cfg, "build-name = 12345\nbuild-uid = x\n");
    check(!dm_data_build_ok(cfg, have, sizeof have, want, sizeof want) && !strcmp(have, "12345") && !strcmp(want, build), "another build is refused and named");
    snprintf(text, sizeof text, "build-name-old = %s\nbuild-uid = x\n", build); put(cfg, text);
    check(!dm_data_build_ok(cfg, have, sizeof have, want, sizeof want) && !have[0], "a file without the build line is refused");
    unlink(cfg);
    check(!dm_data_build_ok(cfg, have, sizeof have, want, sizeof want) && !have[0], "a missing file is refused");
    rmdir(dir);
    if (!fails) printf("t_data_build: ok (build %s)\n", build);
    return fails != 0;
}
