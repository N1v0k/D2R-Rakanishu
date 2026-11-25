

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>

#include "monpic.h"

namespace {

long peak_kib() {
    std::ifstream f("/proc/self/status");
    std::string line;
    while (std::getline(f, line))
        if (line.compare(0, 6, "VmHWM:") == 0) return std::atol(line.c_str() + 6);
    return -1;
}

}

int main(int argc, char **argv) {
    if (argc != 7 || std::strcmp(argv[1], "render") != 0) {
        std::fprintf(stderr, "usage: %s render <romfs> <job file> <out> <threads> <results file>\n", argv[0]);
        return 2;
    }
    const int threads = std::atoi(argv[5]);
    if (threads < 1 || threads > 64) {
        std::fprintf(stderr, "threads must be 1..64\n");
        return 2;
    }
    const auto t0 = std::chrono::steady_clock::now();
    char err[1024] = {0};
    const int rc = monpic_render(argv[2], argv[3], argv[4], threads, argv[6], err, sizeof err);
    const double s = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    if (rc) {
        std::fprintf(stderr, "monpic: %s\n", err);
        return 1;
    }
    int ok = 0, unsupported = 0, error = 0;
    std::ifstream r(argv[6]);
    std::string line;
    while (std::getline(r, line)) {
        if (line.find("\tok") != std::string::npos) ok++;
        else if (line.find("\tunsupported\t") != std::string::npos) unsupported++;
        else error++;
    }
    std::printf("pictures ok=%d unsupported=%d error=%d threads=%d wall_s=%.3f peak_kib=%ld\n", ok, unsupported, error, threads, s, peak_kib());
    return error ? 1 : 0;
}
