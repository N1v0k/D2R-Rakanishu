#!/bin/bash




set -u
export LC_ALL=C
HERE=$(cd "$(dirname "$0")" && pwd)
X=${DM_WORK_ROOT:-${DM_ROOT:-$HOME/dm-work}}
PY=${DM_PY:-$X/build/cache/dm/venv/bin/python3}
TR=${DM_LOGICSELF_TRUTH:-$X/data/switch/truth_runs/cvd5}
OUT_ARG=${1:-out/$(bash "$(dirname "$0")/../tools/product.sh" name touch)}
N=${2:-230}
case "$N" in ''|*[!0-9]*) echo "LOGICSELF RED 拍数非法：$N"; exit 2;; esac
case "$OUT_ARG" in /*) OUT=$OUT_ARG;; *) OUT=$X/build/cache/dm/$OUT_ARG;; esac
BASELINE=${LOGICSELF_BASELINE:-$HERE/logicself/touch.json}
[ -f "$TR/meta.json" ] && [ -f "$TR/c2s.jsonl" ] || { echo "LOGICSELF RED 真值文件不全：$TR"; exit 2; }
[ -f "$OUT/main.patched.bin" ] || { echo "LOGICSELF RED 产物缺 main.patched.bin：$OUT"; exit 2; }
if [ "${LOGICSELF_REBASE:-0}" != 1 ] && [ ! -f "$BASELINE" ]; then
    echo "LOGICSELF RED 没有基线文件：$BASELINE"
    exit 1
fi
mkdir -p "$HERE/logicself" || exit 2
RUNROOT=${DM_LOGICSELF_RUNROOT:-$X/run/verify/dm/logicself}
mkdir -p "$RUNROOT" || exit 2
STAMP=$(date +%Y%m%d_%H%M%S)_$$

run_once() {
    local tag=$1 run
    run="$RUNROOT/${STAMP}_${tag}"
    mkdir -p "$run/user" || return 2
    save=$($PY - "$TR/meta.json" <<'PY'
import json, sys
m=json.load(open(sys.argv[1])); s=m['save']; print(s[1] if isinstance(s, list) else s)
PY
) || return 2
    [ -d "$save/0" ] || { echo "LOGICSELF RED 起档缺 0/：$save" >&2; return 2; }
    cp -R "$save/0" "$run/user/0" && chmod -R u+w "$run/user/0" || return 2
    seed=$($PY - "$TR/meta.json" <<'PY'
import json, sys
print(json.load(open(sys.argv[1]))['game_seed'])
PY
) || return 2
    ft=$($PY - "$TR/meta.json" <<'PY'
import json, sys
m=json.load(open(sys.argv[1])); print(m.get('fix_time') if m.get('fix_time') is not None else m['wall_start'])
PY
) || return 2
    ( cd "$HERE/../host" && timeout 100s make -s ) >"$run/make.log" 2>&1 || { echo "LOGICSELF RED 宿主编译失败；局目录 $run" >&2; return 1; }
    K=${KEYS_FRONT:-A@OpenPanel:MainMenuButtonRibbon>OpenPanel:CharacterDifficultyModal,A@OpenPanel:CharacterDifficultyModal>FrontEndNavigation:ToGame}






    env DM_SAVE="$run/user/0" DM_GAME_SEED="$seed" DM_FIXED_TIME="$ft" DM_DETCLOCK=33333333 \
        DM_KEYS="$K" DM_C2S_REPLAY="$TR/c2s.jsonl" DM_TICKS="$run/ticks.jsonl" DM_TICKS_N="$((N + 1))" \
        DM_C2S_LOG="$run/c2s.out" DM_UIMSG=1 DM_PRESENT_HZ=60 DM_WATCH_STALL=15 DM_WATCH=90 \
        timeout 100s "$HERE/../host/dmhost" "$OUT" main sdk >"$run/host.log" 2>&1
    local rc=$?
    [ "$rc" -eq 0 ] || { echo "LOGICSELF RED 宿主回放失败；局目录 $run" >&2; grep '【红】看门狗' "$run/host.log" | tail -1 >&2 || true; return 1; }
    "$PY" - "$run/ticks.jsonl" "$run/summary.json" "$N" <<'PY'
import json, sys
src, dst, limit = sys.argv[1], sys.argv[2], int(sys.argv[3]); rows=[]
for line in open(src):
    x=json.loads(line)
    if x.get('tick', -1) <= limit:
        rows.append({'tick':x['tick'], 'n':x['n'], 'h':x['h']})
json.dump(rows, open(dst, 'w'), ensure_ascii=False, separators=(',', ':'))
PY
    echo "$run/summary.json"
}

compare() {
    "$PY" - "$1" "$2" "$N" <<'PY'
import json, sys
a=json.load(open(sys.argv[1])); b=json.load(open(sys.argv[2])); n=int(sys.argv[3])
for i,(x,y) in enumerate(zip(a,b)):
    if x != y:
        print(f'LOGICSELF RED 逻辑与自身基线不一致，首个不同拍 {x.get("tick", i)}')
        print('LOGICSELF 首个差异：' + json.dumps({'actual':x,'baseline':y}, ensure_ascii=False, separators=(',', ':')))
        raise SystemExit(1)
if len(a) != len(b):
    print(f'LOGICSELF RED 逻辑与自身基线不一致，首个不同拍 {a[len(b)].get("tick", len(b)) if len(a)>len(b) else b[len(a)].get("tick", len(a))}')
    raise SystemExit(1)
if len(a) != n + 1:
    print(f'LOGICSELF RED 射程不足：{len(a)} 条，期望 {n + 1} 条')
    raise SystemExit(1)
print(f'LOGICSELF OK 拍数 {n} 射程 {len(a)}')
PY
}

if [ "${LOGICSELF_REBASE:-0}" = 1 ]; then
    A=$(run_once a) || exit $?
    B=$(run_once b) || exit $?
    compare "$A" "$B" || exit $?
    "$PY" - "$A" "$BASELINE" "$HERE" "$OUT" <<'PY'
import json, sys, subprocess, datetime, os
rows=json.load(open(sys.argv[1])); out={'commit':subprocess.check_output(['git','-C',sys.argv[3], 'rev-parse','HEAD'], text=True).strip(), 'artifact':os.path.basename(sys.argv[4]), 'created_at':datetime.datetime.now(datetime.timezone.utc).isoformat(), 'ticks':rows}
json.dump(out, open(sys.argv[2], 'w'), ensure_ascii=False, indent=2); print('LOGICSELF 基线已写入 ' + sys.argv[2])
PY
    exit 0
fi
ACTUAL=$(run_once actual) || exit $?
"$PY" - "$BASELINE" "$RUNROOT/${STAMP}_baseline.json" <<'PY'
import json, sys
x=json.load(open(sys.argv[1])); rows=x['ticks'] if isinstance(x,dict) else x; json.dump(rows, open(sys.argv[2],'w'), ensure_ascii=False, separators=(',', ':'))
PY
compare "$ACTUAL" "$RUNROOT/${STAMP}_baseline.json"
