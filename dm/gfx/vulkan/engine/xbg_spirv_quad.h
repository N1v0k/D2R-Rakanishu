

#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

enum class xbg_quad_skip : uint32_t {
    malformed, module_contract, non_float, lane_index, control_flow,
    bit_pattern, integer_flow, unknown_source, unknown_use, memory_flow, count
};


enum class xbg_quad_call_skip : uint32_t { module_contract, helper, arguments, control_flow, source, use, count };
struct xbg_spirv_quad_stats {
    size_t candidates=0, rewritten=0, guarded_nonfinite=0;
    size_t skipped[static_cast<size_t>(xbg_quad_skip::count)]={};
    size_t calls=0, calls_rewritten=0;
    size_t call_skipped[static_cast<size_t>(xbg_quad_call_skip::count)]={};





    size_t calls_relaxed_float=0, calls_integer=0;
};
const char *xbg_spirv_quad_skip_name(xbg_quad_skip reason);
const char *xbg_spirv_quad_call_skip_name(xbg_quad_call_skip reason);





size_t xbg_spirv_quad_derivatives(const uint32_t *words,size_t count,std::vector<uint32_t> &out,
                                  xbg_spirv_quad_stats *stats);
size_t xbg_spirv_quad_derivatives(const uint32_t *words,size_t count,std::vector<uint32_t> &out);











size_t xbg_spirv_quad_derivatives(const uint32_t *words,size_t count,std::vector<uint32_t> &out,
                                  xbg_spirv_quad_stats *stats,bool relaxed);







size_t xbg_spirv_strip_subgroup(const uint32_t *words,size_t count,std::vector<uint32_t> &out);
