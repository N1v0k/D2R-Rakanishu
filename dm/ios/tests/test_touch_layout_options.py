
import importlib
import json
import os
from pathlib import Path
import re
import sys

root = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(root / 'ios/tools'))
os.environ.pop('MOVIE_FFMPEG', None)
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

assert nodes['TouchCustom_touch_swap_above']['type'] == 'CarouselWidget' and 'TouchCustom_touch_swap_above_near' not in nodes
assert nodes['TouchCustom_touch_swap_above']['fields']['states'] == ['dmCustomTouchSwapNear', 'dmCustomTouchSwapFar']
assert nodes['TouchCustom_touch_swap_above_Text']['fields']['rect']['height'] == layout._CUSTOM_LINE_H





def find(node, name):
    if isinstance(node, dict):
        if node.get('name') == name: return node
        for child in node.values():
            r = find(child, name)
            if r: return r
    elif isinstance(node, list):
        for child in node:
            r = find(child, name)
            if r: return r
video = find(page, 'TouchVideoRows')
vnodes = {}
def vwalk(node):
    if isinstance(node, dict):
        if 'name' in node: vnodes[node['name']] = node
        for child in node.values(): vwalk(child)
    elif isinstance(node, list):
        for child in node: vwalk(child)
vwalk(video)
toggles = sorted(n for n, v in vnodes.items() if v.get('type') == 'ToggleButtonWidget')
assert toggles == sorted('TouchCustom_status_' + b for b in ('clock', 'battery', 'fps')), toggles
profile = {'focusIndicatorPadding': '$OptionsSliderFocusIndicatorPadding', 'focusIndicatorFilename': '$OptionsSliderFocusIndicatorFilename', 'isHorizontal': True,
           'indicatorFilepath': 'PauseMenu\\HorizontalIndicator', 'background': '$SettingsSliderBackground', 'leftButton': '$SettingsSliderLeftButtonFields',
           'rightButton': '$SettingsSliderRightButtonFields', 'valueText': '$SettingsSliderValueFields', 'increment': 1}
sliders = sorted(n for n, v in vnodes.items() if v.get('type') == 'SettingsSliderWidget')
assert sliders == sorted('TouchCustom_%s_Slider' % k for k in ('fps', 'res_pct', 's3d', 'q3d', 'bright')), sliders
for key in ('fps', 'res_pct', 's3d', 'q3d', 'bright'):
    sl = vnodes['TouchCustom_%s_Slider' % key]['fields']
    assert {k: sl[k] for k in profile} == profile, (key, sl)
    assert 'onUpdateMessage' not in sl and sl['rect']['width'] == 480 and sl['rect']['height'] == 72
    assert sl['rect']['x'] - 60 >= 0 and sl['rect']['x'] + 482 + 48 <= 1385
    for sfx in ('Text', 'Value'): assert 'TouchCustom_%s_%s' % (key, sfx) in vnodes, (key, sfx)
    for sfx in ('Label', 'Hint'): assert 'TouchVideo_%s_%s' % (key, sfx) in vnodes, (key, sfx)
    val = vnodes['TouchCustom_%s_Value' % key]['fields']
    assert val['rect']['x'] + val['rect']['width'] == sl['rect']['x'] - 89 and val['style'] == {'alignment': {'h': 'right', 'v': 'center'}}
    assert '"TouchCustom_%s_Value"' % key in out and '"style": "$StyleSettingsNumeric", "style": {' in out
    tx = vnodes['TouchCustom_%s_Text' % key]['fields']['rect']; r = sl['rect']
    assert tx['x'] <= r['x'] - 60 and tx['x'] + tx['width'] >= r['x'] + 482 + 48 - 1 and tx['y'] <= r['y'] and tx['y'] + tx['height'] >= r['y'] + r['height']
for key in ('fps', 'res_pct', 's3d'): assert 'TouchVideo_%s_Note' % key in vnodes
for key in ('fps', 's3d'): assert 'TouchVideo_%s_Warn' % key in vnodes
assert layout._VIDEO_H == layout._CUSTOM_TITLE_H + sum(layout._VIDEO_ROW_H[k] for k in layout._VIDEO_ORDER) and video['fields']['rect']['height'] == layout._VIDEO_H


order = list(layout._VIDEO_ORDER); assert order.index('s3d_preset') + 1 == order.index('s3d') and layout._VIDEO_PSEUDO == ('s3d_preset', 'driver') and order[-1] == 'driver'
q = vnodes['TouchCustom_s3d_preset']; qf = q['fields']
assert q['type'] == 'CarouselWidget' and 'onUpdateMessage' not in qf and 'gapList' not in qf and qf['fontType'] == '8ptE' and qf['enabledColor'] == '$CarouselEnabledColor'
assert qf['states'] == ['dmVideoS3qSave', 'dmVideoS3qBal', 'dmVideoS3qSharp', 'dmVideoS3qOwn'], qf['states']
assert qf['rect']['width'] == 596 and qf['rect']['height'] == 72 and qf['rect']['x'] + 596 == 1385, qf['rect']
assert qf['rect']['x'] <= vnodes['TouchCustom_s3d_Slider']['fields']['rect']['x'] - 60
qt = vnodes['TouchCustom_s3d_preset_Text']['fields']['rect']; ql = vnodes['TouchVideo_s3d_preset_Label']['fields']
assert qt['x'] == 0 and qt['width'] >= 1385 and qt['y'] <= qf['rect']['y'] and qt['y'] + qt['height'] >= qf['rect']['y'] + 72
assert ql['text'] == '@dmVideoS3q' and ql['rect']['y'] + ql['rect']['height'] <= qf['rect']['y']
assert vnodes['TouchVideo_s3d_preset_Hint']['fields']['style']['fontColor'] == '$PanelNoteTextColor' and vnodes['TouchVideo_s3d_preset_Hint']['fields']['rect']['y'] >= qf['rect']['y'] + 72
assert qt['y'] + layout._VIDEO_ROW_H['s3d_preset'] == vnodes['TouchCustom_s3d_Text']['fields']['rect']['y']
host_c = (root / 'host/dm_custom.c').read_text()
res_h = (root / 'host/dm_res_scale.h').read_text()
assert re.search(r'#define S3Q_N 4\b', host_c) and re.search(r'#define S3Q_OWN DM_RES_PRESET_N\b', host_c) and re.search(r'#define DM_RES_PRESET_N 3\b', res_h)
assert re.search(r'dm_res_presets\[DM_RES_PRESET_N\] = \{ \{ DM_RES_AUTO, 60 \}, \{ DM_RES_AUTO, 0 \}, \{ 100, 100 \} \}', res_h)
assert re.search(r'static const int v\[\] = \{ K_FPS, K_STATUS, K_RESP, ROW_S3Q, K_S3D, K_Q3D, K_BRIGHT, ROW_DRIVER \}', host_c)
assert '"TouchCustom_s3d_preset"' in host_c and all('"%sHint"' % k in host_c for k in qf['states'])


dv = vnodes['TouchCustomDriverOpen']['fields']
assert vnodes['TouchCustomDriverOpen']['type'] == 'ButtonWidget' and dv['onClickMessage'] == 'PanelManager:ClosePanel:TouchCustomDriverOpen'
assert dv['rect']['x'] == 1385 - 329 and dv['rect']['scale'] == 0.9 and dv['textString'] == '@dmCustomTechOpen'
dvv = vnodes['TouchCustom_driver_Value']['fields']
assert dvv['rect']['x'] == 0 and dvv['rect']['x'] + dvv['rect']['width'] <= dv['rect']['x'] - 20 and dvv['style'] == {'alignment': {'h': 'right', 'v': 'center'}}
assert vnodes['TouchVideo_driver_Label']['fields']['text'] == '@dmVideoDriver' and vnodes['TouchVideo_driver_Hint']['fields']['text'] == '@dmVideoDriverHint'
dft = vnodes['TouchCustom_driver_Text']['fields']['rect']
assert dft['x'] == 0 and dft['width'] >= 1385 and dft['y'] <= dv['rect']['y'] and dft['y'] + dft['height'] >= dv['rect']['y'] + 83
assert '#define VCAP_DRIVER 8' in host_c and '"TouchCustomDriverOpen"' in host_c and 'dm_driver_open()' in host_c and 'dm_driver_label(' in host_c
assert all(v['fields']['style']['fontColor'] in ('$PanelNoteTextColor', '$FontColorGoldYellow') for n, v in vnodes.items() if n.startswith('TouchVideo_') and n.endswith(('_Hint', '_Note', '_Warn')))

import csv
table = {r['键']: r for r in csv.DictReader(open(root / 'i18n/ui_text.tsv', encoding='utf-8'), delimiter='\t')}
for t in set(re.findall(r'"@(dm[A-Za-z0-9]+)"', out)) | {k for c in re.findall(r'"states": \[([^\]]*)\]', out) for k in re.findall(r'"(dm[A-Za-z0-9]+)"', c)}:
    assert t in table and all(table[t][c] for c in ('中文', 'English', 'Español', 'Русский', '한국어', 'Deutsch')), t
host_src = (root / 'host/dm_custom.c').read_text()
for t in set(re.findall(r'"(dmVideo[A-Za-z0-9]+|dmCustom(?:Q3d|Bright|Cam|DmgNum|TouchSwap|Voice|Lang)[A-Za-z0-9]*)"', host_src)):
    assert t in table, t



assert nodes['TouchCustom_dmg_num']['type'] == 'CarouselWidget' and 'TouchCustom_dmg_num_simple' not in nodes and 'TouchCustom_dmg_num_color' not in nodes
assert nodes['TouchCustom_dmg_num']['fields']['states'] == ['dmCustomDmgNum' + s for s in ('Off', 'Simple', 'Color')]
assert nodes['TouchCustom_dmg_num_Text']['fields']['rect']['height'] == layout._CUSTOM_LINE_H and nodes['TouchCustom_dmg_num_Hint']['fields']['text'] == ''
assert [key for key, _ in layout._CUSTOM_ALL[-8:]] == ['status_fx', 'combat_sim', 'fight_log', 'streak', 'melee_splash', 'shrine_stacking', 'shrine_run_speed', 'resume_pos']

assert nodes['TouchCustom_melee_splash']['type'] == 'CarouselWidget'
assert nodes['TouchCustom_melee_splash']['fields']['states'] == ['dmCustomSplashOff', 'dmCustomSplashOn']
assert nodes['TouchCustom_melee_splash_Hint']['fields']['text'] == ''

for k in ('shrine_stacking', 'shrine_run_speed', 'resume_pos'):
    assert nodes['TouchCustom_' + k]['type'] == 'CarouselWidget'
    assert nodes['TouchCustom_' + k + '_Hint']['fields']['text'] == ''

for key, text in (('build_hints', '@dmCustomBuildHints'), ('status_fx', '@dmCustomStatusFx'), ('combat_sim', '@dmCustomCombatSim'), ('fight_log', '@dmCustomFightLog')):
    assert nodes['TouchCustom_' + key]['type'] == 'ToggleButtonWidget'
    assert nodes['TouchCustom_' + key]['fields']['onClickMessage'].endswith('TouchCustom_' + key)
    assert nodes['TouchCustom_' + key + '_Text']['fields']['text'] == text
    assert nodes['TouchCustom_' + key + '_Text']['fields']['rect']['height'] == layout._CUSTOM_LINE_H

assert nodes['TouchCustom_item_cmp']['type'] == 'ToggleButtonWidget' and nodes['TouchCustom_item_cmp_Text']['fields']['text'] == '@dmCustomItemCmp'
host_src = (root / 'host/dm_custom.c').read_text()
def host_variant(voice):
    keep = re.sub(r'#if VOICE_ROW\n(.*?)#endif\n', (lambda m: m[1]) if voice else '', host_src, flags=re.S)
    return re.sub(r'(?m)^#.*\n', '', keep)
for voice in (False, True):
    if voice:
        os.environ['MOVIE_FFMPEG'] = '1'; layout = importlib.reload(layout)
    host = host_variant(voice)
    keys = re.findall(r'"([a-z_0-9]+)"', re.search(r'k_keys\[\].*?=\s*\{(.*?)\}', host, re.S)[1])


    other = {'eco_fps', 'res', 'shoulder_l', 'shoulder_r'} | set(layout._VIDEO_ORDER)
    assert [k for k in keys if k not in other] == [key for key, _ in layout._CUSTOM_ALL], (voice, keys)
    assert set(layout._VIDEO_ORDER) - set(layout._VIDEO_PSEUDO) <= set(keys) and not set(layout._VIDEO_PSEUDO) & set(keys)
    assert keys[-9:] == ['shoulder_l', 'shoulder_r', 'combat_sim', 'fight_log', 'streak', 'melee_splash', 'shrine_stacking', 'shrine_run_speed', 'resume_pos'], keys
    assert ('voice' in keys) == voice
    defaults = [{'FPS_AUTO': 1, 'DM_RES_AUTO': 1}.get(x.strip(), None) or int(x) for x in re.search(r'k_def\[NK\].*?=\s*\{(.*?)\}', host, re.S)[1].split(',')]
    assert len(defaults) == len(keys) and defaults[keys.index('touch_swap_above')] == 0
    assert defaults[keys.index('combat_sim')] == 1 and defaults[keys.index('fight_log')] == 1 and defaults[keys.index('streak')] == 2
    assert defaults[keys.index('bright')] == 2
    assert defaults[keys.index('melee_splash')] == 0
    assert defaults[keys.index('dmg_num')] == 2
    assert defaults[keys.index('build_hints')] == 1
    assert defaults[keys.index('res_pct')] == 1 and defaults[keys.index('fps')] == 1
    assert defaults[keys.index('shrine_stacking')] == 1 and defaults[keys.index('shrine_run_speed')] == 1 and defaults[keys.index('resume_pos')] == 1
    assert keys[-10] == 'status_fx' and defaults[keys.index('status_fx')] == 1
    assert keys.index('item_cmp') == keys.index('item_notes') + 1 and defaults[keys.index('item_cmp')] == 1
    assert defaults[keys.index('shoulder_l')] == 6 and defaults[keys.index('shoulder_r')] == 7
os.environ.pop('MOVIE_FFMPEG'); layout = importlib.reload(layout)
host = host_src


def custom_nodes():
    o, _ = layout._custom_page('"name": "VideoOptionsPanel", "name": "OptionsTable"')
    found = {}
    def w(node):
        if isinstance(node, dict):
            if 'name' in node: found[node['name']] = node
            for child in node.values(): w(child)
        elif isinstance(node, list):
            for child in node: w(child)
    w(json.loads(re.sub(r',\s*([}\]])', r'\1', o)))
    return found
assert 'TouchCustom_icloud' in nodes and 'TouchCustom_icloud_Text' in nodes and not layout._CUSTOM_HIDDEN
ios_all, ios_h, ios_tech = layout._CUSTOM_ALL, layout._CUSTOM_H, layout._TECH_Y
ios_y = {k: nodes['TouchCustom_%s_Text' % k]['fields']['rect']['y'] for k in ('item_cmp', 'icloud', 'pot_pick', 'status_fx')}
os.environ['DM_OVERLAY_NO_ICLOUD'] = '1'; layout = importlib.reload(layout)
try:
    an = custom_nodes()
    assert not [n for n in an if 'icloud' in n.lower()], [n for n in an if 'icloud' in n.lower()]
    assert layout._CUSTOM_ALL == ios_all
    y = lambda k: an['TouchCustom_%s_Text' % k]['fields']['rect']['y']
    assert y('item_cmp') == ios_y['item_cmp'] and y('pot_pick') == ios_y['icloud'] and y('status_fx') == ios_y['status_fx'] - layout._CUSTOM_LINE_H
    assert layout._TECH_Y == ios_tech - layout._CUSTOM_LINE_H and an['TouchCustomTech_Title']['fields']['rect']['y'] == layout._TECH_Y
    assert layout._CUSTOM_H == ios_h - layout._CUSTOM_LINE_H and an['TouchCustomRows']['fields']['rect']['height'] == layout._CUSTOM_H
    tops = [y(k) for k, _ in layout._CUSTOM_ALL if k != 'icloud']
    assert all(b - a == layout._CUSTOM_LINE_H for a, b in zip(tops, tops[1:])), tops
finally:
    os.environ.pop('DM_OVERLAY_NO_ICLOUD'); layout = importlib.reload(layout)
assert not layout._CUSTOM_HIDDEN and layout._CUSTOM_H == ios_h and 'TouchCustom_icloud' in custom_nodes()
pack_src = (root / 'tools/d2rm_android.py').read_text()
overlay_call = [l for l in pack_src.splitlines() if 'gen_romfs_overlay.py' in l and l.lstrip().startswith('run(')]
assert len(overlay_call) == 1 and "{'DM_OVERLAY_NO_ICLOUD': '1'} if T == 'android' else {}" in overlay_call[0], overlay_call
assert 'TouchCustom_touch_swap_above_near' not in host and 'k_swap_box' not in host
print('PASS touch layout options: iCloud row left out on Android, skill-set switch carousel, Video page sliders (game slider fields, value text, frames, notes, no single-choice checkboxes), damage number carousel, build guide hints and status effects switches, labels, defaults, host order (default and MOVIE_FFMPEG=1) and scrolling row extent')
