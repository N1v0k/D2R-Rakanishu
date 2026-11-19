

#pragma once
#include <stddef.h>
#include <stdint.h>

enum {
    DM_FL_RING = 50,
    DM_FL_GROUPS = 8,
    DM_FL_TRACK = 64,
    DM_FL_MEMBERS = 256,
    DM_FL_BUCKETS = 256,
    DM_FL_MARKS = 64,
    DM_FL_SKILLS = 8,
    DM_FL_TPS = 25,
    DM_FL_LEAD = 2 * DM_FL_TPS,
    DM_FL_PAUSE = 5 * DM_FL_TPS,
    DM_FL_GRACE = 2 * DM_FL_TPS,
    DM_FL_MIN = 3 * DM_FL_TPS,
    DM_FL_SAMPLE = 5,
    DM_FL_SCAN = DM_FL_TPS,
    DM_FL_HITS = 256,
    DM_FL_PREROLL = 8,
};
enum { DM_FL_END_KILL = 1, DM_FL_END_DEATH, DM_FL_END_PAUSE, DM_FL_END_AREA, DM_FL_END_LEFT };
enum { DM_FL_RANK_NORMAL, DM_FL_RANK_MINION, DM_FL_RANK_CHAMPION, DM_FL_RANK_UNIQUE, DM_FL_RANK_SUPER };
enum { DM_FL_OUT_PLAYER, DM_FL_OUT_MINION, DM_FL_IN_PLAYER, DM_FL_IN_MINION };
enum { DM_FL_F_MORE_KINDS = 1, DM_FL_F_TABLE_FULL = 2, DM_FL_F_MARKS_FULL = 4, DM_FL_F_HITS_LOST = 8, DM_FL_F_MEMBERS_FULL = 16, DM_FL_F_DOT_START = 32 };
enum { DM_FL_G_OTHERS = 1 };


enum { DM_FL_ME_ATTACK = 1, DM_FL_ME_CAST = 2, DM_FL_ME_HIT = 4, DM_FL_ME_CRIT = 8, DM_FL_ME_MINION = 16, DM_FL_ME_DOT = 32, DM_FL_ME_POTION = 64 };
enum { DM_FL_EN_ATTACK = 1, DM_FL_EN_HIT = 2, DM_FL_EN_BLOCK = 4, DM_FL_EN_MISS = 8, DM_FL_EN_MINION_HIT = 16, DM_FL_EN_DODGE = 32 };
enum { DM_FL_S_ENEMY, DM_FL_S_LIFE, DM_FL_S_MANA, DM_FL_SERIES };
enum { DM_FL_MARK_POTION = 1, DM_FL_MARK_STATE_ON, DM_FL_MARK_STATE_OFF, DM_FL_MARK_KILL };
enum { DM_FL_STATE_WORDS = 8 };

typedef struct { uint8_t lo[DM_FL_SERIES], hi[DM_FL_SERIES], last[DM_FL_SERIES], me, en, skill; } dm_fl_bucket;

typedef struct { uint16_t t; uint8_t kind, a; uint16_t b; } dm_fl_mark;

typedef struct { uint16_t cls, su; uint8_t rank, flags; uint16_t joined, kills, tracked, t_first, t_done; uint32_t dealt, life; } dm_fl_group;
typedef struct {
    uint32_t duration;
    uint32_t dealt[3];
    uint32_t taken;
    uint32_t life_total;
    uint16_t lead, width;
    uint16_t area, kills, enemies, nbuckets;
    uint16_t skill[DM_FL_SKILLS];
    uint8_t end, ngroups, nmarks, flags, min_life, difficulty;
    uint16_t pad;
    uint32_t build;
} dm_fl_head;









enum { DM_FL_COMPS = 4, DM_FL_SPLIT_KNOWN = 1 };
typedef struct { uint16_t cls, pad; uint32_t dealt; } dm_fl_comp;
typedef struct {
    uint32_t mine, merc, rest, unknown;
    uint32_t skill[DM_FL_SKILLS], none, over;
    dm_fl_comp comp[DM_FL_COMPS];
    uint32_t gmine[DM_FL_GROUPS];
    uint32_t state;
    uint16_t t_act;
    uint8_t flags, pad;
    uint16_t gact[DM_FL_GROUPS];
} dm_fl_split;
typedef struct {
    dm_fl_head h;
    dm_fl_split s;
    dm_fl_group g[DM_FL_GROUPS];
    dm_fl_bucket b[DM_FL_BUCKETS];
    uint8_t glife[DM_FL_BUCKETS][DM_FL_GROUPS];
    dm_fl_mark m[DM_FL_MARKS];
} dm_fl_fight;
_Static_assert(sizeof(dm_fl_bucket) == 12 && sizeof(dm_fl_mark) == 6 && sizeof(dm_fl_group) == 24 && sizeof(dm_fl_head) == 64 && sizeof(dm_fl_split) == 144, "fight record layout");






enum { DM_FL_LAYS_POISON = 1, DM_FL_LAYS_BURN = 2 };
typedef struct { uint32_t enemy, cls; int32_t before, after; uint16_t su; uint8_t side, rank, crit, boss, merc, lays; int16_t skill; uint16_t by; uint8_t delayed; } dm_fl_hit;

typedef struct { int hp, hp_max, mp, mp_max, dead, town, area, difficulty; } dm_fl_player;


typedef struct dm_fl_state dm_fl_state;
dm_fl_state *dm_fl_core(void);
void dm_fl_reset(dm_fl_state *s);
int dm_fl_game(dm_fl_state *s, uint32_t tick);


unsigned dm_fl_games(void);
void dm_fl_discard(dm_fl_state *s);
int dm_fl_open(const dm_fl_state *s);
int dm_fl_flags(const dm_fl_state *s);
void dm_fl_begin(dm_fl_state *s, uint32_t tick, const dm_fl_player *p);



int dm_fl_known(const dm_fl_state *s, uint32_t id);
int dm_fl_on_hit(dm_fl_state *s, const dm_fl_hit *h, int life, int max_life);
void dm_fl_lost_hits(dm_fl_state *s, int n);

int dm_fl_on_dot(dm_fl_state *s, uint32_t enemy, uint32_t cls, uint8_t rank, uint16_t su, int boss, int life, int max_life);



int dm_fl_slots(const dm_fl_state *s);
uint32_t dm_fl_slot_id(const dm_fl_state *s, int i);
int dm_fl_slot_unexplained(const dm_fl_state *s, int i, int life);

enum { DM_FL_DOT_PLAYER, DM_FL_DOT_MERC, DM_FL_DOT_COMP, DM_FL_DOT_UNKNOWN, DM_FL_DOTS = 8 };
typedef struct { int n; struct { uint8_t kind, burn; uint16_t cls; int32_t rate; } e[DM_FL_DOTS]; } dm_fl_dot;


void dm_fl_slot_seen(dm_fl_state *s, int i, int gone, int dead, int attack, int life, int dot_ours);



void dm_fl_dot_write(dm_fl_state *s, uint32_t enemy, int before, int after, const dm_fl_dot *dot);


int dm_fl_members(const dm_fl_state *s);
uint32_t dm_fl_member_watch(const dm_fl_state *s, int m);
void dm_fl_member_seen(dm_fl_state *s, int m, int gone, int dead);
void dm_fl_action(dm_fl_state *s, uint32_t tick, int bits, int skill);


void dm_fl_engage(dm_fl_state *s, uint32_t enemy, int skill, int delayed);
void dm_fl_block(dm_fl_state *s, uint32_t tick);
void dm_fl_avoided(dm_fl_state *s, int what);
int dm_fl_elites_folded(const dm_fl_state *s);
void dm_fl_potion(dm_fl_state *s, uint32_t tick, int column, int cls);
void dm_fl_states(dm_fl_state *s, uint32_t tick, const uint32_t bits[DM_FL_STATE_WORDS]);

int dm_fl_end(dm_fl_state *s, int *kept);
const dm_fl_fight *dm_fl_last(const dm_fl_state *s);
int dm_fl_pending(dm_fl_state *s, dm_fl_fight *out);
int dm_fl_difficulty(const dm_fl_state *s);
void dm_fl_build(dm_fl_state *s, uint32_t key, uint32_t state);

void dm_fl_player_loss(const dm_fl_state *s, uint32_t *lost, uint32_t *hits);

int64_t dm_fl_split_gap(const dm_fl_state *s);


int dm_fl_kills(const dm_fl_state *s, uint32_t tick, const uint32_t **ids);

int dm_fl_hits(const dm_fl_state *s, uint32_t tick);
void dm_fl_record(dm_fl_state *s, int on);


int dm_fl_count(dm_fl_state *s);
int dm_fl_get(dm_fl_state *s, int newest, dm_fl_fight *out);





enum { DM_FL_MAGIC = 0x4c46444d, DM_FL_VERSION_1 = 1, DM_FL_VERSION = 2, DM_FL_IMAGE_HEAD = 16 };
#define DM_FL_IMAGE_MAX (DM_FL_IMAGE_HEAD + DM_FL_RING * (sizeof(dm_fl_head) + sizeof(dm_fl_split) + DM_FL_GROUPS * sizeof(dm_fl_group) + DM_FL_BUCKETS * (sizeof(dm_fl_bucket) + DM_FL_GROUPS) + DM_FL_MARKS * sizeof(dm_fl_mark)))
enum { DM_FL_BAD_SIZE = 1, DM_FL_BAD_MAGIC, DM_FL_BAD_VERSION, DM_FL_BAD_CRC, DM_FL_BAD_COUNT, DM_FL_BAD_FIGHT };


int dm_fl_owner(dm_fl_state *s, const char *who, const uint8_t *image, size_t n);
int dm_fl_owned_by(dm_fl_state *s, const char *who);


size_t dm_fl_image(dm_fl_state *s, char who[64], uint8_t *buf, size_t cap, uint32_t *gen);
void dm_fl_saved(dm_fl_state *s, uint32_t gen);
void dm_fl_unsaved(dm_fl_state *s);


int dm_fl_store_load(dm_fl_state *s, const char *root, const char *who);
int dm_fl_store_commit(dm_fl_state *s, const char *wd, uint64_t commits);

int dm_fl_store_follow(dm_fl_state *s, const char *root, const char *who);


void dm_fightlog_hit(const uint64_t *regs);


void dm_fightlog_obs_chain(int id, void (*fn)(const uint64_t *regs));
void dm_fightlog_tick(uint64_t G, uint64_t player, uint8_t *main_base, const dm_fl_player *p, const int belt[4], const uint64_t belt_item[4]);
