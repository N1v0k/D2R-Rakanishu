#!/usr/bin/env python3
"""生成电视专用覆盖层；iOS 生成器及产物不变。
Generate a controller-only TV overlay without changing the iOS generator/output.
Usage: DM_ASTC_PACK=... python3 dm/tvos/gen_romfs_overlay.py ROMFS OUTPUT
Generate a controller-only TV overlay without changing the iOS generator/output. Usage: DM_ASTC_PACK=... python3 dm/tvos/gen_romfs_overlay.py ROMFS OUTPUT '''
"""
import runpy
import sys
from pathlib import Path


def main():
    if len(sys.argv) != 3:
        raise SystemExit(__doc__)
    tools = Path(__file__).resolve().parents[1] / 'ios' / 'tools'
    sys.path.insert(0, str(tools))
    import touch_layout

    original_profile = touch_layout.build_profile
    original_custom = touch_layout.pad_custom_page

    def tv_profile(romfs, outdir):
        replacements = original_profile(romfs, outdir)



        old = '"OptionsDividerRect": { "x": 0, "y": 150 },'
        for filename in replacements.values():
            path = Path(filename)
            text = path.read_text(encoding='utf-8')
            if text.count(old) != 1:
                raise SystemExit('Unexpected options divider profile; refusing to patch')
            path.write_text(text.replace(old, '"OptionsDividerRect": { "x": 0, "y": 90 },'), encoding='utf-8')
        return replacements

    def tv_custom(text):
        text = original_custom(text)



        old = '"rect": "$OptionsDividerRect"'
        if '"rowHeight": 180' not in text or old not in text:
            raise SystemExit('Unexpected custom page geometry; refusing to patch')
        return text.replace(old, '"rect": { "x": 0, "y": 150 }')

    touch_layout.build_profile = tv_profile
    touch_layout.pad_custom_page = tv_custom
    try:
        runpy.run_path(str(tools / 'gen_romfs_overlay.py'), run_name='__main__')
    finally:
        touch_layout.build_profile = original_profile
        touch_layout.pad_custom_page = original_custom


if __name__ == '__main__':
    main()
