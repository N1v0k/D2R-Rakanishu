



#include <cmath>

#include "error.h"
#include "granny.h"
#include "model.h"
#include "support.h"

using namespace test;
using monpic::Mat4;
using monpic::PictureError;

namespace {

constexpr uint32_t kReal32 = 10, kUInt8 = 12, kInt16 = 15, kUInt16 = 16, kInt32 = 19, kUInt32 = 20, kReal16 = 21;
const monpic::QuaternionRanges kRanges = [] {
    monpic::QuaternionRanges r;
    for (auto &e : r) e = {2.0, -1.0};
    return r;
}();

std::vector<Object> list(uint32_t kind, std::vector<double> values) {
    std::vector<Object> out;
    for (double v : values) out.push_back({num("V", kind, {v})});
    return out;
}

Object curve(const std::string &header, std::vector<Field> fields = {}) {
    Object data{num("CurveDataHeader_" + header, kUInt8, {0})};
    data.insert(data.end(), fields.begin(), fields.end());
    return {var("CurveData", data)};
}

Object identity(int dim) { return curve("DaIdentity", {num("Dimension", kInt16, {static_cast<double>(dim)})}); }

Object track(const std::string &bone, Object o, Object p, Object s) {
    return {str("Name", bone), inl("OrientationCurve", std::move(o)), inl("PositionCurve", std::move(p)), inl("ScaleShearCurve", std::move(s))};
}

Object bone(const std::string &name, int parent, std::vector<double> at) {
    std::vector<double> inv(16, 0);
    inv[0] = inv[5] = inv[10] = inv[15] = 1;
    for (int i = 0; i < 3; i++) inv[12 + i] = -at[i];
    return {str("Name", name), num("ParentIndex", kInt32, {static_cast<double>(parent)}),
            xform("Transform", at, {0, 0, 0, 1}, {1, 0, 0, 0, 1, 0, 0, 0, 1}), num("InverseWorldTransform", kReal32, inv)};
}

Object skeleton(std::vector<Object> bones) { return {str("Name", "sk"), recs("Bones", std::move(bones))}; }

Object vertex(double x, double y, int bone_index, int weight, double u, double v) {
    return {num("Position", kReal32, {x, y, 0}), num("BoneWeights", kUInt8, {static_cast<double>(weight), 0, 0, 0}),
            num("BoneIndices", kUInt8, {static_cast<double>(bone_index), 0, 0, 0}), num("Normal", kReal32, {0, 0, 1}),
            num("Tangent", kReal32, {1, 0, 0, 1}), num("TextureCoordinates0", kReal16, {u, v})};
}


Object quad(const std::string &top_bone, int first_weight = 255) {
    Object topo{recs("Groups", {{num("MaterialIndex", kInt32, {0}), num("TriFirst", kInt32, {0}), num("TriCount", kInt32, {2})},
                               {num("MaterialIndex", kInt32, {1}), num("TriFirst", kInt32, {0}), num("TriCount", kInt32, {2})}}),
                recs("Indices16", list(kInt16, {0, 1, 2, 0, 2, 3})), recs("Indices", {})};
    return {str("Name", "q"),
            ref("PrimaryVertexData", {var_recs("Vertices", {vertex(-0.5, 0, 0, first_weight, 0, 1), vertex(0.5, 0, 0, 255, 1, 1),
                                                            vertex(0.5, 1, 1, 255, 1, 0), vertex(-0.5, 1, 1, 255, 0, 0)})}),
            recs("BoneBindings", {{str("BoneName", "root")}, {str("BoneName", top_bone)}}), ref("PrimaryTopology", topo),
            recs("MaterialBindings", {{ref("Material", {str("Name", "alive")})}, {ref("Material", {str("Name", "dead")})}}),
            var("ExtendedData", {recs("MaterialSets", {{num("FirstIndex", kInt32, {0}), num("Count", kInt32, {1})},
                                                        {num("FirstIndex", kInt32, {1}), num("Count", kInt32, {1})}})})};
}

struct Posed {
    std::unique_ptr<monpic::Tree> tree;
    std::vector<Mat4> world;
};


Posed pose(const Object &sk, std::vector<Object> tracks, const Object *mesh = nullptr) {
    Object root{ref("Skeleton", sk), recs("Tracks", std::move(tracks))};
    if (mesh) root.push_back(ref("Mesh", *mesh));
    Posed p;
    p.tree = monpic::read_granny(object_file(root), "made-up");
    const monpic::Skeleton s(p.tree->root().object("Skeleton"));
    const monpic::Array a = monpic::array(p.tree->root(), "Tracks");
    std::vector<monpic::Obj> t;
    for (uint32_t i = 0; i < a.size(); i++) t.push_back(a.at(i));
    p.world = s.pose(t, kRanges);
    return p;
}


Mat4 one(Object o, Object p, Object s) { return pose(skeleton({bone("b", -1, {0, 0, 0})}), {track("b", o, p, s)}).world[0]; }

bool close(const Mat4 &m, const std::vector<double> &rot3, const std::vector<double> &t, double tol) {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++)
            if (std::fabs(m[4 * i + j] - rot3[3 * i + j]) > tol) return false;
        if (std::fabs(m[4 * i + 3] - t[i]) > tol) return false;
    }
    return true;
}

std::vector<double> rotation(double x, double y, double z, double w) {
    const double n = std::sqrt(x * x + y * y + z * z + w * w);
    x /= n, y /= n, z /= n, w /= n;
    return {1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w), 2 * (x * y + z * w), 1 - 2 * (x * x + z * z),
            2 * (y * z - x * w), 2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)};
}

const std::vector<double> kEye = {1, 0, 0, 0, 1, 0, 0, 0, 1};

}

int main() {

    CHECK(close(one(identity(4), curve("D3Constant32f", {num("Controls", kReal32, {1, 2, 3})}), identity(9)), kEye, {1, 2, 3}, 0), "constant");
    CHECK(close(one(identity(4), curve("D3K8uC8u", {num("ControlScales", kReal32, {1, 2, 3}), num("ControlOffsets", kReal32, {10, 20, 30}),
                                                    recs("KnotsControls", list(kUInt8, {0, 9, 1, 2, 3, 4, 5, 6}))}),
                    identity(9)), kEye, {11, 24, 39}, 0), "quantized: offset + scale x first control");
    CHECK(close(one(identity(4), curve("D3I1K16uC16u", {num("ControlScales", kReal32, {1, -1, 2}), num("ControlOffsets", kReal32, {0, 0, 1}),
                                                        recs("KnotsControls", list(kUInt16, {0, 5, 7, 9}))}),
                    identity(9)), kEye, {7, -7, 15}, 0), "one control for three dimensions");

    CHECK(close(one(curve("DaK32fC32f", {recs("Knots", list(kReal32, {0, 0.5})), recs("Controls", list(kReal32, {0, 0, 0, 1, .5, .5, .5, .5}))}),
                    identity(3), identity(9)), kEye, {0, 0, 0}, 1e-12), "float knots: the first control");
    const double mid = (1 << 15) / 2;
    CHECK(close(one(curve("D4nK16uC15u", {num("ScaleOffsetTableEntries", kUInt32, {0}), num("OneOverKnotScale", kReal32, {1}),
                                          recs("KnotsControls", list(kUInt16, {0, mid, mid + 32768, mid + 32768}))}),
                    identity(3), identity(9)), kEye, {0, 0, 0}, 2e-4), "quaternion with w dropped, positive, x y z at their mid value");
    CHECK(close(one(curve("D4nK8uC7u", {num("ScaleOffsetTableEntries", kUInt32, {0}), num("OneOverKnotScale", kReal32, {1}),
                                        recs("KnotsControls", list(kUInt8, {0, 95 + 128, 95, 95}))}),
                    identity(3), identity(9)), rotation(-0.512, 0.496, 0.496, 0.496), {0, 0, 0}, 5e-3),
          "quaternion with x dropped and negative: y z w stored after it");

    CHECK(close(one(identity(4), identity(3), curve("D9I1K8uC8u", {num("ControlScale", kReal32, {0.5}), num("ControlOffset", kReal32, {1}),
                                                                   recs("KnotsControls", list(kUInt8, {0, 4}))})),
                {3, 0, 0, 0, 3, 0, 0, 0, 3}, {0, 0, 0}, 0), "uniform scale");
    std::vector<double> so(9, 1);
    so.resize(18, 0);
    CHECK(close(one(identity(4), identity(3), curve("DaK16uC16u", {recs("ControlScaleOffsets", list(kReal32, so)),
                                                                   recs("KnotsControls", list(kUInt16, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}))})),
                {1, 2, 3, 4, 5, 6, 7, 8, 9}, {0, 0, 0}, 0), "per-dimension scale and offset");

    const Object d3 = curve("D3K16uC16u", {num("ControlScales", kReal32, {1, 1, 1}), num("ControlOffsets", kReal32, {0, 0, 0}),
                                           recs("KnotsControls", list(kUInt16, {0, 100, 200, 300}))});
    CHECK(throws<PictureError>([&] { one(identity(4), curve("D7Mystery"), identity(9)); }), "unknown encoding");
    CHECK(throws<PictureError>([&] { one(identity(4), curve("D3Constant32f", {num("Controls", kReal32, {1, 2})}), identity(9)); }), "constant of the wrong width");
    CHECK(throws<PictureError>([&] { one(d3, identity(3), identity(9)); }), "a three-value encoding as orientation");
    CHECK(throws<PictureError>([&] { one(identity(4), identity(3), d3); }), "a three-value encoding as scale");


    const Object sk = skeleton({bone("Root", -1, {0, 0, 0}), bone("Arm", 0, {0, 1, 0})});
    const Object lift_pos = curve("D3Constant32f", {num("Controls", kReal32, {0, 2, 0})});
    const Object q = quad("ARM");
    const Posed lifted = pose(sk, {track("arm", identity(4), lift_pos, identity(9))}, &q);
    CHECK(close(lifted.world[1], kEye, {0, 2, 0}, 1e-12), "a track moves its bone");
    CHECK(close(pose(sk, {}).world[1], kEye, {0, 1, 0}, 1e-12), "without a track the bone keeps its rest");
    CHECK(close(pose(sk, {track("arm", identity(4), identity(3), identity(9))}).world[1], kEye, {0, 0, 0}, 1e-12),
          "an identity curve replaces the rest value");
    const monpic::Skeleton s(lifted.tree->root().object("Skeleton"));
    const monpic::Mesh m = monpic::posed_mesh(lifted.tree->root().object("Mesh"), s, lifted.world, s);
    CHECK(m.positions[0][1] == 0 && m.positions[1][1] == 0 && std::fabs(m.positions[2][1] - 2) < 1e-12 && std::fabs(m.positions[3][1] - 2) < 1e-12,
          "points follow their bone");
    CHECK(m.materials == std::vector<int32_t>({0, 0}) && m.uvs[2][0] == 1 && m.uvs[2][1] == 0, "the first material set, texture coordinates");
    const double h = std::sqrt(0.5);
    const Posed turned = pose(sk, {track("arm", curve("D4Constant32f", {num("Controls", kReal32, {0, 0, h, h})}),
                                         curve("D3Constant32f", {num("Controls", kReal32, {0, 1, 0})}), identity(9))}, &q);
    const monpic::Mesh t = monpic::posed_mesh(turned.tree->root().object("Mesh"), s, turned.world, s);
    CHECK(std::fabs(t.positions[2][0]) < 1e-6 && std::fabs(t.positions[2][1] - 1.5) < 1e-6 && std::fabs(t.normals[2][2] - 1) < 1e-6,
          "rotation about the bone: (%g, %g)", t.positions[2][0], t.positions[2][1]);

    const Object tail = quad("tail"), weightless = quad("ARM", 0);
    const Posed loose = pose(skeleton({bone("Root", -1, {0, 0, 0}), bone("Arm", 0, {0, 1, 0}), bone("Tail", 0, {0, 1, 0})}), {}, &tail);
    const monpic::Skeleton own(loose.tree->root().object("Skeleton"));
    const monpic::Mesh still = monpic::posed_mesh(loose.tree->root().object("Mesh"), own, lifted.world, s);
    CHECK(still.positions[0][1] == 0 && std::fabs(still.positions[2][1] - 1) < 1e-12 && std::fabs(still.positions[3][1] - 1) < 1e-12,
          "points of a bone the pose lacks keep their bind place: %g", still.positions[2][1]);

    CHECK(throws<PictureError>([&] {
              const Posed p = pose(sk, {}, &tail);
              monpic::posed_mesh(p.tree->root().object("Mesh"), s, p.world, s);
          }), "binding to a bone the skeleton lacks");
    CHECK(throws<PictureError>([&] {
              const Posed p = pose(sk, {}, &weightless);
              monpic::posed_mesh(p.tree->root().object("Mesh"), s, p.world, s);
          }), "vertex without weight");
    CHECK(throws<PictureError>([&] { pose(skeleton({bone("Root", 3, {0, 0, 0})}), {}); }), "bone with a later parent");
    std::printf("t_model: %d checks passed\n", test::checks);
    return 0;
}
