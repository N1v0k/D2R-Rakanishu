
#pragma once
#include <stdatomic.h>
#include <stdint.h>
#include <stddef.h>
#include "dm_plural.h"
extern _Atomic int g_dm_lang;
int dm_lang_index(void);
int dm_lang_zh(void);
enum { DM_LG_EN, DM_LG_ZH, DM_LG_ES, DM_LG_MX, DM_LG_RU, DM_LG_KO, DM_LG_DE, DM_LG_PT };
int dm_lang_group(void);
int dm_lang_group_of(int lang);
const char *dm_ui_group(const char *key, int group);
const char *dm_untag(const char *s);
int dm_lang_from_tag(const char *tag);
int dm_sys_lang(void);
const char *dm_str(uint8_t *main_base, const char *key);
const char *dm_ui(const char *key);
const char *dm_strn(uint8_t *main_base, const char *key, long long n, char *buf, size_t size);
