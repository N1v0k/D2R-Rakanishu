















import json, re, sys


def load_text(t):
    t = re.sub(r'//.*', '', t); t = re.sub(r',(\s*[}\]])', r'\1', t); return json.loads(t)


def diff(a, b):
    if type(a) != type(b): return {'value': b}
    if isinstance(a, dict):
        sub = {k: (diff(a[k], b[k]) if k in a else {'value': b[k]}) for k in b}
        if all(v is None for v in sub.values()) and list(a) == list(b): return None
        return {'dict': {k: v for k, v in sub.items() if v is not None}, 'order': list(b)}
    if isinstance(a, list):
        names = {x.get('name'): i for i, x in enumerate(a) if isinstance(x, dict) and x.get('name')}
        spec, used, changed = [], set(), len(a) != len(b)
        for j, y in enumerate(b):
            i = names.get(y.get('name')) if isinstance(y, dict) and y.get('name') else None
            if i is None and j < len(a) and j not in used and not (isinstance(a[j], dict) and a[j].get('name')): i = j
            if i is None or i in used: spec.append({'value': y}); changed = True; continue
            used.add(i); p = diff(a[i], y); spec.append({'base': i, 'patch': p}); changed |= p is not None or i != j
        return {'list': spec} if changed else None
    return None if a == b else {'value': b}


def apply(a, p):
    if p is None: return a
    if 'value' in p: return p['value']
    if 'dict' in p: return {k: apply(a.get(k), p['dict'].get(k)) if k in a else p['dict'][k]['value'] for k in p['order']}
    if 'list' in p: return [apply(a[s['base']], s['patch']) if 'base' in s else s['value'] for s in p['list']]
    raise ValueError('layout_patch：不认识的补丁 %r' % list(p))


if __name__ == '__main__' and sys.argv[1:2] == ['diff']:
    a = load_text(open(sys.argv[2], encoding='utf-8-sig').read()); b = load_text(open(sys.argv[3], encoding='utf-8-sig').read())
    p = diff(a, b); assert apply(a, p) == b and json.dumps(apply(a, p)) == json.dumps(b)
    json.dump(p, sys.stdout, ensure_ascii=False, indent=1); sys.stdout.write('\n')
