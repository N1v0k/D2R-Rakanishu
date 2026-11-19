





#pragma once
#include <stddef.h>
#include <stdint.h>

#define DM_RESUME_VERSION 1
#define DM_RESUME_NAME_MAX 16
#define DM_RESUME_ARRIVE_TICKS 125

typedef struct {
    char who[DM_RESUME_NAME_MAX];
    int level, x, y, difficulty;
    uint64_t map_fp;
    uint64_t save_fp;
} dm_resume_pos;


size_t dm_resume_format(const dm_resume_pos *p, char *out, size_t cap);
int dm_resume_parse(const char *text, dm_resume_pos *out);



int dm_resume_level_allowed(int level);
int dm_resume_is_town(int level);


#define DM_RESUME_FP_START 1469598103934665603ull
uint64_t dm_resume_fp_room(uint64_t fp, int32_t x, int32_t y, int32_t w, int32_t h);


typedef enum {
    DM_RESUME_OFFER, DM_RESUME_SWITCH_OFF, DM_RESUME_NO_MARK, DM_RESUME_BAD_FILE, DM_RESUME_OTHER_CHAR,
    DM_RESUME_OTHER_SAVE, DM_RESUME_OTHER_DIFFICULTY, DM_RESUME_LEVEL,
} dm_resume_why;
dm_resume_why dm_resume_decide(int switch_on, int marked, const dm_resume_pos *p, const char *who, uint64_t loaded_save_fp, int difficulty);
const char *dm_resume_why_name(dm_resume_why w);



uint64_t dm_resume_save_fp(const char *dir, const char *who);
int dm_resume_sidecar_path(const char *dir, const char *who, char *out, size_t cap);
int dm_resume_sidecar_write(const char *dir, const dm_resume_pos *p);
int dm_resume_sidecar_read(const char *dir, const char *who, dm_resume_pos *out);

int dm_resume_committed(const char *dir, const char *who);

int dm_resume_mark_write(const char *path, const char *who);
int dm_resume_mark_read(const char *path, char who[DM_RESUME_NAME_MAX]);
int dm_resume_mark_delete(const char *path);



dm_resume_why dm_resume_join(const char *mark_path, const char *save_dir, const char *who, int switch_on, int difficulty, dm_resume_pos *out, int *removal_failed);





typedef struct {
    unsigned gen;
    int left;
    int saving;
    unsigned saving_gen;
    unsigned side_gen;
} dm_resume_marks;
void dm_resume_left(dm_resume_marks *m);
int dm_resume_back(dm_resume_marks *m);
void dm_resume_dropped(dm_resume_marks *m);
unsigned dm_resume_leave_begin(dm_resume_marks *m);
int dm_resume_sidecar_due(const dm_resume_marks *m);
void dm_resume_sidecar_done(dm_resume_marks *m);
int dm_resume_leave_end(dm_resume_marks *m, unsigned gen);


typedef enum { DM_RESUME_IDLE, DM_RESUME_ASKING, DM_RESUME_GO, DM_RESUME_ARRIVING, DM_RESUME_DONE } dm_resume_stage;
typedef enum { DM_RESUME_ACT_NONE, DM_RESUME_ACT_WARP, DM_RESUME_ACT_PLACE, DM_RESUME_ACT_GIVE_UP } dm_resume_act;
typedef struct { dm_resume_stage st; dm_resume_pos pos; uint32_t warp_tick; } dm_resume_run;
void dm_resume_ask(dm_resume_run *r, const dm_resume_pos *p);
int dm_resume_choose(dm_resume_run *r, int here);


dm_resume_act dm_resume_step(dm_resume_run *r, uint32_t tick, int server_level, int client_level);
