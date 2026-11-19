#pragma once
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

int xbg_vertex_input_classes(const uint32_t *words,size_t count,const uint32_t *locations,size_t inputs,uint32_t *classes);

int xbg_vertex_attributes_replace(uint32_t state[16],const uint32_t *attributes,int count);

int xbg_vertex_zero_binding(const uint32_t attributes[16],const uint32_t *locations,size_t count);
#ifdef __cplusplus
}
#endif
