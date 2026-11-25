


#ifndef DM_ITEMBUILD_H
#define DM_ITEMBUILD_H
#include <stdint.h>

typedef struct {


    uint64_t (*build)(void *ctx, const uint8_t *buf, int len, int *used, int *socks);

    int (*attach)(void *ctx, uint64_t parent, uint64_t filler);

    int (*loose)(void *ctx, uint64_t it);

    void (*release)(void *ctx, uint64_t it);
    void *ctx;
} dm_ib_ops;

#define DM_IB_AT 8
typedef struct {
    int want;
    int socked;
    int bytes;
    int failed;
    int at[DM_IB_AT];
} dm_ib_result;



uint64_t dm_ib_make(const dm_ib_ops *o, const uint8_t *buf, int len, int all, dm_ib_result *r);
#endif
