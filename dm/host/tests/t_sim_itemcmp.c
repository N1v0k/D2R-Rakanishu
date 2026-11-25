#include "../dm_sim_internal.h"
#include "../dm_itemcmp_core.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    dm_sim_data d={0};dm_sim_skill attack={0};d.ok=1;d.ncls=1;d.skill[0]=&attack;
    attack.fam=DM_FAM_W1;attack.srcdam=128;attack.hitshift=8;attack.etype=-1;attack.alwayshit=1;
    attack.missile=attack.descmissile=attack.summon=-1;
    for(int i=0;i<DM_CF_N;i++)attack.calc[i]=-1;
    for(int i=0;i<DM_ROLE_N;i++)attack.role[i]=-1;
    dm_sim_char c;dm_sim_char_init(&c,0);c.hand[0].min=100;c.hand[0].max=200;c.hand[0].kind=DM_WK_MELEE;
    dm_sim_target t={0};t.hp=1000;t.mlvl=50;dm_sim_tmod m;dm_sim_tmod_init(&m);
    for(int strength=0;strength<=200;strength+=50)for(int ed=0;ed<=300;ed+=100) {
        c.str=strength;c.ed=ed;dm_sim_skill_out o;assert(!sim_action(&d,&c,&t,&m,0,&o));
        dm_ic_cb x={0};x.armed=1;x.mn[0]=100;x.mx[0]=200;x.srcdam=128;x.str=strength;x.strb=100;x.dpct=ed;
        int v[IC_C_N+1],range[2];assert(dm_ic_cb_eval(&x,v));assert(!sim_normal_physical(&c,range));
        assert(range[0]==v[IC_C_DMG] && range[1]==v[IC_C_N]);
    }
    c.str=100;c.ed=0;c.hand[0].min=c.hand[0].max=10;c.hand[0].edmin=c.hand[0].edmax=100;c.hand[0].postflat=3;
    dm_sim_skill_out out;assert(!sim_action(&d,&c,&t,&m,0,&out));
    assert(out.hit[0]==33);
    puts("sim item comparison: 20 normal-attack profiles agree with the existing core");return 0;
}
