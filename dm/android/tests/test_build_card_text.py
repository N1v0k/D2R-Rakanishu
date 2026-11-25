








import csv
import io
import json
import os
import re
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'dm/ios/tools'))
sys.path.insert(0, str(ROOT / 'dm/android/tools'))

UNITS = {'formal': 1.294, 'exocet': 1.281}
PITCH = {'formal': 1.71, 'exocet': 1.77}
WRAP = (336 / 353, 350 / 353)
TITLE_LINE = 362
HOST_UI = ROOT / 'dm/host/dm_build_ui.c'

COLUMNS = {'enUS': 2, 'esES': 3, 'esMX': 4, 'ruRU': 5, 'deDE': 7, 'ptBR': 8}
FALLBACK = {'esMX': 'esES', 'ptBR': 'enUS'}

ALL_COLUMNS = dict(COLUMNS, zhCN=1, koKR=6)


def strings(prefix):
    with (ROOT / 'dm/i18n/ui_text.tsv').open(newline='', encoding='utf-8') as table:
        rows = {r[0]: r for r in csv.reader(table, delimiter='\t', quoting=csv.QUOTE_NONE) if r and r[0].startswith(prefix)}
    def get(key, lang):
        text = rows[key][ALL_COLUMNS[lang]]
        return get(key, FALLBACK[lang]) if not text and lang in FALLBACK else text
    return get


def common_suffix(texts):

    k = 0
    while all(len(t) > k for t in texts) and len({t[-1 - k] for t in texts}) == 1: k += 1
    return texts[0][len(texts[0]) - k:]


@unittest.skipUnless(os.environ.get('D2TEST_GAME_FONTS') and os.environ.get('DM_ROMFS_SRC'), 'needs D2TEST_GAME_FONTS and DM_ROMFS_SRC')
class CardText(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        from PIL import ImageFont
        import romfs_art
        import touch_layout
        fonts = Path(os.environ['D2TEST_GAME_FONTS'])
        cls.font = {'exocet': ImageFont.truetype(str(fonts / 'ExocetBlizzardOT-Medium.otf'), 200),
                    'formal': ImageFont.truetype(str(fonts / 'Formal436BT.ttf'), 200),
                    'formal-ru': ImageFont.truetype(str(fonts / 'Philosopher-BoldItalic.ttf'), 200),
                    'formal-ko': ImageFont.truetype(str(fonts / 'IrisL.ttf'), 200), 'exocet-ko': ImageFont.truetype(str(fonts / 'KoDia.ttf'), 200),
                    'global': ImageFont.truetype(str(fonts / 'BlizzardGlobal-V5_81.ttf'), 200)}
        card = touch_layout._build_card(False, touch_layout.build_card_place(os.environ['DM_ROMFS_SRC']))['children'][0]
        cls.box = {k['name']: (k['fields']['rect']['width'], k['fields']['rect']['height'], k['fields']['style']['pointSize'],
                               'formal' if k['fields']['style'].get('fontFace') == 'Formal' else 'exocet')
                   for k in card['children'] if k['type'] == 'TextBoxWidget'}
        rows = json.loads(romfs_art.read(os.environ['DM_ROMFS_SRC'], 'local/lng/strings/skills.json').decode('utf-8-sig'))
        names = {r['Key']: r for r in rows}
        excel = lambda f: csv.DictReader(io.StringIO(romfs_art.read(os.environ['DM_ROMFS_SRC'], 'global/excel/' + f).decode()), delimiter='\t')
        desc = {r['skilldesc']: r for r in excel('skilldesc.txt')}
        key = {r['*Id']: desc[r['skilldesc']]['str name'] for r in excel('skills.txt') if r.get('*Id', '').isdigit() and r.get('skilldesc') in desc}
        cls.guides = [json.loads(p.read_text(encoding='utf-8')) for p in sorted((ROOT / 'dm/android/builds').glob('*.json')) if p.name != 'schema.json']
        ids = {s['skill'] for g in cls.guides for p in g['phases'] for s in p['skills']}
        cls.names = names
        cls.skill_keys = [key[str(i)] for i in sorted(ids)]
        cls.skill_names = {lang: [names[k][lang] for k in cls.skill_keys] for lang in COLUMNS}

    def face(self, kind, lang):
        if lang == 'zhCN': return self.font['global']
        if lang == 'koKR': return self.font[kind + '-ko']
        return self.font['formal-ru' if kind == 'formal' and lang == 'ruRU' else kind]

    def units(self, kind, lang, text, size):
        if not text: return 0.0
        left, _, right, _ = self.face(kind, lang).getbbox(text)
        return (right - left) * size / 200 * UNITS[kind]

    def wrap(self, kind, lang, text, size, limit):
        lines, cut = [], []
        for word in text.replace('\n', ' ').split():
            if lines and self.units(kind, lang, lines[-1] + ' ' + word, size) <= limit:
                lines[-1] += ' ' + word
                continue
            if self.units(kind, lang, word, size) > limit: cut.append(word)
            while self.units(kind, lang, word, size) > limit and len(word) > 1:
                n = max([k for k in range(1, len(word)) if self.units(kind, lang, word[:k], size) <= limit] or [1])
                lines.append(word[:n]); word = word[n:]
            lines.append(word)
        return lines, cut

    def layouts(self, box, lang, text):

        width, height, size, kind = self.box[box]
        most = int((height - 1.3 * size) // (PITCH[kind] * size)) + 1
        return [self.wrap(kind, lang, text, size, width * w) for w in WRAP], most

    def fits(self, box, lang, text, whole=True):
        problems = []
        layouts, most = self.layouts(box, lang, text)
        for lines, cut in layouts:
            if len(lines) > most: problems.append('%s %s: %d lines > %d: %r' % (box, lang, len(lines), most, lines))
            if whole and cut: problems.append('%s %s: word cut: %r' % (box, lang, cut))
        return problems

    def card_title(self, title):

        font, size = self.font['exocet'], self.box['TouchBuildCardTitle'][2]
        width = lambda t: font.getlength(t) * size / 200 * UNITS['exocet']
        out = []
        for word in title.split(' '):
            piece = ''
            while width(word) > TITLE_LINE:
                ends = [k + 2 if word[k + 1] == '/' else k + 1 for k in range(1, len(word) - 1) if word[k] == '-']
                cuts = [e for e in ends if e < len(word) and width(word[:e]) <= TITLE_LINE]
                if not cuts: break
                piece += word[:cuts[-1]] + '\n'; word = word[cuts[-1]:]
            out.append(piece + word)
        return ' '.join(out)

    def test_titles_break_at_hyphens(self):
        problems = []
        for g in self.guides:
            for lang, text in (('deDE', g['title']['de']), ('enUS', g['title']['en'])):
                shown = self.card_title(text)
                problems += self.fits('TouchBuildCardTitle', lang, shown)
                for lines, _ in self.layouts('TouchBuildCardTitle', lang, shown)[0]:
                    problems += self.breaks_at_line_ends(g['id'], shown, lines)
                problems += ['%s %s: a line starts with a slash: %r' % (g['id'], lang, shown)] if '\n/' in shown else []
        self.assertEqual(problems, [])

    def breaks_at_line_ends(self, what, shown, lines):

        return ['%s: break after %r not at a line end: %r' % (what, head, lines) for head in shown.split('\n')[:-1]
                if not any(line.endswith(head.split(' ')[-1]) for line in lines)]

    def test_phase_and_note(self):
        problems = []
        for g in self.guides:
            for p in g['phases']:
                for lang, de in (('deDE', True), ('enUS', False)):
                    problems += self.fits('TouchBuildCardPhase', lang, '%s · %d–%d' % (p['name']['de' if de else 'en'], p['levels'][0], p['levels'][1]))
                for s in p['skills']:
                    n = s.get('note')
                    if not s.get('save') or not n or 'aufheben' in n['de'].lower() or n['en'].lower().startswith('keep'): continue
                    problems += self.fits('TouchBuildCardNote', 'deDE', n['de']) + self.fits('TouchBuildCardNote', 'enUS', n['en'])
        self.assertEqual(problems, [])

    def test_label_and_level_lines(self):

        get, problems = strings('dmBuildCard'), []
        for lang in ALL_COLUMNS:
            for key, value in (('dmBuildCardNext', None), ('dmBuildCardSave1', None), ('dmBuildCardSaveN', 99)):
                problems += self.fits('TouchBuildCardLabel', lang, get(key, lang) % value if value else get(key, lang))
            for key, value in (('dmBuildCardTo', 99), ('dmBuildCardLevelUp', None), ('dmBuildCardFrom', 99), ('dmBuildCardUntil', 99)):
                problems += self.fits('TouchBuildCardTo', lang, get(key, lang) % value if value else get(key, lang))
        self.assertEqual(problems, [])

    def test_skill_line(self):




        source = HOST_UI.read_text(encoding='utf-8')
        listed = set(re.findall(r'"([^"]+)"', re.search(r'SKILL_BREAK_DE\[\] = \{([^}]*)\}', source).group(1)))
        suffix = common_suffix([self.names[k]['deDE'] for k in re.findall(r'"([^"]+)"', re.search(r'SKILL_BREAK_DE\[\] = \{([^}]*)\}', source).group(1))])
        self.assertGreaterEqual(len(suffix.encode('utf-8')), int(re.search(r'SKILL_BREAK_MIN = (\d+)', source).group(1)), 'the host would not break')
        get, problems, broken = strings('dmBuildCard'), [], set()
        for lang in COLUMNS:
            for key, name in zip(self.skill_keys, self.skill_names[lang]):
                name = re.sub(r'\[[a-z]{2}\]', '', name)
                whole = not any(cut for _, cut in self.layouts('TouchBuildCardSkill', lang, name)[0])
                if lang == 'deDE' and key in listed and len(name) > len(suffix):
                    if whole: problems.append('%s %s fits whole, the host should not break it' % (key, name))
                    name = name[:-len(suffix)] + '-\n' + suffix; broken.add(key)
                    problems += self.breaks_at_line_ends(key, name, self.layouts('TouchBuildCardSkill', lang, name)[0][0][0])
                    problems += self.breaks_at_line_ends(key, name, self.layouts('TouchBuildCardSkill', lang, name)[0][1][0])
                problems += self.fits('TouchBuildCardSkill', lang, name)
            for key in ('dmBuildCardWait', 'dmBuildCardDone', 'dmBuildCardBlocked'):
                problems += self.fits('TouchBuildCardSkill', lang, get(key, lang))
        problems += ['%s is in the host list but no guide shows it' % k for k in sorted(listed - broken)]
        print('German skill names the host breaks before their common suffix (%d characters): %d' % (len(suffix), len(broken)))
        self.assertEqual(problems, [])


if __name__ == '__main__':
    unittest.main()
