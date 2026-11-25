





import contextlib
import importlib.util
import io
import os
from pathlib import Path
import sys
import tempfile
import types
import unittest

from PIL import Image

TOOLS = Path(__file__).resolve().parents[1] / 'tools'
spec = importlib.util.spec_from_file_location('gen_style', TOOLS / 'gen_style.py')
gs = importlib.util.module_from_spec(spec)
spec.loader.exec_module(gs)
profile_hd = gs.profile_hd

HD = b'''{
    "FontColorPartyGreen": { "r": 79, "g": 194, "b": 56, "a": 255 },
    "FontColorWhite": { "r": 240, "g": 240, "b": 240, "a": 255 },
    "FontColorBlue": { "r": 110, "g": 110, "b": 255, "a": 255 },
    "FontColorGray": { "r": 99, "g": 99, "b": 99, "a": 255 },
    "FontColorGrey": "$FontColorGray",
    "FontColorLightGray": { "r": 148, "g": 148, "b": 148, "a": 255 },
    "FontColorDarkGrayGold" : { "r": 173, "g": 168, "b": 148, "a": 255 },
    "PanelNoteTextColor" : "$FontColorDarkGrayGold",
    "MediumFontSize": 26, // Usage : Settings field text
    "SmallFontSize": 22,
    "TooltipFontSize": 36,
    "MediumPanelFontSize": "$MediumFontSize",
    "ConsoleUIScale": 1.0,
    "DefaultSpacing" : { "kerning": 1.0 },
    "MinimumSpacing" : { "kerning": 0.88 },
    "DefaultDropShadow": { "shadowColor": { "r": 0, "g": 0, "b": 0, "a": 200 }, "offset": { "x": 3, "y": 3 }, },
    "StyleVictoryText": { "dropShadow": { "shadowColor": { "r": 0, "g": 0, "b": 0, "a": 255 }, "offset": { "x": 8, "y": 8 } } },
    "SkillQuantityTextStyle": { "fontColor": "$FontColorBlue", "dropShadow": { "shadowColor": { "r": 0, "g": 0, "b": 0, "a": 200 }, "offset": { "x": 4, "y": 4 } } },
    "SkillHotkeyTextRect": { "x": -4, "y": 3, "width": 132, "height": 124 },
    "TooltipStyle": {
        "padding": [ 20, 12, 20, 12 ],
        "inGameTargetPadding": [ 7, 1, 7, 1 ],
        "backgroundColor": [ 0, 0, 0, 0.75 ],
        "inGameBackgroundColor": [ 0, 0, 0, 0.6 ],
        "substringColor": "$FontColorLightGray",
        "SocketedColor": "$FontColorGrey",
    },
}'''
CONTROLLER = b'{ "basedOn": "HD", }'
CONSOLE = b'''{
    "basedOn": "ControllerHD",
    "MediumFontSize": 28,
    "SmallFontSize": 26,
    "ConsoleUIScale": 1.16,
}'''
SKILLTREE = b'''{ "type": "SkillsTreePanel", "name": "SkillsTreePanel",
    "fields": { "hoverButtonTint": { "r": 1.5, "g": 1.447, "b": 1.335, "a": 1.5 }, "grantedColor": { "r": 102, "g": 102, "b": 255 } },
    "children": [ { "type": "TextBoxWidget", "name": "CounterTemplate", "fields": { "rect": { "x": 148, "y": 82, "width": 52, "height": 52 } } } ] }'''
SKILLTREE_PAD = b'''{ "basedOn": "SkillsTreePanelHD.json", "type": "SkillsTreePanel", "name": "SkillsTreePanel", "fields": { },
    "children": [ { "type": "Widget", "name": "Group", "children": [
        { "type": "TextBoxWidget", "name": "CounterTemplate",
          "fields": { "rect": { "x": 154, "y": 79, "width": 62, "height": 62 },
                      "style": { "fontColor": "$FontColorWhite", "pointSize": "$MediumFontSize", "spacing": "$MinimumSpacing" } } } ] } ] }'''
GLOBALDATA = b'''{ "children": [ { "type": "LanguageFontRemapper", "name": "LanguageFontRemapper", "fields": { "fontRemapping": [
    { "inLanguage": "Russian", "useFont": "Philosopher", "asFont": "Formal" },
    { "inLanguage": "Korean", "useFont": "Iris", "asFont": "Formal" },
    { "inLanguage": "Korean", "useFont": "KoDia", "asFont": "Exocet" },
    { "inLanguage": "Korean", "useFont": "BlizzardGlobalTraditionalChinese", "asFont": "BlizzardGlobal" },
    { "inLanguage": "Chinese", "useFont": "BlizzardGlobal", "asFont": "Exocet" },
    { "inLanguage": "Chinese", "useFont": "BlizzardGlobal", "asFont": "Formal" },
    { "inLanguage": "German", "useFont": "Unknown Font", "asFont": "Formal" },
] } } ] }'''
FILES = {profile_hd.FILES['HD']: HD, profile_hd.FILES['ControllerHD']: CONTROLLER, profile_hd.FILES['ConsoleHD']: CONSOLE,
         gs.SKILLTREE: SKILLTREE, gs.SKILLTREE_PAD: SKILLTREE_PAD, gs.GLOBALDATA: GLOBALDATA}
FONT_FILES = set(gs.FAMILIES.values())


def square(im, l, t, size, rim, inner, rim_w=3):
    px = im.load()
    for x in range(l, l + size):
        for y in range(t, t + size):
            edge = min(x - l, l + size - 1 - x, y - t, t + size - 1 - y)
            px[x, y] = (20, 20, 20, rim if edge < rim_w else inner)


def tree(box=31, box_inner=126, slot=76):


    im = Image.new('RGBA', (629, 638), (0, 0, 0, 0))
    square(im, 95, 84, slot, 210, 100)
    px = im.load()
    for x in range(126, 140):
        for y in range(84 + slot, 84 + slot + 40): px[x, y] = (60, 60, 60, 150)
    c = 192
    square(im, c - box // 2, 144 - box // 2, box, 210, box_inner)
    return im


def fake_romfs(files=FILES, picture=None, fonts=FONT_FILES):
    m = types.ModuleType('romfs_art')
    def read(romfs, path):
        if path in files: return files[path]
        if path.startswith(gs.FONTS) and path[len(gs.FONTS):] in fonts: return b'font:' + path.encode()
        raise SystemExit('no %s' % path)
    def frame(romfs, path, i=0):
        if path == gs.TREE:
            if picture is None: raise SystemExit('no tree')
            return picture
        if path == gs.BUTTON: return Image.new('RGBA', (265, 51), (i * 40, 0, 0, 255))
        raise SystemExit('no %s' % path)
    m.read, m.frame = read, frame
    return m


@contextlib.contextmanager
def romfs(module):
    saved = sys.modules.get('romfs_art')
    sys.modules['romfs_art'] = module
    try: yield
    finally:
        if saved is None: del sys.modules['romfs_art']
        else: sys.modules['romfs_art'] = saved


def stage(module):
    d = tempfile.mkdtemp()
    out = io.StringIO()
    with romfs(module), contextlib.redirect_stdout(out): ok = gs.stage('romfs', d)
    return ok, d, out.getvalue()


def table(d):
    with open(os.path.join(d, 'style', 'style.tsv'), encoding='utf-8') as f:
        return [tuple(line.rstrip('\n').split('\t')) for line in f if not line.startswith('#')]


class RowsTest(unittest.TestCase):
    def setUp(self):
        with contextlib.redirect_stdout(io.StringIO()): self.rows = gs.rows(lambda path: FILES[path])

    def kind(self, k): return [r for r in self.rows if r[0] == k]

    def test_colours(self):
        c = {r[1]: r[2:] for r in self.kind('colour')}
        self.assertEqual(c['FontColorWhite'], (240, 240, 240, 255))
        self.assertEqual(c['FontColorGrey'], (99, 99, 99, 255))
        self.assertEqual(c['PanelNoteTextColor'], (173, 168, 148, 255))
        self.assertEqual(c['Tooltip.backgroundColor'], (0, 0, 0, 191))
        self.assertEqual(c['Tooltip.inGameBackgroundColor'], (0, 0, 0, 153))
        self.assertEqual(c['Tooltip.substringColor'], (148, 148, 148, 255))
        self.assertEqual(c['Tooltip.SocketedColor'], (99, 99, 99, 255))
        self.assertEqual(c['SkillTree.granted'], (102, 102, 255, 255))
        self.assertNotIn('Tooltip.UniqueColor', c)
        self.assertNotIn('TabsActiveTextColor', c)

    def test_shadows_spacing_pads(self):
        self.assertEqual(self.kind('shadow'), [('shadow', 'default', '3', '3', 0, 0, 0, 200), ('shadow', 'button', '4', '4', 0, 0, 0, 200),
                                               ('shadow', 'victory', '8', '8', 0, 0, 0, 255)])
        self.assertEqual(self.kind('spacing'), [('spacing', 'DefaultSpacing', '1'), ('spacing', 'MinimumSpacing', '0.88')])
        self.assertEqual(self.kind('pad'), [('pad', 'tooltip', '20', '12', '20', '12'), ('pad', 'tooltipInGame', '7', '1', '7', '1')])

    def test_sizes_per_profile(self):
        s = {(r[1], r[2]): r[3] for r in self.kind('size')}
        self.assertEqual((s[('hd', 'MediumFontSize')], s[('console', 'MediumFontSize')]), ('26', '28'))
        self.assertEqual((s[('hd', 'SmallFontSize')], s[('console', 'SmallFontSize')]), ('22', '26'))
        self.assertEqual(s[('console', 'MediumPanelFontSize')], '28')
        self.assertEqual(s[('console', 'TooltipFontSize')], '36')
        self.assertEqual(self.kind('scale'), [('scale', 'hd', '1'), ('scale', 'console', '1.16')])

    def test_skill_tree(self):
        self.assertEqual(self.kind('counter'), [('counter', '154', '79', '62', '62', '132', 'MediumFontSize', 'MinimumSpacing', 'FontColorWhite')])
        self.assertEqual(self.kind('tint'), [('tint', 'hover', '1.5', '1.447', '1.335', '1.5')])

    def test_fonts(self):
        f = {(r[1], r[2]): r[3] for r in self.kind('font')}
        self.assertEqual(f[('en', 'exocet')], 'ExocetBlizzardOT-Medium.otf')
        self.assertEqual(f[('ru', 'formal')], 'Philosopher-BoldItalic.ttf')
        self.assertEqual((f[('ko', 'exocet')], f[('ko', 'formal')], f[('ko', 'global')]), ('KoDia.ttf', 'IrisL.ttf', 'BlizzardGlobalTCUnicode.ttf'))
        self.assertEqual(f[('zh', 'exocet')], 'BlizzardGlobal-V5_81.ttf')
        self.assertNotIn(('de', 'formal'), f)
        self.assertEqual(f[('de', 'exocet')], 'ExocetBlizzardOT-Medium.otf')
        self.assertEqual(len(f), 7 * 3 - 1)

    def test_missing_parts(self):
        files = dict(FILES)
        del files[gs.SKILLTREE_PAD]; del files[gs.GLOBALDATA]
        with contextlib.redirect_stdout(io.StringIO()): rows = gs.rows(lambda path: files[path])
        kinds = {r[0] for r in rows}
        self.assertNotIn('counter', kinds); self.assertNotIn('font', kinds); self.assertIn('colour', kinds)
        with self.assertRaises(ValueError), contextlib.redirect_stdout(io.StringIO()): gs.rows(lambda path: {}[path])


class PictureTest(unittest.TestCase):
    COUNTER = (154, 79, 62, 62, 132)

    def test_cuts(self):
        im = tree()
        box = gs.numbox(im, self.COUNTER)
        self.assertEqual(box.size, (31, 31))
        self.assertEqual(box.getpixel((15, 15))[3], 126)
        self.assertEqual(gs.three_part(box), 3)
        slot = gs.slot(im)
        self.assertEqual(slot.size, (76, 76))
        self.assertEqual(slot.getpixel((0, 0))[3], 210)

    def test_checks(self):
        with self.assertRaises(ValueError): gs.numbox(tree(box=21), self.COUNTER)
        with self.assertRaises(ValueError): gs.numbox(tree(box_inner=250), self.COUNTER)
        with self.assertRaises(ValueError): gs.numbox(Image.new('RGBA', (629, 638)), self.COUNTER)
        with self.assertRaises(ValueError): gs.slot(tree(slot=60))
        with self.assertRaises(ValueError): gs.numbox(tree(), (154, 79, 100, 100, 132))
        with self.assertRaises(ValueError): gs.numbox(tree(), (154, 79, 62, 30, 132))


class StageTest(unittest.TestCase):
    def test_all(self):
        ok, d, log = stage(fake_romfs(picture=tree()))
        self.assertTrue(ok, log)
        names = sorted(os.listdir(os.path.join(d, 'style')))
        self.assertEqual(names, ['button_0.png', 'button_1.png', 'button_2.png', 'button_3.png', 'numbox.png', 'slot.png', 'style.tsv'])
        rows = table(d)
        self.assertIn(('box', 'numbox', '3'), rows)
        self.assertIn(('colour', 'FontColorWhite', '240', '240', '240', '255'), rows)
        self.assertIn(('colour', 'FontColorPartyGreen', '79', '194', '56', '255'), rows)
        self.assertTrue(os.path.isfile(os.path.join(d, 'fonts', 'BlizzardGlobalTCUnicode.ttf')))
        self.assertFalse(any(n.endswith('.part') for n in os.listdir(os.path.join(d, 'fonts'))))

    def test_bad_tree_keeps_the_table(self):
        ok, d, log = stage(fake_romfs(picture=tree(box=20)))
        self.assertTrue(ok)
        names = os.listdir(os.path.join(d, 'style'))
        self.assertNotIn('numbox.png', names); self.assertIn('slot.png', names); self.assertIn('style.tsv', names)
        self.assertFalse([r for r in table(d) if r[0] == 'box'])
        self.assertIn('point box not cut', log)
        ok, d, log = stage(fake_romfs(picture=None))
        self.assertTrue(ok)
        self.assertEqual(sorted(n for n in os.listdir(os.path.join(d, 'style')) if n.endswith('.png')), ['button_%d.png' % i for i in range(4)])

    def test_missing_font_drops_its_rows(self):
        ok, d, log = stage(fake_romfs(picture=tree(), fonts=FONT_FILES - {'BlizzardGlobalTCUnicode.ttf'}))
        self.assertTrue(ok)
        self.assertNotIn(('font', 'ko', 'global', 'BlizzardGlobalTCUnicode.ttf'), table(d))
        self.assertIn(('font', 'ko', 'exocet', 'KoDia.ttf'), table(d))
        self.assertEqual(os.listdir(os.path.join(d, 'fonts')).count('BlizzardGlobalTCUnicode.ttf.part'), 0)

    def test_nothing_readable(self):
        d = tempfile.mkdtemp()
        os.makedirs(os.path.join(d, 'style'))
        open(os.path.join(d, 'style', 'old.png'), 'wb').close()
        out = io.StringIO()
        with romfs(fake_romfs(files={})), contextlib.redirect_stdout(out): ok = gs.stage('romfs', d)
        self.assertFalse(ok)
        self.assertFalse(os.path.exists(os.path.join(d, 'style')))
        self.assertIn('game style not staged', out.getvalue())
        self.assertEqual(len(out.getvalue().strip().splitlines()), 1, out.getvalue())


if __name__ == '__main__':
    unittest.main()
