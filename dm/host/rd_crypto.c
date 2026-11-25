
#include "rd_crypto.h"

#include <pthread.h>
#include <string.h>
#if defined(__aarch64__) && defined(__linux__)
#include <sys/auxv.h>
#ifndef HWCAP_AES
#define HWCAP_AES (1 << 3)
#endif
#ifndef HWCAP_SHA2
#define HWCAP_SHA2 (1 << 6)
#endif
#endif

static uint8_t sbox[256], isbox[256];
static uint32_t te0[256], te1[256], te2[256], te3[256];
static int use_arm_aes, use_arm_sha, tables_ready;

static uint8_t xtime(uint8_t x) { return (uint8_t)((x << 1) ^ ((x & 0x80) ? 0x1B : 0)); }
static uint8_t rotl8(uint8_t x, int s) { return (uint8_t)((x << s) | (x >> (8 - s))); }
static uint32_t ror32(uint32_t x, int s) { return (x >> s) | (x << (32 - s)); }
static uint8_t gmul(uint8_t a, uint8_t b) {
    uint8_t r = 0;
    while (b) {
        if (b & 1) r ^= a;
        a = xtime(a);
        b >>= 1;
    }
    return r;
}

static void build_tables(void) {

    uint8_t p = 1, q = 1;
    do {
        p = (uint8_t)(p ^ xtime(p));
        q = (uint8_t)(q ^ (q << 1));
        q = (uint8_t)(q ^ (q << 2));
        q = (uint8_t)(q ^ (q << 4));
        if (q & 0x80) q ^= 0x09;
        uint8_t x = (uint8_t)(q ^ rotl8(q, 1) ^ rotl8(q, 2) ^ rotl8(q, 3) ^ rotl8(q, 4));
        sbox[p] = (uint8_t)(x ^ 0x63);
    } while (p != 1);
    sbox[0] = 0x63;
    for (int i = 0; i < 256; i++) {
        uint8_t s = sbox[i];
        isbox[s] = (uint8_t)i;
        uint32_t w = ((uint32_t)xtime(s) << 24) | ((uint32_t)s << 16) | ((uint32_t)s << 8) |
                     (uint32_t)(xtime(s) ^ s);
        te0[i] = w;
        te1[i] = ror32(w, 8);
        te2[i] = ror32(w, 16);
        te3[i] = ror32(w, 24);
    }
    tables_ready = 1;
}

void rd_crypto_init(int force_portable) {
    if (!tables_ready) build_tables();
    use_arm_aes = use_arm_sha = 0;
#if defined(__aarch64__) && defined(__linux__)
    if (!force_portable) {
        unsigned long hw = getauxval(AT_HWCAP);
        use_arm_aes = (hw & HWCAP_AES) != 0;
        use_arm_sha = (hw & HWCAP_SHA2) != 0;
    }
#else
    (void)force_portable;
#endif
}

static pthread_once_t ready_once = PTHREAD_ONCE_INIT;
static int ready_result = -1;
static void ready_run(void) {
    rd_crypto_init(0);
    ready_result = rd_crypto_selftest() == 0 ? 0 : -1;
}
int rd_crypto_ready(void) {
    pthread_once(&ready_once, ready_run);
    return ready_result;
}

const char *rd_crypto_impl(void) {
    if (use_arm_aes && use_arm_sha) return "armv8-aes+sha2";
    if (use_arm_aes) return "armv8-aes+portable-sha";
    if (use_arm_sha) return "portable-aes+armv8-sha2";
    return "portable";
}

void rd_wipe(void *p, size_t n) {
    volatile uint8_t *v = (volatile uint8_t *)p;
    while (n--) *v++ = 0;
}

void rd_aes128_setkey(rd_aes128 *k, const uint8_t key[16]) {
    if (!tables_ready) build_tables();
    uint8_t rcon = 1;
    memcpy(k->rk, key, 16);
    for (int i = 16; i < 176; i += 4) {
        uint8_t t[4] = {k->rk[i - 4], k->rk[i - 3], k->rk[i - 2], k->rk[i - 1]};
        if (i % 16 == 0) {
            uint8_t t0 = t[0];
            t[0] = (uint8_t)(sbox[t[1]] ^ rcon);
            t[1] = sbox[t[2]];
            t[2] = sbox[t[3]];
            t[3] = sbox[t0];
            rcon = xtime(rcon);
        }
        for (int j = 0; j < 4; j++) k->rk[i + j] = (uint8_t)(k->rk[i - 16 + j] ^ t[j]);
        rd_wipe(t, sizeof t);
    }
    for (int i = 0; i < 44; i++)
        k->ek[i] = ((uint32_t)k->rk[4 * i] << 24) | ((uint32_t)k->rk[4 * i + 1] << 16) |
                   ((uint32_t)k->rk[4 * i + 2] << 8) | k->rk[4 * i + 3];
}

static uint32_t ld32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
}
static void st32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16); p[2] = (uint8_t)(v >> 8); p[3] = (uint8_t)v;
}

static void enc_portable(const rd_aes128 *k, const uint8_t in[16], uint8_t out[16]) {
    const uint32_t *rk = k->ek;
    uint32_t s0 = ld32(in) ^ rk[0], s1 = ld32(in + 4) ^ rk[1];
    uint32_t s2 = ld32(in + 8) ^ rk[2], s3 = ld32(in + 12) ^ rk[3];
    for (int r = 1; r < 10; r++) {
        rk += 4;
        uint32_t t0 = te0[s0 >> 24] ^ te1[(s1 >> 16) & 255] ^ te2[(s2 >> 8) & 255] ^ te3[s3 & 255] ^ rk[0];
        uint32_t t1 = te0[s1 >> 24] ^ te1[(s2 >> 16) & 255] ^ te2[(s3 >> 8) & 255] ^ te3[s0 & 255] ^ rk[1];
        uint32_t t2 = te0[s2 >> 24] ^ te1[(s3 >> 16) & 255] ^ te2[(s0 >> 8) & 255] ^ te3[s1 & 255] ^ rk[2];
        uint32_t t3 = te0[s3 >> 24] ^ te1[(s0 >> 16) & 255] ^ te2[(s1 >> 8) & 255] ^ te3[s2 & 255] ^ rk[3];
        s0 = t0; s1 = t1; s2 = t2; s3 = t3;
    }
    rk += 4;
#define LAST(a, b, c, d) (((uint32_t)sbox[(a) >> 24] << 24) | ((uint32_t)sbox[((b) >> 16) & 255] << 16) | \
                          ((uint32_t)sbox[((c) >> 8) & 255] << 8) | (uint32_t)sbox[(d) & 255])
    st32(out, LAST(s0, s1, s2, s3) ^ rk[0]);
    st32(out + 4, LAST(s1, s2, s3, s0) ^ rk[1]);
    st32(out + 8, LAST(s2, s3, s0, s1) ^ rk[2]);
    st32(out + 12, LAST(s3, s0, s1, s2) ^ rk[3]);
#undef LAST
}

void rd_aes128_encrypt_block(const rd_aes128 *k, const uint8_t in[16], uint8_t out[16]) {
    enc_portable(k, in, out);
}


void rd_aes128_decrypt_block(const rd_aes128 *k, const uint8_t in[16], uint8_t out[16]) {
    uint8_t s[16], t[16];
    for (int i = 0; i < 16; i++) s[i] = (uint8_t)(in[i] ^ k->rk[160 + i]);
    for (int r = 9; r >= 0; r--) {
        for (int c = 0; c < 4; c++)
            for (int row = 0; row < 4; row++) t[row + 4 * c] = isbox[s[row + 4 * ((c - row + 4) % 4)]];
        for (int i = 0; i < 16; i++) t[i] ^= k->rk[16 * r + i];
        if (r == 0) { memcpy(s, t, 16); break; }
        for (int c = 0; c < 4; c++) {
            uint8_t *col = t + 4 * c;
            uint8_t a0 = col[0], a1 = col[1], a2 = col[2], a3 = col[3];
            s[4 * c + 0] = (uint8_t)(gmul(a0, 14) ^ gmul(a1, 11) ^ gmul(a2, 13) ^ gmul(a3, 9));
            s[4 * c + 1] = (uint8_t)(gmul(a0, 9) ^ gmul(a1, 14) ^ gmul(a2, 11) ^ gmul(a3, 13));
            s[4 * c + 2] = (uint8_t)(gmul(a0, 13) ^ gmul(a1, 9) ^ gmul(a2, 14) ^ gmul(a3, 11));
            s[4 * c + 3] = (uint8_t)(gmul(a0, 11) ^ gmul(a1, 13) ^ gmul(a2, 9) ^ gmul(a3, 14));
        }
    }
    memcpy(out, s, 16);
    rd_wipe(s, sizeof s);
    rd_wipe(t, sizeof t);
}

void rd_aes128_ctr(const rd_aes128 *k, const uint8_t ctr_hi[8], uint64_t idx, uint8_t *d, size_t len) {
#if defined(__aarch64__)
    size_t nblk = len / 16;
    if (use_arm_aes && nblk) {
        rd_arm_ctr(k->rk, ctr_hi, idx, d, nblk);
        d += nblk * 16;
        idx += nblk;
        len -= nblk * 16;
    }
#endif
    uint8_t cb[16], ks[16];
    memcpy(cb, ctr_hi, 8);
    while (len) {
        for (int i = 0; i < 8; i++) cb[8 + i] = (uint8_t)(idx >> (56 - 8 * i));
        enc_portable(k, cb, ks);
        size_t n = len < 16 ? len : 16;
        if (n == 16) {
            uint64_t a, b, x, y;
            memcpy(&a, d, 8); memcpy(&b, d + 8, 8); memcpy(&x, ks, 8); memcpy(&y, ks + 8, 8);
            a ^= x; b ^= y;
            memcpy(d, &a, 8); memcpy(d + 8, &b, 8);
        } else {
            for (size_t i = 0; i < n; i++) d[i] ^= ks[i];
        }
        d += n; len -= n; idx++;
    }
    rd_wipe(ks, sizeof ks);
    rd_wipe(cb, sizeof cb);
}

void rd_aes128_xts_decrypt(const rd_aes128 *kd, const rd_aes128 *kt, uint64_t unit, uint8_t *data,
                           size_t unit_size) {
    uint8_t tw[16] = {0}, x[16];
    for (int i = 0; i < 8; i++) tw[15 - i] = (uint8_t)(unit >> (8 * i));
    rd_aes128_encrypt_block(kt, tw, tw);
    for (size_t off = 0; off + 16 <= unit_size; off += 16) {
        for (int i = 0; i < 16; i++) x[i] = (uint8_t)(data[off + i] ^ tw[i]);
        rd_aes128_decrypt_block(kd, x, x);
        for (int i = 0; i < 16; i++) data[off + i] = (uint8_t)(x[i] ^ tw[i]);
        uint8_t carry = (uint8_t)(tw[15] >> 7);
        for (int i = 15; i > 0; i--) tw[i] = (uint8_t)((tw[i] << 1) | (tw[i - 1] >> 7));
        tw[0] = (uint8_t)((tw[0] << 1) ^ (carry ? 0x87 : 0));
    }
    rd_wipe(tw, sizeof tw);
    rd_wipe(x, sizeof x);
}


static const uint32_t K256[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};
const uint32_t *rd_sha256_k = K256;

static void sha_blocks_portable(uint32_t h[8], const uint8_t *p, size_t nblk) {
    while (nblk--) {
        uint32_t w[64];
        for (int i = 0; i < 16; i++) w[i] = ld32(p + 4 * i);
        for (int i = 16; i < 64; i++) {
            uint32_t s0 = ror32(w[i - 15], 7) ^ ror32(w[i - 15], 18) ^ (w[i - 15] >> 3);
            uint32_t s1 = ror32(w[i - 2], 17) ^ ror32(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }
        uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4], f = h[5], g = h[6], hh = h[7];
        for (int i = 0; i < 64; i++) {
            uint32_t t1 = hh + (ror32(e, 6) ^ ror32(e, 11) ^ ror32(e, 25)) + ((e & f) ^ (~e & g)) + K256[i] + w[i];
            uint32_t t2 = (ror32(a, 2) ^ ror32(a, 13) ^ ror32(a, 22)) + ((a & b) ^ (a & c) ^ (b & c));
            hh = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2;
        }
        h[0] += a; h[1] += b; h[2] += c; h[3] += d; h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
        p += 64;
    }
}

static void sha_blocks(uint32_t h[8], const uint8_t *p, size_t nblk) {
#if defined(__aarch64__)
    if (use_arm_sha) { rd_arm_sha256_blocks(h, p, nblk); return; }
#endif
    sha_blocks_portable(h, p, nblk);
}

void rd_sha256_init(rd_sha256_ctx *s) {
    static const uint32_t iv[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                                   0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
    memcpy(s->h, iv, sizeof iv);
    s->total = 0;
    s->used = 0;
}

void rd_sha256_update(rd_sha256_ctx *s, const void *data, size_t len) {
    const uint8_t *p = (const uint8_t *)data;
    s->total += len;
    if (s->used) {
        size_t n = 64 - s->used;
        if (n > len) n = len;
        memcpy(s->buf + s->used, p, n);
        s->used += n; p += n; len -= n;
        if (s->used < 64) return;
        sha_blocks(s->h, s->buf, 1);
        s->used = 0;
    }
    if (len >= 64) {
        sha_blocks(s->h, p, len / 64);
        p += len & ~(size_t)63;
        len &= 63;
    }
    memcpy(s->buf, p, len);
    s->used = len;
}

void rd_sha256_final(rd_sha256_ctx *s, uint8_t out[32]) {
    uint64_t bits = s->total * 8;
    uint8_t pad[72] = {0x80};
    size_t padlen = (s->used < 56) ? 56 - s->used : 120 - s->used;
    uint8_t lenb[8];
    for (int i = 0; i < 8; i++) lenb[i] = (uint8_t)(bits >> (56 - 8 * i));
    uint64_t keep = s->total;
    rd_sha256_update(s, pad, padlen);
    rd_sha256_update(s, lenb, 8);
    s->total = keep;
    for (int i = 0; i < 8; i++) st32(out + 4 * i, s->h[i]);
}

void rd_sha256(const void *data, size_t len, uint8_t out[32]) {
    rd_sha256_ctx s;
    rd_sha256_init(&s);
    rd_sha256_update(&s, data, len);
    rd_sha256_final(&s, out);
}


static int unhex(const char *h, uint8_t *o, size_t n) {
    for (size_t i = 0; i < n; i++) {
        unsigned v = 0;
        for (int j = 0; j < 2; j++) {
            char c = h[2 * i + j];
            v <<= 4;
            if (c >= '0' && c <= '9') v |= (unsigned)(c - '0');
            else if (c >= 'a' && c <= 'f') v |= (unsigned)(c - 'a' + 10);
            else return -1;
        }
        o[i] = (uint8_t)v;
    }
    return 0;
}

int rd_crypto_selftest(void) {
    int fails = 0;
    rd_aes128 k;
    uint8_t key[16], pt[64], ct[64], out[64], iv[16];
    unhex("000102030405060708090a0b0c0d0e0f", key, 16);
    unhex("00112233445566778899aabbccddeeff", pt, 16);
    unhex("69c4e0d86a7b0430d8cdb78070b4c55a", ct, 16);
    rd_aes128_setkey(&k, key);
    rd_aes128_encrypt_block(&k, pt, out);
    fails += memcmp(out, ct, 16) != 0;
    rd_aes128_decrypt_block(&k, ct, out);
    fails += memcmp(out, pt, 16) != 0;

    unhex("2b7e151628aed2a6abf7158809cf4f3c", key, 16);
    unhex("f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff", iv, 16);
    unhex("6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e51"
          "30c81c46a35ce411e5fbc1191a0a52eff69f2445df4f9b17ad2b417be66c3710", pt, 64);
    unhex("874d6191b620e3261bef6864990db6ce9806f66b7970fdff8617187bb9fffdff"
          "5ae4df3edbd5d35e5b4f09020db03eab1e031dda2fbe03d1792170a0f3009cee", ct, 64);
    rd_aes128_setkey(&k, key);
    uint64_t lo = 0;
    for (int i = 0; i < 8; i++) lo = (lo << 8) | iv[8 + i];
    for (size_t len = 64; len >= 16; len = len > 24 ? len - 24 : 0) {
        memcpy(out, pt, len);
        rd_aes128_ctr(&k, iv, lo, out, len);
        fails += memcmp(out, ct, len) != 0;
    }

    uint8_t d[32], e[32];
    rd_sha256("abc", 3, d);
    unhex("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", e, 32);
    fails += memcmp(d, e, 32) != 0;
    const char *m2 = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    rd_sha256(m2, strlen(m2), d);
    unhex("248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1", e, 32);
    fails += memcmp(d, e, 32) != 0;

    static uint8_t big[4096 + 7];
    for (size_t i = 0; i < sizeof big; i++) big[i] = (uint8_t)(i * 131 + 7);
    uint32_t h1[8], h2[8];
    for (int i = 0; i < 8; i++) h1[i] = h2[i] = (uint32_t)(0x01020304u * (unsigned)(i + 1));
    sha_blocks(h1, big, 64);
    sha_blocks_portable(h2, big, 64);
    fails += memcmp(h1, h2, sizeof h1) != 0;
    uint8_t ref[4096 + 7];
    memcpy(ref, big, sizeof big);
    rd_aes128_ctr(&k, iv, 12345, big, sizeof big);

    uint8_t cb[16], ks[16];
    memcpy(cb, iv, 8);
    for (size_t b = 0; b * 16 < sizeof ref; b++) {
        uint64_t idx = 12345 + b;
        for (int i = 0; i < 8; i++) cb[8 + i] = (uint8_t)(idx >> (56 - 8 * i));
        enc_portable(&k, cb, ks);
        for (size_t i = 0; i < 16 && b * 16 + i < sizeof ref; i++) ref[b * 16 + i] ^= ks[i];
    }
    fails += memcmp(ref, big, sizeof big) != 0;
    return fails;
}
