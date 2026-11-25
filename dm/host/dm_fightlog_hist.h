



#pragma once
#include "dm_fightlog_text.h"

enum { DM_DH_SEGS = 500, DM_DH_WINDOWS = 8, DM_DH_WINDOW = 50, DM_DH_SRCS = 12, DM_DH_MIN = 5, DM_DH_MIN_TICKS = DM_FL_TPS };

enum { DM_DH_SRC_COMP = 0x8000, DM_DH_SRC_MERC = 0xfff0, DM_DH_SRC_REST, DM_DH_SRC_UNKNOWN, DM_DH_SRC_NONE, DM_DH_SRC_OTHER };
typedef struct { uint16_t id, pad; uint32_t dealt; } dm_dh_src;

typedef struct { uint32_t mine, team, ticks; uint8_t nsrc, pad[3]; dm_dh_src src[DM_DH_SRCS]; } dm_dh_fight;
typedef struct { uint32_t key; dm_fl_parts parts; uint32_t n, next; dm_dh_fight f[DM_DH_WINDOW]; } dm_dh_window;

typedef struct { uint32_t key; dm_fl_parts parts; uint32_t fights, ticks, value, lo, hi; } dm_dh_seg;

typedef struct dm_dh dm_dh;
dm_dh *dm_dh_core(void);
void dm_dh_reset(dm_dh *h);

void dm_dh_add(dm_dh *h, uint32_t key, const dm_fl_parts *parts, const dm_fl_fight *f);

int dm_dh_read(dm_dh *h, uint32_t key, dm_dh_window *w, dm_dh_seg *segs);


double dm_dh_value(const dm_dh_fight *f, int n, int team);


int dm_dh_range(const dm_dh_fight *f, int n, int team, int src, double *lo, double *hi);





enum { DM_DH_MAGIC = 0x4844444d, DM_DH_VERSION = 1, DM_DH_HEAD = 16 };
#define DM_DH_IMAGE_MAX (DM_DH_HEAD + 8 + DM_DH_SEGS * sizeof(dm_dh_seg) + DM_DH_WINDOWS * sizeof(dm_dh_window))
enum { DM_DH_BAD_SIZE = 1, DM_DH_BAD_MAGIC, DM_DH_BAD_VERSION, DM_DH_BAD_CRC, DM_DH_BAD_COUNT, DM_DH_BAD_RECORD };
int dm_dh_owner(dm_dh *h, const char *who, const uint8_t *image, size_t n);
size_t dm_dh_image(dm_dh *h, const dm_fl_fight *pend, const dm_fl_parts *parts, char who[64], uint8_t *buf, size_t cap, uint32_t *gen);
void dm_dh_saved(dm_dh *h, uint32_t gen);
void dm_dh_unsaved(dm_dh *h);
int dm_dh_store_follow(dm_dh *h, const char *root, const char *who);
int dm_dh_store_commit(dm_dh *h, const char *wd, uint64_t commits, const dm_fl_fight *pend, const dm_fl_parts *parts);
