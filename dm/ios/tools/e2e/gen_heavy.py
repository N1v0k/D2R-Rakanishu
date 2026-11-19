#!/usr/bin/env python3








import sys
total = float(sys.argv[1]); t0 = float(sys.argv[2]) if len(sys.argv) > 2 else 17.0
ev = [(t0, "down 0 @joy"), (t0 + 0.1, "move 0 @joy+40,0")]
t = t0 + 2.0; left = True
while t < total - 1.0:
    ev.append((t, f"move 0 @joy+{-40 if left else 40},0")); left = not left; t += 4.0
ev.append((total - 0.5, "up 0"))
t = t0 + 2.2
while t < total - 1.0:
    ev.append((t, "down 1 @main")); ev.append((t + 0.15, "up 1")); t += 0.5
for t, s in sorted(ev, key=lambda e: e[0]): print(f"{t:.2f} {s}")
