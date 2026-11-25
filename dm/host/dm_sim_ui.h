#ifndef DM_SIM_UI_H
#define DM_SIM_UI_H
#include "dm_sim_core.h"

typedef struct {
    dm_sim_char c;
    int valid, session, act, area, rift, rush, next_boss, set, players;


    int ref_area;
    int next_boss_by_diff[3], item_charges;
    int elysium;
    char name[48];
    uint32_t gear;
    int magic_find;
} dm_sim_snapshot;


static inline uint32_t dm_sim_gear_stat(uint32_t key, int32_t value) {
    uint32_t st = key >> 16, v = (uint32_t)value;
    if (st == 70 || st == 72) return 0;
    if (st == 204) v &= ~0xffu;
    uint32_t h = (2166136261u ^ key) * 16777619u;
    return (h ^ v) * 16777619u;
}



typedef struct {
    uint32_t cls;
    int mode, page, slot, charm, counted;
    uint32_t stats;
} dm_sim_gear_item;
static inline int dm_sim_gear_counts(const dm_sim_gear_item *it) { return it->mode == 1 || (it->mode == 0 && it->page == 0 && it->charm && it->counted); }

static inline uint32_t dm_sim_gear_add(uint32_t h, const dm_sim_gear_item *it) {
    if (!dm_sim_gear_counts(it)) return h;
    uint32_t own = ((2166136261u ^ (it->cls << 8 | (it->mode == 1 ? (uint32_t)it->slot & 0xff : 0))) * 16777619u) + it->stats;
    return h + own * 2654435761u;
}
typedef struct {
    int diff, players, potions, terror, herald, target, ordered;
    int order[DM_SIM_MAXBAR], norder;
    int tz_min, tz_max, tz_boost, herald_hp, herald_damage;
    char excluded[1024][24]; int nexcluded;
} dm_sim_options;
typedef int (*dm_sim_reader)(const dm_sim_data *, void *);
int dm_sim_read(const char *set, dm_sim_reader reader, void *ctx);
int dm_sim_try_read(const char *set, dm_sim_reader reader, void *ctx);


const dm_sim_data *dm_sim_pin(const char *set);
void dm_sim_unpin(const dm_sim_data *d);

const dm_sim_data *dm_sim_try_pin(const char *set, int *busy);

unsigned dm_sim_generation(void);

int dm_sim_is_town(int area);



typedef struct {unsigned gen;char name[48];int last[6];} dm_sim_ref;
int dm_sim_ref_capture(dm_sim_ref *r,unsigned gen,const char *name,int act,int area);
void dm_sim_publish(const dm_sim_snapshot *snapshot);
int dm_sim_snapshot_copy(dm_sim_snapshot *out);
void dm_sim_live_pump(uint8_t *b, uint64_t pl);
int dm_sim_ui_report(const dm_sim_data *d, const dm_sim_snapshot *snap, const dm_sim_options *opt, char *out, size_t cap);




int dm_sim_kind_key(const dm_sim_data *d, int cls, int su, char *out, size_t cap);
int dm_sim_ui_query(const dm_sim_options *opt, char *out, size_t cap);
int dm_sim_next_boss(const uint16_t words[48]);

enum { DM_SIM_QUEST_BYTES = 96 };

int dm_sim_quest_words(const uint8_t *buffer, uint32_t bits, uint16_t out[48]);

void dm_sim_quest_publish(const char *name, const uint16_t words[3][48], const int state[3]);

void dm_sim_quest_next(const char *name, int out[3]);

int dm_sim_quest_copy(const char *name, uint16_t words[3][48], int state[3]);
const char *dm_sim_skill_tooltip(int id);
void dm_sim_bar_add(dm_sim_char *c, int id);

void dm_sim_bar_bind(dm_sim_char *c, int id);



uint64_t dm_sim_tip_key(const dm_sim_data *d, int diff, int area, int clvl, uint64_t key);


int dm_sim_skill_preview(const dm_sim_data *d, const dm_sim_char *c, const dm_sim_target *t, int id, int own_level, int next_level, int german, char *out, size_t cap);
#endif
