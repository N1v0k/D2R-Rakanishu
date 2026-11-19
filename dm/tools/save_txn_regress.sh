#!/bin/bash






















set -u
export LC_ALL=C
X=${DM_ROOT:-$HOME/dm-work}; HERE=$(cd "$(dirname "$0")/../.." && pwd)
TR=${TRUTH_RUN:-$X/data/switch/truth_runs/cvd5}; O=${1:-$(bash "$(dirname "$0")/product.sh" path keypage)}
REF=$X/data/switch/save_ref_warlock/0000000000000001/0; AFT=$TR/save_after/0; PY=$X/build/cache/dm/venv/bin/python3
[ -d "$TR" ] && [ -d "$REF" ] || { echo "【红】缺 cvd5 真值局或起始存档"; exit 2; }
bad=0
RUN=$X/run/verify/dm/save_txn/$(date +%m%d_%H%M%S)_$$; mkdir -p "$RUN" || exit 2
echo "SAVE_TXN 现场 ${RUN}；射程 4 例，每例独立存档副本"


for c in KILL_AT:1 KILL_AT:3 FS_FAIL:1 FS_FAIL:3; do
  V=${c%%:*}; K=${c##*:}
  env DM_TEST_$V=write:$K bash "$HERE/dm/tools/host_savecmp.sh" "$TR" "$O" > "$RUN/${V}_$K.log" 2>&1 &
done
wait
for c in KILL_AT:1 KILL_AT:3 FS_FAIL:1 FS_FAIL:3; do
  V=${c%%:*}; K=${c##*:}; L=$RUN/${V}_$K.log
  D=$(grep -o '局目录 .*' "$L" | cut -d' ' -f2)
  [ -n "$D" ] && [ -f "$D/host.log" ] || { echo "【红】$c 没跑起来，见 $L"; echo "$c 启动红" >> "$RUN/KEEP"; bad=1; continue; }
  echo "$D" > "$RUN/${V}_$K.dir"
  $PY - "$D" "$REF" "$c" "$K" "$AFT" <<'PY'
import sys,os,hashlib,struct
d,ref,c,k,aft=sys.argv[1],sys.argv[2],sys.argv[3],int(sys.argv[4]),sys.argv[5]
log=open(d+'/host.log',errors='replace').read()
def tree(r):
    o={}
    for root,_,fs in os.walk(r):
        for f in fs: p=os.path.join(root,f); o[os.path.relpath(p,r)]=open(p,'rb').read()
    return o
def d2s_ok(b):
    if len(b)<16: return False
    b=bytearray(b); n,c0=struct.unpack_from('<II',b,8); struct.pack_into('<I',b,12,0); s=0
    for x in b: s=(((s<<1)|(s>>31))+x)&0xffffffff
    return n==len(b) and s==c0
com,a,work=tree(d+'/user/0'),tree(ref),tree(d+'/user/0.work')
hit=('DM_TEST_%s：存档第 %d 次写' % (c.split(':')[0],k)) in log
STASH='ModernSharedStashSoftCoreV2/ModernSharedStashSoftCoreV2.d2i'
fail=c.startswith('FS_FAIL')
exempt={'joe/joe.d2s','joe/joe.d2s.meta'} if k==3 else set()
want=dict(a)
if fail and k==1: want[STASH]=tree(aft)[STASH]; exempt=set()
diff=sorted(f for f in set(want)|set(com) if f not in exempt and want.get(f)!=com.get(f))
ok_d2s=d2s_ok(com.get('joe/joe.d2s',b''))
torn=STASH if k==3 else 'joe/joe.d2s'
if fail:
    rl=[l for l in log.splitlines() if '存档提交拒绝' in l]
    neg=len(rl)==1 and torn in rl[0] and '仍有存档文件开着' not in rl[0]
    if k==1:
        after=log[log.index('存档提交拒绝'):] if rl else ''
        nxt=after.count('（此前存档写 3 次）')==1 and after.count('存档提交 #')==1
        print(f'  下一个事务照常提交（拒绝之后恰 1 条成功提交且此前写 3 次）：{nxt}'); neg=neg and nxt
else: neg=len(work.get(torn,b'x'))==0
ok=hit and not diff and ok_d2s and neg
print(f"{'✓' if ok else '✗'} {c}：注入命中 {hit}；已提交 {len(com)} 个文件，应同期望版 {len(want)-len(exempt)} 个、不同 {len(diff)} {diff[:3]}；joe.d2s 自洽 {ok_d2s}；负对照 {'拒绝日志恰 1 条且列出' if fail else '工作目录'} {torn} 0 字节 {neg}")
sys.exit(0 if ok else 1)
PY
  rc=$?
  if [ "$rc" != 0 ]; then bad=1; echo "$c 判据红；现场保留" >> "$RUN/KEEP"; echo "$c 判据红；现场保留" >> "$D/KEEP"; fi
done
[ $bad = 0 ] && echo "结论：存档事务回归 4/4 绿" || echo "结论：存档事务回归有红"
exit $bad
