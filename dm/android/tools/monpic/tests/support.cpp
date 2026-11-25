#include "support.h"

#include <cstring>
#include <algorithm>
#include <cmath>
#include <map>
#include <fstream>
#include <unordered_map>
#include <unistd.h>

#include "bitknit_model.h"
#include "md5.h"

namespace test {

int checks = 0;

uint64_t rnd(uint64_t &s) {
    s ^= s >> 12;
    s ^= s << 25;
    s ^= s >> 27;
    return s * 0x2545F4914F6CDD1Dull;
}

namespace {

using monpic::bitknit::Model;


struct Op {
    enum Kind { symbol, bits, word } kind;
    uint32_t a, b;
};

class Writer {
public:
    Writer() {
        for (auto &m : command_) m.init(300, 36);
        for (auto &m : cache_ref_) m.init(40, 0);
        offset_bits_.init(21, 0);
        for (auto &o : cache_) o = 1;
    }

    std::vector<uint8_t> run(const std::vector<uint8_t> &d, int stored_every) {
        std::vector<uint8_t> out = {0xB1, 0x75};
        size_t pos = 0;
        int quantum = 0;
        while (pos < d.size()) {
            const size_t end = std::min(d.size(), pos + (65536 - pos % 65536));
            quantum++;
            if (stored_every > 0 && quantum % stored_every == 0) {
                out.push_back(0);
                out.push_back(0);
                out.insert(out.end(), d.begin() + static_cast<long>(pos), d.begin() + static_cast<long>(end));
                pos = end;
                continue;
            }
            ops_.clear();
            while (pos < end) pos = command(d, pos);
            flush(out);
        }
        return out;
    }

private:
    void sym(Model &m, int s) {
        uint32_t start, width;
        m.range(s, &start, &width);
        ops_.push_back({Op::symbol, start, width});
        m.update(s);
    }

    size_t command(const std::vector<uint8_t> &d, size_t pos) {
        if (pos == 0) {
            ops_.push_back({Op::bits, d[0], 8});
            return 1;
        }

        size_t best_len = 0, best_off = 0;
        auto match = [&](size_t off) {
            if (off == 0 || off > pos) return;
            size_t n = 0;
            while (pos + n < d.size() && n < 8223 && d[pos + n] == d[pos - off + n]) n++;
            if (n >= 2 && n > best_len) {
                best_len = n;
                best_off = off;
            }
        };
        for (uint64_t o : cache_) match(o);
        if (pos + 3 <= d.size()) {
            const uint32_t h = (d[pos] | d[pos + 1] << 8 | d[pos + 2] << 16) * 2654435761u >> 12;
            const auto it = last_.find(h);
            if (it != last_.end() && pos - it->second < (1u << 26) - 32) match(pos - it->second);
        }
        for (size_t p = pos; p < pos + std::max<size_t>(best_len, 1) && p + 3 <= d.size(); p++)
            last_[(d[p] | d[p + 1] << 8 | d[p + 2] << 16) * 2654435761u >> 12] = p;
        const size_t model = pos % 4;
        if (!best_len) {
            sym(command_[model], (d[pos] - d[pos - last_offset_]) & 255);
            return pos + 1;
        }
        if (best_len <= 33) {
            sym(command_[model], static_cast<int>(best_len + 254));
        } else {
            int b = 0;
            while ((size_t{2} << b) <= best_len - 32) b++;
            sym(command_[model], b + 287);
            ops_.push_back({Op::bits, static_cast<uint32_t>(best_len - 32 - (size_t{1} << b)), static_cast<uint32_t>(b)});
        }
        int hit = -1;
        for (int i = 0; i < 8 && hit < 0; i++)
            if (cache_[i] == best_off) hit = i;
        if (hit >= 0) {
            sym(cache_ref_[model], hit);
            for (int i = hit; i > 0; i--) cache_[i] = cache_[i - 1];
            cache_[0] = best_off;
        } else {
            const uint64_t t = best_off + 31;
            int b = 0;
            while ((uint64_t{1} << (b + 6)) <= t) b++;
            const uint64_t rem = t - (uint64_t{1} << (5 + b)), low = rem >> 5;
            sym(cache_ref_[model], static_cast<int>((rem & 31) + 8));
            sym(offset_bits_, b);
            ops_.push_back({Op::bits, static_cast<uint32_t>(b >= 16 ? low >> 16 : low), static_cast<uint32_t>(b & 15)});
            if (b >= 16) ops_.push_back({Op::word, static_cast<uint32_t>(low & 0xffff), 0});
            cache_[7] = cache_[6];
            cache_[6] = best_off;
        }
        last_offset_ = best_off;
        return pos + best_len;
    }


    void flush(std::vector<uint8_t> &out) {
        uint32_t x[2] = {1u << 16, 1u << 16};
        std::vector<uint16_t> rev;
        size_t state_ops = 0;
        for (const Op &op : ops_) state_ops += op.kind != Op::word;
        size_t i = state_ops;
        for (auto it = ops_.rbegin(); it != ops_.rend(); ++it) {
            if (it->kind == Op::word) {
                rev.push_back(static_cast<uint16_t>(it->a));
                continue;
            }
            uint32_t &s = x[--i % 2];
            uint64_t v = s;
            if (it->kind == Op::symbol) {
                if (v >= (uint64_t{1} << 17) * it->b) {
                    rev.push_back(static_cast<uint16_t>(v & 0xffff));
                    v >>= 16;
                }
                v = ((v / it->b) << 15) + v % it->b + it->a;
            } else {
                if (v >= (uint64_t{1} << (32 - it->b))) {
                    rev.push_back(static_cast<uint16_t>(v & 0xffff));
                    v >>= 16;
                }
                v = v << it->b | it->a;
            }
            s = static_cast<uint32_t>(v);
        }
        const uint32_t a = x[0], b = x[1];
        int split = 0;
        while ((uint64_t{1} << (17 + split)) <= b) split++;
        const uint64_t low = b - (uint64_t{1} << (16 + split));
        std::vector<uint16_t> words;
        uint64_t init;
        bool second = false;
        if ((uint64_t{a} << split) < (uint64_t{1} << 32)) {
            init = uint64_t{a} << split | low >> 16;
        } else {
            init = uint64_t{a >> 16} << split | low >> 16;
            second = true;
        }
        if (init < (1u << 28)) {
            const uint32_t v = static_cast<uint32_t>(init << 4 | split);
            words = {static_cast<uint16_t>(v >> 16), static_cast<uint16_t>(v)};
        } else {
            const uint32_t v = static_cast<uint32_t>((init >> 16) << 4 | split);
            words = {static_cast<uint16_t>(v >> 16), static_cast<uint16_t>(v), static_cast<uint16_t>(init & 0xffff)};
        }
        if (second) words.push_back(static_cast<uint16_t>(a & 0xffff));
        words.push_back(static_cast<uint16_t>(low & 0xffff));
        words.insert(words.end(), rev.rbegin(), rev.rend());
        for (uint16_t w : words) {
            out.push_back(static_cast<uint8_t>(w));
            out.push_back(static_cast<uint8_t>(w >> 8));
        }
    }

    Model command_[4], cache_ref_[4], offset_bits_;
    uint64_t cache_[8];
    uint64_t last_offset_ = 1;
    std::vector<Op> ops_;
    std::unordered_map<uint32_t, size_t> last_;
};

}

std::vector<uint8_t> bitknit_encode(const std::vector<uint8_t> &data, int stored_every) { return Writer().run(data, stored_every); }

Container::Container() : data(16, 0) {}

uint64_t Container::put(const std::vector<uint8_t> &raw, size_t align) {
    while (data.size() % align) data.push_back(0);
    const uint64_t at = data.size();
    data.insert(data.end(), raw.begin(), raw.end());
    return at;
}

uint64_t Container::string(const std::string &s) {
    std::vector<uint8_t> raw(s.begin(), s.end());
    raw.push_back(0);
    return put(raw, 1);
}

void Container::pointer(uint64_t at, uint64_t target) { fixups_.push_back({at, target}); }
void Container::u32(uint64_t at, uint32_t v) { std::memcpy(&data[at], &v, 4); }
void Container::f32(uint64_t at, float v) { std::memcpy(&data[at], &v, 4); }

uint64_t Container::type(const std::vector<M> &members) {
    const uint64_t at = put(std::vector<uint8_t>(44 * (members.size() + 1), 0));
    for (size_t i = 0; i < members.size(); i++) {
        const uint64_t m = at + 44 * i;
        u32(m, members[i].kind);
        pointer(m + 4, string(members[i].name));
        if (members[i].sub) pointer(m + 12, members[i].sub);
        u32(m + 20, static_cast<uint32_t>(members[i].count));
    }
    return at;
}

std::vector<uint8_t> Container::file(uint64_t root_type, uint64_t root, bool packed) const {
    std::vector<uint8_t> fix;
    for (const auto &f : fixups_) {
        const uint32_t v[3] = {static_cast<uint32_t>(f.first), 0, static_cast<uint32_t>(f.second)};
        const uint8_t *p = reinterpret_cast<const uint8_t *>(v);
        fix.insert(fix.end(), p, p + 12);
    }
    std::vector<uint8_t> sec_stored = data, fix_stored = fix;
    if (packed) {
        sec_stored = bitknit_encode(data);
        const std::vector<uint8_t> f = bitknit_encode(fix);
        const uint32_t n = static_cast<uint32_t>(f.size());
        fix_stored.assign(reinterpret_cast<const uint8_t *>(&n), reinterpret_cast<const uint8_t *>(&n) + 4);
        fix_stored.insert(fix_stored.end(), f.begin(), f.end());
    }
    const uint32_t head = 0x20 + 0x24, data_off = head + 44, fix_off = data_off + static_cast<uint32_t>(sec_stored.size());
    const uint32_t total = fix_off + static_cast<uint32_t>(fix_stored.size());
    static const uint8_t magic[16] = {0xe5, 0x9b, 0x49, 0x5e, 0x6f, 0x63, 0x1f, 0x14, 0x1e, 0x13, 0xeb, 0xa9, 0x90, 0xbe, 0xed, 0xc4};
    std::vector<uint8_t> out(magic, magic + 16);
    auto u32s = [&](std::initializer_list<uint32_t> vs) {
        for (uint32_t v : vs)
            for (int b = 0; b < 4; b++) out.push_back(static_cast<uint8_t>(v >> (8 * b)));
    };
    u32s({0x1b8, 0, 0, 0});
    u32s({7, total, 0, 0x24, 1});
    u32s({0, static_cast<uint32_t>(root_type), 0, static_cast<uint32_t>(root)});
    const uint32_t size = static_cast<uint32_t>(data.size());
    u32s({packed ? 4u : 0u, data_off, static_cast<uint32_t>(sec_stored.size()), size, 4, size, size, fix_off,
          static_cast<uint32_t>(fixups_.size()), 0, 0});
    out.insert(out.end(), sec_stored.begin(), sec_stored.end());
    out.insert(out.end(), fix_stored.begin(), fix_stored.end());
    return out;
}

Field num(const std::string &name, uint32_t kind, std::vector<double> values, bool counted) {
    Field f{Field::number, name};
    f.number_kind = kind;
    f.values = std::move(values);
    f.counted = counted;
    return f;
}
Field str(const std::string &name, const std::string &text) {
    Field f{Field::string, name};
    f.text = text;
    return f;
}
Field xform(const std::string &name, std::vector<double> p, std::vector<double> o, std::vector<double> s) {
    Field f{Field::transform, name};
    f.values = p;
    f.values.insert(f.values.end(), o.begin(), o.end());
    f.values.insert(f.values.end(), s.begin(), s.end());
    return f;
}
static Field with(Field::Kind kind, const std::string &name, std::vector<Object> list) {
    Field f{kind, name};
    f.objects = std::move(list);
    return f;
}
Field inl(const std::string &name, Object o) { return with(Field::inline_, name, {std::move(o)}); }
Field ref(const std::string &name, Object o) { return with(Field::ref, name, {std::move(o)}); }
Field null_ref(const std::string &name) {
    Field f = with(Field::ref, name, {Object{num("Unused", 19, {0})}});
    f.null = true;
    return f;
}
Field var(const std::string &name, Object o) { return with(Field::variant, name, {std::move(o)}); }
Field recs(const std::string &name, std::vector<Object> list) { return with(Field::records, name, std::move(list)); }
Field var_recs(const std::string &name, std::vector<Object> list) { return with(Field::variant_records, name, std::move(list)); }

namespace {

size_t number_size(uint32_t kind) {
    switch (kind) {
    case 10: case 19: case 20: return 4;
    case 15: case 16: case 17: case 18: case 21: return 2;
    default: return 1;
    }
}

size_t field_size(const Field &f);
size_t object_size(const Object &o) {
    size_t n = 0;
    for (const Field &f : o) n += field_size(f);
    return n;
}
size_t field_size(const Field &f) {
    switch (f.kind) {
    case Field::number: return number_size(f.number_kind) * f.values.size();
    case Field::string: case Field::ref: return 8;
    case Field::transform: return 68;
    case Field::inline_: return object_size(f.objects[0]);
    case Field::variant: return 16;
    case Field::records: return 12;
    case Field::variant_records: return 20;
    }
    return 0;
}

class ObjectWriter {
public:
    uint64_t type_of(const Object &o) {
        std::vector<Container::M> members;
        std::string key;
        for (const Field &f : o) {
            uint32_t kind = 0, sub = 0;
            int32_t count = 0;
            switch (f.kind) {
            case Field::number:
                kind = f.number_kind;
                count = f.values.size() > 1 || f.counted ? static_cast<int32_t>(f.values.size()) : 0;
                break;
            case Field::string: kind = 8; break;
            case Field::transform: kind = 9; break;
            case Field::inline_: kind = 1; sub = static_cast<uint32_t>(type_of(f.objects[0])); break;
            case Field::ref: kind = 2; sub = static_cast<uint32_t>(type_of(f.objects[0])); break;
            case Field::variant: kind = 5; break;
            case Field::records:
                kind = 3;
                sub = static_cast<uint32_t>(type_of(f.objects.empty() ? Object{num("Unused", 19, {0})} : f.objects[0]));
                break;
            case Field::variant_records: kind = 7; break;
            }
            members.push_back({kind, f.name, sub, count});
            key += std::to_string(kind) + ":" + f.name + ":" + std::to_string(sub) + ":" + std::to_string(count) + ";";
        }
        auto it = types_.find(key);
        if (it == types_.end()) it = types_.emplace(key, c.type(members)).first;
        return it->second;
    }

    uint64_t write(const Object &o) {
        const uint64_t at = c.put(std::vector<uint8_t>(std::max<size_t>(object_size(o), 1), 0));
        fill(at, o);
        return at;
    }

    void fill(uint64_t at, const Object &o) {
        for (const Field &f : o) {
            switch (f.kind) {
            case Field::number:
                for (double v : f.values) {
                    const size_t n = number_size(f.number_kind);
                    if (f.number_kind == 10) c.f32(at, static_cast<float>(v));
                    else if (f.number_kind == 21) {
                        const uint16_t h = half(v);
                        std::memcpy(&c.data[at], &h, 2);
                    } else {
                        const int64_t i = static_cast<int64_t>(v);
                        std::memcpy(&c.data[at], &i, n);
                    }
                    at += n;
                }
                continue;
            case Field::string:
                if (!f.null) c.pointer(at, c.string(f.text));
                break;
            case Field::transform: {
                const uint32_t flags = 0;
                std::memcpy(&c.data[at], &flags, 4);
                for (int i = 0; i < 16; i++) c.f32(at + 4 + 4 * i, static_cast<float>(f.values[i]));
                break;
            }
            case Field::inline_: fill(at, f.objects[0]); break;
            case Field::ref:
                if (!f.null) c.pointer(at, write(f.objects[0]));
                break;
            case Field::variant:
                c.pointer(at, type_of(f.objects[0]));
                c.pointer(at + 8, write(f.objects[0]));
                break;
            case Field::records: case Field::variant_records: {
                const uint32_t n = static_cast<uint32_t>(f.objects.size());
                const size_t head = f.kind == Field::variant_records ? 8 : 0;
                c.u32(at + head, n);
                if (n) {
                    const size_t size = object_size(f.objects[0]);
                    const uint64_t list = c.put(std::vector<uint8_t>(size * n, 0));
                    for (uint32_t i = 0; i < n; i++) fill(list + size * i, f.objects[i]);
                    if (f.kind == Field::variant_records) c.pointer(at, type_of(f.objects[0]));
                    c.pointer(at + head + 4, list);
                }
                break;
            }
            }
            at += field_size(f);
        }
    }

    Container c;

private:
    static uint16_t half(double v) {
        if (v == 0) return 0;
        const uint16_t sign = v < 0 ? 0x8000 : 0;
        v = std::fabs(v);
        int e = 0;
        while (v >= 2) v /= 2, e++;
        while (v < 1) v *= 2, e--;
        return static_cast<uint16_t>(sign | (e + 15) << 10 | static_cast<uint16_t>((v - 1) * 1024 + 0.5));
    }
    std::map<std::string, uint64_t> types_;
};

}

std::vector<uint8_t> object_file(const Object &root, bool packed) {
    ObjectWriter w;
    const uint64_t type = w.type_of(root);
    const uint64_t at = w.write(root);
    return w.c.file(type, at, packed);
}

std::string temp_dir() {
    char t[] = "/tmp/monpic-test-XXXXXX";
    if (!mkdtemp(t)) std::abort();
    return t;
}

void write_file(const std::string &path, const std::vector<uint8_t> &data) {
    std::ofstream f(path, std::ios::binary);
    f.write(reinterpret_cast<const char *>(data.data()), static_cast<std::streamsize>(data.size()));
}

std::vector<uint8_t> read_file(const std::string &path) {
    std::ifstream f(path, std::ios::binary);
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(f), {});
}

std::string md5_hex(const std::vector<uint8_t> &data) {
    uint8_t d[16];
    monpic::md5(data.data(), data.size(), d);
    char s[33];
    for (int i = 0; i < 16; i++) std::snprintf(s + 2 * i, 3, "%02x", d[i]);
    return s;
}

std::vector<uint8_t> zlib_stored(const std::vector<uint8_t> &data) {
    std::vector<uint8_t> out = {0x78, 0x01};
    size_t pos = 0;
    do {
        const size_t n = std::min<size_t>(65535, data.size() - pos);
        out.push_back(pos + n == data.size() ? 1 : 0);
        out.push_back(static_cast<uint8_t>(n));
        out.push_back(static_cast<uint8_t>(n >> 8));
        out.push_back(static_cast<uint8_t>(~n));
        out.push_back(static_cast<uint8_t>(~n >> 8));
        out.insert(out.end(), data.begin() + static_cast<long>(pos), data.begin() + static_cast<long>(pos + n));
        pos += n;
    } while (pos < data.size());
    uint32_t a = 1, b = 0;
    for (uint8_t c : data) {
        a = (a + c) % 65521;
        b = (b + a) % 65521;
    }
    const uint32_t adler = b << 16 | a;
    for (int i = 3; i >= 0; i--) out.push_back(static_cast<uint8_t>(adler >> (8 * i)));
    return out;
}

}
