#!/bin/bash




















set -u; HOLD=
export LC_ALL=C
TR=${1:?真值局目录}; OUT=${2:-${DM_ROOT:-$HOME/dm-work}/build/cache/dm/out/3.3.94012-r12}
FT=${FT:-1790000000}
X=${DM_ROOT:-$HOME/dm-work}; PY=$X/build/cache/dm/venv/bin/python3; HERE=$(cd "$(dirname "$0")/.." && pwd)
R=$X/run/platform; T=$R/hsc_$(basename "$TR")_$(date +%m%d_%H%M%S)_$$; mkdir -p "$T/user" || exit 1
echo "局目录 $T"
red() { echo "【红】$*"; exit 2; }
WALL=${HSC_WALL:-600}
case "$WALL" in ''|*[!0-9]*) red "HSC_WALL 须为大于30的整数秒";; esac
WALL=$((10#$WALL)); [ "$WALL" -gt 30 ] || red "HSC_WALL 须大于30秒"
WATCH=$((WALL * 3 / 4))
echo "期限：宿主无进展看门狗 15 秒、总墙钟兜底 ${WATCH} 秒、退出墙钟 ${WALL} 秒"
for f in meta.json c2s.jsonl save_after/0; do [ -e "$TR/$f" ] || red "真值局缺 $f"; done



WANT=$($PY -c "import json,sys;m=json.load(open(sys.argv[1]));s=m['save'];print(s[0] if isinstance(s,list) else s)" "$TR/meta.json") || red "meta.json 读不出 save"
SEED=$($PY -c "import json,sys;v=json.load(open(sys.argv[1])).get('game_seed');print('' if v is None else v)" "$TR/meta.json")
REF=
for m in $X/data/switch/*/MANIFEST.sha256; do [ "$(shasum -a 256 "$m" | cut -c1-64)" = "$WANT" ] && { REF=$(dirname "$m"); break; }; done
[ -n "$REF" ] || red "起始存档指纹 $WANT 在 data/switch/*/MANIFEST.sha256 里找不到"
echo "① 起始存档 ${REF}（指纹 ${WANT:0:12}… 与 meta.json 一致）"
cp -R "$REF/0000000000000001/0" "$T/user/0" && chmod -R u+w "$T/user/0" || red "拷存档失败"
( cd "$T/user" && grep '  0000000000000001/0/' "$REF/MANIFEST.sha256" | sed 's|  0000000000000001/|  |' | shasum -a 256 -c --quiet ) || red "0/ 副本与 MANIFEST 不符"
NF=$(grep -c '  0000000000000001/0/' "$REF/MANIFEST.sha256"); echo "   0/ 副本 $NF 个文件逐个核 sha 通过"



K=${KEYS_FRONT:-A@OpenPanel:MainMenuButtonRibbon>OpenPanel:CharacterDifficultyModal,A@OpenPanel:CharacterDifficultyModal>FrontEndNavigation:ToGame}








if [ -z "${KEYS_EXIT+x}" ] && [ -f "$TR/events.jsonl" ]; then
  KEYS_EXIT=$($PY - "$TR/events.jsonl" <<'PYEOF'
import json, sys
L = E = None
for line in open(sys.argv[1]):
    try: e = json.loads(line)
    except ValueError: continue
    if e.get('kind') == 'play': L = e['last']
    if e.get('kind') == 'after_done': E = e['last_tick']
if L is not None and E is not None:
    print(f'PLUS@DM:Tick:{L}#>OpenPanel:PlayerInventoryExpansionLayout,R@DM:Tick:{L + 18}#>OpenPanel:SkillsTreePanel,'
          f'R@DM:Tick:{E}#>OpenPanel:PauseLayout,A@OpenPanel:PauseLayout%1>PausePanelMessage:ExitGame\t{E}')
PYEOF
)
  HOLD=${KEYS_EXIT##*	}; KEYS_EXIT=${KEYS_EXIT%	*}
  echo "② 退出键序（按真值 events.jsonl 生成）：${KEYS_EXIT}；DM_TICK_HOLD=$HOLD"
fi
[ -n "${KEYS_EXIT:-}" ] && K="$K,$KEYS_EXIT"
cd "$HERE/host" || exit 1
make -s >/dev/null || red "宿主编译失败"
mkdir -p "$T/temp"
env DM_SAVE="$T/user/0" DM_TEMP="$T/temp" ${SEED:+DM_GAME_SEED=$SEED} ${HOLD:+DM_TICK_HOLD=$HOLD} DM_FIXED_TIME=$FT "DM_KEYS=$K" DM_C2S_REPLAY="$TR/c2s.jsonl" DM_TICKS="$T/ticks.jsonl" DM_C2S_LOG="$T/out.c2s" \
    DM_UIMSG=1 DM_PRESENT_HZ=60 DM_WATCH_STALL=15 DM_WATCH=$WATCH ./dmhost "$OUT" main sdk > "$T/host.log" 2>&1 &
P=$!
for i in $(seq 1 $((WALL * 10))); do
  [ "$(grep -c 'OpenPanel MainMenuButtonRibbon' "$T/host.log")" -ge 2 ] && { echo "② 已回到主菜单（第 $((i/10)) 秒）"; break; }
  kill -0 $P 2>/dev/null || { echo "② 宿主提前退出"; break; }; sleep 0.1
done
[ "$(grep -c 'OpenPanel MainMenuButtonRibbon' "$T/host.log")" -ge 2 ] || { kill $P 2>/dev/null; grep '【红】看门狗' "$T/host.log" | tail -1; red "${WALL} 秒内没回到主菜单"; }
sleep 2; kill $P 2>/dev/null; wait $P 2>/dev/null
grep -E 'DM_FIXED_TIME|DM_C2S_REPLAY|hid 脚本步.*(确认|红)|【红】|ExitGame' "$T/host.log" | grep -v 贴图号 | cut -c1-160
K6=0; [ -f "$TR/ticks.jsonl" ] && { $PY "$HERE/verify/ryu/selfcmp.py" "$TR/ticks.jsonl" "$T/ticks.jsonl" | tail -8; K6=${PIPESTATUS[0]}; }



SC=${SC:-$HERE/verify/savecmp.py}
MJ=$TR/meta.json; [ -f "$TR/meta.v2.json" ] && MJ=$TR/meta.v2.json
MJ=${META:-$MJ}
AT="--a-meta=$MJ"; grep -q wall_start "$MJ" || AT=${A_TIME:+--a-time=$A_TIME}
echo "③ 正对照：真值 vs 宿主（$SC --b-time=$FT ${AT}）"; $PY "$SC" "$TR/save_after" "$T/user" --b-time=$FT $AT; G=$?
echo "③ 负对照：真值 vs 起始存档"; $PY "$SC" "$TR/save_after" "$REF/0000000000000001" --b-time=$FT $AT; N=$?
echo "结论：逐拍退出码 ${K6}、正对照退出码 ${G}（0 = 绿），负对照退出码 ${N}（须非 0）"
[ $K6 = 0 ] && [ $G = 0 ] && [ $N != 0 ]
