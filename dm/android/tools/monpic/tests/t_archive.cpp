

#include <cstring>

#include "archive.h"
#include "error.h"
#include "md5.h"
#include "support.h"

using monpic::Archive;
using monpic::PictureError;

namespace {

std::vector<uint8_t> bytes(const std::string &s) { return std::vector<uint8_t>(s.begin(), s.end()); }

std::string ekey(uint64_t offset) {
    uint8_t k[16] = {0};
    k[9] = 0;
    k[10] = 1;
    for (int i = 0; i < 5; i++) k[11 + i] = static_cast<uint8_t>(offset >> (8 * (4 - i)));
    char s[33];
    for (int i = 0; i < 16; i++) std::snprintf(s + 2 * i, 3, "%02x", k[i]);
    return s;
}

void be32(std::vector<uint8_t> &v, uint32_t x) {
    for (int i = 3; i >= 0; i--) v.push_back(static_cast<uint8_t>(x >> (8 * i)));
}

}

int main() {
    const std::pair<std::string, std::string> vectors[] = {
        {"", "d41d8cd98f00b204e9800998ecf8427e"},
        {"abc", "900150983cd24fb0d6963f7d28e17f72"},
        {"message digest", "f96b697d7cb7938d525a2f31aaf161d0"},
        {"12345678901234567890123456789012345678901234567890123456789012345678901234567890", "57edf4a22be3c955ac49da2e2107b67a"}};
    for (const auto &v : vectors) CHECK(test::md5_hex(bytes(v.first)) == v.second, "md5 of '%s'", v.first.c_str());

    const std::string dir = test::temp_dir();
    std::string mkdir_cmd = "mkdir -p " + dir + "/data";
    CHECK(std::system(mkdir_cmd.c_str()) == 0, "data folder");
    std::vector<uint8_t> data;
    std::string manifest = "FileId!STRING:0|KeyLayout!DEC:4|Chunk!DEC:8|Archive!DEC:8|CKey!HEX:16|CSize!DEC:4|EKey!HEX:16|ESize!DEC:4|Encoding!STRING:0|Path!STRING:0|Flags!STRING:0\n## comment\n";
    auto add = [&](const std::string &path, const std::vector<uint8_t> &content, const std::vector<uint8_t> &stored, const std::string &mode,
                   const std::string &md5 = "") {
        manifest += "x|0|0|1|" + (md5.empty() ? test::md5_hex(content) : md5) + "|" + std::to_string(content.size()) + "|" + ekey(data.size()) + "|" +
                    std::to_string(stored.size()) + "|" + mode + "|data:" + path + "|table\n";
        data.insert(data.end(), stored.begin(), stored.end());
    };
    const std::vector<uint8_t> plain = bytes("plain entry"), packed(100000, 'z');
    add("data/hd/Plain.TXT", plain, plain, "n");
    add("data/hd/plain.txt", bytes("second row"), bytes("second row"), "n");
    add("data/hd/zipped.bin", packed, test::zlib_stored(packed), "z");

    const std::vector<uint8_t> f1 = bytes("first frame,"), f2 = bytes("second frame");
    std::vector<uint8_t> framed = bytes("BLTE");
    be32(framed, 12 + 2 * 24);
    framed.insert(framed.end(), {0x0f, 0, 0, 2});
    std::vector<uint8_t> z2 = test::zlib_stored(f2);
    be32(framed, static_cast<uint32_t>(f1.size() + 1));
    be32(framed, static_cast<uint32_t>(f1.size()));
    framed.insert(framed.end(), 16, 0);
    be32(framed, static_cast<uint32_t>(z2.size() + 1));
    be32(framed, static_cast<uint32_t>(f2.size()));
    framed.insert(framed.end(), 16, 0);
    framed.push_back('N');
    framed.insert(framed.end(), f1.begin(), f1.end());
    framed.push_back('Z');
    framed.insert(framed.end(), z2.begin(), z2.end());
    std::vector<uint8_t> whole = f1;
    whole.insert(whole.end(), f2.begin(), f2.end());
    add("data/hd/framed.bin", whole, framed, "b:{12=n,*=z}");
    add("data/hd/badsum.txt", plain, plain, "n", std::string(32, '0'));
    framed[8 + 3] = 9;
    add("data/hd/badframes.bin", whole, framed, "b:{*=z}");
    add("data/hd/badzip.bin", packed, bytes("not zlib at all"), "z");
    manifest += "x|0|0|1|" + test::md5_hex(plain) + "|11|" + ekey(1u << 30) + "|11|n|data:data/hd/beyond.txt|table\n";
    manifest += "x|0|0|1|nothex|11|" + ekey(0) + "|11|n|data:data/hd/badrow.txt|table\n";
    test::write_file(dir + "/data/00-00000001.data", data);
    test::write_file(dir + "/data/build_manifest.psv", bytes(manifest));

    const Archive a(dir);
    CHECK(a.read("hd/plain.txt") == plain && a.read("/DATA/HD/PLAIN.txt") == plain, "stored entry, path without case and data/, first row wins");
    CHECK(a.has("hd/zipped.bin") && !a.has("hd/nothing.bin"), "has");
    CHECK(a.read("data/hd/zipped.bin") == packed, "zlib entry");
    CHECK(a.read("hd/framed.bin") == whole, "framed entry");
    for (const char *bad : {"hd/nothing.bin", "hd/badsum.txt", "hd/badframes.bin", "hd/badzip.bin", "hd/beyond.txt", "hd/badrow.txt"})
        CHECK(test::throws<PictureError>([&] { a.read(bad); }), "refused: %s", bad);
    CHECK(test::throws<PictureError>([&] { Archive none(dir + "/nothing"); }), "no manifest");
    std::printf("t_archive: %d checks passed\n", test::checks);
    return 0;
}
