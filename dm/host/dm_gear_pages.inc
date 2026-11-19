


#include "dm_gear_pages.h"

int dm_gear_pages_profile(uint8_t *b, uint64_t pl, uint64_t it, int x, int y, int nodes, dm_gpg_prof *out) {
    memset(out, 0, sizeof *out); out->kind = DM_GEAR_K_NONE;
    const dm_gear_tables *t = gear_tables();
    if (!b || !pl || !it || !*(uint64_t *)(it + 0x10) || !t) return -1;
    if (nodes) gt_nodes(pl);
    static gt_prof g;
    gt_item e; memset(&e, 0, sizeof e);
    e.it = it; e.id = *(uint32_t *)(it + 8); e.x = x; e.y = y; e.where = DM_GTAB_STASH;
    if (gt_profile(b, pl, &e, t, (int)*(uint32_t *)(pl + 4), gstat(b, pl, 12, 0), &g)) return 0;
    out->kind = g.kind; out->hasw = g.hasw; out->c = g.c; out->w = g.w; out->show = g.show;
    return 0;
}


uint64_t dm_gear_pages_psig(uint8_t *b, uint64_t pl) { return gt_psig(b, pl); }
uint32_t dm_gear_pages_sig(uint64_t it) { uint64_t d = *(uint64_t *)(it + 0x10); return d ? gt_sig(it, d) : 0; }


int dm_gear_pages_tables(void) { return g_gt_state != 0; }
