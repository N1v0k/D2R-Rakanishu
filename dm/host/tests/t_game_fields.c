

#include "../dm.h"
#include "../game_addrs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
module_t g_mod[MAX_MOD];
int g_nmod;
void dm_port_assume(const char *out_dir);
static int fails;
static void check(int ok, const char *what) { if (!ok) { fprintf(stderr, "FAIL: %s\n", what); fails++; } }
static const struct { const char *name; uint32_t off, witness; } want[] = {
#define GB(module, version, build_id)
#define GL(module, name, value)
#define GA(module, name, addr, kind, note)
#define GF(module, name, off, witness, note) {#name, off, witness},
#include "../game_addrs.def"
#undef GB
#undef GL
#undef GA
#undef GF
    {NULL, 0, 0}};
static uint32_t encode(uint32_t off) {
    return off < 0x1000 ? 0x91000020 | off << 10 : 0x52800000 | (off & 0xffff) << 5;
}
static char *run(const char *dir) {
    char path[256]; snprintf(path, sizeof path, "%s/log", dir);
    fflush(stderr); int keep = dup(2); if (!freopen(path, "w", stderr)) exit(2);
    dm_port_assume(dir);
    fflush(stderr); dup2(keep, 2); close(keep);
    static char text[65536]; FILE *f = fopen(path, "r"); size_t n = f ? fread(text, 1, sizeof text - 1, f) : 0; text[n] = 0; if (f) fclose(f);
    return text;
}
int main(void) {
    char dir[] = "/tmp/dm-game-fields.XXXXXX", path[256], line[128];
    if (!mkdtemp(dir)) return 2;
    uint8_t *img = calloc(1, GL_TEXT_END);
    strcpy(g_mod[0].name, "main"); g_mod[0].base = img; g_nmod = 1;
    int n = 0;
    for (int i = 0; want[i].name; i++, n++) { uint32_t w = encode(want[i].off); memcpy(img + want[i].witness, &w, 4); }
    snprintf(path, sizeof path, "%s/main.cuts.tsv", dir);
    FILE *f = fopen(path, "w"); fprintf(f, "# BUILD_ID\t%s\n", GAME_BUILD_ID_main); fclose(f);
    snprintf(line, sizeof line, "%d field witnesses checked, 0 differ", n);
    char *log = run(dir);
    check(strstr(log, line) && !strstr(log, "ASSUME"), "the table's witnesses and build id pass");
    f = fopen(path, "w"); fprintf(f, "# BUILD_ID\t00000000000000000000000000000000\n"); fclose(f);
    check(strstr(run(dir), "ASSUME port.build_id: the program's build id is 00000000000000000000000000000000") != NULL, "another build id is named");
    f = fopen(path, "w"); fprintf(f, "# BUILD_ID\t%s\n", GAME_BUILD_ID_main); fclose(f);
    if (n) {
        uint32_t w = encode(want[0].off + 8); memcpy(img + want[0].witness, &w, 4);
        snprintf(line, sizeof line, "ASSUME port.field: %s:", want[0].name);
        check(strstr(run(dir), line) != NULL, "a witness that encodes another offset is named");
    }
    unlink(path); snprintf(path, sizeof path, "%s/log", dir); unlink(path); rmdir(dir);
    if (!fails) printf("t_game_fields: ok (%d fields)\n", n);
    return fails != 0;
}
