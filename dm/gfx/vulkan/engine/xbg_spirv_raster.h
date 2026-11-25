


#pragma once
#include <cstddef>
#include <cstdint>

struct XbgRasterFacts {



    int pos_input=-1;

    bool pos_w1=false;

    uint32_t outputs=0;

    bool sample_mask=false;
};


XbgRasterFacts xbg_spirv_raster_facts(const uint32_t *words, size_t count, uint32_t stage);
