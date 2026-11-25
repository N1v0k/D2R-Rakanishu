







#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace monpic {

struct Texture {
    uint32_t width = 0, height = 0;
    std::vector<uint8_t> rgba;
};

Texture decode_texture(const std::vector<uint8_t> &raw, const std::string &path);


void bc3_decode_block(const uint8_t *block, uint8_t out[64]);

}
