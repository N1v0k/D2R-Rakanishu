#include "texture.h"

#include <cstring>

#include "error.h"

extern "C" {
#include "nvn_deswizzle.h"
}

namespace monpic {
namespace {

constexpr int kMaxBlockHeight = 7;
constexpr int kNvnBc3 = 0x44, kNvnBc3Srgb = 0x48;

uint32_t le32(const uint8_t *p) { return p[0] | p[1] << 8 | p[2] << 16 | static_cast<uint32_t>(p[3]) << 24; }

void rgb565(uint32_t c, int out[3]) {
    const int r = c >> 11 & 31, g = c >> 5 & 63, b = c & 31;
    out[0] = r << 3 | r >> 2;
    out[1] = g << 2 | g >> 4;
    out[2] = b << 3 | b >> 2;
}

}

void bc3_decode_block(const uint8_t *b, uint8_t out[64]) {
    const int a0 = b[0], a1 = b[1];
    int alpha[8];
    alpha[0] = a0;
    alpha[1] = a1;
    for (int k = 2; k < 8; k++) {
        if (a0 > a1) alpha[k] = ((8 - k) * a0 + (k - 1) * a1 + 3) / 7;
        else alpha[k] = k == 6 ? 0 : k == 7 ? 255 : ((6 - k) * a0 + (k - 1) * a1 + 2) / 5;
    }
    uint64_t abits = 0;
    for (int i = 0; i < 6; i++) abits |= static_cast<uint64_t>(b[2 + i]) << (8 * i);
    int e0[3], e1[3], pal[4][3];
    rgb565(b[8] | b[9] << 8, e0);
    rgb565(b[10] | b[11] << 8, e1);
    for (int c = 0; c < 3; c++) {
        pal[0][c] = e0[c];
        pal[1][c] = e1[c];
        pal[2][c] = (2 * e0[c] + e1[c] + 1) / 3;
        pal[3][c] = (e0[c] + 2 * e1[c] + 1) / 3;
    }
    const uint32_t cbits = le32(b + 12);
    for (int i = 0; i < 16; i++) {
        const int ci = cbits >> (2 * i) & 3;
        for (int c = 0; c < 3; c++) out[4 * i + c] = static_cast<uint8_t>(pal[ci][c]);
        out[4 * i + 3] = static_cast<uint8_t>(alpha[abits >> (3 * i) & 7]);
    }
}

Texture decode_texture(const std::vector<uint8_t> &raw, const std::string &path) {
    auto fail = [&](const std::string &why) { return PictureError(path + ": " + why); };
    if (raw.size() < 44 || std::memcmp(raw.data(), "<DE(", 4) != 0) throw fail("not a texture file");
    const int fmt = raw[4], mips = raw[7];
    const uint32_t w = le32(&raw[8]), h = le32(&raw[12]);
    if (fmt != 0x3d && fmt != 0x3e) throw fail(format("texture format code %#x is not BC3", fmt));
    if (!(w > 0 && w <= 8192 && h > 0 && h <= 8192 && mips > 0 && mips <= 16) || raw.size() < 36 + 8 * static_cast<size_t>(mips))
        throw fail(format("texture header %ux%u with %d levels", w, h, mips));
    if (raw[23] > kMaxBlockHeight) throw fail(format("texture block height code %d", raw[23]));
    const uint64_t size = le32(&raw[36]), start = 40 + uint64_t{le32(&raw[40])};
    if (start + size > raw.size()) throw fail("texture level 0 lies beyond the file");
    for (int i = 1; i < mips; i++)
        if (40 + 8 * uint64_t(i) + le32(&raw[40 + 8 * i]) + le32(&raw[36 + 8 * i]) > raw.size()) throw fail(format("texture level %d lies beyond the file", i));

    nvn_texdesc desc{};
    desc.target = 1;
    desc.format = fmt == 0x3d ? kNvnBc3 : kNvnBc3Srgb;
    desc.width = w;
    desc.height = h;
    desc.depth = 1;
    desc.levels = 1;
    desc.pkg_block_h = raw[23] + 1;
    nvn_texlayout layout{};
    nvn_tex_layout(&desc, &layout);
    if (size < layout.size) throw fail(format("texture level of %ux%u holds %llu bytes, its layout spans %llu", w, h,
                                              static_cast<unsigned long long>(size), static_cast<unsigned long long>(layout.size)));
    const uint32_t bw = (w + 3) / 4, bh = (h + 3) / 4;
    const size_t pitch = static_cast<size_t>(bw) * 16;
    std::vector<uint8_t> lin(pitch * bh);
    if (nvn_deswizzle(&desc, 0, 0, raw.data() + start, lin.data(), pitch) != 0) throw fail(format("texture of %ux%u could not be un-tiled", w, h));
    Texture t;
    t.width = w;
    t.height = h;
    t.rgba.assign(static_cast<size_t>(w) * h * 4, 0);
    uint8_t px[64];
    for (uint32_t by = 0; by < bh; by++)
        for (uint32_t bx = 0; bx < bw; bx++) {
            bc3_decode_block(&lin[by * pitch + bx * 16], px);
            for (uint32_t y = 0; y < 4 && by * 4 + y < h; y++)
                for (uint32_t x = 0; x < 4 && bx * 4 + x < w; x++)
                    std::memcpy(&t.rgba[((by * 4 + y) * static_cast<size_t>(w) + bx * 4 + x) * 4], &px[(4 * y + x) * 4], 4);
        }
    return t;
}

}
