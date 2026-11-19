



#ifndef __APPLE__
#define st_mtimespec st_mtim
#endif
#include "../dm_cloudmerge.c"
#include <errno.h>
#include <sys/mman.h>
#include <sys/wait.h>

static int checks, fails;
#define CHECK(c, ...) do { checks++; if (!(c)) { fails++; printf("  ✗ line %d: ", __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

#define STASH "ModernSharedStashSoftCoreV2"
static char g_wd[512];
static int g_busy, g_saves, g_poison;
static uint64_t g_commits;
static char g_last_notice[256];
void (*g_dm_pre_commit_hook[DM_PRE_COMMIT_HOOKS])(const char *);


const char *dm_save_workdir(void) { return g_wd; }
const char *dm_stash_name(uint8_t *b) { (void)b; return STASH; }
int dm_request_save(void) { return ++g_saves; }
uint64_t dm_fs_commit_count(void) { return g_commits; }
void *dm_memfind(const uint8_t *h, size_t hn, const uint8_t *n, size_t nn) { return memmem(h, hn, n, nn); }
void dm_game_notice(uint8_t *main_base, const char *text) { (void)main_base; snprintf(g_last_notice, sizeof g_last_notice, "%s", text); }
int dm_ui_panel_state(void) { return 1; }
int dm_itemjob_owner(void) { return g_busy; }
const char *dm_str(uint8_t *main_base, const char *key) { (void)main_base; return key; }
int dm_ser_state(void) { return 0; }
uint64_t dm_fnv64(const uint8_t *d, size_t n) { uint64_t h = 1469598103934665603ull; for (size_t i = 0; i < n; i++) h = (h ^ d[i]) * 1099511628211ull; return h; }
void dm_fs_save_poison(int e) { g_poison = e; }
int dm_bits_simple(const uint8_t *bits, int len) { (void)bits; (void)len; return 0; }
int dm_blobs_have(const dm_blob *bl, int nb, const uint8_t *bits, int len) { for (int i = 0; i < nb; i++) if (memmem(bl[i].d, bl[i].n, bits, (size_t)len)) return 1; return 0; }
void dm_blobs_free(dm_blob *bl, int nb) { for (int i = 0; i < nb; i++) free(bl[i].d); }
static int blob_of(const char *p, dm_blob *b) { uint8_t *d; size_t n; if (!file_blob(p, &d, &n)) { free(d); return 0; } b->d = d; b->n = n; return 1; }

int dm_save_blobs(const char *wd, const char *stash, dm_blob *out, int max) {
    int n = 0; char p[1200];
    snprintf(p, sizeof p, "%s/Ann/Ann.d2s", wd); if (n < max && blob_of(p, &out[n])) n++;
    snprintf(p, sizeof p, "%s/%s/%s.d2i", wd, stash, stash); if (n < max && blob_of(p, &out[n])) n++;
    return n;
}


int dm_cloud_pack_each(const char *path, void (*cb)(const char *rel, const uint8_t *d, uint32_t n, void *ctx), void *ctx) {
    FILE *f = fopen(path, "rb"); if (!f) return -1;
    uint8_t h[24]; uint32_t n = 0; int ok = fread(h, 1, 24, f) == 24 && !memcmp(h, "SWPK1\n\0\0", 8); if (ok) memcpy(&n, h + 20, 4); ok = ok && n <= 64;
    for (uint32_t i = 0; ok && i < n; i++) { uint16_t pl; char rel[256]; uint64_t mt; uint32_t sz;
        ok = fread(&pl, 2, 1, f) == 1 && pl < 256 && fread(rel, 1, pl, f) == pl && fread(&mt, 8, 1, f) == 1 && fread(&sz, 4, 1, f) == 1 && sz <= (16u << 20);
        if (!ok) break; rel[pl] = 0; uint8_t *d = malloc(sz ? sz : 1); ok = !sz || fread(d, 1, sz, f) == sz; if (ok) cb(rel, d, sz, ctx); free(d); }
    fclose(f); return ok ? (int)n : -1;
}


typedef struct { uint8_t raw[0x200]; int len; uint8_t bits[VB]; uint8_t nd[0x100], ip[0x40]; } fake_unit;
static fake_unit *g_units[256]; static int g_nunits; static uint32_t g_next_id = 100;
static uint64_t unit_new(const uint8_t *bits, int len) {
    fake_unit *u = calloc(1, sizeof *u); u->len = len; memcpy(u->bits, bits, (size_t)len);
    *(uint32_t *)(u->raw + 8) = g_next_id++; *(uint64_t *)(u->raw + 0x10) = (uint64_t)u->nd; *(uint64_t *)(u->raw + 0x38) = (uint64_t)u->ip;
    g_units[g_nunits++ & 255] = u; return (uint64_t)u;
}
static fake_unit *unit_of(uint64_t it) { return (fake_unit *)it; }
uint64_t dm_item_make_n(uint8_t *b, uint64_t G, uint64_t room, const uint8_t *buf, int len, int *socks_out, int *bytes_out) {
    (void)b; (void)G; (void)room; if (len < 8 || buf[0] < 8 || buf[0] > len) return 0;
    if (socks_out) *socks_out = 0; if (bytes_out) *bytes_out = buf[0]; return unit_new(buf, buf[0]);
}
int dm_item_write(uint8_t *b, uint64_t it, uint8_t *buf) { (void)b; fake_unit *u = unit_of(it); memcpy(buf, u->bits, (size_t)u->len); return u->len; }



static uint8_t g_page[0x200], g_player[0x200] __attribute__((unused)), g_pp[0x40] __attribute__((unused)), g_inv[0x40] __attribute__((unused)), g_G[0x3000] __attribute__((unused));
static long long g_page_gold __attribute__((unused)); static int g_notified;
static uint8_t g_placed[64][VB] __attribute__((unused)); static int g_placed_len[64] __attribute__((unused)), g_nplaced __attribute__((unused));
int dm_stash_pages(uint64_t G, uint64_t player, uint64_t pg[8], uint32_t id[8]) { (void)G; (void)player; pg[0] = (uint64_t)g_page; id[0] = 7; return 1; }
void dm_stash_gold_notify(uint8_t *b, uint64_t G, uint64_t player, uint32_t uid, int value) { (void)b; (void)G; (void)player; (void)uid; (void)value; g_notified++; }
uint64_t dm_unit_inv(uint64_t u) { (void)u; return 0; }

#if defined(__x86_64__)
static int f_get_stat(uint64_t unit, int stat, int layer) { (void)unit; (void)layer; return stat == 15 ? (int)g_page_gold : 0; }
static void f_set_stat(uint64_t unit, int stat, int value, int layer) { (void)unit; (void)layer; if (stat == 15) g_page_gold = value; }
static void f_free(uint64_t G, uint64_t it) { (void)G; (void)it; }
static int f_near(uint64_t G, uint64_t player, int a, int b) { (void)G; (void)player; (void)a; (void)b; return 1; }
static int f_slot(uint64_t inv, uint64_t nu, int page, uint32_t *fx, uint32_t *fy, int z) { (void)inv; (void)nu; (void)page; (void)z; *fx = (uint32_t)g_nplaced; *fy = 0; return g_nplaced < 10; }
static int f_place(uint64_t G, uint64_t player, uint64_t *one, uint64_t *ox) { (void)G; (void)ox; fake_unit *u = unit_of(*one);
    *(uint64_t *)(u->nd + 0xa0) = player; memcpy(g_placed[g_nplaced], u->bits, (size_t)u->len); g_placed_len[g_nplaced++] = u->len; return 1; }
static void f_notify(uint64_t G, uint64_t player, uint64_t nu, int a, int b, int c, int d, int e, uint64_t pos) { (void)G; (void)player; (void)nu; (void)a; (void)b; (void)c; (void)d; (void)e; (void)pos; }
static uint8_t *g_base;
static void tramp(uint64_t off, void *fn) { uint8_t *p = g_base + off; p[0] = 0x48; p[1] = 0xb8; memcpy(p + 2, &fn, 8); p[10] = 0xff; p[11] = 0xe0; }
static int base_init(void) {
    g_base = mmap(NULL, 0x2200000, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (g_base == MAP_FAILED) return 0;
    tramp(0x5b9290, (void *)f_get_stat); tramp(0x5bc1f0, (void *)f_set_stat); tramp(0x658be0, (void *)f_free); tramp(0x650d10, (void *)f_near);
    tramp(0x55f620, (void *)f_slot); tramp(0x6f1940, (void *)f_place); tramp(0x642f70, (void *)f_notify);
    return 1;
}
#endif


static void mkpath(const char *p) { char t[1200]; snprintf(t, sizeof t, "%s", p); for (char *s = t + 1; *s; s++) if (*s == '/') { *s = 0; mkdir(t, 0755); *s = '/'; } mkdir(t, 0755); }
static void put(const char *p, const void *d, size_t n) { char dir[1200]; snprintf(dir, sizeof dir, "%s", p); char *s = strrchr(dir, '/'); if (s) { *s = 0; mkpath(dir); }
    FILE *f = fopen(p, "wb"); fwrite(d, 1, n, f); fclose(f); }
static int exists(const char *p) { struct stat st; return !stat(p, &st); }
static void item(uint8_t *out, uint8_t tag) { memset(out, 0, 12); out[0] = 12; out[7] = tag; out[8] = (uint8_t)(tag * 3 + 1); out[11] = 0x5a; }

static size_t d2i(uint8_t *out, uint32_t gold, const uint8_t *tags, int n) {
    uint32_t len = 68 + 12u * (uint32_t)n; memset(out, 0, len); memcpy(out, "\x55\xaa\x55\xaa", 4); memcpy(out + 12, &gold, 4); memcpy(out + 16, &len, 4);
    out[64] = 'J'; out[65] = 'M'; uint16_t c = (uint16_t)n; memcpy(out + 66, &c, 2); for (int i = 0; i < n; i++) item(out + 68 + 12 * i, tags[i]); return len; }
static size_t vault(uint8_t *out, const uint8_t *tags, int n) {
    memcpy(out, "SWVT\x01\x00", 6); uint16_t c = (uint16_t)n; memcpy(out + 6, &c, 2); size_t o = 8;
    for (int i = 0; i < n; i++) { out[o] = 1; out[o + 1] = 0; uint16_t k = (uint16_t)i, l = 12; memcpy(out + o + 2, &k, 2); memcpy(out + o + 4, &l, 2); item(out + o + 6, tags[i]); o += 18; }
    return o; }
typedef struct { const char *rel; const uint8_t *d; uint32_t n; } pfile;
static void pack(const char *p, const pfile *f, int n) {
    uint8_t *b = malloc(1 << 16); size_t o = 0; memcpy(b, "SWPK1\n\0\0", 8); o = 8; uint64_t st = 1; memcpy(b + o, &st, 8); o += 8; uint32_t fl = 0, c = (uint32_t)n; memcpy(b + o, &fl, 4); o += 4; memcpy(b + o, &c, 4); o += 4;
    for (int i = 0; i < n; i++) { uint16_t pl = (uint16_t)strlen(f[i].rel); memcpy(b + o, &pl, 2); o += 2; memcpy(b + o, f[i].rel, pl); o += pl; uint64_t mt = 1; memcpy(b + o, &mt, 8); o += 8;
        memcpy(b + o, &f[i].n, 4); o += 4; memcpy(b + o, f[i].d, f[i].n); o += f[i].n; }
    put(p, b, o); free(b); }
static void plan_file(const char *entry_dir, int items, long long gold) {
    char p[1200]; snprintf(p, sizeof p, "%s/plan-%s.bin", entry_dir, STASH); uint8_t h[8] = { 'S', 'W', 'C', 'M', 1, 0, 0, 0 }; *(uint16_t *)(h + 6) = (uint16_t)items;
    uint8_t b[64]; memcpy(b, h, 8); memcpy(b + 8, &gold, 8); size_t o = 16; for (int i = 0; i < items; i++) { uint16_t l = 12; memcpy(b + o, &l, 2); item(b + o + 2, (uint8_t)(200 + i)); o += 14; }
    put(p, b, o); }


static void many_entries(const char *base) {
    snprintf(g_wd, sizeof g_wd, "%s/many", base); snprintf(g_stash, sizeof g_stash, "%s", STASH);
    uint8_t s[200]; uint8_t tags[1] = { 1 }; size_t n = d2i(s, 10, tags, 1);
    char p[1200];
    for (int i = 1; i <= 300; i++) { snprintf(p, sizeof p, "%s/_cloudinbox/%04d-import", g_wd, i); mkpath(p); plan_file(p, 0, 0);
        snprintf(p, sizeof p, "%s/_cloudinbox/%04d-import/mine.pack", g_wd, i); pfile f = { STASH "/" STASH ".d2i", s, (uint32_t)n }; pack(p, &f, 1); }
    snprintf(p, sizeof p, "%s/_cloudinbox/0301-import/mine.pack", g_wd); pfile f = { STASH ".swvault", s, 0 }; uint8_t v[64]; f.n = (uint32_t)vault(v, tags, 1); f.d = v; pack(p, &f, 1);
    g_entry[0] = 0;
    CHECK(entry_pick(g_wd) == 1 && !strcmp(g_entry, "_cloudinbox/0301-import") && !g_have, "the open entry after 300 finished ones: %s", g_entry);
    snprintf(p, sizeof p, "%s/_cloudinbox/0301-import", g_wd); plan_file(p, 0, 0);
    CHECK(entry_pick(g_wd) == 0, "all finished: nothing to merge");
}

#if defined(__x86_64__)


static void sh(const char *fmt, const char *a) { char c[4000]; snprintf(c, sizeof c, fmt, a, a, a); if (system(c)) { printf("  ✗ %s\n", c); fails++; } }
static void snapshot(void) { sh("rm -rf '%s.commit' && cp -a '%s' '%s.commit'", g_wd); }
static void remount(void) { sh("rm -rf '%s' && cp -a '%s.commit' '%s'", g_wd); }


static void phone(const char *base, const char *name) {
    snprintf(g_wd, sizeof g_wd, "%s/%s", base, name);
    uint8_t s[400], v[200], m[400], mv[200], tags9[1] = { 9 }, tags12[2] = { 1, 2 }, tags34[2] = { 3, 4 }, tags4[1] = { 4 };
    char p[1200];
    snprintf(p, sizeof p, "%s/%s/%s.d2i", g_wd, STASH, STASH); put(p, s, d2i(s, 1000, tags9, 1));
    snprintf(p, sizeof p, "%s/%s.swvault", g_wd, STASH); put(p, v, vault(v, tags4, 1));
    snprintf(p, sizeof p, "%s/Ann/Ann.d2s", g_wd); put(p, "ANN", 3);
    pfile f[2] = { { STASH "/" STASH ".d2i", m, (uint32_t)d2i(m, 5000, tags12, 2) }, { STASH ".swvault", mv, (uint32_t)vault(mv, tags34, 2) } };
    snprintf(p, sizeof p, "%s/_cloudinbox/1700000000000-import/mine.pack", g_wd); pack(p, f, 2);
    g_page_gold = 1000; g_nplaced = 0; g_last_notice[0] = 0; g_busy = 0;
    memset(g_player, 0, sizeof g_player); *(uint32_t *)(g_player + 4) = 1; static const char ann[] = "Ann"; *(uint64_t *)(g_player + 0x10) = (uint64_t)ann;
    *(uint64_t *)(g_player + 0x38) = (uint64_t)g_pp; *(uint64_t *)(g_pp + 0x20) = 1; *(uint64_t *)(g_player + 0x90) = (uint64_t)g_inv;
    snapshot();
}
static uint32_t g_tick = 1000;
static void tick(void) { g_tick += 30; *(uint32_t *)(g_G + 0x170) = g_tick; dm_cloudmerge_srv_tick(g_base, (uint64_t)g_G, (uint64_t)g_player); }
static void leave(void) { dm_cloudmerge_srv_tick(g_base, (uint64_t)g_G, 0); }

static void commit_ex(int stash) {
    char p[1200]; uint8_t c[2048]; size_t o = 3; memcpy(c, "ANN", 3); for (int i = 0; i < g_nplaced; i++) { memcpy(c + o, g_placed[i], (size_t)g_placed_len[i]); o += (size_t)g_placed_len[i]; }
    snprintf(p, sizeof p, "%s/Ann/Ann.d2s", g_wd); put(p, c, o);
    struct timespec ts = { 0, 20 * 1000 * 1000 }; nanosleep(&ts, NULL);
    if (stash) { snprintf(p, sizeof p, "%s/%s/%s.d2i", g_wd, STASH, STASH); uint8_t s[400], tags9[1] = { 9 }; put(p, s, d2i(s, (uint32_t)g_page_gold, tags9, 1)); }
    g_dm_pre_commit_hook[4](g_wd); snapshot(); g_commits++;
}
static void commit(void) { commit_ex(1); }
static int plan_on_disk(int *items, long long *gold) {
    char p[1200]; snprintf(p, sizeof p, "%s/_cloudinbox/1700000000000-import/plan-%s.bin", g_wd, STASH); cm_list l = {0}; int r = plan_read(p, &l, gold); *items = l.n; cl_free(&l); return r; }

static void import_merge(const char *base) {
    phone(base, "merge");
    leave();
    g_busy = 1; tick(); tick();
    int items = -1; long long gold = -1;
    CHECK(!g_have && g_page_gold == 1000 && g_nplaced == 0 && plan_on_disk(&items, &gold) == 0, "recipe lock: nothing computed, paid or placed");
    g_busy = 0; tick();
    CHECK(!strcmp(g_entry, "_cloudinbox/1700000000000-import") && g_have, "entry picked: %s", g_entry);
    CHECK(g_page_gold == 6000 && g_notified == 1, "the backup's gold once onto the page: %lld", g_page_gold);
    CHECK(g_nplaced == 3, "items 1, 2 and 3 into the inventory, item 4 is already in the phone's vault: %d", g_nplaced);
    CHECK(!strcmp(g_last_notice, "dmImportMergeBack"), "the import notice, not the iCloud one: %s", g_last_notice);
    commit();
    CHECK(plan_on_disk(&items, &gold) == 1 && items == 0 && gold == 0, "after the save: plan empty, gold crossed off (%d items, %lld gold)", items, gold);
    tick(); tick(); tick();
    CHECK(g_page_gold == 6000 && g_nplaced == 3, "no second payment in the same game");
    leave(); tick(); tick();
    CHECK(g_page_gold == 6000 && g_nplaced == 3 && !g_entry[0], "entering again: nothing paid twice");
}


static void char_file(const char *name, int hc) {
    char p[1200]; uint8_t h[0x40] = { 0x55, 0xaa, 0x55, 0xaa, 0x69 }; h[0x14] = hc ? 0x04 : 0; snprintf(p, sizeof p, "%s/%s/%s.d2s", g_wd, name, name); put(p, h, sizeof h); }


static void side_files(const char *base) {
    phone(base, "side");
    uint8_t i3[12], i2[12], pg[40] = "SWPG"; item(i3, 3); item(i2, 2); char p[1200];
    char_file("Zed", 0); memcpy(pg + 8, i3, 12); snprintf(p, sizeof p, "%s/Zed/Zed.swpage", g_wd); put(p, pg, sizeof pg);
    char_file("Hal", 1); memcpy(pg + 8, i2, 12); snprintf(p, sizeof p, "%s/Hal/Hal.swpouch", g_wd); put(p, pg, sizeof pg);
    snapshot(); leave(); tick();
    int has2 = 0, has3 = 0; for (int i = 0; i < g_nplaced; i++) { has2 |= g_placed[i][7] == 2; has3 |= g_placed[i][7] == 3; }
    CHECK(g_nplaced == 2 && has2 && !has3, "item 3 from Zed's page file stays out, item 2 (only with the hard core Hal) is returned: %d placed", g_nplaced);
    leave();
}

static void many_side_files(const char *base) {
    snprintf(g_wd, sizeof g_wd, "%s/manyside", base); snprintf(g_stash, sizeof g_stash, "%s", STASH); g_char[0] = 0; char p[1200];
    for (int i = 0; i < 200; i++) { char nm[16]; snprintf(nm, sizeof nm, "C%03d", i); uint8_t h[0x40] = { 0x55, 0xaa, 0x55, 0xaa, 0x69 };
        snprintf(p, sizeof p, "%s/%s/%s.d2s", g_wd, nm, nm); put(p, h, sizeof h);
        snprintf(p, sizeof p, "%s/%s/%s.swpage", g_wd, nm, nm); put(p, "page", 4); snprintf(p, sizeof p, "%s/%s/%s.swpouch", g_wd, nm, nm); put(p, "pouch", 5); }
    dm_blob *bl = NULL; int nb = dedup_blobs(g_wd, &bl);
    CHECK(nb == 400, "200 characters: all 400 page and pouch files read (%d)", nb);
    dm_blobs_free(bl, nb); free(bl);
}


static void no_gold(const char *base) {
    phone(base, "nogold");
    char p[1200]; snprintf(p, sizeof p, "%s/_cloudinbox/1700000000000-import/nogold-%s", g_wd, STASH); put(p, "", 0);
    snapshot(); leave(); tick();
    CHECK(g_page_gold == 1000 && g_nplaced == 3, "nogold: no gold paid (%lld), items 1-3 returned (%d)", g_page_gold, g_nplaced);
    commit();
    int items = -1; long long gold = -1;
    CHECK(plan_on_disk(&items, &gold) == 1 && items == 0 && gold == 0, "nogold: the plan is done after the save (%d, %lld)", items, gold);
    leave();
}



static int kill_point(const char *base, int at) {
    int before = fails; char name[32]; snprintf(name, sizeof name, "kill%d", at); phone(base, name);
    fflush(stdout); pid_t pid = fork();
    if (!pid) { char v[16]; snprintf(v, sizeof v, "cm:%d", at); setenv("DM_TEST_KILL_AT", v, 1); if (!freopen("/dev/null", "w", stderr)) _exit(2);
        tick(); commit_ex(at != 4); tick(); commit(); tick(); _exit(0); }
    int st = 0; waitpid(pid, &st, 0);
    CHECK(WIFSIGNALED(st) && WTERMSIG(st) == SIGKILL, "cm:%d: the child died at the kill point", at);
    remount();
    char p[1200]; uint8_t *d; size_t n; snprintf(p, sizeof p, "%s/%s/%s.d2i", g_wd, STASH, STASH);
    g_page_gold = 0; if (file_blob(p, &d, &n)) { uint32_t g; memcpy(&g, d + 12, 4); g_page_gold = g; } free(d);
    snprintf(p, sizeof p, "%s/Ann/Ann.d2s", g_wd); g_nplaced = 0;
    if (file_blob(p, &d, &n)) for (size_t o = 3; o + 12 <= n; o += 12) { memcpy(g_placed[g_nplaced], d + o, 12); g_placed_len[g_nplaced++] = 12; }
    free(d);
    tick(); commit(); tick(); commit(); tick(); commit();
    int items = -1; long long gold = -1; plan_on_disk(&items, &gold);
    CHECK(g_page_gold == 6000, "cm:%d: gold paid exactly once in the end (%lld)", at, g_page_gold);
    int seen[256] = {0}, twice = 0; for (int i = 0; i < g_nplaced; i++) twice |= seen[g_placed[i][7]]++ > 0;
    CHECK(g_nplaced == 3 && !twice && items == 0 && gold == 0, "cm:%d: items 1-3 exactly once (%d placed, plan %d / %lld)", at, g_nplaced, items, gold);
    return fails - before;
}
#endif

int main(void) {
    char base[] = "/tmp/t_cloudmerge.XXXXXX"; const char *tmp = getenv("TMPDIR"); char tb[512];
    if (tmp) { snprintf(tb, sizeof tb, "%s/t_cloudmerge.XXXXXX", tmp); } else snprintf(tb, sizeof tb, "%s", base);
    if (!mkdtemp(tb)) { perror("mkdtemp"); return 1; }
    printf("t_cloudmerge\n");
    many_entries(tb);
    many_side_files(tb);
#if defined(__x86_64__) && defined(CM_HAVE_STASH_SEED)
    printf("  (stash_seed.h present: the gold expectations assume no seed, game function part skipped)\n");
#elif defined(__x86_64__)
    if (base_init()) {

        int at[] = { 1, 2, 4 };
        for (int i = 0; i < 3; i++) { fflush(stdout); pid_t pid = fork(); if (!pid) _exit(kill_point(tb, at[i]) ? 1 : 0);
            int st = 0; waitpid(pid, &st, 0); checks++; if (!WIFEXITED(st) || WEXITSTATUS(st)) { fails++; printf("  ✗ kill point cm:%d\n", at[i]); } }
        import_merge(tb);
        side_files(tb);
        no_gold(tb);
    } else printf("  (no executable mapping: game function part skipped)\n");
#else
    printf("  (not x86-64: game function part skipped)\n");
#endif
    printf("%d checks, %d failed\n", checks, fails);
    char rm[600]; snprintf(rm, sizeof rm, "rm -rf '%s'", tb); if (system(rm)) printf("  (could not remove %s)\n", tb);
    return fails != 0;
}
