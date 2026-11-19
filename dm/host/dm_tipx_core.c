


#include "dm_tipx_core.h"
#include <stdio.h>
#include <string.h>

#define CC0 0xee
#define CC1 0x81
#define CC2 0xbe
#define MAXP (DM_TIPX_MAXL * DM_TIPX_WRAP)
#define PBUF (DM_TIPX_MAXL * (DM_TIPX_LINEB + DM_TIPX_WRAP * 8))

int dm_tipx_text_lines(const char *s) { int n = 1; for (; s && *s; s++) n += *s == '\n'; return n; }

static int is_cc(const unsigned char *p, size_t i, size_t n) { return i + 2 < n && p[i] == CC0 && p[i + 1] == CC1 && p[i + 2] == CC2; }
size_t dm_tipx_insert_under(const char *text, int top, const char *line, char *out, size_t cap) {
    size_t n = strlen(text), at = 0, ln = strlen(line);
    for (int k = dm_tipx_text_lines(text) - top; k > 0; k--) { const char *e = strchr(text + at, '\n'); if (!e) break; at = (size_t)(e - text) + 1; }
    char colour[4] = { (char)CC0, (char)CC1, (char)CC2, '0' };
    for (size_t i = 0; i + 3 < at; i++) if (is_cc((const unsigned char *)text, i, at)) memcpy(colour, text + i, 4);
    if (at + ln + 1 + 4 + (n - at) + 1 > cap) return 0;
    memcpy(out, text, at); memcpy(out + at, line, ln); out[at + ln] = '\n'; memcpy(out + at + ln + 1, colour, 4);
    memcpy(out + at + ln + 5, text + at, n - at + 1);
    return n + ln + 5;
}
size_t dm_tipx_utf8_cut(const char *s, size_t n, int max_cp) {
    const unsigned char *p = (const unsigned char *)s; size_t i = 0; int cp = 0;
    while (i < n) {
        if (is_cc(p, i, n)) { size_t j = i + 3 < n ? i + 4 : n; i = j; continue; }
        size_t j = i + 1; while (j < n && (p[j] & 0xc0) == 0x80) j++;
        if (cp + 1 > max_cp) break;
        cp++; i = j;
    }
    return i;
}
static int vis_len(const char *s, size_t n) { const unsigned char *p = (const unsigned char *)s; int cp = 0; for (size_t i = 0; i < n; ) { if (is_cc(p, i, n)) { i += 4; continue; } i++; while (i < n && (p[i] & 0xc0) == 0x80) i++; cp++; } return cp; }
static const char *last_cc(const char *s, size_t n) { const unsigned char *p = (const unsigned char *)s; const char *r = NULL; for (size_t i = 0; i + 3 < n; i++) if (is_cc(p, i, n)) r = s + i; return r; }

static struct { char buf[PBUF]; size_t used; struct { const char *s; size_t n; int unit, flags; } p[MAXP]; int np; } W;
static int add_piece(const char *pre, size_t pn, const char *s, size_t n, const char *post, int unit, int flags) {
    if (W.np >= MAXP || W.used + pn + n + strlen(post) + 1 > PBUF) return 0;
    char *d = W.buf + W.used; memcpy(d, pre, pn); memcpy(d + pn, s, n); strcpy(d + pn + n, post);
    W.p[W.np].s = d; W.p[W.np].n = pn + n + strlen(post); W.p[W.np].unit = unit; W.p[W.np].flags = flags; W.np++; W.used += pn + n + strlen(post) + 1; return 1;
}
static int wrap(const char *s, int unit, int flags, int maxc) {
    size_t n = strlen(s); char col[4] = { 0 }; size_t coln = 0; int pieces = 0;
    for (int guard = 0; guard < DM_TIPX_WRAP; guard++) {
        if (maxc <= 0 || vis_len(s, n) <= maxc) return add_piece(col, coln, s, n, "", unit, flags) ? pieces + 1 : -1;
        size_t cut = dm_tipx_utf8_cut(s, n, maxc), sp = cut;
        int depth=0;sp=0;
        for(size_t i=0;i<=cut && i<n;i++) {
            if(s[i]=='(' || s[i]=='[')depth++;
            if(s[i]==')' || s[i]==']')depth--;
            if(s[i]==' ' && depth==0)sp=i+1;
        }

        depth=0;
        for(size_t i=0;i+1<cut && i+1<n;i++) {
            if(s[i]=='(' || s[i]=='[')depth++;
            if(s[i]==')' || s[i]==']')depth--;
            if(s[i]==':' && s[i+1]==' ' && depth==0){if(vis_len(s+i+2,n-i-2)<=maxc)sp=i+2;break;}
        }
        if (sp == 0 || guard == DM_TIPX_WRAP - 1) { size_t c3 = dm_tipx_utf8_cut(s, n, maxc > 3 ? maxc - 3 : maxc); return add_piece(col, coln, s, c3, "...", unit, flags) ? pieces + 1 : -1; }
        size_t end = sp; while (end > 0 && s[end - 1] == ' ') end--;
        if (!add_piece(col, coln, s, end, "", unit, flags)) return -1;
        pieces++;
        const char *lc = last_cc(s, end); if (lc) { memcpy(col, lc, 4); coln = 4; }
        s += sp; n -= sp; while (n && *s == ' ') { s++; n--; }
    }
    return -1;
}



static int compose1(const char *orig, const dm_tipx_line *ln, int n, const dm_tipx_cfg *c, int page, char *out, size_t cap, size_t *len, int demote) {
    if (len) *len = 0;
    if (!orig || !ln || n <= 0 || n > DM_TIPX_MAXL || !out || cap < 2) return 0;
    for (int i = 0; i < n; i++) if (!ln[i].s || strlen(ln[i].s) >= DM_TIPX_LINEB) return 0;
    W.used = 0; W.np = 0;
    static int unit_first[MAXP + 1]; int nf = 0, nbody_units = 0, nu = 0;
    for (int i = 0, seen_warn = 0; i < n; i++) { int fl = ln[i].flags; if (demote && (fl & DM_TIPX_WARN)) { if (seen_warn) fl &= ~DM_TIPX_WARN; seen_warn = 1; }
        int r = wrap(ln[i].s, i, fl, c->max_chars); if (r < 0) return 0; }

    for (int i = 0; i < W.np; i++) { if (W.p[i].flags) { nf++; continue; } if (!i || W.p[i].unit != W.p[i - 1].unit || W.p[i - 1].flags) unit_first[nu++] = i; }
    nbody_units = nu; unit_first[nu] = W.np;
    int body_lines = W.np - nf, orig_lines = dm_tipx_text_lines(orig);
    int cap1 = c->max_lines - orig_lines - 1, capd = c->max_lines - c->name_lines - 1 - (c->props ? 1 : 0);

    static int ub[MAXP + 2]; int npages = 1; ub[0] = 0;
    if (cap1 >= nf + body_lines) ub[1] = nbody_units;
    else {
        int room = cap1 - nf - 1, u = 0, used = 0;
        while (u < nbody_units && room > 0 && used + (unit_first[u + 1] - unit_first[u]) <= room) { used += unit_first[u + 1] - unit_first[u]; u++; }
        ub[1] = u;
        int roomd = capd - nf - 1; if (roomd < 1) return demote ? 0 : -1;
        { int big = 0; for (int k = u; k < nbody_units; k++) big |= unit_first[k + 1] - unit_first[k] > roomd;
          if (big) { int k2 = 0; for (int i = unit_first[u]; i < W.np; i++) if (!W.p[i].flags) unit_first[u + k2++] = i; nbody_units = u + k2; unit_first[nbody_units] = W.np; } }
        while (u < nbody_units && npages <= MAXP) { int k = u, used2 = 0; while (k < nbody_units && (k == u || used2 + (unit_first[k + 1] - unit_first[k]) <= roomd)) { used2 += unit_first[k + 1] - unit_first[k]; k++; }
            ub[++npages] = k; u = k; }
        if (u < nbody_units) return 0;
    }
    if (page < 0 || page >= npages) page = 0;

    static const char *L[MAXP + 4]; static size_t LN[MAXP + 4]; int nl = 0;
    static char hintbuf[256], onebuf[512]; size_t hl = 0;
    if (npages > 1 && c->hint) { int k = snprintf(hintbuf, sizeof hintbuf, c->hint, page + 1, npages); hl = k < 0 ? 0 : (size_t)k < sizeof hintbuf ? (size_t)k : sizeof hintbuf - 1; }
    int squeeze = page == 0 && npages > 1 && cap1 < nf + 1;
    if (squeeze) {
        if (cap1 <= 0) {                                                                       }
        else {
            int head = -1, warn = -1; for (int i = 0; i < W.np; i++) { if ((W.p[i].flags & DM_TIPX_HEAD) && head < 0) head = i; if ((W.p[i].flags & DM_TIPX_WARN) && warn < 0) warn = i; }
            if (cap1 >= 2 && head >= 0) { L[nl] = W.p[head].s; LN[nl++] = W.p[head].n; }
            if (warn >= 0) { int lim = c->max_chars > 0 ? c->max_chars : 80; if (lim < 60) lim = 60; int keep = lim - vis_len(hintbuf, hl) - 3; if (keep < 12) keep = 12;
                size_t wc = dm_tipx_utf8_cut(W.p[warn].s, W.p[warn].n, keep); int k = snprintf(onebuf, sizeof onebuf, "%.*s \xc2\xb7 %.*s", (int)wc, W.p[warn].s, (int)hl, hintbuf);
                L[nl] = onebuf; LN[nl++] = k > 0 && (size_t)k < sizeof onebuf ? (size_t)k : 0; }
            else if (hl) { L[nl] = hintbuf; LN[nl++] = hl; } }
    } else {
        for (int i = 0; i < W.np; i++) if (W.p[i].flags) { L[nl] = W.p[i].s; LN[nl++] = W.p[i].n; }
        for (int u = ub[page]; u < ub[page + 1]; u++) for (int i = unit_first[u]; i < unit_first[u + 1]; i++) { L[nl] = W.p[i].s; LN[nl++] = W.p[i].n; }
        if (hl) { L[nl] = hintbuf; LN[nl++] = hl; }
    }

    size_t o = 0;
#define PUT(p_, n_) do { size_t n__ = (n_); if (o + n__ + 1 >= cap) return 0; memcpy(out + o, (p_), n__); o += n__; } while (0)
    for (int i = nl - 1; i >= 0; i--) { PUT(L[i], LN[i]); PUT("\n", 1); }
    if (page == 0) PUT(orig, strlen(orig));
    else {
        if (c->props) { PUT(c->props, strlen(c->props)); PUT("\n", 1); }
        int segs = dm_tipx_text_lines(orig), from = segs - (c->name_lines > 0 ? c->name_lines : 1); if (from < 0) from = 0;
        const char *p = orig; for (int k = 0; k < from; k++) { p = strchr(p, '\n'); if (!p) break; p++; }
        if (p) PUT(p, strlen(p));
    }
#undef PUT
    out[o] = 0; if (len) *len = o;
    return npages;
}
int dm_tipx_compose(const char *orig, const dm_tipx_line *ln, int n, const dm_tipx_cfg *c, int page, char *out, size_t cap, size_t *len) {
    int r = compose1(orig, ln, n, c, page, out, cap, len, 0);
    if (r < 0) r = compose1(orig, ln, n, c, page, out, cap, len, 1);
    return r < 0 ? 0 : r;
}

int dm_tipx_top_line(const char *text, int i, size_t *start, size_t *len) {
    if (!text || i < 0) return 0;
    size_t end = strlen(text);
    for (;;) {
        size_t from = end;
        while (from > 0 && text[from - 1] != '\n') from--;
        if (i-- == 0) { if (start) *start = from; if (len) *len = end - from; return 1; }
        if (from == 0) return 0;
        end = from - 1;
    }
}



int dm_tipx_dot_line(const char *text, int vendor) {
    if (!vendor) return 0;
    size_t s = 0, n = 0;
    if (!dm_tipx_top_line(text, 0, &s, &n) || !n) return 0;
    const unsigned char *p = (const unsigned char *)text;
    int digit = 0;
    for (size_t k = 0; k < n; ) {
        if (is_cc(p, s + k, s + n)) { k += 4; continue; }
        unsigned char c = p[s + k];
        if (c != ' ') digit = c >= '0' && c <= '9';
        k++;
    }
    return digit;
}


int dm_tipx_text_object_ok(const uint8_t *o, uint64_t frame) {
    uint64_t text = *(const uint64_t *)o, size = *(const uint64_t *)(o + 8), cap = *(const uint64_t *)(o + 0x10), room = cap & ~(1ull << 63);
    if (size > room) return 0;
    if (cap >> 63) return text == (uint64_t)(uintptr_t)(o + 0x18) && room <= 15;
    return text != (uint64_t)(uintptr_t)(o + 0x18) && text >= 0x10000 && text < 1ull << 48 && !(text & 7) && (text < frame || text >= frame + 0x330) && room < 1u << 20;
}




int dm_tipx_side_boxes(uint64_t x29, const uint64_t *regs) {
    uint64_t frame = x29 - 0x320;
    if (frame != (uint64_t)(uintptr_t)regs + 0x330) return -1;
    const uint8_t *l1 = (const uint8_t *)(uintptr_t)(frame + 0x40), *l2 = (const uint8_t *)(uintptr_t)(frame + 0x18);
    if (!dm_tipx_text_object_ok(l1, frame) || !dm_tipx_text_object_ok(l2, frame)) return -1;
    return *(const uint64_t *)(l1 + 8) || *(const uint64_t *)(l2 + 8) ? 1 : 0;
}

int dm_tipx_colour_index(char code) {
    return code == '3' ? 0 : code == '1' ? 1 : code == '5' ? 2 : code == '6' ? 3 : -1;
}
static int own_code(const dm_tipx_span *s, int n, size_t i) { for (int k = 0; k < n; k++) if (i >= s[k].at && i + 3 < s[k].at + s[k].len) return 1; return 0; }
size_t dm_tipx_colour_spans(const char *text, const dm_tipx_span *span, int nspan, char *out, size_t capacity) {
    size_t n = strlen(text), used = 0;
    for (int k = 0; k < nspan; k++) if (span[k].at > n || span[k].len > n - span[k].at) return 0;
    for (size_t i = 0; i < n; i++) {
        if (is_cc((const unsigned char *)text, i, n) && own_code(span, nspan, i) && dm_tipx_colour_index(text[i + 3]) >= 0) {
            if (used + 8 >= capacity) return 0;
            memcpy(out + used, DM_TIPX_COLOUR_TAG, 8); used += 8;
        }
        if (used + 1 >= capacity) return 0;
        out[used++] = text[i];
    }
    out[used] = 0;
    return used;
}
size_t dm_tipx_colour_text(const char *text, size_t own_bytes, char *out, size_t capacity) {
    dm_tipx_span s = { 0, own_bytes };
    return dm_tipx_colour_spans(text, &s, 1, out, capacity);
}

const char *dm_tipx_render_spans(char *text, const dm_tipx_span *span, int nspan, int colour_ready, char *out, size_t capacity) {
    if (colour_ready && dm_tipx_colour_spans(text, span, nspan, out, capacity)) return out;
    size_t n = strlen(text);

    for (size_t i = 0; i + 3 < n; i++) {
        if (is_cc((const unsigned char *)text, i, n) && text[i + 3] == '6' && own_code(span, nspan, i)) text[i + 3] = '3';
    }
    return text;
}
const char *dm_tipx_render_text(char *text, size_t own_bytes, int colour_ready, char *out, size_t capacity) {
    size_t n = strlen(text);
    dm_tipx_span s = { 0, own_bytes > n ? n : own_bytes };
    return dm_tipx_render_spans(text, &s, 1, colour_ready, out, capacity);
}

int dm_tipx_width(const char *s) { return s ? vis_len(s, strlen(s)) : 0; }
int dm_tipx_join(const char *const *part, int n, const char *sep, int max_chars, char *out, size_t cap) {
    size_t w = 0; int k = 0, width = 0, sw = dm_tipx_width(sep);
    if (!cap) return 0;
    out[0] = 0;
    for (; k < n; k++) {
        int pw = dm_tipx_width(part[k]); size_t need = strlen(part[k]) + (k ? strlen(sep) : 0);
        if (k && ((max_chars > 0 && width + sw + pw > max_chars) || w + need + 1 > cap)) break;
        if (!k && need + 1 > cap) break;
        w += (size_t)snprintf(out + w, cap - w, "%s%s", k ? sep : "", part[k]); width += (k ? sw : 0) + pw;
    }
    return k;
}


static int line_is(const char *orig, size_t from, size_t to, const char *want) {
    const unsigned char *p = (const unsigned char *)orig; size_t i = from, wn = strlen(want), k = 0;
    while (i < to && (p[i] == ' ' || is_cc(p, i, to))) i += p[i] == ' ' ? 1 : 4;
    while (wn && want[wn - 1] == ' ') wn--;
    while (*want == ' ' && wn) { want++; wn--; }
    for (; i < to; ) {
        if (is_cc(p, i, to)) { i += 4; continue; }
        if (k < wn && p[i] == (unsigned char)want[k]) { i++; k++; continue; }
        if (k < wn) return 0;
        while (i < to && (p[i] == ' ' || is_cc(p, i, to))) i += p[i] == ' ' ? 1 : 4;
        return i >= to;
    }
    return k == wn && wn > 0;
}
size_t dm_tipx_annotate(const char *orig, const dm_tipx_ann *a, int n, int max_chars, char *out, size_t cap, dm_tipx_span *span, int max_span, int *nspan, int *placed) {
    enum { MAXA = 64 };
    size_t on = strlen(orig), at[MAXA]; const char *put[MAXA]; char col[MAXA][4]; int ord[MAXA], np = 0;
    *nspan = 0;
    for (int i = 0; i < n; i++) placed[i] = DM_TIPX_ANN_NONE;
    if (n > MAXA) n = MAXA;
    for (int i = 0; i < n; i++) {
        if (!a[i].line || !a[i].line[0] || !a[i].text) continue;
        size_t end = 0; int hits = 0;
        for (size_t from = 0; from <= on; ) {
            const char *nl = strchr(orig + from, '\n'); size_t to = nl ? (size_t)(nl - orig) : on;
            if (line_is(orig, from, to, a[i].line)) { hits++; end = to; }
            if (!nl) break;
            from = to + 1;
        }
        if (hits != 1) { placed[i] = hits ? DM_TIPX_ANN_MANY : DM_TIPX_ANN_NONE; continue; }
        int taken = 0; for (int k = 0; k < np; k++) taken |= at[k] == end;
        if (taken) continue;
        size_t from = end; while (from > 0 && orig[from - 1] != '\n') from--;
        int width = vis_len(orig + from, end - from) + 1;
        const char *t = a[i].text;
        if (max_chars > 0 && width + vis_len(t, strlen(t)) > max_chars) t = a[i].brief && width + vis_len(a[i].brief, strlen(a[i].brief)) <= max_chars ? a[i].brief : NULL;
        if (!t) { placed[i] = DM_TIPX_ANN_WIDE; continue; }
        const char *lc = last_cc(orig, end);
        if (!lc || (size_t)(lc - orig) + 4 > end) { placed[i] = DM_TIPX_ANN_COLOUR; continue; }
        if (np >= max_span) break;
        at[np] = end; put[np] = t; memcpy(col[np], lc, 4); ord[np] = i; np++;
    }

    for (int x = 1; x < np; x++) for (int y = x; y > 0 && at[y - 1] > at[y]; y--) {
        size_t ta = at[y]; at[y] = at[y - 1]; at[y - 1] = ta; const char *tp = put[y]; put[y] = put[y - 1]; put[y - 1] = tp;
        char tc[4]; memcpy(tc, col[y], 4); memcpy(col[y], col[y - 1], 4); memcpy(col[y - 1], tc, 4); int to = ord[y]; ord[y] = ord[y - 1]; ord[y - 1] = to; }
    size_t w = 0, from = 0;
    for (int k = 0; k <= np; k++) {
        size_t upto = k < np ? at[k] : on, len = upto - from;
        if (w + len + 1 > cap) { *nspan = 0; for (int i = 0; i < n; i++) if (placed[i] > 0) placed[i] = DM_TIPX_ANN_NONE; return 0; }
        memcpy(out + w, orig + from, len); w += len; from = upto;
        if (k == np) break;
        size_t tl = strlen(put[k]);
        if (w + 1 + tl + 4 + 1 > cap) { *nspan = 0; for (int i = 0; i < n; i++) if (placed[i] > 0) placed[i] = DM_TIPX_ANN_NONE; return 0; }
        span[k].at = w; span[k].len = 1 + tl;
        out[w++] = ' '; memcpy(out + w, put[k], tl); w += tl; memcpy(out + w, col[k], 4); w += 4;
        placed[ord[k]] = DM_TIPX_ANN_PLACED;
    }
    out[w] = 0; *nspan = np;
    return w;
}

int dm_tipx_colour_rgb(const char *code, size_t offset, const uint32_t colours[4][3], uint64_t *regs) {
    if (!code || offset < 8 || memcmp(code - 11, DM_TIPX_COLOUR_TAG "\xee\x81\xbe", 11)) return 0;
    int i = dm_tipx_colour_index(*code);
    if (i < 0) return 0;
    regs[8] = colours[i][0]; regs[10] = colours[i][1]; regs[9] = colours[i][2];
    return 1;
}

static int round_away(float v) { return v >= 0 ? (int)(v + 0.5f) : -(int)(-v + 0.5f); }
void dm_tipx_place(const dm_tipx_place_in *p, int *left, int *top) {
    int above = p->above, x, y;
    for (;;) {
        x = p->centre ? p->tx + p->tw / 2 - p->w / 2 : p->tx;
        y = above ? p->ty - p->h : p->ty + p->th;
        int dx = round_away(p->scale * (float)(above ? p->ox : -p->ox)), dy = round_away(p->scale * (float)(above ? p->oy : -p->oy));
        x += dx; y += dy;
        if (y >= 0 || !above) break;
        above = 0;
    }
    int max_x = p->screen_w - p->pad - p->w, max_y = p->screen_h - p->pad - p->h;
    *left = x < p->pad ? p->pad : x > max_x ? max_x : x;
    *top = y < p->pad ? p->pad : y > max_y ? max_y : y;
}
