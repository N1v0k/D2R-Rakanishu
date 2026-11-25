




#pragma once
#include <stddef.h>
#include <stdint.h>

enum {
    DM_SK_START = 8,
    DM_SK_KEPT_MIN = 6,
    DM_SK_WINDOW = 100,
    DM_SK_TIERS = 9,
    DM_SK_IDS = 4096,


    DM_SK_SEEN = 10,
    DM_SK_LATE = 1,
    DM_SK_CREDITS = 256,
};
enum { DM_SK_OFF, DM_SK_SHOW, DM_SK_BONUS };
enum { DM_SK_END_TIME = 1, DM_SK_END_DEATH };


typedef struct { int kind, pct, no_bonus; uint32_t kills, first, last; uint64_t base; } dm_sk_end;

struct dm_sk_seen { uint32_t id, t, amount; };
typedef struct {
    uint32_t kills, first, last; int no_bonus; uint64_t base;
    uint32_t due;
    uint32_t ids[2 * DM_SK_IDS]; int nids, ids_full;
    struct dm_sk_seen cred[DM_SK_CREDITS]; int ncred;
    struct dm_sk_seen wait[DM_SK_CREDITS]; int nwait;
    uint32_t dropped;
    dm_sk_end ended[2]; int nended;
} dm_sk_state;

void dm_sk_reset(dm_sk_state *s);

int dm_sk_kill(dm_sk_state *s, uint32_t tick, uint32_t id, int no_bonus);

void dm_sk_hits(dm_sk_state *s, uint32_t tick, int n);

void dm_sk_credit(dm_sk_state *s, uint32_t tick, uint32_t id, uint32_t amount);


int dm_sk_tick(dm_sk_state *s, uint32_t tick, int dead, dm_sk_end out[2]);
int dm_sk_pct(uint32_t kills);
int dm_sk_tier(uint32_t kills);
uint64_t dm_sk_bonus(uint64_t base, int pct);

int dm_sk_shown(const dm_sk_state *s, uint32_t tick, uint32_t *kills, int *left);



int dm_sk_tiers_text(char *out, size_t cap);


enum { DM_SK_RING = 20, DM_SK_DIFFS = 3 };


typedef struct { uint32_t kills, ticks, bonus; uint16_t area; uint8_t difficulty, end; } dm_sk_rec;
_Static_assert(sizeof(dm_sk_rec) == 16, "streak record layout");
typedef struct dm_sk_hist dm_sk_hist;
dm_sk_hist *dm_sk_hist_core(void);


int dm_sk_hist_add(dm_sk_hist *h, const dm_sk_rec *r);

int dm_sk_hist_keep(dm_sk_hist *h, const dm_sk_rec *r, int recording);
int dm_sk_hist_count(dm_sk_hist *h);
int dm_sk_hist_get(dm_sk_hist *h, int newest, dm_sk_rec *out);
int dm_sk_hist_record(dm_sk_hist *h, int difficulty, dm_sk_rec *out);


enum { DM_SK_MAGIC = 0x4b53444d, DM_SK_VERSION = 1, DM_SK_IMAGE_HEAD = 16 };
#define DM_SK_IMAGE_MAX (DM_SK_IMAGE_HEAD + (DM_SK_DIFFS + DM_SK_RING) * sizeof(dm_sk_rec))
enum { DM_SK_BAD_SIZE = 1, DM_SK_BAD_MAGIC, DM_SK_BAD_VERSION, DM_SK_BAD_CRC, DM_SK_BAD_COUNT, DM_SK_BAD_RECORD };


int dm_sk_hist_owner(dm_sk_hist *h, const char *who, const uint8_t *image, size_t n);
int dm_sk_hist_owned_by(dm_sk_hist *h, const char *who);

int dm_sk_hist_loaded(dm_sk_hist *h, const char *who);

size_t dm_sk_hist_image(dm_sk_hist *h, char who[64], uint8_t *buf, size_t cap, uint32_t *gen);
void dm_sk_hist_saved(dm_sk_hist *h, uint32_t gen);
void dm_sk_hist_unsaved(dm_sk_hist *h);



int dm_sk_hist_load(dm_sk_hist *h, const char *root, const char *who);
int dm_sk_hist_follow(dm_sk_hist *h, const char *root, const char *who);
int dm_sk_hist_commit(dm_sk_hist *h, const char *wd, uint64_t commits);


int dm_sk_hist_text(dm_sk_hist *h, char *out, size_t cap);




void dm_streak_tick(uint64_t G, uint64_t player, uint8_t *main_base, int dead, int area, int difficulty);


enum { DM_SK_HUD_TICK, DM_SK_HUD_KILLS, DM_SK_HUD_LEFT, DM_SK_HUD_WINDOW, DM_SK_HUD_PCT, DM_SK_HUD_END_SEQ, DM_SK_HUD_END_KILLS, DM_SK_HUD_END_BONUS,
       DM_SK_HUD_END_KIND, DM_SK_HUD_END_RECORD, DM_SK_HUD_TIER, DM_SK_HUD_END_TIER, DM_SK_HUD_N };
int dm_streak_hud(int *out, int max);

int dm_streak_text(char *out, size_t cap);
