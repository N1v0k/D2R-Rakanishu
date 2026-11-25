
#include <stdatomic.h>
#include "dm_itemjob.h"

static _Atomic int g_owner;
static atomic_uint g_ends;

int dm_itemjob_acquire(int kind) {
    int none = DM_ITEMJOB_NONE;
    if (kind <= DM_ITEMJOB_NONE || kind >= DM_ITEMJOB_N) return 0;
    return atomic_compare_exchange_strong(&g_owner, &none, kind);
}
int dm_itemjob_release(int kind) {
    int mine = kind;
    if (kind <= DM_ITEMJOB_NONE || kind >= DM_ITEMJOB_N) return 0;
    if (!atomic_compare_exchange_strong(&g_owner, &mine, DM_ITEMJOB_NONE)) return 0;
    atomic_fetch_add(&g_ends, 1);
    return 1;
}
int dm_itemjob_owner(void) { return atomic_load(&g_owner); }
unsigned dm_itemjob_ends(void) { return atomic_load(&g_ends); }

void dm_itemjob_settle_begin(dm_itemjob_settle *s, int limit, int retry) { s->left = limit; s->retry = retry; s->wait = 0; s->backs = 0; }
int dm_itemjob_settle_step(dm_itemjob_settle *s, const dm_itemjob_seen *v) {
    int over = --s->left <= 0;
    if (v->hand == DM_ITEMJOB_HAND_OTHER) return DM_ITEMJOB_OTHER;
    if (v->hand == DM_ITEMJOB_HAND_OWN) {
        if (over) return DM_ITEMJOB_IN_HAND;
        if (s->backs == 0 || (--s->wait <= 0 && s->backs < 2)) { s->backs++; s->wait = s->retry; return DM_ITEMJOB_PUT_BACK; }
        return DM_ITEMJOB_WAIT;
    }
    if (v->tracked ? v->placed : !v->pending) return DM_ITEMJOB_SETTLED;
    if (!over) return DM_ITEMJOB_WAIT;
    return !v->tracked && v->at_source ? DM_ITEMJOB_SETTLED : DM_ITEMJOB_UNPLACED;
}
