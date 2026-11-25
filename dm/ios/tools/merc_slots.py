#!/usr/bin/env python3












import os, re, shutil, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import touch_layout

PANEL = 'controller/hirelinginventorypanelhd.json'; PLAYER = 'controller/playerinventoryoriginallayouthd.json'
BG = 'data/hd/global/ui/controller/panel/hirelinginventory/v2/hirelinginventorybg.sprite'
SHIFT = 216
ROWS = {902: 1064, 903: 1065, 905: 1067, 994: 1132, 993: 1131, 995: 1133, 1088: 1200, 1089: 1201, 1090: 1202, 1183: 1268, 1185: 1270, 1279: 1336}




ROWS_TXT = {'CharacterName': (884, 60), 'HireTypeText': (984, 60)}
BAND = ('CharacterName', 'HireTypeText', 'XpProgress', 'NextLevelXP', 'CurrentXP', 'Skill0', 'Skill1', 'Skill2')
STATS = ('HPTitle', 'HPStat', 'StrengthTitle', 'StrengthStat', 'DexTitle', 'DexStat', 'DamageTitle', 'Damage', 'ArmorClassTitle', 'ArmorClass',
         'FireResistanceTitle', 'FireText', 'ColdResistanceTitle', 'ColdText', 'LightningResistanceTitle', 'LightningText', 'PoisonResistanceTitle', 'PoisonText')
NEW = ('neck', 'gloves', 'right_hand', 'belt', 'left_hand', 'feet')
NAV = {
    'head': {'left': 'right_arm', 'right': 'neck', 'down': 'torso'}, 'neck': {'left': 'head', 'right': 'left_arm', 'down': 'left_hand'},
    'torso': {'left': 'right_arm', 'right': 'left_arm', 'up': 'head', 'down': 'belt'}, 'right_arm': {'right': 'torso', 'down': 'gloves'},
    'left_arm': {'left': 'neck', 'up': 'head', 'down': 'feet'}, 'gloves': {'up': 'right_arm', 'right': 'right_hand'},
    'right_hand': {'left': 'gloves', 'right': 'belt', 'up': 'torso'}, 'belt': {'left': 'right_hand', 'right': 'left_hand', 'up': 'torso'},
    'left_hand': {'left': 'belt', 'right': 'feet', 'up': 'neck'}, 'feet': {'left': 'left_hand', 'up': 'left_arm'}}

def _block(text, name):
    m = re.search(r'\{\s*"type"\s*:\s*"\w+"\s*,\s*"name"\s*:\s*"%s"' % re.escape(name), text)
    if not m: raise SystemExit('merc_slots：布局里没有控件 ' + name)
    return m.start(), touch_layout._block_end(text, m.start()) + 1

def _set_y(text, name, fn):
    a, b = _block(text, name); blk = text[a:b]; r = re.compile(r'("rect"\s*:\s*\{\s*"x"\s*:\s*-?\d+\s*,\s*"y"\s*:\s*)(-?\d+)').search(blk)
    if not r: raise SystemExit('merc_slots：控件 %s 没有 rect 的 y' % name)
    return text[:a] + blk[:r.start(2)] + str(fn(int(r.group(2)))) + blk[r.end(2):] + text[b:]

def _nav(slot):
    rows = ['"%s": { "name": "slot_%s" }' % (d, t) for d, t in NAV[slot].items()]
    if slot in ('left_arm', 'feet'): rows.append('"right": { "panelBoundary": true }')
    return '"navigation": { ' + ', '.join(rows) + ' }'

INV_AT = (196, 43)
ALL = ('head', 'torso', 'right_arm', 'left_arm') + NEW

def rects(player_text):

    import gen_merc_art
    dx, dy = 2 * gen_merc_art.DX - INV_AT[0], 2 * gen_merc_art.DY - INV_AT[1]; out = {}
    for s in ALL:
        a, b = _block(player_text, 'slot_' + s); m = re.search(r'"rect"\s*:\s*\{\s*"x"\s*:\s*(\d+)\s*,\s*"y"\s*:\s*(\d+)\s*,\s*"width"\s*:\s*(\d+)\s*,\s*"height"\s*:\s*(\d+)\s*\}', player_text[a:b])
        if not m: raise SystemExit('merc_slots：玩家物品栏布局里 slot_%s 没有 rect' % s)
        x, y, w, h = map(int, m.groups()); out[s] = (x + dx, y + dy, w, h)
    return out

def transform(text, player_text):
    for n in BAND: text = _set_y(text, n, lambda y: y + SHIFT)
    for n, (y, h) in ROWS_TXT.items():
        a, b = _block(text, n); blk = text[a:b]
        blk, n1 = re.subn(r'("rect"\s*:\s*\{\s*"x"\s*:\s*-?\d+\s*,\s*"y"\s*:\s*)-?\d+(\s*,\s*"width"\s*:\s*\d+\s*,\s*"height"\s*:\s*)\d+', lambda m: '%s%d%s%d' % (m.group(1), y, m.group(2), h), blk, count=1)
        blk, n2 = re.subn(r'"v"\s*:\s*"bottom"', '"v": "center"', blk, count=1)
        if n1 != 1 or n2 != 1: raise SystemExit('merc_slots：%s 的 rect / 竖向对齐和原版布局对不上（原版布局变了？）' % n)
        text = text[:a] + blk + text[b:]
    for n in STATS:
        def fn(y, n=n):
            if y not in ROWS: raise SystemExit('merc_slots：属性表控件 %s 的 y = %d 不在行表里（原版布局变了？）' % (n, y))
            return ROWS[y]
        text = _set_y(text, n, fn)
    R = rects(player_text)
    for s in ('head', 'torso', 'right_arm', 'left_arm'):
        a, b = _block(text, 'slot_' + s); blk = text[a:b]; i = blk.index('"navigation"'); j = touch_layout._block_end(blk, blk.index('{', i)) + 1
        blk = blk[:i] + _nav(s) + blk[j:]; x, y, w, h = R[s]
        blk, n = re.subn(r'"rect"\s*:\s*\{[^}]*\}', '"rect": { "x": %d, "y": %d, "width": %d, "height": %d }' % (x, y, w, h), blk, count=1)
        if n != 1: raise SystemExit('merc_slots：佣兵面板 slot_%s 没有 rect' % s)
        text = text[:a] + blk + text[b:]
    add = ''
    for s in NEW:
        pa, pb = _block(player_text, 'slot_' + s); bg = re.search(r'"backgroundFilename"\s*:\s*"([^"]+)"', player_text[pa:pb]).group(1); x, y, w, h = R[s]
        add += ',\n        { "type": "InventorySlotWidget", "name": "slot_%s", "fields": { "rect": { "x": %d, "y": %d, "width": %d, "height": %d }, "backgroundFilename": "%s", %s } }' % (s, x, y, w, h, bg, _nav(s))




    for s in NEW:
        x, y, w, h = R[s]
        add += ',\n        { "type": "RectangleWidget", "name": "TouchMercLock_%s", "fields": { "rect": { "x": 99999, "y": %d, "width": %d, "height": %d }, "color": [ 0.187, 0.019, 0.022, 0.78 ] } }' % (s, y, w, h)
    a, b = _block(text, 'slot_left_arm')
    return text[:b] + add + text[b:]

def build(romfs, outdir, repl):

    import gen_merc_art
    out = {}; p = os.path.join(outdir, 'ms_hirelinginventorybg.sprite'); shutil.copyfile(os.path.join(gen_merc_art.ART, 'HirelingInventoryBG.sprite'), p); out[BG] = p
    logical = touch_layout.read_layout(romfs, PANEL)[0].lower(); src = repl.get(logical)
    text = open(src, encoding='utf-8').read() if src else touch_layout.read_layout(romfs, PANEL)[1]
    p = os.path.join(outdir, 'ms_hireling.json'); open(p, 'w', encoding='utf-8').write(transform(text, touch_layout.read_layout(romfs, PLAYER)[1])); out[logical] = p
    return out

def build_pad(romfs, outdir):

    logical, text = touch_layout.read_layout(romfs, PANEL)
    p = os.path.join(outdir, 'pad_hireling.json'); open(p, 'w', encoding='utf-8').write(transform(text, touch_layout.read_layout(romfs, PLAYER)[1]))
    return {logical.lower(): p}
