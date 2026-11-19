

#include "dm_fightlog_hist.h"
#include "dm.h"
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

typedef struct {
    dm_dh_seg seg[DM_DH_SEGS]; int nseg;
    dm_dh_window win[DM_DH_WINDOWS]; int nwin;
    uint32_t carry;
} book;
struct dm_dh {
    pthread_mutex_t mu;
    book b;
    char who[64]; uint32_t gen, saved; int saved_pend;
    uint32_t added[4]; unsigned nadded;
    uint64_t born[21]; unsigned nborn; int churn_logged;
};
static dm_dh g_dh = { .mu = PTHREAD_MUTEX_INITIALIZER };
dm_dh *dm_dh_core(void) { return &g_dh; }

void dm_dh_reset(dm_dh *h) {
    pthread_mutex_lock(&h->mu);
    h->b.nseg = h->b.nwin = 0; h->b.carry = 0; h->who[0] = 0; h->saved = ++h->gen; h->saved_pend = 0; h->nborn = 0; h->nadded = 0;
    pthread_mutex_unlock(&h->mu);
}

static void add_src(dm_dh_fight *x, uint16_t id, uint32_t v) {
    if (!v) return;
    if (x->nsrc < DM_DH_SRCS - 1) { x->src[x->nsrc].id = id; x->src[x->nsrc++].dealt = v; return; }
    if (x->nsrc == DM_DH_SRCS - 1) { x->src[x->nsrc].id = DM_DH_SRC_OTHER; x->src[x->nsrc++].dealt = 0; }
    x->src[DM_DH_SRCS - 1].dealt += v;
}
static void fight_of(const dm_fl_fight *f, dm_dh_fight *x) {
    const dm_fl_split *s = &f->s; memset(x, 0, sizeof *x);
    x->mine = s->mine; x->team = s->mine + s->merc + s->rest; x->ticks = f->h.duration;
    for (int k = 0; k < DM_FL_SKILLS; k++) if (f->h.skill[k]) add_src(x, (uint16_t)(f->h.skill[k] - 1), s->skill[k]);
    add_src(x, DM_DH_SRC_NONE, s->none); add_src(x, DM_DH_SRC_OTHER, s->over);
    add_src(x, DM_DH_SRC_MERC, s->merc);
    for (int k = 0; k < DM_FL_COMPS; k++) if (s->comp[k].dealt) { x->team += s->comp[k].dealt; add_src(x, (uint16_t)(DM_DH_SRC_COMP | (s->comp[k].cls & 0x7fff)), s->comp[k].dealt); }
    add_src(x, DM_DH_SRC_REST, s->rest); add_src(x, DM_DH_SRC_UNKNOWN, s->unknown);
}

double dm_dh_value(const dm_dh_fight *f, int n, int team) {
    uint64_t d = 0, t = 0;
    for (int i = 0; i < n; i++) { d += team ? f[i].team : f[i].mine; t += f[i].ticks; }
    return t ? (double)d * DM_FL_TPS / (double)t : 0;
}
static uint32_t of_src(const dm_dh_fight *f, int src) { for (int k = 0; k < f->nsrc; k++) if (f->src[k].id == src) return f->src[k].dealt; return 0; }
typedef struct { double dps, w; } wv;
static int by_dps(const void *a, const void *b) { double x = ((const wv *)a)->dps, y = ((const wv *)b)->dps; return x < y ? -1 : x > y; }
int dm_dh_range(const dm_dh_fight *f, int n, int team, int src, double *lo, double *hi) {
    wv v[DM_DH_WINDOW]; int m = 0; double total = 0;
    *lo = *hi = 0;
    for (int i = 0; i < n && m < DM_DH_WINDOW; i++) {
        if (!f[i].ticks) continue;
        uint32_t d = src >= 0 ? of_src(&f[i], src) : team ? f[i].team : f[i].mine;
        if (src >= 0 && !d) continue;
        v[m].dps = (double)d * DM_FL_TPS / f[i].ticks; v[m].w = f[i].ticks; total += v[m].w; m++;
    }
    if (m < DM_DH_MIN) return 0;
    qsort(v, (size_t)m, sizeof v[0], by_dps);
    double acc = 0; int got_lo = 0;
    for (int i = 0; i < m; i++) {
        acc += v[i].w;
        if (!got_lo && acc >= 0.1 * total) { *lo = v[i].dps; got_lo = 1; }
        if (acc >= 0.9 * total) { *hi = v[i].dps; break; }
    }
    return 1;
}


static void window_front(book *h, uint32_t key, const dm_fl_parts *parts) {
    int at = -1;
    for (int i = 0; i < h->nwin; i++) if (h->win[i].key == key) { at = i; break; }
    dm_dh_window w;
    if (at < 0) { memset(&w, 0, sizeof w); w.key = key; w.parts = *parts; at = h->nwin < DM_DH_WINDOWS ? h->nwin++ : DM_DH_WINDOWS - 1; }
    else w = h->win[at];
    memmove(&h->win[1], &h->win[0], (size_t)at * sizeof w);
    h->win[0] = w;
}
static void window_put(dm_dh_window *w, const dm_dh_fight *x) { w->f[w->next] = *x; w->next = (w->next + 1) % DM_DH_WINDOW; if (w->n < DM_DH_WINDOW) w->n++; }

static uint32_t identity(const dm_fl_fight *f) {
    uint32_t x = 2166136261u; const uint8_t *p = (const uint8_t *)f;
    for (size_t i = 0; i < sizeof f->h + sizeof f->s; i++) x = (x ^ p[i]) * 16777619u;
    return x;
}
_Static_assert(offsetof(dm_fl_fight, s) == sizeof(dm_fl_head), "head and split adjacent");
static int counts(uint32_t key, const dm_fl_fight *f) { return key && (f->s.flags & DM_FL_SPLIT_KNOWN) && f->h.duration >= DM_DH_MIN_TICKS; }

static int book_add(book *h, uint32_t key, const dm_fl_parts *parts, const dm_fl_fight *f) {
    dm_dh_fight x; fight_of(f, &x); int born = 0;
    window_front(h, key, parts); window_put(&h->win[0], &x);
    dm_dh_seg *last = h->nseg ? &h->seg[h->nseg - 1] : NULL;
    if (last && last->key != key && last->fights < DM_DH_MIN) { h->carry += last->ticks; h->nseg--; last = h->nseg ? &h->seg[h->nseg - 1] : NULL; }
    if (!last || last->key != key) {
        if (h->nseg == DM_DH_SEGS) { memmove(&h->seg[0], &h->seg[1], (DM_DH_SEGS - 1) * sizeof h->seg[0]); h->nseg--; }
        last = &h->seg[h->nseg++]; memset(last, 0, sizeof *last); last->key = key; last->parts = *parts; last->ticks = h->carry; h->carry = 0; born = 1;
    }
    last->fights++; last->ticks += x.ticks;
    const dm_dh_window *w = &h->win[0]; double lo, hi;
    last->value = (uint32_t)(dm_dh_value(w->f, (int)w->n, 0) * 100 + 0.5);
    if (dm_dh_range(w->f, (int)w->n, 0, -1, &lo, &hi)) { last->lo = (uint32_t)(lo * 100 + 0.5); last->hi = (uint32_t)(hi * 100 + 0.5); } else last->lo = last->hi = 0;
    return born;
}

void dm_dh_add(dm_dh *h, uint32_t key, const dm_fl_parts *parts, const dm_fl_fight *f) {
    if (!counts(key, f)) return;
    pthread_mutex_lock(&h->mu);
    h->added[h->nadded++ % 4] = identity(f);
    if (book_add(&h->b, key, parts, f)) {
        uint64_t t = now_ns(); h->born[h->nborn++ % 21] = t;
        if (h->nborn >= 21 && t - h->born[h->nborn % 21] < 600ull * 1000000000ull && !h->churn_logged) {
            h->churn_logged = 1; LOG("ASSUME dps.state_churn: more than 20 new states in 10 minutes; something that is not gear, level, attributes, points or difficulty moves the state key");
        }
    }
    h->gen++;
    pthread_mutex_unlock(&h->mu);
}

int dm_dh_read(dm_dh *h, uint32_t key, dm_dh_window *w, dm_dh_seg *segs) {
    pthread_mutex_lock(&h->mu);
    memset(w, 0, sizeof *w);
    for (int i = 0; key && i < h->b.nwin; i++) if (h->b.win[i].key == key) { *w = h->b.win[i]; break; }
    int n = h->b.nseg; memcpy(segs, h->b.seg, (size_t)n * sizeof *segs);
    pthread_mutex_unlock(&h->mu);
    return n;
}


static uint32_t dh_crc32(const uint8_t *p, size_t n) {
    static uint32_t table[256]; static int ready;
    if (!ready) { for (uint32_t i = 0; i < 256; i++) { uint32_t c = i; for (int k = 0; k < 8; k++) c = c & 1 ? 0xedb88320u ^ (c >> 1) : c >> 1; table[i] = c; } ready = 1; }
    uint32_t c = 0xffffffffu;
    while (n--) c = table[(c ^ *p++) & 0xff] ^ (c >> 8);
    return c ^ 0xffffffffu;
}
static void dh_put32(uint8_t *p, uint32_t v) { memcpy(p, &v, 4); }
static uint32_t dh_get32(const uint8_t *p) { uint32_t v; memcpy(&v, p, 4); return v; }
static int window_ok(const dm_dh_window *w) {
    if (!w->key || w->n > DM_DH_WINDOW || w->next >= DM_DH_WINDOW || (w->n < DM_DH_WINDOW && w->next != w->n)) return 0;
    for (uint32_t i = 0; i < w->n; i++) { const dm_dh_fight *x = &w->f[i]; if (x->nsrc > DM_DH_SRCS || x->mine > x->team || x->ticks < DM_DH_MIN_TICKS) return 0; }
    return 1;
}
static int seg_ok(const dm_dh_seg *s) { return s->key && s->fights && s->lo <= s->hi; }

int dm_dh_owner(dm_dh *h, const char *who, const uint8_t *image, size_t n) {
    static dm_dh_seg segs[DM_DH_SEGS]; static dm_dh_window wins[DM_DH_WINDOWS];
    int bad = 0; uint32_t ns = 0, nw = 0;
    if (n) {
        if (n < DM_DH_HEAD + 8 || n > DM_DH_IMAGE_MAX) bad = DM_DH_BAD_SIZE;
        else if (dh_get32(image) != DM_DH_MAGIC) bad = DM_DH_BAD_MAGIC;
        else if (dh_get32(image + 4) != DM_DH_VERSION) bad = DM_DH_BAD_VERSION;
        else if (dh_get32(image + 12) != dh_crc32(image + DM_DH_HEAD, n - DM_DH_HEAD)) bad = DM_DH_BAD_CRC;
        else {
            ns = dh_get32(image + DM_DH_HEAD); nw = dh_get32(image + DM_DH_HEAD + 4);
            if (ns > DM_DH_SEGS || nw > DM_DH_WINDOWS) bad = DM_DH_BAD_COUNT;
            else if (n != DM_DH_HEAD + 8 + ns * sizeof(dm_dh_seg) + nw * sizeof(dm_dh_window)) bad = DM_DH_BAD_SIZE;
            else {
                memcpy(segs, image + DM_DH_HEAD + 8, ns * sizeof *segs); memcpy(wins, image + DM_DH_HEAD + 8 + ns * sizeof *segs, nw * sizeof *wins);
                for (uint32_t i = 0; i < ns && !bad; i++) if (!seg_ok(&segs[i])) bad = DM_DH_BAD_RECORD;
                for (uint32_t i = 0; i < nw && !bad; i++) if (!window_ok(&wins[i])) bad = DM_DH_BAD_RECORD;
            }
        }
    }
    if (bad) ns = nw = 0;
    pthread_mutex_lock(&h->mu);
    memcpy(h->b.seg, segs, ns * sizeof *segs); h->b.nseg = (int)ns; memcpy(h->b.win, wins, nw * sizeof *wins); h->b.nwin = (int)nw; h->b.carry = 0;
    snprintf(h->who, sizeof h->who, "%s", who ? who : ""); h->gen++; h->saved = h->gen; h->saved_pend = 0; h->nadded = 0;
    pthread_mutex_unlock(&h->mu);
    return bad;
}
size_t dm_dh_image(dm_dh *h, const dm_fl_fight *pend, const dm_fl_parts *parts, char who[64], uint8_t *buf, size_t cap, uint32_t *gen) {
    static book with;
    size_t n = 0;
    if (pend && (!parts || !counts(pend->s.state, pend))) pend = NULL;
    pthread_mutex_lock(&h->mu);

    for (unsigned k = 0; pend && k < h->nadded && k < 4; k++) if (h->added[k] == identity(pend)) pend = NULL;

    if ((h->gen != h->saved || pend || h->saved_pend) && h->who[0] && cap >= DM_DH_IMAGE_MAX) {
        const book *b = &h->b;
        if (pend) { with = h->b; book_add(&with, pend->s.state, parts, pend); b = &with; }
        dh_put32(buf, DM_DH_MAGIC); dh_put32(buf + 4, DM_DH_VERSION); dh_put32(buf + 8, (uint32_t)(pend != NULL));
        dh_put32(buf + DM_DH_HEAD, (uint32_t)b->nseg); dh_put32(buf + DM_DH_HEAD + 4, (uint32_t)b->nwin); n = DM_DH_HEAD + 8;
        memcpy(buf + n, b->seg, (size_t)b->nseg * sizeof b->seg[0]); n += (size_t)b->nseg * sizeof b->seg[0];
        memcpy(buf + n, b->win, (size_t)b->nwin * sizeof b->win[0]); n += (size_t)b->nwin * sizeof b->win[0];
        memcpy(who, h->who, 64); *gen = h->gen;
    }
    pthread_mutex_unlock(&h->mu);
    if (n) dh_put32(buf + 12, dh_crc32(buf + DM_DH_HEAD, n - DM_DH_HEAD));
    return n;
}
void dm_dh_saved(dm_dh *h, uint32_t gen) { pthread_mutex_lock(&h->mu); h->saved = gen; pthread_mutex_unlock(&h->mu); }
static void saved_pend(dm_dh *h, int pend) { pthread_mutex_lock(&h->mu); h->saved_pend = pend; pthread_mutex_unlock(&h->mu); }
void dm_dh_unsaved(dm_dh *h) { pthread_mutex_lock(&h->mu); h->saved = h->gen - 1; pthread_mutex_unlock(&h->mu); }


static pthread_mutex_t g_dh_fmu = PTHREAD_MUTEX_INITIALIZER;
static uint8_t g_dh_buf[DM_DH_IMAGE_MAX + 1];
static uint64_t g_dh_wrote;
static const char *const dh_bad[] = { "", "size", "magic", "version", "checksum", "count", "record" };
static int dh_path(char *out, size_t cap, const char *root, const char *who) {
    return root && who && who[0] && !strchr(who, '/') && snprintf(out, cap, "%s/%s/%s.sw_dps", root, who, who) < (int)cap;
}
int dm_dh_store_follow(dm_dh *h, const char *root, const char *who) {
    pthread_mutex_lock(&h->mu); int same = !strcmp(h->who, who ? who : ""); pthread_mutex_unlock(&h->mu);
    if (same) return 0;
    char p[1200], q[1220]; size_t n = 0;
    pthread_mutex_lock(&g_dh_fmu);
    if (dh_path(p, sizeof p, root, who)) { FILE *f = fopen(p, "rb"); if (f) { n = fread(g_dh_buf, 1, sizeof g_dh_buf, f); if (ferror(f)) n = sizeof g_dh_buf; fclose(f); } }
    int bad = dm_dh_owner(h, who, g_dh_buf, n);
    pthread_mutex_unlock(&g_dh_fmu);
    if (bad) {
        snprintf(q, sizeof q, "%s.bad", p); int moved = !rename(p, q);
        LOG("ERROR dps.file_rejected: the DPS history of this character failed its %s check; %s, the history starts empty", dh_bad[bad], moved ? "set aside as .bad" : "could not be set aside");
    }
    return 1;
}
int dm_dh_store_commit(dm_dh *h, const char *wd, uint64_t commits, const dm_fl_fight *pend, const dm_fl_parts *parts) {
    char who[64], p[1200], t[1220], dir[1200]; uint32_t gen; struct stat st; int written = 0;
    pthread_mutex_lock(&g_dh_fmu);
    if (g_dh_wrote) { if (commits < g_dh_wrote) dm_dh_unsaved(h); g_dh_wrote = 0; }
    size_t n = wd ? dm_dh_image(h, pend, parts, who, g_dh_buf, sizeof g_dh_buf, &gen) : 0;
    if (n && dh_path(p, sizeof p, wd, who) && snprintf(dir, sizeof dir, "%s/%s", wd, who) < (int)sizeof dir && !stat(dir, &st) && S_ISDIR(st.st_mode)) {
        snprintf(t, sizeof t, "%s.tmp", p);
        FILE *f = fopen(t, "wb"); int ok = f && fwrite(g_dh_buf, 1, n, f) == n;
        if (f && fclose(f)) ok = 0;
        if (ok && !rename(t, p)) { dm_dh_saved(h, gen); saved_pend(h, dh_get32(g_dh_buf + 8) != 0); g_dh_wrote = commits + 1; written = 1; }
        else { remove(t); LOG("ERROR dps.write_failed: the DPS history could not be written; tried again with the next save"); }
    }
    pthread_mutex_unlock(&g_dh_fmu);
    return written;
}
