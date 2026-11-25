

#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>









size_t xbg_spirv_warp32(const uint32_t *words, size_t count, uint32_t subgroup_size, std::vector<uint32_t> &out);
