#!/usr/bin/env python3
from pathlib import Path
import hashlib,json
root=Path(__file__).resolve().parents[1]






old=root/'docs/touch_layout_spec.json'
assert hashlib.sha256(old.read_bytes()).hexdigest()=='38f3fed9acc16276ce278ae1b05ea7dad979e19cac53c447c4b73f014c3cb606'
previous=root/'docs/touch_layout_spec_native_binding_v2.json'
assert hashlib.sha256(previous.read_bytes()).hexdigest()=='2209cb320184d9ed58cd9fbee72dac734333773ff3332882405dba889749b13b'
previous3=root/'docs/touch_layout_spec_native_binding_v3.json'
assert hashlib.sha256(previous3.read_bytes()).hexdigest()=='c2921d0ef1bba260c30134603d4c2cbee50235483fa4b261e62069ef5cf9e3bb'
previous4=root/'docs/touch_layout_spec_native_binding_v4.json'
assert hashlib.sha256(previous4.read_bytes()).hexdigest()=='048ee8bfffa348d6a6829feb394ce723ad6acf892447d051eb7073b863a32d2f'
new=root/'docs/touch_layout_spec_native_binding_v5.json'
data=json.loads(new.read_text());assert data['profile_id']=='touch_native_binding_v5'
assert hashlib.sha256(new.read_bytes()).hexdigest() in (root/'Sources/dm_touch_native_art.c').read_text()
print('PASS frozen v1/v2/v3/v4 spec hashes; v5 profile/spec hash matches compiled constant')
