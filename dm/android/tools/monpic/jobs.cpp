
#include <atomic>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <system_error>
#include <sstream>
#include <thread>

#include "archive.h"
#include "bitknit.h"
#include "error.h"
#include "granny.h"
#include "model.h"
#include "monpic.h"
#include "render.h"
#include "texture.h"
#include "webp_writer.h"

namespace monpic {
namespace {

const std::vector<int> kSizes = {256, 96};
constexpr int kSupersample = 3;
constexpr double kSharpness = 0.35;
constexpr double kDegree = 3.141592653589793 / 180.0;
const double kElevation = 35 * kDegree;
const double kYaw = -20 * kDegree;
constexpr double kSkinWrap = 0.5;
constexpr double kHairCutoff = 0.5;
constexpr size_t kMaxJobLine = 1 << 16;


struct Step {
    std::string kind;
    std::vector<std::string> fields;
};

struct Job {
    std::string name;
    std::vector<Step> steps;
};

struct JobFile {
    QuaternionRanges ranges{};
    std::vector<Job> jobs;
};

double number(const std::string &s, const char *what) {
    errno = 0;
    char *end = nullptr;
    const double v = std::strtod(s.c_str(), &end);
    if (s.empty() || *end || errno == ERANGE || !std::isfinite(v)) throw PictureError(format("job file: %s %s is not a finite number", what, s.c_str()));
    return v;
}

bool valid_name(const std::string &n) {
    if (n.empty() || n.size() > 40) return false;
    for (char c : n)
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_')) return false;
    return true;
}


const std::map<std::string, int> kStepFields = {{"skeleton", 1}, {"animation", 2}, {"colour", 60}, {"pose", 0}, {"turn", 1},
                                                {"model", 3},    {"attach", 10},   {"item", 3},    {"fail", 2}};

JobFile parse_jobs(const std::string &path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw PictureError(format("cannot open the job file %s", path.c_str()));
    JobFile f;
    std::string line;
    size_t line_no = 0;
    bool open = false, ranges = false;
    std::set<std::string> names;
    while (std::getline(in, line)) {
        line_no++;
        if (line.size() > kMaxJobLine) throw PictureError(format("job file line %zu is too long", line_no));
        std::vector<std::string> fields;
        std::stringstream ss(line);
        std::string field;
        while (std::getline(ss, field, '\t')) fields.push_back(field);
        if (fields.empty()) throw PictureError(format("job file line %zu is empty", line_no));
        const std::string kind = fields[0];
        fields.erase(fields.begin());
        if (line_no == 1) {
            if (line != "monpic 1") throw PictureError("not a monpic 1 job file");
            continue;
        }
        if (kind == "ranges") {
            if (fields.size() != 32 || ranges || open) throw PictureError(format("job file line %zu: misplaced or malformed ranges", line_no));
            for (int i = 0; i < 16; i++) f.ranges[i] = {number(fields[2 * i], "a range"), number(fields[2 * i + 1], "a range")};
            ranges = true;
        } else if (kind == "picture") {
            if (open || fields.size() != 1 || !valid_name(fields[0]) || !names.insert(fields[0]).second)
                throw PictureError(format("job file line %zu: misplaced picture or bad name", line_no));
            f.jobs.push_back(Job{fields[0], {}});
            open = true;
        } else if (kind == "end") {
            if (!open || !fields.empty()) throw PictureError(format("job file line %zu: misplaced end", line_no));
            open = false;
        } else {
            const auto it = kStepFields.find(kind);
            if (it == kStepFields.end() || !open) throw PictureError(format("job file line %zu: unknown or misplaced record %s", line_no, kind.c_str()));
            if (static_cast<int>(fields.size()) != it->second && !(kind == "fail" && fields.size() == 1))
                throw PictureError(format("job file line %zu: %s with %zu fields", line_no, kind.c_str(), fields.size()));
            f.jobs.back().steps.push_back(Step{kind, fields});
        }
    }
    if (line_no == 0 || open || !ranges) throw PictureError("job file incomplete");
    return f;
}


template <typename T>
class Cache {
public:
    using Load = std::function<std::shared_ptr<const T>(const std::string &)>;
    explicit Cache(Load load) : load_(std::move(load)) {}

    std::shared_ptr<const T> get(const std::string &key) {
        std::shared_ptr<Slot> slot;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto &s = slots_[key];
            if (!s) s = std::make_shared<Slot>();
            slot = s;
        }
        std::call_once(slot->once, [&] {
            try {
                slot->value = load_(key);
            } catch (const Unsupported &e) {
                slot->unsupported = true;
                slot->error = e.what();
            } catch (const std::exception &e) {
                slot->error = e.what();
            }
        });
        if (slot->unsupported) throw Unsupported(slot->error);
        if (!slot->value) throw PictureError(slot->error);
        return slot->value;
    }

private:
    struct Slot {
        std::once_flag once;
        std::shared_ptr<const T> value;
        bool unsupported = false;
        std::string error;
    };
    Load load_;
    std::mutex mutex_;
    std::map<std::string, std::shared_ptr<Slot>> slots_;
};

struct Run {
    explicit Run(const std::string &romfs)
        : archive(romfs),
          trees([this](const std::string &p) { return std::shared_ptr<const Tree>(read_granny(archive.read(p), p)); }),
          textures([this](const std::string &p) { return std::make_shared<const Texture>(decode_texture(archive.read(p), p)); }) {}
    Archive archive;
    QuaternionRanges ranges{};
    Cache<Tree> trees;
    Cache<Texture> textures;
};


class Figure {
public:
    Figure(Run &run, const std::string &name) : run_(run), name_(name) {}

    void step(const Step &s) {
        const auto &f = s.fields;
        if (s.kind == "skeleton") {
            const Tree &t = *keep(run_.trees.get(f[0]));
            const Array skeletons = array(t.root(), "Skeletons");
            if (!skeletons.size()) throw PictureError(format("%s holds no skeleton", f[0].c_str()));
            skeleton_ = std::make_unique<Skeleton>(skeletons.at(0));
        } else if (s.kind == "animation") {
            const Tree &t = *keep(run_.trees.get(f[0]));
            const Array clips = array(t.root(), "Animations");
            const std::string want = lower_ascii(f[1]);
            for (uint32_t i = 0; i < clips.size() && !clip_; i++) {
                const Obj c = clips.at(i);
                if (c && lower_ascii(c.string("Name")) == want) clip_ = c;
            }
            if (!clip_) throw Unsupported(format("animation %s is not in its combined animations", f[1].c_str()));
        } else if (s.kind == "colour") {
            for (int k = 0; k < 3; k++) {
                ColourTransform &t = variant_[k];
                const std::string *v = &f[20 * k];
                for (int i = 0; i < 4; i++) t.adjust[i] = number(v[i], "a colour value");
                for (int i = 0; i < 4; i++) t.tint[i] = number(v[4 + i], "a colour value");
                for (int i = 0; i < 6; i++) t.hue[i] = number(v[8 + i], "a colour value");
                for (int i = 0; i < 6; i++) t.sat[i] = number(v[14 + i], "a colour value");
            }
            recolour_ = !(variant_[0].identity() && variant_[1].identity() && variant_[2].identity());
        } else if (s.kind == "pose") {
            if (!skeleton_ || !clip_) throw PictureError("job file: pose before skeleton and animation");
            std::vector<Obj> tracks;
            const Array groups = array(clip_, "TrackGroups");
            for (uint32_t g = 0; g < groups.size(); g++) {
                const Obj grp = groups.at(g);
                if (!grp) throw PictureError("a null track group");
                const Array tt = array(grp, "TransformTracks");
                for (uint32_t i = 0; i < tt.size(); i++) {
                    const Obj t = tt.at(i);
                    if (!t) throw PictureError("a null transform track");
                    tracks.push_back(t);
                }
            }
            world_ = skeleton_->pose(tracks, run_.ranges);
        } else if (s.kind == "turn") {
            front_ = number(f[0], "a turn");
            turned_ = true;
        } else if (s.kind == "model") {
            if (world_.empty()) throw PictureError("job file: model before pose");
            add(f[2], f[1], layers(f[0]), world_, *skeleton_, true);
        } else if (s.kind == "attach") {
            if (world_.empty()) throw PictureError("job file: attach before pose");
            const int bone = skeleton_->find(f[0]);
            if (bone < 0) throw Unsupported(format("attachment on bone '%s', which the skeleton lacks", f[0].c_str()));
            double v[9];
            for (int i = 0; i < 9; i++) v[i] = number(f[1 + i], "an attachment value");
            place_ = mul(world_[static_cast<size_t>(bone)], attachment_matrix(v));
            attached_ = true;
        } else if (s.kind == "item") {
            if (!attached_) throw PictureError("job file: item without attach");
            const Tree &t = *keep(run_.trees.get(f[2]));
            const Array models = array(t.root(), "Models");
            for (uint32_t i = 0; i < models.size(); i++) {
                const Obj model = models.at(i);
                if (!model) throw PictureError("a null model");
                const Skeleton own(model.object("Skeleton"));
                std::vector<Mat4> world = own.rest_pose();
                for (Mat4 &w : world) w = mul(place_, w);
                meshes_of(model, f[1], layers(f[0]), world, own, own, false);
            }
        } else if (s.kind == "fail") {
            const std::string why = f.size() > 1 ? f[1] : std::string();
            if (f[0] == "unsupported") throw Unsupported(why);
            throw PictureError(why);
        }
    }

    std::vector<Picture> render() const {
        if (meshes_.empty()) throw Unsupported("no visible mesh");
        if (!turned_) throw PictureError("job file: picture without turn");
        return render_pictures(meshes_, table_, Camera{front_ + kYaw, kElevation}, kSizes, kSupersample, kSharpness);
    }

private:
    template <typename T>
    std::shared_ptr<const T> keep(std::shared_ptr<const T> p) {
        held_.push_back(p);
        return p;
    }

    static int64_t layers(const std::string &s) {
        const double v = number(s, "a layer mask");
        if (v != std::floor(v) || std::fabs(v) > 9.0e15) throw PictureError("job file: layer mask is no integer");
        return static_cast<int64_t>(v);
    }

    static Mat4 attachment_matrix(const double v[9]) {
        const double rx = v[0] * kDegree, ry = v[1] * kDegree, rz = v[2] * kDegree;
        const double cx = std::cos(rx), sx = std::sin(rx), cy = std::cos(ry), sy = std::sin(ry), cz = std::cos(rz), sz = std::sin(rz);
        const Mat4 z{cz, -sz, 0, 0, sz, cz, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
        const Mat4 y{cy, 0, sy, 0, 0, 1, 0, 0, -sy, 0, cy, 0, 0, 0, 0, 1};
        const Mat4 x{1, 0, 0, 0, 0, cx, -sx, 0, 0, sx, cx, 0, 0, 0, 0, 1};
        const Mat4 s{v[3], 0, 0, 0, 0, v[4], 0, 0, 0, 0, v[5], 0, 0, 0, 0, 1};
        Mat4 m = mul(mul(mul(z, y), x), s);
        m[3] = v[6];
        m[7] = v[7];
        m[11] = v[8];
        return m;
    }

    void add(const std::string &path, const std::string &key, int64_t visible, const std::vector<Mat4> &world, const Skeleton &pose,
             bool colours) {
        const Tree &t = *keep(run_.trees.get(path));
        const Array models = array(t.root(), "Models");
        for (uint32_t i = 0; i < models.size(); i++) {
            const Obj model = models.at(i);
            if (!model) throw PictureError("a null model");
            const Skeleton own(model.object("Skeleton"));
            meshes_of(model, key, visible, world, own, pose, colours);
        }
    }

    void meshes_of(const Obj &model, const std::string &key, int64_t visible, const std::vector<Mat4> &world, const Skeleton &own,
                   const Skeleton &pose, bool colours) {
        const Array bindings = array(model, "MeshBindings");
        for (uint32_t b = 0; b < bindings.size(); b++) {
            const Obj binding = bindings.at(b);
            if (!binding) throw PictureError("a null mesh binding");
            const Obj mesh = binding.object("Mesh");
            if (!mesh) throw PictureError("a mesh binding without a mesh");
            int64_t layer = 1;
            if (mesh.find("ExtendedData")) {
                const Obj ext = mesh.object("ExtendedData");
                if (ext && ext.find("meshRenderLayer")) {
                    const double v = ext.number("meshRenderLayer");
                    if (!(std::fabs(v) < 9.0e15)) throw PictureError("a render layer that is no integer");
                    layer = static_cast<int64_t>(v);
                }
            }
            if (!(layer & visible)) continue;
            DrawnMesh dm{posed_mesh(mesh, own, world, pose), {}};
            const Array mats = array(mesh, "MaterialBindings");
            dm.material_of_binding.assign(mats.size(), -1);
            std::set<int32_t> used(dm.mesh.materials.begin(), dm.mesh.materials.end());
            for (int32_t i : used) {
                const Obj mb = mats.at(static_cast<uint32_t>(i));
                const Obj m = mb ? mb.object("Material") : Obj();
                if (!m) throw PictureError(format("%s: mesh %s has triangles without a material", name_.c_str(), mesh.string("Name").c_str()));
                const std::string mkey = key + '\n' + m.string("Name");
                auto it = keys_.find(mkey);
                if (it == keys_.end()) {
                    table_.push_back(material(m, colours));
                    it = keys_.emplace(mkey, static_cast<int32_t>(table_.size() - 1)).first;
                }
                dm.material_of_binding[static_cast<size_t>(i)] = it->second;
            }
            meshes_.push_back(std::move(dm));
        }
    }


    Material material(const Obj &m, bool colours) {
        const std::string mname = m.string("Name");
        std::map<std::string, std::string> maps;
        const Array list = array(m, "Maps");
        for (uint32_t i = 0; i < list.size(); i++) {
            const Obj entry = list.at(i);
            if (!entry) throw PictureError("a null material map");
            const Obj map = entry.object("Map");
            const Obj tex = map ? map.object("Texture") : Obj();
            if (!tex) continue;
            maps[entry.string("Usage")] = tex.string("FromFileName");
        }
        const Obj ext = m.find("ExtendedData") ? m.object("ExtendedData") : Obj();
        if (!maps.count("AlbedoTexture")) throw PictureError(format("material %s has no albedo texture", mname.c_str()));
        Material out;
        out.albedo = run_.textures.get(maps["AlbedoTexture"]);
        std::string technique;
        const bool hair = ext && ext.find("technique") && ext.member("technique").kind == kString && ext.has_string("technique", &technique) &&
                          technique == "StandardHair";
        if (hair) {
            out.cutout = true;
            out.cutoff = kHairCutoff;
        } else if (ext && ext.find("PrimaryAlphaThresholdEnabled") && truthy(ext, "PrimaryAlphaThresholdEnabled") &&
                   ext.find("PrimaryAlphaThreshold")) {
            out.cutout = true;
            out.cutoff = ext.number("PrimaryAlphaThreshold");
        }
        if (maps.count("NormalTexture")) out.normal = run_.textures.get(maps["NormalTexture"]);
        if (maps.count("ORMTexture")) out.orm = run_.textures.get(maps["ORMTexture"]);
        if (maps.count("MaskTexture")) {
            out.mask = run_.textures.get(maps["MaskTexture"]);
            out.recolour = colours && recolour_;
            out.variant = variant_;
        }
        out.wrap = maps.count("ThicknessTexture") ? kSkinWrap : 0.0;
        return out;
    }

    static bool truthy(const Obj &o, const char *name) {
        const Member &m = o.member(name);
        if (numeric_kind(m.kind)) return o.number(name) != 0;
        if (m.kind == kString) {
            std::string s;
            return o.has_string(name, &s) && !s.empty();
        }
        throw PictureError(format("material value %s of an unexpected kind", name));
    }

    Run &run_;
    std::string name_;
    std::vector<std::shared_ptr<const void>> held_;
    std::unique_ptr<Skeleton> skeleton_;
    Obj clip_;
    Variant variant_;
    bool recolour_ = false, turned_ = false, attached_ = false;
    std::vector<Mat4> world_;
    Mat4 place_{};
    double front_ = 0;
    std::vector<DrawnMesh> meshes_;
    std::vector<Material> table_;
    std::map<std::string, int32_t> keys_;
};

void write_file(const std::string &path, const std::vector<uint8_t> &data) {
    std::FILE *f = std::fopen(path.c_str(), "wb");
    if (!f) throw PictureError(format("cannot write %s: %s", path.c_str(), std::strerror(errno)));
    const bool ok = std::fwrite(data.data(), 1, data.size(), f) == data.size();
    if (std::fclose(f) != 0 || !ok) throw PictureError(format("cannot write %s", path.c_str()));
}

struct Outcome {
    enum Kind { pending, ok, unsupported, error } kind = pending;
    std::string message;
};

void render_one(Run &run, const Job &job, const std::string &out) {
    Figure fig(run, job.name);
    for (const Step &s : job.steps) fig.step(s);
    const std::vector<Picture> pics = fig.render();
    write_file(out + "/mon/" + job.name + ".webp", encode_webp(pics[0], false));
    write_file(out + "/mon/s/" + job.name + ".webp", encode_webp(pics[1], true));
}

void render_all(const std::string &romfs, const std::string &jobs_path, const std::string &out, int threads, const std::string &results) {
    const JobFile jobs = parse_jobs(jobs_path);
    Run run(romfs);
    run.ranges = jobs.ranges;
    std::vector<Outcome> outcomes(jobs.jobs.size());
    std::atomic<size_t> next{0};
    std::atomic<bool> stop{false};
    auto worker = [&] {
        for (;;) {
            if (stop) return;
            const size_t i = next++;
            if (i >= jobs.jobs.size()) return;
            Outcome &o = outcomes[i];
            try {
                render_one(run, jobs.jobs[i], out);
                o.kind = Outcome::ok;
            } catch (const Unsupported &e) {
                o.kind = Outcome::unsupported;
                o.message = e.what();
            } catch (const std::exception &e) {
                o.kind = Outcome::error;
                o.message = e.what();
                stop = true;
            }
        }
    };
    const int n = threads < 1 ? 1 : threads;
    std::vector<std::thread> pool;
    try {
        for (int t = 1; t < n; t++) pool.emplace_back(worker);
    } catch (const std::system_error &e) {
        stop = true;
        for (std::thread &t : pool) t.join();
        throw PictureError(format("cannot start %d worker threads: %s", n, e.what()));
    }
    worker();
    for (std::thread &t : pool) t.join();
    std::map<std::string, const Outcome *> sorted;
    for (size_t i = 0; i < jobs.jobs.size(); i++) sorted[jobs.jobs[i].name] = &outcomes[i];
    std::ofstream r(results, std::ios::binary | std::ios::trunc);
    if (!r) throw PictureError(format("cannot write %s", results.c_str()));
    for (const auto &kv : sorted) {
        const Outcome &o = *kv.second;
        if (o.kind == Outcome::pending) continue;
        std::string msg = o.message;
        for (char &c : msg)
            if (c == '\n' || c == '\t' || c == '\r') c = ' ';
        r << kv.first << (o.kind == Outcome::ok ? "\tok" : o.kind == Outcome::unsupported ? "\tunsupported\t" : "\terror\t") << msg << '\n';
    }
    r.close();
    if (!r) throw PictureError(format("cannot write %s", results.c_str()));
}

}
}

extern "C" MONPIC_API int monpic_bitknit_decode(const uint8_t *src, size_t src_len, uint8_t *dst, size_t dst_len) {
    return static_cast<int>(monpic::bitknit_decode(src, src_len, dst, dst_len));
}

extern "C" MONPIC_API int monpic_render(const char *romfs, const char *jobs, const char *out, int threads, const char *results, char *err,
                                        size_t err_len) {
    try {
        monpic::render_all(romfs, jobs, out, threads, results);
        return 0;
    } catch (const std::exception &e) {
        if (err && err_len) std::snprintf(err, err_len, "%s", e.what());
        return 1;
    }
}
