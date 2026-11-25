









#include "dm_gear_equip.h"
#include "dm_feats.h"
#include "dm_gear_equip_rules.h"
#include "dm_gear_equip_tab.h"
#include "dm_gear_pages.h"
#include "dm_cursor.h"
#include "dm_itemjob.h"
#include "dm.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

int dm_pages_shown(void); int dm_pages_slot_n(int page); int dm_pages_slot_item(int page, int i, const uint8_t **bits, int *len, int *x, int *y);
unsigned dm_pages_flips(void); int dm_pages_settled(uint64_t *commit); void dm_pages_job_flip(int dir);


_Static_assert(DM_GE_STASH_W == DM_GPG_GRID_W && DM_GE_STASH_H == DM_GPG_GRID_H, "the equip job's stash grid is the stock's");

static uint64_t now_ms(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return (uint64_t)t.tv_sec * 1000ull + (uint64_t)t.tv_nsec / 1000000ull; }
static void glog(void *ctx, const char *line) { (void)ctx; LOG("[gear-equip] %s", line); }


static uint64_t inv_of(uint64_t u) { uint64_t inv = u ? *(uint64_t *)(u + 0x90) : 0; return inv && *(uint32_t *)inv == 0x01020304 ? inv : 0; }
static uint64_t first_of(uint64_t u) { uint64_t inv = inv_of(u); return inv ? *(uint64_t *)(inv + 0x10) : 0; }
static uint64_t next_of(uint64_t it) { uint64_t d = *(uint64_t *)(it + 0x10); return d ? *(uint64_t *)(d + 0xb0) : 0; }
static uint64_t held_of(uint64_t pl) { uint64_t inv = inv_of(pl), h = inv ? *(uint64_t *)(inv + 0x40) : 0; return h && *(uint32_t *)h == 4 ? h : 0; }
static uint64_t unit_of(uint8_t *b, uint32_t id, int type) { return id ? ((uint64_t (*)(uint64_t, uint32_t, uint32_t, uint32_t))(b + 0x45f5f0))((uint64_t)(uintptr_t)(b + 0x2d603a0 + type * 0x400), id & 0x7f, id, (uint32_t)type) : 0; }
static uint32_t id_of(uint64_t it) { return it ? *(uint32_t *)(it + 8) : 0; }
static uint32_t code_of(uint8_t *b, uint64_t it) { return ((uint32_t (*)(uint64_t))(b + 0x570260))(it); }
static void size_of(uint8_t *b, uint64_t it, int *w, int *h) {
    *w = *h = 1; uint8_t ver = *(uint8_t *)(it + 0x1bd); uint64_t t = ver >= 1 && ver <= 3 ? *(uint64_t *)(b + 0x2d64a18 + (uint64_t)ver * 16) : 0; uint32_t ci = *(uint32_t *)(it + 4);
    if (t && ci < *(uint32_t *)(t + 0x15c8)) { uint64_t rec = *(uint64_t *)(t + 0x15c0) + (uint64_t)ci * 0x1c0; *w = *(uint8_t *)(rec + 0x11e); *h = *(uint8_t *)(rec + 0x11f); }
}
static void cell_of(uint64_t it, int *x, int *y) { uint64_t pp = *(uint64_t *)(it + 0x38); *x = pp ? *(int32_t *)(pp + 0x10) : -1; *y = pp ? *(int32_t *)(pp + 0x14) : -1; }
static int stash_open(uint8_t *b) { return *(uint8_t *)(b + 0x2d5ff60) != 0; }

static dm_ge_loc where_of(uint64_t it, int shown) {
    dm_ge_loc l = { DM_GE_NOWHERE, -1, -1, -1 }; uint64_t d = *(uint64_t *)(it + 0x10); if (!d) return l;
    uint32_t mode = *(uint32_t *)(it + 0xc); int x, y;
    if (mode == 1) { int s = *(uint8_t *)(d + 0x54); if (s >= 1 && s <= DM_GO_GLOVES) { l.where = DM_GE_BODY; l.index = (int8_t)s; } return l; }
    if (mode != 0) return l;
    int page = *(uint8_t *)(d + 0x55); cell_of(it, &x, &y);
    if (page == 0) { l.where = DM_GE_INV; l.index = 0; }
    else if (page == 4 && shown >= 0) { l.where = DM_GE_PAGE; l.index = (int8_t)shown; }
    else return l;
    l.x = (int8_t)x; l.y = (int8_t)y; return l;
}


#define VH_CACHE 512
static struct { uint32_t id, code; uint64_t vh; } g_vh[VH_CACHE]; static int g_nvh;
static int vh_of(uint8_t *b, uint64_t it, uint64_t *vh) {
    uint32_t id = id_of(it), code = code_of(b, it);
    for (int i = 0; i < g_nvh; i++) if (g_vh[i].id == id && g_vh[i].code == code) { *vh = g_vh[i].vh; return 0; }
    if (dm_gear_pages_unit_vhash(b, id, vh)) return -1;
    if (g_nvh == VH_CACHE) g_nvh = 0;
    g_vh[g_nvh].id = id; g_vh[g_nvh].code = code; g_vh[g_nvh].vh = *vh; g_nvh++;
    return 0;
}


#define SZ_CACHE 2048
static struct { uint64_t vh; uint8_t w, h; } g_sz[SZ_CACHE];
static int g_builds = -1;

static int bits_size(uint8_t *b, const uint8_t *bits, int len, int *w, int *h) {
    uint64_t vh = dm_gear_pages_bits_vhash(bits, len); unsigned k = (unsigned)(vh ^ vh >> 29) & (SZ_CACHE - 1);
    for (unsigned i = 0; i < 8; i++) { unsigned j = (k + i) & (SZ_CACHE - 1); if (g_sz[j].vh == vh && g_sz[j].w) { *w = g_sz[j].w; *h = g_sz[j].h; return 0; } }
    if (!g_builds) return -2;
    if (g_builds > 0) g_builds--;
    if (dm_gear_pages_bits_size(b, bits, len, w, h)) return -1;
    unsigned j = k; for (unsigned i = 0; i < 8; i++) if (!g_sz[(k + i) & (SZ_CACHE - 1)].w) { j = (k + i) & (SZ_CACHE - 1); break; }
    g_sz[j].vh = vh; g_sz[j].w = (uint8_t)*w; g_sz[j].h = (uint8_t)*h;
    return 0;
}


static const char *g_fail = "";
static int fail(const char *why) { g_fail = why; return -1; }
const char *dm_gear_equip_fail(void) { return g_fail; }


typedef struct { int w, h; uint16_t used[DM_GO_GRIDW]; } grid_t;

static int mark(grid_t *g, int x, int y, int w, int h) {
    if (x < 0 || y < 0 || w < 1 || h < 1 || x + w > g->w || y + h > g->h) return fail("an item lies outside its grid");
    for (int r = y; r < y + h; r++) g->used[r] |= (uint16_t)(((1u << w) - 1u) << x);
    return 0;
}


static int grid_of(uint8_t *b, uint64_t pl, dm_ge_place p, int shown, grid_t *g) {
    memset(g, 0, sizeof *g);
    if (p.where == DM_GE_INV || (p.where == DM_GE_PAGE && p.index == shown)) {
        dm_ge_grid_size(p.where, &g->w, &g->h);
        for (uint64_t it = first_of(pl); it; it = next_of(it)) { if (*(uint32_t *)it != 4) continue; dm_ge_loc l = where_of(it, shown);
            if (l.where != p.where || l.index != p.index) continue; int w, h; size_of(b, it, &w, &h); if (mark(g, l.x, l.y, w, h)) return -1; }
        return 0;
    }
    if (p.where == DM_GE_TAB) {
        uint32_t ids[DM_GPG_TABS]; int nt = dm_gear_pages_tab_ids(ids); if (p.index >= nt) return fail("no such shared tab");
        uint64_t u = unit_of(b, ids[p.index], 0); if (!u) return fail("shared tab unit not found");
        dm_ge_grid_size(p.where, &g->w, &g->h);
        for (uint64_t it = first_of(u); it; it = next_of(it)) { if (*(uint32_t *)it != 4 || *(uint32_t *)(it + 0xc) != 0) continue; int x, y, w, h; cell_of(it, &x, &y); size_of(b, it, &w, &h); if (mark(g, x, y, w, h)) return -1; }
        return 0;
    }
    if (p.where == DM_GE_PAGE) {
        int n = dm_pages_slot_n(p.index); if (n < 0) return fail("page not loaded");
        dm_ge_grid_size(p.where, &g->w, &g->h);
        for (int i = 0; i < n; i++) { const uint8_t *bits; int len, x, y, w, h, e; if (dm_pages_slot_item(p.index, i, &bits, &len, &x, &y)) return -1;
            if ((e = bits_size(b, bits, len, &w, &h))) return e;
            if (mark(g, x, y, w, h)) return -1; }
        return 0;
    }
    return -1;
}
static int rect_free(const grid_t *g, int x, int y, int w, int h) {
    if (x < 0 || y < 0 || x + w > g->w || y + h > g->h) return 0;
    for (int r = y; r < y + h; r++) if (g->used[r] & (((1u << w) - 1u) << x)) return 0;
    return 1;
}


static uint64_t fnv(const void *p, size_t n, uint64_t h) { const uint8_t *d = p; for (size_t i = 0; i < n; i++) { h ^= d[i]; h *= 1099511628211ull; } return h; }
static uint64_t cap_hash(const dm_gear_equip_cap *c) {
    uint64_t h = fnv(&c->w.ngrid, sizeof c->w.ngrid, 1469598103934665603ull);
    for (int g = 0; g < c->w.ngrid; g++) { h = fnv(&c->gp[g], sizeof c->gp[g], h); h = fnv(c->w.grid[g].used, sizeof c->w.grid[g].used, h); }
    for (int i = 0; i < c->w.nitem; i++) { const dm_go_item *it = &c->w.item[i]; int v[5] = { it->slot, it->cell.grid, it->cell.x, it->cell.y, it->w | it->h << 8 };
        h = fnv(v, sizeof v, h); h = fnv(&c->vh[i], sizeof c->vh[i], h); }
    h = fnv(c->target, sizeof c->target, h); h = fnv(&c->shown, sizeof c->shown, h);
    return h ? h : 1;
}
static int grid_index(dm_gear_equip_cap *c, uint8_t *b, uint64_t pl, dm_ge_place p, int place) {
    for (int g = 0; g < c->w.ngrid; g++) if (c->gp[g].where == p.where && c->gp[g].index == p.index) { if (place) c->w.grid[g].place = 1; return g; }
    if (c->w.ngrid >= DM_GO_MAXGRID) return -1;
    grid_t gr; int e = grid_of(b, pl, p, c->shown, &gr); if (e) return e;
    int g = c->w.ngrid++; c->gp[g] = p; c->w.grid[g].w = (uint8_t)gr.w; c->w.grid[g].h = (uint8_t)gr.h; c->w.grid[g].place = (uint8_t)(place != 0);
    memcpy(c->w.grid[g].used, gr.used, sizeof gr.used);
    return g;
}
int dm_gear_equip_capture(uint8_t *b, uint64_t pl, const dm_ge_loc *src, dm_gear_equip_cap *c) {
    memset(c, 0, sizeof *c); g_fail = "no room for the world";
    if (!b || !pl) return fail("no player");
    c->shown = dm_pages_shown(); c->w.inv = 0;
    for (int s = 0; s < DM_GO_SLOTS; s++) c->target[s] = -1;
    { int e = grid_index(c, b, pl, (dm_ge_place){ DM_GE_INV, 0 }, 1); if (e) return e < -1 ? e : -1; }

    int8_t worn_at[DM_GO_SLOTS]; memset(worn_at, -1, sizeof worn_at);
    for (uint64_t it = first_of(pl); it; it = next_of(it)) {
        if (*(uint32_t *)it != 4) continue; dm_ge_loc l = where_of(it, c->shown); if (l.where != DM_GE_BODY) continue;
        if (c->w.nitem >= DM_GO_MAXITEM) return -1;
        int i = c->w.nitem++, w, h; size_of(b, it, &w, &h);
        c->w.item[i] = (dm_go_item){ (uint8_t)w, (uint8_t)h, DM_GO_H_NONE, l.index, { -1, -1, -1 } };
        c->unit[i] = id_of(it); c->code[i] = code_of(b, it); if (vh_of(b, it, &c->vh[i])) return fail("worn item not written");
        c->target[l.index] = worn_at[l.index] = (int8_t)i;
    }

    for (int s = 1; s <= DM_GO_GLOVES; s++) {
        dm_ge_loc l = src[s];
        if (l.where == DM_GE_BODY) {
            if (l.index != s) { if (l.index < 1 || l.index > DM_GO_GLOVES || worn_at[l.index] < 0) return fail("worn item not at its place"); c->target[s] = worn_at[l.index]; }
            continue;
        }
        if (l.where == DM_GE_NOWHERE) { c->target[s] = -1; continue; }
        if (c->w.nitem >= DM_GO_MAXITEM) return -1;
        dm_ge_place p = { l.where, l.index }; int g = grid_index(c, b, pl, p, 1); if (g < 0) return g;
        int i = c->w.nitem, w = 0, h = 0; uint64_t found = 0;
        if (l.where == DM_GE_PAGE && l.index != c->shown) {
            int n = dm_pages_slot_n(l.index);
            for (int k = 0; k < n; k++) { const uint8_t *bits; int len, x, y; if (dm_pages_slot_item(l.index, k, &bits, &len, &x, &y)) return -1;
                if (x == l.x && y == l.y) { int e = bits_size(b, bits, len, &w, &h); if (e) return e; c->vh[i] = dm_gear_pages_bits_vhash(bits, len); found = 1; break; } }
            c->unit[i] = 0; c->code[i] = 0;
        } else {
            uint64_t owner = pl;
            if (l.where == DM_GE_TAB) { uint32_t ids[DM_GPG_TABS]; int nt = dm_gear_pages_tab_ids(ids); owner = l.index < nt ? unit_of(b, ids[l.index], 0) : 0; }
            for (uint64_t it = first_of(owner); it && !found; it = next_of(it)) {
                if (*(uint32_t *)it != 4 || *(uint32_t *)(it + 0xc) != 0) continue; int x, y; cell_of(it, &x, &y);
                dm_ge_loc at = l.where == DM_GE_TAB ? l : where_of(it, c->shown);
                if (at.where != l.where || at.index != l.index || x != l.x || y != l.y) continue;
                found = it; size_of(b, it, &w, &h); c->unit[i] = id_of(it); c->code[i] = code_of(b, it); if (vh_of(b, it, &c->vh[i])) return fail("item not written");
            }
        }
        if (!found || w < 1 || h < 1) return fail("item not at its place");
        c->w.nitem++; c->w.item[i] = (dm_go_item){ (uint8_t)w, (uint8_t)h, DM_GO_H_NONE, 0, { (int8_t)g, l.x, l.y } };
        c->target[s] = (int8_t)i;
    }
    c->hash = cap_hash(c);
    return 0;
}




int dm_gear_equip_capture_want(uint8_t *b, uint64_t pl, const dm_gew_target *t, dm_gear_equip_cap *c) {
    static dm_ge_loc src[DM_GO_SLOTS]; static dm_gew_slot want[DM_GO_SLOTS]; int shown = dm_pages_shown(); g_fail = "";

    uint32_t worn[2] = { 0, 0 };
    for (uint64_t it = first_of(pl); it; it = next_of(it)) { if (*(uint32_t *)it != 4) continue; dm_ge_loc l = where_of(it, shown);
        if (l.where == DM_GE_BODY && (l.index == DM_GO_RRING || l.index == DM_GO_LRING)) worn[l.index - DM_GO_RRING] = id_of(it); }
    int swap = dm_gew_ring_swap(t, worn[0], worn[1]); if (swap < 0) return fail("other rings worn");
    memcpy(want, t->slot, sizeof want);
    if (swap) { want[DM_GO_RRING] = t->slot[DM_GO_LRING]; want[DM_GO_LRING] = t->slot[DM_GO_RRING]; }
    for (int s = 0; s < DM_GO_SLOTS; s++) {
        const dm_gew_slot *w = &want[s]; src[s] = (dm_ge_loc){ DM_GE_BODY, (int8_t)s, -1, -1 };
        if (s < 1 || s > DM_GO_GLOVES || w->kind == DM_GEW_KEEP) continue;
        if (w->kind == DM_GEW_EMPTY) { src[s] = (dm_ge_loc){ DM_GE_NOWHERE, -1, -1, -1 }; continue; }
        if (w->kind == DM_GEW_UNIT) {
            uint64_t it = first_of(pl); while (it && !(*(uint32_t *)it == 4 && id_of(it) == w->id)) it = next_of(it);
            if (!it) return fail("unit not found");
            src[s] = where_of(it, shown);

            int other = (s == DM_GO_RIGHT || s == DM_GO_LEFT) && src[s].where == DM_GE_BODY && src[s].index == DM_GO_RIGHT + DM_GO_LEFT - s;
            if (!other && (*(uint32_t *)(it + 0xc) != 0 || src[s].where != DM_GE_INV)) return fail("unit not in the inventory");
            continue;
        }
        int place = (int)(w->id >> 16 & 0xff), x = (int)(w->id & 0xff), y = (int)(w->id >> 8 & 0xff);
        if (place < DM_GPG_PAGES) src[s] = (dm_ge_loc){ DM_GE_PAGE, (int8_t)place, (int8_t)x, (int8_t)y };
        else if (place < DM_GPG_PLACES) src[s] = (dm_ge_loc){ DM_GE_TAB, (int8_t)(place - DM_GPG_PAGES), (int8_t)x, (int8_t)y };
        else return -1;
    }
    int e = dm_gear_equip_capture(b, pl, src, c); if (e) return e < -1 ? e : -1;
    for (int s = 1; s <= DM_GO_GLOVES; s++) if (want[s].kind == DM_GEW_STOCK && (c->target[s] < 0 || c->vh[c->target[s]] != want[s].vh)) return fail("value hash differs");
    return 0;
}


static int cand_of(const dm_gear_snap *s, const dm_gear_equip_cap *c, int i, const dm_gear_pick *now) {
    const dm_go_item *it = &c->w.item[i];
    if (it->slot) {
        for (int k = 0; k < s->p.ncand; k++) if (c->unit[i] && s->show[k].where == DM_GTAB_WORN && s->show[k].id == c->unit[i]) return k;
        int k = dm_ge_place_of_slot[it->slot]; return k >= 0 && it->slot != DM_GO_RRING && it->slot != DM_GO_LRING ? now->at[k] : -1; }
    const dm_ge_place *p = &c->gp[it->cell.grid];
    int place = p->where == DM_GE_PAGE ? p->index : p->where == DM_GE_TAB ? DM_GPG_PAGES + p->index : -1;
    uint32_t stock = place >= 0 ? DM_GTAB_STOCK_ID | (uint32_t)place << 16 | (uint32_t)it->cell.y << 8 | (uint32_t)it->cell.x : 0;
    for (int k = 0; k < s->p.ncand; k++) {
        if (c->unit[i] && s->show[k].id == c->unit[i]) return k;
        if (stock && s->show[k].id == stock && s->show[k].vhash == c->vh[i]) return k;
    }
    return -1;
}
int dm_gear_equip_plan(const dm_gear_snap *s, dm_gear_equip_cap *c, dm_go_plan *plan) { return dm_gear_equip_plan_seq(s, c, NULL, 0, plan); }
int dm_gear_equip_plan_seq(const dm_gear_snap *s, dm_gear_equip_cap *c, const int8_t *slots, int nslots, dm_go_plan *plan) {
    if (!s || !s->p.ready || s->state != DM_GTAB_OK) return DM_GO_E_INPUT;
    static _Thread_local dm_gear_pick now; dm_gear_pick_now(&s->p, &now);
    int16_t cand[DM_GO_MAXITEM];
    for (int i = 0; i < c->w.nitem; i++) {
        int k = cand_of(s, c, i, &now); cand[i] = (int16_t)k;
        int needed = 0; for (int sl = 1; sl <= DM_GO_GLOVES; sl++) needed |= c->target[sl] == i;
        if (k < 0 && (needed || c->w.item[i].slot == DM_GO_RIGHT || c->w.item[i].slot == DM_GO_LEFT)) return -10;
        c->w.item[i].hand = (uint8_t)(k >= 0 ? dm_ge_hand_of(&s->p, k) : DM_GO_H_NONE);
    }
    static _Thread_local dm_ge_rules rr; dm_go_rules r;
    if (dm_ge_rules_init(&rr, &s->p, cand, c->w.nitem, &r)) return DM_GO_E_INPUT;
    c->hash = cap_hash(c);
    return dm_gear_order_all_seq(&c->w, c->target, &r, slots, nslots, plan);
}


static struct {
    int on; dm_ge_core core; dm_gear_equip_cap cap;
    uint32_t unit[DM_GO_MAXITEM];
    uint64_t pl;

    int view_frames; uint64_t view_key;
    int worthy;
} g_job;
static atomic_int g_next_worthy;
void dm_gear_equip_worthy(int worthy) { atomic_store(&g_next_worthy, worthy != 0); }
static atomic_int g_state, g_why, g_done, g_changes;
int dm_gear_equip_state(int *why, int *done, int *changes) {
    if (why) *why = atomic_load(&g_why); if (done) *done = atomic_load(&g_done); if (changes) *changes = atomic_load(&g_changes);
    return atomic_load(&g_state);
}
#ifndef DM_PLAYER_BUILD
static dm_ge_fault g_fault;
static int g_fault_change;
static void qa_census(uint8_t *b, const char *when);
#endif


static void observe(uint8_t *b, uint64_t pl, dm_ge_seen *s) {
    dm_gear_equip_cap *c = &g_job.cap; int n = c->w.nitem;
    memset(s, 0, sizeof *s);
    s->now_ms = now_ms(); s->run_ok = pl == g_job.pl && pl; s->panel_open = stash_open(b); s->shown = dm_pages_shown();
    s->flips = dm_pages_flips(); s->settled = dm_pages_settled(&s->commits); s->code = -1; s->cell_free = -1;
    for (int i = 0; i < n; i++) { s->at[i] = (dm_ge_loc){ DM_GE_NOWHERE, -1, -1, -1 }; g_job.unit[i] = 0; }
    if (!s->run_ok) return;
    uint64_t held = held_of(pl);

    static struct { uint64_t it; dm_ge_loc l; uint64_t vh; int ok; } u[1024]; int nu = 0;
    for (uint64_t it = first_of(pl); it && nu < 1024; it = next_of(it)) { if (*(uint32_t *)it != 4 || it == held) continue; dm_ge_loc l = where_of(it, s->shown); if (l.where != DM_GE_NOWHERE) { u[nu].it = it; u[nu].l = l; nu++; } }
    if (held && nu < 1024) { u[nu].it = held; u[nu].l = (dm_ge_loc){ DM_GE_HAND, -1, -1, -1 }; nu++; }
    { uint32_t ids[DM_GPG_TABS]; int nt = dm_gear_pages_tab_ids(ids);
      for (int t = 0; t < nt; t++) { uint64_t tu = unit_of(b, ids[t], 0); for (uint64_t it = first_of(tu); it && nu < 1024; it = next_of(it)) {
          if (*(uint32_t *)it != 4 || *(uint32_t *)(it + 0xc) != 0) continue; int x, y; cell_of(it, &x, &y); u[nu].it = it; u[nu].l = (dm_ge_loc){ DM_GE_TAB, (int8_t)t, (int8_t)x, (int8_t)y }; nu++; } } }
    for (int k = 0; k < nu; k++) {
        u[k].ok = 0; uint32_t code = code_of(b, u[k].it); int want = 0;
        for (int i = 0; i < n && !want; i++) want = !c->code[i] || c->code[i] == code;
        if (want && !vh_of(b, u[k].it, &u[k].vh)) u[k].ok = 1;
    }
    unsigned char taken[1024]; memset(taken, 0, sizeof taken);
    for (int pass = 0; pass < 3; pass++) for (int i = 0; i < n; i++) {
        if (s->at[i].where != DM_GE_NOWHERE) continue;
        dm_ge_loc e = dm_ge_expect(&g_job.core, i, pass == 1);
        for (int k = 0; k < nu; k++) {
            if (taken[k] || !u[k].ok || u[k].vh != c->vh[i]) continue;
            dm_ge_loc l = u[k].l;
            int same = l.where == e.where && (l.where == DM_GE_HAND || (l.where == DM_GE_BODY ? l.index == e.index : l.index == e.index && l.x == e.x && l.y == e.y));
            if (pass < 2 && !same) continue;
            taken[k] = 1; s->at[i] = l; g_job.unit[i] = id_of(u[k].it); if (!c->code[i]) c->code[i] = code_of(b, u[k].it); break;
        }
    }

    for (int p = 0; p < 5; p++) {
        if (p == s->shown) continue;
        int m = dm_pages_slot_n(p);
        for (int k = 0; k < m; k++) { const uint8_t *bits; int len, x, y; if (dm_pages_slot_item(p, k, &bits, &len, &x, &y)) break;
            uint64_t vh = dm_gear_pages_bits_vhash(bits, len);
            for (int i = 0; i < n; i++) if (s->at[i].where == DM_GE_NOWHERE && c->vh[i] == vh) { s->at[i] = (dm_ge_loc){ DM_GE_PAGE, (int8_t)p, (int8_t)x, (int8_t)y }; break; } }
    }
    s->hand = held ? -2 : -1;
    for (int i = 0; i < n; i++) if (s->at[i].where == DM_GE_HAND) s->hand = i;
    const dm_go_move *m = dm_ge_pending(&g_job.core);
    if (m && m->op == DM_GO_PUT_SLOT && held && s->hand == m->item) s->code = ((int (*)(uint64_t, int, uint64_t, int, int))(b + 0x565950))(pl, m->slot, held, 0, 0);
    if (m && m->op == DM_GO_PUT_CELL && m->cell.grid >= 0 && m->cell.grid < c->w.ngrid) {
        grid_t g; const dm_go_item *it = &c->w.item[m->item];
        s->cell_free = !grid_of(b, pl, c->gp[m->cell.grid], s->shown, &g) && rect_free(&g, m->cell.x, m->cell.y, it->w, it->h);
    }
}
static void send(uint8_t *b, const uint8_t *p, int n) { ((void (*)(const uint8_t *, int))(b + 0x2ceab0))(p, n); }
static void carry_out(uint8_t *b, uint64_t pl, const dm_go_move *m) {
    const dm_gear_equip_cap *c = &g_job.cap; uint64_t held = held_of(pl);
    switch (m->op) {
    case DM_GO_PICK_SLOT: ((void (*)(uint64_t, int, int, int, int))(b + 0x3bb24c))(0, m->slot, 0, -1, 0); break;
    case DM_GO_PUT_SLOT: if (held) ((void (*)(uint64_t, int, int, int, int))(b + 0x3bb24c))(held, m->slot, 0, -1, 0); break;
    case DM_GO_PICK_CELL: case DM_GO_PUT_CELL: {
        dm_ge_place p = c->gp[m->cell.grid]; uint32_t id = m->op == DM_GO_PICK_CELL ? g_job.unit[m->item] : id_of(held);
        if (!id) { LOG("[gear-equip] item %d has no unit to move", m->item); break; }
        if (p.where == DM_GE_TAB) {
            uint32_t ids[DM_GPG_TABS]; int nt = dm_gear_pages_tab_ids(ids); if (p.index >= nt) break;
            uint8_t pk[13] = { m->op == DM_GO_PICK_CELL ? 0x46 : 0x45 }; uint32_t v[2] = { id, ids[p.index] }; memcpy(pk + 1, v, 8);
            if (m->op == DM_GO_PICK_CELL) { uint32_t xy = (uint32_t)m->cell.x | (uint32_t)m->cell.y << 16; memcpy(pk + 9, &xy, 4); }
            else { uint16_t xy[2] = { (uint16_t)m->cell.x, (uint16_t)m->cell.y }; memcpy(pk + 9, xy, 4); }
            send(b, pk, 13);
        } else {
            uint8_t pk[17] = { m->op == DM_GO_PICK_CELL ? 0x19 : 0x18 }; uint32_t v[4] = { id, (uint32_t)m->cell.x, (uint32_t)m->cell.y, p.where == DM_GE_INV ? 0u : 4u };
            memcpy(pk + 1, v, 16); send(b, pk, 17);
        }
        break; }
    }
}
int dm_gear_equip_start(uint8_t *b, uint64_t pl, const dm_gear_equip_cap *cap, const dm_go_plan *plan) {
    int worthy = atomic_exchange(&g_next_worthy, 0);
    if (g_job.on || dm_itemjob_owner()) return 3;
    if (!stash_open(b) || held_of(pl)) return 2;
    static dm_gear_equip_cap now; static dm_ge_loc src[DM_GO_SLOTS];
    g_nvh = 0; g_builds = -1;
    for (int s = 0; s < DM_GO_SLOTS; s++) { int i = cap->target[s]; src[s] = (dm_ge_loc){ DM_GE_NOWHERE, -1, -1, -1 };
        if (i < 0) continue; const dm_go_item *it = &cap->w.item[i];
        if (it->slot) { src[s].where = DM_GE_BODY; src[s].index = it->slot; continue; }
        src[s].where = cap->gp[it->cell.grid].where; src[s].index = cap->gp[it->cell.grid].index; src[s].x = it->cell.x; src[s].y = it->cell.y; }
    if (dm_gear_equip_capture(b, pl, src, &now)) return 1;
    for (int i = 0; i < now.w.nitem && i < cap->w.nitem; i++) now.w.item[i].hand = cap->w.item[i].hand;
    now.hash = cap_hash(&now);
    if (now.hash != cap->hash) { LOG("[gear-equip] the world changed since the plan (%016llx, planned on %016llx): nothing moves", (unsigned long long)now.hash, (unsigned long long)cap->hash); return 1; }
    if (!dm_itemjob_acquire(DM_ITEMJOB_EQUIP)) return 3;
    g_job.cap = *cap; g_job.pl = pl;
    g_job.core.log = glog; g_job.core.log_ctx = NULL;
    const dm_ge_fault *f = NULL;
#ifndef DM_PLAYER_BUILD
    static dm_ge_fault fc; fc = g_fault;
    for (int k = 0; g_fault_change > 0 && k < plan->nstep; k++) if (plan->step[k].change == g_fault_change - 1) { fc.step = k + 1; LOG("[gear-equip] fault: change %d starts at step %d", g_fault_change, k + 1); break; }
    f = &fc;
    qa_census(b, "start");
#endif
    for (int i = 0; i < cap->w.nitem; i++) { const dm_go_item *it = &cap->w.item[i]; char cs[5] = { 0 }; memcpy(cs, &cap->code[i], 4);
        dm_ge_loc l = { DM_GE_BODY, it->slot, -1, -1 }; if (!it->slot) { l.where = cap->gp[it->cell.grid].where; l.index = cap->gp[it->cell.grid].index; l.x = it->cell.x; l.y = it->cell.y; }
        LOG("[gear-equip] start item %d %s %016llx at %d %d %d,%d", i, cs, (unsigned long long)cap->vh[i], l.where, l.index, l.x, l.y); }
    for (int k = 0; k < plan->nstep; k++) { const dm_go_move *m = &plan->step[k].go;
        LOG("[gear-equip] plan step %d (change %d): op %d item %d slot %d swap %d cell %d:%d,%d", k + 1, plan->step[k].change, m->op, m->item, m->slot, m->swap, m->cell.grid, m->cell.x, m->cell.y); }
    if (dm_ge_begin(&g_job.core, &cap->w, plan, cap->gp, cap->shown, now_ms(), f)) { dm_itemjob_release(DM_ITEMJOB_EQUIP); return 4; }
    { int tab; uint32_t unit; dm_ui_stash_view(&tab, &unit); g_job.view_key = ~0ull; g_job.view_frames = 0;
      LOG("[gear-equip] stash view at the start: tab %d unit %u", tab, unit); }
    g_job.on = 1; g_job.worthy = worthy; atomic_store(&g_state, DM_GE_RUNNING); atomic_store(&g_done, 0); atomic_store(&g_changes, plan->nchange); atomic_store(&g_why, 0);
    return 0;
}



#define VIEW_FRAMES 45
static int view_step(int run, int tab, uint32_t unit) {
    uint64_t key = ((uint64_t)(uint32_t)run << 40) ^ ((uint64_t)(uint32_t)tab << 32) ^ unit;
    if (key != g_job.view_key) { g_job.view_key = key; g_job.view_frames = 0; }
    if (g_job.view_frames < 0) return 0;
    int r = dm_ui_stash_toward(tab, unit);
    if (r == 1) return 0;
    if (r < 0 || ++g_job.view_frames > VIEW_FRAMES) {
        LOG("[gear-equip] stash view: tab %d unit %u not reached (%s), the job goes on", tab, unit, r < 0 ? "cannot turn there" : "too many frames");
        g_job.view_frames = -1; return 0;
    }
    return 1;
}
static void job_pump(uint8_t *b, uint64_t pl) {
#ifndef DM_PLAYER_BUILD


    static long slow = -1; static uint64_t next;
    if (slow < 0) { const char *e = getenv("DM_TEST_GEAR_SLOW"); slow = e ? atol(e) : 0; if (slow) LOG("[gear-equip] DM_TEST_GEAR_SLOW: %ld ms after each move", slow); }
    if (slow && now_ms() < next) return;
#endif
    { dm_ge_place v;
      if (!held_of(pl) && dm_ge_view(&g_job.core, &v)) {
          uint32_t ids[DM_GPG_TABS]; int nt = v.where == DM_GE_TAB ? dm_gear_pages_tab_ids(ids) : 0;
          int run = g_job.core.mode << 8 | g_job.core.j;
          if (v.where == DM_GE_PAGE ? view_step(run, 0, 0) : v.index < nt && ids[v.index] && view_step(run, 1, ids[v.index])) return; } }
    dm_ge_seen s; observe(b, pl, &s);
    dm_ge_act a; dm_ge_step(&g_job.core, &s, &a);
    if (a.kind == DM_GE_A_SEND) carry_out(b, pl, &a.move);
    else if (a.kind == DM_GE_A_FLIP) dm_pages_job_flip(a.dir);
#ifndef DM_PLAYER_BUILD
    if (a.kind == DM_GE_A_SEND && slow) next = now_ms() + (uint64_t)slow;
#endif
    int why = 0, changes = 0, end = dm_ge_end(&g_job.core, &why, &changes);
    atomic_store(&g_done, changes);
    if (a.kind != DM_GE_A_END) return;
    for (int i = 0; i < g_job.cap.w.nitem; i++) { char cs[5] = { 0 }; memcpy(cs, &g_job.cap.code[i], 4);
        LOG("[gear-equip] item %d %s %016llx at %d %d %d,%d", i, cs, (unsigned long long)g_job.cap.vh[i], s.at[i].where, s.at[i].index, s.at[i].x, s.at[i].y); }
    LOG("[gear-equip] end %s why %s changes %d of %d", dm_ge_end_name(end), dm_ge_why_name(why), changes, g_job.cap.w.nitem ? g_job.core.p.nchange : 0);
    if (end == DM_GE_LOST) LOG("ASSUME bestgear.equip_lost: an item of the equip job is nowhere to be found");
    if (end == DM_GE_IN_HAND) LOG("[gear-equip] the item in hand goes to the hand guards (inventory when there is room, never the ground)");
#ifndef DM_PLAYER_BUILD
    qa_census(b, "end");
#endif

    g_job.on = 0; atomic_store(&g_why, why);
    dm_itemjob_release(DM_ITEMJOB_EQUIP);
    dm_gear_tab_jobs(dm_itemjob_ends());
    atomic_store(&g_state, end);
    if (end == DM_GE_DONE && changes > 0 && g_job.worthy) dm_feats_event(DM_FT_E_EQUIP);
}

#ifndef DM_PLAYER_BUILD






static void qa_census(uint8_t *b, const char *when) {
    static int on = -1; if (on < 0) on = getenv("DM_TEST_GEAR_INV") != NULL; if (!on) return;
    int n = 0; uint64_t h = 0;
    if (dm_gear_pages_census(b, &n, &h)) LOG("[gear-equip] items unreadable (%s)", when);
    else LOG("[gear-equip] items %d %016llx (%s)", n, (unsigned long long)h, when);
}
static struct { pthread_mutex_t mu; pthread_cond_t cv; int want, have, rc, thread; dm_gear_equip_cap cap; dm_go_plan plan; } g_qa = { .mu = PTHREAD_MUTEX_INITIALIZER, .cv = PTHREAD_COND_INITIALIZER };
static void *qa_planner(void *unused) {
    (void)unused;
    static dm_gear_equip_cap cap; static dm_go_plan plan;
    for (;;) {
        pthread_mutex_lock(&g_qa.mu); while (!g_qa.want) pthread_cond_wait(&g_qa.cv, &g_qa.mu); cap = g_qa.cap; g_qa.want = 0; pthread_mutex_unlock(&g_qa.mu);
        int rc = DM_GO_E_INPUT;
        for (int tries = 0; tries < 150; tries++) {
            dm_gear_snap *s = dm_gear_tab_hold();
            if (s) { dm_gear_equip_cap c = cap; rc = dm_gear_equip_plan(s, &c, &plan); if (rc != -10 && rc != DM_GO_E_INPUT) cap = c; dm_gear_tab_unhold(s); }
            if (rc != -10 && rc != DM_GO_E_INPUT) break;
            usleep(200000);
        }
        pthread_mutex_lock(&g_qa.mu); g_qa.cap = cap; g_qa.plan = plan; g_qa.rc = rc; g_qa.have = 1; pthread_mutex_unlock(&g_qa.mu);
    }
    return NULL;
}
static int slot_named(const char *n) {
    static const char *const k[] = { "", "head", "neck", "torso", "right", "left", "rring", "lring", "belt", "feet", "gloves" };
    for (int s = 1; s <= DM_GO_GLOVES; s++) if (!strcmp(n, k[s])) return s;
    return 0;
}
static void qa_pump(uint8_t *b, uint64_t pl) {
    static int init, stage, tries, wanted; static const char *file; static dm_ge_loc src[DM_GO_SLOTS]; static uint64_t t_census;
    if (!init) { init = 1; const char *e = getenv("DM_TEST_GEAR_EQUIP"); file = e && e[0] == '@' ? e + 1 : NULL;
        const char *f = getenv("DM_TEST_GEAR_FAIL");
        if (f) { int st = 0, pt = 0; if (sscanf(f, "f%d:%d", &st, &pt) == 2) { g_fault.flip = st; g_fault.point = pt; }
                 else if (sscanf(f, "c%d:%d", &st, &pt) == 2) { g_fault_change = st; g_fault.point = pt; }
                 else if (sscanf(f, "%d:%d", &st, &pt) == 2) { g_fault.step = st; g_fault.point = pt; }
            LOG("[gear-equip] fault: step %d flip %d point %d", g_fault.step, g_fault.flip, g_fault.point); } }
    if (!t_census && dm_pages_shown() >= 0) t_census = now_ms();
    if (t_census > 1 && now_ms() - t_census > 5000) { t_census = 1; qa_census(b, "run"); }
    if (!file) return;
    if (stage == 0) {
        static int tick; if (++tick % 30) return;
        FILE *fp = fopen(file, "r"); if (!fp) return;
        for (int s = 0; s < DM_GO_SLOTS; s++) src[s] = (dm_ge_loc){ DM_GE_BODY, (int8_t)s, -1, -1 };
        char ln[96]; int n = 0;
        while (fgets(ln, sizeof ln, fp)) {
            char name[16] = "", where[32] = ""; if (sscanf(ln, "%15[a-z]=%31s", name, where) != 2) continue;
            int s = slot_named(name), idx = 0, x = 0, y = 0; if (!s) { LOG("[gear-equip] qa: no slot %s", name); continue; }
            if (!strcmp(where, "-")) src[s] = (dm_ge_loc){ DM_GE_NOWHERE, -1, -1, -1 };
            else if (sscanf(where, "i:%d,%d", &x, &y) == 2) src[s] = (dm_ge_loc){ DM_GE_INV, 0, (int8_t)x, (int8_t)y };
            else if (sscanf(where, "p%d:%d,%d", &idx, &x, &y) == 3 && idx >= 1) src[s] = (dm_ge_loc){ DM_GE_PAGE, (int8_t)(idx - 1), (int8_t)x, (int8_t)y };
            else if (sscanf(where, "s%d:%d,%d", &idx, &x, &y) == 3 && idx >= 1) src[s] = (dm_ge_loc){ DM_GE_TAB, (int8_t)(idx - 1), (int8_t)x, (int8_t)y };
            else { LOG("[gear-equip] qa: bad place %s", where); continue; }
            n++;
        }
        fclose(fp); remove(file);
        LOG("[gear-equip] qa: %d slots asked", n);
        wanted = !dm_gear_tab_wanted(); if (wanted) dm_gear_tab_want(1);
        stage = 1; tries = 0; return;
    }
    if (stage == 1) {
        if (!stash_open(b) || held_of(pl) || dm_pages_shown() < 0 || dm_itemjob_owner()) return;
        static dm_gear_equip_cap cap;
        if (dm_gear_equip_capture(b, pl, src, &cap)) { LOG("[gear-equip] qa: the capture failed"); stage = 3; return; }
        pthread_mutex_lock(&g_qa.mu);
        if (!g_qa.thread) { pthread_t th; if (!pthread_create(&th, NULL, qa_planner, NULL)) { pthread_detach(th); g_qa.thread = 1; } }
        g_qa.cap = cap; g_qa.have = 0; g_qa.want = 1; pthread_cond_signal(&g_qa.cv); pthread_mutex_unlock(&g_qa.mu);
        LOG("[gear-equip] qa: %d items captured (%016llx), planning", cap.w.nitem, (unsigned long long)cap.hash);
        stage = 2; return;
    }
    if (stage == 2) {
        if (pthread_mutex_trylock(&g_qa.mu)) return;
        int have = g_qa.have, rc = g_qa.rc; static dm_gear_equip_cap cap; static dm_go_plan plan; if (have) { cap = g_qa.cap; plan = g_qa.plan; g_qa.have = 0; }
        pthread_mutex_unlock(&g_qa.mu);
        if (!have) return;
        if (rc) { LOG("[gear-equip] qa: no plan (%d)", rc); stage = 3; return; }
        int st = dm_gear_equip_start(b, pl, &cap, &plan);
        LOG("[gear-equip] qa: start -> %d (%d steps, %d changes)", st, plan.nstep, plan.nchange);
        if (st == 1 && ++tries < 3) { stage = 1; return; }
        stage = 3; return;
    }
    if (stage == 3 && !g_job.on) { if (wanted) dm_gear_tab_want(0); wanted = 0; stage = 0; }
}
#endif


void dm_gear_equip_builds(int n) { g_builds = n; }
void dm_gear_equip_fresh(void) { g_nvh = 0; }
int dm_gear_equip_ready(uint8_t *b, uint64_t pl) { return b && pl && !g_job.on && stash_open(b) && !held_of(pl) && dm_pages_shown() >= 0 && !dm_itemjob_owner(); }
int dm_gear_equip_open(uint8_t *b) { return b && stash_open(b); }

void dm_gear_equip_pump(uint8_t *b, uint64_t pl) {
    if (!b) return;
    if (g_job.on) { job_pump(b, pl); return; }
    if (pl) dm_gear_equip_tab_pump(b, pl);
#ifndef DM_PLAYER_BUILD
    if (pl) qa_pump(b, pl);
#endif
}
