














#pragma once
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace monpic {

class Tree;

enum Kind : uint32_t {
    kEnd = 0, kInline = 1, kRef = 2, kRefArray = 3, kArrayOfRefs = 4, kVariantRef = 5, kRefVariantArray = 7, kString = 8,
    kTransform = 9, kReal32 = 10, kInt8 = 11, kUInt8 = 12, kBinormalInt8 = 13, kNormalUInt8 = 14, kInt16 = 15, kUInt16 = 16,
    kBinormalInt16 = 17, kNormalUInt16 = 18, kInt32 = 19, kUInt32 = 20, kReal16 = 21, kEmptyReference = 22,
};

struct Member {
    uint32_t kind;
    std::string name;
    uint64_t sub;
    uint32_t count;
    uint64_t offset = 0;
    uint64_t size = 0;
};

bool numeric_kind(uint32_t kind);


class Obj {
public:
    Obj() = default;
    Obj(const Tree *tree, uint64_t type, uint64_t at) : tree_(tree), type_(type), at_(at) {}
    explicit operator bool() const { return at_ != 0; }
    const Tree &tree() const { return *tree_; }
    uint64_t type() const { return type_; }
    uint64_t at() const { return at_; }

    const std::vector<Member> &members() const;
    const Member *find(const char *name) const;
    const Member &member(const char *name) const;
    std::string first_member_name() const;

    Obj object(const char *name) const;
    std::string string(const char *name) const;
    bool has_string(const char *name, std::string *out) const;
    double number(const char *name) const;
    std::vector<double> numbers(const char *name) const;

private:
    const Tree *tree_ = nullptr;
    uint64_t type_ = 0, at_ = 0;
};


class Array {
public:
    Array() = default;
    Array(const Tree *tree, uint32_t kind, uint64_t type, uint64_t at, uint32_t n) : tree_(tree), kind_(kind), type_(type), at_(at), n_(n) {}
    uint32_t size() const { return n_; }
    uint64_t type() const { return type_; }
    Obj at(uint32_t i) const;
    bool records() const { return kind_ != kArrayOfRefs; }
    uint64_t record_size() const;
    uint64_t data() const { return at_; }

private:
    const Tree *tree_ = nullptr;
    uint32_t kind_ = 0;
    uint64_t type_ = 0, at_ = 0;
    uint32_t n_ = 0;
};

Array array(const Obj &o, const char *name);



std::vector<double> flat_numbers(const Obj &o, const char *name);

class Tree {
public:
    Tree(std::vector<uint8_t> buf, uint64_t root_type, uint64_t root) : buf_(std::move(buf)), root_type_(root_type), root_(root) {}
    Obj root() const { return Obj(this, root_type_, root_); }

    void check(uint64_t at, uint64_t n) const;
    uint64_t ptr(uint64_t at) const;
    uint32_t u32(uint64_t at) const;
    std::string cstring(uint64_t at) const;
    double value(uint32_t kind, uint64_t at) const;
    const uint8_t *bytes(uint64_t at, uint64_t n) const {
        check(at, n);
        return buf_.data() + at;
    }

    const std::vector<Member> &members(uint64_t type) const;
    const std::vector<Member> &layout(uint64_t type) const;
    uint64_t size(uint64_t type) const;

private:
    uint64_t size_at(uint64_t type, int depth) const;
    std::vector<uint8_t> buf_;
    uint64_t root_type_, root_;
    mutable std::mutex mutex_;
    mutable std::unordered_map<uint64_t, std::unique_ptr<std::vector<Member>>> members_, layouts_;
    mutable std::unordered_map<uint64_t, uint64_t> sizes_;
};


std::unique_ptr<Tree> read_granny(const std::vector<uint8_t> &raw, const std::string &path);

}
