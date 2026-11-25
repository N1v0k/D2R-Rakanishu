

#include <string.h>
#include "dm_unsocket.h"

int dm_uns_overflow(int cube_w, int cube_h, int base_w, int base_h, int fillers) {
    int over = fillers - (cube_w * cube_h - base_w * base_h);
    return over > 0 ? over : 0;
}

int dm_uns_decide(const dm_uns_view *v, int *overflow) {
    *overflow = 0;
    if (v->items != 3 || v->bases != 1 || v->ral != 1 || v->scroll != 1 || v->fillers < 1) return DM_UNS_PASS;
    if (v->job_busy) return DM_UNS_REFUSE_BUSY;
    if (v->cube_w < 1 || v->cube_h < 1 || v->cube_w > 16 || v->cube_h > 16 || v->base_w < 1 || v->base_h < 1 || v->base_w > v->cube_w || v->base_h > v->cube_h)
        return DM_UNS_REFUSE_READ;
    if (v->fillers_small != v->fillers) return DM_UNS_REFUSE_SHAPE;
    int over = dm_uns_overflow(v->cube_w, v->cube_h, v->base_w, v->base_h, v->fillers);
    if (over > v->inv_free) { *overflow = over - v->inv_free; return DM_UNS_REFUSE_ROOM; }
    *overflow = over;
    return DM_UNS_APPLY;
}

int dm_uns_penalty(const dm_uns_item *it) {
    if (it->ethereal || it->no_durability || it->max_dur <= 0 || it->indestructible_after) return 0;
    if (it->indestructible_before && !DM_UNS_PENALTY_INSERT_INDESTRUCTIBLE) return 0;
    return 1;
}

int dm_uns_outcome(const dm_uns_after *a) {
    if (a->base_found && a->ral_left && a->scroll_left && a->base_fillers == a->fillers_noted) return DM_UNS_NOT_RUN;
    if (a->base_found && !a->ral_left && !a->scroll_left && a->base_fillers == 0 && a->fillers_home == a->fillers_noted) return DM_UNS_DONE;
    return DM_UNS_ODD;
}

static uint16_t u16(const uint8_t *p) { return (uint16_t)(p[0] | p[1] << 8); }
static int input_is(const uint8_t *in, uint8_t flags, uint16_t what) {
    static const uint8_t zero[4];
    return in[0] == flags && in[1] == 0 && u16(in + 2) == what && !memcmp(in + 4, zero, 4);
}
int dm_uns_row_is_clear(const uint8_t *row, uint16_t hel, uint16_t scroll) {
    static const uint8_t zero[DM_UNS_INPUT];
    const uint8_t *in = row + DM_UNS_ROW_INPUTS;
    if (row[0] != 1 || u16(row + 0xc) != 3) return 0;
    if (!input_is(in, 0x09, 0xffff) || !input_is(in + DM_UNS_INPUT, 0x01, hel) || !input_is(in + 2 * DM_UNS_INPUT, 0x01, scroll)) return 0;
    for (int k = 3; k < DM_UNS_INPUTS; k++) if (memcmp(in + k * DM_UNS_INPUT, zero, DM_UNS_INPUT)) return 0;
    return (u16(row + DM_UNS_ROW_FLAGS) & (DM_UNS_OUT_DESTROY | DM_UNS_OUT_REMOVE)) == DM_UNS_OUT_DESTROY;
}
void dm_uns_row_lend(uint8_t *row, uint16_t ral) {
    uint8_t *rune = row + DM_UNS_ROW_INPUTS + DM_UNS_INPUT + 2;
    rune[0] = (uint8_t)ral; rune[1] = (uint8_t)(ral >> 8);
    uint16_t f = (uint16_t)((u16(row + DM_UNS_ROW_FLAGS) & ~(DM_UNS_OUT_DESTROY | DM_UNS_OUT_REMOVE)) | DM_UNS_OUT_REMOVE);
    row[DM_UNS_ROW_FLAGS] = (uint8_t)f; row[DM_UNS_ROW_FLAGS + 1] = (uint8_t)(f >> 8);
}



uint64_t dm_uns_gate(const dm_uns_ops *ops, void *ctx, int mode) {
    dm_uns_view v; int over = 0, d;
    memset(&v, 0, sizeof v);
    if (!ops->view(ctx, &v)) { ops->note(ctx, DM_UNS_N_UNREAD, mode); d = DM_UNS_PASS; }
    else d = dm_uns_decide(&v, &over);
    int lent = 0;
    if (d == DM_UNS_APPLY) { lent = ops->lend(ctx, over); if (!lent) { ops->note(ctx, DM_UNS_N_ROW, mode); d = DM_UNS_REFUSE_READ; } }
    uint64_t r = ops->body(ctx);
    if (lent) ops->give_back(ctx);
    if (d >= DM_UNS_REFUSE_ROOM) { ops->note(ctx, DM_UNS_N_REFUSED, mode); ops->notice(ctx, d, over); return r; }
    if (d != DM_UNS_APPLY) return r;
    if (mode == 0) { ops->note(ctx, r == DM_UNS_QUEUED ? DM_UNS_N_QUEUED : DM_UNS_N_NOT_QUEUED, mode); return r; }
    dm_uns_after a; dm_uns_item it;
    memset(&a, 0, sizeof a); memset(&it, 0, sizeof it);
    ops->after(ctx, &a, &it);
    switch (dm_uns_outcome(&a)) {
    case DM_UNS_NOT_RUN: ops->note(ctx, DM_UNS_N_NOT_RUN, mode); break;
    case DM_UNS_ODD: ops->note(ctx, DM_UNS_N_ODD, mode); break;
    default:
        ops->note(ctx, DM_UNS_N_DONE, mode);
        if (dm_uns_penalty(&it)) { ops->penalty(ctx); ops->note(ctx, DM_UNS_N_PENALISED, mode); }
        else ops->note(ctx, DM_UNS_N_SPARED, mode);
    }
    return r;
}
