





import os, sys
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '../../../tools'))
DM_ROOT = os.environ.get('DM_ROOT') or os.path.expanduser('~/dm-work')
os.environ.setdefault('DMX_ROOT', os.path.join(DM_ROOT, 'data/switch', open(os.path.join(os.path.dirname(os.path.abspath(__file__)), '../../../PRODUCT'), encoding='utf-8').readline().split('-')[0], 'Update/RomFS/data'))
import dmx
PC = os.path.join(DM_ROOT, 'data/assets-src/raw')
SAMPLES = [
    ('hd/vfx/textures/gradient/T_FX_perlin_wipe_03.texture', 'RGBA8 256² 同分辨率'),
    ('hd/vfx/textures/gradient/T_FX_portal_gradient.texture', 'RGBA8 256×16 非方形 同分辨率'),
    ('hd/env/texture/terrain/path/066_PATH.texture', 'BC4 32² 同分辨率'),
    ('hd/debug/wasted_mip_size_2048.texture', 'BC3 sRGB 2048² 12 级 同分辨率'),
    ('hd/items/armor/armor/plate_mail/textures/armor_plate_mail_alb.texture', 'BC3 sRGB 256²（PC 1024²）降分辨率'),
    ('hd/env/texture/global/flow/act4/lava/lava_lavaNS_FLOW.texture', 'BC1 64²（PC 256²）降分辨率'),
    ('hd/character/enemy/fingermage1/Textures/fingermage_KTINT.texture', 'BC3 256²（PC 1024²）降分辨率'),
]
out = sys.argv[1]
os.makedirs(f'{out}/dm', exist_ok=True); os.makedirs(f'{out}/pc', exist_ok=True)
with open(f'{out}/list.tsv', 'w') as f:
    for i, (p, why) in enumerate(SAMPLES):
        open(f'{out}/dm/{i}.texture', 'wb').write(dmx.get('data:data/' + p))
        open(f'{out}/pc/{i}.texture', 'wb').write(open(os.path.join(PC, p), 'rb').read())
        f.write(f'{i}\t{p}\t{why}\n')
print(len(SAMPLES), '对样本 →', out)
