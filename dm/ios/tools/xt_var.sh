#!/bin/bash










PN=$(bash "$(dirname "$0")/../../tools/product.sh" name touch); D=${XT_MINI_DIR:-$(ls -dt "${DM_ROOT:-$HOME/dm-work}"/run/mini/*_"$PN" 2>/dev/null | head -1)}; [ -d "$D" ] || { echo "没有 mini 同步目录 ${DM_ROOT:-$HOME/dm-work}/run/mini/*_${PN}（先 mini_sync.sh，或用 XT_MINI_DIR 指定）"; exit 2; }; T=$D/src/dm/ios/tools; W=${DM_ROOT:-$HOME/dm-work}/run/xtrace/terr; B=${DM_ROOT:-$HOME/dm-work}/run/xtrace/batch_$(date +%m%d_%H%M%S)_var; mkdir -p $B; O=$B/out.tsv; : > $O
source $T/mlock.sh; mlock_acquire "${XT_OWNER:?先设 XT_OWNER=线名} 着色器变体 $# 局 约 $(( $# * 95 / 60 )) 分钟"; trap mlock_release EXIT
i=0; for v in "$@"; do i=$((i + 1)); R=$B/$(printf %02d $i)_$v
  e="DM_NVN_ENC_LABEL=1"; [ $v = base ] || { [ -d $W/$v/b-89f2d5b38edb ] || { echo "没有变体库 $W/$v"; continue; }; e="$e DM_MSL_DIR=$W/$v"; }
  XT_DIR=$R XT_ENV="$e" XT_ONLY='^metal-gpu-intervals$' XT_TREE=$D/src XT_OUT=$D/out/$PN XT_SAVE=$D/data/switch/save_ref_warlock/0000000000000001 bash $T/xtrace.sh mac camp 'Game Performance' 12 35 > $R.log 2>&1
  red=$(grep -c '【红】' $R/host.err 2>/dev/null)
  python3 $T/xtrace_report.py $R 2>/dev/null | awk -v k=$v -v red=$red '/metal-gpu-intervals/{match($0,/[0-9]+ 帧/); nf=substr($0,RSTART,RLENGTH); match($0,/合计 [0-9.]+/); tot=substr($0,RSTART+7,RLENGTH-7)} / p11 /{if($6=="Fragment")f=$1} / p08 /{if($6=="Fragment")df=$1} END{w=(nf+0>=350 && tot+0>=5 && tot+0<=13)?"":"\t⚠作废（帧数 / 采样量异常：正常约 440 帧、合计 7–10）"; printf "%s\t%s 合计 %s\tp11 片元 %s\tp08 片元 %s\t对p08 %.3f\t红 %s%s\n",k,nf,tot,f,df,(df>0?f/df:0),red,w}' | tee -a $O
  [ -d $R/tables ] && find $R/tables -delete
done; echo "结果 $O"
