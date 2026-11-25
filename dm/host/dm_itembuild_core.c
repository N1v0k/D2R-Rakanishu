
#include "dm_itembuild.h"
#include <string.h>

uint64_t dm_ib_make(const dm_ib_ops *o, const uint8_t *buf, int len, int all, dm_ib_result *r) {
    dm_ib_result z; if (!r) r = &z;
    memset(r, 0, sizeof *r);
    if (!o || !buf || len <= 0) return 0;
    int used = 0, socks = 0;
    uint64_t it = o->build(o->ctx, buf, len, &used, &socks);
    if (!it) return 0;
    if (used <= 0 || used > len) { o->release(o->ctx, it); return 0; }
    r->want = socks > 0 ? socks : 0;
    int off = used;
    for (int s = 0; s < r->want; s++) {
        int cu = 0, cs = 0;
        uint64_t ch = off < len ? o->build(o->ctx, buf + off, len - off, &cu, &cs) : 0;
        if (!ch || cu <= 0 || cu > len - off) {
            if (ch) o->release(o->ctx, ch);
            r->failed += r->want - s;
            break;
        }
        if (s < DM_IB_AT) r->at[s] = off;
        off += cu;
        if (o->attach(o->ctx, it, ch)) { r->socked++; continue; }
        r->failed++;

        if (o->loose(o->ctx, ch)) o->release(o->ctx, ch);
    }
    r->bytes = off;
    if (all && r->failed) { o->release(o->ctx, it); return 0; }
    return it;
}
