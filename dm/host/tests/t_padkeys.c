


#include <stdio.h>
#include "../dm_padkeys.h"
#include "../dm_input.h"

static int checks, fails;
#define CHECK(c, msg) do { checks++; if (!(c)) { fails++; fprintf(stderr, "FAIL: %s (line %d)\n", msg, __LINE__); } } while (0)

int main(void) {
    dm_padkeys k = { 0 }; uint64_t pr, g;
    const uint64_t X = DM_BTN_X, A = DM_BTN_A, L = DM_BTN_LEFT;
    g = dm_padkeys_step(&k, X, X | A | L, &pr);
    CHECK(pr == X && g == 0, "an owned press goes to the page, not to the game");
    g = dm_padkeys_step(&k, X, A | L, &pr);
    CHECK(pr == 0 && g == 0, "X still held after the page gave it up: the game does not see a new press");
    g = dm_padkeys_step(&k, X, 0, &pr);
    CHECK(g == 0, "nor once no page owns anything");
    g = dm_padkeys_step(&k, 0, 0, &pr);
    g = dm_padkeys_step(&k, X, 0, &pr);
    CHECK(g == X, "released and pressed again, X is the game's");
    g = dm_padkeys_step(&k, X | L, L, &pr);
    CHECK(pr == L && g == X, "a held button the page never owned stays the game's");
    g = dm_padkeys_step(&k, 0, L, &pr); g = dm_padkeys_step(&k, A, L, &pr);
    CHECK(pr == 0 && g == A, "buttons the page does not own pass");
    printf("t_padkeys: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
