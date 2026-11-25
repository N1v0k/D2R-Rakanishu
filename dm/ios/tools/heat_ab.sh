#!/bin/bash




set -u
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/local_env.sh" && local_env_need DM_DEVICE || exit 1
S=${1:-300}; D=$DM_DEVICE; B=${DM_BUNDLE_ID:?先在本机配置里设 DM_BUNDLE_ID（App 包名，见 dm/ios/local.env.example）}; R=${DM_ROOT:-$HOME/dm-work}/run/ios/heat; RUN=$(date +%H%M%S); mkdir -p $R
t(){ echo "[$(date +%T)] $*"; }
pid(){ xcrun devicectl device info processes --device $D 2>/dev/null | grep "DmIOS.app/DmIOS" | awk '{print $1}'; }
for g in B A; do
  m=boot; [ $g = B ] && m=blank
  DEVICECTL_CHILD_DM_HEAT="heat_${g}_$RUN.tsv,10" DEVICECTL_CHILD_DM_THRCPU="thr_${g}_$RUN.tsv,10" DEVICECTL_CHILD_DM_NVN_TSV="frames_${g}_$RUN.tsv" DEVICECTL_CHILD_DM_SAVE_FRESH=1 DEVICECTL_CHILD_DM_WATCH=$((S + 20)) \
    xcrun devicectl device process launch --device $D --terminate-existing $B -mode $m > $R/launch_$g.log 2>&1 || { t "🔴 $g 启动失败"; exit 1; }
  t "${g}（${m}）开始 $S 秒"; sleep $S
  p=$(pid); [ -n "$p" ] && xcrun devicectl device process terminate --device $D --pid $p > /dev/null 2>&1; sleep 3
  [ -z "$(pid)" ] || t "🔴 $g 停不掉"
  for f in heat thr frames; do [ $g = B ] && [ $f = frames ] && continue
    xcrun devicectl device copy from --device $D --domain-type appDataContainer --domain-identifier $B --source "Documents/${f}_${g}_$RUN.tsv" --destination $R/${f}_$g.tsv > /dev/null 2>&1 || t "🔴 拉 ${f}_$g 失败"; done
  t "$g 结束"
done
python3 - $R <<'PY'
import sys, statistics as st
R = sys.argv[1]
for g, name in (('B', '空白负对照'), ('A', '我们的 App 选角页')):
    rows = [l.rstrip('\n').split('\t') for l in open(f'{R}/heat_{g}.tsv')][1:]
    rows = [r for r in rows if len(r) == 9]
    th = [int(r[1]) for r in rows]; cpu = [float(r[2]) for r in rows]; fps = [float(r[3]) for r in rows]
    ga = [float(r[4]) for r in rows if float(r[4]) >= 0]; gp = [float(r[5]) for r in rows if float(r[5]) >= 0]
    print(f'{g} {name}：{len(rows)} 行 / {rows[-1][0] if rows else 0} 秒；温控 起 {th[0] if th else "?"} 止 {th[-1] if th else "?"}（序列 {"".join(map(str, th))}）；'
          f'CPU% 中位 {st.median(cpu):.1f}（{min(cpu):.1f}–{max(cpu):.1f}）；出图帧率 中位 {st.median(fps):.1f}；'
          + (f'GPU 每帧 均值中位 {st.median(ga):.2f} ms、p95 中位 {st.median(gp):.2f} ms；' if ga else 'GPU 无（空白模式不取）；')
          + f'亮度 {rows[0][7]}；最高刷新 {rows[0][8]}')
PY
