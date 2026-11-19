#!/usr/bin/env python3












import os, re, sys

NATIVE = ['grid', 'gold_amount', 'gold_withdraw', 'gold_max', 'basicstash_container', 'advancedstash_gems', 'advancedstash_materials', 'advancedstash_runes',
          'advancedstash_horadriccube', 'SharedStashTabContainer', 'StashNavigation', 'CurrentSharedStashText', 'background', 'PreviousSeasonToggleDisplay']

def scan(text, where):
    bad = []
    for no, line in enumerate(text.split('\n'), 1):
        code = line.split('//', 1)[0]
        for nm in NATIVE:
            if re.search(r'"%s"' % re.escape(nm), code): bad.append('%s:%d 出现原版控件名「%s」：%s' % (where, no, nm, line.strip()[:120]))
    return bad

VIS = re.compile(r'\+\s*0x5[01]\)\s*=(?!=)')
GUARDED = ('ui_show_ours', 'vu_show', 'kb_show')
def vis_scan(text, where):
    bad, inside = [], None
    for no, line in enumerate(text.split('\n'), 1):
        m = re.match(r'static\s+\w+\s+(\w+)\(', line)
        if m: inside = m.group(1) if m.group(1) in GUARDED else None
        code = line.split('//', 1)[0]
        if VIS.search(code) and not inside: bad.append('%s:%d 直接写了可见位（要走 %s）：%s' % (where, no, ' / '.join(GUARDED), line.strip()[:120]))
        if line.startswith('}'): inside = None
    return bad





RECT_OK = ('mv_offset', 'ui_move_ours')
RECT_OFFS = ['0x7%x' % i for i in range(13)]


NATIVE_FIELD_OK = {
    ('dm_cursor.c', '0x19c'): (('ui_note_dock_pump', 'note_dock_restore'), 'NotificationPanel 停靠目标 y：原版只在重排时按布局抄进、滑入滑出按它插值写 +0x74；'
                               '改 +0x74 会被动画每帧盖回（试过）。原值我们自己记、停靠条件一结束当场写回（面板关 / 读取画面 / 保存并退出 / 回到前端）'),
    ('dm_cursor.c', '0x74'): (('note_dock_snap',), 'NotificationPanel 矩形 y：停靠目标切换（开 / 关面板）那一刻框已停稳（原版停稳不写坐标）才写成新目标一次，动画期不写；写后下一帧核'),
}
def field_write_scan(text, where, off, ok, what):
    ptr = re.compile(r'(?:int32_t|,)\s*\*\s*(\w+)\s*=\s*\(int32_t\s*\*\)\s*\([^;)]*\+\s*%s' % off)
    direct = re.compile(r'\*\s*\(int32_t\s*\*\)\s*\([^)]*\+\s*%s\)\s*=(?!=)' % off)
    bad, fn, ptrs = [], None, set()
    for no, line in enumerate(text.split('\n'), 1):
        m = re.match(r'(?:static\s+)?\w+\s+\*?(\w+)\(', line)
        if m and not line.startswith(' '): fn, ptrs = m.group(1), set()
        code = line.split('//', 1)[0]
        for pm in ptr.finditer(code): ptrs.add(pm.group(1))
        code = ptr.sub('', code)
        hit = direct.search(code) or any(re.search(r'(?<![\w.>])(?:\*\s*%s|%s\s*\[\s*\d\s*\])\s*=(?!=)' % (q, q), code) for q in ptrs)
        if hit and fn not in ok: bad.append('%s:%d 在 %s 里直接写了%s：%s' % (where, no, fn, what, line.strip()[:120]))
    return bad
def rect_scan(text, where):
    exc = [off for (f, off) in NATIVE_FIELD_OK if where.startswith(f) and off in RECT_OFFS]
    bad = field_write_scan(text, where, '(?:%s)\\b' % '|'.join(o for o in RECT_OFFS if o not in exc), RECT_OK, '控件矩形（原版控件走 mv_offset、我们的走 ui_move_ours）')
    for (f, off), (fns, why) in NATIVE_FIELD_OK.items():
        if where.startswith(f): bad += field_write_scan(text, where, re.escape(off) + r'\b', fns + (RECT_OK if off in RECT_OFFS else ()), '原版控件字段 +%s（登记例外只许在 %s 里写）' % (off, ' / '.join(fns)))
    return bad

def pages_pump(src):
    i = src.find('static void ui_pages_pump(')
    if i < 0: raise SystemExit('native_write_scan：dm_cursor.c 里找不到 ui_pages_pump（改名了要同步这里）')
    j = src.find('\n}\n', i)
    return src[i:j]

def main():
    if '--selftest' in sys.argv:
        if not scan('    vu_show(vu_find(bk, "grid", 2), 0);', 'selftest') or scan('    vu_show(vu_find(vt, "TouchV_Back", 0), 1);   // "grid" 只在注释里', 'selftest') \
           or not vis_scan('static void f(uint64_t w) {\n    *(uint8_t *)(w + 0x50) = 0;\n}', 'selftest') or vis_scan('static void ui_show_ours(uint64_t w, int on) {\n    *(uint8_t *)(w + 0x50) = 1;\n}\n    if (*(uint8_t *)(w + 0x50) == 1) x();', 'selftest') \
           or not rect_scan('static void f(uint64_t w) {\n    int32_t *r = (int32_t *)(w + 0x70); r[0] = 5;\n}', 'selftest') or not rect_scan('static void g(uint64_t w) {\n    *(int32_t *)(w + 0x74) = 3;\n}', 'selftest') \
           or rect_scan('static void ui_note_dock_pump(uint64_t w) {\n    int32_t *t = (int32_t *)(w + 0x19c), h = *(int32_t *)(w + 0x7c); *t = h;\n}', 'dm_cursor.c') \
           or not rect_scan('static void n(uint64_t w) {\n    int32_t *t = (int32_t *)(w + 0x19c); *t = 1;\n}', 'dm_cursor.c') \
           or not rect_scan('static void n2(uint64_t w) {\n    int32_t *t = (int32_t *)(w + 0x19c), *ry = (int32_t *)(w + 0x74); *ry = 1;\n}', 'dm_cursor.c') \
           or rect_scan('static void k(uint64_t w) {\n    int32_t *rx = (int32_t *)(w + 0x70), ww = *(int32_t *)(w + 0x78); if (*rx) x();\n}\nstatic void mv_offset(uint64_t w) {\n    int32_t *v = (int32_t *)(w + 0x70); *v = 1;\n}\nstatic int h(float *r) {\n    r[0] = 1.0f; int x = *(int32_t *)(w + 0x70); return x;\n}', 'selftest'):
            print('✗ native_write_scan 自检失败：该报的没报或不该报的报了'); return 1
        print('✓ native_write_scan 自检：违规行会报红、注释里的名字不算')
    host = next(a for a in sys.argv[1:] if not a.startswith('--'))
    bad = scan(open(os.path.join(host, 'dm_vault_ui.c'), encoding='utf-8').read(), 'dm_vault_ui.c')
    bad += scan(pages_pump(open(os.path.join(host, 'dm_cursor.c'), encoding='utf-8').read()), 'dm_cursor.c ui_pages_pump')
    srcs = sorted(f for f in os.listdir(host) if f.endswith('.c'))
    for f in srcs: bad += vis_scan(open(os.path.join(host, f), encoding='utf-8', errors='replace').read(), f)
    bad += rect_scan(open(os.path.join(host, 'dm_cursor.c'), encoding='utf-8').read(), 'dm_cursor.c')
    if bad:
        print('✗ 我们的功能碰了原版控件（编年史页 / 个人仓库页码只许写 TouchVault / TouchPgPage 底下的；可见位只许走带守卫的函数）：'); [print('   ' + b) for b in bad]; return 1
    print('✓ 编年史页与个人仓库页码不碰原版储藏箱控件（扫 %d 个名字）；宿主 %d 个源文件没有绕过守卫直接写可见位；dm_cursor.c 写控件矩形只在 mv_offset / ui_move_ours 里' % (len(NATIVE), len(srcs))); return 0

if __name__ == '__main__':
    sys.exit(main())
