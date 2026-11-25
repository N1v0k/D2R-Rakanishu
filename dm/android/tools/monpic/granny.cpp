#include "granny.h"

#include <cstring>
#include <limits>

#include "bitknit.h"
#include "error.h"

namespace monpic {
namespace {

const uint8_t kMagic[16] = {0xe5, 0x9b, 0x49, 0x5e, 0x6f, 0x63, 0x1f, 0x14, 0x1e, 0x13, 0xeb, 0xa9, 0x90, 0xbe, 0xed, 0xc4};
constexpr uint64_t kBase = 16;
constexpr uint32_t kMaxSection = 64u << 20;
constexpr uint32_t kMaxFile = 256u << 20;
constexpr size_t kMaxMembers = 4096;
constexpr int kMaxDepth = 64;

uint32_t le32(const uint8_t *p) { return p[0] | p[1] << 8 | p[2] << 16 | static_cast<uint32_t>(p[3]) << 24; }
uint64_t le64(const uint8_t *p) { return le32(p) | static_cast<uint64_t>(le32(p + 4)) << 32; }

uint64_t kind_size(uint32_t kind) {
    switch (kind) {
    case kRef: case kString: case kEmptyReference: return 8;
    case kRefArray: case kArrayOfRefs: return 12;
    case kVariantRef: return 16;
    case kRefVariantArray: return 20;
    case kTransform: return 68;
    case kReal32: case kInt32: case kUInt32: return 4;
    case kInt8: case kUInt8: case kBinormalInt8: case kNormalUInt8: return 1;
    case kInt16: case kUInt16: case kBinormalInt16: case kNormalUInt16: case kReal16: return 2;
    default: return 0;
    }
}

double half(uint16_t h) {
    const int exp = h >> 10 & 31, mant = h & 1023;
    const double sign = h & 0x8000 ? -1.0 : 1.0;
    if (exp == 0) return sign * mant * (1.0 / (1 << 24));
    if (exp == 31) return mant ? std::numeric_limits<double>::quiet_NaN() : sign * std::numeric_limits<double>::infinity();
    return sign * (1024 + mant) * static_cast<double>(uint64_t{1} << exp) / static_cast<double>(uint64_t{1} << 25);
}

}

bool numeric_kind(uint32_t kind) { return kind >= kReal32 && kind <= kReal16; }

void Tree::check(uint64_t at, uint64_t n) const {
    if (at < kBase || at > buf_.size() || n > buf_.size() - at)
        throw PictureError(format("object at %#llx of %llu bytes outside the data", static_cast<unsigned long long>(at),
                                  static_cast<unsigned long long>(n)));
}

uint64_t Tree::ptr(uint64_t at) const {
    check(at, 8);
    return le64(buf_.data() + at);
}

uint32_t Tree::u32(uint64_t at) const {
    check(at, 4);
    return le32(buf_.data() + at);
}

std::string Tree::cstring(uint64_t at) const {
    check(at, 1);
    const uint8_t *p = buf_.data() + at;
    const void *end = std::memchr(p, 0, buf_.size() - at);
    if (!end) throw PictureError(format("unterminated string at %#llx", static_cast<unsigned long long>(at)));
    return std::string(reinterpret_cast<const char *>(p), static_cast<const uint8_t *>(end) - p);
}

double Tree::value(uint32_t kind, uint64_t at) const {
    const uint64_t n = kind_size(kind);
    check(at, n);
    const uint8_t *p = buf_.data() + at;
    switch (kind) {
    case kReal32: { float f; std::memcpy(&f, p, 4); return f; }
    case kInt8: case kBinormalInt8: return static_cast<int8_t>(p[0]);
    case kUInt8: case kNormalUInt8: return p[0];
    case kInt16: case kBinormalInt16: return static_cast<int16_t>(p[0] | p[1] << 8);
    case kUInt16: case kNormalUInt16: return static_cast<uint16_t>(p[0] | p[1] << 8);
    case kInt32: return static_cast<int32_t>(le32(p));
    case kUInt32: return le32(p);
    case kReal16: return half(static_cast<uint16_t>(p[0] | p[1] << 8));
    default: throw PictureError(format("member kind %u is no number", kind));
    }
}

const std::vector<Member> &Tree::members(uint64_t type) const {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto it = members_.find(type);
        if (it != members_.end()) return *it->second;
    }
    auto out = std::make_unique<std::vector<Member>>();
    for (uint64_t at = type;; at += 44) {
        const uint32_t kind = u32(at);
        if (kind == kEnd) break;
        if (kind != kInline && kind_size(kind) == 0)
            throw PictureError(format("member kind %u in the type at %#llx", kind, static_cast<unsigned long long>(type)));
        check(at, 44);
        Member m;
        m.kind = kind;
        const uint64_t name = ptr(at + 4);
        m.name = name ? cstring(name) : std::string();
        m.sub = ptr(at + 12);
        const int32_t count = static_cast<int32_t>(u32(at + 20));
        if (count < 0 || count > (1 << 16) || ((kind == kInline || kind == kRef || kind == kRefArray || kind == kArrayOfRefs) && !m.sub))
            throw PictureError(format("member %s of the type at %#llx is malformed", m.name.c_str(), static_cast<unsigned long long>(type)));
        m.count = static_cast<uint32_t>(count);
        out->push_back(m);
        if (out->size() > kMaxMembers) throw PictureError(format("type at %#llx does not end", static_cast<unsigned long long>(type)));
    }
    std::lock_guard<std::mutex> lock(mutex_);
    auto &slot = members_[type];
    if (!slot) slot = std::move(out);
    return *slot;
}

uint64_t Tree::size_at(uint64_t type, int depth) const {
    if (depth > kMaxDepth) throw PictureError(format("types nested deeper than %d", kMaxDepth));
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto it = sizes_.find(type);
        if (it != sizes_.end()) return it->second;
    }
    uint64_t total = 0;
    for (const Member &m : members(type)) {
        const uint64_t one = m.kind == kInline ? size_at(m.sub, depth + 1) : kind_size(m.kind);
        total += one * (m.count > 1 ? m.count : 1);
        if (total > buf_.size()) throw PictureError(format("type at %#llx larger than its file", static_cast<unsigned long long>(type)));
    }
    std::lock_guard<std::mutex> lock(mutex_);
    sizes_[type] = total;
    return total;
}

uint64_t Tree::size(uint64_t type) const { return size_at(type, 0); }

const std::vector<Member> &Tree::layout(uint64_t type) const {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto it = layouts_.find(type);
        if (it != layouts_.end()) return *it->second;
    }
    auto out = std::make_unique<std::vector<Member>>(members(type));
    uint64_t off = 0;
    for (Member &m : *out) {
        m.offset = off;
        m.size = m.kind == kInline ? size(m.sub) : kind_size(m.kind);
        off += m.size * (m.count > 1 ? m.count : 1);
    }
    std::lock_guard<std::mutex> lock(mutex_);
    auto &slot = layouts_[type];
    if (!slot) slot = std::move(out);
    return *slot;
}

const std::vector<Member> &Obj::members() const { return tree_->layout(type_); }

const Member *Obj::find(const char *name) const {
    const std::vector<Member> &list = tree_->layout(type_);
    const Member *hit = nullptr;
    for (const Member &m : list)
        if (m.name == name) hit = &m;
    return hit;
}

const Member &Obj::member(const char *name) const {
    const Member *m = find(name);
    if (!m) throw PictureError(format("an object without %s", name));
    return *m;
}

std::string Obj::first_member_name() const {
    const std::vector<Member> &list = tree_->layout(type_);
    if (list.empty()) throw PictureError("an object of a type without members");
    return list[0].name;
}

Obj Obj::object(const char *name) const {
    const Member &m = member(name);
    const uint64_t at = at_ + m.offset;
    tree_->check(at, m.size);
    switch (m.kind) {
    case kInline: return Obj(tree_, m.sub, at);
    case kRef: {
        const uint64_t p = tree_->ptr(at);
        if (p) tree_->check(p, tree_->size(m.sub));
        return Obj(tree_, m.sub, p);
    }
    case kVariantRef: {
        const uint64_t t = tree_->ptr(at), p = tree_->ptr(at + 8);
        if (!t || !p) return Obj(tree_, 0, 0);
        tree_->check(p, tree_->size(t));
        return Obj(tree_, t, p);
    }
    default: throw PictureError(format("member %s is no object", name));
    }
}

bool Obj::has_string(const char *name, std::string *out) const {
    const Member &m = member(name);
    if (m.kind != kString) throw PictureError(format("member %s is no string", name));
    const uint64_t p = tree_->ptr(at_ + m.offset);
    if (!p) return false;
    *out = tree_->cstring(p);
    return true;
}

std::string Obj::string(const char *name) const {
    std::string s;
    if (!has_string(name, &s)) throw PictureError(format("member %s is a null string", name));
    return s;
}

std::vector<double> Obj::numbers(const char *name) const {
    const Member &m = member(name);
    if (!numeric_kind(m.kind)) throw PictureError(format("member %s is no number", name));
    const uint32_t n = m.count > 1 ? m.count : 1;
    std::vector<double> out(n);
    for (uint32_t i = 0; i < n; i++) out[i] = tree_->value(m.kind, at_ + m.offset + i * m.size);
    return out;
}

double Obj::number(const char *name) const {
    const std::vector<double> v = numbers(name);
    if (v.size() != 1) throw PictureError(format("member %s holds %zu numbers, not one", name, v.size()));
    return v[0];
}

uint64_t Array::record_size() const { return tree_->size(type_); }

Obj Array::at(uint32_t i) const {
    if (i >= n_) throw PictureError("array index outside the array");
    if (kind_ == kArrayOfRefs) {
        const uint64_t p = tree_->ptr(at_ + 8 * static_cast<uint64_t>(i));
        if (p) tree_->check(p, tree_->size(type_));
        return Obj(tree_, type_, p);
    }
    return Obj(tree_, type_, at_ + record_size() * i);
}

Array array(const Obj &o, const char *name) {
    const Member &m = o.member(name);
    const Tree &tree = o.tree();
    const uint64_t at = o.at() + m.offset;
    switch (m.kind) {
    case kRefArray: case kArrayOfRefs: {
        const uint32_t n = tree.u32(at);
        const uint64_t p = tree.ptr(at + 4);
        if (!p || !n) return Array(&tree, m.kind, m.sub, 0, 0);
        if (m.kind == kArrayOfRefs) {
            tree.check(p, 8 * static_cast<uint64_t>(n));
        } else {
            const uint64_t size = tree.size(m.sub);
            if (!size) throw PictureError(format("array %s of the empty type", name));
            if (n > (UINT64_MAX / size)) throw PictureError(format("array %s too long", name));
            tree.check(p, size * n);
        }
        return Array(&tree, m.kind, m.sub, p, n);
    }
    case kRefVariantArray: {
        const uint64_t t = tree.ptr(at);
        const uint32_t n = tree.u32(at + 8);
        const uint64_t p = tree.ptr(at + 12);
        if (!t || !p || !n) return Array(&tree, m.kind, t, 0, 0);
        const uint64_t size = tree.size(t);
        if (!size) throw PictureError(format("array %s of the empty type", name));
        if (n > (UINT64_MAX / size)) throw PictureError(format("array %s too long", name));
        tree.check(p, size * n);
        return Array(&tree, m.kind, t, p, n);
    }
    default: throw PictureError(format("member %s is no array", name));
    }
}

std::vector<double> flat_numbers(const Obj &o, const char *name) {
    const Member &m = o.member(name);
    if (numeric_kind(m.kind)) return o.numbers(name);
    const Array a = array(o, name);
    std::vector<double> out;
    if (!a.size()) return out;
    if (!a.records()) throw PictureError(format("%s is a list of references, not of numbers", name));
    const std::vector<Member> &fields = o.tree().layout(a.type());
    if (fields.empty() || !numeric_kind(fields[0].kind) || fields[0].count > 1)
        throw PictureError(format("%s is not a list of numbers", name));
    for (const Member &f : fields)
        if (!numeric_kind(f.kind)) throw PictureError(format("%s is not a list of numbers", name));
    out.resize(a.size());
    const uint64_t step = a.record_size();
    for (uint32_t i = 0; i < a.size(); i++) out[i] = o.tree().value(fields[0].kind, a.data() + step * i);
    return out;
}

std::unique_ptr<Tree> read_granny(const std::vector<uint8_t> &raw, const std::string &path) {
    auto fail = [&](const std::string &why) { return PictureError(path + ": " + why); };
    if (raw.size() < 0x44 || std::memcmp(raw.data(), kMagic, 16) != 0) throw fail("not a 64-bit little-endian model file");
    const uint32_t total = le32(&raw[0x24]), table = le32(&raw[0x2c]), count = le32(&raw[0x30]);
    if (total != raw.size()) throw fail(format("header names %u bytes, the file has %zu", total, raw.size()));
    if (uint64_t{0x20} + table + uint64_t{44} * count > raw.size()) throw fail("section table beyond the file");
    struct Section { uint32_t fmt, off, stored, size, fix, nfix; };
    std::vector<Section> secs(count);
    for (uint32_t i = 0; i < count; i++) {
        const uint8_t *e = &raw[0x20 + table + 44 * static_cast<size_t>(i)];
        Section s{le32(e), le32(e + 4), le32(e + 8), le32(e + 12), le32(e + 28), le32(e + 32)};
        if (s.fmt != 0 && s.fmt != 4) throw fail(format("section %u has format %u", i, s.fmt));
        if (s.size && uint64_t{s.off} + (s.fmt == 4 ? s.stored : s.size) > raw.size()) throw fail(format("section %u lies beyond the file", i));
        if (s.size > kMaxSection || uint64_t{12} * s.nfix > kMaxSection)
            throw fail(format("section %u expands to %u bytes with %u fixups, more than this reader takes", i, s.size, s.nfix));
        secs[i] = s;
    }
    std::vector<uint64_t> starts(count);
    uint64_t len = kBase;
    for (uint32_t i = 0; i < count; i++) {
        len += (32 - len % 32) % 32;
        starts[i] = len;
        len += secs[i].size;
        if (len > kMaxFile) throw fail(format("sections expand to more than %u bytes", kMaxFile));
    }
    std::vector<uint8_t> buf(len, 0);
    for (uint32_t i = 0; i < count; i++) {
        const Section &s = secs[i];
        if (!s.size) continue;
        if (s.fmt == 4) {
            const BitknitError e = bitknit_decode(&raw[s.off], s.stored, &buf[starts[i]], s.size);
            if (e != BitknitError::none) throw fail(format("section %u: %s", i, bitknit_error_text(e)));
        } else {
            std::memcpy(&buf[starts[i]], &raw[s.off], s.size);
        }
    }
    std::vector<uint8_t> fixups;
    for (uint32_t i = 0; i < count; i++) {
        const Section &s = secs[i];
        if (!s.nfix) continue;
        fixups.assign(12 * static_cast<size_t>(s.nfix), 0);
        if (s.fmt == 4) {
            if (uint64_t{s.fix} + 4 > raw.size()) throw fail(format("fixups of section %u lie beyond the file", i));
            const uint32_t n = le32(&raw[s.fix]);
            if (uint64_t{s.fix} + 4 + n > raw.size()) throw fail(format("fixups of section %u lie beyond the file", i));
            const BitknitError e = bitknit_decode(&raw[s.fix + 4], n, fixups.data(), fixups.size());
            if (e != BitknitError::none) throw fail(format("fixups of section %u: %s", i, bitknit_error_text(e)));
        } else {
            if (uint64_t{s.fix} + fixups.size() > raw.size()) throw fail(format("fixups of section %u lie beyond the file", i));
            std::memcpy(fixups.data(), &raw[s.fix], fixups.size());
        }
        for (uint32_t k = 0; k < s.nfix; k++) {
            const uint8_t *f = &fixups[12 * static_cast<size_t>(k)];
            const uint32_t at = le32(f), sec = le32(f + 4), to = le32(f + 8);
            if (uint64_t{at} + 8 > s.size || sec >= count || to > secs[sec].size)
                throw fail(format("fixup %u of section %u points outside the file", k, i));
            const uint64_t target = starts[sec] + to;
            for (int b = 0; b < 8; b++) buf[starts[i] + at + b] = static_cast<uint8_t>(target >> (8 * b));
        }
    }
    auto ref = [&](size_t at) {
        const uint32_t sec = le32(&raw[at]), off = le32(&raw[at + 4]);
        if (sec >= count || off > secs[sec].size) throw fail("root reference outside the file");
        return starts[sec] + off;
    };
    const uint64_t root_type = ref(0x34), root = ref(0x3c);
    return std::make_unique<Tree>(std::move(buf), root_type, root);
}

}
