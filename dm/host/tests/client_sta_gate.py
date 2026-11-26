

import sys,struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_ARM64,UC_MODE_ARM,UC_HOOK_CODE
from unicorn.arm64_const import *
import os
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'tools'))
from game_addrs import GA
b=Path(sys.argv[1]).read_bytes();u=Uc(UC_ARCH_ARM64,UC_MODE_ARM);u.mem_map(0,0x4000000);u.mem_write(0,b);u.mem_map(0x7000000,0x10000)
UNIT=0x7001000;ctx={}
def hook(u,a,n,d):
 if a in [GA.AT_CL_RUN_STAMINA_STOP,GA.AT_CL_RUN_STAMINA_OK]:ctx['target']=a;u.emu_stop()
u.hook_add(UC_HOOK_CODE,hook);bad=0;n=0
for kind in [0,1,2,3,4,5,6,99]:
 for sta in [0,1,22016,0xffffffff]:
  ctx.clear();u.mem_write(UNIT,struct.pack('<I',kind));u.reg_write(UC_ARM64_REG_X19,UNIT);u.reg_write(UC_ARM64_REG_W0,sta);u.reg_write(UC_ARM64_REG_SP,0x700f000);u.reg_write(UC_ARM64_REG_NZCV,0xa0000000)
  u.emu_start(GA.AT_CL_RUN_STAMINA_CHECK,0x4000000,count=100)
  target=GA.AT_CL_RUN_STAMINA_OK if kind==0 or sta else GA.AT_CL_RUN_STAMINA_STOP
  ok=ctx.get('target')==target and u.reg_read(UC_ARM64_REG_W0)==sta and u.reg_read(UC_ARM64_REG_X19)==UNIT and u.reg_read(UC_ARM64_REG_SP)==0x700f000 and u.reg_read(UC_ARM64_REG_NZCV)==0xa0000000
  n+=1
  if not ok:bad+=1;print('FAIL type',kind,'STA',sta,'期望',hex(target),'→真',ctx)
print(f'CLIENT_STA_GATE {n}项，失败{bad}；仅type0跳STA自动Stop，非player保持原零/非零分支，原W0/X19/SP/NZCV不改；末端为替身')
sys.exit(bool(bad))
