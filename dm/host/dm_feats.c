









































#include "dm_feats.h"
#include "game_addrs.h"
#include "dm_effects.h"
#include "dm_fightlog.h"
#include "dm_gear_compose.h"
#include "dm_gear_pages.h"
#include "dm_i18n.h"
#include "dm_rift.h"
#include "dm_sim_ui.h"
#include "dm_streak.h"
#include "dm.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

enum { FACTS_EVERY = 25, NAMES_CAP = 96, NAME_CAP = 64 };
static uint8_t *B;
static pthread_mutex_t g_mu = PTHREAD_MUTEX_INITIALIZER;
static int g_hud[DM_FT_HUD_N];
static struct { uint32_t seq; char names[NAMES_CAP]; } g_names[DM_FT_NOTICES];
static int g_class = -1, g_hardcore = -1, g_uniques = -1, g_bosses = -1, g_quests = -1;
static char g_class_name[DM_FT_CLASSES][NAME_CAP], g_diff_name[DM_FT_DIFFS][NAME_CAP];
static char g_row_name[DM_FT_NAME_KEYS][NAME_CAP];

static uint64_t rd(uint64_t a) { return a ? *(volatile uint64_t *)(uintptr_t)a : 0; }
static uint32_t rd32(uint64_t a) { return *(volatile uint32_t *)(uintptr_t)a; }
static int ustat(uint64_t u, int id) { return ((int (*)(uint64_t, int, int))(uintptr_t)(B + GA_STAT_GET))(u, id, 0); }
static const char *game_str(const char *key, int plain) {
    struct { const char *k; uint64_t n; } s = { key, strlen(key) };
    const char *r = ((const char *(*)(void *))(uintptr_t)(B + GA_LOC_STRING_GET))(&s);
    if (!dm_ft_game_text_ok(r, key, plain)) { static int logged; if (logged++ < 3) LOG("ASSUME feats.game_name: no game text for %s", key); return ""; }
    return dm_untag(r);
}

const char *dm_save_workdir(void);
uint64_t dm_fs_commit_count(void);
uint64_t dm_fs_fail_count(void);
int dm_char_hardcore(const char *wd, const char *name);
int dm_kills_board(const char *who, dm_ft_board_row *out, int max);
const dm_gear_tables *dm_gear_tables_shared(void);
int dm_pages_shown(void);
static void stage_victory(void);
static void store_hook(const char *wd) { stage_victory(); dm_ft_store_commit(dm_ft_store_core(), wd, dm_fs_commit_count(), dm_fs_fail_count()); }
__attribute__((constructor)) static void store_hook_install(void) { extern void (*g_dm_pre_commit_hook[DM_PRE_COMMIT_HOOKS])(const char *); g_dm_pre_commit_hook[9] = store_hook; }

int dm_feats_hud(int *out, int max) {
    int n = max < DM_FT_HUD_N ? max : DM_FT_HUD_N; if (n <= 0) return 0;
    pthread_mutex_lock(&g_mu); memcpy(out, g_hud, (size_t)n * sizeof *out); pthread_mutex_unlock(&g_mu);
    return n;
}
int dm_feats_names(uint32_t seq, char *out, size_t cap) {
    if (!out || !cap) return -1;
    pthread_mutex_lock(&g_mu);
    int n = -1;
    if (seq && g_names[seq % DM_FT_NOTICES].seq == seq) n = snprintf(out, cap, "%s", g_names[seq % DM_FT_NOTICES].names);
    pthread_mutex_unlock(&g_mu);
    return n >= (int)cap ? (int)cap - 1 : n;
}

static const char *const k_class_code[DM_FT_CLASSES] = { "ama", "sor", "nec", "pal", "bar", "dru", "ass", "war" };
static const char *const k_diff_key[DM_FT_DIFFS] = { "strCreateGameNormalText", "strCreateGameNightmareText", "strCreateGameHellText" };


static void line_name(char out[NAME_CAP], const char *s) {
    size_t n = strlen(s);
    if (n >= NAME_CAP) { n = NAME_CAP - 1; while (n && ((unsigned char)s[n] & 0xc0) == 0x80) n--; }
    for (size_t i = 0; i < n; i++) out[i] = s[i] == '\t' || s[i] == '\n' || s[i] == '\r' ? ' ' : s[i];
    out[n] = 0;
}

static void page_names(int cls, int hardcore, int uniques, int bosses, int quests) {
    char c[DM_FT_CLASSES][NAME_CAP], d[DM_FT_DIFFS][NAME_CAP], r[DM_FT_NAME_KEYS][NAME_CAP], key[24];
    for (int i = 0; i < DM_FT_CLASSES; i++) { snprintf(key, sizeof key, "partychar%s", k_class_code[i]); line_name(c[i], game_str(key, 0)); }
    for (int i = 0; i < DM_FT_DIFFS; i++) line_name(d[i], game_str(k_diff_key[i], 0));
    for (int i = 0; i < DM_FT_NAME_KEYS; i++) line_name(r[i], game_str(dm_ft_name_keys[i].key, dm_ft_name_keys[i].plain));
    pthread_mutex_lock(&g_mu);
    g_class = cls; g_hardcore = hardcore; g_uniques = uniques; g_bosses = bosses; g_quests = quests;
    memcpy(g_class_name, c, sizeof c); memcpy(g_diff_name, d, sizeof d); memcpy(g_row_name, r, sizeof r);
    pthread_mutex_unlock(&g_mu);
}
int dm_feats_text(char *out, size_t cap) {
    int n = dm_ft_store_text(dm_ft_store_core(), out, cap);
    if (n < 0) return n;
    size_t k = (size_t)n; int w;
    pthread_mutex_lock(&g_mu);
    #define PUT(...) do { w = snprintf(out + k, cap - k, __VA_ARGS__); if (w < 0 || (size_t)w >= cap - k) { pthread_mutex_unlock(&g_mu); out[0] = 0; return -2; } k += (size_t)w; } while (0)
    PUT("hero\t%d\t%d\t%d\t%d\t%d\n", g_class, g_hardcore, g_uniques, g_bosses, g_quests);
    for (int i = 0; i < DM_FT_CLASSES; i++) PUT("class\t%d\t%s\n", i, g_class_name[i]);
    for (int i = 0; i < DM_FT_DIFFS; i++) PUT("difficulty\t%d\t%s\n", i, g_diff_name[i]);
    for (int i = 0; i < DM_FT_NAME_KEYS; i++) PUT("name\t%u\t%s\n", dm_ft_name_keys[i].id, g_row_name[i]);
    #undef PUT
    pthread_mutex_unlock(&g_mu);
    int a = dm_ft_store_all_text(dm_ft_store_core(), dm_save_workdir(), out + k, cap - k);
    if (a < 0) { out[0] = 0; return -2; }
    return (int)(k + (size_t)a);
}

static void post(const dm_ft_note *note, int n) {
    for (int i = 0; i < n; i++) {
        const dm_ft_row *r = dm_ft_row_of(note[i].id);
        char names[NAMES_CAP] = "", key[24] = "";
        if (r && r->rule == DM_FT_R_CLASS_HELL && r->arg < DM_FT_CLASSES) {
            snprintf(key, sizeof key, "partychar%s", k_class_code[r->arg]);
            snprintf(names, sizeof names, "%s\n%s", game_str(key, 0), game_str(k_diff_key[2], 0));
        } else if (r && r->rule == DM_FT_R_DIFF && r->arg < DM_FT_DIFFS) snprintf(names, sizeof names, "\n%s", game_str(k_diff_key[r->arg], 0));
        else if (r && r->rule == DM_FT_R_EVENT && r->arg == DM_FT_E_DRY) snprintf(names, sizeof names, "\n%s", game_str(k_diff_key[1], 0));
        else if (r && dm_ft_name_key_of(r->id) >= 0) { const dm_ft_name_key *k = &dm_ft_name_keys[dm_ft_name_key_of(r->id)]; snprintf(names, sizeof names, "%s\n", game_str(k->key, k->plain)); }
        char quoted[2 * NAMES_CAP]; size_t q = 0;
        for (const char *c = names; *c && q + 3 < sizeof quoted; c++) {
            if (*c == '"' || *c == '\\') quoted[q++] = '\\';
            if (*c == '\n') { quoted[q++] = '\\'; quoted[q++] = 'n'; } else if ((unsigned char)*c >= 0x20) quoted[q++] = *c;
        }
        quoted[q] = 0;
        LOG("[feats] {\"notice\":%u,\"id\":%u,\"arg\":%u,\"names\":\"%s\"}", note[i].seq, note[i].id, note[i].arg, quoted);
        pthread_mutex_lock(&g_mu);
        g_names[note[i].seq % DM_FT_NOTICES].seq = note[i].seq; memcpy(g_names[note[i].seq % DM_FT_NOTICES].names, names, sizeof names);
        int k = (int)(note[i].seq % DM_FT_NOTICES);
        g_hud[DM_FT_HUD_NOTES + 3 * k] = (int)note[i].seq; g_hud[DM_FT_HUD_NOTES + 3 * k + 1] = note[i].id; g_hud[DM_FT_HUD_NOTES + 3 * k + 2] = (int)note[i].arg;
        if ((int)note[i].seq > g_hud[DM_FT_HUD_SEQ]) g_hud[DM_FT_HUD_SEQ] = (int)note[i].seq;
        pthread_mutex_unlock(&g_mu);
    }
}

void dm_feats_event(unsigned events) {
    if (!events) return;
    dm_ft_note note[DM_FT_NOTICES]; int nn = 0;
    LOG("[feats] {\"events\":%u}", events);
    dm_ft_store_event(dm_ft_store_core(), events, (uint32_t)time(NULL), note, &nn); post(note, nn);
}
void dm_feats_stage_count(int counter, uint64_t n) {
    if (!n) return;
    int k = dm_ft_store_stage_count(dm_ft_store_core(), counter, n, (uint32_t)time(NULL), dm_fs_commit_count(), dm_fs_fail_count());
    LOG("[feats] {\"staged_count\":%d,\"n\":%llu,\"rows\":%d}", counter, (unsigned long long)n, k);
}
void dm_feats_stage(unsigned events) {
    if (!events) return;
    int k = dm_ft_store_stage(dm_ft_store_core(), events, (uint32_t)time(NULL), dm_fs_commit_count(), dm_fs_fail_count());
    LOG("[feats] {\"staged\":%u,\"rows\":%d}", events, k);
}

#ifndef DM_PLAYER_BUILD


static void test_seed(void) {
    static int done; if (done) return; done = 1;
    static const struct { const char *name; int counter; } k[] = { { "DM_TEST_FEATS_KILLS", DM_FT_KILLS }, { "DM_TEST_FEATS_GOLD", DM_FT_GOLD } };
    for (int i = 0; i < 2; i++) {
        const char *e = getenv(k[i].name); if (!e || !*e) continue;
        dm_ft_store_seed(dm_ft_store_core(), k[i].counter, strtoull(e, NULL, 10));
        LOG("feats: test switch %s=%s", k[i].name, e);
    }
}
static uint32_t item_code(uint64_t it);






enum { TEST_PILES = 8 };
static _Atomic int g_pick_n;
static uint32_t g_pick[TEST_PILES][3];
static void test_gold(uint64_t G, uint64_t player, uint32_t t) {
    static int n = -1, next; static struct { uint32_t at; int amount, own; } q[4];
    if (n < 0) {
        n = 0; const char *e = getenv("DM_TEST_GOLD_PILE"); char buf[160], *sv;
        if (e) { snprintf(buf, sizeof buf, "%s", e); for (char *k = strtok_r(buf, ",", &sv); k && n < 4; k = strtok_r(NULL, ",", &sv)) if (sscanf(k, "%u:%d", &q[n].at, &q[n].amount) == 2 && q[n].amount > 0) { q[n].own = strstr(k, ":own") != NULL; n++; } }
    }
    if (next >= n || t < q[next].at || atomic_load(&g_pick_n)) return;
    int amount = q[next].amount, own = q[next].own; next++;
    if (own) ((int (*)(uint64_t, uint64_t, uint32_t, int))(uintptr_t)(B + GA_GOLD_DROP_OWN))(G, player, rd32(player + 8), amount);
    else { uint64_t out[32]; int made = 0; ((void (*)(uint64_t, uint64_t, int, uint64_t *, int *, int))(uintptr_t)(B + GA_GOLD_PILE_SPAWN))(G, player, amount, out, &made, 32); }
    int k = 0;
    for (int b = 0; b < 128; b++) for (uint64_t u = rd(G + 0x2e30 + 8 * (uint64_t)b); u && k < TEST_PILES; u = rd(u + 0x158)) {
        if (!dm_ftr_unit(u, 0x1c0) || rd32(u) != 4 || rd32(u + 0xc) != 3 || item_code(u) != 0x20646c67) continue;
        uint64_t path = rd(u + 0x38); if (!path) continue;
        int linked; uint32_t type; dm_ftr_gold_link(u, &linked, &type);
        g_pick[k][0] = rd32(u + 8); g_pick[k][1] = rd32(path + 0x10); g_pick[k][2] = rd32(path + 0x14);
        LOG("[gold-test] pile %u amount %d linked %d type %u at %u,%u (put down %d%s)", g_pick[k][0], ustat(u, 14), linked, type, g_pick[k][1], g_pick[k][2], amount, own ? " own" : "");
        k++;
    }
    atomic_store(&g_pick_n, k);
}


static void test_quests(const dm_ft_facts *x, int first) {
    static uint16_t last[DM_FT_DIFFS][DM_FT_WORDS];
    for (int d = 0; d < DM_FT_DIFFS; d++) {
        if (!first && !memcmp(last[d], x->quests[d], sizeof last[d])) continue;
        char b[DM_FT_WORDS * 6 + 1]; int k = 0;
        for (int w = 0; w < DM_FT_WORDS; w++) k += snprintf(b + k, sizeof b - (size_t)k, "%s%u", w ? "," : "", x->quests[d][w]);
        LOG("[feats] {\"quests\":%d,\"words\":[%s]}", d, b);
        memcpy(last[d], x->quests[d], sizeof last[d]);
    }
}

enum { PICK_GAP = 15 };
void dm_feats_test_pick(uint8_t *b) {
    static int sent, wait;
    if (wait > 0) { wait--; return; }
    int n = atomic_load(&g_pick_n);
    if (sent >= n) { if (n) { atomic_store(&g_pick_n, 0); sent = 0; } return; }
    uint8_t pk[17] = { 0x16 };
    memcpy(pk + 1, &g_pick[sent][0], 4); memcpy(pk + 5, &g_pick[sent][1], 4); memcpy(pk + 9, &g_pick[sent][2], 4);
    ((void (*)(void *, int))(uintptr_t)(b + GA_CLIENT_SEND))(pk, 17);
    LOG("[gold-test] pick up %u at %u,%u", g_pick[sent][0], g_pick[sent][1], g_pick[sent][2]);
    sent++; wait = PICK_GAP;
}
#endif


static uint64_t version_table(uint64_t G) {
    unsigned v = *(volatile uint8_t *)(uintptr_t)(G + 0x106);
    return v >= 1 && v <= 3 ? rd((uint64_t)(uintptr_t)(B + GA_DATA_TABLES + (uint64_t)v * 16)) : 0;
}


enum { FILE_NONE = -1, FILE_UNKNOWN = -2 };
static int file_merc(const char *wd, const char *name) {
    char p[1400]; uint8_t h[0xab];
    if (!wd || snprintf(p, sizeof p, "%s/%s/%s.d2s", wd, name, name) >= (int)sizeof p) return FILE_UNKNOWN;
    FILE *f = fopen(p, "rb"); if (!f) return FILE_UNKNOWN;
    size_t n = fread(h, 1, sizeof h, f); fclose(f);
    uint32_t ver, seed; uint16_t id;
    if (n < sizeof h || memcmp(h, "\x55\xaa\x55\xaa", 4)) return FILE_UNKNOWN;
    memcpy(&ver, h + 4, 4); memcpy(&seed, h + 0xa3, 4); memcpy(&id, h + 0xa9, 2);
    return ver != 0x69 ? FILE_UNKNOWN : seed ? id : FILE_NONE;
}


static int merc_act(uint64_t G, uint64_t player, uint64_t t, int hid) {
    uint64_t m = ((uint64_t (*)(uint64_t, uint64_t, int, int))(uintptr_t)(B + GA_PET_FIND))(G, player, 7, 0);
    if (m && !dm_ftr_unit(m, 0x10)) return -1;
    if (!m) { if (hid == FILE_NONE) return 0; if (hid < 0) return -1; }
    int act = dm_ftr_hireling_act(t, m ? 8 : 4, m ? rd32(m + 4) : (uint32_t)hid);
    return act > 0 ? act : -1;
}
static uint32_t item_code(uint64_t it) { return ((uint32_t (*)(uint64_t))(uintptr_t)(B + GA_ITEM_CODE))(it); }
static uint32_t item_two_hands(uint64_t it) { return ((int (*)(uint64_t))(uintptr_t)(B + GA_ITEM_IS_2HANDED))(it) != 0; }


static dm_ft_pages g_pages;
static void page_item(uint64_t it, void *arg) {
    dm_ft_item e; memset(&e, 0, sizeof e);
    uint64_t d = rd(it + 0x10);
    e.code = item_code(it); e.quality = d ? (uint8_t)rd32(d) : 0; e.row = d ? (uint16_t)rd32(d + 0x34) : 0;
    *(unsigned *)arg |= dm_ft_item_owned(&e);
}
static int page_sig(int p, uint64_t *sig, void *ctx) { (void)ctx; return dm_gear_pages_slot_sig(p, sig); }
static int page_items(int p, unsigned *owned, void *ctx) {
    int r = dm_gear_pages_slot_items(B, *(uint64_t *)ctx, p, page_item, owned);
    if (r) { static int logged; if (!logged++) LOG("ASSUME feats.pages: an item of stash page %d could not be built; its items are not known until it is read whole", p + 1); }
    return r;
}

static uint32_t g_streak_live;
static int g_facts_due;
void dm_feats_streak_end(uint32_t kills) { if (kills > g_streak_live) g_streak_live = kills; g_facts_due = 1; }




static int read_facts(uint64_t G, uint64_t player, const char *name, int hardcore, int hid, dm_ft_facts *x) {
    enum { ITEMS = 512 };
    static dm_ft_item it[ITEMS];
    int state[DM_FT_DIFFS];
    memset(x, 0, sizeof *x);
    if (!dm_ftr_unit(player, 0x98)) return 0;
    x->level = ustat(player, 12); x->cls = (int)rd32(player + 4);
    if (x->cls < 0 || x->cls >= DM_FT_CLASSES) { static int logged; if (!logged++) LOG("ASSUME feats.class: class %d out of range; no achievements from facts", x->cls); return 0; }
    if (x->level < 1 || x->level > 255) return 0;
    if (!dm_sim_quest_copy(name, x->quests, state)) return 0;
    for (int d = 0; d < DM_FT_DIFFS; d++) if (state[d]) { static int logged; if (!logged++) LOG("ASSUME feats.quests: the quest record of difficulty %d is not readable; no achievements from facts in this game", d); return 0; }
    uint64_t t = version_table(G);
    x->waypoints = dm_ftr_waypoint_count(t);
    if (!x->waypoints || !dm_ftr_waypoints(rd(player + 0x10), x->wp)) { static int logged; if (!logged++) LOG("ASSUME feats.waypoints: %d waypoints in the level table or their records not readable; no achievements from facts", x->waypoints); return 0; }
    int n = dm_ftr_items(rd(player + 0x90), it, ITEMS, item_code, item_two_hands);
    if (n < 0) { static int logged; if (!logged++) LOG("ASSUME feats.items: the item list is not readable whole; no achievements from facts"); return 0; }
    const dm_gear_tables *g = dm_gear_tables_shared();
    for (int i = 0; i < n; i++) it[i].set = it[i].quality == DM_FT_Q_SET && g && it[i].row < g->nitem ? g->item[it[i].row].set : -1;
    dm_ft_items(x, it, n, g ? g->pieces : NULL, g ? g->nset : 0);
    if (g) x->known |= DM_FT_K_SETS;
    else { static int logged; if (!logged++) LOG("ASSUME feats.sets: no set table; the set achievement waits"); }
    if (dm_ft_pages_owned(&g_pages, dm_pages_shown(), page_sig, page_items, &G, &x->owned)) x->known |= DM_FT_K_PAGES;
    if (hardcore >= 0) { x->hardcore = hardcore; x->known |= DM_FT_K_HARDCORE; }
    x->merc = merc_act(G, player, t, hid);
    if (x->merc >= 0) x->known |= DM_FT_K_MERC;
    else { static int logged; if (!logged++) LOG("ASSUME feats.merc: the mercenary is not known (character file, hireling table); the mercenary achievements wait"); x->merc = 0; }
    dm_sk_hist *h = dm_sk_hist_core(); dm_sk_rec rec;
    if (dm_sk_hist_loaded(h, name)) {
        x->known |= DM_FT_K_STREAKS;
        for (int d = 0; d < DM_SK_DIFFS; d++) if (dm_sk_hist_record(h, d, &rec) && (int)rec.kills > x->streak) x->streak = (int)rec.kills;
    }
    if ((int)g_streak_live > x->streak) x->streak = (int)g_streak_live;
    static dm_ft_board_row board[DM_FT_BOARD_ROWS];
    int nb = dm_kills_board(name, board, DM_FT_BOARD_ROWS);
    if (nb >= 0) { dm_ft_board(x, board, nb); x->known |= DM_FT_K_KILLS; }
    static dm_rift_stats_t rift;
    if (dm_rift_stats_of(name, &rift)) { dm_ft_rift(x, rift.best, rift.fast); x->known |= DM_FT_K_RIFT; }
    uint32_t states[DM_EFFECTS_STATE_WORDS];
    if (dm_effects_state_bits_of(G, states)) x->shrines = dm_ft_shrines(states, DM_EFFECTS_STATE_WORDS);
    return 1;
}



static pthread_mutex_t g_cmd_mu = PTHREAD_MUTEX_INITIALIZER;
static dm_ft_cmds g_cmds;
typedef struct { uint64_t player, inv; } cmd_ctx;
static int cmd_item(uint32_t id, uint32_t *code, void *ctx) {
    uint64_t it; int r = dm_ftr_find(((cmd_ctx *)ctx)->inv, id, &it);
    if (r > 0) *code = item_code(it);
    return r;
}
static int cmd_target(uint32_t target, uint32_t item, int *placed, int *runeword, void *ctx) {
    uint64_t it; int r = dm_ftr_find(((cmd_ctx *)ctx)->inv, target, &it);
    if (r <= 0) return r;
    if ((r = dm_ftr_socket(it, item, runeword)) < 0) return -1;
    *placed = r;
    return 1;
}
static int cmd_cube(dm_ft_cube_item *out, int max, void *ctx) { return dm_ftr_cube(((cmd_ctx *)ctx)->inv, out, max, item_code); }
static int cmd_top(uint32_t *id, void *ctx) { return dm_ftr_top(((cmd_ctx *)ctx)->inv, id); }
static int cmd_gold(uint64_t *g, void *ctx) {
    int a = ustat(((cmd_ctx *)ctx)->player, 14), b = ustat(((cmd_ctx *)ctx)->player, 15);
    if (a < 0 || b < 0) return -1;
    *g = (uint64_t)a + (uint64_t)b;
    return 1;
}
static const dm_ft_reader k_reader = { cmd_item, cmd_target, cmd_cube, cmd_top, cmd_gold };

static uint64_t g_hero, g_game;
static dm_ft_own_gold g_own;
static dm_ft_duel g_duel;
static unsigned g_fl_ends, g_fl_wins;


static pthread_mutex_t g_won_mu = PTHREAD_MUTEX_INITIALIZER;
static unsigned g_won_ev; static char g_won_who[64];
static void won_set(unsigned ev, const char *who) {
    pthread_mutex_lock(&g_won_mu); g_won_ev = ev; snprintf(g_won_who, sizeof g_won_who, "%s", who ? who : ""); pthread_mutex_unlock(&g_won_mu);
}
static void stage_victory(void) {
    char who[64]; pthread_mutex_lock(&g_won_mu); unsigned ev = g_won_ev; memcpy(who, g_won_who, sizeof who); pthread_mutex_unlock(&g_won_mu);
    if (ev && dm_ft_store_owned_by(dm_ft_store_core(), who)) dm_feats_stage(ev);
}
static void gold_obs(const uint64_t *regs) {
    if (!g_hero || regs[19] != g_hero || !g_game) return;
    int linked; uint32_t type;
    if (dm_ftr_gold_link(regs[20], &linked, &type) < 0) { static int logged; if (!logged++) LOG("ASSUME feats.gold: a pile picked up is not readable as an item; its gold is not counted"); return; }
    uint32_t pile = rd32(regs[20] + 8), last = rd32(g_game + 0x154);
    int32_t added = (int32_t)(uint32_t)regs[23], rest = (int32_t)(uint32_t)regs[22];
    uint64_t n = dm_ft_gold_pickup(&g_own, pile, added, rest, linked, type, last == 0xfffffffeu ? 1 : last + 1);
    LOG("[feats] {\"gold\":%d,\"linked\":%d,\"type\":%u,\"rest\":%d,\"counted\":%llu}", added, linked, type, rest, (unsigned long long)n);
    if (!n) return;
    dm_ft_note note[DM_FT_NOTICES]; int nn = 0;
    dm_ft_store_count(dm_ft_store_core(), DM_FT_GOLD, n, (uint32_t)time(NULL), note, &nn); post(note, nn);
}
static void gamble_obs(const uint64_t *regs) {
    if (!g_hero || regs[19] != g_hero || !g_game) return;
    uint32_t id; int q;
    if (dm_ftr_quality(regs[24], &id, &q) < 0) { static int logged; if (!logged++) LOG("ASSUME feats.gamble: a gambled item is not readable; it is not judged"); return; }
    LOG("[feats] {\"gamble\":%u,\"quality\":%d}", id, q);
    pthread_mutex_lock(&g_cmd_mu);
    int r = dm_ft_gamble_seen(&g_cmds, id, q, rd32(g_game + 0x170));
    pthread_mutex_unlock(&g_cmd_mu);
    if (r < 0) { static int logged; if (!logged++) LOG("ASSUME feats.commands: a gamble not taken (%d waiting); it is not judged", DM_FT_CMDS); }
}

void dm_feats_c2s(uint64_t G, uint64_t player, const uint8_t *pkt, uint32_t n) {
    if (!B || !G || !n || !dm_ftr_unit(player, 0x98)) return;
    cmd_ctx ctx = { player, rd(player + 0x90) };
    if (n >= 14 && pkt[0] == 0x26) {
        uint32_t id = (uint32_t)pkt[10] | (uint32_t)pkt[11] << 8 | (uint32_t)pkt[12] << 16 | (uint32_t)pkt[13] << 24, code = 0;
        if (cmd_item(id, &code, &ctx) <= 0 || dm_ft_potion(code)) dm_ft_duel_potion(&g_duel);
    }
    pthread_mutex_lock(&g_cmd_mu);
    int r = dm_ft_cmd_seen(&g_cmds, pkt, n, rd32(G + 0x170), &k_reader, &ctx);
    pthread_mutex_unlock(&g_cmd_mu);
    if (r < 0) { static int logged; if (!logged++) LOG("ASSUME feats.commands: command %#x not taken (items not readable or %d waiting); its achievement is not judged", pkt[0], DM_FT_CMDS); }
}

static void judge_cmds(dm_ft_store *s, uint64_t player, uint32_t t, uint32_t now) {
    dm_ft_note note[DM_FT_NOTICES]; int nn = 0, sold = 0, unread = 0; unsigned ev = 0;
    cmd_ctx ctx = { player, rd(player + 0x90) };
    pthread_mutex_lock(&g_cmd_mu);
    if (g_cmds.n) ev = dm_ft_cmd_judge(&g_cmds, t, &k_reader, &ctx, &sold, &unread);
    pthread_mutex_unlock(&g_cmd_mu);
    if (unread) { static int logged; if (!logged++) LOG("ASSUME feats.commands: %d command(s) dropped, their items not readable after", unread); }
    if (ev) { LOG("[feats] {\"events\":%u}", ev); dm_ft_store_event(s, ev, now, note, &nn); post(note, nn); }
    if (sold) { dm_ft_store_count(s, DM_FT_SALES, (uint64_t)sold, now, note, &nn); post(note, nn); }
}



static void duel_tick(uint64_t player, uint32_t t, uint32_t now, const char *who) {
    dm_fl_state *fl = dm_fl_core();
    if (!dm_fl_ticked(fl, t)) return;
    dm_ft_duel_life(&g_duel, ustat(player, 6), ustat(player, 7));
    if (dm_fl_player_below_half(fl, t)) dm_ft_duel_below_half(&g_duel);
    const dm_fl_fatal *fa; int nf = dm_fl_fatals(fl, t, &fa);
    for (int i = 0; i < nf; i++) dm_ft_duel_fatal(&g_duel, fa[i].cls, fa[i].plife, fa[i].pmax);
    unsigned wins = dm_fl_wins(fl), ends = dm_fl_ends(fl);
    if (wins != g_fl_wins) { g_fl_wins = wins; dm_ft_duel_won(&g_duel); won_set(dm_ft_duel_judge(&g_duel, dm_fl_won(fl)), who); }
    if (ends != g_fl_ends) {
        g_fl_ends = ends; won_set(0, "");
        unsigned ev = dm_ft_duel_end(&g_duel, dm_fl_last(fl));
        if (ev) { dm_ft_note note[DM_FT_NOTICES]; int nn = 0; LOG("[feats] {\"events\":%u,\"fight\":%u}", ev, dm_fl_last(fl)->h.end);
                  dm_ft_store_event(dm_ft_store_core(), ev, now, note, &nn); post(note, nn); }
    }
    if (!dm_fl_open(fl)) dm_ft_duel_idle(&g_duel);
}

void dm_feats_tick(uint64_t G, uint64_t player, uint8_t *main_base) {
    if (!G || !player || !main_base) return;
    B = main_base;
    static uint64_t game; static uint32_t last_t, facts_at; static int logged_game, logged_known, hardcore, hid, hooked; static char name[64];
    if (!hooked) {
        hooked = 1;
        if (gate_obs_present(OBS_GOLD_PICKUP)) gate_obs_set(OBS_GOLD_PICKUP, gold_obs); else LOG("ASSUME feats.gold_gate: the pack has no gold pickup gate; gold picked up is not counted");
        if (gate_obs_present(OBS_GAMBLE_ITEM)) gate_obs_set(OBS_GAMBLE_ITEM, gamble_obs); else LOG("ASSUME feats.gamble_gate: the pack has no gamble gate; gambling is not judged");
    }
    g_hero = player; g_game = G;
    dm_ft_store *s = dm_ft_store_core();
    dm_ft_note note[DM_FT_NOTICES]; int nn = 0;
    uint32_t t = rd32(G + 0x170), now = (uint32_t)time(NULL);
    if (G != game || t < last_t) {
        game = G; g_facts_due = 1; logged_game = logged_known = 0;
        memset(&g_duel, 0, sizeof g_duel); g_fl_ends = dm_fl_ends(dm_fl_core()); g_fl_wins = dm_fl_wins(dm_fl_core()); g_streak_live = 0;
        won_set(0, "");
        uint64_t pd = rd(player + 0x10); memset(name, 0, sizeof name);
        if (pd) memcpy(name, (const void *)(uintptr_t)pd, sizeof name - 1);
        dm_ft_store_follow(s, dm_save_workdir(), name);
        const char *wd = dm_save_workdir();
        hardcore = wd ? dm_char_hardcore(wd, name) : -1; hid = file_merc(wd, name);
        memset(&g_pages, 0, sizeof g_pages); memset(&g_own, 0, sizeof g_own);
        pthread_mutex_lock(&g_cmd_mu); g_cmds.n = 0; pthread_mutex_unlock(&g_cmd_mu);
        if (hardcore < 0) LOG("ASSUME feats.hardcore: the character file's status is not readable; the hardcore achievements wait");
#ifndef DM_PLAYER_BUILD
        test_seed();
#endif
    }
    last_t = t;
#ifndef DM_PLAYER_BUILD
    test_gold(G, player, t);
#endif
    { int32_t gold; uint32_t id = dm_ft_own_gold_pending(&g_own, &gold); int found = 0;
      if (id) for (uint64_t u = rd(G + 0x2e30 + 8 * (uint64_t)(id & 0x7f)); u && !found; u = rd(u + 0x158)) {
          if (!dm_ftr_unit(u, 0x1c0)) break;
          found = rd32(u + 8) == id && rd32(u) == 4 && rd32(u + 0xc) == 3 && item_code(u) == 0x20646c67 && ustat(u, 14) == gold; }
      if (id) { if (!found) LOG("ASSUME feats.gold_rest: the rest %d of the hero's own gold is not pile %u; it counts when picked up", gold, id); dm_ft_own_gold_confirm(&g_own, found); } }
    const uint32_t *ids; int k = dm_fl_kills(dm_fl_core(), t, &ids);
    if (!dm_fl_ticked(dm_fl_core(), t)) { static int logged; if (!logged++) LOG("ASSUME feats.kills: the fight log did not follow tick %u; its kills are not counted", t); }
    if (k > 0) { dm_ft_store_count(s, DM_FT_KILLS, (uint64_t)k, now, note, &nn); post(note, nn); }
    if (dm_ftr_unit(player, 0x98)) { judge_cmds(s, player, t, now); duel_tick(player, t, now, name); }
    if (dm_ft_store_staged(s, dm_fs_commit_count(), dm_fs_fail_count(), note, &nn)) post(note, nn);
    if (g_facts_due || t - facts_at >= FACTS_EVERY) {
        dm_ft_facts x;
        g_facts_due = 0; facts_at = t;
        int ok = read_facts(G, player, name, hardcore, hid, &x);
        int uniques = ok ? __builtin_popcount(x.uniques & ((1u << DM_FT_SLOTS) - 1)) : -1, quests = -1;
        for (int d = 0; ok && d < DM_FT_DIFFS; d++) { int q = dm_ft_quests(x.quests[d]); if (q > quests) quests = q; }
        page_names(x.cls >= 0 && x.cls < DM_FT_CLASSES ? x.cls : -1, hardcore, uniques, ok && (x.known & DM_FT_K_KILLS) ? x.bosses : -1, quests);
        if (ok) {
#ifndef DM_PLAYER_BUILD
            test_quests(&x, !logged_game);
#endif
            if (!logged_game) { logged_game = 1; LOG("[feats] {\"class\":%d,\"level\":%d,\"kills\":%llu,\"mode\":%d,\"hardcore\":%d,\"waypoints\":%d,\"merc\":%d,\"uniques\":%d,\"owned\":%u,\"set\":%d,\"streak\":%d,\"bosses\":%d,\"boss_fast\":%d,\"rift\":[%d,%d,%d],\"shrines\":%d,\"known\":%u}",
                x.cls, x.level, (unsigned long long)dm_ft_store_counter(s, DM_FT_KILLS), dm_ft_store_mode(s), x.hardcore, x.waypoints, x.merc, uniques, x.owned, x.full_set,
                x.streak, x.bosses, x.boss_fast, x.rift_best, x.rift_timed, x.rift_fast, x.shrines, x.known); }
            if (x.known == DM_FT_K_ALL && !logged_known) { logged_known = 1; LOG("[feats] {\"known\":%u,\"owned\":%u,\"bosses\":%d}", x.known, x.owned, x.bosses); }
            dm_ft_store_facts(s, &x, now, note, &nn); post(note, nn);
        }
    }
}
