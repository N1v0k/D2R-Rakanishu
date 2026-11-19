#!/usr/bin/env python3











import pathlib
import re
import sys

PROTO = re.compile(r'VKAPI_ATTR\s+([^;{}]+?)\s+VKAPI_CALL\s+(vk\w+)\s*\(([^;]*?)\)\s*;', re.S)

GLOBAL = {"vkCreateInstance", "vkEnumerateInstanceExtensionProperties", "vkEnumerateInstanceLayerProperties", "vkEnumerateInstanceVersion"}
SPECIAL = {"vkGetInstanceProcAddr"}


def prototypes(text):
    out = []
    for m in PROTO.finditer(text):
        ret, name, params = ' '.join(m.group(1).split()), m.group(2), ' '.join(m.group(3).split())
        out.append((ret, name, params))
    return out


def param_names(params):
    if params in ('', 'void'):
        return []
    names = []
    for p in params.split(','):
        p = re.sub(r'\[[^\]]*\]\s*$', '', p.strip())
        m = re.search(r'(\w+)\s*$', p)
        if not m:
            raise ValueError(f'cannot name parameter {p!r}')
        names.append(m.group(1))
    return names


def build_table(headers):
    seen, funcs = set(), []
    for h in headers:
        for ret, name, params in prototypes(h.read_text(encoding='utf-8')):
            if name in seen:
                continue
            seen.add(name)
            funcs.append((ret, name, params))
    lines = ['// 生成文件，勿手改：dm/android/tools/gen_vk_shim.py | written by dm/android/tools/gen_vk_shim.py, do not edit',
             '#define VK_USE_PLATFORM_ANDROID_KHR', '#include <vulkan/vulkan.h>', '#include "vk_loader.h"', '']
    for ret, name, params in funcs:
        args = ', '.join(param_names(params))
        lines.append(f'VKAPI_ATTR {ret} VKAPI_CALL {name}({params}) {{')
        if name in SPECIAL:
            lines.append(f'    return dm_vk_loader_gipa({args});')
        else:
            glob = 1 if name in GLOBAL else 0
            lines.append(f'    static dm_vk_slot s = {{"{name}", {glob}, 0, 0}};')
            call = f'((PFN_{name})dm_vk_resolve(&s))({args})'
            if name == 'vkCreateInstance':
                lines += [f'    VkResult r = {call};', '    if (r == VK_SUCCESS) dm_vk_loader_instance(*pInstance);', '    return r;']
            elif name == 'vkDestroyInstance':
                lines += [f'    {call};', '    dm_vk_loader_instance(VK_NULL_HANDLE);']
            elif ret == 'void':
                lines.append(f'    {call};')
            else:
                lines.append(f'    return {call};')
        lines.append('}')
    lines.append(f'const unsigned dm_vk_shim_count = {len(funcs)};')
    return '\n'.join(lines) + '\n', funcs


def main(argv):
    if len(argv) != 3:
        print(__doc__, file=sys.stderr)
        return 2
    base = pathlib.Path(argv[1])
    text, funcs = build_table([base / 'vulkan_core.h', base / 'vulkan_android.h'])
    if len(funcs) < 100 or 'vkCreateInstance' not in {f[1] for f in funcs}:
        print(f'gen_vk_shim: only {len(funcs)} prototypes found in {base}', file=sys.stderr)
        return 1
    out = pathlib.Path(argv[2])
    if not out.exists() or out.read_text(encoding='utf-8') != text:
        out.write_text(text, encoding='utf-8')
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
