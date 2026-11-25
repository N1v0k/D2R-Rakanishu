





import importlib.util
import re
from pathlib import Path
import sys
import types
import unittest

TOOLS = Path(__file__).resolve().parents[1] / 'tools'
sys.path.insert(0, str(TOOLS))
import profile_hd


def load(name):
    spec = importlib.util.spec_from_file_location(name, TOOLS / (name + '.py'))
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    return m


HD = b'''{
    // Named globals.
    "FontColorWhite": { "r": 240, "g": 240, "b": 240, "a": 255 },
    "FontColorRed": { "r": 252, "g": 70, "b": 70, "a": 255 },
    "FontColorGrey": "$FontColorGray",
    "FontColorGray": { "r": 99, "g": 99, "b": 99 },
    "MediumFontSize": 26, // Usage : Settings field text
    "SmallFontSize": 22,
    "Url": "http://example//not-a-comment",
    "Shadow": { "shadowColor": { "r": 0, "g": 0, "b": 0, "a": 200 }, "offset": { "x": 3, "y": 3 }, },
    "TooltipStyle": { "backgroundColor": [ 0, 0, 0, 0.75 ], "substringColor": "$FontColorGrey", "UniqueColor": "$Missing", },
}'''
CONTROLLER = b'''{
    "basedOn": "HD",
    "Shadow": { "offset": { "x": 4 } },
    "Only": 1,
}'''
CONSOLE = b'''{
    // Base (parent) profile.
    "basedOn": "ControllerHD",
    "MediumFontSize": 28,
    "Scale": "$ConsoleScale",
    "ConsoleScale": 1.16,
}'''


def reader(files):
    by_path = {profile_hd.FILES[k]: v for k, v in files.items()}
    return lambda path: by_path[path]


class ProfileTest(unittest.TestCase):
    def test_parse_comments_and_commas(self):
        p = profile_hd.parse(HD.decode())
        self.assertEqual(p['MediumFontSize'], 26)
        self.assertEqual(p['Url'], 'http://example//not-a-comment')
        self.assertEqual(p['Shadow']['offset'], {'x': 3, 'y': 3})

    def test_chain_child_over_parent(self):
        p = profile_hd.chain(reader({'ConsoleHD': CONSOLE, 'ControllerHD': CONTROLLER, 'HD': HD}))
        self.assertEqual(p['MediumFontSize'], 28)
        self.assertEqual(p['SmallFontSize'], 22)
        self.assertEqual(p['Only'], 1)
        self.assertEqual(p['Shadow']['offset'], {'x': 4, 'y': 3})
        self.assertEqual(p['Shadow']['shadowColor']['a'], 200)
        self.assertEqual(profile_hd.number(p['Scale'], p), 1.16)
        hd = profile_hd.chain(reader({'HD': HD}), profile_hd.HD)
        self.assertEqual(hd['MediumFontSize'], 26)

    def test_chain_refuses_unknown_and_loops(self):
        with self.assertRaises(ValueError): profile_hd.chain(reader({'ConsoleHD': b'{"basedOn": "Nope"}'}))
        loop = {'ConsoleHD': b'{"basedOn": "ControllerHD"}', 'ControllerHD': b'{"basedOn": "ConsoleHD"}'}
        with self.assertRaises(ValueError): profile_hd.chain(reader(loop))
        with self.assertRaises(ValueError): profile_hd.chain(reader({'ConsoleHD': b'[1, 2]'}))

    def test_merge_leaves_inputs(self):
        a, b = {'x': {'y': 1, 'z': 2}}, {'x': {'y': 3}}
        self.assertEqual(profile_hd.merge(a, b), {'x': {'y': 3, 'z': 2}})
        self.assertEqual(a, {'x': {'y': 1, 'z': 2}})
        self.assertEqual(profile_hd.merge({'x': {'y': 1}}, {'x': 5}), {'x': 5})

    def test_colours_and_aliases(self):
        p = profile_hd.parse(HD.decode())
        tip = p['TooltipStyle']
        self.assertEqual(profile_hd.colour(tip['backgroundColor'], p), [0, 0, 0, 191])
        self.assertEqual(profile_hd.colour(tip['substringColor'], p), [99, 99, 99, 255])
        self.assertIsNone(profile_hd.colour(tip['UniqueColor'], p))
        self.assertIsNone(profile_hd.colour({'r': 300, 'g': 0, 'b': 0}, p))
        self.assertIsNone(profile_hd.colour([0, 0, 2, 1], p))
        self.assertIsNone(profile_hd.colour(p.get('NoSuchKey'), p))
        loop = {'A': '$B', 'B': '$A'}
        self.assertIsNone(profile_hd.colour('$A', loop))
        self.assertIsNone(profile_hd.resolve('$A', loop))
        self.assertEqual(profile_hd.number('$MediumFontSize', p), 26)
        self.assertIsNone(profile_hd.number('$FontColorWhite', p))
        self.assertIsNone(profile_hd.number(True, p))
        self.assertEqual(profile_hd.alias('$MediumFontSize'), 'MediumFontSize')
        self.assertIsNone(profile_hd.alias('MediumFontSize'))
        self.assertIsNone(profile_hd.alias('$'))

    def test_effect_card_style(self):
        style = load('gen_effect_data').style(HD.decode())
        self.assertEqual(style, {'background': [0, 0, 0, 191], 'sub': [99, 99, 99, 255], 'red': [252, 70, 70, 255]})

    def test_poison_keeps_element_colour(self):
        art = load('gen_app_art')
        fake = types.ModuleType('romfs_art')
        fake.read = lambda *args: b'{"FontColorGreen":{"r":0,"g":252,"b":0},"FontColorPartyGreen":{"r":79,"g":194,"b":56}}'
        saved = sys.modules.get('romfs_art')
        sys.modules['romfs_art'] = fake
        try:
            self.assertEqual(art.dmg_colours('romfs'), ['poison 0 252 0'])
        finally:
            if saved is None: del sys.modules['romfs_art']
            else: sys.modules['romfs_art'] = saved
        src = TOOLS.parent / 'app/src/main/java/com/xiaoboluo/d2r'
        numbers = (src / 'DamageNumbers.java').read_text()
        values = re.search(r'String\[\] COLOUR = \{(.*?)\}', numbers).group(1)
        self.assertEqual(re.findall(r'"([^"\n]+)"', values), [colour for _, colour in art.DMG_COLOURS])
        legacy = (src / 'GameStyle.java').read_text()
        values = re.search(r'String\[\] DMG_KEYS = \{(.*?)\}', legacy, re.S).group(1)
        self.assertEqual(re.findall(r'"([^"\n]+)"', values), [entry for pair in art.DMG_COLOURS for entry in pair])

    def test_damage_colours(self):
        art = load('gen_app_art')
        fake = types.ModuleType('romfs_art')
        fake.read = lambda romfs, path: {profile_hd.FILES['HD']: HD}[path]
        saved = sys.modules.get('romfs_art')
        sys.modules['romfs_art'] = fake
        try: lines = art.dmg_colours('romfs')
        finally:
            if saved is None: del sys.modules['romfs_art']
            else: sys.modules['romfs_art'] = saved
        self.assertEqual(lines, ['physical 240 240 240', 'fire 252 70 70'])


if __name__ == '__main__':
    unittest.main()
