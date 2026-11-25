










#include "xbg_spirv_temps.h"

namespace {
enum : uint32_t {
    OpName = 5, OpMemberName = 6, OpDecorate = 71, OpMemberDecorate = 72, OpFunction = 54, OpFunctionEnd = 56,
    OpVariable = 59, OpLoad = 61, OpStore = 62, OpLabel = 248, StorageFunction = 7,
};

bool literal_at(uint32_t op, uint32_t w) {
    switch (op) {
    case 8: return w >= 2;
    case 12: return w == 4;
    case OpVariable: return w == 3;
    case OpLoad: return w >= 4;
    case 62: case 63: return w >= 3;
    case 79: case 82: return w >= 5;
    case 81: return w >= 4;
    case 246: return w >= 3;
    case 247: return w >= 2;
    case 250: return w >= 4;
    case 251: return w >= 3;

    case 87: case 88: case 91: case 92: case 95: case 98: return w == 5;
    case 89: case 90: case 93: case 94: case 96: case 97: return w == 6;
    case 99: return w == 4;
    default: return false;
    }
}
}

size_t xbg_spirv_forward_temps(const uint32_t *in, size_t n, std::vector<uint32_t> &out) {
    out.clear();
    if (!in || n < 5 || in[0] != 0x07230203u) return 0;
    const uint32_t bound = in[3];

    std::vector<uint8_t> state(bound, 0);
    std::vector<uint32_t> value(bound, 0), store_block(bound, 0), repl(bound, 0);
    size_t first_func = 0;
    for (size_t i = 5; i < n;) {
        uint32_t wc = in[i] >> 16, op = in[i] & 0xffff;
        if (!wc || i + wc > n) return 0;
        if (op == OpFunction && !first_func) first_func = i;
        if (op == OpVariable && wc >= 4 && in[i + 2] < bound && in[i + 3] == StorageFunction) state[in[i + 2]] = wc == 4 ? 1 : 3;
        i += wc;
    }
    if (!first_func) return 0;

    std::vector<uint8_t> named(bound, 0);
    for (size_t i = 5; i < first_func;) {
        uint32_t wc = in[i] >> 16, op = in[i] & 0xffff;
        if ((op == OpName || op == OpMemberName || op == OpDecorate || op == OpMemberDecorate) && wc >= 2 && in[i + 1] < bound) {
            named[in[i + 1]] = 1;
            if (state[in[i + 1]]) state[in[i + 1]] = 3;
        }
        i += wc;
    }


    uint32_t block = 0;
    std::vector<uint32_t> load_var(bound, 0);
    for (size_t i = first_func; i < n;) {
        uint32_t wc = in[i] >> 16, op = in[i] & 0xffff;
        if (op == OpLabel && wc >= 2) block = in[i + 1];
        if (op == OpStore && wc >= 3 && in[i + 1] < bound && state[in[i + 1]] && state[in[i + 1]] != 3) {
            uint32_t v = in[i + 1], x = in[i + 2];
            if (x < bound && state[x]) state[x] = 3;
            if (state[v] == 1 && wc == 3) { state[v] = 2; value[v] = x < bound && repl[x] ? repl[x] : x; store_block[v] = block; }
            else state[v] = 3;
        }
        else if (op == OpLoad && wc >= 4 && in[i + 3] < bound && state[in[i + 3]] && state[in[i + 3]] != 3) {
            uint32_t v = in[i + 3];
            if (state[v] != 2 || store_block[v] != block || wc != 4 || in[i + 2] >= bound || named[in[i + 2]]) state[v] = 3;
            else { repl[in[i + 2]] = value[v]; load_var[in[i + 2]] = v; }
        }
        else {
            for (uint32_t w = 1; w < wc; w++) {
                uint32_t id = in[i + w];
                if (id < bound && state[id] && !(op == OpVariable && w == 2) && !literal_at(op, w)) state[id] = 3;
            }
        }
        i += wc;
    }

    size_t removed = 0;
    for (uint32_t v = 0; v < bound; v++) if (state[v] == 2) removed++;
    if (!removed) return 0;
    for (uint32_t id = 0; id < bound; id++) if (load_var[id] && state[load_var[id]] != 2) repl[id] = 0;

    auto resolve = [&](uint32_t id) { for (int k = 0; k < 64 && id < bound && repl[id]; k++) id = repl[id]; return id; };
    out.reserve(n);
    out.insert(out.end(), in, in + first_func);
    for (size_t i = first_func; i < n;) {
        uint32_t wc = in[i] >> 16, op = in[i] & 0xffff;
        bool drop = (op == OpVariable && wc >= 4 && in[i + 2] < bound && state[in[i + 2]] == 2) ||
                    (op == OpStore && wc >= 3 && in[i + 1] < bound && state[in[i + 1]] == 2) ||
                    (op == OpLoad && wc >= 4 && in[i + 3] < bound && state[in[i + 3]] == 2);
        if (!drop) {
            size_t at = out.size();
            out.insert(out.end(), in + i, in + i + wc);
            for (uint32_t w = 1; w < wc; w++) {
                uint32_t id = in[i + w];
                if (id < bound && repl[id] && !literal_at(op, w)) out[at + w] = resolve(id);
            }
        }
        i += wc;
    }
    return removed;
}
