




from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
src = root / 'Sources/dm_touch_game_bridge.c'
text = src.read_text()
call = 'skill_live((const unsigned char*)m->base,id,item)'
assert text.count(call) == 1, 'ownership call site changed'
text = text.replace(call, 'test_skill_live(id,item)')
text = text.replace('#include "../../host/dm.h"', '#include "%s"' % (root.parent / 'host/dm.h'))
hook_at = text.index('static void binding_getter(')
text = text[:hook_at] + 'int test_skill_live(uint32_t id,int32_t item);\n' + text[hook_at:]

harness = r'''
#include "dm_touch_game_state.h"
#include "%(dm_h)s"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
module_t g_mod[MAX_MOD]; int g_nmod;
static void (*hook)(const uint64_t *);
static uint64_t clock_ns=1000000000ull;
static int calls,checks,bad;
enum { N=DM_TG_SLOTS };
static uint32_t dead_id=UINT32_MAX;
int gate_obs_present(int id) { return id==6; }
void gate_obs_set(int id,void (*fn)(const uint64_t *)) { if(id==6)hook=fn; }
uint64_t now_ns(void) { return clock_ns; }
int test_skill_live(uint32_t id,int32_t item) { (void)item; calls++; return id!=dead_id; }
int dm_swapskill_equipped(int32_t item) { (void)item; return 0; }
int dm_swapskill_offhand_gives(int skill) { (void)skill; return 0; }
#ifdef DM_TG_TEST_SYNTHETIC_IDENTITY
int dm_touch_game_test_identity(const uint64_t *regs,uint64_t *caller,uint64_t *hud) { (void)regs; *caller=77; *hud=88; return DM_TG_IDENTITY_SYNTHETIC; }
#endif
#define CHECK(x) do { checks++; if(!(x)) { fprintf(stderr,"FAIL line=%%d %%s\n",__LINE__,#x); bad++; } } while(0)
static void put32(unsigned char *p,uint32_t v) { memcpy(p,&v,4); }
static void call_bank(unsigned char *mem,int bank,int slot,int x23_off) {
    uint64_t regs[31]={0};
    regs[10]=(uint64_t)(mem+0x2d5f0c0); regs[8]=(uint64_t)(bank+slot)*28; regs[19]=(uint64_t)slot; regs[23]=(uint64_t)slot+(uint64_t)x23_off;
    hook(regs);
}
int main(void) {
    size_t size=0x2d5f0c0+32*28; unsigned char *mem=calloc(1,size); if(!mem) return 2;
    g_mod[0].base=mem; g_mod[0].size=size; strcpy(g_mod[0].name,"main"); g_nmod=1;
    const uint32_t map[6][3]={{0x1000,1,7},{0x2000,0,6},{0x4000,2,8},{0x8000,3,9},{0x200,4,10},{0x800,5,11}};
    for(int i=0;i<6;i++) for(int j=0;j<3;j++) put32(mem+0x2d13cbc+i*12+j*4,map[i][j]);
    for(int i=0;i<32;i++) put32(mem+0x2d5f0c0+i*28,UINT32_MAX);
    put32(mem+0x2d5f0c0+0*28,36); put32(mem+0x2d5f0c0+2*28,52); put32(mem+0x2d5f0c0+16*28,40);
    CHECK(dm_touch_game_attach()==1 && hook);
    dm_touch_game_set_scene(1,2,1);
    dm_tg_snapshot s;
#ifdef DM_TG_TEST_SYNTHETIC_IDENTITY
    call_bank(mem,0,0,0); CHECK(calls==0);                       /* check off: the predicate is never called */
    dm_touch_game_live_check(1);
    dead_id=52; call_bank(mem,0,0,0); CHECK(calls==N);          /* first call of bank 0 checks every slot of the bank */
    CHECK(dm_touch_game_snapshot(&s) && s.raw_skill_id[0]==36 && s.raw_skill_id[2]==UINT32_MAX);
    dead_id=UINT32_MAX; clock_ns+=100000000ull;
    for(int i=1;i<6;i++) call_bank(mem,0,i,0);
    CHECK(calls==N);                                            /* inside 0.2 s: cached, no further checks */
    CHECK(dm_touch_game_snapshot(&s) && s.raw_skill_id[2]==UINT32_MAX);   /* cached dead slot stays empty */
    call_bank(mem,16,0,0); CHECK(calls==2*N);                     /* the other bank has its own cache */
    CHECK(dm_touch_game_snapshot(&s) && s.selected_bank==16 && s.raw_skill_id[0]==40);
    clock_ns+=100000000ull; call_bank(mem,0,0,0); CHECK(calls==3*N);   /* 0.2 s after the first check: recheck */
    CHECK(dm_touch_game_snapshot(&s) && s.selected_bank==0 && s.raw_skill_id[2]==52);   /* skill came back */
    dead_id=36; clock_ns+=199999999ull; call_bank(mem,0,1,0); CHECK(calls==3*N);
    CHECK(dm_touch_game_snapshot(&s) && s.raw_skill_id[0]==36);
    clock_ns+=1; call_bank(mem,0,1,0); CHECK(calls==4*N);
    CHECK(dm_touch_game_snapshot(&s) && s.raw_skill_id[0]==UINT32_MAX);
    printf("skill bank recheck (synthetic identity): %%d/%%d checks passed\n",checks-bad,checks);
#else
    dm_touch_game_live_check(1);
    for(int i=0;i<6;i++) call_bank(mem,0,i,1);
    CHECK(calls==0);                                             /* x23 != x19: not the HUD updater, filtered before any check */
    CHECK(!dm_touch_game_snapshot(&s));
    call_bank(mem,0,0,0); CHECK(calls==N);                      /* HUD updater shape: checked once */
    call_bank(mem,0,1,0); CHECK(calls==N);
    printf("skill bank recheck (production filter): %%d/%%d checks passed\n",checks-bad,checks);
#endif
    free(mem);
    return bad?1:0;
}
''' % {'dm_h': root.parent / 'host/dm.h'}

with tempfile.TemporaryDirectory(prefix='dm-skill-bank-') as temp:
    bridge = Path(temp) / 'bridge.c'
    test = Path(temp) / 't_skill_bank.c'
    bridge.write_text(text)
    test.write_text(harness)
    for defines, name in ((['-DDM_TG_TEST_SYNTHETIC_IDENTITY'], 'synthetic'), ([], 'production')):
        binary = Path(temp) / ('t_skill_bank_' + name)
        subprocess.run([os.environ.get('CC', 'cc'), '-std=c11', '-O1', '-Wall', '-Wextra', '-Werror',
                        '-Wno-unused-function', *defines, '-I' + str(root / 'Sources'),
                        str(test), str(bridge), str(root / 'Sources/dm_touch_game_identity.c'),
                        str(root / 'Sources/dm_touch_game_state.c'), '-lpthread', '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
