



















#include "xbg_spirv_dispatch.h"

namespace {
enum : uint32_t {
    OpEntryPoint = 15, OpExecutionMode = 16, OpTypeVoid = 19, OpTypeBool = 20, OpTypeInt = 21, OpTypeVector = 23,
    OpTypePointer = 32, OpConstant = 43, OpConstantComposite = 44, OpFunction = 54, OpVariable = 59, OpLoad = 61,
    OpDecorate = 71, OpIAdd = 128, OpIMul = 132, OpAny = 154, OpLogicalOr = 166, OpUGreaterThanEqual = 174, OpSelectionMerge = 247, OpLabel = 248,
    OpBranchConditional = 250, OpReturn = 253, OpExecutionModeId = 331,
    ModelGLCompute = 5, ModeLocalSize = 17, ModeLocalSizeId = 38, DecBuiltIn = 11, StorageInput = 1,
    BuiltInNumWorkgroups = 24, BuiltInWorkgroupSize = 25, BuiltInWorkgroupId = 26, BuiltInLocalInvocationId = 27,
};
inline uint32_t inst(uint32_t op, uint32_t words) { return (words << 16) | op; }


bool is_global_decl(uint32_t op) { return (op >= 19 && op <= 39) || (op >= 41 && op <= 52) || op == OpVariable || op == 1 || op == 322; }
}

size_t xbg_spirv_dispatch_guard(const uint32_t *in, size_t n, std::vector<uint32_t> &out) {
    out.clear();
    if (!in || n < 5 || in[0] != 0x07230203u) return 0;
    uint32_t bound = in[3];


    static const uint32_t builtin[3] = { BuiltInNumWorkgroups, BuiltInWorkgroupId, BuiltInLocalInvocationId };
    uint32_t var[3] = { 0, 0, 0 }, var_type[3] = { 0, 0, 0 }, var_storage[3] = { 0, 0, 0 };
    size_t entry_at = 0, first_decl = 0, first_func = 0;
    uint32_t entry = 0, entries = 0, local[3] = { 0, 0, 0 };
    bool have_local = false;
    uint32_t t_void = 0, t_bool = 0, t_uint = 0, t_v3uint = 0, t_v3bool = 0;


    std::vector<uint32_t> in_ptr;



    uint32_t size_constant = 0;
    bool size_constant_read = false;
    std::vector<uint32_t> constant_value(bound, 0);
    std::vector<uint8_t> is_constant(bound, 0);
    for (size_t i = 5; i < n;) {
        const uint32_t wc = in[i] >> 16, op = in[i] & 0xffff;
        if (!wc || i + wc > n) return 0;
        if (op == OpFunction) { first_func = i; break; }
        if (!first_decl && is_global_decl(op)) first_decl = i;
        if (op == OpEntryPoint && wc >= 4) {
            ++entries;
            if (in[i + 1] == ModelGLCompute) { entry = in[i + 2]; entry_at = i; }
        } else if (op == OpExecutionMode && wc >= 3 && in[i + 1] == entry && entry) {
            if (in[i + 2] == ModeLocalSize && wc == 6) { local[0] = in[i + 3]; local[1] = in[i + 4]; local[2] = in[i + 5]; have_local = true; }
        } else if (op == OpExecutionModeId && wc >= 3 && in[i + 2] == ModeLocalSizeId) {
            return 0;
        } else if (op == OpDecorate && wc == 4 && in[i + 2] == DecBuiltIn) {
            if (in[i + 3] == BuiltInWorkgroupSize) { if (size_constant) return 0; size_constant = in[i + 1]; }
            for (int k = 0; k < 3; k++) if (in[i + 3] == builtin[k]) { if (var[k]) return 0; var[k] = in[i + 1]; }
        } else if (op == OpTypeVoid && wc == 2) t_void = in[i + 1];
        else if (op == OpTypeBool && wc == 2) t_bool = in[i + 1];
        else if (op == OpTypeInt && wc == 4 && in[i + 2] == 32 && in[i + 3] == 0) t_uint = in[i + 1];
        else if (op == OpTypeVector && wc == 4 && in[i + 3] == 3) {
            if (t_uint && in[i + 2] == t_uint) t_v3uint = in[i + 1];
            else if (t_bool && in[i + 2] == t_bool) t_v3bool = in[i + 1];
        } else if (op == OpTypePointer && wc == 4 && in[i + 2] == StorageInput && t_v3uint && in[i + 3] == t_v3uint) in_ptr.push_back(in[i + 1]);
        else if (op == OpVariable && wc >= 4) {
            for (int k = 0; k < 3; k++) if (var[k] && in[i + 2] == var[k]) { var_type[k] = in[i + 1]; var_storage[k] = in[i + 3]; }
        } else if (op == OpConstant && wc == 4 && t_uint && in[i + 1] == t_uint && in[i + 2] < bound) {
            constant_value[in[i + 2]] = in[i + 3];
            is_constant[in[i + 2]] = 1;
        } else if (size_constant && op >= 41 && op <= 52 && wc >= 3 && in[i + 2] == size_constant) {


            if (op != OpConstantComposite || wc != 6 || !t_v3uint || in[i + 1] != t_v3uint) return 0;
            for (int k = 0; k < 3; k++) {
                const uint32_t c = in[i + 3 + k];
                if (c >= bound || !is_constant[c]) return 0;
                local[k] = constant_value[c];
            }
            size_constant_read = true;
        }
        i += wc;
    }
    if (size_constant && !size_constant_read) return 0;
    if (entries != 1 || !entry || !(have_local || size_constant_read) || !local[0] || !local[1] || !local[2]) return 0;
    if (!first_decl || !first_func || !(entry_at < first_decl && first_decl < first_func) || !t_void) return 0;


    for (int k = 0; k < 3; k++) {
        if (!var[k]) continue;
        bool ok = var_storage[k] == StorageInput;
        if (ok) { ok = false; for (uint32_t p : in_ptr) if (p == var_type[k]) ok = true; }
        if (!ok) return 0;
    }


    size_t func_at = 0;
    for (size_t i = first_func; i < n;) {
        const uint32_t wc = in[i] >> 16, op = in[i] & 0xffff;
        if (!wc || i + wc > n) return 0;
        if (op == OpFunction && wc == 5 && in[i + 2] == entry) { func_at = i; break; }
        i += wc;
    }
    if (!func_at || in[func_at + 1] != t_void) return 0;
    const size_t label_at = func_at + 5;
    if (label_at + 2 > n || in[label_at] != inst(OpLabel, 2)) return 0;
    const uint32_t entry_label = in[label_at + 1];
    size_t vars_end = label_at + 2;
    while (vars_end < n && (in[vars_end] & 0xffff) == OpVariable) {
        const uint32_t wc = in[vars_end] >> 16;
        if (!wc || vars_end + wc > n) return 0;
        vars_end += wc;
    }


    auto fresh = [&]() { return bound++; };
    const bool need_uint = !t_uint, need_bool = !t_bool;
    if (need_uint) t_uint = fresh();
    if (need_bool) t_bool = fresh();
    const bool need_v3uint = !t_v3uint, need_v3bool = !t_v3bool;
    if (need_v3uint) t_v3uint = fresh();
    if (need_v3bool) t_v3bool = fresh();
    const bool need_ptr = in_ptr.empty();
    const uint32_t t_ptr = need_ptr ? fresh() : in_ptr[0];
    bool created[3] = { false, false, false };
    uint32_t added = 0;
    for (int k = 0; k < 3; k++) if (!var[k]) { var[k] = fresh(); created[k] = true; ++added; }
    const uint32_t c_x = fresh(), c_y = fresh(), c_z = fresh(), c_local = fresh();
    const uint32_t v_num = fresh(), v_group = fresh(), v_local = fresh(), b_group = fresh(), b_local = fresh(), b_or = fresh(), b_any = fresh();
    const uint32_t v_base = fresh(), v_pos = fresh(), v_limit = fresh(), b_pos = fresh(), b_or2 = fresh();
    const uint32_t new_label = fresh(), return_label = fresh();
    const uint32_t entry_wc = in[entry_at] >> 16;
    if (entry_wc + added > 0xffff) return 0;

    out.reserve(n + 96);
    out.insert(out.end(), in, in + entry_at);


    out.push_back(inst(OpEntryPoint, entry_wc + added));
    out.insert(out.end(), in + entry_at + 1, in + entry_at + entry_wc);
    for (int k = 0; k < 3; k++) if (created[k]) out.push_back(var[k]);
    out.insert(out.end(), in + entry_at + entry_wc, in + first_decl);
    for (int k = 0; k < 3; k++) if (created[k]) { const uint32_t d[4] = { inst(OpDecorate, 4), var[k], DecBuiltIn, builtin[k] }; out.insert(out.end(), d, d + 4); }
    out.insert(out.end(), in + first_decl, in + first_func);
    auto emit = [&](std::initializer_list<uint32_t> w) { out.insert(out.end(), w); };
    if (need_uint) emit({ inst(OpTypeInt, 4), t_uint, 32, 0 });
    if (need_bool) emit({ inst(OpTypeBool, 2), t_bool });
    if (need_v3uint) emit({ inst(OpTypeVector, 4), t_v3uint, t_uint, 3 });
    if (need_v3bool) emit({ inst(OpTypeVector, 4), t_v3bool, t_bool, 3 });
    if (need_ptr) emit({ inst(OpTypePointer, 4), t_ptr, StorageInput, t_v3uint });
    for (int k = 0; k < 3; k++) if (created[k]) emit({ inst(OpVariable, 4), t_ptr, var[k], StorageInput });
    emit({ inst(OpConstant, 4), t_uint, c_x, local[0] });
    emit({ inst(OpConstant, 4), t_uint, c_y, local[1] });
    emit({ inst(OpConstant, 4), t_uint, c_z, local[2] });
    emit({ inst(OpConstantComposite, 6), t_v3uint, c_local, c_x, c_y, c_z });
    out.insert(out.end(), in + first_func, in + label_at);



    emit({ inst(OpLabel, 2), new_label });
    out.insert(out.end(), in + label_at + 2, in + vars_end);
    emit({ inst(OpLoad, 4), t_v3uint, v_num, var[0] });
    emit({ inst(OpLoad, 4), t_v3uint, v_group, var[1] });
    emit({ inst(OpLoad, 4), t_v3uint, v_local, var[2] });
    emit({ inst(OpUGreaterThanEqual, 5), t_v3bool, b_group, v_group, v_num });
    emit({ inst(OpUGreaterThanEqual, 5), t_v3bool, b_local, v_local, c_local });
    emit({ inst(OpLogicalOr, 5), t_v3bool, b_or, b_group, b_local });






    emit({ inst(OpIMul, 5), t_v3uint, v_base, v_group, c_local });
    emit({ inst(OpIAdd, 5), t_v3uint, v_pos, v_base, v_local });
    emit({ inst(OpIMul, 5), t_v3uint, v_limit, v_num, c_local });
    emit({ inst(OpUGreaterThanEqual, 5), t_v3bool, b_pos, v_pos, v_limit });
    emit({ inst(OpLogicalOr, 5), t_v3bool, b_or2, b_or, b_pos });
    emit({ inst(OpAny, 4), t_bool, b_any, b_or2 });
    emit({ inst(OpSelectionMerge, 3), entry_label, 0 });
    emit({ inst(OpBranchConditional, 4), b_any, return_label, entry_label });
    emit({ inst(OpLabel, 2), return_label });
    emit({ inst(OpReturn, 1) });
    emit({ inst(OpLabel, 2), entry_label });
    out.insert(out.end(), in + vars_end, in + n);
    out[3] = bound;
    return 1;
}
