
















#include "xbg_spirv_warp.h"

namespace {
enum : uint32_t {
    OpEntryPoint = 15, OpCapability = 17, OpTypeBool = 20, OpTypeInt = 21, OpTypeVector = 23, OpTypePointer = 32,
    OpConstantTrue = 41, OpConstant = 43, OpFunction = 54, OpVariable = 59, OpLoad = 61, OpAccessChain = 65,
    OpInBoundsAccessChain = 66, OpDecorate = 71, OpCompositeExtract = 81, OpBitcast = 124, OpSelect = 169,
    OpIEqual = 170, OpINotEqual = 171, OpShiftRightLogical = 194, OpBitwiseOr = 197, OpBitwiseAnd = 199,
    OpGroupNonUniformFirst = 333, OpGroupNonUniformAll = 334, OpGroupNonUniformAny = 335, OpGroupNonUniformBallot = 339,
    OpGroupNonUniformShuffle = 345, OpGroupNonUniformLast = 366, OpSubgroupKHRFirst = 4421, OpSubgroupKHRLast = 4432,
    CapGroupNonUniformBallot = 64, DecBuiltIn = 11, StorageInput = 1, ModelGLCompute = 5,
    BuiltInSubgroupSize = 36, BuiltInNumSubgroups = 38, BuiltInSubgroupId = 40, BuiltInSubgroupLocalInvocationId = 41,
    BuiltInSubgroupEqMask = 4416, BuiltInSubgroupLtMask = 4420,
};


enum Kind : uint8_t { Other, LaneLoad, BallotDef, MaskLoad, MaskChain, WordExtract, ChainLoad, VoteAll, VoteAny, Shuffle };
inline uint32_t inst(uint32_t op, uint32_t words) { return (words << 16) | op; }
bool is_global_decl(uint32_t op) { return (op >= 19 && op <= 39) || (op >= 41 && op <= 52) || op == OpVariable || op == 1                              || op == 322                                                ; }



bool literal_at(uint32_t op, uint32_t w) {
    switch (op) {
    case 8: return w >= 2;
    case 12: return w == 4;
    case OpVariable: return w == 3;
    case OpLoad: return w >= 4;
    case 62: case 63: return w >= 3;
    case 79: case 82: return w >= 5;
    case OpCompositeExtract: return w >= 4;
    case 246: return w >= 3;
    case 247: return w >= 2;
    case 250: return w >= 4;
    case 251: return w >= 3;
    case 99: return w >= 4;
    default: return op >= 87 && op <= 98 && w >= 5;
    }
}
}

size_t xbg_spirv_warp32(const uint32_t *in, size_t n, uint32_t subgroup_size, std::vector<uint32_t> &out) {
    out.clear();
    if (!in || n < 5 || in[0] != 0x07230203u || (subgroup_size != 64 && subgroup_size != 128)) return 0;
    const uint32_t words = (subgroup_size + 31) / 32;
    uint32_t bound = in[3];


    struct Type { uint32_t op, a, b; };
    std::vector<Type> types(bound, Type{0, 0, 0});
    std::vector<uint32_t> var_type(bound, 0);
    std::vector<uint8_t> builtin(bound, 0);
    std::vector<uint8_t> zero(bound, 0);
    size_t first_cap = 0, entry = 0, first_type = 0, first_func = 0, entry_count = 0;
    bool has_ballot_cap = false;
    uint32_t lane_var = 0, t_bool = 0, t_uint = 0, t_v4u = 0, t_ptr_in_uint = 0, c_true = 0, c_u0 = 0, c_u5 = 0, c_u31 = 0, c_u96 = 0;
    uint32_t c_word[4] = {};
    for (size_t i = 5; i < n;) {
        uint32_t wc = in[i] >> 16, op = in[i] & 0xffff;
        if (!wc || i + wc > n) return 0;
        if (op == OpFunction && !first_func) first_func = i;
        if (!first_func) {
            if (op == OpCapability) { if (!first_cap) first_cap = i; if (in[i + 1] == CapGroupNonUniformBallot) has_ballot_cap = true; }
            else if (op == OpEntryPoint) { if (wc < 4 || in[i + 1] != ModelGLCompute) return 0; ++entry_count; if (!entry) entry = i; }
            else if (op == OpDecorate && wc >= 4 && in[i + 2] == DecBuiltIn && in[i + 1] < bound) {
                uint32_t b = in[i + 3];
                if (b == BuiltInSubgroupSize || b == BuiltInNumSubgroups || b == BuiltInSubgroupId) return 0;
                if (b == BuiltInSubgroupLocalInvocationId) { lane_var = in[i + 1]; builtin[lane_var] = 1; }
                else if (b >= BuiltInSubgroupEqMask && b <= BuiltInSubgroupLtMask) builtin[in[i + 1]] = 2;
            }
            if (is_global_decl(op) && !first_type) first_type = i;
            if (op == OpTypeBool && wc == 2) t_bool = in[i + 1];
            else if (op == OpTypeInt && wc == 4 && in[i + 1] < bound) { types[in[i + 1]] = {op, in[i + 2], in[i + 3]}; if (in[i + 2] == 32 && !in[i + 3] && !t_uint) t_uint = in[i + 1]; }
            else if (op == OpTypeVector && wc == 4 && in[i + 1] < bound) { types[in[i + 1]] = {op, in[i + 2], in[i + 3]}; if (t_uint && in[i + 2] == t_uint && in[i + 3] == 4 && !t_v4u) t_v4u = in[i + 1]; }
            else if (op == OpTypePointer && wc == 4 && in[i + 1] < bound) { types[in[i + 1]] = {op, in[i + 2], in[i + 3]}; if (in[i + 2] == StorageInput && t_uint && in[i + 3] == t_uint && !t_ptr_in_uint) t_ptr_in_uint = in[i + 1]; }
            else if (op == OpConstantTrue && wc == 3 && t_bool && in[i + 1] == t_bool && !c_true) c_true = in[i + 2];
            else if (op == OpConstant && wc == 4 && in[i + 1] < bound && in[i + 2] < bound && types[in[i + 1]].op == OpTypeInt && types[in[i + 1]].a == 32) {
                uint32_t id = in[i + 2], v = in[i + 3];
                if (!v) zero[id] = 1;
                if (in[i + 1] == t_uint) { if (v >= 1 && v <= 3 && !c_word[v]) c_word[v] = id; if (v == 0 && !c_u0) c_u0 = id; if (v == 5 && !c_u5) c_u5 = id; if (v == 31 && !c_u31) c_u31 = id; if (v == 96 && !c_u96) c_u96 = id; }
            }
            else if (op == OpVariable && wc >= 4 && in[i + 2] < bound) var_type[in[i + 2]] = in[i + 1];
        }
        i += wc;
    }
    if (!entry || !first_cap || !first_type || !first_func || !t_uint) return 0;
    uint32_t lane_t = t_uint;
    if (lane_var) {
        uint32_t p = var_type[lane_var] < bound ? var_type[lane_var] : 0;
        lane_t = p && types[p].op == OpTypePointer ? types[p].b : 0;
        if (!lane_t || lane_t >= bound || types[lane_t].op != OpTypeInt || types[lane_t].a != 32) return 0;
    }



    std::vector<uint8_t> val(bound, 0);
    std::vector<uint32_t> chain_mask(bound, 0);
    for (size_t i = first_func; i < n;) {
        uint32_t wc = in[i] >> 16, op = in[i] & 0xffff;
        if (op == OpGroupNonUniformBallot && wc == 5 && in[i + 2] < bound) val[in[i + 2]] = 1;
        else if (op == OpLoad && wc >= 4 && in[i + 2] < bound && in[i + 3] < bound && builtin[in[i + 3]] == 2) val[in[i + 2]] = 2;
        else if ((op == OpAccessChain || op == OpInBoundsAccessChain) && wc >= 4 && in[i + 2] < bound && in[i + 3] < bound && builtin[in[i + 3]] == 2) { val[in[i + 2]] = 3; chain_mask[in[i + 2]] = in[i + 3]; }
        i += wc;
    }


    struct Site { size_t at; Kind kind; };
    std::vector<Site> sites;
    bool votes = false;
    for (size_t i = first_func; i < n;) {
        uint32_t wc = in[i] >> 16, op = in[i] & 0xffff;
        Kind k = Other;
        auto tracked = [&](uint32_t id) { return id < bound && (val[id] || builtin[id] || (lane_var && id == lane_var)); };
        if (op == OpLoad && wc >= 4 && lane_var && in[i + 3] == lane_var) k = LaneLoad;
        else if (op == OpLoad && wc >= 4 && in[i + 3] < bound && val[in[i + 3]] == 3) k = ChainLoad;
        else if (op == OpLoad && wc >= 4 && in[i + 3] < bound && builtin[in[i + 3]] == 2) k = MaskLoad;
        else if ((op == OpAccessChain || op == OpInBoundsAccessChain) && wc >= 4 && in[i + 3] < bound && builtin[in[i + 3]] == 2) {
            if (wc != 5 || in[i + 4] >= bound || !zero[in[i + 4]]) return 0;
            k = MaskChain;
        }
        else if (op == OpCompositeExtract && wc >= 4 && in[i + 3] < bound && (val[in[i + 3]] == 1 || val[in[i + 3]] == 2)) {
            if (wc != 5 || in[i + 4] != 0) return 0;
            k = WordExtract;
        }
        else if (op == OpGroupNonUniformBallot && wc == 5) k = BallotDef;
        else if (op == OpGroupNonUniformAll && wc == 5) k = VoteAll;
        else if (op == OpGroupNonUniformAny && wc == 5) k = VoteAny;
        else if (op == OpGroupNonUniformShuffle && wc == 6) k = Shuffle;
        else if ((op >= OpGroupNonUniformFirst && op <= OpGroupNonUniformLast) || (op >= OpSubgroupKHRFirst && op <= OpSubgroupKHRLast)) return 0;


        for (uint32_t w = 1; w < wc; w++) {
            if (!tracked(in[i + w]) || literal_at(op, w)) continue;
            bool ok = (w == 2 && (k == BallotDef || k == MaskLoad || k == MaskChain)) ||
                      (w == 3 && (k == LaneLoad || k == ChainLoad || k == MaskLoad || k == MaskChain || k == WordExtract));
            if (!ok) return 0;
        }
        if (k == LaneLoad || k == WordExtract || k == ChainLoad || k == VoteAll || k == VoteAny || k == Shuffle) sites.push_back({i, k});
        if (k == VoteAll || k == VoteAny) votes = true;
        i += wc;
    }
    if (sites.empty()) return 0;


    std::vector<uint32_t> decl, deco;
    auto fresh = [&] { return bound++; };
    auto constant = [&](uint32_t &id, uint32_t v) { if (!id) { id = fresh(); decl.insert(decl.end(), {inst(OpConstant, 4), t_uint, id, v}); } };
    constant(c_u5, 5); constant(c_u31, 31); constant(c_u96, 96);
    for (uint32_t k = 1; k < words; k++) constant(c_word[k], k);
    if (!t_bool) { t_bool = fresh(); decl.insert(decl.end(), {inst(OpTypeBool, 2), t_bool}); }
    if (votes) {
        constant(c_u0, 0);
        if (!c_true) { c_true = fresh(); decl.insert(decl.end(), {inst(OpConstantTrue, 3), t_bool, c_true}); }
        if (!t_v4u) { t_v4u = fresh(); decl.insert(decl.end(), {inst(OpTypeVector, 4), t_v4u, t_uint, 4}); }
    }
    bool new_lane = !lane_var;


    if (new_lane && entry_count != 1) return 0;
    if (new_lane) {
        if (!t_ptr_in_uint) { t_ptr_in_uint = fresh(); decl.insert(decl.end(), {inst(OpTypePointer, 4), t_ptr_in_uint, StorageInput, t_uint}); }
        lane_var = fresh(); decl.insert(decl.end(), {inst(OpVariable, 4), t_ptr_in_uint, lane_var, StorageInput});
        deco.insert(deco.end(), {inst(OpDecorate, 4), lane_var, DecBuiltIn, BuiltInSubgroupLocalInvocationId});
    }


    out.reserve(n + 64 + sites.size() * 16);
    out.insert(out.end(), in, in + 5);
    auto raw_lane = [&]() -> uint32_t {
        uint32_t r = fresh(); out.insert(out.end(), {inst(OpLoad, 4), lane_t, r, lane_var});
        if (lane_t == t_uint) return r;
        uint32_t u = fresh(); out.insert(out.end(), {inst(OpBitcast, 4), t_uint, u, r}); return u;
    };
    auto warp_word = [&]() -> uint32_t { uint32_t r = raw_lane(), w = fresh(); out.insert(out.end(), {inst(OpShiftRightLogical, 5), t_uint, w, r, c_u5}); return w; };



    auto pick = [&](uint32_t rt, uint32_t res, uint32_t vec, uint32_t w) {
        uint32_t r = words > 1 ? fresh() : res; out.insert(out.end(), {inst(OpCompositeExtract, 5), words > 1 ? t_uint : rt, r, vec, 0});
        for (uint32_t k = 1; k < words; k++) {
            uint32_t x = fresh(), c = fresh(), s = k + 1 == words ? res : fresh();
            out.insert(out.end(), {inst(OpCompositeExtract, 5), t_uint, x, vec, k});
            out.insert(out.end(), {inst(OpIEqual, 5), t_bool, c, w, c_word[k]});
            out.insert(out.end(), {inst(OpSelect, 6), k + 1 == words ? rt : t_uint, s, c, x, r}); r = s;
        }
    };
    size_t next = 0;
    for (size_t i = 5; i < n;) {
        uint32_t wc = in[i] >> 16;
        if (i == first_cap && votes && !has_ballot_cap) out.insert(out.end(), {inst(OpCapability, 2), CapGroupNonUniformBallot});
        if (i == first_type) out.insert(out.end(), deco.begin(), deco.end());
        if (i == first_func) out.insert(out.end(), decl.begin(), decl.end());
        if (i == entry && new_lane) { out.push_back(inst(OpEntryPoint, wc + 1)); out.insert(out.end(), in + i + 1, in + i + wc); out.push_back(lane_var); i += wc; continue; }
        if (next < sites.size() && sites[next].at == i) {
            Kind k = sites[next++].kind;
            uint32_t rt = in[i + 1], res = in[i + 2];
            if (k == LaneLoad) {
                uint32_t raw = fresh(); out.push_back(inst(OpLoad, wc)); out.insert(out.end(), {rt, raw}); out.insert(out.end(), in + i + 3, in + i + wc);
                if (rt == t_uint) out.insert(out.end(), {inst(OpBitwiseAnd, 5), rt, res, raw, c_u31});
                else {


                    uint32_t u = fresh(), masked = fresh();
                    out.insert(out.end(), {inst(OpBitcast, 4), t_uint, u, raw});
                    out.insert(out.end(), {inst(OpBitwiseAnd, 5), t_uint, masked, u, c_u31});
                    out.insert(out.end(), {inst(OpBitcast, 4), rt, res, masked});
                }
            } else if (k == WordExtract) {
                uint32_t w = warp_word(); pick(rt, res, in[i + 3], w);
            } else if (k == ChainLoad) {
                uint32_t mask = chain_mask[in[i + 3]], mp = var_type[mask], mt = types[mp].b, m = fresh();
                out.insert(out.end(), {inst(OpLoad, 4), mt, m, mask});
                uint32_t w = warp_word(); pick(rt, res, m, w);
            } else if (k == VoteAll || k == VoteAny) {
                uint32_t scope = in[i + 3], pred = in[i + 4], b1 = fresh(), x1 = fresh();
                out.insert(out.end(), {inst(OpGroupNonUniformBallot, 5), t_v4u, b1, scope, pred});
                uint32_t w = warp_word(); pick(t_uint, x1, b1, w);
                if (k == VoteAll) {
                    uint32_t b2 = fresh(), x2 = fresh();
                    out.insert(out.end(), {inst(OpGroupNonUniformBallot, 5), t_v4u, b2, scope, c_true});
                    pick(t_uint, x2, b2, w);
                    out.insert(out.end(), {inst(OpIEqual, 5), rt, res, x1, x2});
                } else out.insert(out.end(), {inst(OpINotEqual, 5), rt, res, x1, c_u0});
            } else if (k == Shuffle) {
                uint32_t raw = raw_lane(), hi = fresh(), idx = fresh();
                out.insert(out.end(), {inst(OpBitwiseAnd, 5), t_uint, hi, raw, c_u96});
                out.insert(out.end(), {inst(OpBitwiseOr, 5), t_uint, idx, hi, in[i + 5]});
                out.insert(out.end(), {inst(OpGroupNonUniformShuffle, 6), rt, res, in[i + 3], in[i + 4], idx});
            }
            i += wc; continue;
        }
        out.insert(out.end(), in + i, in + i + wc);
        i += wc;
    }
    out[3] = bound;
    return sites.size();
}
