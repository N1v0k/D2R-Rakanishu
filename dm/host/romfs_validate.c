#include "romfs_validate.h"
#include "dm_i18n.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

static int count_files(const char *root) {
    DIR *d = opendir(root); if (!d) return -1;
    int n = 0; struct dirent *e;
    while ((e = readdir(d))) {
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
        char p[4096]; snprintf(p, sizeof p, "%s/%s", root, e->d_name);
        struct stat st; if (stat(p, &st)) { closedir(d); return -1; }
        n += S_ISDIR(st.st_mode) ? count_files(p) : 1;
        if (n < 0) { closedir(d); return -1; }
    }
    closedir(d); return n;
}










static const char *need[] = {
    "00-00000001.data", "00-00000002.data", "00-00000003.data", "00-00000004.data", "00-00000005.data", "00-00000006.data",
    "01-00000001.data", "01-00000002.data", "02-00000001.data", "02-00000002.data", "03-00000001.data", "03-00000002.data",
    "04-00000001.data", "04-00000002.data", "05-00000001.data", "05-00000002.data", "17-00000001.data", "17-00000002.data"};






static const long long size[] = {
    1073741824, 1073741821, 1026342790, 3664822, 391990840, 3794407,
    344377972, 37189352, 368762692, 23206186, 251620752, 55071917,
    118831405, 8915855, 478931036, 92048235, 204495617, 4496514};
#define NEED_N (sizeof need / sizeof *need)



static int need_state(const char *dir, size_t i, long long *have, long long *want) {
    char p[4096]; snprintf(p, sizeof p, "%s/%s", dir, need[i]); struct stat st; *have = 0; *want = size[i];
    if (stat(p, &st) || !S_ISREG(st.st_mode)) return 1;
    char hp[4200]; snprintf(hp, sizeof hp, "%s.holes", p); FILE *hf = fopen(hp, "r");
    if (hf) { long long lsz = -1, o, l; if (fscanf(hf, "size %lld", &lsz) != 1 || lsz != size[i]) { fclose(hf); return 3; }
        while (fscanf(hf, "%lld %lld", &o, &l) == 2) *want -= l; fclose(hf); }
    *have = (long long)st.st_size; return *have == *want ? 0 : 2;
}
static void human(long long b, char *o, size_t n) { if (b < 1000000) snprintf(o, n, dm_ui("dmRvBytes"), b); else if (b < 1000000000) snprintf(o, n, "%.0f MB", b / 1048576.0); else snprintf(o, n, "%.2f GB", b / 1073741824.0); }


int dm_romfs_report(const char *dir, char *out, size_t out_n) {
    int bad = 0, ok = 0; char miss[1200] = "", part[800] = ""; long long have, want;
    for (size_t i = 0; i < NEED_N; i++) { int st = need_state(dir, i, &have, &want); char a[40], b[40]; human(want, a, sizeof a); human(have, b, sizeof b);
        if (!st) { ok++; continue; } bad++;
        if (st == 1) { if (strlen(miss) < sizeof miss - 80) snprintf(miss + strlen(miss), sizeof miss - strlen(miss), dm_ui("dmRvItem"), *miss ? dm_ui("dmRvSep") : "", need[i], a); }
        else if (strlen(part) < sizeof part - 100) snprintf(part + strlen(part), sizeof part - strlen(part), dm_ui("dmRvItem2"), *part ? dm_ui("dmRvSep") : "", need[i], st == 3 ? dm_ui("dmRvHolesBad") : b, a); }
    if (!bad) { snprintf(out, out_n, dm_ui("dmRvAllOk"), ok); return 0; }
    snprintf(out, out_n, dm_ui("dmRvReport"), ok, (int)NEED_N, *miss ? dm_ui("dmRvMissing") : "", miss, *miss ? dm_ui("dmRvPeriod") : "", *part ? dm_ui("dmRvPartial") : "", part, *part ? dm_ui("dmRvPeriod") : "");
    return bad;
}
int dm_romfs_validate(const char *dir, char *why, size_t why_n) {
    for (size_t i = 0; i < NEED_N; i++) { long long have, want; int st = need_state(dir, i, &have, &want);
        if (st == 1) { snprintf(why, why_n, dm_ui("dmRvNoFile"), need[i]); return 0; }
        if (st == 3) { snprintf(why, why_n, dm_ui("dmRvHoles"), need[i]); return 0; }
        if (st == 2) { snprintf(why, why_n, dm_ui("dmRvShort"), need[i], have, want); return 0; }
    }
    snprintf(why, why_n, "档案 %d 个齐、字节数对；目录里共 %d 个文件", (int)NEED_N, count_files(dir));
    return 1;
}
