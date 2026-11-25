






import json
import re

TAB_W = 212
ART = 'data/hd/global/ui/ztouch_looks_ui/vendor_tabs_%d.sprite' % TAB_W
TAB_FONT = 26
LABEL_FONT = 17
SMALL_FONT = LABEL_FONT - 2
HINT_FONT = 26
TABS = (('"filename": "Controller/Panel/Vendor/V2/VendorTabs",', '"filename": "ztouch_looks_ui/vendor_tabs_%d",' % TAB_W),
        ('"inactiveFrames": [ 1, 1, 1, 1 ],', '"tabCount": 5, "inactiveFrames": [ 1, 1, 1, 1, 1 ],'),
        ('"activeFrames": [ 0, 0, 0, 0 ],', '"activeFrames": [ 0, 0, 0, 0, 0 ], "textStrings": [ "@strBSArmor", "@strBSWeapons", "@strBSWeapons", "@strBSMisc", "@dmLooksTab" ],'),
        ('"textStyle": { "pointSize": "$MediumFontSize" },', '"textStyle": { "pointSize": %d },' % TAB_FONT),
        ('"tabSize": { "x": 269, "y": 118 },', '"tabSize": { "x": %d, "y": 118 },' % TAB_W))


def narrow(frame, width):


    from PIL import Image
    if frame.size != (134, 60) or width >= frame.width or width % 2 or width < 40:
        raise SystemExit('looks_tab: unexpected vendor tab art')
    image = Image.new('RGBA', (width, frame.height))
    source = (0, 6, frame.width // 2 - 8, frame.width // 2 + 8, frame.width - 6, frame.width)
    target = (0, 6, width // 2 - 8, width // 2 + 8, width - 6, width)
    for left, right, x0, x1 in zip(source, source[1:], target, target[1:]):
        image.paste(frame.crop((left, 0, right, frame.height)).resize((x1 - x0, frame.height), Image.Resampling.BILINEAR), (x0, 0))
    return image


def art(romfs, outdir):
    import os
    import struct
    from PIL import Image
    import romfs_art
    import gen_widestash_art as codec
    raw = romfs_art.read(romfs, 'hd/global/ui/Controller/Panel/Vendor/V2/VendorTabs.sprite')
    version, frame_width, sheet_width, height = struct.unpack_from('<HHII', raw, 4)
    frames = struct.unpack_from('<H', raw, 20)[0]
    if (version, frame_width, sheet_width, height, frames) != (61, 134, 269, 60, 2):
        raise SystemExit('looks_tab: unexpected vendor tab sprite')
    lib = codec.lib(); source = codec.decode(lib, raw); width = TAB_W // 2
    sheet = Image.new('RGBA', (width * frames, height))
    for index in range(frames):
        left = index * (sheet_width // frames)
        sheet.paste(narrow(source.crop((left, 0, left + frame_width, height)), width), (index * width, 0))
    path = os.path.join(outdir, os.path.basename(ART))
    with open(path, 'wb') as output:
        output.write(codec.pack(lib, raw, codec.bc3(sheet), sheet.width, height, width, frames))
    return {ART: path}


def tab(text):
    i = text.index('"name": "tabs"')
    j = text.index('"tabRightIndicatorPosition"', i)
    head, block, tail = text[:i], text[i:j], text[j:]
    for a, b in TABS:
        a_re = re.escape(a).replace(r'\ ', r'\s*')
        if len(re.findall(a_re, block)) != 1:
            raise SystemExit('looks_tab: the vendor tab bar is not as expected (%s)' % a)
        block = re.sub(a_re, lambda m: b, block)
    return head + block + tail


TILES = 50
DYE_TILES = 18
DYE_FONT = 20
DYE_COLOURS = 21
SWATCH = (12, 72, 74, 18)
GRID = (162, 159)
CELL = 98
GOLD = (547, 1254, 250, 57)
SELECTOR = 'Controller/PANEL/ControllerSelector/ControllerSelectorBorder'


def _box(name, x, y, w, h, rgba, border=None):
    f = {'rect': {'x': x, 'y': y, 'width': w, 'height': h}, 'backgroundColor': rgba}
    if border:
        f.update(filename=border, tileScale=0.5)
    return {'type': 'NineTileImageWidget', 'name': name, 'fields': f}


def _text(name, x, y, w, h, color, size, align='left', text='', trunc=True):
    return {'type': 'TextBoxWidget', 'name': name, 'fields': {'rect': {'x': x, 'y': y, 'width': w, 'height': h}, 'text': text,
            'style': {'fontColor': color, 'pointSize': size, 'alignment': {'h': align, 'v': 'center'}}, 'truncateWithEllipsis': trunc}}


def _tile(k):


    return {'type': 'Widget', 'name': 'TouchL_T%d' % k, 'fields': {'rect': {'x': 99999, 'y': 0, 'width': 2 * CELL, 'height': 2 * CELL}}, 'children': [
        _box('Hit', 0, 0, 2 * CELL, 2 * CELL, [0, 0, 0, 0]),
        _box('Hi', 0, 0, 2 * CELL, 2 * CELL, [0.10, 0.32, 0.08, 0.85]),
        {'type': 'Widget', 'name': 'Box', 'fields': {'rect': {'x': 6, 'y': 6, 'width': 2 * CELL - 12, 'height': 2 * CELL - 12}},
         'children': [{'type': 'ImageWidget', 'name': 'Img', 'fields': {'rect': {'x': 0, 'y': 0}}}]},
        _text('Label', 0, 2 * CELL - 40, 2 * CELL, 36, '$FontColorWhite', LABEL_FONT, 'center', trunc=False),
        _text('LabelS', 0, 2 * CELL - 30, 2 * CELL, 28, '$FontColorWhite', SMALL_FONT, 'center', trunc=False),
        _box('Gold', -18, -18, 2 * CELL + 36, 2 * CELL + 36, [0, 0, 0, 0], SELECTOR)]}


def swatch_colours(romfs):


    import romfs_art
    maps, pal = romfs_art.read(romfs, 'global/items/Palette/grey.dat'), romfs_art.read(romfs, 'global/palette/ACT1/pal.dat')
    if len(maps) != DYE_COLOURS * 256 or len(pal) != 768:
        raise SystemExit('looks_tab: unexpected item colour maps or palette')
    rgb = [(pal[3 * i + 2], pal[3 * i + 1], pal[3 * i]) for i in range(256)]
    greys = [i for i, c in enumerate(rgb) if max(c) - min(c) < 14 and 110 <= c[0] <= 230]
    if len(greys) < 4:
        raise SystemExit('looks_tab: the palette has no light greys')
    out = []
    for c in range(DYE_COLOURS):
        seen = [rgb[maps[c * 256 + i]] for i in greys]
        out.append(tuple(sum(x[k] for x in seen) // len(seen) for k in range(3)))
    return out


def _dye_tile(k):


    return {'type': 'Widget', 'name': 'TouchL_D%d' % k, 'fields': {'rect': {'x': 99999, 'y': 0, 'width': CELL, 'height': CELL}}, 'children': [
        _box('Hit', 0, 0, CELL, CELL, [0, 0, 0, 0]),
        _box('Hi', 0, 0, CELL, CELL, [0.10, 0.32, 0.08, 0.85]),
        {'type': 'Widget', 'name': 'Box', 'fields': {'rect': {'x': 6, 'y': 6, 'width': CELL - 12, 'height': CELL - 12}},
         'children': [{'type': 'ImageWidget', 'name': 'Img', 'fields': {'rect': {'x': 0, 'y': 0}}}]},
        _text('Label', 0, CELL - 30, CELL, 28, '$FontColorWhite', SMALL_FONT, 'center', trunc=False),
        _box('Gold', -18, -18, CELL + 36, CELL + 36, [0, 0, 0, 0], SELECTOR)]}


PAD_KEYS = (('A', 'ButtonRight'), ('X', 'ButtonTop'), ('Y', 'ButtonLeft'))
PAD_LEG_FONT = 34


def _pad_legend():

    out = []
    for k, cap in PAD_KEYS:
        out.append({'type': 'ImageWidget', 'name': 'TouchL_PK' + k, 'fields': {'rect': {'x': 99999, 'y': 0, 'scale': 1.1}, 'filename': 'Controller/ControllerButtons/Switch/' + cap}})
        out.append({'type': 'TextBoxWidget', 'name': 'TouchL_PT' + k, 'fields': {'rect': {'x': 99999, 'y': 0, 'width': 10, 'height': 60}, 'text': '', 'fitToText': True,
                    'style': {'fontColor': '$FontColorWhite', 'pointSize': PAD_LEG_FONT, 'alignment': {'h': 'left', 'v': 'center'}}}})
    return out


def widget(romfs):
    x, y, w, h = GOLD

    swatches = [_box('TouchL_S%d' % c, 99999, 0, SWATCH[2], SWATCH[3], [r / 255, g / 255, b / 255, 1.0]) for c, (r, g, b) in enumerate(swatch_colours(romfs))]
    return json.dumps({'type': 'Widget', 'name': 'TouchLooks', 'fields': {'rect': {'x': 99999, 'y': 0, 'width': 1220, 'height': 1700}}, 'children': [
        _box('TouchL_Back', GRID[0], GRID[1], 10 * CELL, 10 * CELL, [0, 0, 0, 0])] + [_tile(k) for k in range(TILES)] + [
        _box('TouchL_DyeLine', 99999, 0, 10 * CELL, 4, [0.78, 0.64, 0.35, 1.0]),
        _text('TouchL_DyeTitle', 99999, 0, CELL - 20, 44, '$FontColorGoldYellow', DYE_FONT, 'center', trunc=False)] + [_dye_tile(k) for k in range(DYE_TILES)] + swatches + [
        _box('TouchL_NameBg', 99999, 0, 400, 60, [0.0, 0.0, 0.0, 0.85], 'PANEL/TooltipBorder/TooltipBorder'),
        _text('TouchL_Name', 99999, 0, 400, 60, '$FontColorWhite', '$MediumFontSize', 'center'),
        _box('TouchL_DyeNameBg', 99999, 0, 900, 60, [0.0, 0.0, 0.0, 0.85], 'PANEL/TooltipBorder/TooltipBorder'),
        _text('TouchL_DyeName', 99999, 0, 900, 60, '$FontColorWhite', HINT_FONT, 'center'),
        _text('TouchL_Hint', GRID[0], GRID[1] + 10 * CELL + 4, 720, 48, '$FontColorBeige', HINT_FONT),
        {'type': 'ImageWidget', 'name': 'TouchL_PgL', 'fields': {'rect': {'x': 99999, 'y': GRID[1] + 10 * CELL + 2, 'scale': 0.5}, 'filename': 'Lobby/Final/LobbyArrowLargeL', 'frame': 0}},
        _text('TouchL_Page', 960, GRID[1] + 10 * CELL + 4, 100, 48, '$FontColorGoldYellow', '$SmallFontSize', 'center'),
        {'type': 'ImageWidget', 'name': 'TouchL_PgR', 'fields': {'rect': {'x': 99999, 'y': GRID[1] + 10 * CELL + 2, 'scale': 0.5}, 'filename': 'Lobby/Final/LobbyArrowLargeR', 'frame': 0}},
        _box('TouchL_PgLHit', 99999, GRID[1] + 10 * CELL - 10, 110, 90, [0, 0, 0, 0]), _box('TouchL_PgRHit', 99999, GRID[1] + 10 * CELL - 10, 110, 90, [0, 0, 0, 0]),
        _box('TouchL_PriceBg', x, y + 4, w, h - 8, [0.05, 0.045, 0.04, 1.0]),
        _text('TouchL_Price', x, y, w, h, '$FontColorCurrencyGold', '$XMediumLargeFontSize'),
        _text('TouchL_Broken', GRID[0] + 40, GRID[1] + 300, 10 * CELL - 80, 160, '$FontColorRed', '$MediumFontSize', 'center'),
        {'type': 'ButtonWidget', 'name': 'TouchL_Apply', 'fields': {'rect': {'x': 99999, 'y': 1492, 'width': 366, 'scale': 0.74}, 'filename': 'FrontEnd/HD/Final/FrontEnd_ButtonSmall',
         'hoveredFrame': 1, 'textString': '@dmLooksApply', 'textColor': '$LightButtonTextColor', 'text/style': '$StyleFEButtonText', 'onClickMessage': 'PanelManager:ClosePanel:TouchLooksApply'}},
        {'type': 'ButtonWidget', 'name': 'TouchL_Anew', 'fields': {'rect': {'x': 99999, 'y': GRID[1] + 500, 'width': 366, 'scale': 0.74}, 'filename': 'FrontEnd/HD/Final/FrontEnd_ButtonSmall',
         'hoveredFrame': 1, 'textString': '@dmLooksAnew', 'textColor': '$LightButtonTextColor', 'text/style': '$StyleFEButtonText', 'onClickMessage': 'PanelManager:ClosePanel:TouchLooksAnew'}},
        _box('TouchL_Focus', 99999, 0, CELL + 36, CELL + 36, [0, 0, 0, 0], SELECTOR)] + _pad_legend()},
        ensure_ascii=False)


def slot_frame():

    return json.dumps(_box('TouchLooksSel', 99999, 0, 232, 232, [0, 0, 0, 0], SELECTOR), ensure_ascii=False)
