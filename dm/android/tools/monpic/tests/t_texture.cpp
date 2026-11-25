


#include "error.h"
#include "support.h"
#include "texture.h"

using monpic::PictureError;

namespace {


std::vector<uint8_t> texture(uint32_t w, uint32_t h, int block_height, uint32_t stored, int format = 0x3d) {
    std::vector<uint8_t> raw = {'<', 'D', 'E', '('};
    raw.resize(44, 0);
    raw[4] = static_cast<uint8_t>(format);
    raw[7] = 1;
    raw[23] = static_cast<uint8_t>(block_height);
    for (int i = 0; i < 4; i++) {
        raw[8 + i] = static_cast<uint8_t>(w >> (8 * i));
        raw[12 + i] = static_cast<uint8_t>(h >> (8 * i));
        raw[36 + i] = static_cast<uint8_t>(stored >> (8 * i));
    }
    raw[40] = 4;
    raw.resize(44 + stored, 0);
    return raw;
}

}

int main() {
    for (const auto &c : {std::make_tuple(8192u, 7, 16u), std::make_tuple(4096u, 7, 16u), std::make_tuple(64u, 200, 16u)})
        CHECK(test::throws<PictureError>([&] { monpic::decode_texture(texture(std::get<0>(c), std::get<0>(c), std::get<1>(c), std::get<2>(c)), "t"); }),
              "texture %u with block height %d refused", std::get<0>(c), std::get<1>(c));
    const monpic::Texture t = monpic::decode_texture(texture(8, 4, 7, 512), "t");
    CHECK(t.width == 8 && t.height == 4 && t.rgba.size() == 8 * 4 * 4, "a level that holds its layout decodes");
    std::vector<uint8_t> cut = texture(8, 4, 7, 512);
    cut.pop_back();
    CHECK(test::throws<PictureError>([&] { monpic::decode_texture(cut, "t"); }), "level beyond the file");
    CHECK(test::throws<PictureError>([&] { monpic::decode_texture(texture(8, 4, 7, 512, 0x47), "t"); }), "a format that is not BC3");
    CHECK(test::throws<PictureError>([&] { monpic::decode_texture({'x', 'y'}, "t"); }), "not a texture");



    uint8_t block[16] = {255, 0, 2 | 7 << 3, 0, 0, 0, 0, 0, 0x00, 0xF8, 0, 0, 2 | 3 << 2, 0, 0, 0};
    uint8_t px[64];
    monpic::bc3_decode_block(block, px);
    CHECK(px[0] == 170 && px[1] == 0 && px[2] == 0 && px[3] == 219 && px[4] == 85 && px[7] == 36 && px[8] == 255 && px[11] == 255,
          "eight-value alpha and the colour palette: %d %d %d %d", px[0], px[3], px[4], px[7]);

    uint8_t six[16] = {0, 255, (2 | 6 << 3 | 7 << 6) & 255, (2 | 6 << 3 | 7 << 6) >> 8, 0, 0, 0, 0};
    monpic::bc3_decode_block(six, px);
    CHECK(px[3] == 51 && px[7] == 0 && px[11] == 255, "six-value alpha: %d %d %d", px[3], px[7], px[11]);
    std::printf("t_texture: %d checks passed\n", test::checks);
    return 0;
}
