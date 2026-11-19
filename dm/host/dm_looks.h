
#ifndef DM_LOOKS_H
#define DM_LOOKS_H
#include <pthread.h>
#include <stdint.h>
#include "dm_looks_core.h"

#define DM_LOOKS_FILE "Looks.swlooks"
#define DM_LOOKS_IMPORT "Looks.import-"
enum { DM_LOOKS_STATE_OFF = 0, DM_LOOKS_STATE_OK = 1, DM_LOOKS_STATE_BROKEN = 2, DM_LOOKS_STATE_NEWER = 3 };

extern pthread_mutex_t g_looks_mu;
extern dm_looks_store g_looks;
extern int g_looks_state;

void dm_looks_load(void);
int dm_looks_start_anew(void);
int dm_looks_latch(void);
int dm_looks_locked(void);



void dm_looks_request_apply(uint32_t unit_id, dm_look look, int dye);
void dm_looks_request_preview(uint32_t unit_id, dm_look look, int dye);
int dm_looks_unlocked(void);
int dm_looks_state(void);

const char *dm_looks_name(dm_look l);
int dm_looks_of_client(uint64_t item, dm_look *out);
int dm_looks_page_item(uint64_t item, int *group, int *w, int *h);
int dm_looks_page_offer(uint64_t item, int pclass, dm_look *out, int max);
int dm_looks_page_price(uint64_t item, dm_look l);
int dm_looks_page_state(uint64_t item, dm_look *cur, dm_look *shown);
int dm_looks_page_busy(uint64_t item);
const char *dm_looks_icon(uint64_t item, dm_look l);
int dm_looks_page_dye(uint64_t item, int *cur, int *shown);
int dm_looks_page_where(uint64_t item, dm_look l);
int dm_looks_page_gems(dm_dye_offer *out, int max);
int dm_looks_dyes_on(void);
int dm_looks_dye_gem(int dye);
const char *dm_looks_gem_icon(int gem);
const char *dm_looks_gem_name(int gem);
#endif
