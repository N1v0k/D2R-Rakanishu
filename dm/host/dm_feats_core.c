

#include "dm_feats.h"
#include "dm_rift.h"
#include <stdio.h>
#include <string.h>





const dm_ft_row dm_ft_rows[] = {
    { 1, DM_FT_R_KILLS, DM_FT_G_COMBAT, 1, 500 },
    { 2, DM_FT_R_KILLS, DM_FT_G_COMBAT, 1, 5000 },
    { 3, DM_FT_R_KILLS, DM_FT_G_COMBAT, 1, 10000 },
    { 44, DM_FT_R_STREAK, DM_FT_G_HUNT, 5, 20 },
    { 45, DM_FT_R_STREAK, DM_FT_G_HUNT, 5, 65 },
    { 46, DM_FT_R_STREAK, DM_FT_G_HUNT, 5, 250 },
    { 47, DM_FT_R_BOSSES, DM_FT_G_HUNT, 5, DM_FT_BOARD_ROWS },
    { 48, DM_FT_R_BOSS_FAST, DM_FT_G_HUNT, 5, DM_FT_FAST_TICKS },
    { 49, DM_FT_R_EVENT, DM_FT_G_HUNT, 5, DM_FT_E_CALM },
    { 50, DM_FT_R_EVENT, DM_FT_G_HUNT, 5, DM_FT_E_DRY },
    { 51, DM_FT_R_EVENT, DM_FT_G_HUNT, 5, DM_FT_E_ESCAPE },
    { 4, DM_FT_R_ACT, DM_FT_G_CAMPAIGN, 1, 1 },
    { 5, DM_FT_R_ACT, DM_FT_G_CAMPAIGN, 1, 2 },
    { 6, DM_FT_R_ACT, DM_FT_G_CAMPAIGN, 1, 3 },
    { 7, DM_FT_R_ACT, DM_FT_G_CAMPAIGN, 1, 4 },
    { 8, DM_FT_R_ACT, DM_FT_G_CAMPAIGN, 1, 5 },
    { 9, DM_FT_R_DIFF, DM_FT_G_CAMPAIGN, 1, 1 },
    { 10, DM_FT_R_DIFF, DM_FT_G_CAMPAIGN, 1, 2 },
    { 11, DM_FT_R_QUESTS, DM_FT_G_CAMPAIGN, 6, DM_FT_QUESTS },
    { 12, DM_FT_R_WAYPOINTS, DM_FT_G_CAMPAIGN, 2, 0 },
    { 13, DM_FT_R_CLASS_HELL, DM_FT_G_MASTERY, 1, 0 },
    { 14, DM_FT_R_CLASS_HELL, DM_FT_G_MASTERY, 1, 1 },
    { 15, DM_FT_R_CLASS_HELL, DM_FT_G_MASTERY, 1, 2 },
    { 16, DM_FT_R_CLASS_HELL, DM_FT_G_MASTERY, 1, 3 },
    { 17, DM_FT_R_CLASS_HELL, DM_FT_G_MASTERY, 1, 4 },
    { 18, DM_FT_R_CLASS_HELL, DM_FT_G_MASTERY, 1, 5 },
    { 19, DM_FT_R_CLASS_HELL, DM_FT_G_MASTERY, 1, 6 },
    { 43, DM_FT_R_CLASS_HELL, DM_FT_G_MASTERY, 1, 7 },
    { 20, DM_FT_R_EVERY_CLASS, DM_FT_G_MASTERY, 1, DM_FT_CLASSES },
    { 21, DM_FT_R_LEVEL, DM_FT_G_MASTERY, 1, 99 },
    { 23, DM_FT_R_HC_GAME, DM_FT_G_MASTERY, 2, 0 },
    { 22, DM_FT_R_HC_LEVEL, DM_FT_G_MASTERY, 2, 99 },
    { 52, DM_FT_R_RIFT_CLEAR, DM_FT_G_RIFTS, 5, 1 },
    { 53, DM_FT_R_RIFT_TIER, DM_FT_G_RIFTS, 5, 10 },
    { 54, DM_FT_R_RIFT_TIER, DM_FT_G_RIFTS, 5, 30 },
    { 55, DM_FT_R_RIFT_FAST, DM_FT_G_RIFTS, 5, 5 * 60 },
    { 56, DM_FT_R_EVENT, DM_FT_G_RIFTS, 5, DM_FT_E_RUSH },
    { 24, DM_FT_R_EVENT, DM_FT_G_CRAFT, 3, DM_FT_E_GEM },
    { 25, DM_FT_R_EVENT, DM_FT_G_CRAFT, 3, DM_FT_E_RUNEWORD },
    { 26, DM_FT_R_EVENT, DM_FT_G_CRAFT, 3, DM_FT_E_RUNE_UP },
    { 27, DM_FT_R_EVENT, DM_FT_G_CRAFT, 3, DM_FT_E_PERFECT },
    { 28, DM_FT_R_EVENT, DM_FT_G_CRAFT, 3, DM_FT_E_PRISM },
    { 29, DM_FT_R_EVENT, DM_FT_G_CRAFT, 3, DM_FT_E_RECHARGE },
    { 30, DM_FT_R_EVENT, DM_FT_G_CRAFT, 3, DM_FT_E_UNSOCKET },
    { 57, DM_FT_R_SHRINES, DM_FT_G_CRAFT, 5, 4 },
    { 58, DM_FT_R_EVENT, DM_FT_G_CRAFT, 5, DM_FT_E_EQUIP },
    { 59, DM_FT_R_LOOKS, DM_FT_G_CRAFT, 5, 5 },
    { 60, DM_FT_R_DYES, DM_FT_G_CRAFT, 5, 5 },
    { 31, DM_FT_R_OWN, DM_FT_G_TREASURE, 2, DM_FT_OWN_SOJ },
    { 32, DM_FT_R_OWN, DM_FT_G_TREASURE, 2, DM_FT_OWN_STD },
    { 33, DM_FT_R_UNIQUES, DM_FT_G_TREASURE, 2, DM_FT_SLOTS },
    { 34, DM_FT_R_FULL_SET, DM_FT_G_TREASURE, 2, 0 },
    { 35, DM_FT_R_SALES, DM_FT_G_TREASURE, 3, 1000 },
    { 36, DM_FT_R_GOLD, DM_FT_G_TREASURE, 4, 500000 },
    { 37, DM_FT_R_GOLD, DM_FT_G_TREASURE, 4, 1000000 },
    { 38, DM_FT_R_EVENT, DM_FT_G_TREASURE, 4, DM_FT_E_GAMBLE },
    { 39, DM_FT_R_MERC, DM_FT_G_COMPANIONS, 2, 1 },
    { 40, DM_FT_R_MERC, DM_FT_G_COMPANIONS, 2, 2 },
    { 41, DM_FT_R_MERC, DM_FT_G_COMPANIONS, 2, 3 },
    { 42, DM_FT_R_MERC, DM_FT_G_COMPANIONS, 2, 5 },
};
const int dm_ft_nrows = (int)(sizeof dm_ft_rows / sizeof *dm_ft_rows);
_Static_assert(sizeof dm_ft_rows / sizeof *dm_ft_rows <= DM_FT_ROWS_MAX, "the sum has a place for every row");
const dm_ft_name_key dm_ft_name_keys[DM_FT_NAME_KEYS] = {
    { 31, 1, "The Stone of Jordan" }, { 32, 0, "std" }, { 39, 0, "RogueScout" }, { 40, 0, "DesertMerc" }, { 41, 1, "Iron Wolf" }, { 42, 1, "Barbarian" },
};
int dm_ft_name_key_of(int id) { for (int i = 0; i < DM_FT_NAME_KEYS; i++) if (dm_ft_name_keys[i].id == id) return i; return -1; }
int dm_ft_game_text_ok(const char *text, const char *key, int plain) { return text && *text && (plain || strcmp(text, key)); }
const dm_ft_row *dm_ft_row_of(int id) {
    for (int i = 0; i < dm_ft_nrows; i++) if (dm_ft_rows[i].id == id) return &dm_ft_rows[i];
    return NULL;
}


static const int k_act_word[5] = { 6, 14, 22, 26, 40 };
static int act_done(const dm_ft_facts *x, int difficulty, int act) { return x->quests[difficulty][k_act_word[act - 1]] & 1; }


static const uint8_t k_quest_word[] = { 1, 2, 3, 4, 5, 6, 9, 10, 11, 12, 13, 14, 17, 18, 19, 20, 21, 22, 25, 26, 27, 35, 36, 37, 38, 39, 40 };
_Static_assert(sizeof k_quest_word == DM_FT_QUESTS, "every quest word once");
int dm_ft_quests(const uint16_t words[DM_FT_WORDS]) {
    int n = 0;
    for (int i = 0; i < DM_FT_QUESTS; i++) n += words[k_quest_word[i]] & 1;
    return n;
}
static int all_quests(const dm_ft_facts *x) {
    for (int d = 0; d < DM_FT_DIFFS; d++) if (dm_ft_quests(x->quests[d]) == DM_FT_QUESTS) return 1;
    return 0;
}

int dm_ft_account_wide(const dm_ft_row *r) { return r->rule == DM_FT_R_EVERY_CLASS; }

static unsigned needs(const dm_ft_row *r) {
    switch (r->rule) {
    case DM_FT_R_HC_LEVEL: case DM_FT_R_HC_GAME: return DM_FT_K_HARDCORE;
    case DM_FT_R_MERC: return DM_FT_K_MERC;
    case DM_FT_R_FULL_SET: return DM_FT_K_SETS;
    case DM_FT_R_OWN: return DM_FT_K_PAGES;
    case DM_FT_R_STREAK: return DM_FT_K_STREAKS;
    case DM_FT_R_BOSSES: case DM_FT_R_BOSS_FAST: return DM_FT_K_KILLS;
    case DM_FT_R_RIFT_CLEAR: case DM_FT_R_RIFT_TIER: case DM_FT_R_RIFT_FAST: return DM_FT_K_RIFT;
    }
    return 0;
}


static int all_waypoints(const dm_ft_facts *x) {
    if (x->waypoints < 1 || x->waypoints > 8 * DM_FT_WP_BYTES) return 0;
    for (int d = 0; d < DM_FT_DIFFS; d++) {
        int all = 1;
        for (int w = 0; w < x->waypoints && all; w++) all = (x->wp[d][w / 8] >> (w % 8)) & 1;
        if (all) return 1;
    }
    return 0;
}
static int bits(unsigned v) { int n = 0; for (; v; v &= v - 1) n++; return n; }

unsigned dm_ft_item_owned(const dm_ft_item *t) {
    return (t->quality == DM_FT_Q_UNIQUE && t->row == DM_FT_SOJ_ROW ? DM_FT_OWN_SOJ : 0) | (!memcmp(&t->code, "std", 3) ? DM_FT_OWN_STD : 0);
}
void dm_ft_items(dm_ft_facts *x, const dm_ft_item *it, int n, const int16_t *pieces, int nset) {
    enum { RIGHT = 1u << 3, LEFT = 1u << 4 };
    unsigned unique = 0, two = 0, worn_any = 0;
    int16_t rows[DM_FT_SLOTS], sets[DM_FT_SLOTS]; int worn = 0;
    x->owned = 0; x->full_set = 0;
    for (int i = 0; i < n; i++) {
        const dm_ft_item *t = &it[i];
        x->owned |= dm_ft_item_owned(t);
        if (t->slot < 1 || t->slot > DM_FT_SLOTS) continue;
        unsigned bit = 1u << (t->slot - 1);
        worn_any |= bit;
        if (t->quality == DM_FT_Q_UNIQUE) { unique |= bit; if (t->two_hands) two |= bit; }
        if (t->quality == DM_FT_Q_SET && t->set >= 0 && t->set < nset && worn < DM_FT_SLOTS) {
            int dup = 0;
            for (int k = 0; k < worn; k++) dup |= rows[k] == (int16_t)t->row;
            if (!dup) { rows[worn] = (int16_t)t->row; sets[worn++] = t->set; }
        }
    }
    if ((two & RIGHT) && !(worn_any & LEFT)) unique |= LEFT;
    if ((two & LEFT) && !(worn_any & RIGHT)) unique |= RIGHT;
    x->uniques = unique;
    for (int k = 0; k < worn && !x->full_set; k++) {
        int have = 0;
        for (int j = 0; j < worn; j++) have += sets[j] == sets[k];
        x->full_set = pieces[sets[k]] > 0 && have >= pieces[sets[k]];
    }
}

int dm_ft_pages_owned(dm_ft_pages *s, int shown, dm_ft_page_sig_fn sig, dm_ft_page_items_fn items, void *ctx, unsigned *owned) {
    int all = shown >= 0, built = 0, start = s->next;
    for (int i = 0; i < DM_FT_PAGES && shown >= 0; i++) {
        int p = (start + i) % DM_FT_PAGES;
        uint64_t now;
        if (p == shown) continue;
        if (sig(p, &now, ctx)) { all = 0; continue; }
        if (!s->page[p].read || s->page[p].sig != now) {
            if (built) { all = 0; continue; }
            built = 1; s->next = (p + 1) % DM_FT_PAGES;
            unsigned o = 0; int ok = !items(p, &o, ctx);
            s->page[p].sig = now; s->page[p].read = ok; s->page[p].owned = ok ? o : 0;
        }
        if (!s->page[p].read) all = 0;
        *owned |= s->page[p].owned;
    }
    return all;
}

static const uint32_t k_act_bosses[DM_FT_DUEL_BOSSES] = { 156, 211, 242, 243, 544 };
int dm_ft_act_boss(uint32_t cls) { for (int i = 0; i < DM_FT_DUEL_BOSSES; i++) if (k_act_bosses[i] == cls) return 1; return 0; }
void dm_ft_board(dm_ft_facts *x, const dm_ft_board_row *row, int n) {
    x->bosses = 0; x->boss_fast = 0;
    for (int i = 0; i < n; i++) {
        int killed = 0;
        for (int d = 0; d < DM_FT_DIFFS; d++) {
            if (row[i].n[d] <= 0) continue;
            killed = 1;
            int t = row[i].best[d];
            if (row[i].cls >= 0 && dm_ft_act_boss((uint32_t)row[i].cls) && t > 0 && (!x->boss_fast || t < x->boss_fast)) x->boss_fast = t;
        }
        x->bosses += killed;
    }
}
void dm_ft_rift(dm_ft_facts *x, int best, const int fast[DM_FT_RIFT_TIERS + 1]) {
    x->rift_best = best > 0 ? best : 0; x->rift_timed = 0; x->rift_fast = 0;
    for (int t = 1; t <= DM_FT_RIFT_TIERS; t++) {
        if (fast[t] <= 0 || fast[t] >= DM_RIFT_SECS) continue;
        x->rift_timed = 1;
        if (t >= DM_FT_RIFT_FAST_TIER && (!x->rift_fast || fast[t] < x->rift_fast)) x->rift_fast = fast[t];
    }
}
int dm_ft_equip_worthy(double now_group, double after_group, double now_single, double after_single) {
    return (now_group > 0 && after_group * 100 >= now_group * (100 + DM_FT_EQUIP_PCT)) || (now_single > 0 && after_single * 100 >= now_single * (100 + DM_FT_EQUIP_PCT));
}
int dm_ft_shrines(const uint32_t *bits, int words) {
    int n = 0;
    for (int s = DM_FT_SHRINE_FIRST; s <= DM_FT_SHRINE_LAST && s / 32 < words; s++) n += (bits[s / 32] >> (s % 32)) & 1;
    return n;
}
int dm_ft_potion(uint32_t code) {
    const char *c = (const char *)&code;
    if (c[3] != ' ') return 0;
    if ((c[0] == 'h' || c[0] == 'm') && c[1] == 'p' && c[2] >= '1' && c[2] <= '5') return 1;
    return !memcmp(c, "rvs", 3) || !memcmp(c, "rvl", 3) || !memcmp(c, "yps", 3) || !memcmp(c, "vps", 3) || !memcmp(c, "wms", 3);
}


void dm_ft_duel_life(dm_ft_duel *d, int64_t life, int64_t max) { if (max > 0 && life * 2 < max) d->now.below_half = 1; }
void dm_ft_duel_below_half(dm_ft_duel *d) { d->now.below_half = 1; }
void dm_ft_duel_potion(dm_ft_duel *d) { d->now.potion = 1; }
void dm_ft_duel_fatal(dm_ft_duel *d, uint32_t cls, int64_t life, int64_t max) {
    dm_ft_duel_seen *n = &d->now;
    if (!dm_ft_act_boss(cls)) return;
    for (int i = 0; i < n->nfatal; i++) if (n->fatal[i].cls == cls) { n->fatal[i].life = life; n->fatal[i].max = max; return; }
    if (n->nfatal < DM_FT_DUEL_FATALS) n->fatal[n->nfatal++] = (typeof(n->fatal[0])){ cls, life, max };
}
void dm_ft_duel_won(dm_ft_duel *d) { d->won = d->now; d->has_won = 1; }
void dm_ft_duel_idle(dm_ft_duel *d) { memset(&d->now, 0, sizeof d->now); memset(&d->won, 0, sizeof d->won); d->has_won = 0; }
static void carry(dm_ft_duel *d, uint32_t cls, int below_half, int potion) {
    for (int i = 0; i < d->ncarry; i++) if (d->carry[i].cls == cls) { d->carry[i].below_half |= below_half; d->carry[i].potion |= potion; return; }
    if (d->ncarry < DM_FT_DUEL_BOSSES) d->carry[d->ncarry++] = (typeof(d->carry[0])){ cls, below_half, potion };
}
static int carried(const dm_ft_duel *d, uint32_t cls) { for (int c = 0; c < d->ncarry; c++) if (d->carry[c].cls == cls) return c; return -1; }

static int boss_dead(const dm_fl_group *x) { return !(x->flags & DM_FL_G_OTHERS) && dm_ft_act_boss(x->cls) && x->t_done && x->kills; }
unsigned dm_ft_duel_judge(const dm_ft_duel *d, const dm_fl_fight *won) {
    const dm_ft_duel_seen *w = &d->won; unsigned ev = 0;
    if (!d->has_won) return 0;
    for (int g = 0; g < won->h.ngroups && g < DM_FL_GROUPS; g++) {
        const dm_fl_group *x = &won->g[g];
        if (!boss_dead(x)) continue;
        int below = w->below_half, drank = w->potion, c = carried(d, x->cls);
        if (c >= 0) { below |= d->carry[c].below_half; drank |= d->carry[c].potion; }
        if (!below) ev |= DM_FT_E_CALM;
        if (!drank && won->h.difficulty >= 1) ev |= DM_FT_E_DRY;
        for (int i = 0; i < w->nfatal; i++)
            if (w->fatal[i].cls == x->cls && w->fatal[i].max > 0 && w->fatal[i].life > 0 && w->fatal[i].life * 100 < w->fatal[i].max * DM_FT_ESCAPE_PCT) ev |= DM_FT_E_ESCAPE;
        if (x->t_done > x->t_first && x->t_done - x->t_first < DM_FT_FAST_TICKS) ev |= DM_FT_E_FAST;
    }
    return ev;
}
unsigned dm_ft_duel_end(dm_ft_duel *d, const dm_fl_fight *f) {
    int won = f->h.end == DM_FL_END_KILL && d->has_won;
    unsigned ev = won ? dm_ft_duel_judge(d, f) : 0;
    for (int g = 0; g < f->h.ngroups && g < DM_FL_GROUPS; g++) {
        const dm_fl_group *x = &f->g[g];
        if (x->flags & DM_FL_G_OTHERS) { if (!x->t_done) for (int b = 0; b < DM_FT_DUEL_BOSSES; b++) carry(d, k_act_bosses[b], d->now.below_half, d->now.potion); continue; }
        if (!dm_ft_act_boss(x->cls)) continue;
        if (!won || !boss_dead(x)) { carry(d, x->cls, d->now.below_half, d->now.potion); continue; }
        int c = carried(d, x->cls);
        if (c >= 0) d->carry[c] = d->carry[--d->ncarry];
    }
    return ev;
}



static int code_is(uint32_t code, const char *s) { return !memcmp(&code, s, 3) && ((const char *)&code)[3] == ' '; }
static int rune_rank(uint32_t code) {
    const char *c = (const char *)&code;
    if (c[0] != 'r' || c[1] < '0' || c[1] > '9' || c[2] < '0' || c[2] > '9' || c[3] != ' ') return 0;
    int n = (c[1] - '0') * 10 + c[2] - '0';
    return n <= 33 ? n : 0;
}
enum { GEM_CHIPPED, GEM_FLAWED, GEM_STANDARD, GEM_FLAWLESS, GEM_PERFECT, SKULL = 6 };


static int gem(uint32_t code, int *colour) {
    static const char grades[] = "cfslp", skulls[] = "cfulz", colours[] = "vwgrby";
    const char *c = (const char *)&code, *g, *k;
    if (c[3] != ' ' || !c[1] || !c[2]) return -1;
    if (c[0] == 's' && c[1] == 'k' && (g = strchr(skulls, c[2]))) { *colour = SKULL; return (int)(g - skulls); }
    if (c[0] != 'g' || !(k = strchr(colours, c[2]))) return -1;
    *colour = (int)(k - colours);
    if (c[2] == 'v' && (c[1] == 'z' || c[1] == 'l')) return c[1] == 'z' ? GEM_FLAWLESS : -1;
    return (g = strchr(grades, c[1])) ? (int)(g - grades) : -1;
}
static int has_id(const dm_ft_cube_item *it, int n, uint32_t id) { for (int i = 0; i < n; i++) if (it[i].id == id) return 1; return 0; }

unsigned dm_ft_cube_events(const dm_ft_cube_item *before, int nb, const dm_ft_cube_item *after, int na) {
    enum { MAGIC = 4 };
    unsigned ev = 0, flawless = 0, perfect = 0;
    int top_rune = 0, runes = 0, ort = 0, ral = 0, hel = 0, portal = 0, chipped = 0, amulet = 0, filled = 0;
    for (int i = 0; i < nb; i++) {
        const dm_ft_cube_item *t = &before[i]; int colour, grade = gem(t->code, &colour), rank = rune_rank(t->code);
        if (has_id(after, na, t->id)) continue;
        if (rank) { runes++; if (rank > top_rune) top_rune = rank; ort |= rank == 9; ral |= rank == 8; hel |= rank == 15; }
        if (grade == GEM_FLAWLESS) flawless |= 1u << colour;
        if (grade == GEM_PERFECT) perfect |= 1u << colour;
        chipped |= grade == GEM_CHIPPED;
        portal |= code_is(t->code, "tsc");
        amulet |= code_is(t->code, "amu") && t->quality == MAGIC;
    }
    for (int i = 0; i < nb; i++) filled |= before[i].filled > 0 && !code_is(before[i].code, "tsc") && !rune_rank(before[i].code);


    int kept = ral && portal && filled && runes == 1;
    for (int i = 0; i < na && !kept; i++) {
        const dm_ft_cube_item *t = &after[i]; int colour, grade = gem(t->code, &colour);
        if (has_id(before, nb, t->id)) continue;
        if (runes && rune_rank(t->code) > top_rune) ev |= DM_FT_E_RUNE_UP;
        if (grade == GEM_PERFECT && (flawless >> colour & 1)) ev |= DM_FT_E_PERFECT;
        if (code_is(t->code, "amu") && t->quality == MAGIC && amulet && (perfect & 0x3f) == 0x3f) ev |= DM_FT_E_PRISM;
    }
    if (ort && chipped) ev |= DM_FT_E_RECHARGE;
    if ((hel || kept) && portal && filled) ev |= DM_FT_E_UNSOCKET;
    return ev;
}
unsigned dm_ft_socket_events(uint32_t code, int placed, int runeword_before, int runeword_after) {
    int colour;
    if (!placed) return 0;
    return (gem(code, &colour) >= 0 ? DM_FT_E_GEM : 0) | (runeword_after && !runeword_before ? DM_FT_E_RUNEWORD : 0);
}

static uint32_t u32(const uint8_t *p) { return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24; }


static int item_command(uint8_t op) { return (op >= 0x16 && op <= 0x2a) || (op >= 0x32 && op <= 0x35) || op == 0x46 || op == 0x5f || op == 0x63; }
int dm_ft_cmd_seen(dm_ft_cmds *q, const uint8_t *pkt, uint32_t n, uint32_t tick, const dm_ft_reader *r, void *ctx) {
    dm_ft_cmd c; memset(&c, 0, sizeof c); c.tick = tick;
    int got, placed, keep = 0;
    if (n >= 1 && item_command(pkt[0])) {
        for (int i = 0; i < q->n; i++) if (q->cmd[i].kind == DM_FT_CMD_SOCKET || q->cmd[i].kind == DM_FT_CMD_GAMBLE) q->cmd[keep++] = q->cmd[i];
        q->n = keep;
    }
    if (n >= 9 && pkt[0] == 0x28) {
        c.kind = DM_FT_CMD_SOCKET; c.item = u32(pkt + 1); c.target = u32(pkt + 5);
        if ((got = r->item(c.item, &c.code, ctx)) <= 0 || (got = r->target(c.target, c.item, &placed, &c.runeword, ctx)) <= 0) return got;
        if (placed) return 0;
    } else if (n >= 1 && pkt[0] == 0x20) {
        c.kind = DM_FT_CMD_CUBE;
        if ((c.n = r->cube(c.cube, DM_FT_CUBE, ctx)) < 0 || r->top(&c.top, ctx) < 0) return -1;
    } else if (n >= 9 && pkt[0] == 0x33) {
        c.kind = DM_FT_CMD_SELL; c.item = u32(pkt + 5);
        if ((got = r->item(c.item, &c.code, ctx)) <= 0) return got;
        if (r->gold(&c.gold, ctx) < 0) return -1;
    } else return 0;
    if (q->n >= DM_FT_CMDS) return -1;
    q->cmd[q->n++] = c;
    return 1;
}
int dm_ft_gamble_seen(dm_ft_cmds *q, uint32_t item, int quality, uint32_t tick) {
    if (q->n >= DM_FT_CMDS) return -1;
    dm_ft_cmd c; memset(&c, 0, sizeof c);
    c.kind = DM_FT_CMD_GAMBLE; c.tick = tick; c.item = item; c.quality = quality;
    q->cmd[q->n++] = c;
    return 1;
}
uint64_t dm_ft_gold_picked(int32_t added, int linked, uint32_t link_type) { return added > 0 && !(linked && link_type == 0) ? (uint64_t)added : 0; }
uint64_t dm_ft_gold_pickup(dm_ft_own_gold *o, uint32_t pile, int32_t added, int32_t rest, int linked, uint32_t link_type, uint32_t next_id) {
    int own = linked && link_type == 0;
    for (int i = 0; i < o->n; i++) if (o->id[i] == pile) { own = 1; o->id[i] = o->id[--o->n]; break; }
    if (own && rest > 0) { o->pending = next_id; o->pending_gold = rest; }
    return own ? 0 : dm_ft_gold_picked(added, 0, 0);
}
uint32_t dm_ft_own_gold_pending(const dm_ft_own_gold *o, int32_t *gold) { *gold = o->pending_gold; return o->pending; }
void dm_ft_own_gold_confirm(dm_ft_own_gold *o, int found) {
    if (found && o->pending) {
        if (o->n == DM_FT_OWN_PILES) { memmove(o->id, o->id + 1, sizeof o->id - sizeof *o->id); o->n--; }
        o->id[o->n++] = o->pending;
    }
    o->pending = 0; o->pending_gold = 0;
}
static int same_cube(const dm_ft_cube_item *a, int na, const dm_ft_cube_item *b, int nb) {
    if (na != nb) return 0;
    for (int i = 0; i < na; i++) if (!has_id(b, nb, a[i].id)) return 0;
    return 1;
}


static int transmuted(const dm_ft_cmd *c, const dm_ft_reader *r, void *ctx, dm_ft_cube_item out[2 * DM_FT_CUBE], int *n) {
    dm_ft_cube_item now[DM_FT_CUBE]; uint32_t code;
    int k = r->cube(now, DM_FT_CUBE, ctx);
    if (k < 0) return -1;
    *n = 0;
    for (int i = 0; i < k; i++) if (has_id(c->cube, c->n, now[i].id) || now[i].id > c->top) out[(*n)++] = now[i];
    for (int i = 0; i < c->n; i++) {
        if (has_id(now, k, c->cube[i].id)) continue;
        int held = r->item(c->cube[i].id, &code, ctx);
        if (held < 0) return -1;
        if (held) out[(*n)++] = c->cube[i];
    }
    return 1;
}
unsigned dm_ft_cmd_judge(dm_ft_cmds *q, uint32_t tick, const dm_ft_reader *r, void *ctx, int *sold, int *unread) {
    unsigned ev = 0; int keep = 0;
    *sold = 0; *unread = 0;
    for (int i = 0; i < q->n; i++) {
        dm_ft_cmd *c = &q->cmd[i];
        int shown = 0, placed = 0, runeword = 0, got = 0, k; uint32_t code; uint64_t gold;
        dm_ft_cube_item after[2 * DM_FT_CUBE];
        if (c->kind == DM_FT_CMD_SOCKET) {
            got = r->target(c->target, c->item, &placed, &runeword, ctx);
            if (got > 0 && placed) { shown = 1; ev |= dm_ft_socket_events(c->code, 1, c->runeword, runeword); }
        } else if (c->kind == DM_FT_CMD_CUBE) {
            got = transmuted(c, r, ctx, after, &k);
            if (got > 0 && !same_cube(c->cube, c->n, after, k)) { shown = 1; ev |= dm_ft_cube_events(c->cube, c->n, after, k); }
        } else if (c->kind == DM_FT_CMD_SELL) {
            got = r->item(c->item, &code, ctx);
            if (!got && (got = r->gold(&gold, ctx)) > 0) { shown = 1; *sold += gold > c->gold; }
        } else if (c->kind == DM_FT_CMD_GAMBLE) {
            got = r->item(c->item, &code, ctx);
            if (got > 0) { shown = 1; ev |= c->quality == DM_FT_Q_UNIQUE ? DM_FT_E_GAMBLE : 0; }
        }
        if (got < 0) { ++*unread; continue; }
        if (!shown && tick - c->tick < DM_FT_CMD_TICKS) q->cmd[keep++] = *c;
    }
    q->n = keep;
    return ev;
}

int dm_ft_met(const dm_ft_row *r, const dm_ft_facts *x, const uint64_t counters[DM_FT_COUNTERS]) {
    switch (r->rule) {
    case DM_FT_R_KILLS: return counters[DM_FT_KILLS] >= r->arg;
    case DM_FT_R_ACT: for (int d = 0; d < DM_FT_DIFFS; d++) if (act_done(x, d, (int)r->arg)) return 1; return 0;
    case DM_FT_R_DIFF: return act_done(x, (int)r->arg, 5);
    case DM_FT_R_CLASS_HELL: return x->cls == (int)r->arg && act_done(x, 2, 5);
    case DM_FT_R_LEVEL: return x->level >= (int)r->arg;
    case DM_FT_R_EVERY_CLASS: return 0;
    case DM_FT_R_WAYPOINTS: return all_waypoints(x);
    case DM_FT_R_QUESTS: return all_quests(x);
    case DM_FT_R_HC_LEVEL: return x->hardcore == 1 && x->level >= (int)r->arg;
    case DM_FT_R_HC_GAME: if (x->hardcore != 1) return 0; for (int d = 0; d < DM_FT_DIFFS; d++) if (act_done(x, d, 5)) return 1; return 0;
    case DM_FT_R_OWN: return (x->owned & r->arg) != 0;
    case DM_FT_R_UNIQUES: return bits(x->uniques & ((1u << DM_FT_SLOTS) - 1)) >= (int)r->arg;
    case DM_FT_R_FULL_SET: return x->full_set != 0;
    case DM_FT_R_MERC: return x->merc == (int)r->arg;
    case DM_FT_R_EVENT: return 0;
    case DM_FT_R_SALES: return counters[DM_FT_SALES] >= r->arg;
    case DM_FT_R_GOLD: return counters[DM_FT_GOLD] >= r->arg;
    case DM_FT_R_STREAK: return x->streak >= (int)r->arg;
    case DM_FT_R_BOSSES: return x->bosses >= (int)r->arg;
    case DM_FT_R_BOSS_FAST: return x->boss_fast > 0 && x->boss_fast < (int)r->arg;
    case DM_FT_R_RIFT_CLEAR: return x->rift_timed != 0;
    case DM_FT_R_RIFT_TIER: return x->rift_best >= (int)r->arg;
    case DM_FT_R_RIFT_FAST: return x->rift_fast > 0 && x->rift_fast < (int)r->arg;
    case DM_FT_R_SHRINES: return x->shrines >= (int)r->arg;
    case DM_FT_R_LOOKS: return counters[DM_FT_LOOKS] >= r->arg;
    case DM_FT_R_DYES: return counters[DM_FT_DYES] >= r->arg;
    }
    return 0;
}

static int counter_of(const dm_ft_row *r) {
    switch (r->rule) {
    case DM_FT_R_KILLS: return DM_FT_KILLS; case DM_FT_R_SALES: return DM_FT_SALES; case DM_FT_R_GOLD: return DM_FT_GOLD;
    case DM_FT_R_LOOKS: return DM_FT_LOOKS; case DM_FT_R_DYES: return DM_FT_DYES;
    }
    return -1;
}
static int counter_rule(const dm_ft_row *r) { return counter_of(r) >= 0; }

static dm_ft_rec *find(dm_ft *f, int kind, int id) {
    for (int i = 0; i < f->nrec; i++) if (f->rec[i].kind == kind && f->rec[i].id == id) return &f->rec[i];
    return NULL;
}
const dm_ft_rec *dm_ft_done(const dm_ft *f, int id) { return find((dm_ft *)f, DM_FT_REC_DONE, id); }
uint64_t dm_ft_counter(const dm_ft *f, int counter) { const dm_ft_rec *r = find((dm_ft *)f, DM_FT_REC_COUNT, counter); return r ? r->value : 0; }
static void counters_of(const dm_ft *f, uint64_t out[DM_FT_COUNTERS]) { for (int c = 0; c < DM_FT_COUNTERS; c++) out[c] = dm_ft_counter(f, c); }

void dm_ft_clear(dm_ft *f) { memset(f, 0, sizeof *f); }
void dm_ft_take(dm_ft *f, const dm_ft_rec *rec, int n, uint32_t rev) {
    uint32_t gen = f->gen, seq = f->seq; dm_ft_note note[DM_FT_NOTICES]; memcpy(note, f->note, sizeof note);
    memset(f, 0, sizeof *f);
    f->gen = gen + 1; f->seq = seq; memcpy(f->note, note, sizeof note);
    if (n > DM_FT_RECS) n = DM_FT_RECS;
    memcpy(f->rec, rec, (size_t)n * sizeof *rec); f->nrec = n; f->rev = rev;
    counters_of(f, f->loaded);
}


static int enter(dm_ft *f, const dm_ft_row *r, uint32_t now, int silent) {
    if (f->nrec >= DM_FT_RECS) return 0;
    f->rec[f->nrec++] = (dm_ft_rec){ r->id, DM_FT_REC_DONE, (uint8_t)(f->level > 0 && f->level < 256 ? f->level : 0), now, 0 };
    f->gen++;
    if (silent) return 0;
    f->noticed = 1; f->seq++;
    f->note[f->seq % DM_FT_NOTICES] = (dm_ft_note){ f->seq, r->id, r->arg };
    return 1;
}

int dm_ft_update(dm_ft *f, const dm_ft_facts *x, uint32_t now) {
    f->level = x->level;
    uint64_t c[DM_FT_COUNTERS]; int posted = 0, wait = 0;


    for (int i = 0; i < dm_ft_nrows && f->rev < DM_FT_REV; i++) {
        const dm_ft_row *r = &dm_ft_rows[i];
        if (r->rev <= f->rev || f->judged[i]) continue;
        if (needs(r) & ~x->known) { wait = 1; continue; }
        f->judged[i] = 1;
        if (!dm_ft_done(f, r->id) && dm_ft_met(r, x, f->loaded)) enter(f, r, now, 1);
    }
    if (!wait && f->rev < DM_FT_REV) { f->rev = DM_FT_REV; f->gen++; }
    f->reconciled = 1;

    counters_of(f, c);
    for (int i = 0; i < dm_ft_nrows; i++) {
        const dm_ft_row *r = &dm_ft_rows[i];
        if ((r->rev > f->rev && !f->judged[i]) || dm_ft_done(f, r->id) || r->rule == DM_FT_R_BOSS_FAST) continue;
        if (dm_ft_met(r, x, c)) posted += enter(f, r, now, 0);
    }
    return posted;
}

static dm_ft_rec *counter(dm_ft *f, int c) {
    dm_ft_rec *r = find(f, DM_FT_REC_COUNT, c);
    if (r || f->nrec >= DM_FT_RECS) return r;
    r = &f->rec[f->nrec++]; *r = (dm_ft_rec){ (uint16_t)c, DM_FT_REC_COUNT, 0, 0, 0 };
    return r;
}
int dm_ft_count(dm_ft *f, int c, uint64_t n, uint32_t now) {
    if (c < 0 || c >= DM_FT_COUNTERS || !n) return 0;
    dm_ft_rec *r = counter(f, c); if (!r) return 0;
    r->value = r->value > UINT64_MAX - n ? UINT64_MAX : r->value + n;
    f->gen++;
    if (!f->reconciled) return 0;
    uint64_t v[DM_FT_COUNTERS]; counters_of(f, v);
    dm_ft_facts none; memset(&none, 0, sizeof none); none.cls = -1;
    int posted = 0;
    for (int i = 0; i < dm_ft_nrows; i++) {
        const dm_ft_row *row = &dm_ft_rows[i];
        if (counter_rule(row) && !dm_ft_done(f, row->id) && dm_ft_met(row, &none, v)) posted += enter(f, row, now, 0);
    }
    return posted;
}

static unsigned row_event(const dm_ft_row *r) { return r->rule == DM_FT_R_EVENT ? r->arg : r->rule == DM_FT_R_BOSS_FAST ? DM_FT_E_FAST : 0; }
int dm_ft_event(dm_ft *f, unsigned events, uint32_t now) {
    int posted = 0;
    for (int i = 0; i < dm_ft_nrows; i++) {
        const dm_ft_row *r = &dm_ft_rows[i];
        if ((events & row_event(r)) && !dm_ft_done(f, r->id)) posted += enter(f, r, now, 0);
    }
    return posted;
}
int dm_ft_stage(dm_ft *f, unsigned events, uint32_t now, uint16_t *ids, int n) {
    int k = 0;
    for (int i = 0; i < dm_ft_nrows && k < n; i++) {
        const dm_ft_row *r = &dm_ft_rows[i];
        if ((events & row_event(r)) && !dm_ft_done(f, r->id) && f->nrec < DM_FT_RECS) { enter(f, r, now, 1); ids[k++] = r->id; }
    }
    return k;
}
int dm_ft_stage_count(dm_ft *f, int c, uint64_t n, uint32_t now, uint16_t *ids, int max) {
    if (c < 0 || c >= DM_FT_COUNTERS || !n) return 0;
    dm_ft_rec *r = counter(f, c); if (!r) return 0;
    r->value = r->value > UINT64_MAX - n ? UINT64_MAX : r->value + n;
    f->gen++;
    uint64_t v[DM_FT_COUNTERS]; counters_of(f, v);
    dm_ft_facts none; memset(&none, 0, sizeof none); none.cls = -1;
    int k = 0;
    for (int i = 0; i < dm_ft_nrows && k < max; i++) {
        const dm_ft_row *row = &dm_ft_rows[i];
        if (counter_of(row) == c && !dm_ft_done(f, row->id) && dm_ft_met(row, &none, v) && f->nrec < DM_FT_RECS) { enter(f, row, now, 1); ids[k++] = row->id; }
    }
    return k;
}
void dm_ft_unstage(dm_ft *f, const uint16_t *ids, int n) {
    for (int k = 0; k < n; k++) {
        dm_ft_rec *r = find(f, DM_FT_REC_DONE, ids[k]);
        if (r) { *r = f->rec[--f->nrec]; f->gen++; }
    }
}
int dm_ft_notice(dm_ft *f, const uint16_t *ids, int n) {
    int posted = 0;
    for (int k = 0; k < n; k++) {
        const dm_ft_row *r = dm_ft_row_of(ids[k]);
        if (!r || !dm_ft_done(f, r->id)) continue;
        f->noticed = 1; f->seq++; posted++;
        f->note[f->seq % DM_FT_NOTICES] = (dm_ft_note){ f->seq, r->id, r->arg };
    }
    return posted;
}
void dm_ft_set_counter(dm_ft *f, int c, uint64_t v) {
    if (c < 0 || c >= DM_FT_COUNTERS) return;
    dm_ft_rec *r = counter(f, c); if (!r) return;
    r->value = v; f->gen++;
}


#define PUT(...) do { w = snprintf(out + n, cap - n, __VA_ARGS__); if (w < 0 || (size_t)w >= cap - n) { out[0] = 0; return -2; } n += (size_t)w; } while (0)
int dm_ft_text(const dm_ft *f, char *out, size_t cap) {
    if (!out || !cap) return -1;
    size_t n = 0; int w;
    for (int c = 0; c < DM_FT_COUNTERS; c++) PUT("counter\t%d\t%llu\n", c, (unsigned long long)dm_ft_counter(f, c));
    for (int i = 0; i < dm_ft_nrows; i++) {
        const dm_ft_row *r = &dm_ft_rows[i]; const dm_ft_rec *d = dm_ft_done(f, r->id);
        if (dm_ft_account_wide(r)) continue;
        PUT("row\t%u\t%u\t%u\t%u\t%d\t%u\n", r->id, r->group, r->rule, r->arg, d != NULL, d ? d->when : 0u);
    }
    return (int)n;
}


void dm_ft_sum_clear(dm_ft_sum *a) { memset(a, 0, sizeof *a); }
static int row_index(int id) { for (int i = 0; i < dm_ft_nrows; i++) if (dm_ft_rows[i].id == id) return i; return -1; }
void dm_ft_sum_add(dm_ft_sum *a, const dm_ft_rec *rec, int n) {
    a->heroes++;
    for (int k = 0; k < n; k++) {
        const dm_ft_rec *r = &rec[k];
        if (r->kind == DM_FT_REC_COUNT && r->id < DM_FT_COUNTERS) {
            uint64_t *c = &a->counters[r->id]; if (r->value > *c) *c = r->value;
        } else if (r->kind == DM_FT_REC_DONE) {
            int i = row_index(r->id);
            if (i < 0 || dm_ft_account_wide(&dm_ft_rows[i])) continue;
            if (!a->done[i] || r->when < a->when[i]) a->when[i] = r->when;
            a->done[i] = 1;
        }
    }

    for (int i = 0; i < dm_ft_nrows; i++) {
        if (dm_ft_rows[i].rule != DM_FT_R_EVERY_CLASS) continue;
        int all = 1; uint32_t last = 0;
        for (int j = 0; j < dm_ft_nrows; j++) if (dm_ft_rows[j].rule == DM_FT_R_CLASS_HELL) { all &= a->done[j]; if (a->when[j] > last) last = a->when[j]; }
        a->done[i] = (uint8_t)all; a->when[i] = all ? last : 0;
    }
}
int dm_ft_sum_text(const dm_ft_sum *a, char *out, size_t cap) {
    if (!out || !cap) return -1;
    size_t n = 0; int w;
    for (int c = 0; c < DM_FT_COUNTERS; c++) PUT("allcounter\t%d\t%llu\n", c, (unsigned long long)a->counters[c]);
    for (int i = 0; i < dm_ft_nrows; i++) {
        const dm_ft_row *r = &dm_ft_rows[i];
        PUT("allrow\t%u\t%u\t%u\t%u\t%d\t%u\n", r->id, r->group, r->rule, r->arg, a->done[i] != 0, a->done[i] ? a->when[i] : 0u);
    }
    return (int)n;
}
#undef PUT
