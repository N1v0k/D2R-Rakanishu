


#define _GNU_SOURCE
#include "../dump_reader.h"
#include "../rd_crypto.h"
#include "../romfs_validate.h"
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

static int fails;
static void check(int ok, const char *what) {
    if (!ok) { fprintf(stderr, "FAIL: %s\n", what); fails++; }
}
static void check_err(const pb_error *e, const char *id, const char *detail, const char *what) {
    int ok = !strcmp(e->id, id) && (!detail || !strcmp(e->detail, detail));
    if (!ok) fprintf(stderr, "FAIL: %s: got %s %s, want %s %s\n", what, e->id, e->detail, id, detail ? detail : "*");
    fails += !ok;
}

static const char *fx;
static char tmp[4096];

static char *path(const char *dir, const char *name) {
    static char p[8][4200];
    static int k;
    char *o = p[k++ & 7];
    snprintf(o, sizeof p[0], "%s/%s", dir, name);
    return o;
}
static uint8_t *slurp(const char *p, size_t *n) {
    FILE *f = fopen(p, "rb");
    if (!f) { fprintf(stderr, "cannot read %s\n", p); exit(2); }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    rewind(f);
    uint8_t *b = malloc(sz + 1);
    if (fread(b, 1, sz, f) != (size_t)sz) exit(2);
    fclose(f);
    b[sz] = 0;
    *n = (size_t)sz;
    return b;
}
static void spill(const char *p, const void *b, size_t n) {
    FILE *f = fopen(p, "wb");
    if (!f || fwrite(b, 1, n, f) != n || fclose(f)) exit(2);
}
static int open_ro(const char *p) {
    int fd = open(p, O_RDONLY);
    if (fd < 0) { fprintf(stderr, "cannot open %s\n", p); exit(2); }
    return fd;
}
static uint64_t fsize(int fd) { struct stat sb; fstat(fd, &sb); return (uint64_t)sb.st_size; }
static void rmrf(const char *d) { char c[4300]; snprintf(c, sizeof c, "rm -rf '%s'", d); if (system(c)) exit(2); }
static void fresh(const char *d) { rmrf(d); if (mkdir(d, 0755)) exit(2); }
static int count_entries(const char *d, const char *suffix) {
    DIR *dir = opendir(d);
    int n = 0;
    struct dirent *de;
    if (!dir) return -1;
    while ((de = readdir(dir))) {
        size_t l = strlen(de->d_name), s = strlen(suffix);
        if (strcmp(de->d_name, ".") && strcmp(de->d_name, "..") && l >= s && !strcmp(de->d_name + l - s, suffix)) n++;
    }
    closedir(dir);
    return n;
}
static void file_hex(const char *p, char out[65]) {
    size_t n;
    uint8_t *b = slurp(p, &n), d[32];
    rd_sha256(b, n, d);
    for (int i = 0; i < 32; i++) sprintf(out + 2 * i, "%02x", d[i]);
    free(b);
}

static uint8_t *keys;
static size_t keys_n;
static pb_contract fixture;



static int probe(const char *file, const uint8_t *k, size_t kn, const pb_contract *c, int *kind, char id[65], pb_error *e) {
    int fd = open_ro(file);
    int r = pb_probe(fd, fsize(fd), k, kn, c, kind, id, e);
    close(fd);
    return r;
}


static char *keys_with(const char *name, const char *line) {
    char *out = calloc(1, keys_n + 256), *src = strndup((const char *)keys, keys_n), *save = NULL;
    for (char *l = strtok_r(src, "\n", &save); l; l = strtok_r(NULL, "\n", &save)) {
        if (!strncmp(l, name, strlen(name)) && l[strlen(name)] == ' ') { strcat(out, line); if (*line) strcat(out, "\n"); }
        else { strcat(out, l); strcat(out, "\n"); }
    }
    free(src);
    return out;
}

static void test_probe(void) {
    pb_error e = {0};
    int kind = -1;
    char id[65], id2[65], idu[65];
    check(probe(path(fx, "keys.txt"), NULL, 0, &fixture, &kind, id, &e) == 0 && kind == PB_KIND_KEYS && !id[0], "probe: key file");
    check(probe(path(fx, "base.nsp"), NULL, 0, &fixture, &kind, id, &e) == 0 && kind == PB_KIND_PACKAGE && strlen(id) == 64, "probe: package without keys");
    check(probe(path(fx, "base.nsp"), keys, keys_n, &fixture, &kind, id2, &e) == 0 && kind == PB_KIND_BASE, "probe: base");
    check(!strcmp(id, id2), "probe: identity does not depend on the keys");
    check(probe(path(fx, "base.nsp"), keys, keys_n, &fixture, &kind, id2, &e) == 0 && !strcmp(id, id2), "probe: identity stable");
    check(probe(path(fx, "update.nsp"), keys, keys_n, &fixture, &kind, idu, &e) == 0 && kind == PB_KIND_UPDATE && strcmp(idu, id), "probe: update");
    check(probe(path(fx, "update.nsp"), keys, keys_n, pb_contract_game(), &kind, id, &e) == -1, "probe: another title");
    check_err(&e, "PB-NOTGAME", "title", "probe: another title");
    check(probe(path(fx, "addon.nsp"), NULL, 0, &fixture, &kind, id, &e) == 0 && kind == PB_KIND_PACKAGE && strlen(id) == 64, "probe: an add-on without keys is only a package");
    check(probe(path(fx, "addon.nsp"), keys, keys_n, &fixture, &kind, id, &e) == 0 && kind == PB_KIND_ADDON, "probe: add-on with keys (content ids)");
    check(probe(path(fx, "base-addon.nsp"), NULL, 0, &fixture, &kind, id, &e) == 0 && kind == PB_KIND_PACKAGE, "probe: a ticketless base bundled with an add-on, without keys: a package");
    check(probe(path(fx, "base-addon.nsp"), keys, keys_n, &fixture, &kind, id, &e) == 0 && kind == PB_KIND_BASE, "probe: the same with keys: the base (its program makes it no add-on)");
    check(probe(path(fx, "addon.nsp"), NULL, 0, pb_contract_game(), &kind, id, &e) == 0 && kind == PB_KIND_PACKAGE, "probe: another title's add-on is a package");
    check(probe(path(fx, "addon.nsp"), keys, keys_n, pb_contract_game(), &kind, id, &e) == -1, "probe: another title's add-on with keys");
    check_err(&e, "PB-NOTGAME", "no program", "probe: another title's add-on with keys");
    check(probe(path(fx, "base.nsp"), NULL, 0, &fixture, &kind, id, &e) == 0 && kind == PB_KIND_PACKAGE, "probe: the base's own ticket is no add-on");

    char *k = keys_with("header_key", "header_key = 00112233445566778899aabbccddeeff00112233445566778899aabbccddeeff");
    check(probe(path(fx, "update.nsp"), (uint8_t *)k, strlen(k), &fixture, &kind, id, &e) == -1, "probe: wrong header_key");
    check_err(&e, "PB-KEYWRONG", "header_key", "probe: wrong header_key");
    free(k);
    k = keys_with("titlekek_0a", "");
    check(probe(path(fx, "update.nsp"), (uint8_t *)k, strlen(k), &fixture, &kind, id, &e) == -1, "probe: titlekek missing");
    check_err(&e, "PB-KEYMISSING", "titlekek_0a", "probe: titlekek missing");
    free(k);
    k = keys_with("titlekek_0a", "TitleKek_0A = 000102030405060708090a0b0c0d0e0f");
    check(probe(path(fx, "base.nsp"), (uint8_t *)k, strlen(k), &fixture, &kind, id, &e) == -1, "probe: wrong titlekek value");
    check_err(&e, "PB-KEYWRONG", "titlekek_0a", "probe: wrong titlekek value (name case-insensitive)");
    free(k);
    k = keys_with("titlekek_0a", "titlekek_0a = 0001020304");
    check(probe(path(fx, "base.nsp"), (uint8_t *)k, strlen(k), &fixture, &kind, id, &e) == -1, "probe: titlekek of wrong length");
    check_err(&e, "PB-KEYWRONG", "titlekek_0a", "probe: titlekek of wrong length");
    free(k);
    k = keys_with("header_key", "");
    spill(path(tmp, "nohdr.txt"), k, strlen(k));
    check(probe(path(tmp, "nohdr.txt"), NULL, 0, &fixture, &kind, id, &e) == -1, "probe: key file without header_key");
    check_err(&e, "PB-KEYMISSING", "header_key", "probe: key file without header_key");
    free(k);
    spill(path(tmp, "text.txt"), "hello\nworld\n", 12);
    check(probe(path(tmp, "text.txt"), NULL, 0, &fixture, &kind, id, &e) == -1, "probe: plain text");
    check_err(&e, "PB-NOTGAME", "file", "probe: plain text");
    uint8_t bin[64] = {1, 2, 0, 3};
    spill(path(tmp, "bin.dat"), bin, sizeof bin);
    check(probe(path(tmp, "bin.dat"), NULL, 0, &fixture, &kind, id, &e) == -1, "probe: binary");
    check_err(&e, "PB-NOTGAME", "file", "probe: binary");
    spill(path(tmp, "empty"), "", 0);
    check(probe(path(tmp, "empty"), NULL, 0, &fixture, &kind, id, &e) == -1, "probe: empty file");
    check_err(&e, "PB-NOTGAME", "file", "probe: empty file");
    int fd = open_ro(path(fx, "base.nsp"));
    check(pb_probe(fd, fsize(fd) - 1, NULL, 0, &fixture, &kind, id, &e) == -1, "probe: length differs from the descriptor");
    check_err(&e, "PB-SOURCE", NULL, "probe: length differs from the descriptor");
    close(fd);
    check(probe(path(fx, "base.nsp"), (const uint8_t *)"x = 1\n", 6, &fixture, &kind, id, &e) == -1, "probe: package with keys lacking header_key");
    check_err(&e, "PB-KEYMISSING", "header_key", "probe: package with keys lacking header_key");
}



static void test_contract(void) {
    const pb_contract *g = pb_contract_game();
    check(g->n == 27 + DM_ROMFS_VOICE_N && DM_ROMFS_VOICE_N == 22 && g->title == 0x0100726014352000ULL, "game contract: 27 files and the 22 voice archives of the game title");
    for (size_t i = 0; i < DM_ROMFS_VOICE_N; i++)
        check(g->files[27 + i].size == (uint64_t)dm_romfs_voice_size[i] && !strcmp(g->files[27 + i].name, dm_romfs_voice[i]) && !strcmp(g->files[27 + i].unit, dm_romfs_voice[i])
              && strlen(g->files[27 + i].sha256) == 64, "game contract: voice archives, one unit each, sizes from romfs_validate.h");
    check(!strcmp(g->files[0].unit, "exefs") && !strcmp(g->files[3].unit, "meta") && !strcmp(g->files[9].unit, dm_romfs_need[0]), "game contract: unit order");
    for (size_t i = 0; i < DM_ROMFS_NEED_N; i++)
        check(g->files[9 + i].size == (uint64_t)dm_romfs_size[i] && !strcmp(g->files[9 + i].name, dm_romfs_need[i]), "game contract: archive sizes from romfs_validate.h");
    size_t n;
    char *sh = (char *)slurp("../../../setup.sh", &n), want[80];
    snprintf(want, sizeof want, "%s main", g->files[0].sha256);
    check(strstr(sh, want) != NULL, "game contract: main digest equals the version check of setup.sh");
    free(sh);

    check(fixture.n == 27 && fixture.title == 0x0100c0de00105000ULL, "fixture contract parsed");
    static const char *bad[] = {
        "title\t0100c0de00105000\n",
        "title\t0100c0de0010500\nfile\tu\texefs\tmain\t1\t" "aa" "\n",
        "title\t0100c0de00105000\nfile\tu\texefs\tmain\t1\t52c2dd2f3f95b9d6b092b6e3b7420f9fcbb3a1e6cc56215ac4df526c9f00a24f",
        "title\t0100c0de00105000\nfiles\tu\texefs\tmain\t1\t52c2dd2f3f95b9d6b092b6e3b7420f9fcbb3a1e6cc56215ac4df526c9f00a24f\n",
        "title\t0100c0de00105000\nfile\tu\texefs\tmain\t1\t52C2DD2F3F95B9D6B092B6E3B7420F9FCBB3A1E6CC56215AC4DF526C9F00A24F\n",
        "title\t0100c0de00105000\nfile\tu\texefs\tmain\t1\t52c2dd2f3f95b9d6b092b6e3b7420f9fcbb3a1e6cc56215ac4df526c9f00a24\n",
        "title\t0100c0de00105000\nfile\tu\tother\tmain\t1\t52c2dd2f3f95b9d6b092b6e3b7420f9fcbb3a1e6cc56215ac4df526c9f00a24f\n",
        "title\t0100c0de00105000\nfile\tu\tdata\ta/b\t1\t52c2dd2f3f95b9d6b092b6e3b7420f9fcbb3a1e6cc56215ac4df526c9f00a24f\n",
        "title\t0100c0de00105000\nfile\tu\tdata\t..\t1\t52c2dd2f3f95b9d6b092b6e3b7420f9fcbb3a1e6cc56215ac4df526c9f00a24f\n",
        "title\t0100c0de00105000\nfile\tu\tdata\tx\t0\t52c2dd2f3f95b9d6b092b6e3b7420f9fcbb3a1e6cc56215ac4df526c9f00a24f\n",
        "title\t0100c0de00105000\nfile\tu\tdata\tx\t+5\t52c2dd2f3f95b9d6b092b6e3b7420f9fcbb3a1e6cc56215ac4df526c9f00a24f\n",
        "title\t0100c0de00105000\nfile\tu\tdata\tx\t5\t52c2dd2f3f95b9d6b092b6e3b7420f9fcbb3a1e6cc56215ac4df526c9f00a24f\tz\n",
        "title\t0100c0de00105000\n\nfile\tu\tdata\tx\t5\t52c2dd2f3f95b9d6b092b6e3b7420f9fcbb3a1e6cc56215ac4df526c9f00a24f\n",
        "title\t0100c0de00105000\nfile\tu\tdata\tx\t5\t52c2dd2f3f95b9d6b092b6e3b7420f9fcbb3a1e6cc56215ac4df526c9f00a24f\n"
        "file\tv\tdata\tx\t5\t52c2dd2f3f95b9d6b092b6e3b7420f9fcbb3a1e6cc56215ac4df526c9f00a24f\n",
        "title\t0100c0de00105000\nfile\tu\tdata\tx\t5\t52c2dd2f3f95b9d6b092b6e3b7420f9fcbb3a1e6cc56215ac4df526c9f00a24f\n"
        "file\tv\tdata\ty\t5\t52c2dd2f3f95b9d6b092b6e3b7420f9fcbb3a1e6cc56215ac4df526c9f00a24f\n"
        "file\tu\tdata\tz\t5\t52c2dd2f3f95b9d6b092b6e3b7420f9fcbb3a1e6cc56215ac4df526c9f00a24f\n",
    };
    for (size_t i = 0; i < sizeof bad / sizeof *bad; i++) {
        pb_contract c;
        pb_error e = {0};
        char what[64];
        snprintf(what, sizeof what, "contract parser refuses case %zu", i);
        check(pb_contract_parse(bad[i], strlen(bad[i]), &c, &e) == -1 && !strcmp(e.id, "PB-IO") && c.files == NULL, what);
    }
    const char *ok = "title\t0100c0de00105000\nfile\tu\texefs\tmain\t1\t52c2dd2f3f95b9d6b092b6e3b7420f9fcbb3a1e6cc56215ac4df526c9f00a24f\n"
                     "file\tu\tdata\tmain\t2\t52c2dd2f3f95b9d6b092b6e3b7420f9fcbb3a1e6cc56215ac4df526c9f00a24f\n";
    pb_contract c;
    pb_error e = {0};
    check(pb_contract_parse(ok, strlen(ok), &c, &e) == 0 && c.n == 2 && c.files[1].size == 2, "contract parser: same name in two folders");
    pb_contract_free(&c);
}



typedef struct { int calls, starts; uint64_t last; char first[64], names[1024]; int cancel_after; pb_reader *r; } progress_t;
static void on_progress(void *ctx, uint64_t bytes, const char *file) {
    progress_t *p = ctx;
    if (bytes < p->last) p->calls = -1000000;
    p->last = bytes;
    if (p->calls >= 0) p->calls++;
    size_t l = strlen(p->names);
    const char *tail = l ? strrchr(p->names, ' ') : NULL;
    if (!l || strcmp(tail ? tail + 1 : p->names, file)) {
        snprintf(p->names + l, sizeof p->names - l, "%s%s", l ? " " : "", file);
        p->starts++;
    }
    if (p->cancel_after && p->calls == p->cancel_after) pb_cancel(p->r);
}

static pb_reader *open_pair(const char *base, const char *upd, const pb_contract *c, int *fds, pb_error *e) {
    fds[0] = base ? open_ro(base) : -1;
    fds[1] = upd ? open_ro(upd) : -1;
    return pb_open(fds[0], fds[1], keys, keys_n, c, e);
}
static void close_pair(pb_reader *r, int *fds) {
    pb_close(r);
    if (fds[0] >= 0) close(fds[0]);
    if (fds[1] >= 0) close(fds[1]);
}

static void test_extract(void) {
    char ex[4200], da[4200];
    snprintf(ex, sizeof ex, "%s/exefs", tmp);
    snprintf(da, sizeof da, "%s/data", tmp);
    fresh(ex);
    fresh(da);
    int fds[2];
    pb_error e = {0};

    pb_reader *r = open_pair(path(fx, "update.nsp"), path(fx, "base.nsp"), &fixture, fds, &e);
    check(r != NULL, "open the fixture");
    if (!r) { fprintf(stderr, "  %s %s\n", e.id, e.detail); return; }
    const char *unit = "";
    int units = 0;
    for (size_t i = 0; i < fixture.n; i++) {
        if (!strcmp(unit, fixture.files[i].unit)) continue;
        unit = fixture.files[i].unit;
        progress_t p = {0};
        int rc = pb_extract(r, unit, ex, da, on_progress, &p, &e);
        if (rc) fprintf(stderr, "  unit %s: %s %s\n", unit, e.id, e.detail);
        check(rc == 0 && p.calls > 0, "extract a unit");
        size_t files = 0;
        uint64_t bytes = 0;
        for (size_t j = 0; j < fixture.n; j++) if (!strcmp(fixture.files[j].unit, unit)) { files++; bytes += fixture.files[j].size; }
        check((size_t)p.starts == files, "progress names every file of the unit");
        check(p.last <= bytes, "progress stays within the unit's bytes");
        units++;
    }
    check(units == 20, "20 units in the fixture contract");
    for (size_t i = 0; i < fixture.n; i++) {
        char hx[65];
        file_hex(path(fixture.files[i].target == PB_TARGET_EXEFS ? ex : da, fixture.files[i].name), hx);
        check(!strcmp(hx, fixture.files[i].sha256), fixture.files[i].name);
    }
    check(count_entries(ex, ".part") == 0 && count_entries(da, ".part") == 0, "no .part left after a full extraction");
    check(count_entries(ex, "") == 3 && count_entries(da, "") == 24, "exactly the contract files written");
    check(pb_extract(r, "nope", ex, da, NULL, NULL, &e) == -1, "unknown unit");
    check_err(&e, "PB-IO", "unit nope", "unknown unit");
    check(pb_extract(r, "exefs", path(tmp, "missing"), da, NULL, NULL, &e) == -1, "missing output folder");
    check_err(&e, "PB-IO", "output folder", "missing output folder");


    pb_cancel(r);
    check(pb_extract(r, "meta", ex, da, NULL, NULL, &e) == -1, "cancel before a unit");
    check_err(&e, "PB-CANCEL", NULL, "cancel before a unit");
    close_pair(r, fds);
    r = open_pair(path(fx, "base.nsp"), path(fx, "update.nsp"), &fixture, fds, &e);
    fresh(da);
    progress_t p = {0};
    p.cancel_after = 2;
    p.r = r;
    check(r && pb_extract(r, "meta", ex, da, on_progress, &p, &e) == -1, "cancel inside a unit");
    check_err(&e, "PB-CANCEL", NULL, "cancel inside a unit");
    check(count_entries(da, ".part") == 0, "no .part left after a cancel");
    close_pair(r, fds);


    r = open_pair(path(fx, "base.nsp"), NULL, &fixture, fds, &e);
    check(!r, "only the base");
    check_err(&e, "PB-NOUPDATE", NULL, "only the base");
    close_pair(r, fds);
    r = open_pair(NULL, path(fx, "update.nsp"), &fixture, fds, &e);
    check(!r, "only the update");
    check_err(&e, "PB-NOBASE", NULL, "only the update");
    close_pair(r, fds);
    fds[0] = open_ro(path(fx, "base.nsp"));
    r = pb_open(fds[0], fds[0], keys, keys_n, &fixture, &e);
    check(!r, "the base twice");
    check_err(&e, "PB-NOUPDATE", NULL, "the base twice");
    close(fds[0]);


    char *k = keys_with("header_key", "header_key = 00112233445566778899aabbccddeeff00112233445566778899aabbccddeeff");
    fds[0] = open_ro(path(fx, "base.nsp"));
    fds[1] = open_ro(path(fx, "update.nsp"));
    r = pb_open(fds[0], fds[1], (uint8_t *)k, strlen(k), &fixture, &e);
    check(!r, "open with a wrong header_key");
    check_err(&e, "PB-KEYWRONG", "header_key", "open with a wrong header_key");
    free(k);
    k = keys_with("titlekek_0a", "");
    r = pb_open(fds[0], fds[1], (uint8_t *)k, strlen(k), &fixture, &e);
    check(!r, "open without titlekek");
    check_err(&e, "PB-KEYMISSING", "titlekek_0a", "open without titlekek");
    free(k);
    close(fds[0]);
    close(fds[1]);
}


static void contract_changed(const char *name, int field, const char *value, pb_contract *c) {
    size_t n;
    char *t = (char *)slurp(path(fx, "contract.tsv"), &n), *out = calloc(1, n + 256), *save = NULL;
    for (char *l = strtok_r(t, "\n", &save); l; l = strtok_r(NULL, "\n", &save)) {
        char *f[6], *s2 = NULL, copy[512];
        snprintf(copy, sizeof copy, "%s", l);
        int nf = 0;
        for (char *x = strtok_r(copy, "\t", &s2); x && nf < 6; x = strtok_r(NULL, "\t", &s2)) f[nf++] = x;
        if (nf == 6 && !strcmp(f[3], name)) {
            f[field] = (char *)value;
            sprintf(out + strlen(out), "%s\t%s\t%s\t%s\t%s\t%s\n", f[0], f[1], f[2], f[3], f[4], f[5]);
        } else sprintf(out + strlen(out), "%s\n", l);
    }
    pb_error e;
    if (pb_contract_parse(out, strlen(out), c, &e)) { fprintf(stderr, "changed contract does not parse\n"); exit(2); }
    free(t);
    free(out);
}

static void test_wrong_contract(void) {
    char ex[4200], da[4200];
    snprintf(ex, sizeof ex, "%s/exefs", tmp);
    snprintf(da, sizeof da, "%s/data", tmp);
    pb_contract c;
    pb_error e = {0};
    int fds[2];
    contract_changed("main", 5, "0000000000000000000000000000000000000000000000000000000000000000", &c);
    pb_reader *r = open_pair(path(fx, "base.nsp"), path(fx, "update.nsp"), &c, fds, &e);
    fresh(ex);
    check(r && pb_extract(r, "exefs", ex, da, NULL, NULL, &e) == -1, "wrong main digest");
    check_err(&e, "PB-VERSION", "main", "wrong main digest");
    check(count_entries(ex, "") == 0, "nothing left of the refused main");
    close_pair(r, fds);
    pb_contract_free(&c);
    contract_changed("03-00000001.data", 5, "1111111111111111111111111111111111111111111111111111111111111111", &c);
    r = open_pair(path(fx, "base.nsp"), path(fx, "update.nsp"), &c, fds, &e);
    fresh(da);
    check(r && pb_extract(r, "03-00000001.data", ex, da, NULL, NULL, &e) == -1, "wrong archive digest");
    check_err(&e, "PB-DAMAGED", "03-00000001.data", "wrong archive digest");
    check(count_entries(da, "") == 0, "nothing left of the refused archive");
    close_pair(r, fds);
    pb_contract_free(&c);
    contract_changed("sdk", 4, "97330", &c);
    r = open_pair(path(fx, "base.nsp"), path(fx, "update.nsp"), &c, fds, &e);
    check(!r, "wrong ExeFS size");
    check_err(&e, "PB-VERSION", "sdk", "wrong ExeFS size");
    close_pair(r, fds);
    pb_contract_free(&c);
    contract_changed("build_key.txt", 4, "33", &c);
    r = open_pair(path(fx, "base.nsp"), path(fx, "update.nsp"), &c, fds, &e);
    check(!r, "wrong data size");
    check_err(&e, "PB-DAMAGED", "build_key.txt", "wrong data size");
    close_pair(r, fds);
    pb_contract_free(&c);
    contract_changed("02-00000002.data", 3, "02-00000009.data", &c);
    r = open_pair(path(fx, "base.nsp"), path(fx, "update.nsp"), &c, fds, &e);
    check(!r, "a declared file the dump lacks");
    check_err(&e, "PB-NOTGAME", "02-00000009.data", "a declared file the dump lacks");
    close_pair(r, fds);
    pb_contract_free(&c);
}



static void test_inplace(void) {
    char ex[4200], da[4200], in[4200];
    snprintf(ex, sizeof ex, "%s/exefs", tmp);
    snprintf(da, sizeof da, "%s/data", tmp);
    snprintf(in, sizeof in, "%s/in", tmp);
    fresh(ex); fresh(da); fresh(in);
    size_t nb, nu;
    uint8_t *b0 = slurp(path(fx, "base.nsp"), &nb), *u0 = slurp(path(fx, "update.nsp"), &nu);
    struct { const char *what, *link_dir, *link_name, *unit, *file; int base; } v[] = {
        {"base package at the .part of main", ex, "main.part", "exefs", "main", 1},
        {"update package at the final build_key.txt", da, "build_key.txt", "meta", "build_key.txt", 0},
    };
    for (size_t i = 0; i < sizeof v / sizeof *v; i++) {
        char copy_b[4300], copy_u[4300];
        snprintf(copy_b, sizeof copy_b, "%s/base.nsp", in);
        snprintf(copy_u, sizeof copy_u, "%s/update.nsp", in);
        FILE *f = fopen(copy_b, "wb"); fwrite(b0, 1, nb, f); fclose(f);
        f = fopen(copy_u, "wb"); fwrite(u0, 1, nu, f); fclose(f);
        char *link_path = path(v[i].link_dir, v[i].link_name);
        unlink(link_path);
        check(link(v[i].base ? copy_b : copy_u, link_path) == 0, "hard link an input at an output path");
        pb_error e = {0};
        int fds[2];
        pb_reader *r = open_pair(copy_b, copy_u, &fixture, fds, &e);
        check(r && pb_extract(r, v[i].unit, ex, da, NULL, NULL, &e) == -1, v[i].what);
        check_err(&e, "PB-INPLACE", v[i].file, v[i].what);
        close_pair(r, fds);
        size_t n;
        uint8_t *after = slurp(v[i].base ? copy_b : copy_u, &n);
        check(n == (v[i].base ? nb : nu) && !memcmp(after, v[i].base ? b0 : u0, n), "the input is untouched");
        free(after);
        unlink(link_path);
    }

    char victim[4300];
    snprintf(victim, sizeof victim, "%s/victim", in);
    FILE *vf = fopen(victim, "wb"); fputs("not an output", vf); fclose(vf);
    char *part = path(ex, "main.part");
    unlink(part);
    check(symlink(victim, part) == 0, "a link at the .part path");
    pb_error e = {0};
    int fds[2];
    pb_reader *r = open_pair(path(fx, "base.nsp"), path(fx, "update.nsp"), &fixture, fds, &e);
    check(r && pb_extract(r, "exefs", ex, da, NULL, NULL, &e) == 0, "extraction over a stale .part link");
    close_pair(r, fds);
    size_t vn;
    uint8_t *vb = slurp(victim, &vn);
    check(vn == 13 && !memcmp(vb, "not an output", 13), "the linked file is untouched");
    free(vb);
    free(b0); free(u0);
}



static void test_tree_only(void) {
    static const struct { const char *variant, *unit, *name; int target; long off; } v[] = {
        {"upd-patch-flip.update", "00-00000002.data", "00-00000002.data", PB_TARGET_DATA, 5},
        {"base-data-flip.base", "00-00000001.data", "00-00000001.data", PB_TARGET_DATA, 7},
        {"upd-exefs-flip.update", "exefs", "main", PB_TARGET_EXEFS, 0x2000 - 0x90},
    };
    char ex[4200], da[4200];
    snprintf(ex, sizeof ex, "%s/exefs", tmp);
    snprintf(da, sizeof da, "%s/data", tmp);
    for (size_t i = 0; i < sizeof v / sizeof *v; i++) {
        int fds[2];
        pb_error e = {0};
        fresh(ex);
        fresh(da);
        pb_reader *r = open_pair(path(fx, "base.nsp"), path(fx, "update.nsp"), &fixture, fds, &e);
        if (!r || pb_extract(r, v[i].unit, ex, da, NULL, NULL, &e)) { check(0, "extract the good file"); close_pair(r, fds); continue; }
        close_pair(r, fds);
        size_t n;
        uint8_t *b = slurp(path(v[i].target == PB_TARGET_EXEFS ? ex : da, v[i].name), &n), d[32];
        b[v[i].off] ^= 1;
        rd_sha256(b, n, d);
        free(b);
        char hx[65];
        for (int k = 0; k < 32; k++) sprintf(hx + 2 * k, "%02x", d[k]);
        pb_contract c;
        contract_changed(v[i].name, 5, hx, &c);
        char file[4300], what[160];
        snprintf(file, sizeof file, "%s/variants/%s.nsp", fx, v[i].variant);
        int is_base = strstr(v[i].variant, ".base") != NULL;
        fresh(ex);
        fresh(da);
        r = open_pair(is_base ? file : path(fx, "base.nsp"), is_base ? path(fx, "update.nsp") : file, &c, fds, &e);
        snprintf(what, sizeof what, "hash tree alone refuses %s", v[i].variant);
        check(r && pb_extract(r, v[i].unit, ex, da, NULL, NULL, &e) == -1, what);
        check_err(&e, "PB-DAMAGED", v[i].name, what);
        close_pair(r, fds);
        pb_contract_free(&c);
    }
}



static void test_variants(void) {
    static const struct { const char *name, *unit, *id, *detail; } v[] = {
        {"pfs-count.update", NULL, "PB-NOTGAME", "container"},
        {"pfs-offset.update", NULL, "PB-NOTGAME", "container"},
        {"pfs-strtab.update", NULL, "PB-NOTGAME", "container"},
        {"upd-reloc-count.update", NULL, "PB-DAMAGED", "update table"},
        {"upd-reloc-buckets.update", NULL, "PB-DAMAGED", "update table"},
        {"upd-reloc-offset.update", NULL, "PB-DAMAGED", "update table"},
        {"upd-reloc-order.update", NULL, "PB-DAMAGED", "update table"},
        {"upd-sub-order.update", NULL, "PB-DAMAGED", "update table"},
        {"upd-sub-end.update", NULL, "PB-DAMAGED", "update table"},
        {"upd-table-offset.update", NULL, "PB-DAMAGED", "update table"},
        {"upd-ivfc-size.update", NULL, "PB-DAMAGED", "integrity"},
        {"upd-ivfc-block.update", NULL, "PB-DAMAGED", "integrity"},
        {"upd-feature.update", NULL, "PB-NOTGAME", "data"},
        {"upd-nca2.update", NULL, "PB-NOTGAME", "content format"},
        {"upd-ticket-personal.update", NULL, "PB-TICKET", "personal"},
        {"upd-no-ticket.update", NULL, "PB-TICKET", "missing"},
        {"upd-patch-flip.update", "00-00000002.data", "PB-DAMAGED", "00-00000002.data"},
        {"base-data-flip.base", "00-00000001.data", "PB-DAMAGED", "00-00000001.data"},
        {"upd-exefs-flip.update", "exefs", "PB-DAMAGED", "main"},
    };
    char dir[4200], ex[4200], da[4200];
    snprintf(dir, sizeof dir, "%s/variants", fx);
    snprintf(ex, sizeof ex, "%s/exefs", tmp);
    snprintf(da, sizeof da, "%s/data", tmp);
    check(count_entries(dir, ".nsp") == (int)(sizeof v / sizeof *v), "every fixture variant is covered");
    for (size_t i = 0; i < sizeof v / sizeof *v; i++) {
        char file[4300], what[160];
        snprintf(file, sizeof file, "%s/%s.nsp", dir, v[i].name);
        int is_base = strstr(v[i].name, ".base") != NULL, fds[2];
        pb_error e = {0};
        pb_reader *r = open_pair(is_base ? file : path(fx, "base.nsp"), is_base ? path(fx, "update.nsp") : file, &fixture, fds, &e);
        snprintf(what, sizeof what, "variant %s", v[i].name);
        if (!v[i].unit) {
            check(!r, what);
            check_err(&e, v[i].id, v[i].detail, what);
        } else {
            fresh(ex);
            fresh(da);
            check(r != NULL, what);
            if (r) {
                check(pb_extract(r, v[i].unit, ex, da, NULL, NULL, &e) == -1, what);
                check_err(&e, v[i].id, v[i].detail, what);
                check(count_entries(ex, "") + count_entries(da, "") == 0, "nothing left of a damaged file");
                if (strcmp(v[i].unit, "17-00000001.data")) check(pb_extract(r, "17-00000001.data", ex, da, NULL, NULL, &e) == 0, "other units still extract");
            }
        }
        close_pair(r, fds);
        if (!is_base) {
            int kind;
            char id[65];
            int rp = probe(file, keys, keys_n, &fixture, &kind, id, &e);
            if (!strncmp(v[i].name, "pfs-", 4) || !strcmp(v[i].name, "upd-nca2.update") || !strncmp(v[i].name, "upd-ticket", 10) || !strncmp(v[i].name, "upd-no-ticket", 13))
                check(rp == -1 && !strcmp(e.id, v[i].id), what);
        }
    }

    size_t n;
    uint8_t *u = slurp(path(fx, "update.nsp"), &n), *b = slurp(path(fx, "base.nsp"), &n);
    size_t bn = n, un;
    free(u);
    u = slurp(path(fx, "update.nsp"), &un);
    const size_t cuts[] = {0, 8, 20, 0x100, 0x200, 0x1000, un / 2, un - 0x5000, un - 1};
    for (size_t i = 0; i < sizeof cuts / sizeof *cuts; i++) {
        spill(path(tmp, "cut.nsp"), u, cuts[i]);
        int fds[2];
        pb_error e = {0};
        pb_reader *r = open_pair(path(fx, "base.nsp"), path(tmp, "cut.nsp"), &fixture, fds, &e);
        char what[64];
        snprintf(what, sizeof what, "update cut at %zu refused", cuts[i]);
        check(!r && !strcmp(e.id, "PB-NOTGAME"), what);
        close_pair(r, fds);
        int kind;
        char id[65];
        check(probe(path(tmp, "cut.nsp"), keys, keys_n, &fixture, &kind, id, &e) == -1 && !strcmp(e.id, "PB-NOTGAME"), what);
    }
    spill(path(tmp, "cut.nsp"), b, bn - 0x200);
    int fds[2];
    pb_error e = {0};
    pb_reader *r = open_pair(path(tmp, "cut.nsp"), path(fx, "update.nsp"), &fixture, fds, &e);
    check(!r && !strcmp(e.id, "PB-NOTGAME"), "base cut refused");
    close_pair(r, fds);
    free(u);
    free(b);
}




static int test_real(const char *dir, const char *out) {
    DIR *d = opendir(dir);
    if (!d) { fprintf(stderr, "cannot list %s\n", dir); return 2; }
    char base[4200] = "", update[4200] = "", keyfile[4200] = "";
    struct dirent *de;
    while ((de = readdir(d))) {
        size_t l = strlen(de->d_name);
        if (l > 5 && !strcmp(de->d_name + l - 5, ".keys")) snprintf(keyfile, sizeof keyfile, "%s", path(dir, de->d_name));
    }
    if (!keyfile[0]) { closedir(d); fprintf(stderr, "no *.keys in %s\n", dir); return 2; }
    keys = slurp(keyfile, &keys_n);
    rewinddir(d);
    while ((de = readdir(d))) {
        size_t l = strlen(de->d_name);
        if (l < 4 || strcmp(de->d_name + l - 4, ".nsp")) continue;
        pb_error e = {0};
        int kind = -1;
        char id[65];
        if (probe(path(dir, de->d_name), keys, keys_n, pb_contract_game(), &kind, id, &e)) { printf("real: %s skipped (%s %s)\n", de->d_name, e.id, e.detail); continue; }
        if (kind == PB_KIND_BASE) snprintf(base, sizeof base, "%s", path(dir, de->d_name));
        if (kind == PB_KIND_UPDATE) snprintf(update, sizeof update, "%s", path(dir, de->d_name));
    }
    closedir(d);
    check(base[0] && update[0], "real: base and update recognised by the probe");
    if (!base[0] || !update[0]) return 1;
    fresh(out);
    char exefs[4200], data[4200];
    snprintf(exefs, sizeof exefs, "%s", path(out, "exefs"));
    snprintf(data, sizeof data, "%s", path(out, "data"));
    if (mkdir(exefs, 0755) || mkdir(data, 0755)) return 2;
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    int fds[2] = {open_ro(base), open_ro(update)};
    pb_error e = {0};
    const pb_contract *c = pb_contract_game();
    pb_reader *r = pb_open(fds[0], fds[1], keys, keys_n, c, &e);
    check(r != NULL, "real: reader opens");
    if (!r) fprintf(stderr, "real: %s %s\n", e.id, e.detail);
    uint64_t bytes = 0;
    for (size_t i = 0; r && i < c->n; i++) {
        if (i && !strcmp(c->files[i].unit, c->files[i - 1].unit)) continue;
        int ok = pb_extract(r, c->files[i].unit, exefs, data, NULL, NULL, &e) == 0;
        if (!ok) fprintf(stderr, "real: unit %s: %s %s\n", c->files[i].unit, e.id, e.detail);
        check(ok, "real: unit extracted and digest-checked");
    }
    for (size_t i = 0; i < c->n; i++) bytes += c->files[i].size;
    pb_close(r);
    close(fds[0]);
    close(fds[1]);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double s = (double)(t1.tv_sec - t0.tv_sec) + (double)(t1.tv_nsec - t0.tv_nsec) / 1e9;
    check(count_entries(exefs, "") + count_entries(data, "") == (int)c->n && count_entries(data, ".part") == 0, "real: every declared file and no partial file");
    printf("real: %zu files, %llu bytes in %.1f s (%s)\n", c->n, (unsigned long long)bytes, s, rd_crypto_impl());
    rd_wipe(keys, keys_n);
    free(keys);
    rmrf(out);
    printf("t_dump_reader real: %s\n", fails ? "FAIL" : "ok");
    return fails != 0;
}

int main(void) {
    const char *real = getenv("DM_REAL_DUMP_DIR"), *real_out = getenv("DM_REAL_OUT");
    if (real && real_out) return test_real(real, real_out);
    fx = getenv("DM_DUMP_FIXTURE");
    if (!fx) { fprintf(stderr, "DM_DUMP_FIXTURE not set (see the Makefile)\n"); return 2; }
    snprintf(tmp, sizeof tmp, "%s/t_dump_reader.out", fx);
    fresh(tmp);
    check(rd_crypto_ready() == 0, "crypto self test");
    keys = slurp(path(fx, "keys.txt"), &keys_n);
    size_t n;
    char *ct = (char *)slurp(path(fx, "contract.tsv"), &n);
    pb_error e = {0};
    if (pb_contract_parse(ct, n, &fixture, &e)) { fprintf(stderr, "fixture contract: %s %s\n", e.id, e.detail); return 1; }
    free(ct);
    test_contract();
    test_probe();
    test_extract();
    test_wrong_contract();
    test_variants();
    test_tree_only();
    test_inplace();
    rmrf(tmp);
    pb_contract_free(&fixture);
    free(keys);
    printf("t_dump_reader: %s (%s)\n", fails ? "FAIL" : "ok", rd_crypto_impl());
    return fails != 0;
}
