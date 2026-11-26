#!/usr/bin/env python3

import argparse,hashlib,json,re
from pathlib import Path
from PIL import Image
import os; DM_ROOT = os.environ.get('DM_ROOT') or os.path.expanduser('~/dm-work')
IOS=Path(__file__).resolve().parent.parent


GENERATED=Path(os.environ.get('DM_NATIVE_ART_DIR', str(IOS/'Sources')))
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
try:






 manifest=json.loads((IOS/'Sources/TouchArt/native_art_manifest.json').read_text())
 compiled=(GENERATED/'dm_touch_native_art.c').read_text(encoding='utf-8')
 for name,target in [('dm_ta_manifest_sha',IOS/'Sources/TouchArt/native_art_manifest.json'),
                     ('dm_ta_geometry_sha',IOS/'docs/native_art_v1_geometry.json'),
                     ('dm_ta_binding_sha',IOS/'docs/touch_layout_spec_native_binding_v5.json')]:
  values=re.findall(r'\bconst\s+char\s+'+name+r'\s*\[\s*\]\s*=\s*"([^"\n]*)"\s*;',compiled)
  assert len(values)==1, f'{name} declaration must occur exactly once'
  assert values[0]==sha(target), f'{name} differs from actual file SHA256'
 assert manifest['geometry_sha256']==sha(IOS/'docs/native_art_v1_geometry.json')
 assert manifest['binding_protocol_sha256']==sha(IOS/'docs/touch_layout_spec_native_binding_v5.json')
 for a in manifest['assets']:
  with Image.open(GENERATED/'TouchArt'/a['asset']) as image:
   rgba=image.convert('RGBA')
   assert list(rgba.size)==a['atlas_px'], a['asset']+' dimensions'
   assert hashlib.sha256(rgba.tobytes()).hexdigest()==a['decoded_rgba_sha256'], a['asset']+' RGBA pixels'
  assert a['pixel_format_code']==61 and len(a['official_sprite_md5'])==32 and len(a['official_sprite_sha256'])==64
 import os,sys
 romfs=os.environ.get('DM_ROMFS_SRC') or os.path.join(DM_ROOT, 'data/switch', open(os.path.join(os.path.dirname(os.path.abspath(__file__)), '../../PRODUCT'), encoding='utf-8').readline().split('-')[0], 'Update/RomFS')
 if os.path.isfile(os.path.join(romfs,'data','build_manifest.psv')):
  sys.path.insert(0,str(IOS/'tools')); import romfs_art
  for table in manifest['official_tables']:
   raw=romfs_art.read(romfs,'global/excel/'+table['file'])
   assert hashlib.sha256(raw).hexdigest()==table['sha256']==table['official']['sha256']
   assert hashlib.md5(raw).hexdigest()==table['official']['ckey_md5']
 else: print('（这台机器没有游戏文件，跳过两张技能表的哈希核对）')
 assert len(manifest['assets'])==12
 print('PASS native_art_capture: 3/3 real C hash constants match actual files; geometry+binding hashes distinct; 12/12 dimensions and RGBA pixel hashes; 2/2 frozen official tables SHA+MD5')
except AssertionError as e:
 print(f'FAIL: {e}');raise SystemExit(1)
