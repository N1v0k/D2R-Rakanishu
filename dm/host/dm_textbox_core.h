#pragma once


#include <stddef.h>
int dm_textbox_load(const char *path);
void dm_textbox_reset(void);
int dm_textbox_count(void);


int dm_textbox_rewrite(const char *s, size_t n, char **out, size_t *on);


int dm_textbox_ensure_loaded(void);
