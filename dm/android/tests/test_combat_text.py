
import ast
import csv
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import tempfile
import unittest
from PIL import ImageFont

ROOT = Path(__file__).resolve().parents[3]


def cmap(path):
    data = path.read_bytes()
    u16 = lambda at: struct.unpack_from('>H', data, at)[0]
    u32 = lambda at: struct.unpack_from('>I', data, at)[0]
    table = next(u32(12 + i*16 + 8) for i in range(u16(4)) if data[12+i*16:16+i*16] == b'cmap')
    glyphs = set()
    for i in range(u16(table+2)):
        record = table+4+i*8
        if u16(record) not in (0, 3):
            continue
        sub = table+u32(record+4)
        if u16(sub) == 4:
            n = u16(sub+6)//2
            for j in range(n):
                end = u16(sub+14+j*2)
                start = u16(sub+16+n*2+j*2)
                delta = u16(sub+16+n*4+j*2)
                address = sub+16+n*6+j*2
                offset = u16(address)
                for cp in range(start, end+1):
                    glyph = u16(address+offset+2*(cp-start)) if offset else cp
                    if glyph and (glyph+delta)&65535:
                        glyphs.add(cp)
        elif u16(sub) == 12:
            for j in range(u32(sub+12)):
                start, end, glyph = struct.unpack_from('>III', data, sub+16+j*12)
                glyphs.update(range(start+(glyph == 0), end+1))
    if not glyphs:
        raise ValueError('Font has no Unicode glyph table')
    return glyphs


def literals(path, skip_calls=frozenset()):

    source = path.read_text()
    tokens = list(re.finditer(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])\'', source))
    masked = list(source)
    for token in tokens:
        for at in range(token.start(), token.end()):
            masked[at] = ' '
    masked = ''.join(masked)
    for token in tokens:
        text = token.group()
        if not text.startswith('"') or skip_calls and calls(masked, token.start()) & skip_calls:
            continue
        text = text.replace(r'\n', '').replace(r'\t', '').replace(r'\r', '')
        if path.suffix == '.java':
            yield ast.literal_eval(text)
        else:

            escaped = ''.join(chr(b) if b < 128 else f'\\x{b:02x}' for b in text.encode())
            yield ast.literal_eval('b'+escaped).decode().replace('\ue07e', '')


def calls(masked, at):

    names, depth = set(), 0
    for i in range(at-1, -1, -1):
        c = masked[i]
        if c == ')':
            depth += 1
        elif c == '(':
            if depth:
                depth -= 1
            else:
                name = re.search(r'([A-Za-z_][A-Za-z0-9_]*)\s*$', masked[:i])
                if name:
                    names.add(name.group(1))
        elif depth == 0 and c in ';{}':
            break
    return names



NOT_DRAWN = frozenset({'LOG', 'strstr', 'strcmp', 'strncmp', 'memcmp', 'getenv'})


def ui_rows(prefix):

    with (ROOT/'dm/i18n/ui_text.tsv').open(newline='') as table:
        for row in csv.reader(table, delimiter='\t', quoting=csv.QUOTE_NONE):
            if row and row[0].startswith(prefix):
                yield from (row[2], row[7], row[8])


def production_lines():

    names = json.loads((Path(os.environ['DM_SIM_UI_DIR'])/'sim-names.json').read_text())
    tests = ROOT/'dm/host/tests'
    with tempfile.TemporaryDirectory() as directory:
        executable = str(Path(directory)/'lines')
        sources = [str(tests/'t_sim_lines.c')] + [str(ROOT/'dm/host'/name) for name in
                   ('dm_sim_calc.c', 'dm_sim_data.c', 'dm_sim_core.c', 'dm_sim_skill.c', 'dm_sim_timeline.c', 'dm_sim_report.c',
                    'dm_splash_rule.c')]
        subprocess.run(['gcc', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-I'+str(ROOT/'dm/host'), *sources, '-o', executable, '-lm'], check=True)
        ids = subprocess.check_output([executable, 'ids'], text=True).split()
        if not ids:
            raise ValueError('No damage skills in the simulation data')

        for german, language in ((1, 1), (0, 0), (0, 12)):
            skill_names = sorted({names['@skill:'+i][language] for i in ids if '@skill:'+i in names})
            if len(skill_names) < len(ids) // 2:
                raise ValueError('Skill names missing from sim-names.json')
            yield from subprocess.check_output([executable, 'lines', str(german), *skill_names], text=True).splitlines()
    yield from gear_lines()


def gear_lines():

    with (ROOT/'dm/i18n/ui_text.tsv').open(newline='') as table:
        rows = {row[0]: row for row in csv.reader(table, delimiter='\t', quoting=csv.QUOTE_NONE) if row and row[0].startswith('dmGear')}
    for column in (2, 7, 8):
        word = lambda key: rows[key][column]
        for key in ('dmGearGroup', 'dmGearSingle', 'dmGearSurvival', 'dmGearRecovery'):
            yield word(key) + ' -29999,9 %'
        for key in ('dmGearMissed', 'dmGearReached'):
            yield word(key).replace('%d', '200')
        yield word('dmGearBelow').replace('%s', '199,9')
        yield word('dmGearBelowSame').replace('%s', '199')
        for group in (k for k in rows if k.startswith('dmGearR') and k != 'dmGearRecovery'):
            for key in ('dmGearMore', 'dmGearLess'):
                yield word(key).replace('%s', word(group))
        for reason in (k for k in rows if k.startswith('dmGearNo')):
            yield word('dmGearUnrated').replace('%s', word(reason))


class CombatTextTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.fonts = Path(os.environ['D2TEST_GAME_FONTS'])
        cls.global_, cls.formal, cls.exocet = (cmap(cls.fonts/name) for name in ('BlizzardGlobal-V5_81.ttf', 'Formal436BT.ttf', 'ExocetBlizzardOT-Medium.otf'))

    def test_every_template_has_glyphs(self):

        native = list((ROOT/'dm/host').glob('dm_sim_*.c'))
        native += [ROOT/'dm/host'/name for name in ('dm_sim_live.inc', 'dm_sim_text.h', 'dm_itemcmp.c', 'dm_itemcmp_core.c', 'dm_tipx.c', 'dm_tipx_core.c')]
        texts = [(str(path.relative_to(ROOT)), text) for path in native for text in literals(path, NOT_DRAWN)]
        texts += [('dm/i18n/ui_text.tsv', text) for text in ui_rows('dmCmp')]
        texts += [('dm/i18n/ui_text.tsv', text) for text in ui_rows('dmGear')]

        java = list((ROOT/'dm/android/app/src/main/java/com/xiaoboluo/d2r').glob('Combat*.java'))
        page = [(str(path.relative_to(ROOT)), text) for path in java for text in literals(path)]
        page += [('dm/i18n/ui_text.tsv', text) for text in ui_rows('dmAndroidCombat')]


        names = json.loads((Path(os.environ['DM_SIM_UI_DIR'])/'sim-names.json').read_text())
        page += [('sim-names.json', names['@potion:%d' % group][language]) for group in range(3) for language in (0, 1, 2, 3, 4, 6, 10, 12)]
        forbidden = {0x2248, 0x2014, 0x2212}
        missing = []
        for where, text in texts:
            for cp in set(map(ord, text)):
                if cp >= 32 and (cp in forbidden or any(cp not in table for table in (self.global_, self.formal, self.exocet))):
                    missing.append((where, f'U+{cp:04X}', text))
        for where, text in page:
            for cp in set(map(ord, text)):
                if cp >= 32 and (cp in forbidden or cp not in self.global_ and (cp not in self.formal or cp not in self.exocet)):
                    missing.append((where, f'U+{cp:04X}', text))
        self.assertEqual(missing, [])
        self.assertGreater(len(texts), 300, 'item comparison and simulation sources must be part of the check')

    def test_not_drawn_text_is_recognised(self):
        sample = Path(tempfile.mkdtemp())/'sample.c'
        self.addCleanup(sample.unlink)
        sample.write_text('LOG("a %s", f("b")); add(K, 0, "c", g("d"));\nif (strstr(s, "e")) x = "f";\n')
        self.assertEqual(list(literals(sample, NOT_DRAWN)), ['c', 'd', 'f'])

    def test_tooltip_formats_fit(self):

        viewport, line_height, advance = 2688, 36, 19
        width = int(.45 * viewport)
        max_chars = int(.30 * viewport) // advance
        fonts = [ImageFont.truetype(str(self.fonts/name), line_height)
                 for name in ("BlizzardGlobal-V5_81.ttf", "ExocetBlizzardOT-Medium.otf")]
        lines = list(production_lines())
        self.assertGreater(len(lines), 100)
        for text in lines:
            with self.subTest(text=text):
                self.assertNotIn('(', text)

                if text.startswith(("Schaden/s", "Damage/s", "Erster Punkt", "First point")):
                    pieces = [text]
                elif len(text) <= max_chars:
                    pieces = [text]
                else:

                    label, value = text.split(': ', 1)
                    pieces = [label+':', value]
                    self.assertLessEqual(len(pieces[0]), max_chars)
                    self.assertLessEqual(len(pieces[1]), max_chars)
                for piece in pieces:
                    for font in fonts:
                        self.assertLessEqual(font.getlength(piece), width)


if __name__ == '__main__':
    unittest.main()
