

#include "dm_resume_core.h"
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int name_ok(const char *who) {
    size_t n = strlen(who);
    if (!n || n >= DM_RESUME_NAME_MAX) return 0;
    for (size_t i = 0; i < n; i++) if (!((who[i] >= 'A' && who[i] <= 'Z') || (who[i] >= 'a' && who[i] <= 'z') || who[i] == '-' || who[i] == '_')) return 0;
    return 1;
}

size_t dm_resume_format(const dm_resume_pos *p, char *out, size_t cap) {
    if (!name_ok(p->who)) return 0;
    if (p->boss != 0 && p->boss != 1) return 0;
    int n = snprintf(out, cap, "# dm_resume %d\nwho %s\nlevel %d\npos %d %d\ndifficulty %d\nboss %d\nmap %016" PRIx64 "\nsave %016" PRIx64 "\n",
                     DM_RESUME_VERSION, p->who, p->level, p->x, p->y, p->difficulty, p->boss, p->map_fp, p->save_fp);
    return n > 0 && (size_t)n < cap ? (size_t)n : 0;
}

int dm_resume_parse(const char *text, dm_resume_pos *out) {
    dm_resume_pos p; memset(&p, 0, sizeof p);
    char who[64]; unsigned seen = 0; int version = 0, k = 0;
    if (sscanf(text, "# dm_resume %d\n%n", &version, &k) != 1 || version != DM_RESUME_VERSION || !k) return 0;
    for (const char *s = text + k; *s; ) {
        const char *e = strchr(s, '\n'); if (!e) return 0;
        char line[128]; size_t n = (size_t)(e - s); if (n >= sizeof line) return 0;
        memcpy(line, s, n); line[n] = 0; s = e + 1;
        int used = 0;
        if (sscanf(line, "who %63s%n", who, &used) == 1 && !line[used] && !(seen & 1)) { if (strlen(who) >= sizeof p.who) return 0; strcpy(p.who, who); seen |= 1; }
        else if (sscanf(line, "level %d%n", &p.level, &used) == 1 && !line[used] && !(seen & 2)) seen |= 2;
        else if (sscanf(line, "pos %d %d%n", &p.x, &p.y, &used) == 2 && !line[used] && !(seen & 4)) seen |= 4;
        else if (sscanf(line, "difficulty %d%n", &p.difficulty, &used) == 1 && !line[used] && !(seen & 8)) seen |= 8;
        else if (sscanf(line, "map %16" SCNx64 "%n", &p.map_fp, &used) == 1 && !line[used] && !(seen & 16)) seen |= 16;
        else if (sscanf(line, "save %16" SCNx64 "%n", &p.save_fp, &used) == 1 && !line[used] && !(seen & 32)) seen |= 32;
        else if (sscanf(line, "boss %d%n", &p.boss, &used) == 1 && !line[used] && !(seen & 64)) seen |= 64;
        else return 0;
    }
    if (seen != 127 || (p.boss != 0 && p.boss != 1) || !name_ok(p.who) || p.level < 1 || p.level > 255 || p.x <= 0 || p.y <= 0 || p.difficulty < 0 || p.difficulty > 2 || !p.save_fp) return 0;
    *out = p;
    return 1;
}

int dm_resume_is_town(int level) { return level == 1 || level == 40 || level == 75 || level == 103 || level == 109; }

int dm_resume_level_allowed(int level) {
    static const int refused[] = { 37, 73, 102, 108, 131, 132, 39, 136 };
    if (level < 1 || level > 255 || dm_resume_is_town(level)) return 0;
    for (size_t i = 0; i < sizeof refused / sizeof *refused; i++) if (refused[i] == level) return 0;
    return 1;
}

uint64_t dm_resume_fp_room(uint64_t fp, int32_t x, int32_t y, int32_t w, int32_t h) {
    int32_t v[4] = { x, y, w, h };
    const uint8_t *b = (const uint8_t *)v;
    for (size_t i = 0; i < sizeof v; i++) { fp ^= b[i]; fp *= 1099511628211ull; }
    return fp;
}

dm_resume_why dm_resume_decide(int switch_on, int marked, const dm_resume_pos *p, const char *who, uint64_t loaded_save_fp, int difficulty) {
    if (!switch_on) return DM_RESUME_SWITCH_OFF;
    if (!marked) return DM_RESUME_NO_MARK;
    if (!p) return DM_RESUME_BAD_FILE;
    if (strcmp(p->who, who)) return DM_RESUME_OTHER_CHAR;
    if (!loaded_save_fp || p->save_fp != loaded_save_fp) return DM_RESUME_OTHER_SAVE;
    if (p->difficulty != difficulty) return DM_RESUME_OTHER_DIFFICULTY;
    if (dm_resume_is_town(p->level)) return DM_RESUME_BAD_FILE;
    return dm_resume_level_allowed(p->level) && !p->boss ? DM_RESUME_RETURN : DM_RESUME_WAYPOINT;
}

const char *dm_resume_why_name(dm_resume_why w) {
    static const char *const n[] = { "back to the spot", "switch off", "no marker", "no valid sidecar", "sidecar of another character",
                                     "the character file changed since", "another difficulty", "to the nearest waypoint" };
    return (unsigned)w < sizeof n / sizeof *n ? n[w] : "?";
}

int dm_resume_nearest_wp(int from, const dm_resume_level *lv, int n, const uint8_t wps[DM_RESUME_WP_BYTES]) {
    enum { MAX = 1024 };
    if (from <= 0 || from >= n || n > MAX) return 0;
    static uint8_t seen[MAX]; static int ring[MAX], next[MAX];
    memset(seen, 0, (size_t)n);
    int act = lv[from].act, nr = 1, nn;
    ring[0] = from; seen[from] = 1;
    while (nr) {
        int best = 0, town = 0;
        for (int i = 0; i < nr; i++) {
            int v = ring[i], w = lv[v].wp;
            if (dm_resume_is_town(v)) town = 1;
            else if (w >= 0 && w < 8 * DM_RESUME_WP_BYTES && (wps[w / 8] >> (w % 8) & 1) && (!best || v < best)) best = v;
        }
        if (best) return best;
        if (town) return 0;
        nn = 0;
        for (int i = 0; i < nr; i++) for (int k = 0; k < 8; k++) {
            int v = lv[ring[i]].vis[k];
            if (v > 0 && v < n && !seen[v] && lv[v].act == act) { seen[v] = 1; next[nn++] = v; }
        }
        memcpy(ring, next, sizeof *ring * (size_t)nn); nr = nn;
    }


    int low = from, best = 0;
    for (int v = 1; v < n; v++) if (seen[v] && v < low) low = v;
    for (int v = 1; v <= low; v++) {
        int w = lv[v].wp;
        if (lv[v].act == act && !dm_resume_is_town(v) && w >= 0 && w < 8 * DM_RESUME_WP_BYTES && (wps[w / 8] >> (w % 8) & 1)) best = v;
    }
    return best;
}

int dm_resume_wp_bits(const uint8_t *save, size_t n, int difficulty, uint8_t out[DM_RESUME_WP_BYTES]) {
    static const uint8_t head[] = { 'W', 'S', 1, 0, 0, 0, 0x50, 0 };
    if (difficulty < 0 || difficulty > 2) return 0;
    for (size_t at = 0; at + sizeof head + 3 * 24 <= n; at++) {
        if (memcmp(save + at, head, sizeof head)) continue;
        const uint8_t *b = save + at + sizeof head + 24 * (size_t)difficulty;
        if (b[0] != 2 || b[1] != 1) return 0;
        memcpy(out, b + 2, DM_RESUME_WP_BYTES);
        return 1;
    }
    return 0;
}

int dm_resume_save_wps(const char *dir, const char *who, int difficulty, uint8_t out[DM_RESUME_WP_BYTES]) {
    char path[1200]; if (!name_ok(who) || snprintf(path, sizeof path, "%s/%s/%s.d2s", dir, who, who) >= (int)sizeof path) return 0;
    FILE *f = fopen(path, "rb"); if (!f) return 0;
    static uint8_t buf[1 << 16]; size_t n = fread(buf, 1, sizeof buf, f); int ok = !ferror(f) && feof(f); fclose(f);
    return ok && dm_resume_wp_bits(buf, n, difficulty, out);
}

uint64_t dm_resume_save_fp(const char *dir, const char *who) {
    char path[1200]; if (!name_ok(who) || snprintf(path, sizeof path, "%s/%s/%s.d2s", dir, who, who) >= (int)sizeof path) return 0;
    FILE *f = fopen(path, "rb"); if (!f) return 0;
    uint64_t h = DM_RESUME_FP_START; uint8_t buf[4096]; size_t n, total = 0;
    while ((n = fread(buf, 1, sizeof buf, f)) > 0) { for (size_t i = 0; i < n; i++) { h ^= buf[i]; h *= 1099511628211ull; } total += n; }
    int bad = ferror(f); fclose(f);
    return bad || !total ? 0 : h ? h : 1;
}

int dm_resume_sidecar_path(const char *dir, const char *who, char *out, size_t cap) {
    return name_ok(who) && snprintf(out, cap, "%s/%s/%s.sw_resume", dir, who, who) < (int)cap;
}

static int write_atomic(const char *path, const char *text, size_t n) {
    char tmp[1300]; if (snprintf(tmp, sizeof tmp, "%s.tmp", path) >= (int)sizeof tmp) return 0;
    FILE *f = fopen(tmp, "w"); if (!f) return 0;
    int ok = fwrite(text, 1, n, f) == n; ok = fclose(f) == 0 && ok;
    if (ok && !rename(tmp, path)) return 1;
    remove(tmp); return 0;
}

static int read_small(const char *path, char *out, size_t cap) {
    FILE *f = fopen(path, "r"); if (!f) return 0;
    size_t n = fread(out, 1, cap - 1, f); int ok = !ferror(f) && feof(f); fclose(f);
    out[n] = 0; return ok;
}

int dm_resume_sidecar_write(const char *dir, const dm_resume_pos *p) {
    char path[1200], text[512]; size_t n = dm_resume_format(p, text, sizeof text);
    return n && dm_resume_sidecar_path(dir, p->who, path, sizeof path) && write_atomic(path, text, n);
}

int dm_resume_sidecar_read(const char *dir, const char *who, dm_resume_pos *out) {
    char path[1200], text[512];
    return dm_resume_sidecar_path(dir, who, path, sizeof path) && read_small(path, text, sizeof text) && dm_resume_parse(text, out);
}

int dm_resume_committed(const char *dir, const char *who) {
    dm_resume_pos p; uint64_t fp = dm_resume_save_fp(dir, who);
    return fp && dm_resume_sidecar_read(dir, who, &p) && !strcmp(p.who, who) && p.save_fp == fp;
}

int dm_resume_mark_write(const char *path, const char *who) {
    char text[64]; if (!name_ok(who)) return 0;
    int n = snprintf(text, sizeof text, "%s\n", who);
    return n > 0 && (size_t)n < sizeof text && write_atomic(path, text, (size_t)n);
}

int dm_resume_mark_read(const char *path, char who[DM_RESUME_NAME_MAX]) {
    char text[64], name[64]; int used = 0;
    if (!read_small(path, text, sizeof text) || sscanf(text, "%63s\n%n", name, &used) != 1 || text[used] || strlen(name) >= DM_RESUME_NAME_MAX || !name_ok(name)) return 0;
    strcpy(who, name); return 1;
}

int dm_resume_mark_delete(const char *path) { return !unlink(path) || errno == ENOENT; }

dm_resume_why dm_resume_join(const char *mark_path, const char *save_dir, const char *who, int switch_on, int difficulty, dm_resume_pos *out, int *removal_failed) {
    char mark_who[DM_RESUME_NAME_MAX]; dm_resume_pos p;
    *removal_failed = 0;
    int marked = dm_resume_mark_read(mark_path, mark_who) && !strcmp(mark_who, who);
    if (!marked) return dm_resume_decide(switch_on, 0, NULL, who, 0, difficulty);
    if (!dm_resume_mark_delete(mark_path)) { *removal_failed = 1; return DM_RESUME_NO_MARK; }
    int parsed = dm_resume_sidecar_read(save_dir, who, &p);
    dm_resume_why w = dm_resume_decide(switch_on, 1, parsed ? &p : NULL, who, dm_resume_save_fp(save_dir, who), difficulty);
    if (parsed) *out = p;
    return w;
}

void dm_resume_left(dm_resume_marks *m) { m->gen++; m->left = 1; }

int dm_resume_back(dm_resume_marks *m) {
    if (!m->left) return 0;
    m->gen++; m->left = 0; return 1;
}

void dm_resume_dropped(dm_resume_marks *m) { m->gen++; }

unsigned dm_resume_leave_begin(dm_resume_marks *m) { m->saving = 1; m->saving_gen = m->gen; return m->gen; }

int dm_resume_sidecar_due(const dm_resume_marks *m) { return m->saving; }

void dm_resume_sidecar_done(dm_resume_marks *m) { m->side_gen = m->saving_gen; }

int dm_resume_leave_end(dm_resume_marks *m, unsigned gen) {
    m->saving = 0;

    return m->left && m->gen == gen && m->side_gen == gen && gen;
}

void dm_resume_go(dm_resume_run *r, const dm_resume_pos *p, int target, int on_spot, int camp) {
    memset(r, 0, sizeof *r); r->st = DM_RESUME_GO; r->pos = *p; r->target = target; r->on_spot = on_spot; r->camp = camp;
}

int dm_resume_leave_there(dm_resume_run *r, int to_camp) {
    if (r->st != DM_RESUME_THERE) return 0;
    r->st = to_camp ? DM_RESUME_TO_CAMP : DM_RESUME_SHIELD; r->started = 0;
    return 1;
}

int dm_resume_shielded(const dm_resume_run *r) { return r->st >= DM_RESUME_GO && r->st <= DM_RESUME_TO_CAMP; }

dm_resume_act dm_resume_step(dm_resume_run *r, uint32_t tick, int server_level, int client_level, int acting) {
    switch (r->st) {
    case DM_RESUME_GO: r->st = DM_RESUME_ARRIVING; r->since = tick; return DM_RESUME_ACT_WARP;
    case DM_RESUME_ARRIVING:
        if (server_level == r->target && client_level == r->target) { r->st = DM_RESUME_THERE; return DM_RESUME_ACT_PLACE; }
        if (tick - r->since > DM_RESUME_ARRIVE_TICKS) { r->st = DM_RESUME_THERE; return DM_RESUME_ACT_GIVE_UP; }
        return DM_RESUME_ACT_NONE;
    case DM_RESUME_SHIELD:
        if (!r->started) { r->started = 1; r->since = tick; }
        if (acting || tick - r->since >= DM_RESUME_SHIELD_TICKS) { r->st = DM_RESUME_DONE; return DM_RESUME_ACT_SHIELD_OFF; }
        return DM_RESUME_ACT_NONE;
    case DM_RESUME_TO_CAMP:
        if (!r->started) { r->started = 1; r->since = tick; return DM_RESUME_ACT_CAMP; }
        if (server_level == r->camp || tick - r->since > DM_RESUME_ARRIVE_TICKS) r->st = DM_RESUME_DONE;
        return DM_RESUME_ACT_NONE;
    default: return DM_RESUME_ACT_NONE;
    }
}
