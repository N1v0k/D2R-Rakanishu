#pragma once
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif





typedef struct xbg_draw_call {
    int32_t primitive,index_type,first,count,instances,base_instance,base_vertex;
    uint64_t index_address;
} xbg_draw_call;
int xbg_vertex_buffer_range(int32_t first,int32_t count);
int xbg_draw_arrays_plan(xbg_draw_call *out,int32_t primitive,int32_t first,
    int32_t count,int32_t base_instance,int32_t instances);
int xbg_draw_indirect_plan(xbg_draw_call *out,int32_t primitive,int32_t index_type,
    uint64_t index_base,const void *arguments,size_t argument_bytes);
int xbg_draw_indirect_address(uint64_t base,uint32_t element,int64_t stride,
    int32_t index_type,uint64_t *out);
#ifdef __cplusplus
}
#endif
