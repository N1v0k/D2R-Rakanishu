






import csv, json, re, sys
from pathlib import Path

root = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(root / 'ios/tools'))
import touch_layout as layout

out, _ = layout._custom_page('"name": "VideoOptionsPanel", "name": "OptionsTable"')
page = json.loads(re.sub(r',\s*([}\]])', r'\1', out))
nodes = {}
def walk(node):
    if isinstance(node, dict):
        if 'name' in node:
            assert node['name'] not in nodes, node['name']
            nodes[node['name']] = node
        for child in node.values():
            walk(child)
    elif isinstance(node, list):
        for child in node:
            walk(child)
walk(page)
host = (root / 'host/dm_custom.c').read_text()

game = {'focusIndicatorPadding': '$OptionsCarouselFocusIndicatorPadding', 'focusIndicatorFilename': '$OptionsCarouselFocusIndicatorFilename', 'fontType': '8ptE',
        'style': {'pointSize': 36, 'spacing': '$ReducedSpacing', 'alignment': {'h': 'center', 'v': 'center'}},
        'enabledColor': '$CarouselEnabledColor', 'disabledColor': '$CarouselDisabledColor'}
video_names = set()
def vnames(node):
    if isinstance(node, dict):
        if 'name' in node: video_names.add(node['name'])
        for child in node.values(): vnames(child)
    elif isinstance(node, list):
        for child in node: vnames(child)
vnames(nodes['TouchVideoRows'])

cars = {n: v for n, v in nodes.items() if v.get('type') == 'CarouselWidget' and n not in video_names}
assert sorted(cars) == sorted('TouchCustom_' + k for k in ('lang', 'touch_swap_above', 'cam', 'dmg_num', 'streak', 'melee_splash', 'shrine_stacking', 'shrine_run_speed', 'resume_pos', 'shoulder_l', 'shoulder_r')), sorted(cars)
for n, c in cars.items():
    f = c['fields']
    assert {k: f[k] for k in game} == game and 'onUpdateMessage' not in f, (n, f)


    assert f['rect']['width'] == 596 and f['rect']['height'] == 72 and f['rect']['x'] == 1052, (n, f['rect'])
    assert f['rect']['x'] * 2 + 596 == 1315 * 2 + 70, (n, f['rect'])
    assert 'gapList' not in f, n
    tx = nodes[n + '_Text']['fields']['rect']
    assert tx['x'] == 0 and tx['width'] >= 1052 + 596 and tx['y'] <= f['rect']['y'] and tx['y'] + tx['height'] >= f['rect']['y'] + 72, n
    lab = nodes[n + '_Label']['fields']
    assert lab['rect']['x'] + lab['rect']['width'] < f['rect']['x'] and lab['style'] == '$StyleSettingsText', n
assert not [n for n, v in nodes.items() if v.get('type') == 'ToggleButtonWidget' and re.match(r'TouchCustom_(lang|touch_swap_above|cam|dmg_num|streak|melee_splash|shrine_stacking|shrine_run_speed|resume_pos|voice)', n)]

order = [v for v, _, _ in layout._CAROUSEL_ORDER['lang']]
assert order == [1, 5, 2, 4, 6, 3, 0], order
assert cars['TouchCustom_lang']['fields']['states'] == [k for _, k, _ in layout._CAROUSEL_ORDER['lang']]
host_order = [int(x) for x in re.search(r'k_lang_order\[LANG_N\]\s*=\s*\{([^}]*)\}', host)[1].split(',')]
assert host_order == order, host_order
by_value = {'dmCustomLangZh': 0, 'dmCustomLangEnOpt': 1, 'dmCustomLangEsOpt': 2, 'dmCustomLangRuOpt': 3, 'dmCustomLangKoOpt': 4, 'dmCustomLangDeOpt': 5, 'dmCustomLangPortOpt': 6}
assert [by_value[k] for k in cars['TouchCustom_lang']['fields']['states']] == order
assert re.search(r'static int car_cjk\(int v\) \{ return v == 0 \|\| v == 4; \}', host), 'CJK = 中文 (0), 한국어 (4)'
for key in ('touch_swap_above', 'cam', 'dmg_num', 'streak', 'melee_splash', 'shrine_stacking', 'shrine_run_speed', 'resume_pos'):
    assert [v for v, _, _ in layout._CAROUSEL_ORDER[key]] == list(range(len(layout._CAROUSEL_ORDER[key]))), key
    assert cars['TouchCustom_' + key]['fields']['states'] == [k for _, k, _ in layout._CAROUSEL_ORDER[key]]
    hints = [h for _, _, h in layout._CAROUSEL_ORDER[key]]
    assert all('"%s"' % h in host for h in hints), (key, hints)
    assert nodes['TouchCustom_%s_Hint' % key]['fields']['style']['fontColor'] == '$PanelNoteTextColor'
assert cars['TouchCustom_shoulder_l']['fields']['states'] == ['dmShoulderAct%d' % k for k in range(15)]

std, cjk = nodes['TouchCustom_lang_FontStd']['fields'], nodes['TouchCustom_lang_FontCjk']['fields']
assert std['style'] == cjk['style'] == '$StyleSettingsCarousel' and 'style/fontFace' not in std and cjk['style/fontFace'] == 'BlizzardGlobal'
assert std['rect']['x'] > 10000 and cjk['rect']['x'] > 10000

for cap in ('Zh', 'En', 'Es', 'Ru', 'Ko', 'De', 'Pt'):
    h = nodes['TouchCustomLangHint' + cap]['fields']
    assert h['text'] == '@dmCustomLangHint' + cap and h['rect']['x'] > 10000 and h['style']['fontColor'] == '$PanelNoteTextColor', cap
    assert h.get('style/fontFace') == ('BlizzardGlobal' if cap in ('Zh', 'Ko') else None), cap
hint_names = re.findall(r'"(TouchCustomLangHint[A-Za-z]+)"', re.search(r'k_lang_hint\[LANG_N\].*?=\s*\{(.*?)\}', host, re.S)[1])
assert hint_names == ['TouchCustomLangHint' + c for c in ('Zh', 'En', 'Es', 'Ru', 'Ko', 'De', 'Pt')], hint_names
assert re.search(r'#define LANG_N 7\b', host), 'LANG_N must be 7'

rows = [n for n in nodes['TouchCustomRows']['children']]
assert not [r['name'] for r in rows if isinstance(r.get('fields', {}).get('style'), dict) and r['fields']['style'].get('fontColor') == '$FontColorGray'], 'grey notes'

table = {r['键']: r for r in csv.DictReader(open(root / 'i18n/ui_text.tsv', encoding='utf-8'), delimiter='\t')}
for c in cars.values():
    for k in c['fields']['states']: assert k in table and all(table[k][col] for col in ('中文', 'English', 'Español', 'Русский', '한국어', 'Deutsch')), k

refs = set(re.findall(r'"@(dm[A-Za-z0-9_]+)"', out))
assert refs and not refs - set(table), sorted(refs - set(table))
print('PASS touch layout language row and single-choice carousels: game fields, host order tables, font references, restart hints, note colour; %d Custom page keys all in the string table' % len(refs))
