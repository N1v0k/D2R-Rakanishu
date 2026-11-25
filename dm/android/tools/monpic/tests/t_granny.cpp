

#include <cstring>
#include <string>

#include "error.h"
#include "granny.h"
#include "support.h"

using monpic::Array;
using monpic::Obj;
using monpic::PictureError;
using monpic::read_granny;

namespace {

std::vector<uint8_t> zeros(size_t n) { return std::vector<uint8_t>(n, 0); }

struct Figure {
    test::Container c;
    uint64_t root_type = 0, root = 0;
};



Figure figure() {
    Figure f;
    test::Container &c = f.c;
    const uint64_t level = c.type({{19, "Level", 0, 0}});
    const uint64_t vertex = c.type({{10, "Position", 0, 3}, {15, "Offset", 0, 2}});
    const uint64_t bone = c.type({{8, "Name", 0, 0}, {19, "ParentIndex", 0, 0}, {9, "Transform", 0, 0}, {1, "Extra", level, 0}});
    f.root_type = c.type({{8, "Name", 0, 0}, {3, "Bones", bone, 0}, {3, "Vertices", vertex, 0}, {2, "Child", level, 0},
                          {5, "Variant", 0, 0}, {2, "Empty", level, 0}, {12, "Bytes", 0, 4}});
    const uint64_t bones = c.put(zeros(2 * 84));
    const char *names[] = {"root", "arm"};
    for (int i = 0; i < 2; i++) {
        const uint64_t at = bones + 84 * i;
        c.pointer(at, c.string(names[i]));
        c.u32(at + 8, static_cast<uint32_t>(i - 1));
        const float t[16] = {0, 1.0f * i, 2, 3, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0};
        std::memcpy(&c.data[at + 16], t, 64);
        c.u32(at + 80, static_cast<uint32_t>(10 + i));
    }
    std::vector<uint8_t> v(2 * 16);
    const float p[6] = {1, 2, 3, 6, 7, 8};
    const int16_t o[4] = {-4, 5, 9, -10};
    for (int i = 0; i < 2; i++) {
        std::memcpy(&v[16 * i], &p[3 * i], 12);
        std::memcpy(&v[16 * i + 12], &o[2 * i], 4);
    }
    const uint64_t verts = c.put(v, 4);
    const uint64_t child = c.put({42, 0, 0, 0}), var = c.put({7, 0, 0, 0});
    f.root = c.put(zeros(8 + 12 + 12 + 8 + 16 + 8 + 4));
    const uint64_t r = f.root;
    c.pointer(r, c.string("figure"));
    c.u32(r + 8, 2);
    c.pointer(r + 12, bones);
    c.u32(r + 20, 2);
    c.pointer(r + 24, verts);
    c.pointer(r + 32, child);
    c.pointer(r + 40, level);
    c.pointer(r + 48, var);
    std::memcpy(&c.data[r + 64], "\x01\x02\x03\x04", 4);
    return f;
}

void write32(std::vector<uint8_t> &f, size_t at, uint32_t v) { std::memcpy(&f[at], &v, 4); }

}

int main() {
    Figure f = figure();
    for (bool packed : {false, true}) {
        const auto t = read_granny(f.c.file(f.root_type, f.root, packed), "figure");
        const Obj r = t->root();
        CHECK(r.string("Name") == "figure", "name (%d)", packed);
        CHECK(r.object("Child").number("Level") == 42 && r.object("Variant").number("Level") == 7 && !r.object("Empty"), "references (%d)", packed);
        const Array bones = monpic::array(r, "Bones");
        CHECK(bones.size() == 2 && bones.at(0).string("Name") == "root" && bones.at(1).string("Name") == "arm" &&
                  bones.at(1).number("ParentIndex") == 0 && bones.at(0).number("ParentIndex") == -1, "array of records (%d)", packed);
        CHECK(bones.at(1).object("Extra").number("Level") == 11, "inline member (%d)", packed);
        const std::vector<double> x = monpic::flat_numbers(r, "Bytes");
        CHECK(x == std::vector<double>({1, 2, 3, 4}), "counted member (%d)", packed);
        const Array v = monpic::array(r, "Vertices");
        const auto &layout = t->layout(v.type());
        CHECK(v.size() == 2 && v.record_size() == 16 && layout[1].offset == 12 && v.at(1).numbers("Offset") == std::vector<double>({9, -10}) &&
                  v.at(0).numbers("Position") == std::vector<double>({1, 2, 3}), "numeric records (%d)", packed);
        CHECK(test::throws<PictureError>([&] { r.member("Missing"); }) && test::throws<PictureError>([&] { r.number("Name"); }) &&
                  test::throws<PictureError>([&] { monpic::array(r, "Name"); }), "members of the wrong kind or name refused");
    }

    const std::vector<uint8_t> raw = f.c.file(f.root_type, f.root, false), packed = f.c.file(f.root_type, f.root, true);
    auto refused = [](std::vector<uint8_t> bad, const char *what) {
        CHECK(test::throws<PictureError>([&] { read_granny(bad, what)->root().string("Name"); }), "refused: %s", what);
    };
    refused(std::vector<uint8_t>(64, 'x'), "wrong magic");
    std::vector<uint8_t> bad = raw;
    write32(bad, 0x24, static_cast<uint32_t>(raw.size() + 1));
    refused(bad, "size not the file size");
    bad = raw;
    write32(bad, raw.size() - 12 + 8, 0x7fff0000);
    refused(bad, "fixup outside the file");
    bad = raw;
    write32(bad, 0x44, 3);
    refused(bad, "unknown section format");
    bad = raw;
    write32(bad, 0x44 + 12, (64u << 20) + 1);
    refused(bad, "section larger than the reader takes");
    bad = raw;
    write32(bad, 0x44 + 28, static_cast<uint32_t>(raw.size() - 4));
    refused(bad, "fixup table beyond the file");
    bad = packed;
    write32(bad, 0x44 + 12, static_cast<uint32_t>(f.c.data.size() + 1));
    refused(bad, "packed section shorter than stated");
    bad = packed;
    bad[0x44 + 44 + 10] ^= 0x40;
    refused(bad, "damaged packed section");
    bad = raw;
    write32(bad, 0x30, 1000);
    refused(bad, "section table beyond the file");
    bad = raw;
    write32(bad, 0x3c, 5);
    refused(bad, "root in a section the file lacks");


    std::vector<uint8_t> huge(0x44 + 44 * 8, 0);
    std::memcpy(huge.data(), raw.data(), 16);
    write32(huge, 0x24, static_cast<uint32_t>(huge.size()));
    write32(huge, 0x2c, 0x24);
    write32(huge, 0x30, 8);
    for (int i = 0; i < 8; i++) {
        write32(huge, 0x44 + 44 * i, 4);
        write32(huge, 0x44 + 44 * i + 12, 64u << 20);
    }
    std::string why;
    try {
        read_granny(huge, "huge");
    } catch (const PictureError &e) {
        why = e.what();
    }
    CHECK(why.find("expand to more than") != std::string::npos, "sections expanding to more than a file may hold: %s", why.c_str());


    test::Container e;
    const uint64_t empty = e.type({});
    const uint64_t odd = e.type({{6, "Odd", 0, 0}});
    const uint64_t holder = e.type({{3, "Many", empty, 0}, {2, "Odd", odd, 0}, {2, "Far", odd, 0}});
    const uint64_t at = e.put(zeros(12 + 8 + 8));
    e.u32(at, 1u << 30);
    e.pointer(at + 4, e.put(zeros(8)));
    e.pointer(at + 12, e.put(zeros(8)));
    const uint64_t far = 0x7fffff0;
    std::memcpy(&e.data[at + 20], &far, 8);
    const auto t = read_granny(e.file(holder, at, false), "holder");
    CHECK(test::throws<PictureError>([&] { monpic::array(t->root(), "Many"); }), "records of an empty type refused");
    CHECK(test::throws<PictureError>([&] { t->root().object("Odd").number("Odd"); }), "unknown member kind refused");
    CHECK(test::throws<PictureError>([&] { t->root().object("Far"); }), "reference beyond the data refused");
    std::printf("t_granny: %d checks passed\n", test::checks);
    return 0;
}
