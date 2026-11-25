









#include "dm_feats.h"
#include "dm.h"
#include <dirent.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

struct dm_ft_store {
    pthread_mutex_t mu;
    dm_ft f;
    char who[64];
    int mode;
    uint32_t saved;
    uint16_t staged[DM_FT_STAGED]; int nstaged;
    uint64_t staged_count[DM_FT_COUNTERS];
    uint64_t staged_at, staged_fails;
    uint16_t notify[DM_FT_STAGED]; int nnotify;
};
static dm_ft_store g_store = { .mu = PTHREAD_MUTEX_INITIALIZER };
dm_ft_store *dm_ft_store_core(void) { return &g_store; }


static uint32_t ft_crc32(const uint8_t *p, size_t n) {
    static uint32_t table[256]; static int ready;
    if (!ready) { for (uint32_t i = 0; i < 256; i++) { uint32_t c = i; for (int k = 0; k < 8; k++) c = c & 1 ? 0xedb88320u ^ (c >> 1) : c >> 1; table[i] = c; } ready = 1; }
    uint32_t c = 0xffffffffu;
    while (n--) c = table[(c ^ *p++) & 0xff] ^ (c >> 8);
    return c ^ 0xffffffffu;
}
static void put32(uint8_t *p, uint32_t v) { memcpy(p, &v, 4); }
static uint32_t get32(const uint8_t *p) { uint32_t v; memcpy(&v, p, 4); return v; }

static int owner_ok(const uint8_t *o) {
    size_t len = strnlen((const char *)o, DM_FT_OWNER);
    if (!len || len >= DM_FT_OWNER) return 0;
    for (size_t i = len; i < DM_FT_OWNER; i++) if (o[i]) return 0;
    return 1;
}
static int rec_ok(const dm_ft_rec *r) {
    if (r->kind == DM_FT_REC_DONE) return r->value == 0;
    if (r->kind == DM_FT_REC_COUNT) return r->level == 0 && r->when == 0;
    return 0;
}
int dm_ft_parse(const uint8_t *image, size_t n, const char *who, dm_ft_rec *rec, int *nrec, uint32_t *rev) {
    *nrec = 0; *rev = 0;
    if (n < 8) return DM_FT_BAD_SIZE;
    if (get32(image) != DM_FT_MAGIC) return DM_FT_BAD_MAGIC;
    uint32_t v = get32(image + 4);
    if (v > DM_FT_VERSION) return DM_FT_NEWER;
    if (v != DM_FT_VERSION) return DM_FT_BAD_VERSION;
    if (n >= 12 && get32(image + 8) > DM_FT_REV) return DM_FT_NEWER;
    if (n < DM_FT_IMAGE_HEAD + DM_FT_OWNER) return DM_FT_BAD_SIZE;
    if (get32(image + 16) != ft_crc32(image + DM_FT_IMAGE_HEAD, n - DM_FT_IMAGE_HEAD)) return DM_FT_BAD_CRC;
    uint32_t count = get32(image + 12);
    if (count > DM_FT_FILE_RECS) return DM_FT_BAD_COUNT;
    if (n != DM_FT_IMAGE_HEAD + DM_FT_OWNER + count * sizeof *rec) return DM_FT_BAD_SIZE;
    const uint8_t *owner = image + DM_FT_IMAGE_HEAD;
    if (!owner_ok(owner)) return DM_FT_BAD_OWNER;
    memcpy(rec, owner + DM_FT_OWNER, count * sizeof *rec);
    for (uint32_t i = 0; i < count; i++) {
        if (!rec_ok(&rec[i])) return DM_FT_BAD_RECORD;
        for (uint32_t j = 0; j < i; j++) if (rec[j].kind == rec[i].kind && rec[j].id == rec[i].id) return DM_FT_BAD_RECORD;
    }
    if (strcmp((const char *)owner, who ? who : "")) return DM_FT_FOREIGN;
    *nrec = (int)count; *rev = get32(image + 8);
    return 0;
}

int dm_ft_store_owner(dm_ft_store *s, const char *who, const uint8_t *image, size_t n) {
    dm_ft_rec rec[DM_FT_FILE_RECS]; int nrec = 0; uint32_t rev = 0;
    int r = n ? dm_ft_parse(image, n, who, rec, &nrec, &rev) : 0;
    pthread_mutex_lock(&s->mu);
    dm_ft_take(&s->f, rec, r ? 0 : nrec, r ? 0 : rev);
    snprintf(s->who, sizeof s->who, "%s", who ? who : "");
    s->nstaged = s->nnotify = 0; memset(s->staged_count, 0, sizeof s->staged_count);
    s->mode = r == DM_FT_NEWER ? DM_FT_MODE_NEWER : r == DM_FT_FOREIGN ? DM_FT_MODE_FOREIGN : DM_FT_MODE_OWN;
    s->saved = s->f.gen;
    pthread_mutex_unlock(&s->mu);
    return r;
}
int dm_ft_store_owned_by(dm_ft_store *s, const char *who) {
    pthread_mutex_lock(&s->mu); int same = !strcmp(s->who, who ? who : ""); pthread_mutex_unlock(&s->mu);
    return same;
}
int dm_ft_store_mode(dm_ft_store *s) { pthread_mutex_lock(&s->mu); int m = s->mode; pthread_mutex_unlock(&s->mu); return m; }
size_t dm_ft_store_image(dm_ft_store *s, char who[64], uint8_t *buf, size_t cap, uint32_t *gen) {
    size_t n = 0;
    pthread_mutex_lock(&s->mu);
    if (s->mode == DM_FT_MODE_FOREIGN && s->f.noticed) s->mode = DM_FT_MODE_OWN;
    int may = s->mode == DM_FT_MODE_OWN && s->who[0] && s->f.gen != s->saved && s->f.nrec <= DM_FT_FILE_RECS && cap >= DM_FT_IMAGE_MAX;
    if (may) {
        memset(buf, 0, DM_FT_IMAGE_HEAD + DM_FT_OWNER);
        put32(buf, DM_FT_MAGIC); put32(buf + 4, DM_FT_VERSION); put32(buf + 8, s->f.rev); put32(buf + 12, (uint32_t)s->f.nrec);
        memcpy(buf + DM_FT_IMAGE_HEAD, s->who, strlen(s->who));
        n = DM_FT_IMAGE_HEAD + DM_FT_OWNER;
        memcpy(buf + n, s->f.rec, (size_t)s->f.nrec * sizeof *s->f.rec); n += (size_t)s->f.nrec * sizeof *s->f.rec;
        memcpy(who, s->who, 64); *gen = s->f.gen;
    }
    pthread_mutex_unlock(&s->mu);
    if (n) put32(buf + 16, ft_crc32(buf + DM_FT_IMAGE_HEAD, n - DM_FT_IMAGE_HEAD));
    return n;
}
void dm_ft_store_saved(dm_ft_store *s, uint32_t gen) { pthread_mutex_lock(&s->mu); s->saved = gen; pthread_mutex_unlock(&s->mu); }
void dm_ft_store_unsaved(dm_ft_store *s) { pthread_mutex_lock(&s->mu); s->saved = s->f.gen - 1; pthread_mutex_unlock(&s->mu); }

static int resting(const dm_ft_store *s) { return s->mode == DM_FT_MODE_NEWER || s->mode == DM_FT_MODE_UNREAD; }
static int copy_notes(const dm_ft *f, uint32_t from, dm_ft_note out[DM_FT_NOTICES]) {
    int n = 0;
    for (uint32_t q = from + 1; q <= f->seq && n < DM_FT_NOTICES; q++) if (f->note[q % DM_FT_NOTICES].seq == q) out[n++] = f->note[q % DM_FT_NOTICES];
    return n;
}
int dm_ft_store_facts(dm_ft_store *s, const dm_ft_facts *x, uint32_t now, dm_ft_note out[DM_FT_NOTICES], int *n) {
    pthread_mutex_lock(&s->mu);
    uint32_t from = s->f.seq; int posted = resting(s) ? 0 : dm_ft_update(&s->f, x, now);
    *n = copy_notes(&s->f, from, out);
    pthread_mutex_unlock(&s->mu);
    return posted;
}
int dm_ft_store_count(dm_ft_store *s, int counter, uint64_t v, uint32_t now, dm_ft_note out[DM_FT_NOTICES], int *n) {
    pthread_mutex_lock(&s->mu);
    uint32_t from = s->f.seq; int posted = resting(s) ? 0 : dm_ft_count(&s->f, counter, v, now);
    *n = copy_notes(&s->f, from, out);
    pthread_mutex_unlock(&s->mu);
    return posted;
}
int dm_ft_store_event(dm_ft_store *s, unsigned events, uint32_t now, dm_ft_note out[DM_FT_NOTICES], int *n) {
    pthread_mutex_lock(&s->mu);
    uint32_t from = s->f.seq; int posted = resting(s) ? 0 : dm_ft_event(&s->f, events, now);
    *n = copy_notes(&s->f, from, out);
    pthread_mutex_unlock(&s->mu);
    return posted;
}

static int staging(const dm_ft_store *s) { int any = s->nstaged; for (int c = 0; c < DM_FT_COUNTERS; c++) any |= s->staged_count[c] != 0; return any; }
static void stage_resolve(dm_ft_store *s, uint64_t commits, uint64_t fails) {
    if (!staging(s)) return;
    if (commits >= s->staged_at) {
        for (int k = 0; k < s->nstaged && s->nnotify < DM_FT_STAGED; k++) s->notify[s->nnotify++] = s->staged[k];
    } else if (fails > s->staged_fails) {
        dm_ft_unstage(&s->f, s->staged, s->nstaged);
        for (int c = 0; c < DM_FT_COUNTERS; c++) {
            uint64_t v = dm_ft_counter(&s->f, c), d = s->staged_count[c];
            if (d) dm_ft_set_counter(&s->f, c, v > d ? v - d : 0);
        }
    } else return;
    s->nstaged = 0; memset(s->staged_count, 0, sizeof s->staged_count);
}

static void stage_begin(dm_ft_store *s, uint64_t commits, uint64_t fails) { if (!staging(s)) { s->staged_at = commits + 1; s->staged_fails = fails; } }
int dm_ft_store_stage(dm_ft_store *s, unsigned events, uint32_t now, uint64_t commits, uint64_t fails) {
    pthread_mutex_lock(&s->mu);
    stage_resolve(s, commits, fails);
    int k = 0;
    if (events && !resting(s)) {
        stage_begin(s, commits, fails);
        k = dm_ft_stage(&s->f, events, now, s->staged + s->nstaged, DM_FT_STAGED - s->nstaged);
        s->nstaged += k;
    }
    pthread_mutex_unlock(&s->mu);
    return k;
}
int dm_ft_store_stage_count(dm_ft_store *s, int counter, uint64_t n, uint32_t now, uint64_t commits, uint64_t fails) {
    pthread_mutex_lock(&s->mu);
    stage_resolve(s, commits, fails);
    int k = 0;
    if (n && counter >= 0 && counter < DM_FT_COUNTERS && !resting(s)) {
        stage_begin(s, commits, fails);
        uint64_t before = dm_ft_counter(&s->f, counter);
        k = dm_ft_stage_count(&s->f, counter, n, now, s->staged + s->nstaged, DM_FT_STAGED - s->nstaged);
        s->nstaged += k; s->staged_count[counter] += dm_ft_counter(&s->f, counter) - before;
    }
    pthread_mutex_unlock(&s->mu);
    return k;
}
int dm_ft_store_staged(dm_ft_store *s, uint64_t commits, uint64_t fails, dm_ft_note out[DM_FT_NOTICES], int *n) {
    pthread_mutex_lock(&s->mu);
    stage_resolve(s, commits, fails);
    uint32_t from = s->f.seq; int posted = dm_ft_notice(&s->f, s->notify, s->nnotify);
    s->nnotify = 0;
    *n = copy_notes(&s->f, from, out);
    pthread_mutex_unlock(&s->mu);
    return posted;
}
void dm_ft_store_seed(dm_ft_store *s, int counter, uint64_t v) {
    pthread_mutex_lock(&s->mu); if (!resting(s)) dm_ft_set_counter(&s->f, counter, v); pthread_mutex_unlock(&s->mu);
}
uint64_t dm_ft_store_counter(dm_ft_store *s, int counter) { pthread_mutex_lock(&s->mu); uint64_t v = dm_ft_counter(&s->f, counter); pthread_mutex_unlock(&s->mu); return v; }
int dm_ft_store_text(dm_ft_store *s, char *out, size_t cap) {
    if (!out || !cap) return -1;
    pthread_mutex_lock(&s->mu);
    int n = snprintf(out, cap, "feats\t%d\t%s\n", s->mode, s->who), w = n >= 0 && (size_t)n < cap ? dm_ft_text(&s->f, out + n, cap - (size_t)n) : -2;
    pthread_mutex_unlock(&s->mu);
    if (w < 0) { out[0] = 0; return -2; }
    return n + w;
}


static pthread_mutex_t g_fmu = PTHREAD_MUTEX_INITIALIZER;
static uint8_t g_out[DM_FT_IMAGE_MAX], g_in[DM_FT_IMAGE_MAX + 1];
static uint64_t g_wrote;
static const char *const k_bad[] = { "", "size", "magic", "version", "checksum", "record count", "owner", "record" };

static int path_of(char *out, size_t cap, const char *root, const char *who, const char *suffix) {
    return root && who && who[0] && !strchr(who, '/') && snprintf(out, cap, "%s/%s/%s.sw_feats%s", root, who, who, suffix) < (int)cap;
}
int dm_ft_store_load(dm_ft_store *s, const char *root, const char *who) {
    char p[1200] = "", q[1220]; size_t n = 0; int exists = 0, failed = 0;
    pthread_mutex_lock(&g_fmu);
    if (path_of(p, sizeof p, root, who, "")) {
        errno = 0;
        FILE *f = fopen(p, "rb");
        if (f) { exists = 1; n = fread(g_in, 1, sizeof g_in, f); if (ferror(f)) failed = errno ? errno : EIO; fclose(f); }
        else if (errno != ENOENT) failed = errno;
    }
    int r;
    if (failed) { r = DM_FT_UNREAD; dm_ft_store_owner(s, who, NULL, 0); pthread_mutex_lock(&s->mu); s->mode = DM_FT_MODE_UNREAD; pthread_mutex_unlock(&s->mu); }
    else {
        r = dm_ft_store_owner(s, who, g_in, n);
        if (!r && exists && !n) r = DM_FT_BAD_SIZE;
    }
    pthread_mutex_unlock(&g_fmu);
    if (r == DM_FT_UNREAD) LOG("ASSUME feats.read: the achievements file of this character could not be read (%s); they rest, the file stays as it is, and it is read again when the character next enters a game", strerror(failed));
    else if (r == DM_FT_NEWER) LOG("ASSUME feats.version: the achievements of this character were written by a newer version of the app; they rest and the file stays as it is");
    else if (r == DM_FT_FOREIGN) LOG("ASSUME feats.owner: the achievements file in this character's folder names another character; it is not used and stays until this character's first achievement");
    else if (r) {
        snprintf(q, sizeof q, "%s.bad", p);
        int moved = !rename(p, q);
        LOG("ERROR feats.file_rejected: the achievements of this character failed their %s check; %s, they start empty", k_bad[r], moved ? "set aside as .bad" : "could not be set aside");
    } else if (n) { pthread_mutex_lock(&s->mu); int k = s->f.nrec; pthread_mutex_unlock(&s->mu); LOG("feats: %d records loaded", k); }
    return r;
}
int dm_ft_store_follow(dm_ft_store *s, const char *root, const char *who) {
    if (dm_ft_store_owned_by(s, who) && dm_ft_store_mode(s) != DM_FT_MODE_UNREAD) return 0;
    dm_ft_store_load(s, root, who);
    return 1;
}
int dm_ft_store_commit(dm_ft_store *s, const char *wd, uint64_t commits, uint64_t fails) {
    char who[64], p[1200], t[1220], dir[1200]; uint32_t gen; struct stat st; int written = 0;
    pthread_mutex_lock(&s->mu); stage_resolve(s, commits, fails); pthread_mutex_unlock(&s->mu);
    pthread_mutex_lock(&g_fmu);
    if (g_wrote) { if (commits < g_wrote) dm_ft_store_unsaved(s); g_wrote = 0; }
    size_t n = wd ? dm_ft_store_image(s, who, g_out, sizeof g_out, &gen) : 0;
    if (n && path_of(p, sizeof p, wd, who, "") && snprintf(dir, sizeof dir, "%s/%s", wd, who) < (int)sizeof dir) {
        static int missing;
        if (stat(dir, &st) || !S_ISDIR(st.st_mode)) { if (missing++ < 3) LOG("feats: no character folder in this save yet, written with a later one"); }
        else {
            snprintf(t, sizeof t, "%s.tmp", p);
            FILE *f = fopen(t, "wb");
            int ok = f && fwrite(g_out, 1, n, f) == n;
            if (f && fclose(f)) ok = 0;
            if (ok && !rename(t, p)) { dm_ft_store_saved(s, gen); g_wrote = commits + 1; written = 1; }
            else { remove(t); LOG("ERROR feats.write_failed: the achievements could not be written; tried again with the next save"); }
        }
    }
    pthread_mutex_unlock(&g_fmu);
    return written;
}


typedef char ft_name[DM_FT_OWNER];
static int by_name(const void *a, const void *b) { return strcmp((const char *)a, (const char *)b); }

static void line_value(char *out, const char *s) { for (; *s; s++) *out++ = *s == '\t' || *s == '\n' || *s == '\r' ? ' ' : *s; *out = 0; }
int dm_ft_store_all_text(dm_ft_store *s, const char *root, char *out, size_t cap) {
    if (!out || !cap) return -1;
    dm_ft_sum *a = malloc(sizeof *a);
    ft_name *names = malloc(DM_FT_SUM_HEROES * sizeof *names), *skipped = malloc((DM_FT_SUM_HEROES + 1) * sizeof *skipped), *waiting = malloc(DM_FT_SUM_HEROES * sizeof *waiting);
    uint8_t *img = malloc(DM_FT_IMAGE_MAX + 1), *newer = malloc(DM_FT_SUM_HEROES + 1); dm_ft_rec *rec = malloc(DM_FT_FILE_RECS * sizeof *rec);
    int r = -2, nn = 0, ns = 0, nw = 0, more = 0, unlisted = 0;
    if (!a || !names || !skipped || !waiting || !img || !newer || !rec) goto done;
    dm_ft_sum_clear(a);
    char who[64];
    pthread_mutex_lock(&s->mu);
    memcpy(who, s->who, sizeof who);
    if (who[0] && resting(s)) { newer[ns] = s->mode == DM_FT_MODE_NEWER; snprintf(skipped[ns++], sizeof *skipped, "%s", who); }
    else if (who[0]) dm_ft_sum_add(a, s->f.rec, s->f.nrec);
    pthread_mutex_unlock(&s->mu);


    errno = 0;
    DIR *d = root ? opendir(root) : NULL;
    if (!d) unlisted = root ? errno ? errno : EIO : ENOENT;
    for (struct dirent *e; d;) {
        errno = 0;
        if (!(e = readdir(d))) { if (errno) unlisted = errno; break; }
        char p[1200]; struct stat st;
        if (e->d_name[0] == '.' || strlen(e->d_name) >= sizeof *names || strchr(e->d_name, '/') || !strcmp(e->d_name, who)) continue;
        if (snprintf(p, sizeof p, "%s/%s/%s.d2s", root, e->d_name, e->d_name) >= (int)sizeof p || stat(p, &st) || !S_ISREG(st.st_mode)) continue;
        if (nn == DM_FT_SUM_HEROES) { more++; continue; }
        snprintf(names[nn++], sizeof *names, "%s", e->d_name);
    }
    if (d) closedir(d);
    if (unlisted) LOG("feats: the save folder could not be listed (%s); the other characters' achievements are not shown", root ? strerror(unlisted) : "no save mounted");
    else if (more) LOG("feats: %d characters beyond the first %d of the save folder are not summed", more, DM_FT_SUM_HEROES);
    qsort(names, (size_t)nn, sizeof *names, by_name);
    for (int i = 0; i < nn; i++) {
        char p[1200]; size_t n = 0; int failed = 0, missing = 0, nrec = 0; uint32_t rev;
        if (!path_of(p, sizeof p, root, names[i], "")) failed = 1;
        else {
            errno = 0;
            FILE *f = fopen(p, "rb");
            if (f) { n = fread(img, 1, DM_FT_IMAGE_MAX + 1, f); failed = ferror(f) != 0; fclose(f); }
            else if (errno == ENOENT) missing = 1;
            else failed = 1;
        }
        if (missing) snprintf(waiting[nw++], sizeof *waiting, "%s", names[i]);
        else {
            int bad = failed || !n ? DM_FT_UNREAD : dm_ft_parse(img, n, names[i], rec, &nrec, &rev);
            if (!bad) dm_ft_sum_add(a, rec, nrec);
            else { newer[ns] = bad == DM_FT_NEWER; snprintf(skipped[ns++], sizeof *skipped, "%s", names[i]); }
        }
    }

    for (int i = 1; i < ns; i++) for (int j = i; j > 0 && strcmp(skipped[j - 1], skipped[j]) > 0; j--) {
        ft_name t; uint8_t b = newer[j]; memcpy(t, skipped[j], sizeof t); memcpy(skipped[j], skipped[j - 1], sizeof t); memcpy(skipped[j - 1], t, sizeof t);
        newer[j] = newer[j - 1]; newer[j - 1] = b;
    }
    size_t n = 0; int w; char v[DM_FT_OWNER];
    #define PUT(...) do { w = snprintf(out + n, cap - n, __VA_ARGS__); if (w < 0 || (size_t)w >= cap - n) goto done; n += (size_t)w; } while (0)
    PUT("all\t%d\n", a->heroes);
    if (unlisted) PUT("incomplete\tunavailable\n");
    else if (more) PUT("incomplete\tlimit\n");
    for (int i = 0; i < ns; i++) { line_value(v, skipped[i]); PUT("skipped\t%s\t%s\n", v, newer[i] ? "newer" : "unreadable"); }
    for (int i = 0; i < nw; i++) { line_value(v, waiting[i]); PUT("waiting\t%s\n", v); }
    #undef PUT
    w = dm_ft_sum_text(a, out + n, cap - n);
    if (w >= 0) r = (int)n + w;
done:
    if (r < 0) out[0] = 0;
    free(a); free(names); free(skipped); free(waiting); free(img); free(newer); free(rec);
    return r;
}
