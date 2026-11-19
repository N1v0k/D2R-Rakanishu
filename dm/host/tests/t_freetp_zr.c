





#include "../dm_freetp.c"
static int g_wset;
int dm_swapskill_wset(void) { return g_wset; }
static int checks, fails;
#define CHECK(c) do { checks++; if (!(c)) { fails++; printf("FAIL line %d: %s\n", __LINE__, #c); } } while (0)
enum { MAP = 0x2d13cbc, BIND = 0x2d5f0c0, STRIDE = 28 };
static uint8_t *mb; static _Alignas(8) uint8_t player[0x200];
static void face(int i, uint32_t lg, uint32_t nm, uint32_t al) { uint32_t v[3] = { lg, nm, al }; memcpy(mb + MAP + 12 * i, v, 12); }
static void bind(int slot, uint32_t id) { memcpy(mb + BIND + (uint64_t)slot * STRIDE, &id, 4); }
static void step(void) { dm_freetp_client_step(mb, (uint64_t)(uintptr_t)player); }
int main(void) {
    mb = calloc(1, BIND + 32 * STRIDE); if (!mb) return 2;
    for (int s = 0; s < 32; s++) bind(s, ZR_EMPTY);
    face(0, 0x2000, 0, 6); face(1, 0x1000, 1, 7); face(2, 0x8000, 2, 8); face(3, 0x4000, 3, 9); face(4, 0x200, 4, 10); face(5, 0x800, 5, 11);

    for (g_wset = -1; g_wset <= 1; g_wset++) { step(); CHECK(dm_freetp_zr_free(0) && dm_freetp_zr_free(1)); }

    bind(5, 249); bind(11, 250);
    for (g_wset = -1; g_wset <= 1; g_wset++) { step(); CHECK(!dm_freetp_zr_free(0)); CHECK(!dm_freetp_zr_free(1)); }

    bind(5, ZR_EMPTY); g_wset = 1; step(); CHECK(dm_freetp_zr_free(0)); CHECK(!dm_freetp_zr_free(1));

    bind(11, ZR_EMPTY); bind(16 + 5, 249); g_wset = 1; step(); CHECK(!dm_freetp_zr_free(0)); CHECK(dm_freetp_zr_free(1));

    dm_freetp_client_step(mb, 0); CHECK(!dm_freetp_zr_free(0) && !dm_freetp_zr_free(1));
    printf("%s t_freetp_zr：%d 项检查，%d 项失败\n", fails ? "✗" : "✓", checks, fails);
    return fails != 0;
}
