
#include "dm_looks_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int grow(void **p, int *cap, int need, size_t sz) {
    if (need <= *cap) return 0;
    int c = *cap ? *cap * 2 : 16;
    while (c < need) c *= 2;
    void *n = realloc(*p, (size_t)c * sz);
    if (!n) return -1;
    *p = n; *cap = c;
    return 0;
}


static int base_cmp(const void *a, const void *b) {
    uint32_t x = ((const dm_look_base *)a)->code, y = ((const dm_look_base *)b)->code;
    return x < y ? -1 : x > y;
}
int dm_looks_table_parse(const char *text, dm_look_table *t) {
    t->b = NULL; t->n = 0;
    int cap = 0;
    for (const char *p = text; p && *p;) {
        const char *e = strchr(p, '\n'); size_t len = e ? (size_t)(e - p) : strlen(p);
        char line[160];
        if (len && p[0] != '#') {
            if (len >= sizeof line) goto bad;
            memcpy(line, p, len); line[len] = 0;
            char code[8], pic[8], icon[64]; unsigned g, w, h, st; int cls;
            if (sscanf(line, "%4[^\t]\t%u\t%u\t%u\t%d\t%u\t%4[^\t]\t%63[^\t\r]", code, &g, &w, &h, &cls, &st, pic, icon) != 8 || strlen(icon) >= sizeof t->b->icon || !g || g >= DM_LG_COUNT || !w || !h || w > 4 || h > 4 || cls < -1 || cls > 7) goto bad;
            if (grow((void **)&t->b, &cap, t->n + 1, sizeof *t->b)) goto bad;
            dm_look_base *b = &t->b[t->n++];
            b->code = 0x20202020; memcpy(&b->code, code, strlen(code));
            b->group = (uint8_t)g; b->w = (uint8_t)w; b->h = (uint8_t)h; b->cls = (int8_t)cls; b->stance = st;
            b->pic = 0x20202020; memcpy(&b->pic, pic, strlen(pic));
            memcpy(b->icon, icon, strlen(icon) + 1);
        }
        p = e ? e + 1 : NULL;
    }
    if (!t->n) goto bad;
    qsort(t->b, (size_t)t->n, sizeof *t->b, base_cmp);
    for (int i = 1; i < t->n; i++) if (t->b[i].code == t->b[i - 1].code) goto bad;
    for (int i = 0; i < t->n; i++) {
        const dm_look_base *a = &t->b[i], *p = dm_looks_table_find(t, a->pic);
        if (!p || p->pic != p->code || p->group != a->group || p->w != a->w || p->h != a->h || p->cls != a->cls || p->stance != a->stance) goto bad;
    }
    return 0;
bad:
    dm_looks_table_free(t);
    return -1;
}
void dm_looks_table_free(dm_look_table *t) { free(t->b); t->b = NULL; t->n = 0; }
const dm_look_base *dm_looks_table_find(const dm_look_table *t, uint32_t code) {
    dm_look_base k = { .code = code };
    return t->n ? bsearch(&k, t->b, (size_t)t->n, sizeof *t->b, base_cmp) : NULL;
}
int dm_looks_group(const dm_look_table *t, uint32_t code) { const dm_look_base *b = dm_looks_table_find(t, code); return b ? b->group : DM_LG_NONE; }

int dm_looks_fits(const dm_look_table *t, uint32_t target, uint32_t donor, int pclass) {
    const dm_look_base *a = dm_looks_table_find(t, target), *d = dm_looks_table_find(t, donor);
    if (!a || !d || a->group != d->group || a->w != d->w || a->h != d->h) return 0;
    if (a->group == DM_LG_WEAPON && a->stance != d->stance) return 0;
    return d->cls < 0 || d->cls == pclass;
}
int dm_looks_invisible_ok(const dm_look_table *t, uint32_t target) {
    int g = dm_looks_group(t, target);
    return g == DM_LG_HELM || g == DM_LG_SHIELD;
}
int dm_looks_price(int item_level, int kind) {
    if (item_level < 1) item_level = 1;
    switch (kind) {
    case DM_LOOK_BASE: return item_level * 100 < 500 ? 500 : item_level * 100;
    case DM_LOOK_UNIQUE: case DM_LOOK_SET: return item_level * 500;
    case DM_LOOK_INVISIBLE: return item_level * 100;
    default: return 0;
    }
}

int dm_looks_item_eq(const dm_look_item *a, const dm_look_item *b) {
    return a->fp == b->fp && a->code == b->code && a->quality == b->quality && a->row == b->row;
}
uint64_t dm_looks_sig_add(uint64_t h, const dm_look_item *it, uint32_t unit, int mode, int identified) {
    uint32_t v[7] = { it->fp, it->code, it->quality, (uint32_t)it->row, unit, (uint32_t)mode, identified != 0 };
    if (!h) h = 1469598103934665603ull;
    for (int i = 0; i < 7; i++) for (int k = 0; k < 4; k++) { h ^= (uint8_t)(v[i] >> (8 * k)); h *= 1099511628211ull; }
    return h;
}
static int item_cmp(const dm_look_item *a, const dm_look_item *b) {
    if (a->fp != b->fp) return a->fp < b->fp ? -1 : 1;
    if (a->code != b->code) return a->code < b->code ? -1 : 1;
    if (a->quality != b->quality) return a->quality < b->quality ? -1 : 1;
    return a->row < b->row ? -1 : a->row > b->row;
}
static int look_cmp(dm_look a, dm_look b) { return a.kind != b.kind ? (a.kind < b.kind ? -1 : 1) : a.id < b.id ? -1 : a.id > b.id; }

void dm_looks_store_free(dm_looks_store *s) { free(s->coll); free(s->map); free(s->pend); free(s->chars); memset(s, 0, sizeof *s); }


static int coll_pos(const dm_looks_store *s, dm_look l, int *found) {
    int lo = 0, hi = s->ncoll;
    while (lo < hi) { int m = (lo + hi) / 2, c = look_cmp(s->coll[m], l); if (!c) { *found = 1; return m; } if (c < 0) lo = m + 1; else hi = m; }
    *found = 0; return lo;
}
int dm_looks_coll_add(dm_looks_store *s, dm_look l) {
    int f, i = coll_pos(s, l, &f);
    if (f) return 0;
    if (grow((void **)&s->coll, &s->capcoll, s->ncoll + 1, sizeof *s->coll)) return -1;
    memmove(&s->coll[i + 1], &s->coll[i], sizeof *s->coll * (size_t)(s->ncoll - i));
    s->coll[i] = l; s->ncoll++; s->rev++;
    return 1;
}
int dm_looks_coll_has(const dm_looks_store *s, dm_look l) { int f; coll_pos(s, l, &f); return f; }


static int map_pos(const dm_looks_store *s, const dm_look_item *it, int *found) {
    int lo = 0, hi = s->nmap;
    while (lo < hi) { int m = (lo + hi) / 2, c = item_cmp(&s->map[m].it, it); if (!c) { *found = 1; return m; } if (c < 0) lo = m + 1; else hi = m; }
    *found = 0; return lo;
}
static int map_set(dm_looks_store *s, const dm_look_item *it, dm_look l, uint8_t dye) {
    int f, i = map_pos(s, it, &f);
    if (f) { s->map[i].look = l; s->map[i].dye = dye; return 0; }
    if (grow((void **)&s->map, &s->capmap, s->nmap + 1, sizeof *s->map)) return -1;
    memmove(&s->map[i + 1], &s->map[i], sizeof *s->map * (size_t)(s->nmap - i));
    s->map[i].it = *it; s->map[i].look = l; s->map[i].dye = dye; s->nmap++;
    return 0;
}
dm_look dm_looks_current(const dm_looks_store *s, const dm_look_item *it) {
    for (int i = s->npend - 1; i >= 0; i--) if (dm_looks_item_eq(&s->pend[i].it, it)) return s->pend[i].look;
    int f, i = map_pos(s, it, &f);
    return f ? s->map[i].look : (dm_look){ DM_LOOK_ORIGINAL, 0 };
}
int dm_looks_current_dye(const dm_looks_store *s, const dm_look_item *it) {
    for (int i = s->npend - 1; i >= 0; i--) if (dm_looks_item_eq(&s->pend[i].it, it)) return s->pend[i].dye;
    int f, i = map_pos(s, it, &f);
    return f ? s->map[i].dye : 0;
}
int dm_looks_pay_choose(dm_looks_store *s, const dm_look_item *it, dm_look l, int dye, uint64_t seq, int (*pay)(void *ctx, int *dye), void *ctx) {

    if (grow((void **)&s->pend, &s->cappend, s->npend + 1, sizeof *s->pend)) return -1;
    if (pay && !pay(ctx, &dye)) return 0;
    int k = 0;
    for (int i = 0; i < s->npend; i++) if (!dm_looks_item_eq(&s->pend[i].it, it)) s->pend[k++] = s->pend[i];
    s->npend = k;
    s->pend[s->npend++] = (dm_look_pending){ *it, l, (uint8_t)dye, seq };
    s->rev++;
    return 1;
}
int dm_looks_choose(dm_looks_store *s, const dm_look_item *it, dm_look l, int dye, uint64_t seq) {
    return dm_looks_pay_choose(s, it, l, dye, seq, NULL, NULL) < 0 ? -1 : 0;
}
int dm_looks_busy(const dm_looks_store *s, const dm_look_item *it) {
    for (int i = 0; i < s->npend; i++) if (s->pend[i].seq && dm_looks_item_eq(&s->pend[i].it, it)) return 1;
    return 0;
}
static int pend_eq(const dm_look_pending *a, const dm_look_pending *b) {
    return dm_looks_item_eq(&a->it, &b->it) && a->look.kind == b->look.kind && a->look.id == b->look.id && a->dye == b->dye && a->seq == b->seq;
}
int dm_looks_promotable(const dm_looks_store *s, uint64_t work_seq, dm_look_pending *out, int max) {
    int n = 0;
    for (int i = 0; i < s->npend && n < max; i++) {
        const dm_look_pending *p = &s->pend[i];
        if (!p->seq || p->seq < work_seq) out[n++] = *p;
    }
    return n;
}
int dm_looks_settle(dm_looks_store *s, const dm_look_pending *done, int n) {
    int err = 0;
    for (int d = 0; d < n; d++) {
        if (map_set(s, &done[d].it, done[d].look, done[d].dye)) { err = -1; continue; }
        int k = 0;
        for (int i = 0; i < s->npend; i++) if (!pend_eq(&s->pend[i], &done[d])) s->pend[k++] = s->pend[i];
        s->npend = k;
    }
    if (n) s->rev++;
    return err;
}
int dm_looks_char_set(dm_looks_store *s, const dm_look_char *c) {
    for (int i = 0; i < s->nchars; i++) if (!strcmp(s->chars[i].name, c->name)) { s->chars[i] = *c; return 0; }
    if (grow((void **)&s->chars, &s->capchars, s->nchars + 1, sizeof *s->chars)) return -1;
    s->chars[s->nchars++] = *c;
    return 0;
}
const dm_look_char *dm_looks_char_get(const dm_looks_store *s, const char *name) {
    for (int i = 0; i < s->nchars; i++) if (!strcmp(s->chars[i].name, name)) return &s->chars[i];
    return NULL;
}



static const struct { char c; const char *b; } HUF[] = {
    { ' ', "10" }, { '0', "11111011" }, { '1', "1111100" }, { '2', "001100" }, { '3', "1101101" }, { '4', "11111010" }, { '5', "00010110" },
    { '6', "1101111" }, { '7', "01111" }, { '8', "000100" }, { '9', "01110" }, { 'a', "11110" }, { 'b', "0101" }, { 'c', "01000" }, { 'd', "110001" },
    { 'e', "110000" }, { 'f', "010011" }, { 'g', "11010" }, { 'h', "00011" }, { 'i', "1111110" }, { 'j', "000101110" }, { 'k', "010010" },
    { 'l', "11101" }, { 'm', "01101" }, { 'n', "001101" }, { 'o', "1111111" }, { 'p', "11001" }, { 'q', "11011001" }, { 'r', "11100" },
    { 's', "0010" }, { 't', "01100" }, { 'u', "00001" }, { 'v', "1101110" }, { 'w', "00000" }, { 'x', "00111" }, { 'y', "0001010" }, { 'z', "11011000" } };
typedef struct { const uint8_t *d; int nbits, at; } bitr;
static int bit(bitr *r) { if (r->at >= r->nbits) { r->at = r->nbits + 1; return 0; } int b = r->d[r->at >> 3] >> (r->at & 7) & 1; r->at++; return b; }
static uint32_t bits(bitr *r, int n) { uint32_t v = 0; for (int i = 0; i < n; i++) v |= (uint32_t)bit(r) << i; return v; }
int dm_looks_bits_item(const uint8_t *b, int len, dm_look_item *it, int *identified) {
    bitr r = { b, len * 8, 0 };
    uint32_t flags = bits(&r, 32);
    if (flags & 1u << 16) return -1;
    bits(&r, 3 + 3 + 4 + 4 + 4 + 3);
    char code[4];
    for (int k = 0; k < 4; k++) {
        char cur[12]; int n = 0, hit = -1;
        while (hit < 0 && n < 9) {
            cur[n++] = (char)('0' + bit(&r)); cur[n] = 0;
            for (size_t h = 0; h < sizeof HUF / sizeof HUF[0]; h++) if (!strcmp(HUF[h].b, cur)) { hit = (int)h; break; }
        }
        if (hit < 0) return -1;
        code[k] = HUF[hit].c;
    }
    memset(it, 0, sizeof *it); memcpy(&it->code, code, 4); it->row = -1; it->quality = 2;
    *identified = (flags >> 4) & 1;
    if (flags & 1u << 21) { bits(&r, 1); return r.at > r.nbits ? -1 : 0; }
    bits(&r, 3);
    it->fp = bits(&r, 32);
    bits(&r, 7);
    it->quality = (uint8_t)bits(&r, 4);
    if (bit(&r)) bits(&r, 3);
    if (bit(&r)) bits(&r, 11);
    if (it->quality == 5 || it->quality == 7) it->row = (int32_t)bits(&r, 12);
    return r.at > r.nbits ? -1 : 0;
}
dm_look dm_looks_unlocks(const dm_look_table *t, const dm_look_item *it, int identified, int own_look) {
    if (!identified || !dm_looks_table_find(t, it->code)) return (dm_look){ DM_LOOK_ORIGINAL, 0 };
    if (own_look && it->quality == 7) return (dm_look){ DM_LOOK_UNIQUE, (uint32_t)it->row };
    if (own_look && it->quality == 5) return (dm_look){ DM_LOOK_SET, (uint32_t)it->row };
    return (dm_look){ DM_LOOK_BASE, dm_looks_table_find(t, it->code)->pic };
}
int dm_looks_offer(const dm_looks_store *s, const dm_look_table *t, uint32_t target, int pclass, uint32_t (*base_of)(dm_look), dm_look *out, int max) {
    const dm_look_base *own = dm_looks_table_find(t, target);
    int n = 0;
    for (int i = 0; own && i < s->ncoll && n < max; i++) {
        dm_look l = s->coll[i];
        uint32_t b = l.kind == DM_LOOK_BASE ? l.id : base_of ? base_of(l) : 0;
        if (!b || !dm_looks_fits(t, target, b, pclass)) continue;
        if (l.kind == DM_LOOK_BASE) {
            l.id = dm_looks_table_find(t, b)->pic;
            if (l.id == own->pic) continue;
            int k = 0; while (k < n && !(out[k].kind == DM_LOOK_BASE && out[k].id == l.id)) k++;
            if (k < n) continue;
        }
        out[n++] = l;
    }
    return n;
}





#define TAG(a, b, c, d) ((uint32_t)(a) | (uint32_t)(b) << 8 | (uint32_t)(c) << 16 | (uint32_t)(d) << 24)
enum { T_COLL = TAG('C', 'O', 'L', 'L'), T_MAP = TAG('M', 'A', 'P', ' '), T_CHAR = TAG('C', 'H', 'A', 'R') };
static uint64_t fnv(const uint8_t *d, size_t n) { uint64_t h = 1469598103934665603ull; for (size_t i = 0; i < n; i++) { h ^= d[i]; h *= 1099511628211ull; } return h; }

typedef struct { uint8_t *d; size_t n, cap; int err; } wbuf;
static void put(wbuf *w, const void *p, size_t n) {
    if (w->err) return;
    if (w->n + n > w->cap) { size_t c = w->cap ? w->cap * 2 : 256; while (c < w->n + n) c *= 2; uint8_t *x = realloc(w->d, c); if (!x) { w->err = 1; return; } w->d = x; w->cap = c; }
    memcpy(w->d + w->n, p, n); w->n += n;
}
static void put32(wbuf *w, uint32_t v) { uint8_t b[4] = { (uint8_t)v, (uint8_t)(v >> 8), (uint8_t)(v >> 16), (uint8_t)(v >> 24) }; put(w, b, 4); }
static void put8(wbuf *w, uint8_t v) { put(w, &v, 1); }
static void put_look(wbuf *w, dm_look l) { put8(w, l.kind); put32(w, l.id); }
static void put_item(wbuf *w, const dm_look_item *it) { put32(w, it->fp); put32(w, it->code); put8(w, it->quality); put32(w, (uint32_t)it->row); }
static size_t sect_begin(wbuf *w, uint32_t tag) { put32(w, tag); put32(w, 0); return w->n; }
static void sect_end(wbuf *w, size_t at) { if (w->err) return; uint32_t len = (uint32_t)(w->n - at); for (int i = 0; i < 4; i++) w->d[at - 4 + i] = (uint8_t)(len >> (8 * i)); }

size_t dm_looks_encode(const dm_looks_store *s, uint8_t **out) {
    wbuf w = { 0 };
    int ver = 1;
    for (int i = 0; i < s->nmap; i++) if (s->map[i].dye) ver = 2;
    put(&w, "DMLK", 4); put8(&w, (uint8_t)ver); put8(&w, 0); put8(&w, 0); put8(&w, 0);
    size_t a = sect_begin(&w, T_COLL); put32(&w, (uint32_t)s->ncoll);
    for (int i = 0; i < s->ncoll; i++) put_look(&w, s->coll[i]);
    sect_end(&w, a);
    a = sect_begin(&w, T_MAP); put32(&w, (uint32_t)s->nmap);
    for (int i = 0; i < s->nmap; i++) { put_item(&w, &s->map[i].it); put_look(&w, s->map[i].look); if (ver > 1) put8(&w, s->map[i].dye); }
    sect_end(&w, a);
    a = sect_begin(&w, T_CHAR); put32(&w, (uint32_t)s->nchars);
    for (int i = 0; i < s->nchars; i++) {
        put(&w, s->chars[i].name, sizeof s->chars[i].name);
        for (int k = 0; k < DM_LOOK_PLACES; k++) { const dm_look_place *p = &s->chars[i].p[k]; put32(&w, p->code); put8(&w, p->quality); put32(&w, (uint32_t)p->row); put_look(&w, p->look); }
    }
    sect_end(&w, a);
    uint64_t h = fnv(w.d, w.n);
    for (int i = 0; i < 8; i++) put8(&w, (uint8_t)(h >> (8 * i)));
    if (w.err) { free(w.d); *out = NULL; return 0; }
    *out = w.d;
    return w.n;
}

typedef struct { const uint8_t *d; size_t n, at; int err; } rbuf;
static uint32_t get32(rbuf *r) { if (r->at + 4 > r->n) { r->err = 1; return 0; } const uint8_t *p = r->d + r->at; r->at += 4; return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24; }
static uint8_t get8(rbuf *r) { if (r->at + 1 > r->n) { r->err = 1; return 0; } return r->d[r->at++]; }
static dm_look get_look(rbuf *r) { dm_look l; l.kind = get8(r); l.id = get32(r); if (l.kind > DM_LOOK_INVISIBLE) r->err = 1; return l; }
static dm_look_item get_item(rbuf *r) { dm_look_item it; it.fp = get32(r); it.code = get32(r); it.quality = get8(r); it.row = (int32_t)get32(r); return it; }

int dm_looks_decode(const uint8_t *d, size_t n, dm_looks_store *s) {
    memset(s, 0, sizeof *s);
    if (n < 16 || memcmp(d, "DMLK", 4)) return DM_LOOKS_CORRUPT;
    uint64_t h = 0;
    for (int i = 0; i < 8; i++) h |= (uint64_t)d[n - 8 + i] << (8 * i);
    if (fnv(d, n - 8) != h) return DM_LOOKS_CORRUPT;
    if (d[4] > DM_LOOKS_VERSION) return DM_LOOKS_NEWER;
    if (d[4] < 1) return DM_LOOKS_CORRUPT;
    rbuf r = { d, n - 8, 8, 0 };
    int seen = 0;
    while (r.at < r.n && !r.err) {
        uint32_t tag = get32(&r), len = get32(&r);
        if (r.err || len > r.n - r.at) { r.err = 1; break; }
        rbuf b = { d + r.at, len, 0, 0 };
        uint32_t cnt = get32(&b);
        if (tag == T_COLL && !(seen & 1)) {
            seen |= 1;
            if (cnt > len / 5) { r.err = 1; break; }
            for (uint32_t i = 0; i < cnt && !b.err; i++) { dm_look l = get_look(&b); if (!b.err && dm_looks_coll_add(s, l) < 0) { dm_looks_store_free(s); return DM_LOOKS_NOMEM; } }
        } else if (tag == T_MAP && !(seen & 2)) {
            seen |= 2;
            if (cnt > len / (d[4] > 1 ? 19 : 18)) { r.err = 1; break; }
            for (uint32_t i = 0; i < cnt && !b.err; i++) {
                dm_look_item it = get_item(&b); dm_look l = get_look(&b); uint8_t dye = d[4] > 1 ? get8(&b) : 0;
                if (dye > DM_DYE_COLOURS) b.err = 1;
                if (!b.err && map_set(s, &it, l, dye)) { dm_looks_store_free(s); return DM_LOOKS_NOMEM; }
            }
        } else if (tag == T_CHAR && !(seen & 4)) {
            seen |= 4;
            if (cnt > len / 24) { r.err = 1; break; }
            for (uint32_t i = 0; i < cnt && !b.err; i++) {
                dm_look_char c; memset(&c, 0, sizeof c);
                if (b.at + sizeof c.name > b.n) { b.err = 1; break; }
                memcpy(c.name, b.d + b.at, sizeof c.name); b.at += sizeof c.name;
                if (c.name[sizeof c.name - 1]) b.err = 1;
                for (int k = 0; k < DM_LOOK_PLACES; k++) { dm_look_place *p = &c.p[k]; p->code = get32(&b); p->quality = get8(&b); p->row = (int32_t)get32(&b); p->look = get_look(&b); }
                if (!b.err && dm_looks_char_set(s, &c)) { dm_looks_store_free(s); return DM_LOOKS_NOMEM; }
            }
        } else r.err = 1;
        if (b.err || b.at != b.n) r.err = 1;
        r.at += len;
    }
    if (r.err || seen != 7) { dm_looks_store_free(s); return DM_LOOKS_CORRUPT; }
    s->rev = 1;
    return DM_LOOKS_OK;
}

int dm_looks_merge(dm_looks_store *dev, const dm_looks_store *in) {
    for (int i = 0; i < in->ncoll; i++) if (dm_looks_coll_add(dev, in->coll[i]) < 0) return -1;
    for (int i = 0; i < in->nmap; i++) {
        int f; map_pos(dev, &in->map[i].it, &f);
        if (!f && map_set(dev, &in->map[i].it, in->map[i].look, in->map[i].dye)) return -1;
    }
    for (int i = 0; i < in->nchars; i++) if (!dm_looks_char_get(dev, in->chars[i].name) && dm_looks_char_set(dev, &in->chars[i])) return -1;
    dev->rev++;
    return 0;
}


int dm_dyes_parse(const char *text, dm_dye_table *t) {
    t->g = NULL; t->n = 0;
    int cap = 0;
    for (const char *p = text; p && *p;) {
        const char *e = strchr(p, '\n'); size_t len = e ? (size_t)(e - p) : strlen(p);
        char line[160];
        if (len && p[0] != '#') {
            if (len >= sizeof line) goto bad;
            memcpy(line, p, len); line[len] = 0;
            char code[8], icon[64]; unsigned colour, grade;
            if (sscanf(line, "%4[^\t]\t%u\t%u\t%63[^\t\r]", code, &colour, &grade, icon) != 4 || colour >= DM_DYE_COLOURS || grade > 9 || strlen(icon) >= sizeof t->g->icon) goto bad;
            if (grow((void **)&t->g, &cap, t->n + 1, sizeof *t->g)) goto bad;
            dm_dye_gem *g = &t->g[t->n++];
            g->code = 0x20202020; memcpy(&g->code, code, strlen(code));
            g->colour = (uint8_t)colour; g->grade = (uint8_t)grade; memcpy(g->icon, icon, strlen(icon) + 1);
            for (int i = 0; i < t->n - 1; i++) if (t->g[i].code == g->code) goto bad;
        }
        p = e ? e + 1 : NULL;
    }
    if (!t->n) goto bad;
    return 0;
bad:
    dm_dyes_free(t);
    return -1;
}
void dm_dyes_free(dm_dye_table *t) { free(t->g); t->g = NULL; t->n = 0; }
const dm_dye_gem *dm_dyes_find(const dm_dye_table *t, uint32_t code) {
    for (int i = 0; i < t->n; i++) if (t->g[i].code == code) return &t->g[i];
    return NULL;
}

int dm_dye_where(int fg, int pg) {
    return (fg >= 1 && fg <= 8 ? DM_DYE_FIGURE : 0) | (pg >= 0 && pg <= 8 && !(1u << pg & 0x19u) ? DM_DYE_PICTURE : 0);
}
int dm_dye_source(int dye, int look_kind) {
    if (dye) return DM_DYE_CHOSEN;
    if (look_kind == DM_LOOK_UNIQUE || look_kind == DM_LOOK_SET) return DM_DYE_LOOK_ROW;
    if (look_kind == DM_LOOK_BASE) return DM_DYE_UNCOLOURED;
    return DM_DYE_GAME;
}
int dm_dyes_offer_at(const dm_dye_offer *o, int n, int dye) {
    for (int i = 0; i < n; i++) if (o[i].dye == dye) return i;
    return -1;
}
int dm_dyes_offer(const dm_dye_table *t, const uint32_t *codes, int n, dm_dye_offer *out, int max) {
    int m = 0;
    for (int i = 0; i < t->n; i++) {
        int best = -1;
        for (int k = 0; k < n; k++) {
            const dm_dye_gem *g = dm_dyes_find(t, codes[k]);
            if (g && g->colour == t->g[i].colour && (best < 0 || g->grade < t->g[best].grade)) best = (int)(g - t->g);
        }
        if (best < 0) continue;
        int seen = 0;
        for (int j = 0; j < m; j++) if (out[j].dye == t->g[i].colour + 1) seen = 1;
        if (!seen && m < max) out[m++] = (dm_dye_offer){ (uint8_t)(t->g[i].colour + 1), best };
    }
    return m;
}


int dm_looks_page_cap(int w, int h, int colour_rows) {
    if (w < 1 || h < 1 || w > DM_LOOKS_GRID || h > DM_LOOKS_GRID) return 0;
    int cap = DM_LOOKS_GRID / w * ((colour_rows ? DM_LOOKS_DYE_ROW : DM_LOOKS_GRID) / h);
    return cap > DM_LOOKS_TILES ? DM_LOOKS_TILES : cap;
}
int dm_looks_page_index(int cap, int page, int tile, int n) {
    if (cap < 1 || page < 0 || tile < 0 || tile >= cap) return -1;
    int i = page * cap + tile;
    return i < n ? i : -1;
}


int dm_looks_apply_gate_step(dm_looks_apply_gate *g, uint64_t state, int shown, int pressed) {
    int fresh = shown && g->shown && g->state == state;
    g->state = state; g->shown = shown;
    return fresh ? pressed != 0 : -1;
}


void dm_looks_dress(const uint8_t *u, const uint8_t *d, uint32_t dclass, uint8_t dquality, int32_t drow, uint8_t *fu, size_t nu, uint8_t *fd, size_t nd) {
    memcpy(fu, u, nu); memcpy(fd, d, nd);
    memcpy(fu + DM_LOOK_U_CLASS, &dclass, 4); uint64_t p = (uint64_t)(uintptr_t)fd; memcpy(fu + DM_LOOK_U_DATA, &p, 8);
    uint32_t q = dquality, f; memcpy(fd + DM_LOOK_D_QUALITY, &q, 4); memcpy(fd + DM_LOOK_D_ROW, &drow, 4);
    memcpy(&f, fd + DM_LOOK_D_FLAGS, 4); f |= DM_LOOK_FL_IDENT; memcpy(fd + DM_LOOK_D_FLAGS, &f, 4);
}
