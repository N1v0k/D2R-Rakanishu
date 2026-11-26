



#include "game_addrs.h"
#include "dm.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>


static int insn_imm(uint32_t w, int64_t *v) {
    if ((w & 0x3b000000) == 0x39000000) {
        unsigned size = w >> 30; if ((w >> 26 & 1) && (w >> 23 & 1)) size = 4;
        *v = (int64_t)(w >> 10 & 0xfff) << size; return 1; }
    if ((w & 0x3a000000) == 0x28000000 && (w >> 23 & 3)) {
        unsigned opc = w >> 30, scale = (w >> 26 & 1) ? 2 + opc : (opc == 2 ? 3 : 2); int64_t imm = w >> 15 & 0x7f;
        *v = (imm & 0x40 ? imm - 0x80 : imm) * ((int64_t)1 << scale); return 1; }
    if ((w & 0x3b200c00) == 0x38000000) { int64_t imm = w >> 12 & 0x1ff; *v = imm & 0x100 ? imm - 0x200 : imm; return 1; }
    if ((w & 0x1f800000) == 0x11000000) { *v = (int64_t)(w >> 10 & 0xfff) << (w >> 22 & 1 ? 12 : 0); return 1; }
    if ((w & 0x7f800000) == 0x52800000) { *v = (int64_t)(w >> 5 & 0xffff) << (16 * (w >> 21 & 3)); return 1; }
    return 0;
}

static const struct { const char *name; uint32_t off, witness; } fields[] = {
#define GB(module, version, build_id)
#define GL(module, name, value)
#define GA(module, name, addr, kind, note)
#define GF(module, name, off, witness, note) {#name, off, witness},
#include "game_addrs.def"
#undef GB
#undef GL
#undef GA
#undef GF
    {NULL, 0, 0}};

void dm_port_assume(const char *out_dir) {
    char path[1100], line[256], id[64] = ""; snprintf(path, sizeof path, "%s/main.cuts.tsv", out_dir);
    FILE *f = fopen(path, "r");
    if (f) { if (fgets(line, sizeof line, f) && !strncmp(line, "# BUILD_ID", 10)) sscanf(line + 10, " %63s", id); fclose(f); }
    if (strcmp(id, GAME_BUILD_ID_main)) LOG("ASSUME port.build_id: the program's build id is %s, the address table is for %s (%s)", *id ? id : "(none)", GAME_BUILD_ID_main, GAME_VERSION_main);
    const uint8_t *b = NULL;
    for (int i = 0; i < g_nmod; i++) if (!strcmp(g_mod[i].name, "main")) b = (const uint8_t *)g_mod[i].base;
    if (!b) return;
    int n = 0, bad = 0;
    for (int i = 0; fields[i].name; i++, n++) {
        uint32_t w; memcpy(&w, b + fields[i].witness, 4); int64_t v;
        if (!insn_imm(w, &v) || v != (int64_t)fields[i].off) {
            bad++; LOG("ASSUME port.field: %s: the witness at main+%#x (%08x) does not encode %#x", fields[i].name, fields[i].witness, w, fields[i].off); }
    }
    LOG("boot: address table %s, %d field witnesses checked, %d differ", GAME_VERSION_main, n, bad);
}
