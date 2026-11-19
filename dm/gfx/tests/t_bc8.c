








#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "xbg_texture.h"


uint64_t nvn_bl_offset(uint32_t x, uint32_t y, uint32_t z, uint32_t row_bytes, uint32_t rows, unsigned tileH, unsigned tileD) {
    (void)y; (void)z; (void)row_bytes; (void)rows; (void)tileH; (void)tileD;
    return (uint64_t)x;
}

static int fails;

static void check(int ok, const char *what, uint32_t fmt) {
    if (!ok) { printf("FAIL %s (format %#x)\n", what, fmt); ++fails; }
}



static void run(uint32_t fmt, const char *name, int eight) {
    uint8_t block[16];
    for (int i = 0; i < 16; i++) block[i] = (uint8_t)(i * 37 + 11);

    xbg_bc_format f16, f8;
    xbg_bc_8bit_channels = 0; check(xbg_bc_format_info(fmt, &f16) == 0, "format_info", fmt);
    xbg_bc_8bit_channels = 1; check(xbg_bc_format_info(fmt, &f8) == 0, "format_info 8bit", fmt);

    uint8_t out16[16 * 8], out8[16 * 8];
    memset(out16, 0xAA, sizeof out16);
    memset(out8, 0xAA, sizeof out8);
    xbg_bc_8bit_channels = 0; int r16 = xbg_decode_block(fmt, block, f16.bytes_per_block, out16, 16 * f16.bytes_per_pixel);
    xbg_bc_8bit_channels = 1; int r8 = xbg_decode_block(fmt, block, f16.bytes_per_block, out8, 16 * f8.bytes_per_pixel);
    xbg_bc_8bit_channels = 0;

    check(r16 == 0 && r8 == 0, "decode returns 0", fmt);
    if (r16 || r8) { printf("%s: decode failed r16=%d r8=%d\n", name, r16, r8); return; }

    if (!eight) {

        check(f8.pixels == f16.pixels && f8.bytes_per_pixel == f16.bytes_per_pixel, "signed format unchanged", fmt);
        printf("%s: unchanged, %u bytes per pixel\n", name, f16.bytes_per_pixel);
        return;
    }
    unsigned nch = f16.bytes_per_pixel / 2;
    check(f8.bytes_per_pixel == nch, "8bit is one byte per channel", fmt);
    for (unsigned i = 0; i < 16; i++)
        for (unsigned c = 0; c < nch; c++)
            check(out8[i * f8.bytes_per_pixel + c] == out16[i * f16.bytes_per_pixel + c * 2 + 1], "8bit equals the high byte", fmt);
    printf("%s: %u -> %u bytes per pixel, high bytes match\n", name, f16.bytes_per_pixel, f8.bytes_per_pixel);
}

int main(void) {
    run(0x49, "BC4 UNORM (0x49)", 1);
    run(0x4a, "BC4 SNORM (0x4a)", 0);
    run(0x4b, "BC5 UNORM (0x4b)", 1);
    run(0x4c, "BC5 SNORM (0x4c)", 0);

    for (uint32_t fmt = 0x41; fmt <= 0x50; fmt++) {
        if (fmt == 0x49 || fmt == 0x4b) continue;
        xbg_bc_format a, b;
        xbg_bc_8bit_channels = 0; xbg_bc_format_info(fmt, &a);
        xbg_bc_8bit_channels = 1; xbg_bc_format_info(fmt, &b);
        xbg_bc_8bit_channels = 0;
        check(a.pixels == b.pixels && a.bytes_per_pixel == b.bytes_per_pixel, "switch must not touch this format", fmt);
    }
    printf(fails ? "BC8 FAIL %d\n" : "BC8 OK\n", fails);
    return fails != 0;
}
