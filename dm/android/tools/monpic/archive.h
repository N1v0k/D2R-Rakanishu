


#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace monpic {

class Archive {
public:
    explicit Archive(const std::string &romfs);

    bool has(const std::string &path) const;
    std::vector<uint8_t> read(const std::string &path) const;

    static std::string key(const std::string &path);

private:
    struct Entry {
        std::string ekey, md5, raw_len, size, mode;
    };
    std::string data_dir_;
    std::unordered_map<std::string, Entry> entries_;
};

}
