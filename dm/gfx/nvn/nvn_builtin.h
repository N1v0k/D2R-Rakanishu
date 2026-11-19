





















#pragma once

static const char *const k_nvn_builtin_msl =
    "#include <metal_stdlib>\n"
    "using namespace metal;\n"
    "#define HEAD uint id = (cta.x << 5) + tid.x; if (id >= n[0]) return;\n"
    "#define LD(o) (*(device const ulong *)(src + (o)))\n"
    "#define ARGS constant uint *n [[buffer(0)]], device const uchar *src [[buffer(1)]], device uchar *dst [[buffer(2)]], "
    "uint3 cta [[threadgroup_position_in_grid]], uint3 tid [[thread_position_in_threadgroup]]\n"
    "#define ST(v) (*(device ulong *)(dst + (id << 3)) = (v))\n"
    "kernel void q_val0(ARGS) { HEAD ST(LD(id << 4)); }\n"
    "kernel void q_val8(ARGS) { HEAD ST(LD((id << 4) + 8u)); }\n"
    "kernel void q_dlt0(ARGS) { HEAD uint b = id << 5; ST(LD(b + 16u) - LD(b)); }\n"
    "kernel void q_dlt8(ARGS) { HEAD uint b = (id << 5) + 16u; ST(LD(b + 8u) - LD(b - 8u)); }\n";



static const struct { const char *ctl; const char *fn; } k_nvn_builtin_map[] = {
    {"d0f769bb916bbba9", "q_val0"},
    {"16479cc6d75c2b28", "q_val8"},
    {"0687e2ef95133577", "q_dlt0"},
    {"4df5909e6dabee05", "q_dlt8"},
};
