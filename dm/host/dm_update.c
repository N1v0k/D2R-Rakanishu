













#include "hle.h"
#include "dm.h"
#include "dm_i18n.h"
#include "dm_cursor.h"
#include <string.h>
#include <stdio.h>
#include <stdatomic.h>
#include <pthread.h>
#include <TargetConditionals.h>

static int g_own = -1; static void (*g_on_select)(void), (*g_on_click)(void);
static pthread_mutex_t g_mu = PTHREAD_MUTEX_INITIALIZER; static struct { int build, urgent; char note[160]; } g_offer;
static _Atomic int g_safe_l = -1, g_dirty, g_click, g_appear, g_fg;

void dm_update_init(int own_build, void (*on_select)(void), void (*on_click)(void)) { g_own = own_build; g_on_select = on_select; g_on_click = on_click; atomic_store(&g_dirty, 30); }


void dm_update_offer(int new_build, const char *note, int urgent) {
    pthread_mutex_lock(&g_mu); g_offer.build = new_build; g_offer.urgent = urgent; strlcpy(g_offer.note, note ? note : "", sizeof g_offer.note); pthread_mutex_unlock(&g_mu);
    atomic_store(&g_dirty, 30);
}






static _Atomic int g_fail;
static char g_site[96];
void dm_update_site(const char *host) { snprintf(g_site, sizeof g_site, "%s", host ? host : ""); }
void dm_update_fail(int kind) { if (atomic_exchange(&g_fail, kind) != kind) atomic_store(&g_dirty, 30); }


void dm_update_safe_left(int units) { atomic_store(&g_safe_l, units < 0 ? 0 : units > 800 ? 800 : units); atomic_store(&g_dirty, 30); }


void dm_update_foreground(void) { if (g_own >= 0) atomic_store(&g_fg, 1); }


void dm_update_mode_enter(void) { if (g_own >= 0) atomic_store(&g_dirty, 30); }
void dm_update_on_uimsg(const char *cat, const char *act, const char *arg) {
    if (g_own < 0 || strcmp(cat, "PanelManager")) return;
    if (!strcmp(act, "ClosePanel") && !strcmp(arg, "TouchUpdate")) atomic_store(&g_click, 1);
    else if (!strcmp(act, "OpenPanel") && (!strcmp(arg, "MainMenuPanel") || !strcmp(arg, "CharacterSelectPanel"))) { atomic_store(&g_dirty, 30);
        if (arg[0] == 'C') atomic_store(&g_appear, 1); }
}
static uint8_t *tb(void) { static uint8_t *b; if (!b) for (int i = 0; i < g_nmod; i++) if (!strcmp(g_mod[i].name, "main")) b = (uint8_t *)g_mod[i].base; return b; }
static uint64_t find(uint64_t w, const char *name, int dep) {
    if (!w || dep > 6) return 0; const char *nm = *(const char **)(w + 8); if (nm && !strcmp(nm, name)) return w;
    uint64_t ch = *(uint64_t *)(w + 0x58), n = *(uint64_t *)(w + 0x60); if (!ch || n > 256) return 0;
    for (uint64_t i = 0; i < n; i++) { uint64_t r = find(*(uint64_t *)(ch + 8 * i), name, dep + 1); if (r) return r; }
    return 0;
}
static void set_text(uint8_t *b, uint64_t w, const char *s);
static void place(uint64_t w, int x, int y) {
    int32_t *r = (int32_t *)(w + 0x70); if (r[0] == x && r[1] == y) return; r[0] = x; r[1] = y; ((void (*)(uint64_t))*(uint64_t *)(*(uint64_t *)w + 0x10))(w);
}
void dm_update_pump(void) {
    if (g_own < 0) return;
    if (atomic_exchange(&g_click, 0)) { LOG("更新：点了「更新」按钮"); if (g_on_click) g_on_click(); }


    { int dm_padmode_current(void), dm_padmode_busy(void); uint64_t dm_padmode_page_keys(uint64_t);
      uint8_t *pb = tb(); uint64_t pr = pb ? *(uint64_t *)(pb + 0x2f05458) : 0; int have; pthread_mutex_lock(&g_mu); have = g_offer.build > 0; pthread_mutex_unlock(&g_mu);
      if (have && pr && dm_padmode_current() && !dm_padmode_busy()) { uint64_t mm = find(pr, "MainMenuPanel", 0), lg = mm ? find(mm, "MainMenuLegend", 0) : 0, cs = find(pr, "CharacterSelectPanel", 0), cc = find(pr, "CharacterCreatePanel", 0), st = find(pr, "SettingsPanel", 0);
          if (mm && *(uint8_t *)(mm + 0x50) && lg && find(mm, "TouchBuild", 0) && !(cs && *(uint8_t *)(cs + 0x50)) && !(cc && *(uint8_t *)(cc + 0x50)) && !(st && *(uint8_t *)(st + 0x50))) {
              if (dm_padmode_page_keys(0x8) & 0x8) { LOG("更新：手柄主菜单按了 Y（更新）"); if (g_on_click) g_on_click(); } } } }
    if (atomic_exchange(&g_fg, 0)) {
        uint8_t *fb = tb(); uint64_t fr = fb ? *(uint64_t *)(fb + 0x2f05458) : 0, fm = fr ? find(fr, "MainMenuPanel", 0) : 0, fl = fr ? find(fr, "CharacterSelectPanel", 0) : 0;
        if (fm && find(fm, "TouchBuild", 0) && fl && *(uint8_t *)(fl + 0x50)) { atomic_store(&g_appear, 1); atomic_store(&g_dirty, 30); LOG("更新：回到前台时停在选角页，重查一次新版本"); } }
    int left = atomic_load(&g_dirty); if (left <= 0) return;
    uint8_t *b = tb(); uint64_t root = b ? *(uint64_t *)(b + 0x2f05458) : 0, mm = root ? find(root, "MainMenuPanel", 0) : 0, bw = mm ? find(mm, "TouchBuild", 0) : 0;
    if (!bw) return;
    if (atomic_exchange(&g_appear, 0) && g_on_select) g_on_select();
    if (atomic_load(&g_safe_l) < 0) return;
    atomic_compare_exchange_strong(&g_dirty, &left, left - 1);
    int build, urgent; char note[160]; pthread_mutex_lock(&g_mu); build = g_offer.build; urgent = g_offer.urgent; memcpy(note, g_offer.note, sizeof note); pthread_mutex_unlock(&g_mu);
    char t[256]; snprintf(t, sizeof t, dm_str(b, "dmUpdBuild"), g_own); set_text(b, bw, t);
    int sl = atomic_load(&g_safe_l), bx = sl + 6 > 79 ? sl + 6 : 79; int dm_padmode_current(void); int pad = dm_padmode_current();




    int vw = *(int32_t *)(b + 0x211da38), vh = *(int32_t *)(b + 0x211da3c), xr = vh > 0 ? (int)((long)vw * 2160 / vh) : 3840;
    if (pad) place(bw, xr - 420 - bx, 60); else place(bw, bx, 2046);


    uint64_t nt = find(mm, "TouchUpdNote", 0), sm = find(mm, "TouchUpdSmall", 0), bg = find(mm, "TouchUpdBg", 0), ti = find(mm, "TouchUpdTitle", 0), de = find(mm, "TouchUpdDesc", 0), big = find(mm, "TouchUpdBig", 0);
    if (!nt || !sm || !bg || !ti || !de || !big) { LOG("【红】更新：主菜单面板里的提示控件不全，不显示提示"); return; }
    int normal = build > 0 && !urgent, urg = build > 0 && urgent, fail = build > 0 ? 0 : atomic_load(&g_fail);
    snprintf(t, sizeof t, dm_str(b, "dmUpdNew"), build, note);
    char ft[192]; if (g_site[0]) snprintf(ft, sizeof ft, dm_str(b, "dmUpdFailSite"), g_site); else snprintf(ft, sizeof ft, "%s", dm_str(b, "dmUpdFail"));
    if (pad && normal) { size_t L = strlen(t); snprintf(t + L, sizeof t - L, "   \xee\x80\x91 %s", dm_str(b, "dmUpdButton")); }
    set_text(b, nt, normal ? t : fail == 1 ? dm_str(b, "dmUpdNoNet") : fail ? ft : "");
    snprintf(t, sizeof t, dm_str(b, "dmUpdNewUrgent"), build, note); if (pad && urg) { size_t L = strlen(t); snprintf(t + L, sizeof t - L, "   \xee\x80\x91 %s", dm_str(b, "dmUpdButton")); } set_text(b, de, urg ? t : ""); set_text(b, ti, urg ? dm_str(b, "dmUpdUrgentTitle") : "");
    if (pad) { place(bg, urg ? xr / 2 - 700 : 99999, urg ? 1506 : 99999); place(ti, xr / 2 - 700, 1524); place(de, xr / 2 - 700, 1610); }
    else { place(bg, urg ? -700 : 99999, urg ? 1506 : 99999); place(big, urg ? -303 : 99999, urg ? 1712 : 99999); }


    int nw = *(int32_t *)(nt + 0x78);
    if (pad) { place(nt, xr - 2000 - bx, 130); place(sm, 99999, 99999); place(big, 99999, 99999); }
    else { place(nt, bx + 341, 2046); place(sm, normal ? bx + 341 + nw + 34 : 99999, normal ? 2030 : 99999); }
    static dm_wref lw; static int ls = -1, lnw; int st = normal ? 1 : urg ? 2 : fail ? 2 + fail : 0;
    if (dm_wref_get(&lw) != bw || ls != st || (normal && lnw != nw)) { dm_wref_set(&lw, bw); ls = st; lnw = nw; LOG("更新：选角页写上构建 %d%s（提示字宽 %d）", g_own, normal ? "，普通更新提示" : urg ? "，建议立即更新提示" : fail == 1 ? "，查更新失败提示（无线数据许可）" : fail ? "，查更新失败提示（浏览器手动更新）" : "，无提示", normal ? nw : 0); }
}
#if TARGET_OS_OSX






int dm_update_click_at(float nx, float ny) {
    if (g_own < 0) return 0; int have, dm_padmode_busy(void); pthread_mutex_lock(&g_mu); have = g_offer.build > 0 && !g_offer.urgent; pthread_mutex_unlock(&g_mu);
    uint8_t *b = tb(); uint64_t pr = b ? *(uint64_t *)(b + 0x2f05458) : 0; if (!have || !pr || dm_padmode_busy()) return 0;
    uint64_t mm = find(pr, "MainMenuPanel", 0), cs = find(pr, "CharacterSelectPanel", 0), cc = find(pr, "CharacterCreatePanel", 0), st = find(pr, "SettingsPanel", 0);
    if (!mm || !*(uint8_t *)(mm + 0x50) || !find(mm, "TouchBuild", 0) || (cs && *(uint8_t *)(cs + 0x50)) || (cc && *(uint8_t *)(cc + 0x50)) || (st && *(uint8_t *)(st + 0x50))) return 0;
    int vw = *(int32_t *)(b + 0x211da38), vh = *(int32_t *)(b + 0x211da3c), xr = vh > 0 ? (int)((long)vw * 2160 / vh) : 3840, sl = atomic_load(&g_safe_l), bx = sl + 6 > 79 ? sl + 6 : 79;
    float x = nx * xr, y = ny * 2160; if (x < xr - 1400 - bx || x > xr - bx + 20 || y < 100 || y > 230) return 0;
    LOG("更新：鼠标点了主菜单的新版本提示"); if (g_on_click) g_on_click(); return 1;
}
#endif




static void set_text(uint8_t *b, uint64_t w, const char *s) {
    if (*(uint64_t *)w - (uint64_t)(uintptr_t)b != 0x20ba4c8) return;
    size_t n = strlen(s); if (*(uint64_t *)(w + 0x90) == n && !memcmp(*(const char **)(w + 0x88), s, n)) return;
    ((void *(*)(uint64_t, const char *, uint64_t))(b + 0x136e5c))(w + 0x88, s, n); ((void (*)(uint64_t))(b + 0xadc434))(w);
}
