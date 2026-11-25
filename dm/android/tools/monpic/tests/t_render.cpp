


#include <cmath>
#include <cstring>

#include "error.h"
#include "render.h"
#include "support.h"

using namespace monpic;

namespace {

ColourTransform transform(double x = 0, double y = 0, double z = 0, double w = 1) {
    ColourTransform t;
    t.adjust[0] = x;
    t.adjust[1] = y;
    t.adjust[2] = z;
    t.adjust[3] = w;
    return t;
}

bool near3(const double a[3], double r, double g, double b) {
    return std::fabs(a[0] - r) < 1e-9 && std::fabs(a[1] - g) < 1e-9 && std::fabs(a[2] - b) < 1e-9;
}

std::shared_ptr<const Texture> solid(uint8_t r, uint8_t g, uint8_t b, bool left_half_clear = false) {
    auto t = std::make_shared<Texture>();
    t->width = t->height = 4;
    for (int y = 0; y < 4; y++)
        for (int x = 0; x < 4; x++) t->rgba.insert(t->rgba.end(), {r, g, b, static_cast<uint8_t>(left_half_clear && x < 2 ? 0 : 255)});
    return t;
}


DrawnMesh square(double z, double wide, int32_t material) {
    DrawnMesh d;
    Mesh &m = d.mesh;
    m.positions = {{-0.5 * wide, 0, z}, {0.5 * wide, 0, z}, {0.5 * wide, 1, z}, {-0.5 * wide, 1, z}};
    m.normals.assign(4, {0, 0, 1});
    m.tangents.assign(4, {1, 0, 0, 1});
    m.uvs = {{0, 1}, {1, 1}, {1, 0}, {0, 0}};
    m.triangles = {{0, 1, 2}, {0, 2, 3}};
    m.materials = {0, 0};
    d.material_of_binding = {material};
    return d;
}

const uint8_t *px(const Picture &p, int x, int y) { return &p.rgba[(static_cast<size_t>(y) * p.size + x) * 4]; }

}

int main() {
    double out[3];
    const double red[3] = {0.8, 0.2, 0.2}, blue[3] = {0.1, 0.2, 0.9}, grey[3] = {0.25, 0.25, 0.25};
    CHECK(transform().identity() && !transform(0, 0, 0, 0.9).identity(), "the empty transform is the identity");
    transform(0.5).apply(red, out);
    CHECK(near3(out, 0.2, 0.8, 0.8), "hue turned half a turn: red to cyan");
    transform(0, -1).apply(red, out);
    CHECK(near3(out, 0.8, 0.8, 0.8), "saturation scaled by 1 + y");
    transform(0, 0, -0.5).apply(red, out);
    CHECK(near3(out, 0.4, 0.1, 0.1), "value scaled by 1 + z");
    transform(0, 0, 1).apply(red, out);
    CHECK(near3(out, 1, 0.4, 0.4), "value beyond 1 clamped after the conversion back");
    transform(0, 0, 0, 0.5).apply(grey, out);
    CHECK(near3(out, 0.5, 0.5, 0.5), "w raises the result to its power");
    ColourTransform tint;
    tint.tint[0] = 1;
    tint.tint[1] = 0;
    tint.tint[2] = 0.5;
    tint.tint[3] = 0.5;
    tint.apply(grey, out);
    CHECK(near3(out, 0.25, 0.125, 0.1875), "the tint multiplies, weighted by its w");
    ColourTransform to_green;
    to_green.hue[0] = 1.0 / 3;
    to_green.apply(red, out);
    CHECK(near3(out, 0.2, 0.8, 0.2), "a band's hue shift turns its hues");
    to_green.apply(blue, out);
    CHECK(near3(out, 0.1, 0.2, 0.9), "and leaves the others");
    ColourTransform grey_blue;
    grey_blue.sat[4] = -1;
    grey_blue.apply(red, out);
    CHECK(near3(out, 0.8, 0.2, 0.2), "a band's saturation scales only its hues");

    const Camera front{0, 0};
    std::vector<Material> table(2);
    table[0].albedo = solid(255, 0, 0, true);
    table[0].cutout = true;
    table[0].cutoff = 0.5;
    table[1].albedo = solid(0, 0, 255);
    const std::vector<DrawnMesh> both = {square(0.5, 1, 0), square(0, 2, 1)};
    const std::vector<Picture> pics = render_pictures(both, table, front, {32, 8}, 2, 0.0);
    CHECK(pics.size() == 2 && pics[0].size == 32 && pics[1].size == 8 && pics[0].rgba.size() == 32 * 32 * 4, "both sizes");
    int c0 = 32, c1 = -1;
    for (int y = 0; y < 32; y++)
        for (int x = 0; x < 32; x++)
            if (px(pics[0], x, y)[3]) c0 = std::min(c0, x), c1 = std::max(c1, x);
    CHECK(c0 <= 2 && c1 >= 29, "the figure fills the width: %d..%d", c0, c1);
    const int row = 16;
    bool near_red = true, cut_blue = true;
    for (int x = 20; x < 23; x++) near_red = near_red && px(pics[0], x, row)[0] > 150 && px(pics[0], x, row)[2] < 60;
    for (int x = 9; x < 12; x++) cut_blue = cut_blue && px(pics[0], x, row)[2] > 150 && px(pics[0], x, row)[0] < 60;
    CHECK(near_red && cut_blue, "near red in front, its cut-out half shows the far blue");
    CHECK(render_pictures(both, table, front, {32, 8}, 2, 0.0)[0].rgba == pics[0].rgba, "the same input gives the same bytes");

    const std::vector<Picture> alone = render_pictures({square(0.5, 1, 0)}, table, front, {32, 8}, 2, 0.0);
    int x0 = 32, x1 = -1, y0 = 32, y1 = -1;
    for (int y = 0; y < 32; y++)
        for (int x = 0; x < 32; x++)
            if (px(alone[0], x, y)[3]) x0 = std::min(x0, x), x1 = std::max(x1, x), y0 = std::min(y0, y), y1 = std::max(y1, y);
    CHECK(std::max(x1 - x0, y1 - y0) >= 29, "a cut-out edge does not shrink the figure: %d..%d x %d..%d", x0, x1, y0, y1);
    CHECK(test::throws<PictureError>([&] { render_pictures(both, table, front, {32, 7}, 2, 0.0); }), "a size that does not divide the rendering");
    CHECK(test::throws<PictureError>([&] { render_pictures({}, table, front, {32, 8}, 2, 0.0); }), "a figure without meshes");
    DrawnMesh flat = square(0, 1, 1);
    for (auto &p : flat.mesh.positions) p = {0, 0, 0};
    CHECK(test::throws<PictureError>([&] { render_pictures({flat}, table, front, {32, 8}, 2, 0.0); }), "a figure without extent");


    std::vector<Material> masked(1);
    masked[0].albedo = solid(200, 30, 30);
    masked[0].recolour = true;
    masked[0].variant = {transform(0.5), transform(), transform()};
    masked[0].mask = solid(0, 0, 0);
    std::vector<Material> plain(1);
    plain[0].albedo = masked[0].albedo;
    const std::vector<DrawnMesh> one = {square(0, 1, 0)};
    CHECK(render_pictures(one, masked, front, {32, 8}, 2, 0.35)[1].rgba == render_pictures(one, plain, front, {32, 8}, 2, 0.35)[1].rgba,
          "an empty mask leaves the albedo");
    masked[0].mask = solid(255, 0, 0);
    const Picture cyan = render_pictures(one, masked, front, {32, 8}, 2, 0.35)[1];
    CHECK(px(cyan, 4, 4)[1] > px(cyan, 4, 4)[0] && px(cyan, 4, 4)[2] > px(cyan, 4, 4)[0], "a full mask channel applies its transform");


    const std::string got = test::md5_hex(pics[0].rgba) + " " + test::md5_hex(pics[1].rgba);
    std::printf("t_render: expected image %s\n", got.c_str());
    CHECK(got == "c140971f1453d706869851addb226b39 3a6067c6e80c3d11c7d86935ee05e975", "the expected image");
    std::printf("t_render: %d checks passed\n", test::checks);
    return 0;
}
