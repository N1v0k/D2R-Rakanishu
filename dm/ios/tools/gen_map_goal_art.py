#!/usr/bin/env python3








import csv, json, os, struct, sys
from PIL import Image, ImageDraw, ImageFont
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import romfs_art
from gen_paths import gen_dir
OUT = gen_dir('NativeHUD', 'mapgoal'); ROMFS = None
NPC, OBJ = (212, 196, 145), (255, 255, 255)


LABELS = [
    ('levels', "Tal Rasha's Tomb", OBJ),
    ('monsters', 'Bloodraven', NPC),
    ('objects', 'StoneAlpha', OBJ),
    ('objects', 'Inifuss', OBJ),
    ('monsters', 'The Countess', NPC),
    ('item-names', 'hdm', OBJ),
    ('monsters', 'Radament', NPC),
    ('item-names', 'box', OBJ),
    ('item-names', 'Staff of Kings', OBJ),
    ('objects', 'taintedsunaltar', OBJ),
    ('monsters', 'The Summoner', NPC),
    ('item-names', 'qey', OBJ),
    ('item-names', 'qbr', OBJ),
    ('item-names', 'qhr', OBJ),
    ('objects', 'compellingorb', OBJ),
    ('item-names', 'bbb', OBJ),
    ('levels', 'Ruined Temple', OBJ),
    ('monsters', 'Izual', NPC),
    ('objects', 'Hellforge', OBJ),
    ('item-names', 'Seal', OBJ),
    ('monsters', 'Siege Boss', NPC),
    ('objects', 'cagedwussie1', OBJ),
    ('npcs', 'Drehya', NPC),
    ('objects', 'orifice', OBJ),
    ('objects', 'gidbinn altar', OBJ),
    ('monsters', 'Andariel', NPC),
    ('monsters', 'Duriel', NPC),
    ('monsters', 'Mephisto', NPC),
    ('monsters', 'Baal', NPC),
    ('objects', 'ancientsaltar', OBJ),
    ('npcs', 'Nihlathak', NPC),
]
TEXT_PX, PAD_X, PAD_Y, BOX_A = 48, 10, 6, 150

def sprite_png(path):
    b = romfs_art.read(ROMFS, path); assert b[:4] == b'SpA1'
    code, fw, w, h, frames = struct.unpack_from('<HHII4xH', b, 4); assert code == 0x3d, (path, code)
    return romfs_art.dxt5(b, w, h), fw, frames, w // frames

def strings(name):
    return {r['Key']: r for r in json.loads(romfs_art.read(ROMFS, 'local/lng/strings/%s.json' % name).decode('utf-8-sig'))}

def label(text, col, font):
    d0 = ImageDraw.Draw(Image.new('RGBA', (1, 1))); w = int(d0.textlength(text, font=font)) + 2 * PAD_X; h = TEXT_PX + 2 * PAD_Y
    im = Image.new('RGBA', (w, h), (0, 0, 0, BOX_A)); ImageDraw.Draw(im).text((w / 2, h / 2), text, font=font, fill=col + (255,), anchor='mm')
    return im

def main():
    os.makedirs(OUT, exist_ok=True)
    sym, fw, n, pitch = sprite_png('hd/global/ui/PANEL/Quest_Log/TalRashaSymbol.sprite'); assert n == 7
    for i in range(7): sym.crop((i * pitch, 0, i * pitch + fw, sym.height)).save(os.path.join(OUT, 'sym_%d.png' % i))
    units, ufw, un, upitch = sprite_png('hd/global/ui/AUTOMAP/Units.sprite'); assert un == 8
    cross = units.crop((6 * upitch, 0, 6 * upitch + ufw, units.height)); a = cross.split()[3].point(lambda v: v // 2); cross.putalpha(a); cross.save(os.path.join(OUT, 'cross.png'))





    ptr, pfw, pn, ppitch = sprite_png('hd/global/ui/Controller/Panel/EmoteWheel/Emote_Arrow.sprite'); assert pn == 12



    up = ptr.crop((3 * ppitch, 0, 3 * ppitch + pfw, ptr.height)).resize((max(1, pfw // 2), ptr.height), Image.LANCZOS); up.save(os.path.join(OUT, 'heading.png'))





    def tint(im, rgb):
        a = im.getchannel('A'); g = im.convert('L').point(lambda v: min(255, v * 2))
        out = Image.merge('RGB', [g.point(lambda v, c=c: v * c // 255) for c in rgb]).convert('RGBA'); out.putalpha(a); return out
    tint(up, (225, 225, 225)).save(os.path.join(OUT, 'threat_0.png'))
    tint(up, (110, 110, 255)).save(os.path.join(OUT, 'threat_1.png'))
    up.save(os.path.join(OUT, 'threat_2.png'))
    tint(up, (255, 80, 64)).save(os.path.join(OUT, 'threat_3.png'))
    font = ImageFont.truetype(romfs_art.font(ROMFS, 'BlizzardGlobal-V5_81.ttf'), TEXT_PX)
    oldp = os.path.join(HERE, 'annot', 'oldnames.tsv')
    old = {(r[0], r[1]): r[3] for r in csv.reader(open(oldp, encoding='utf-8'), delimiter='\t') if len(r) >= 4} if os.path.isfile(oldp) else {}
    cache, man = {}, []
    for i, (f, key, col) in enumerate(LABELS):
        if f not in cache: cache[f] = strings(f)
        zh = cache[f][key]['zhCN']; zo = old.get((f, key), zh)
        label(zh, col, font).save(os.path.join(OUT, 'label_%d.png' % i))
        if zo != zh: label(zo, col, font).save(os.path.join(OUT, 'label_%d_old.png' % i))
        en = cache[f][key]['enUS']; label(en, col, font).save(os.path.join(OUT, 'label_%d_en.png' % i))
        es = cache[f][key]['esES']; label(es, col, font).save(os.path.join(OUT, 'label_%d_es.png' % i))
        mx = cache[f][key]['esMX']; label(mx, col, font).save(os.path.join(OUT, 'label_%d_mx.png' % i))
        ru = cache[f][key]['ruRU']; label(ru, col, font).save(os.path.join(OUT, 'label_%d_ru.png' % i))
        ko = cache[f][key]['koKR']; label(ko, col, font).save(os.path.join(OUT, 'label_%d_ko.png' % i))
        de = cache[f][key]['deDE']; label(de, col, font).save(os.path.join(OUT, 'label_%d_de.png' % i))
        pt = cache[f][key]['ptBR']; label(pt, col, font).save(os.path.join(OUT, 'label_%d_pt.png' % i))
        man.append({'id': i, 'file': f, 'key': key, 'zhCN': zh, 'old': zo, 'enUS': en, 'esES': es, 'esMX': mx, 'ruRU': ru, 'koKR': ko, 'deDE': de, 'ptBR': pt, 'color': list(col)})





    from annot_strings import excel
    lvs = [('lvl_%s' % r['Id'], 'levels', r['LevelName']) for r in excel(ROMFS, 'levels.txt') if r.get('Id', '').isdigit() and r.get('LevelName')]
    nl = 0
    for name, f, key in lvs + [('lvl_wp', 'objects', 'Waypoint')]:
        if f not in cache: cache[f] = strings(f)
        row = cache[f].get(key)
        if not row: continue
        zh = row['zhCN']; zo = old.get((f, key), zh); nl += 1
        label(zh, OBJ, font).save(os.path.join(OUT, name + '.png'))
        if zo != zh: label(zo, OBJ, font).save(os.path.join(OUT, name + '_old.png'))
        for suf, col in (('en', 'enUS'), ('es', 'esES'), ('mx', 'esMX'), ('ru', 'ruRU'), ('ko', 'koKR'), ('de', 'deDE'), ('pt', 'ptBR')): label(row[col], OBJ, font).save(os.path.join(OUT, '%s_%s.png' % (name, suf)))
    json.dump({'text_px': TEXT_PX, 'symbol_frames': 7, 'labels': man, 'exit_labels': nl}, open(os.path.join(OUT, 'mapgoal.json'), 'w'), ensure_ascii=False, indent=1)
    print('mapgoal：符号 7 帧、十字 1、朝向指针 1、名牌 %d（旧译名不同的 %d）、出入口 / 传送点名牌 %d → %s' % (len(man), sum(m['old'] != m['zhCN'] for m in man), nl, OUT))

if __name__ == '__main__': ROMFS = sys.argv[1]; main()
