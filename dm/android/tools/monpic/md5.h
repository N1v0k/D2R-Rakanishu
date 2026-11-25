
#pragma once
#include <cstddef>
#include <cstdint>

namespace monpic {

void md5(const uint8_t *data, size_t len, uint8_t digest[16]);

}
