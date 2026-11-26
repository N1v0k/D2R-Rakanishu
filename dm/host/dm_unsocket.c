



















#include <stdatomic.h>
#include "game_addrs.h"
#include <string.h>
#include <unistd.h>
#include "dm.h"
#include "hle.h"
#include "dm_itemjob.h"
#include "dm_unsocket.h"

enum {
    TABLES = GA_DATA_TABLES, T_ROWS = 0x1590, T_NROWS = 0x1598, T_ITEMS = 0x15c0, T_NITEMS = 0x15c8, T_CODES = 0x258,
    ITEM_REC = 0x1c0, ITEM_W = 0x11e, ITEM_H = 0x11f, ITEM_NODUR = 0x122,
    G_VER = 0x106, G_ITEMS = 0x2e30, U_NEXT = 0x158, U_DATA = 0x10, U_ID = 8, U_MODE = 0xc, U_INV = 0x90, U_VER = 0x1bd,
    D_FLAGS = 0x18, D_NEXT = 0xb0, D_PAGE = 0x55, INV_SIG = 0x01020304, INV_FIRST = 0x10, INV_GRIDS = 0x20, INV_NGRIDS = 0x28,
    PAGE_INV = 0, PAGE_CUBE = 3, FL_ETHEREAL = 0x400000,
    ST_DUR = 72, ST_MAXDUR = 73, ST_INDESTRUCTIBLE = 152, ST_SOCKETS = 194,
    FN_TRANSMUTE = GA_CUBE_TRANSMUTE, FN_CODE = GA_ITEM_CODE, FN_STAT = GA_STAT_GET, FN_BROKE = GA_ITEM_BREAK, FN_FIND = GA_INV_FIND_FREE, FN_MODE = GA_UNIT_SET_MODE, FN_SOCKET = GA_ITEM_SOCKET_INSERT, FN_FREE = GA_ITEM_FREE,
    INV_ROWS = GA_CLASS_INV_GRID,
    MAX_FILL = 8, MAX_TABLES = 3,
};

static uint8_t *base(void) {
    static uint8_t *b;
    if (!b) { extern module_t g_mod[]; extern int g_nmod; for (int i = 0; i < g_nmod; i++) if (!strcmp(g_mod[i].name, "main")) b = (uint8_t *)g_mod[i].base; }
    return b;
}
int dm_guest_heap_readable(uint64_t a, size_t n);
static int rd(uint64_t a, void *v, size_t n) { if (!a || (a & 3) || !dm_guest_heap_readable(a, n)) return 0; memcpy(v, (const void *)(uintptr_t)a, n); return 1; }
static uint8_t rd8(uint64_t a) { uint8_t v = 0; return dm_guest_heap_readable(a, 1) ? (memcpy(&v, (const void *)(uintptr_t)a, 1), v) : 0; }
static uint64_t rd64(uint64_t a) { uint64_t v = 0; return rd(a, &v, 8) ? v : 0; }
static uint32_t rd32(uint64_t a) { uint32_t v = 0; return rd(a, &v, 4) ? v : 0; }
static uint64_t table(unsigned ver) { return ver >= 1 && ver <= 3 ? *(const uint64_t *)(base() + TABLES + 16 * (uint64_t)ver) : 0; }
static uint32_t code_of(uint64_t it) { return ((uint32_t (*)(uint64_t))(uintptr_t)(base() + FN_CODE))(it); }
static int gstat(uint64_t u, int st) { return ((int (*)(uint64_t, int, int))(uintptr_t)(base() + FN_STAT))(u, st, 0); }
static uint32_t code4(const char *c) { uint32_t v = 0x20202020; memcpy(&v, c, strlen(c) < 4 ? strlen(c) : 4); return v; }
static int item_index(uint64_t T, uint32_t code) {
    uint64_t ix = rd64(T + T_CODES), ar = rd64(ix), n = rd64(ix + 8);
    for (uint64_t k = 0; ar && k < n && k < 4096; k++) if (rd32(ar + 8 * k) == code) return (int)rd32(ar + 8 * k + 4);
    return -1;
}

static const uint8_t *record(uint64_t it) {
    uint64_t T = table(rd8(it + U_VER)), rec = rd64(T + T_ITEMS); uint32_t cls = rd32(it + 4), n = rd32(T + T_NITEMS);
    if (!rec || cls >= n || !dm_guest_heap_readable(rec + (uint64_t)cls * ITEM_REC, ITEM_REC)) return NULL;
    return (const uint8_t *)(uintptr_t)(rec + (uint64_t)cls * ITEM_REC);
}

static int cells(uint64_t it, int *w, int *h) {
    const uint8_t *p = record(it);
    if (!p) return 0;
    *w = p[ITEM_W]; *h = p[ITEM_H]; return *w > 0 && *h > 0;
}
static uint64_t item_data(uint64_t it) { return rd64(it + U_DATA); }
static uint64_t first_in(uint64_t u) { uint64_t inv = rd64(u + U_INV); return inv && rd32(inv) == INV_SIG ? rd64(inv + INV_FIRST) : 0; }
static uint64_t next_of(uint64_t it) { uint64_t d = item_data(it); return d ? rd64(d + D_NEXT) : 0; }
static uint64_t srv_item(uint64_t G, uint32_t id) {
    for (uint64_t u = rd64(G + G_ITEMS + 8 * (uint64_t)(id & 0x7f)); u; u = rd64(u + U_NEXT)) if (rd32(u) == 4 && rd32(u + U_ID) == id) return u;
    return 0;
}


static struct row { uint64_t T, rows, nrows; int index; uint16_t ral; uint8_t saved[DM_UNS_ROW]; } g_rows[MAX_TABLES];
static int g_nrows, g_ready = -1;
static void find_rows(void) {
    if (g_ready >= 0 || !base()) return;
    for (unsigned v = 1; v <= 3; v++) if (!table(v)) return;
    int ok = 1; g_nrows = 0;
    for (unsigned v = 1; v <= 3 && ok; v++) {
        uint64_t T = table(v); int dup = 0;
        for (int i = 0; i < g_nrows; i++) dup |= g_rows[i].T == T;
        if (dup) continue;
        struct row *r = &g_rows[g_nrows];
        int hel = item_index(T, code4("r15")), ral = item_index(T, code4("r08")), tsc = item_index(T, code4("tsc"));
        r->T = T; r->rows = rd64(T + T_ROWS); r->nrows = rd64(T + T_NROWS); r->index = -1;
        int hits = 0;
        if (hel >= 0 && ral >= 0 && tsc >= 0 && r->rows && r->nrows < 4096 && dm_guest_heap_readable(r->rows, r->nrows * DM_UNS_ROW))
            for (uint64_t k = 0; k < r->nrows; k++) if (dm_uns_row_is_clear((const uint8_t *)(uintptr_t)(r->rows + k * DM_UNS_ROW), (uint16_t)hel, (uint16_t)tsc)) { hits++; r->index = (int)k; }
        if (hits != 1) { LOG("ASSUME unsocket.row: table of version %u has %d socket-clearing rows (Hel %d, Ral %d, scroll %d); unsocketing is off", v, hits, hel, ral, tsc); ok = 0; break; }
        r->ral = (uint16_t)ral; memcpy(r->saved, (const void *)(uintptr_t)(r->rows + (uint64_t)r->index * DM_UNS_ROW), DM_UNS_ROW);
        LOG("unsocket: table of version %u, row %d of %llu (Hel %d, Ral %d, scroll %d)", v, r->index, (unsigned long long)r->nrows, hel, ral, tsc);
#ifndef DM_PLAYER_BUILD
        { char hx[2 * 0x58 + 1]; for (int i = 0; i < 0x58; i++) snprintf(hx + 2 * i, 3, "%02x", r->saved[i]); LOG("unsocket test: row bytes %s", hx); }
#endif
        g_nrows++;
    }
    g_ready = ok;
}
int dm_unsocket_ready(void) { find_rows(); return g_ready == 1; }


typedef struct {
    uint64_t G, pl, req, mode, entry;
    struct row *row; uint8_t *at;
    uint32_t base, ral, tsc, fill[MAX_FILL]; int nfill, indestructible;
} call_t;




static int free_cells(uint64_t grids, uint64_t ng, int page) {
    uint64_t g = grids + 0x20 * (uint64_t)(page + 2), cells = grids && ng > (uint64_t)(page + 2) ? rd64(g + 0x18) : 0;
    int w = cells ? rd8(g + 0x10) : 0, h = cells ? rd8(g + 0x11) : 0, n = 0;
    if (!cells || w < 1 || h < 1 || w > 16 || h > 16 || !dm_guest_heap_readable(cells, (size_t)w * h * 8)) return 0;
    for (int i = 0; i < w * h; i++) n += rd64(cells + 8 * (uint64_t)i) == 0;
    return n;
}
static int op_view(void *x, dm_uns_view *v) {
    call_t *c = x;
    if (!c->pl || !c->G) return 0;
    uint64_t inv = rd64(c->pl + U_INV);
    if (!inv || rd32(inv) != INV_SIG) return 0;
    uint32_t ralc = code4("r08"), tscc = code4("tsc"); uint64_t bu = 0;
    int k = 0;
    for (uint64_t it = rd64(inv + INV_FIRST); it && k < 1024; it = next_of(it), k++) {
        uint64_t d = item_data(it); if (!d) return 0;
        if (rd32(it + U_MODE) != 0 || rd8(d + D_PAGE) != PAGE_CUBE) continue;
        v->items++; uint32_t code = code_of(it);
        if (code == ralc) { v->ral++; c->ral = rd32(it + U_ID); continue; }
        if (code == tscc) { v->scroll++; c->tsc = rd32(it + U_ID); continue; }
        int n = 0; for (uint64_t f = first_in(it); f && n < MAX_FILL; f = next_of(f)) n++;
        if (n) { v->bases++; bu = it; }
    }
    if (v->items != 3 || v->bases != 1 || v->ral != 1 || v->scroll != 1) return 1;
    c->base = rd32(bu + U_ID); c->nfill = 0;
    for (uint64_t f = first_in(bu); f && c->nfill < MAX_FILL; f = next_of(f)) {
        int w = 0, h = 0; c->fill[c->nfill++] = rd32(f + U_ID);
        v->fillers++; v->fillers_small += cells(f, &w, &h) && w == 1 && h == 1;
    }
    if (!cells(bu, &v->base_w, &v->base_h)) v->base_w = v->base_h = 0;
    uint64_t grids = rd64(inv + INV_GRIDS), ng = rd64(inv + INV_NGRIDS), g = grids + 0x20 * (PAGE_CUBE + 2);
    if (grids && ng > PAGE_CUBE + 2 && rd64(g + 0x18)) { v->cube_w = rd8(g + 0x10); v->cube_h = rd8(g + 0x11); }

    v->inv_free = gate_svc_present(SVC_HOST_TRANSMUTE_SPILL) && gate_svc_present(SVC_HOST_TRANSMUTE_KEEP) ? free_cells(grids, ng, PAGE_INV) : 0;
    c->indestructible = gstat(bu, ST_INDESTRUCTIBLE) > 0;
    v->job_busy = dm_itemjob_owner() != DM_ITEMJOB_NONE;
    return 1;
}


static struct { int active, left, done, kept; uint32_t base; uint64_t G, pl; } g_spill;
static int op_lend(void *x, int spill) {
    call_t *c = x; find_rows();
    if (g_ready != 1) return 0;
    uint64_t T = table(rd8(c->G + G_VER));
    for (int i = 0; i < g_nrows; i++) if (g_rows[i].T == T) c->row = &g_rows[i];
    struct row *r = c->row;
    if (!r || rd64(T + T_ROWS) != r->rows || rd64(T + T_NROWS) != r->nrows) return 0;
    c->at = (uint8_t *)(uintptr_t)(r->rows + (uint64_t)r->index * DM_UNS_ROW);
    if (memcmp(c->at, r->saved, DM_UNS_ROW)) { c->at = NULL; return 0; }
    dm_uns_row_lend(c->at, r->ral);
    if (c->mode & 1) { g_spill.active = 1; g_spill.left = spill; g_spill.done = 0; g_spill.kept = 0; g_spill.base = c->base; g_spill.G = c->G; g_spill.pl = c->pl; }
    return 1;
}
static void op_give_back(void *x) {
    call_t *c = x;
    if (g_spill.active) {
        if (g_spill.left) LOG("ASSUME unsocket.spill: item %u: %d of the inserts meant for the inventory did not go there", g_spill.base, g_spill.left);
        if (g_spill.done) LOG("unsocket: item %u, %d inserts went to the inventory", g_spill.base, g_spill.done);
        if (g_spill.kept) LOG("ASSUME unsocket.kept: item %u: %d inserts the game could not place went back into their item", g_spill.base, g_spill.kept);
    }
    memset(&g_spill, 0, sizeof g_spill);
    if (c->at) memcpy(c->at, c->row->saved, DM_UNS_ROW);
#ifndef DM_PLAYER_BUILD
    if (c->at) LOG("unsocket test: row given back, %s", memcmp(c->at, c->row->saved, DM_UNS_ROW) ? "DIFFERS" : "equal to the saved bytes");
#endif
    c->at = NULL;
}
uint64_t dm_unsocket_body(uint64_t g, uint64_t pl, uint64_t req, uint64_t mode);
static uint64_t call_body(uint64_t entry, uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3) {
    register uint64_t x0 __asm__("x0") = a0, x1 __asm__("x1") = a1, x2 __asm__("x2") = a2, x3 __asm__("x3") = a3, x16 __asm__("x16") = entry,
        x17 __asm__("x17") = (uint64_t)(uintptr_t)dm_unsocket_body;
    __asm__ volatile("blr x17" : "+r"(x0), "+r"(x1), "+r"(x2), "+r"(x3), "+r"(x16), "+r"(x17) : : "x4", "x5", "x6", "x7", "x8", "x9", "x10", "x11",
        "x12", "x13", "x14", "x15", "x30", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23",
        "v24", "v25", "v26", "v27", "v28", "v29", "v30", "v31", "cc", "memory");
    return x0;
}
static uint64_t op_body(void *x) { call_t *c = x; return call_body(c->entry, c->G, c->pl, c->req, c->mode); }
static int at_home(uint64_t pl, uint32_t id) {
    for (uint64_t it = first_in(pl); it; it = next_of(it)) {
        if (rd32(it + U_ID) != id) continue;
        uint64_t d = item_data(it); int page = d ? rd8(d + D_PAGE) : -1;
        return rd32(it + U_MODE) == 0 && (page == PAGE_CUBE || page == PAGE_INV);
    }
    return 0;
}
static void op_after(void *x, dm_uns_after *a, dm_uns_item *it) {
    call_t *c = x; uint64_t bu = srv_item(c->G, c->base);
    a->base_found = bu != 0; a->ral_left = srv_item(c->G, c->ral) != 0; a->scroll_left = srv_item(c->G, c->tsc) != 0; a->fillers_noted = c->nfill;
    for (uint64_t f = bu ? first_in(bu) : 0; f && a->base_fillers < MAX_FILL; f = next_of(f)) a->base_fillers++;
    for (int i = 0; i < c->nfill; i++) a->fillers_home += at_home(c->pl, c->fill[i]);
    if (!bu) return;
    uint64_t d = item_data(bu);
    it->ethereal = d && (rd32(d + D_FLAGS) & FL_ETHEREAL); it->indestructible_before = c->indestructible;
    it->indestructible_after = gstat(bu, ST_INDESTRUCTIBLE) > 0; it->max_dur = gstat(bu, ST_MAXDUR);
    const uint8_t *rec = record(bu);
    it->no_durability = !rec || rec[ITEM_NODUR] != 0;
}
static void op_penalty(void *x) {
    call_t *c = x; uint64_t bu = srv_item(c->G, c->base);
    if (bu) ((void (*)(uint64_t, uint64_t, uint64_t))(uintptr_t)(base() + FN_BROKE))(c->G, c->pl, bu);
}
static _Atomic int g_notice_req, g_notice_over;






enum { X26_CUBE = 0x300000000ull };
#ifndef DM_PLAYER_BUILD
static int test_fail_search(void);
#endif

static int inv_row(uint64_t pl) { uint32_t cls = rd32(pl + 4); return cls <= 7 ? *(const int32_t *)(base() + INV_ROWS + 8 * (uint64_t)cls + 4) : -1; }
typedef uint64_t (*find_fn)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);
void svc_unsocket_spill(uint64_t *regs) {
    find_fn find = (find_fn)(uintptr_t)(base() + FN_FIND);
    if (!g_spill.active) { regs[0] = find(regs[0], regs[1], regs[2], regs[3], regs[4], regs[5]); return; }
    int spill = g_spill.left > 0, ours = regs[26] == X26_CUBE || regs[26] == (X26_CUBE & 0xffffffffull);
    if (!ours && spill) { LOG("ASSUME unsocket.spill_page: page word %#llx, not the cube's; no insert goes to the inventory", (unsigned long long)regs[26]); spill = 0; g_spill.left = 0; }
    if (ours) regs[26] = X26_CUBE;
    uint64_t r = find(regs[0], regs[1], regs[2], regs[3], regs[4], regs[5]);
#ifndef DM_PLAYER_BUILD
    if (r && test_fail_search()) r = 0;
#endif
    if (!r && spill) {
        int row = inv_row(g_spill.pl);
        r = row >= 0 ? find(regs[0], regs[1], (uint64_t)(uint32_t)row, regs[3], regs[4], PAGE_INV) : 0;
#ifndef DM_PLAYER_BUILD
        if (r && test_fail_search()) r = 0;
#endif
        if (r) { regs[26] = X26_CUBE & 0xffffffffull; g_spill.left--; g_spill.done++; }
        else LOG("ASSUME unsocket.spill: no inventory cell for insert %u although the check counted one", rd32(regs[1] + U_ID));
    }
    regs[0] = r;
}




void svc_unsocket_keep(uint64_t *regs) {
    uint64_t ins = regs[1], bu = g_spill.active ? srv_item(g_spill.G, g_spill.base) : 0;
    if (bu && ins) {
        ((void (*)(uint64_t, int))(uintptr_t)(base() + FN_MODE))(ins, 4);
        if (((int (*)(uint64_t, uint64_t, uint32_t, uint32_t, int))(uintptr_t)(base() + FN_SOCKET))(g_spill.G, bu, rd32(ins + U_ID), g_spill.base, 2)) {
            g_spill.kept++;
            LOG("unsocket: insert %u could not be placed and went back into item %u", rd32(ins + U_ID), g_spill.base);
            return;
        }
    }
    if (g_spill.active) LOG("ASSUME unsocket.lost: insert %u could not be placed nor put back into item %u; the game deletes it", rd32(ins + U_ID), g_spill.base);
    ((void (*)(uint64_t, uint64_t))(uintptr_t)(base() + FN_FREE))(regs[0], ins);
}
static void op_notice(void *x, int why, int over) {
    call_t *c = x;
    switch (why) {
    case DM_UNS_REFUSE_ROOM: LOG("unsocket: item %u with %d inserts refused, the inventory lacks %d free cells for those the cube cannot take", c->base, c->nfill, over);
        atomic_store(&g_notice_over, over); atomic_store(&g_notice_req, 1); break;
    case DM_UNS_REFUSE_SHAPE: LOG("ASSUME unsocket.fillers: an insert of item %u takes more than one cell; refused", c->base); break;
    case DM_UNS_REFUSE_BUSY: LOG("unsocket: item %u refused, an item job (%d) owns the items", c->base, dm_itemjob_owner()); break;
    default: LOG("unsocket: item %u refused, the cube or the item could not be read", c->base); break;
    }
}
#ifndef DM_PLAYER_BUILD
static void phase(const char *name);
static uint32_t g_look_id; static int g_look_wait;
#endif
static void op_note(void *x, int what, int mode) {
    call_t *c = x; uint64_t bu = c->base ? srv_item(c->G, c->base) : 0;
    switch (what) {
    case DM_UNS_N_UNREAD: LOG("unsocket: mode %d, the cube could not be read; the game runs as always", mode); break;
    case DM_UNS_N_ROW: LOG("ASSUME unsocket.row: the recipe row is not the one checked at the start (mode %d); refused", mode); break;
    case DM_UNS_N_REFUSED: break;
    case DM_UNS_N_QUEUED: LOG("unsocket: mode 0, item %u with %d inserts: the run is queued", c->base, c->nfill); break;
    case DM_UNS_N_NOT_QUEUED: LOG("unsocket: mode 0, item %u: the game did not queue the run", c->base); break;
    case DM_UNS_N_NOT_RUN: LOG("unsocket: mode 1, item %u: the game did not run the recipe; nothing changed", c->base); break;
    case DM_UNS_N_ODD: LOG("ASSUME unsocket.returned: after the run item %u is %s, not every insert of %d lies in the cube or inventory, or an ingredient is left; no durability change",
                           c->base, bu ? "there" : "gone", c->nfill); break;
    case DM_UNS_N_DONE: {


        int dm_request_save(void); int asked = dm_request_save();
        LOG("unsocket: item %u emptied, %d inserts back; save requested %d", c->base, c->nfill, asked); break; }
    case DM_UNS_N_PENALISED:
#ifndef DM_PLAYER_BUILD
        g_look_id = c->base; g_look_wait = 25;
#endif
        LOG("unsocket: item %u now durability %d of %d", c->base, bu ? gstat(bu, ST_DUR) : -1, bu ? gstat(bu, ST_MAXDUR) : -1); break;
    case DM_UNS_N_SPARED: LOG("unsocket: item %u keeps its durability (ethereal, indestructible or none)", c->base); break;
    }
#ifndef DM_PLAYER_BUILD
    static const char *names[] = { [DM_UNS_N_QUEUED] = "queued", [DM_UNS_N_DONE] = "done", [DM_UNS_N_PENALISED] = "penalised", [DM_UNS_N_SPARED] = "penalised" };
    if (what < (int)(sizeof names / sizeof *names) && names[what]) phase(names[what]);
#endif
}
#ifndef DM_PLAYER_BUILD
static int op_nolend(void *x, int spill) { (void)x; (void)spill; return 0; }
#endif
static const dm_uns_ops OPS = { op_view, op_lend, op_give_back, op_body, op_after, op_penalty, op_notice, op_note };



static int g_in_run;
void svc_transmute(uint64_t *regs) {
    call_t c; memset(&c, 0, sizeof c);
    c.G = regs[0]; c.pl = regs[1]; c.req = regs[2]; c.mode = regs[3]; c.entry = (uint64_t)(uintptr_t)base() + FN_TRANSMUTE + 8;
    g_in_run = (int)(c.mode & 1);
#ifndef DM_PLAYER_BUILD
    static int nolend = -1; if (nolend < 0) nolend = getenv("DM_TEST_UNSOCKET_NOLEND") != NULL;
    dm_uns_ops ops = OPS; if (nolend) ops.lend = op_nolend;
    regs[0] = dm_uns_gate(&ops, &c, (int)(c.mode & 1));
    LOG("unsocket test: mode %llu answer %llu tick %u thread %d", (unsigned long long)c.mode, (unsigned long long)regs[0], rd32(c.G + 0x170), gettid());
#else
    regs[0] = dm_uns_gate(&OPS, &c, (int)(c.mode & 1));
#endif
    g_in_run = 0;
}
#ifndef DM_PLAYER_BUILD
static void test_serialized(void);
#endif
static void on_serialize(const uint64_t *regs) {
    (void)regs;
    if (g_in_run) LOG("ASSUME unsocket.serialize: a save started inside a transmute run (thread %d); it may hold a half-done state", gettid());
#ifndef DM_PLAYER_BUILD
    test_serialized();
#endif
}


void dm_game_notice(uint8_t *main_base, const char *text);
const char *dm_str(uint8_t *b, const char *key);
#ifndef DM_PLAYER_BUILD
static void test_step(uint8_t *b, uint64_t G, uint64_t pl);
#endif
void dm_unsocket_server_step(uint8_t *b, uint64_t G, uint64_t pl) {
    if (!b || !pl) return;
    static int hooked;
    if (!hooked) { hooked = 1; if (gate_obs_present(OBS_SAVE_SERIALIZE)) gate_obs_set(OBS_SAVE_SERIALIZE, on_serialize); else LOG("ASSUME unsocket.serialize_gate: the pack has no save serializer gate; a save inside a run would go unnoticed"); }
    if (atomic_exchange(&g_notice_req, 0)) { char t[200]; snprintf(t, sizeof t, dm_str(b, "dmUnsocketRoom"), atomic_load(&g_notice_over)); dm_game_notice(b, t); }
#ifndef DM_PLAYER_BUILD
    test_step(b, G, pl);
#else
    (void)G;
#endif
}

#ifndef DM_PLAYER_BUILD







void dm_fs_test_fail_next(void);
uint64_t dm_fs_commit_count(void);
int dm_fs_txn_open(void);
int dm_request_save(void);
uint64_t now_ns(void);
static char g_at_phase[16], g_at_action[16];
static int g_post;
static int g_settle;
static uint64_t g_quiet_n, g_quiet_since, g_post_commits, g_serials;
static void phase(const char *name) {
    static int parsed;
    if (!parsed) { parsed = 1; const char *e = getenv("DM_TEST_UNSOCKET_AT"); if (e) sscanf(e, "%15[^:]:%15s", g_at_phase, g_at_action); }
    if (!strcmp(name, "penalised")) { g_post = 1; g_quiet_since = 0; }
    if (!g_at_phase[0] || strcmp(name, g_at_phase)) return;
    LOG("unsocket test: phase %s reached, action %s", name, g_at_action);
    g_at_phase[0] = 0;
    if (!strcmp(g_at_action, "kill")) { fflush(stderr); usleep(500000); _exit(9); }
    if (!strcmp(g_at_action, "failsave")) dm_fs_test_fail_next();
}


static int test_fail_search(void) {
    static int at = -2, seen;
    if (at == -2) { const char *e = getenv("DM_TEST_UNSOCKET_FAILPLACE"); at = e ? atoi(e) : -1; }
    if (at <= 0 || ++seen != at) return 0;
    LOG("unsocket test: free-cell search %d reports no cell as asked", seen);
    return 1;
}
static void test_serialized(void) {
    g_serials++;
    LOG("unsocket test: save serialized (%llu), commits %llu, write open %d, thread %d%s", (unsigned long long)g_serials,
        (unsigned long long)dm_fs_commit_count(), dm_fs_txn_open(), gettid(), g_in_run ? ", INSIDE A RUN" : "");
    if (g_post == 2) { g_post = 3; g_post_commits = dm_fs_commit_count(); if (!g_settle) phase("saving"); }
    else if (g_post == 3) { g_post = 0; g_settle = 0; LOG("unsocket test: another save started before the requested one committed; no committed phase"); }
}
static void save_phases(void) {
    uint64_t n = dm_fs_commit_count(), t = now_ns();
    if (g_post == 1) {
        if (dm_fs_txn_open() || n != g_quiet_n || !g_quiet_since) { g_quiet_n = n; g_quiet_since = t; return; }
        if (t - g_quiet_since >= 2000000000ull) { g_post = 2; LOG("unsocket test: the save mount stood idle 2 s, a save is requested"); dm_request_save(); }
    }
    if (g_post == 3 && n >= g_post_commits + 2 && !dm_fs_txn_open()) {
        g_post = 0;
        if (g_settle) { g_settle = 0; LOG("unsocket test: settled, a save serialized after the request is committed"); } else phase("committed");
    }
}








static uint64_t stat_hash(uint64_t u, int part) {
    uint64_t sl = rd64(u + 0x88);
    if (part && (!sl || !(rd32(sl + 0x1c) >> 31))) return 0;
    uint64_t arr = sl ? rd64(sl + (part ? 0xe8 : 0x30)) : 0, n = sl ? rd64(sl + (part ? 0xf0 : 0x38)) : 0, h = 0;
    for (uint64_t i = 0; arr && i < n && i < 512; i++) {
        uint64_t e = rd64(arr + 8 * i); if ((uint32_t)e >> 16 == ST_DUR) continue;
        uint64_t m = (e ^ 0x9e3779b97f4a7c15ull) * 0xbf58476d1ce4e5b9ull; h += m ^ (m >> 31);
    }
    return h;
}
static int census_line(const char *tag, uint64_t it, const char *place) {
    uint64_t d = item_data(it); if (!d) return 0;
    uint32_t code = code_of(it); char kids[64] = ""; uint32_t kc[MAX_FILL]; int nk = 0;
    for (uint64_t f = first_in(it); f && nk < MAX_FILL; f = next_of(f)) kc[nk++] = code_of(f);
    for (int i = 1; i < nk; i++) for (int j = i; j > 0 && memcmp(&kc[j - 1], &kc[j], 4) > 0; j--) { uint32_t t = kc[j]; kc[j] = kc[j - 1]; kc[j - 1] = t; }
    size_t o = 0; for (int i = 0; i < nk; i++) o += (size_t)snprintf(kids + o, sizeof kids - o, "%s%.3s", i ? "," : "", (const char *)&kc[i]);
    LOG("unsocket census %s: %.4s q%u f%#x l%u s%d [%s] h%016llx d%d/%d at %s t%016llx", tag, (const char *)&code, rd32(d), rd32(d + D_FLAGS) & 0x05400910u, rd32(d + 0x38),
        gstat(it, ST_SOCKETS), kids, (unsigned long long)stat_hash(it, 0), gstat(it, ST_DUR), gstat(it, ST_MAXDUR), place, (unsigned long long)stat_hash(it, 1));
    int n = 1; for (uint64_t f = first_in(it); f && n <= MAX_FILL; f = next_of(f)) n += census_line(tag, f, "socket");
    return n;
}
static void census(uint64_t pl, const char *tag) {
    int n = 0, k = 0; LOG("unsocket census %s begin", tag);
    for (uint64_t it = first_in(pl); it && k < 2048; it = next_of(it), k++) {
        uint64_t d = item_data(it); char place[24];
        snprintf(place, sizeof place, "%u.%u", d ? rd8(d + D_PAGE) : 255, rd32(it + U_MODE));
        n += census_line(tag, it, place);
    }
    LOG("unsocket census %s end %d", tag, n);
}

static void client_look(uint8_t *b, uint32_t id) {
    uint64_t c = ((uint64_t (*)(uint64_t, uint32_t, uint32_t, uint32_t))(uintptr_t)(b + GA_UNIT_FIND))((uint64_t)(uintptr_t)(b + GA_CLIENT_UNIT_TABLE + 4 * 0x400), id & 0x7f, id, 4);
    uint64_t d = c ? item_data(c) : 0;
    LOG("unsocket test: client copy of item %u: %s durability %d of %d, flags %#x", id, c ? "found," : "missing,", c ? gstat(c, ST_DUR) : -1, c ? gstat(c, ST_MAXDUR) : -1, d ? rd32(d + D_FLAGS) : 0);
}
static void test_step(uint8_t *b, uint64_t G, uint64_t pl) {
    (void)G;
    save_phases();
    if (g_look_id && --g_look_wait <= 0) { client_look(b, g_look_id); g_look_id = 0; }
    static const char *f; static int got; if (!got) { got = 1; f = getenv("DM_TEST_UNSOCKET_CENSUS"); }
    static int tick; if (!f || ++tick % 10) return;
    FILE *fp = fopen(f, "r"); if (!fp) return;
    char tag[32] = "x"; if (fscanf(fp, "%31s", tag) != 1) strcpy(tag, "x");
    fclose(fp); unlink(f);
    if (!strcmp(tag, "settle")) { if (!g_post) { g_settle = 1; g_post = 1; g_quiet_since = 0; } return; }
    if (!strcmp(tag, "failnext")) { dm_fs_test_fail_next(); dm_request_save(); return; }
    census(pl, tag);
}
#endif
