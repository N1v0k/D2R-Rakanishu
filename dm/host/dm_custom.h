

#pragma once
#include <stddef.h>









int dm_custom_get(const char *key);
int dm_custom_lang_explicit(void);
int dm_custom_voice_explicit(void);


int dm_fps_setting(void);
int dm_custom_status(void);
int dm_custom_splash_on(void);
void dm_custom_legacy(int fps_cap, int status);



void dm_custom_legacy_shoulder(int left, int right);


void dm_custom_has_triggers(int on);

int dm_custom_tech_take(void);
void dm_custom_res_reset(void);
int dm_custom_path(const char *name, char *p, size_t n);

void dm_custom_init(const char *dir);



void dm_custom_on_uimsg(const char *cat, const char *act, const char *arg);
void dm_custom_ui_pump(unsigned long long root);
