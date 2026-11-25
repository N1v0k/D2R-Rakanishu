

from pathlib import Path
import re
root=Path(__file__).resolve().parents[4]
for name in ['gfx/vulkan/engine/xbg_game.cpp','gfx/nvn/hle_nvn.c','android/vulkan_present.cpp']:
    s=(root/name).read_text(encoding='utf-8')
    s=re.sub(r"""/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])*'""", "", s,flags=re.S)
    stack=[]
    for ch in s:
        if ch in '({[':stack.append(ch)
        elif ch in ')}]':assert stack and stack.pop()=={')':'(',']':'[','}':'{'}[ch],name
    assert not stack,name
print('PASS: C/C++ balanced delimiters for engine, NVN and Android presenter (not a compile)')
