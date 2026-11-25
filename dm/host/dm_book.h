






#pragma once
#include <stddef.h>
#include <stdint.h>

struct dm_sim_data;

enum { DM_BK_KINDS = 2048, DM_BK_DIFFS = 3, DM_BK_OWNER = 64 };
enum { DM_BK_MAGIC = 0x4b42444d, DM_BK_VERSION = 1, DM_BK_HEAD = 20 };
enum { DM_BK_F_BACKFILLED = 1 };
#define DM_BK_IMAGE_MAX (DM_BK_HEAD + DM_BK_OWNER + DM_BK_KINDS * sizeof(dm_bk_rec))
enum { DM_BK_BAD_SIZE = 1, DM_BK_BAD_MAGIC, DM_BK_BAD_VERSION, DM_BK_BAD_CRC, DM_BK_BAD_COUNT, DM_BK_BAD_OWNER, DM_BK_BAD_RECORD, DM_BK_NEWER, DM_BK_FOREIGN,
       DM_BK_UNREAD };
enum { DM_BK_MODE_OWN, DM_BK_MODE_NEWER, DM_BK_MODE_UNREAD };


typedef struct { uint8_t type, pad; uint16_t idx; uint32_t n[DM_BK_DIFFS]; } dm_bk_rec;
typedef struct { dm_bk_rec rec[DM_BK_KINDS]; int nrec; uint32_t flags, gen; } dm_bk;



int dm_bk_key(const char *key, uint8_t *type, uint16_t *idx);

int dm_bk_add(dm_bk *b, uint8_t type, uint16_t idx, int diff, uint32_t n);
int dm_bk_at_least(dm_bk *b, uint8_t type, uint16_t idx, int diff, uint32_t n);
uint32_t dm_bk_count(const dm_bk *b, uint8_t type, uint16_t idx, int diff);

size_t dm_bk_image(const dm_bk *b, const char *who, uint8_t *buf, size_t cap);
int dm_bk_parse(const uint8_t *image, size_t n, const char *who, dm_bk *out);

int dm_bk_text(const dm_bk *b, int mode, char *out, size_t cap);



typedef struct { int cls, su; const char *name; int diff; uint32_t n; } dm_bk_seen;


int dm_bk_kind_of(const struct dm_sim_data *d, const dm_bk_seen *s, uint8_t *type, uint16_t *idx);


int dm_bk_backfill(dm_bk *b, const struct dm_sim_data *d, const dm_bk_seen *board, int nboard, const dm_bk_seen *fights, int nfights, int *skipped);



void dm_book_tick(uint64_t G, uint64_t player, uint8_t *main_base);

int dm_book_text(char *out, size_t cap);
