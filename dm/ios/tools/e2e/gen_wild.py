#!/usr/bin/env python3








import sys
total = float(sys.argv[1]); t0 = float(sys.argv[2]) if len(sys.argv) > 2 else 18.0
print(f"{t0:.2f} down 0 @joy"); print(f"{t0 + 0.1:.2f} move 0 @joy+-40,0")
t = t0 + 0.5
while t < total - 1.0:
    print(f"{t:.2f} down 1 @main"); print(f"{t + 0.15:.2f} up 1"); t += 0.6
print(f"{total - 0.5:.2f} up 0")
