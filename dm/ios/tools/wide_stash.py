#!/usr/bin/env python3














import json, os, re, shutil, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import touch_layout

HERE = os.path.dirname(os.path.abspath(__file__)); ART = __import__('gen_paths').gen_dir('NativeHUD', 'widestash')
COLS = 14; EX = 392
BANK = 'controller/bankexpansionlayouthd.json'; OVERLAY = 'controller/controlleroverlayhd.json'
SPRITES = {n: 'controller/panel/stash/v2/' + n.lower() for n in ('StashPanelBG', 'StashPanelBGPersonal', 'StashPanelBGShared')}
SPRITES.update({n: 'controller/panel/stash/v2/additionalstash/' + n.lower() for n in ('AdvancedStash_Gems_Console', 'AdvancedStash_Consumables_Console', 'AdvancedStash_Runes_Console')})
WIDE_FRAME = 'controller/panel/zswide/mainpanelbg_wide'

def _entry(romfs, suffix):
    for line in open(os.path.join(romfs, 'data', 'build_manifest.psv'), encoding='utf-8'):
        f = line.rstrip('\n').split('|')
        if len(f) >= 11 and not line.startswith('#') and f[9].lower().endswith(suffix): return f[9].split(':', 1)[1].lower(), touch_layout._read_entry(romfs, f)
    raise SystemExit('wide_stash：包里没有 ' + suffix)

def _shift(s, name, dx, start=0):

    m = re.search(r'"name"\s*:\s*"%s"' % re.escape(name), s[start:])
    if not m: raise SystemExit('wide_stash：储藏箱布局里没有控件 ' + name)
    i = start + m.end(); r = re.compile(r'"rect"\s*:\s*\{\s*"x"\s*:\s*(-?\d+)').search(s, i)
    if not r or s.find('"name"', i, r.start()) != -1: raise SystemExit('wide_stash：控件 %s 的 rect 不在它自己的字段里' % name)
    return s[:r.start(1)] + str(int(r.group(1)) + dx) + s[r.end(1):]

def _container_x(s, name, dx):

    m = re.search(r'"name"\s*:\s*"%s"' % re.escape(name), s); i = m.end()
    nxt = s.find('"children"', i); f = s.find('"fields"', i)
    if f != -1 and f < nxt: return _shift(s, name, dx)
    return s[:i] + ',\n            "fields": { "rect": { "x": %d, "y": 0 } }' % dx + s[i:]





def trove_slots(s):
    import gen_trove_art as T
    pat = re.compile(r'\{\s*"type"\s*:\s*"AdvancedStashSlotWidget"\s*,\s*"name"\s*:\s*"(\w+)"\s*,\s*"fields"\s*:\s*\{\s*"rect"\s*:\s*\{[^}]*\}\s*,\s*"itemCode"\s*:\s*"(\w+)"\s*,?\s*\}\s*,?\s*\}\s*,?')
    codes = [m.group(2) for m in pat.finditer(s)]; want = T.slots()
    if sorted(codes) != sorted(want) or any(m.group(1) != m.group(2) for m in pat.finditer(s)): raise SystemExit('wide_stash：高级仓库槽控件不是原版那 91 个（找到 %d 个；原版布局变了？）' % len(codes))
    s = pat.sub('', s); i = s.index('"children"', s.index('"name": "advancedstash_gems"')); i = s.index('[', i) + 1
    new = ',\n'.join('                { "type": "AdvancedStashSlotWidget", "name": "%s", "fields": { "rect": { "width": %d, "height": %d, "x": %d, "y": %d }, "itemCode": "%s" } }' % (c, w, h, x, y, c) for c, (x, y, w, h) in sorted(want.items()))
    return s[:i] + '\n' + new + '\n            ' + s[i:]

TOUCH_ONLY = {'TouchStashLeft', 'TouchStashRight', 'close'}
def _has(s, nm): return re.search(r'"name"\s*:\s*"%s"' % re.escape(nm), s) is not None
def bank_layout(text):
    s = text; n0 = s.count('"cellCount": { "x": 10, "y": 10 }')
    if n0 != 1: raise SystemExit('wide_stash：储藏箱格子 cellCount 10×10 应为 1 处，找到 %d 处（原版布局变了？）' % n0)
    s = s.replace('"cellCount": { "x": 10, "y": 10 }', '"cellCount": { "x": %d, "y": 10 }' % COLS)
    half = EX // 2
    for nm in ('gold_amount', 'gold_withdraw', 'StashNavigation', 'TouchStashLeft', 'TouchStashRight'):
        if nm in TOUCH_ONLY and not _has(s, nm): continue
        s = _shift(s, nm, half)
    for nm in ('ShowPreviousLadderSeasonToggle', 'close'):
        if nm in TOUCH_ONLY and not _has(s, nm): continue
        s = _shift(s, nm, EX)
    s = trove_slots(s)
    for nm in ('advancedstash_gems', 'advancedstash_materials', 'advancedstash_runes', 'advancedstash_horadriccube'): s = _container_x(s, nm, half)
    s = _shift(s, 'advancedstash_horadriccube', -2 * __import__('gen_trove_art').SHIFT)
    s = vault_tab(s)
    return s









VAULT_COLS, VAULT_ROWS, VAULT_SET_ROWS, VAULT_SET_N = 3, 4, 7, 6
VX, VW = 156, 1372
PARTS = ['@dmVaultPart%d' % i for i in range(11)]
PT_FONT = 23
PT_Y, PT_PITCH = 228, 108
UNIQ_Y = PT_Y + 2 * PT_PITCH + 72
def _vt_text(name, x, y, w, h, color, size, align='left', text='', trunc=True):
    return {'type': 'TextBoxWidget', 'name': name, 'fields': {'rect': {'x': x, 'y': y, 'width': w, 'height': h}, 'text': text,
            'style': {'fontColor': color, 'pointSize': size, 'alignment': {'h': align, 'v': 'center'}}, 'truncateWithEllipsis': trunc}}
def _vt_nl(w):
    w['fields']['style']['options'] = {'newlineHandling': 'standard'}; return w
def _vt_box(name, x, y, w, h, rgba, border=None):
    f = {'rect': {'x': x, 'y': y, 'width': w, 'height': h}, 'backgroundColor': rgba}
    if border: f.update(filename=border, tileScale=0.5)
    return {'type': 'NineTileImageWidget', 'name': name, 'fields': f}
def _vt_tile(name, x, y, w, h):
    return [_vt_box(name, x, y, w, h, [0.03, 0.028, 0.026, 1.0]), dict(_vt_box(name + 'Gold', x - 18, y - 18, w + 36, h + 36, [0, 0, 0, 0], 'Controller/PANEL/ControllerSelector/ControllerSelectorBorder')),
            {'type': 'Widget', 'name': name + 'Box', 'fields': {'rect': {'x': x + 8, 'y': y + 8, 'width': w - 16, 'height': h - 16}},
             'children': [{'type': 'ImageWidget', 'name': name + 'Img', 'fields': {'rect': {'x': 0, 'y': 0}}}]}]
def _vt_check(name, x, y, label):
    return [{'type': 'ToggleButtonWidget', 'name': name, 'fields': {'rect': {'x': x, 'y': y, 'width': 65, 'height': 65}, 'sound': 'cursor_menu_select_hd',
             'filename': 'Lobby\\CreateGame\\CreateGame_AdvancedCheckbox', 'untoggledFrame': 0, 'untoggledPressedFrame': 1, 'untoggledHoveredFrame': 3, 'untoggledDisabledFrame': 2,
             'toggledFrame': 4, 'toggledPressedFrame': 5, 'toggledHoveredFrame': 6, 'onClickMessage': 'PanelManager:ClosePanel:' + name}},
            {'type': 'TextBoxWidget', 'name': name + '_Text', 'fields': {'text': label, 'rect': {'x': x + 75, 'y': y - 5, 'height': 50}, 'style': '$StyleSettingsTitle', 'fitToText': True}}]
def _vt_sub(name, x, label):
    return {'type': 'Widget', 'name': name, 'fields': {'rect': {'x': x, 'y': 0, 'width': 358, 'height': 94, 'scale': 0.7}},
            'children': [
                {'type': 'ImageWidget', 'name': 'Img', 'fields': {'rect': {'x': 0, 'y': 0}, 'filename': 'PANEL\\Chronicle\\Slot_list_small_golddeco', 'frame': 0}},
                _vt_box('Hit', 0, 0, 358, 94, [0, 0, 0, 0]),
                _vt_text('Text', 25, 0, 330, 94, '$FontColorWhite', '$LargeFontSize', 'center', label)]}
def vault_widgets():
    CW, CH, GAP = (VW - 2 * 8) // VAULT_COLS, 130, 8
    cells = []
    for k in range(VAULT_COLS * VAULT_ROWS):
        r, c = divmod(k, VAULT_COLS); x, y = c * (CW + GAP), r * (CH + GAP)
        cells.append({'type': 'Widget', 'name': 'TouchV_U%d' % k, 'fields': {'rect': {'x': x, 'y': y, 'width': CW, 'height': CH}}, 'children':






            [_vt_box('Bg', 0, 0, CW, CH, [0.065, 0.062, 0.058, 1.0])] + _vt_tile('Tile', 10, 10, 90, CH - 20) + [_vt_text('Name', 110, 10, CW - 160, 56, '$FontColorGoldYellow', '$MediumFontSize'),
             _vt_text('Sub', 110, 66, CW - 124, 54, '$FontColorMoreLightGray', '$SmallFontSize'),
             _vt_text('Dot', CW - 54, 8, 44, 44, '$FontColorGoldYellow', '$SmallFontSize', 'center', trunc=False),
             _vt_box('CellGold', -18, -18, CW + 36, CH + 36, [0, 0, 0, 0], 'Controller/PANEL/ControllerSelector/ControllerSelectorBorder')]})
    RH = 116; sets = []
    for r in range(VAULT_SET_ROWS):


        row = [_vt_box('Bg', 0, 0, VW, RH, [0.08, 0.075, 0.07, 1.0]), _vt_box('BgGold', 0, 0, VW, RH, [0.19, 0.15, 0.07, 1.0]),
               _vt_text('Name', 20, -2, 520, 60, '$FontColorGreen', '$MediumLargeFontSize'), _vt_text('Count', 20, 66, 520, 46, '$FontColorMoreLightGray', '$SmallFontSize')]
        for j in range(VAULT_SET_N): row += _vt_tile('Tile%d' % j, 560 + j * 132, 6, 124, RH - 12) + [_vt_text('Tile%dDot' % j, 560 + j * 132 + 80, 0, 52, 46, '$FontColorGoldYellow', '$SmallFontSize', 'center', trunc=False)]
        sets.append({'type': 'Widget', 'name': 'TouchV_S%d' % r, 'fields': {'rect': {'x': 0, 'y': r * (RH + 8), 'width': VW, 'height': RH}}, 'children': row})
    return {'type': 'Widget', 'name': 'TouchVault', 'fields': {'rect': {'x': 99999, 'y': 0, 'width': 1730, 'height': 1300}}, 'children': [
        _vt_box('TouchV_Back', VX - 14, 136, VW + 28, 1000, [0.13, 0.125, 0.12, 1.0]),




        {'type': 'ImageWidget', 'name': 'TouchV_Cover', 'fields': {'rect': {'x': -2, 'y': 1132}, 'filename': 'Controller/Panel/zswide/VaultBottom'}},
        {'type': 'Widget', 'name': 'TouchV_Top', 'fields': {'rect': {'x': VX, 'y': 150, 'width': VW, 'height': 70}}, 'children': [
            _vt_sub('TouchV_SubU', 0, '@Unique'), _vt_sub('TouchV_SubS', 260, '@Sets'),




            {'type': 'GrowableImageWidget', 'name': 'TouchV_SearchBg', 'fields': {'rect': {'x': 540, 'y': -6, 'width': 1109, 'height': 102, 'scale': 0.75},
                'filename': 'PANEL\\Chronicle\\Chronicle_InputBox_Center', 'hoveredFrame': 1, 'leftCap/filename': 'PANEL\\Chronicle\\Chronicle_InputBox_Left', 'rightCap/filename': 'PANEL\\Chronicle\\Chronicle_InputBox_Right'},
             'children': [{'type': 'InputTextBoxWidget', 'name': 'TouchV_Search', 'fields': {'rect': {'x': 40, 'y': 0, 'width': 1036, 'height': 102}, 'backgroundColor': {'r': 0, 'g': 0, 'b': 0, 'a': 0.0},
                'maxStringLength': 16, 'imeEnabled': True, 'hint': {'text': '@dmVaultSearchHint', 'style': {'fontFace': 'BlizzardGlobal', 'alignment': {'v': 'center'}, 'pointSize': '$MediumLargeFontSize', 'fontColor': {'a': 84}}},
                'fontStyle': {'fontFace': 'BlizzardGlobal', 'alignment': {'v': 'center'}, 'pointSize': '$MediumLargeFontSize'}, 'useParentRectAsFocusRect': True, 'fontType': '12ptF',
                'onUpdateMessage': 'TouchVaultMessage:Search', 'virtualKeyboardType': 'Alphabet', 'leftCursorPadding': 40}}]}]},




        {'type': 'Widget', 'name': 'TouchV_Parts', 'fields': {'rect': {'x': VX, 'y': PT_Y, 'width': VW, 'height': 2 * PT_PITCH}}, 'children': [
            {'type': 'Widget', 'name': 'TouchV_PT%d' % i, 'fields': {'rect': {'x': (i % 6) * 228 + (i // 6) * 114, 'y': (i // 6) * PT_PITCH, 'width': 214, 'height': 104}}, 'children': [
                {'type': 'ImageWidget', 'name': 'Img', 'fields': {'rect': {'x': 0, 'y': 0}, 'filename': 'Controller/Panel/Stash/V2/AdditionalStash/CTRL_Tabs_215_TGA', 'frame': 0}},
                _vt_box('Hit', 0, 0, 214, 104, [0, 0, 0, 0]),
                _vt_nl(_vt_text('Text', 4, 0, 206, 104, '$FontColorGoldYellow', PT_FONT, 'center', p, trunc=False)),
                _vt_nl(_vt_text('TextOn', 4, 0, 206, 104, '$FontColorWhite', PT_FONT, 'center', p, trunc=False))]} for i, p in enumerate(PARTS)]},
        _vt_text('TouchV_Stats', VX, PT_Y + 2 * PT_PITCH + 6, 780, 56, '$FontColorBeige', '$SmallFontSize'),
        {'type': 'Widget', 'name': 'TouchV_Tiers', 'fields': {'rect': {'x': VX + VW - 590, 'y': PT_Y + 2 * PT_PITCH, 'width': 590, 'height': 65}}, 'children':
            _vt_check('TouchV_T0', 0, 0, '@dmVaultTierN') + _vt_check('TouchV_T1', 200, 0, '@dmVaultTierX') + _vt_check('TouchV_T2', 400, 0, '@dmVaultTierE')},
        {'type': 'Widget', 'name': 'TouchV_Uniq', 'fields': {'rect': {'x': VX, 'y': UNIQ_Y, 'width': VW, 'height': VAULT_ROWS * 138}}, 'children': cells},
        {'type': 'Widget', 'name': 'TouchV_Sets', 'fields': {'rect': {'x': 99999, 'y': 296, 'width': VW, 'height': 870}}, 'children': sets},








        {'type': 'ImageWidget', 'name': 'TouchV_PgL', 'fields': {'rect': {'x': 562, 'y': 1195, 'scale': 1.25}, 'filename': 'Lobby/Final/LobbyArrowLargeL', 'frame': 0}},
        {'type': 'ImageWidget', 'name': 'TouchV_PgR', 'fields': {'rect': {'x': 992, 'y': 1195, 'scale': 1.25}, 'filename': 'Lobby/Final/LobbyArrowLargeR', 'frame': 0}},
        _vt_box('TouchV_PgLHit', 466, 1168, 310, 310, [0, 0, 0, 0]), _vt_box('TouchV_PgRHit', 896, 1168, 310, 310, [0, 0, 0, 0]),
        _vt_text('TouchV_Page', 696, 1222, 280, 80, '$FontColorGoldYellow', '$LargeFontSize', 'center'),
        {'type': 'ImageWidget', 'name': 'TouchV_Drag', 'fields': {'rect': {'x': 99999, 'y': 0}}},


        {'type': 'NineTileImageWidget', 'name': 'TouchV_TipBg', 'fields': {'rect': {'x': 99999, 'y': 0, 'width': 900, 'height': 600}, 'filename': 'PANEL/TooltipBorder/TooltipBorder', 'backgroundColor': [0.0, 0.0, 0.0, 0.85], 'tileScale': 1.0}},
        {'type': 'TextBoxWidget', 'name': 'TouchV_Tip', 'fields': {'rect': {'x': 99999, 'y': 0, 'width': 1100, 'height': 600}, 'text': '', 'fitToText': True,
            'style': {'fontColor': '$FontColorWhite', 'pointSize': '$TooltipFontSize', 'alignment': {'h': 'center', 'v': 'top'}, 'options': {'pipeCharacterBehavior': 'handle', 'newlineHandling': 'reversed'}}}}]}

def vault_tab(s):
    for a, b in (('"tabCount": 5,', '"tabCount": 6,'), ('"inactiveFrames": [ 0, 0, 0, 0, 0 ],', '"inactiveFrames": [ 0, 0, 0, 0, 0, 0 ],'), ('"activeFrames": [ 1, 1, 1, 1, 1 ],', '"activeFrames": [ 1, 1, 1, 1, 1, 1 ],'),
                 ('"disabledFrames": [-1, 0, 0, 0, 0 ],', '"disabledFrames": [-1, 0, 0, 0, 0, -1 ],'), ('"textStrings": [ "@personal", "@shared", "@gems", "@materials", "@runes" ],', '"textStrings": [ "@personal", "@shared", "@dmTroveTab", "@materials", "@runes", "@dmStashVaultTab" ],')):
        if s.count(a) != 1: raise SystemExit('wide_stash：储藏箱布局 %s 不是原样（原版布局变了？）' % a.strip())
        s = s.replace(a, b)




    i = s.index('"name": "BankTabs"'); k = '"tabRightIndicatorPosition": "$HalfPanelRightTabButtonPromptPosition",'; j = s.find(k, i)
    if j < 0 or j - i > 2500: raise SystemExit('wide_stash：BankTabs 的右端键帽位置不是原样（原版布局变了？）')
    s = s[:j] + '"tabRightIndicatorPosition": { "x": 920, "y": -2 },' + s[j + len(k):]


    pg = {'type': 'Widget', 'name': 'TouchPgPage', 'fields': {'rect': {'x': 99999, 'y': 0, 'width': 1730, 'height': 1300}}, 'children': [
        _vt_text('TouchPgPage_Text', 675, 1159, 322, 64, '$FontColorGoldYellow', '$MediumFontSize', 'center')]}
    end = s.rstrip().rstrip('}').rstrip(); assert end.endswith(']'); at = len(end) - 1
    return s[:at].rstrip().rstrip(',') + ',\n        ' + json.dumps(pg, ensure_ascii=False) + ',\n        ' + json.dumps(vault_widgets(), ensure_ascii=False) + '\n    ' + s[at:]

def overlay_layout(text):
    i = text.find('"name": "Background"'); j = text.find('"TabBarWidget", "name": "Tabs"')
    if i < 0 or j < 0: raise SystemExit('wide_stash：外框布局里没有 Background / Tabs（原版布局变了？）')
    k = text.rfind('{', 0, j)
    add = ('{\n                    "type": "ImageWidget", "name": "WideBackground",\n                    "fields": {\n'
           '                        "rect": { "x": 99999, "y": 0 },\n                        "filename": "Controller/Panel/zswide/MainPanelBG_Wide",\n                    },\n                },\n                ')
    text = text[:k] + add + text[k:]






    st = {'type': 'Widget', 'name': 'TouchSideTitle', 'fields': {'rect': {'x': 99999, 'y': 0, 'width': 3272, 'height': 103}}, 'children': [
        _vt_text('TouchSideTitle_L', 0, 0, 1832, 103, '$FontColorWhite', '$MediumFontSize', 'center', '@stash'),
        _vt_text('TouchSideTitle_R', 1832, 0, 1440, 103, '$FontColorWhite', '$MediumFontSize', 'center', '@box')]}


    j = text.find('"TabBarWidget", "name": "Tabs"'); k = text.rfind('{', 0, j); d = 0; e = k
    while e < len(text):
        c = text[e]
        if c == '{': d += 1
        elif c == '}':
            d -= 1
            if d == 0: break
        e += 1
    if d != 0: raise SystemExit('wide_stash：外框布局里 Tabs 控件的括号没配上（原版布局变了？）')
    return text[:e + 1] + ',\n                ' + json.dumps(st, ensure_ascii=False) + text[e + 1:]

def build(romfs, outdir, repl):
    out = {}




    for sub in ('', 'base_'):
        path, inv = _entry(romfs, '/global/excel/' + sub.replace('_', '/') + 'inventory.bin'); inv = bytearray(inv)
        for row in (10, 25):
            o = 4 + row * 240 + 16
            if inv[o:o + 2] != b'\x0a\x0a': raise SystemExit('wide_stash：%s 第 %d 行不是 10×10（原版表变了？）' % (path, row))
            inv[o] = COLS






        import struct
        for row, dx, dy in ((11, -320, 0), (26, -240, 60)):
            base = 4 + row * 240 + 0x28
            if struct.unpack_from('<4i', inv, base + 4 * 0x14) != (-1, -1, -1, -1): raise SystemExit('wide_stash：%s 第 %d 行的项链格不是空的（原版表变了？）' % (path, row))
            for k in range(4, 10):
                l, r, t, b_, x = struct.unpack_from('<5i', inv, 4 + 0x28 + k * 0x14)
                struct.pack_into('<5i', inv, base + k * 0x14, l + dx, r + dx, t + dy, b_ + dy, x)
        p = os.path.join(outdir, 'ws_' + sub + 'inventory.bin'); open(p, 'wb').write(inv); out[path] = p
    for n, rel in SPRITES.items():
        p = os.path.join(outdir, 'ws_' + n + '.sprite'); shutil.copyfile(os.path.join(ART, n + '_wide.sprite'), p); out['data/hd/global/ui/' + rel + '.sprite'] = p
    p = os.path.join(outdir, 'ws_mainpanelbg_wide.sprite'); shutil.copyfile(os.path.join(ART, 'MainPanelBG_Wide.sprite'), p); out['+data/hd/global/ui/' + WIDE_FRAME + '.sprite'] = p
    for n in ('VaultBottom',):
        p = os.path.join(outdir, 'ws_' + n.lower() + '.sprite'); shutil.copyfile(os.path.join(ART, n + '.sprite'), p); out['+data/hd/global/ui/controller/panel/zswide/' + n.lower() + '.sprite'] = p
    for rel, fn in ((BANK, bank_layout), (OVERLAY, overlay_layout)):
        logical = touch_layout.read_layout(romfs, rel)[0].lower(); src = repl.get(logical)
        text = open(src, encoding='utf-8').read() if src else touch_layout.read_layout(romfs, rel)[1]
        p = os.path.join(outdir, 'ws_' + os.path.basename(rel)); open(p, 'w', encoding='utf-8').write(fn(text)); out[logical] = p
    return out, {'cols': COLS, 'files': len(out)}
