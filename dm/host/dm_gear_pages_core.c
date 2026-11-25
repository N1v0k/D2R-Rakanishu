


#include "dm_gear_pages.h"
#include <stdlib.h>
#include <string.h>

enum { P_ABSENT, P_STALE, P_READ, P_UNREADABLE };
typedef struct { uint64_t key; int skip, done, hashed; dm_gpg_item it; } entry;
typedef struct {
    int state;
    uint64_t content, psig;
    int n, skipped;
    entry *items;
    int *bykey, *byhash, nidx;


    uint64_t wcontent, wpsig;
    int wn, pn;
    entry *work, *prev;
} place;
struct dm_gpg_core {
    place p[DM_GPG_PLACES];
    uint64_t rev;
    int cursor, sweeping;
    unsigned asked, answered, sweep_ask;

    uint32_t *units, *unit_name; uint64_t *unit_hash; unsigned char *unit_cand; int nunits; uint64_t units_rev;
};

uint64_t dm_gpg_fnv(const void *data, size_t n, uint64_t h) {
    const unsigned char *d = data;
    for (size_t i = 0; i < n; i++) { h ^= d[i]; h *= 1099511628211ull; }
    return h;
}
uint64_t dm_gpg_content(const dm_gpg_in *in, int n) {
    uint64_t h = dm_gpg_fnv(&n, sizeof n, DM_GPG_FNV0);
    int m = n < DM_GPG_PLACE_ITEMS ? n : DM_GPG_PLACE_ITEMS;
    for (int i = 0; i < m; i++) { int v[3] = { in[i].x, in[i].y, in[i].skip }; h = dm_gpg_fnv(v, sizeof v, h); h = dm_gpg_fnv(&in[i].key, sizeof in[i].key, h); }
    return h ? h : 1;
}
dm_gpg_core *dm_gpg_new(void) { return calloc(1, sizeof(dm_gpg_core)); }
static void work_drop(place *p) { free(p->work); free(p->prev); p->work = p->prev = NULL; p->wn = p->pn = 0; p->wcontent = p->wpsig = 0; }
static void index_drop(place *p) { free(p->bykey); free(p->byhash); p->bykey = p->byhash = NULL; p->nidx = 0; }
static void place_clear(place *p) { free(p->items); p->items = NULL; p->n = p->skipped = 0; index_drop(p); work_drop(p); p->content = p->psig = 0; }
void dm_gpg_reset(dm_gpg_core *c) {
    for (int i = 0; i < DM_GPG_PLACES; i++) { place_clear(&c->p[i]); c->p[i].state = P_ABSENT; }
    c->cursor = c->sweeping = 0;
    c->rev++;
}
void dm_gpg_free(dm_gpg_core *c) { if (!c) return; dm_gpg_reset(c); free(c->units); free(c->unit_name); free(c->unit_hash); free(c->unit_cand); free(c); }


static uint64_t now_of(const dm_gpg_budget *b) { return b->clock ? b->clock(b->ctx) : 0; }
void dm_gpg_budget_start(dm_gpg_budget *b) { b->made = 0; b->begun = 0; b->t0 = now_of(b); }
uint64_t dm_gpg_budget_used(const dm_gpg_budget *b) { return now_of(b) - b->t0 + b->spent; }
static int fits(const dm_gpg_budget *b, int kind) {
    if (kind == DM_GPG_W_PROFILE && b->max && b->made >= b->max) return 0;
    if (!b->limit_ns || !b->clock) return 1;
    uint64_t used = dm_gpg_budget_used(b);
    if (!(b->begun & 1u << kind)) return used < b->limit_ns / 2 || used + b->est[kind] <= b->limit_ns;
    return used + b->est[kind] <= b->limit_ns;
}

static void done(dm_gpg_budget *b, int kind, uint64_t t) {
    uint64_t d = now_of(b) - t, *e = &b->est[kind];
    *e = d > *e ? d : *e - *e / 8;
    b->begun |= 1u << kind;
    if (kind == DM_GPG_W_PROFILE) b->made++;
}

static void stale(dm_gpg_core *c, place *p) { if (p->state != P_STALE) { p->state = P_STALE; c->rev++; } }
void dm_gpg_absent(dm_gpg_core *c, int place_i) {
    if (place_i < 0 || place_i >= DM_GPG_PLACES) return;
    place *p = &c->p[place_i];
    if (p->state != P_ABSENT) { place_clear(p); p->state = P_ABSENT; c->rev++; }
}
void dm_gpg_missing(dm_gpg_core *c, int place_i) { if (place_i >= 0 && place_i < DM_GPG_PLACES) stale(c, &c->p[place_i]); }


static const entry *g_sort_items;
static int cmp_key(const void *a, const void *b) { uint64_t x = g_sort_items[*(const int *)a].key, y = g_sort_items[*(const int *)b].key; return x < y ? -1 : x > y; }
static int cmp_hash(const void *a, const void *b) { uint64_t x = g_sort_items[*(const int *)a].it.hash, y = g_sort_items[*(const int *)b].it.hash; return x < y ? -1 : x > y; }
static void index_build(place *p) {
    index_drop(p);
    int m = 0; for (int i = 0; i < p->n; i++) m += !p->items[i].skip;
    if (!m) return;
    p->bykey = malloc(sizeof(int) * (size_t)m); p->byhash = malloc(sizeof(int) * (size_t)m);
    if (!p->bykey || !p->byhash) { index_drop(p); return; }
    for (int i = 0, k = 0; i < p->n; i++) if (!p->items[i].skip) { p->bykey[k] = p->byhash[k] = i; k++; }
    p->nidx = m; g_sort_items = p->items;
    qsort(p->bykey, (size_t)m, sizeof(int), cmp_key); qsort(p->byhash, (size_t)m, sizeof(int), cmp_hash);
}
static const entry *index_find(const place *p, int byhash, uint64_t v) {
    const int *ix = byhash ? p->byhash : p->bykey; int lo = 0, hi = p->nidx;
    while (lo < hi) { int mid = (lo + hi) / 2; const entry *e = &p->items[ix[mid]]; uint64_t k = byhash ? e->it.hash : e->key; if (k < v) lo = mid + 1; else hi = mid; }
    if (lo < p->nidx) { const entry *e = &p->items[ix[lo]]; if ((byhash ? e->it.hash : e->key) == v) return e; }
    return NULL;
}


static const entry *reuse(const dm_gpg_core *c, const place *self, int byhash, uint64_t v, uint64_t psig) {
    for (int i = 0; i < DM_GPG_PLACES; i++) {
        const place *p = &c->p[i];
        if (p->items && p->psig == psig && p->nidx) { const entry *e = index_find(p, byhash, v); if (e) return e; }
    }
    for (int k = 0; k < self->wn; k++) { const entry *e = &self->work[k];
        if (e->done && !e->skip && (byhash ? e->hashed && e->it.hash == v : e->key == v)) return e; }
    if (self->wpsig == psig) for (int k = 0; k < self->pn; k++) { const entry *e = &self->prev[k];
        if (e->done && !e->skip && (byhash ? e->hashed && e->it.hash == v : e->key == v)) return e; }
    return NULL;
}
static void identify(entry *e, int place_i, const dm_gpg_in *in) {
    dm_gpg_item *it = &e->it;
    it->where = place_i < DM_GPG_PAGES ? DM_GPG_PERSONAL : DM_GPG_SHARED; it->index = place_i < DM_GPG_PAGES ? place_i : place_i - DM_GPG_PAGES;
    it->x = in->x; it->y = in->y; it->unit = in->unit;
    it->p.c.item.id = in->unit;
    it->p.show.where = it->where == DM_GPG_PERSONAL ? DM_GTAB_STASH : DM_GTAB_SHARED; it->p.show.page = it->index + 1; it->p.show.id = in->unit;
}
static void unreadable(dm_gpg_core *c, place *p, uint64_t content, uint64_t psig) {
    work_drop(p); p->state = P_UNREADABLE; p->content = content; p->psig = psig; free(p->items); p->items = NULL; p->n = 0; index_drop(p); c->rev++;
}

static int place_read(dm_gpg_core *c, int place_i, uint64_t psig, const dm_gpg_in *in, int n, dm_gpg_profile_fn fn, dm_gpg_hash_fn hfn, void *ctx,
                      dm_gpg_budget *bud) {
    if (place_i < 0 || place_i >= DM_GPG_PLACES || n < 0 || (n && !in)) return -1;
    place *p = &c->p[place_i];
    uint64_t content = dm_gpg_content(in, n);
    if (p->state == P_READ && p->content == content && p->psig == psig) return 1;
    if (p->state == P_UNREADABLE && p->content == content && p->psig == psig) return -2;
    if (n > DM_GPG_PLACE_ITEMS) { unreadable(c, p, content, psig); return -1; }
    stale(c, p);
    if (p->wcontent != content || p->wpsig != psig || (!p->work && n)) {
        if (p->work && p->wpsig == psig) { free(p->prev); p->prev = p->work; p->pn = p->wn; p->work = NULL; }
        else { free(p->prev); p->prev = NULL; p->pn = 0; free(p->work); p->work = NULL; }
        if (n && !(p->work = calloc((size_t)n, sizeof *p->work))) return 0;
        p->wn = n; p->wcontent = content; p->wpsig = psig;
    }
    int open = 0;
    for (int i = 0; i < p->wn; i++) {
        entry *e = &p->work[i]; if (e->done) continue;
        e->key = in[i].key;
        if (in[i].skip) { e->skip = e->done = 1; identify(e, place_i, &in[i]); continue; }
        const entry *r = reuse(c, p, 0, in[i].key, psig);
        if (!r && hfn && !e->hashed) {
            if (!fits(bud, DM_GPG_W_HASH)) { open++; continue; }
            uint64_t t = now_of(bud), h = 0; int rc = hfn(ctx, &in[i], &h); done(bud, DM_GPG_W_HASH, t);
            if (rc < 0) { unreadable(c, p, content, psig); return -1; }
            e->it.hash = h; e->hashed = 1;
            r = reuse(c, p, 1, h, psig);
        } else if (!r && e->hashed) r = reuse(c, p, 1, e->it.hash, psig);
        if (r) { uint64_t h = r->it.hash; e->it = r->it; e->it.hash = h; e->hashed = 1; e->done = 1; identify(e, place_i, &in[i]); continue; }
        if (!fits(bud, DM_GPG_W_PROFILE)) { open++; continue; }
        uint64_t t = now_of(bud), hash = 0; int rc = fn(ctx, &in[i], &hash, &e->it.p); done(bud, DM_GPG_W_PROFILE, t);
        if (rc < 0) { unreadable(c, p, content, psig); return -1; }
        e->it.hash = hash; e->hashed = 1; e->skip = rc == 1; e->done = 1; identify(e, place_i, &in[i]);
    }
    if (open) return 0;
    free(p->items);
    p->items = p->work; p->n = p->wn; p->skipped = 0; p->work = NULL; work_drop(p);
    for (int i = 0; i < p->n; i++) p->skipped += p->items[i].skip;
    index_build(p);
    p->content = content; p->psig = psig; p->state = P_READ; c->rev++;
    return 1;
}
int dm_gpg_place(dm_gpg_core *c, int place_i, uint64_t psig, const dm_gpg_in *in, int n, dm_gpg_profile_fn fn, dm_gpg_hash_fn hash, void *ctx,
                 dm_gpg_budget *bud) {
    int rc = place_read(c, place_i, psig, in, n, fn, hash, ctx, bud);
    return rc < 0 ? -1 : rc;
}


void dm_gpg_ask(dm_gpg_core *c, unsigned n) { c->asked = n; }
unsigned dm_gpg_answered(const dm_gpg_core *c) { return c->answered; }
void dm_gpg_pass(dm_gpg_core *c, uint64_t psig, dm_gpg_list_fn list, void *ctx, dm_gpg_in *in, int max, dm_gpg_budget *bud, dm_gpg_pass_out *out) {
    memset(out, 0, sizeof *out);
    if (max < DM_GPG_PLACE_ITEMS) return;
    if (!c->sweeping) { c->sweeping = 1; c->cursor = 0; c->sweep_ask = c->asked; }

    for (int i = 0; i < DM_GPG_PLACES; i++) { place *p = &c->p[i]; if ((p->state == P_READ || p->state == P_UNREADABLE) && p->psig != psig) stale(c, p); }
    while (c->cursor < DM_GPG_PLACES) {
        int pl = c->cursor;
        if (!fits(bud, DM_GPG_W_LIST)) return;
        dm_gpg_profile_fn fn = NULL; dm_gpg_hash_fn hfn = NULL;
        uint64_t t = now_of(bud);
        int n = list(ctx, pl, in, max, &fn, &hfn);
        done(bud, DM_GPG_W_LIST, t); out->listed++;
        if (n == DM_GPG_ABSENT) dm_gpg_absent(c, pl);
        else if (n < 0 || !fn) dm_gpg_missing(c, pl);
        else {
            int rc = place_read(c, pl, psig, in, n, fn, hfn, ctx, bud);
            if (rc == 0) return;
            if (rc == -1) out->unreadable |= 1u << pl;
        }
        c->cursor++;
    }
    c->sweeping = 0; c->cursor = 0; c->answered = c->sweep_ask; out->swept = 1;
}


void dm_gpg_state_get(const dm_gpg_core *c, dm_gpg_state *st) {
    memset(st, 0, sizeof *st); st->rev = c->rev;
    for (int i = 0; i < DM_GPG_PLACES; i++) {
        const place *p = &c->p[i];
        if (p->state == P_ABSENT) continue;
        st->present |= 1u << i;
        if (p->state != P_READ) { st->missing |= 1u << i; if (p->state == P_UNREADABLE) st->unreadable |= 1u << i; continue; }
        st->place_skipped[i] = p->skipped; st->skipped += p->skipped;
        for (int k = 0; k < p->n; k++) st->place_items[i] += !p->items[k].skip && p->items[k].it.p.kind != DM_GEAR_K_NONE;
        st->items += st->place_items[i];
    }
}
int dm_gpg_items(const dm_gpg_core *c, dm_gpg_item *out, int max) {
    int n = 0;
    for (int i = 0; i < DM_GPG_PLACES; i++) {
        const place *p = &c->p[i];
        if (p->state != P_READ) continue;
        for (int k = 0; k < p->n && n < max; k++) if (!p->items[k].skip && p->items[k].it.p.kind != DM_GEAR_K_NONE) out[n++] = p->items[k].it;
    }
    return n;
}
static const uint32_t *g_sort_unit;
static int cmp_unit(const void *a, const void *b) { uint32_t x = g_sort_unit[*(const int *)a], y = g_sort_unit[*(const int *)b]; return x < y ? -1 : x > y; }

static void units_drop(dm_gpg_core *c) {
    free(c->units); free(c->unit_name); free(c->unit_hash); free(c->unit_cand);
    c->units = c->unit_name = NULL; c->unit_hash = NULL; c->unit_cand = NULL; c->nunits = 0;
}
static int units_index(dm_gpg_core *c) {
    if (c->units_rev == c->rev && c->units) return 1;
    units_drop(c);
    int m = 0;
    for (int i = 0; i < DM_GPG_PLACES; i++) if (c->p[i].state == P_READ) for (int k = 0; k < c->p[i].n; k++) m += c->p[i].items[k].it.unit && !c->p[i].items[k].skip;
    uint32_t *u = malloc(sizeof *u * (size_t)(m + 1)), *nm = malloc(sizeof *nm * (size_t)(m + 1)); int *ix = malloc(sizeof *ix * (size_t)(m + 1));
    const entry **at = malloc(sizeof *at * (size_t)(m + 1));
    c->units = malloc(sizeof *c->units * (size_t)(m + 1)); c->unit_name = malloc(sizeof *c->unit_name * (size_t)(m + 1));
    c->unit_hash = malloc(sizeof *c->unit_hash * (size_t)(m + 1)); c->unit_cand = malloc((size_t)(m + 1));
    int ok = u && nm && ix && at && c->units && c->unit_name && c->unit_hash && c->unit_cand;
    if (ok) {
        int k2 = 0;
        for (int i = 0; i < DM_GPG_PLACES; i++) if (c->p[i].state == P_READ) for (int k = 0; k < c->p[i].n; k++) {
            const entry *e = &c->p[i].items[k]; if (e->it.unit && !e->skip) { u[k2] = e->it.unit; nm[k2] = dm_gpg_stock_id(i, e->it.x, e->it.y); at[k2] = e; ix[k2] = k2; k2++; } }
        g_sort_unit = u; qsort(ix, (size_t)m, sizeof *ix, cmp_unit);
        for (int i = 0; i < m; i++) { c->units[i] = u[ix[i]]; c->unit_name[i] = nm[ix[i]]; c->unit_hash[i] = at[ix[i]]->it.hash; c->unit_cand[i] = at[ix[i]]->it.p.kind != DM_GEAR_K_NONE; }
        c->nunits = m; c->units_rev = c->rev;
    } else units_drop(c);
    free(u); free(nm); free(ix); free(at);
    return ok;
}
static int unit_at(dm_gpg_core *c, uint32_t unit) {
    if (!unit || !units_index(c)) return -1;
    int lo = 0, hi = c->nunits;
    while (lo < hi) { int mid = (lo + hi) / 2; if (c->units[mid] < unit) lo = mid + 1; else hi = mid; }
    return lo < c->nunits && c->units[lo] == unit ? lo : -1;
}
int dm_gpg_unit_hash(dm_gpg_core *c, uint32_t unit, uint64_t *hash) {
    int k = unit_at(c, unit);
    if (k < 0) return 0;
    *hash = c->unit_hash[k]; return c->unit_cand[k] ? 1 : 2;
}
uint32_t dm_gpg_stock_id(int place, int x, int y) { return DM_GTAB_STOCK_ID | (uint32_t)(place & 0xff) << 16 | (uint32_t)(y & 0xff) << 8 | (uint32_t)(x & 0xff); }
uint32_t dm_gpg_row_name(dm_gpg_core *c, uint32_t unit, uint64_t *vhash) {
    int k = unit_at(c, unit);
    if (k < 0) { *vhash = 0; return unit; }
    *vhash = c->unit_hash[k]; return c->unit_name[k];
}
void dm_gpg_select(const dm_gpg_state *st, const dm_gpg_item *items, int n, int pcur, const int tabno[DM_GPG_TABS], unsigned tabs_read, dm_gpg_sel *o) {
    o->n = o->pages = o->shared = o->skipped = o->busy = 0;
    int copied[DM_GPG_PLACES] = { 0 };
    for (int i = 0; i < n; i++) { int at = items[i].where == DM_GPG_PERSONAL ? items[i].index : DM_GPG_PAGES + items[i].index;
        if (at >= 0 && at < DM_GPG_PLACES) copied[at]++; }
    for (int p = 0; p < DM_GPG_PLACES; p++) {
        unsigned bit = 1u << p; int personal = p < DM_GPG_PAGES, index = personal ? p : p - DM_GPG_PAGES, no = index + 1;
        if (personal && p == pcur) continue;
        if (!personal) {
            if (!(st->present & bit)) continue;
            if (tabno && tabno[index] > 0) no = tabno[index];
            if (no <= 32 && (tabs_read & 1u << (no - 1))) continue;
        }
        if (!(st->present & bit) || (st->missing & bit) || copied[p] < st->place_items[p]) {
            if (personal) o->pages++; else o->shared = 1;
            if (!(st->unreadable & bit)) o->busy++;
            continue;
        }
        o->skipped += st->place_skipped[p];
        for (int i = 0; i < n && o->n < DM_GPG_SEL_MAX; i++) {
            if (items[i].where != (personal ? DM_GPG_PERSONAL : DM_GPG_SHARED) || items[i].index != index) continue;
            o->idx[o->n] = i; o->page[o->n] = no; o->n++;
        }
    }
}
static int cmp_id(const void *a, const void *b) {
    const dm_gpg_id *x = a, *y = b;
    if (x->where != y->where) return x->where < y->where ? -1 : 1;
    if (x->page != y->page) return x->page < y->page ? -1 : 1;
    if (x->y != y->y) return x->y < y->y ? -1 : 1;
    if (x->x != y->x) return x->x < y->x ? -1 : 1;
    return x->hash < y->hash ? -1 : x->hash > y->hash;
}
uint64_t dm_gpg_ids_hash(dm_gpg_id *ids, int n, uint64_t h) {
    if (n > 1) qsort(ids, (size_t)n, sizeof *ids, cmp_id);
    h = dm_gpg_fnv(&n, sizeof n, h);
    for (int i = 0; i < n; i++) { int v[4] = { ids[i].where, ids[i].page, ids[i].x, ids[i].y }; h = dm_gpg_fnv(v, sizeof v, h); h = dm_gpg_fnv(&ids[i].hash, sizeof ids[i].hash, h); }
    return h;
}
