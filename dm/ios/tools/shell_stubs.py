#!/usr/bin/env python3


















import os, re, sys

CSTR = r'"((?:[^"\\]|\\.)*)"'


def unq(s): return bytes(s, 'utf-8').decode('unicode_escape').encode('latin-1').decode('utf-8') if '\\' in s else s


def paths(dm, gen=None):
    if gen: return tuple(os.path.join(gen, f) for f in ('dm_hud_exp.h', 'dm_vault_tab.h', 'dm_touch_native_art.c'))
    return (os.path.join(dm, 'host', 'dm_hud_exp.h'), os.path.join(dm, 'host', 'dm_vault_tab.h'), os.path.join(dm, 'ios', 'Sources', 'dm_touch_native_art.c'))


def parse(dm, gen=None):
    hud, vault, art = (open(p, encoding='utf-8').read() for p in paths(dm, gen))
    exp = [int(x) for x in re.search(r'k_exp\[100\] = \{(.*?)\};', hud, re.S).group(1).replace('\n', ' ').split(',') if x.strip()]
    assert len(exp) == 100, len(exp)
    parts = [unq(x) for x in re.findall(CSTR, re.search(r'vt_part_name\[VT_NPART\] = \{(.*?)\};', vault).group(1))]
    rows = [(int(a), int(b), int(c), int(d), int(e), int(f), unq(k), unq(i)) for a, b, c, d, e, f, k, i in
            re.findall(r'^\s*\{ (\d+), (\d+), (\d+), (\d+), (\d+), (-?\d+), %s, %s \},$' % (CSTR, CSTR), vault, re.M)]
    sets = [unq(x) for x in re.findall(CSTR, re.search(r'vt_set_key\[\] = \{(.*?)\};', vault).group(1))]
    assert rows and sets and len(parts) == int(re.search(r'#define VT_NPART (\d+)', vault).group(1))
    assert vault.count('\n    { ') == len(rows), (vault.count('\n    { '), len(rows))
    m = re.search(r'static const struct \{ uint32_t id; int atlas,frame; \} rows\[\]=\{\n(.*?)\n\};', art, re.S)
    trows = [tuple(int(x) for x in t) for t in re.findall(r'^ \{(\d+),(-?\d+),(-?\d+)\},$', m.group(1), re.M)]
    assert len(trows) == m.group(1).count('\n') + 1, (len(trows), m.group(1).count('\n') + 1)
    return exp, parts, rows, sets, trows, (hud, vault, art, m)


def data(dm, dst, gen=None):
    exp, parts, rows, sets, trows, _ = parse(dm, gen)
    L = ['EXP\t' + '\t'.join(map(str, exp[i:i + 10])) for i in range(0, 100, 10)]
    L += ['VTPART\t' + p for p in parts] + ['VTROW\t' + '\t'.join(map(str, r)) for r in rows] + ['VTSET\t' + s for s in sets]
    L += ['TAROW\t%d\t%d\t%d' % t for t in trows]
    for l in L: assert '\n' not in l and l.count('\t') < 16
    for r in rows: assert '\t' not in r[6] and '\t' not in r[7]
    with open(dst, 'w', encoding='utf-8', newline='\n') as f: f.write('\n'.join(L) + '\n')
    return len(exp), len(parts), len(rows), len(sets), len(trows)


def stubs(dm, gen=None):
    exp, parts, rows, sets, trows, (hud, vault, art, m) = parse(dm, gen)
    p_hud, p_vault, p_art = paths(dm, gen)
    open(p_hud, 'w', encoding='utf-8').write('''// 壳的替身（ios/tools/shell_stubs.py 生成）：经验表不编进代码，启动时从安装包的 dm/derived.tsv 读（标签 EXP，每行 10 个）
#pragma once
#include <stdint.h>
#include "dm_derived_rt.h"
static uint32_t k_exp[100];
static void k_exp_row(char **f, int n, void *ctx) { int *i = ctx; for (int k = 0; k < n && *i < 100; k++) k_exp[(*i)++] = (uint32_t)strtoul(f[k], NULL, 10); }
static int k_exp_load(void) { int i = 0; dm_derived_each("EXP", k_exp_row, &i); return i == 100 ? 0 : -1; }
__attribute__((constructor)) static void k_exp_reg(void) { dm_derived_register(k_exp_load); }
''')
    open(p_vault, 'w', encoding='utf-8').write('''// 壳的替身（ios/tools/shell_stubs.py 生成）：编年史仓库固定表不编进代码，启动时从安装包的 dm/derived.tsv 读
// （标签 VTPART / VTROW / VTSET）。条数随游戏版本固定，写成数组大小；内容全在数据文件里。
#include "dm_derived_rt.h"
#define VT_NPART %d
static const char *vt_part_name[VT_NPART];
typedef struct { uint8_t kind, part, tier; uint16_t row, lvl; int16_t set; const char *key, *icon; } vt_row;
static vt_row vt_rows[%d];
#define VT_NROW (int)(sizeof vt_rows / sizeof vt_rows[0])
static const char *vt_set_key[%d];
#define VT_NSET (int)(sizeof vt_set_key / sizeof vt_set_key[0])
static void vt_part_cb(char **f, int n, void *ctx) { int *i = ctx; if (n >= 1 && *i < VT_NPART) vt_part_name[(*i)++] = strdup(f[0]); }
static void vt_set_cb(char **f, int n, void *ctx) { int *i = ctx; if (n >= 1 && *i < VT_NSET) vt_set_key[(*i)++] = strdup(f[0]); }
static void vt_row_cb(char **f, int n, void *ctx) { int *i = ctx; if (n < 8 || *i >= VT_NROW) return;
    vt_rows[(*i)++] = (vt_row){ (uint8_t)atoi(f[0]), (uint8_t)atoi(f[1]), (uint8_t)atoi(f[2]), (uint16_t)atoi(f[3]), (uint16_t)atoi(f[4]), (int16_t)atoi(f[5]), strdup(f[6]), strdup(f[7]) }; }
static int vt_load(void) { int a = 0, b = 0, c = 0;
    dm_derived_each("VTPART", vt_part_cb, &a); dm_derived_each("VTROW", vt_row_cb, &b); dm_derived_each("VTSET", vt_set_cb, &c);
    for (int i = a; i < VT_NPART; i++) vt_part_name[i] = ""; for (int i = c; i < VT_NSET; i++) vt_set_key[i] = "";
    for (int i = b; i < VT_NROW; i++) vt_rows[i] = (vt_row){ .set = -1, .key = "", .icon = "" };
    if (a == VT_NPART && b == VT_NROW && c == VT_NSET) return 0;
    fprintf(stderr, "[dm] 【红】编年史仓库表没读全：部位 %%d / %%d、条目 %%d / %%d、套装 %%d / %%d\\n", a, VT_NPART, b, VT_NROW, c, VT_NSET); return -1; }
__attribute__((constructor)) static void vt_reg(void) { dm_derived_register(vt_load); }
''' % (len(parts), len(rows), len(sets)))
    stub = art[:m.start()] + '''static struct { uint32_t id; int atlas,frame; } rows[%d];   /* 壳的替身：技能索引不编进代码，启动时从安装包的 dm/derived.tsv 读（标签 TAROW） */
static void ta_row_cb(char **f, int n, void *ctx) { int *i = ctx; if (n < 3 || *i >= (int)(sizeof rows / sizeof rows[0])) return;
    rows[*i].id = (uint32_t)strtoul(f[0], NULL, 10); rows[*i].atlas = atoi(f[1]); rows[*i].frame = atoi(f[2]); (*i)++; }
static int ta_load(void) { int i = 0; dm_derived_each("TAROW", ta_row_cb, &i);
    for (int k = i; k < (int)(sizeof rows / sizeof rows[0]); k++) { rows[k].id = UINT32_MAX - 1; rows[k].atlas = rows[k].frame = -1; }
    if (i == (int)(sizeof rows / sizeof rows[0])) return 0;
    fprintf(stderr, "[dm] 【红】技能图标索引没读全：%%d / %%d\\n", i, (int)(sizeof rows / sizeof rows[0])); return -1; }
__attribute__((constructor)) static void ta_reg(void) { dm_derived_register(ta_load); }''' % len(trows) + art[m.end():]
    stub = stub.replace('#include <stddef.h>', '#include <stddef.h>\n#include "%s"' % ('dm_derived_rt.h' if gen else '../../host/dm_derived_rt.h'), 1); assert 'dm_derived_rt.h' in stub
    open(p_art, 'w', encoding='utf-8').write(stub)
    return len(parts), len(rows), len(sets), len(trows)


if __name__ == '__main__':
    if len(sys.argv) == 4 and sys.argv[1] == 'data': print('derived.tsv：经验 %d、部位 %d、仓库条目 %d、套装 %d、技能索引 %d' % data(sys.argv[2], sys.argv[3]))
    elif len(sys.argv) == 3 and sys.argv[1] == 'stubs': print('替身已写：部位 %d、仓库条目 %d、套装 %d、技能索引 %d' % stubs(sys.argv[2]))
    elif len(sys.argv) == 4 and sys.argv[1] == 'data-gen': print('derived.tsv：经验 %d、部位 %d、仓库条目 %d、套装 %d、技能索引 %d' % data(None, sys.argv[3], sys.argv[2]))
    elif len(sys.argv) == 3 and sys.argv[1] == 'stubs-gen': print('替身已写：部位 %d、仓库条目 %d、套装 %d、技能索引 %d' % stubs(None, sys.argv[2]))
    else: sys.exit('用法：shell_stubs.py data <dm 目录> <输出 derived.tsv> | stubs <dm 目录> | data-gen <生成目录> <输出 derived.tsv> | stubs-gen <生成目录>')
