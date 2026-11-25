


#include "dm_itemcmp_core.h"
#include "dm_plural.h"
#include <stdio.h>
#include <string.h>

const int dm_ic_stat_id[IC_S_N] = { 0, 1, 2, 3, 7, 9, 39, 40, 41, 42, 43, 44, 45, 46, 60, 62, 79, 80, 93, 96, 99, 102, 105, 127, 83, 76, 77 };
#define CC "\xee\x81\xbe"
#define MINUS "-"

int dm_ic_metric_of_stat(int st) {
    switch (st) {
    case 105: return IC_M_FCR; case 99: return IC_M_FHR; case 102: return IC_M_FBR; case 93: return IC_M_IAS;
    case 39: case 40: return IC_M_RES_FIRE; case 43: case 44: return IC_M_RES_COLD; case 41: case 42: return IC_M_RES_LIGHT; case 45: case 46: return IC_M_RES_POIS;
    case 7: return IC_M_LIFE; case 9: return IC_M_MANA; case 127: return IC_M_ALLSK; case 83: return IC_M_CLSK;
    case 0: return IC_M_STR; case 2: return IC_M_DEX; case 3: return IC_M_VIT; case 1: return IC_M_ENE;
    case 80: return IC_M_MF; case 79: return IC_M_GF; case 96: return IC_M_FRW; case 60: return IC_M_LL; case 62: return IC_M_ML;
    default: return -1; }
}
int dm_ic_res_cap(int maxres) { int c = 75 + maxres; return c > 95 ? 95 : c; }
int dm_ic_res_shown(int total, int maxres, int penalty) { int v = total + penalty, c = dm_ic_res_cap(maxres); return v > c ? c : v < -100 ? -100 : v; }

static int num(char *o, size_t n, int v) { return v < 0 ? snprintf(o, n, MINUS "%d", -v) : snprintf(o, n, "%d", v); }
static int signed_num(char *o, size_t n, int v) { return v < 0 ? snprintf(o, n, MINUS "%d", -v) : snprintf(o, n, "+%d", v); }





int dm_ic_join(char *o, size_t n, const char *fmt, const char *before, const char *after) {
    int k = 0; const char *p;
    if (!fmt) fmt = "";
    for (p = strchr(fmt, '%'); p; p = strchr(p + 2, '%')) { if (p[1] != 's') { k = -1; break; } k++; }
    if (k != 2) fmt = "%s to %s";
    size_t w = 0; int arg = 0; if (!n) return 0; o[0] = 0;
    for (p = fmt; *p; ) {
        const char *s = NULL; size_t len;
        if (p[0] == '%' && p[1] == 's') { s = arg++ ? after : before; if (!s) s = ""; len = strlen(s); p += 2; }
        else { const char *q = strchr(p, '%'); len = q ? (size_t)(q - p) : strlen(p); s = p; p += len; }
        if (w + len >= n) len = n - 1 - w;
        memcpy(o + w, s, len); w += len; o[w] = 0;
    }
    return (int)w;
}

int dm_ic_unproven_text(char *o, size_t n, const char *fmt, unsigned mask, int attack, const char *const *labels) {
    if (!n) return 0; o[0] = 0;
    if (attack) mask &= ~(1u << IC_C_AR | 1u << IC_C_DMG);
    char names[192]; size_t k = 0; names[0] = 0;
    for (int m = 0; m < IC_C_N; m++) if (mask >> m & 1 && labels && labels[m] && labels[m][0] && k < sizeof names) k += (size_t)snprintf(names + k, sizeof names - k, "%s%s", k ? ", " : "", labels[m]);
    if (!names[0]) return 0;
    int count = 0; const char *p;
    if (!fmt) fmt = "";
    for (p = strchr(fmt, '%'); p; p = strchr(p + 2, '%')) { if (p[1] != 's') { count = -1; break; } count++; }
    if (count != 1) fmt = "%s: not computed";
    p = strstr(fmt, "%s");
    snprintf(o, n, "%.*s%s%s", (int)(p - fmt), fmt, names, p + 2);
    return (int)strlen(o);
}
int dm_ic_insert(void *base, size_t size, int n, int cap, int at) {
    if (n >= cap || at < 0 || at > n) return n;
    unsigned char *b = base; memmove(b + (size_t)(at + 1) * size, b + (size_t)at * size, (size_t)(n - at) * size);
    return n + 1;
}

int dm_ic_head_stays(int removing, int above, int below, int marks) { return above > 0 || marks || (removing && below > 0); }

int dm_ic_shown(const int *t, int m, int penalty) {
    switch (m) {
    case IC_M_FCR: return t[IC_S_FCR]; case IC_M_FHR: return t[IC_S_FHR]; case IC_M_FBR: return t[IC_S_FBR]; case IC_M_IAS: return t[IC_S_IAS];
    case IC_M_RES_FIRE: return dm_ic_res_shown(t[IC_S_FIRE], t[IC_S_MAXFIRE], penalty); case IC_M_RES_COLD: return dm_ic_res_shown(t[IC_S_COLD], t[IC_S_MAXCOLD], penalty);
    case IC_M_RES_LIGHT: return dm_ic_res_shown(t[IC_S_LIGHT], t[IC_S_MAXLIGHT], penalty); case IC_M_RES_POIS: return dm_ic_res_shown(t[IC_S_POIS], t[IC_S_MAXPOIS], penalty);
    case IC_M_LIFE: return t[IC_S_HP] >> 8; case IC_M_MANA: return t[IC_S_MANA] >> 8;
    case IC_M_ALLSK: return t[IC_S_ALLSK]; case IC_M_CLSK: return t[IC_S_CLSK];
    case IC_M_STR: return t[IC_S_STR]; case IC_M_DEX: return t[IC_S_DEX]; case IC_M_VIT: return t[IC_S_VIT]; case IC_M_ENE: return t[IC_S_ENE];
    case IC_M_MF: return t[IC_S_MF]; case IC_M_GF: return t[IC_S_GF]; case IC_M_FRW: return t[IC_S_FRW]; case IC_M_LL: return t[IC_S_LL]; case IC_M_ML: return t[IC_S_ML];
    default: return 0; }
}
void dm_ic_after(const dm_ic_in *in, int *after) {
    for (int i = 0; i < IC_S_N; i++) after[i] = in->tot[i] - in->old_[i] + in->new_[i];
    if (in->composed) return;
    int dvit = after[IC_S_VIT] - in->tot[IC_S_VIT], dene = after[IC_S_ENE] - in->tot[IC_S_ENE];
    after[IC_S_HP] += dvit * in->lpv * 64; after[IC_S_MANA] += dene * in->mpm * 64;
}
int dm_ic_compute(const dm_ic_in *in, const char *const *labels, const char *cap_word, const char *to_fmt, dm_ic_line *out, int max) {
    int after[IC_S_N]; dm_ic_after(in, after);
    int n = 0;
    for (int m = 0; m < IC_M_N && n < max; m++) {
        if ((in->blocked | IC_M_HIDDEN) & (1u << m)) continue;
        int b = dm_ic_shown(in->tot, m, in->penalty), a = dm_ic_shown(after, m, in->penalty), cap = 0;
        int pct = m <= IC_M_RES_POIS || (m >= IC_M_MF && m <= IC_M_ML);
        if (m >= IC_M_RES_FIRE && m <= IC_M_RES_POIS) { static const int mx[4] = { IC_S_MAXFIRE, IC_S_MAXCOLD, IC_S_MAXLIGHT, IC_S_MAXPOIS }; int cb = dm_ic_res_cap(in->tot[mx[m - IC_M_RES_FIRE]]), ca = dm_ic_res_cap(after[mx[m - IC_M_RES_FIRE]]);
            cap = a >= ca && b < cb ? 1 : b >= cb && a < ca ? -1 : 0; }
        if (a == b) continue;
        dm_ic_line *l = &out[n++]; l->metric = m; l->before = b; l->after = a; l->cap = cap;
        char d[24], sb[32], sa[32], ba[96]; signed_num(d, sizeof d, a - b); num(sb, sizeof sb, b); num(sa, sizeof sa, a);
        if (pct) { strcat(sb, " %"); strcat(sa, " %"); }
        dm_ic_join(ba, sizeof ba, to_fmt, sb, sa);
        char col = in->hypothetical ? '5' : a > b ? '3' : '1';
        int show_cap = cap > 0 && !in->hypothetical;
        snprintf(l->text, sizeof l->text, CC "%c%s " CC "%c%s (%s)%s%s", in->hypothetical ? '5' : '0', labels && labels[m] ? labels[m] : "?", col, d, ba,
                 show_cap ? " " CC "4" : "", show_cap ? cap_word : "");
    }
    return n;
}

int dm_ic_req_value(int table_req, int percent, int ethereal) { return table_req + table_req * percent / 100 - (ethereal ? 10 : 0); }
int dm_ic_req_outcome(int req, int have_now, int have_after, int own) {
    int r[2];
    for (int v = 0; v < 2; v++) { int sub = v ? own : 0, ok0 = have_now - sub >= req, ok1 = have_after - sub >= req; r[v] = ok0 == ok1 ? IC_REQ_SAME : ok1 ? IC_REQ_GAIN : IC_REQ_LOSS; }
    return r[0] == r[1] ? r[0] : IC_REQ_AMBIG;
}
int dm_ic_req_kept(int req, int total_now, int displaced) { return !req || total_now - displaced >= req; }
void dm_ic_skill_order(dm_ic_sk *s, int n) {
    for (int i = 1; i < n; i++) { dm_ic_sk x = s[i]; int j = i - 1;
        for (; j >= 0; j--) { const dm_ic_sk *y = &s[j]; int before = x.key >= 0 ? y->key < 0 || x.key < y->key : y->key < 0 && (x.base > y->base || (x.base == y->base && x.id < y->id)); if (!before) break; s[j + 1] = s[j]; }
        s[j + 1] = x; }
}
int dm_ic_pack(const int *kind, int n, int cap, int lost, char *keep, int *more_at) {
#define PRIO(k) ((k) == IC_K_END ? IC_K_MUST : (k))
    int cnt[4] = { 0 }, quota[4], more = 0; *more_at = n;
    for (int i = 0; i < n; i++) cnt[PRIO(kind[i])]++;
    for (int p = 0; p < 4; p++) quota[p] = cnt[p];
    if (n + lost > cap || lost) {
        int avail = cap - 1; for (int p = 0; p < 4; p++) { quota[p] = cnt[p] < avail ? cnt[p] : avail > 0 ? avail : 0; avail -= quota[p]; more += cnt[p] - quota[p]; }
        more += lost; }
    int used[4] = { 0 }, at = -1, endi = -1;
    for (int i = 0; i < n; i++) { int p = PRIO(kind[i]); keep[i] = used[p]++ < quota[p]; if (!keep[i] && at < 0) at = i; if (kind[i] == IC_K_END && endi < 0) endi = i; }
    if (more) *more_at = at >= 0 ? at : endi >= 0 ? endi : n;
    return more;
#undef PRIO
}

int dm_ic_hints(const int *kind, int n, char *drop) {
    int why = 0, out = 0;
    for (int i = 0; i < n; i++) { drop[i] = 0; if (kind[i] == IC_K_WHY && why++) drop[i] = 1; }
    for (int i = 0; i < n; i++) { if (why && kind[i] == IC_K_END) drop[i] = 1; out += drop[i]; }
    return out;
}

int dm_ic_slot(int want, int l1, int l2, const int worn[16]) {
    if (want > 0 && (want == l1 || want == l2)) return want;
    if (l1 == 6 && l2 == 7) return !worn[6] ? 6 : !worn[7] ? 7 : 6;
    if (l1 == l2 && (l1 == 1 || l1 == 2 || l1 == 3 || l1 == 8 || l1 == 9 || l1 == 10)) return l1;
    return 0;
}
int dm_ic_other_hand(int slot) { return slot == 4 ? 5 : slot == 5 ? 4 : slot == 11 ? 12 : slot == 12 ? 11 : -1; }
void dm_ic_hands_after(int slot, int mask, uint64_t cand, uint64_t right, uint64_t left, uint64_t out[2]) {
    out[0] = slot == 4 && cand ? cand : (mask >> 4 & 1) ? 0 : right;
    out[1] = slot == 5 && cand ? cand : (mask >> 5 & 1) ? 0 : left;
}
int dm_ic_hands_change(int slot, int mask, uint64_t cand, uint64_t right, uint64_t left) {
    uint64_t after[2]; dm_ic_hands_after(slot, mask, cand, right, left, after);
    return after[0] != right || after[1] != left;
}
int dm_ic_displaced(int code, int slot, int occ_slot, int occ_other) {
    int o = dm_ic_other_hand(slot); if (o < 0) occ_other = 0;
    switch (code) {
    case 1: return occ_slot ? -1 : 0;
    case 2: return !occ_slot && occ_other ? 1 << o : -1;
    case 5: return occ_slot ? 1 << slot : -1;
    case 7: return occ_slot && occ_other ? (1 << slot) | (1 << o) : -1;
    default: return -1; }
}
int dm_ic_charm_move(int is_charm, int own, int mode, int page) {
    if (!is_charm || mode != 0) return IC_CHARM_NONE;
    return own && page == 0 ? IC_CHARM_OUT : IC_CHARM_IN;
}
int dm_ic_panel_hide(const dm_ic_panel_in *in) {
    if (!((in->loc1 == 4 && in->loc2 == 5) || (in->loc1 == 5 && in->loc2 == 4)) || in->merc) return 0;
    if ((in->slot != 4 && in->slot != 5) || (in->code != 1 && in->code != 5)) return 0;

    if (!in->other_worn || in->other_code == 6 || in->pair != 0) return 0;
    return dm_ic_other_hand(in->slot);
}
int dm_ic_panel_memo_get(const dm_ic_panel_memo *m, uint64_t key, uint64_t frame) {
    return m->valid && m->key == key && frame >= m->frame && frame - m->frame < DM_IC_PANEL_FRAMES ? m->hide : -1;
}
void dm_ic_panel_memo_set(dm_ic_panel_memo *m, uint64_t key, uint64_t frame, int hide) { m->key = key; m->frame = frame; m->hide = hide; m->valid = 1; }


#define CB_LIM 0x8000




typedef struct { int overflow; } cb_ctx;
static int mul(cb_ctx *x, long long a, long long b) { long long r = a * b; if (r > 0x7fffffffLL || r < -0x7fffffffLL - 1) { x->overflow = 1; return 0; } return (int)r; }
static int add3(cb_ctx *x, long long a, long long b, long long c) { long long r = a + b + c; if (r > 0x7fffffffLL || r < -0x7fffffffLL - 1) { x->overflow = 1; return 0; } return (int)r; }
static int cb_fields_ok(const dm_ic_cb *c) {
    const int *p = (const int *)c; size_t n = sizeof *c / sizeof(int);
    for (size_t i = 0; i < n; i++) if (p[i] <= -CB_LIM || p[i] >= CB_LIM) return 0;
    return c->srcdam >= 0 && c->srcdam <= 255;
}
static int def_(cb_ctx *x, const dm_ic_cb *c) {
    int base = c->ac + c->dex / 4, q = mul(x, base, c->acpct) / 100, def = base + (base > 0 ? q : -q);
    if (c->acovr) { if (def > 0x100000 || c->acovr > 0x10000) x->overflow = 1;
        def = add3(x, def, mul(x, def, c->acovr) / 100, 0); }
    return def;
}
static int block_raw(cb_ctx *x, const dm_ic_cb *c) {
    int tb = c->toblock + c->bf;
    if (c->expansion) tb = mul(x, c->dex - 15, tb) / (2 * (c->lvl > 1 ? c->lvl : 1));
    return tb;
}
static int block_(cb_ctx *x, const dm_ic_cb *c) { if (!c->can_block) return 0; int tb = block_raw(x, c); return tb > 74 ? 75 : tb; }
static int ar_(cb_ctx *x, const dm_ic_cb *c) {
    int base = c->ar_zero ? 0 : add3(x, c->tohit, mul(x, 5, c->dex), c->thf - 35), pct = add3(x, add3(x, c->ar_mast, c->tohitpct, c->ar_skill), c->ar_prog, 0);
    return add3(x, base, mul(x, base, pct) / 100, 0);
}
static int mast_add(cb_ctx *x, int v, int m) { return m ? add3(x, v, mul(x, v, m) / 100, 0) : v; }
static void dmg_(cb_ctx *x, const dm_ic_cb *c, int *omn, int *omx) {
    int mn = 0, mx = 0, a, b;
    if (c->armed) { a = c->mode2 ? c->mn[1] : c->mn[0]; b = c->mode2 ? c->mx[1] : c->mx[0]; }
    else { a = c->mn[0] + 1; b = c->mx[0] + 2; }
    a += c->bmn; b += c->bmx;
    if (c->srcdam) {
        if (c->srcdam != 128) { a = mul(x, a, c->srcdam); b = mul(x, b, c->srcdam); a = (a < 0 ? a + 127 : a) >> 7; b = (b < 0 ? b + 127 : b) >> 7; }
        mn += a > 1 ? a : 1; mx += b > 2 ? b : 2; }
    int pct = add3(x, c->dpct, c->dmast, c->xpct), fl = c->xflat;
    if (c->armed) { int sb = c->strb == 1 ? 100 : c->strb, db = c->dexb == 1 ? 100 : c->dexb;
        if (c->strb) pct = add3(x, pct, mul(x, c->str, sb) / 100, 0);
        if (c->dexb) pct = add3(x, pct, mul(x, c->dex, db) / 100, 0);
        fl = add3(x, fl, c->flat, 0); }
    else pct = add3(x, pct, c->str, 0);
    if (pct <= -90) pct = -90;
    mn = add3(x, mul(x, mn, add3(x, 100, pct, c->edmin)) / 100, fl, 0); mx = add3(x, mul(x, mx, add3(x, 100, pct, c->edmax)) / 100, fl, 0);
    static const int el[4][3] = { { 0, 1, 0 }, { 2, 3, 1 }, { 6, 7, 2 }, { 4, 5, -1 } };
    for (int k = 0; k < 4; k++) { int lo = c->el[el[k][0]], hi = c->el[el[k][1]];
        if (el[k][2] >= 0 && c->mast[el[k][2]]) { lo = mast_add(x, lo, c->mast[el[k][2]]); hi = mast_add(x, hi, c->mast[el[k][2]]); }
        mn = add3(x, mn, lo < hi ? lo : hi, 0); mx = add3(x, mx, hi, 0); }
    if (c->el[10]) { int lo = mast_add(x, c->el[9], c->mast[3]), hi = mast_add(x, c->el[10], c->mast[3]);
        int len = c->plen_ovr > 0 ? c->plen_ovr : c->plen / (c->pcnt > 1 ? c->pcnt : 1);
        mn = add3(x, mn, mul(x, len, lo) >> 8, 0); mx = add3(x, mx, mul(x, len, hi) >> 8, 0); }
    if (mx >= 1) { if (mn < 1) mn = 1; if (!(mn < mx)) mx = mn + 1; }
    mn = add3(x, mn, c->sadd_mn, 0); mx = add3(x, mx, c->sadd_mx, 0); if (!(mn < mx)) mx = mn + 1;
    *omn = mn; *omx = mx;
}
int dm_ic_cb_eval(const dm_ic_cb *c, int *v) {
    if (!cb_fields_ok(c)) return 0;
    cb_ctx x = { 0 }; v[IC_C_DEF] = def_(&x, c); v[IC_C_BLK] = block_(&x, c); v[IC_C_AR] = ar_(&x, c); dmg_(&x, c, &v[IC_C_DMG], &v[IC_C_N]);
    return !x.overflow;
}
unsigned dm_ic_cb_new_cap(const dm_ic_cb *now, const dm_ic_cb *aft) {
    if (!cb_fields_ok(now) || !cb_fields_ok(aft)) return 1u << IC_C_BLK;
    cb_ctx x = { 0 };
    int cn = now->can_block && block_raw(&x, now) > 74, ca = aft->can_block && block_raw(&x, aft) > 74;
    return x.overflow || (ca && !cn) ? 1u << IC_C_BLK : 0;
}
int dm_ic_slot_group(int sl, int moved_is_weapon) {
    if (sl == 4 || (sl == 5 && moved_is_weapon)) return 0;
    if (sl == 5) return 1;
    if (sl == 2 || sl == 6 || sl == 7) return 3;
    return 2;
}
uint64_t dm_ic_cell_key(int cls, int metric, int sgroup, int charm, int hand_mode, int wtype, uint32_t sources, int cap_new, int set_comp) {
    if (wtype < 0 || wtype > 127 || cls < 0 || cls > 7) return 0;
    if (metric == IC_C_BLK) hand_mode = wtype = 0;
    uint64_t k = (uint64_t)cls << 29 | (uint64_t)metric << 27 | (uint64_t)sgroup << 25 | (uint64_t)hand_mode << 23 | (uint64_t)wtype << 16 | (uint64_t)sources;
    if (cap_new) k |= DM_IC_CELL_CAP_NEW;
    if (set_comp) k |= DM_IC_CELL_SET_COMP;
    if (charm) k |= DM_IC_CELL_CHARM;
    return k;
}
int dm_ic_cb_line(int m, int bmin, int bmax, int amin, int amax, const char *label, const char *cap_word, const char *mixed_word, const char *to_fmt, int hypo, char *out, size_t n) {
    char sb[48], sa[48], d[24], ba[128], c0 = hypo ? '5' : '0';
    if (m == IC_C_DMG) {
        char b1[24], b2[24], a1[24], a2[24]; num(b1, sizeof b1, bmin); num(b2, sizeof b2, bmax); num(a1, sizeof a1, amin); num(a2, sizeof a2, amax);
        char col = hypo ? '5' : amin >= bmin && amax >= bmax && (amin > bmin || amax > bmax) ? '3' : amin <= bmin && amax <= bmax && (amin < bmin || amax < bmax) ? '1' : '5';
        snprintf(sb, sizeof sb, "%s-%s", b1, b2); snprintf(sa, sizeof sa, "%s-%s", a1, a2); dm_ic_join(ba, sizeof ba, to_fmt, sb, sa);
        int mixed = (amin < bmin && amax > bmax) || (amin > bmin && amax < bmax);
        return snprintf(out, n, CC "%c%s " CC "%c%s%s%s", c0, label, col, ba,
                        mixed ? " · " : "", mixed && mixed_word ? mixed_word : "");
    }
    int cap = !hypo && m == IC_C_BLK && amin >= 75 && bmin < 75;
    char col = hypo || amin == bmin ? '5' : amin > bmin ? '3' : '1';
    signed_num(d, sizeof d, amin - bmin); num(sb, sizeof sb, bmin); num(sa, sizeof sa, amin);
    if (m == IC_C_BLK) { strcat(sb, " %"); strcat(sa, " %"); }
    dm_ic_join(ba, sizeof ba, to_fmt, sb, sa);
    return snprintf(out, n, CC "%c%s " CC "%c%s (%s)%s%s", c0, label, col, d, ba, cap ? " " CC "4" : "", cap ? cap_word : "");
}
unsigned dm_ic_cb_without(const dm_ic_cb *now, const int *game, int d_acpct, int d_tohit, int d_dpct, int *before, int *after) {
    dm_ic_cb b = *now; b.acpct -= d_acpct; b.tohit -= d_tohit; b.dpct -= d_dpct;
    memset(before, 0, (IC_C_N + 1) * sizeof *before); memset(after, 0, (IC_C_N + 1) * sizeof *after);
    if (!dm_ic_cb_eval(now, after) || !dm_ic_cb_eval(&b, before)) return 0;
    unsigned ok = 0;
    for (int m = 0; m < IC_C_N; m++) if (after[m] == game[m] && (m != IC_C_DMG || after[IC_C_N] == game[IC_C_N])) ok |= 1u << m;
    return ok;
}
void dm_ic_cb_minus(dm_ic_cb *c, const int *off) {
    c->str -= off[0]; c->dex -= off[2]; c->ac -= off[31]; c->acpct -= off[171] + off[16]; c->acovr -= off[182]; c->toblock -= off[20];
    c->tohit -= off[19]; c->tohitpct -= off[119];
    c->mn[0] -= off[21]; c->mx[0] -= off[22]; c->mn[1] -= off[23]; c->mx[1] -= off[24]; c->bmn -= off[185]; c->bmx -= off[186];
    c->dpct -= off[25]; c->edmax -= off[17]; c->edmin -= off[18];
    for (int i = 0; i < 11; i++) c->el[i] -= off[48 + i];
    for (int i = 0; i < 4; i++) c->mast[i] -= off[329 + i];
    c->plen_ovr -= off[101]; c->plen -= off[59]; c->pcnt -= off[326];
}
int dm_ic_label(const char *s, int kind, char *out, size_t n) {
    if (!s || !n) return 0;
    const char *p = s, *e = s + strlen(s);
    if (kind == 1) { const char *nl = strchr(s, '\n'); if (!nl) return 0; const char *a = s, *ae = nl, *b2 = nl + 1, *be = e;
        int ap = memchr(a, '%', (size_t)(ae - a)) != NULL, bp = memchr(b2, '%', (size_t)(be - b2)) != NULL; if (ap == bp) return 0;
        if (ap) { p = b2; e = be; } else { p = a; e = ae; } }
    else if (kind == 2) { const char *q = strstr(s, "\xef\xbc\x9a"), *q2 = strchr(s, ':'); if (q2 && (!q || q2 < q)) q = q2; if (!q) return 0; e = q; }
    while (p < e && *p == ' ') p++;
    while (e > p && e[-1] == ' ') e--;
    size_t len = (size_t)(e - p); int g = 0;
    if (!len || len >= n) return 0;
    for (const char *q = p; q < e; q++) { if (*q == '%' || *q == '\n' || *q == '\r') return 0; if (((unsigned char)*q & 0xc0) != 0x80) g++; }
    if (g > 24) return 0;
    memcpy(out, p, len); out[len] = 0; return 1;
}

int dm_ic_pkg_lines(const dm_ic_pkg_in *in, const dm_ic_pkg_words *w, dm_ic_pkg_line *out, int max) {
    int n = 0;
#define LINE(k_, ...) do { if (n < max) { out[n].kind = (k_); snprintf(out[n].text, sizeof out[n].text, __VA_ARGS__); n++; } } while (0)
    if (in->with && in->with[0] && w->with_fmt) { char t[200]; snprintf(t, sizeof t, w->with_fmt, in->with); LINE(IC_PKG_NOTE, CC "5%s", t); }
    dm_ic_in x; memset(&x, 0, sizeof x); x.penalty = in->penalty; x.composed = 1;
    for (int i = 0; i < IC_S_N; i++) { x.tot[i] = in->before[i]; x.new_[i] = in->after[i] - in->before[i]; }
    dm_ic_line v[IC_M_N]; int nv = dm_ic_compute(&x, w->labels, w->cap_word, w->to_fmt, v, IC_M_N);
    for (int i = 0; i < nv; i++) if (v[i].metric <= IC_M_MANA) LINE(IC_PKG_VAL, "%s", v[i].text);
    int others = 0, hidden = 0;
    for (int i = 0; i < in->nsk; i++) {
        const dm_ic_pkg_sk *k = &in->sk[i];
        if (k->before == k->after) continue;
        if (k->key < 0 && others >= 6) { hidden++; continue; }
        if (k->key < 0) others++;
        char t[160]; snprintf(t, sizeof t, w->skill_fmt, k->name, k->before, k->after);
        LINE(IC_PKG_VAL, CC "%c%s%s%s", k->after > k->before ? '6' : '1', t, k->after ? "" : " ", k->after ? "" : w->skill_gone);
    }
    if (hidden) { char t[96], f[96]; snprintf(t, sizeof t, dm_plural(w->skill_more, hidden, 0, f, sizeof f), hidden); LINE(IC_PKG_MORE, CC "5%s", t); }
    for (int m = 0; in->cb && m < IC_C_N; m++) {
        int bmn = in->cb_before[m], bmx = m == IC_C_DMG ? in->cb_before[IC_C_N] : 0, amn = in->cb_after[m], amx = m == IC_C_DMG ? in->cb_after[IC_C_N] : 0;
        if (bmn == amn && bmx == amx) continue;
        if (n < max) { out[n].kind = IC_PKG_VAL; dm_ic_cb_line(m, bmn, bmx, amn, amx, w->cb_label[m] ? w->cb_label[m] : "?", w->cap_word, w->mixed_word, w->to_fmt, 0, out[n].text, sizeof out[n].text); n++; }
    }
    if (in->splash && in->splash[0]) LINE(IC_PKG_VAL, "%s", in->splash);
    for (int i = 0; i < nv; i++) if (v[i].metric > IC_M_MANA) LINE(IC_PKG_VAL, "%s", v[i].text);
#undef LINE
    return n;
}
int dm_ic_splash_hand(const int weapon[2], int slot, int active) {
    if (weapon[0] && weapon[1]) return slot == 4 || slot == 5 ? slot - 4 : active == 0 || active == 1 ? active : -1;
    return weapon[0] ? 0 : weapon[1] ? 1 : -1;
}
int dm_ic_splash_line(int s0, int w0, int s1, int w1, int weapon, int change_ok, int hypo, int known, const char *label, const char *cone, const char *to_fmt, char *out, size_t n) {
    if (n) out[0] = 0;
    if (!known) return IC_SPLASH_NONE;
    char c0 = hypo ? '5' : '0', sb[32], sa[32], ba[96], d[24], part[160] = "";
    if (change_ok && (s0 != s1 || (weapon && s1 > 0 && w0 != w1))) {
        char col = hypo ? '5' : s1 != s0 ? (s1 > s0 ? '3' : '1') : w1 > w0 ? '3' : '1';
        if (s1 > 0 && w1 != w0) {
            if (s0 > 0) { snprintf(sb, sizeof sb, "%d\xc2\xb0", w0); snprintf(sa, sizeof sa, "%d\xc2\xb0", w1); dm_ic_join(ba, sizeof ba, to_fmt, sb, sa); snprintf(part, sizeof part, " \xc2\xb7 %s\xc2\xa0%s", cone, ba); }
            else snprintf(part, sizeof part, " \xc2\xb7 %s\xc2\xa0%d\xc2\xb0", cone, w1);
        }
        if (s1 == s0) { snprintf(out, n, CC "%c%s " CC "%c%d %%%s", c0, label, col, s1, part); return IC_SPLASH_CHANGE; }
        signed_num(d, sizeof d, s1 - s0); snprintf(sb, sizeof sb, "%d %%", s0); snprintf(sa, sizeof sa, "%d %%", s1); dm_ic_join(ba, sizeof ba, to_fmt, sb, sa);
        snprintf(out, n, CC "%c%s " CC "%c%s (%s)%s", c0, label, col, d, ba, part);
        return IC_SPLASH_CHANGE;
    }
    if (!weapon || s1 <= 0) return IC_SPLASH_NONE;
    snprintf(out, n, CC "%c%s: " CC "5%d %% \xc2\xb7 %s\xc2\xa0%d\xc2\xb0", c0, label, s1, cone, w1);
    return IC_SPLASH_OWN;
}
int dm_ic_block_on(int item_cmp, int combat_sim, int splash_on, int weapon) { return item_cmp || combat_sim || (splash_on && weapon); }
uint64_t dm_ic_splash_key(uint64_t key, int on, int kind, int share, int width) {
    uint64_t v = (uint64_t)(on != 0) | (uint64_t)(kind & 3) << 1 | (uint64_t)(share & 0xffff) << 3 | (uint64_t)(width & 0xffff) << 19;
    return (key ^ v) * 1099511628211ull;
}
int dm_ic_ranged_unproven(const int ranged_after[2], int current_snapshot) { return !current_snapshot && (ranged_after[0] || ranged_after[1]); }
void dm_ic_veto_add(dm_ic_veto *v, unsigned mask, const char *key, int display_only) {
    for (int i = 0; i < IC_C_N; i++) if (mask >> i & 1) {
        if (!(v->bad >> i & 1)) { v->bad |= 1u << i; v->why[i] = key; }
        if (!display_only && !(v->sim >> i & 1)) { v->sim |= 1u << i; v->sim_why[i] = key; }
    }
}
unsigned dm_ic_sim_swap_attack(const dm_ic_veto *v, unsigned live, const char **why) {
    unsigned r = 0; *why = NULL;
    for (int m = IC_C_AR; m <= IC_C_DMG; m++) if (v->sim >> m & 1) {
        const char *k = v->sim_why[m];
        if (k && !strcmp(k, "dmCmpPassive")) { r |= 1; continue; }
        r |= 2;
        if (!*why) *why = live & 16 ? "dmGearNoRanged" : !k ? NULL : !strcmp(k, "dmCmpDualWield") ? "dmGearNoDual" : !strcmp(k, "dmCmpLive") ? "dmGearNoLive" : NULL;
    }
    return r;
}
int dm_ic_mastery_moves(int type_now, int type_after, int mastery) { return mastery && (type_now < 0 || type_after < 0 || type_now != type_after); }
int dm_ic_one_after_mastery(int cls, const int weapon_now[2], const int kept[2], const int type_now[2], const int mastery_now[2], int type_after, int mastery_after) {
    if ((cls == 4 || cls == 6) && weapon_now[0] && weapon_now[1] && (kept[0] || kept[1])) return 0;
    for (int h = 0; h < 2; h++) if (weapon_now[h] && !kept[h] && dm_ic_mastery_moves(type_now[h], type_after, mastery_now[h] || mastery_after)) return 1;
    return 0;
}
const char *dm_ic_unrated_key(int read_rc, int now_valid, unsigned now_bad, unsigned after_bad, const char *attack_why) {
    (void)after_bad;
    return !read_rc && now_valid && !(now_bad & 1) && attack_why ? attack_why : "dmGearNoSim";
}

static int round_half_away(double v) { return v >= 0 ? (int)(v + 0.5) : -(int)(-v + 0.5); }
static double diminished(int v, int k) { return v > 0 ? (double)k * v / (k + v) : v; }
int dm_ic_effect(int m, int b, int a, int *pct) {
    double r;
    if (m >= IC_M_RES_FIRE && m <= IC_M_RES_POIS) r = (100.0 - a) / (100.0 - b) - 1;
    else if (m == IC_X_BLOCK) { if (b >= 100) return 0; r = (100.0 - a) / (100.0 - b) - 1; }
    else if (m == IC_M_LIFE || m == IC_M_MANA) { if (b <= 0) return 0; r = (double)a / b - 1; }
    else if (m == IC_M_MF) { double eb = 100 + diminished(b, 250), ea = 100 + diminished(a, 250); if (eb <= 0 || ea < 0) return 0; r = ea / eb - 1; }
    else if (m == IC_M_GF) { if (100 + b <= 0 || 100 + a < 0) return 0; r = (100.0 + a) / (100.0 + b) - 1; }
    else return 0;
    if (r * 100 > -1 && r * 100 < 1) return 0;
    *pct = round_half_away(r * 100);
    return 1;
}
int dm_ic_fill(const char *fmt, int v, char *out, size_t n) {
    size_t w = 0; if (!n) return 0;
    for (const char *p = fmt; p && *p; p++) {
        char t[16]; const char *s = t; size_t len;
        if (*p != '%') { t[0] = *p; t[1] = 0; len = 1; }
        else if (p[1] == '%') { t[0] = '%'; t[1] = 0; len = 1; p++; }
        else if (p[1] == 'd') { len = (size_t)num(t, sizeof t, v); p++; }
        else if (p[1] == '+' && p[2] == 'd') { len = (size_t)signed_num(t, sizeof t, v); p += 2; }
        else return 0;
        if (w + len >= n) return 0;
        memcpy(out + w, s, len); w += len;
    }
    out[w] = 0;
    return (int)w;
}
static int one_s(const char *f) {
    const char *p = f ? strchr(f, '%') : NULL; if (!p || p[1] != 's') return 0;
    return !strchr(p + 2, '%');
}
int dm_ic_fx_text(int m, int b, int a, int cap, int over, int hypo, const dm_ic_fx_words *w, dm_ic_fx *o) {
    o->bracket[0] = o->brief[0] = o->rest[0] = 0;
    if (m < 0 || m > IC_X_BLOCK) return 0;
    if (over > 0 && a == b) {
        char f[96]; const char *ov = w->over ? dm_plural(w->over, over, 0, f, sizeof f) : NULL;
        if (!ov || strchr(ov, '%') != strrchr(ov, '%') || !strstr(ov, "%d")) return 0;
        char t[96]; snprintf(t, sizeof t, ov, over);
        snprintf(o->bracket, sizeof o->bracket, CC "5(%s)", t); snprintf(o->brief, sizeof o->brief, "%s", o->bracket);
        return 1;
    }
    const char *f = w->fmt[m]; if (!one_s(f)) return 0;
    int speed = m <= IC_M_FBR || m == IC_M_FRW, pct, inverse = (m >= IC_M_RES_FIRE && m <= IC_M_RES_POIS) || m == IC_X_BLOCK;
    if (speed) pct = a - b; else if (!dm_ic_effect(m, b, a, &pct)) return 0;
    if (!pct) return 0;
    char col = hypo ? '5' : (pct < 0) == inverse ? '3' : '1', v[24], body[96], cw[48] = "";
    snprintf(v, sizeof v, "%s%d %%", pct < 0 ? MINUS : "+", pct < 0 ? -pct : pct);
    { const char *q = strstr(f, "%s"); snprintf(body, sizeof body, "%.*s%s%s", (int)(q - f), f, v, q + 2); }
    if (cap > 0 && !hypo && w->cap && w->cap[0]) snprintf(cw, sizeof cw, CC "4 \xc2\xb7 %s", w->cap);
    snprintf(o->rest, sizeof o->rest, CC "%c%s%s", col, body, cw);
    if (speed) return 1;
    snprintf(o->bracket, sizeof o->bracket, CC "%c(%s%s" CC "%c)", col, body, cw, col);
    const char *bf = inverse && m != IC_X_BLOCK && one_s(w->brief) ? w->brief : f, *q = strstr(bf, "%s");
    snprintf(body, sizeof body, "%.*s%s%s", (int)(q - bf), bf, v, q + 2);
    snprintf(o->brief, sizeof o->brief, CC "%c(%s%s" CC "%c)", col, body, cw, col);
    return 1;
}
int dm_ic_res_group(const int v[4], const dm_ic_fx *const fx[4]) {
    for (int k = 0; k < 4; k++) if (!v[k] || v[k] != v[0] || !fx[k] || !fx[k]->brief[0] || strcmp(fx[k]->brief, fx[0]->brief)) return 0;
    return 1;
}
int dm_ic_line_says(int m, int before, int after, int item_value) {
    return (m <= IC_M_FBR || m == IC_M_FRW) && item_value && after - before == item_value;
}
static int count_d(const char *f) {
    int n = 0;
    for (const char *p = f ? strchr(f, '%') : NULL; p; p = strchr(p + 2, '%')) { if (p[1] != 'd' && p[1] != '%') return -1; n += p[1] == 'd'; }
    return f ? n : -1;
}
int dm_ic_frames_text(int b, int a, int next, int hypo, const char *name, const dm_ic_frame_words *w, dm_ic_fx *o) {
    o->bracket[0] = o->brief[0] = o->rest[0] = 0;
    char body[96], col = hypo || a == b ? '5' : a < b ? '3' : '1';
    char pf[96];
    if (a != b) { const char *f = w->to ? dm_plural(w->to, a, 0, pf, sizeof pf) : NULL; if (count_d(f) != 2) return 0; snprintf(body, sizeof body, f, b, a); }
    else if (next > 0) { const char *f = w->keep_next ? dm_plural(w->keep_next, a, 0, pf, sizeof pf) : NULL; if (count_d(f) != 2) return 0; snprintf(body, sizeof body, f, a, next); }
    else { const char *f = w->keep ? dm_plural(w->keep, a, 0, pf, sizeof pf) : NULL; if (count_d(f) != 1) return 0; snprintf(body, sizeof body, f, a); }
    const char *nm = name && name[0] ? name : NULL;
    snprintf(o->bracket, sizeof o->bracket, CC "%c(%s%s%s)", col, nm ? nm : "", nm ? ": " : "", body);
    snprintf(o->brief, sizeof o->brief, CC "%c(%s)", col, body);
    snprintf(o->rest, sizeof o->rest, CC "%c%s%s%s", col, nm ? nm : "", nm ? ": " : "", body);
    return 1;
}
