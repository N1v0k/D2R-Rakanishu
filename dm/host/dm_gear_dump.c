

#define _POSIX_C_SOURCE 200809L
#include "dm_gear_dump.h"
#include "dm_itemcmp_core.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>


#define DUMP_PREFIX "gear-problem-"

static uint64_t dg_mix(uint64_t h) { h ^= h >> 33; h *= 0xff51afd7ed558ccdULL; h ^= h >> 33; h *= 0xc4ceb9fe1a85ec53ULL; h ^= h >> 33; return h; }
static uint64_t dg_bytes(uint64_t h, const void *p, size_t n) {
    const unsigned char *b = p;
    for (size_t i = 0; i < n; i++) h = (h ^ b[i]) * 1099511628211ull;
    return h;
}
static uint64_t dg_int(uint64_t h, uint64_t v) { return dg_bytes(h, &v, sizeof v); }

uint64_t dm_gdump_sim_hash(const dm_sim_data *d) {
    if (!d) return 0;
    uint64_t h = 1469598103934665603ull;
    h = dg_bytes(h, d->sha, sizeof d->sha); h = dg_bytes(h, d->set_name, sizeof d->set_name);
    h = dg_int(h, (uint64_t)d->ncls); h = dg_int(h, (uint64_t)d->nstd); h = dg_int(h, (uint64_t)d->nskill); h = dg_int(h, (uint64_t)d->nmon);
    return dg_mix(h) ? dg_mix(h) : 1;
}
uint64_t dm_gdump_gear_hash(const dm_gear_tables *t) {
    if (!t || !t->ok) return 0;
    uint64_t h = 1469598103934665603ull;
    h = dg_bytes(h, &t->nop, sizeof t->nop); h = dg_bytes(h, t->op, sizeof *t->op * (size_t)t->nop);
    h = dg_bytes(h, t->shift, sizeof t->shift);
    h = dg_bytes(h, &t->nset, sizeof t->nset); h = dg_bytes(h, t->pieces, sizeof *t->pieces * (size_t)t->nset);
    h = dg_bytes(h, &t->nitem, sizeof t->nitem); h = dg_bytes(h, t->item, sizeof *t->item * (size_t)t->nitem);
    h = dg_bytes(h, &t->nsetb, sizeof t->nsetb); h = dg_bytes(h, t->setb, sizeof *t->setb * (size_t)t->nsetb);
    h = dg_bytes(h, &t->nitemb, sizeof t->nitemb); h = dg_bytes(h, t->itemb, sizeof *t->itemb * (size_t)t->nitemb);
    h = dg_mix(h);
    return h ? h : 1;
}

static void dg_sizes(uint32_t *sz) {
    sz[DG_SZ_PROBLEM] = (uint32_t)sizeof(dm_gear_problem);      sz[DG_SZ_CAND] = (uint32_t)sizeof(dm_gear_cand);
    sz[DG_SZ_WEAPON] = (uint32_t)sizeof(dm_gear_weapon);        sz[DG_SZ_GOAL] = (uint32_t)sizeof(dm_gear_goal);
    sz[DG_SZ_PICK] = (uint32_t)sizeof(dm_gear_pick);            sz[DG_SZ_BASE] = (uint32_t)sizeof(dm_gear_base);
    sz[DG_SZ_CHAR] = (uint32_t)sizeof(dm_gear_char);            sz[DG_SZ_ITEM] = (uint32_t)sizeof(dm_gear_item);
    sz[DG_SZ_LIST] = (uint32_t)sizeof(dm_gear_list);            sz[DG_SZ_SIMCHAR] = (uint32_t)sizeof(dm_sim_char);
    sz[DG_SZ_SIMWEAPON] = (uint32_t)sizeof(dm_sim_weapon);      sz[DG_SZ_ICCB] = (uint32_t)sizeof(dm_ic_cb);
}
static void dg_limits(uint32_t *lim) {
    lim[DG_LIM_MAXSKILL] = DM_SIM_MAXSKILL;     lim[DG_LIM_MAXGRANT] = DM_SIM_MAXGRANT;     lim[DG_LIM_MAXBAR] = DM_SIM_MAXBAR;
    lim[DG_LIM_MAXCAND] = DM_GEAR_MAXCAND;      lim[DG_LIM_NSTAT] = DM_GEAR_NSTAT;          lim[DG_LIM_MAXV] = DM_GEAR_MAXV;
    lim[DG_LIM_GREENV] = DM_GEAR_GREENV;        lim[DG_LIM_MAXITEMS] = DM_GEAR_MAXITEMS;    lim[DG_LIM_PLACES] = DM_GEAR_P_N;
    lim[DG_LIM_GRIDW] = DM_GEAR_GRID_W;         lim[DG_LIM_GRIDH] = DM_GEAR_GRID_H;         lim[DG_LIM_EL] = DM_EL_N;
}
static void dg_path(char *out, size_t cap, const char *dir, uint64_t rev, const char *ext) {
    snprintf(out, cap, "%s/%s%016llx.%s", dir, DUMP_PREFIX, (unsigned long long)rev, ext);
}

static int dg_refuse(const char *what) { fprintf(stderr, "gear dump: %s\n", what); return -1; }

#ifndef DM_PLAYER_BUILD

static void dg_prune(const char *dir) {
    DIR *d = opendir(dir);
    if (!d) return;
    long long mt[DM_GDUMP_KEEP + 64]; uint64_t rev[DM_GDUMP_KEEP + 64]; int n = 0;
    const size_t plen = strlen(DUMP_PREFIX);
    struct dirent *e;
    while ((e = readdir(d))) {
        const char *nm = e->d_name;
        if (strncmp(nm, DUMP_PREFIX, plen) || !strstr(nm, ".bin")) continue;
        char p[1200]; snprintf(p, sizeof p, "%s/%s", dir, nm);
        struct stat st;
        if (stat(p, &st)) continue;
        unsigned long long r = 0;
        if (sscanf(nm + plen, "%016llx", &r) != 1) continue;
        if (n >= (int)(sizeof mt / sizeof *mt)) continue;
        long long w = (long long)st.st_mtim.tv_sec * 1000000000ll + st.st_mtim.tv_nsec;
        int i = n++;
        for (; i > 0 && mt[i - 1] > w; i--) { mt[i] = mt[i - 1]; rev[i] = rev[i - 1]; }
        mt[i] = w; rev[i] = r;
    }
    closedir(d);
    for (int i = 0; i + DM_GDUMP_KEEP < n; i++) {
        char p[1200];
        dg_path(p, sizeof p, dir, rev[i], "bin"); unlink(p);
        dg_path(p, sizeof p, dir, rev[i], "txt"); unlink(p);
    }
}

int dm_gdump_write(const char *dir, uint64_t rev, const dm_gear_problem *p, const dm_gear_goal *goal, int threads, const dm_gear_pick *seed) {
    if (!dir || !*dir || !p || !goal || !p->ready || p->ncand < 0 || p->ncand > DM_GEAR_MAXCAND || p->nweapon < 1 || !p->cand || !p->weapon) return -1;
    dm_gdump_head h; memset(&h, 0, sizeof h);
    memcpy(h.magic, DM_GDUMP_MAGIC, sizeof h.magic);
    h.format = DM_GDUMP_FORMAT; h.endian = 0x01020304u;
    dg_sizes(h.sz); dg_limits(h.lim);
    h.rev = rev; h.sim_hash = dm_gdump_sim_hash(p->d); h.gear_hash = dm_gdump_gear_hash(p->t);
    h.ncand = p->ncand; h.nweapon = p->nweapon; h.threads = threads; h.charms_fixed = p->charms_fixed; h.has_seed = seed != NULL;
    size_t bytes = sizeof h + sizeof *p + sizeof(dm_gear_cand) * (size_t)p->ncand + sizeof(dm_gear_weapon) * (size_t)p->nweapon + sizeof *goal + sizeof(dm_gear_pick);
    h.bytes = (uint64_t)bytes;

    dm_gear_problem q = *p;
    q.d = NULL; q.t = NULL; q.cand = NULL; q.weapon = NULL;

    char path[1200]; dg_path(path, sizeof path, dir, rev, "bin");
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    int ok = fwrite(&h, sizeof h, 1, f) == 1 && fwrite(&q, sizeof q, 1, f) == 1 &&
             (!h.ncand || fwrite(p->cand, sizeof *p->cand, (size_t)h.ncand, f) == (size_t)h.ncand) &&
             fwrite(p->weapon, sizeof *p->weapon, (size_t)h.nweapon, f) == (size_t)h.nweapon &&
             fwrite(goal, sizeof *goal, 1, f) == 1 && fwrite(seed ? seed : &(dm_gear_pick){ 0 }, sizeof(dm_gear_pick), 1, f) == 1;
    if (fclose(f)) ok = 0;
    if (!ok) { unlink(path); return -1; }
    dg_prune(dir);
    return 0;
}

void dm_gdump_result(const char *dir, uint64_t rev, const char *line) {
    if (!dir || !*dir) return;
    char p[1200]; dg_path(p, sizeof p, dir, rev, "bin");
    if (access(p, F_OK)) return;
    dg_path(p, sizeof p, dir, rev, "txt");
    FILE *f = fopen(p, "a");
    if (!f) return;
    fputs(line, f);
    size_t n = strlen(line);
    if (!n || line[n - 1] != '\n') fputc('\n', f);
    fclose(f);
}
#endif

int dm_gdump_read(const char *path, const dm_sim_data *d, const dm_gear_tables *t, dm_gdump *out) {
    if (!path || !d || !t || !out) return -1;
    memset(out, 0, sizeof *out);
    FILE *f = fopen(path, "rb");
    if (!f) return dg_refuse("cannot open the dump");
    dm_gdump_head h;
    if (fread(&h, sizeof h, 1, f) != 1) { fclose(f); return dg_refuse("too short for a header"); }
    uint32_t sz[DG_SZ_N], lim[DG_LIM_N]; dg_sizes(sz); dg_limits(lim);
    if (memcmp(h.magic, DM_GDUMP_MAGIC, sizeof h.magic)) { fclose(f); return dg_refuse("not a gear problem dump"); }
    if (h.format != DM_GDUMP_FORMAT) { fclose(f); return dg_refuse("dump format this build does not read"); }
    if (h.endian != 0x01020304u) { fclose(f); return dg_refuse("written on a machine of the other byte order"); }
    if (memcmp(h.sz, sz, sizeof sz)) { fclose(f); return dg_refuse("struct sizes differ from this build"); }
    if (memcmp(h.lim, lim, sizeof lim)) { fclose(f); return dg_refuse("array bounds differ from this build"); }
    if (h.ncand < 0 || h.ncand > DM_GEAR_MAXCAND || h.nweapon < 1) { fclose(f); return dg_refuse("candidate and weapon counts out of range"); }
    uint64_t want = sizeof h + sizeof(dm_gear_problem) + sizeof(dm_gear_cand) * (uint64_t)h.ncand + sizeof(dm_gear_weapon) * (uint64_t)h.nweapon
                    + sizeof(dm_gear_goal) + sizeof(dm_gear_pick);
    if (h.bytes != want) { fclose(f); return dg_refuse("header size does not match its arrays"); }
    if (fseek(f, 0, SEEK_END) || (uint64_t)ftell(f) != want) { fclose(f); return dg_refuse("the file is shorter than the header promises"); }
    if (h.sim_hash != dm_gdump_sim_hash(d)) { fclose(f); return dg_refuse("the sim tables are not the ones of this dump"); }
    if (h.gear_hash != dm_gdump_gear_hash(t)) { fclose(f); return dg_refuse("the gear tables are not the ones of this dump"); }

    dm_gear_problem p; dm_gear_goal goal; dm_gear_pick seed;
    rewind(f);
    if (fseek(f, (long)sizeof h, SEEK_SET) || fread(&p, sizeof p, 1, f) != 1) { fclose(f); return dg_refuse("cannot read the problem"); }
    dm_gear_cand *cand = h.ncand ? malloc(sizeof *cand * (size_t)h.ncand) : calloc(1, 1);
    dm_gear_weapon *weapon = malloc(sizeof *weapon * (size_t)h.nweapon);
    int ok = cand && weapon && fread(cand, sizeof *cand, (size_t)h.ncand, f) == (size_t)h.ncand && fread(weapon, sizeof *weapon, (size_t)h.nweapon, f) == (size_t)h.nweapon
             && fread(&goal, sizeof goal, 1, f) == 1 && fread(&seed, sizeof seed, 1, f) == 1;
    fclose(f);
    if (!ok) { free(cand); free(weapon); return dg_refuse("cannot read the arrays"); }
    if (!p.ready || p.ncand != h.ncand || p.nweapon != h.nweapon) { free(cand); free(weapon); return dg_refuse("the problem is not a ready search input"); }
    p.d = d; p.t = t; p.cand = cand; p.weapon = weapon;
    out->head = h; out->p = p; out->cand = cand; out->weapon = weapon; out->goal = goal; out->seed = seed;
    return 0;
}

void dm_gdump_free(dm_gdump *g) {
    if (!g) return;
    free(g->cand); free(g->weapon); memset(g, 0, sizeof *g);
}
