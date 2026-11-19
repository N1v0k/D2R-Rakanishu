




















#ifndef MTL_BACKEND_H
#define MTL_BACKEND_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mtlb_ctx mtlb_ctx;
typedef struct mtlb_rt  mtlb_rt;

typedef enum {
    MTLB_FMT_BGRA8 = 0,
    MTLB_FMT_RGBA8 = 1,
} mtlb_fmt;

mtlb_ctx *mtlb_init(void);
void      mtlb_shutdown(mtlb_ctx *ctx);

mtlb_rt  *mtlb_target(mtlb_ctx *ctx, uint32_t w, uint32_t h, mtlb_fmt fmt);
void      mtlb_target_free(mtlb_rt *rt);

int mtlb_clear(mtlb_ctx *ctx, mtlb_rt *rt, float r, float g, float b, float a);
int mtlb_triangle(mtlb_ctx *ctx, mtlb_rt *rt);


int mtlb_readback(mtlb_ctx *ctx, mtlb_rt *rt, void *buf, size_t buf_size);

#ifdef __cplusplus
}
#endif
#endif
