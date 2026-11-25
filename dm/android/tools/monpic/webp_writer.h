

#pragma once
#include <cstdint>
#include <vector>

#include "render.h"

namespace monpic {

std::vector<uint8_t> encode_webp(const Picture &p, bool lossless);

}
