

































#include "dm_tipx.h"
#include "game_addrs.h"
#include "dm.h"
#include "hle.h"
#include "dm_i18n.h"
#include "dm_looks.h"
#include "dm_actions.h"
#include "dm_sim_ui.h"
#include "dm_gear_live.h"
#include "dm_cursor.h"
#include "dm_uiscreen.h"
#include "dm_custom.h"
#include "dm_itemcmp_core.h"
#include <stdatomic.h>
#include <math.h>

static uint8_t *mb(void) { static uint8_t *b; if (!b) for (int i = 0; i < g_nmod; i++) if (!strcmp(g_mod[i].name, "main")) b = g_mod[i].base; return b; }

static uint32_t comparison_colours[4][3] = {{79, 194, 56}, {252, 70, 70}, {148, 148, 148}, {102, 102, 255}};
static void load_comparison_colours(void) {
    const char *names[] = {"FontColorPartyGreen", "FontColorRed", "FontColorLightGray", "SkillTree.granted"};
    const char *derived = getenv("DM_DERIVED");
    char path[1200];
    if (!derived || strlen(derived) >= sizeof path) return;
    strcpy(path, derived);
    char *slash = strrchr(path, '/');
    if (!slash || (size_t)(slash - path) + sizeof "/appart/style/style.tsv" > sizeof path) return;
    strcpy(slash, "/appart/style/style.tsv");
    FILE *f = fopen(path, "r");
    if (!f) return;
    char line[512], name[96]; unsigned r, g, b, a;
    while (fgets(line, sizeof line, f)) {
        if (sscanf(line, "colour\t%95[^\t]\t%u\t%u\t%u\t%u", name, &r, &g, &b, &a) != 5 || r > 255 || g > 255 || b > 255 || a != 255) continue;
        for (int i = 0; i < 4; i++) if (!strcmp(name, names[i])) {
            comparison_colours[i][0] = r; comparison_colours[i][1] = g; comparison_colours[i][2] = b;
        }
    }
    fclose(f);
}
static void comparison_colour_obs(const uint64_t *saved) {
    uint64_t *r = (uint64_t *)(uintptr_t)saved;
    const char *code = (const char *)(uintptr_t)r[25];


    dm_tipx_colour_rgb(code, r[26], comparison_colours, r);
}


static struct { int H, W, lang, ok; int line_h, margin, limit, char_w, max_lines; unsigned gen; } M;
static int rec_box(uint8_t *b, const uint64_t *regs, const char *txt, int *w, int *h) {
    static uint8_t rec[0x2e8]; memset(rec, 0, sizeof rec);
    ((void (*)(void *, const char *, uint64_t, uint64_t, uint64_t, const char *))(b + GA_TOOLTIP_RECORD_BUILD))(rec, txt, regs[2], regs[3], regs[4], "");
    const int32_t *q = (const int32_t *)(rec + 0x60); *w = q[2] + q[6]; *h = q[3] + q[7];
    return *(uint64_t *)rec == (uint64_t)(uintptr_t)(rec + 0x18);
}
static int measure(uint8_t *b, const uint64_t *regs) {
    int H = *(int32_t *)(b + GA_UI_HEIGHT), W = *(int32_t *)(b + GA_UI_WIDTH), lang = dm_lang_group();

    if (M.ok && M.H == H && M.W == W && M.lang == lang) return M.ok > 0;
    M.H = H; M.W = W; M.lang = lang; M.ok = -1;
    int w1, h1, w2, h2, w15, h15;
    if (!rec_box(b, regs, "M", &w1, &h1) || !rec_box(b, regs, "M\nM", &w2, &h2) || !rec_box(b, regs, "Leben +35 (500)", &w15, &h15)) { LOG("【红】物品对比：量说明框时串没放在记录里，不出块"); return 0; }


    uint64_t root = *(uint64_t *)(b + GA_UI_ROOT); float sc = root ? ((float (*)(uint64_t))(b + GA_UI_SCALE_GET))(root) : 1.0f;
    int Heff = (root && *(uint8_t *)(root + 0xb9)) || *(float *)(b + GA_PICTURE_TRANSITION) > 0 ? H : *(int32_t *)(b + GA_SCREEN_H);
    int pad = *(int32_t *)(regs[0] + 0xa40); int padpx = (int)(sc * (float)pad);
    M.line_h = h2 - h1; M.margin = h1 - M.line_h; M.limit = Heff - 2 * padpx; M.char_w = (w15 - w1 + 13) / 14;
    M.max_lines = M.line_h > 0 ? (M.limit - M.margin) / M.line_h : 0;
    if (M.line_h < 6 || M.line_h > 300 || M.max_lines < 1 || M.max_lines > 120 || M.char_w < 2 || M.char_w > 200) { LOG("【红】物品对比：量出来的不合理（行高 %d、边距 %d、上限 %d、字宽 %d），不出块", M.line_h, M.margin, M.limit, M.char_w); return 0; }
    { static int tl = -2; if (tl == -2) { const char *e = getenv("DM_TEST_ITEMCMP_LINES"); tl = e ? atoi(e) : -1; }
      if (tl > 0) { LOG("物品对比：测试开关 DM_TEST_ITEMCMP_LINES=%d（量出 %d 行）", tl, M.max_lines); M.max_lines = tl; } }
    M.ok = 1; M.gen++; LOG("物品对比：量好了——界面 %dx%d，行高 %d、边距 %d、上限高 %d（边距 %d × %.3f）→ 最多 %d 行，字宽约 %d（语言组 %d）", W, H, M.line_h, M.margin, M.limit, pad, sc, M.max_lines, M.char_w, lang);
    return 1;
}
static int widest_line(const char *s) {
    int best = 0, cur = 0; const unsigned char *p = (const unsigned char *)s;
    for (size_t i = 0; p[i]; ) { if (p[i] == '\n') { if (cur > best) best = cur; cur = 0; i++; continue; }
        if (p[i] == 0xee && p[i + 1] == 0x81 && p[i + 2] == 0xbe) { i += p[i + 3] ? 4 : 3; continue; }
        i++; while ((p[i] & 0xc0) == 0x80) i++; cur++; }
    return cur > best ? cur : best;
}




static int text_width(uint8_t *b, const uint64_t *regs, const char *s, size_t n, int pad) {
    int width = 0; char piece[16]; size_t k = 0; const unsigned char *p = (const unsigned char *)s;
    for (size_t i = 0; i <= n; ) {
        size_t j = i;
        if (i < n && p[i] == 0xee && i + 3 < n && p[i + 1] == 0x81 && p[i + 2] == 0xbe) { i += 4; continue; }
        if (i < n) { j = i + 1; while (j < n && (p[j] & 0xc0) == 0x80) j++; }
        if (k && (i == n || k + (j - i) > 15)) {
            int w, h; piece[k] = 0;
            if (!rec_box(b, regs, piece, &w, &h)) return -1;
            width += w - pad; k = 0;
        }
        if (i == n) break;
        memcpy(piece + k, s + i, j - i); k += j - i; i = j;
    }
    return width;
}
static int tip_box(uint8_t *b, const uint64_t *regs, const char *text, float box[DM_GEAR_DOTS_BOX]) {
    static uint64_t sig; static int width, pad, head[2]; static unsigned gen;

    int vendor = (dm_ui_panel_state() & DM_PS_VENDOR) != 0;
    int first = dm_tipx_dot_line(text, vendor);
    if (!first && vendor) {
        static int said; if (!said++) LOG("ASSUME itemcmp.vendor_price: the vendor tooltip's top line does not end in a number, no line skipped");
    }


    int fresh = gen != M.gen;
    if (fresh) { int w1, h1, w2, h2; if (!rec_box(b, regs, "MMMMMMM", &w1, &h1) || !rec_box(b, regs, "MMMMMMMMMMMMMM", &w2, &h2)) return 0; pad = 2 * w1 - w2; gen = M.gen; }
    uint64_t h = 1469598103934665603ull; for (const char *c = text; *c; c++) h = (h ^ (uint8_t)*c) * 1099511628211ull;
    h = (h ^ (uint8_t)first) * 1099511628211ull;
    if (fresh || h != sig) {
        width = 0;
        for (const char *line = text; ; ) {
            const char *end = strchr(line, '\n'); size_t n = end ? (size_t)(end - line) : strlen(line);
            int w = text_width(b, regs, line, n, pad); if (w < 0) { width = -1; break; }
            if (w > width) width = w;
            if (!end) break; line = end + 1;
        }


        for (int i = 0; i < 2 && width >= 0; i++) {
            size_t at, n; head[i] = dm_tipx_top_line(text, first + i, &at, &n) ? text_width(b, regs, text + at, n, pad) : 0;
            if (head[i] < 0) width = -1;
        }
        sig = h;
    }




    int tw = (int32_t)(uint32_t)regs[3], th = (int32_t)(regs[3] >> 32), tx = (int32_t)(uint32_t)regs[2], ty = (int32_t)(regs[2] >> 32);
    int W = width + pad, H = M.margin + dm_tipx_text_lines(text) * M.line_h;
    const uint8_t *style = (const uint8_t *)(uintptr_t)regs[4];
    const char *second = (const char *)(uintptr_t)regs[5];
    uint64_t root = *(uint64_t *)(b + GA_UI_ROOT); float sc = root ? ((float (*)(uint64_t))(b + GA_UI_SCALE_GET))(root) : 1.0f;
    int ui = (root && *(uint8_t *)(root + 0xb9)) || *(float *)(b + GA_PICTURE_TRANSITION) > 0;
    dm_tipx_place_in in = { .tx = tx, .ty = ty, .tw = tw, .th = th, .w = W, .h = H, .scale = sc,
        .screen_w = ui ? M.W : *(int32_t *)(b + GA_SCREEN_W), .screen_h = ui ? M.H : *(int32_t *)(b + GA_SCREEN_H) };
    if (style) { in.centre = style[0xd0]; in.above = style[0x138]; memcpy(&in.ox, style + 0x13c, 4); memcpy(&in.oy, style + 0x140, 4); }
    float edge = sc * (float)*(int32_t *)(regs[0] + 0xa40); in.pad = (int)edge;
    int ux, uy, uw, uh, sw = 0, sh = 0; dm_ui_rect(&ux, &uy, &uw, &uh); dm_screen_real(&sw, &sh);
    if (!style || (second && *second) || width <= 0 || W > M.W || H > M.H || tw < 0 || th < 0 || tx < -M.W || tx > 2 * M.W || ty < -M.H || ty > 2 * M.H
        || in.pad < 0 || in.pad > M.H / 4 || in.screen_w != M.W || in.screen_h != M.H || uw <= 0 || uh <= 0 || sw <= 0 || sh <= 0) {
        static int said; if (!said++) LOG("ASSUME bestgear.tip_rect: tooltip box unreadable, no dots (target %d,%d %dx%d, box %dx%d of %dx%d, second text %d)", tx, ty, tw, th, W, H, M.W, M.H, second && *second);
        return 0;
    }
    int left, top; dm_tipx_place(&in, &left, &top);
    float x0 = ux + (float)left * uw / M.W, x1 = ux + (float)(left + W) * uw / M.W, y0 = uy + (float)top * uh / M.H, y1 = uy + (float)(top + H) * uh / M.H;
    box[0] = x0 / sw; box[1] = y0 / sh; box[2] = x1 / sw; box[3] = y1 / sh;


    box[4] = (float)M.line_h * uh / M.H / sh; box[5] = (M.margin / 2.0f + first * M.line_h) * uh / M.H / sh; box[6] = pad / 2.0f * uw / M.W / sw;
    box[7] = (float)head[0] * uw / M.W / sw; box[8] = (float)head[1] * uw / M.W / sw;
    { static uint64_t said; static int n; if (said != sig && n < 20) { said = sig; n++; LOG("Item tooltip box: target %d,%d %dx%d, style centre %d above %d offset %d,%d, edge %d, placed %d,%d %dx%d of %dx%d", tx, ty, tw, th, in.centre, in.above, in.ox, in.oy, in.pad, left, top, W, H, M.W, M.H); } }
    return 1;
}


static int item_version_ok(uint64_t it) { uint8_t v = *(uint8_t *)(it + 0x1bd); return v >= 1 && v <= 3; }


static void hand_panel(uint8_t *b, uint64_t *regs, uint64_t it) {
    if (!dm_custom_get("item_cmp") || *(uint8_t *)(b + GA_PANEL_MERC)) return;
    uint8_t ver = *(uint8_t *)(it + 0x1bd); uint64_t t = ver >= 1 && ver <= 3 ? *(uint64_t *)(b + GA_DATA_TABLES + (uint64_t)ver * 16) : 0; uint32_t ci = *(uint32_t *)(it + 4);
    int l1 = -1, l2 = -1;
    if (t && ci < *(uint32_t *)(t + 0x15c8)) { int16_t ty = *(int16_t *)(*(uint64_t *)(t + 0x15c0) + (uint64_t)ci * 0x1c0 + 0x12e);
        if (ty >= 0 && (uint64_t)ty < *(uint64_t *)(t + 0x1370)) { uint64_t tr = *(uint64_t *)(t + 0x1368) + (uint64_t)ty * 0xe8; l1 = *(uint8_t *)(tr + 0xa); l2 = *(uint8_t *)(tr + 0xb); } }
    if (!((l1 == 4 && l2 == 5) || (l1 == 5 && l2 == 4))) return;
    static int said;
    uint64_t frame = regs[29] - 0x320;
    if (frame != (uint64_t)(uintptr_t)regs + 0x330 || regs[20] != (uint64_t)l1 || regs[21] != (uint64_t)l2) {
        if (!said++) LOG("ASSUME itemcmp.panel_frame: item cell frame not as expected (x29 %#llx, regs %p, body locations %llu/%llu, type %d/%d); boxes left as they are",
                         (unsigned long long)regs[29], (void *)regs, (unsigned long long)regs[20], (unsigned long long)regs[21], l1, l2);
        return;
    }
    uint8_t *loc1 = (uint8_t *)(uintptr_t)(frame + 0x40), *loc2 = (uint8_t *)(uintptr_t)(frame + 0x18);
    if (!dm_tipx_text_object_ok(loc1, frame) || !dm_tipx_text_object_ok(loc2, frame)) {
        if (!said++) LOG("ASSUME itemcmp.panel_frame: comparison box texts not as expected; boxes left as they are");
        return;
    }
    if (!*(uint64_t *)(loc1 + 8) || !*(uint64_t *)(loc2 + 8)) return;
    uint64_t pl = ((uint64_t (*)(uint32_t))(b + GA_CLIENT_PLAYER_GET))(*(uint32_t *)(b + GA_LOCAL_PLAYER_SLOT)); if (!pl) return;
    uint64_t hand[2] = { 0, 0 };
    { uint64_t inv = *(uint64_t *)(pl + 0x90), q = inv && *(uint32_t *)inv == 0x01020304 ? *(uint64_t *)(inv + 0x10) : 0;
      for (int k = 0; q && k < 512; k++) { uint64_t qd = *(uint64_t *)(q + 0x10); if (!qd) break;
          uint8_t loc = *(uint8_t *)(qd + 0x54); if (*(uint32_t *)(q + 0xc) == 1 && (loc == 4 || loc == 5)) hand[loc - 4] = q; q = *(uint64_t *)(qd + 0xb0); } }
    uint64_t d = *(uint64_t *)(it + 0x10), key = 1469598103934665603ull;
    const uint64_t part[6] = { *(uint32_t *)(it + 8), d ? *(uint32_t *)(d + 0x18) : 0, hand[0] ? *(uint32_t *)(hand[0] + 8) : 0, hand[1] ? *(uint32_t *)(hand[1] + 8) : 0,
                               *(uint32_t *)(pl + 8), *(uint32_t *)(pl + 4) };
    for (int i = 0; i < 6; i++) key = (key ^ part[i]) * 1099511628211ull;
    static dm_ic_panel_memo memo; uint64_t fr = dm_guest_frame();
    int hide = dm_ic_panel_memo_get(&memo, key, fr);
    dm_ic_panel_in in = { .loc1 = l1, .loc2 = l2, .pair = -1 };
    if (hide < 0) {
        if (((int (*)(uint64_t, uint64_t, int *, int, int))(b + GA_EQUIP_SLOT_PICK))(pl, it, &in.slot, 1, 0) && (in.slot == 4 || in.slot == 5)) {
            int other = dm_ic_other_hand(in.slot);
            in.other_worn = hand[other - 4] != 0;
            int (*place)(uint64_t, int, uint64_t, int, int) = (int (*)(uint64_t, int, uint64_t, int, int))(b + GA_ITEM_PLACE_CHECK);
            in.code = place(pl, in.slot, it, 0, 1); in.other_code = place(pl, other, it, 0, 1);
            uint64_t kept = hand[in.slot - 4];
            if (!kept) in.pair = 1;
            else if (item_version_ok(it) && item_version_ok(kept)) in.pair = ((int (*)(uint64_t, uint64_t, uint64_t, int, int))(b + GA_ITEM_PAIR_ALLOWED))(pl, it, kept, other, 0xff) != 0;
        } else in.slot = 0;
        hide = dm_ic_panel_hide(&in); dm_ic_panel_memo_set(&memo, key, fr, hide);
        static uint32_t last_id; static int lines;
        if (hide && (uint32_t)part[0] != last_id && lines < 50) { last_id = (uint32_t)part[0]; lines++; LOG("Item tooltip: other hand's comparison box hidden (item %u, auto-equip slot %d code %d, other hand code %d)", (unsigned)part[0], in.slot, in.code, in.other_code); }
    }
    if (hide) *(uint64_t *)((hide == l1 ? loc1 : loc2) + 8) = 0;
}


static struct { uint32_t id; uint64_t key, last_fr, since; int page, pages, pad; } P;
static _Atomic uint64_t g_tap_fr;
static _Atomic int g_pages_pub;
void dm_tipx_tap(void) { atomic_store(&g_tap_fr, dm_guest_frame() | 1ull << 63); }
int dm_tipx_pages(void) { return atomic_load(&g_pages_pub); }
int dm_tipx_pad_page(void) {
    if (dm_guest_frame() > P.last_fr + 10) { P.pages = 0; P.page = 0; }
    if (P.pages > 1 && P.page + 1 < P.pages) { P.page++; LOG("物品对比：手柄翻到第 %d/%d 页", P.page + 1, P.pages); return 1; }
    P.page = 0; return 0;
}



static void look_line(uint8_t *b, uint64_t *regs, uint64_t it) {
    dm_look l; const char *name, *orig = (const char *)(uintptr_t)regs[1];
    if (!dm_looks_of_client(it, &l) || !(name = dm_looks_name(l))) return;
    static char buf[4][16384]; static int k; char line[256];
    snprintf(line, sizeof line, "\xee\x81\xbe" "0");
    snprintf(line + 4, sizeof line - 4, dm_str(b, "dmLooksTip"), name);
    uint64_t d = *(uint64_t *)(it + 0x10); uint32_t q = d ? *(uint32_t *)d : 0, fl = d ? *(uint32_t *)(d + 0x18) : 0;
    int top = (fl & 0x10) && (q >= 5 || (fl & 0x4000000)) ? 2 : 1;
    top += dm_tipx_dot_line(orig, dm_shop_open());
    k = (k + 1) & 3;
    if (dm_tipx_insert_under(orig, top, line, buf[k], sizeof buf[k])) regs[1] = (uint64_t)(uintptr_t)buf[k];
}


static char g_buf[8][16384];
void dm_tipx_obs(uint64_t *regs) {
    uint8_t *b = mb(); if (!b || !regs[1]) return;
    static int colour_ready;
    if (!colour_ready && gate_obs_present(OBS_COMPARE_COLOUR)) {
        load_comparison_colours(); gate_obs_set(OBS_COMPARE_COLOUR, comparison_colour_obs); colour_ready = 1;
    }
    static int depth; if (depth) return;
    uint64_t lro = regs[101] - (uint64_t)(uintptr_t)b; int prod = lro == GA_AT_ITEM_TIP_SUBMIT_RET || lro == GA_AT_ITEM_TIP_SUBMIT2_RET ? DM_TIPX_CELL : lro == GA_AT_EQUIP_TIP_SUBMIT_RET ? DM_TIPX_SLOT : 0;
    if(lro==GA_AT_SKILL_TREE_TOOLTIP_RET) {
        uint64_t container=regs[23],index=(uint32_t)regs[19];
        if(!container)return;
        uint64_t count=*(uint64_t *)(container+0x60),array=*(uint64_t *)(container+0x58);
        if(!array || count>64 || index>=count)return;
        uint64_t widget=((uint64_t *)array)[index],record=widget?*(uint64_t *)(widget+0x568):0;
        const char *line=record?dm_sim_skill_tooltip(*(int32_t *)record):"";
        if(!*line)return;
        static char skill_text[16384],skill_coloured[49152];
        size_t own=strlen(line)+1;


        int length=snprintf(skill_text,sizeof skill_text,"%s\n\xee\x81\xbe" "1%s",line,(const char *)regs[1]);
        if(length<0 || (size_t)length>=sizeof skill_text)return;
        regs[1]=(uint64_t)dm_tipx_render_text(skill_text,own,colour_ready,skill_coloured,sizeof skill_coloured);
        return;
    }
    if (!prod) return;
    uint64_t w = regs[19], vt = w ? *(uint64_t *)w - (uint64_t)(uintptr_t)b : 0; if (vt != (prod == DM_TIPX_CELL ? (uint32_t)GA_VT_ITEM_GRID : (uint32_t)GA_VT_EQUIP_SLOT)) return;
    uint64_t it = ((uint64_t (*)(uint64_t, void *))(b + GA_ITEM_CELL_ITEM))(w, NULL); if (!it || *(uint32_t *)it != 4) return;
    look_line(b, regs, it);
    uint64_t t0 = now_ns();
    if (lro == GA_AT_ITEM_TIP_SUBMIT_RET) hand_panel(b, regs, it);
    depth = 1;
    int dm_padmode_current(void), dm_padmode_busy(void);

    const char *orig = (const char *)(uintptr_t)regs[1];
    int measured = measure(b, regs), mc = 0;
    if (measured) { int W = M.W, cw = M.char_w, lo = (int)(0.30f * W) / cw, hi = (int)(0.45f * W) / cw, ow = widest_line(orig); mc = ow > lo ? ow : lo; if (mc > hi) mc = hi; }
    dm_tipx_ctx c = { .b = b, .widget = w, .item = it, .player = ((uint64_t (*)(uint32_t))(b + GA_CLIENT_PLAYER_GET))(*(uint32_t *)(b + GA_LOCAL_PLAYER_SLOT)), .producer = prod, .pad = dm_padmode_current() || dm_padmode_busy(),
                      .orig = orig, .max_chars = mc };
    static dm_tipx_line ln[DM_TIPX_MAXL]; uint64_t key = 0; int n = c.player && measured ? dm_itemcmp_block(&c, ln, DM_TIPX_MAXL, &key) : 0;
    uint64_t fr = dm_guest_frame(); static uint64_t fr_last; static int idx; if (fr != fr_last) { fr_last = fr; idx = 0; } else idx++;
    if (n <= 0 || idx >= 8) { depth = 0; dm_gear_dots_set(fr, 0, NULL); if (n <= 0) { atomic_store(&g_pages_pub, 0); P.pages = 0; P.page = 0; P.id = 0; } return; }
    uint32_t id = *(uint32_t *)(it + 8);
    if (id != P.id || key != P.key || c.pad != P.pad || fr > P.last_fr + 10) { if (P.page && id == P.id) LOG("物品对比：内容 / 模式变了或说明框隔了一阵，回第 1 页"); P.id = id; P.key = key; P.pad = c.pad; P.page = 0; P.since = fr; }
    P.last_fr = fr;
    { uint64_t tf = atomic_load(&g_tap_fr); if (tf >> 63) { uint64_t at = tf & ~(1ull << 63);
          if (fr >= at + 3) { atomic_store(&g_tap_fr, 0); if (P.since + 3 <= at && P.pages > 1) { P.page = (P.page + 1) % P.pages; LOG("物品对比：在同一件上再点 → 第 %d/%d 页", P.page + 1, P.pages); } } } }

    const dm_tipx_span *ann_span; int ann_n = 0; const char *ann = dm_itemcmp_marks(&ann_span, &ann_n);
    if (ann) orig = ann; else ann_n = 0;
    dm_tipx_cfg cfg = { .max_lines = M.max_lines, .max_chars = mc, .name_lines = 1, .hint = NULL, .props = NULL };
    static char hint[160], props[96]; snprintf(hint, sizeof hint, "\xee\x81\xbe" "5... %s", dm_str(b, c.pad ? "dmCmpHintPad" : "dmCmpHintTouch")); snprintf(props, sizeof props, "\xee\x81\xbe" "5%s", dm_str(b, "dmCmpProps"));
    cfg.hint = hint; cfg.props = props;
    { uint64_t d = *(uint64_t *)(it + 0x10); uint32_t q = d ? *(uint32_t *)d : 0; if (q >= 5) cfg.name_lines = 2; }
    size_t len = 0; int pages = dm_tipx_compose(orig, ln, n, &cfg, P.page, g_buf[idx], sizeof g_buf[idx], &len);
    if (pages <= 0) { static int lg; if (lg++ < 20) LOG("【红】物品对比：合成装不下（%d 行），原文照旧", n); depth = 0; return; }
    if (P.page >= pages) { P.page = 0; pages = dm_tipx_compose(orig, ln, n, &cfg, 0, g_buf[idx], sizeof g_buf[idx], &len); }
    P.pages = pages; atomic_store(&g_pages_pub, pages);
    static char coloured[8][49152];
    size_t own_bytes = 0;
    const char *suffix = P.page == 0 ? orig : NULL;
    if (!suffix) {
        int skip = dm_tipx_text_lines(orig) - cfg.name_lines;
        suffix = orig;
        while (skip-- > 0) { const char *next = strchr(suffix, '\n'); if (!next) break; suffix = next + 1; }
    }
    size_t original_bytes = strlen(suffix);
    if (original_bytes <= len) own_bytes = len - original_bytes;

    dm_tipx_span spans[1 + 48]; int nsp = 0; spans[nsp++] = (dm_tipx_span){ 0, own_bytes };
    for (int i = 0; i < ann_n && nsp < (int)(sizeof spans / sizeof *spans); i++) {
        size_t from = (size_t)(suffix - orig);
        if (ann_span[i].at >= from && original_bytes <= len) spans[nsp++] = (dm_tipx_span){ own_bytes + ann_span[i].at - from, ann_span[i].len };
    }
    regs[1] = (uint64_t)(uintptr_t)dm_tipx_render_spans(g_buf[idx], spans, nsp, colour_ready, coloured[idx], sizeof coloured[idx]);
    { int dm_itemcmp_dots(void), dots = dm_itemcmp_dots(); float box[DM_GEAR_DOTS_BOX] = { 0 };
      if (dots && tip_box(b, regs, (const char *)(uintptr_t)regs[1], box)) {


          int sb = lro == GA_AT_ITEM_TIP_SUBMIT_RET ? dm_tipx_side_boxes(regs[29], regs) : lro == GA_AT_ITEM_TIP_SUBMIT2_RET ? 1 : 0;
          if (sb < 0) { static int said; if (!said++) LOG("ASSUME itemcmp.side_boxes: the comparison texts beside the tooltip are not as expected, the boxes are kept"); sb = 1; }
          box[9] = (float)sb;
          dm_gear_dots_set(fr, dots, box);
      } else dm_gear_dots_set(fr, 0, box); }
    { static uint64_t ns, calls, mx, last; static int stat = -1; if (stat < 0) stat = getenv("DM_ITEMCMP_STAT") != NULL;
      uint64_t dt = now_ns() - t0; ns += dt; calls++; if (dt > mx) mx = dt;
      if (stat && t0 - last > 10000000000ull) { last = t0; LOG("物品对比：耗时 %llu 次，平均 %.1f 微秒，最长 %.1f 微秒", (unsigned long long)calls, calls ? ns / 1e3 / calls : 0, mx / 1e3); } }
    depth = 0;
}
