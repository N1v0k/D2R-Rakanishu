


#include "../dm_vault_ui.c"
#include "../game_addrs.h"
#include <stddef.h>

module_t g_mod[MAX_MOD];
int g_nmod;
static int checks, fails, loads, relayouts;
static uint32_t generation = 1;
uint32_t dm_ui_gen(void) { return generation; }


uint64_t dm_wref_get(dm_wref *r) { return r->gen == generation ? r->w : 0; }
#define CHECK(c, msg) do { checks++; if (!(c)) { fails++; fprintf(stderr, "FAIL: %s\n", msg); } } while (0)

typedef union { uint64_t align; uint8_t bytes[0x100]; } test_widget;
typedef struct {
    test_widget cell, tile, box, image, gold;
    uint64_t children[3], image_child[1];
    int native_w, native_h, loads, fail_loads;
    char loaded_icon[160];
} test_cell;
static test_widget owner;
static uint64_t vtable[4];
static uint64_t addr(const void *p) { return (uint64_t)(uintptr_t)p; }
static int32_t *rect(test_widget *w) { return (int32_t *)(w->bytes + 0x70); }
static float scale(test_cell *c) { return *(float *)(c->image.bytes + 0x80); }
static float opacity(test_cell *c) { return *(float *)(c->image.bytes + 0x94); }
static void relayout(uint64_t widget) { (void)widget; relayouts++; }

static void image_stub(uint64_t widget, void *text) {
    struct { const char *p; uint64_t n, cap; } *s = text;
    test_cell *c = (test_cell *)((uint8_t *)(uintptr_t)widget - offsetof(test_cell, image));
    loads++; c->loads++;
    if (c->fail_loads) { c->fail_loads--; return; }
    if (s->n >= sizeof c->loaded_icon) abort();
    memcpy(c->loaded_icon, s->p, s->n); c->loaded_icon[s->n] = 0;
    rect(&c->image)[2] = c->native_w; rect(&c->image)[3] = c->native_h;
}
static void widget_init(test_widget *w, const char *name, uint64_t parent) {
    memset(w, 0, sizeof *w);
    *(uint64_t *)(w->bytes + 0x00) = addr(vtable);
    *(uint64_t *)(w->bytes + 0x08) = addr(name);
    *(uint64_t *)(w->bytes + 0x30) = parent;
    w->bytes[0x50] = w->bytes[0x51] = 1;
    *(float *)(w->bytes + 0x80) = 1.0f;
}
static void cell_init(test_cell *c, int bw, int bh, int iw, int ih) {
    memset(c, 0, sizeof *c);
    widget_init(&c->cell, "Cell", addr(&owner));
    widget_init(&c->tile, "Tile", addr(&c->cell));
    widget_init(&c->box, "TileBox", addr(&c->cell));
    widget_init(&c->image, "TileImg", addr(&c->box));
    widget_init(&c->gold, "TileGold", addr(&c->cell));
    c->children[0] = addr(&c->tile); c->children[1] = addr(&c->box); c->children[2] = addr(&c->gold);
    *(uint64_t *)(c->cell.bytes + 0x58) = addr(c->children);
    *(uint64_t *)(c->cell.bytes + 0x60) = 3;
    c->image_child[0] = addr(&c->image);
    *(uint64_t *)(c->box.bytes + 0x58) = addr(c->image_child);
    *(uint64_t *)(c->box.bytes + 0x60) = 1;
    rect(&c->box)[2] = bw; rect(&c->box)[3] = bh;
    c->native_w = iw; c->native_h = ih;
}
static void tile(uint8_t *base, test_cell *c, const char *icon, int state, int slot) {
    vt_row r = { .icon = icon };
    (void)slot;
    vu_tile(base, addr(&c->cell), "Tile", &r, state);
}
int main(void) {



    uint8_t *base = (uint8_t *)((uintptr_t)image_stub - GA_IMAGE_SET_PATH);
    g_nmod = 1; strcpy(g_mod[0].name, "main"); g_mod[0].base = base;
    vtable[0x10 / 8] = addr(relayout);
    widget_init(&owner, "TouchVault", 0);
    enum { SLOTS = VPER + VSET_ROWS * VSET_N, BATCHES = 12 };
    test_cell *cells = calloc(SLOTS * BATCHES + 5, sizeof *cells);
    if (!cells) return 2;
    const char *first = "Items/armor/helmet/cap";
    const char *second = "Items/weapon/sword/short_sword";




    for (int batch = 0; batch < BATCHES; batch++) {
        generation++;
        int before = loads;
        for (int slot = 0; slot < SLOTS; slot++) {
            test_cell *c = &cells[batch * SLOTS + slot];
            cell_init(c, 100, 100, 200, 100);
            tile(base, c, first, ST_STORED, slot);
            CHECK(c->loads == 1 && !strcmp(c->loaded_icon, first),
                  "every replacement widget receives its icon");
        }
        CHECK(loads - before == SLOTS, "all 54 slots load in every replacement batch");
    }
    test_cell *live = &cells[(BATCHES - 1) * SLOTS];
    int before = loads;
    tile(base, live, first, ST_SEEN, 0);
    CHECK(loads == before, "unchanged live icon is a cache hit");
    CHECK(scale(live) == 0.5f && rect(&live->image)[0] == 0 && rect(&live->image)[1] == 25,
          "a wide icon is scaled and centered inside its box");
    CHECK(opacity(live) == 0.55f, "seen icon opacity updates without reloading");
    tile(base, live, second, ST_NONE, 0);
    CHECK(loads == before + 1 && !strcmp(live->loaded_icon, second),
          "changing the page reloads a different icon in the same slot");
    CHECK(opacity(live) == 0.3f, "unseen icon keeps its dimmed opacity");



    rect(&live->image)[2] = rect(&live->image)[3] = 0;
    live->loaded_icon[0] = 0; before = loads;
    tile(base, live, second, ST_STORED, 0);
    CHECK(loads == before + 1 && !strcmp(live->loaded_icon, second),
          "same-address empty image reloads the cached icon");
    CHECK(opacity(live) == 1.0f, "stored icon is fully opaque after recreation");



    generation++;
    test_cell *retry = &cells[SLOTS * BATCHES];
    cell_init(retry, 100, 100, 50, 50); retry->fail_loads = 1;
    before = loads; tile(base, retry, first, ST_STORED, 1);
    CHECK(loads == before + 1 && !rect(&retry->image)[2], "fixture models an initially unavailable image");
    tile(base, retry, first, ST_STORED, 1);
    CHECK(loads == before + 2 && !strcmp(retry->loaded_icon, first), "empty image retries after a failed load");
    CHECK(scale(retry) == 1.0f && rect(&retry->image)[0] == 25 && rect(&retry->image)[1] == 25,
          "small icons are centered without enlargement");
    before = loads; tile(base, retry, first, ST_STORED, 1);
    CHECK(loads == before, "a successful retry restores cache hits");
    CHECK(relayouts > 0, "fixture exercises the production relayout path");

    free(cells);
    printf("t_vault_icons: %d checks, %d failures\n", checks, fails);
    return fails != 0;
}
