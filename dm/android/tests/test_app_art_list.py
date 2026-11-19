#!/usr/bin/env python3





import importlib.util
import pathlib
import re
import sys

dm = pathlib.Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('gen_app_art', dm / 'android/tools/gen_app_art.py')
art = importlib.util.module_from_spec(spec); spec.loader.exec_module(art)
java = (dm / 'android/app/src/main/java/com/xiaoboluo/d2r/AppArt.java').read_text(encoding='utf-8')
def arr(name):
    return re.findall(r'"([^"]+)"', re.search(r'static final String\[\] %s = \{(.*?)\};' % name, java, re.S)[1])
images = [i + '.png' for i in arr('VERIFY_IMAGES')]
fonts = ['fonts/' + f for f in arr('VERIFY_FONTS')]
assert sorted(images + fonts + ['ui_colors.txt']) == sorted(art.VERIFY_FILES), (sorted(set(images + fonts + ['ui_colors.txt']) ^ set(art.VERIFY_FILES)))
assert arr('UI_COLOURS') == list(art.UI_COLOURS), (arr('UI_COLOURS'), art.UI_COLOURS)
assert set(arr('VERIFY_FONTS')) == set(art.ROLE_FONTS)
pack = (dm / 'tools/d2rm_android.py').read_text(encoding='utf-8')
act = (dm / 'android/app/src/main/java/com/xiaoboluo/d2r/DmActivity.java').read_text(encoding='utf-8')
assert "CAPS=%s" in pack and "caps.append('quickmenu1')" in pack and 'gen_app_art.missing' in pack and '"quickmenu1"' in act

assert 'arrow_%s.png' not in (dm / 'android/tools/gen_app_art.py').read_text(encoding='utf-8')
print('PASS app art list: gen_app_art VERIFY_FILES == AppArt VERIFY_IMAGES + VERIFY_FONTS + ui_colors.txt, colour keys, CAPS mark')
