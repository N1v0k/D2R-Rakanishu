
#include <stdlib.h>
#include "../compat/include/dm_player_env.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static const char *by_name(const char *name) { return getenv(name); }

int main(void) {
    assert(!(setenv)("DM_TEST_GOLD", "100000", 1) && !(setenv)("DM_TEST_STAT", "0:5", 1) && !(setenv)("DM_KEYS", "A@1", 1));
    assert(!(setenv)("DM_QA_STATE", "1", 1) && !(setenv)("DM_SAVE", "/data/save", 1) && !(setenv)("DM_GFX_WTRACK", "0", 1));
    assert(!(setenv)("DM_3D_SCALE", "1.00", 1) && !(setenv)("HOME", "/home/x", 1) && !(setenv)("DM_", "x", 1) && !(setenv)("D", "y", 1));

    assert(getenv("DM_TEST_GOLD") == NULL);
    assert(getenv("DM_TEST_STAT") == NULL);
    assert(getenv("DM_KEYS") == NULL);
    assert(getenv("DM_QA_STATE") == NULL);
    assert(getenv("DM_") == NULL);
    assert(getenv("DM_SAVE") && !strcmp(getenv("DM_SAVE"), "/data/save"));
    assert(getenv("DM_GFX_WTRACK") && !strcmp(getenv("DM_GFX_WTRACK"), "0"));
    assert(getenv("DM_3D_SCALE") == NULL);
    assert(getenv("HOME") && !strcmp(getenv("HOME"), "/home/x"));
    assert(getenv("D") && !strcmp(getenv("D"), "y"));

    char name[32]; snprintf(name, sizeof name, "DM_TEST_%s", "GOLD");
    assert(by_name(name) == NULL && (getenv)(name) != NULL);
    assert(by_name("DM_KEYS") == NULL);
    assert(by_name("DM_SAVE") && !strcmp(by_name("DM_SAVE"), "/data/save"));
    assert(by_name("HOME") != NULL);
    assert(by_name("DM_SAVE_X") == NULL && by_name("DM_SAV") == NULL);

    (unsetenv)("DM_TEMP"); assert(getenv("DM_TEMP") == NULL);
    puts("PASS player getenv filter: test names hidden, listed and non-DM names read, literal and run-time names alike");
    return 0;
}
