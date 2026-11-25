#!/bin/bash












set -u
OV=${1:?用法：art_compare.sh <overlay 目录> [包名]}; PKG=${2:-com.xiaoboluo.d2r}; ADB=${ADB:-adb}
HERE=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd); O=${DM_ROOT:-$HOME/dm-work}/build/android_art; rm -rf "$O" && mkdir -p "$O/ref" "$O/dev"
clang -fobjc-arc -O1 -o "$O/art_ref" "$HERE/art_ref.m" -framework Foundation -framework CoreGraphics -framework ImageIO -framework UniformTypeIdentifiers || exit 1
"$O/art_ref" "$OV/touchart/dm_mobile_socket.png" "$OV/hud_v4_ui/sil_dm_native_skill_0.png" "$O/ref" || exit 1
d=$($ADB shell wm density | awk '/Physical/{print $3}'); [ "$d" = 480 ] || { echo "🔴 手机密度 ${d}，不是 480（3 倍）"; exit 1; }
$ADB shell run-as "$PKG" rm -rf files/dm/artdump
$ADB shell am start -W -n "$PKG/com.xiaoboluo.d2r.DmActivity" --ez dmArtDump true > /dev/null || exit 1
for i in $(seq 1 20); do [ "$($ADB shell run-as "$PKG" ls files/dm/artdump 2> /dev/null | grep -c png)" -ge 15 ] && break; sleep 1; done
for f in $($ADB shell run-as "$PKG" ls files/dm/artdump | tr -d '\r'); do $ADB exec-out run-as "$PKG" cat "files/dm/artdump/$f" > "$O/dev/$f"; done
python3 - "$O/ref" "$O/dev" <<'EOF'
import os, sys
import numpy as np
from PIL import Image
ref, dev = sys.argv[1:3]; bad = 0; names = sorted(os.listdir(ref))
def pm(p):
    a = np.asarray(Image.open(p).convert('RGBA')).astype(np.float64); a[..., :3] *= a[..., 3:4] / 255; return a
print('%-16s %10s %8s %8s  %s' % ('控件', '尺寸', '平均差', '99分位', '判'))
for n in names:
    if not os.path.exists(os.path.join(dev, n)): print('%-16s 手机没出这张' % n); bad += 1; continue
    a, b = pm(os.path.join(ref, n)), pm(os.path.join(dev, n))
    if a.shape != b.shape: print('%-16s 尺寸不同 %s / %s' % (n, a.shape, b.shape)); bad += 1; continue
    d = np.abs(a - b); mean, p99 = d.mean(), np.percentile(d, 99); ok = mean <= 2 and p99 <= 32; bad += not ok
    print('%-16s %10s %8.2f %8.1f  %s' % (n[:-4], '%dx%d' % a.shape[1::-1], mean, p99, '✓' if ok else '✗'))
print('共 %d 张，不过 %d 张' % (len(names), bad)); sys.exit(1 if bad or not names else 0)
EOF
