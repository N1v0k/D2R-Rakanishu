




#include "dm_book.h"
#include "dm_feats.h"
#include "dm_fightlog.h"
#include "dm_sim_ui.h"
#include "dm.h"
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

const char *dm_save_workdir(void);
uint64_t dm_fs_commit_count(void);
int dm_kills_board(const char *who, dm_ft_board_row *out, int max);




typedef struct { int8_t kind; int16_t id; const char *name; } board_row;
static const board_row k_board[DM_FT_BOARD_ROWS] = {
    { 0, -1, NULL }, { 1, 6, "The Countess" }, { 0, -1, NULL }, { 1, 39, "The Cow King" }, { 1, 0, "Bishibosh" }, { 1, 40, "Corpsefire" },
    { 1, 20, "The Smith" }, { 1, 5, "Griswold" }, { 1, 3, "Rakanishu" }, { 1, 2, "Coldcrow" }, { 1, 4, "Treehead WoodFist" }, { 0, -1, NULL },
    { 0, -1, NULL }, { 0, -1, NULL }, { 1, 19, "Ancient Kaa the Soulless" }, { 1, 15, "Coldworm the Burrower" }, { 1, 12, "Fangskin" },
    { 1, 11, "Bloodwitch the Wild" }, { 1, 13, "Beetleburst" }, { 1, 17, "Dark Elder" }, { 1, 16, "Fire Eye" }, { 0, -1, NULL }, { 2, 0, NULL },
    { 1, 28, "Bremm Sparkfist" }, { 1, 30, "Wyand Voidfinger" }, { 1, 31, "Maffer Dragonhand" }, { 1, 21, "Web Mage the Burning" },
    { 1, 22, "Witch Doctor Endugu" }, { 1, 23, "Stormtree" }, { 1, 24, "Sarina the Battlemaid" }, { 1, 25, "Icehawk Riftwing" },
    { 1, 32, "Winged Death" }, { 1, 33, "The Tormentor" }, { 1, 34, "Taintbreeder" }, { 1, 35, "Riftwraith the Cannibal" }, { 0, -1, NULL },
    { 1, 38, "Grand Vizier of Chaos" }, { 1, 37, "Lord De Seis" }, { 1, 36, "Infector of Souls" }, { 0, -1, NULL }, { 1, 41, "The Feature Creep" },
    { 0, -1, NULL }, { 1, 60, "Nihlathak" }, { 1, 42, "Siege Boss" }, { 1, 49, "Megaflow Rectifier" }, { 1, 52, "Pindleskin" },
    { 1, 48, "Dac Farren" }, { 1, 51, "Threash Socket" }, { 1, 53, "Snapchip Shatter" }, { 1, 59, "Frozenstein" }, { 2, 1, NULL },
    { 1, 61, "Baal Subject 1" }, { 1, 62, "Baal Subject 2" }, { 1, 63, "Baal Subject 3" }, { 1, 64, "Baal Subject 4" }, { 1, 65, "Baal Subject 5" },
    { 1, 47, "Bonesaw Breaker" }, { 1, 46, "Axe Dweller" }, { 1, 50, "Eyeback Unleashed" }, { 1, 56, "Sharp Tooth Sayer" },
    { 1, 55, "Vinvear Molech" }, { 1, 57, "Magma Torquer" }, { 1, 54, "Anodized Elite" }, { 1, 58, "Blaze Ripper" }
};
static const int16_t k_board_group[2][3] = { { 26, 27, 29 }, { 43, 44, 45 } };

static int board_seen(const char *who, dm_bk_seen *out, int max) {
    dm_ft_board_row rows[DM_FT_BOARD_ROWS];
    if (dm_kills_board(who, rows, DM_FT_BOARD_ROWS) != DM_FT_BOARD_ROWS) return -1;
    int n = 0;
    for (int r = 0; r < DM_FT_BOARD_ROWS; r++) for (int d = 0; d < DM_BK_DIFFS; d++) {
        uint32_t c = rows[r].n[d] > 0 ? (uint32_t)rows[r].n[d] : 0; const board_row *k = &k_board[r];
        if (!c) continue;
        if (k->kind == 2) { for (int m = 0; m < 3 && n < max; m++) out[n++] = (dm_bk_seen){ -1, k_board_group[k->id][m], NULL, d, c }; }
        else if (n < max) out[n++] = k->kind == 0 ? (dm_bk_seen){ rows[r].cls, -1, NULL, d, c } : (dm_bk_seen){ -1, k->id, k->name, d, c };
    }
    return n;
}

enum { QUEUE = 512, BOARD_SEEN = 3 * 64 };
static pthread_mutex_t g_mu = PTHREAD_MUTEX_INITIALIZER;
static dm_bk g_bk;
static char g_who[64];
static int g_mode = DM_BK_MODE_OWN, g_due;
static uint32_t g_saved;
static struct { uint16_t cls, su; uint8_t diff; } g_q[QUEUE];
static int g_nq;
static uint64_t g_wrote;

static int path_of(char *out, size_t cap, const char *root, const char *who, const char *suffix) {
    return root && who && who[0] && !strchr(who, '/') && snprintf(out, cap, "%s/%s/%s.sw_book%s", root, who, who, suffix) < (int)cap;
}
static const char *const k_bad[] = { "", "size", "magic", "version", "checksum", "record count", "owner", "record", "", "owner" };



static void load(const char *root, const char *who) {
    static uint8_t img[DM_BK_IMAGE_MAX + 1];
    char p[1200] = "", q[1220]; size_t n = 0; int exists = 0, failed = 0, r = 0;
    if (path_of(p, sizeof p, root, who, "")) {
        errno = 0;
        FILE *f = fopen(p, "rb");
        if (f) { exists = 1; n = fread(img, 1, sizeof img, f); if (ferror(f)) failed = errno ? errno : EIO; fclose(f); }
        else if (errno != ENOENT) failed = errno;
    }
    dm_bk *fresh = calloc(1, sizeof *fresh);
    if (!fresh) return;
    if (!failed && exists) r = n ? dm_bk_parse(img, n, who, fresh) : DM_BK_BAD_SIZE;
    pthread_mutex_lock(&g_mu);
    snprintf(g_who, sizeof g_who, "%s", who ? who : "");
    g_nq = 0;
    g_mode = failed ? DM_BK_MODE_UNREAD : r == DM_BK_NEWER ? DM_BK_MODE_NEWER : DM_BK_MODE_OWN;
    if (g_mode == DM_BK_MODE_OWN && r) memset(fresh, 0, sizeof *fresh);
    g_bk = *fresh;
    g_due = g_mode == DM_BK_MODE_OWN && !(g_bk.flags & DM_BK_F_BACKFILLED);
    g_saved = g_bk.gen;
    int kinds = g_bk.nrec;
    pthread_mutex_unlock(&g_mu);
    free(fresh);
    if (failed) LOG("ASSUME book.read: the monster book file of this character could not be read (%s); its kills rest and the file stays as it is", strerror(failed));
    else if (r == DM_BK_NEWER) LOG("ASSUME book.version: the monster book of this character was written by a newer version of the app; its kills rest and the file stays as it is");
    else if (r) {
        snprintf(q, sizeof q, "%s.bad", p);
        int moved = !rename(p, q);
        LOG("ERROR book.file_rejected: the monster book of this character failed its %s check; %s, it starts over from the character's records", k_bad[r], moved ? "set aside as .bad" : "could not be set aside");
    } else LOG("book: %s, %d kinds%s", exists ? "loaded" : "no file yet", kinds, g_due ? ", back-fill due" : "");
}


static int sources(const char *who, dm_bk_seen *board, int *nboard, dm_bk_seen **fights, int *nfights) {
    dm_fl_state *s = dm_fl_core();
    *nboard = board_seen(who, board, BOARD_SEEN);
    if (*nboard < 0 || !dm_fl_owned_by(s, who)) return -1;
    int count = dm_fl_count(s), n = 0;
    dm_bk_seen *out = calloc((size_t)(count > 0 ? count : 1) * DM_FL_GROUPS, sizeof *out);
    dm_fl_fight *f = malloc(sizeof *f);
    if (!out || !f) { free(out); free(f); return -1; }
    for (int i = 0; i < count; i++) {
        if (!dm_fl_get(s, i, f)) continue;
        for (int g = 0; g < f->h.ngroups && g < DM_FL_GROUPS; g++) {
            const dm_fl_group *x = &f->g[g];
            if ((x->flags & DM_FL_G_OTHERS) || !x->kills) continue;
            out[n++] = (dm_bk_seen){ x->cls, x->su == 0xffff ? -1 : x->su, NULL, f->h.difficulty, x->kills };
        }
    }
    free(f);
    *fights = out; *nfights = n;
    return 0;
}






enum { GIVE_UP = 250 };
typedef struct { const char *who; int give_up, server, unknown, skipped, backfilled, dropped, kinds, nboard, nfights; } settling;
static int settle_read(const dm_sim_data *d, void *ctx) {
    settling *x = ctx;
    dm_bk_seen board[BOARD_SEEN], *fights = NULL;
    pthread_mutex_lock(&g_mu);
    int due = g_due && !strcmp(g_who, x->who);
    pthread_mutex_unlock(&g_mu);
    int have = due && x->server && !sources(x->who, board, &x->nboard, &fights, &x->nfights);
    pthread_mutex_lock(&g_mu);
    if (!strcmp(g_who, x->who) && g_mode == DM_BK_MODE_OWN) {
        if (have && g_due) { dm_bk_backfill(&g_bk, d, board, x->nboard, fights, x->nfights, &x->skipped); g_due = 0; x->backfilled = 1; }
        else if (g_due && x->give_up) { g_bk.flags |= DM_BK_F_BACKFILLED; g_bk.gen++; g_due = 0; x->dropped = 1; }
        for (int i = 0; !g_due && i < g_nq; i++) {
            dm_bk_seen s = { g_q[i].cls, g_q[i].su == 0xffff ? -1 : g_q[i].su, NULL, g_q[i].diff, 1 };
            uint8_t type; uint16_t idx;
            if (dm_bk_kind_of(d, &s, &type, &idx) || dm_bk_add(&g_bk, type, idx, s.diff, 1)) x->unknown++;
#ifndef DM_PLAYER_BUILD
            else LOG("[book] {\"kill\":\"%c%u\",\"diff\":%d,\"count\":%u}", type, idx, s.diff, dm_bk_count(&g_bk, type, idx, s.diff));
#endif
        }
        if (!g_due) g_nq = 0;
    }
    x->kinds = g_bk.nrec;
    pthread_mutex_unlock(&g_mu);
    free(fights);
    return 0;
}
static void settle(const char *who, int wait, int give_up, int server) {
    dm_sim_snapshot *snap = malloc(sizeof *snap);
    if (!snap) return;
    int set = dm_sim_snapshot_copy(snap) && !strcmp(snap->name, who) ? snap->set : -1;
    free(snap);
    if (set < 0) return;
    static int unknown_logged;
    settling x = { .who = who, .give_up = give_up, .server = server };
    if ((wait ? dm_sim_read : dm_sim_try_read)(set ? "rotw" : "lod", settle_read, &x) < 0) return;
    if (x.unknown && !unknown_logged++) LOG("ASSUME book.kills: %d kills named no kind of the loaded tables (or the book is full); they are not counted", x.unknown);
    if (x.backfilled) LOG("book: back-filled from %d boss ranking and %d fight records, %d without a kind; %d kinds", x.nboard, x.nfights, x.skipped, x.kinds);
    if (x.dropped) LOG("ASSUME book.backfill: the boss ranking or the last fights of this character were not readable in time; the book counts from now on without them");
}

void dm_book_tick(uint64_t G, uint64_t player, uint8_t *main_base) {
    (void)main_base;
    if (!G || !player) return;
    static uint64_t game; static uint32_t last_t, start; static char who[64]; static int lost_logged;
    uint32_t t = *(volatile uint32_t *)(uintptr_t)(G + 0x170);
    if (G != game || t < last_t) {
        game = G; start = t;
        uint64_t pd = *(volatile uint64_t *)(uintptr_t)(player + 0x10); memset(who, 0, sizeof who);
        if (pd) memcpy(who, (const void *)(uintptr_t)pd, sizeof who - 1);
        pthread_mutex_lock(&g_mu); int same = !strcmp(g_who, who) && g_mode != DM_BK_MODE_UNREAD; pthread_mutex_unlock(&g_mu);
        if (!same && who[0]) load(dm_save_workdir(), who);
    }
    last_t = t;
    if (!who[0]) return;
    const dm_fl_kind *kinds; int lost, n = dm_fl_kill_kinds(dm_fl_core(), t, &kinds, &lost), diff = dm_fl_difficulty(dm_fl_core());
    if (lost && !lost_logged++) LOG("ASSUME book.members: %d kills of enemies beyond a fight's members; their kind is not known and they are not counted", lost);
    pthread_mutex_lock(&g_mu);
    int counting = g_mode == DM_BK_MODE_OWN && !strcmp(g_who, who), pending;
    for (int i = 0; counting && i < n && diff >= 0 && diff < DM_BK_DIFFS; i++) {
        if (g_nq < QUEUE) { g_q[g_nq].cls = kinds[i].cls; g_q[g_nq].su = kinds[i].su; g_q[g_nq].diff = (uint8_t)diff; g_nq++; }
    }
    pending = counting && (g_nq || g_due);
    int due = counting && g_due;
    pthread_mutex_unlock(&g_mu);


    if (pending) settle(who, due && t - start < 2, t - start >= GIVE_UP, 1);
}

int dm_book_text(char *out, size_t cap) {
    pthread_mutex_lock(&g_mu);
    int n = dm_bk_text(&g_bk, g_mode, out, cap);
    pthread_mutex_unlock(&g_mu);
    return n;
}



static void book_pre_commit(const char *wd) {
    static uint8_t buf[DM_BK_IMAGE_MAX];
    char who[64], p[1200], t[1220], dir[1200]; struct stat st; uint64_t commits = dm_fs_commit_count();
    pthread_mutex_lock(&g_mu); int waiting = g_mode == DM_BK_MODE_OWN && (g_nq || g_due); memcpy(who, g_who, sizeof who); pthread_mutex_unlock(&g_mu);
    if (waiting) settle(who, 1, 1, 0);
    pthread_mutex_lock(&g_mu);
    if (g_wrote) { if (commits < g_wrote) g_saved = g_bk.gen - 1; g_wrote = 0; }
    size_t n = wd && g_mode == DM_BK_MODE_OWN && g_who[0] && g_bk.gen != g_saved ? dm_bk_image(&g_bk, g_who, buf, sizeof buf) : 0;
    uint32_t gen = g_bk.gen; memcpy(who, g_who, sizeof who);
    pthread_mutex_unlock(&g_mu);
    if (!n || !path_of(p, sizeof p, wd, who, "") || snprintf(dir, sizeof dir, "%s/%s", wd, who) >= (int)sizeof dir) return;
    if (stat(dir, &st) || !S_ISDIR(st.st_mode)) return;
    snprintf(t, sizeof t, "%s.tmp", p);
    FILE *f = fopen(t, "wb");
    int ok = f && fwrite(buf, 1, n, f) == n;
    if (f && fclose(f)) ok = 0;
    if (ok && !rename(t, p)) { pthread_mutex_lock(&g_mu); g_saved = gen; g_wrote = commits + 1; pthread_mutex_unlock(&g_mu); }
    else { remove(t); LOG("ERROR book.write_failed: the monster book could not be written; tried again with the next save"); }
}
__attribute__((constructor)) static void book_hook_install(void) { extern void (*g_dm_pre_commit_hook[DM_PRE_COMMIT_HOOKS])(const char *); g_dm_pre_commit_hook[10] = book_pre_commit; }
