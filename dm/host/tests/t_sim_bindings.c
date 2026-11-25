#include "../../ios/Sources/dm_touch_game_state.h"
#include "../dm.h"
#include <assert.h>

module_t g_mod[MAX_MOD];int g_nmod;
int gate_obs_present(int id){return id==6;}
void gate_obs_set(int id,void (*fn)(const uint64_t *)){(void)id;(void)fn;}
uint64_t now_ns(void){return 1;}
int dm_swapskill_equipped(int32_t item){(void)item;return 0;}
int dm_swapskill_offhand_gives(int skill){(void)skill;return 0;}
int dm_touch_game_test_identity(const uint64_t *regs,uint64_t *caller,uint64_t *hud){(void)regs;*caller=*hud=0;return DM_TG_IDENTITY_UNCONFIRMED;}
int main(void){
    size_t size=0x2d5f0c0+32*DM_TG_STRIDE;unsigned char *memory=calloc(1,size);assert(memory);
    g_mod[0].base=memory;g_mod[0].size=size;strcpy(g_mod[0].name,"main");g_nmod=1;
    unsigned char *binding=memory+0x2d5f0c0;
    memset(binding,255,32*DM_TG_STRIDE);
    uint32_t ids[]={4,9,4,40};int slots[]={0,7,16,31};
    for(int i=0;i<4;i++)memcpy(binding+slots[i]*DM_TG_STRIDE,&ids[i],4);
    assert(dm_touch_game_attach());int out[64];
    assert(!dm_touch_game_skills(out,64));
    dm_touch_game_set_scene(1,1,1);assert(dm_touch_game_skills(out,64)==3);
    assert(out[0]==4 && out[1]==9 && out[2]==40);
    dm_touch_game_set_scene(0,2,1);assert(dm_touch_game_skills(out,64)==3);
    assert(dm_touch_game_skills(out,2)==2 && !dm_touch_game_skills(NULL,64));
    dm_touch_game_set_scene(0,3,0);assert(!dm_touch_game_skills(out,64));
    free(memory);puts("sim bindings: both banks, panel navigation, deduplication, capacity and world invalidation passed");return 0;
}
