
#pragma once
#include <cstdarg>
#include <cstdio>
#include <stdexcept>
#include <string>

namespace monpic {


struct PictureError : std::runtime_error {
    explicit PictureError(const std::string &what) : std::runtime_error(what) {}
};


struct Unsupported : PictureError {
    explicit Unsupported(const std::string &what) : PictureError(what) {}
};

#if defined(__GNUC__)
__attribute__((format(printf, 1, 2)))
#endif
inline std::string format(const char *fmt, ...) {
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    std::vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    return buf;
}

}
