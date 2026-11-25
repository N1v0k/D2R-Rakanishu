




#ifndef DM_ITEMJOB_H
#define DM_ITEMJOB_H

enum { DM_ITEMJOB_NONE, DM_ITEMJOB_RECIPE, DM_ITEMJOB_RUNEWORD, DM_ITEMJOB_EQUIP, DM_ITEMJOB_N };



int dm_itemjob_acquire(int kind);

int dm_itemjob_release(int kind);
int dm_itemjob_owner(void);

unsigned dm_itemjob_ends(void);






enum { DM_ITEMJOB_HAND_EMPTY, DM_ITEMJOB_HAND_OWN, DM_ITEMJOB_HAND_OTHER };
typedef struct {
    int hand;
    int tracked;
    int placed;
    int pending;
    int at_source;
} dm_itemjob_seen;
typedef struct { int left, retry, wait, backs; } dm_itemjob_settle;
enum {
    DM_ITEMJOB_WAIT,
    DM_ITEMJOB_PUT_BACK,
    DM_ITEMJOB_SETTLED,
    DM_ITEMJOB_IN_HAND,
    DM_ITEMJOB_OTHER,
    DM_ITEMJOB_UNPLACED
};

void dm_itemjob_settle_begin(dm_itemjob_settle *s, int limit, int retry);
int dm_itemjob_settle_step(dm_itemjob_settle *s, const dm_itemjob_seen *v);
#endif
