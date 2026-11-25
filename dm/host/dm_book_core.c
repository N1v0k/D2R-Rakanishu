
#include "dm_book.h"
#include "dm_sim_ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int dm_bk_key(const char *key, uint8_t *type, uint16_t *idx) {
    if (!key || (key[0] != 'M' && key[0] != 'S') || !key[1]) return -1;
    char *end; long v = strtol(key + 1, &end, 10);
    if (*end || v < 0 || v > 0xffff || key[1] == '-' || key[1] == '+') return -1;
    *type = (uint8_t)key[0]; *idx = (uint16_t)v;
    return 0;
}
static dm_bk_rec *find(dm_bk *b, uint8_t type, uint16_t idx, int make) {
    for (int i = 0; i < b->nrec; i++) if (b->rec[i].type == type && b->rec[i].idx == idx) return &b->rec[i];
    if (!make || b->nrec >= DM_BK_KINDS) return NULL;
    dm_bk_rec *r = &b->rec[b->nrec++]; memset(r, 0, sizeof *r); r->type = type; r->idx = idx;
    return r;
}
static int valid(uint8_t type, int diff) { return (type == 'M' || type == 'S') && diff >= 0 && diff < DM_BK_DIFFS; }
int dm_bk_add(dm_bk *b, uint8_t type, uint16_t idx, int diff, uint32_t n) {
    if (!b || !valid(type, diff)) return -1;
    if (!n) return 0;
    dm_bk_rec *r = find(b, type, idx, 1); if (!r) return -1;
    r->n[diff] = r->n[diff] > UINT32_MAX - n ? UINT32_MAX : r->n[diff] + n; b->gen++;
    return 0;
}
int dm_bk_at_least(dm_bk *b, uint8_t type, uint16_t idx, int diff, uint32_t n) {
    if (!b || !valid(type, diff)) return -1;
    if (!n) return 0;
    dm_bk_rec *r = find(b, type, idx, 1); if (!r) return -1;
    if (r->n[diff] < n) { r->n[diff] = n; b->gen++; }
    return 0;
}
uint32_t dm_bk_count(const dm_bk *b, uint8_t type, uint16_t idx, int diff) {
    if (!b || diff < 0 || diff >= DM_BK_DIFFS) return 0;
    for (int i = 0; i < b->nrec; i++) if (b->rec[i].type == type && b->rec[i].idx == idx) return b->rec[i].n[diff];
    return 0;
}

static uint32_t crc32(const uint8_t *p, size_t n) {
    static uint32_t table[256]; static int ready;
    if (!ready) { for (uint32_t i = 0; i < 256; i++) { uint32_t c = i; for (int k = 0; k < 8; k++) c = c & 1 ? 0xedb88320u ^ (c >> 1) : c >> 1; table[i] = c; } ready = 1; }
    uint32_t c = 0xffffffffu;
    while (n--) c = table[(c ^ *p++) & 0xff] ^ (c >> 8);
    return c ^ 0xffffffffu;
}
static void put32(uint8_t *p, uint32_t v) { memcpy(p, &v, 4); }
static uint32_t get32(const uint8_t *p) { uint32_t v; memcpy(&v, p, 4); return v; }
size_t dm_bk_image(const dm_bk *b, const char *who, uint8_t *buf, size_t cap) {
    size_t len = who ? strlen(who) : 0;
    if (!b || !buf || cap < DM_BK_IMAGE_MAX || !len || len >= DM_BK_OWNER || b->nrec < 0 || b->nrec > DM_BK_KINDS) return 0;
    memset(buf, 0, DM_BK_HEAD + DM_BK_OWNER);
    put32(buf, DM_BK_MAGIC); put32(buf + 4, DM_BK_VERSION); put32(buf + 8, b->flags); put32(buf + 12, (uint32_t)b->nrec);
    memcpy(buf + DM_BK_HEAD, who, len);
    size_t n = DM_BK_HEAD + DM_BK_OWNER;
    memcpy(buf + n, b->rec, (size_t)b->nrec * sizeof *b->rec); n += (size_t)b->nrec * sizeof *b->rec;
    put32(buf + 16, crc32(buf + DM_BK_HEAD, n - DM_BK_HEAD));
    return n;
}
int dm_bk_parse(const uint8_t *image, size_t n, const char *who, dm_bk *out) {
    memset(out, 0, sizeof *out);
    if (n < 8) return DM_BK_BAD_SIZE;
    if (get32(image) != DM_BK_MAGIC) return DM_BK_BAD_MAGIC;
    uint32_t v = get32(image + 4);
    if (v > DM_BK_VERSION) return DM_BK_NEWER;
    if (v != DM_BK_VERSION) return DM_BK_BAD_VERSION;
    if (n < DM_BK_HEAD + DM_BK_OWNER) return DM_BK_BAD_SIZE;
    if (get32(image + 16) != crc32(image + DM_BK_HEAD, n - DM_BK_HEAD)) return DM_BK_BAD_CRC;
    uint32_t count = get32(image + 12);
    if (count > DM_BK_KINDS) return DM_BK_BAD_COUNT;
    if (n != DM_BK_HEAD + DM_BK_OWNER + count * sizeof(dm_bk_rec)) return DM_BK_BAD_SIZE;
    const uint8_t *owner = image + DM_BK_HEAD;
    size_t len = strnlen((const char *)owner, DM_BK_OWNER);
    if (!len || len >= DM_BK_OWNER) return DM_BK_BAD_OWNER;
    for (size_t i = len; i < DM_BK_OWNER; i++) if (owner[i]) return DM_BK_BAD_OWNER;
    const dm_bk_rec *rec = (const dm_bk_rec *)(owner + DM_BK_OWNER);
    for (uint32_t i = 0; i < count; i++) {
        dm_bk_rec r; memcpy(&r, &rec[i], sizeof r);
        if ((r.type != 'M' && r.type != 'S') || r.pad) { memset(out, 0, sizeof *out); return DM_BK_BAD_RECORD; }
        for (int j = 0; j < out->nrec; j++) if (out->rec[j].type == r.type && out->rec[j].idx == r.idx) { memset(out, 0, sizeof *out); return DM_BK_BAD_RECORD; }
        out->rec[out->nrec++] = r;
    }
    if (strcmp((const char *)owner, who ? who : "")) { memset(out, 0, sizeof *out); return DM_BK_FOREIGN; }
    out->flags = get32(image + 8);
    return 0;
}
int dm_bk_text(const dm_bk *b, int mode, char *out, size_t cap) {
    if (!b || !out || !cap) return -1;
    size_t n = 0; int w = snprintf(out, cap, "book\t%d\t%u\n", mode, b->flags);
    if (w < 0 || (size_t)w >= cap) { out[0] = 0; return -2; }
    n = (size_t)w;
    for (int i = 0; i < b->nrec; i++) {
        const dm_bk_rec *r = &b->rec[i];
        if (!r->n[0] && !r->n[1] && !r->n[2]) continue;
        w = snprintf(out + n, cap - n, "kill\t%c%u\t%u\t%u\t%u\n", r->type, r->idx, r->n[0], r->n[1], r->n[2]);
        if (w < 0 || (size_t)w >= cap - n) { out[0] = 0; return -2; }
        n += (size_t)w;
    }
    return (int)n;
}

int dm_bk_kind_of(const struct dm_sim_data *d, const dm_bk_seen *s, uint8_t *type, uint16_t *idx) {
    if (!d || !s) return -1;
    char key[16];
    int cls = s->cls;
    if (s->su >= 0) {
        int listed = 0;
        for (int i = 0; i < d->nsu; i++) if (d->su[i].idx == s->su) listed = 1;

        if (!listed && cls < 0 && s->name) for (int i = 0; i < d->nboss; i++) {
            int m = d->boss[i].mon;
            if (m >= 0 && m < d->nmon && !strcmp(d->mon[m].namekey, s->name)) { cls = m; break; }
        }
        if (!listed && cls < 0) return -1;
    }
    if (dm_sim_kind_key(d, cls, s->su, key, sizeof key)) return -1;
    return dm_bk_key(key, type, idx);
}

static void sum(const struct dm_sim_data *d, const dm_bk_seen *s, int n, dm_bk *into, int *skipped) {
    for (int i = 0; i < n; i++) {
        uint8_t type; uint16_t idx;
        if (!s[i].n) continue;
        if (s[i].diff < 0 || s[i].diff >= DM_BK_DIFFS || dm_bk_kind_of(d, &s[i], &type, &idx)) { (*skipped)++; continue; }
        dm_bk_add(into, type, idx, s[i].diff, s[i].n);
    }
}
int dm_bk_backfill(dm_bk *b, const struct dm_sim_data *d, const dm_bk_seen *board, int nboard, const dm_bk_seen *fights, int nfights, int *skipped) {
    int skip = 0;
    if (!b || !d) return -1;
    dm_bk *a = calloc(2, sizeof *a);
    if (!a) return -1;
    sum(d, board, nboard, &a[0], &skip);
    sum(d, fights, nfights, &a[1], &skip);
    for (int k = 0; k < 2; k++) for (int i = 0; i < a[k].nrec; i++)
        for (int diff = 0; diff < DM_BK_DIFFS; diff++) dm_bk_at_least(b, a[k].rec[i].type, a[k].rec[i].idx, diff, a[k].rec[i].n[diff]);
    free(a);
    b->flags |= DM_BK_F_BACKFILLED; b->gen++;
    if (skipped) *skipped = skip;
    return 0;
}
