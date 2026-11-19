



#include "../log_policy.h"
#include <assert.h>

static long long file_size(const char *p) { struct stat st; return stat(p, &st) ? -1 : (long long)st.st_size; }
static void fill(const char *p, long long bytes) {
    FILE *f = fopen(p, "w"); assert(f); long long n = 0; unsigned i = 0;
    while (n < bytes) { int w = fprintf(f, "line %u xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n", i++); assert(w > 0); n += w; }
    fclose(f);
}
static int mode_of(const char *p) { struct stat st; return stat(p, &st) ? -1 : (int)(st.st_mode & 0777); }


static int window(dm_perf_sampler *f, uint64_t t, int poolreset, int new_pipes, int *kept_detail, int *notes) {
    int v1 = 0, k;
    if (poolreset) { k = dm_perf_keep(f, "NVN PERF poolreset queued=1.00 waits=0.00 wait_ms=0.000", t); *kept_detail += k > 0; }
    k = dm_perf_keep(f, "NVN PERF v=1 sync=1 frames=60 wall_ms=1000.0 fps=60.00 submit_cpu_ms=9.0", t); v1 = k > 0;
    if (k == 2) { char note[160]; dm_perf_note(f, note, sizeof note); assert(!strncmp(note, "NVN PERF sampling thinned=", 26)); (*notes)++; }
    for (int d = 0; d < 24; d++) *kept_detail += dm_perf_keep(f, "NVN PERF pass_ends op1=2.0 fb_hits=3.0", t) > 0;
    *kept_detail += dm_perf_keep(f, new_pipes ? "NVN PERF pipelines_new=3 evicted=0 warm_hits=1" : "NVN PERF pipelines_new=0 evicted=0 warm_hits=1", t) > 0;
    *kept_detail += dm_perf_keep(f, "NVN PERF textures bc_native=1 bc_decoded=0", t) > 0;
    return v1;
}

int main(void) {
    char dir[] = "/tmp/dm_log_policy_XXXXXX"; assert(mkdtemp(dir));
    char log[256], one[256], tr[256]; snprintf(log, sizeof log, "%s/boot.log", dir); snprintf(one, sizeof one, "%s/boot.log.1", dir); snprintf(tr, sizeof tr, "%s/boot.log.trim", dir);
    long long old; int trim;


    assert(dm_log_rotate_action(0) == 0 && dm_log_rotate_action(DM_LOG_ROTATE_START - 1) == 0);
    assert(dm_log_rotate_action(DM_LOG_ROTATE_START) == 1 && dm_log_rotate_action(DM_LOG_ROTATE_MAX) == 1);
    assert(dm_log_rotate_action(DM_LOG_ROTATE_MAX + 1LL) == 2 && dm_log_rotate_action(115LL << 20) == 2);


    assert(dm_log_rotate_at_start(log, &old, &trim) == 0 && old == 0 && !trim);
    long long sz = -1; FILE *lf = dm_log_open(log, &sz); assert(lf && sz == 0); fputs("x\n", lf); fclose(lf); assert(mode_of(log) == 0600);
    chmod(log, 0644); lf = dm_log_open(log, &sz); assert(lf && sz == 2); fclose(lf); assert(mode_of(log) == 0600);
    fill(log, 1 << 20); assert(dm_log_rotate_at_start(log, &old, &trim) == 0 && old == file_size(log) && file_size(one) < 0 && !trim);


    fill(one, 1000); fill(log, DM_LOG_ROTATE_START + 5000LL); long long was = file_size(log);
    assert(dm_log_rotate_at_start(log, &old, &trim) == 1 && old == was && file_size(one) == was && file_size(log) < 0 && !trim);


    fill(log, DM_LOG_ROTATE_MAX + (3LL << 20)); was = file_size(log);
    assert(dm_log_rotate_at_start(log, &old, &trim) == 2 && old == was && trim && file_size(log) < 0 && file_size(tr) == was);
    assert(dm_log_trim(log) == 0 && file_size(tr) < 0);
    long long kept = file_size(one); assert(kept > DM_LOG_KEEP_TAIL - 200 && kept <= DM_LOG_KEEP_TAIL && mode_of(one) == 0600);
    { FILE *f = fopen(one, "r"); char first[16] = ""; assert(f && fread(first, 1, 5, f) == 5); fclose(f); assert(!memcmp(first, "line ", 5)); }
    { char tmp[300]; snprintf(tmp, sizeof tmp, "%s.tmp", one); assert(file_size(tmp) < 0); }

    fill(tr, 5000); fill(log, 100); assert(dm_log_rotate_at_start(log, &old, &trim) == 0 && trim);
    fill(log, DM_LOG_ROTATE_MAX + 100LL); assert(dm_log_rotate_at_start(log, &old, &trim) == 2 && trim && file_size(tr) == old && old > DM_LOG_ROTATE_MAX);
    assert(dm_log_trim(log) == 0 && file_size(tr) < 0);


    fill(tr, 5000); fill(log, DM_LOG_ROTATE_START + 10LL); was = file_size(log);
    assert(dm_log_rotate_at_start(log, &old, &trim) == 1 && !trim && file_size(tr) < 0 && file_size(one) == was);

    assert(dm_log_trim(log) == -1 && file_size(tr) < 0);

    kept = file_size(one); unlink(log); assert(!symlink(one, log)); assert(dm_log_rotate_at_start(log, &old, &trim) == 0 && file_size(one) == kept && !dm_log_open(log, NULL)); unlink(log);



    dm_perf_sampler f = {.period_ns = 30000000000ull}; uint64_t t = 5000000000ull; int v1 = 0, detail = 0, notes = 0;
    for (int w = 0; w < 100; w++, t += 1000000000ull) v1 += window(&f, t, 1, w == 50, &detail, &notes);
    assert(v1 == 100);


    assert(detail == 6 * 27 + 1);
    assert(notes == 3);

    assert(dm_perf_keep(&f, "NVN PERF uploads window prologue=3 inpass=0 broke_pass=0", t) == 1);
    assert(dm_perf_keep(&f, "NVN PERF evguard engine_opens=1 recloses=0", t) == 1);
    assert(dm_perf_keep(&f, "NVN PERF brand_new_counter x=1", t) == 1);
    assert(dm_perf_keep(&f, "NVN PERF present_gpu_us=812 extent=2480x1116", t) == 1);
    assert(dm_perf_keep(&f, "NVN shader 1234 stage=4", t) == 1 && dm_perf_keep(&f, "[dm] dm_hud：区域 40 (1,2) Lv 7", t) == 1);

    dm_perf_sampler g = {.period_ns = 30000000000ull}; t = 1; v1 = detail = notes = 0;
    for (int w = 0; w < 100; w++, t += 1000000000ull) v1 += window(&g, t, 0, 0, &detail, &notes);
    assert(v1 == 100 && detail == 6 * 26 && notes == 3);

    dm_perf_sampler e = {.period_ns = 30000000000ull}; t = 1; v1 = detail = notes = 0;
    for (int w = 0; w < 20; w++, t += 8500000000ull) v1 += window(&e, t, 1, 0, &detail, &notes);
    assert(v1 == 20 && detail == (3 + 4) * 27);

    dm_perf_sampler all = {0}; t = 1; v1 = detail = notes = 0;
    for (int w = 0; w < 10; w++, t += 1000000000ull) v1 += window(&all, t, 1, 0, &detail, &notes);
    assert(v1 == 10 && detail == 10 * 27 && notes == 0);

    { dm_perf_sampler n = {.period_ns = 30000000000ull}; t = 1; int d = 0, no = 0;
      for (int w = 0; w < 33; w++, t += 1000000000ull) window(&n, t, 1, 0, &d, &no);
      assert(no == 1 && n.thinned == 0 && n.dropped == 0); }

    unlink(one); rmdir(dir);
    puts("PASS log rotation (start rename, background tail trim, 0600, no symlink follow) and NVN PERF window sampling");
    return 0;
}
