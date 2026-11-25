
#pragma once
#include <cstddef>
#include <cstdint>

namespace monpic {

enum class BitknitError {
    none = 0,
    magic,
    truncated,
    command,
    final_state,
};



BitknitError bitknit_decode(const uint8_t *src, size_t src_len, uint8_t *dst, size_t dst_len);

const char *bitknit_error_text(BitknitError e);

}
