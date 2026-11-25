







#include "dm_streak.h"
#include <stdio.h>
#include <string.h>

static const struct { uint32_t kills; int pct; } k_steps[DM_SK_TIERS] = { { DM_SK_START, 10 }, { 12, 15 }, { 20, 20 }, { 30, 25 }, { 45, 30 }, { 65, 35 }, { 100, 40 }, { 150, 50 }, { 250, 60 } };
int dm_sk_tier(uint32_t kills) {
    int t = 0;
    while (t < DM_SK_TIERS && kills >= k_steps[t].kills) t++;
    return t;
}
int dm_sk_pct(uint32_t kills) { int t = dm_sk_tier(kills); return t ? k_steps[t - 1].pct : 0; }
uint64_t dm_sk_bonus(uint64_t base, int pct) { return pct <= 0 ? 0 : base / 100 * (uint64_t)pct + base % 100 * (uint64_t)pct / 100; }

static void clear_streak(dm_sk_state *s) {
    if (s->nids) memset(s->ids, 0, sizeof s->ids);
    s->nids = s->ids_full = 0; s->kills = s->first = s->last = 0; s->no_bonus = 0; s->base = 0; s->due = 0; s->nwait = 0;
}
void dm_sk_reset(dm_sk_state *s) { clear_streak(s); s->ncred = 0; s->nended = 0; s->dropped = 0; }

static void finish(dm_sk_state *s, int kind) {
    if (s->kills >= DM_SK_START && s->nended < 2)
        s->ended[s->nended++] = (dm_sk_end){ kind, dm_sk_pct(s->kills), s->no_bonus, s->kills, s->first, s->last, s->base };
    clear_streak(s);
}
static int expired(const dm_sk_state *s, uint32_t tick) { return s->kills && tick - s->last > s->due; }


static int remember(dm_sk_state *s, uint32_t id) {
    if (!id) return 1;
    for (unsigned h = id * 2654435761u % (2 * DM_SK_IDS), k = 0; k < 2 * DM_SK_IDS; k++, h = (h + 1) % (2 * DM_SK_IDS)) {
        if (s->ids[h] == id) return 0;
        if (!s->ids[h]) {
            if (s->nids >= DM_SK_IDS) { s->ids_full = 1; return 1; }
            s->ids[h] = id; s->nids++; return 1;
        }
    }
    return 1;
}

static uint32_t take_credit(dm_sk_state *s, uint32_t id) {
    int k = -1;
    for (int i = 0; i < s->ncred; i++) if (s->cred[i].id == id && (k < 0 || s->cred[i].t < s->cred[k].t)) k = i;
    if (k < 0) return 0;
    uint32_t a = s->cred[k].amount; s->cred[k] = s->cred[--s->ncred];
    return a;
}
int dm_sk_kill(dm_sk_state *s, uint32_t tick, uint32_t id, int no_bonus) {
    if (expired(s, tick)) finish(s, DM_SK_END_TIME);
    if (!remember(s, id)) { take_credit(s, id); return 0; }
    if (!s->kills) s->first = tick;
    s->kills++; s->last = tick; s->due = DM_SK_WINDOW;
    if (no_bonus) s->no_bonus = 1;
    uint32_t a = take_credit(s, id);
    if (a) s->base += a;
    else if (id && s->nwait < DM_SK_CREDITS) s->wait[s->nwait++] = (struct dm_sk_seen){ id, tick, 0 };
    return 1;
}
void dm_sk_hits(dm_sk_state *s, uint32_t tick, int n) {
    if (n <= 0 || !s->kills || expired(s, tick)) return;
    s->due = tick - s->last + DM_SK_WINDOW;
}
void dm_sk_credit(dm_sk_state *s, uint32_t tick, uint32_t id, uint32_t amount) {
    if (!amount || !id) return;
    for (int i = 0; i < s->nwait; i++)
        if (s->wait[i].id == id) { s->base += amount; s->wait[i] = s->wait[--s->nwait]; return; }
    if (s->ncred < DM_SK_CREDITS) s->cred[s->ncred++] = (struct dm_sk_seen){ id, tick, amount };
    else s->dropped++;
}
int dm_sk_tick(dm_sk_state *s, uint32_t tick, int dead, dm_sk_end out[2]) {
    for (int i = 0; i < s->ncred; ) if (tick - s->cred[i].t >= DM_SK_SEEN) s->cred[i] = s->cred[--s->ncred]; else i++;
    for (int i = 0; i < s->nwait; ) if (tick - s->wait[i].t >= DM_SK_LATE) s->wait[i] = s->wait[--s->nwait]; else i++;
    if (dead && s->kills) finish(s, DM_SK_END_DEATH);
    else if (expired(s, tick)) finish(s, DM_SK_END_TIME);
    int n = s->nended;
    for (int i = 0; i < n; i++) out[i] = s->ended[i];
    s->nended = 0;
    return n;
}
int dm_sk_shown(const dm_sk_state *s, uint32_t tick, uint32_t *kills, int *left) {
    if (s->kills < DM_SK_START || expired(s, tick)) return 0;
    *kills = s->kills; *left = (int)(s->due - (tick - s->last));
    return 1;
}


int dm_sk_tiers_text(char *out, size_t cap) {
    size_t n = 0;
    if (cap) out[0] = 0;
    for (int t = 0; t < DM_SK_TIERS; t++) {
        int w = snprintf(out + n, cap - n, "tier\t%d\t%u\t%d\n", t + 1, k_steps[t].kills, k_steps[t].pct);
        if (w < 0 || (size_t)w >= cap - n) { if (cap) out[0] = 0; return -2; }
        n += (size_t)w;
    }
    return (int)n;
}
