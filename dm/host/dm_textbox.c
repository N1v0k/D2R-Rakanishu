













#include "hle.h"
#include "game_addrs.h"
#include "dm.h"
#include "dm_textbox_core.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

static int g_loaded;
static uint8_t *g_b;
static uint64_t g_calls, g_hits, g_ns, g_max_ns, g_t0;

static void load(void) {
    g_loaded = -1; const char *ov = getenv("DM_ROMFS_OVERLAY"); if (!ov) { LOG("文字框颜色码：没有覆盖层，不处理"); return; }
    char path[1200]; int n = dm_overlay_file("dm_textbox_plain.tsv", path, sizeof path) ? dm_textbox_load(path) : -1;
    if (n <= 0) { LOG("文字框颜色码：覆盖层里没有对照表（物品注释关着），不处理"); return; }
    g_loaded = 1; LOG("文字框颜色码：读入对照表 %d 条（%s）", n, path);
}
int dm_textbox_ensure_loaded(void) { if (!g_loaded) load(); return g_loaded > 0; }
static void tb_obs(const uint64_t *regs) {
    if (!g_loaded) load(); if (g_loaded < 0) return;
    uint64_t t0 = clock_gettime_nsec_np(CLOCK_UPTIME_RAW); if (!g_t0) g_t0 = t0;
    uint64_t w = regs[0]; const char *s = w ? *(const char **)(uintptr_t)(w + 0x88) : NULL; size_t n = w ? *(const size_t *)(uintptr_t)(w + 0x90) : 0;
    g_calls++; char *o = NULL; size_t on = 0; int k = 0;




    int handles = *(const uint8_t *)(uintptr_t)(w + 0xfc) == 1;
    if (!handles) k = dm_textbox_rewrite(s, n, &o, &on);
    { static int lg = -1; if (lg < 0) lg = getenv("DM_TEXTBOX_LOG") != NULL;
      if (lg && s && n && memmem(s, n, "\xee\x81\xbe", 3)) { static const char *seen[64]; static int ns; const char *nm = *(const char **)(uintptr_t)(w + 8); int i = 0; while (i < ns && seen[i] != nm) i++;
          if (i == ns && ns < 64) { seen[ns++] = nm; LOG("文字框颜色码：带码的文字框「%s」→ %s", nm ? nm : "?", handles ? "认颜色码，不动" : k ? "按表改写" : "不认颜色码但不在表里（原版自带的串），不动"); } } }
    if (k) { static int lg = -1; if (lg < 0) lg = getenv("DM_TEXTBOX_LOG") != NULL;
        if (lg && g_hits < 200) { const char *nm = *(const char **)(uintptr_t)(w + 8); LOG("文字框颜色码：%s 控件「%s」→「%s」", k == 1 ? "整串" : "子串", nm ? nm : "?", o); }
        g_hits++; ((void *(*)(uint64_t, const char *, uint64_t))(g_b + GA_TEXT_SET))(w + 0x88, o, on); free(o); }
    uint64_t dt = clock_gettime_nsec_np(CLOCK_UPTIME_RAW) - t0; g_ns += dt; if (dt > g_max_ns) g_max_ns = dt;
    static uint64_t last; static int stat = -1; if (stat < 0) stat = getenv("DM_TEXTBOX_STAT") != NULL;
    if (stat && t0 - last > 10000000000ull) { last = t0; double sec = (t0 - g_t0) / 1e9;
        LOG("文字框颜色码：%.0f 秒内门触发 %llu 次（%.1f 次 / 秒），替换 %llu 次，平均 %.2f 微秒、最长 %.1f 微秒", sec, g_calls, sec > 0 ? g_calls / sec : 0, g_hits, g_calls ? g_ns / 1e3 / g_calls : 0, g_max_ns / 1e3); }
}
void dm_textbox_install(uint8_t *main_base) {
    g_b = main_base; if (!gate_obs_present(OBS_TEXTBOX)) { LOG("文字框颜色码：产物没有观测门 14（r33 起），不处理"); return; }
    if (getenv("DM_TEXTBOX_OFF")) { LOG("文字框颜色码：测试开关 DM_TEXTBOX_OFF，不挂门（A/B 量帧时间用）"); return; }
    gate_obs_set(OBS_TEXTBOX, tb_obs);
}
