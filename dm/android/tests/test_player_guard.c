
#include "../player_guard.h"
#include <assert.h>
#include <stdio.h>

#define RT "/data/user/0/at.example/files/dm"
static int blocked(const char *path) { return dm_player_marker_blocked(RT, path) != NULL; }

int main(void) {

    assert(blocked(RT "/gfx-native-bc"));
    assert(blocked(RT "/gfx-ab-toggle"));
    assert(blocked(RT "/gfx-event-guard"));
    assert(blocked(RT "/gfx-x"));
    assert(!strcmp(dm_player_marker_blocked(RT, RT "/gfx-native-bc"), "gfx-native-bc"));

    assert(blocked(RT "/spv-override/x.spv"));
    assert(!strcmp(dm_player_marker_blocked(RT, RT "/spv-override/x.spv"), "spv-override/x.spv"));

    assert(!blocked(RT "/gfx-pipe-skip"));
    assert(!blocked(RT "/gfx-pipe-suspect"));
    assert(!blocked(RT "/gfx-pipe-inflight.3"));
    assert(!blocked(RT "/gfx-pipe-inflight.12"));
    assert(blocked(RT "/gfx-pipe-inflight"));
    assert(blocked(RT "/gfx-pipe-stats"));

    assert(!blocked(RT "/boot.log"));
    assert(!blocked(RT "/vk-pipeline-cache.bin"));
    assert(!blocked(RT "/vk-pipeline-list.bin"));
    assert(!blocked(RT "/shaders/abc.xgs"));
    assert(!blocked(RT "/out/gfx-x"));
    assert(!blocked("/sdcard/gfx-x"));
    assert(!blocked(RT "x/gfx-x"));
    assert(!blocked(RT));
    assert(!blocked(NULL));

    assert(!dm_player_marker_blocked("", RT "/gfx-native-bc"));
    assert(!dm_player_marker_blocked(NULL, RT "/gfx-native-bc"));
    puts("PASS player marker guard: gfx-* and spv-override blocked, crash guard state and other paths allowed");
    return 0;
}
