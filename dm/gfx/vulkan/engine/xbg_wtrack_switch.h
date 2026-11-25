







#pragma once
#include <stdbool.h>
#include <stdlib.h>

enum { XBG_WTRACK_NO_MARKER = -2 };



static inline int xbg_wtrack_marker(int marker_byte, bool no_wtrack, const char *env) {
    int marker = -1;
    if (marker_byte != XBG_WTRACK_NO_MARKER) marker = marker_byte == '1' ? 1 : 0;
    else if (env) marker = atoi(env) != 0 ? 1 : 0;
    return no_wtrack ? 0 : marker;
}
static inline bool xbg_wtrack_enabled(int marker, bool host_hooks) { return host_hooks ? marker != 0 : marker == 1; }
