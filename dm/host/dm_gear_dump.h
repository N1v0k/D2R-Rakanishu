


#ifndef DM_GEAR_DUMP_H
#define DM_GEAR_DUMP_H
#include "dm_gear_search.h"
#include <stdint.h>

#define DM_GDUMP_MAGIC "D2RGEARD"
#define DM_GDUMP_FORMAT 1u

#define DM_GDUMP_KEEP 4


typedef struct {
    char magic[8];
    uint32_t format;
    uint32_t endian;
    uint32_t sz[12];
    uint32_t lim[12];
    uint64_t rev;
    uint64_t sim_hash;
    uint64_t gear_hash;
    uint64_t bytes;
    int32_t ncand, nweapon;
    int32_t threads, charms_fixed, has_seed;
    int32_t pad;
} dm_gdump_head;
enum { DG_SZ_PROBLEM, DG_SZ_CAND, DG_SZ_WEAPON, DG_SZ_GOAL, DG_SZ_PICK, DG_SZ_BASE, DG_SZ_CHAR, DG_SZ_ITEM, DG_SZ_LIST,
       DG_SZ_SIMCHAR, DG_SZ_SIMWEAPON, DG_SZ_ICCB, DG_SZ_N };
enum { DG_LIM_MAXSKILL, DG_LIM_MAXGRANT, DG_LIM_MAXBAR, DG_LIM_MAXCAND, DG_LIM_NSTAT, DG_LIM_MAXV, DG_LIM_GREENV, DG_LIM_MAXITEMS,
       DG_LIM_PLACES, DG_LIM_GRIDW, DG_LIM_GRIDH, DG_LIM_EL, DG_LIM_N };


uint64_t dm_gdump_sim_hash(const dm_sim_data *d);
uint64_t dm_gdump_gear_hash(const dm_gear_tables *t);




int dm_gdump_write(const char *dir, uint64_t rev, const dm_gear_problem *p, const dm_gear_goal *goal, int threads, const dm_gear_pick *seed);

void dm_gdump_result(const char *dir, uint64_t rev, const char *line);

typedef struct {
    dm_gdump_head head;
    dm_gear_problem p;
    dm_gear_cand *cand;
    dm_gear_weapon *weapon;
    dm_gear_goal goal;
    dm_gear_pick seed;
} dm_gdump;


int dm_gdump_read(const char *path, const dm_sim_data *d, const dm_gear_tables *t, dm_gdump *out);
void dm_gdump_free(dm_gdump *g);
#endif
