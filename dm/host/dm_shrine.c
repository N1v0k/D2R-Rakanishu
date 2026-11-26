



































#include "dm.h"
#include "game_addrs.h"
#include "hle.h"
#include "dm_custom.h"
#include <string.h>

enum {
    A_SOURCE = 0x00, A_TARGET = 0x08, A_DUR = 0x18, A_STATE = 0x24,
    U_STATLIST = 0x88, U_TIMER = 0xd8, U_VER = 0x1bd,
    L_FLAGS = 0x1c, L_STATE = 0x20, L_END = 0x24, L_NEXT = 0x68, L_HEAD0 = 0xd0, L_HEAD1 = 0xd8,
    TABLES = GA_DATA_TABLES, D_CURSE = 0x360,
    FL_CURSE = 0x20, FL_BASE = 0x80000000u,
    RUN_STAT = 96, RUN_PCT = 50, STAMINA_STATE = 136,
    STATE_FIRST = 128, STATE_LAST = 137,
    FN_APPLY = GA_STATE_APPLY_TIMED, FN_UNLINK = GA_STATLIST_UNLINK, FN_FREE = GA_STATLIST_FREE, FN_TIMER = GA_EVENT_TIMER_SET, FN_STAT = GA_STATLIST_SET, FN_STATEBIT = GA_STATE_SET_BIT,
};
enum { MAX_SEEN = 64, MAX_MASK = 16, MAX_SAME = 4, MAX_CUT = 16, MAX_DEPTH = 8 };

static uint8_t *base(void) {
    static uint8_t *b;
    if (!b) {
        extern module_t g_mod[]; extern int g_nmod;
        for (int i = 0; i < g_nmod; i++) if (!strcmp(g_mod[i].name, "main")) b = (uint8_t *)g_mod[i].base;
    }
    return b;
}


uint64_t dm_shrine_apply_body(uint64_t arg, uint64_t entry);
#ifndef DM_SHRINE_GAME_APPLY
#define DM_SHRINE_GAME_APPLY(arg, entry) dm_shrine_apply_body((arg), (entry))
#endif
#ifndef DM_SHRINE_GAME_UNLINK
static void game_unlink(unsigned ver, uint64_t list) { ((void (*)(unsigned, uint64_t))(uintptr_t)(base() + FN_UNLINK))(ver, list); }
#else
#define game_unlink DM_SHRINE_GAME_UNLINK
#endif
#ifndef DM_SHRINE_GAME_FREE
static void game_free(unsigned ver, uint64_t list) { ((void (*)(unsigned, uint64_t))(uintptr_t)(base() + FN_FREE))(ver, list); }
#else
#define game_free DM_SHRINE_GAME_FREE
#endif
#ifndef DM_SHRINE_GAME_TIMER
static void game_timer(uint64_t timer, uint64_t unit, int end) { ((void (*)(uint64_t, uint64_t, unsigned, int, uint64_t, unsigned, unsigned, unsigned))(uintptr_t)(base() + FN_TIMER))(timer, unit, 0xc, end, 0, 0, 0, 0); }
#else
#define game_timer DM_SHRINE_GAME_TIMER
#endif
#ifndef DM_SHRINE_GAME_STAT
static void game_stat(unsigned ver, uint64_t list, int stat, int value) { ((void (*)(unsigned, uint64_t, int, int, int))(uintptr_t)(base() + FN_STAT))(ver, list, stat, value, 0); }
#else
#define game_stat DM_SHRINE_GAME_STAT
#endif
#ifndef DM_SHRINE_GAME_STATEBIT
static void game_statebit(uint64_t unit, int state) { ((void (*)(uint64_t, int, int))(uintptr_t)(base() + FN_STATEBIT))(unit, state, 1); }
#else
#define game_statebit DM_SHRINE_GAME_STATEBIT
#endif

int dm_guest_heap_readable(uint64_t a, size_t n);



static int rd(uint64_t a, void *buf, size_t n) {
    if (!a || !dm_guest_heap_readable(a, n)) return 0;
    memcpy(buf, (const void *)(uintptr_t)a, n);
    return 1;
}
static int rd32(uint64_t a, uint32_t *v) { return rd(a, v, 4); }
static int rd64(uint64_t a, uint64_t *v) { return rd(a, v, 8); }
static int is_shrine_state(int state) { return state >= STATE_FIRST && state <= STATE_LAST; }


static uint64_t data_tables(uint8_t *b, unsigned ver) {
    if (ver - 1 > 3) return 0;
    return *(const uint64_t *)(b + TABLES + 16 * (uint64_t)ver);
}


static int state_is_curse(uint64_t D, int state) {
    uint64_t bm = 0; uint32_t w = 0;
    if (!rd64(D + D_CURSE, &bm) || !rd(bm + 4 * (uint64_t)(state >> 5), &w, 4)) return -1;
    return (int)((w >> (state & 31)) & 1);
}

struct walk {
    uint64_t base;
    int new_state, do_same, do_mask, do_cut_shrine, do_cut_curse;
    uint64_t mask[MAX_MASK]; int n_mask;
    uint64_t same[MAX_SAME]; uint32_t same_end[MAX_SAME]; int n_same;
    uint64_t cut[MAX_CUT]; uint32_t cut_state[MAX_CUT]; int n_cut;
    int over;
    int seen; uint64_t seen_ptr[MAX_SEEN];
};


static int step_node(struct walk *w, uint64_t l) {
    uint32_t flags = 0, state = 0; uint64_t next = 0;
    if (w->seen >= MAX_SEEN) return 0;
    if (!rd32(l + L_FLAGS, &flags) || !rd32(l + L_STATE, &state) || !rd64(l + L_NEXT, &next)) return 0;
    if (is_shrine_state((int)state)) {
        if (w->do_same && (int)state == w->new_state) {
            if (w->n_same >= MAX_SAME) return 0;
            w->same[w->n_same] = l;
            if (!rd(l + L_END, &w->same_end[w->n_same], 4)) return 0;
            w->n_same++;
        } else if (w->do_cut_shrine) {
            if (w->n_cut >= MAX_CUT) return 0;
            w->cut[w->n_cut] = l; w->cut_state[w->n_cut] = state; w->n_cut++;
        } else if (w->do_mask && (flags & FL_CURSE)) {
            if (w->n_mask >= MAX_MASK) return 0;
            w->mask[w->n_mask++] = l;
            *(volatile uint32_t *)(uintptr_t)(l + L_FLAGS) = flags & ~FL_CURSE;
        }
    } else if (w->do_cut_curse && (flags & FL_CURSE) && (int)state != w->new_state) {
        if (w->n_cut >= MAX_CUT) return 0;
        w->cut[w->n_cut] = l; w->cut_state[w->n_cut] = state; w->n_cut++;
    }
    w->seen_ptr[w->seen++] = l;
    (void)next;
    return 1;
}

static void walk_chain_head(struct walk *w, uint64_t head) {
    uint64_t l = head;
    while (l) {
        for (int i = 0; i < w->seen; i++) if (w->seen_ptr[i] == l) return;
        if (!step_node(w, l)) { w->over = 1; return; }
        uint64_t next = 0;
        if (!rd64(l + L_NEXT, &next)) { w->over = 1; return; }
        l = next;
    }
}


static void walk_chains(struct walk *w) {
    uint32_t flags = 0;
    w->seen = 0;
    if (!rd32(w->base + L_FLAGS, &flags) || !(flags & FL_BASE)) return;
    for (int chain = 0; chain < 2 && !w->over; chain++) {
        uint64_t head = 0;
        if (!rd64(w->base + L_HEAD0 + 8 * (uint64_t)chain, &head)) { w->over = 1; return; }
        walk_chain_head(w, head);
    }
}


static int chain_has(uint64_t base, uint64_t list, int state) {
    if (!base) return 0;
    uint32_t bf = 0;
    if (!rd32(base + L_FLAGS, &bf) || !(bf & FL_BASE)) return 0;
    for (int chain = 0; chain < 2; chain++) {
        uint64_t head = 0, seen[MAX_SEEN]; int n = 0;
        if (!rd64(base + L_HEAD0 + 8 * (uint64_t)chain, &head)) continue;
        for (uint64_t l = head; l; ) {
            int again = 0;
            for (int i = 0; i < n && !again; i++) again = seen[i] == l;
            if (again || n >= MAX_SEEN) break;
            seen[n++] = l;
            uint32_t st = 0; uint64_t next = 0;
            if (!rd32(l + L_STATE, &st) || !rd64(l + L_NEXT, &next)) break;
            if (l == list && (state < 0 || (int)st == state)) return 1;
            l = next;
        }
    }
    return 0;
}


static void remove_list(unsigned ver, uint64_t base, uint64_t list, uint32_t state) {
    if (!chain_has(base, list, (int)state)) return;
    uint32_t flags = 0;
    if (!rd32(list + L_FLAGS, &flags)) return;
    game_unlink(ver, list);
    if (!(flags & FL_BASE)) game_free(ver, list);
}

static void restore_masks(struct walk *w, uint64_t base) {
    for (int i = 0; i < w->n_mask; i++) {
        if (!chain_has(base, w->mask[i], -1)) continue;
        uint32_t flags = 0;
        if (rd32(w->mask[i] + L_FLAGS, &flags) && !(flags & FL_CURSE))
            *(volatile uint32_t *)(uintptr_t)(w->mask[i] + L_FLAGS) = flags | FL_CURSE;
    }
}


void svc_shrine_apply(uint64_t *regs) {
    static __thread int t_depth;
    uint8_t *b = base();
    uint64_t arg = regs[0];
    if (!b) return;
    int stack = dm_custom_get("shrine_stacking"), speed = dm_custom_get("shrine_run_speed");
    struct walk w; memset(&w, 0, sizeof w);
    int rewrite = 0, entered = 0;
    uint64_t target = 0, unit_list = 0, timer = 0; unsigned ver = 0; int state = 0;

    if (arg && (stack || speed)) { entered = 1; t_depth++; }
    if (entered && t_depth <= MAX_DEPTH) {
        uint64_t source = 0;
        do {



            if (!arg || (arg & 7)) break;
            source = *(volatile uint64_t *)(uintptr_t)(arg + A_SOURCE);
            target = *(volatile uint64_t *)(uintptr_t)(arg + A_TARGET);
            if (!source || !target || (source & 7) || (target & 7)) break;
            if (!rd(source + U_VER, &ver, 1)) break;
            uint64_t D = data_tables(b, ver);
            if (!D) break;
            state = (int32_t)(int)*(volatile uint32_t *)(uintptr_t)(arg + A_STATE);
            if (state < 0) break;
            int curse = state_is_curse(D, state), shrine = is_shrine_state(state);
            if (curse < 0) break;
            if (shrine ? !stack && !(speed && state == STAMINA_STATE) : !(stack && curse)) break;
            w.base = 0;
            w.do_same = w.do_mask = shrine && stack;
            w.do_cut_shrine = stack && curse && !shrine;
            w.do_cut_curse = stack && (shrine || curse);
            if ((w.do_same || w.do_mask || w.do_cut_shrine || w.do_cut_curse)
                && rd64(target + U_STATLIST, &unit_list) && unit_list) {
                w.base = unit_list; w.new_state = state;
                walk_chains(&w);
                if (w.over) { restore_masks(&w, unit_list); break; }
            }
            rd64(target + U_TIMER, &timer);
            rewrite = 1;
        } while (0);
    }

    uint64_t r = DM_SHRINE_GAME_APPLY(arg, (uint64_t)(uintptr_t)b + FN_APPLY + 8);
    regs[0] = r;
    if (!rewrite) { if (entered) t_depth--; return; }

    uint64_t survivor = 0;
    if (r) {
        uint32_t dur = 0;
        dur = *(volatile uint32_t *)(uintptr_t)(arg + A_DUR);
        int shrine = is_shrine_state(state);
        if (shrine && w.n_same) {
            int keep = -1;
            for (int i = 0; i < w.n_same; i++) if (w.same[i] == r) keep = i;
            if (dur == 0) {


                if (keep < 0) {
                    remove_list(ver, unit_list, r, (uint32_t)state);
                    game_statebit(target, state);
                }
                survivor = keep >= 0 ? r : w.same[0];
                regs[0] = survivor;
            } else {
                int removed = 0;
                for (int i = 0; i < w.n_same; i++)
                    if (i != keep) { remove_list(ver, unit_list, w.same[i], (uint32_t)state); removed = 1; }
                if (removed) game_statebit(target, state);
                survivor = r;
                uint32_t bits = w.same_end[keep >= 0 ? keep : 0];
                float old_end; memcpy(&old_end, &bits, 4);
                if (!(old_end >= 0.0f && old_end < 2147483520.0f)) old_end = 0.0f;
                int32_t end = (int32_t)old_end + (int32_t)dur;
                float ef = (float)end; memcpy(&bits, &ef, 4);
                if (chain_has(unit_list, survivor, state)) {
                    *(volatile uint32_t *)(uintptr_t)(survivor + L_END) = bits;
                    if (timer) game_timer(timer, target, end);
                }
            }
        } else if (shrine) {
            survivor = r;
        } else {
            survivor = r;
        }
        for (int i = 0; i < w.n_cut; i++) remove_list(ver, unit_list, w.cut[i], w.cut_state[i]);
        if (survivor && state == STAMINA_STATE && speed) game_stat(ver, survivor, RUN_STAT, RUN_PCT);
    }
    restore_masks(&w, unit_list);
    if (entered) t_depth--;
}
