#!/bin/bash










set -u
cd "$(dirname "$0")/.."
N=300; case "${1:-}" in [0-9]*) N=$1; shift ;; esac
OUT=${DM_OUT:-${DM_ROOT:-$HOME/dm-work}/build/cache/dm/out/3.3.94012-r10}
C=${DM_ROOT:-$HOME/dm-work}/run/ios/cmp; rm -rf "$C"; mkdir -p "$C"
source tools/local_env.sh && local_env_need DM_DEVICE || exit 1
D=$DM_DEVICE; B=${DM_BUNDLE_ID:?先在本机配置里设 DM_BUNDLE_ID（App 包名，见 dm/ios/local.env.example）}
SAVE_REF=${DM_ROOT:-$HOME/dm-work}/data/switch/save_ref/0000000000000001/0
t(){ echo "[$(date +%T)] $*"; }



xcrun devicectl device info files --device $D --domain-type appDataContainer --domain-identifier $B --subdirectory Documents/romfs/data --json-output "$C/romfs_ls.json" > /dev/null 2>&1
nf=$(python3 -c "import json; d=json.load(open('$C/romfs_ls.json')); print(sum(not f['resources']['isDirectory'] for f in d['result']['files']))" 2>/dev/null)
[ "${nf:-0}" = 46 ] || { t "🔴 真机 Documents/romfs/data 文件数 ${nf:-0} ≠ 46：RomFS 不全，先推送（见 dm/ios/README.md 推送一节）"; exit 1; }
make -s -C ../host || { t "🔴 Mac 宿主编译失败"; exit 1; }




for k in a b; do
  POL=""; [ $k = b ] && POL="taskpolicy -c background"
  rm -rf "$C/save_$k"; cp -R "$SAVE_REF" "$C/save_$k" && chmod -R u+w "$C/save_$k" || { t "🔴 拷标准存档失败"; exit 1; }
  DM_SAVE="$C/save_$k" DM_NVN_SHOT="$N:$C/mac_$k.png" DM_WATCH=120 $POL ../host/dmhost "$OUT" main sdk > "$C/mac_$k.log" 2>&1 & pid=$!
  s=$SECONDS
  until grep -q "截图.*$C/mac_$k.png" "$C/mac_$k.log" 2>/dev/null; do
    kill -0 $pid 2>/dev/null || { t "🔴 Mac 第 $k 次在截到第 $N 帧前退出"; tail -3 "$C/mac_$k.log"; exit 1; }
    [ $((SECONDS - s)) -gt 120 ] && { kill $pid; t "🔴 Mac 第 $k 次 120 秒未到第 $N 帧"; exit 1; }
    sleep 1
  done
  kill $pid 2>/dev/null; wait $pid 2>/dev/null
  t "Mac 第 $k 次截到（$((SECONDS - s)) 秒）"
done

./run.sh build > "$C/ios_build.log" 2>&1 || { t "🔴 真机构建失败"; tail -5 "$C/ios_build.log"; exit 1; }
./run.sh install >> "$C/ios_build.log" 2>&1 || { t "🔴 真机装机失败"; tail -5 "$C/ios_build.log"; exit 1; }


DEVICECTL_CHILD_DM_SAVE_FRESH=1 DEVICECTL_CHILD_DM_NVN_SHOT="$N:cmp_$N.png" DEVICECTL_CHILD_DM_SCREEN_SHOT="$((N / 30 + 3)):screen.png" DEVICECTL_CHILD_DM_WATCH=${DM_IOS_WATCH:-45} ./run.sh boot 120 > "$C/ios_run.log" 2>&1
cp "${DM_ROOT:-$HOME/dm-work}/run/ios/out/boot.txt" "$C/ios.log" 2>/dev/null
grep -q "截图.*cmp_$N.png 成功" "$C/ios.log" || { t "🔴 真机没截到第 $N 帧（看 $C/ios.log；可加大 DM_IOS_WATCH）"; exit 1; }
xcrun devicectl device copy from --device $D --domain-type appDataContainer --domain-identifier $B \
  --source "Documents/cmp_$N.png" --destination "$C/ios.png" > /dev/null 2>&1 || { t "🔴 拉回真机截图失败"; exit 1; }
t "真机截到并拉回"


for f in screen.png screen.png.txt; do
  xcrun devicectl device copy from --device $D --domain-type appDataContainer --domain-identifier $B --source "Documents/$f" --destination "$C/$f" > /dev/null 2>&1 || { t "🔴 拉回屏幕级截图 $f 失败"; exit 1; }
done
python3 tools/screen_check.py "$C/screen.png" "$C/screen.png.txt" "$C/ios.log" --want "${DM_SCREEN_WANT:-full}"; SC=$?

python3 tools/frame_cmp.py "$C/mac_a.png" "$C/mac_b.png" "$C/ios.png" --mask-out "$C/mask.png" "$@"; FC=$?
exit $(( FC || SC ))
