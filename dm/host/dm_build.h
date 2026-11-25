


#pragma once
#include <stdint.h>



enum { DM_BUILD_HEAD = 17, DM_BUILD_MAX_SKILLS = 40, DM_BUILD_INTS = DM_BUILD_HEAD + 3 * DM_BUILD_MAX_SKILLS, DM_BUILD_NAME = 16, DM_BUILD_CARD_TEXT = 256, DM_BUILD_CARD_PARTS = 7, DM_BUILD_KEYS = 8 };




void dm_build_pump(uint8_t *b, uint64_t pl);


int dm_build_state(int *out, int n, char *name, int name_cap, int *name_len);


int dm_build_skills(const int *ids, int n);


void dm_build_target(int session, int page, int row, int col, int show, int guide);


int dm_build_open_take(void);







void dm_build_card(int session, const char *text, int len, int to, int at, int state);


void dm_build_stat(int session, int stat, int show);



void dm_build_keys(int session, const int *ids, int n);

int dm_build_hints(void);


int dm_build_session(void);
void dm_build_target_get(int *session, int *page, int *row, int *col, int *show, int *guide);

int dm_build_card_get(char out[DM_BUILD_CARD_PARTS][DM_BUILD_CARD_TEXT], int *to, int *at, int *state);
void dm_build_stat_get(int *session, int *stat, int *show);
int dm_build_keys_get(int *ids, int max);
int dm_build_free_stats(void);
void dm_build_open_request(void);
void dm_build_on_uimsg(const char *cat, const char *act, const char *arg);
void dm_build_ui_pump(uint64_t root);

int dm_skill_reason(uint8_t *b, uint64_t pl, int level, int skill, int base);
