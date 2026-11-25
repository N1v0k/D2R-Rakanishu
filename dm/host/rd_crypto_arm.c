
#include "rd_crypto.h"

#if defined(__aarch64__)
#include <arm_neon.h>

extern const uint32_t *rd_sha256_k;

static inline uint8x16_t ctr_block(uint8x8_t hi, uint64_t idx) {
    return vcombine_u8(hi, vrev64_u8(vcreate_u8(idx)));
}

#define ROUND4(x)                                                     \
    do {                                                              \
        x##0 = vaesmcq_u8(vaeseq_u8(x##0, k[r]));                      \
        x##1 = vaesmcq_u8(vaeseq_u8(x##1, k[r]));                      \
        x##2 = vaesmcq_u8(vaeseq_u8(x##2, k[r]));                      \
        x##3 = vaesmcq_u8(vaeseq_u8(x##3, k[r]));                      \
    } while (0)

void rd_arm_ctr(const uint8_t rk[176], const uint8_t ctr_hi[8], uint64_t idx, uint8_t *d, size_t nblk) {
    uint8x16_t k[11];
    for (int i = 0; i < 11; i++) k[i] = vld1q_u8(rk + 16 * i);
    uint8x8_t hi = vld1_u8(ctr_hi);
    while (nblk >= 4) {
        uint8x16_t b0 = ctr_block(hi, idx), b1 = ctr_block(hi, idx + 1);
        uint8x16_t b2 = ctr_block(hi, idx + 2), b3 = ctr_block(hi, idx + 3);
        for (int r = 0; r < 9; r++) ROUND4(b);
        b0 = veorq_u8(vaeseq_u8(b0, k[9]), k[10]);
        b1 = veorq_u8(vaeseq_u8(b1, k[9]), k[10]);
        b2 = veorq_u8(vaeseq_u8(b2, k[9]), k[10]);
        b3 = veorq_u8(vaeseq_u8(b3, k[9]), k[10]);
        vst1q_u8(d, veorq_u8(vld1q_u8(d), b0));
        vst1q_u8(d + 16, veorq_u8(vld1q_u8(d + 16), b1));
        vst1q_u8(d + 32, veorq_u8(vld1q_u8(d + 32), b2));
        vst1q_u8(d + 48, veorq_u8(vld1q_u8(d + 48), b3));
        d += 64; idx += 4; nblk -= 4;
    }
    while (nblk--) {
        uint8x16_t b = ctr_block(hi, idx++);
        for (int r = 0; r < 9; r++) b = vaesmcq_u8(vaeseq_u8(b, k[r]));
        b = veorq_u8(vaeseq_u8(b, k[9]), k[10]);
        vst1q_u8(d, veorq_u8(vld1q_u8(d), b));
        d += 16;
    }
    rd_wipe(k, sizeof k);
}

#define SHA_R4(m, kv)                                \
    do {                                             \
        uint32x4_t wk_ = vaddq_u32(m, kv), s0_ = st0; \
        st0 = vsha256hq_u32(st0, st1, wk_);          \
        st1 = vsha256h2q_u32(st1, s0_, wk_);         \
    } while (0)
#define SHA_SU(a, b, c, d) a = vsha256su1q_u32(vsha256su0q_u32(a, b), c, d)

void rd_arm_sha256_blocks(uint32_t h[8], const uint8_t *p, size_t nblk) {
    const uint32_t *K = rd_sha256_k;
    uint32x4_t k[16];
    for (int i = 0; i < 16; i++) k[i] = vld1q_u32(K + 4 * i);
    uint32x4_t st0 = vld1q_u32(h), st1 = vld1q_u32(h + 4);
    while (nblk--) {
        uint32x4_t a0 = st0, a1 = st1;
        uint32x4_t m0 = vreinterpretq_u32_u8(vrev32q_u8(vld1q_u8(p)));
        uint32x4_t m1 = vreinterpretq_u32_u8(vrev32q_u8(vld1q_u8(p + 16)));
        uint32x4_t m2 = vreinterpretq_u32_u8(vrev32q_u8(vld1q_u8(p + 32)));
        uint32x4_t m3 = vreinterpretq_u32_u8(vrev32q_u8(vld1q_u8(p + 48)));
        SHA_R4(m0, k[0]);  SHA_SU(m0, m1, m2, m3);
        SHA_R4(m1, k[1]);  SHA_SU(m1, m2, m3, m0);
        SHA_R4(m2, k[2]);  SHA_SU(m2, m3, m0, m1);
        SHA_R4(m3, k[3]);  SHA_SU(m3, m0, m1, m2);
        SHA_R4(m0, k[4]);  SHA_SU(m0, m1, m2, m3);
        SHA_R4(m1, k[5]);  SHA_SU(m1, m2, m3, m0);
        SHA_R4(m2, k[6]);  SHA_SU(m2, m3, m0, m1);
        SHA_R4(m3, k[7]);  SHA_SU(m3, m0, m1, m2);
        SHA_R4(m0, k[8]);  SHA_SU(m0, m1, m2, m3);
        SHA_R4(m1, k[9]);  SHA_SU(m1, m2, m3, m0);
        SHA_R4(m2, k[10]); SHA_SU(m2, m3, m0, m1);
        SHA_R4(m3, k[11]); SHA_SU(m3, m0, m1, m2);
        SHA_R4(m0, k[12]);
        SHA_R4(m1, k[13]);
        SHA_R4(m2, k[14]);
        SHA_R4(m3, k[15]);
        st0 = vaddq_u32(st0, a0);
        st1 = vaddq_u32(st1, a1);
        p += 64;
    }
    vst1q_u32(h, st0);
    vst1q_u32(h + 4, st1);
}
#endif
