









#include <stdio.h>
#include "../dm_padmap.h"
static int checks, fails;
#define CHECK(c, ...) do { checks++; if (c) printf("  ✓ "); else { fails++; printf("  ✗ "); } printf(__VA_ARGS__); printf("\n"); } while (0)
enum { GA, GB, GX, GY };
int main(void) {
    static const struct { const char *cat, *ven, *pos, *cap; int sys; uint64_t want; } r[] = {
        { "Switch Pro Controller", "Pro Controller", "右", "A", GA, DM_BTN_A }, { "Switch Pro Controller", "Pro Controller", "下", "B", GB, DM_BTN_B },
        { "Switch Pro Controller", "Pro Controller", "上", "X", GX, DM_BTN_X }, { "Switch Pro Controller", "Pro Controller", "左", "Y", GY, DM_BTN_Y },
        { "Nintendo Switch Joy-Con (L/R)", "Joy-Con (L/R)", "右", "A", GA, DM_BTN_A }, { "Nintendo Switch Joy-Con (L/R)", "Joy-Con (L/R)", "下", "B", GB, DM_BTN_B },
        { "Nintendo Switch Joy-Con (L/R)", "Joy-Con (L/R)", "上", "X", GX, DM_BTN_X }, { "Nintendo Switch Joy-Con (L/R)", "Joy-Con (L/R)", "左", "Y", GY, DM_BTN_Y },
        { "Xbox One", "Xbox Wireless Controller", "下", "A", GA, DM_BTN_A }, { "Xbox One", "Xbox Wireless Controller", "右", "B", GB, DM_BTN_B },
        { "Xbox One", "Xbox Wireless Controller", "左", "X", GX, DM_BTN_X }, { "Xbox One", "Xbox Wireless Controller", "上", "Y", GY, DM_BTN_Y },
        { "DualSense", "DualSense Wireless Controller", "下", "×", GA, DM_BTN_A }, { "DualSense", "DualSense Wireless Controller", "右", "○", GB, DM_BTN_B },
        { "DualSense", "DualSense Wireless Controller", "左", "□", GX, DM_BTN_X }, { "DualSense", "DualSense Wireless Controller", "上", "△", GY, DM_BTN_Y } };
    static const char *const nm[] = { "A", "B", "X", "Y" };
    for (unsigned i = 0; i < sizeof r / sizeof *r; i++)
        CHECK(dm_padmap_face(r[i].sys == GA, r[i].sys == GB, r[i].sys == GX, r[i].sys == GY) == r[i].want, "%s：按%s键（印 %s，系统给 button%s）→ 游戏 %s", r[i].cat, r[i].pos, r[i].cap, nm[r[i].sys], nm[__builtin_ctzll(r[i].want)]);


    static const struct { const char *cat, *pos; int sys, nin; uint64_t want; } x[] = {
        { "Switch Pro Controller", "下", GB, 1, DM_BTN_A }, { "Switch Pro Controller", "右", GA, 1, DM_BTN_B },
        { "Switch Pro Controller", "上", GX, 1, DM_BTN_X }, { "Switch Pro Controller", "左", GY, 1, DM_BTN_Y },
        { "Xbox One", "下", GA, 0, DM_BTN_A }, { "Xbox One", "右", GB, 0, DM_BTN_B },
        { "Xbox One", "上", GY, 0, DM_BTN_X }, { "Xbox One", "左", GX, 0, DM_BTN_Y } };
    for (unsigned i = 0; i < sizeof x / sizeof *x; i++)
        CHECK(dm_padmap_face_xbox(x[i].sys == GA, x[i].sys == GB, x[i].sys == GX, x[i].sys == GY, x[i].nin) == x[i].want, "Xbox 键位 %s：按%s键（系统给 button%s）→ 游戏 %s", x[i].cat, x[i].pos, nm[x[i].sys], nm[__builtin_ctzll(x[i].want)]);
    CHECK(dm_padmap_face(1, 1, 0, 0) == (DM_BTN_A | DM_BTN_B), "两颗同时按：各算各的"); CHECK(dm_padmap_face(0, 0, 0, 0) == 0, "都没按 = 0");
    printf("== %d 项，失败 %d ==\n", checks, fails);
    return fails != 0;
}
