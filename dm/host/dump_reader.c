



#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64
#include "dump_reader.h"
#include "rd_crypto.h"
#include "romfs_validate.h"

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define GAME_TITLE 0x0100726014352000ULL
#define CHUNK (8u << 20)
#define MIB (1u << 20)



static int fail(pb_error *e, const char *id, const char *fmt, ...) {
    va_list ap;
    snprintf(e->id, sizeof e->id, "%s", id);
    va_start(ap, fmt);
    vsnprintf(e->detail, sizeof e->detail, fmt, ap);
    va_end(ap);
    return -1;
}
#define TRY(x) do { if ((x) != 0) return -1; } while (0)

static int range_ok(uint64_t off, uint64_t len, uint64_t limit) { return off <= limit && len <= limit - off; }
static uint32_t le32(const uint8_t *p) { return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24); }
static uint64_t le64(const uint8_t *p) { return (uint64_t)le32(p) | ((uint64_t)le32(p + 4) << 32); }
static void hex(const uint8_t *d, size_t n, char *out) {
    static const char digits[] = "0123456789abcdef";
    for (size_t i = 0; i < n; i++) { out[2 * i] = digits[d[i] >> 4]; out[2 * i + 1] = digits[d[i] & 15]; }
    out[2 * n] = 0;
}
static int ends_with(const char *s, const char *suf) {
    size_t a = strlen(s), b = strlen(suf);
    return a >= b && !strcmp(s + a - b, suf);
}



typedef struct {
    int fd;
    uint64_t size;
    dev_t dev;
    ino_t ino;
} src_t;

static int src_init(src_t *f, int fd, uint64_t length, pb_error *e) {
    struct stat sb;
    if (fd < 0 || fstat(fd, &sb) != 0) return fail(e, "PB-SOURCE", "open");
    if (!S_ISREG(sb.st_mode) || (uint64_t)sb.st_size != length) return fail(e, "PB-SOURCE", "not a regular file of the given length");
    f->fd = fd;
    f->size = length;
    f->dev = sb.st_dev;
    f->ino = sb.st_ino;
    return 0;
}

static int src_read(src_t *f, uint64_t off, void *buf, size_t len, pb_error *e) {
    if (!range_ok(off, len, f->size)) return fail(e, "PB-DAMAGED", "package");
    uint8_t *p = buf;
    while (len) {
        ssize_t r = pread(f->fd, p, len, (off_t)off);
        if (r < 0 && errno == EINTR) continue;
        if (r <= 0) return fail(e, "PB-SOURCE", "read");
        p += r; off += (uint64_t)r; len -= (size_t)r;
    }
    return 0;
}



#define MAX_KEYS 256
typedef struct {
    char name[48];
    uint8_t val[32];
    size_t len;
} key_ent;
typedef struct {
    key_ent k[MAX_KEYS];
    size_t n;
    int lines;
} keyset;

static int want_key_name(const char *n) {
    return !strcmp(n, "header_key") || !strncmp(n, "titlekek_", 9) || !strncmp(n, "key_area_key_application_", 25) ||
           !strncmp(n, "key_area_key_ocean_", 19) || !strncmp(n, "key_area_key_system_", 20);
}

static int hexval(char c) {
    return (c >= '0' && c <= '9') ? c - '0' : (c >= 'a' && c <= 'f') ? c - 'a' + 10 : (c >= 'A' && c <= 'F') ? c - 'A' + 10 : -1;
}


static int keys_parse(const uint8_t *buf, size_t len, keyset *ks) {
    memset(ks, 0, sizeof *ks);
    if (memchr(buf, 0, len)) return -1;
    size_t i = 0;
    while (i < len) {
        size_t s = i;
        while (i < len && buf[i] != '\n') i++;
        size_t end = i;
        if (i < len) i++;
        const uint8_t *eq = memchr(buf + s, '=', end - s);
        if (!eq) continue;
        size_t ns = s, ne = (size_t)(eq - buf);
        while (ns < ne && (buf[ns] == ' ' || buf[ns] == '\t')) ns++;
        while (ne > ns && (buf[ne - 1] == ' ' || buf[ne - 1] == '\t')) ne--;
        if (ne == ns) continue;
        ks->lines++;
        char name[48];
        if (ne - ns >= sizeof name) continue;
        for (size_t j = ns; j < ne; j++) name[j - ns] = (char)(buf[j] >= 'A' && buf[j] <= 'Z' ? buf[j] - 'A' + 'a' : buf[j]);
        name[ne - ns] = 0;
        if (!want_key_name(name) || ks->n >= MAX_KEYS) continue;
        size_t vs = (size_t)(eq - buf) + 1, ve;
        while (vs < end && (buf[vs] == ' ' || buf[vs] == '\t')) vs++;
        ve = vs;
        while (ve < end && buf[ve] != '\r' && buf[ve] != ' ' && buf[ve] != '\t') ve++;
        key_ent *k = &ks->k[ks->n++];
        memcpy(k->name, name, sizeof name);
        k->len = 0;
        size_t vl = ve - vs;
        if (vl != 32 && vl != 64) continue;
        size_t j;
        for (j = 0; j < vl; j++) {
            int d = hexval((char)buf[vs + j]);
            if (d < 0) break;
            if (j % 2 == 0) k->val[j / 2] = (uint8_t)(d << 4); else k->val[j / 2] |= (uint8_t)d;
        }
        if (j == vl) k->len = vl / 2;
        else rd_wipe(k->val, sizeof k->val);
    }
    return 0;
}

static const uint8_t *key_need(const keyset *ks, const char *name, size_t len, pb_error *e) {
    for (size_t i = 0; i < ks->n; i++) {
        if (strcmp(ks->k[i].name, name)) continue;
        if (ks->k[i].len != len) { fail(e, "PB-KEYWRONG", "%s", name); return NULL; }
        return ks->k[i].val;
    }
    fail(e, "PB-KEYMISSING", "%s", name);
    return NULL;
}



typedef struct {
    uint64_t off, size;
    char name[128];
} pfs_ent;
typedef struct {
    pfs_ent *e;
    uint32_t n;
    uint64_t data_start;
} pfs_t;


static int pfs_parse(const uint8_t *h, size_t hlen, uint64_t region, pfs_t *out, pb_error *e) {
    memset(out, 0, sizeof *out);
    if (hlen < 16 || memcmp(h, "PFS0", 4)) return fail(e, "PB-NOTGAME", "container");
    uint32_t n = le32(h + 4), strsz = le32(h + 8);
    if (n > 4096 || strsz > MIB) return fail(e, "PB-NOTGAME", "container");
    uint64_t hdr = 16 + 24ull * n + strsz;
    if (hdr > hlen || hdr > region) return fail(e, "PB-NOTGAME", "container");
    out->e = calloc(n ? n : 1, sizeof *out->e);
    if (!out->e) return fail(e, "PB-IO", "memory");
    out->n = n;
    out->data_start = hdr;
    const char *strs = (const char *)h + 16 + 24ull * n;
    for (uint32_t i = 0; i < n; i++) {
        const uint8_t *en = h + 16 + 24ull * i;
        uint64_t off = le64(en), size = le64(en + 8);
        uint32_t so = le32(en + 16);
        size_t nl = so < strsz ? strnlen(strs + so, strsz - so) : 0;
        if (!range_ok(off, size, region - hdr) || so >= strsz || nl == 0 || nl >= sizeof out->e[i].name || nl == strsz - so) {
            free(out->e);
            out->e = NULL;
            return fail(e, "PB-NOTGAME", "container");
        }
        out->e[i].off = off;
        out->e[i].size = size;
        memcpy(out->e[i].name, strs + so, nl + 1);
    }
    return 0;
}


static int pfs_read_file(src_t *f, pfs_t *p, uint8_t **hdr_out, size_t *hdr_len, pb_error *e) {
    uint8_t h16[16];
    if (f->size < 16) return fail(e, "PB-NOTGAME", "container");
    TRY(src_read(f, 0, h16, 16, e));
    if (memcmp(h16, "PFS0", 4)) return fail(e, "PB-NOTGAME", "container");
    uint64_t hdr = 16 + 24ull * le32(h16 + 4) + le32(h16 + 8);
    if (hdr > (8u << 20) || hdr > f->size) return fail(e, "PB-NOTGAME", "container");
    uint8_t *h = malloc((size_t)hdr);
    if (!h) return fail(e, "PB-IO", "memory");
    if (src_read(f, 0, h, (size_t)hdr, e) || pfs_parse(h, (size_t)hdr, f->size, p, e)) { free(h); return -1; }
    if (hdr_out) { *hdr_out = h; *hdr_len = (size_t)hdr; } else free(h);
    return 0;
}



#define MAX_TICKETS 32
typedef struct {
    uint8_t rid[16];
    uint8_t enc_key[16];
    uint8_t kind;
} ticket_t;
typedef struct {
    ticket_t t[MAX_TICKETS];
    int n;
} tickets_t;

static int ticket_add(tickets_t *ts, src_t *f, uint64_t off, uint64_t size, pb_error *e) {
    uint8_t t[0x2000];
    if (size < 0x200 || size > sizeof t) return fail(e, "PB-TICKET", "ticket");
    TRY(src_read(f, off, t, (size_t)size, e));
    uint64_t b;
    switch (le32(t)) {
    case 0x10000: case 0x10003: b = 0x240; break;
    case 0x10001: case 0x10004: b = 0x140; break;
    case 0x10002: case 0x10005: b = 0x80; break;
    default: return fail(e, "PB-TICKET", "ticket");
    }
    if (b + 0x180 > size) return fail(e, "PB-TICKET", "ticket");
    if (ts->n >= MAX_TICKETS) return fail(e, "PB-TICKET", "ticket");
    ticket_t *k = &ts->t[ts->n++];
    memcpy(k->enc_key, t + b + 0x40, 16);
    k->kind = t[b + 0x141];
    memcpy(k->rid, t + b + 0x160, 16);
    rd_wipe(t, sizeof t);
    return 0;
}



typedef struct {
    int present;
    uint64_t start, size;
    const uint8_t *fsh;
    uint8_t ctr_hi[8];
    uint8_t fs_kind, hash_kind, crypt;
} nca_sec;
typedef struct {
    src_t *f;
    uint64_t base, size;
    uint8_t h[0xC00];
    uint8_t ctype, kgen, kaek;
    uint64_t tid;
    uint8_t rid[16];
    int has_rid, is_patch;
    nca_sec s[4];
    rd_aes128 ck;
    char key_name[48];
} nca_t;



static int sec_plain(const nca_sec *c) {
    for (int i = 0x148; i < 0x200; i++) if (c->fsh[i]) return 0;
    return 1;
}


static int nca_open(nca_t *n, src_t *f, uint64_t base, uint64_t size, const keyset *ks, pb_error *e) {
    memset(n, 0, sizeof *n);
    n->f = f; n->base = base; n->size = size;
    if (size < 0xC00) return fail(e, "PB-NOTGAME", "content");
    const uint8_t *hk = key_need(ks, "header_key", 32, e);
    if (!hk) return -1;
    rd_aes128 kd, kt;
    rd_aes128_setkey(&kd, hk);
    rd_aes128_setkey(&kt, hk + 16);
    int r = src_read(f, base, n->h, 0xC00, e);
    if (!r) for (int u = 0; u < 6; u++) rd_aes128_xts_decrypt(&kd, &kt, (uint64_t)u, n->h + 0x200 * u, 0x200);
    rd_wipe(&kd, sizeof kd);
    rd_wipe(&kt, sizeof kt);
    TRY(r);
    if (memcmp(n->h + 0x200, "NCA3", 4)) {
        if (!memcmp(n->h + 0x200, "NCA2", 4) || !memcmp(n->h + 0x200, "NCA0", 4)) return fail(e, "PB-NOTGAME", "content format");
        return fail(e, "PB-KEYWRONG", "header_key");
    }
    if (le64(n->h + 0x208) != size) return fail(e, "PB-DAMAGED", "content");
    n->ctype = n->h[0x205];
    uint8_t g = n->h[0x206] > n->h[0x220] ? n->h[0x206] : n->h[0x220];
    n->kgen = g ? (uint8_t)(g - 1) : 0;
    n->kaek = n->h[0x207];
    n->tid = le64(n->h + 0x210);
    memcpy(n->rid, n->h + 0x230, 16);
    for (int i = 0; i < 16; i++) n->has_rid |= n->rid[i] != 0;
    for (int i = 0; i < 4; i++) {
        uint64_t s = (uint64_t)le32(n->h + 0x240 + 16 * i) * 0x200, en = (uint64_t)le32(n->h + 0x244 + 16 * i) * 0x200;
        if (!s && !en) continue;
        if (s < 0xC00 || en <= s || en > size) return fail(e, "PB-DAMAGED", "content");
        nca_sec *c = &n->s[i];
        c->present = 1;
        c->start = s;
        c->size = en - s;
        c->fsh = n->h + 0x400 + 0x200 * i;
        uint8_t d[32];
        rd_sha256(c->fsh, 0x200, d);
        if (memcmp(d, n->h + 0x280 + 32 * i, 32)) return fail(e, "PB-DAMAGED", "content");
        c->fs_kind = c->fsh[2];
        c->hash_kind = c->fsh[3];
        c->crypt = c->fsh[4];
        for (int j = 0; j < 8; j++) c->ctr_hi[j] = c->fsh[0x140 + 7 - j];
        n->is_patch |= c->crypt == 4;
    }
    return 0;
}


static void nca_copy(nca_t *to, const nca_t *from) {
    *to = *from;
    for (int i = 0; i < 4; i++) if (to->s[i].present) to->s[i].fsh = to->h + 0x400 + 0x200 * i;
}


static int nca_key_name(nca_t *n, pb_error *e) {
    static const char *area[3] = {"application", "ocean", "system"};
    if (n->has_rid) { snprintf(n->key_name, sizeof n->key_name, "titlekek_%02x", n->kgen); return 0; }
    if (n->kaek > 2) return fail(e, "PB-NOTGAME", "content key");
    snprintf(n->key_name, sizeof n->key_name, "key_area_key_%s_%02x", area[n->kaek], n->kgen);
    return 0;
}

static int nca_setup_key(nca_t *n, const tickets_t *ts, const keyset *ks, pb_error *e) {
    uint8_t ck[16];
    rd_aes128 kk;
    TRY(nca_key_name(n, e));
    if (n->has_rid) {
        const ticket_t *t = NULL;
        for (int i = 0; i < ts->n; i++) if (!memcmp(ts->t[i].rid, n->rid, 16)) t = &ts->t[i];
        if (!t) return fail(e, "PB-TICKET", "missing");
        if (t->kind != 0) return fail(e, "PB-TICKET", "personal");
        const uint8_t *kek = key_need(ks, n->key_name, 16, e);
        if (!kek) return -1;
        rd_aes128_setkey(&kk, kek);
        rd_aes128_decrypt_block(&kk, t->enc_key, ck);
    } else {
        const uint8_t *kak = key_need(ks, n->key_name, 16, e);
        if (!kak) return -1;
        rd_aes128_setkey(&kk, kak);
        rd_aes128_decrypt_block(&kk, n->h + 0x300 + 0x20, ck);
    }
    rd_aes128_setkey(&n->ck, ck);
    rd_wipe(ck, sizeof ck);
    rd_wipe(&kk, sizeof kk);
    return 0;
}


static int ctr_read(nca_t *n, int si, const uint8_t hi[8], uint64_t off, uint8_t *buf, size_t len, pb_error *e) {
    nca_sec *c = &n->s[si];
    if (!range_ok(off, len, c->size)) return fail(e, "PB-DAMAGED", "section");
    uint64_t abs = c->start + off;
    while (len) {
        if ((abs & 15) || len < 16) {
            uint8_t blk[16];
            uint64_t a = abs & ~15ull;
            size_t skip = (size_t)(abs - a), take = 16 - skip < len ? 16 - skip : len;
            TRY(src_read(n->f, n->base + a, blk, 16, e));
            rd_aes128_ctr(&n->ck, hi, a >> 4, blk, 16);
            memcpy(buf, blk + skip, take);
            abs += take; buf += take; len -= take;
        } else {
            size_t m = len & ~(size_t)15;
            TRY(src_read(n->f, n->base + abs, buf, m, e));
            rd_aes128_ctr(&n->ck, hi, abs >> 4, buf, m);
            abs += m; buf += m; len -= m;
        }
    }
    return 0;
}


typedef struct {
    int si;
    uint32_t bs;
    uint64_t ho, hs, po, ps;
    uint8_t *ht;
} exefs_t;

static int exefs_find(nca_t *n) {
    for (int i = 0; i < 4; i++) if (n->s[i].present && n->s[i].fs_kind == 1 && n->s[i].hash_kind == 2) return i;
    return -1;
}



static int exefs_open(nca_t *n, int si, exefs_t *x, pb_error *e) {
    memset(x, 0, sizeof *x);
    nca_sec *c = &n->s[si];
    if (c->crypt != 3 || !sec_plain(c)) return fail(e, "PB-NOTGAME", "program files");
    const uint8_t *sb = c->fsh + 8;
    x->si = si;
    x->bs = le32(sb + 0x20);
    uint32_t levels = le32(sb + 0x24);
    x->ho = le64(sb + 0x28); x->hs = le64(sb + 0x30); x->po = le64(sb + 0x38); x->ps = le64(sb + 0x40);
    if (levels != 2 || x->bs < 0x200 || x->bs > (1u << 24) || !range_ok(x->ho, x->hs, c->size) || !range_ok(x->po, x->ps, c->size) ||
        x->ps == 0 || x->ps > (256u << 20) || x->hs > MIB || ((x->ps + x->bs - 1) / x->bs) * 32 > x->hs)
        return fail(e, "PB-NOTGAME", "program files");
    x->ht = malloc((size_t)x->hs);
    if (!x->ht) return fail(e, "PB-IO", "memory");
    uint8_t d[32];
    if (ctr_read(n, si, c->ctr_hi, x->ho, x->ht, (size_t)x->hs, e)) return -1;
    rd_sha256(x->ht, (size_t)x->hs, d);
    if (memcmp(d, sb, 32)) return fail(e, "PB-KEYWRONG", "%s", n->key_name);
    return 0;
}

typedef int (*sink_fn)(void *ctx, const uint8_t *p, size_t n, pb_error *e);



static int exefs_stream(nca_t *n, exefs_t *x, uint8_t **chunkp, size_t cap, uint64_t off, uint64_t len, sink_fn sink, void *ctx, pb_error *e) {
    if (!range_ok(off, len, x->ps)) return fail(e, "PB-DAMAGED", "program files");
    if (!len) return 0;
    uint64_t bs = x->bs, per = cap / bs ? cap / bs : 1, b = off / bs, bend = (off + len + bs - 1) / bs;
    if (per * bs > cap) return fail(e, "PB-IO", "memory");
    while (b < bend) {
        uint8_t *chunk = *chunkp;
        uint64_t cb = bend - b < per ? bend - b : per, rs = b * bs, rl = cb * bs;
        if (rl > x->ps - rs) rl = x->ps - rs;
        TRY(ctr_read(n, x->si, n->s[x->si].ctr_hi, x->po + rs, chunk, (size_t)rl, e));
        for (uint64_t k = 0; k < cb; k++) {
            uint64_t bo = k * bs, bl = rl - bo < bs ? rl - bo : bs;
            uint8_t d[32];
            rd_sha256(chunk + bo, (size_t)bl, d);
            if (memcmp(d, x->ht + 32 * (b + k), 32)) return fail(e, "PB-DAMAGED", "program files");
        }
        uint64_t s = off > rs ? off : rs, en = off + len < rs + rl ? off + len : rs + rl;
        TRY(sink(ctx, chunk + (s - rs), (size_t)(en - s), e));
        b += cb;
    }
    return 0;
}

static int sink_copy(void *ctx, const uint8_t *p, size_t n, pb_error *e) {
    (void)e;
    uint8_t **dst = ctx;
    memcpy(*dst, p, n);
    *dst += n;
    return 0;
}



typedef struct { uint64_t v, p; uint32_t patch; } rel_t;
typedef struct { uint64_t p; uint32_t ctr; } sub_t;
typedef struct {
    nca_t *base, *upd;
    int bsi, usi;
    rel_t *r;
    size_t nr;
    uint64_t vsize;
    sub_t *s;
    size_t ns;
    uint64_t pend;
} layer_t;


static int bucket_parse(const uint8_t *t, uint64_t size, size_t esz, size_t emax, uint64_t *end_out, uint8_t **flat, size_t *count, pb_error *e) {
    uint32_t nb = le32(t + 4);
    if (nb < 1 || nb > (0x4000 - 0x10) / 8 || (1ull + nb) * 0x4000 > size) return fail(e, "PB-DAMAGED", "update table");
    *end_out = le64(t + 8);
    size_t total = 0;
    for (uint32_t b = 0; b < nb; b++) {
        uint32_t ne = le32(t + 0x4000ull * (1 + b) + 4);
        if (ne < 1 || ne > emax) return fail(e, "PB-DAMAGED", "update table");
        total += ne;
    }
    *flat = malloc(total * esz);
    if (!*flat) return fail(e, "PB-IO", "memory");
    size_t k = 0;
    for (uint32_t b = 0; b < nb; b++) {
        const uint8_t *bk = t + 0x4000ull * (1 + b);
        uint32_t ne = le32(bk + 4);
        if (le64(bk + 0x10) != le64(t + 0x10 + 8ull * b)) { free(*flat); *flat = NULL; return fail(e, "PB-DAMAGED", "update table"); }
        memcpy(*flat + k * esz, bk + 0x10, ne * esz);
        k += ne;
    }
    *count = total;
    return 0;
}

static int load_table(nca_t *u, int si, uint64_t off, uint64_t size, uint8_t **out, pb_error *e) {
    *out = NULL;
    if (size < 0x8000 || size > (64u << 20)) return fail(e, "PB-DAMAGED", "update table");
    uint8_t *t = malloc((size_t)size);
    if (!t) return fail(e, "PB-IO", "memory");
    if (ctr_read(u, si, u->s[si].ctr_hi, off, t, (size_t)size, e)) { free(t); return -1; }
    *out = t;
    return 0;
}

static int layer_open(layer_t *L, nca_t *base, int bsi, nca_t *upd, int usi, pb_error *e) {
    L->base = base; L->bsi = bsi; L->upd = upd; L->usi = usi;
    const uint8_t *fsh = upd->s[usi].fsh;
    uint64_t ro = le64(fsh + 0x100), rs = le64(fsh + 0x108), so = le64(fsh + 0x120), ss = le64(fsh + 0x128);
    if (memcmp(fsh + 0x110, "BKTR", 4) || memcmp(fsh + 0x130, "BKTR", 4)) return fail(e, "PB-NOTGAME", "update");
    if (!range_ok(ro, rs, upd->s[usi].size) || !range_ok(so, ss, upd->s[usi].size) || ro + rs != so || so + ss != upd->s[usi].size)
        return fail(e, "PB-DAMAGED", "update table");
    L->pend = ro;
    uint8_t *t, *flat;
    size_t n;
    TRY(load_table(upd, usi, ro, rs, &t, e));
    int r = bucket_parse(t, rs, 0x14, (0x4000 - 0x10) / 0x14, &L->vsize, &flat, &n, e);
    free(t);
    TRY(r);
    if (n != le32(fsh + 0x118)) { free(flat); return fail(e, "PB-DAMAGED", "update table"); }
    if (!(L->r = malloc(n * sizeof *L->r))) { free(flat); return fail(e, "PB-IO", "memory"); }
    L->nr = n;
    for (size_t i = 0; i < n; i++) {
        const uint8_t *en = flat + 0x14 * i;
        L->r[i].v = le64(en); L->r[i].p = le64(en + 8); L->r[i].patch = le32(en + 16);
    }
    free(flat);
    for (size_t i = 0; i < n; i++) {
        if (L->r[i].patch > 1 || (i ? L->r[i].v <= L->r[i - 1].v : L->r[i].v != 0) || L->r[i].v >= L->vsize) return fail(e, "PB-DAMAGED", "update table");
        uint64_t len = (i + 1 < n ? L->r[i + 1].v : L->vsize) - L->r[i].v;
        uint64_t lim = L->r[i].patch ? L->pend : base->s[bsi].size;
        if (!range_ok(L->r[i].p, len, lim)) return fail(e, "PB-DAMAGED", "update table");
    }
    uint64_t send;
    TRY(load_table(upd, usi, so, ss, &t, e));
    r = bucket_parse(t, ss, 0x10, 0x3FF, &send, &flat, &n, e);
    free(t);
    TRY(r);
    if (n != le32(fsh + 0x138) || send != so) { free(flat); return fail(e, "PB-DAMAGED", "update table"); }
    if (!(L->s = malloc(n * sizeof *L->s))) { free(flat); return fail(e, "PB-IO", "memory"); }
    L->ns = n;
    for (size_t i = 0; i < n; i++) {
        const uint8_t *en = flat + 0x10 * i;
        L->s[i].p = le64(en); L->s[i].ctr = le32(en + 12);
    }
    free(flat);
    for (size_t i = 0; i < n; i++)
        if ((i ? L->s[i].p <= L->s[i - 1].p : L->s[i].p != 0) || L->s[i].p >= L->pend) return fail(e, "PB-DAMAGED", "update table");
    return 0;
}

static int patch_read(layer_t *L, uint64_t p, uint8_t *buf, size_t len, pb_error *e) {
    if (!range_ok(p, len, L->pend)) return fail(e, "PB-DAMAGED", "update data");
    while (len) {
        size_t lo = 0, hi = L->ns;
        while (hi - lo > 1) { size_t m = (lo + hi) / 2; if (L->s[m].p <= p) lo = m; else hi = m; }
        uint64_t end = lo + 1 < L->ns ? L->s[lo + 1].p : L->pend;
        size_t m = end - p < len ? (size_t)(end - p) : len;
        uint8_t ctr[8];
        memcpy(ctr, L->upd->s[L->usi].ctr_hi, 4);
        uint32_t cv = L->s[lo].ctr;
        ctr[4] = (uint8_t)(cv >> 24); ctr[5] = (uint8_t)(cv >> 16); ctr[6] = (uint8_t)(cv >> 8); ctr[7] = (uint8_t)cv;
        TRY(ctr_read(L->upd, L->usi, ctr, p, buf, m, e));
        p += m; buf += m; len -= m;
    }
    return 0;
}

static int layer_read(layer_t *L, uint64_t v, uint8_t *buf, size_t len, pb_error *e) {
    if (!range_ok(v, len, L->vsize)) return fail(e, "PB-DAMAGED", "update data");
    while (len) {
        size_t lo = 0, hi = L->nr;
        while (hi - lo > 1) { size_t m = (lo + hi) / 2; if (L->r[m].v <= v) lo = m; else hi = m; }
        uint64_t end = lo + 1 < L->nr ? L->r[lo + 1].v : L->vsize;
        size_t m = end - v < len ? (size_t)(end - v) : len;
        uint64_t p = L->r[lo].p + (v - L->r[lo].v);
        if (L->r[lo].patch) TRY(patch_read(L, p, buf, m, e));
        else TRY(ctr_read(L->base, L->bsi, L->base->s[L->bsi].ctr_hi, p, buf, m, e));
        v += m; buf += m; len -= m;
    }
    return 0;
}



typedef struct {
    uint64_t off, size, bs;
    uint8_t *data;
} lvl_t;
typedef struct {
    layer_t *L;
    lvl_t lv[6];
} ivfc_t;

static int ivfc_open(ivfc_t *iv, layer_t *L, const uint8_t *fsh, const char *key_name, pb_error *e) {
    iv->L = L;
    const uint8_t *ih = fsh + 8;
    if (memcmp(ih, "IVFC", 4) || le32(ih + 8) != 0x20 || le32(ih + 12) != 7) return fail(e, "PB-NOTGAME", "integrity");
    uint64_t total = 0;
    for (int i = 0; i < 6; i++) {
        const uint8_t *en = ih + 0x10 + 0x18 * i;
        lvl_t *l = &iv->lv[i];
        l->off = le64(en); l->size = le64(en + 8);
        uint32_t lg = le32(en + 16);
        if (lg < 9 || lg > 20 || !range_ok(l->off, l->size, L->vsize) || l->size == 0) return fail(e, "PB-DAMAGED", "integrity");
        l->bs = 1ull << lg;
        if (i < 5) total += l->size;
    }
    if (total > (64u << 20)) return fail(e, "PB-DAMAGED", "integrity");
    const uint8_t *master = ih + 0xC0;
    for (int i = 0; i < 5; i++) {
        lvl_t *l = &iv->lv[i];
        uint64_t nb = (l->size + l->bs - 1) / l->bs;
        const uint8_t *tab = i ? iv->lv[i - 1].data : master;
        uint64_t tabsz = i ? iv->lv[i - 1].size : 32;
        if (nb * 32 > tabsz) return fail(e, "PB-DAMAGED", "integrity");
        l->data = calloc(1, (size_t)(nb * l->bs));
        if (!l->data) return fail(e, "PB-IO", "memory");
        TRY(layer_read(L, l->off, l->data, (size_t)l->size, e));
        for (uint64_t b = 0; b < nb; b++) {
            uint8_t d[32];
            rd_sha256(l->data + b * l->bs, (size_t)l->bs, d);
            if (memcmp(d, tab + 32 * b, 32)) return i == 0 ? fail(e, "PB-KEYWRONG", "%s", key_name) : fail(e, "PB-DAMAGED", "integrity");
        }
    }
    uint64_t nb5 = (iv->lv[5].size + iv->lv[5].bs - 1) / iv->lv[5].bs;
    if (nb5 * 32 > iv->lv[4].size) return fail(e, "PB-DAMAGED", "integrity");
    return 0;
}


static int ivfc_stream(ivfc_t *iv, uint8_t **chunkp, size_t cap, uint64_t off, uint64_t len, sink_fn sink, void *ctx, pb_error *e) {
    lvl_t *d = &iv->lv[5];
    if (!range_ok(off, len, d->size)) return fail(e, "PB-DAMAGED", "data");
    if (!len) return 0;
    uint64_t bs = d->bs, per = cap / bs, b = off / bs, bend = (off + len + bs - 1) / bs;
    if (!per) return fail(e, "PB-IO", "memory");
    while (b < bend) {
        uint8_t *chunk = *chunkp;
        uint64_t cb = bend - b < per ? bend - b : per, rs = b * bs, rl = cb * bs;
        if (rl > d->size - rs) rl = d->size - rs;
        TRY(layer_read(iv->L, d->off + rs, chunk, (size_t)rl, e));
        if (rl < cb * bs) memset(chunk + rl, 0, (size_t)(cb * bs - rl));
        for (uint64_t k = 0; k < cb; k++) {
            uint8_t h[32];
            rd_sha256(chunk + k * bs, (size_t)bs, h);
            if (memcmp(h, iv->lv[4].data + 32 * (b + k), 32)) return fail(e, "PB-DAMAGED", "data");
        }
        uint64_t s = off > rs ? off : rs, en = off + len < rs + rl ? off + len : rs + rl;
        TRY(sink(ctx, chunk + (s - rs), (size_t)(en - s), e));
        b += cb;
    }
    return 0;
}




static const char *const archive_sha256[DM_ROMFS_NEED_N] = {
    "8660f1c886811cccfa71c8534e4177b7192616d63be1d6bb9b10eebd8ba51ff7",
    "7d921a22861be3ab05f3083b80a795a3b9dd838507f680431aca4ee32b070e95",
    "106e7d79d661868bf79294ed88dff4a1b8def46f2e7c0bb6102af7b142bca495",
    "effa89857557ad6264e28d874cdc1405406429ab82edb2900dbd5c6f87b656d1",
    "2d676f1958c17f8a3c5fdeb4d6583bf31e2923739bfdb75969374696d8f222ad",
    "bef361010394858b114da118a3e2a1a8be53c5048f284d963a5656ea09efb40f",
    "bccc1bf8f38e612146a39c6ad02ee1b36227e1544666a86ce59bec918d2f66a6",
    "78fa70d76b98b949ee6cc10c81f8b1532615b212b8a263cd0ed93dee8e850806",
    "8bdf545124ae01d38abea4d42cd0ebe93d9637a83a23d2c54584974fe3da75b9",
    "cce480f8735c51eea487929aa9473fc771b5a74ef89b5c96065c90b98768384c",
    "5f233f3920b4369f2659685383b552b1711d5fff85972ca76a6a86fa168126ea",
    "03e7e29ffb5d012370f7f1ac946d8d6e1171b303ecb5e1691d7c93b61442e4ad",
    "dc1783a8cc19416bdc690d0120ce3b07c8d5b86b746dd3e9b2b1844cf54baf00",
    "04d83041940371be3c6f9c5a3c0d027b83dd0463b278931112edcf2bf4f350e1",
    "1acf3b3ec9a27b0e3f97b2a51f41ab36ca7898fbb0ce389d6593826fdc248d56",
    "4d6cf68da7cc64fc2966571f237505e8f1bfda44f2339998644e84cfaf6485b2",
    "b812d95abf8a17d5b32477478132bd7a605bc6da99466f0cc03ac84453d23446",
    "60b1494340b329ae613d12cc1a7a68540259d8dbfd9f489c2bf2ec485d9651c3",
};

static const char *const voice_sha256[DM_ROMFS_VOICE_N] = {
    "923c06818cf9be5f0e77f273f9e2bdabc120542c878d18eee2d89221532c2f7e",
    "009f482dffc8da71ca66038c85080fbc120cc9e61200528495a05e2ef6edd2c1",
    "4d4e25f68286207f15854fb2b53956e13748f86c13db524034b98c0384c19785",
    "3b4f2b849de96a80168733f489c20cd74b26ea82474923a327578d027df7d9bb",
    "6d383e75963a7cf5fdc591712f72495266a3f0fee419e43d015bc946841461f2",
    "4bbfae48dc5ddc4cd0604238cdd40d5df4710c785f638a5f0ee7bb04a03b50c8",
    "b486f56442f2912dc0df46ed420f97acca10af8f7ccc91975503360660b6c04e",
    "3a1ecf502d8c11fafedd9b90b795b95ba2425eac5ba40826ab4bb721cce299f2",
    "4a4254c2fee64d5ea0541205e479a7a55d74575c8f062218da3d87e90bb338e6",
    "efcc0359c0a29eda3e32220e0c0fd77da1c9a2af930bb579a9360aecd17e0f4d",
    "b5f895e2bf706fc6a8f5aae1fb297896cc7c9b79b545fc4e034ea4aad5bca685",
    "62fdd6e5dcb7feb73432b19d942ce4d65048eed5c24dc2b82b799a409e10b1ec",
    "828ceb2ce76d08a491e7ad6ad88021979e2b4aa68f78f1c4ce829c4559bb1c2f",
    "9efe6ddbfdc69ed4c395af71b525971cc571d77daa052aca9ff34debc25d76be",
    "f8f5cd1e64afbb36184f9cd70080f1c055cf6a0ccf6c293467809785c1fef29e",
    "a383bc734fd03bd956a21459489d32f6e4ffb5b9a5a405a4a43bd13a758c7c6f",
    "91d6ea6a8bceaa5e83525048b7da210dc68f8e5e0472957668ee07371f2401c1",
    "af220ae7f75ef1b8b9c87cc4c6811d462d41d7dad6e9950af725fba463f26d22",
    "ee39bbf953eb46b552a735c50ac607605ddfc5e37a333eaed2bc0b2c608017ee",
    "1ac5f58e574159f78137d989744a90d12fd8cedf012b57cba1eb2deeca229b22",
    "9507a8f767d284e9d4f806cd81f868d74fb92ef4d993b0e235ebe7936f1a74fd",
    "2bb1024923b5fcf89ea01e9c4aeaef083c38fc77d083cbcafaa645d76c0f8026",
};

static const struct { const char *unit; int target; const char *name; uint64_t size; const char *sha256; } fixed_files[] = {
    {"exefs", PB_TARGET_EXEFS, "main", 18099792, "55fd909bb7230b002357e5b0d8499c11f06888f345c49e69eb4734df44a27d0e"},
    {"exefs", PB_TARGET_EXEFS, "sdk", 5755393, "1c9f198a91c0bcfee5cf8f5b53fb941895e91334dfb766ce79fc68576797f017"},
    {"exefs", PB_TARGET_EXEFS, "subsdk0", 3409290, "67a38fcf89cd13e040a021eb3d157072f38ebf6680a907874c35c7b231b5d4e2"},
    {"meta", PB_TARGET_DATA, ".build.config", 857, "271447d7af5b89aca7f19b4aabd4a12e0334c5981a074527af7952321eac15a3"},
    {"meta", PB_TARGET_DATA, "build_key.txt", 32, "c91fe6f3ea375baf636c42721229a2ae80af285f01087a1fef8cbb66c88fe193"},
    {"meta", PB_TARGET_DATA, "archive_manifest.psv", 2200, "0ab3bf2b974d7e2449ca2a4aa767178f2fe0cf624a5845a1249eedba16a53777"},
    {"meta", PB_TARGET_DATA, "base_manifest.psv", 29863136, "2b6c006a505d75922440a0f16a20936468af6e2e5b018ae8e8dafbb8ac5a3db1"},
    {"meta", PB_TARGET_DATA, "build_manifest.psv", 30835092, "01458a33bec27f5d831600986024bd3106f9fd1f2ff26a4d44133846cdb57b82"},
    {"meta", PB_TARGET_DATA, "equivalent_manifest.psv", 29863127, "2cfe2df3355157bc86e56d3788b8e437066e8308f543e9da6dbd43ceb497a356"},
};

#define GAME_FILES (sizeof fixed_files / sizeof *fixed_files + DM_ROMFS_NEED_N + DM_ROMFS_VOICE_N)
static pb_file game_files[GAME_FILES];
static pb_contract game_contract;
static pthread_once_t game_once = PTHREAD_ONCE_INIT;
static void game_build(void) {
    size_t k = 0;
    for (size_t i = 0; i < sizeof fixed_files / sizeof *fixed_files; i++)
        game_files[k++] = (pb_file){fixed_files[i].unit, fixed_files[i].target, fixed_files[i].name, fixed_files[i].size, fixed_files[i].sha256};
    for (size_t i = 0; i < DM_ROMFS_NEED_N; i++)
        game_files[k++] = (pb_file){dm_romfs_need[i], PB_TARGET_DATA, dm_romfs_need[i], (uint64_t)dm_romfs_size[i], archive_sha256[i]};
    for (size_t i = 0; i < DM_ROMFS_VOICE_N; i++)
        game_files[k++] = (pb_file){dm_romfs_voice[i], PB_TARGET_DATA, dm_romfs_voice[i], (uint64_t)dm_romfs_voice_size[i], voice_sha256[i]};
    game_contract = (pb_contract){GAME_TITLE, k, game_files, NULL, NULL};
}
const pb_contract *pb_contract_game(void) {
    pthread_once(&game_once, game_build);
    return &game_contract;
}


static int token_ok(const char *s) {
    size_t n = strlen(s);
    if (n == 0 || n >= 64 || !strcmp(s, ".") || !strcmp(s, "..")) return 0;
    for (size_t i = 0; i < n; i++) {
        char c = s[i];
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-')) return 0;
    }
    return 1;
}
static int hex_ok(const char *s, size_t n) {
    if (strlen(s) != n) return 0;
    for (size_t i = 0; i < n; i++) if (!((s[i] >= '0' && s[i] <= '9') || (s[i] >= 'a' && s[i] <= 'f'))) return 0;
    return 1;
}

int pb_contract_parse(const char *text, size_t len, pb_contract *c, pb_error *e) {
    memset(c, 0, sizeof *c);
    if (len == 0 || len > MIB || memchr(text, 0, len) || text[len - 1] != '\n') return fail(e, "PB-IO", "contract");
    char *t = malloc(len + 1);
    size_t lines = 0;
    for (size_t i = 0; i < len; i++) lines += text[i] == '\n';
    pb_file *files = calloc(lines ? lines : 1, sizeof *files);
    if (!t || !files) { free(t); free(files); return fail(e, "PB-IO", "memory"); }
    memcpy(t, text, len);
    t[len] = 0;
    c->owned_text = t;
    c->owned_files = files;
    c->files = files;
    size_t line = 0;
    for (char *s = t, *next; *s; s = next, line++) {
        next = strchr(s, '\n');
        *next++ = 0;
        char *f[7];
        int nf = 0;
        for (char *p = s;; ) {
            if (nf == 7) { nf = 8; break; }
            f[nf++] = p;
            char *tab = strchr(p, '\t');
            if (!tab) break;
            *tab = 0;
            p = tab + 1;
        }
        if (line == 0) {
            if (nf != 2 || strcmp(f[0], "title") || !hex_ok(f[1], 16)) goto bad;
            c->title = strtoull(f[1], NULL, 16);
            continue;
        }
        if (nf != 6 || strcmp(f[0], "file") || !token_ok(f[1]) || !token_ok(f[3]) || !hex_ok(f[5], 64)) goto bad;
        int target = !strcmp(f[2], "exefs") ? PB_TARGET_EXEFS : !strcmp(f[2], "data") ? PB_TARGET_DATA : -1;
        char *end;
        errno = 0;
        unsigned long long size = strtoull(f[4], &end, 10);
        if (target < 0 || *end || errno || f[4][0] < '1' || f[4][0] > '9' || size > (1ull << 40)) goto bad;
        for (size_t i = 0; i < c->n; i++) {
            if (!strcmp(files[i].name, f[3]) && files[i].target == target) goto bad;
            if (!strcmp(files[i].unit, f[1]) && strcmp(files[c->n - 1].unit, f[1])) goto bad;
        }
        files[c->n++] = (pb_file){f[1], target, f[3], size, f[5]};
    }
    if (c->n == 0) goto bad;
    return 0;
bad:
    pb_contract_free(c);
    return fail(e, "PB-IO", "contract line %zu", line + 1);
}

void pb_contract_free(pb_contract *c) {
    free(c->owned_files);
    free(c->owned_text);
    memset(c, 0, sizeof *c);
}




static int addon_of(uint64_t id, uint64_t title) {
    return (id & ~0xFFFull) == (title & ~0xFFFull) + 0x1000 && (id & 0xFFF) != 0;
}

static int probe_package(src_t *f, const keyset *ks, const pb_contract *c, int *kind, char identity[65], pb_error *e) {
    pfs_t p;
    uint8_t *hdr;
    size_t hlen;
    TRY(pfs_read_file(f, &p, &hdr, &hlen, e));
    rd_sha256_ctx sh;
    uint8_t lenle[8], d[32];
    for (int i = 0; i < 8; i++) lenle[i] = (uint8_t)(f->size >> (8 * i));
    rd_sha256_init(&sh);
    rd_sha256_update(&sh, lenle, 8);
    rd_sha256_update(&sh, hdr, hlen);
    rd_sha256_final(&sh, d);
    free(hdr);
    int ncas = 0, r = 0;
    for (uint32_t j = 0; j < p.n; j++) ncas += ends_with(p.e[j].name, ".nca");
    if (!ncas) { free(p.e); return fail(e, "PB-NOTGAME", "container"); }

    if (!ks) { free(p.e); hex(d, 32, identity); *kind = PB_KIND_PACKAGE; return 0; }
    tickets_t *ts = calloc(1, sizeof *ts);
    nca_t *n = malloc(sizeof *n);
    exefs_t x = {0};
    int bases = 0, updates = 0, programs = 0, addons = 0;
    if (!ts || !n) { r = fail(e, "PB-IO", "memory"); goto out; }


    for (uint32_t j = 0; j < p.n && !r && addons >= 0; j++) {
        if (!ends_with(p.e[j].name, ".nca")) continue;
        if ((r = nca_open(n, f, p.data_start + p.e[j].off, p.e[j].size, ks, e))) break;
        addons = addon_of(n->tid, c->title) ? addons + 1 : -1;
    }
    if (r) goto out;
    if (addons > 0) { hex(d, 32, identity); *kind = PB_KIND_ADDON; goto out; }
    for (uint32_t j = 0; j < p.n && !r; j++)
        if (ends_with(p.e[j].name, ".tik")) r = ticket_add(ts, f, p.data_start + p.e[j].off, p.e[j].size, e);
    for (uint32_t j = 0; j < p.n && !r; j++) {
        if (!ends_with(p.e[j].name, ".nca")) continue;
        if ((r = nca_open(n, f, p.data_start + p.e[j].off, p.e[j].size, ks, e))) break;
        if (n->ctype != 0) continue;
        programs++;
        if (n->tid != c->title) continue;
        if (n->is_patch) updates++; else bases++;
        if ((r = nca_setup_key(n, ts, ks, e))) break;
        int si = exefs_find(n);
        if (si >= 0) {
            r = exefs_open(n, si, &x, e);
            free(x.ht);
            x.ht = NULL;
        }
        rd_wipe(&n->ck, sizeof n->ck);
    }
    if (r) goto out;
    if (!bases && !updates) { r = fail(e, "PB-NOTGAME", programs ? "title" : "no program"); goto out; }
    if (bases + updates > 1) { r = fail(e, "PB-NOTGAME", "duplicate"); goto out; }
    hex(d, 32, identity);
    *kind = bases ? PB_KIND_BASE : PB_KIND_UPDATE;
out:
    if (n) rd_wipe(n, sizeof *n);
    free(n);
    free(ts);
    free(p.e);
    return r;
}

int pb_probe(int fd, uint64_t length, const uint8_t *keys, size_t keys_len, const pb_contract *c, int *kind, char identity[65], pb_error *e) {
    identity[0] = 0;
    if (rd_crypto_ready()) return fail(e, "PB-IO", "crypto self test");
    src_t f;
    TRY(src_init(&f, fd, length, e));
    uint8_t head[4] = {0};
    if (length >= 4) TRY(src_read(&f, 0, head, 4, e));
    keyset *ks = NULL;
    int r;
    if (!memcmp(head, "PFS0", 4)) {
        if (keys) {
            if (!(ks = malloc(sizeof *ks))) return fail(e, "PB-IO", "memory");
            if (keys_parse(keys, keys_len, ks)) {
                rd_wipe(ks, sizeof *ks);
                free(ks);
                return fail(e, "PB-KEYMISSING", "header_key");
            }
        }
        r = probe_package(&f, ks, c, kind, identity, e);
    } else if (length == 0 || length > MIB) r = fail(e, "PB-NOTGAME", "file");
    else {
        uint8_t *buf = malloc((size_t)length);
        keyset *own = malloc(sizeof *own);
        r = !buf || !own ? fail(e, "PB-IO", "memory") : src_read(&f, 0, buf, (size_t)length, e);
        if (!r && (keys_parse(buf, (size_t)length, own) || own->lines == 0)) r = fail(e, "PB-NOTGAME", "file");
        if (!r && !key_need(own, "header_key", 32, e)) r = -1;
        if (!r) *kind = PB_KIND_KEYS;
        if (buf) { rd_wipe(buf, (size_t)length); free(buf); }
        if (own) { rd_wipe(own, sizeof *own); free(own); }
    }
    if (ks) { rd_wipe(ks, sizeof *ks); free(ks); }
    return r;
}



typedef struct {
    const pb_file *file;
    uint64_t off;
} located_t;

struct pb_reader {
    src_t src[2];
    int nsrc;
    keyset keys;
    tickets_t tickets;
    nca_t base, upd;
    exefs_t exefs;
    uint64_t exefs_data;
    layer_t layer;
    ivfc_t ivfc;
    const pb_contract *c;
    located_t *loc;
    uint8_t *chunk, *chunk2;
    int cancel;
};

void pb_close(pb_reader *r) {
    if (!r) return;
    free(r->exefs.ht);
    free(r->layer.r);
    free(r->layer.s);
    for (int i = 0; i < 5; i++) free(r->ivfc.lv[i].data);
    free(r->loc);
    free(r->chunk);
    free(r->chunk2);
    rd_wipe(r, sizeof *r);
    free(r);
}

void pb_cancel(pb_reader *r) { __atomic_store_n(&r->cancel, 1, __ATOMIC_RELAXED); }

static int scan_source(pb_reader *r, src_t *f, int *nbase, int *nupd, nca_t *cand, pb_error *e) {
    pfs_t p;
    TRY(pfs_read_file(f, &p, NULL, NULL, e));
    int rc = 0;
    for (uint32_t j = 0; j < p.n && !rc; j++)
        if (ends_with(p.e[j].name, ".tik")) rc = ticket_add(&r->tickets, f, p.data_start + p.e[j].off, p.e[j].size, e);
    for (uint32_t j = 0; j < p.n && !rc; j++) {
        if (!ends_with(p.e[j].name, ".nca")) continue;
        if ((rc = nca_open(cand, f, p.data_start + p.e[j].off, p.e[j].size, &r->keys, e))) break;
        if (cand->ctype != 0 || cand->tid != r->c->title) continue;
        if (cand->is_patch) { if (!(*nupd)++) nca_copy(&r->upd, cand); }
        else if (!(*nbase)++) nca_copy(&r->base, cand);
    }
    free(p.e);
    rd_wipe(cand, sizeof *cand);
    return rc;
}


static int locate_data(pb_reader *r, pb_error *e) {
    ivfc_t *iv = &r->ivfc;
    uint8_t h[0x50], *p = h;
    TRY(ivfc_stream(iv, &r->chunk, CHUNK, 0, sizeof h, sink_copy, &p, e));
    uint64_t f[10];
    for (int i = 0; i < 10; i++) f[i] = le64(h + 8 * i);
    uint64_t lsz = iv->lv[5].size;
    if (f[0] != 0x50) return fail(e, "PB-NOTGAME", "data");
    for (int i = 1; i < 9; i += 2) if (!range_ok(f[i], f[i + 1], lsz)) return fail(e, "PB-DAMAGED", "data");
    if (f[4] > (32u << 20) || f[8] > (64u << 20) || f[4] < 0x18 || f[9] > lsz) return fail(e, "PB-DAMAGED", "data");
    uint8_t *dt = malloc((size_t)f[4]), *ft = malloc((size_t)f[8] ? (size_t)f[8] : 1);
    int rc = !dt || !ft ? fail(e, "PB-IO", "memory") : 0;
    if (!rc) { p = dt; rc = ivfc_stream(iv, &r->chunk, CHUNK, f[3], f[4], sink_copy, &p, e); }
    if (!rc) { p = ft; rc = ivfc_stream(iv, &r->chunk, CHUNK, f[7], f[8], sink_copy, &p, e); }
    uint32_t off = rc ? 0xFFFFFFFF : le32(dt + 8), data_dir = 0xFFFFFFFF;
    for (uint64_t guard = 0; !rc && off != 0xFFFFFFFF; guard++) {
        if (guard > f[4] / 0x18 || !range_ok(off, 0x18, f[4])) { rc = fail(e, "PB-DAMAGED", "data"); break; }
        uint32_t nl = le32(dt + off + 0x14);
        if (!range_ok(off + 0x18ull, nl, f[4])) { rc = fail(e, "PB-DAMAGED", "data"); break; }
        if (nl == 4 && !memcmp(dt + off + 0x18, "data", 4)) { data_dir = off; break; }
        off = le32(dt + off + 4);
    }
    if (!rc && data_dir == 0xFFFFFFFF) rc = fail(e, "PB-NOTGAME", "data");
    off = rc ? 0xFFFFFFFF : le32(dt + data_dir + 12);
    for (uint64_t guard = 0; !rc && off != 0xFFFFFFFF; guard++) {
        if (guard > f[8] / 0x20 || !range_ok(off, 0x20, f[8])) { rc = fail(e, "PB-DAMAGED", "data"); break; }
        const uint8_t *en = ft + off;
        uint32_t nl = le32(en + 0x1C);
        if (!range_ok(off + 0x20ull, nl, f[8]) || nl >= 256) { rc = fail(e, "PB-DAMAGED", "data"); break; }
        for (size_t w = 0; w < r->c->n; w++) {
            const pb_file *cf = &r->c->files[w];
            if (cf->target != PB_TARGET_DATA || strlen(cf->name) != nl || memcmp(en + 0x20, cf->name, nl)) continue;
            uint64_t fo = le64(en + 8), fs = le64(en + 16);
            if (!range_ok(fo, fs, lsz - f[9])) { rc = fail(e, "PB-DAMAGED", "%s", cf->name); break; }
            if (fs != cf->size) { rc = fail(e, "PB-DAMAGED", "%s", cf->name); break; }
            r->loc[w].file = cf;
            r->loc[w].off = f[9] + fo;
        }
        off = le32(en + 4);
    }
    free(dt);
    free(ft);
    return rc;
}


static int locate_exefs(pb_reader *r, pb_error *e) {
    exefs_t *x = &r->exefs;
    uint8_t h16[16], *p = h16;
    if (x->ps < 16) return fail(e, "PB-NOTGAME", "program files");
    TRY(exefs_stream(&r->upd, x, &r->chunk, CHUNK, 0, 16, sink_copy, &p, e));
    if (memcmp(h16, "PFS0", 4)) return fail(e, "PB-NOTGAME", "program files");
    uint64_t hdr = 16 + 24ull * le32(h16 + 4) + le32(h16 + 8);
    if (hdr > MIB || hdr > x->ps) return fail(e, "PB-NOTGAME", "program files");
    uint8_t *h = malloc((size_t)hdr);
    if (!h) return fail(e, "PB-IO", "memory");
    pfs_t pf;
    p = h;
    int rc = exefs_stream(&r->upd, x, &r->chunk, CHUNK, 0, hdr, sink_copy, &p, e);
    if (!rc) rc = pfs_parse(h, (size_t)hdr, x->ps, &pf, e);
    free(h);
    TRY(rc);
    r->exefs_data = pf.data_start;
    for (size_t w = 0; w < r->c->n && !rc; w++) {
        const pb_file *cf = &r->c->files[w];
        if (cf->target != PB_TARGET_EXEFS) continue;
        for (uint32_t i = 0; i < pf.n; i++) {
            if (strcmp(pf.e[i].name, cf->name)) continue;
            if (pf.e[i].size != cf->size) { rc = fail(e, "PB-VERSION", "%s", cf->name); break; }
            r->loc[w].file = cf;
            r->loc[w].off = pf.data_start + pf.e[i].off;
        }
        if (!rc && !r->loc[w].file) rc = fail(e, "PB-VERSION", "%s", cf->name);
    }
    free(pf.e);
    return rc;
}

pb_reader *pb_open(int base_fd, int update_fd, const uint8_t *keys, size_t keys_len, const pb_contract *c, pb_error *e) {
    if (rd_crypto_ready()) { fail(e, "PB-IO", "crypto self test"); return NULL; }
    pb_reader *r = calloc(1, sizeof *r);
    nca_t *cand = malloc(sizeof *cand);
    if (!r || !cand || !(r->loc = calloc(c->n, sizeof *r->loc)) || !(r->chunk = malloc(CHUNK)) || !(r->chunk2 = malloc(CHUNK))) {
        free(cand);
        pb_close(r);
        fail(e, "PB-IO", "memory");
        return NULL;
    }
    r->c = c;
    int rc = 0, nbase = 0, nupd = 0;
    if (keys_parse(keys, keys_len, &r->keys)) rc = fail(e, "PB-KEYMISSING", "header_key");
    int fds[2] = {base_fd, update_fd};
    for (int i = 0; i < 2 && !rc; i++) {
        if (fds[i] < 0 || (i == 1 && fds[1] == fds[0])) continue;
        struct stat sb;
        if (fstat(fds[i], &sb) != 0) { rc = fail(e, "PB-SOURCE", "open"); break; }
        if ((rc = src_init(&r->src[r->nsrc], fds[i], (uint64_t)sb.st_size, e))) break;
        r->nsrc++;
    }

    for (int i = 0; i < r->nsrc && !rc; i++) rc = scan_source(r, &r->src[i], &nbase, &nupd, cand, e);
    free(cand);
    if (!rc && !nbase) rc = fail(e, "PB-NOBASE", "base");
    if (!rc && !nupd) rc = fail(e, "PB-NOUPDATE", "update");
    if (!rc && (nbase > 1 || nupd > 1)) rc = fail(e, "PB-NOTGAME", "duplicate");
    if (!rc) rc = nca_setup_key(&r->base, &r->tickets, &r->keys, e);
    if (!rc) rc = nca_setup_key(&r->upd, &r->tickets, &r->keys, e);
    int si = rc ? -1 : exefs_find(&r->upd);
    if (!rc && si < 0) rc = fail(e, "PB-NOTGAME", "program files");
    if (!rc) rc = exefs_open(&r->upd, si, &r->exefs, e);
    int bsi = -1, usi = -1;
    for (int s = 0; s < 4 && !rc; s++) {
        if (r->base.s[s].present && r->base.s[s].fs_kind == 0 && r->base.s[s].hash_kind == 3 && r->base.s[s].crypt == 3) bsi = s;
        if (r->upd.s[s].present && r->upd.s[s].fs_kind == 0 && r->upd.s[s].hash_kind == 3 && r->upd.s[s].crypt == 4) usi = s;
    }
    if (!rc && (bsi < 0 || usi < 0 || !sec_plain(&r->base.s[bsi]) || !sec_plain(&r->upd.s[usi]))) rc = fail(e, "PB-NOTGAME", "data");
    if (!rc) rc = layer_open(&r->layer, &r->base, bsi, &r->upd, usi, e);
    if (!rc) rc = ivfc_open(&r->ivfc, &r->layer, r->upd.s[usi].fsh, r->upd.key_name, e);
    if (!rc) rc = locate_exefs(r, e);
    if (!rc) rc = locate_data(r, e);
    for (size_t w = 0; w < c->n && !rc; w++)
        if (!r->loc[w].file) rc = fail(e, c->files[w].target == PB_TARGET_EXEFS ? "PB-VERSION" : "PB-NOTGAME", "%s", c->files[w].name);
    rd_wipe(&r->keys, sizeof r->keys);
    if (rc) { pb_close(r); return NULL; }
    return r;
}






typedef struct {
    pthread_mutex_t m;
    pthread_cond_t c;
    pthread_t thread;
    int fd;
    const char *name;
    const uint8_t *p;
    size_t n;
    int stop;
    int failed;
    pb_error err;
    uint64_t written;
    rd_sha256_ctx sha;
} writer_t;

static void *writer_main(void *arg) {
    writer_t *w = arg;
    pthread_mutex_lock(&w->m);
    for (;;) {
        while (!w->p && !w->stop) pthread_cond_wait(&w->c, &w->m);
        if (!w->p) break;
        const uint8_t *p = w->p;
        size_t n = w->n, done = 0;
        int err = 0;
        pthread_mutex_unlock(&w->m);
        rd_sha256_update(&w->sha, p, n);
        while (done < n) {
            ssize_t k = write(w->fd, p + done, n - done);
            if (k < 0 && errno == EINTR) continue;
            if (k <= 0) { err = k < 0 ? errno : EIO; break; }
            done += (size_t)k;
        }
        pthread_mutex_lock(&w->m);
        w->written += done;
        w->p = NULL;
        if (err) { w->failed = 1; fail(&w->err, err == ENOSPC ? "PB-SPACE" : "PB-IO", "%s", w->name); }
        pthread_cond_broadcast(&w->c);
        if (err) break;
    }
    pthread_mutex_unlock(&w->m);
    return NULL;
}

static int writer_start(writer_t *w, int fd, const char *name, pb_error *e) {
    memset(w, 0, sizeof *w);
    w->fd = fd;
    w->name = name;
    rd_sha256_init(&w->sha);
    if (pthread_mutex_init(&w->m, NULL)) return fail(e, "PB-IO", "thread");
    if (pthread_cond_init(&w->c, NULL)) { pthread_mutex_destroy(&w->m); return fail(e, "PB-IO", "thread"); }
    if (pthread_create(&w->thread, NULL, writer_main, w)) {
        pthread_cond_destroy(&w->c);
        pthread_mutex_destroy(&w->m);
        return fail(e, "PB-IO", "thread");
    }
    return 0;
}


static int writer_idle(writer_t *w, pb_error *e) {
    pthread_mutex_lock(&w->m);
    while (w->p && !w->failed) pthread_cond_wait(&w->c, &w->m);
    int r = w->failed ? -1 : 0;
    if (r) *e = w->err;
    pthread_mutex_unlock(&w->m);
    return r;
}

static int writer_put(writer_t *w, const uint8_t *p, size_t n, pb_error *e) {
    pthread_mutex_lock(&w->m);
    while (w->p && !w->failed) pthread_cond_wait(&w->c, &w->m);
    int r = w->failed ? -1 : 0;
    if (r) *e = w->err;
    else { w->p = p; w->n = n; pthread_cond_broadcast(&w->c); }
    pthread_mutex_unlock(&w->m);
    return r;
}

static uint64_t writer_written(writer_t *w) {
    pthread_mutex_lock(&w->m);
    uint64_t n = w->written;
    pthread_mutex_unlock(&w->m);
    return n;
}


static void writer_end(writer_t *w) {
    pthread_mutex_lock(&w->m);
    w->stop = 1;
    pthread_cond_broadcast(&w->c);
    pthread_mutex_unlock(&w->m);
    pthread_join(w->thread, NULL);
    pthread_cond_destroy(&w->c);
    pthread_mutex_destroy(&w->m);
}

typedef struct {
    pb_reader *r;
    writer_t w;
    uint8_t *cur;
    uint64_t unit_done;
    uint64_t next_report;
    pb_progress cb;
    void *ctx;
} out_t;

static int sink_out(void *ctx, const uint8_t *p, size_t n, pb_error *e) {
    out_t *o = ctx;
    if (__atomic_load_n(&o->r->cancel, __ATOMIC_RELAXED)) return fail(e, "PB-CANCEL", "%s", o->w.name);
    TRY(writer_put(&o->w, p, n, e));

    o->cur = o->cur == o->r->chunk ? o->r->chunk2 : o->r->chunk;
    uint64_t at = o->unit_done + writer_written(&o->w);
    if (o->cb && at >= o->next_report) {
        o->next_report = at + MIB;
        o->cb(o->ctx, at, o->w.name);
    }
    return 0;
}

static int sync_dir(const char *dir, pb_error *e) {
    int fd = open(dir, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (fd < 0) return fail(e, "PB-IO", "output folder");
    int r = fsync(fd);
    close(fd);
    return r ? fail(e, "PB-IO", "output folder") : 0;
}


static int is_source(const pb_reader *r, const char *path) {
    struct stat sb;
    if (stat(path, &sb) != 0) return 0;
    for (int i = 0; i < r->nsrc; i++) if (sb.st_dev == r->src[i].dev && sb.st_ino == r->src[i].ino) return 1;
    return 0;
}

static int write_file(pb_reader *r, const located_t *l, const char *dir, out_t *o, pb_error *e) {
    const pb_file *cf = l->file;
    char path[4096], part[4110];
    if (snprintf(path, sizeof path, "%s/%s", dir, cf->name) >= (int)sizeof path) return fail(e, "PB-IO", "output folder");
    snprintf(part, sizeof part, "%s.part", path);
    struct stat sb;
    if (stat(dir, &sb) != 0 || !S_ISDIR(sb.st_mode)) return fail(e, "PB-IO", "output folder");
    if (is_source(r, path) || is_source(r, part)) return fail(e, "PB-INPLACE", "%s", cf->name);

    if (unlink(part) != 0 && errno != ENOENT) return fail(e, "PB-IO", "%s", cf->name);
    int fd = open(part, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0644);
    if (fd < 0) return fail(e, errno == ENOSPC ? "PB-SPACE" : "PB-IO", "%s", cf->name);
    if (writer_start(&o->w, fd, cf->name, e)) { close(fd); unlink(part); return -1; }
    if (o->cb) o->cb(o->ctx, o->unit_done, cf->name);
    o->next_report = o->unit_done + MIB;
    o->cur = r->chunk;
    int rc = cf->target == PB_TARGET_EXEFS
        ? exefs_stream(&r->upd, &r->exefs, &o->cur, CHUNK, l->off, cf->size, sink_out, o, e)
        : ivfc_stream(&r->ivfc, &o->cur, CHUNK, l->off, cf->size, sink_out, o, e);
    if (rc && !strcmp(e->id, "PB-DAMAGED")) snprintf(e->detail, sizeof e->detail, "%s", cf->name);
    if (!rc) rc = writer_idle(&o->w, e);
    writer_end(&o->w);
    if (!rc && fsync(fd) != 0) rc = fail(e, errno == ENOSPC ? "PB-SPACE" : "PB-IO", "%s", cf->name);
    if (close(fd) != 0 && !rc) rc = fail(e, "PB-IO", "%s", cf->name);
    if (!rc) {
        uint8_t d[32];
        char hx[65];
        rd_sha256_final(&o->w.sha, d);
        hex(d, 32, hx);
        if (o->w.written != cf->size || strcmp(hx, cf->sha256)) rc = fail(e, cf->target == PB_TARGET_EXEFS ? "PB-VERSION" : "PB-DAMAGED", "%s", cf->name);
    }
    if (!rc && rename(part, path) != 0) rc = fail(e, "PB-IO", "%s", cf->name);
    if (rc) { unlink(part); return -1; }
    o->unit_done += cf->size;
    return sync_dir(dir, e);
}

int pb_extract(pb_reader *r, const char *unit, const char *exefs_dir, const char *data_dir, pb_progress cb, void *ctx, pb_error *e) {
    out_t o;
    memset(&o, 0, sizeof o);
    o.r = r; o.cb = cb; o.ctx = ctx;
    int any = 0;
    for (size_t w = 0; w < r->c->n; w++) {
        if (strcmp(r->c->files[w].unit, unit)) continue;
        any = 1;
        if (__atomic_load_n(&r->cancel, __ATOMIC_RELAXED)) return fail(e, "PB-CANCEL", "%s", r->c->files[w].name);
        TRY(write_file(r, &r->loc[w], r->c->files[w].target == PB_TARGET_EXEFS ? exefs_dir : data_dir, &o, e));
    }
    return any ? 0 : fail(e, "PB-IO", "unit %s", unit);
}
