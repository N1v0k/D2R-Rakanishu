
















#pragma once
#include <array>
#include <string>
#include <unordered_map>
#include <vector>

#include "granny.h"

namespace monpic {

using Mat4 = std::array<double, 16>;
using QuaternionRanges = std::array<std::array<double, 2>, 16>;

Mat4 identity4();
Mat4 mul(const Mat4 &a, const Mat4 &b);

std::string lower_ascii(std::string s);

class Skeleton {
public:
    explicit Skeleton(const Obj &granny_skeleton);

    std::vector<Mat4> pose(const std::vector<Obj> &tracks, const QuaternionRanges &ranges) const;
    std::vector<Mat4> rest_pose() const;

    int find(const std::string &lower_name) const {
        const auto it = index_.find(lower_name);
        return it == index_.end() ? -1 : it->second;
    }
    const Mat4 &inverse_bind(int i) const { return inverse_bind_[i]; }

private:
    struct Rest {
        std::array<double, 3> position;
        std::array<double, 4> orientation;
        std::array<double, 9> scale;
    };
    std::vector<std::string> names_;
    std::vector<int> parents_;
    std::vector<Rest> rest_;
    std::vector<Mat4> inverse_bind_;
    std::unordered_map<std::string, int> index_;
};



struct Mesh {
    std::vector<std::array<double, 3>> positions, normals;
    std::vector<std::array<double, 4>> tangents;
    std::vector<std::array<double, 2>> uvs;
    std::vector<std::array<int32_t, 3>> triangles;
    std::vector<int32_t> materials;
};



Mesh posed_mesh(const Obj &mesh, const Skeleton &model_skeleton, const std::vector<Mat4> &pose_world, const Skeleton &pose_skeleton);

}
