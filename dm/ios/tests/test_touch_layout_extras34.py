




import json
import os
from pathlib import Path
import re
import sys

root = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(root / 'ios/tools'))
os.environ.pop('MOVIE_FFMPEG', None)
import touch_layout as layout
import dm_i18n

out, _ = layout._custom_page('"name": "VideoOptionsPanel", "name": "OptionsTable"')
page = json.loads(re.sub(r',\s*([}\]])', r'\1', out))
nodes, order = {}, []
def walk(node):
    if isinstance(node, dict):
        if 'name' in node:
            assert node['name'] not in nodes, node['name']
            nodes[node['name']] = node; order.append(node['name'])
        for child in node.values(): walk(child)
    elif isinstance(node, list):
        for child in node: walk(child)
walk(page)
host = (root / 'host/dm_custom.c').read_text()
names = []
for arr in ('k_tech_names', 'k_sh_names', 'k_tech_box'):
    body = re.search(r'%s\[[^\]]*\]\s*=\s*\{(.*?)\};' % arr, host, re.S)[1]
    names += re.findall(r'"([A-Za-z0-9_]+)"', body)
for n in names: assert n in nodes, n
rect = lambda n: nodes[n]['fields']['rect']

rows = nodes['TouchCustomRows']; kids = [c['name'] for c in rows['children']]
assert kids.index('TouchCustomTech_Title') > kids.index('TouchCustom_build_hints') and kids.index('TouchCustomShoulder_Title') > kids.index('TouchCustomTechOpen')
assert rect('TouchCustomTech_Title')['y'] >= max(rect(k)['y'] + rect(k).get('height', 0) for k in kids if k.startswith('TouchCustom_build_hints'))
assert rect('TouchCustomShoulder_Title')['y'] > rect('TouchCustomTechOpen')['y']

bottom = max(rect(k)['y'] + rect(k).get('height', 0) for k in kids if 'height' in rect(k))
assert rows['fields']['rect']['height'] == layout._CUSTOM_H >= bottom, (rows['fields']['rect']['height'], bottom)

for side in 'lr':
    f = nodes['TouchCustom_shoulder_' + side]
    assert f['type'] == 'CarouselWidget' and f['fields']['states'] == ['dmShoulderAct%d' % k for k in range(15)] and 'gapList' not in f['fields']
    for gone in ('prev', 'next', 'Value'): assert 'TouchCustom_shoulder_%s_%s' % (side, gone) not in nodes and 'TouchCustom_shoulder_%s_%s' % (side, gone) not in host
assert re.search(r'#define SH_N 15\b', host) and layout._SH_N == 15
o = nodes['TouchCustomTechOpen']
assert o['type'] == 'ButtonWidget' and o['fields']['filename'] == 'FrontEnd\\HD\\Final\\FrontEnd_ButtonSmall' and o['fields']['onClickMessage'] == 'PanelManager:ClosePanel:TouchCustomTechOpen'
assert abs(o['fields']['rect']['x'] + 366 * o['fields']['rect']['scale'] - 1385) <= 1

keys = {r[0] for r in dm_i18n.rows()}
for n in ('TouchCustomTech_Title', 'TouchCustom_tech_Label', 'TouchCustomShoulder_Title', 'TouchCustom_shoulder_l_Label', 'TouchCustom_shoulder_r_Label', 'TouchCustomShoulder_Hint'):
    t = nodes[n]['fields']['text']; assert t.startswith('@') and t[1:] in keys, (n, t)
assert o['fields']['textString'][1:] in keys
for k in ['dmCustomTechState', 'dmCustomTechDownload'] + ['dmShoulderAct%d' % i for i in range(15)]: assert k in keys, k
print('PASS touch layout extras: tech and shoulder sections, host widget names, shoulder carousels, open button, container height, texts')
