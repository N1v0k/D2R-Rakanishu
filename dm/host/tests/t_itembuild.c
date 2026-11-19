

#include "../dm_itembuild.h"
#include <stdio.h>
#include <string.h>

static int fails;
#define CHECK(c) do { if (!(c)) { fails++; fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #c); } } while (0)



enum { F_OK = 0x10, F_NOBUILD = 0x20, F_NOATTACH = 0x30, F_ATTACH_LIES = 0x40 };
#define OBJS 64
static struct { int live, owner, freed_twice; } obj[OBJS];
static int nobj, parent_nobuild;
static uint8_t g_bits[16];
static uint64_t mk(void) { if (nobj >= OBJS) return 0; obj[nobj].live = 1; obj[nobj].owner = -1; return (uint64_t)++nobj; }
static uint64_t build(void *ctx, const uint8_t *buf, int len, int *used, int *socks) {
    (void)ctx; (void)len;
    if (buf[0] == F_NOBUILD) return 0;
    if (buf[0] < F_OK) { if (parent_nobuild) return 0; *socks = buf[0]; } else *socks = 0;
    *used = 1;
    return mk();
}
static int attach(void *ctx, uint64_t p, uint64_t ch) {
    (void)ctx;
    int b = g_bits[ch - 1];
    if (b == F_NOATTACH) return 0;
    obj[ch - 1].owner = (int)p - 1;
    return b != F_ATTACH_LIES;
}
static int loose(void *ctx, uint64_t it) { (void)ctx; return obj[it - 1].owner < 0; }
static void release(void *ctx, uint64_t it) {
    (void)ctx; int k = (int)it - 1;
    if (!obj[k].live) { obj[k].freed_twice = 1; return; }
    obj[k].live = 0;
    for (int i = 0; i < nobj; i++) if (obj[i].live && obj[i].owner == k) release(ctx, (uint64_t)i + 1);
}
static const dm_ib_ops ops = { build, attach, loose, release, NULL };

static void reset(const uint8_t *bits, int n) { memset(obj, 0, sizeof obj); nobj = 0; parent_nobuild = 0; memcpy(g_bits, bits, (size_t)n); }
static int live(void) { int n = 0; for (int i = 0; i < nobj; i++) n += obj[i].live; return n; }
static int twice(void) { int n = 0; for (int i = 0; i < nobj; i++) n += obj[i].freed_twice; return n; }

static int owned_by(uint64_t p) { for (int i = 0; i < nobj; i++) if (obj[i].live && i != (int)p - 1 && obj[i].owner != (int)p - 1) return 0; return 1; }

static void run(const char *what, const uint8_t *bits, int n, int all, int expect_parent, int want, int socked, int failed, int bytes) {
    reset(bits, n); dm_ib_result r;
    uint64_t it = dm_ib_make(&ops, g_bits, n, all, &r);
    int ok = !!it == expect_parent && r.want == want && r.socked == socked && r.failed == failed && r.bytes == (it || !all ? bytes : r.bytes) && !twice();
    if (it) { ok = ok && owned_by(it); release(NULL, it); }
    ok = ok && live() == 0 && !twice();
    if (!ok) { fails++; fprintf(stderr, "%s (all %d): parent %d want %d socked %d failed %d bytes %d live %d twice %d\n", what, all, !!it, r.want, r.socked, r.failed, r.bytes, live(), twice()); }
}

int main(void) {
    for (int all = 0; all < 2; all++) {
        const uint8_t plain[] = { 0 };
        run("no sockets", plain, 1, all, 1, 0, 0, 0, 1);
        const uint8_t two[] = { 2, F_OK, F_OK };
        run("two fillers", two, 3, all, 1, 2, 2, 0, 3);

        const uint8_t nobuild[] = { 3, F_OK, F_NOBUILD, F_OK };
        run("filler not built", nobuild, 4, all, !all, 3, 1, 2, 2);

        const uint8_t noattach[] = { 3, F_OK, F_NOATTACH, F_OK };
        run("filler not taken", noattach, 4, all, !all, 3, 2, 1, 4);

        const uint8_t lies[] = { 2, F_ATTACH_LIES, F_OK };
        run("filler taken against the answer", lies, 3, all, !all, 2, 1, 1, 3);

        const uint8_t short_[] = { 3, F_OK };
        run("bitstream ends early", short_, 2, all, !all, 3, 1, 2, 2);
    }

    { const uint8_t two[] = { 2, F_OK, F_OK }; reset(two, 3); dm_ib_result r; uint64_t it = dm_ib_make(&ops, g_bits, 3, 1, &r);
      CHECK(it && r.at[0] == 1 && r.at[1] == 2); if (it) release(NULL, it); }

    { const uint8_t one[] = { 1, F_OK }; reset(one, 2); parent_nobuild = 1; dm_ib_result r;
      CHECK(dm_ib_make(&ops, g_bits, 2, 1, &r) == 0 && live() == 0 && r.want == 0); }
    { reset((const uint8_t[]){ 0 }, 1); CHECK(dm_ib_make(&ops, g_bits, 0, 0, NULL) == 0 && nobj == 0); }
    if (fails) { fprintf(stderr, "t_itembuild: %d failed\n", fails); return 1; }
    printf("t_itembuild: fillers built, taken, refused, missing and the parent itself, in both modes, nothing left behind or freed twice: passed\n");
    return 0;
}
