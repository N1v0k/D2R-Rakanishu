
















import re, subprocess, sys

objdump, lib = sys.argv[1], sys.argv[2]
newer = re.compile(r'(cas(p)?(a|al|l)?(b|h)?|swp(a|al|l)?(b|h)?|ld(add|clr|eor|set|smax|smin|umax|umin)(a|al|l)?(b|h)?'
                   r'|st(add|clr|eor|set|smax|smin|umax|umin)(l)?(b|h)?|ldapr(b|h)?|ldapur(b|h|sb|sh|sw)?|stlur(b|h)?|sdot|udot)')
RUNTIME_SELECTED = ('adler32_arm_neon_dotprod',)
func, hits, count = '?', [], 0
proc = subprocess.Popen([objdump, '-d', '--no-show-raw-insn', lib], stdout=subprocess.PIPE, text=True, errors='replace')
for line in proc.stdout:
    m = re.match(r'[0-9a-f]+ <(.+)>:$', line)
    if m: func = m.group(1); continue
    m = re.match(r'\s*([0-9a-f]+):\s+(\S+)', line)
    if m and newer.fullmatch(m.group(2)) and not func.startswith('__aarch64_') and func not in RUNTIME_SELECTED:
        count += 1
        if len(hits) < 20: hits.append('  %s  %s  %s' % (m.group(1), func, line.split(':', 1)[1].strip()))
if proc.wait() != 0: sys.exit('check_armv80.py: %s failed on %s' % (objdump, lib))
if count:
    sys.exit('check_armv80.py: %s has %d instruction(s) newer than ARMv8.0, which crash Snapdragon 680-class phones (#293):\n%s'
             % (lib, count, '\n'.join(hits)))
print('check_armv80.py: %s has no instruction newer than ARMv8.0' % lib)
