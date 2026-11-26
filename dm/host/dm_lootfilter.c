































#include "dm.h"
#include "game_addrs.h"
#include "dm_custom.h"
#include <dirent.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define T_MISSILE 47
int dm_diag_difficulty(void);







static void write_fltr(const char *dir, const char *name, int heavy, int ammo) {
    char p[1300], t[1320]; snprintf(p, sizeof p, "%s/%s.fltr", dir, name); snprintf(t, sizeof t, "%s.tmp", p);
    FILE *f = fopen(t, "w"); if (!f) { LOG("【红】掉落过滤：规则文件写不了 %s", p); return; }
    fprintf(f, "{\n \"name\": \"%s\",\n \"rules\": [\n", name);
    fprintf(f, "  {\"name\": \"HIDE consumables\", \"enabled\": true, \"ruleType\": \"hide\", \"filterEtherealSocketed\": false, \"itemCode\": [\"vps\", \"yps\", \"wms\", \"hp1\", \"hp2\", \"mp1\", \"mp2\", %s\"tsc\", \"isc\", \"tbk\", \"ibk\"]},\n", heavy ? "\"hp3\", \"mp3\", " : "");
    fprintf(f, "  {\"name\": \"HIDE throwing potions\", \"enabled\": true, \"ruleType\": \"hide\", \"filterEtherealSocketed\": false, \"equipmentItemCode\": [\"gps\", \"gpm\", \"gpl\", \"ops\", \"opm\", \"opl\"]},\n");
    if (ammo) fprintf(f, "  {\"name\": \"HIDE ammo\", \"enabled\": true, \"ruleType\": \"hide\", \"filterEtherealSocketed\": false, \"itemCategory\": [\"ammo\"]},\n");
    if (heavy) {
        fprintf(f, "  {\"name\": \"SHOW socketed or ethereal\", \"enabled\": true, \"ruleType\": \"show\", \"filterEtherealSocketed\": true},\n");
        fprintf(f, "  {\"name\": \"SHOW normal circlets\", \"enabled\": true, \"ruleType\": \"show\", \"filterEtherealSocketed\": false, \"equipmentRarity\": [\"normal\", \"hiQuality\"], \"equipmentCategory\": [\"circl\"]},\n");
        fprintf(f, "  {\"name\": \"SHOW magic accessories\", \"enabled\": true, \"ruleType\": \"show\", \"filterEtherealSocketed\": false, \"equipmentRarity\": [\"magic\"], \"equipmentCategory\": [\"acce\"]},\n");
        fprintf(f, "  {\"name\": \"SHOW magic circlets\", \"enabled\": true, \"ruleType\": \"show\", \"filterEtherealSocketed\": false, \"equipmentRarity\": [\"magic\"], \"equipmentCategory\": [\"circl\"]},\n");
        fprintf(f, "  {\"name\": \"SHOW magic class items\", \"enabled\": true, \"ruleType\": \"show\", \"filterEtherealSocketed\": false, \"equipmentRarity\": [\"magic\"], \"equipmentCategory\": [\"barbh\", \"druid\", \"palad\", \"necro\", \"warlo\", \"amazo\", \"assas\", \"sorce\"]},\n");
        fprintf(f, "  {\"name\": \"HIDE normal-tier white and magic\", \"enabled\": true, \"ruleType\": \"hide\", \"filterEtherealSocketed\": false, \"equipmentRarity\": [\"normal\", \"hiQuality\", \"magic\"], \"equipmentQuality\": [\"normal\"]},\n"); }
    fprintf(f, "  {\"name\": \"HIDE low quality\", \"enabled\": true, \"ruleType\": \"hide\", \"filterEtherealSocketed\": false, \"equipmentRarity\": [\"lowQuality\"]}\n ]\n}\n");
    int ok = fclose(f) == 0 && rename(t, p) == 0; if (!ok) LOG("【红】掉落过滤：规则文件没写成 %s", p);
}


void dm_lootfilter_seed(const char *save_dir) {
    char d[1100]; snprintf(d, sizeof d, "%s/Settings", save_dir); mkdir(d, 0755);
    write_fltr(d, "DmLite", 0, 1); write_fltr(d, "DmLiteBow", 0, 0); write_fltr(d, "DmHeavy", 1, 1); write_fltr(d, "DmHeavyBow", 1, 0);
}
__attribute__((constructor)) static void lootfilter_hook_install(void) { extern void (*g_dm_mount_seed_hook)(const char *); g_dm_mount_seed_hook = dm_lootfilter_seed; }
static const char *want(int bow, int *why_diff) {
    int diff = dm_diag_difficulty(), lvl = diff == 1 ? 1 : diff == 2 ? 2 : 0; *why_diff = diff;
    const char *e = getenv("DM_LOOTFILTER");
    if (e && !strcmp(e, "off")) lvl = 0; else if (e && !strcmp(e, "lite")) lvl = 1; else if (e && !strcmp(e, "heavy")) lvl = 2;
    else if (!dm_custom_get("loot_filter")) lvl = 0;
    return lvl == 0 ? "" : lvl == 1 ? (bow ? "DmLiteBow" : "DmLite") : (bow ? "DmHeavyBow" : "DmHeavy");
}


uint64_t dm_guest_frame(void);
void dm_lootfilter_pump(uint8_t *b, uint64_t pl) {
    static uint64_t next; uint64_t fr = dm_guest_frame(); if (!pl || fr < next) return; next = fr + 30;
    uint64_t F = (uint64_t)(uintptr_t)(b + GA_LOOT_FILTER); if (!*(uint64_t *)(F + 0x10)) return;
    uint64_t inv = *(uint64_t *)(pl + 0x90), it = inv && *(uint32_t *)inv == 0x01020304 ? *(uint64_t *)(inv + 0x10) : 0; int bow = 0;
    for (int k = 0; it && k < 512 && !bow; k++) { uint64_t d = *(uint64_t *)(it + 0x10); if (!d) break;
        if (*(uint32_t *)(it + 0xc) == 1 && ((int (*)(uint64_t, int))(b + GA_ITEM_IS_TYPE))(it, T_MISSILE)) bow = 1;
        it = *(uint64_t *)(d + 0xb0); }
    if (getenv("DM_TEST_LOOT_BOW")) bow = 1;
    int diff = -1; const char *w = want(bow, &diff); size_t wl = strlen(w);
    char *cur = (char *)(uintptr_t)*(uint64_t *)(F + 0x20); uint64_t len = *(uint64_t *)(F + 0x28), cap = *(uint64_t *)(F + 0x30) & 0x7fffffffffffffffull;
    if (!cur || (len == wl && !memcmp(cur, w, wl))) return;
    if (wl > cap || len > cap) { static int n; if (n++ < 3) LOG("【红】掉落过滤：当前配置名的缓冲放不下（容量 %llu、现长 %llu），没改", (unsigned long long)cap, (unsigned long long)len); return; }
    char old[24]; size_t ol = len < sizeof old - 1 ? (size_t)len : sizeof old - 1; memcpy(old, cur, ol); old[ol] = 0;
    memcpy(cur, w, wl); cur[wl] = 0; *(uint64_t *)(F + 0x28) = wl;
    LOG("掉落过滤：难度 %d、身上%s弓弩 → 当前配置「%s」→「%s」", diff, bow ? "有" : "没有", old, w[0] ? w : "（不过滤）");
}


int dm_lootfilter_hidden(uint8_t *b, uint64_t item) { return item && !(((int (*)(uint64_t, uint64_t))(b + GA_LOOTFILTER_VISIBLE))((uint64_t)(uintptr_t)(b + GA_LOOT_FILTER), item) & 1); }


uint64_t dm_guest_frame(void);
void dm_lootfilter_test_dump(uint8_t *b) {
    static long at = -1; if (at < 0) { const char *e = getenv("DM_TEST_LOOTDUMP"); at = e ? atol(e) : 0; } if (at <= 0 || dm_guest_frame() < (uint64_t)at) return; at = 0;
    int n = 0, hid = 0;
    for (int k = 0; k < 128; k++) for (uint64_t it = *(uint64_t *)(b + GA_CLIENT_UNIT_TABLE + 4 * 0x400 + k * 8); it; it = *(uint64_t *)(it + 0x158)) {
        if (*(uint32_t *)(it + 0xc) != 3) continue; uint64_t d = *(uint64_t *)(it + 0x10); uint32_t code = ((uint32_t (*)(uint64_t))(b + GA_ITEM_CODE))(it); int h = dm_lootfilter_hidden(b, it); n++; hid += h;
        LOG("掉落过滤读数：%.3s 稀有度 %u 标志 %#x → %s", (const char *)&code, d ? *(uint32_t *)d : 0, d ? *(uint32_t *)(d + 0x18) : 0, h ? "藏" : "显示"); }
    LOG("掉落过滤读数：地上 %d 件，其中藏 %d 件", n, hid);
}
