#!/usr/bin/env python3












import os, shutil
import touch_layout

HERE = os.path.dirname(os.path.abspath(__file__))
INV = 'controller/console/switch/playerinventoryexpansionlayouthd.json'
TIP = 99999

def widgets():
    return ('        { "type": "Widget", "name": "TouchPouchBox", "children": [\n'
            '            { "type": "ButtonWidget", "name": "TouchPouch", "fields": { "rect": { "x": 222, "y": 1294, "width": 280, "height": 128 }, "filename": "Controller/Panel/zspouch/PouchBox", '
            '"onClickMessage": "PanelManager:ClosePanel:TouchPouch" } },\n'
            '            { "type": "ImageWidget", "name": "TouchPouchIcon", "fields": { "rect": { "x": 246, "y": 1310, "scale": 0.95 }, "filename": "Items/misc/rune/el_rune" } },\n'
            '            { "type": "TextBoxWidget", "name": "TouchPouchCount", "fields": { "rect": { "x": 350, "y": 1294, "width": 140, "height": 128 }, "text": "0", '
            '"style": { "fontColor": "$FontColorGoldYellow", "pointSize": "$MediumLargeFontSize", "alignment": { "h": "left", "v": "center" } } } },\n'
            '            { "type": "ButtonWidget", "name": "TouchStore", "fields": { "rect": { "x": 916, "y": 1323, "width": 366, "scale": 0.74 }, "filename": "FrontEnd/HD/Final/FrontEnd_ButtonSmall", "hoveredFrame": 1, '
            '"textString": "@strGoldDeposit", "textColor": "$LightButtonTextColor", "text/style": "$StyleFEButtonText", "onClickMessage": "PanelManager:ClosePanel:TouchStore" } },\n'
            '            { "type": "NineTileImageWidget", "name": "TouchPouchTipBg", "fields": { "rect": { "x": %d, "y": 0, "width": 700, "height": 400 }, "filename": "PANEL/TooltipBorder/TooltipBorder", "backgroundColor": [0.0, 0.0, 0.0, 0.85], "tileScale": 1.0 } },\n'
            '            { "type": "TextBoxWidget", "name": "TouchPouchTip", "fields": { "rect": { "x": %d, "y": 0, "width": 900, "height": 600 }, "text": "", "fitToText": true, '
            '"style": { "fontColor": "$FontColorWhite", "pointSize": "$TooltipFontSize", "alignment": { "h": "center", "v": "top" }, "options": { "newlineHandling": "standard" } } } },\n'
            '        ] },\n') % (TIP, TIP)

def inv_layout(s):
    if '"TouchPouchBox"' in s: raise SystemExit('pouch_layout：物品栏布局里已经有 TouchPouchBox')
    i = s.rstrip().rfind(']'); assert s.rstrip().endswith('}') and i > 0
    return s[:i].rstrip(' ') + widgets() + '    ' + s[i:]

def build(romfs, outdir, repl):
    out = {}
    p = os.path.join(outdir, 'ws_pouchbox.sprite'); shutil.copyfile(os.path.join(__import__('gen_paths').gen_dir('NativeHUD', 'pouch'), 'PouchBox.sprite'), p); out['+data/hd/global/ui/controller/panel/zspouch/pouchbox.sprite'] = p
    logical = touch_layout.read_layout(romfs, INV)[0].lower(); src = repl.get(logical)
    text = open(src, encoding='utf-8').read() if src else touch_layout.read_layout(romfs, INV)[1]
    p = os.path.join(outdir, 'pouch_' + os.path.basename(INV)); open(p, 'w', encoding='utf-8').write(inv_layout(text)); out[logical] = p
    return out, {'files': len(out)}
