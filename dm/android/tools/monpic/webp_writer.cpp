#include "webp_writer.h"

#include "error.h"
#include "webp/encode.h"

namespace monpic {
namespace {

constexpr float kWebpQuality = 90;

}

std::vector<uint8_t> encode_webp(const Picture &p, bool lossless) {
    WebPConfig config;
    if (!WebPConfigInit(&config)) throw PictureError("WebP: configuration not initialised");
    config.lossless = lossless ? 1 : 0;
    config.quality = lossless ? 100 : kWebpQuality;
    config.alpha_quality = 100;
    config.method = 6;
    config.exact = 0;
    if (!WebPValidateConfig(&config)) throw PictureError("WebP: invalid configuration");
    WebPPicture pic;
    if (!WebPPictureInit(&pic)) throw PictureError("WebP: picture not initialised");
    pic.width = p.size;
    pic.height = p.size;
    pic.use_argb = 1;
    if (!WebPPictureAlloc(&pic)) throw PictureError("WebP: out of memory");
    for (int y = 0; y < p.size; y++)
        for (int x = 0; x < p.size; x++) {
            const uint8_t *s = &p.rgba[(static_cast<size_t>(y) * p.size + x) * 4];
            pic.argb[y * pic.argb_stride + x] = static_cast<uint32_t>(s[3]) << 24 | static_cast<uint32_t>(s[0]) << 16 | s[1] << 8 | s[2];
        }
    WebPMemoryWriter writer;
    WebPMemoryWriterInit(&writer);
    pic.writer = WebPMemoryWrite;
    pic.custom_ptr = &writer;
    const int ok = WebPEncode(&config, &pic);
    const int code = pic.error_code;
    WebPPictureFree(&pic);
    std::vector<uint8_t> out(writer.mem, writer.mem + writer.size);
    WebPMemoryWriterClear(&writer);
    if (!ok) throw PictureError(format("WebP: encoding error %d", code));
    return out;
}

}
