

#include <algorithm>
#include <cstdlib>

#include "render.h"
#include "support.h"
#include "webp/decode.h"
#include "webp_writer.h"

using namespace monpic;

namespace {

Picture pattern(int size, uint64_t seed) {
    Picture p;
    p.size = size;
    p.rgba.resize(static_cast<size_t>(size) * size * 4);
    for (int y = 0; y < size; y++)
        for (int x = 0; x < size; x++) {
            uint8_t *q = &p.rgba[(static_cast<size_t>(y) * size + x) * 4];
            const bool inside = (x - size / 2) * (x - size / 2) + (y - size / 2) * (y - size / 2) < size * size / 9;
            q[0] = static_cast<uint8_t>(x * 255 / size);
            q[1] = static_cast<uint8_t>(y * 255 / size);
            q[2] = static_cast<uint8_t>(test::rnd(seed) % 32 + 100);
            q[3] = inside ? 255 : (x + y) % 7 == 0 ? 128 : 0;
            if (!q[3]) q[0] = q[1] = q[2] = 0;
        }
    return p;
}

std::vector<uint8_t> decode(const std::vector<uint8_t> &file, int size) {
    int w = 0, h = 0;
    uint8_t *px = WebPDecodeRGBA(file.data(), file.size(), &w, &h);
    CHECK(px && w == size && h == size, "decodes at %d px", size);
    std::vector<uint8_t> out(px, px + static_cast<size_t>(w) * h * 4);
    WebPFree(px);
    return out;
}

}

int main() {
    const Picture small = pattern(96, 1), big = pattern(256, 2);
    const std::vector<uint8_t> lossless = encode_webp(small, true), lossy = encode_webp(big, false);
    CHECK(lossless.size() > 12 && std::string(lossless.begin() + 8, lossless.begin() + 12) == "WEBP", "a WebP file");

    const std::vector<uint8_t> same = decode(lossless, 96);
    bool equal = true;
    for (size_t i = 0; i < same.size(); i += 4)
        equal = equal && same[i + 3] == small.rgba[i + 3] && (!same[i + 3] || std::equal(&same[i], &same[i + 3], &small.rgba[i]));
    CHECK(equal, "lossless: the same visible pixels");
    const std::vector<uint8_t> back = decode(lossy, 256);
    long worst = 0, total = 0, n = 0;
    bool alpha = true;
    for (size_t i = 0; i < back.size(); i += 4) {
        alpha = alpha && back[i + 3] == big.rgba[i + 3];
        if (back[i + 3] < 255) continue;
        for (int k = 0; k < 3; k++) {
            const long d = std::labs(static_cast<long>(back[i + k]) - big.rgba[i + k]);
            worst = std::max(worst, d);
            total += d;
            n++;
        }
    }
    CHECK(alpha, "lossy: alpha kept exactly");
    CHECK(total < 6 * n && worst < 80, "lossy: colours close to the input (mean %.2f, worst %ld)", static_cast<double>(total) / n, worst);
    CHECK(encode_webp(small, true) == lossless && encode_webp(big, false) == lossy, "the same pixels give the same files");
    std::printf("t_webp: %d checks passed\n", test::checks);
    return 0;
}
