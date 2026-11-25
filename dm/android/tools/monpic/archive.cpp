#include "archive.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <memory>

#include "error.h"
#include "libdeflate.h"
#include "md5.h"

namespace monpic {
namespace {

std::vector<std::string> split(const std::string &line, char sep) {
    std::vector<std::string> out;
    size_t start = 0;
    for (;;) {
        const size_t at = line.find(sep, start);
        out.push_back(line.substr(start, at == std::string::npos ? std::string::npos : at - start));
        if (at == std::string::npos) return out;
        start = at + 1;
    }
}

std::string lower(std::string s) {
    for (char &c : s)
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    return s;
}

bool decimal(const std::string &s, uint64_t limit, uint64_t *out) {
    if (s.empty() || s.size() > 19) return false;
    uint64_t v = 0;
    for (char c : s) {
        if (c < '0' || c > '9') return false;
        v = v * 10 + static_cast<uint64_t>(c - '0');
    }
    if (v > limit) return false;
    *out = v;
    return true;
}

bool hex16(const std::string &s, uint8_t out[16]) {
    if (s.size() != 32) return false;
    for (int i = 0; i < 16; i++) {
        int v = 0;
        for (int k = 0; k < 2; k++) {
            const char c = s[2 * i + k];
            const int d = c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1;
            if (d < 0) return false;
            v = v * 16 + d;
        }
        out[i] = static_cast<uint8_t>(v);
    }
    return true;
}

uint32_t be32(const uint8_t *p) { return static_cast<uint32_t>(p[0]) << 24 | p[1] << 16 | p[2] << 8 | p[3]; }

constexpr uint64_t kMaxEntry = 256u << 20;

struct Inflater {
    libdeflate_decompressor *d = libdeflate_alloc_decompressor();
    ~Inflater() { libdeflate_free_decompressor(d); }
    void run(const uint8_t *src, size_t n, uint8_t *dst, size_t size, const std::string &path) const {
        if (!d) throw PictureError("out of memory");
        size_t got = 0;
        if (libdeflate_zlib_decompress(d, src, n, dst, size, &got) != LIBDEFLATE_SUCCESS || got != size)
            throw PictureError(format("%s: a zlib block does not expand to its stated size", path.c_str()));
    }
};

}

std::string Archive::key(const std::string &path) {
    std::string p = lower(path);
    const size_t skip = p.find_first_not_of('/');
    p = skip == std::string::npos ? std::string() : p.substr(skip);
    return p.compare(0, 5, "data/") == 0 ? p : "data/" + p;
}

Archive::Archive(const std::string &romfs) : data_dir_(romfs + "/data/") {
    const std::string manifest = data_dir_ + "build_manifest.psv";
    std::ifstream f(manifest, std::ios::binary);
    if (!f) throw PictureError(format("cannot open %s: %s", manifest.c_str(), std::strerror(errno)));
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        const std::vector<std::string> fields = split(line, '|');
        if (fields.size() < 11) continue;
        std::string path = lower(fields[9]);
        const size_t colon = path.find(':');
        if (colon != std::string::npos) path = path.substr(colon + 1);
        entries_.emplace(path, Entry{fields[6], fields[4], fields[7], fields[5], fields[8]});
    }
    if (f.bad()) throw PictureError(format("cannot read %s", manifest.c_str()));
}

bool Archive::has(const std::string &path) const { return entries_.count(key(path)) != 0; }

std::vector<uint8_t> Archive::read(const std::string &path) const {
    const auto it = entries_.find(key(path));
    if (it == entries_.end()) throw PictureError(format("the game files have no %s", path.c_str()));
    const Entry &e = it->second;
    uint8_t ekey[16], digest[16];
    uint64_t raw_len, size;
    if (!hex16(e.ekey, ekey) || !hex16(e.md5, digest) || !decimal(e.raw_len, kMaxEntry, &raw_len) || !decimal(e.size, kMaxEntry, &size))
        throw PictureError(format("%s: malformed manifest row", path.c_str()));
    char name[32];
    std::snprintf(name, sizeof name, "%02d-%08d.data", ekey[9], ekey[10]);
    uint64_t offset = 0;
    for (int i = 11; i < 16; i++) offset = offset << 8 | ekey[i];

    std::vector<uint8_t> raw(raw_len);
    const std::string file = data_dir_ + name;
    std::unique_ptr<std::FILE, int (*)(std::FILE *)> f(std::fopen(file.c_str(), "rb"), std::fclose);
    if (!f) throw PictureError(format("%s: cannot open %s", path.c_str(), name));
#if defined(_WIN32)
    const bool seek_ok = _fseeki64(f.get(), static_cast<long long>(offset), SEEK_SET) == 0;
#else
    const bool seek_ok = fseeko(f.get(), static_cast<off_t>(offset), SEEK_SET) == 0;
#endif
    if (!seek_ok || std::fread(raw.data(), 1, raw.size(), f.get()) != raw.size())
        throw PictureError(format("%s: %s ends before the entry", path.c_str(), name));

    std::vector<uint8_t> data(size);
    const Inflater inflate;
    if (e.mode == "n") {
        if (raw_len != size) throw PictureError(format("%s: stored entry of %llu bytes, %llu stated", path.c_str(),
                                                       static_cast<unsigned long long>(raw_len), static_cast<unsigned long long>(size)));
        data = std::move(raw);
    } else if (e.mode == "z") {
        inflate.run(raw.data(), raw.size(), data.data(), data.size(), path);
    } else {
        if (raw.size() < 12) throw PictureError(format("%s: framed entry shorter than its header", path.c_str()));
        const uint64_t header = be32(raw.data() + 4);
        const uint64_t frames = static_cast<uint64_t>(raw[9]) << 16 | raw[10] << 8 | raw[11];
        if (header < 12 + 24 * frames || header > raw.size())
            throw PictureError(format("%s: frame table of %llu frames outside the entry", path.c_str(), static_cast<unsigned long long>(frames)));
        uint64_t pos = header, out = 0;
        for (uint64_t i = 0; i < frames; i++) {
            const uint8_t *t = raw.data() + 12 + 24 * i;
            const uint64_t stored = be32(t), expanded = be32(t + 4);
            if (stored < 1 || stored > raw.size() - pos || expanded > size - out)
                throw PictureError(format("%s: frame %llu outside the entry", path.c_str(), static_cast<unsigned long long>(i)));
            const uint8_t *b = raw.data() + pos;
            if (b[0] == 'N') {
                if (stored - 1 != expanded) throw PictureError(format("%s: stored frame of the wrong size", path.c_str()));
                std::memcpy(data.data() + out, b + 1, expanded);
            } else if (b[0] == 'Z') {
                inflate.run(b + 1, stored - 1, data.data() + out, expanded, path);
            } else {
                throw PictureError(format("%s: frame mode %d", path.c_str(), b[0]));
            }
            pos += stored;
            out += expanded;
        }
        if (out != size) throw PictureError(format("%s: frames expand to %llu bytes, %llu stated", path.c_str(),
                                                   static_cast<unsigned long long>(out), static_cast<unsigned long long>(size)));
    }
    uint8_t got[16];
    md5(data.data(), data.size(), got);
    if (std::memcmp(got, digest, 16) != 0) throw PictureError(format("%s: content does not match its MD5", path.c_str()));
    return data;
}

}
