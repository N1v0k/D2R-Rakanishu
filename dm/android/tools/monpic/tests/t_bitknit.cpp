



#include <chrono>
#include <cstring>
#include <memory>

#include "bitknit.h"
#include "support.h"

using monpic::BitknitError;
using monpic::bitknit_decode;

namespace {


BitknitError decode(const std::vector<uint8_t> &src, size_t n, std::vector<uint8_t> *out) {
    std::unique_ptr<uint8_t[]> buf(new uint8_t[n ? n : 1]);
    std::unique_ptr<uint8_t[]> in(new uint8_t[src.size() ? src.size() : 1]);
    if (!src.empty()) std::memcpy(in.get(), src.data(), src.size());
    const BitknitError e = bitknit_decode(in.get(), src.size(), buf.get(), n);
    if (out) out->assign(buf.get(), buf.get() + n);
    return e;
}

std::vector<uint8_t> text(size_t n, uint64_t seed) {
    static const char *words[] = {"skeleton ", "fallen ", "zombie ", "andariel ", "the ", "of ", "bone ", "spear "};
    std::vector<uint8_t> d;
    while (d.size() < n) {
        const char *w = words[test::rnd(seed) % 8];
        d.insert(d.end(), w, w + std::strlen(w));
    }
    d.resize(n);
    return d;
}

std::vector<uint8_t> random_bytes(size_t n, uint64_t seed) {
    std::vector<uint8_t> d(n);
    for (auto &b : d) b = static_cast<uint8_t>(test::rnd(seed));
    return d;
}

void round_trip(const std::vector<uint8_t> &d, int stored_every, const char *what) {
    const std::vector<uint8_t> s = test::bitknit_encode(d, stored_every);
    std::vector<uint8_t> out;
    const BitknitError e = decode(s, d.size(), &out);
    CHECK(e == BitknitError::none && out == d, "%s: round trip of %zu bytes (%s)", what, d.size(), monpic::bitknit_error_text(e));
}

}

int main() {
    round_trip({0x42}, 0, "one byte");
    round_trip(text(5000, 1), 0, "text");
    round_trip(random_bytes(70000, 2), 0, "random bytes over two quanta");
    round_trip(std::vector<uint8_t>(200000, 7), 0, "one repeated byte: long copies at offset 1");
    std::vector<uint8_t> records;
    for (int i = 0; i < 30000; i++)
        for (int k = 0; k < 12; k++) records.push_back(static_cast<uint8_t>(k < 4 ? i >> (8 * k) : k * 17));
    round_trip(records, 0, "records: repeated offsets from the cache");

    std::vector<uint8_t> far = random_bytes(3 << 20, 3);
    const std::vector<uint8_t> head(far.begin(), far.begin() + 5000);
    far.insert(far.end(), head.begin(), head.end());
    round_trip(far, 0, "far offset");
    round_trip(far, 3, "far offset with stored quanta");
    round_trip(text(300000, 4), 1, "every quantum stored");

    std::vector<uint8_t> cross = random_bytes(65530, 5);
    cross.insert(cross.end(), cross.begin(), cross.begin() + 4000);
    round_trip(cross, 0, "copy across a quantum boundary");


    const std::vector<uint8_t> d = text(100000, 6), s = test::bitknit_encode(d);
    CHECK(decode({}, 10, nullptr) == BitknitError::magic, "empty stream");
    CHECK(decode({0x75, 0xB1, 0, 0}, 10, nullptr) == BitknitError::magic, "wrong magic");
    CHECK(decode(std::vector<uint8_t>(s.begin(), s.begin() + static_cast<long>(s.size() / 2)), d.size(), nullptr) == BitknitError::truncated,
          "stream cut in half");
    CHECK(decode({0xB1, 0x75}, 1, nullptr) == BitknitError::truncated, "magic only");
    CHECK(decode({0xB1, 0x75, 0, 0, 1, 2}, 4, nullptr) == BitknitError::truncated, "stored quantum shorter than its output");
    CHECK(decode(s, d.size() + 1, nullptr) != BitknitError::none, "output size one more than the stream holds");
    CHECK(decode(s, 0, nullptr) == BitknitError::none, "no output wanted");



    uint64_t seed = 0x105105;
    size_t refused = 0, cases = 0;
    const auto t0 = std::chrono::steady_clock::now();
    const std::vector<std::vector<uint8_t>> bases = {s, test::bitknit_encode(cross), test::bitknit_encode(records, 2),
                                                     test::bitknit_encode(random_bytes(3000, 7))};
    const std::vector<size_t> sizes = {d.size(), cross.size(), records.size(), 3000};
    for (int round = 0; round < 3000; round++) {
        const size_t which = test::rnd(seed) % bases.size();
        std::vector<uint8_t> m = bases[which];
        size_t n = sizes[which];
        switch (test::rnd(seed) % 6) {
        case 0: m.resize(test::rnd(seed) % m.size()); break;
        case 1: for (int k = 0; k < 4; k++) m[test::rnd(seed) % m.size()] ^= static_cast<uint8_t>(1u << (test::rnd(seed) % 8)); break;
        case 2: { const size_t p = test::rnd(seed) % m.size(); for (size_t k = p; k < p + 16 && k < m.size(); k++) m[k] = static_cast<uint8_t>(test::rnd(seed)); } break;
        case 3: for (size_t k = 2; k < m.size(); k++) m[k] = static_cast<uint8_t>(test::rnd(seed)); break;
        case 4: n = test::rnd(seed) % (2 * n + 1); break;
        default: m[2 + test::rnd(seed) % 8] ^= static_cast<uint8_t>(1u << (test::rnd(seed) % 8)); break;
        }
        cases++;
        refused += decode(m, n, nullptr) != BitknitError::none;
    }
    const double s_used = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    CHECK(refused > cases / 2, "most mutated streams refused: %zu of %zu", refused, cases);
    std::printf("t_bitknit: %d checks passed; %zu mutated streams in %.1f s, %zu refused\n", test::checks, cases, s_used, refused);
    return 0;
}
