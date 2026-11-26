













#include "dm_uiscreen.h"
#include "game_addrs.h"
#include "hle.h"
#include "dm.h"
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

static _Atomic int g_mode = -1;
static _Atomic int g_rw, g_rh, g_uh;
static uint8_t *main_base(void) { static uint8_t *b; if (!b) for (int i = 0; i < g_nmod; i++) if (!strcmp(g_mod[i].name, "main")) b = (uint8_t *)g_mod[i].base; return b; }
static int mode(void) { int m = atomic_load(&g_mode); if (m >= 0) return m;
    const char *e = getenv("DM_UI_169"), *p = getenv("DM_UI_YPOS"); m = !(e && strcmp(e, "0")) ? DM_UISCREEN_OFF : p && !strcmp(p, "center") ? DM_UISCREEN_CENTER : p && !strcmp(p, "bottom") ? DM_UISCREEN_BOTTOM : DM_UISCREEN_TOP;
    int exp = -1; atomic_compare_exchange_strong(&g_mode, &exp, m); return atomic_load(&g_mode); }
void dm_uiscreen_enable(int m) { atomic_store(&g_mode, m < DM_UISCREEN_OFF || m > DM_UISCREEN_BOTTOM ? DM_UISCREEN_OFF : m); }








static _Atomic int g_ingame;
void (*g_dm_charsel_hook)(void);
void (*g_dm_ingame_hook)(int on);
void dm_uiscreen_set_ingame(int on) { if (atomic_exchange(&g_ingame, on != 0) != (on != 0)) { LOG("界面画面：%s", on ? "进局（真全屏）" : "出局（留边）"); if (g_dm_ingame_hook) g_dm_ingame_hook(on != 0); } }
int dm_uiscreen_ingame(void) { return atomic_load(&g_ingame); }






static _Atomic int g_panel;
int dm_uiscreen_panel(void) { return atomic_load(&g_panel); }
void dm_uiscreen_set_panel(int on) { if (atomic_exchange(&g_panel, on != 0) != (on != 0) && atomic_load(&g_ingame) && atomic_load(&g_rw)) LOG("界面画面：%s", on ? "开大面板 → 界面移到正中" : "大面板关了 → 界面回落"); }
static int yoff(void) { int rh = atomic_load(&g_rh), uh = atomic_load(&g_uh), m = mode(); if (!atomic_load(&g_rw) || rh <= uh || m == DM_UISCREEN_OFF) return 0;
    if (!atomic_load(&g_ingame) || atomic_load(&g_panel)) m = DM_UISCREEN_CENTER; return m == DM_UISCREEN_CENTER ? (rh - uh) / 2 : m == DM_UISCREEN_BOTTOM ? rh - uh : 0; }









static _Atomic int g_proj_on;
static void ui_project_obs(const uint64_t *regs) {
    int dy = atomic_load(&g_ingame) ? yoff() : 0; const uint8_t *c = (const uint8_t *)(uintptr_t)regs[0]; if (!c || c[0x160]) return;



    int dm_cam_label_matrix(const float p[16], float out[16]);
    float pm[16]; int cam = dm_cam_label_matrix((const float *)(c + 0x80), pm); if (!dy && !cam) return;
    float H = *(const float *)(c + 0x144); if (dy && !(H > 0)) return;







    static _Alignas(16) uint8_t ring[64][0x170]; static _Atomic unsigned slot;
    uint8_t *copy = ring[atomic_fetch_add(&slot, 1) & 63]; memcpy(copy, c, sizeof ring[0]); float *m = (float *)(copy + 0x80);
    if (cam) memcpy(m, pm, sizeof pm);
    if (dy) { float k = 2.0f * dy / H; for (int j = 0; j < 4; j++) m[j * 4 + 1] += k * m[j * 4 + 3]; }
    ((uint64_t *)(uintptr_t)regs)[0] = (uint64_t)(uintptr_t)copy;
}
int dm_ui_proj_dy(void) { return atomic_load(&g_proj_on) && atomic_load(&g_ingame) ? yoff() : 0; }



int dm_ui_proj_gate(void) { return atomic_load(&g_proj_on); }







uint64_t front_game_res(int docked); void dm_oe_res_changed(void);




void dm_uiscreen_res_reread(void) { uint8_t *b = main_base(); int rw = atomic_load(&g_rw); if (!b || !rw) return; int32_t *g = (int32_t *)(b + GA_UI_WIDTH);
    if (g[0] == rw && g[1] == atomic_load(&g_uh)) g[1] = atomic_load(&g_rh); atomic_store(&g_rw, 0); }




static _Atomic int g_world, g_res_live;

int dm_game_win_init_size(int *w, int *h) { uint8_t *b = main_base(); if (!b) return 0; *w = *(int32_t *)(b + GA_OUTPUT_RES_CAP); *h = *(int32_t *)(b + GA_OUTPUT_RES_CAP_H); return *w > 0 && *h > 0; }
int dm_uiscreen_res_live(void) { return atomic_load(&g_res_live); }










static int ui_floor(int uh, int rh) {
    static int once, min_h = -1;
    if (!once) { once = 1; const char *e = getenv("DM_UI_MIN_H"); min_h = e ? atoi(e) : 720; if (min_h < 0) min_h = 0; }
    if (min_h && uh < min_h) uh = min_h < rh ? min_h : rh;
    if (uh > rh) uh = rh;
    return uh;
}
uint64_t dm_game_res(int docked) { uint64_t r = front_game_res(docked); uint32_t w = (uint32_t)r, h = (uint32_t)(r >> 32);
    if (atomic_load(&g_res_live) && mode() != DM_UISCREEN_OFF && !atomic_load(&g_world) && (uint64_t)w * 9 < (uint64_t)h * 16) h = (uint32_t)ui_floor((int)(((uint64_t)w * 9 / 16) & ~1u), (int)h);
    return (uint64_t)h << 32 | w; }

void dm_uiscreen_pump(void) {
    uint8_t *b = main_base(); if (!b) return;
    { int world = ((uint64_t (*)(uint32_t))(b + GA_CLIENT_PLAYER_GET))(*(uint32_t *)(b + GA_LOCAL_PLAYER_SLOT)) != 0;
      int first = !atomic_exchange(&g_res_live, 1);
      if ((atomic_exchange(&g_world, world) != world || (first && !world)) && mode() != DM_UISCREEN_OFF) { LOG("界面画面：%s → 游戏分辨率切到 %u x %u", world ? "玩家单位建立" : first ? "窗口已建好、在前端" : "玩家单位销毁", (unsigned)dm_game_res(1), (unsigned)(dm_game_res(1) >> 32)); dm_oe_res_changed(); } }
    static int reg; if (!reg) { reg = 1; if (gate_obs_present(OBS_UI_PROJECT)) { gate_obs_set(OBS_UI_PROJECT, ui_project_obs); atomic_store(&g_proj_on, 1); } } int32_t *g = (int32_t *)(b + GA_UI_WIDTH); int m = mode(), rw = atomic_load(&g_rw);
    if (!atomic_load(&g_world) && m != DM_UISCREEN_OFF) return;
    if (m == DM_UISCREEN_OFF) { if (rw) { if (g[0] == rw && g[1] == atomic_load(&g_uh)) g[1] = atomic_load(&g_rh); atomic_store(&g_rw, 0); LOG("界面画面：关掉，屏幕高写回 %d", g[1]); } return; }
    int w = g[0], h = g[1]; if (w <= 0 || h <= 0) return;
    if (rw && w == rw && h == atomic_load(&g_uh)) return;
    static int n; int uh = h; if ((long)w * 9 < (long)h * 16) { uh = (int)((long)w * 9 / 16) & ~1; uh = ui_floor(uh, h); g[1] = uh; }
    atomic_store(&g_rh, h); atomic_store(&g_uh, uh); atomic_store(&g_rw, w);
    LOG("界面画面：游戏写入屏幕宽高 %d x %d（第 %d 次）→ 界面用 %d x %d，纵向位置 %d", w, h, ++n, w, uh, yoff());
}
int dm_screen_real(int *w, int *h) { int rw = atomic_load(&g_rw);
    if (rw) { *w = rw; *h = atomic_load(&g_rh); return 1; }
    uint8_t *b = main_base(); if (!b) return 0; *w = *(int32_t *)(b + GA_UI_WIDTH); *h = *(int32_t *)(b + GA_UI_HEIGHT); return *w > 0 && *h > 0; }
void dm_ui_rect(int *x, int *y, int *w, int *h) { int rw = 0, rh = 0; dm_screen_real(&rw, &rh); *x = 0; *y = yoff(); *w = rw; *h = atomic_load(&g_rw) ? atomic_load(&g_uh) : rh; }
float dm_ui_ny(float ny) { int x, y, w, h, rw = 0, rh = 0; dm_ui_rect(&x, &y, &w, &h); if (!dm_screen_real(&rw, &rh) || h <= 0 || h == rh) return ny; return (ny * rh - y) / h; }
float dm_ui_ny_back(float ny) { int x, y, w, h, rw = 0, rh = 0; dm_ui_rect(&x, &y, &w, &h); if (!dm_screen_real(&rw, &rh) || rh <= 0 || h == rh) return ny; return (ny * h + y) / rh; }
