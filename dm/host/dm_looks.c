















#include "dm.h"
#include "game_addrs.h"
#include "hle.h"
#include "dm_looks.h"
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

enum {
    TABLES = GA_DATA_TABLES, RESMGR = GA_RESOURCE_MGR, CL_PLAYER_ID = GA_LOCAL_PLAYER_SLOT, NPC_QUEST = GA_CLIENT_QUEST_FLAGS,
    FN_MODEL = GA_UNIT_MODEL_PATH, FN_ICON = GA_ITEM_ICON, FN_OWN = GA_ITEM_OWN_LOOK, FN_PAY = GA_GOLD_PAY, FN_STAT = GA_STAT_GET, FN_SEND = GA_ITEM_NOTIFY_CLIENT, FN_CL_UNIT = GA_CLIENT_PLAYER_GET,
    FN_COLOUR = GA_ITEM_COLOUR, FN_UNLINK = GA_INV_UNLINK_ITEM, FN_FREE = GA_ITEM_FREE, COLOUR_TAB = GA_ITEM_COLOUR_TABLE,
    B_GROUP_FIG = 0x151, B_GROUP_INV = 0x152, B_GEM_ROW = 0xf8, T_GEMS = 0x1678, T_NGEMS = 0x1680, GEM_STRIDE = 0xc0, GEM_COLOUR = 0x2f,
    U_CLASS = DM_LOOK_U_CLASS, U_ID = 8, U_MODE = 0xc, U_DATA = DM_LOOK_U_DATA, U_IDENT = 0x28, U_PATH = 0x38, U_INV = 0x90, U_VER = 0x1bd,
    D_QUALITY = DM_LOOK_D_QUALITY, D_FLAGS = DM_LOOK_D_FLAGS, D_ROW = DM_LOOK_D_ROW, D_LEVEL = 0x38, D_BODY = 0x54, D_PAGE = 0x55, D_NEXT = 0xb0,
    T_ITEMS = 0x15c0, T_NITEMS = 0x15c8, ITEM_STRIDE = 0x1c0, ITEM_CODE = 0x80,
    T_SETS = 0x13c8, T_NSETS = 0x13d0, SET_STRIDE = 0x1cc, T_UNIQ = 0x13e8, T_NUNIQ = 0x13f0, UNIQ_STRIDE = 0x15c,
    FL_IDENT = DM_LOOK_FL_IDENT, MODE_WORN = 1, Q_SET = 5, Q_UNIQUE = 7,
    TOOLS_WORD = 3,
};
enum { SNAP_MAX = 256, REQ_MAX = 8 };

void dm_look_model_body(void);
void dm_look_icon_body(void);
void dm_look_colour_body(void);
int dm_guest_heap_readable(uint64_t a, size_t n);
uint32_t dm_cur_tick(void);
int dm_request_save(void);
int dm_pouch_holds(uint32_t unit_id);
int dm_looks_pay_proof(uint64_t G, uint64_t *seq); void dm_looks_settle_committed(void);

static uint8_t *base(void) {
    static uint8_t *b;
    if (!b) for (int i = 0; i < g_nmod; i++) if (!strcmp(g_mod[i].name, "main")) b = (uint8_t *)g_mod[i].base;
    return b;
}
static uint64_t tables(unsigned ver) { return ver - 1 > 3 ? 0 : *(const uint64_t *)(base() + TABLES + 16 * (uint64_t)ver); }


static uint32_t item_code(uint64_t T, uint32_t cls) {
    if (!T || cls >= *(const uint32_t *)(T + T_NITEMS)) return 0;
    return *(const uint32_t *)(*(const uint64_t *)(T + T_ITEMS) + cls * (uint64_t)ITEM_STRIDE + ITEM_CODE);
}
static int item_class(uint64_t T, uint32_t code) {
    if (!T) return -1;
    uint32_t n = *(const uint32_t *)(T + T_NITEMS); uint64_t rows = *(const uint64_t *)(T + T_ITEMS);
    for (uint32_t i = 0; i < n; i++) if (*(const uint32_t *)(rows + i * (uint64_t)ITEM_STRIDE + ITEM_CODE) == code) return (int)i;
    return -1;
}

static int g_uniq_code_at = -1, g_set_code_at = -1;
static int code_offset(uint64_t T, uint64_t rows_at, uint64_t n_at, int stride) {
    uint64_t rows = *(const uint64_t *)(T + rows_at), n = *(const uint64_t *)(T + n_at);
    if (!rows || n < 8) return -1;
    int best = -1; uint64_t bestn = 0;
    for (int off = 0; off + 4 <= stride; off += 4) {
        uint64_t hit = 0;
        for (uint64_t r = 0; r < n; r += 4) if (item_class(T, *(const uint32_t *)(rows + r * stride + off)) >= 0) hit++;
        if (hit > bestn) { bestn = hit; best = off; }
    }
    return bestn * 10 >= ((n + 3) / 4) * 9 ? best : -1;
}
static uint32_t row_code(uint64_t T, int unique, int row) {
    if (!T || row < 0) return 0;
    if (unique && g_uniq_code_at < 0) g_uniq_code_at = code_offset(T, T_UNIQ, T_NUNIQ, UNIQ_STRIDE);
    if (!unique && g_set_code_at < 0) g_set_code_at = code_offset(T, T_SETS, T_NSETS, SET_STRIDE);
    int at = unique ? g_uniq_code_at : g_set_code_at; uint64_t n = *(const uint64_t *)(T + (unique ? T_NUNIQ : T_NSETS));
    if (at < 0 || (uint64_t)row >= n) return 0;
    return *(const uint32_t *)(*(const uint64_t *)(T + (unique ? T_UNIQ : T_SETS)) + (uint64_t)row * (unique ? UNIQ_STRIDE : SET_STRIDE) + at);
}
static unsigned g_ver = 1;

static uint32_t base_of_ver(dm_look l, unsigned ver) {
    if (l.kind == DM_LOOK_BASE) return l.id;
    if (l.kind == DM_LOOK_UNIQUE || l.kind == DM_LOOK_SET) return row_code(tables(ver), l.kind == DM_LOOK_UNIQUE, (int)l.id);
    return 0;
}
static uint32_t base_of(dm_look l) { return base_of_ver(l, g_ver); }

static int own_look(uint32_t code, int quality, int row) {
    return ((uint64_t (*)(uint64_t, uint32_t, int, int, int))(uintptr_t)(base() + FN_OWN))((uint64_t)(uintptr_t)(base() + RESMGR), code, quality, row, 1) != 0;
}


static dm_look_table g_tab; static int g_tab_state;
static int table_ready(void) {
    if (g_tab_state) return g_tab_state > 0;
    char p[1100]; g_tab_state = -1;
    if (!dm_overlay_file("looks.tsv", p, sizeof p)) { LOG("ASSUME looks.table: the pack has no looks.tsv, looks are off"); return 0; }
    FILE *f = fopen(p, "rb"); if (!f) return 0;
    static char buf[1 << 16]; size_t n = fread(buf, 1, sizeof buf - 1, f); fclose(f); buf[n] = 0;
    if (n == sizeof buf - 1 || dm_looks_table_parse(buf, &g_tab)) { LOG("ASSUME looks.table: looks.tsv is malformed, looks are off"); return 0; }
    g_tab_state = 1;
    return 1;
}


static dm_dye_table g_dyes; static _Atomic int g_dyes_state;
static void dyes_load(uint64_t T) {
    if (g_dyes_state || !T) return;
    char p[1100]; static char buf[1 << 14]; size_t n = 0; FILE *f;
    if (!dm_overlay_file("dyes.tsv", p, sizeof p) || !(f = fopen(p, "rb"))) { LOG("ASSUME looks.dye_table: the pack has no dyes.tsv, colours are off"); g_dyes_state = -1; return; }
    n = fread(buf, 1, sizeof buf - 1, f); fclose(f); buf[n] = 0;
    if (n == sizeof buf - 1 || dm_dyes_parse(buf, &g_dyes)) { LOG("ASSUME looks.dye_table: dyes.tsv is malformed, colours are off"); g_dyes_state = -1; return; }
    uint64_t gems = *(const uint64_t *)(T + T_GEMS), ngems = *(const uint64_t *)(T + T_NGEMS);
    for (int i = 0; i < g_dyes.n; i++) {
        int cls = item_class(T, g_dyes.g[i].code); int32_t row = cls < 0 ? -1 : *(const int32_t *)(*(const uint64_t *)(T + T_ITEMS) + cls * (uint64_t)ITEM_STRIDE + B_GEM_ROW);
        if (row < 0 || (uint64_t)row >= ngems || !gems || *(const uint8_t *)(gems + row * (uint64_t)GEM_STRIDE + GEM_COLOUR) != g_dyes.g[i].colour) {
            LOG("ASSUME looks.dye_table: the pack and the game name different colours for %.4s, colours are off", (const char *)&g_dyes.g[i].code);
            dm_dyes_free(&g_dyes); g_dyes_state = -1; return;
        }
    }
    g_dyes_state = 1;
}

static int dye_where(uint64_t T, uint32_t code) {
    int cls = item_class(T, code);
    if (cls < 0) return 0;
    const uint8_t *row = (const uint8_t *)(uintptr_t)(*(const uint64_t *)(T + T_ITEMS) + cls * (uint64_t)ITEM_STRIDE);
    return dm_dye_where(row[B_GROUP_FIG], row[B_GROUP_INV]);
}



typedef struct { uint32_t unit_id, code; uint8_t quality, invisible; int32_t row; uint32_t dclass; uint8_t dquality; int32_t drow; dm_look look, cur; uint8_t busy, dye, cur_dye; } snap_ent;
typedef struct { _Atomic uint32_t seq; int n; snap_ent e[SNAP_MAX]; } snap;
static snap g_snap[2]; static _Atomic int g_snap_cur;
static int snap_find(const snap *s, uint32_t id) {
    int lo = 0, hi = s->n;
    while (lo < hi) { int m = (lo + hi) / 2; if (s->e[m].unit_id == id) return m; if (s->e[m].unit_id < id) lo = m + 1; else hi = m; }
    return -1;
}
static int snap_get(uint32_t id, snap_ent *out) {
    const snap *s = &g_snap[atomic_load_explicit(&g_snap_cur, memory_order_acquire)];
    uint32_t a = atomic_load_explicit(&s->seq, memory_order_acquire);
    if (a & 1) return 0;
    int i = snap_find(s, id); if (i < 0) return 0;
    *out = s->e[i];
    atomic_thread_fence(memory_order_acquire);
    return atomic_load_explicit(&s->seq, memory_order_relaxed) == a;
}
static int ent_cmp(const void *a, const void *b) { uint32_t x = ((const snap_ent *)a)->unit_id, y = ((const snap_ent *)b)->unit_id; return x < y ? -1 : x > y; }
static void snap_publish(snap_ent *e, int n) {
    int nxt = 1 - atomic_load(&g_snap_cur); snap *s = &g_snap[nxt];
    if (n) qsort(e, (size_t)n, sizeof *e, ent_cmp);
    atomic_fetch_add_explicit(&s->seq, 1, memory_order_acq_rel);
    s->n = n; if (n) memcpy(s->e, e, sizeof *e * (size_t)n);
    atomic_fetch_add_explicit(&s->seq, 1, memory_order_release);
    atomic_store_explicit(&g_snap_cur, nxt, memory_order_release);
}


static void call_body(void (*tramp)(void), uint64_t entry, uint64_t a0, uint64_t a1, uint64_t a8) {
    register uint64_t x0 __asm__("x0") = a0, x1 __asm__("x1") = a1, x8 __asm__("x8") = a8, x16 __asm__("x16") = entry,
        x17 __asm__("x17") = (uint64_t)(uintptr_t)tramp;
    __asm__ volatile("blr x17" : "+r"(x0), "+r"(x1), "+r"(x8), "+r"(x16), "+r"(x17) : : "x2", "x3", "x4", "x5", "x6", "x7", "x9", "x10", "x11",
        "x12", "x13", "x14", "x15", "x30", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23",
        "v24", "v25", "v26", "v27", "v28", "v29", "v30", "v31", "cc", "memory");
}
static _Atomic uint64_t g_draws;

static int snap_of(uint64_t u, snap_ent *e, size_t nu, size_t nd) {
    if (!u || (u & 7) || !dm_guest_heap_readable(u, nu) || *(const uint32_t *)u != 4) return 0;
    if (!snap_get(*(const uint32_t *)(u + U_ID), e)) return 0;
    uint64_t d = *(const uint64_t *)(u + U_DATA);
    if (!d || !dm_guest_heap_readable(d, nd)) return 0;
    int q = (int)*(const uint32_t *)(d + D_QUALITY), row = q == Q_SET || q == Q_UNIQUE ? *(const int32_t *)(d + D_ROW) : -1;
    return item_code(tables(*(const uint8_t *)(u + U_VER)), *(const uint32_t *)(u + U_CLASS)) == e->code && q == e->quality && row == e->row;
}

static void dress(uint64_t u, const snap_ent *e, uint8_t *fu, size_t nu, uint8_t *fd, size_t nd) {
    dm_looks_dress((const uint8_t *)(uintptr_t)u, (const uint8_t *)(uintptr_t)*(const uint64_t *)(u + U_DATA), e->dclass, e->dquality, e->drow, fu, nu, fd, nd);
}

static int dressed(uint64_t u, uint8_t *fu, size_t nu, uint8_t *fd, size_t nd, int *invisible) {
    snap_ent e;
    if (!snap_of(u, &e, nu, nd) || e.look.kind == DM_LOOK_ORIGINAL) return 0;
    *invisible = e.invisible;
    if (!e.invisible) dress(u, &e, fu, nu, fd, nd);
    return 1;
}








enum { SEL_MODEL_RET = GA_AT_SELECT_MODEL_RET, SEL_COLOUR_RET = GA_AT_SELECT_COLOUR_RET, SEL_UNIT = 0x3c8, SEL_DATA = 0x2f0, SEL_SAVED_X19 = 0x5f8, SEL_NU = 0x1c0, SEL_ND = 0xd8,
       SEL_HDR = 0x30, GATE_SP = 816, GATE_LR = 101 };
static uint32_t select_base(dm_look l, void *ver) { return base_of_ver(l, *(const unsigned *)ver); }

static int select_ent(const uint64_t *regs, uint64_t ret, snap_ent *e) {
    uint8_t *b = base(); uint64_t u = regs[1], sp = (uint64_t)(uintptr_t)regs + GATE_SP;
    if (!b || regs[GATE_LR] != (uint64_t)(uintptr_t)b + ret) return 0;
    static int warned;
    if (u != sp + SEL_UNIT || *(const uint32_t *)u != 4 || *(const uint64_t *)(u + U_DATA) != sp + SEL_DATA) {
        if (!warned) { warned = 1; LOG("ASSUME looks.select_frame: the character select asks with another item than its stack item, it shows the originals"); }
        return 0;
    }
    memset(e, 0, sizeof *e);
    uint64_t d = sp + SEL_DATA; unsigned ver = *(const uint8_t *)(u + U_VER); uint64_t T = tables(ver);
    dm_look_item k = { *(const uint32_t *)(u + U_IDENT), item_code(T, *(const uint32_t *)(u + U_CLASS)), (uint8_t)*(const uint32_t *)(d + D_QUALITY), -1 };
    if (k.quality == Q_SET || k.quality == Q_UNIQUE) k.row = *(const int32_t *)(d + D_ROW);
    if (!k.fp || !k.code || !table_ready()) return 1;
    uint64_t h = *(const uint64_t *)(sp + SEL_SAVED_X19);
    int cls = h && dm_guest_heap_readable(h, SEL_HDR) ? dm_looks_header_class((const uint8_t *)(uintptr_t)h, SEL_HDR) : -1;
    if (cls < 0 && !warned) { warned = 1; LOG("ASSUME looks.select_frame: no character file header where the character select keeps it, looks of one class stay original"); }
    dm_look l = { DM_LOOK_ORIGINAL, 0 }; int dye = 0, on;
    pthread_mutex_lock(&g_looks_mu);
    dm_looks_ensure_loaded();
    dm_looks_settle_committed();
    dyes_load(T);
    on = g_looks_state == DM_LOOKS_STATE_OK && dm_looks_select(&g_looks, &g_tab, &k, cls, g_dyes_state == 1, select_base, &ver, &l, &dye);
    pthread_mutex_unlock(&g_looks_mu);
    if (on) *e = (snap_ent){ 0, k.code, k.quality, l.kind == DM_LOOK_INVISIBLE, k.row, 0, 2, -1, l, l, 0, (uint8_t)dye, (uint8_t)dye };
    if (on && !e->invisible && l.kind != DM_LOOK_ORIGINAL) {
        int c = item_class(T, base_of_ver(l, ver));
        if (c < 0) e->look = e->cur = (dm_look){ DM_LOOK_ORIGINAL, 0 };
        else {
            e->dclass = (uint32_t)c; e->dquality = l.kind == DM_LOOK_UNIQUE ? Q_UNIQUE : l.kind == DM_LOOK_SET ? Q_SET : 2;
            e->drow = l.kind == DM_LOOK_UNIQUE || l.kind == DM_LOOK_SET ? (int32_t)l.id : -1;
        }
    }
#ifndef DM_PLAYER_BUILD
    LOG("[looks] select %s: class %d code %.4s look kind %d id %u colour %d", ret == SEL_MODEL_RET ? "model" : "colour", cls, (const char *)&k.code,
        e->look.kind, e->look.id, e->dye - 1);
#endif
    return 1;
}

static const char INVISIBLE_HELM[] = "data/hd/items/armor/zinv/helmet.json", INVISIBLE_SHIELD[] = "data/hd/items/armor/zinv/shield.json";
void svc_look_model(uint64_t *regs) {
    uint64_t entry = (uint64_t)(uintptr_t)base() + FN_MODEL + 8;
    uint8_t fu[0x200], fd[0x100]; int inv = 0; snap_ent se;
    if (select_ent(regs, SEL_MODEL_RET, &se)) {
        if (se.look.kind == DM_LOOK_ORIGINAL) { call_body(dm_look_model_body, entry, regs[0], regs[1], regs[8]); return; }
        inv = se.invisible;
        if (!inv) dress(regs[1], &se, fu, SEL_NU, fd, SEL_ND);
    } else if (!dressed(regs[1], fu, sizeof fu, fd, sizeof fd, &inv)) { call_body(dm_look_model_body, entry, regs[0], regs[1], regs[8]); return; }
    atomic_fetch_add(&g_draws, 1);
    if (!inv) { call_body(dm_look_model_body, entry, regs[0], (uint64_t)(uintptr_t)fu, regs[8]); return; }
    call_body(dm_look_model_body, entry, regs[0], regs[1], regs[8]);

    uint8_t *out = (uint8_t *)(uintptr_t)regs[8]; char *s = *(char **)out; uint64_t cap = *(uint64_t *)(out + 0x10) & 0x7FFFFFFFFFFFFFFFull;
    const char *p = dm_looks_group(&g_tab, item_code(tables(*(const uint8_t *)(regs[1] + U_VER)), *(const uint32_t *)(regs[1] + U_CLASS))) == DM_LG_HELM ? INVISIBLE_HELM : INVISIBLE_SHIELD;
    size_t n = strlen(p);
    if (s && n < cap) { memcpy(s, p, n + 1); *(uint64_t *)(out + 8) = n; }
}
void svc_look_icon(uint64_t *regs) {
    uint64_t entry = (uint64_t)(uintptr_t)base() + FN_ICON + 8;
    uint8_t fu[0x200], fd[0x100]; int inv = 0;
    if (!dressed(regs[0], fu, sizeof fu, fd, sizeof fd, &inv) || inv) { call_body(dm_look_icon_body, entry, regs[0], regs[1], regs[8]); return; }
    call_body(dm_look_icon_body, entry, (uint64_t)(uintptr_t)fu, regs[1], regs[8]);
}


static uint64_t call_colour(uint64_t entry, uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3) {
    register uint64_t x0 __asm__("x0") = a0, x1 __asm__("x1") = a1, x2 __asm__("x2") = a2, x3 __asm__("x3") = a3, x16 __asm__("x16") = entry,
        x17 __asm__("x17") = (uint64_t)(uintptr_t)dm_look_colour_body;
    __asm__ volatile("blr x17" : "+r"(x0), "+r"(x1), "+r"(x2), "+r"(x3), "+r"(x16), "+r"(x17) : : "x4", "x5", "x6", "x7", "x8", "x9", "x10", "x11",
        "x12", "x13", "x14", "x15", "x30", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23",
        "v24", "v25", "v26", "v27", "v28", "v29", "v30", "v31", "cc", "memory");
    return x0;
}
void svc_look_colour(uint64_t *regs) {
    uint64_t entry = (uint64_t)(uintptr_t)base() + FN_COLOUR + 8, u = regs[1];
    snap_ent e; uint8_t fu[0x200], fd[0x100]; size_t nu = sizeof fu, nd = sizeof fd;
    if (select_ent(regs, SEL_COLOUR_RET, &e)) {
        nu = SEL_NU; nd = SEL_ND;
        if (e.look.kind == DM_LOOK_ORIGINAL && !e.dye) { regs[0] = call_colour(entry, regs[0], regs[1], regs[2], regs[3]); return; }
    } else if (!snap_of(u, &e, nu, nd)) { regs[0] = call_colour(entry, regs[0], regs[1], regs[2], regs[3]); return; }
    int looked = e.look.kind != DM_LOOK_ORIGINAL && !e.invisible;
    switch (dm_dye_source(e.dye, looked ? e.look.kind : DM_LOOK_ORIGINAL)) {
    case DM_DYE_GAME: regs[0] = call_colour(entry, regs[0], regs[1], regs[2], regs[3]); return;
    case DM_DYE_UNCOLOURED: regs[0] = 0; return;
    case DM_DYE_LOOK_ROW:
        dress(u, &e, fu, nu, fd, nd);
        regs[0] = call_colour(entry, regs[0], (uint64_t)(uintptr_t)fu, regs[2], regs[3]); return;
    }
    uint64_t T = tables(*(const uint8_t *)(u + U_VER)); uint32_t cls = looked ? e.dclass : *(const uint32_t *)(u + U_CLASS);
    if (!T || cls >= *(const uint32_t *)(T + T_NITEMS)) { regs[0] = call_colour(entry, regs[0], regs[1], regs[2], regs[3]); return; }
    const uint8_t *row = (const uint8_t *)(uintptr_t)(*(const uint64_t *)(T + T_ITEMS) + cls * (uint64_t)ITEM_STRIDE);
    unsigned c = e.dye - 1u, gf = row[B_GROUP_FIG], g = (uint32_t)regs[3] ? row[B_GROUP_INV] : gf;
    if (regs[2]) *(uint8_t *)(uintptr_t)regs[2] = gf - 1u <= 7u ? (uint8_t)(gf << 5 | c) : 0;
    regs[0] = g > 8 || (1u << g & 0x19u) ? 0 : (uint64_t)(uintptr_t)base() + COLOUR_TAB + g * 0x1500ull + c * 0x100ull;
}


const char *dm_vault_item_name(uint8_t *b, int kind, int row);
const char *dm_untag(const char *s);
const char *dm_str(uint8_t *b, const char *key);
static const char *game_name(uint8_t *b, uint32_t code) {
    char key[5] = { 0 }; memcpy(key, &code, 4); for (int i = 3; i >= 0 && key[i] == ' '; i--) key[i] = 0;
    struct { const char *k; uint64_t n; } s = { key, strlen(key) };
    const char *r = dm_untag(((const char *(*)(void *))(uintptr_t)(b + GA_LOC_STRING_GET))(&s));
    return r && *r && strcmp(r, key) ? r : NULL;
}

const char *dm_looks_name(dm_look l) {
    uint8_t *b = base();
    if (!b) return NULL;
    if (l.kind == DM_LOOK_INVISIBLE) return dm_str(b, "dmLooksInvisible");
    if (l.kind == DM_LOOK_UNIQUE || l.kind == DM_LOOK_SET) { const char *n = dm_vault_item_name(b, l.kind == DM_LOOK_UNIQUE, (int)l.id); if (n) return n; }
    uint32_t c = base_of(l);
    return c ? game_name(b, c) : NULL;
}

int dm_looks_of_client(uint64_t u, dm_look *out) {
    snap_ent e;
    if (!snap_of(u, &e, 0x1c0, D_NEXT) || e.look.kind == DM_LOOK_ORIGINAL) return 0;
    *out = e.look;
    return 1;
}


const char *dm_vault_item_icon(int kind, int row);
static int client_key(uint64_t u, dm_look_item *k, int *level) {
    if (!u || (u & 7) || !dm_guest_heap_readable(u, 0x1c0) || *(const uint32_t *)u != 4) return 0;
    uint64_t d = *(const uint64_t *)(u + U_DATA);
    if (!d || !dm_guest_heap_readable(d, D_NEXT)) return 0;
    k->fp = 0; k->code = item_code(tables(*(const uint8_t *)(u + U_VER)), *(const uint32_t *)(u + U_CLASS));
    k->quality = (uint8_t)*(const uint32_t *)(d + D_QUALITY); k->row = k->quality == Q_SET || k->quality == Q_UNIQUE ? *(const int32_t *)(d + D_ROW) : -1;
    if (level) *level = (int)*(const uint32_t *)(d + D_LEVEL);
    return k->code != 0;
}
int dm_looks_page_item(uint64_t u, int *group, int *w, int *h) {
    dm_look_item k; const dm_look_base *t;
    if (!table_ready() || !client_key(u, &k, NULL) || !(t = dm_looks_table_find(&g_tab, k.code))) return 0;
    *group = t->group; *w = t->w; *h = t->h;
    return 1;
}
int dm_looks_page_offer(uint64_t u, int pclass, dm_look *out, int max) {
    dm_look_item k; int n = 0;
    if (!table_ready() || !client_key(u, &k, NULL)) return 0;
    if (n < max) out[n++] = (dm_look){ DM_LOOK_ORIGINAL, 0 };
    if (n < max && dm_looks_invisible_ok(&g_tab, k.code)) out[n++] = (dm_look){ DM_LOOK_INVISIBLE, 0 };
    pthread_mutex_lock(&g_looks_mu);
    if (g_looks_state == DM_LOOKS_STATE_OK) n += dm_looks_offer(&g_looks, &g_tab, k.code, pclass, base_of, out + n, max - n);
    pthread_mutex_unlock(&g_looks_mu);

    if (k.quality == Q_UNIQUE || k.quality == Q_SET)
        for (int i = 0; i < n; i++) if (out[i].kind == (k.quality == Q_UNIQUE ? DM_LOOK_UNIQUE : DM_LOOK_SET) && (int)out[i].id == k.row) { memmove(&out[i], &out[i + 1], sizeof *out * (size_t)(n - i - 1)); n--; break; }
    return n;
}
int dm_looks_page_price(uint64_t u, dm_look l) {
    dm_look_item k; int level = 0;
    return client_key(u, &k, &level) && l.kind != DM_LOOK_ORIGINAL ? dm_looks_price(level, l.kind) : 0;
}
int dm_looks_page_state(uint64_t u, dm_look *cur, dm_look *shown) {
    snap_ent e;
    *cur = *shown = (dm_look){ DM_LOOK_ORIGINAL, 0 };
    if (!u || !dm_guest_heap_readable(u, 0x10) || !snap_get(*(const uint32_t *)(u + U_ID), &e)) return 0;
    *cur = e.cur; *shown = e.look;
    return 1;
}
int dm_looks_page_busy(uint64_t u) {
    snap_ent e;
    return u && dm_guest_heap_readable(u, 0x10) && snap_get(*(const uint32_t *)(u + U_ID), &e) && e.busy;
}
const char *dm_looks_icon(uint64_t u, dm_look l) {
    dm_look_item k; const dm_look_base *t;
    if (!table_ready() || !client_key(u, &k, NULL)) return NULL;
    if (l.kind == DM_LOOK_UNIQUE || l.kind == DM_LOOK_SET) return dm_vault_item_icon(l.kind == DM_LOOK_UNIQUE, (int)l.id);
    if (l.kind == DM_LOOK_BASE) return (t = dm_looks_table_find(&g_tab, l.id)) ? t->icon : NULL;

    if ((k.quality == Q_UNIQUE || k.quality == Q_SET) && own_look(k.code, k.quality, k.row)) { const char *i = dm_vault_item_icon(k.quality == Q_UNIQUE, k.row); if (i) return i; }
    return (t = dm_looks_table_find(&g_tab, k.code)) ? t->icon : NULL;
}


int dm_looks_page_dye(uint64_t u, int *cur, int *shown) {
    snap_ent e; *cur = *shown = 0;
    if (!u || !dm_guest_heap_readable(u, 0x10) || !snap_get(*(const uint32_t *)(u + U_ID), &e)) return 0;
    *cur = e.cur_dye; *shown = e.dye;
    return 1;
}
int dm_looks_page_where(uint64_t u, dm_look l) {
    dm_look_item k;
    if (g_dyes_state != 1 || !client_key(u, &k, NULL)) return 0;
    uint32_t c = l.kind == DM_LOOK_ORIGINAL || l.kind == DM_LOOK_INVISIBLE ? 0 : base_of(l);
    return dye_where(tables(*(const uint8_t *)(u + U_VER)), c ? c : k.code);
}

int dm_looks_page_gems(dm_dye_offer *out, int max) {
    uint8_t *b = base();
    if (g_dyes_state != 1 || !b) return 0;
    uint64_t pl = ((uint64_t (*)(uint32_t))(uintptr_t)(b + FN_CL_UNIT))(*(const uint32_t *)(b + CL_PLAYER_ID));
    uint64_t inv = pl ? *(const uint64_t *)(pl + U_INV) : 0, it = inv ? *(const uint64_t *)(inv + 0x10) : 0;
    uint32_t code[64]; int n = 0;
    for (int c = 0; it && c < 1024 && n < 64; c++) {
        uint64_t d = *(const uint64_t *)(it + U_DATA); if (!d) break;
        if (*(const uint32_t *)(it + U_MODE) == 0 && *(const uint8_t *)(d + D_PAGE) == 0) code[n++] = item_code(tables(*(const uint8_t *)(it + U_VER)), *(const uint32_t *)(it + U_CLASS));
        it = *(const uint64_t *)(d + D_NEXT);
    }
    return dm_dyes_offer(&g_dyes, code, n, out, max);
}
int dm_looks_dyes_on(void) { return g_dyes_state == 1; }

int dm_looks_dye_gem(int dye) {
    int best = -1;
    for (int i = 0; g_dyes_state == 1 && i < g_dyes.n; i++) if (g_dyes.g[i].colour + 1 == dye && (best < 0 || g_dyes.g[i].grade < g_dyes.g[best].grade)) best = i;
    return best;
}
const char *dm_looks_gem_icon(int gem) { return g_dyes_state == 1 && gem >= 0 && gem < g_dyes.n ? g_dyes.g[gem].icon : NULL; }
const char *dm_looks_gem_name(int gem) { return g_dyes_state == 1 && gem >= 0 && gem < g_dyes.n && base() ? game_name(base(), g_dyes.g[gem].code) : NULL; }


typedef struct { uint64_t it; dm_look_item key; int level, identified, mode, body; } owned;
static int srv_item(uint64_t T, uint64_t it, owned *o) {
    if (!dm_guest_heap_readable(it, 0x40) || *(const uint32_t *)it != 4) return 0;
    uint64_t d = *(const uint64_t *)(it + U_DATA);
    if (!d || !dm_guest_heap_readable(d, D_NEXT + 8)) return 0;
    o->it = it;
    o->key.code = item_code(T, *(const uint32_t *)(it + U_CLASS));
    o->key.quality = (uint8_t)*(const uint32_t *)(d + D_QUALITY);
    o->key.row = o->key.quality == Q_SET || o->key.quality == Q_UNIQUE ? *(const int32_t *)(d + D_ROW) : -1;
    o->key.fp = *(const uint32_t *)(it + U_IDENT);
    o->level = (int)*(const uint32_t *)(d + D_LEVEL);
    o->identified = (*(const uint32_t *)(d + D_FLAGS) & FL_IDENT) != 0;
    o->mode = (int)*(const uint32_t *)(it + U_MODE);
    o->body = *(const uint8_t *)(d + D_BODY);
    return o->key.code != 0;
}
static int owned_in(uint64_t T, uint64_t inv, owned *out, int n, int max) {
    uint64_t it = inv ? *(const uint64_t *)(inv + 0x10) : 0;
    for (int c = 0; it && c < 1024 && n < max; c++) {
        if (srv_item(T, it, &out[n]) && out[n].key.fp) n++;
        uint64_t d = *(const uint64_t *)(it + U_DATA); if (!d) break;
        it = *(const uint64_t *)(d + D_NEXT);
    }
    return n;
}


int dm_stash_pages(uint64_t G, uint64_t player, uint64_t pg[8], uint32_t id[8]); uint64_t dm_unit_inv(uint64_t u);
static int owned_items(uint64_t G, uint64_t player, owned *out, int max) {
    uint64_t T = tables(g_ver), pg[8]; uint32_t id[8];
    int n = owned_in(T, *(const uint64_t *)(player + U_INV), out, 0, max);
    for (int t = 0, m = dm_stash_pages(G, player, pg, id); t < m; t++) n = owned_in(T, dm_unit_inv(pg[t]), out, n, max);
    return n;
}
static void unlock(const owned *o, int *added) {
    dm_look l = dm_looks_unlocks(&g_tab, &o->key, o->identified, (o->key.quality == Q_UNIQUE || o->key.quality == Q_SET) && own_look(o->key.code, o->key.quality, o->key.row));
    if (l.kind != DM_LOOK_ORIGINAL && dm_looks_coll_add(&g_looks, l) == 1) (*added)++;
}


int dm_pages_slot_n(int page); int dm_pages_slot_item(int page, int i, const uint8_t **bits, int *len, int *x, int *y);
int dm_vault_records(int (*each)(const uint8_t *bits, int len, void *ctx), void *ctx); int dm_vault_settled(void);
static int unlock_bits(const uint8_t *bits, int len, void *ctx) {
    owned o = { 0 }; if (dm_looks_bits_item(bits, len, &o.key, &o.identified)) return 0;
    unlock(&o, (int *)ctx); return 0;
}
static void unlock_chronicle(uint64_t player, int *added) {
    uint64_t pd = *(const uint64_t *)(player + 0x10), T = tables(g_ver);
    if (!pd) return;
    for (int kind = 0; kind < 2; kind++) {
        uint64_t t = pd + 0x220 + 0x20 * (uint64_t)kind, nb = *(const uint64_t *)t, bk = *(const uint64_t *)(t + 8);
        if (!bk || !nb || (nb & (nb - 1)) || nb > 65536) continue;
        for (uint64_t i = 0; i < nb; i++)
            for (uint64_t node = *(const uint64_t *)(bk + 8 * i), k = 0; node && k < 4096; node = *(const uint64_t *)node, k++) {
                int row = (int)*(const uint32_t *)(node + 8); uint32_t code = row_code(T, kind == 1, row);
                if (!code) continue;
                owned o = { .key = { 1, code, (uint8_t)(kind ? Q_UNIQUE : Q_SET), row }, .identified = 1 };
                unlock(&o, added);
            }
    }
}


typedef struct { uint32_t unit_id; dm_look look; int dye; } req;
static req g_req[REQ_MAX]; static _Atomic int g_nreq; static pthread_mutex_t g_reqmu = PTHREAD_MUTEX_INITIALIZER;
static _Atomic uint64_t g_preview;
static _Atomic uint64_t g_preview_dye;
void dm_looks_request_apply(uint32_t unit_id, dm_look look, int dye) {
    pthread_mutex_lock(&g_reqmu);
    int n = atomic_load(&g_nreq); if (n < REQ_MAX) { g_req[n] = (req){ unit_id, look, dye }; atomic_store(&g_nreq, n + 1); }
    pthread_mutex_unlock(&g_reqmu);
}
void dm_looks_request_preview(uint32_t unit_id, dm_look look, int dye) {
    atomic_store(&g_preview, unit_id ? (uint64_t)unit_id << 40 | (uint64_t)look.kind << 32 | look.id : 0);
    atomic_store(&g_preview_dye, unit_id && dye >= 0 ? (uint64_t)unit_id << 8 | (uint64_t)(dye + 1) : 0);
}
int dm_looks_state(void) { return g_looks_state; }
int dm_looks_unlocked(void) {
    uint64_t q = *(const uint64_t *)(base() + NPC_QUEST);
    const uint8_t *flags = q && dm_guest_heap_readable(q, 8) ? *(const uint8_t *const *)(uintptr_t)q : NULL;
    if (!flags || !dm_guest_heap_readable((uint64_t)(uintptr_t)flags, 2 * TOOLS_WORD + 2)) return 0;
    return (flags[2 * TOOLS_WORD] & 3) != 0;
}

static int gold(uint64_t player) {
    int (*get)(uint64_t, int, int) = (int (*)(uint64_t, int, int))(uintptr_t)(base() + FN_STAT);
    return get(player, 14, 0) + get(player, 15, 0);
}
static int pclass(uint64_t player) { return (int)*(const uint32_t *)(player + U_CLASS); }
static int look_price(const owned *o, dm_look l) { return dm_looks_price(o->level, l.kind); }


static uint64_t g_resend[REQ_MAX * 2]; static int g_nresend;
static void resend_later(const owned *o) { for (int i = 0; i < g_nresend; i++) if (g_resend[i] == o->it) return; if (g_nresend < REQ_MAX * 2) g_resend[g_nresend++] = o->it; }
static void resend(uint64_t G, uint64_t player, const owned *o) {
    uint64_t d = *(const uint64_t *)(o->it + U_DATA), pp = *(const uint64_t *)(o->it + U_PATH);
    uint64_t pos = pp ? (uint64_t)*(const uint32_t *)(pp + 0x10) | (uint64_t)*(const uint32_t *)(pp + 0x14) << 16 : 0;
    ((void (*)(uint64_t, uint64_t, uint64_t, int, int, int, int, int, uint64_t))(uintptr_t)(base() + FN_SEND))
        (G, player, o->it, 1, 0, o->mode, *(const uint8_t *)(d + D_PAGE), *(const uint8_t *)(d + D_BODY), pos);
}
static int fits(const owned *o, dm_look l, int cls) {
    if (l.kind == DM_LOOK_ORIGINAL) return 1;
    if (l.kind == DM_LOOK_INVISIBLE) return dm_looks_invisible_ok(&g_tab, o->key.code);
    return dm_looks_coll_has(&g_looks, l) && dm_looks_fits(&g_tab, o->key.code, base_of(l), cls);
}

static uint64_t gem_for(uint64_t player, int dye) {
    uint64_t inv = *(const uint64_t *)(player + U_INV), it = inv ? *(const uint64_t *)(inv + 0x10) : 0, T = tables(g_ver), unit[64];
    uint32_t code[64]; int n = 0;
    for (int c = 0; it && c < 1024 && n < 64; c++) {
        uint64_t d = *(const uint64_t *)(it + U_DATA); if (!d) break;
        if (*(const uint32_t *)(it + U_MODE) == 0 && *(const uint8_t *)(d + D_PAGE) == 0 && !dm_pouch_holds(*(const uint32_t *)(it + U_ID))) {
            uint32_t k = item_code(T, *(const uint32_t *)(it + U_CLASS));
            if (dm_dyes_find(&g_dyes, k)) { unit[n] = it; code[n++] = k; }
        }
        it = *(const uint64_t *)(d + D_NEXT);
    }
    dm_dye_offer o[DM_DYE_COLOURS]; int m = dm_dyes_offer(&g_dyes, code, n, o, DM_DYE_COLOURS);
    for (int i = 0; i < m; i++) if (o[i].dye == dye) for (int k = 0; k < n; k++) if (code[k] == g_dyes.g[o[i].gem].code) return unit[k];
    return 0;
}


static int use_gem(uint64_t G, uint64_t player, uint64_t it, int *x, int *y) {
    uint64_t pp = *(const uint64_t *)(it + U_PATH), inv = *(const uint64_t *)(player + U_INV);
    *x = pp ? *(const int32_t *)(pp + 0x10) : 0; *y = pp ? *(const int32_t *)(pp + 0x14) : 0;
    ((void (*)(uint64_t))(uintptr_t)(base() + FN_UNLINK))(it);
    for (uint64_t v = inv ? *(const uint64_t *)(inv + 0x10) : 0, c = 0; v && c < 1024; c++) {
        if (v == it) { LOG("ASSUME looks.dye_gem: the gem is still in the inventory after it was taken out, no colour"); return 0; }
        uint64_t d = *(const uint64_t *)(v + U_DATA); if (!d) break;
        v = *(const uint64_t *)(d + D_NEXT);
    }
    ((void (*)(uint64_t, uint64_t, uint64_t, int, int, int, int, int, int))(uintptr_t)(base() + FN_SEND))(G, player, it, 0x20, 4, 0, 0, *y, *x);
    ((void (*)(uint64_t, uint64_t))(uintptr_t)(base() + FN_FREE))(G, it);
    return 1;
}

static uint32_t drawn(const owned *o, dm_look l) { uint32_t c = l.kind == DM_LOOK_ORIGINAL || l.kind == DM_LOOK_INVISIBLE ? 0 : base_of(l); return c ? c : o->key.code; }


typedef struct { uint64_t G, player; int price; uint64_t gem; int cdye; uint32_t gem_code; int gx, gy; } payment;
static int pay(void *ctx, int *dye) {
    payment *p = ctx;
    if (p->price && !((int (*)(uint64_t, uint64_t, int))(uintptr_t)(base() + FN_PAY))(0, p->player, p->price)) { LOG("looks: the game refused the payment of %d", p->price); return 0; }
    if (p->gem && !use_gem(p->G, p->player, p->gem, &p->gx, &p->gy)) { p->gem = 0; *dye = p->cdye; return p->price != 0; }
    return 1;
}


static int carry_out(uint64_t G, uint64_t player, const owned *all, int n, const req *r) {
    const owned *o = NULL;
    for (int i = 0; i < n; i++) if (*(const uint32_t *)(all[i].it + U_ID) == r->unit_id && all[i].mode == MODE_WORN) o = &all[i];
    if (dm_looks_latch()) { LOG("looks: changing looks is locked after a failed save, request dropped"); return 1; }
    if (!o) { LOG("looks: unit %u is not worn, request dropped", r->unit_id); return 1; }
    if (!fits(o, r->look, pclass(player))) { LOG("looks: the look does not fit unit %u, request dropped", r->unit_id); return 1; }
    if (dm_looks_busy(&g_looks, &o->key)) { LOG("looks: unit %u has a paid change not saved yet, request dropped", r->unit_id); return 1; }
    dm_look cur = dm_looks_current(&g_looks, &o->key);
    int cdye = dm_looks_current_dye(&g_looks, &o->key), dye = r->dye < 0 ? cdye : r->dye;
    int new_look = cur.kind != r->look.kind || cur.id != r->look.id, new_dye = dye != cdye;
    if (!new_look && !new_dye) return 1;
    uint64_t gem = 0;
    if (new_dye && dye) {
        if (dye > DM_DYE_COLOURS || g_dyes_state != 1) { LOG("looks: colour %d cannot be given, request dropped", dye); return 1; }
        if (!dye_where(tables(g_ver), drawn(o, r->look))) { LOG("ASSUME looks.dye_group: unit %u is drawn as a base type the game does not colour, request dropped", r->unit_id); return 1; }
        if (!(gem = gem_for(player, dye))) { LOG("looks: no gem of colour %d in the inventory, request dropped", dye - 1); return 1; }
    }
    int price = new_look && r->look.kind != DM_LOOK_ORIGINAL ? look_price(o, r->look) : 0;
    uint64_t seq = 0;
    if (price || gem) {

        int pr = dm_looks_pay_proof(G, &seq);
        if (!pr) return 0;
        if (pr == -2) { LOG("looks: changing looks is locked after a failed save, request dropped"); return 1; }
        if (gold(player) < price) { LOG("looks: %d gold needed, not enough", price); return 1; }
    }
    payment p = { G, player, price, gem, cdye, gem ? item_code(tables(g_ver), *(const uint32_t *)(gem + U_CLASS)) : 0, 0, 0 };
    dm_look l = new_look ? r->look : cur;
    int done = dm_looks_pay_choose(&g_looks, &o->key, l, dye, price || gem ? seq : 0, pay, &p);
    if (done < 0) { LOG("ASSUME looks.memory: no room for the change of unit %u, nothing paid, request dropped", r->unit_id); return 1; }
    if (!done) return 1;
    resend_later(o);
    dm_request_save();
    char used[40] = ""; if (p.gem) snprintf(used, sizeof used, " and the gem %.3s at %d,%d", (const char *)&p.gem_code, p.gx, p.gy);
    LOG("[looks] unit %u code %.4s now kind %d id %u colour %d for %d gold%s", r->unit_id, (const char *)&o->key.code, l.kind, l.id,
        dm_looks_current_dye(&g_looks, &o->key) - 1, price, used);
    return 1;
}


static char g_char[24]; static uint64_t g_pub_rev = ~0ull, g_pub_sig, g_pub_prev, g_pub_prevd;
int dm_pages_char(char *out, size_t n);
#ifndef DM_PLAYER_BUILD
static void test_apply(uint64_t G, uint64_t player, const owned *all, int n);
#endif
void dm_looks_tick(uint64_t G, uint64_t player) {
    uint8_t *b = base();
    if (!b || !player || !table_ready()) return;
    g_ver = *(const uint8_t *)(player + U_VER);
    char ch[24] = "";
    if (dm_pages_char(ch, sizeof ch)) return;
    static owned all[1024];
    int n = owned_items(G, player, all, 1024);
    pthread_mutex_lock(&g_looks_mu);
    if (strcmp(ch, g_char)) {
        if (!g_char[0] || g_looks_state == DM_LOOKS_STATE_OFF) dm_looks_load();
        strlcpy(g_char, ch, sizeof g_char); g_pub_rev = ~0ull;
    }
    dm_looks_latch();
    dm_looks_settle_committed();
    dyes_load(tables(g_ver));
    if (g_looks_state == DM_LOOKS_STATE_OK && !dm_looks_char_get(&g_looks, ch) && dm_pages_slot_n(0) >= 0 && dm_vault_settled()) {
            int added = 0;
            for (int p = 0; p < 5; p++) for (int i = 0, m = dm_pages_slot_n(p); i < m; i++) {
                const uint8_t *bits; int len, x, y; if (!dm_pages_slot_item(p, i, &bits, &len, &x, &y)) unlock_bits(bits, len, &added); }
            dm_vault_records(unlock_bits, &added);
            unlock_chronicle(player, &added);
            dm_look_char c; memset(&c, 0, sizeof c); strlcpy(c.name, ch, sizeof c.name); dm_looks_char_set(&g_looks, &c);
            LOG("looks: first visit of this character, %d looks from stash pages, Chronicle stash and Chronicle", added);
    }
    if (g_looks_state != DM_LOOKS_STATE_OK) {
        if (g_pub_rev) { snap_publish(NULL, 0); g_pub_rev = 0; }
        pthread_mutex_unlock(&g_looks_mu);
        return;
    }
    uint64_t sig = 0;
    for (int i = 0; i < n; i++) sig = dm_looks_sig_add(sig, &all[i].key, *(const uint32_t *)(all[i].it + U_ID), all[i].mode, all[i].identified);
    if (sig != g_pub_sig) { int added = 0; for (int i = 0; i < n; i++) unlock(&all[i], &added); if (added) LOG("looks: %d new looks collected", added); }
    {
        static uint64_t warned;
        if (sig != g_pub_sig) for (int i = 0; i < n; i++) for (int k = i + 1; k < n; k++)
            if (dm_looks_item_eq(&all[i].key, &all[k].key) && warned != all[i].key.fp) { warned = all[i].key.fp; LOG("ASSUME looks.unique: two items share the identity %08x", all[i].key.fp); }
    }
    for (int k = 0, m = atomic_load(&g_nreq); k < m; ) {
        pthread_mutex_lock(&g_reqmu);
        req r = g_req[0];
        pthread_mutex_unlock(&g_reqmu);
        if (!carry_out(G, player, all, n, &r)) break;
        pthread_mutex_lock(&g_reqmu);
        int c = atomic_load(&g_nreq); memmove(&g_req[0], &g_req[1], sizeof g_req[0] * (size_t)(c - 1)); atomic_store(&g_nreq, c - 1);
        pthread_mutex_unlock(&g_reqmu);
        m = c - 1;
    }
#ifndef DM_PLAYER_BUILD
    test_apply(G, player, all, n);
#endif
    uint64_t prev = atomic_load(&g_preview), prevd = atomic_load(&g_preview_dye);
    if (g_looks.rev != g_pub_rev || sig != g_pub_sig || prev != g_pub_prev || prevd != g_pub_prevd) {
        static snap_ent e[SNAP_MAX]; int k = 0, over = 0;
        uint64_t T = tables(g_ver);
        for (int i = 0; i < n && !over; i++) {
            dm_look cur = dm_looks_current(&g_looks, &all[i].key), l = cur;
            if (!fits(&all[i], cur, pclass(player))) cur = l = (dm_look){ DM_LOOK_ORIGINAL, 0 };
            uint32_t id = *(const uint32_t *)(all[i].it + U_ID);
            if (prev && (uint32_t)(prev >> 40) == id) l = (dm_look){ (uint8_t)(prev >> 32 & 0xff), (uint32_t)prev };
            if (!fits(&all[i], l, pclass(player))) l = cur;
            int cdye = g_dyes_state == 1 ? dm_looks_current_dye(&g_looks, &all[i].key) : 0, dye = cdye;
            if (prevd && (uint32_t)(prevd >> 8) == id && g_dyes_state == 1) dye = (int)(prevd & 0xff) - 1;
            if (l.kind == DM_LOOK_ORIGINAL && cur.kind == DM_LOOK_ORIGINAL && !dye && !cdye) continue;
            if (k == SNAP_MAX) {
                static int warned; over = 1;
                if (!warned) { warned = 1; LOG("ASSUME looks.display_cap: more than %d dressed items, the others are drawn as themselves", SNAP_MAX); }
                break;
            }
            snap_ent s = { id, all[i].key.code, all[i].key.quality, l.kind == DM_LOOK_INVISIBLE, all[i].key.row, 0, 2, -1, l, cur, (uint8_t)dm_looks_busy(&g_looks, &all[i].key), (uint8_t)dye, (uint8_t)cdye };
            if (!s.invisible && l.kind != DM_LOOK_ORIGINAL) {
                int cls = item_class(T, base_of(l));
                if (cls < 0) { LOG("ASSUME looks.known: the game does not know the look kind %d id %u, the item stays original", l.kind, l.id); continue; }
                s.dclass = (uint32_t)cls;
                s.dquality = l.kind == DM_LOOK_UNIQUE ? Q_UNIQUE : l.kind == DM_LOOK_SET ? Q_SET : 2;
                s.drow = l.kind == DM_LOOK_UNIQUE || l.kind == DM_LOOK_SET ? (int32_t)l.id : -1;
            }
            e[k++] = s;
        }

        if (prev != g_pub_prev || prevd != g_pub_prevd) for (int i = 0; i < n; i++) {
            uint32_t id = *(const uint32_t *)(all[i].it + U_ID);
            if (all[i].mode == MODE_WORN && (id == (uint32_t)(prev >> 40) || id == (uint32_t)(g_pub_prev >> 40) || id == (uint32_t)(prevd >> 8) || id == (uint32_t)(g_pub_prevd >> 8))) resend_later(&all[i]);
        }
        snap_publish(e, k);
        g_pub_rev = g_looks.rev; g_pub_sig = sig; g_pub_prev = prev; g_pub_prevd = prevd;
    }
    for (int r = 0; r < g_nresend; r++) for (int i = 0; i < n; i++) if (all[i].it == g_resend[r]) { resend(G, player, &all[i]); break; }
    g_nresend = 0;
    pthread_mutex_unlock(&g_looks_mu);
}


#ifndef DM_PLAYER_BUILD



static void test_apply(uint64_t G, uint64_t player, const owned *all, int n) {
    (void)G;
    static int done[8], nt = -1; static struct { uint32_t code; dm_look look; uint32_t at; } t[8];
    if (nt < 0) {
        nt = 0; const char *e = getenv("DM_TEST_LOOKS_APPLY"); if (!e) return;
        char buf[256]; strlcpy(buf, e, sizeof buf);
        for (char *sv, *s = strtok_r(buf, ",", &sv); s && nt < 8; s = strtok_r(NULL, ",", &sv)) {
            char a[8] = "", l[8] = ""; unsigned at = 0;
            if (sscanf(s, "%4[^:]:%5[^@]@%u", a, l, &at) < 2) continue;
            uint32_t c = 0x20202020; memcpy(&c, a, strlen(a)); t[nt].code = c; t[nt].at = at;
            if (l[0] == '-') t[nt].look = (dm_look){ DM_LOOK_INVISIBLE, 0 };
            else if (l[0] == '=') t[nt].look = (dm_look){ DM_LOOK_ORIGINAL, 0 };
            else if ((l[0] == 'u' || l[0] == 's') && l[1] >= '0' && l[1] <= '9') t[nt].look = (dm_look){ l[0] == 'u' ? DM_LOOK_UNIQUE : DM_LOOK_SET, (uint32_t)atoi(l + 1) };
            else { uint32_t d = 0x20202020; memcpy(&d, l, strlen(l) > 4 ? 4 : strlen(l)); t[nt].look = (dm_look){ DM_LOOK_BASE, d }; }
            nt++;
        }
    }
    for (int k = 0; k < nt; k++) {
        if (done[k] || dm_cur_tick() < t[k].at) continue;
        for (int i = 0; i < n; i++) if (all[i].mode == MODE_WORN && all[i].key.code == t[k].code) {
            done[k] = 1;
            if (t[k].look.kind == DM_LOOK_BASE || t[k].look.kind == DM_LOOK_UNIQUE || t[k].look.kind == DM_LOOK_SET) dm_looks_coll_add(&g_looks, t[k].look);
            dm_looks_request_apply(*(const uint32_t *)(all[i].it + U_ID), t[k].look, -1);
            LOG("[looks] test apply to unit %u at tick %u", *(const uint32_t *)(all[i].it + U_ID), dm_cur_tick());
            break;
        }
    }
}
#endif
