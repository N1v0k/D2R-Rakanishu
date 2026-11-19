#!/bin/bash










set -u
cd "$(dirname "$0")/.."
T=${1:?mac 或 ios}; OUT=${DM_OUT:-${DM_ROOT:-$HOME/dm-work}/build/cache/dm/out/3.3.94012-r10}
P=${DM_ROOT:-$HOME/dm-work}/run/ios/pages; mkdir -p "$P"; TR=${DM_ROOT:-$HOME/dm-work}/run/verify/truth_shots
source tools/local_env.sh && local_env_need DM_DEVICE || exit 1
D=$DM_DEVICE; B=${DM_BUNDLE_ID:?先在本机配置里设 DM_BUNDLE_ID（App 包名，见 dm/ios/local.env.example）}
SAVE_REF=${DM_ROOT:-$HOME/dm-work}/data/switch/save_ref/0000000000000001/0
KEYS_CLS='PLUS@OpenPanel:MainMenuButtonRibbon>OpenPanel:OnlinePlayModeModal,DOWN@OnlinePlayModeModal,A@ControllerActionEnd>CharacterCreatePanel'
KEYS_GAME='X@OpenPanel:MainMenuButtonRibbon>OpenPanel:CharacterSelectPanel,MSG:CharacterSelect:LoadCharacter:0@OpenPanel:CharacterSelectPanel%1>FrontEndNavigation:ToGame'
keys(){ case $1 in cls) echo "$KEYS_CLS" ;; game) echo "$KEYS_GAME" ;; esac; }
frame(){ case $1 in game) echo 900 ;; *) echo 300 ;; esac; }
t(){ echo "[$(date +%T)] $*"; }
truth(){ case $1 in sel) echo "$TR/选角页_saveref_joe_有DLC.png" ;; cls) echo "$TR/职业选择页_有DLC.png" ;; game) echo "${DM_ROOT:-$HOME/dm-work}/run/verify/dm/pq/p0.png" ;; esac; }
side(){
  python3 - "$1" "$2" "$3" <<'PY'
import sys; from PIL import Image, ImageDraw
a, b = (Image.open(p).convert('RGB').resize((1600, 900)) for p in sys.argv[1:3])
o = Image.new('RGB', (3216, 940), (40, 40, 40)); o.paste(a, (0, 40)); o.paste(b, (1616, 40))
import numpy as np
la, lb = (float(np.asarray(x).mean()) for x in (a, b))
d = ImageDraw.Draw(o); d.text((10, 10), f'OURS  mean {la:.1f}  ' + sys.argv[1].split('/')[-1], fill=(255, 255, 0)); d.text((1626, 10), f'TRUTH (Switch)  mean {lb:.1f}  ' + sys.argv[2].split('/')[-1], fill=(0, 255, 255))
o.save(sys.argv[3]); print(f'平均亮度 我方 {la:.1f} / 真值 {lb:.1f}')
PY
}
rc=0; RUN=$(date +%H%M%S)
[ $T = mac ] && { make -s -C ../host || { t "🔴 Mac 宿主编译失败"; exit 1; }; }
[ $T = ios ] && { ./run.sh build > "$P/ios_build.log" 2>&1 || { t "🔴 真机构建失败"; tail -5 "$P/ios_build.log"; exit 1; }; ./run.sh install >> "$P/ios_build.log" 2>&1 || { t "🔴 真机装机失败"; tail -5 "$P/ios_build.log"; exit 1; }; }
for pg in ${PAGES:-sel cls game}; do
  unset DM_KEYS DEVICECTL_CHILD_DM_KEYS
  if [ $T = mac ]; then
    rm -f "$P/mac_$pg.png"; rm -rf "$P/save_$pg"; cp -R "$SAVE_REF" "$P/save_$pg" && chmod -R u+w "$P/save_$pg"
    [ $pg != sel ] && export DM_KEYS="$(keys $pg)"
    DM_SAVE="$P/save_$pg" DM_UIMSG=1 DM_NVN_SHOT="$(frame $pg):$P/mac_$pg.png" DM_WATCH=$(( $(frame $pg) / 30 + 8 )) ../host/dmhost "$OUT" main sdk > "$P/mac_$pg.log" 2>&1
  else
    [ $pg != sel ] && export DEVICECTL_CHILD_DM_KEYS="$(keys $pg)"
    DEVICECTL_CHILD_DM_SAVE_FRESH=1 DEVICECTL_CHILD_DM_UIMSG=1 DEVICECTL_CHILD_DM_NVN_SHOT="$(frame $pg):shot_${pg}_$RUN.png" DEVICECTL_CHILD_DM_SCREEN_SHOT="$(( $(frame $pg) / 30 + 2 )):screen_${pg}_$RUN.png" \
      DEVICECTL_CHILD_DM_WATCH=$(( $(frame $pg) / 30 + 6 )) ./run.sh boot 90 > "$P/ios_$pg.run" 2>&1
    cp "${DM_ROOT:-$HOME/dm-work}/run/ios/out/boot.txt" "$P/ios_$pg.log"
    rm -f "$P/ios_$pg.png" "$P/ios_screen_$pg.png"
    for f in shot screen; do for x in png png.txt; do [ $f.$x = shot.png.txt ] && continue
      dst="$P/ios_$([ $f = screen ] && echo screen_)$pg.$x"
      xcrun devicectl device copy from --device $D --domain-type appDataContainer --domain-identifier $B --source "Documents/${f}_${pg}_$RUN.$x" --destination "$dst" > /dev/null 2>&1 || { t "🔴 拉回 ${f}_${pg}_$RUN.$x 失败"; rc=1; }
    done; done
    t "真机 $pg 屏幕级三项："; python3 tools/screen_check.py "$P/ios_screen_$pg.png" "$P/ios_screen_$pg.png.txt" "$P/ios_$pg.log" --want "${DM_SCREEN_WANT:-16:9}" || rc=1
    if [ "${DM_SCREEN_WANT:-16:9}" = full ]; then
      NEG="${DM_FS_NEG_DIR:-${DM_ROOT:-$HOME/dm-work}/run/ios/fs}/neg_$pg.png"; [ -f "$NEG" ] || NEG=""
      t "真机 $pg 全屏核对："; python3 tools/fullscreen_check.py $pg "$P/ios_screen_$pg.png" "$P/ios_screen_$pg.png.txt" "$P/ios_$pg.log" $NEG || rc=1
    fi
  fi
  [ -f "$P/${T}_$pg.png" ] || { t "🔴 $T $pg 没截到第 $(frame $pg) 帧（看 $P/${T}_$pg.log）"; rc=1; continue; }
  side "$P/${T}_$pg.png" "$(truth $pg)" "$P/${T}_${pg}_并排.png" && t "$T $pg 并排图：$P/${T}_${pg}_并排.png"
done
exit $rc
