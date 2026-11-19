






#pragma once
#include <string.h>


static inline const char *dm_player_marker_blocked(const char *dir, const char *path) {
    size_t n = dir ? strlen(dir) : 0;
    if (!n || !path || strncmp(path, dir, n) || path[n] != '/') return NULL;
    const char *name = path + n + 1;
    if (!strncmp(name, "spv-override/", 13)) return name;
    if (strncmp(name, "gfx-", 4) || strchr(name, '/')) return NULL;
    if (!strcmp(name, "gfx-pipe-suspect") || !strcmp(name, "gfx-pipe-skip") || !strncmp(name, "gfx-pipe-inflight.", 18)) return NULL;
    return name;
}

void dm_player_guard_set_dir(const char *dir);
