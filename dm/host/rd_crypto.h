


#ifndef RD_CRYPTO_H
#define RD_CRYPTO_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t rk[176];
    uint32_t ek[44];
} rd_aes128;

typedef struct {
    uint32_t h[8];
    uint64_t total;
    uint8_t buf[64];
    size_t used;
} rd_sha256_ctx;



int rd_crypto_ready(void);
const char *rd_crypto_impl(void);

void rd_crypto_init(int force_portable);
int rd_crypto_selftest(void);

void rd_aes128_setkey(rd_aes128 *k, const uint8_t key[16]);
void rd_aes128_encrypt_block(const rd_aes128 *k, const uint8_t in[16], uint8_t out[16]);
void rd_aes128_decrypt_block(const rd_aes128 *k, const uint8_t in[16], uint8_t out[16]);


void rd_aes128_ctr(const rd_aes128 *k, const uint8_t ctr_hi[8], uint64_t block_index,
                   uint8_t *data, size_t len);

void rd_aes128_xts_decrypt(const rd_aes128 *kdata, const rd_aes128 *ktweak, uint64_t unit,
                           uint8_t *data, size_t unit_size);

void rd_sha256_init(rd_sha256_ctx *s);
void rd_sha256_update(rd_sha256_ctx *s, const void *data, size_t len);
void rd_sha256_final(rd_sha256_ctx *s, uint8_t out[32]);
void rd_sha256(const void *data, size_t len, uint8_t out[32]);

void rd_wipe(void *p, size_t n);


void rd_arm_ctr(const uint8_t rk[176], const uint8_t ctr_hi[8], uint64_t idx, uint8_t *d, size_t nblk);
void rd_arm_sha256_blocks(uint32_t h[8], const uint8_t *p, size_t nblk);

#endif
