// SPDX-License-Identifier: BSD-2-Clause
// Copyright © 2021 Billy Laws

#include <array>
#include <cstdio>
#include <cstdlib>
#include <errno.h>
#include <dlfcn.h>
#include <unistd.h>
#include <fcntl.h>
#include <android/dlext.h>
#include <android/log.h>
#include <android/api-level.h>
#include <sys/mman.h>
#include "elf_soname_patcher.h"
#include "android_linker_ns.h"

using loader_android_create_namespace_t = android_namespace_t *(*)(const char *, const char *, const char *, uint64_t, const char *, android_namespace_t *, const void *);
static loader_android_create_namespace_t loader_android_create_namespace;

static bool lib_loaded;



bool linkernsbypass_load_status() {
    return lib_loaded;
}

struct android_namespace_t *android_create_namespace(const char *name,
                                                     const char *ld_library_path,
                                                     const char *default_library_path,
                                                     uint64_t type,
                                                     const char *permitted_when_isolated_path,
                                                     android_namespace_t *parent_namespace) {
    auto caller{__builtin_return_address(0)};
    return loader_android_create_namespace(name, ld_library_path, default_library_path, type,
                                           permitted_when_isolated_path, parent_namespace, caller);
}

struct android_namespace_t *android_create_namespace_escape(const char *name,
                                                            const char *ld_library_path,
                                                            const char *default_library_path,
                                                            uint64_t type,
                                                            const char *permitted_when_isolated_path,
                                                            android_namespace_t *parent_namespace) {
    auto caller{reinterpret_cast<void *>(&dlopen)};
    return loader_android_create_namespace(name, ld_library_path, default_library_path, type,
                                           permitted_when_isolated_path, parent_namespace, caller);
}

android_get_exported_namespace_t android_get_exported_namespace;

android_link_namespaces_all_libs_t android_link_namespaces_all_libs;

android_link_namespaces_t android_link_namespaces;

bool linkernsbypass_link_namespace_to_default_all_libs(android_namespace_t *to) {


    static auto defaultNs{android_create_namespace_escape("default_copy", nullptr, nullptr, ANDROID_NAMESPACE_TYPE_SHARED, nullptr, nullptr)};
    return android_link_namespaces_all_libs(to, defaultNs);
}

void *linkernsbypass_namespace_dlopen(const char *filename, int flags, android_namespace_t *ns) {
    android_dlextinfo extInfo{
        .flags = ANDROID_DLEXT_USE_NAMESPACE,
        .library_namespace = ns
    };

    return android_dlopen_ext(filename, flags, &extInfo);
}

#ifndef __NR_memfd_create
    #if defined(__aarch64__)
        #define __NR_memfd_create 279
    #else
        #error Unsupported target architecture!
    #endif
#endif

void *linkernsbypass_namespace_dlopen_unique(const char *libPath, const char *libTargetDir, int flags, android_namespace_t *ns) {
    static std::array<char, PATH_MAX> PathBuf{};


    static uint16_t TargetId{};

    int libTargetFd{[&] () {
        if (libTargetDir) {
            snprintf(PathBuf.data(), PathBuf.size(), "%s/%d_patched.so", libTargetDir, TargetId);
            return open(PathBuf.data(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
        } else {

            errno = 0;
            int fd{static_cast<int>(syscall(__NR_memfd_create, libPath, 0))};
            if (errno == ENOSYS || fd < 0)
                return -1;
            else
                return fd;
        }
    }()};
    if (libTargetFd == -1)
        return nullptr;


    std::array<char, 3> sonameOverwrite{};
    snprintf(sonameOverwrite.data(), sonameOverwrite.size(), "%03u", TargetId++);

    if (!elf_soname_patch(libPath, libTargetFd, sonameOverwrite.data()))
        return nullptr;


    android_dlextinfo hookExtInfo{
        .flags = ANDROID_DLEXT_USE_NAMESPACE | ANDROID_DLEXT_USE_LIBRARY_FD,
        .library_fd = libTargetFd,
        .library_namespace = ns
    };


    snprintf(PathBuf.data(), PathBuf.size(), "/proc/self/fd/%d", libTargetFd);

    return android_dlopen_ext(PathBuf.data(), flags, &hookExtInfo);
}

static void *align_ptr(void *ptr) {
    return reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(ptr) & ~(getpagesize() - 1));
}


static bool range_readable(uintptr_t begin, uintptr_t end) {
    FILE *maps{fopen("/proc/self/maps", "re")};
    if (!maps)
        return false;
    char line[512];
    uintptr_t covered{begin};
    while (covered < end && fgets(line, sizeof(line), maps)) {
        unsigned long lo, hi;
        char perms[5]{};
        if (sscanf(line, "%lx-%lx %4s", &lo, &hi, perms) != 3)
            continue;
        if (lo <= covered && covered < hi) {
            if (perms[0] != 'r')
                break;
            covered = hi;
        }
    }
    fclose(maps);
    return covered >= end;
}


__attribute__((constructor)) static void resolve_linker_symbols() {
    using loader_dlopen_t = void *(*)(const char *, int, const void *);

    if (android_get_device_api_level() < 28)
        return;


    auto loader_dlopen{[]() {
        union BranchLinked {
            uint32_t raw;

            struct {
                int32_t offset : 26;
                uint8_t sig : 6;
            };

            bool Verify() {
                return sig == 0x25;
            }
        };
        static_assert(sizeof(BranchLinked) == 4, "BranchLinked is wrong size");



        auto walkStart{reinterpret_cast<uintptr_t>(&dlopen)}, walkEnd{walkStart + 64 * sizeof(BranchLinked)};
        bool madeReadable{};
        if (!range_readable(walkStart, walkEnd)) {
            if (mprotect(align_ptr(reinterpret_cast<void *>(walkStart)), walkEnd - reinterpret_cast<uintptr_t>(align_ptr(reinterpret_cast<void *>(walkStart))), PROT_READ | PROT_EXEC))
                return static_cast<loader_dlopen_t>(nullptr);
            madeReadable = true;
        }
        struct Undo { bool on; uintptr_t a, b; ~Undo() { if (on) mprotect(align_ptr(reinterpret_cast<void *>(a)), b - reinterpret_cast<uintptr_t>(align_ptr(reinterpret_cast<void *>(a))), PROT_EXEC); } } undo{madeReadable, walkStart, walkEnd};



        auto blInstr{reinterpret_cast<BranchLinked *>(&dlopen)};
        bool found{};
        for (int i{}; i < 64; i++, blInstr++)
            if (blInstr->Verify()) { found = true; break; }
        if (!found)
            return static_cast<loader_dlopen_t>(nullptr);

        return reinterpret_cast<loader_dlopen_t>(blInstr + blInstr->offset);
    }()};
    if (!loader_dlopen)
        return;





    auto ldHandle{loader_dlopen("ld-android.so", RTLD_LAZY, reinterpret_cast<void *>(&dlopen))};
    if (!ldHandle)
        return;

    android_link_namespaces_all_libs = reinterpret_cast<android_link_namespaces_all_libs_t>(dlsym(ldHandle, "__loader_android_link_namespaces_all_libs"));
    if (!android_link_namespaces_all_libs)
        return;

    android_link_namespaces = reinterpret_cast<android_link_namespaces_t>(dlsym(ldHandle, "__loader_android_link_namespaces"));
    if (!android_link_namespaces)
        return;

    auto libdlAndroidHandle{loader_dlopen("libdl_android.so", RTLD_LAZY, reinterpret_cast<void *>(&dlopen))};
    if (!libdlAndroidHandle)
        return;

    loader_android_create_namespace = reinterpret_cast<loader_android_create_namespace_t>(dlsym(libdlAndroidHandle, "__loader_android_create_namespace"));
    if (!loader_android_create_namespace)
        return;

    android_get_exported_namespace = reinterpret_cast<android_get_exported_namespace_t>(dlsym(libdlAndroidHandle, "__loader_android_get_exported_namespace"));
    if (!android_get_exported_namespace)
        return;


    lib_loaded = true;
}
