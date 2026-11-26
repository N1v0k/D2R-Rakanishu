









#include "dm_cursor_item.h"
#include "game_addrs.h"
#include "hle.h"
#include "dm.h"
#include <string.h>

static uint8_t *main_base(void) { static uint8_t *b; if (!b) for (int i = 0; i < g_nmod; i++) if (!strcmp(g_mod[i].name, "main")) b = (uint8_t *)g_mod[i].base; return b; }
static void item_size(uint8_t *b, uint64_t item, int *w, int *h) {
    uint8_t ver = *(uint8_t *)(item + 0x1bd); uint64_t t = ver >= 1 && ver <= 3 ? *(uint64_t *)(b + GA_DATA_TABLES + (uint64_t)ver * 16) : 0; uint32_t ci = *(uint32_t *)(item + 4);
    if (t && ci < *(uint32_t *)(t + 0x15c8)) { uint64_t rec = *(uint64_t *)(t + 0x15c0) + (uint64_t)ci * 0x1c0; *w = *(uint8_t *)(rec + 0x11e); *h = *(uint8_t *)(rec + 0x11f); }
}

int dm_cursor_item(dm_cursor_item_info *out) {
    uint8_t *b = main_base();
    if (!b || !out) return 0;
    memset(out, 0, sizeof *out);
    uint64_t pm = *(uint64_t *)(b + GA_UI_ROOT), fm = pm ? *(uint64_t *)(pm + 0xd0) : 0, item = 0, w = 0, vt = 0; if (!fm) return 0;
    const uint64_t cand[2] = { *(uint64_t *)(fm + 0x190), *(uint64_t *)(fm + 0x178) };
    for (int i = 0; i < 2 && !item; i++) {
        if (!(w = cand[i])) continue;
        vt = *(uint64_t *)w - (uint64_t)(uintptr_t)b;
        if (vt != GA_VT_ITEM_GRID && vt != GA_VT_BELT && vt != GA_VT_EQUIP_SLOT && vt != GA_VT_ASTASH_SLOT) continue;
        item = vt == GA_VT_ASTASH_SLOT ? *(uint64_t *)(w + GF_STACK_CELL_ITEM) : ((uint64_t (*)(uint64_t, void *))(b + GA_ITEM_CELL_ITEM))(w, NULL);
        if (item && *(uint32_t *)item != 4) item = 0;
    }
    if (!item) return 0;
    out->id = *(uint32_t *)(item + 8); out->cls = *(uint32_t *)(item + 4);
    uint32_t code = ((uint32_t (*)(uint64_t))(b + GA_ITEM_CODE))(item);
    for (int i = 0; i < 3; i++) { char c = (char)(code >> (8 * i)); out->code[i] = c == ' ' ? 0 : c; }
    if (vt == GA_VT_BELT) out->container = DM_ITEM_AT_BELT;
    else if (vt == GA_VT_EQUIP_SLOT) out->container = DM_ITEM_AT_EQUIP;
    else if (vt == GA_VT_ASTASH_SLOT) out->container = DM_ITEM_AT_STASH;
    else if (*(uint32_t *)(w + 0x5c8) == 1) out->container = DM_ITEM_AT_SHOP;
    else switch (*(uint8_t *)(w + 0x630)) { case 0: out->container = DM_ITEM_AT_INVENTORY; break; case 3: out->container = DM_ITEM_AT_CUBE; break; case 4: out->container = DM_ITEM_AT_STASH; break; default: out->container = DM_ITEM_AT_TRADE; }
    int (*is_type)(uint64_t, int) = (void *)(b + GA_ITEM_IS_TYPE);
    out->potion = is_type(item, 9) != 0; out->scroll = is_type(item, 0x16) != 0; out->book = is_type(item, 0x12) != 0;


    uint8_t ver = *(uint8_t *)(item + 0x1bd);
    if (ver >= 1 && ver <= 3) {
        uint64_t t = *(uint64_t *)(b + GA_DATA_TABLES + (uint64_t)ver * 16); uint64_t recs = *(uint64_t *)(t + 0x15c0), types = *(uint64_t *)(t + 0x1368);
        if (recs && types && out->cls < *(uint32_t *)(t + 0x15c8)) { int16_t ty = *(int16_t *)(recs + (uint64_t)out->cls * 0x1c0 + 0x12e); if (ty >= 0 && (uint64_t)ty < *(uint64_t *)(t + 0x1370)) out->beltable = *(uint8_t *)(types + (uint64_t)ty * 0xe8 + 0x17) != 0; }
    }
    out->belt_room = ((int (*)(uint64_t))(b + GA_ITEM_CAN_BELT))(item) & 1;
    uint64_t player = ((uint64_t (*)(uint32_t))(b + GA_CLIENT_PLAYER_GET))(*(uint32_t *)(b + GA_LOCAL_PLAYER_SLOT));
    if (player) { uint32_t slot = 0; out->equippable = ((int (*)(uint64_t, uint64_t, uint32_t *, int, int))(b + GA_EQUIP_SLOT_PICK))(player, item, &slot, 1, 0) && ((int (*)(uint64_t, uint32_t, uint64_t, int, int))(b + GA_ITEM_PLACE_CHECK))(player, slot, item, 1, 0); }


    { uint64_t d = *(uint64_t *)(item + 0x10); out->stack = d ? *(uint8_t *)(d + 0x9c) : 0; out->unid = d && !(*(uint32_t *)(d + 0x18) & 0x10); }
    out->gw = out->gh = 1;
    if (out->container != DM_ITEM_AT_EQUIP) { uint64_t pp = *(uint64_t *)(item + 0x38); if (pp) { out->gx = *(int32_t *)(pp + 0x10); out->gy = out->container == DM_ITEM_AT_BELT ? 0 : *(int32_t *)(pp + 0x14); } }
    if (out->container != DM_ITEM_AT_EQUIP && out->container != DM_ITEM_AT_BELT) item_size(b, item, &out->gw, &out->gh);
    return 1;
}



int dm_held_size(int *w, int *h) {
    uint8_t *b = main_base(); if (!b) return 0;
    uint64_t pl = ((uint64_t (*)(uint32_t))(b + GA_CLIENT_PLAYER_GET))(*(uint32_t *)(b + GA_LOCAL_PLAYER_SLOT)), inv = pl ? *(uint64_t *)(pl + 0x90) : 0, held = inv && *(uint32_t *)inv == 0x01020304 ? *(uint64_t *)(inv + 0x40) : 0;
    if (!held || *(uint32_t *)held != 4) return 0;
    int gw = 1, gh = 1; item_size(b, held, &gw, &gh); if (w) *w = gw; if (h) *h = gh; return 1;
}





int dm_held_pending(void) {
    uint8_t *b = main_base(); if (!b) return 0;
    uint64_t pl = ((uint64_t (*)(uint32_t))(b + GA_CLIENT_PLAYER_GET))(*(uint32_t *)(b + GA_LOCAL_PLAYER_SLOT)), inv = pl ? *(uint64_t *)(pl + 0x90) : 0, held = inv && *(uint32_t *)inv == 0x01020304 ? *(uint64_t *)(inv + 0x40) : 0;
    if (!held || *(uint32_t *)held != 4) return 0;
    return ((int (*)(uint64_t))(b + GA_ITEM_IS_PENDING))(held) & 1;
}





int dm_vendor_repair_mode(void) {
    uint8_t *b = main_base(); if (!b || !*(uint8_t *)(b + GA_PANEL_VENDOR)) return 0;
    uint64_t t = *(uint64_t *)(b + GA_UI_STATE_TABLE), bk = t ? *(uint64_t *)(t + 8) : 0; if (!bk) return 0;
    for (uint64_t n = *(uint64_t *)(bk + ((*(uint64_t *)t - 1) & 0x0cfa4cd5c8419dedull) * 8); n; n = *(uint64_t *)n)
        if (*(uint64_t *)(n + 8) == 0x838e27f92c679132ull) return *(uint32_t *)*(uint64_t *)(*(uint64_t *)(n + 0x10) + 0x10) == 3;
    return 0;
}
int dm_item_targeting(void) {
    uint8_t *b = main_base(); if (!b) return 0;
    return *(uint32_t *)(b + GA_CURSOR_STATE) == 5 && *(uint64_t *)(b + GA_CURSOR_ITEM) ? (int)*(uint32_t *)(b + GA_CURSOR_TARGET_TYPE) + 1 : 0;
}
