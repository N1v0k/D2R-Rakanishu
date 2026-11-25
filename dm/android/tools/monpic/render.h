





#pragma once
#include <array>
#include <memory>
#include <vector>

#include "model.h"
#include "texture.h"

namespace monpic {






struct ColourTransform {
    double adjust[4] = {0, 0, 0, 1};
    double tint[4] = {0, 0, 0, 0};
    double hue[6] = {0, 0, 0, 0, 0, 0};
    double sat[6] = {0, 0, 0, 0, 0, 0};
    bool identity() const;
    void apply(const double in[3], double out[3]) const;
};

using Variant = std::array<ColourTransform, 3>;





struct Material {
    std::shared_ptr<const Texture> albedo, mask, normal, orm;
    bool recolour = false;
    Variant variant;
    bool cutout = false;
    double cutoff = 0;
    double wrap = 0;
};


struct DrawnMesh {
    Mesh mesh;
    std::vector<int32_t> material_of_binding;
};

struct Camera {
    double yaw, elevation;
};


struct Picture {
    int size = 0;
    std::vector<uint8_t> rgba;
};


std::vector<Picture> render_pictures(const std::vector<DrawnMesh> &meshes, const std::vector<Material> &table, const Camera &camera,
                                     const std::vector<int> &sizes, int supersample, double sharpness);

}
