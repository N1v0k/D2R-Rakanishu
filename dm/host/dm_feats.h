
























#pragma once
#include "dm_fightlog.h"
#include <stddef.h>
#include <stdint.h>

enum {
    DM_FT_REV = 6,
    DM_FT_DIFFS = 3, DM_FT_WORDS = 48, DM_FT_CLASSES = 8,
    DM_FT_WP_BYTES = 8,
    DM_FT_SLOTS = 10,
    DM_FT_RECS = 256,
    DM_FT_FILE_RECS = 192,
    DM_FT_NOTICES = 8,
    DM_FT_ROWS_MAX = 128,
    DM_FT_SUM_HEROES = 256,
};
enum { DM_FT_KILLS, DM_FT_SALES, DM_FT_GOLD, DM_FT_LOOKS, DM_FT_DYES, DM_FT_COUNTERS };
enum { DM_FT_REC_DONE = 1, DM_FT_REC_COUNT = 2 };










enum { DM_FT_R_KILLS = 1, DM_FT_R_ACT, DM_FT_R_DIFF, DM_FT_R_CLASS_HELL, DM_FT_R_LEVEL, DM_FT_R_EVERY_CLASS, DM_FT_R_WAYPOINTS, DM_FT_R_HC_LEVEL,
       DM_FT_R_HC_GAME, DM_FT_R_OWN, DM_FT_R_UNIQUES, DM_FT_R_FULL_SET, DM_FT_R_MERC, DM_FT_R_EVENT, DM_FT_R_SALES, DM_FT_R_GOLD, DM_FT_R_STREAK,
       DM_FT_R_BOSSES, DM_FT_R_BOSS_FAST, DM_FT_R_RIFT_CLEAR, DM_FT_R_RIFT_TIER, DM_FT_R_RIFT_FAST, DM_FT_R_SHRINES, DM_FT_R_LOOKS, DM_FT_R_DYES,
       DM_FT_R_QUESTS };
enum { DM_FT_G_COMBAT = 1, DM_FT_G_CAMPAIGN, DM_FT_G_MASTERY, DM_FT_G_TREASURE, DM_FT_G_COMPANIONS, DM_FT_G_CRAFT, DM_FT_G_HUNT, DM_FT_G_RIFTS };




enum { DM_FT_E_GEM = 1, DM_FT_E_RUNEWORD = 2, DM_FT_E_RUNE_UP = 4, DM_FT_E_PERFECT = 8, DM_FT_E_PRISM = 16, DM_FT_E_RECHARGE = 32, DM_FT_E_UNSOCKET = 64,
       DM_FT_E_GAMBLE = 128, DM_FT_E_CALM = 256, DM_FT_E_DRY = 512, DM_FT_E_ESCAPE = 1024, DM_FT_E_RUSH = 2048, DM_FT_E_EQUIP = 4096,
       DM_FT_E_FAST = 32768 };
enum { DM_FT_EQUIP_PCT = 25 };


int dm_ft_equip_worthy(double now_group, double after_group, double now_single, double after_single);
enum { DM_FT_FAST_TICKS = 10 * 25 };
enum { DM_FT_OWN_SOJ = 1, DM_FT_OWN_STD = 2 };
enum { DM_FT_Q_SET = 5, DM_FT_Q_UNIQUE = 7, DM_FT_SOJ_ROW = 122 };




enum { DM_FT_QUESTS = 27 };
int dm_ft_quests(const uint16_t words[DM_FT_WORDS]);

typedef struct { uint16_t id; uint8_t rule, group, rev; uint32_t arg; } dm_ft_row;
extern const dm_ft_row dm_ft_rows[];
extern const int dm_ft_nrows;
const dm_ft_row *dm_ft_row_of(int id);
int dm_ft_account_wide(const dm_ft_row *r);


enum { DM_FT_NAME_KEYS = 6 };
typedef struct { uint16_t id; uint8_t plain; const char *key; } dm_ft_name_key;
extern const dm_ft_name_key dm_ft_name_keys[DM_FT_NAME_KEYS];
int dm_ft_name_key_of(int id);


int dm_ft_game_text_ok(const char *text, const char *key, int plain);



typedef struct { uint16_t id; uint8_t kind, level; uint32_t when; uint64_t value; } dm_ft_rec;
_Static_assert(sizeof(dm_ft_rec) == 16, "feat record layout");


enum { DM_FT_K_HARDCORE = 1, DM_FT_K_MERC = 2, DM_FT_K_SETS = 4, DM_FT_K_PAGES = 8, DM_FT_K_STREAKS = 16, DM_FT_K_KILLS = 32, DM_FT_K_RIFT = 64,
       DM_FT_K_ALL = 127 };







typedef struct {
    int level, cls; uint16_t quests[DM_FT_DIFFS][DM_FT_WORDS];
    int waypoints; uint8_t wp[DM_FT_DIFFS][DM_FT_WP_BYTES];
    int hardcore, merc;
    unsigned uniques, owned; int full_set;
    int streak, bosses, boss_fast, rift_best, rift_timed, rift_fast, shrines;
    unsigned known;
} dm_ft_facts;



typedef struct { uint32_t code; uint8_t quality, slot, two_hands; uint16_t row; int16_t set; } dm_ft_item;



void dm_ft_items(dm_ft_facts *x, const dm_ft_item *it, int n, const int16_t *pieces, int nset);
unsigned dm_ft_item_owned(const dm_ft_item *it);




enum { DM_FT_PAGES = 5 };
typedef struct { struct { uint64_t sig; unsigned owned; int read; } page[DM_FT_PAGES]; int next; } dm_ft_pages;
typedef int (*dm_ft_page_sig_fn)(int page, uint64_t *sig, void *ctx);
typedef int (*dm_ft_page_items_fn)(int page, unsigned *owned, void *ctx);
int dm_ft_pages_owned(dm_ft_pages *s, int shown, dm_ft_page_sig_fn sig, dm_ft_page_items_fn items, void *ctx, unsigned *owned);

int dm_ft_act_boss(uint32_t cls);


typedef struct { int32_t cls; int32_t n[DM_FT_DIFFS], best[DM_FT_DIFFS]; } dm_ft_board_row;
enum { DM_FT_BOARD_ROWS = 64 };

void dm_ft_board(dm_ft_facts *x, const dm_ft_board_row *row, int n);

enum { DM_FT_RIFT_TIERS = 100, DM_FT_RIFT_FAST_TIER = 10 };
void dm_ft_rift(dm_ft_facts *x, int best, const int fast[DM_FT_RIFT_TIERS + 1]);

enum { DM_FT_SHRINE_FIRST = 128, DM_FT_SHRINE_LAST = 137 };
int dm_ft_shrines(const uint32_t *bits, int words);

int dm_ft_potion(uint32_t code);









enum { DM_FT_ESCAPE_PCT = 5, DM_FT_DUEL_FATALS = 8, DM_FT_DUEL_BOSSES = 5 };
typedef struct {
    int below_half, potion;
    struct { uint32_t cls; int64_t life, max; } fatal[DM_FT_DUEL_FATALS]; int nfatal;
} dm_ft_duel_seen;
typedef struct {
    dm_ft_duel_seen now, won; int has_won;
    struct { uint32_t cls; int below_half, potion; } carry[DM_FT_DUEL_BOSSES]; int ncarry;
} dm_ft_duel;
void dm_ft_duel_life(dm_ft_duel *d, int64_t life, int64_t max);
void dm_ft_duel_below_half(dm_ft_duel *d);
void dm_ft_duel_potion(dm_ft_duel *d);
void dm_ft_duel_fatal(dm_ft_duel *d, uint32_t cls, int64_t life, int64_t max);
void dm_ft_duel_won(dm_ft_duel *d);
void dm_ft_duel_idle(dm_ft_duel *d);
unsigned dm_ft_duel_judge(const dm_ft_duel *d, const dm_fl_fight *won);
unsigned dm_ft_duel_end(dm_ft_duel *d, const dm_fl_fight *f);


typedef struct { uint32_t id, code; uint8_t quality, filled; } dm_ft_cube_item;




unsigned dm_ft_cube_events(const dm_ft_cube_item *before, int nb, const dm_ft_cube_item *after, int na);


unsigned dm_ft_socket_events(uint32_t code, int placed, int runeword_before, int runeword_after);

















enum { DM_FT_CMDS = 8, DM_FT_CUBE = 16, DM_FT_CMD_TICKS = 25 };
enum { DM_FT_CMD_SOCKET = 1, DM_FT_CMD_CUBE, DM_FT_CMD_SELL, DM_FT_CMD_GAMBLE };
typedef struct {
    int (*item)(uint32_t id, uint32_t *code, void *ctx);
    int (*target)(uint32_t target, uint32_t item, int *placed, int *runeword, void *ctx);
    int (*cube)(dm_ft_cube_item *out, int max, void *ctx);
    int (*top)(uint32_t *id, void *ctx);
    int (*gold)(uint64_t *g, void *ctx);
} dm_ft_reader;
typedef struct { int kind; uint32_t tick, item, target, code, top; uint64_t gold; int runeword, quality, n; dm_ft_cube_item cube[DM_FT_CUBE]; } dm_ft_cmd;
typedef struct { dm_ft_cmd cmd[DM_FT_CMDS]; int n; } dm_ft_cmds;

int dm_ft_cmd_seen(dm_ft_cmds *q, const uint8_t *pkt, uint32_t n, uint32_t tick, const dm_ft_reader *r, void *ctx);

int dm_ft_gamble_seen(dm_ft_cmds *q, uint32_t item, int quality, uint32_t tick);


uint64_t dm_ft_gold_picked(int32_t added, int linked, uint32_t link_type);




enum { DM_FT_OWN_PILES = 8 };
typedef struct { uint32_t id[DM_FT_OWN_PILES]; int n; uint32_t pending; int32_t pending_gold; } dm_ft_own_gold;
uint64_t dm_ft_gold_pickup(dm_ft_own_gold *o, uint32_t pile, int32_t added, int32_t rest, int linked, uint32_t link_type, uint32_t next_id);
uint32_t dm_ft_own_gold_pending(const dm_ft_own_gold *o, int32_t *gold);
void dm_ft_own_gold_confirm(dm_ft_own_gold *o, int found);

unsigned dm_ft_cmd_judge(dm_ft_cmds *q, uint32_t tick, const dm_ft_reader *r, void *ctx, int *sold, int *unread);

typedef struct { uint32_t seq; uint16_t id; uint32_t arg; } dm_ft_note;

typedef struct {
    dm_ft_rec rec[DM_FT_RECS]; int nrec;
    uint32_t rev;
    int reconciled;
    uint8_t judged[DM_FT_ROWS_MAX];
    int level;
    int noticed;
    uint64_t loaded[DM_FT_COUNTERS];
    uint32_t gen;
    uint32_t seq; dm_ft_note note[DM_FT_NOTICES];
} dm_ft;

void dm_ft_clear(dm_ft *f);

void dm_ft_take(dm_ft *f, const dm_ft_rec *rec, int n, uint32_t rev);



int dm_ft_update(dm_ft *f, const dm_ft_facts *x, uint32_t now);

int dm_ft_count(dm_ft *f, int counter, uint64_t n, uint32_t now);

int dm_ft_event(dm_ft *f, unsigned events, uint32_t now);
void dm_ft_set_counter(dm_ft *f, int counter, uint64_t v);



int dm_ft_stage(dm_ft *f, unsigned events, uint32_t now, uint16_t *ids, int n);
int dm_ft_stage_count(dm_ft *f, int counter, uint64_t n, uint32_t now, uint16_t *ids, int max);
void dm_ft_unstage(dm_ft *f, const uint16_t *ids, int n);
int dm_ft_notice(dm_ft *f, const uint16_t *ids, int n);
uint64_t dm_ft_counter(const dm_ft *f, int counter);
const dm_ft_rec *dm_ft_done(const dm_ft *f, int id);
int dm_ft_met(const dm_ft_row *r, const dm_ft_facts *x, const uint64_t counters[DM_FT_COUNTERS]);



int dm_ft_text(const dm_ft *f, char *out, size_t cap);




typedef struct {
    int heroes;
    uint64_t counters[DM_FT_COUNTERS];
    uint8_t done[DM_FT_ROWS_MAX];
    uint32_t when[DM_FT_ROWS_MAX];
} dm_ft_sum;
void dm_ft_sum_clear(dm_ft_sum *a);
void dm_ft_sum_add(dm_ft_sum *a, const dm_ft_rec *rec, int n);


int dm_ft_sum_text(const dm_ft_sum *a, char *out, size_t cap);





enum { DM_FT_MAGIC = 0x5446444d, DM_FT_VERSION = 1, DM_FT_IMAGE_HEAD = 20, DM_FT_OWNER = 64 };
#define DM_FT_IMAGE_MAX (DM_FT_IMAGE_HEAD + DM_FT_OWNER + DM_FT_RECS * sizeof(dm_ft_rec))
enum { DM_FT_BAD_SIZE = 1, DM_FT_BAD_MAGIC, DM_FT_BAD_VERSION, DM_FT_BAD_CRC, DM_FT_BAD_COUNT, DM_FT_BAD_OWNER, DM_FT_BAD_RECORD, DM_FT_NEWER, DM_FT_FOREIGN,
       DM_FT_UNREAD };



enum { DM_FT_MODE_OWN, DM_FT_MODE_FOREIGN, DM_FT_MODE_NEWER, DM_FT_MODE_UNREAD };
typedef struct dm_ft_store dm_ft_store;
dm_ft_store *dm_ft_store_core(void);

int dm_ft_parse(const uint8_t *image, size_t n, const char *who, dm_ft_rec *rec, int *nrec, uint32_t *rev);


int dm_ft_store_owner(dm_ft_store *s, const char *who, const uint8_t *image, size_t n);
int dm_ft_store_owned_by(dm_ft_store *s, const char *who);
int dm_ft_store_mode(dm_ft_store *s);

size_t dm_ft_store_image(dm_ft_store *s, char who[64], uint8_t *buf, size_t cap, uint32_t *gen);
void dm_ft_store_saved(dm_ft_store *s, uint32_t gen);
void dm_ft_store_unsaved(dm_ft_store *s);




int dm_ft_store_load(dm_ft_store *s, const char *root, const char *who);
int dm_ft_store_follow(dm_ft_store *s, const char *root, const char *who);
int dm_ft_store_commit(dm_ft_store *s, const char *wd, uint64_t commits, uint64_t fails);

int dm_ft_store_facts(dm_ft_store *s, const dm_ft_facts *x, uint32_t now, dm_ft_note out[DM_FT_NOTICES], int *n);
int dm_ft_store_count(dm_ft_store *s, int counter, uint64_t v, uint32_t now, dm_ft_note out[DM_FT_NOTICES], int *n);
int dm_ft_store_event(dm_ft_store *s, unsigned events, uint32_t now, dm_ft_note out[DM_FT_NOTICES], int *n);




enum { DM_FT_STAGED = 16 };
int dm_ft_store_stage(dm_ft_store *s, unsigned events, uint32_t now, uint64_t commits, uint64_t fails);
int dm_ft_store_stage_count(dm_ft_store *s, int counter, uint64_t n, uint32_t now, uint64_t commits, uint64_t fails);
int dm_ft_store_staged(dm_ft_store *s, uint64_t commits, uint64_t fails, dm_ft_note out[DM_FT_NOTICES], int *n);
void dm_ft_store_seed(dm_ft_store *s, int counter, uint64_t v);
uint64_t dm_ft_store_counter(dm_ft_store *s, int counter);

int dm_ft_store_text(dm_ft_store *s, char *out, size_t cap);







int dm_ft_store_all_text(dm_ft_store *s, const char *root, char *out, size_t cap);




int dm_ftr_waypoints(uint64_t pd, uint8_t wp[DM_FT_DIFFS][DM_FT_WP_BYTES]);


int dm_ftr_waypoint_count(uint64_t t);


int dm_ftr_hireling_act(uint64_t t, int at, uint32_t want);
int dm_ftr_unit(uint64_t u, size_t n);



typedef uint32_t (*dm_ftr_item_fn)(uint64_t item);
int dm_ftr_items(uint64_t inv, dm_ft_item *out, int max, dm_ftr_item_fn code, dm_ftr_item_fn two_hands);

int dm_ftr_find(uint64_t inv, uint32_t id, uint64_t *item);


int dm_ftr_socket(uint64_t it, uint32_t id, int *runeword);


int dm_ftr_cube(uint64_t inv, dm_ft_cube_item *out, int max, dm_ftr_item_fn code);

int dm_ftr_top(uint64_t inv, uint32_t *id);


int dm_ftr_gold_link(uint64_t it, int *linked, uint32_t *type);

int dm_ftr_quality(uint64_t it, uint32_t *id, int *quality);



void dm_feats_tick(uint64_t G, uint64_t player, uint8_t *main_base);

void dm_feats_c2s(uint64_t G, uint64_t player, const uint8_t *pkt, uint32_t n);

void dm_feats_event(unsigned events);


void dm_feats_stage(unsigned events);
void dm_feats_stage_count(int counter, uint64_t n);

void dm_feats_streak_end(uint32_t kills);

enum { DM_FT_HUD_SEQ, DM_FT_HUD_NOTES, DM_FT_HUD_N = 1 + 3 * DM_FT_NOTICES };
int dm_feats_hud(int *out, int max);


int dm_feats_names(uint32_t seq, char *out, size_t cap);






int dm_feats_text(char *out, size_t cap);
