#!/usr/bin/env python3








import functools, json, os, re, struct, zlib, hashlib
OFF = '"rect": { "x": 99999, "y": 99999, "width": 1, "height": 1 }'

def _read_entry(romfs, f):
    ek = bytes.fromhex(f[6]); raw_len = int(f[7])
    with open(os.path.join(romfs, 'data', '%02d-%08d.data' % (ek[9], ek[10])), 'rb') as s: s.seek(int.from_bytes(ek[11:16], 'big')); raw = s.read(raw_len)
    if f[8] == 'z': data = zlib.decompress(raw)
    elif f[8] == 'n': data = raw
    else:
        hs = struct.unpack('>I', raw[4:8])[0]; pos = hs; data = b''
        for i in range(int.from_bytes(raw[9:12], 'big')):
            e, _ = struct.unpack('>II', raw[12 + i * 24:20 + i * 24]); b = raw[pos:pos + e]; data += b[1:] if b[:1] == b'N' else zlib.decompress(b[1:]); pos += e
    assert hashlib.md5(data).hexdigest() == f[4], f[9]
    return data







def build_cursor(romfs, outdir):




    repl = {}
    for line in open(os.path.join(romfs, 'data', 'build_manifest.psv'), encoding='utf-8'):
        f = line.rstrip('\n').split('|')
        if len(f) < 11 or line.startswith('#') or '/ui/controller/cursor/' not in f[9].lower() or not f[9].lower().endswith('.sprite'): continue
        d = _read_entry(romfs, f); assert d[:4] == b'SpA1' and struct.unpack('<H', d[4:6])[0] == 61 and struct.unpack('<I', d[0x20:0x24])[0] == len(d) - 0x28, f[9]
        path = f[9].split(':', 1)[1]; o = os.path.join(outdir, 'cursor_' + os.path.basename(path)); open(o, 'wb').write(d[:0x28] + bytes(len(d) - 0x28)); repl[path.lower()] = o
    if len(repl) != 3: raise SystemExit('touch_layout：手柄光标贴图应为 3 张，找到 %d 张（原包变了？）' % len(repl))
    return repl
def build_glyphs(romfs, outdir):
    repl = {}
    for line in open(os.path.join(romfs, 'data', 'build_manifest.psv'), encoding='utf-8'):
        f = line.rstrip('\n').split('|')
        if len(f) < 11 or line.startswith('#') or '/ui/controller/controllerbuttons/switch/' not in f[9].lower(): continue
        d = _read_entry(romfs, f); assert d[:4] == b'SpA1' and struct.unpack('<H', d[4:6])[0] == 61 and struct.unpack('<I', d[0x20:0x24])[0] == len(d) - 0x28, f[9]
        path = f[9].split(':', 1)[1]; o = os.path.join(outdir, 'glyph_' + os.path.basename(path)); open(o, 'wb').write(d[:0x28] + bytes(len(d) - 0x28)); repl[path.lower()] = o
    if len(repl) != 22: raise SystemExit('touch_layout：手柄键图标应为 22 张，找到 %d 张（原包变了？）' % len(repl))
    return repl

def read_layout(romfs, rel, suffix=None):

    want = (suffix or '/ui/layouts/' + rel).lower()
    for line in open(os.path.join(romfs, 'data', 'build_manifest.psv'), encoding='utf-8'):
        f = line.rstrip('\n').split('|')
        if len(f) < 11 or line.startswith('#') or not f[9].lower().endswith(want): continue
        ek = bytes.fromhex(f[6]); raw_len = int(f[7])
        with open(os.path.join(romfs, 'data', '%02d-%08d.data' % (ek[9], ek[10])), 'rb') as s: s.seek(int.from_bytes(ek[11:16], 'big')); raw = s.read(raw_len)
        if f[8] == 'z': data = zlib.decompress(raw)
        elif f[8] == 'n': data = raw
        else:
            hs = struct.unpack('>I', raw[4:8])[0]; pos = hs; data = b''
            for i in range(int.from_bytes(raw[9:12], 'big')):
                e, _ = struct.unpack('>II', raw[12 + i * 24:20 + i * 24]); b = raw[pos:pos + e]; data += b[1:] if b[:1] == b'N' else zlib.decompress(b[1:]); pos += e
        assert hashlib.md5(data).hexdigest() == f[4], rel
        return f[9].split(':', 1)[1], data.decode('utf-8-sig')
    raise SystemExit('touch_layout：包里没有布局 ' + rel)

def _block_end(s, i):
    d = 0; q = False
    while i < len(s):
        c = s[i]
        if q:
            if c == '\\': i += 1
            elif c == '"': q = False
        elif c == '"': q = True
        elif c == '/' and s[i:i + 2] == '//': i = s.find('\n', i)
        elif c == '{': d += 1
        elif c == '}':
            d -= 1
            if d == 0: return i
        i += 1
    raise SystemExit('touch_layout：括号不配对')

def offscreen(s, types=('ButtonLegendWidget', 'ButtonLegendScrollWidget'), names=()):

    n = 0; pats = [r'"type"\s*:\s*"%s"' % re.escape(t) for t in types] + [r'"type"\s*:\s*"[A-Za-z0-9_]+"\s*,\s*"[Nn]ame"\s*:\s*"%s"' % re.escape(x) for x in names]
    for pat in pats:
        pos = 0
        while True:
            m = re.compile(pat).search(s, pos)
            if not m: break
            line0 = s.rfind('{', 0, m.start())
            nxt = re.compile(r'"(type|children)"\s*:').search(s, m.end()); end = nxt.start() if nxt else len(s)
            f = re.compile(r'"fields"\s*:\s*\{').search(s, line0, max(end, m.end()))
            if f:
                a = f.end() - 1; b = _block_end(s, a); body = s[a + 1:b]; depth = 0; i = 0; keys = {}
                while i < len(body):
                    c = body[i]
                    if c == '"':
                        j = body.find('"', i + 1)
                        while body[j - 1] == '\\': j = body.find('"', j + 1)
                        if depth == 0 and body[j + 1:].lstrip().startswith(':'): keys.setdefault(body[i + 1:j], i)
                        i = j
                    elif c == '/' and body[i:i + 2] == '//': i = body.find('\n', i)
                    elif c in '{[': depth += 1
                    elif c in '}]': depth -= 1
                    i += 1
                if 'rect' in keys: r0 = body.index('{', keys['rect']); body = body[:keys['rect']] + OFF + body[_block_end(body, r0) + 1:]
                else: body = ' ' + OFF + ',' + body
                body = re.sub(r'"fitToParent"\s*:\s*true', '"fitToParent": false', body, count=1)
                s = s[:a + 1] + body + s[b:]; pos = a + 1 + len(body)
            else:
                ins = ' "fields": { ' + OFF + ' },'; at = m.end(); at = at + 1 if s[at:at + 1] == ',' else at
                s = s[:at] + (ins if s[at - 1] == ',' else ',' + ins) + s[at:]; pos = at + len(ins) + 1
            n += 1
    return s, n





FRONTEND = [('controller/charactercreatepanelhd.json', ()), ('controller/console/charactercreatepanelhd.json', ('GameType', 'GameTypeBackground')),
            ('controller/console/charactercreatenamepanelhd.json', ('EnterNameLabel',)), ('controller/console/characteroptionspanelhd.json', ('LadderFlag', 'LadderToggle', 'NonLadderToggle', 'LadderCenter')),
           
            ('controller/cinematicsmodalhd.json', ('LegendBackground',)), ('controller/console/cinematicsmodalhd.json', ('LegendBackground',)),
            ('controller/creditsmodalhd.json', ()), ('controller/eulamodalhd.json', ()),
            ('controller/confirmationmodalhd.json', ()), ('controller/dismissablemodalhd.json', ()),
            ('controller/deletecharactermodalhd.json', ()), ('controller/renamecharactermodalhd.json', ())]





def _btn(name, y, text, msg, x=825, anchor=True, scale=None):
    return ('        {\n            "type": "ButtonWidget", "name": "%s",\n            "fields": {\n%s                "rect": { "x": %d, "y": %d%s },\n'
            '                "filename": "FrontEnd\\\\HD\\\\Final\\\\FrontEnd_ButtonMed",\n                "textString": "%s",\n                "onClickMessage": "%s",\n'
            '                "textColor": "$LightButtonTextColor",\n                "text/style": "$StyleFEButtonText",\n                "hoveredFrame": 3,\n                "disabledFrame": 2,\n            }\n        },\n') % (name, '                "anchor": {"x": 0.5},\n' if anchor else '', x, y, ', "scale": %s' % scale if scale else '', text, msg)


def _btn_big(name, x, y, text, msg):
    return _btn(name, y, text, msg, x=x, anchor=False)
INSERT = {


          'controller/console/charactercreatepanelhd.json': (r'\{\s*"type"\s*:\s*"ButtonLegendWidget"\s*,\s*"name"\s*:\s*"CharacterCreateLegend"',




          _btn_big('TouchNext', -235, 1900, '@UICreate', 'ConsoleCharacterCreate:ToConsoleCharacterOptions')
          + _btn_big('TouchBack', -785, 1900, '@UIBack', 'FrontEndNavigation:ToCharacterSelect')),


          'controller/console/charactercreatenamepanelhd.json': (r'\{\s*"type"\s*:\s*"ButtonLegendWidget"\s*,\s*"name"\s*:\s*"CharacterNameLegend"',


          _btn_big('TouchNameBack', -659, 340, '@UIBack', 'PanelManager:ClosePanel:TouchKeyB') + _btn_big('TouchNameOK', 189, 340, '@UICreate', 'PanelManager:ClosePanel:TouchKeyPLUS'))}







CLASS_STRIPS = [('Amazon', -1500, -1092), ('Assassin', -1092, -750), ('Necromancer', -750, -396), ('Barbarian', -396, -18),
                ('Paladin', -18, 372), ('Sorceress', 372, 750), ('Warlock', 750, 1158), ('Druid', 1158, 1640)]






_BLISS = ('        { "type": "Widget", "name": "TouchBlissWidget", "children": [\n        {\n            "type": "ToggleButtonWidget", "name": "TouchBliss",\n            "fields": {\n'
          '                "rect": { "x": -1551, "y": 1330 }, "hitMargin": { "left": 30, "top": 30, "right": 30, "bottom": 30 },\n'
          '                "filename": "Lobby\\\\CreateGame\\\\CreateGame_AdvancedCheckbox",\n                "untoggledFrame": 0, "untoggledPressedFrame": 1, "untoggledHoveredFrame": 3, "untoggledDisabledFrame": 2,\n'
          '                "toggledFrame": 4, "toggledPressedFrame": 5, "toggledHoveredFrame": 6,\n                "onClickMessage": "PanelManager:ClosePanel:TouchBliss",\n            }\n        },\n        ] },\n'
          '        { "type": "TextBoxWidget", "name": "TouchBlissTitle", "fields": { "rect": { "x": -1451, "y": 1322, "width": 690, "height": 60 }, "text": "@dmBlissTitle", "style": "$StyleCharacterModeSelectTitle" } },\n'
          '        { "type": "TextBoxWidget", "name": "TouchBlissDesc", "fields": { "rect": { "x": -1451, "y": 1400, "width": 3000, "height": 120 }, "text": "@dmBlissDesc", "style": "$StyleCharacterModeSelectDescription" } },\n')
INSERT['controller/console/characteroptionspanelhd.json'] = (r'\{\s*"type"\s*:\s*"ButtonLegendWidget"\s*,\s*"name"\s*:\s*"CharacterOptionsLegend"', _BLISS)
def pad_bliss(s):






    s, n = offscreen(s, types=(), names=('LadderFlag', 'LadderToggle', 'NonLadderToggle', 'LadderCenter')); assert n == 4, n



    s, n = re.subn(r'\{\s*"action"\s*:\s*"option2"\s*,\s*"description"\s*:\s*"@LegacyGameVersionSwitch"\s*\}\s*,?', '', s); assert n == 1, n
    at, add = INSERT['controller/console/characteroptionspanelhd.json']; ms = list(re.finditer(at, s)); assert len(ms) == 1
    key = '        { "type": "ImageWidget", "name": "TouchBlissKey", "fields": { "rect": { "x": -1665, "y": 1330, "scale": 1.5 }, "filename": "Controller/ControllerButtons/Switch/ButtonLeft" } },\n'
    if os.environ.get('DM_OVERLAY_MAC'):
        key += '        { "type": "ImageWidget", "name": "TouchBlissKeyK", "fields": { "rect": { "x": 99999, "y": 1330, "scale": 1.5 }, "filename": "Ztouch_controller_buttons_ui/switchKButtonLeft" } },\n'
    return s[:ms[0].start()] + add.lstrip() + key + '        ' + s[ms[0].start():]
def set_rect(s, name, x, y, w, h):
    m = re.search(r'"name"\s*:\s*"%s"' % re.escape(name), s); assert m, name
    r = re.compile(r'"rect"\s*:\s*\{[^}]*\}').search(s, m.end()); assert r and r.start() - m.end() < 200, name
    return s[:r.start()] + '"rect": { "x": %d, "y": %d, "width": %d, "height": %d }' % (x, y, w, h) + s[r.end():]

def build(romfs, outdir, files=FRONTEND):

    repl = {}; total = 0
    for rel, names in files:
        path, text = read_layout(romfs, rel); new, n = offscreen(text, names=names)
        if n == 0: raise SystemExit('touch_layout：%s 里没找到键位提示控件（原版布局变了？）' % rel)
        if rel == 'controller/console/charactercreatepanelhd.json':
            for nm, x0, x1 in CLASS_STRIPS: new = set_rect(new, nm, x0, 450, x1 - x0, 1300)
        if rel in INSERT:
            at, add = INSERT[rel]; ms = list(re.finditer(at, new)); assert len(ms) == 1, rel; new = new[:ms[0].start()] + add.lstrip() + '        ' + new[ms[0].start():]
        o = os.path.join(outdir, rel.replace('/', '_')); open(o, 'w', encoding='utf-8').write(new); repl[path.lower()] = o; total += n
    return repl, total





SAFE_X = 200
def _sub1(s, old, new):
    if s.count(old) != 1: raise SystemExit('touch_layout：键鼠版前端布局里「%s」不是恰好 1 处（原版布局变了？）' % old[:60])
    return s.replace(old, new)








_C = '"anchor": { "x": 0.5 }, '
def _upd_text(name, x, y, w, size, color, h='left', anchor='', fit=''):
    return ('        { "type": "TextBoxWidget", "name": "%s", "fields": { %s"rect": { "x": %d, "y": %d, "width": %d, "height": 60 }, "text": "", %s'
            '"style": { "pointSize": %d, "fontColor": { "r": %d, "g": %d, "b": %d, "a": 255 }, "alignment": { "h": "%s", "v": "center" }, "dropShadow": "$DefaultDropShadow" } } },\n') % ((name, anchor, x, y, w, fit, size) + color + (h,))
def _upd_btn(name, scale, anchor=_C):
    return ('        { "type": "ButtonWidget", "name": "%s", "fields": { %s"rect": { "x": 99999, "y": 99999%s }, "filename": "FrontEnd\\\\HD\\\\Final\\\\FrontEnd_ButtonLarge", '
            '"textString": "@dmUpdButton", "onClickMessage": "PanelManager:ClosePanel:TouchUpdate", "text/style": "$StyleFEButtonText", "pointSize": "$ExtraLargeFontSize", "textColor": "$LightButtonTextColor", '
            '"hoveredFrame": 3, "disabledFrame": 2%s } },\n') % (name, anchor, ', "scale": %s' % scale if scale else '', ', "hitMargin": { "left": 30, "top": 40, "right": 30, "bottom": 40 }' if scale else '')
_UPDATE = (_upd_text('TouchBuild', 79, 2046, 420, 30, (150, 140, 115))
           + _upd_text('TouchUpdNote', 420, 2046, 1500, 30, (199, 179, 119), fit='"fitToText": true, ')
           + _upd_btn('TouchUpdSmall', 0.6, '')
           + '        { "type": "RectangleWidget", "name": "TouchUpdBg", "fields": { "anchor": { "x": 0.5 }, "rect": { "x": 99999, "y": 99999, "width": 1400, "height": 186 }, "color": [ 0.0, 0.0, 0.0, 0.59 ] } },\n'
           + _upd_text('TouchUpdTitle', -700, 1524, 1400, 46, (214, 74, 58), 'center', _C)
           + _upd_text('TouchUpdDesc', -700, 1610, 1400, 36, (232, 228, 214), 'center', _C)
           + _upd_btn('TouchUpdBig', None))




_UPDATE_PAD = (_upd_text('TouchBuild', 3341, 60, 420, 30, (150, 140, 115), 'right')
               + _upd_text('TouchUpdNote', 1761, 130, 2000, 42, (199, 179, 119), 'right')
               + _upd_btn('TouchUpdSmall', 0.6, '')
               + '        { "type": "RectangleWidget", "name": "TouchUpdBg", "fields": { "rect": { "x": 99999, "y": 99999, "width": 1400, "height": 186 }, "color": [ 0.0, 0.0, 0.0, 0.59 ] } },\n'
               + _upd_text('TouchUpdTitle', 1220, 1524, 1400, 46, (214, 74, 58), 'center')
               + _upd_text('TouchUpdDesc', 1220, 1610, 1400, 36, (232, 228, 214), 'center')
               + _upd_btn('TouchUpdBig', None, ''))
def pad_ribbon(s, movies=False):
    if movies:
        return s


    s = _sub1(s, '"rect": { "x": 164, "y": 1175 },', '"rect": { "x": 99999, "y": 99999 },')
    s = _sub1(s, '"rect": { "x": 164, "y": 1325 },', '"rect": { "x": 164, "y": 1175 },')
    s = _sub1(s, '"down": { "name": "Cinematics" }', '"down": { "name": "Credits" }')
    s = _sub1(s, '"up": { "name": "Cinematics" }', '"up": { "name": "Settings" }')
    return s
def pad_mainmenu(s):
    i = s.rstrip().rfind(']'); assert s.rstrip().endswith('}') and i > 0
    return s[:i].rstrip(' ') + _UPDATE_PAD + '    ' + s[i:]
_UPDATE_STUB = ''.join('        { "type": "%s", "name": "%s" },\n' % (ty, nm) for ty, nm in re.findall(r'"type": "(\w+)", "name": "(TouchBuild|TouchUpd\w+)"', _UPDATE))
assert _UPDATE_STUB.count('\n') == 7
def _pc_mainmenu(s):


    s = _sub1(s, '"rect": { "x": -1010, "y": 1865 },', '"rect": { "x": -303, "y": 1865 },')
    s = _sub1(s, '"textString": "@PlayTab",', '"textString": "@PlayButton",')
    s, n = offscreen(s, types=(), names=('OnlinePlaySettingsButton', 'Divider', 'SecondaryContextButton'))
    if n != 3: raise SystemExit('touch_layout：键鼠版主菜单要挪走的控件应为 3 个，找到 %d 个' % n)
    i = s.rstrip().rfind(']'); assert s.rstrip().endswith('}') and i > 0
    return s[:i].rstrip(' ') + _UPDATE + '    ' + s[i:], n + 2
def _pc_charselect(s):


    s = _sub1(s, '"rect": { "x": -801, "y": 0 },', '"rect": { "x": %d, "y": 0 },' % (-801 - SAFE_X))
    s, n = offscreen(s, types=(), names=('Tabs', 'ConvertCharacter'))
    if n != 2: raise SystemExit('touch_layout：键鼠版选角列表的页签 / 转换按钮没找到')


    s = _sub1(s, '"rect": { "x": 315, "y": 1940 },', '"rect": { "x": 431, "y": 1940 },')






    s = _sub1(s, '"itemSelectionMessage": "CharacterSelect:SelectCharacter",', '"itemSelectionMessage": "PanelManager:ClosePanel:TouchNoop",')


    s = _sub1(s, '"onClickMessage": "FrontEndNavigation:ToCharacterCreate",', '"onClickMessage": "FrontEndNavigation:ToCharacterCreate:1",')
    for name, var, size in (('Title', '$XMediumFontSize', 30), ('Name', '$XMediumLargeFontSize', 34), ('Level&Class', '$XMediumFontSize', 30)):
        i = s.index('"type": "TextBoxWidget", "name": "%s"' % name); m = re.compile(r'"pointSize":\s*"(\$[A-Za-z]+)"').search(s, i)
        if not m or m.group(1) != var: raise SystemExit('touch_layout：选角列表 %s 的字号不是原样（原版布局变了？）' % name)
        s = s[:m.start()] + '"pointSize": %d' % size + s[m.end():]; n += 1
    return s, n + 1
def _pc_ribbon(s):

    if os.environ.get('MOVIE_FFMPEG') == '1':
        return s, 0


    s, n = offscreen(s, types=(), names=('Ribbon', 'MotDSection'))
    if n != 2: raise SystemExit('touch_layout：键鼠版左侧栏应挪走 2 块，找到 %d 块' % n)
    return s, n
PCFRONT = {
    'controller/console/mainmenupanelhd.json': ('mainmenupanelhd.json', _pc_mainmenu),
    'controller/console/characterselectpanelhd.json': ('characterselectpanelhd.json', _pc_charselect),
    'controller/console/mainmenubuttonribbonhd.json': ('mainmenubuttonribbonhd.json', _pc_ribbon),
    'controller/console/mainmenulinkshd.json': ('mainmenulinkshd.json', None),
}
PCFRONT_PLAIN = ('mainmenubuttonribbonhd.json', 'mainmenulinkshd.json')
def build_pcfront(romfs, outdir):






    repl = {}
    for rel, (pc, fn) in PCFRONT.items():
        cpath, _ = read_layout(romfs, rel); ppath, text = read_layout(romfs, pc)
        if fn: text, n = fn(text)


        base = os.path.dirname(ppath).lower() + '/1touchbase_lay/' + pc
        if pc in PCFRONT_PLAIN: mid = text
        else:
            o = os.path.join(outdir, 'pcfront_base_' + pc); open(o, 'w', encoding='utf-8').write(text); repl[base] = o
            _, mid = read_layout(romfs, 'controller/' + pc); mid, _n = offscreen(mid)
            mid = _sub1(mid, '"basedOn": "%s"' % os.path.basename(ppath), '"basedOn": "1touchbase_lay/%s"' % pc)
        if pc == 'mainmenupanelhd.json': i = mid.rstrip().rfind(']'); mid = mid[:i] + _UPDATE_STUB + mid[i:]
        o = os.path.join(outdir, 'pcfront_' + rel.replace('/', '_')); open(o, 'w', encoding='utf-8').write(mid); repl[cpath.lower()] = o
    return repl









INGAME = {
    'controller/horadriccubelayouthd.json': ('horadriccubelayouthd.json', [('close', None, '"rect": { "x": 2741, "y": -156 }, "filename": "PANEL\\\\HUD_02\\\\front_panel", "hoveredFrame": 0')]),












    'controller/vendorpanellayouthd.json': ('vendorpanellayouthd.json', [('button_repair', None, '"rect": { "x": 960, "y": 1216 }, "filename": "PANEL\\\\Vendors\\\\Gambling_Refresh_Button", "sound": "cursor_gamble_refresh_hd", "tooltipString": "@refresh", "onClickMessage": "PanelManager:ClosePanel:TouchShopRefresh"'),






        ('button_refresh', 'button_blank', '"rect": { "x": 960, "y": 1216 }, "onClickMessage": "PanelManager:ClosePanel:TouchShopRefresh"'),
        ('button_repair_all', None, '"rect": { "x": 224, "y": 1216 }'),




        ('button_refresh', None, '"rect": { "x": 960, "y": 1216 }'), ('close', None, '"rect": { "x": 2741, "y": -156 }, "filename": "PANEL\\\\HUD_02\\\\front_panel", "hoveredFrame": 0')]),
    'controller/bankexpansionlayouthd.json': ('bankexpansionlayouthd.json', [('close', None, '"rect": { "x": 2741, "y": -156 }, "filename": "PANEL\\\\HUD_02\\\\front_panel", "hoveredFrame": 0', 'bankoriginallayouthd.json'),


        ('StashLeftArrow', 'TouchStashLeft', '"rect": { "x": 415, "y": 1159, "width": 64, "height": 64 }'),
        ('StashRightArrow', 'TouchStashRight', '"rect": { "x": 801, "y": 1159, "width": 64, "height": 64 }')]),







    'controller/characterstatspanelhd.json': ('characterstatspanelhd.json', [('AdvancedStats', None, '"rect": { "x": 2644, "y": 1000 }'), ('CloseButton', 'TouchClose', '"rect": { "x": 2747, "y": -157 }, "filename": "PANEL\\\\HUD_02\\\\front_panel", "hoveredFrame": 0, "onClickMessage": "PanelManager:ClosePanel:UISwitcher"')]),


    'controller/advancedstatspanelhd.json': ('advancedstatspanelhd.json', [('CloseButton', None, '"rect": { "x": 2565, "y": 20 }')]),
    'controller/console/switch/playerinventoryexpansionlayouthd.json': ('playerinventoryoriginallayouthd.json', [('close', 'TouchClose', '"rect": { "x": 2079, "y": -129 }, "filename": "PANEL\\\\HUD_02\\\\front_panel", "hoveredFrame": 0, "onClickMessage": "PanelManager:ClosePanel:UISwitcher"')]),




    'controller/skillstreepanelhd.json': ('skillstreepanelhd.json', [('Cancel', 'TouchClose', '"rect": { "x": 2271, "y": -158 }, "filename": "PANEL\\\\HUD_02\\\\front_panel", "hoveredFrame": 0, "onClickMessage": "PanelManager:ClosePanel:UISwitcher"'),
        ('background_right_arm', 'TouchSwapTab0', '"rect": { "x": 1540, "y": 668 }', 'controller/playerinventoryexpansionlayouthd.json'),
        ('background_right_arm_selected', 'TouchSwapTab1', '"rect": { "x": 1540, "y": 668 }', 'controller/playerinventoryexpansionlayouthd.json'),
        ('text_i_left', 'TouchSwapI', '"rect": { "x": 1552, "y": 665, "width": 84, "height": 48 }', 'controller/playerinventoryexpansionlayouthd.json'),
        ('text_ii_left', 'TouchSwapII', '"rect": { "x": 1657, "y": 665, "width": 84, "height": 48 }', 'controller/playerinventoryexpansionlayouthd.json')]),
    'controller/questlogpanelexpansionhd.json': ('questlogpaneloriginalhd.json', [('Cancel', 'TouchClose', '"rect": { "x": 2265, "y": -160 }, "filename": "PANEL\\\\HUD_02\\\\front_panel", "hoveredFrame": 0, "onClickMessage": "PanelManager:ClosePanel:UISwitcher"')]),
    'controller/pauselayoutgardenhd.json': ('playerinventoryoriginallayouthd.json', [('close', 'TouchClose', '"rect": { "x": 2271, "y": -158 }, "filename": "PANEL\\\\HUD_02\\\\front_panel", "hoveredFrame": 0, "onClickMessage": "PanelManager:ClosePanel:UISwitcher"')]),




    'controller/pauselayouthd.json': ('playerinventoryoriginallayouthd.json', [('close', 'TouchClose', '"rect": { "x": 2271, "y": -158 }, "filename": "PANEL\\\\HUD_02\\\\front_panel", "hoveredFrame": 0, "onClickMessage": "PanelManager:ClosePanel:UISwitcher"')]),


    'controller/hiremenupanelhd.json': ('hiremenupanelhd.json', [('CloseButton', 'TouchClose', '"rect": { "x": 2490, "y": -203 }, "filename": "PANEL\\\\HUD_02\\\\front_panel", "hoveredFrame": 0')]),




    'controller/imbueitemspanelhd.json': ('imbueitemspanelhd.json', [('Cancel', 'TouchClose', '"rect": { "x": 2741, "y": -156 }, "filename": "PANEL\\\\HUD_02\\\\front_panel", "hoveredFrame": 0, "onClickMessage": "PanelManager:ClosePanel:ImbueItemsPanel"')]),




    'controller/hirelinginventorypanelhd.json': ('hirelinginventorypanelhd.json', [('CloseButton', 'TouchClose', '"rect": { "x": 2741, "y": -155 }, "filename": "PANEL\\\\HUD_02\\\\front_panel", "hoveredFrame": 0, "onClickMessage": "PanelManager:ClosePanel:UISwitcher"')]),
    'controller/waypointspanelexpansionhd.json': ('waypointspaneloriginalhd.json', [('CloseButton', 'TouchClose', '"rect": { "x": 2267, "y": -153 }, "filename": "PANEL\\\\HUD_02\\\\front_panel", "hoveredFrame": 0')]),




    'controller/console/settingspanelhd.json': ('settingspanelhd.json', [('ActionButtonsTable', None, '"rect": { "x": -750, "y": 1938 }'),






        ('close', 'TouchClose', '"anchor": { "x": 0.5 }, "rect": { "x": 1595, "y": 60, "scale": 1.16 }, "filename": "PANEL\\\\HUD_02\\\\front_panel", "hoveredFrame": 0, "onClickMessage": "PanelManager:ClosePanel:TouchNoop"', 'playerinventoryoriginallayouthd.json')]),
}



def _char_no_stamina(s):

    s, n = offscreen(s, types=(), names=('Row06', 'Divider06', 'Divider07', 'StaminaTitle', 'MaxStamina', 'CurrentStamina'))
    if n != 6: raise SystemExit('touch_layout：人物页耐力行控件应为 6 个，找到 %d 个' % n)
    for name, y in (('Row07', 1075), ('Divider08', 1092), ('Divider09', 1092), ('HPTitle', 1089), ('MaxHP', 1093), ('CurrentHP', 1093)):
        i = s.index('"name": "%s"' % name); m = re.compile(r'"y":\s*(\d+)').search(s, i)
        if int(m.group(1)) != y: raise SystemExit('touch_layout：人物页 %s 的 y 不是 %d（原版布局变了？）' % (name, y))
        s = s[:m.start(1)] + str(y - 106) + s[m.end(1):]; n += 1
    return s, n














def _char_title_up(s):
    a = re.compile(r'("type": "TextBoxWidget", "name": "AttackDamageTitle",\s*"fields": \{\s*"rect": \{ "x": 0, "y": )0(, "width": \d+, "height": 84 \})')
    s, n = a.subn(lambda m: m.group(1) + '-12' + m.group(2), s)
    if n != 2: raise SystemExit('touch_layout：角色页「伤害」标题不是原样（找到 %d 处，应 2 处）' % n)
    return s, n
def _overlay_touch(s):
    s, n = offscreen(s)
    if n != 1: raise SystemExit('touch_layout：外框底部提示条应为 1 个，找到 %d 个' % n)
    return s, n


def _inv_touch(hint):
    def fn(s):
        s, n = offscreen(s)
        return s, n
    return fn














_RW_N = 15
def _rw_text(name, w, size, color, h='left', fit=False, ht=56):
    return ('        { "type": "TextBoxWidget", "name": "%s", "fields": { "rect": { "x": 99999, "y": 99999, "width": %d, "height": %d }, "text": "", %s'
            '"style": { "pointSize": %d, "fontColor": { "r": %d, "g": %d, "b": %d, "a": 255 }, "alignment": { "h": "%s", "v": "center" } } } },\n') % ((name, w, ht, '"fitToText": true, ' if fit else '', size) + color + (h,))
_RW = ('        { "type": "Widget", "name": "TouchRwBox", "children": [\n'
       + '        { "type": "RectangleWidget", "name": "TouchRwBg", "fields": { "rect": { "x": 99999, "y": 99999, "width": 1125, "height": 400 }, "color": [ 0.055, 0.055, 0.055, 0.92 ] } },\n'
       + _rw_text('TouchRwTitle', 1125, 38, (235, 235, 235), 'center')


       + ''.join('        { "type": "ButtonWidget", "name": "TouchRwPg%s", "fields": { "rect": { "x": 99999, "y": 99999, "width": 64, "height": 64 }, "filename": "PANEL\\\\Stash\\\\Stash_%sArrow", '
                 '"normalFrame": 0, "pressedFrame": 2, "hoveredFrame": 1, "disabledFrame": 3, "onClickMessage": "PanelManager:ClosePanel:TouchRwPg%s" } },\n' % (k, a, k) for k, a in (('Prev', 'Left'), ('Next', 'Right')))


       + ''.join('        { "type": "ImageWidget", "name": "TouchRwSep%d", "fields": { "rect": { "x": 99999, "y": 99999, "scale": 1.25 }, "filename": "PauseMenu\\\\Divider_Medium", "transparency": 0.35 } },\n' % k for k in range(7))
       + '        { "type": "TextBoxWidget", "name": "TouchRwPage", "fields": { "text": "", "rect": { "x": 99999, "y": 99999, "width": 230, "height": 64 }, '
         '"style": { "fontColor": "$PanelTitleColor", "pointSize": "$MediumFontSize", "alignment": { "v": "center", "h": "center" }, "spacing": "$MinimumSpacing", "dropShadow": "$DefaultDropShadow" } } },\n'




       + '        { "type": "Widget", "name": "TouchRwRows", "children": [\n'
       + ''.join(_rw_text('TouchRwName%d' % i, 520, 36, (199, 179, 119), fit=True, ht=46) + _rw_text('TouchRwLv%d' % i, 300, 28, (160, 160, 160), fit=True, ht=40)
                 + ''.join(_rw_text('TouchRwR%d_%d%s' % (i, j, c), 300, 30, col, fit=True, ht=40) for j in range(6) for c, col in (('w', (235, 235, 235)), ('r', (255, 90, 90))))


                 + ('        { "type": "TextBoxWidget", "name": "TouchRwHint%d", "fields": { "rect": { "x": 99999, "y": 99999, "width": 400, "height": 40 }, "text": "@dmRwHint", "fitToText": true, '
                    '"style": { "pointSize": 28, "fontColor": "$FontColorGray", "alignment": { "h": "left", "v": "center" } } } },\n') % i
                 + ('        { "type": "ButtonWidget", "name": "TouchRwGo%d", "fields": { "rect": { "x": 99999, "y": 99999, "width": 408, "scale": 0.75 }, "filename": "FrontEnd/HD/Final/FrontEnd_ButtonMed", "hoveredFrame": 3, '
                    '"textString": "@dmRwGo", "textColor": "$LightButtonTextColor", "text/style": "$StyleFEButtonText", "onClickMessage": "PanelManager:ClosePanel:TouchRwGo%d" } },\n') % (i, i)
                 for i in range(_RW_N))
       + '        ] },\n'



       + '        { "type": "NineTileImageWidget", "name": "TouchRwSel", "fields": { "rect": { "x": 99999, "y": 99999, "width": 1000, "height": 100 }, "backgroundColor": [ 0, 0, 0, 0 ], '
         '"filename": "Controller/PANEL/ControllerSelector/ControllerSelectorBorder", "tileScale": 0.5 } },\n'






       + ''.join('        { "type": "TextBoxWidget", "name": "TouchRwDesc%s", "fields": { "rect": { "x": 99999, "y": 99999, "width": 1000, "height": 600 }, "text": "", "fitToText": true, '
                 '"style": { "fontColor": "$FontColorWhite", "pointSize": %d, "alignment": { "h": "center", "v": "top" }, "options": { "pipeCharacterBehavior": "handle", "newlineHandling": "reversed" } } } },\n' % (k, pt)
                 for k, pt in (('', 32), ('S', 22)))
       + '        { "type": "ButtonWidget", "name": "TouchRwBack", "fields": { "rect": { "x": 99999, "y": 99999, "width": 64, "height": 64 }, "filename": "PANEL\\\\Stash\\\\Stash_LeftArrow", '
         '"normalFrame": 0, "pressedFrame": 2, "hoveredFrame": 1, "disabledFrame": 3, "onClickMessage": "PanelManager:ClosePanel:TouchRwBack" } },\n'
       + _rw_text('TouchRwBackTxt', 300, 34, (199, 179, 119), fit=True, ht=64).replace('"text": "",', '"text": "@Return",')
       + ('        { "type": "ButtonWidget", "name": "TouchRwDetGo", "fields": { "rect": { "x": 99999, "y": 99999, "width": 408, "scale": 0.75 }, "filename": "FrontEnd/HD/Final/FrontEnd_ButtonMed", "hoveredFrame": 3, '
          '"textString": "@dmRwGo", "textColor": "$LightButtonTextColor", "text/style": "$StyleFEButtonText", "onClickMessage": "PanelManager:ClosePanel:TouchRwDetGo" } },\n')
       + '        ] },\n')
def _rw_children_check():

    import json
    t = '{"c":[' + _RW.replace('        ] },\n', ']},').rstrip().rstrip(',') + ']}'
    def walk(w, path):
        ch = w.get('children', [])
        if len(ch) > 256: raise SystemExit('touch_layout：符文之语清单容器 %s 直接子控件 %d 个，超过 256' % (path, len(ch)))
        for c in ch: walk(c, path + '/' + c.get('name', '?'))
    walk(json.loads(re.sub(r',(\s*[}\]])', r'\1', t)), '')
def _inv_rw(s):
    _rw_children_check()
    s, n = offscreen(s)
    i = s.rstrip().rfind(']'); assert s.rstrip().endswith('}') and i > 0
    return s[:i].rstrip(' ') + _RW + '    ' + s[i:], n + 1
def pad_inv_rw(s):
    _rw_children_check()
    i = s.rstrip().rfind(']'); assert s.rstrip().endswith('}') and i > 0
    return s[:i].rstrip(' ') + _RW + '    ' + s[i:]






def _bind_touch(s):
    n = 0
    for pre, names in (('SkillButton', ('@skillname0', 'F1', 'F3', 'F2', 'F4')), ('AltSkillButton', ('@skillname0', 'F5', 'F7', 'F6', 'F8'))):
        for i, label in enumerate(names, 1):
            pn = '%s%dPrompt' % (pre, i) if pre == 'SkillButton' else ('Bottom', 'Right', 'Left', 'Top', 'RB')[i - 1] + 'ButtonPrompt'
            m = re.search(r'\{\s*"type":\s*"SkillButtonPromptWidget",\s*"name":\s*"%s"' % pn, s)
            if not m: raise SystemExit('touch_layout：技能绑定页没有 %s（原版布局变了？）' % pn)
            s = s[:m.start()] + ('{\n                                    "type": "TextBoxWidget", "name": "TouchLabel",\n                                    "fields": {\n'
                '                                        "anchor": { "x": 0.5, "y": 1.0 }, "rect": { "x": -70, "y": 2, "width": 140, "height": 44 }, "text": "%s",\n'
                '                                        "style": { "fontColor": "$FontColorGoldYellow", "pointSize": "$SmallFontSize", "alignment": { "h": "center", "v": "center" } },\n'
                '                                    }\n                                },\n                                ') % label + s[m.start():]; n += 1
        for a_, b_ in ((3, 965), (4, 830)):
            m = re.search(r'("name":\s*"%s%d",\s*"fields":\s*\{\s*"rect":\s*\{ "x": )(\d+)' % (pre, a_), s)
            if not m or int(m.group(2)) != (830 if a_ == 3 else 965): raise SystemExit('touch_layout：技能绑定页 %s%d 的位置不是原样' % (pre, a_))
            s = s[:m.start(2)] + str(b_) + s[m.end(2):]; n += 1
    return s, n


def _pause_no_help(ys, hide=('Help',), restart_y=None, extra=()):








    def fn(s):
        s = _sub1(s, '"textString": "@minipanelmessage",', '"textString": "@dmPauseShareLog",')
        s = _sub1(s, '"onClickMessage": "PanelManager:OpenPanel:MessageLogPanel",', '"onClickMessage": "PanelManager:ClosePanel:TouchShareLog",')
        s, n = offscreen(s, types=(), names=hide)
        if n != len(hide): raise SystemExit('touch_layout：暂停菜单要挪走的按钮应为 %d 个，找到 %d 个' % (len(hide), n))
        for name, (y0, y1) in ys.items():
            m = re.search(r'"type":\s*"ButtonWidget",\s*"name":\s*"%s",\s*"fields":\s*\{\s*("rect":\s*\{ "x": 564, "y": (\d+)\s*\})' % name, s)
            if not m or int(m.group(2)) != y0: raise SystemExit('touch_layout：暂停菜单按钮 %s 的位置不是原样（原版布局变了？）' % name)
            s = s[:m.start(1)] + '"rect": { "x": 564, "y": %d }' % y1 + s[m.end(1):]; n += 1




        if restart_y is not None:
            b = _widget_block(s, 'Exit'); e = s.index(b) + len(b)
            r = _sub1(_sub1(_sub1(b, '"name": "Exit"', '"name": "TouchRestart"'), '"textString": "@SaveAndExit"', '"textString": "@dmPauseRestart"'), '"onClickMessage": "PausePanelMessage:ExitGame"', '"onClickMessage": "PanelManager:ClosePanel:TouchRestart"')
            r = re.sub(r'"rect":\s*\{ "x": 564, "y": \d+ \}', '"rect": { "x": 564, "y": %d }' % restart_y, r, count=1)
            s = s[:e] + ',\n        ' + r + s[e:]; n += 1



        for name, text, y in extra:
            b = _widget_block(s, 'Exit'); e = s.index(b) + len(b)
            r = _sub1(_sub1(_sub1(b, '"name": "Exit"', '"name": "%s"' % name), '"textString": "@SaveAndExit"', '"textString": "%s"' % text), '"onClickMessage": "PausePanelMessage:ExitGame"', '"onClickMessage": "PanelManager:ClosePanel:%s"' % name)
            r = re.sub(r'"rect":\s*\{ "x": 564, "y": \d+ \}', '"rect": { "x": 564, "y": %d }' % y, r, count=1)
            s = s[:e] + ',\n        ' + r + s[e:]; n += 1
        return s, n
    return fn










def pad_pause(s, garden, share=True):
    ex = (('TouchPadDiff', '', 278), ('TouchPadStore', '@strGoldDeposit', 778)); y = 278
    sh = lambda y0, y1: {'MessageLog': (y0, y1)} if share else {}; hs = () if share else ('MessageLog',)
    if garden: s = _pause_no_help({'Settings': (428, 653), 'Exit': (728, 403), 'Chronicle': (1028, 903), **sh(278, 1028)}, hide=('Help', 'LootFilter') + hs, restart_y=528, extra=ex)(s)[0]
    else:
        s = _sub1(s, '"filename": "Controller/Panel/EscapeMenu/EscapeOptionsBGClassic"', '"filename": "Controller/Panel/EscapeMenu/EscapeOptionsBG"')
        s = _pause_no_help({'Settings': (578, 653), 'Exit': (878, 403), **sh(428, 903)}, hide=('Help',) + hs, restart_y=528, extra=ex)(s)[0]




    i = s.rstrip().rfind(']'); assert s.rstrip().endswith('}') and i > 0
    return (s[:i].rstrip(' ') + '        { "type": "TextBoxWidget", "name": "TouchPadDiffTxt", "fields": { "rect": { "x": 564, "y": %d, "width": 664, "height": 120 }, "text": "@OptionsMenuGameDifficulty", '
            '"style": { "fontColor": "$LightButtonTextColor", "pointSize": "$MediumLargeFontSize", "alignment": { "h": "center", "v": "center" } } } },\n    ' % y + s[i:])






def _chronicle_touch(s):
    if '"onClickMessage": "PanelManager:ClosePanel:ChroniclePanel"' not in s: raise SystemExit('touch_layout：编年史关闭消息不是原样（原版布局变了？）')
    add = (',\n        {\n            "type": "Widget", "name": "TouchCloseBox",\n            "fields": { "rect": { "x": -998, "y": 256, "width": 2600, "height": 121, "scale": "$ConsoleUIScale" }, "anchor": { "x": 0.5 } },\n            "children": [\n'
           '                { "type": "ButtonWidget", "name": "TouchClose", "fields": { "rect": { "x": 2236, "y": -171 }, "filename": "PANEL\\\\HUD_02\\\\front_panel", "sound": "cursor_close_window_hd", "onClickMessage": "PanelManager:ClosePanel:ChroniclePanel" } }\n'
           '            ]\n        }\n    ')
    end = s.rstrip().rstrip('}').rstrip(); assert end.endswith(']'); at = len(end) - 1
    return s[:at].rstrip() + add + s[at:], 1








KB_ROW_H = 188


KB_ICON = {'andariel': 'a1q6', 'countess': 'a1q4', 'bloodraven': 'a1q2', 'cowking': 'Misc\\CowKing', 'corpsefire': 'a1q1', 'smith': 'a1q5', 'griswold': 'a1q3', 'rakanishu': 'a1q3',
           'duriel': 'a2q6', 'summoner': 'a2q5', 'radament': 'a2q1', 'fangskin': 'a2q3',
           'mephisto': 'a3q6', 'council': 'a3q5', 'bremm': 'a3q6', 'wyand': 'a3q6', 'maffer': 'a3q6',
           'diablo': 'a4q2', 'vizier': 'a4q2', 'deseis': 'a4q2', 'infector': 'a4q2', 'izual': 'a4q1', 'hephasto': 'a4q3',
           'baal': 'a5q6', 'nihlathak': 'a5q4', 'shenk': 'a5q1', 'frozenstein': 'a5q3', 'ancients': 'a5q5',
           'colenzo': 'a5q6', 'achmel': 'a5q6', 'bartuc': 'a5q6', 'ventar': 'a5q6', 'lister': 'a5q6'}
def _kb_keys():
    src = open(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'host', 'dm_kills.c'), encoding='utf-8').read()
    ks = re.findall(r'\{"([a-z]+)", (?:"[^"]*"|NULL), ([1-5]), K_', src)
    if len(ks) < 60 or len({k for k, _ in ks}) != len(ks): raise SystemExit('touch_layout：从 dm_kills.c 读到的积分牌名单不对（%d 行）' % len(ks))
    return [(k, int(a)) for k, a in ks]
def _kb_text(name, x, y, w, h, color, size, align='left', text=''):
    return {'type': 'TextBoxWidget', 'name': name, 'fields': {'rect': {'x': x, 'y': y, 'width': w, 'height': h}, 'text': text,
            'style': {'fontColor': color, 'pointSize': size, 'alignment': {'h': align, 'v': 'center'}}}}
def _kb_check(name, x, label):
    return [{'type': 'ToggleButtonWidget', 'name': name, 'fields': {'rect': {'x': x, 'y': 0, 'width': 65, 'height': 65}, 'sound': 'cursor_menu_select_hd',
             'filename': 'Lobby\\CreateGame\\CreateGame_AdvancedCheckbox', 'untoggledFrame': 0, 'untoggledPressedFrame': 1, 'untoggledHoveredFrame': 3, 'untoggledDisabledFrame': 2,
             'toggledFrame': 4, 'toggledPressedFrame': 5, 'toggledHoveredFrame': 6, 'onClickMessage': 'PanelManager:ClosePanel:' + name}},
            {'type': 'TextBoxWidget', 'name': name + '_Text', 'fields': {'text': label, 'rect': {'x': x + 80, 'y': -5, 'height': 50}, 'style': '$StyleSettingsTitle', 'fitToText': True}}]
_KB_SPACER = {'type': 'Widget', 'name': 'spacer', 'fields': {'rect': {'width': 20, 'height': 65}}}
def _kb_row(key):
    ic = KB_ICON.get(key, 'Done')
    return {'type': 'Widget', 'name': 'TouchKB_' + key, 'fields': {'rect': {'x': 0, 'y': 0, 'width': 1157, 'height': KB_ROW_H}}, 'children': [
        {'type': 'ImageWidget', 'name': 'Bg', 'fields': {'filename': 'PANEL\\Chronicle\\Slot_list_item_XL'}},
        {'type': 'ImageWidget', 'name': 'Icon', 'fields': {'rect': {'x': 22, 'y': 24, 'scale': 0.83}, 'filename': 'QuestIcons\\' + ic}},
        _kb_text('Name', 180, 8, 640, 60, '$FontColorWhite', '$LootFilterMediumLargeFontSize'),
        _kb_text('Kills', 760, 8, 360, 60, '$FontColorGoldYellow', '$LootFilterMediumLargeFontSize', 'right'),
        _kb_text('First', 180, 76, 560, 48, '$FontColorMoreLightGray', '$LootFilterSmallFontSize'),
        _kb_text('Drops', 740, 76, 380, 48, '$FontColorMoreLightGray', '$LootFilterSmallFontSize', 'right'),
        _kb_text('Times', 180, 126, 940, 48, '$FontColorMoreLightGray', '$LootFilterSmallFontSize')]}
def _kb_board():
    acts = []
    for i in range(6):
        y = 80 + 100 * i
        acts += [{'type': 'ImageWidget', 'name': 'TouchKB_ActArrow%d' % i, 'fields': {'rect': {'x': 0, 'y': y}, 'filename': 'PANEL\\Chronicle\\Btn_collapse'}},
                 {'type': 'ToggleButtonWidget', 'name': 'TouchKB_Act%d' % i, 'fields': {'rect': {'x': 100, 'y': y, 'width': 360, 'height': 100}, 'sound': 'cursor_menu_select_hd',
                  'filename': 'PANEL\\Chronicle\\Slot_list_small_golddeco', 'untoggledFrame': 0, 'toggledFrame': 1, 'onClickMessage': 'PanelManager:ClosePanel:TouchKB_Act%d' % i},
                  'children': [_kb_text('Text', 35, 0, 315, 100, '$FontColorWhite', '$MediumFontSize', text=('@dmKbAct%d' % (i + 1)) if i < 5 else '@dmKbRift')]}]




    return {'type': 'Widget', 'name': 'TouchKillBoard', 'fields': {'rect': '$OptionsPanelRect', 'anchor': {'x': 0.5}}, 'children': [
        {'type': 'Widget', 'name': 'TouchKB_Body', 'fields': {'rect': {'x': 99999, 'y': 0, 'width': 1730, 'height': 1284}}, 'children':
        [{'type': 'StackWidget', 'name': 'TouchKB_DiffStack', 'fields': {'rect': {'x': 0, 'y': 0, 'height': 65, 'width': 1300}, 'direction': 'LeftRight', 'padding': 15},
          'children': _kb_check('TouchKB_D0', 0, '@strCreateGameNormalText') + [_KB_SPACER] + _kb_check('TouchKB_D1', 406, '@strCreateGameNightmareText') + [_KB_SPACER] + _kb_check('TouchKB_D2', 950, '@strCreateGameHellText')}] + [
        {'type': 'DropdownListWidget', 'name': 'TouchKB_Sort', 'fields': {'rect': {'x': -390, 'y': 0, 'width': 610, 'height': 105, 'scale': 0.65},
            'background': {'rect': {'width': 610, 'height': 105}, 'filename': 'PANEL/DropdownButton_Large_Center', 'leftCap/filename': 'PANEL/DropdownButton_Large_Left', 'rightCap/filename': 'PANEL/DropdownButton_Large_Right'},


            'pressedFrame': 1, 'disabledFrame': 2, 'hoveredFrame': 3, 'anchor': {'x': 1.0}, 'states': ['@dmKbSortAct', '@dmKbSortCount', '@dmKbSortFast'],
            'onUpdateMessage': 'TouchKillBoardMessage:Sort', 'text/rect': {'scale': 1.538}, 'text/anchor': {'x': 0, 'y': -0.3}, 'textMargin': {'left': 20, 'right': 290},
            'text/style': '$StyleFilterRuleListButtonTextSmall', 'text/truncateWithEllipsis': True, 'textColor': '$FontColorLightGold'}},
        {'type': 'ImageWidget', 'name': 'TouchKB_TypeBar', 'fields': {'rect': {'x': 466, 'y': 80}, 'filename': 'Panel\\Chronicle\\Chronicle_ScrollBar_Small'}}] + acts + [
        {'type': 'ImageWidget', 'name': 'TouchKB_TotalBox', 'fields': {'rect': {'x': 0, 'y': 1000, 'width': 450, 'height': 110}, 'filename': 'PANEL\\Chronicle\\Footer_BG_Progress'}, 'children': [
            {'type': 'TextBoxWidget', 'name': 'TouchKB_Total', 'fields': {'text': '', 'rect': {'y': 15, 'width': 450, 'height': 40}, 'style': {'pointSize': '$MediumFontSize', 'alignment': {'h': 'center', 'v': 'center'}}}},
            {'type': 'ImageWidget', 'name': 'FillBg', 'fields': {'rect': {'x': 25, 'y': 65, 'width': 450, 'height': 80}, 'filename': 'PANEL\\Chronicle\\ProgressBar_Small_2'}}]},
        ] + [_kb_page(i) for i in range(5)] + [_kb_rift_page()]}]}


def _kb_page(i):
    return {'type': 'Widget', 'name': 'TouchKB_Page%d' % i, 'fields': {'rect': {'x': 99999, 'y': 0, 'width': 1730, 'height': 1284}}, 'children': [
        {'type': 'ImageWidget', 'name': 'TouchKB_ListBar', 'fields': {'rect': {'x': -20, 'y': 80}, 'anchor': {'x': 1.0}, 'filename': 'Panel\\Chronicle\\Chronicle_Scrollbar_Large'}, 'children': [
            {'type': 'ScrollControllerWidget', 'name': 'TouchKB_ScrollController%d' % i, 'fields': {'rect': {'x': -31, 'y': 29, 'width': 26, 'height': 1095}, 'anchor': {'x': 1.0},
             'upArrowFilepath': 'FrontEnd\\HD\\Final\\FrontEnd_ScrollUpBtn', 'downArrowFilepath': 'FrontEnd\\HD\\Final\\FrontEnd_ScrollDownBtn', 'barFilepath': 'PauseMenu\\VerticalIndicator',
             'viewName': 'TouchKB_Scroll%d' % i, 'wheelScrollSound': 'cursor_scroll_hd', 'buttonScrollSound': 'cursor_scroll_hd'}}]},
        {'type': 'Widget', 'name': 'TouchKB_ListBox', 'fields': {'rect': {'x': 530, 'y': 80, 'width': 1157, 'height': 1200}}, 'children': [
            {'type': 'ScrollViewWidget', 'name': 'TouchKB_Scroll%d' % i, 'fields': {'fitToParent': True, 'scrollControllerName': 'TouchKB_ScrollController%d' % i}, 'children': [
                {'type': 'Widget', 'name': 'TouchKB_List', 'fields': {'rect': {'x': 0, 'y': 0, 'width': 1157, 'height': KB_ROW_H * sum(1 for _, a in _kb_keys() if a == i + 1)}},
                 'children': [_kb_row(k) for k, a in _kb_keys() if a == i + 1]}]}]}]}





























RF_T0 = -24
RF_EN_CAP, RF_EN_NOTE = 31, 26
RF_EN_PT = 32
RF_SLOT = (20, 20, 140)
RF_MS = [(10, '@dmRiftMsItem10', '@dmRiftMsSub10', 'Items/misc/charm/charm_large', (49, 147), (2, 4, 48, 146)), (20, '@dmRiftMsItem20', '', 'Items/misc/quest/mephisto_soul_stone', (49, 49), (9, 4, 39, 49)),
         (30, '@dmRiftMsItem30', '', 'Items/misc/torch/torch', (49, 98), (9, 5, 38, 95)), (40, '@dmRiftMsItem40', '@dmRiftMsSub40', 'Items/weapon/polearm/war_scythe', (98, 196), (3, 3, 93, 192)),
         (50, '@dmRiftMsItem50', '@dmRiftMsSub50', 'Items/misc/body_part/fragment_magic', (49, 49), (2, 0, 48, 48), 1.3), (60, '@dmRiftMsItem60', '', 'Items/weapon/orb/dragon_stone', (49, 147), (5, 7, 46, 136)),
         (70, '@dmRiftMsItem70', '', 'Items/armor/circlet/diadem', (98, 98), (0, 20, 95, 87)), (80, '@dmRiftMsItem80', '', 'Items/armor/armor/victors_silk', (98, 147), (2, 10, 94, 139)),
         (90, '@dmRiftMsItem90', '@dmRiftMsSub90', 'Items/misc/charm/charm_large', (49, 147), (2, 4, 48, 146)), (100, '@dmRiftMsItem100', '@dmRiftMsSub100', 'Items/misc/rune/zod_rune', (49, 49), (3, 4, 47, 48))]
def _rf_btn(name, x, y, sprite, frames, scale, text='', pt='$MediumLargeFontSize'):
    f = {'rect': {'x': x, 'y': y, 'scale': scale}, 'filename': sprite, 'onClickMessage': 'PanelManager:ClosePanel:' + name, 'sound': 'cursor_menu_select_hd'}
    f.update(frames)
    if text: f.update({'textString': text, 'pointSize': pt, 'textColor': '$LightButtonTextColor', 'text/style': '$StyleFEButtonText'})
    return {'type': 'ButtonWidget', 'name': name, 'fields': f}
_RF_ARROW = {'hoveredFrame': 1, 'pressedFrame': 2, 'disabledFrame': 3}
_RF_BTN = {'pressedFrame': 1, 'disabledFrame': 2, 'hoveredFrame': 3}
def _rf_icon(icon, px, bb, up=1.0):
    x0, y0, side = RF_SLOT; bw, bh = (bb[2] - bb[0]) * 2, (bb[3] - bb[1]) * 2
    k = side * 0.95 / bh if bh > bw * 1.4 and px[1] * 2 > side else min(side * 0.95 / bw, side * 0.95 / bh, up)
    return {'type': 'ImageWidget', 'name': 'Icon', 'fields': {'rect': {'x': round(x0 + side / 2 - (bb[0] * 2 + bw / 2) * k), 'y': round(y0 + side / 2 - (bb[1] * 2 + bh / 2) * k), 'scale': round(k, 4)}, 'filename': icon}}
def _rf_row(i, m, name, sub, icon, px, bb, up=1.0):


    dx = 34 if m >= 100 else 0
    return {'type': 'Widget', 'name': 'TouchRF_Row%d' % i, 'fields': {'rect': {'x': 0, 'y': KB_ROW_H * i, 'width': 1157, 'height': KB_ROW_H}}, 'children': [
        {'type': 'ImageWidget', 'name': 'Bg', 'fields': {'filename': 'PANEL\\Chronicle\\Slot_list_item_XL'}}, _rf_icon(icon, px, bb, up),
        _kb_text('Tier', 180, 30 if sub else 0, 150 + dx, 70 if sub else 188, '$FontColorWhite', '$MediumLargeFontSize', text='@dmRiftMsTier%d' % m),
        _kb_text('Name', 330 + dx, 30 if sub else 0, 520 - dx, 70 if sub else 188, '$FontColorGoldYellow', '$MediumLargeFontSize', text=name),
        _kb_text('Sub', 180, 100, 670, 56, '$FontColorMoreLightGray', '$MediumFontSize', text=sub),
        _kb_text('State', 850, 0, 287, 188, '$FontColorMoreLightGray', '$MediumLargeFontSize', 'right', text='@dmRiftMsNo')]}
def _rf_kv(k, y, label, vcolor='$FontColorWhite'):






    return [_kb_text('TouchRF_%sL' % k, 0, y, 250, 56, '$FontColorMoreLightGray', '$MediumFontSize', 'left', label),
            _kb_text('TouchRF_%sLE' % k, 99999, y, 330, 56, '$FontColorMoreLightGray', RF_EN_PT, 'left', label),
            _kb_text('TouchRF_%s' % k, 230, y, 250, 56, vcolor, '$MediumFontSize', 'right')]
RF_LW = 500
RF_X10_K = 0.66
RF_X10 = (0, 258)
RF_DIV_W = 1590
def _rf_div(name, x, y, w=500):
    return {'type': 'ImageWidget', 'name': name, 'fields': {'rect': {'x': x, 'y': y, 'scale': round(w / RF_DIV_W, 4)}, 'filename': 'PauseMenu\\Divider'}}
def _rf_key(name, cap, by):
    return {'type': 'ImageWidget', 'name': name, 'fields': {'rect': {'x': 99999, 'y': by + 26, 'scale': 1.5}, 'filename': 'Controller/ControllerButtons/Switch/' + cap}}
def _rf_keys(name, cap, by):
    k = _rf_key(name, cap, by); return [k] + ([dict(k, name=name + 'K', fields=dict(k['fields'], filename='Ztouch_controller_buttons_ui/switchK' + cap))] if os.environ.get('DM_OVERLAY_MAC') else [])
def _rift_board():






    sel = [_kb_text('TouchRF_PickT', 0, RF_T0, 500, 60, '$FontColorGoldYellow', '$MediumFontSize', 'left', '@dmRiftSelT'), _rf_div('TouchRF_DivA', 0, 62),
           _rf_btn('TouchRF_M1', 0, 96, 'Lobby/Final/LobbyArrowLargeL', _RF_ARROW, 1.0), _rf_btn('TouchRF_P1', 406, 96, 'Lobby/Final/LobbyArrowLargeR', _RF_ARROW, 1.0),
           _kb_text('TouchRF_Num', 94, 96, 312, 108, '$FontColorLightGold', '$HugeFontSize', 'center', '1'),
           _rf_btn('TouchRF_M10', RF_X10[0], 224, 'FrontEnd/HD/Final/FrontEnd_ButtonSmall', _RF_BTN, RF_X10_K, '-10', 52), _rf_btn('TouchRF_P10', RF_X10[1], 224, 'FrontEnd/HD/Final/FrontEnd_ButtonSmall', _RF_BTN, RF_X10_K, '+10', 52)]
    sel += [_kb_text('TouchRF_InfT', 0, 332, 500, 60, '$FontColorGoldYellow', '$MediumFontSize', 'left', '@dmRiftInfT'), _rf_div('TouchRF_DivB', 0, 394)]
    for n, (k, lab) in enumerate((('Range', '@dmRiftRange'), ('Best', '@dmRiftBest'), ('Str', '@dmRiftStr'), ('Lvl', '@dmRiftLvl'), ('Time', '@dmRiftTime'))): sel += _rf_kv(k, 414 + 64 * n, lab)
    sel += [_kb_text('TouchRF_GamT', 0, 760, 500, 60, '$FontColorGoldYellow', '$MediumFontSize', 'left', '@dmRiftGamT'), _rf_div('TouchRF_DivC', 0, 822)] + []




    for n in range(4): sel += [_kb_text('TouchRF_Gam%dL' % n, 0, 842 + 64 * n, 300, 56, '$FontColorMoreLightGray', '$MediumFontSize', 'left'), _kb_text('TouchRF_Gam%dLE' % n, 99999, 842 + 64 * n, 330, 56, '$FontColorMoreLightGray', RF_EN_PT, 'left'), _kb_text('TouchRF_Gam%dU' % n, 230, 842 + 64 * n, 250, 56, '$FontColorGoldYellow', '$MediumFontSize', 'right'), _kb_text('TouchRF_Gam%dS' % n, 230, 842 + 64 * n, 250, 56, '$FontColorPartyGreen', '$MediumFontSize', 'right')]
    sel += [_rf_btn('TouchRF_Start', 0, 1155, 'FrontEnd/HD/Final/FrontEnd_ButtonMed', _RF_BTN, 1.064, '@dmRiftBtnStart')] + _rf_keys('TouchRF_KeyA', 'ButtonRight', 1155)




    cho = [_kb_text('TouchRF_ChT', 0, RF_T0, 500, 60, '$FontColorGoldYellow', '$MediumFontSize', 'left', '@dmRiftTitleRun'), _rf_div('TouchRF_DivD', 0, 62),
           _kb_text('TouchRF_ChTier', 0, 96, 500, 108, '$FontColorLightGold', '$HugeFontSize', 'center'),
           _kb_text('TouchRF_ChInfT', 0, 332, 500, 60, '$FontColorGoldYellow', '$MediumFontSize', 'left', '@dmRiftInfT'), _rf_div('TouchRF_DivE', 0, 394)]
    for n, (k, lab) in enumerate((('ChLeft', '@dmRiftTimeLeft'), ('ChProg', '@dmRiftProgress'), ('ChNote', ''))): cho += _rf_kv(k, 414 + 64 * n, lab)



    cho += [_kb_text('TouchRF_AbTE', 99999, 746, 500, 56, '$FontColorMoreLightGray', RF_EN_CAP, 'center'), _kb_text('TouchRF_ContTE', 99999, 1083, 500, 56, '$FontColorMoreLightGray', RF_EN_CAP, 'center')]
    cho += [_kb_text('TouchRF_AbT', 0, 746, 500, 56, '$FontColorMoreLightGray', '$SmallFontSize', 'center', '@dmRiftCapVoid'),
            _rf_btn('TouchRF_Abandon', 0, 818, 'FrontEnd/HD/Final/FrontEnd_ButtonMed', _RF_BTN, 1.064, '@dmRiftBtnAbandon'),
            _kb_text('TouchRF_ContT', 0, 1083, 500, 56, '$FontColorMoreLightGray', '$SmallFontSize', 'center', '@dmRiftCapCont'),
            _rf_btn('TouchRF_Cont', 0, 1155, 'FrontEnd/HD/Final/FrontEnd_ButtonMed', _RF_BTN, 1.064, '@dmRiftBtnCont'),



            _rf_btn('TouchRF_Resel', 99999, 818, 'FrontEnd/HD/Final/FrontEnd_ButtonMed', _RF_BTN, 1.064, '@dmRiftBtnResel'),
            ] + _rf_keys('TouchRF_KeyA2', 'ButtonRight', 1155) + _rf_keys('TouchRF_KeyX', 'ButtonTop', 818)
    right = [_kb_text('TouchRF_MsT', 530, RF_T0, 500, 60, '$FontColorGoldYellow', '$MediumFontSize', 'left', '@dmRiftMsT'), _rf_div('TouchRF_DivR', 530, 62, 1157),
             _kb_text('TouchRF_MsN', 1030, RF_T0, 657, 60, '$FontColorMoreLightGray', '$SmallFontSize', 'right', '@dmRiftMsN'),



             _kb_text('TouchRF_MsNE', 99999, RF_T0, 757, 60, '$FontColorMoreLightGray', RF_EN_NOTE, 'right', '@dmRiftMsNEn'),
             {'type': 'ImageWidget', 'name': 'TouchRF_ListBar', 'fields': {'rect': {'x': -20, 'y': 80}, 'anchor': {'x': 1.0}, 'filename': 'Panel\\Chronicle\\Chronicle_Scrollbar_Large'}, 'children': [
                 {'type': 'ScrollControllerWidget', 'name': 'TouchRF_ScrollController', 'fields': {'rect': {'x': -31, 'y': 29, 'width': 26, 'height': 1095}, 'anchor': {'x': 1.0},
                  'upArrowFilepath': 'FrontEnd\\HD\\Final\\FrontEnd_ScrollUpBtn', 'downArrowFilepath': 'FrontEnd\\HD\\Final\\FrontEnd_ScrollDownBtn', 'barFilepath': 'PauseMenu\\VerticalIndicator',
                  'viewName': 'TouchRF_Scroll', 'wheelScrollSound': 'cursor_scroll_hd', 'buttonScrollSound': 'cursor_scroll_hd'}}]},
             {'type': 'Widget', 'name': 'TouchRF_ListBox', 'fields': {'rect': {'x': 530, 'y': 80, 'width': 1157, 'height': 1200}}, 'children': [
                 {'type': 'ScrollViewWidget', 'name': 'TouchRF_Scroll', 'fields': {'fitToParent': True, 'scrollControllerName': 'TouchRF_ScrollController'}, 'children': [
                     {'type': 'Widget', 'name': 'TouchRF_List', 'fields': {'rect': {'x': 0, 'y': 0, 'width': 1157, 'height': KB_ROW_H * len(RF_MS)}},
                      'children': [_rf_row(i, *r) for i, r in enumerate(RF_MS)]}]}]}]
    return {'type': 'Widget', 'name': 'TouchRift', 'fields': {'rect': '$OptionsPanelRect', 'anchor': {'x': 0.5}}, 'children': [
        {'type': 'Widget', 'name': 'TouchRF_Body', 'fields': {'rect': {'x': 99999, 'y': 0, 'width': 1730, 'height': 1284}}, 'children': [
            {'type': 'Widget', 'name': 'TouchRF_Sel', 'fields': {'rect': {'x': 0, 'y': 0, 'width': 500, 'height': 1284}}, 'children': sel},
            {'type': 'Widget', 'name': 'TouchRF_Cho', 'fields': {'rect': {'x': 99999, 'y': 0, 'width': 500, 'height': 1284}}, 'children': cho}] + right}]}
def _rift_panel(s):






    for a in ('"type": "MessageLogPanel", "name": "MessageLogPanel"', '"name": "ListContainer"', '"name": "ScrollBar"', '"name": "ChatEntryTemplate"'):
        if s.count(a) != 1: raise SystemExit('touch_layout：消息记录面板不是原样（%s），大秘境面板没换' % a)
    off = {'rect': {'x': 99999, 'y': 0, 'width': 10, 'height': 10}}
    native = [{'type': 'ClickCatcherWidget', 'name': 'ClickCatcher', 'fields': off},
              {'type': 'RectangleWidget', 'name': 'MessageLogBackground', 'fields': off},
              {'type': 'ImageWidget', 'name': 'Background', 'fields': off},
              {'type': 'TextBoxWidget', 'name': 'title', 'fields': dict(off, text='')},
              {'type': 'ChatListWidget', 'name': 'ListContainer', 'fields': off, 'children': [
                  {'type': 'ListItemWidget', 'name': 'ChatEntryTemplate', 'fields': {'rect': {'x': 0, 'y': 0, 'width': 10, 'height': 10}}, 'children': [
                      {'type': 'TextBoxWithLinksWidget', 'name': 'MessageText', 'fields': {'fitToParent': True}}]}]},
              {'type': 'ImageWidget', 'name': 'ScrollBarBackground', 'fields': off, 'children': [
                  {'type': 'ScrollControllerWidget', 'name': 'ScrollBar', 'fields': {'rect': {'x': 0, 'y': 0, 'width': 10, 'height': 10}, 'viewName': 'ListContainer'}}]}]
    close = 'PanelManager:ClosePanel:MessageLogPanel'
    frame = [{'type': 'RectangleWidget', 'name': 'TouchRF_Dim', 'fields': {'fitToScreen': True, 'color': [0.0, 0.0, 0.0, 0.7]}, 'children': [
                 {'type': 'ClickCatcherWidget', 'name': 'TouchRF_Catcher', 'fields': {'fitToParent': True}},
                 {'type': 'Widget', 'name': 'TouchRF_Anchor', 'fields': {'anchor': {'x': 0.5, 'y': 0.5}, 'rect': '$SettingsPanelAnchorRect'}, 'children': [
                     {'type': 'ImageWidget', 'name': 'TouchRF_FrameBg', 'fields': {'filename': '\\PANEL\\Options\\FrontEndOptionsBG'}},
                     {'type': 'TextBoxWidget', 'name': 'TouchRF_Title', 'fields': {'rect': {'x': 0, 'y': 45, 'width': 1950, 'height': 103}, 'text': '@dmRiftPanelTitle', 'style': '$StyleTitleBlock'}},
                     {'type': 'ButtonWidget', 'name': 'TouchRF_CloseBtn', 'fields': {'rect': {'x': 1868, 'y': 8}, 'filename': 'PANEL\\closebtn_4x', 'hoveredFrame': 3, 'onClickMessage': close,
                      'tooltipString': '@strClose', 'sound': 'cursor_close_window_hd', 'acceptsEscKeyEverywhere': True}},








                     {'type': 'ImageWidget', 'name': 'TouchRF_CloseX', 'fields': {'rect': {'x': 1766, 'y': 64}, 'filename': 'PANEL\\HUD_02\\front_panel'}},
                     {'type': 'ButtonWidget', 'name': 'TouchClose', 'fields': {'rect': {'x': 1705, 'y': -11, 'scale': 2.4}, 'filename': 'Ztouch_controller_buttons_ui/switch_ButtonRight', 'sound': 'cursor_close_window_hd', 'onClickMessage': close}}]}]},
             {'type': 'ImageWidget', 'name': 'TouchRF_ContentBg', 'fields': {'rect': '$SettingsPanelBackgroundRect', 'anchor': {'x': 0.5}, 'filename': 'Controller/Panel/Options/Panel_Options_BG'}},
             ]
    board = _rift_board(); board['children'][0]['fields']['rect']['x'] = 0; board['children'][0]['fields']['rect']['y'] = -54
    lay = {'basedOn': 'MessageLogPanel_800x600HD.json', 'type': 'MessageLogPanel', 'name': 'MessageLogPanel',
           'fields': {'priority': 9002, 'fitToParent': True, 'defaultWidget': 'TouchRF_Start'}, 'children': native + frame + [board]}
    return json.dumps(lay, ensure_ascii=False, indent=1), 1


RIFT_ROWS = 100
def _kb_rift_row(name):
    return {'type': 'Widget', 'name': name, 'fields': {'rect': {'x': 99999, 'y': 0, 'width': 1157, 'height': KB_ROW_H}}, 'children': [
        {'type': 'ImageWidget', 'name': 'Bg', 'fields': {'filename': 'PANEL\\Chronicle\\Slot_list_item_XL'}},
        {'type': 'ImageWidget', 'name': 'Icon', 'fields': {'rect': {'x': 22, 'y': 24, 'scale': 0.83}, 'filename': 'QuestIcons\\Done'}},
        _kb_text('Name', 180, 8, 640, 60, '$FontColorWhite', '$LootFilterMediumLargeFontSize'),
        _kb_text('Kills', 760, 8, 360, 60, '$FontColorGoldYellow', '$LootFilterMediumLargeFontSize', 'right'),
        _kb_text('First', 180, 76, 560, 48, '$FontColorMoreLightGray', '$LootFilterSmallFontSize'),
        _kb_text('Drops', 740, 76, 380, 48, '$FontColorMoreLightGray', '$LootFilterSmallFontSize', 'right'),
        _kb_text('Times', 180, 126, 940, 48, '$FontColorMoreLightGray', '$LootFilterSmallFontSize')]}
def _kb_rift_page():
    return {'type': 'Widget', 'name': 'TouchKB_Page5', 'fields': {'rect': {'x': 99999, 'y': 0, 'width': 1730, 'height': 1284}}, 'children': [
        {'type': 'ImageWidget', 'name': 'TouchKB_ListBar', 'fields': {'rect': {'x': -20, 'y': 80}, 'anchor': {'x': 1.0}, 'filename': 'Panel\\Chronicle\\Chronicle_Scrollbar_Large'}, 'children': [
            {'type': 'ScrollControllerWidget', 'name': 'TouchKB_ScrollController5', 'fields': {'rect': {'x': -31, 'y': 29, 'width': 26, 'height': 1095}, 'anchor': {'x': 1.0},
             'upArrowFilepath': 'FrontEnd\\HD\\Final\\FrontEnd_ScrollUpBtn', 'downArrowFilepath': 'FrontEnd\\HD\\Final\\FrontEnd_ScrollDownBtn', 'barFilepath': 'PauseMenu\\VerticalIndicator',
             'viewName': 'TouchKB_Scroll5', 'wheelScrollSound': 'cursor_scroll_hd', 'buttonScrollSound': 'cursor_scroll_hd'}}]},
        {'type': 'Widget', 'name': 'TouchKB_ListBox', 'fields': {'rect': {'x': 530, 'y': 80, 'width': 1157, 'height': 1200}}, 'children': [
            {'type': 'ScrollViewWidget', 'name': 'TouchKB_Scroll5', 'fields': {'fitToParent': True, 'scrollControllerName': 'TouchKB_ScrollController5'}, 'children': [
                {'type': 'Widget', 'name': 'TouchKB_List', 'fields': {'rect': {'x': 0, 'y': 0, 'width': 1157, 'height': KB_ROW_H * (RIFT_ROWS + 2)}},
                 'children': [_kb_rift_row('TouchKB_RiftSum'), _kb_rift_row('TouchKB_RiftSum2')] + [_kb_rift_row('TouchKB_RiftT%d' % t) for t in range(1, RIFT_ROWS + 1)]}]}]}]}
def _chronicle_killboard(s):
    source = '"filename": "Panel\\\\Chronicle\\\\Chronicle_Tabs"'
    if s.count(source) != 1:
        raise ValueError("Unexpected Chronicle tab image")
    s = s.replace(source, '"filename": "ztouch_options_ui/chronicle_tabs_400"')

    import tab_art
    block = updated = _widget_block(s, "ChronicleTabs")
    for a, b in (('"pointSize": "$MediumFontSize"', '"pointSize": %d' % tab_art.CHRONICLE_TAB_SIZE), ('"truncateText": true', '"truncateText": false'),
                 ('"tabTextOffset": { "x": 0, "y": -4 }', '"tabTextOffset": { "x": 0, "y": -9 }')):
        if updated.count(a) != 1:
            raise ValueError("Unexpected Chronicle tab text field")
        updated = updated.replace(a, b)
    s = s.replace(block, updated)

    n = 0
    for a, b in (('"tabSize": { "x": 266, "y": 121 },', '"tabSize": { "x": 400, "y": 121 },'), ('"tabCount": 3,', '"tabCount": 4,'), ('"inactiveFrames": [ 0, 0, 0],', '"inactiveFrames": [ 0, 0, 0, 0],'), ('"activeFrames": [ 1, 1, 1],', '"activeFrames": [ 1, 1, 1, 1],'),
                 ('"textStrings": [ "@Unique", "@Sets", "@Runewords"],', '"textStrings": [ "@Unique", "@Sets", "@dmChronRunewords", "@Bosses"],'),
                 ('"ChroniclePanelMessage:ToggleChildPanel:ChronicleRunewordsListPanel"', '"ChroniclePanelMessage:ToggleChildPanel:ChronicleRunewordsListPanel",\n                    "ChroniclePanelMessage:ToggleChildPanel:TouchKillBoard"')):
        if s.count(a) != 1: raise SystemExit('touch_layout：编年史页签条 %s 不是原样（原版布局变了？）' % a)
        s = s.replace(a, b); n += 1
    end = s.rstrip().rstrip('}').rstrip(); assert end.endswith(']'); at = len(end) - 1
    return s[:at].rstrip() + ',\n        ' + json.dumps(_kb_board(), ensure_ascii=False) + '\n    ' + s[at:], n + 1


def _dropdown_rows(s):
    a = '"rect": { "width": 625, "height": 40 }'
    if s.count(a) != 2: raise SystemExit('touch_layout：下拉列表行高不是原样（原版布局变了？）')
    return s.replace(a, '"rect": { "width": 625, "height": 56 }'), 2


def _chronicle_no_search(s):
    n = 0
    for var in ('"$ChronicleSearchBox"', '"$ChronicleSearchInput"'):
        if s.count('"rect": ' + var) != 1: raise SystemExit('touch_layout：编年史搜索框的 rect 不是原样（原版布局变了？）')
        s = s.replace('"rect": ' + var, '"rect": { "x": 99999, "y": 99999, "width": 1, "height": 1 }'); n += 1


    for x in (80, 486, 1030):
        a = '"rect": { "x": %d, "y": 7, "height": 50 },' % x
        if s.count(a) != 1: raise SystemExit('touch_layout：编年史品质勾选框文字的 rect 不是原样（原版布局变了？）')
        s = s.replace(a, '"rect": { "x": %d, "y": -5, "height": 50 },' % x); n += 1
    return s, n








NPC_PAD_X, NPC_PAD_Y = 150, 62
def _npc_menu(s):
    for a, b in (('"padding": { "left": 32, "top": 24, "right": 32, "bottom": 24 },', '"padding": { "left": %d, "top": 24, "right": %d, "bottom": 24 },' % (NPC_PAD_X, NPC_PAD_X)),
                 ('"listEntryPadding": { "x": 0, "y": 10 },', '"listEntryPadding": { "x": 0, "y": %d },' % NPC_PAD_Y)):
        if s.count(a) != 1: raise SystemExit('touch_layout：NPC 对话菜单 %s 不是原样（原版布局变了？）' % a)
        s = s.replace(a, b)
    return s, 2








def _tooltip_pad(s):
    a = '"screenEdgePadding": 50,'
    if s.count(a) != 1: raise SystemExit('touch_layout：说明框 screenEdgePadding 不是原样（原版布局变了？）')
    s = s.replace(a, '"screenEdgePadding": 190,')


    b = '"showDelayMilliseconds": 400,'
    if s.count(b) != 1: raise SystemExit('touch_layout：说明框 showDelayMilliseconds 不是原样（原版布局变了？）')
    return s.replace(b, '"showDelayMilliseconds": 0,'), 2
PRESENT = {
    'controller/uiswitcherhd.json': lambda s: offscreen(s, types=(), names=('CycleLeftIndicator', 'CycleRightIndicator')),
    'controller/characterstatspanelhd.json': lambda s: (lambda r1: (lambda r2: (r2[0], r1[1] + r2[1]))(_char_title_up(r1[0])))(_char_no_stamina(s)),
    'controller/tooltipspanelhd.json': _tooltip_pad,
    'controller/dropgoldmodalhd.json': lambda s: offscreen(s),
    'controller/bankexpansionlayouthd.json': lambda s: offscreen(s, types=(), names=('gold_max',)),
    'npcdialogpanelhd.json': _npc_menu,
    'chroniclebaselistpanelhd.json': _chronicle_no_search,
    'dropdownlistcontentshd.json': _dropdown_rows,
    'chroniclepanelhd.json': lambda s: (lambda a: (lambda b: (b[0], a[1] + b[1]))(_chronicle_killboard(a[0])))(_chronicle_touch(s)),
    'controller/messagelogpanelhd.json': _rift_panel,








    'controller/pauselayoutgardenhd.json': _pause_no_help({'Settings': (428, 293), 'Exit': (728, 473), 'Chronicle': (1028, 833), 'MessageLog': (278, 1013)}, hide=('Help', 'LootFilter'), restart_y=653),
    'controller/pauselayouthd.json': _pause_no_help({'Settings': (578, 428), 'Exit': (878, 578), 'MessageLog': (428, 878)}, hide=('Help',), restart_y=728),
    'helpbuttonpanelhd.json': lambda s: offscreen(s, types=(), names=('Help',)),
    'controller/buttonbindingmodalhd.json': _bind_touch,
    'youdiedmodalhd.json': lambda s: offscreen(s, types=(), names=('SubText',)),
    'controller/controlleroverlayhd.json': _overlay_touch,
    'controller/console/switch/playerinventoryexpansionlayouthd.json': _inv_rw,
    'controller/playerinventoryexpansionlayouthd.json': _inv_touch(False),
}




BIG_FONT = (('"XMediumFontSize": 30,\n    "MediumFontSize": 28,\n    "SmallFontSize": 26,\n'),
            ('"SmallFontSize": 40,\n    "MediumFontSize": 40,\n    "MediumLargeFontSize": 40,\n    "XMediumFontSize": 40,\n    "XMediumLargeFontSize": 40,\n    "LargeFontSize": 40,\n    "SmallPanelFontSize": 32,\n    "TooltipFontSize": 42,\n    "OptionsDividerRect": { "x": 0, "y": 150 },\n'))


def build_profile(romfs, outdir):
    path, text = read_layout(romfs, 'controller/_profileconsolehd.json')
    text = text.replace('\r\n', '\n')
    if text.count(BIG_FONT[0]) != 1: raise SystemExit('touch_layout：_profileConsoleHD 的字号段不是原样（原版变了？）')
    o = os.path.join(outdir, 'profile_consolehd.json'); open(o, 'w', encoding='utf-8').write(text.replace(BIG_FONT[0], BIG_FONT[1])); repl = {path.lower(): o}




    path, text = read_layout(romfs, 'controller/_profilehandheldhd.json'); text = text.replace('\r\n', '\n')
    a = '"XMediumLargeFontSize": 36,\n    "MediumLargeFontSize": 36,\n    "XMediumFontSize": 34,\n'
    if text.count(a) != 1: raise SystemExit('touch_layout：_profileHandheldHD 的字号段不是原样（原版变了？）')
    o = os.path.join(outdir, 'profile_handheldhd.json'); open(o, 'w', encoding='utf-8').write(text.replace(a, BIG_FONT[1])); repl[path.lower()] = o
    return repl





def _npc_dialog_touch(s):
    b = '"basedOn": "NpcDialogPanelHD.json",'
    if s.count(b) != 1: raise SystemExit('touch_layout：NPC 对话菜单的 basedOn 不是原样（原版布局变了？）')
    return s.replace(b, b + '\n    "fields": { "listEntryPadding": { "x": 0, "y": 124 } },'), 1








def _boss_bar_top(text):
    j = json.loads(re.sub(r',(\s*[\]}])', r'\1', text))
    sa, ta = (next(c for c in j['children'] if c.get('name') == n) for n in ('ScreenAnchored', 'TargetAttached'))
    moved = {c['name']: c for c in ta['children']}
    if set(moved) != {'UniqueMonsterBackground', 'NormalMonsterBackground', 'Health'} or [c['name'] for c in sa['children']] != ['Name', 'Unique', 'Additional']:
        raise SystemExit('touch_layout：首领血条布局不是原样（%s / %s）' % (sorted(moved), [c['name'] for c in sa['children']]))
    dy = 86
    for c in moved.values():
        r = c.setdefault('rect', {}) if 'rect' in c else c.setdefault('fields', {}).setdefault('rect', {}); r['y'] = r.get('y', 0) + dy












    f = moved['Health']['fields']; f.pop('filename'); f['rect'].update(width=45, height=57); f['rect']['y'] += 8; f['normalColor'] = {'r': 0.18, 'g': 0, 'b': 0}; f['transparency'] = 0.75
    moved['Health'].setdefault('children', []).append({'type': 'TextBoxWidget', 'name': 'TouchBossPct', 'fields': {
        'anchor': {'x': 1, 'y': 0.5}, 'rect': {'x': 24, 'y': 0},
        'style': {'pointSize': '$LargeFontSize', 'fontColor': {'r': 212, 'g': 196, 'b': 145, 'a': 255},
                  'alignment': {'h': 'left', 'v': 'center'}}}})
    uniq, add = sa['children'][1], sa['children'][2]
    uniq['fields']['rect'] = {'y': 100}; add['fields']['rect'] = {'y': 160}
    sa['children'] = [moved['UniqueMonsterBackground'], moved['NormalMonsterBackground'], moved['Health'], uniq, add]
    ta['children'] = []
    return json.dumps(j, ensure_ascii=False, indent=1), 1
PRESENT['controller/hudmonsterhealthhd.json'] = _boss_bar_top
def pad_boss_pct(text):
    j = json.loads(re.sub(r',(\s*[\]}])', r'\1', text))
    ta = next(c for c in j['children'] if c.get('name') == 'TargetAttached'); hp = next(c for c in ta['children'] if c.get('name') == 'Health')
    hp.setdefault('children', []).append({'type': 'TextBoxWidget', 'name': 'TouchBossPct', 'fields': {
        'anchor': {'x': 1, 'y': 0.5}, 'rect': {'x': 24, 'y': 0},
        'style': {'pointSize': '$LargeFontSize', 'fontColor': {'r': 212, 'g': 196, 'b': 145, 'a': 255}, 'alignment': {'h': 'left', 'v': 'center'}}}})
    return json.dumps(j, ensure_ascii=False, indent=1)
PRESENT['controller/console/switch/npcdialogpanelhd.json'] = _npc_dialog_touch




def _bind_modal_5col(s):
    s, n = offscreen(s, types=(), names=('SkillButton6', 'AltSkillButtonRT'))
    if n != 2: raise SystemExit('touch_layout：绑定框第 6 列应为 2 格，找到 %d 格（原版布局变了？）' % n)
    return s, n
PRESENT['controller/buttonbindingmodalhd.json'] = _bind_modal_5col
def build_present(romfs, outdir):

    repl = {}; total = 0
    for rel, fn in PRESENT.items():
        path, text = read_layout(romfs, rel); new, n = fn(text)
        if n == 0: raise SystemExit('touch_layout：%s 一处也没改到（原版布局变了？）' % rel)
        o = os.path.join(outdir, 'present_' + rel.replace('/', '_')); open(o, 'w', encoding='utf-8').write(new); repl[path.lower()] = o; total += n
    return repl, total



def _settings_tabs(text):

    n = 0
    def drop_first(m):
        nonlocal n; items = [x for x in re.split(r',\s*', m.group(2).strip().rstrip(',')) if x.strip()]; n += 1
        return m.group(1) + ' ' + ', '.join(items[1:]) + ' ' + m.group(3)
    new = re.sub(r'("(?:inactiveFrames|activeFrames|textStrings)"\s*:\s*\[)([^\]]*)(\])', drop_first, text)
    new = re.sub(r'("tabMessages"\s*:\s*\[)([^\]]*)(\])', lambda m: m.group(1) + '\n' + ',\n'.join(x for x in [y.strip() for y in m.group(2).split(',')] if x and 'VideoOptionsPanel' not in x) + ',\n' + m.group(3), new)
    new, k = re.subn(r'"tabCount"\s*:\s*5', '"tabCount": 4', new)


    if new.count('"@Controls"') != 1 or new.count('SettingsPanelMessage:CheckChanges:KeyBindingMenu') != 1: raise SystemExit('touch_layout：页签条第四项不是原样「控制」，自定义页签没换')


    if new.count('"@GameSettingsTab"') != 1: raise SystemExit('touch_layout：页签条第三项不是原样「游戏设定」（原版布局变了？）')
    new = new.replace('"@GameSettingsTab"', '"@dmTabGameSettings"')


    new = new.replace('"@Controls"', '"@dmTabCustom"').replace('SettingsPanelMessage:CheckChanges:KeyBindingMenu', 'SettingsPanelMessage:CheckChanges:VideoOptionsPanel')
    new = _video_tabs(new, 3)
    return new, n + k + 1










_VIDEO_TAB_W = {3: 340, 0: 300}
def _video_tabs(s, ext):

    def arr(name, fn):
        nonlocal s
        m = re.search(r'("%s"\s*:\s*\[)([^\]]*)(\])' % name, s)
        if not m: raise SystemExit('touch_layout：页签条没有 %s（原版布局变了？）' % name)
        items = [x.strip() for x in m.group(2).split(',') if x.strip()]; items = fn(items)
        s = s[:m.start()] + m.group(1) + ' ' + ', '.join(items) + ' ' + m.group(3) + s[m.end():]
        return len(items)
    def msgs(items):
        if items[ext] != '"SettingsPanelMessage:CheckChanges:VideoOptionsPanel"': raise SystemExit('touch_layout：自定义页签的消息不是原样（%s）' % items[ext])
        items[ext] = '"PanelManager:ClosePanel:TouchTabExtras"'; return items[:ext + 1] + ['"PanelManager:ClosePanel:TouchTabVideo"'] + items[ext + 1:]
    n = arr('tabMessages', msgs)
    arr('textStrings', lambda it: it[:ext + 1] + ['"@OptionsMenuVideo"'] + it[ext + 1:])
    arr('inactiveFrames', lambda it: it[:ext + 1] + [it[ext]] + it[ext + 1:])
    arr('activeFrames', lambda it: it[:ext + 1] + [it[ext]] + it[ext + 1:])
    s, k = re.subn(r'"tabCount"\s*:\s*%d' % (n - 1), '"tabCount": %d' % n, s)
    if k != 1: raise SystemExit('touch_layout：页签条 tabCount 不是 %d' % (n - 1))
    width = _VIDEO_TAB_W[ext]
    s, k = re.subn(r'("type"\s*:\s*"TabBarWidget"\s*,\s*"name"\s*:\s*"OptionsTabs"\s*,\s*"fields"\s*:\s*\{)', r'\1\n                "filename": "ztouch_options_ui/options_tabs_%d",\n                "tabSize": { "x": %d, "y": 121 },' % (width, width), s)
    if k != 1: raise SystemExit('touch_layout：页签条 OptionsTabs 的 fields 没找到')
    return s
def _drop_rows(*rows):

    def fn(text):
        n = 0
        for r in rows:
            ms = list(re.finditer(r'\{\s*"type"\s*:\s*"TableRowWidget"\s*,\s*"name"\s*:\s*"Row %s"' % re.escape(r), text))
            if len(ms) != 1: raise SystemExit('touch_layout：选项表里「Row %s」有 %d 行（应恰好 1 行，原版布局变了？）' % (r, len(ms)))
            e = _block_end(text, ms[0].start()) + 1
            while e < len(text) and text[e] in ' \t': e += 1
            if text[e:e + 1] == ',': e += 1
            text = text[:ms[0].start()] + text[e:]; n += 1


        text, k = re.subn(r'[ \t]*"tooltipString"\s*:\s*"[^"\n]*",?[ \t]*\r?\n', '', text)
        return text, n + k
    return fn





_CUSTOM_KEYS = (('voice',) if os.environ.get('MOVIE_FFMPEG') == '1' else ()) + ('auto_mana', 'merc_pot', 'drop_light', 'map_mark', 'item_notes', 'item_cmp', 'icloud', 'pot_pick', 'drop_haptic', 'gem_pick', 'store_btn', 'eco_fps', 'q3d', 'loot_filter', 'auto_buff', 'touch_swap_above', 'cam', 'bright', 'dmg_num', 'build_hints', 'status_fx', 'combat_sim', 'fight_log', 'streak', 'melee_splash', 'shrine_stacking', 'shrine_run_speed', 'resume_pos')


_VIDEO_MOVED = ('eco_fps', 'q3d', 'bright')
_CUSTOM_KEYS = tuple(k for k in _CUSTOM_KEYS if k not in _VIDEO_MOVED)




def _custom_key(key): return 'dmCustom' + ''.join(w.capitalize() for w in key.split('_'))
def _custom_label(key):
    import dm_i18n
    t = {r[0]: r[1:] for r in dm_i18n.rows()}[_custom_key(key)]
    return t[0]
_CUSTOM_ROWS = tuple((k, '@' + _custom_key(k)) for k in _CUSTOM_KEYS)










_CUSTOM_TITLE_H, _CUSTOM_LINE_H = 180, 180
_CUSTOM_ALL = (('lang', '@dmCustomLang'),) + _CUSTOM_ROWS
_CUSTOM_TEST_ROWS = int(os.environ.get('DM_CUSTOM_TEST_ROWS', '0'))





_CUSTOM_HIDDEN = frozenset(('icloud',)) if os.environ.get('DM_OVERLAY_NO_ICLOUD') == '1' else frozenset()
_CUSTOM_ROW_H = [0 if k in _CUSTOM_HIDDEN else _CUSTOM_LINE_H for k, _ in _CUSTOM_ALL] + [_CUSTOM_LINE_H] * _CUSTOM_TEST_ROWS
_CUSTOM_Y = [_CUSTOM_TITLE_H + sum(_CUSTOM_ROW_H[:i]) for i in range(len(_CUSTOM_ROW_H))]








_EXTRA_Y = _CUSTOM_TITLE_H + sum(_CUSTOM_ROW_H)
_SECTION_H = 180
_TECH_Y, _TECH_H = _EXTRA_Y, _SECTION_H + _CUSTOM_LINE_H + 16
_SH_Y = _TECH_Y + _TECH_H
_SH_H = _SECTION_H + 2 * _CUSTOM_LINE_H + 64 + 16
_CUSTOM_H = _SH_Y + _SH_H
_DIVIDER_SCALE = 1
_NOTE = '{ "fontColor": "$PanelNoteTextColor", "pointSize": 30, "alignment": { "h": "left", "v": "center" } }'
def _section_title(name, y, key):
    return ('        { "type": "TextBoxWidget", "name": "%s_Title", "fields": { "rect": { "x": 0, "y": %d, "width": 1385, "height": %d }, "text": "@%s", "style": "$StyleSettingsTitle" } },\n'
            '        { "type": "ImageWidget", "name": "%s_Divider", "fields": { "rect": { "x": 0, "y": %d, "scale": %s }, "filename": "PauseMenu\\\\Divider" } },\n') % (name, y, _SECTION_H, key, name, y + 150, _DIVIDER_SCALE)
def _custom_extras():
    y = _TECH_Y + _SECTION_H
    out = _section_title('TouchCustomTech', _TECH_Y, 'dmCustomTechTitle')
    out += ('        { "type": "TextBoxWidget", "name": "TouchCustom_tech_Text", "fields": { "rect": { "x": 0, "y": %d, "width": 1385, "height": %d }, "text": "" } },\n' % (y, _CUSTOM_LINE_H)
            + '        { "type": "TextBoxWidget", "name": "TouchCustom_tech_Label", "fields": { "rect": { "x": 0, "y": %d, "width": 1020, "height": 100 }, "text": "@dmCustomTech", "style": "$StyleSettingsText" } },\n' % (y + 10)
            + '        { "type": "TextBoxWidget", "name": "TouchCustom_tech_State", "fields": { "rect": { "x": 0, "y": %d, "width": 1020, "height": 70 }, "text": "", "style": %s } },\n' % (y + 100, _NOTE)

            + '        { "type": "ButtonWidget", "name": "TouchCustomTechOpen", "fields": { "rect": { "x": %d, "y": %d, "scale": 0.9 }, "filename": "FrontEnd\\\\HD\\\\Final\\\\FrontEnd_ButtonSmall", '
              '"pressedFrame": 1, "disabledFrame": 2, "hoveredFrame": 3, "sound": "cursor_menu_select_hd", "textString": "@dmCustomTechOpen", "pointSize": "$MediumLargeFontSize", '
              '"textColor": "$LightButtonTextColor", "text/style": "$StyleFEButtonText", "onClickMessage": "PanelManager:ClosePanel:TouchCustomTechOpen" } },\n' % (1385 - 329, y + (_CUSTOM_LINE_H - 83) // 2))
    out += _section_title('TouchCustomShoulder', _SH_Y, 'dmCustomShoulderTitle')
    for k, side in enumerate(('l', 'r')):
        yy = _SH_Y + _SECTION_H + k * _CUSTOM_LINE_H; n = 'TouchCustom_shoulder_' + side



        out += ('        { "type": "TextBoxWidget", "name": "%s_Text", "fields": { "rect": { "x": 0, "y": %d, "width": %d, "height": %d }, "text": "" } },\n' % (n, yy, _CAROUSEL_X + _CAROUSEL_W, _CUSTOM_LINE_H)
                + '        { "type": "TextBoxWidget", "name": "%s_Label", "fields": { "rect": { "x": 0, "y": %d, "width": %d, "height": %d }, "text": "@%s", "style": "$StyleSettingsText" } },\n' % (n, yy, _CAROUSEL_X - 9, _CUSTOM_LINE_H, 'dmCustomShoulderL' if side == 'l' else 'dmCustomShoulderR')
                + _carousel(n, yy + (_CUSTOM_LINE_H - _CAROUSEL_H) // 2, ['dmShoulderAct%d' % j for j in range(_SH_N)]))
    out += '        { "type": "TextBoxWidget", "name": "TouchCustomShoulder_Hint", "fields": { "rect": { "x": 0, "y": %d, "width": 1385, "height": 64 }, "text": "@dmCustomShoulderHint", "style": %s } },\n' % (_SH_Y + _SECTION_H + 2 * _CUSTOM_LINE_H, _NOTE)
    return out
def _custom_toggle_row(i, key, label):
    y, h = _CUSTOM_Y[i], _CUSTOM_LINE_H
    return ('        { "type": "TextBoxWidget", "name": "TouchCustom_%s_Text", "fields": { "rect": { "x": 0, "y": %d, "width": 1000, "height": %d }, "text": "%s", "style": "$StyleSettingsText", "style/options/newlineHandling": "standard" } },\n'
            '        { "type": "ToggleButtonWidget", "name": "TouchCustom_%s", "fields": {\n'
            '            "rect": { "x": 1315, "y": %d, "width": 70, "height": 70 }, "sound": "cursor_menu_select_hd",\n'
            '            "focusIndicatorPadding": "$OptionsToggleFocusIndicatorPadding", "focusIndicatorFilename": "$OptionsToggleFocusIndicatorFilename", "focusIndicatorOffset": { "x": 0, "y": -4 },\n'
            '            "filename": "Lobby\\\\CreateGame\\\\CreateGame_AdvancedCheckbox",\n'
            '            "untoggledFrame": 0, "untoggledPressedFrame": 1, "untoggledHoveredFrame": 3, "untoggledDisabledFrame": 2,\n'
            '            "toggledFrame": 4, "toggledPressedFrame": 5, "toggledHoveredFrame": 6,\n'
            '            "onClickMessage": "PanelManager:ClosePanel:TouchCustom_%s" } },\n') % (key, y, h, label, key, y + (h - 70) // 2, key)



















_OPT_COL1, _OPT_COL2 = 1000, 700
_CAROUSEL_W, _CAROUSEL_H = 596, 72
_CAROUSEL_X = _OPT_COL1 + (_OPT_COL2 - _CAROUSEL_W) // 2
_ROWS_W = _OPT_COL1 + _OPT_COL2





_CAROUSEL_PT = 36
_CAROUSEL_STYLE = '{ "pointSize": %d, "spacing": "$ReducedSpacing", "alignment": { "h": "center", "v": "center" } }' % _CAROUSEL_PT
_CAR_TITLE_H = 116
_SH_N = 15
def _carousel(name, y, states, style=_CAROUSEL_STYLE, x=_CAROUSEL_X):
    return ('        { "type": "CarouselWidget", "name": "%s", "fields": { "rect": { "x": %d, "y": %d, "width": %d, "height": %d }, '
            '"focusIndicatorPadding": "$OptionsCarouselFocusIndicatorPadding", "focusIndicatorFilename": "$OptionsCarouselFocusIndicatorFilename", '
            '"states": [ %s ], "fontType": "8ptE", "style": %s, "enabledColor": "$CarouselEnabledColor", "disabledColor": "$CarouselDisabledColor" } },\n'
            % (name, x, y, _CAROUSEL_W, _CAROUSEL_H, ', '.join('"%s"' % k for k in states), style))




_CAROUSEL_ORDER = {
    'lang': ((1, 'dmCustomLangEnOpt', None), (5, 'dmCustomLangDeOpt', None), (2, 'dmCustomLangEsOpt', None), (4, 'dmCustomLangKoOpt', None), (6, 'dmCustomLangPortOpt', None), (3, 'dmCustomLangRuOpt', None), (0, 'dmCustomLangZh', None)),
    'voice': ((0, 'dmCustomVoiceZh', 'dmCustomVoiceZhHint'), (1, 'dmCustomVoiceEn', 'dmCustomVoiceEnHint')),
    'touch_swap_above': ((0, 'dmCustomTouchSwapNear', 'dmCustomTouchSwapNearHint'), (1, 'dmCustomTouchSwapFar', 'dmCustomTouchSwapFarHint')),
    'cam': ((0, 'dmCustomCamOff', 'dmCustomCamOffHint'), (1, 'dmCustomCamOts', 'dmCustomCamOtsHint'), (2, 'dmCustomCamFp', 'dmCustomCamFpHint')),
    'dmg_num': ((0, 'dmCustomDmgNumOff', 'dmCustomDmgNumOffHint'), (1, 'dmCustomDmgNumSimple', 'dmCustomDmgNumSimpleHint'), (2, 'dmCustomDmgNumColor', 'dmCustomDmgNumColorHint')),
    'streak': ((0, 'dmCustomStreakOff', 'dmCustomStreakOffHint'), (1, 'dmCustomStreakShow', 'dmCustomStreakShowHint'), (2, 'dmCustomStreakBonus', 'dmCustomStreakBonusHint')),
    'melee_splash': ((0, 'dmCustomSplashOff', 'dmCustomSplashOffHint'), (1, 'dmCustomSplashOn', 'dmCustomSplashOnHint')),
    'shrine_stacking': ((0, 'dmCustomShrineStackOff', 'dmCustomShrineStackOffHint'), (1, 'dmCustomShrineStackOn', 'dmCustomShrineStackOnHint')),
    'shrine_run_speed': ((0, 'dmCustomShrineRunOff', 'dmCustomShrineRunOffHint'), (1, 'dmCustomShrineRunOn', 'dmCustomShrineRunOnHint')),
    'resume_pos': ((0, 'dmCustomResumeOff', 'dmCustomResumeOffHint'), (1, 'dmCustomResumeOn', 'dmCustomResumeOnHint')),
}
def _custom_carousel_row(i, key, label):
    y = _CUSTOM_Y[i]; n = 'TouchCustom_' + key
    return ('        { "type": "TextBoxWidget", "name": "%s_Text", "fields": { "rect": { "x": 0, "y": %d, "width": %d, "height": %d }, "text": "" } },\n' % (n, y, _CAROUSEL_X + _CAROUSEL_W, _CUSTOM_LINE_H)
            + '        { "type": "TextBoxWidget", "name": "%s_Label", "fields": { "rect": { "x": 0, "y": %d, "width": %d, "height": %d }, "text": "%s", "style": "$StyleSettingsText" } },\n' % (n, y, _CAROUSEL_X - 9, _CAR_TITLE_H, label)
            + _carousel(n, y + (_CAR_TITLE_H - _CAROUSEL_H) // 2, [k for _, k, _ in _CAROUSEL_ORDER[key]])
            + '        { "type": "TextBoxWidget", "name": "%s_Hint", "fields": { "rect": { "x": 0, "y": %d, "width": 1385, "height": %d }, "text": "", "style": %s } },\n' % (n, y + _CAR_TITLE_H - 8, _CUSTOM_LINE_H - _CAR_TITLE_H, _NOTE))










_LANG_ROW_W = 1385
_LANG_HINTS = ('Zh', 'En', 'Es', 'Ru', 'Ko', 'De', 'Pt')
def _custom_lang_row():
    y = _CUSTOM_Y[0]
    out = _custom_carousel_row(0, 'lang', '@dmCustomLang')
    out += ''.join('        { "type": "TextBoxWidget", "name": "TouchCustomLangHint%s", "fields": { "rect": { "x": 99999, "y": %d, "width": 1385, "height": %d }, "text": "@dmCustomLangHint%s", "style": %s%s } },\n'
                   % (n, y + _CAR_TITLE_H - 8, _CUSTOM_LINE_H - _CAR_TITLE_H, n, _NOTE, ', "style/fontFace": "BlizzardGlobal"' if n in ('Zh', 'Ko') else '') for n in _LANG_HINTS)
    out += ''.join('        { "type": "TextBoxWidget", "name": "TouchCustom_lang_Font%s", "fields": { "rect": { "x": 99999, "y": %d, "width": 10, "height": 10 }, "text": "", "style": "$StyleSettingsCarousel"%s } },\n'
                   % (n, y, f) for n, f in (('Std', ''), ('Cjk', ', "style/fontFace": "BlizzardGlobal"')))
    return out

























_VIDEO_STATUS = (('clock', 'dmVideoStatusClock'), ('battery', 'dmVideoStatusBattery'), ('fps', 'dmVideoStatusFps'))
_VIDEO_TITLE_LINE, _VIDEO_OPT_LINE, _VIDEO_NOTE_LINE = 100, 112, 64
_VIDEO_NOTES = {'fps': 2, 'status': 1, 'res_pct': 2, 's3d_preset': 1, 's3d': 3, 'q3d': 1, 'bright': 1, 'driver': 1}
_VIDEO_ROW_H = {k: _VIDEO_TITLE_LINE + _VIDEO_OPT_LINE + n * _VIDEO_NOTE_LINE + (24 if k == 'status' else 16) for k, n in _VIDEO_NOTES.items()}
_VIDEO_ORDER = ('fps', 'status', 'res_pct', 's3d_preset', 's3d', 'q3d', 'bright', 'driver')




_VIDEO_PSEUDO = ('s3d_preset', 'driver')
_S3Q_STATES = ('dmVideoS3qSave', 'dmVideoS3qBal', 'dmVideoS3qSharp', 'dmVideoS3qOwn')
_VIDEO_Y = {k: _CUSTOM_TITLE_H + sum(_VIDEO_ROW_H[j] for j in _VIDEO_ORDER[:n]) for n, k in enumerate(_VIDEO_ORDER)}
_VIDEO_H = _CUSTOM_TITLE_H + sum(_VIDEO_ROW_H.values())
_SMALL_NOTE = '{ "fontColor": "$PanelNoteTextColor", "pointSize": 30, "alignment": { "h": "left", "v": "center" } }'
_SMALL_GOLD = '{ "fontColor": "$FontColorGoldYellow", "pointSize": 30, "alignment": { "h": "left", "v": "center" } }'
_VIDEO_SLIDER_X, _VIDEO_SLIDER_W, _VIDEO_SLIDER_H = 855, 480, 72
_VIDEO_VALUE_W = _VIDEO_SLIDER_X - 89
def _video_text(name, x, y, w, h, text, style='"$StyleSettingsText"'):
    return '        { "type": "TextBoxWidget", "name": "%s", "fields": { "rect": { "x": %d, "y": %d, "width": %d, "height": %d }, "text": "%s", "style": %s } },\n' % (name, x, y, w, h, text, style)
def _video_box(name, x, y):
    return ('        { "type": "ToggleButtonWidget", "name": "%s", "fields": {\n'
            '            "rect": { "x": %d, "y": %d, "width": 70, "height": 70 }, "sound": "cursor_menu_select_hd",\n'
            '            "focusIndicatorPadding": "$OptionsToggleFocusIndicatorPadding", "focusIndicatorFilename": "$OptionsToggleFocusIndicatorFilename", "focusIndicatorOffset": { "x": 0, "y": -4 },\n'
            '            "filename": "Lobby\\\\CreateGame\\\\CreateGame_AdvancedCheckbox",\n'
            '            "untoggledFrame": 0, "untoggledPressedFrame": 1, "untoggledHoveredFrame": 3, "untoggledDisabledFrame": 2,\n'
            '            "toggledFrame": 4, "toggledPressedFrame": 5, "toggledHoveredFrame": 6,\n'
            '            "onClickMessage": "PanelManager:ClosePanel:%s" } },\n') % (name, x, y, name)
def _video_cells(y, cells, pitch, tw, last_w=None):

    cy = y + (_VIDEO_OPT_LINE - 70) // 2
    return ''.join(_video_box(b, k * pitch, cy) + _video_text(b + '_Label', k * pitch + 80, y, last_w if last_w and k == len(cells) - 1 else tw, _VIDEO_OPT_LINE, t) for k, (b, t) in enumerate(cells))
def _video_slider(key, y):

    return ('        { "type": "TextBoxWidget", "name": "TouchCustom_%s_Value", "fields": { "rect": { "x": 0, "y": %d, "width": %d, "height": %d }, "text": "", '
            '"style": "$StyleSettingsNumeric", "style": { "alignment": { "h": "right", "v": "center" } } } },\n' % (key, y, _VIDEO_VALUE_W, _VIDEO_OPT_LINE)
            + '        { "type": "SettingsSliderWidget", "name": "TouchCustom_%s_Slider", "fields": {\n'
              '            "rect": { "x": %d, "y": %d, "width": %d, "height": %d },\n'
              '            "focusIndicatorPadding": "$OptionsSliderFocusIndicatorPadding", "focusIndicatorFilename": "$OptionsSliderFocusIndicatorFilename",\n'
              '            "isHorizontal": true, "indicatorFilepath": "PauseMenu\\\\HorizontalIndicator", "background": "$SettingsSliderBackground",\n'
              '            "leftButton": "$SettingsSliderLeftButtonFields", "rightButton": "$SettingsSliderRightButtonFields", "valueText": "$SettingsSliderValueFields",\n'
              '            "increment": 1 } },\n' % (key, _VIDEO_SLIDER_X, y + (_VIDEO_OPT_LINE - _VIDEO_SLIDER_H) // 2, _VIDEO_SLIDER_W, _VIDEO_SLIDER_H))
_VIDEO_DRIVER_X = _LANG_ROW_W - 329
def _video_driver(y):

    return ('        { "type": "TextBoxWidget", "name": "TouchCustom_driver_Value", "fields": { "rect": { "x": 0, "y": %d, "width": %d, "height": %d }, "text": "", '
            '"style": "$StyleSettingsNumeric", "style": { "alignment": { "h": "right", "v": "center" } } } },\n' % (y, _VIDEO_DRIVER_X - 40, _VIDEO_OPT_LINE)
            + '        { "type": "ButtonWidget", "name": "TouchCustomDriverOpen", "fields": { "rect": { "x": %d, "y": %d, "scale": 0.9 }, "filename": "FrontEnd\\\\HD\\\\Final\\\\FrontEnd_ButtonSmall", '
              '"pressedFrame": 1, "disabledFrame": 2, "hoveredFrame": 3, "sound": "cursor_menu_select_hd", "textString": "@dmCustomTechOpen", "pointSize": "$MediumLargeFontSize", '
              '"textColor": "$LightButtonTextColor", "text/style": "$StyleFEButtonText", "onClickMessage": "PanelManager:ClosePanel:TouchCustomDriverOpen" } },\n'
            % (_VIDEO_DRIVER_X, y + (_VIDEO_OPT_LINE - 83) // 2))
_VIDEO_TITLES = {'fps': '@FramerateCap', 'status': '@dmVideoStatus', 'res_pct': '@dmVideoRes', 's3d_preset': '@dmVideoS3q', 's3d': '@dmVideoS3d', 'q3d': '@dmVideoQ3d', 'bright': '@dmCustomBright', 'driver': '@dmVideoDriver'}
_VIDEO_HINTS = {'fps': '@dmVideoFpsHint', 'status': '@dmVideoStatusHint', 'res_pct': '@dmVideoResHint', 'driver': '@dmVideoDriverHint'}
def _video_rows():
    out = _video_text('TouchVideoTitle', 0, 0, 1000, _CUSTOM_TITLE_H, '@OptionsMenuVideo', '"$StyleSettingsTitle"')
    for key in _VIDEO_ORDER:
        y = _VIDEO_Y[key]; h = _VIDEO_ROW_H[key]; oy = y + _VIDEO_TITLE_LINE; ny = oy + _VIDEO_OPT_LINE
        out += _video_text('TouchCustom_%s_Text' % key, 0, y, _LANG_ROW_W, h - 16, '')
        out += _video_text('TouchVideo_%s_Label' % key, 0, y, _LANG_ROW_W, _VIDEO_TITLE_LINE, _VIDEO_TITLES[key])
        if key == 'status': out += _video_cells(oy, [('TouchCustom_status_' + b, '@' + t) for b, t in _VIDEO_STATUS], 400, 300)
        elif key == 's3d_preset': out += _carousel('TouchCustom_s3d_preset', oy + (_VIDEO_OPT_LINE - _CAROUSEL_H) // 2, _S3Q_STATES, x=_LANG_ROW_W - _CAROUSEL_W)
        elif key == 'driver': out += _video_driver(oy)
        else: out += _video_slider(key, oy)
        out += _video_text('TouchVideo_%s_Hint' % key, 0, ny, _LANG_ROW_W, _VIDEO_NOTE_LINE, _VIDEO_HINTS.get(key, ''), _SMALL_NOTE)
        if key in ('fps', 'res_pct', 's3d'):
            out += _video_text('TouchVideo_%s_Note' % key, 0, ny + _VIDEO_NOTE_LINE, _LANG_ROW_W, _VIDEO_NOTE_LINE, '', _SMALL_GOLD if key == 'res_pct' else _SMALL_NOTE)
        if key in ('fps', 's3d'):
            out += _video_text('TouchVideo_%s_Warn' % key, 0, ny + (1 if key == 'fps' else 2) * _VIDEO_NOTE_LINE, _LANG_ROW_W, _VIDEO_NOTE_LINE, '', _SMALL_GOLD)
    return out
def _custom_page(text):
    if '"name": "VideoOptionsPanel"' not in text or '"name": "OptionsTable"' not in text: raise SystemExit('touch_layout：主机版视频页不是原样（原版布局变了？），自定义页没换')
    title = ('{ "type": "TableRowWidget", "name": "Row TouchCustomTitle", "children": [\n'
             '    { "type": "TextBoxWidget", "name": "TouchCustomTitle", "fields": { "text": "", "style": "$StyleSettingsTitle" }, "children": [\n'
             '        { "type": "ImageWidget", "name": "Divider", "fields": { "rect": "$OptionsDividerRect", "filename": "PauseMenu\\\\Divider" } },\n'
             '    ] },\n'
             '] },\n')
    out = ('{\n    "basedOn": "VideoOptionsPanelHD.json",\n    "type": "VideoOptionsPanel", "name": "VideoOptionsPanel",\n    "children": [\n'
           '        { "type": "ImageWidget", "name": "ScrollBarBackground", "children": [ { "type": "ScrollControllerWidget", "name": "ScrollController" }, ] },\n'
           '        { "type": "ScrollViewWidget", "name": "ScrollView", "children": [\n'
           '            { "type": "TableWidget", "name": "OptionsTable", "fields": { "columns": [ "$OptionsTableColumn1", "$OptionsTableColumn2" ], "rowHeight": %d }, "children": [\n' % _CUSTOM_TITLE_H
           + title + '            ] },\n'
           + '            { "type": "Widget", "name": "TouchCustomRows", "fields": { "rect": { "x": 0, "y": 0, "width": %d, "height": %d } }, "children": [\n' % (_ROWS_W, _CUSTOM_H)
           + '        { "type": "TextBoxWidget", "name": "TouchCustomTitleText", "fields": { "rect": { "x": 0, "y": 0, "width": 1000, "height": %d }, "text": "@dmTabCustom", "style": "$StyleSettingsTitle" } },\n' % _CUSTOM_TITLE_H
           + _custom_lang_row() + ''.join((_custom_carousel_row(i, k, l) if k in _CAROUSEL_ORDER else _custom_toggle_row(i, k, l)) for i, (k, l) in enumerate(_CUSTOM_ALL) if i and k not in _CUSTOM_HIDDEN)
           + ''.join('        { "type": "TextBoxWidget", "name": "TouchCustomTest%d_Text", "fields": { "rect": { "x": 0, "y": %d, "width": 1000, "height": %d }, "text": "@KeyBindingTypeCustom", "style": "$StyleSettingsText" } },\n'
                     % (n, _CUSTOM_Y[len(_CUSTOM_ALL) + n], _CUSTOM_LINE_H) for n in range(_CUSTOM_TEST_ROWS)) + _custom_extras() +




           '            ] },\n'
           + '            { "type": "Widget", "name": "TouchVideoRows", "fields": { "rect": { "x": 99999, "y": 0, "width": 1500, "height": %d } }, "children": [\n' % _VIDEO_H + _video_rows() + '            ] },\n' + _CUSTOM_SEL
           + '        ] },\n'
           '    ]\n}\n')
    return out, len(_CUSTOM_ALL) + 2
_CUSTOM_SEL = ('        { "type": "NineTileImageWidget", "name": "TouchCustomSel", "fields": { "rect": { "x": 99999, "y": 0, "width": 1000, "height": 100 }, "backgroundColor": [ 0, 0, 0, 0 ], '
               '"filename": "Controller/PANEL/ControllerSelector/ControllerSelectorBorder", "tileScale": 0.5 } },\n')
PRESENT['controller/console/videooptionspanelhd.json'] = _custom_page



def pad_settings(s):



    return _video_tabs(_sub1(_sub1(s, '"@OptionsMenuVideo"', '"@dmTabCustom"'), '"@GameSettingsTab"', '"@dmTabGameSettings"'), 0)
def pad_custom_page(s): return _custom_page(s)[0]


PRESENT['controller/console/settingspanelhd.json'] = _settings_tabs


def _settings_no_legend(s):
    s, n = offscreen(s, types=('ButtonLegendWidget',))
    if n != 1: raise SystemExit('touch_layout：选项页提示条应为 1 个，找到 %d 个' % n)
    return s, n








def _modal_prompt(old, new):
    def fn(s):
        a = '"style": "$StyleModalDialogDescription"'
        if s.count(old) != 1 or s.count(a) != 1: raise SystemExit('touch_layout：确认框正文区 %s 不是原样（原版布局变了？）' % old)
        s = s.replace(old, new)
        return s.replace(a, '"style": { "fontColor": "$ModalDescriptionTextColor", "alignment": { "h": "center", "v": "center" }, "options": { "lineWrap": true, "newlineHandling": "standard" }, "pointSize": 30, "spacing": "$ReducedSpacing" }'), 2
    return fn
PRESENT['confirmationmodalhd.json'] = _modal_prompt('"rect": { "x": 78, "y": 85, "width": 1076, "height": 170 },', '"rect": { "x": 78, "y": 28, "width": 1076, "height": 264 },')
PRESENT['dismissablemodalhd.json'] = _modal_prompt('"rect": { "x": 140, "y": 70, "width": 966, "height": 212 },', '"rect": { "x": 140, "y": 28, "width": 966, "height": 264 },')
PRESENT['controller/settingspanelhd.json'] = _settings_no_legend




PRESENT['soundoptionspanelhd.json'] = _drop_rows('LanguageOptions', 'VoiceLanguage', 'TextLanguage', 'Controller Feedback')
PRESENT['gameplayoptionspanelhd.json'] = _drop_rows('AutoPartyInvite', 'ItemHotkeyTooltipAppender', 'ChatFontSize', 'ChatBackground', 'Controller Options', 'ControllerCursorSpeed',
                                                    'ControllerWeaponSwapSeparateBindings', 'ControllerRumbleEnabled', 'ControllerSwapThumbsticks', 'ShowParty', 'ShowNames', 'ShowRoomName',




                                                    'LowVisionMode',




                                                    'ColorBlindCalibration')
PRESENT['onlinegameoptionspanelhd.json'] = _drop_rows('OfflineDifficultyScaling', 'OfflineSettingsEffectNote')




PAD_ROOT = {
    'soundoptionspanelhd.json': _drop_rows('LanguageOptions', 'VoiceLanguage', 'TextLanguage', 'Controller Feedback'),




    'gameplayoptionspanelhd.json': _drop_rows('AutoPartyInvite', 'ItemHotkeyTooltipAppender', 'ChatFontSize', 'ChatBackground', 'ShowParty', 'ShowNames', 'ShowRoomName', 'LowVisionMode',
                                              'ControllerCursorSpeed', 'ControllerWeaponSwapSeparateBindings', 'ControllerRumbleEnabled'),
    'onlinegameoptionspanelhd.json': _drop_rows('OfflineDifficultyScaling', 'OfflineSettingsEffectNote'),
}
def build_pad_root(romfs, outdir):
    repl = {}
    for rel, fn in PAD_ROOT.items():
        path, text = read_layout(romfs, rel); new, n = fn(text)
        if n == 0: raise SystemExit('touch_layout：手柄版 %s 一处也没改到（原版布局变了？）' % rel)
        o = os.path.join(outdir, 'padroot_' + rel.replace('/', '_')); open(o, 'w', encoding='utf-8').write(new); repl[path.lower()] = o
    return repl


def _row_height(fn):
    def g(text):
        text, n = fn(text); a = '"rowHeight": "$OptionsTableRowHeight"'
        if a not in text: raise SystemExit('touch_layout：选项表的 rowHeight 不是原样（原版布局变了？）')
        return text.replace(a, '"rowHeight": 180'), n + text.count(a)
    return g
for _k in ('soundoptionspanelhd.json', 'gameplayoptionspanelhd.json', 'onlinegameoptionspanelhd.json'): PRESENT[_k] = _row_height(PRESENT[_k])

def _widget_block(s, name):
    m = re.search(r'\{\s*"type"\s*:\s*"[A-Za-z]+"\s*,\s*"name"\s*:\s*"%s"' % re.escape(name), s)
    if not m: raise SystemExit('touch_layout：键鼠版布局里没有控件 ' + name)
    return s[m.start():_block_end(s, m.start()) + 1]








PAD_VENDOR = {'controller/vendorpanellayouthd.json': ('vendorpanellayouthd.json', [INGAME['controller/vendorpanellayouthd.json'][1][0], INGAME['controller/vendorpanellayouthd.json'][1][1]])}














BUILD_ROWS = (179, 368, 560, 750, 939, 1129)
BUILD_STAT_ROWS = (386, 628, 971, 1210)
def _build_frame(name, size):
    return {'type': 'NineTileImageWidget', 'name': name, 'fields': {'rect': {'x': 0, 'y': 0, 'width': size, 'height': size}, 'filename': 'PANEL/TooltipBorder/TooltipBorder',
            'backgroundColor': [0.0, 0.0, 0.0, 0.0], 'tileScale': 1.5}}
def _build_card_text(name, y, h, size, color, formal, text='', title=False, x=28, width=353):
    st = {'fontColor': color, 'pointSize': size, 'alignment': {'h': 'center', 'v': 'center'}, 'options': {'lineWrap': True}, 'spacing': '$ReducedSpacing'}
    if formal: st['fontFace'] = 'Formal'
    if title: st['dropShadow'] = '$DefaultDropShadow'
    return {'type': 'TextBoxWidget', 'name': name, 'fields': {'rect': {'x': x, 'y': y, 'width': width, 'height': h}, 'text': text, 'style': st}}
def _build_widgets(pad=False):
    out = []
    for k, y in enumerate(BUILD_ROWS):
        sign = _kb_text('TouchBuildSign', 6, -40, 160, 40, '$FontColorGoldYellow', 26, 'center', '@dmBuildNext')
        out.append({'type': 'Widget', 'name': 'TouchBuildRow%d' % k, 'fields': {'rect': {'x': 99999, 'y': y - 18, 'width': 172, 'height': 172}}, 'children': [_build_frame('TouchBuildFrame', 172), sign]})
    out.append({'type': 'ImageWidget', 'name': 'TouchBuildTab', 'fields': {'rect': {'x': 99999, 'y': 4, 'scale': 0.6}, 'filename': 'Controller\\Panel\\TabNotificationIcon'}})
    return [json.dumps(w, ensure_ascii=False) for w in out] + [lambda romfs: json.dumps(_build_card(pad, build_card_place(romfs)), ensure_ascii=False)]














BUILD_CARD_H = 752
BUILD_CARD_ART = 'PANEL\\Beta_Exit\\BetaSurveyQR'
BUILD_FRAME = 'hd/global/ui/Controller/Panel/MainPanelBG.sprite'


FIELD_DARK, FIELD_MIN = 10, 100


@functools.lru_cache(maxsize=None)
def build_card_place(romfs):



    import sys
    import romfs_art
    sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'android', 'tools'))
    import profile_hd
    named = profile_hd.chain(lambda path: romfs_art.read(romfs, path))
    over, panel = (profile_hd.resolve(named.get(k), named) for k in ('ControllerOverlayRect', 'ConsoleCenterPanelRect'))
    if not all(isinstance(r, dict) and isinstance(r.get('x'), (int, float)) for r in (over, panel)):
        raise SystemExit('touch_layout: unexpected $ControllerOverlayRect / $ConsoleCenterPanelRect: %r / %r' % (over, panel))
    scale = {profile_hd.number(r.get('scale'), named) for r in (over, panel)}
    if len(scale) != 1 or not next(iter(scale)):
        raise SystemExit('touch_layout: frame and skill tree panel are not drawn at one scale: %r' % scale)
    import numpy as np
    frame = np.asarray(romfs_art.frame(romfs, BUILD_FRAME, 0).convert('L'), dtype=np.float64)
    h, w = frame.shape
    dark = list(frame[h // 5:h * 4 // 5].mean(axis=0) < FIELD_DARK)
    runs, x = [], 0
    while x < w:
        if dark[x]:
            end = x
            while end < w and dark[end]: end += 1
            if end - x >= FIELD_MIN: runs.append((x, end))
            x = end
        x += 1
    if len(runs) != 2:
        raise SystemExit('touch_layout: expected two dark side fields in %s, found %r' % (BUILD_FRAME, runs))
    left, right = runs[0]
    _, tree = read_layout(romfs, 'controller/skillstreepanelhd.json')
    tab, background = (int(re.search(r'"y"\s*:\s*(-?\d+)', r).group(1)) for r in re.findall(r'"rect"\s*:\s*\{([^{}]*)\}', _widget_block(tree, 'Tab0'))[:2])
    arts = re.search(r'"skillBackgroundFile"\s*:\s*\[([^\]]*)\]', tree)
    if not arts: raise SystemExit('touch_layout: the controller skill tree layout names no skillBackgroundFile')
    first = set()
    for art in re.findall(r'"([^"]+)"', arts.group(1)):
        for i in range(3):
            alpha = romfs_art.frame(romfs, 'hd/global/ui/%s.sprite' % art, i).split()[3].point(lambda v: 255 if v >= 128 else 0).getbbox()
            if not alpha: raise SystemExit('touch_layout: %s frame %d is empty' % (art, i))
            first.add(alpha[1])
    return round((over['x'] - panel['x']) / next(iter(scale)) + left * 2), tab + background + 2 * min(first), (right - left) * 2


def _build_card(pad, place):
    x, y, width = place
    centre = lambda w: round((width - w) / 2)
    button = 470 * (0.6 if pad else 0.65)
    cap = 104
    kids = [{'type': 'ImageWidget', 'name': 'TouchBuildCardBg', 'fields': {'rect': {'x': 0, 'y': 0, 'scale': 1.0}, 'filename': BUILD_CARD_ART}},
            _build_card_text('TouchBuildCardTitle', 44, 128, 26, '$FontColorGoldYellow', False, title=True, x=centre(381), width=381),
            _build_card_text('TouchBuildCardPhase', 174, 80, 26, '$FontColorBeige', True, x=centre(353)),
            _build_card_text('TouchBuildCardLabel', 256, 42, 26, '$FontColorGoldYellow', False, '@dmBuildCardNext', x=centre(381), width=381),
            _build_card_text('TouchBuildCardSkill', 300, 164, 26, '$FontColorWhite', True, x=centre(381), width=381),
            _build_card_text('TouchBuildCardTo', 466, 44, 26, '$FontColorBeige', True, x=centre(381), width=381),
            _build_card_text('TouchBuildCardNote', 512, 96, 26, '$FontColorBeige', True, x=centre(353)),
            _rf_btn('TouchBuildOpen', centre(button + cap) + cap if pad else centre(button), 620, 'FrontEnd/HD/Final/FrontEnd_ButtonMed', _RF_BTN, 0.6 if pad else 0.65, '@dmBuildGuide')]
    if pad: kids.append({'type': 'ImageWidget', 'name': 'TouchBuildPadRS', 'fields': {'rect': {'x': centre(button + cap), 'y': 614, 'scale': 1.0}, 'filename': 'Controller\\ControllerButtons\\switch\\StickRightButton'}})
    box = {'type': 'Widget', 'name': 'TouchBuildCardBox', 'fields': {'rect': {'x': x, 'y': y, 'width': width, 'height': BUILD_CARD_H}}, 'children': kids}
    return {'type': 'Widget', 'name': 'TouchBuildCard', 'fields': {'rect': {'x': 99999, 'y': 0}}, 'children': [box]}
def _build_stat_widgets():
    return [json.dumps({'type': 'Widget', 'name': 'TouchBuildStat%d' % k, 'fields': {'rect': {'x': 99999, 'y': y - 18, 'width': 142, 'height': 142}}, 'children': [_build_frame('TouchBuildStatFrame', 142)]},
                       ensure_ascii=False) for k, y in enumerate(BUILD_STAT_ROWS)]


EXTRA = {'controller/skillstreepanelhd.json': _build_widgets(), 'controller/characterstatspanelhd.json': _build_stat_widgets(),
         'controller/vendorpanellayouthd.json': (lambda romfs: __import__('looks_tab').widget(romfs),),
         'controller/console/switch/playerinventoryexpansionlayouthd.json': (__import__('looks_tab').slot_frame(),)}
EXTRA_PAD = {'controller/skillstreepanelhd.json': _build_widgets(pad=True), 'controller/characterstatspanelhd.json': _build_stat_widgets()}

PAD_TREE = {'controller/skillstreepanelhd.json': ('skillstreepanelhd.json', []), 'controller/characterstatspanelhd.json': ('characterstatspanelhd.json', [])}




def _skill_tab_labels(text):
    import tab_art
    inset = (300 - tab_art.SKILL_TAB_WIDTH) // 2

    for index, left in enumerate((276, 587, 904, 1218)):
        block = _widget_block(text, "TextTab%d" % index)
        if block.count('"style": "$StyleSkillTreeTabs"') != 1 or block.count('"truncateWithEllipsis": true') != 1:
            raise ValueError("Unexpected skill tab text style")
        updated, count = re.subn(r'"rect"\s*:\s*\{[^{}]*\}', '"rect": {"x": %d, "y": 9, "width": %d, "height": %d}' % (
            left + inset, tab_art.SKILL_TAB_WIDTH, tab_art.SKILL_TAB_HEIGHT), block, count=1)
        if count != 1:
            raise ValueError("Missing skill tab text rectangle")
        updated = updated.replace('"truncateWithEllipsis": true', '"truncateWithEllipsis": false')
        updated = updated.replace('"style": "$StyleSkillTreeTabs"',
                                  '"style": {"pointSize": %d, "options": {"lineWrap": true}, '
                                  '"alignment": {"h": "center", "v": "center"}, "fontColor": "$FontColorWhite", '
                                  '"spacing": {"leading": %s, "kerning": %s}}' % (
                                      tab_art.SKILL_TAB_SIZE, tab_art.SKILL_TAB_LEADING, tab_art.SKILL_TAB_KERNING))
        text = text.replace(block, updated)
    return text


def build_ingame(romfs, outdir, spec=None, present=True):

    repl = {}; total = 0
    for rel, (src, items) in (INGAME if spec is None else spec).items():
        path, text = read_layout(romfs, rel); _, base = read_layout(romfs, src); add = ''
        if rel == 'controller/skillstreepanelhd.json': text = _skill_tab_labels(text)
        if rel == 'controller/vendorpanellayouthd.json': import looks_tab; text = looks_tab.tab(text)
        if present and rel in PRESENT: text = PRESENT[rel](text)[0]
        for name, new_name, override, *alt in items:
            b = _widget_block(read_layout(romfs, alt[0])[1] if alt else base, name)
            if re.search(r'"name"\s*:\s*"%s"' % re.escape(new_name or name), text): raise SystemExit('touch_layout：%s 里已有同名控件 %s' % (rel, new_name or name))
            if new_name: b = b.replace('"name": "%s"' % name, '"name": "%s"' % new_name, 1)
            if override:
                for k in re.findall(r'"([A-Za-z/]+)"\s*:', override): b = re.sub(r'\n\s*"%s"\s*:\s*(\{[^{}]*\}|"[^"]*"|[^,\n]+),?' % re.escape(k), '', b, count=1)
                f = re.search(r'"fields"\s*:\s*\{', b); b = b[:f.end()] + '\n                ' + override.rstrip(',') + ',' + b[f.end():]
            add += '        ' + b + ',\n'; total += 1
        for b in (EXTRA_PAD if spec is PAD_TREE else EXTRA).get(rel, ()):
            if re.search(r'"name"\s*:\s*"TouchBuild', text): raise SystemExit('touch_layout：%s 里已有 TouchBuild 控件' % rel)
            add += '        ' + (b(romfs) if callable(b) else b) + ',\n'; total += 1
        end = text.rstrip().rstrip('}').rstrip(); assert end.endswith(']'), rel
        at = len(end) - 1; head = text[:at].rstrip(); new = head + ('' if head.endswith((',', '[')) else ',') + '\n' + add + '    ' + text[at:]
        import json as _json
        try: _json.loads(re.sub(r',(\s*[}\]])', r'\1', re.sub(r'(?m)^\s*//[^\n]*|(?<=[,{\[\s])//[^\n]*', '', re.sub(r'/\*[\s\S]*?\*/', '', new))))
        except ValueError as e: raise SystemExit('touch_layout：%s 插回后不是合法布局（%s）——游戏会整个面板不显示且不报错' % (rel, e))
        o = os.path.join(outdir, 'ingame_' + rel.replace('/', '_')); open(o, 'w', encoding='utf-8').write(new); repl[path.lower()] = o
    return repl, total

if __name__ == '__main__':
    import sys, tempfile
    r, n = build(sys.argv[1], sys.argv[2] if len(sys.argv) > 2 else tempfile.mkdtemp()); print('前端布局 %d 个，挪走键位提示 %d 处' % (len(r), n))
    for k, v in r.items(): print(k, '←', v)
