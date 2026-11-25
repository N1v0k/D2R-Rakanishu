
#pragma once
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <string>
#include <vector>

namespace test {

extern int checks;
#define CHECK(cond, ...)                                                          \
    do {                                                                          \
        ::test::checks++;                                                         \
        if (!(cond)) {                                                            \
            std::fprintf(stderr, "%s:%d: CHECK failed: ", __FILE__, __LINE__);   \
            std::fprintf(stderr, __VA_ARGS__);                                    \
            std::fprintf(stderr, "\n");                                           \
            std::exit(1);                                                         \
        }                                                                         \
    } while (0)


template <typename E>
bool throws(const std::function<void()> &fn) {
    try {
        fn();
    } catch (const E &) {
        return true;
    }
    return false;
}

uint64_t rnd(uint64_t &state);


std::vector<uint8_t> bitknit_encode(const std::vector<uint8_t> &data, int stored_every = 0);


class Container {
public:
    Container();
    uint64_t put(const std::vector<uint8_t> &raw, size_t align = 8);
    uint64_t string(const std::string &s);
    void pointer(uint64_t at, uint64_t target);
    void u32(uint64_t at, uint32_t v);
    void f32(uint64_t at, float v);
    struct M {
        uint32_t kind;
        std::string name;
        uint64_t sub;
        int32_t count;
    };
    uint64_t type(const std::vector<M> &members);

    std::vector<uint8_t> file(uint64_t root_type, uint64_t root, bool packed) const;
    std::vector<uint8_t> data;

private:
    std::vector<std::pair<uint64_t, uint64_t>> fixups_;
};


struct Field;
using Object = std::vector<Field>;
struct Field {
    enum Kind { number, string, transform, inline_, ref, variant, records, variant_records } kind;
    std::string name;
    uint32_t number_kind = 0;
    std::vector<double> values;
    bool counted = false;
    std::string text;
    bool null = false;
    std::vector<Object> objects;
};
Field num(const std::string &name, uint32_t kind, std::vector<double> values, bool counted = false);
Field str(const std::string &name, const std::string &text);
Field xform(const std::string &name, std::vector<double> position, std::vector<double> orientation, std::vector<double> scale);
Field inl(const std::string &name, Object o);
Field ref(const std::string &name, Object o);
Field null_ref(const std::string &name);
Field var(const std::string &name, Object o);
Field recs(const std::string &name, std::vector<Object> list);
Field var_recs(const std::string &name, std::vector<Object> list);

std::vector<uint8_t> object_file(const Object &root, bool packed = false);

std::string temp_dir();
void write_file(const std::string &path, const std::vector<uint8_t> &data);
std::vector<uint8_t> read_file(const std::string &path);
std::string md5_hex(const std::vector<uint8_t> &data);

std::vector<uint8_t> zlib_stored(const std::vector<uint8_t> &data);

}
