

import sys,struct,hashlib
from pathlib import Path
import os
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'tools'))
from game_addrs import GA, GAME

D=Path(sys.argv[1]); b=(D/'main.patched.bin').read_bytes(); orig=Path(sys.argv[2]).read_bytes()
headers={};blocks=[]
try:
 for line in (D/'main.rules.tsv').read_text().splitlines():
  if line.startswith('# '):k,v=line[2:].split('\t',1);headers[k]=v
  else:blocks.append(line.split('\t'))
 assert headers['RULESET']=='touch-run-v4'
 assert headers['BUILD_ID']==GAME.build_id
 assert headers['ORIGINAL_SHA256']==hashlib.sha256(orig).hexdigest()=='028b4b840c5776f5be5c8740a0914bb0cd39e130522e08b4ea14a4207f12850f'
 assert headers['IMAGE_SHA256']==hashlib.sha256(b).hexdigest()
 expected={GA.AT_RUN_PREF_INIT_MOV:0x52800029,GA.AT_RUN_PREF_INIT_STR:0xb90e5109,GA.RUN_OFF_CLIENT:None,GA.AT_RUN_ON_CL_STAMINA_GATE:0xd503201f,GA.AT_CTRL_MOVE_STAMINA_GATE:0xd503201f,GA.AT_CTRL_MOVE_STAMINA_CLEAR:0xd503201f,GA.AT_CTRL_RUN_SELECT_BIT:0x52800021,GA.AT_CTRL_RUN_SELECT_DEFAULT:0x52800021,GA.AT_SRV_MOVE_RUN_MODE:0x52800062,GA.AT_RUN_ON_SRV_STAMINA_GATE:0xd503201f,GA.RUN_OFF_SERVER:None,GA.AT_MOVE_SELECT_STAMINA:None,GA.AT_RUN_STAMINA_COST:None,GA.AT_CL_RUN_STAMINA_CHECK:None,GA.AT_MOVE_SELECT_REQ:None}
 cuts={}
 for line in (D/'main.cuts.tsv').read_text().splitlines():
  if line.startswith('#'):continue
  off,old,new,scope,why=line.split('\t',4)
  if why.startswith('touch-run-v4：'):cuts[int(off,16)]=(old,new,scope)
 assert set(cuts)==set(expected),('剪口集合',set(cuts)^set(expected))
 def w(at):return struct.unpack_from('<I',b,at)[0]
 def branch(at):
  v=w(at);assert v&0xfc000000==0x14000000;imm=v&0x3ffffff
  return at+4*((imm^0x2000000)-0x2000000)
 for off,newword in expected.items():
  old,new,scope=cuts[off];assert scope=='全';assert orig[off:off+4]==bytes.fromhex(old);assert b[off:off+4]==bytes.fromhex(new)
  if newword is not None:assert w(off)==newword
 assert branch(GA.RUN_OFF_CLIENT)==GA.RUN_ON_CLIENT and branch(GA.RUN_OFF_SERVER)==GA.RUN_ON_SERVER
 assert len(blocks)==4
 fixheaders={}
 for line in (D/'main.fixups.tsv').read_text().splitlines():
  if line.startswith('# '):k,v=line[2:].split('\t');fixheaders[k]=int(v,0)
 po=fixheaders['PATCH_OFF'];seen=set()
 for off,raw,sha,why in blocks:
  off=int(off,16);body=bytes.fromhex(raw);assert po<=off<len(b) and off+len(body)<=len(b);assert b[off:off+len(body)]==body;assert hashlib.sha256(body).hexdigest()==sha
  src=next(s for s in [GA.AT_MOVE_SELECT_STAMINA,GA.AT_RUN_STAMINA_COST,GA.AT_CL_RUN_STAMINA_CHECK,GA.AT_MOVE_SELECT_REQ] if branch(s)==off);assert src not in seen;seen.add(src)
  if src==GA.AT_MOVE_SELECT_REQ:
   assert len(body)==24 and w(off)==0x35000088 and w(off+4)==0x51000849 and w(off+8)==0x35000049 and w(off+12)==0x52800062 and w(off+16)==0x51000849 and branch(off+20)==GA.AT_MOVE_SELECT_REQ_NEXT
   continue
  assert w(off)==0xb9400268
  assert w(off+4)==(0x34000048 if src==GA.AT_CL_RUN_STAMINA_CHECK else 0x35000048)
  if src==GA.AT_MOVE_SELECT_STAMINA:
   assert len(body)==24 and branch(off+8)==GA.AT_MOVE_SELECT_RUN and w(off+12)==0x34000040 and branch(off+16)==GA.AT_MOVE_SELECT_RUN and branch(off+20)==GA.AT_MOVE_SELECT_NO_STAMINA
  elif src==GA.AT_RUN_STAMINA_COST:
   assert len(body)==20 and branch(off+8)==GA.AT_RUN_STAMINA_COST_END and w(off+12)==0x710006df and branch(off+16)==GA.AT_RUN_STAMINA_COST_NEXT
  else:
   assert len(body)==28 and branch(off+8)==off+16 and branch(off+12)==GA.AT_CL_RUN_STAMINA_OK and w(off+16)==0x34000040 and branch(off+20)==GA.AT_CL_RUN_STAMINA_OK and branch(off+24)==GA.AT_CL_RUN_STAMINA_STOP
 assert seen=={GA.AT_MOVE_SELECT_STAMINA,GA.AT_RUN_STAMINA_COST,GA.AT_CL_RUN_STAMINA_CHECK,GA.AT_MOVE_SELECT_REQ}
 print('RUN_MANIFEST OK 原像/成像hash、buildid、15剪口白名单、4跳板hash/范围/结构/入口B')
except (AssertionError,KeyError,OSError,ValueError,StopIteration) as e:
 print('RUN_MANIFEST FAIL',type(e).__name__,str(e));sys.exit(1)
