#!/usr/bin/env python3


















import os
import shutil
import sys

DM = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))
sys.path.insert(0, os.path.join(DM, 'ios', 'tools'))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import profile_hd

SKILLTREE = 'global/ui/layouts/skillstreepanelhd.json'
SKILLTREE_PAD = 'global/ui/layouts/controller/skillstreepanelhd.json'
GLOBALDATA = 'global/ui/layouts/globaldatahd.json'
TREE = 'hd/global/ui/Controller/Panel/Spells/V2/PaSkillTree.sprite'
BUTTON = 'hd/global/ui/PANEL/Modals/ModalButton.sprite'
FONTS = 'hd/ui/fonts/'


SEMANTIC = ('PanelTitleColor', 'PanelNoteTextColor', 'LightButtonTextColor', 'DarkButtonTextColor', 'ModalDescriptionTextColor', 'TabsActiveTextColor',
            'TabsInactiveTextColor', 'TabsActiveTextColorInGame', 'DescriptionTextColor', 'DescriptionTitleColor')

TOOLTIP = ('backgroundColor', 'inGameBackgroundColor', 'substringColor', 'DefaultColor', 'MagicColor', 'RareColor', 'SetColor', 'UniqueColor', 'CraftedColor',
           'QuestColor', 'SocketedColor')

SHADOWS = (('default', ('DefaultDropShadow',)), ('button', ('SkillQuantityTextStyle', 'dropShadow')), ('victory', ('StyleVictoryText', 'dropShadow')))
SPACINGS = ('DefaultSpacing', 'ReducedSpacing', 'MinimumSpacing')
PADS = (('tooltip', 'padding'), ('tooltipInGame', 'inGameTargetPadding'))
PROFILES = (('hd', profile_hd.HD), ('console', profile_hd.CONSOLE))

LANGS = (('en', 'English'), ('de', 'German'), ('es', 'Spanish'), ('mx', 'Spanish Mexican'), ('ru', 'Russian'), ('ko', 'Korean'), ('zh', 'Chinese'))
ROLES = (('exocet', 'Exocet'), ('formal', 'Formal'), ('global', 'BlizzardGlobal'))

FAMILIES = {'Exocet': 'ExocetBlizzardOT-Medium.otf', 'Formal': 'Formal436BT.ttf', 'BlizzardGlobal': 'BlizzardGlobal-V5_81.ttf',
            'BlizzardGlobalTraditionalChinese': 'BlizzardGlobalTCUnicode.ttf', 'Philosopher': 'Philosopher-BoldItalic.ttf', 'KoDia': 'KoDia.ttf', 'Iris': 'IrisL.ttf'}






PX_PER_UNIT = 0.5
SLOT_CENTRE = (132.5, 122.0)
PROBE = 16
BOX_SIZE, BOX_TOL = 31, 3
BOX_INNER, BOX_RIM = (100, 150), (170, 235)
SLOT_SIZE, SLOT_TOL = 76, 4
SLOT_INNER, SLOT_RIM = (40, 160), (170, 235)


def fmt(v):
    return str(int(v)) if float(v).is_integer() else '%.6g' % v


def colour_rows(named):
    out = []
    names = [k for k in named if k.startswith('FontColor')] + list(SEMANTIC)
    for name in names:
        c = profile_hd.colour(named.get(name), named)
        if c: out.append(('colour', name) + tuple(c))
    tip = named.get('TooltipStyle')
    if isinstance(tip, dict):
        for key in TOOLTIP:
            c = profile_hd.colour(tip.get(key), named)
            if c: out.append(('colour', 'Tooltip.' + key) + tuple(c))
    return out


def dig(node, path):
    for key in path:
        node = node.get(key) if isinstance(node, dict) else None
    return node


def shadow_rows(named):
    out = []
    for name, path in SHADOWS:
        s = profile_hd.resolve(dig(named, path[:1]), named)
        s = profile_hd.resolve(dig(s, path[1:]), named) if len(path) > 1 else s
        if not isinstance(s, dict): continue
        off, c = s.get('offset'), profile_hd.colour(s.get('shadowColor'), named)
        dx, dy = (profile_hd.number(off.get(k), named) if isinstance(off, dict) else None for k in 'xy')
        if c and dx is not None and dy is not None: out.append(('shadow', name, fmt(dx), fmt(dy)) + tuple(c))
    return out


def size_rows(profiles):
    out = []
    for tag, named in profiles:
        for name in named:
            if not name.endswith('FontSize'): continue
            n = profile_hd.number(named[name], named)
            if n is not None and n > 0: out.append(('size', tag, name, fmt(n)))
        k = profile_hd.number(named.get('ConsoleUIScale'), named)
        if k is not None and k > 0: out.append(('scale', tag, fmt(k)))
    return out


def find(node, name):

    if isinstance(node, dict):
        if node.get('name') == name: return node
        node = node.get('children')
    if isinstance(node, list):
        for child in node:
            hit = find(child, name)
            if hit is not None: return hit
    return None


def skilltree_rows(base, pad, named):



    out = []
    fields = profile_hd.merge(base.get('fields', {}), pad.get('fields', {}))
    counter = find(pad, 'CounterTemplate')
    f = counter.get('fields') if isinstance(counter, dict) else None
    rect, style = (f.get('rect'), f.get('style')) if isinstance(f, dict) else (None, None)
    side = dig(profile_hd.resolve(named.get('SkillHotkeyTextRect'), named), ('width',))
    if isinstance(rect, dict) and isinstance(style, dict) and isinstance(side, (int, float)) and side > 0:
        box = [rect.get(k) for k in ('x', 'y', 'width', 'height')]
        names = [profile_hd.alias(style.get(k)) for k in ('pointSize', 'spacing', 'fontColor')]
        if all(isinstance(v, (int, float)) for v in box) and box[2] > 0 and box[3] > 0 and all(names):
            out.append(('counter',) + tuple(fmt(v) for v in box + [side]) + tuple(names))
    tint = fields.get('hoverButtonTint')
    if isinstance(tint, dict) and all(isinstance(tint.get(k), (int, float)) for k in 'rgba'):
        out.append(('tint', 'hover') + tuple(fmt(tint[k]) for k in 'rgba'))
    granted = profile_hd.colour(fields.get('grantedColor'), named)
    if granted: out.append(('colour', 'SkillTree.granted') + tuple(granted))
    return out


def font_rows(globaldata):


    remap = find(globaldata, 'LanguageFontRemapper')
    pairs = dig(remap, ('fields', 'fontRemapping')) or []
    use = {(p.get('inLanguage'), p.get('asFont')): p.get('useFont') for p in pairs if isinstance(p, dict)}
    out = []
    for lang, language in LANGS:
        for role, font in ROLES:
            file = FAMILIES.get(use.get((language, font), font))
            if file: out.append(('font', lang, role, file))
    return out


def rows(read):


    profiles, errors = [], []
    for tag, name in PROFILES:
        try: profiles.append((tag, profile_hd.chain(read, name)))
        except (Exception, SystemExit) as e: errors.append('%s: %s' % (name, e))
    if not profiles: raise ValueError('no UI profile readable (%s)' % '; '.join(errors))
    for e in errors: print('appart: game style profile %s not read' % e)
    named = profiles[-1][1]
    out = colour_rows(named) + shadow_rows(named) + size_rows(profiles)
    for name in SPACINGS:
        k = profile_hd.number(dig(profile_hd.resolve(named.get(name), named), ('kerning',)), named)
        if k is not None and k > 0: out.append(('spacing', name, fmt(k)))
    tip = named.get('TooltipStyle')
    for name, key in PADS:
        p = tip.get(key) if isinstance(tip, dict) else None
        if isinstance(p, list) and len(p) == 4 and all(isinstance(v, (int, float)) and v >= 0 for v in p): out.append(('pad', name) + tuple(fmt(v) for v in p))
    try: out += skilltree_rows(profile_hd.parse(read(SKILLTREE).decode('utf-8', 'replace')), profile_hd.parse(read(SKILLTREE_PAD).decode('utf-8', 'replace')), named)
    except (Exception, SystemExit) as e: print('appart: skill tree style not read (%s)' % e)
    try: out += font_rows(profile_hd.parse(read(GLOBALDATA).decode('utf-8', 'replace')))
    except (Exception, SystemExit) as e: print('appart: font remapping not read (%s)' % e)
    return out


def walk(im, x, y, dx, dy):


    a = im.getchannel('A').load()
    if a[x, y] < PROBE: raise ValueError('no frame at %d,%d' % (x, y))
    while 0 <= x + dx < im.width and 0 <= y + dy < im.height and a[x + dx, y + dy] >= PROBE: x, y = x + dx, y + dy
    return x, y


def check(im, box, size, tol, inner, rim, what):


    l, t, r, b = box
    w, h = r - l, b - t
    if abs(w - size) > tol or abs(h - size) > tol: raise ValueError('%s is %dx%d, expected about %d' % (what, w, h, size))
    a = im.getchannel('A').load()
    centre = a[(l + r) // 2, (t + b) // 2]
    ring = max(a[x, y] for x in range(l, r) for y in range(t, b) if min(x - l, r - 1 - x, y - t, b - 1 - y) < 3)
    if not inner[0] <= centre <= inner[1] or not rim[0] <= ring <= rim[1]: raise ValueError('%s alpha %d / rim %d out of range' % (what, centre, ring))
    return im.crop(box)


def numbox(tree, counter):


    x, y, w, h, side = counter
    if abs(w * PX_PER_UNIT - BOX_SIZE) > BOX_TOL or abs(h * PX_PER_UNIT - BOX_SIZE) > BOX_TOL:
        raise ValueError('point box layout %gx%g units, expected about %d pixels' % (w, h, BOX_SIZE))
    cx = round(SLOT_CENTRE[0] + (x + w / 2 - side / 2) * PX_PER_UNIT)
    cy = round(SLOT_CENTRE[1] + (y + h / 2 - side / 2) * PX_PER_UNIT)
    l, r = walk(tree, cx, cy, -1, 0)[0], walk(tree, cx, cy, 1, 0)[0]
    t, b = walk(tree, cx, cy, 0, -1)[1], walk(tree, cx, cy, 0, 1)[1]
    return check(tree, (l, t, r + 1, b + 1), BOX_SIZE, BOX_TOL, BOX_INNER, BOX_RIM, 'point box')


def slot(tree):


    cx, cy = round(SLOT_CENTRE[0]), round(SLOT_CENTRE[1])
    l, r = walk(tree, cx, cy, -1, 0)[0], walk(tree, cx, cy, 1, 0)[0]
    t, b = walk(tree, l + 2, cy, 0, -1)[1], walk(tree, l + 2, cy, 0, 1)[1]
    return check(tree, (l, t, r + 1, b + 1), SLOT_SIZE, SLOT_TOL, SLOT_INNER, SLOT_RIM, 'skill slot')


def three_part(im):



    a = im.getchannel('A').load()
    y, w = im.height // 2, im.width
    centre = a[w // 2, y]
    def side(xs):
        xs = list(xs)
        for i in range(len(xs) - 3):
            if all(abs(a[xs[i + k], y] - centre) <= 16 for k in range(3)): return i
        return len(xs) // 2
    end = max(side(range(w)), side(range(w - 1, -1, -1)))
    return max(2, min(end, w // 2 - 1))


def stage(romfs, appart):


    import romfs_art
    out = os.path.join(appart, 'style')
    if os.path.isdir(out): shutil.rmtree(out)
    try:
        os.makedirs(out)
        read = lambda path: romfs_art.read(romfs, path)
        lines = rows(read)
        fonts = os.path.join(appart, 'fonts')
        os.makedirs(fonts, exist_ok=True)
        missing = set()
        for row in [r for r in lines if r[0] == 'font']:
            path = os.path.join(fonts, row[3])
            if row[3] in missing or os.path.isfile(path): continue
            try:
                with open(path + '.part', 'wb') as f: f.write(romfs_art.read(romfs, FONTS + row[3]))
                os.replace(path + '.part', path)
            except (Exception, SystemExit) as e:
                missing.add(row[3]); print('appart: font %s not staged (%s); the App uses the default font of that role' % (row[3], e))
                if os.path.exists(path + '.part'): os.remove(path + '.part')
        lines = [r for r in lines if r[0] != 'font' or r[3] not in missing]
        counter = next((r for r in lines if r[0] == 'counter'), None)
        tree = None
        try: tree = romfs_art.frame(romfs, TREE, 0)
        except (Exception, SystemExit) as e: print('appart: skill tree picture not read (%s); the App draws the plain tooltip fill' % e)
        if tree is not None and counter is None: print('appart: no point box layout; the App draws the plain tooltip fill for numbers')
        if tree is not None and counter is not None:
            try:
                box = numbox(tree, tuple(float(v) for v in counter[1:6]))
                box.save(os.path.join(out, 'numbox.png'))
                lines.append(('box', 'numbox', str(three_part(box))))
            except (Exception, SystemExit) as e: print('appart: point box not cut (%s); the App draws the plain tooltip fill for numbers' % e)
        if tree is not None:
            try: slot(tree).save(os.path.join(out, 'slot.png'))
            except (Exception, SystemExit) as e: print('appart: skill slot not cut (%s); the App draws the plain tooltip fill behind symbols' % e)
        buttons = [os.path.join(out, 'button_%d.png' % i) for i in range(4)]
        try:
            for i, path in enumerate(buttons): romfs_art.frame(romfs, BUTTON, i).save(path)
        except (Exception, SystemExit) as e:
            for path in buttons:
                if os.path.exists(path): os.remove(path)
            print('appart: dialog button frames not staged (%s)' % e)
        text = '# game style from the player\'s own game files (gen_style.py)\n' + ''.join('\t'.join(str(v) for v in r) + '\n' for r in lines)
        with open(os.path.join(out, 'style.tsv.part'), 'w', encoding='utf-8', newline='\n') as f: f.write(text)
        os.replace(os.path.join(out, 'style.tsv.part'), os.path.join(out, 'style.tsv'))
    except (Exception, SystemExit) as e:
        if os.path.isdir(out): shutil.rmtree(out)
        print('appart: game style not staged (%r); the App uses its built-in style' % (e,))
        return False
    kinds = {}
    for r in lines: kinds[r[0]] = kinds.get(r[0], 0) + 1
    print('appart: game style %s, pictures %s' % (', '.join('%d %s' % (n, k) for k, n in kinds.items()), ', '.join(sorted(f for f in os.listdir(out) if f.endswith('.png')))))
    return True


if __name__ == '__main__':
    sys.exit(0 if stage(sys.argv[1], sys.argv[2]) else 1)
