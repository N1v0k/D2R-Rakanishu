














#pragma once
#include <stddef.h>
#include "game_addrs.h"
#include <stdint.h>

enum { DM_TIPX_HEAD = 1, DM_TIPX_WARN = 2 };





enum { DM_TIPX_MAXL = 128, DM_TIPX_LINEB = 240, DM_TIPX_WRAP = 8 };
typedef struct { const char *s; int flags; } dm_tipx_line;
typedef struct {
    int max_lines;
    int max_chars;
    int name_lines;
    const char *hint;
    const char *props;
} dm_tipx_cfg;
int dm_tipx_text_lines(const char *s);
size_t dm_tipx_utf8_cut(const char *s, size_t n, int max_cp);


int dm_tipx_compose(const char *orig, const dm_tipx_line *ln, int n, const dm_tipx_cfg *c, int page, char *out, size_t cap, size_t *len);



int dm_tipx_top_line(const char *text, int i, size_t *start, size_t *len);




int dm_tipx_dot_line(const char *text, int vendor);


size_t dm_tipx_insert_under(const char *text, int top, const char *line, char *out, size_t cap);



int dm_tipx_text_object_ok(const uint8_t *o, uint64_t frame);
int dm_tipx_side_boxes(uint64_t x29, const uint64_t *regs);


typedef struct { size_t at, len; } dm_tipx_span;






typedef struct { const char *line, *text, *brief; } dm_tipx_ann;
int dm_tipx_width(const char *s);


int dm_tipx_join(const char *const *part, int n, const char *sep, int max_chars, char *out, size_t cap);
enum { DM_TIPX_ANN_PLACED = 1, DM_TIPX_ANN_NONE = 0, DM_TIPX_ANN_MANY = -1, DM_TIPX_ANN_WIDE = -2, DM_TIPX_ANN_COLOUR = -3 };
size_t dm_tipx_annotate(const char *orig, const dm_tipx_ann *a, int n, int max_chars, char *out, size_t cap, dm_tipx_span *span, int max_span, int *nspan, int *placed);


#define DM_TIPX_COLOUR_TAG "\xee\x81\xbe" ":" "\xee\x81\xbe" ":"
const char *dm_tipx_render_text(char *text, size_t own_bytes, int colour_ready, char *out, size_t capacity);
int dm_tipx_colour_index(char code);
size_t dm_tipx_colour_text(const char *text, size_t own_bytes, char *out, size_t capacity);

const char *dm_tipx_render_spans(char *text, const dm_tipx_span *span, int nspan, int colour_ready, char *out, size_t capacity);
size_t dm_tipx_colour_spans(const char *text, const dm_tipx_span *span, int nspan, char *out, size_t capacity);
int dm_tipx_colour_rgb(const char *code, size_t offset, const uint32_t colours[4][3], uint64_t *regs);





typedef struct { int tx, ty, tw, th, w, h, centre, above, ox, oy, pad, screen_w, screen_h; float scale; } dm_tipx_place_in;
void dm_tipx_place(const dm_tipx_place_in *p, int *left, int *top);
