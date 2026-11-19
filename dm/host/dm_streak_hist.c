





#include "dm_streak.h"
#include "dm.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

struct dm_sk_hist {
    pthread_mutex_t mu;
    dm_sk_rec ring[DM_SK_RING]; int head, count;
    dm_sk_rec best[DM_SK_DIFFS];
    char who[64];
    uint32_t gen, saved;
};
static dm_sk_hist g_hist = { .mu = PTHREAD_MUTEX_INITIALIZER };
dm_sk_hist *dm_sk_hist_core(void) { return &g_hist; }

int dm_sk_hist_add(dm_sk_hist *h, const dm_sk_rec *r) {
    if (r->difficulty >= DM_SK_DIFFS || r->kills < DM_SK_START) return 0;
    pthread_mutex_lock(&h->mu);
    h->ring[h->head] = *r; h->head = (h->head + 1) % DM_SK_RING; if (h->count < DM_SK_RING) h->count++;
    dm_sk_rec *b = &h->best[r->difficulty]; int beaten = b->kills > 0 && r->kills > b->kills;
    if (r->kills > b->kills) *b = *r;
    h->gen++;
    pthread_mutex_unlock(&h->mu);
    return beaten;
}
int dm_sk_hist_keep(dm_sk_hist *h, const dm_sk_rec *r, int recording) { return recording && dm_sk_hist_add(h, r); }
int dm_sk_hist_count(dm_sk_hist *h) { pthread_mutex_lock(&h->mu); int n = h->count; pthread_mutex_unlock(&h->mu); return n; }
int dm_sk_hist_get(dm_sk_hist *h, int newest, dm_sk_rec *out) {
    pthread_mutex_lock(&h->mu);
    int ok = newest >= 0 && newest < h->count;
    if (ok) *out = h->ring[(h->head - 1 - newest + DM_SK_RING) % DM_SK_RING];
    pthread_mutex_unlock(&h->mu);
    return ok;
}
int dm_sk_hist_record(dm_sk_hist *h, int difficulty, dm_sk_rec *out) {
    if (difficulty < 0 || difficulty >= DM_SK_DIFFS) return 0;
    pthread_mutex_lock(&h->mu); *out = h->best[difficulty]; pthread_mutex_unlock(&h->mu);
    return out->kills > 0;
}


static uint32_t sk_crc32(const uint8_t *p, size_t n) {
    static uint32_t table[256]; static int ready;
    if (!ready) { for (uint32_t i = 0; i < 256; i++) { uint32_t c = i; for (int k = 0; k < 8; k++) c = c & 1 ? 0xedb88320u ^ (c >> 1) : c >> 1; table[i] = c; } ready = 1; }
    uint32_t c = 0xffffffffu;
    while (n--) c = table[(c ^ *p++) & 0xff] ^ (c >> 8);
    return c ^ 0xffffffffu;
}
static void sk_put32(uint8_t *p, uint32_t v) { memcpy(p, &v, 4); }
static uint32_t sk_get32(const uint8_t *p) { uint32_t v; memcpy(&v, p, 4); return v; }


static int rec_ok(const dm_sk_rec *r) {
    return r->kills >= DM_SK_KEPT_MIN && r->difficulty < DM_SK_DIFFS && (r->end == DM_SK_END_TIME || r->end == DM_SK_END_DEATH) && r->bonus <= 0x7fffffff
        && !(r->end == DM_SK_END_DEATH && r->bonus);
}
int dm_sk_hist_owner(dm_sk_hist *h, const char *who, const uint8_t *image, size_t n) {
    dm_sk_rec best[DM_SK_DIFFS], ring[DM_SK_RING]; int bad = 0, count = 0;
    memset(best, 0, sizeof best);
    if (n) {
        if (n < DM_SK_IMAGE_HEAD + sizeof best || n > DM_SK_IMAGE_MAX) bad = DM_SK_BAD_SIZE;
        else if (sk_get32(image) != DM_SK_MAGIC) bad = DM_SK_BAD_MAGIC;
        else if ((sk_get32(image + 4) & 0xffff) != DM_SK_VERSION) bad = DM_SK_BAD_VERSION;
        else if (sk_get32(image + 12) != sk_crc32(image + DM_SK_IMAGE_HEAD, n - DM_SK_IMAGE_HEAD)) bad = DM_SK_BAD_CRC;
        else if (sk_get32(image + 8) > DM_SK_RING) bad = DM_SK_BAD_COUNT;
        else if (n != DM_SK_IMAGE_HEAD + sizeof best + sk_get32(image + 8) * sizeof *ring) bad = DM_SK_BAD_SIZE;
        else {
            count = (int)sk_get32(image + 8);
            memcpy(best, image + DM_SK_IMAGE_HEAD, sizeof best);
            memcpy(ring, image + DM_SK_IMAGE_HEAD + sizeof best, (size_t)count * sizeof *ring);
            for (int d = 0; d < DM_SK_DIFFS && !bad; d++) if (best[d].kills && (!rec_ok(&best[d]) || best[d].difficulty != d)) bad = DM_SK_BAD_RECORD;
            for (int d = 0; d < DM_SK_DIFFS && !bad; d++) if (!best[d].kills && memcmp(&best[d], &(dm_sk_rec){ 0 }, sizeof best[d])) bad = DM_SK_BAD_RECORD;

            for (int i = 0; i < count && !bad; i++) if (!rec_ok(&ring[i]) || ring[i].kills > best[ring[i].difficulty].kills) bad = DM_SK_BAD_RECORD;
        }
    }
    if (bad) { count = 0; memset(best, 0, sizeof best); }
    pthread_mutex_lock(&h->mu);
    memcpy(h->ring, ring, (size_t)count * sizeof *ring); h->count = count; h->head = count % DM_SK_RING;
    memcpy(h->best, best, sizeof best);
    snprintf(h->who, sizeof h->who, "%s", who ? who : "");
    h->gen++; h->saved = h->gen;
    pthread_mutex_unlock(&h->mu);
    return bad;
}
int dm_sk_hist_owned_by(dm_sk_hist *h, const char *who) {
    pthread_mutex_lock(&h->mu); int same = !strcmp(h->who, who ? who : ""); pthread_mutex_unlock(&h->mu);
    return same;
}
size_t dm_sk_hist_image(dm_sk_hist *h, char who[64], uint8_t *buf, size_t cap, uint32_t *gen) {
    size_t n = 0;
    pthread_mutex_lock(&h->mu);
    if (h->gen != h->saved && h->who[0] && cap >= DM_SK_IMAGE_MAX) {
        n = DM_SK_IMAGE_HEAD;
        memcpy(buf + n, h->best, sizeof h->best); n += sizeof h->best;
        for (int i = 0; i < h->count; i++) { memcpy(buf + n, &h->ring[(h->head - h->count + i + DM_SK_RING) % DM_SK_RING], sizeof *h->ring); n += sizeof *h->ring; }
        sk_put32(buf, DM_SK_MAGIC); sk_put32(buf + 4, DM_SK_VERSION); sk_put32(buf + 8, (uint32_t)h->count);
        memcpy(who, h->who, 64); *gen = h->gen;
    }
    pthread_mutex_unlock(&h->mu);
    if (n) sk_put32(buf + 12, sk_crc32(buf + DM_SK_IMAGE_HEAD, n - DM_SK_IMAGE_HEAD));
    return n;
}
void dm_sk_hist_saved(dm_sk_hist *h, uint32_t gen) { pthread_mutex_lock(&h->mu); h->saved = gen; pthread_mutex_unlock(&h->mu); }
void dm_sk_hist_unsaved(dm_sk_hist *h) { pthread_mutex_lock(&h->mu); h->saved = h->gen - 1; pthread_mutex_unlock(&h->mu); }


static pthread_mutex_t g_fmu = PTHREAD_MUTEX_INITIALIZER;
static uint8_t g_out[DM_SK_IMAGE_MAX], g_in[DM_SK_IMAGE_MAX + 1];
static uint64_t g_wrote;
static const char *const k_bad[] = { "", "size", "magic", "version", "checksum", "streak count", "streak record" };

static int path_of(char *out, size_t cap, const char *root, const char *who, const char *suffix) {
    return root && who && who[0] && !strchr(who, '/') && snprintf(out, cap, "%s/%s/%s.sw_streaks%s", root, who, who, suffix) < (int)cap;
}
int dm_sk_hist_load(dm_sk_hist *h, const char *root, const char *who) {
    char p[1200] = "", q[1220]; size_t n = 0;
    pthread_mutex_lock(&g_fmu);
    if (path_of(p, sizeof p, root, who, "")) {
        FILE *f = fopen(p, "rb");
        if (f) { n = fread(g_in, 1, sizeof g_in, f); if (ferror(f)) n = sizeof g_in; fclose(f); }
    }
    int bad = dm_sk_hist_owner(h, who, g_in, n);
    pthread_mutex_unlock(&g_fmu);
    if (bad) {
        snprintf(q, sizeof q, "%s.bad", p);
        int moved = !rename(p, q);
        LOG("ERROR streak.file_rejected: the kill streaks of this character failed their %s check; %s, the list starts empty", k_bad[bad], moved ? "set aside as .bad" : "could not be set aside");
    } else if (n) LOG("kill streak: %d streaks loaded", dm_sk_hist_count(h));
    return bad;
}
int dm_sk_hist_follow(dm_sk_hist *h, const char *root, const char *who) {
    if (dm_sk_hist_owned_by(h, who)) return 0;
    dm_sk_hist_load(h, root, who);
    return 1;
}
int dm_sk_hist_commit(dm_sk_hist *h, const char *wd, uint64_t commits) {
    char who[64], p[1200], t[1220], dir[1200]; uint32_t gen; struct stat st; int written = 0;
    pthread_mutex_lock(&g_fmu);
    if (g_wrote) { if (commits < g_wrote) dm_sk_hist_unsaved(h); g_wrote = 0; }
    size_t n = wd ? dm_sk_hist_image(h, who, g_out, sizeof g_out, &gen) : 0;
    if (n && path_of(p, sizeof p, wd, who, "") && snprintf(dir, sizeof dir, "%s/%s", wd, who) < (int)sizeof dir) {
        static int missing;
        if (stat(dir, &st) || !S_ISDIR(st.st_mode)) { if (missing++ < 3) LOG("kill streak: no character folder in this save yet, written with a later one"); }
        else {
            snprintf(t, sizeof t, "%s.tmp", p);
            FILE *f = fopen(t, "wb");
            int ok = f && fwrite(g_out, 1, n, f) == n;
            if (f && fclose(f)) ok = 0;
            if (ok && !rename(t, p)) { dm_sk_hist_saved(h, gen); g_wrote = commits + 1; written = 1; }
            else { remove(t); LOG("ERROR streak.write_failed: the kill streaks could not be written; tried again with the next save"); }
        }
    }
    pthread_mutex_unlock(&g_fmu);
    return written;
}


int dm_sk_hist_text(dm_sk_hist *h, char *out, size_t cap) {
    if (!out || !cap) return -1;
    size_t n = 0; int w;
    #define PUT(...) do { w = snprintf(out + n, cap - n, __VA_ARGS__); if (w < 0 || (size_t)w >= cap - n) { pthread_mutex_unlock(&h->mu); out[0] = 0; return -2; } n += (size_t)w; } while (0)
    pthread_mutex_lock(&h->mu);
    PUT("streaks\t%d\n", h->count);
    for (int d = 0; d < DM_SK_DIFFS; d++) {
        const dm_sk_rec *r = &h->best[d];
        if (r->kills) PUT("best\t%d\t%u\t%u\t%u\t%u\t%u\n", d, r->kills, r->ticks, r->bonus, r->area, r->end);
    }
    for (int i = 0; i < h->count; i++) {
        const dm_sk_rec *r = &h->ring[(h->head - 1 - i + DM_SK_RING) % DM_SK_RING];
        PUT("s\t%d\t%u\t%u\t%u\t%u\t%u\t%u\n", i, r->kills, r->ticks, r->bonus, r->area, r->difficulty, r->end);
    }
    pthread_mutex_unlock(&h->mu);
    #undef PUT
    return (int)n;
}
