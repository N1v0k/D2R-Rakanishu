






















#ifndef MONPIC_H
#define MONPIC_H

#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32)
#define MONPIC_API __declspec(dllexport)
#else
#define MONPIC_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif


MONPIC_API int monpic_bitknit_decode(const uint8_t *src, size_t src_len, uint8_t *dst, size_t dst_len);



MONPIC_API int monpic_render(const char *romfs, const char *jobs, const char *out, int threads, const char *results, char *err,
                             size_t err_len);

#ifdef __cplusplus
}
#endif

#endif
