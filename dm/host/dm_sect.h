





#pragma once
#ifdef __APPLE__
#include <mach-o/getsect.h>
#include <mach-o/ldsyms.h>
#define DM_SECT(name) __attribute__((used, section("__DATA," #name)))


#define DM_SECT_GET(name, type, p, bytes) do { unsigned long dm_sz_ = 0; \
    (p) = (type *)getsectiondata(&_mh_execute_header, "__DATA", #name, &dm_sz_); (bytes) = (p) ? dm_sz_ : 0; } while (0)
#else
#define DM_SECT(name) __attribute__((used, retain, section(#name)))
#define DM_SECT_GET(name, type, p, bytes) do { \
    extern const char __start_##name[] __attribute__((weak, visibility("hidden"))); \
    extern const char __stop_##name[] __attribute__((weak, visibility("hidden"))); \
    (p) = (type *)__start_##name; (bytes) = __start_##name ? (unsigned long)(__stop_##name - __start_##name) : 0; } while (0)
#endif
