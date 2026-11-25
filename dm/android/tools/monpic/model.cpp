#include "model.h"

#include <cmath>
#include <cstring>

#include "error.h"

namespace monpic {
namespace {

using Vec = std::vector<double>;

Mat4 local_matrix(const std::array<double, 3> &position, const std::array<double, 4> &orientation, const std::array<double, 9> &scale) {
    const double n = std::sqrt(orientation[0] * orientation[0] + orientation[1] * orientation[1] + orientation[2] * orientation[2] +
                               orientation[3] * orientation[3]);
    if (!(n > 0)) throw PictureError("bone orientation of zero length");
    const double x = orientation[0] / n, y = orientation[1] / n, z = orientation[2] / n, w = orientation[3] / n;
    const double r[9] = {1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w),
                         2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w),
                         2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)};
    Mat4 m = identity4();
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) m[4 * i + j] = r[3 * i] * scale[j] + r[3 * i + 1] * scale[3 + j] + r[3 * i + 2] * scale[6 + j];
    for (int i = 0; i < 3; i++) m[4 * i + 3] = position[i];
    return m;
}


Vec first_control(const Vec &a, size_t dim) {
    if (a.empty() || a.size() % (dim + 1)) throw PictureError(format("curve of %zu values does not split into knots and %zu-wide controls", a.size(), dim));
    const size_t k = a.size() / (dim + 1);
    return Vec(a.begin() + static_cast<long>(k), a.begin() + static_cast<long>(k + dim));
}


Vec scaled3(const Obj &data, const Vec &c) {
    Vec off = data.numbers("ControlOffsets"), sc = data.numbers("ControlScales");
    auto widen = [](Vec v) { return v.size() == 1 ? Vec(3, v[0]) : v; };
    off = widen(off);
    sc = widen(sc);
    Vec cc = c.size() == 1 ? Vec(3, c[0]) : c;
    if (off.size() != 3 || sc.size() != 3 || cc.size() != 3) throw PictureError("quantized curve with other than three offsets and scales");
    return {off[0] + sc[0] * cc[0], off[1] + sc[1] * cc[1], off[2] + sc[2] * cc[2]};
}

Vec diagonal(const Vec &d) { return {d[0], 0, 0, 0, d[1], 0, 0, 0, d[2]}; }


Vec curve_values(const Obj &curve, size_t dim, const QuaternionRanges &ranges) {
    const Obj data = curve.object("CurveData");
    if (!data) throw PictureError("a curve without data");
    std::string fmt = data.first_member_name();
    const std::string prefix = "CurveDataHeader_";
    if (fmt.compare(0, prefix.size(), prefix) == 0) fmt = fmt.substr(prefix.size());
    if (fmt == "DaIdentity") return {};
    if (fmt == "D3Constant32f" || fmt == "D4Constant32f" || fmt == "DaConstant32f") {
        Vec v = flat_numbers(data, "Controls");
        if (v.size() != dim) throw PictureError(format("constant curve of %zu values where %zu are needed", v.size(), dim));
        return v;
    }
    if (fmt == "DaK32fC32f") {
        const Vec knots = flat_numbers(data, "Knots"), controls = flat_numbers(data, "Controls");
        if (knots.empty() || controls.size() != dim * knots.size())
            throw PictureError(format("float curve with %zu knots and %zu controls", knots.size(), controls.size()));
        return Vec(controls.begin(), controls.begin() + static_cast<long>(dim));
    }
    if (fmt == "DaK16uC16u" || fmt == "DaK8uC8u") {
        const Vec so = flat_numbers(data, "ControlScaleOffsets");
        if (so.size() != 2 * dim) throw PictureError(format("quantized curve with %zu scale/offset values for %zu dimensions", so.size(), dim));
        const Vec c = first_control(flat_numbers(data, "KnotsControls"), dim);
        Vec out(dim);
        for (size_t i = 0; i < dim; i++) out[i] = so[dim + i] + so[i] * c[i];
        return out;
    }
    if (fmt == "D3K16uC16u" || fmt == "D3K8uC8u") return scaled3(data, first_control(flat_numbers(data, "KnotsControls"), 3));
    if (fmt == "D3I1K16uC16u" || fmt == "D3I1K8uC8u") return scaled3(data, first_control(flat_numbers(data, "KnotsControls"), 1));
    if (fmt == "D9I1K16uC16u" || fmt == "D9I1K8uC8u") {
        const double s = data.number("ControlOffset") + data.number("ControlScale") * first_control(flat_numbers(data, "KnotsControls"), 1)[0];
        return diagonal({s, s, s});
    }
    if (fmt == "D9I3K16uC16u" || fmt == "D9I3K8uC8u") return diagonal(scaled3(data, first_control(flat_numbers(data, "KnotsControls"), 3)));
    if (fmt == "D4nK16uC15u" || fmt == "D4nK8uC7u") {
        const int bits = fmt == "D4nK16uC15u" ? 15 : 7;
        const Vec c = first_control(flat_numbers(data, "KnotsControls"), 3);
        int64_t raw[3], top[3], value[3];
        for (int i = 0; i < 3; i++) {
            if (!(c[i] >= 0 && c[i] < 65536)) throw PictureError("quantized quaternion control outside 16 bits");
            raw[i] = static_cast<int64_t>(c[i]);
            top[i] = raw[i] >> bits & 1;
            value[i] = raw[i] & ((int64_t{1} << bits) - 1);
        }
        const int drop = static_cast<int>(top[1] * 2 + top[2]);
        const bool negative = top[0] != 0;
        const double sel_value = data.number("ScaleOffsetTableEntries");
        if (!(sel_value >= 0 && sel_value < 4294967296.0)) throw PictureError("quaternion selector outside 32 bits");
        const uint64_t sel = static_cast<uint64_t>(sel_value);
        Vec q(4, 0.0);
        for (int slot = 0; slot < 3; slot++) {
            const int comp = (drop + 1 + slot) % 4;
            const auto &r = ranges[(sel >> (4 * comp)) & 15];
            q[comp] = static_cast<double>(value[slot]) / static_cast<double>((int64_t{1} << bits) - 1) * r[0] + r[1];
        }
        const double sum = q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3];
        q[drop] = std::sqrt(std::fmax(0.0, 1 - sum)) * (negative ? -1 : 1);
        return q;
    }
    throw PictureError(format("curve encoding %s is not known", fmt.c_str()));
}


Vec curve_start(const Obj &curve, size_t dim, const QuaternionRanges &ranges) {
    Vec v = curve_values(curve, dim, ranges);
    if (!v.empty() && v.size() != dim) throw PictureError(format("curve of %zu values where %zu are needed", v.size(), dim));
    return v;
}


struct Field {
    uint32_t kind = 0, count = 0;
    uint64_t offset = 0, size = 0;
    bool present = false;
};

Field field(const std::vector<Member> &layout, const char *name) {
    Field f;
    for (const Member &m : layout)
        if (m.name == name) {
            f.kind = m.kind;
            f.count = m.count > 1 ? m.count : 1;
            f.offset = m.offset;
            f.size = m.size;
            f.present = true;
        }
    return f;
}

bool float_kind(uint32_t k) { return k == kReal32 || k == kReal16; }
bool int8_kind(uint32_t k) { return k == kInt8 || k == kBinormalInt8; }
bool int16_kind(uint32_t k) { return k == kInt16 || k == kBinormalInt16; }

int64_t integer(double v, const char *what) {
    if (!(std::fabs(v) < 9.0e15)) throw PictureError(format("%s is no integer", what));
    return static_cast<int64_t>(v);
}

Obj optional_object(const Obj &o, const char *name) {
    return o.find(name) ? o.object(name) : Obj();
}

}

Mat4 identity4() { return {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}; }

Mat4 mul(const Mat4 &a, const Mat4 &b) {
    Mat4 m;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            m[4 * i + j] = a[4 * i] * b[j] + a[4 * i + 1] * b[4 + j] + a[4 * i + 2] * b[8 + j] + a[4 * i + 3] * b[12 + j];
    return m;
}

std::string lower_ascii(std::string s) {
    for (char &c : s)
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    return s;
}

Skeleton::Skeleton(const Obj &granny_skeleton) {
    if (!granny_skeleton) throw PictureError("a model without a skeleton");
    const Array bones = array(granny_skeleton, "Bones");
    if (!bones.size()) throw PictureError("skeleton without bones");
    for (uint32_t i = 0; i < bones.size(); i++) {
        const Obj b = bones.at(i);
        if (!b) throw PictureError("a null bone");
        names_.push_back(b.string("Name"));
        const int64_t p = integer(b.number("ParentIndex"), "a parent index");
        if (!(-1 <= p && p < static_cast<int64_t>(i))) throw PictureError(format("bone %s names parent %lld", names_.back().c_str(), static_cast<long long>(p)));
        parents_.push_back(static_cast<int>(p));
        const Member &t = b.member("Transform");
        if (t.kind != kTransform) throw PictureError("a bone transform of another kind");
        const uint8_t *raw = b.tree().bytes(b.at() + t.offset, 68);
        float v[17];
        std::memcpy(v, raw, 68);
        Rest r;
        for (int k = 0; k < 3; k++) r.position[k] = v[1 + k];
        for (int k = 0; k < 4; k++) r.orientation[k] = v[4 + k];
        for (int k = 0; k < 9; k++) r.scale[k] = v[8 + k];
        rest_.push_back(r);
        const std::vector<double> inv = b.numbers("InverseWorldTransform");
        if (inv.size() != 16) throw PictureError("a bone's inverse world transform of other than 16 values");
        Mat4 m;
        for (int row = 0; row < 4; row++)
            for (int col = 0; col < 4; col++) m[4 * row + col] = inv[4 * col + row];
        inverse_bind_.push_back(m);
    }
    for (size_t i = 0; i < names_.size(); i++) index_[lower_ascii(names_[i])] = static_cast<int>(i);
}

std::vector<Mat4> Skeleton::pose(const std::vector<Obj> &tracks, const QuaternionRanges &ranges) const {
    std::unordered_map<std::string, Obj> by_name;
    for (const Obj &t : tracks) by_name[lower_ascii(t.string("Name"))] = t;
    std::vector<Mat4> world;
    world.reserve(names_.size());
    for (size_t i = 0; i < names_.size(); i++) {
        std::array<double, 3> position = rest_[i].position;
        std::array<double, 4> orientation = rest_[i].orientation;
        std::array<double, 9> scale = rest_[i].scale;
        const auto it = by_name.find(lower_ascii(names_[i]));
        if (it != by_name.end()) {
            const Vec o = curve_start(it->second.object("OrientationCurve"), 4, ranges);
            const Vec p = curve_start(it->second.object("PositionCurve"), 3, ranges);
            const Vec s = curve_start(it->second.object("ScaleShearCurve"), 9, ranges);
            orientation = o.empty() ? std::array<double, 4>{0, 0, 0, 1} : std::array<double, 4>{o[0], o[1], o[2], o[3]};
            position = p.empty() ? std::array<double, 3>{0, 0, 0} : std::array<double, 3>{p[0], p[1], p[2]};
            scale = s.empty() ? std::array<double, 9>{1, 0, 0, 0, 1, 0, 0, 0, 1}
                              : std::array<double, 9>{s[0], s[1], s[2], s[3], s[4], s[5], s[6], s[7], s[8]};
        }
        const Mat4 m = local_matrix(position, orientation, scale);
        world.push_back(parents_[i] < 0 ? m : mul(world[static_cast<size_t>(parents_[i])], m));
    }
    return world;
}

std::vector<Mat4> Skeleton::rest_pose() const {
    static const QuaternionRanges unused{};
    return pose({}, unused);
}

Mesh posed_mesh(const Obj &mesh, const Skeleton &model_skeleton, const std::vector<Mat4> &pose_world, const Skeleton &pose_skeleton) {
    const std::string name = mesh.string("Name");
    auto fail = [&](const char *what) { return PictureError(format("mesh %s %s", name.c_str(), what)); };
    const Obj vertex_data = mesh.object("PrimaryVertexData");
    if (!vertex_data) throw fail("has no vertices");
    const Array v = array(vertex_data, "Vertices");
    if (!v.size() || !v.records()) throw fail("has no vertices");
    const Tree &tree = mesh.tree();
    const std::vector<Member> &layout = tree.layout(v.type());
    for (const Member &m : layout)
        if (!numeric_kind(m.kind)) throw fail("has vertices of other than numbers");
    const Obj ext = optional_object(mesh, "ExtendedData");
    const size_t n = v.size();
    const uint64_t stride = v.record_size();
    auto value = [&](const Field &f, size_t i, uint32_t k) { return tree.value(f.kind, v.data() + stride * i + f.offset + f.size * k); };

    Mesh out;
    const Field pos = field(layout, "Position");
    if (!pos.present || pos.count < 3) throw fail("has no 3D positions");
    double scale = 1;
    const bool packed = int16_kind(pos.kind);
    if (packed) {
        const double vs = ext && ext.find("VertexScale") ? ext.number("VertexScale") : 0;
        if (!(vs > 0)) throw fail("has 16-bit positions without a vertex scale");
        scale = vs / 32767.0;
    } else if (!float_kind(pos.kind)) {
        throw fail("stores positions in another kind");
    }
    const Field nrm = field(layout, "Normal"), tan = field(layout, "Tangent"), uvf = field(layout, "TextureCoordinates0");
    if (!uvf.present || !nrm.present) throw fail("lacks texture coordinates or normals");
    auto unit = [&](const Field &f, size_t i, uint32_t k) {
        const double x = value(f, i, k);
        return int8_kind(f.kind) ? x / 127.0 : x;
    };
    for (const Field *f : {&nrm, &tan})
        if (f->present && !float_kind(f->kind) && !int8_kind(f->kind)) throw fail("stores normals or tangents in another kind");
    if (nrm.count < 3 || (tan.present && tan.count < 4) || uvf.count < 2) throw fail("has too few normal, tangent or texture components");

    const Array bindings = array(mesh, "BoneBindings");
    if (!bindings.size()) throw fail("is bound to no bone");
    std::vector<Mat4> skin;
    for (uint32_t b = 0; b < bindings.size(); b++) {
        const std::string bone = lower_ascii(bindings.at(b).string("BoneName"));
        const int k = model_skeleton.find(bone), j = pose_skeleton.find(bone);
        if (k < 0) throw PictureError(format("mesh %s is bound to bone %s, which the skeleton lacks", name.c_str(), bone.c_str()));

        skin.push_back(j < 0 ? identity4() : mul(pose_world[static_cast<size_t>(j)], model_skeleton.inverse_bind(k)));
    }
    const Field bi = field(layout, "BoneIndices"), bw = field(layout, "BoneWeights");
    if (bi.present && (!bw.present || bw.count != bi.count || bi.count < 2)) throw fail("has bone indices without matching weights");

    out.positions.resize(n);
    out.normals.resize(n);
    out.uvs.resize(n);
    if (tan.present) out.tangents.resize(n);
    for (size_t i = 0; i < n; i++) {
        Mat4 m{};
        if (bi.present) {
            double total = 0;
            for (uint32_t k = 0; k < bw.count; k++) total += value(bw, i, k);
            if (!(total > 0)) throw fail("has vertices without bone weight");
            for (uint32_t k = 0; k < bi.count; k++) {
                const double idx = value(bi, i, k);
                if (!(idx >= 0 && idx < static_cast<double>(skin.size())))
                    throw PictureError(format("mesh %s names bone binding %g of %zu", name.c_str(), idx, skin.size()));
                const double w = value(bw, i, k) / total;
                const Mat4 &s = skin[static_cast<size_t>(idx)];
                for (int e = 0; e < 16; e++) m[e] += w * s[e];
            }
        } else {
            m = skin[0];
        }
        double p[3];
        for (int k = 0; k < 3; k++) p[k] = packed ? value(pos, i, static_cast<uint32_t>(k)) * scale : value(pos, i, static_cast<uint32_t>(k));
        for (int r = 0; r < 3; r++) out.positions[i][r] = m[4 * r] * p[0] + m[4 * r + 1] * p[1] + m[4 * r + 2] * p[2] + m[4 * r + 3];
        auto rotate = [&](const double a[3], double o[3]) {
            for (int r = 0; r < 3; r++) o[r] = m[4 * r] * a[0] + m[4 * r + 1] * a[1] + m[4 * r + 2] * a[2];
            const double len = std::fmax(std::sqrt(o[0] * o[0] + o[1] * o[1] + o[2] * o[2]), 1e-12);
            for (int r = 0; r < 3; r++) o[r] /= len;
        };
        const double nn[3] = {unit(nrm, i, 0), unit(nrm, i, 1), unit(nrm, i, 2)};
        double wn[3];
        rotate(nn, wn);
        out.normals[i] = {wn[0], wn[1], wn[2]};
        if (tan.present) {
            const double tt[3] = {unit(tan, i, 0), unit(tan, i, 1), unit(tan, i, 2)};
            double wt[3];
            rotate(tt, wt);
            out.tangents[i] = {wt[0], wt[1], wt[2], unit(tan, i, 3) < 0 ? -1.0 : 1.0};
        }
        out.uvs[i] = {value(uvf, i, 0), value(uvf, i, 1)};
    }

    for (size_t i = 0; i < n; i++) {
        bool finite = true;
        for (int k = 0; k < 3; k++) finite = finite && std::isfinite(out.positions[i][k]) && std::isfinite(out.normals[i][k]);
        for (int k = 0; k < 2; k++) finite = finite && std::isfinite(out.uvs[i][k]);
        if (tan.present)
            for (int k = 0; k < 3; k++) finite = finite && std::isfinite(out.tangents[i][k]);
        if (!finite) throw fail("has a vertex value that is not a finite number");
    }

    const Obj topo = mesh.object("PrimaryTopology");
    if (!topo) throw fail("has no triangles");
    std::vector<double> idx = flat_numbers(topo, "Indices16");
    if (idx.empty()) idx = flat_numbers(topo, "Indices");
    if (idx.empty() || idx.size() % 3) throw fail("has a malformed triangle list");
    out.triangles.resize(idx.size() / 3);
    for (size_t t = 0; t < idx.size(); t++) {
        if (!(idx[t] >= 0 && idx[t] < static_cast<double>(n))) throw fail("has a malformed triangle list");
        out.triangles[t / 3][t % 3] = static_cast<int32_t>(idx[t]);
    }
    const Array material_bindings = array(mesh, "MaterialBindings");
    int64_t first = 0, count = material_bindings.size();
    if (ext && ext.find("MaterialSets")) {
        const Array sets = array(ext, "MaterialSets");
        if (sets.size()) {
            first = integer(sets.at(0).number("FirstIndex"), "a material set start");
            count = integer(sets.at(0).number("Count"), "a material set size");
        }
    }
    if (first < 0 || first + count > static_cast<int64_t>(material_bindings.size())) throw fail("has a material set outside its bindings");
    out.materials.assign(out.triangles.size(), -1);
    const Array groups = array(topo, "Groups");
    for (uint32_t g = 0; g < groups.size(); g++) {
        const Obj grp = groups.at(g);
        const int64_t mi = integer(grp.number("MaterialIndex"), "a material index"), f = integer(grp.number("TriFirst"), "a first triangle"),
                      c = integer(grp.number("TriCount"), "a triangle count");
        if (f < 0 || c < 0 || f + c > static_cast<int64_t>(out.triangles.size())) throw fail("has a triangle group outside its triangles");
        if (first <= mi && mi < first + count)
            for (int64_t t = f; t < f + c; t++) out.materials[static_cast<size_t>(t)] = static_cast<int32_t>(mi);
    }
    for (int32_t m : out.materials)
        if (m < 0) throw fail("has triangles without a material of its first set");
    return out;
}

}
