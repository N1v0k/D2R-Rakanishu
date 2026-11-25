#include "dm_gear_compose.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { ST_STR = 0, ST_ENE = 1, ST_DEX = 2, ST_VIT = 3, ST_MAXHP = 7, ST_MAXMANA = 9, ST_HPPCT = 76, ST_MANAPCT = 77 };

void dm_gear_list_add(dm_gear_list *l, int stat, int layer, int value) {
    if (!value || stat < 0 || stat >= DM_GEAR_NSTAT) return;
    for (int i = 0; i < l->n; i++) if (l->v[i].stat == stat && l->v[i].layer == layer) { l->v[i].value += value; return; }
    if (l->n >= DM_GEAR_MAXV) { l->full = 1; return; }
    l->v[l->n].stat = (uint16_t)stat; l->v[l->n].layer = (uint16_t)layer; l->v[l->n].value = value; l->n++;
}
int dm_gear_list_get(const dm_gear_list *l, int stat, int layer) {
    for (int i = 0; i < l->n; i++) if (l->v[i].stat == stat && l->v[i].layer == layer) return l->v[i].value;
    return 0;
}
int dm_gear_list_sum(const dm_gear_list *l, int stat) {
    int s = 0;
    for (int i = 0; i < l->n; i++) if (l->v[i].stat == stat) s += l->v[i].value;
    return s;
}

static int fields(char *line, char **f, int max) {
    int k = 0;
    for (char *q = line; q && k < max; ) { f[k++] = q; q = strchr(q, '\t'); if (q) *q++ = 0; }
    return k;
}
static int number(const char *s, int *v) { char *e; long x = strtol(s, &e, 10); if (e == s || *e || x < -0x7fffffffL || x > 0x7fffffffL) return 0; *v = (int)x; return 1; }

int dm_gear_tables_parse(dm_gear_tables *t, const char *text) {
    memset(t, 0, sizeof *t);
    if (!text) return -1;
    int nline = 0, end = -1;
    for (const char *p = text; *p; ) {
        const char *e = strchr(p, '\n'); size_t len = e ? (size_t)(e - p) : strlen(p);
        char line[256]; if (len >= sizeof line) return -1;
        memcpy(line, p, len); line[len] = 0; p = e ? e + 1 : p + len; nline++;
        char *f[8]; int k = fields(line, f, 8), v[7] = { 0 };
        for (int i = 1; i < k && i < 8; i++) if (!number(f[i], &v[i - 1]) && !(f[0][0] == 'o' && i == 5) && nline > 1) return -1;
        if (nline == 1) { if (k < 3 || strcmp(f[0], "DM_GEAR") || v[0] != 1) return -1; continue; }
        if (end >= 0) return -1;
        if (!strcmp(f[0], "op") && k == 6) {
            if (t->nop >= DM_GEAR_MAXOP || v[0] < 0 || v[0] >= DM_GEAR_NSTAT) return -1;
            int j = t->nop++; t->op[j].stat = (int16_t)v[0]; t->op[j].op = (int16_t)v[1]; t->op[j].param = (int16_t)v[2]; t->op[j].level = (int16_t)v[3];
            for (char *q = f[5]; q && *q && t->op[j].ntg < 3; ) { int x; char *c = strchr(q, ','); if (c) *c = 0; if (!number(q, &x) || x < 0 || x >= DM_GEAR_NSTAT) return -1; t->op[j].tg[t->op[j].ntg++] = (int16_t)x; q = c ? c + 1 : NULL; }
            if (!t->op[j].ntg) return -1;
        } else if (!strcmp(f[0], "shift") && k == 3) {
            if (v[0] < 0 || v[0] >= DM_GEAR_NSTAT || v[1] < 0 || v[1] > 16) return -1;
            t->shift[v[0]] = (uint8_t)v[1];
        } else if (!strcmp(f[0], "set") && k == 3) {
            if (v[0] != t->nset || t->nset >= DM_GEAR_MAXSET || v[1] < 0) return -1;
            t->pieces[t->nset++] = (int16_t)v[1];
        } else if (!strcmp(f[0], "item") && k == 4) {
            if (v[0] != t->nitem || t->nitem >= DM_GEAR_MAXSETITEM || v[1] < 0 || v[1] >= t->nset || v[2] < 0 || v[2] > 2) return -1;
            t->item[t->nitem].set = (int16_t)v[1]; t->item[t->nitem].func = (int16_t)v[2]; t->nitem++;
        } else if ((!strcmp(f[0], "setb") || !strcmp(f[0], "itemb")) && k == 6) {
            int set = f[0][0] == 's'; int *nb = set ? &t->nsetb : &t->nitemb; dm_gear_bonus *b = set ? t->setb : t->itemb;
            if (*nb >= DM_GEAR_MAXBONUS || v[0] < 0 || v[0] >= (set ? t->nset : t->nitem) || v[2] < 0 || v[2] >= DM_GEAR_NSTAT || v[3] < 0 || v[3] > 0xffff) return -1;
            if (set ? v[1] < 0 || v[1] > 5 || v[1] == 1 : v[1] < 1 || v[1] > 5) return -1;
            b[*nb] = (dm_gear_bonus){ (int16_t)v[0], (int16_t)v[1], (int16_t)v[2], (int16_t)v[3], 0, v[4] }; (*nb)++;
        } else if ((!strcmp(f[0], "setx") || !strcmp(f[0], "itemx")) && k == 4) {
            int set = f[0][0] == 's'; int *nb = set ? &t->nsetb : &t->nitemb; dm_gear_bonus *b = set ? t->setb : t->itemb;
            if (*nb >= DM_GEAR_MAXBONUS || v[0] < 0 || v[0] >= (set ? t->nset : t->nitem) || v[2] < -1 || v[2] >= DM_GEAR_NSTAT) return -1;
            if (set ? v[1] < 0 || v[1] > 5 || v[1] == 1 : v[1] < 1 || v[1] > 5) return -1;
            b[*nb] = (dm_gear_bonus){ (int16_t)v[0], (int16_t)v[1], (int16_t)v[2], 0, 1, 0 }; (*nb)++;
        } else if (!strcmp(f[0], "end") && k == 2) end = v[0];
        else return -1;
    }
    if (end != nline || !t->nop || !t->nset || !t->nitem) { memset(t, 0, sizeof *t); return -1; }
    t->ok = 1;
    return 0;
}

static void note_x(dm_gear_char *o, int stat) {
    o->flags |= DM_GEAR_C_UNMAPPED;
    for (int i = 0; i < o->nx; i++) if (o->xstat[i] == stat) return;
    if (o->nx < 16) o->xstat[o->nx++] = (int16_t)stat;
}
static void add_bonus(dm_gear_list *raw, dm_gear_char *o, const dm_gear_bonus *b) {
    if (b->unmapped) note_x(o, b->stat); else dm_gear_list_add(raw, b->stat, b->layer, b->value);
}
static int worn_pieces(const dm_gear_char *o, int set) { for (int i = 0; i < o->nworn; i++) if (o->worn[i].set == set) return o->worn[i].pieces; return 0; }

static int green_on(const dm_gear_tables *t, const dm_gear_item *it, int n, const dm_gear_item *items, int count, const dm_gear_char *o) {
    int set = t->item[it->setitem].set, func = t->item[it->setitem].func;
    if (func == 2) return worn_pieces(o, set) >= n + 1;
    if (func != 1) return 0;

    int other = 0;
    for (int r = 0; r < t->nitem; r++) {
        if (t->item[r].set != set || r == it->setitem) continue;
        if (++other != n) continue;
        for (int j = 0; j < count; j++) if (o->active[j] && items[j].setitem == r) return 1;
        return 0;
    }
    return 0;
}

static int item_green(const dm_gear_tables *t, const dm_gear_item *it, int stat, const dm_gear_item *items, int count, const dm_gear_char *o) {
    if (it->setitem < 0) return 0;
    int v = 0;
    for (int g = 1; g <= 5; g++) {
        if (!green_on(t, it, g, items, count, o)) continue;
        if (it->green_read) { for (int k = 0; k < it->green[g - 1].n; k++) if (it->green[g - 1].v[k].stat == stat) v += it->green[g - 1].v[k].value; continue; }
        for (int k = 0; k < t->nitemb; k++) { const dm_gear_bonus *x = &t->itemb[k]; if (x->owner == it->setitem && x->level == g && !x->unmapped && x->stat == stat) v += x->value; }
    }
    return v;
}

static void evaluate(const dm_gear_tables *t, const dm_gear_base *b, const dm_gear_item *items, int n, dm_gear_char *o) {
    o->gear.n = o->gear.full = o->green.n = o->green.full = 0; o->nworn = 0; o->nx = 0; o->flags &= DM_GEAR_C_ORDER;
    for (int i = 0; i < n; i++) {
        if (!o->active[i]) continue;
        const dm_gear_item *it = &items[i];
        for (int k = 0; k < it->own.n; k++) dm_gear_list_add(&o->gear, it->own.v[k].stat, it->own.v[k].layer, it->own.v[k].value);
        if (it->own.full) o->flags |= DM_GEAR_C_FULL;
        if (it->setitem < 0) continue;
        int set = t->item[it->setitem].set, dup = 0;
        if (t->item[it->setitem].func == 1) o->flags |= DM_GEAR_C_FUNC1;
        for (int j = 0; j < i; j++) dup |= o->active[j] && items[j].setitem == it->setitem;
        if (dup) continue;
        int w = 0; for (; w < o->nworn && o->worn[w].set != set; w++) {}
        if (w == o->nworn) { o->worn[w].set = (int16_t)set; o->worn[w].pieces = 0; o->nworn++; }
        o->worn[w].pieces++;
    }
    for (int i = 0; i < n; i++) {
        const dm_gear_item *it = &items[i];
        if (!o->active[i] || it->setitem < 0) continue;
        for (int g = 1; g <= 5; g++) {
            if (!green_on(t, it, g, items, n, o)) continue;
            if (it->green_read) { for (int k = 0; k < it->green[g - 1].n; k++) { const dm_gear_v *v = &it->green[g - 1].v[k]; dm_gear_list_add(&o->gear, v->stat, v->layer, v->value); dm_gear_list_add(&o->green, v->stat, v->layer, v->value); } continue; }
            for (int k = 0; k < t->nitemb; k++) if (t->itemb[k].owner == it->setitem && t->itemb[k].level == g) { add_bonus(&o->gear, o, &t->itemb[k]); add_bonus(&o->green, o, &t->itemb[k]); }
        }
    }
    for (int w = 0; w < o->nworn; w++) {
        int set = o->worn[w].set, p = o->worn[w].pieces;
        for (int k = 0; k < t->nsetb; k++) {
            const dm_gear_bonus *sb = &t->setb[k];
            if (sb->owner == set && (sb->level ? p >= sb->level : p == t->pieces[set])) add_bonus(&o->gear, o, sb);
        }
    }
    o->raw = b->base;
    for (int k = 0; k < o->gear.n; k++) dm_gear_list_add(&o->raw, o->gear.v[k].stat, o->gear.v[k].layer, o->gear.v[k].value);
    if (o->raw.full || o->gear.full) o->flags |= DM_GEAR_C_FULL;
    memset(o->value, 0, sizeof o->value);
    for (int k = 0; k < o->raw.n; k++) if (!o->raw.v[k].layer) o->value[o->raw.v[k].stat] = o->raw.v[k].value;


    for (int k = 0; k < t->nop; k++) {
        int src = t->op[k].stat, op = t->op[k].op, v = dm_gear_list_sum(&o->gear, src);
        if (!v) continue;
        if (op == 2 && t->op[k].level) {

            long long add = ((long long)v * b->clvl) >> t->op[k].param;
            for (int g = 0; g < t->op[k].ntg; g++) o->value[t->op[k].tg[g]] += (int)add;
            o->value[src] -= dm_gear_list_get(&o->gear, src, 0);
        } else if (op == 4 && t->op[k].level) {


            int green = 0, own = 0; long long add = 0;
            for (int i = 0; i < n; i++) {
                if (!o->active[i]) continue;
                own += dm_gear_list_sum(&items[i].own, src);
                int gi = item_green(t, &items[i], src, items, n, o); green += gi;
                add += ((long long)gi * b->clvl) >> t->op[k].param;
            }
            if (v != green + own) o->flags |= DM_GEAR_C_ITEMOP;
            for (int g = 0; g < t->op[k].ntg; g++) o->value[t->op[k].tg[g]] += (int)add;
        } else if (op == 3 || op == 5 || op == 6 || op == 7) o->flags |= DM_GEAR_C_ITEMOP;
        else if (op == 11) o->flags |= DM_GEAR_C_PCT;
    }
    if (dm_gear_list_sum(&b->base, ST_HPPCT) || dm_gear_list_sum(&b->base, ST_MANAPCT)) o->flags |= DM_GEAR_C_PCT;
    o->value[ST_MAXHP] += (o->value[ST_VIT] - dm_gear_list_get(&b->base, ST_VIT, 0)) * b->lpv * 64;
    o->value[ST_MAXMANA] += (o->value[ST_ENE] - dm_gear_list_get(&b->base, ST_ENE, 0)) * b->mpm * 64;
}

static int contribution(const dm_gear_tables *t, const dm_gear_base *b, const dm_gear_item *items, int n, int i, const dm_gear_char *o, int stat, int per_level) {
    const dm_gear_item *it = &items[i];
    int v = dm_gear_list_get(&it->own, stat, 0), pl = per_level >= 0 ? dm_gear_list_get(&it->own, per_level, 0) : 0;
    if (it->setitem >= 0)
        for (int g = 1; g <= 5; g++) {
            if (!green_on(t, it, g, items, n, o)) continue;
            if (it->green_read) { for (int k = 0; k < it->green[g - 1].n; k++) { if (it->green[g - 1].v[k].layer) continue; if (it->green[g - 1].v[k].stat == stat) v += it->green[g - 1].v[k].value; if (it->green[g - 1].v[k].stat == per_level) pl += it->green[g - 1].v[k].value; } continue; }
            for (int k = 0; k < t->nitemb; k++) { const dm_gear_bonus *x = &t->itemb[k]; if (x->owner != it->setitem || x->level != g || x->unmapped || x->layer) continue; if (x->stat == stat) v += x->value; if (x->stat == per_level) pl += x->value; }
        }
    int shift = 3;
    for (int k = 0; k < t->nop; k++) if (t->op[k].stat == per_level) shift = t->op[k].param;
    return v + (int)(((long long)pl * b->clvl) >> shift);
}
static int per_level_of(const dm_gear_tables *t, int stat) {
    for (int k = 0; k < t->nop; k++) if (t->op[k].op == 2 && t->op[k].level && t->op[k].ntg == 1 && t->op[k].tg[0] == stat) return t->op[k].stat;
    return -1;
}

static int step(const dm_gear_tables *t, const dm_gear_base *b, const dm_gear_item *items, int n, dm_gear_char *o, int own, unsigned char *next) {
    evaluate(t, b, items, n, o);
    int pls = per_level_of(t, ST_STR), pld = per_level_of(t, ST_DEX), changed = 0;
    for (int i = 0; i < n; i++) {
        const dm_gear_item *it = &items[i];
        int s = o->value[ST_STR], d = o->value[ST_DEX];
        if (!own && o->active[i]) { s -= contribution(t, b, items, n, i, o, ST_STR, pls); d -= contribution(t, b, items, n, i, o, ST_DEX, pld); }
        next[i] = b->clvl >= it->req_lvl && (!it->req_str || s >= it->req_str) && (!it->req_dex || d >= it->req_dex);
        changed |= next[i] != o->active[i];
    }
    return changed;
}

static int settle(const dm_gear_tables *t, const dm_gear_base *b, const dm_gear_item *items, int n, int own, int start, dm_gear_char *o, unsigned char *res) {
    unsigned char next[DM_GEAR_MAXITEMS];
    memset(o->active, start, sizeof o->active);
    for (int round = 0; round < n + 2; round++) {
        if (!step(t, b, items, n, o, own, next)) { memcpy(res, o->active, DM_GEAR_MAXITEMS); return 0; }
        memcpy(o->active, next, sizeof next);
    }
    return -1;
}

int dm_gear_compose(const dm_gear_tables *t, const dm_gear_base *b, const dm_gear_item *items, int n, const unsigned char *given, dm_gear_char *o) {
    if (!t || !t->ok || !b || !o || n < 0 || n > DM_GEAR_MAXITEMS || (n && !items)) return -1;
    for (int i = 0; i < n; i++) if (items[i].setitem >= t->nitem || items[i].setitem < -1) return -1;
    memset(o, 0, sizeof *o);
    if (given) { memcpy(o->active, given, (size_t)n); evaluate(t, b, items, n, o); return 0; }

    unsigned char res[4][DM_GEAR_MAXITEMS] = { { 0 } };
    int bad = 0;
    for (int v = 0; v < 4; v++) bad |= settle(t, b, items, n, v >> 1, v & 1, o, res[v]) < 0;
    for (int v = 1; v < 4; v++) bad |= memcmp(res[v], res[0], (size_t)n) != 0;
    memcpy(o->active, res[0], sizeof o->active);
    evaluate(t, b, items, n, o);
    if (bad) o->flags |= DM_GEAR_C_ORDER;
    return 0;
}

void dm_gear_item_conversions(const dm_gear_tables *t, int clvl, const dm_gear_list *green, dm_gear_list *out) {
    for (int k = 0; k < t->nop; k++) {
        if (t->op[k].op != 4 || !t->op[k].level) continue;
        int v = dm_gear_list_sum(green, t->op[k].stat); if (!v) continue;
        long long add = ((long long)v * clvl) >> t->op[k].param;
        for (int g = 0; g < t->op[k].ntg; g++) dm_gear_list_add(out, t->op[k].tg[g], 0, (int)add);
    }
}
