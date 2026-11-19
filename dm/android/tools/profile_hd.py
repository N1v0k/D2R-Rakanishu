#!/usr/bin/env python3







import json
import re


FILES = {'ConsoleHD': 'global/ui/layouts/controller/_profileconsolehd.json',
         'ControllerHD': 'global/ui/layouts/controller/_profilehd.json',
         'HD': 'global/ui/layouts/_profilehd.json'}
CONSOLE, HD = 'ConsoleHD', 'HD'
CHAIN_MAX = 8
ALIAS_MAX = 8


def parse(text):

    text = re.sub(r'(?m)^((?:[^"\n/]|"(?:[^"\\\n]|\\.)*")*?)//.*$', r'\1', text)
    return json.loads(re.sub(r',(\s*[}\]])', r'\1', text))


def merge(base, over):

    if not isinstance(base, dict) or not isinstance(over, dict): return over
    out = dict(base)
    for k, v in over.items(): out[k] = merge(base[k], v) if k in base else v
    return out


def chain(read, name=CONSOLE):


    layers, seen = [], []
    while name is not None:
        if name not in FILES: raise ValueError('unknown base profile %r' % name)
        if name in seen or len(seen) >= CHAIN_MAX: raise ValueError('profile chain loops or is too deep: %s' % ' -> '.join(seen + [name]))
        seen.append(name)
        layer = parse(read(FILES[name]).decode('utf-8', 'replace'))
        if not isinstance(layer, dict): raise ValueError('profile %s is not an object' % name)
        layers.append(layer)
        name = layer.get('basedOn')
    out = {}
    for layer in reversed(layers): out = merge(out, layer)
    return out


def resolve(value, named, depth=0):

    while isinstance(value, str) and value.startswith('$'):
        if depth >= ALIAS_MAX: return None
        value, depth = named.get(value[1:]), depth + 1
    return value


def colour(value, named, depth=0):

    if isinstance(value, str) and value.startswith('$') and depth < 4: return colour(named.get(value[1:]), named, depth + 1)
    if isinstance(value, dict) and all(k in value for k in 'rgb'):
        v = [value['r'], value['g'], value['b'], value.get('a', 255)]
        return v if all(isinstance(x, int) and 0 <= x <= 255 for x in v) else None
    if isinstance(value, list) and len(value) == 4 and all(isinstance(x, (int, float)) and 0 <= x <= 1 for x in value): return [round(x * 255) for x in value]
    return None


def number(value, named):

    v = resolve(value, named)
    return v if isinstance(v, (int, float)) and not isinstance(v, bool) else None


def alias(value):

    return value[1:] if isinstance(value, str) and value.startswith('$') and len(value) > 1 else None
