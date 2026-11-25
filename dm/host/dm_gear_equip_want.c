
#include "dm_gear_equip_tab.h"
#include <string.h>

int dm_gew_slot_of(int place) {
    static const int8_t slot[DM_GEAR_P_N] = { DM_GO_RIGHT, DM_GO_LEFT, DM_GO_TORSO, DM_GO_NECK, DM_GO_RRING, DM_GO_LRING, DM_GO_HEAD, DM_GO_BELT,
                                              DM_GO_FEET, DM_GO_GLOVES };
    return place >= 0 && place < DM_GEAR_P_N ? slot[place] : 0;
}

static int slot_of_row(const dm_gear_snap *s, const dm_gtab_row *r, dm_gew_slot *out) {
    memset(out, 0, sizeof *out);
    if (r->cand < 0) { out->kind = DM_GEW_EMPTY; return 0; }
    if (r->cand >= s->p.ncand) return -1;
    const dm_gtab_show *w = &s->show[r->cand];
    switch (w->where) {
    case DM_GTAB_INV:
        if (!w->id || (w->id & DM_GTAB_STOCK_ID)) return -1;
        out->kind = DM_GEW_UNIT; out->id = w->id; return 0;
    case DM_GTAB_STASH: case DM_GTAB_SHARED:
        if (!(w->id & DM_GTAB_STOCK_ID) || !w->vhash) return -1;
        out->kind = DM_GEW_STOCK; out->id = w->id; out->vh = w->vhash; return 0;
    case DM_GTAB_WORN:


        if (r->place >= 0 && r->place < DM_GEAR_P_N && s->now.at[r->place] == r->cand) { out->kind = DM_GEW_KEEP; return 0; }
        if ((r->place == DM_GEAR_P_MAIN || r->place == DM_GEAR_P_OFF) && s->now.at[DM_GEAR_P_MAIN + DM_GEAR_P_OFF - r->place] == r->cand && w->id &&
            !(w->id & DM_GTAB_STOCK_ID)) { out->kind = DM_GEW_UNIT; out->id = w->id; return 0; }
        return -1;
    }
    return -1;
}
int dm_gew_target_of(const dm_gear_snap *s, const dm_gtab_row *rows, int n, int row, dm_gew_target *t) {
    if (!s || !rows || !t || n < 0 || n > DM_GTAB_ROWS || row < -1 || row >= n) return -1;
    memset(t, 0, sizeof *t);
    dm_gtab_units want; memset(&want, 0, sizeof want);
    if (row >= 0) {
        if (rows[row].charm || rows[row].place < 0) return -1;
        int u = rows[row].unit; if (u < 0 || u >= DM_GTAB_ROWS) return -1;
        want.w[u >> 6] |= 1ull << (u & 63);

        for (int grew = 1; grew;) {
            grew = 0;
            for (int i = 0; i < n; i++) {
                int v = rows[i].unit; if (v < 0 || v >= DM_GTAB_ROWS || !dm_gtab_has(&want, v)) continue;
                for (size_t k = 0; k < sizeof want.w / sizeof *want.w; k++) { uint64_t add = rows[i].needs.w[k] & ~want.w[k]; if (add) { want.w[k] |= add; grew = 1; } }
            }
        }
    } else {
        for (int i = 0; i < n; i++) if (!rows[i].charm && rows[i].place >= 0 && rows[i].unit >= 0 && rows[i].unit < DM_GTAB_ROWS) want.w[rows[i].unit >> 6] |= 1ull << (rows[i].unit & 63);
    }
    int units[DM_GO_MAXCHANGE];
    for (int i = 0; i < n; i++) {
        const dm_gtab_row *r = &rows[i];
        if (r->unit < 0 || r->unit >= DM_GTAB_ROWS || !dm_gtab_has(&want, r->unit)) continue;
        if (r->charm || r->place < 0) return -1;
        int slot = dm_gew_slot_of(r->place); if (!slot) return -1;
        int k = 0; while (k < t->nunit && units[k] != r->unit) k++;
        if (k == t->nunit) {
            if (t->nunit >= DM_GO_MAXCHANGE) return -1;
            units[t->nunit] = r->unit; t->unit_slots[t->nunit] = 0; t->nunit++;
            t->seq[t->nseq++] = (int8_t)slot;
        }
        if (t->slot[slot].kind != DM_GEW_KEEP || slot_of_row(s, r, &t->slot[slot])) return -1;
        t->unit_slots[k] |= (uint16_t)(1u << slot);
    }
    for (int k = 0; k < 2; k++) { int c = s->now.at[k ? DM_GEAR_P_RING2 : DM_GEAR_P_RING1]; t->ring_was[k] = c >= 0 && c < s->p.ncand ? s->show[c].id : 0; }
    return t->nunit ? 0 : -1;
}
int dm_gew_ring_swap(const dm_gew_target *t, uint32_t right, uint32_t left) {
    if (t->ring_was[0] == right && t->ring_was[1] == left) return 0;
    if (t->ring_was[0] == left && t->ring_was[1] == right) return 1;
    return -1;
}
int dm_gew_units_done(const dm_gew_target *t, const dm_go_plan *p, int changes) {
    if (!t || !p) return 0;
    const uint16_t ring = 1u << DM_GO_RRING | 1u << DM_GO_LRING;
    uint16_t in = 0;
    for (int c = 0; c < changes && c < p->nchange; c++) in |= p->change[c].slots;
    int rings = !!(in & 1u << DM_GO_RRING) + !!(in & 1u << DM_GO_LRING), done = 0;
    for (int u = 0; u < t->nunit; u++) {
        uint16_t m = t->unit_slots[u]; int need = !!(m & 1u << DM_GO_RRING) + !!(m & 1u << DM_GO_LRING);
        if (!m || (m & ~ring & ~in) || need > rings) continue;
        rings -= need; done++;
    }
    return done;
}
