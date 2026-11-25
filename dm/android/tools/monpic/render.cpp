#include "render.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "error.h"

namespace monpic {
namespace {

constexpr double kRoughness = 0.6;
constexpr double kShoulder = 0.15;
constexpr int kHueBands = 6;
constexpr double kPi = 3.141592653589793;





const double kKeyDir[3] = {-0.35, 0.85, 0.4}, kKey[3] = {2.25, 2.08, 1.9};
const double kFillDir[3] = {0.8, 0.1, 0.3}, kFill[3] = {0.25, 0.26, 0.3};
const double kSky[3] = {0.3, 0.31, 0.33}, kGround[3] = {0.12, 0.12, 0.09};
constexpr double kSpecular = 1.0, kReflection = 1.0, kOcclusion = 0.5, kExposure = 1.06, kContrast = 1.05;

struct V3 {
    double x, y, z;
};
V3 operator+(V3 a, V3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
V3 operator-(V3 a, V3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
V3 operator*(V3 a, double s) { return {a.x * s, a.y * s, a.z * s}; }
V3 operator*(V3 a, V3 b) { return {a.x * b.x, a.y * b.y, a.z * b.z}; }
double dot(V3 a, V3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
V3 cross(V3 a, V3 b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }
double norm(V3 a) { return std::sqrt(a.x * a.x + a.y * a.y + a.z * a.z); }
double clip(double v, double lo, double hi) { return std::fmin(std::fmax(v, lo), hi); }


double pymod(double a, double b) {
    double m = std::fmod(a, b);
    if (m != 0) {
        if ((b < 0) != (m < 0)) m += b;
    } else {
        m = std::copysign(0.0, b);
    }
    return m;
}

struct LinearTable {
    double v[256];
    LinearTable() {
        for (int i = 0; i < 256; i++) {
            const double c = i / 255.0;
            v[i] = c <= 0.04045 ? c / 12.92 : std::pow((c + 0.055) / 1.055, 2.4);
        }
    }
};
const LinearTable kLinear;

double linear_to_srgb(double c) {
    c = clip(c, 0, 1);
    return c <= 0.0031308 ? c * 12.92 : 1.055 * std::pow(c, 1 / 2.4) - 0.055;
}


template <int N, typename Texel>
void bilinear(uint32_t w, uint32_t h, double u, double v, const Texel &texel, double out[N]) {
    const double x = u * w - 0.5, y = v * h - 0.5;
    const double x0f = std::floor(x), y0f = std::floor(y);
    const double fx = x - x0f, fy = y - y0f;
    auto wrap = [](double i, uint32_t n) {
        const double m = pymod(i, static_cast<double>(n));
        return m >= 0 && m < n ? static_cast<uint32_t>(m) : 0u;
    };
    const uint32_t x0 = wrap(x0f, w), y0 = wrap(y0f, h), x1 = wrap(x0f + 1, w), y1 = wrap(y0f + 1, h);
    double a[N], b[N], c[N], d[N];
    texel(x0, y0, a);
    texel(x1, y0, b);
    texel(x0, y1, c);
    texel(x1, y1, d);
    for (int k = 0; k < N; k++) out[k] = (a[k] * (1 - fx) + b[k] * fx) * (1 - fy) + (c[k] * (1 - fx) + d[k] * fx) * fy;
}

const uint8_t *texel_at(const Texture &t, uint32_t x, uint32_t y) { return &t.rgba[(static_cast<size_t>(y) * t.width + x) * 4]; }

void rgb_to_hsv(const double c[3], double out[3]) {
    const double r = c[0], g = c[1], b = c[2];
    const double mx = std::max({r, g, b}), mn = std::min({r, g, b}), d = mx - mn;
    double h = 0;
    if (d > 1e-12) {
        if (mx == r) h = pymod((g - b) / d, 6);
        else if (mx == g) h = (b - r) / d + 2;
        else h = (r - g) / d + 4;
    }
    out[0] = h / 6;
    out[1] = mx > 0 ? d / std::fmax(mx, 1e-12) : 0;
    out[2] = mx;
}

void hsv_to_rgb(double hue, double s, double v, double out[3]) {
    const double h = pymod(hue, 1) * 6;
    const double fl = std::floor(h);
    const double sector = pymod(fl, 6);
    const int i = sector >= 0 && sector < 6 ? static_cast<int>(sector) : 0;
    const double f = h - fl;
    const double p = v * (1 - s), q = v * (1 - s * f), t = v * (1 - s * (1 - f));
    const double table[6][3] = {{v, t, p}, {q, v, p}, {p, v, t}, {p, q, v}, {t, p, v}, {v, p, q}};
    for (int k = 0; k < 3; k++) out[k] = table[i][k];
}

}

bool ColourTransform::identity() const {
    if (!(adjust[0] == 0 && adjust[1] == 0 && adjust[2] == 0 && adjust[3] == 1 && tint[3] == 0)) return false;
    for (int k = 0; k < 6; k++)
        if (hue[k] != 0 || sat[k] != 0) return false;
    return true;
}

void ColourTransform::apply(const double in[3], double out[3]) const {
    double hsv[3];
    rgb_to_hsv(in, hsv);
    double bands[kHueBands];
    for (int k = 0; k < kHueBands; k++) {
        const double u = pymod(hsv[0] + 1.0 / 8 - static_cast<double>(k) / kHueBands, 1) * 12;
        bands[k] = clip(u, 0, 1) * clip(3 - u, 0, 1);
    }
    double h = pymod(hsv[0] + adjust[0], 1), s = hsv[1] * (1 + adjust[1]);
    for (int k = 0; k < kHueBands; k++) {
        h = pymod(h + bands[k] * hue[k], 1);
        s = s * (1 + bands[k] * sat[k]);
    }
    double rgb[3];
    hsv_to_rgb(h, s, hsv[2] * (1 + adjust[2]), rgb);
    for (int k = 0; k < 3; k++) {
        const double c = clip(rgb[k], 0, 1);
        const double powered = c > 0 ? std::pow(c, adjust[3]) : 0.0;
        out[k] = powered * (1 + tint[3] * (tint[k] - 1));
    }
}

namespace {


class Surface {
public:
    explicit Surface(const Material &m) : m_(m) {}
    Surface(Surface &&) = default;



    void albedo_texel(uint32_t x, uint32_t y, double out[3]) const {
        const uint8_t *p = texel_at(*m_.albedo, x, y);
        const double c[3] = {kLinear.v[p[0]], kLinear.v[p[1]], kLinear.v[p[2]]};
        if (!m_.recolour) {
            for (int k = 0; k < 3; k++) out[k] = c[k];
            return;
        }
        const uint32_t tiles_x = (m_.albedo->width + kTile - 1) / kTile;
        if (tiles_.empty()) tiles_.resize(static_cast<size_t>(tiles_x) * ((m_.albedo->height + kTile - 1) / kTile));
        std::unique_ptr<Tile> &tile = tiles_[static_cast<size_t>(y / kTile) * tiles_x + x / kTile];
        if (!tile) tile = std::make_unique<Tile>();
        const size_t at = static_cast<size_t>(y % kTile) * kTile + x % kTile;
        double *memo = &tile->rgb[3 * at];
        if (!tile->done[at]) {
            recoloured(x, y, c, memo);
            tile->done[at] = 1;
        }
        for (int k = 0; k < 3; k++) out[k] = memo[k];
    }

    void recoloured(uint32_t x, uint32_t y, const double c[3], double out[3]) const {
        double w[3];
        const Texture &mask = *m_.mask;
        if (mask.width == m_.albedo->width && mask.height == m_.albedo->height) {
            const uint8_t *q = texel_at(mask, x, y);
            for (int k = 0; k < 3; k++) w[k] = q[k] / 255.0;
        } else {
            bilinear<3>(mask.width, mask.height, (x + 0.5) / m_.albedo->width, (y + 0.5) / m_.albedo->height,
                        [&](uint32_t tx, uint32_t ty, double *o) {
                            const uint8_t *q = texel_at(mask, tx, ty);
                            for (int k = 0; k < 3; k++) o[k] = q[k] / 255.0;
                        },
                        w);
        }
        const double total = w[0] + w[1] + w[2];
        if (total > 1)
            for (double &v : w) v = v / std::fmax(total, 1e-12);
        const double keep = 1 - (w[0] + w[1] + w[2]);
        for (int k = 0; k < 3; k++) out[k] = c[k] * keep;
        for (int ch = 0; ch < 3; ch++) {
            double turned[3];
            if (m_.variant[ch].identity()) {
                for (int k = 0; k < 3; k++) turned[k] = c[k];
            } else {
                m_.variant[ch].apply(c, turned);
            }
            for (int k = 0; k < 3; k++) out[k] = out[k] + w[ch] * turned[k];
        }
    }

    void albedo(double u, double v, double out[3]) const {
        bilinear<3>(m_.albedo->width, m_.albedo->height, u, v, [&](uint32_t x, uint32_t y, double *o) { albedo_texel(x, y, o); }, out);
    }

    double alpha(double u, double v) const {
        double a;
        bilinear<1>(m_.albedo->width, m_.albedo->height, u, v,
                    [&](uint32_t x, uint32_t y, double *o) { o[0] = texel_at(*m_.albedo, x, y)[3] / 255.0; }, &a);
        return a;
    }

    void normal(double u, double v, double out[2]) const {
        const Texture &t = *m_.normal;
        bilinear<2>(t.width, t.height, u, v,
                    [&](uint32_t x, uint32_t y, double *o) {
                        const uint8_t *p = texel_at(t, x, y);
                        o[0] = p[3] / 255.0 * 2 - 1;
                        o[1] = p[1] / 255.0 * 2 - 1;
                    },
                    out);
    }

    void orm(double u, double v, double out[3]) const {
        const Texture &t = *m_.orm;
        bilinear<3>(t.width, t.height, u, v,
                    [&](uint32_t x, uint32_t y, double *o) {
                        const uint8_t *p = texel_at(t, x, y);
                        for (int k = 0; k < 3; k++) o[k] = p[k] / 255.0;
                    },
                    out);
    }

    const Material &material() const { return m_; }

private:
    static constexpr uint32_t kTile = 32;
    struct Tile {
        double rgb[kTile * kTile * 3];
        uint8_t done[kTile * kTile] = {0};
    };
    const Material &m_;
    mutable std::vector<std::unique_ptr<Tile>> tiles_;
};

struct Frame {
    double cx, cy, extent;
};


struct Scene {
    std::vector<V3> pos, nrm, tan;
    std::vector<double> sign;
    std::vector<std::array<double, 2>> uv;
    std::vector<std::array<int32_t, 3>> tris;
    std::vector<int32_t> mats;
};

struct Coverage {
    int size;
    std::vector<int32_t> tri;
    std::vector<double> bary;
};

Coverage rasterize(const Scene &s, const std::vector<Surface> &surfaces, int size, int margin, const Frame &f) {
    const double scale = (size - 2 * margin) / f.extent;
    std::vector<double> xs(s.pos.size()), ys(s.pos.size());
    for (size_t i = 0; i < s.pos.size(); i++) {
        xs[i] = (s.pos[i].x - f.cx) * scale + size / 2.0;
        ys[i] = (f.cy - s.pos[i].y) * scale + size / 2.0;
    }
    Coverage c{size, std::vector<int32_t>(static_cast<size_t>(size) * size, -1), std::vector<double>(static_cast<size_t>(size) * size * 3, 0)};
    std::vector<double> best(static_cast<size_t>(size) * size, -std::numeric_limits<double>::infinity());
    for (size_t t = 0; t < s.tris.size(); t++) {
        const auto &tr = s.tris[t];
        const double ax = xs[tr[0]], ay = ys[tr[0]], bx = xs[tr[1]], by = ys[tr[1]], cx = xs[tr[2]], cy = ys[tr[2]];
        const double area = (bx - ax) * (cy - ay) - (cx - ax) * (by - ay);
        if (!(std::fabs(area) > 1e-12)) continue;
        const double sgn = area > 0 ? 1.0 : -1.0;
        const double lox = std::floor(std::min({ax, bx, cx}) - 0.5) + 1, loy = std::floor(std::min({ay, by, cy}) - 0.5) + 1;
        const double hix = std::ceil(std::max({ax, bx, cx}) - 0.5), hiy = std::ceil(std::max({ay, by, cy}) - 0.5);
        const int x0 = static_cast<int>(clip(lox, 0, size)), y0 = static_cast<int>(clip(loy, 0, size));
        const int x1 = static_cast<int>(clip(hix, -1, size - 1)), y1 = static_cast<int>(clip(hiy, -1, size - 1));
        const Surface &surface = surfaces[static_cast<size_t>(s.mats[t])];
        const bool cut = surface.material().cutout;
        const double za = s.pos[tr[0]].z, zb = s.pos[tr[1]].z, zc = s.pos[tr[2]].z;
        for (int py = y0; py <= y1; py++) {
            const double qy = py + 0.5;
            for (int px = x0; px <= x1; px++) {
                const double qx = px + 0.5;
                const double w0 = ((bx - qx) * (cy - qy) - (cx - qx) * (by - qy)) * sgn;
                const double w1 = ((cx - qx) * (ay - qy) - (ax - qx) * (cy - qy)) * sgn;
                const double w2 = ((ax - qx) * (by - qy) - (bx - qx) * (ay - qy)) * sgn;
                if (!(w0 >= 0 && w1 >= 0 && w2 >= 0)) continue;
                const double total = w0 + w1 + w2;
                const double b0 = w0 / total, b1 = w1 / total, b2 = w2 / total;
                if (cut) {
                    const auto &ua = s.uv[tr[0]], &ub = s.uv[tr[1]], &uc = s.uv[tr[2]];
                    const double u = ua[0] * b0 + ub[0] * b1 + uc[0] * b2, v = ua[1] * b0 + ub[1] * b1 + uc[1] * b2;
                    if (!(surface.alpha(u, v) >= surface.material().cutoff)) continue;
                }
                const double depth = b0 * za + b1 * zb + b2 * zc;
                const size_t pix = static_cast<size_t>(py) * size + px;
                if (depth > best[pix]) {
                    best[pix] = depth;
                    c.tri[pix] = static_cast<int32_t>(t);
                    c.bary[3 * pix] = b0;
                    c.bary[3 * pix + 1] = b1;
                    c.bary[3 * pix + 2] = b2;
                }
            }
        }
    }
    return c;
}

Frame drawn_frame(const Coverage &c, const Frame &f, int margin) {
    const int size = c.size;
    int r0 = size, r1 = -1, c0 = size, c1 = -1;
    for (int y = 0; y < size; y++)
        for (int x = 0; x < size; x++)
            if (c.tri[static_cast<size_t>(y) * size + x] >= 0) {
                r0 = std::min(r0, y);
                r1 = std::max(r1, y);
                c0 = std::min(c0, x);
                c1 = std::max(c1, x);
            }
    if (r1 < 0) throw PictureError("the figure covers no pixel");
    const double scale = (size - 2 * margin) / f.extent;
    const double x0 = (c0 - size / 2.0) / scale + f.cx, x1 = (c1 + 1 - size / 2.0) / scale + f.cx;
    const double y1 = f.cy - (r0 - size / 2.0) / scale, y0 = f.cy - (r1 + 1 - size / 2.0) / scale;
    return {(x0 + x1) / 2, (y0 + y1) / 2, std::max(x1 - x0, y1 - y0)};
}

void ggx(V3 n, V3 v, V3 l, double rough, V3 f0, double wrap, V3 *spec, double *lit) {
    V3 h = l + v;
    h = h * (1.0 / std::fmax(norm(h), 1e-9));
    const double nl = clip(dot(n, l), 0, 1);
    *lit = clip((dot(n, l) + wrap) / (1 + wrap), 0, 1);
    const double nv = clip(dot(n, v), 1e-4, 1);
    const double nh = clip(dot(n, h), 0, 1);
    const double vh = clip(dot(v, h), 0, 1);
    const double a = std::fmax(rough * rough, 2e-3);
    const double d = a * a / (kPi * std::pow(nh * nh * (a * a - 1) + 1, 2));
    const double k = std::pow(rough + 1, 2) / 8;
    const double g = nl / (nl * (1 - k) + k) * nv / (nv * (1 - k) + k);
    const double fres = std::pow(1 - vh, 5);
    const V3 fr = f0 + (V3{1, 1, 1} - f0) * fres;
    *spec = fr * (d * g / (4 * nv)) * nl;
}

V3 shade(V3 albedo, V3 normal, double ao, double rough, double metal, double wrap) {
    static const V3 key_dir = [] {
        const V3 d{kKeyDir[0], kKeyDir[1], kKeyDir[2]};
        return d * (1.0 / norm(d));
    }();
    static const V3 fill_dir = [] {
        const V3 d{kFillDir[0], kFillDir[1], kFillDir[2]};
        return d * (1.0 / norm(d));
    }();
    const V3 v{0, 0, 1};
    const V3 diffuse = albedo * (1 - metal);
    const V3 f0 = V3{0.04, 0.04, 0.04} * (1 - metal) + albedo * metal;
    const double up = (normal.y + 1) / 2;
    const V3 ambient = V3{kSky[0], kSky[1], kSky[2]} * up + V3{kGround[0], kGround[1], kGround[2]} * (1 - up);
    const double nv = clip(normal.z, 0, 1);
    const double fres = std::pow(1 - nv, 5);
    const V3 edge = f0 + (V3{std::fmax(1 - rough, f0.x), std::fmax(1 - rough, f0.y), std::fmax(1 - rough, f0.z)} - f0) * fres;
    V3 out = (diffuse * ambient + edge * ambient * kReflection) * ao;
    const double shadowed = 1 - kOcclusion * (1 - ao);
    const V3 lights[2][2] = {{key_dir, {kKey[0], kKey[1], kKey[2]}}, {fill_dir, {kFill[0], kFill[1], kFill[2]}}};
    for (const auto &l : lights) {
        V3 spec;
        double lit;
        ggx(normal, v, l[0], rough, f0, wrap, &spec, &lit);
        out = out + (diffuse * lit + spec * kSpecular) * l[1] * shadowed;
    }
    return out;
}

double tone(double c) {
    c = std::fmax(c * kExposure, 0);
    c = c / (1 + c * kShoulder);
    return 0.18 * std::pow(c / 0.18, kContrast);
}


std::vector<double> colour(const Scene &s, const std::vector<Surface> &surfaces, const Coverage &c) {
    std::vector<double> out(c.tri.size() * 3, 0.0);
    for (size_t pix = 0; pix < c.tri.size(); pix++) {
        const int32_t t = c.tri[pix];
        if (t < 0) continue;
        const auto &tr = s.tris[static_cast<size_t>(t)];
        const double b0 = c.bary[3 * pix], b1 = c.bary[3 * pix + 1], b2 = c.bary[3 * pix + 2];
        auto interp = [&](const std::vector<V3> &a) { return a[tr[0]] * b0 + a[tr[1]] * b1 + a[tr[2]] * b2; };
        V3 n = interp(s.nrm);
        n = n * (1.0 / std::fmax(norm(n), 1e-12));
        const double u = s.uv[tr[0]][0] * b0 + s.uv[tr[1]][0] * b1 + s.uv[tr[2]][0] * b2;
        const double v = s.uv[tr[0]][1] * b0 + s.uv[tr[1]][1] * b1 + s.uv[tr[2]][1] * b2;
        const double sign = s.sign[tr[0]] * b0 + s.sign[tr[1]] * b1 + s.sign[tr[2]] * b2 < 0 ? -1.0 : 1.0;
        const Surface &surface = surfaces[static_cast<size_t>(s.mats[static_cast<size_t>(t)])];
        const Material &m = surface.material();
        double alb[3];
        surface.albedo(u, v, alb);
        if (m.normal) {
            double xy[2];
            surface.normal(u, v, xy);
            const double z = std::sqrt(clip(1 - (xy[0] * xy[0] + xy[1] * xy[1]), 0, 1));
            V3 tg = interp(s.tan);
            tg = tg - n * dot(n, tg);
            tg = tg * (1.0 / std::fmax(norm(tg), 1e-12));
            const V3 bt = cross(n, tg) * sign;
            n = tg * xy[0] + bt * xy[1] + n * z;
            n = n * (1.0 / std::fmax(norm(n), 1e-12));
        }
        double ao = 1, rough = kRoughness, metal = 0;
        if (m.orm) {
            double o[3];
            surface.orm(u, v, o);
            ao = o[0];
            rough = o[1];
            metal = o[2];
        }

        if (n.z < 0) n.z = -n.z;
        const V3 lin = shade({alb[0], alb[1], alb[2]}, n, ao, rough, metal, m.wrap);
        out[3 * pix] = tone(lin.x);
        out[3 * pix + 1] = tone(lin.y);
        out[3 * pix + 2] = tone(lin.z);
    }
    return out;
}


Picture reduce(const std::vector<double> &rgb, const Coverage &c, int size, double sharpness) {
    const int full = c.size, factor = full / size;
    const size_t n = static_cast<size_t>(size) * size;
    std::vector<double> pre(n * 3), alpha(n);
    const double count = static_cast<double>(factor) * factor;
    for (int y = 0; y < size; y++)
        for (int x = 0; x < size; x++) {
            double sum[4] = {0, 0, 0, 0};
            for (int dy = 0; dy < factor; dy++)
                for (int dx = 0; dx < factor; dx++) {
                    const size_t p = static_cast<size_t>(y * factor + dy) * full + x * factor + dx;
                    for (int k = 0; k < 3; k++) sum[k] += rgb[3 * p + k];
                    sum[3] += c.tri[p] >= 0 ? 1.0 : 0.0;
                }
            const size_t q = static_cast<size_t>(y) * size + x;
            const double a = sum[3] / count;
            alpha[q] = a;
            for (int k = 0; k < 3; k++) {
                const double mean = sum[k] / count;
                const double straight = a > 0 ? mean / std::fmax(a, 1e-12) : 0;
                pre[3 * q + k] = linear_to_srgb(straight) * a;
            }
        }


    const double k3[3] = {0.25, 0.5, 0.25};
    std::vector<double> vert(n * 3), blur(n * 3);
    auto at = [&](const std::vector<double> &img, int x, int y, int ch) {
        x = std::min(std::max(x, 0), size - 1);
        y = std::min(std::max(y, 0), size - 1);
        return img[3 * (static_cast<size_t>(y) * size + x) + ch];
    };
    for (int y = 0; y < size; y++)
        for (int x = 0; x < size; x++)
            for (int ch = 0; ch < 3; ch++)
                vert[3 * (static_cast<size_t>(y) * size + x) + ch] =
                    0 + k3[0] * at(pre, x, y - 1, ch) + k3[1] * at(pre, x, y, ch) + k3[2] * at(pre, x, y + 1, ch);
    for (int y = 0; y < size; y++)
        for (int x = 0; x < size; x++)
            for (int ch = 0; ch < 3; ch++)
                blur[3 * (static_cast<size_t>(y) * size + x) + ch] =
                    0 + k3[0] * at(vert, x - 2, y, ch) + k3[1] * at(vert, x - 1, y, ch) + k3[2] * at(vert, x, y, ch);
    Picture p;
    p.size = size;
    p.rgba.resize(n * 4);
    for (size_t q = 0; q < n; q++) {
        const double a = alpha[q];
        for (int ch = 0; ch < 3; ch++) {
            const double v = pre[3 * q + ch];
            const double sharp = clip(v + sharpness * (v - blur[3 * q + ch]), 0, a);
            const double straight = a > 0 ? sharp / std::fmax(a, 1e-12) : 0;
            p.rgba[4 * q + ch] = static_cast<uint8_t>(std::nearbyint(straight * 255));
        }
        p.rgba[4 * q + 3] = static_cast<uint8_t>(std::nearbyint(a * 255));
    }
    return p;
}

}

std::vector<Picture> render_pictures(const std::vector<DrawnMesh> &meshes, const std::vector<Material> &table, const Camera &camera,
                                     const std::vector<int> &sizes, int supersample, double sharpness) {
    const double cyaw = std::cos(camera.yaw), syaw = std::sin(camera.yaw), ce = std::cos(camera.elevation), se = std::sin(camera.elevation);
    const V3 back{syaw * ce, se, cyaw * ce}, right{cyaw, 0.0, -syaw};
    const V3 up = cross(back, right);
    auto to_view = [&](V3 p) { return V3{dot(p, right), dot(p, up), dot(p, back)}; };

    Scene s;
    int32_t base = 0;
    for (const DrawnMesh &dm : meshes) {
        const Mesh &m = dm.mesh;
        for (size_t i = 0; i < m.positions.size(); i++) {
            s.pos.push_back(to_view({m.positions[i][0], m.positions[i][1], m.positions[i][2]}));
            s.nrm.push_back(to_view({m.normals[i][0], m.normals[i][1], m.normals[i][2]}));
            if (m.tangents.empty()) {
                s.tan.push_back({0, 0, 0});
                s.sign.push_back(1);
            } else {
                s.tan.push_back(to_view({m.tangents[i][0], m.tangents[i][1], m.tangents[i][2]}));
                s.sign.push_back(m.tangents[i][3]);
            }
            s.uv.push_back(m.uvs[i]);
        }
        for (size_t t = 0; t < m.triangles.size(); t++) {
            s.tris.push_back({m.triangles[t][0] + base, m.triangles[t][1] + base, m.triangles[t][2] + base});
            s.mats.push_back(dm.material_of_binding[static_cast<size_t>(m.materials[t])]);
        }
        base += static_cast<int32_t>(m.positions.size());
    }
    if (s.pos.empty()) throw PictureError("the figure has no vertices");
    double lo[2] = {s.pos[0].x, s.pos[0].y}, hi[2] = {s.pos[0].x, s.pos[0].y};
    for (const V3 &p : s.pos) {
        if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z)) throw PictureError("the figure has a vertex outside space");
        lo[0] = std::min(lo[0], p.x);
        lo[1] = std::min(lo[1], p.y);
        hi[0] = std::max(hi[0], p.x);
        hi[1] = std::max(hi[1], p.y);
    }
    Frame frame{(lo[0] + hi[0]) / 2, (lo[1] + hi[1]) / 2, std::max(hi[0] - lo[0], hi[1] - lo[1])};
    if (!(frame.extent > 0)) throw PictureError("the figure has no extent");

    std::vector<Surface> surfaces;
    surfaces.reserve(table.size());
    for (const Material &m : table) surfaces.emplace_back(m);
    const int full = sizes[0] * supersample;
    Coverage cov = rasterize(s, surfaces, full, supersample, frame);
    const Frame fitted = drawn_frame(cov, frame, supersample);
    if (fitted.extent < frame.extent * 0.98) cov = rasterize(s, surfaces, full, supersample, fitted);
    const std::vector<double> rgb = colour(s, surfaces, cov);
    std::vector<Picture> out;
    for (int size : sizes) {
        if (full % size) throw PictureError(format("picture size %d does not divide the rendering of %d", size, full));
        out.push_back(reduce(rgb, cov, size, sharpness));
    }
    return out;
}

}
