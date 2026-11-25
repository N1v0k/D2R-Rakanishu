






















import re, sys

src, dst = sys.argv[1], sys.argv[2]
text = open(src, encoding='utf-8').read()
locals_ = set(re.findall(r'^\s*(L\w+):', text, re.M))
out = []
for line in text.split('\n'):
    code, sep, comment = line.partition('//')
    code = re.sub(r'(\w+)@PAGEOFF\b', r':lo12:\1', code)
    code = re.sub(r'(\w+)@PAGE\b', r'\1', code)
    code = re.sub(r'(?<![\w.$])_(\w+)', r'\1', code)
    if locals_: code = re.sub(r'(?<![\w.])(L\w+)\b', lambda m: '.' + m.group(1) if m.group(1) in locals_ else m.group(1), code)
    code = re.sub(r'^(\s*)\.cstring\b', r'\1.section .rodata.str1.1,"aMS",@progbits,1', code)
    code = re.sub(r'^(\s*)\.private_extern\b', r'\1.hidden', code)
    if re.search(r'\bstadd\b', code):
        m = re.fullmatch(r'(\s*)stadd\s+(x\d+)\s*,\s*\[\s*(x\d+)\s*\]\s*', code)



        if not m or {m.group(2), m.group(3)} & {'x14', 'x15'}:
            sys.exit('asm_elf.py: %s: cannot rewrite "%s" for ARMv8.0' % (src, line.strip()))
        ind, xs, xn = m.groups()
        out += ['%s1: ldxr x14, [%s]%s%s' % (ind, xn, sep, comment), '%sadd x14, x14, %s' % (ind, xs),
                '%sstxr w15, x14, [%s]' % (ind, xn), '%scbnz w15, 1b' % ind]
        continue
    out.append(code + sep + comment)
out.append('    .section .note.GNU-stack,"",@progbits')
open(dst, 'w', encoding='utf-8', newline='\n').write('\n'.join(out) + '\n')
