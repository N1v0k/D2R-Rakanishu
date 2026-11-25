#!/usr/bin/env python3












import json, os, re, sys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'tools'))
import dmx

PARTS = ['头盔', '盔甲', '盾牌与手套', '鞋子与腰带', '首饰', '剑与匕首', '斧与爪', '锤与棍', '长柄与矛', '弓弩', '法杖与法器']




PART_OF = [('helm', 0), ('head', 0), ('tors', 1), ('grim', 10), ('shld', 2), ('glov', 2), ('boot', 3), ('belt', 3),
           ('ring', 4), ('amul', 4), ('jewl', 4), ('char', 4), ('swor', 5), ('knif', 5), ('axe', 6), ('h2h', 6), ('scep', 7), ('staf', 10), ('wand', 10), ('orb', 10), ('blun', 7),
           ('pole', 8), ('spea', 8), ('jave', 8), ('miss', 9)]

def txt(name):
    raw = dmx.get('data/global/excel/%s.txt' % name).decode('utf-8', 'replace').splitlines()
    head = raw[0].split('\t'); return [dict(zip(head, l.split('\t'))) for l in raw[1:] if l.strip()]
def js(name):
    out = {}
    for d in json.loads(dmx.get('data/hd/items/%s.json' % name).decode('utf-8')):
        for k, v in d.items(): out[k] = v
    return out

def main():
    dmx.ROOT = os.path.join(sys.argv[1], 'data')
    sprites = set()
    for root in sys.argv[1:]:
        for line in open(os.path.join(root, 'data', 'build_manifest.psv'), encoding='utf-8'):
            p = line.split('|')[9].lower() if line.count('|') >= 10 else ''
            m = re.match(r'data:data/hd/global/ui/(items/.*)\.sprite$', p)
            if m: sprites.add(m.group(1))
    types = {r['Code']: (r['Equiv1'], r['Equiv2']) for r in txt('ItemTypes') if r.get('Code')}
    base = {}
    for cat, f in (('armor', 'armor'), ('weapon', 'weapons'), ('misc', 'misc')):
        for r in txt(f):
            c = r.get('code', '')
            if not c: continue
            tier = 2 if c == r.get('ultracode') else 1 if c == r.get('ubercode') else 0
            base[c] = (cat, r.get('type', ''), tier, int(r.get('level') or 0))
    def part(code):
        seen, todo = set(), [base[code][1]]
        while todo:
            t = todo.pop(0)
            if not t or t in seen: continue
            seen.add(t); todo += list(types.get(t, ('', '')))
        for t, p in PART_OF:
            if t in seen: return p
        raise SystemExit('gen_vault_table：底子 %s 的类型 %s 归不进任何部位' % (code, base[code][1]))
    uj, sj, ij = js('uniques'), js('sets'), js('items')
    def icon(key, code, j):
        cat = base[code][0]
        for a in ((j.get(re.sub(r'[^a-z0-9]+', '_', key.lower().replace("'", '')).strip('_')) or {}).get('normal'), (ij.get(code) or {}).get('asset')):
            if a and 'items/%s/%s' % (cat, a.lower()) in sprites: return 'Items/%s/%s' % (cat, a)
        raise SystemExit('gen_vault_table：%s（%s）找不到物品图' % (key, code))
    ok = lambda r: r.get('spawnable') != '0' and not r.get('disabled')
    rows = []
    for r in txt('UniqueItems'):
        c = r.get('code', '')
        if not ok(r) or c not in base: continue
        rows.append((1, part(c), base[c][2], int(r['*ID']), int(r.get('lvl') or 0), r['index'], icon(r['index'], c, uj), -1))
    sets = [r['index'] for r in txt('Sets') if r.get('index')]
    for r in txt('SetItems'):
        c = r.get('item', '')
        if not ok(r) or c not in base or r['set'] not in sets: continue
        rows.append((0, part(c), base[c][2], int(r['*ID']), int(r.get('lvl') or 0), r['index'], icon(r['index'], c, sj), sets.index(r['set'])))


    rows.sort(key=lambda t: (t[0], t[1], t[2], t[4], t[3]) if t[0] == 1 else (t[0], t[7], t[3]))
    used = [s for i, s in enumerate(sets) if any(t[0] == 0 and t[7] == i for t in rows)]
    q = lambda s: '"' + s.replace('\\', '\\\\').replace('"', '\\"') + '"'
    print('// 生成文件，勿手改：dm/ios/tools/gen_vault_table.py <原版 RomFS>（编年史仓库固定表，见该脚本头注释）')
    print('// 部位：' + '、'.join('%d %s %d 件' % (i, p, sum(1 for t in rows if t[0] == 1 and t[1] == i)) for i, p in enumerate(PARTS)))
    print('// 暗金 %d 件、套装 %d 套 %d 件' % (sum(1 for t in rows if t[0] == 1), len(used), sum(1 for t in rows if t[0] == 0)))
    print('#define VT_NPART %d' % len(PARTS))
    print('static const char *const vt_part_name[VT_NPART] = { %s };' % ', '.join(q(p) for p in PARTS))
    print('typedef struct { uint8_t kind, part, tier; uint16_t row, lvl; int16_t set; const char *key, *icon; } vt_row;   // kind 1 暗金 / 0 套装件；row = 表行号（物品数据 +0x34）；set = 套装序号（暗金 −1）')
    print('static const vt_row vt_rows[] = {')
    for t in rows:
        set_ix = used.index(sets[t[7]]) if t[0] == 0 else -1
        print('    { %d, %d, %d, %d, %d, %d, %s, %s },' % (t[0], t[1], t[2], t[3], t[4], set_ix, q(t[5]), q(t[6])))
    print('};')
    print('#define VT_NROW (int)(sizeof vt_rows / sizeof vt_rows[0])')
    print('static const char *const vt_set_key[] = { %s };   // 套装名的字符串键（Sets.txt index），次序 = 原版' % ', '.join(q(s) for s in used))
    print('#define VT_NSET (int)(sizeof vt_set_key / sizeof vt_set_key[0])')

if __name__ == '__main__':
    main()
