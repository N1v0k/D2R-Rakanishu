



#include "../hle.h"
#include <stdio.h>

void *guest_sym(const char *n) { (void)n; return NULL; }
struct dm_flags g_flag; uint64_t g_t0_ns;
uint64_t now_ns(void) { return 0; }
typedef int64_t (*mv_read_fn)(void *ctx, int64_t off, void *buf, size_t n);
int64_t mv_webm_duration_ns(mv_read_fn rd, void *ctx);

static int fails, checks;
#define CHECK(c, ...) do { checks++; if (!(c)) { fails++; printf("  ✗ "); printf(__VA_ARGS__); printf("\n"); } else { printf("  ✓ "); printf(__VA_ARGS__); printf("\n"); } } while (0)

typedef struct { const uint8_t *p; size_t n; } mem;
static int64_t rd_mem(void *c_, int64_t off, void *buf, size_t n) {
    mem *c = c_; if (off < 0 || (size_t)off >= c->n) return 0;
    size_t k = c->n - off < n ? c->n - off : n; memcpy(buf, c->p + off, k); return (int64_t)k;
}
static int64_t dur(const uint8_t *p, size_t n) { mem m = {p, n}; return mv_webm_duration_ns(rd_mem, &m); }

int main(void) {




    static const uint8_t a[] = {
        0x1A,0x45,0xDF,0xA3, 0x80,
        0x18,0x53,0x80,0x67, 0x9C,
          0x11,0x4D,0x9B,0x74, 0x80,
          0x15,0x49,0xA9,0x66, 0x92,
            0x2A,0xD7,0xB1, 0x83, 0x0F,0x42,0x40,
            0x44,0x89, 0x88, 0x40,0xB9,0xE9,0x00,0x00,0x00,0x00,0x00
    };
    CHECK(dur(a, sizeof a) == 6633000000ll, "float64 时长 6633 × 1ms = 6.633 s（读到 %lld）", (long long)dur(a, sizeof a));


    static const uint8_t b[] = {
        0x1A,0x45,0xDF,0xA3, 0x80,
        0x18,0x53,0x80,0x67, 0x01,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
          0x15,0x49,0xA9,0x66, 0x8E,
            0x2A,0xD7,0xB1, 0x83, 0x07,0xA1,0x20,
            0x44,0x89, 0x84, 0x44,0xFA,0x00,0x00
    };
    CHECK(dur(b, sizeof b) == 1000000000ll, "float32 2000 × 0.5ms + 未知长度 Segment = 1.000 s（读到 %lld）", (long long)dur(b, sizeof b));


    static const uint8_t c[] = {
        0x1A,0x45,0xDF,0xA3, 0x80,
        0x18,0x53,0x80,0x67, 0x90,
          0x15,0x49,0xA9,0x66, 0x8B,
            0x44,0x89, 0x88, 0x40,0xC3,0x98,0x80,0x00,0x00,0x00,0x00
    };
    CHECK(dur(c, sizeof c) == 10033000000ll, "缺省 TimecodeScale = 1ms，10033 ⇒ 10.033 s（读到 %lld）", (long long)dur(c, sizeof c));


    static const uint8_t d[] = { 0x1A,0x45,0xDF,0xA3, 0x80, 0x18,0x53,0x80,0x67, 0x85, 0x1F,0x43,0xB6,0x75, 0x80 };
    CHECK(dur(d, sizeof d) == -1, "无 Info 先到 Cluster ⇒ -1（读到 %lld）", (long long)dur(d, sizeof d));


    static const uint8_t e[] = { 0x1A,0x45,0xDF,0xA3, 0x80, 0x18,0x53,0x80,0x67, 0x8C, 0x15,0x49,0xA9,0x66, 0x87, 0x2A,0xD7,0xB1, 0x83, 0x0F,0x42,0x40 };
    CHECK(dur(e, sizeof e) == -1, "无 Duration ⇒ -1（读到 %lld）", (long long)dur(e, sizeof e));


    CHECK(dur(a, 10) == -1, "截断 ⇒ -1");
    printf(fails ? "t_movie 失败 %d/%d\n" : "t_movie 全过：%d/%d\n", fails ? fails : checks, checks);
    return fails != 0;
}
