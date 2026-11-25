#include "dm_touch_game_state.h"
#include "dm_touch_game_identity.h"
#include "../../host/dm.h"
enum { OBS_BINDING_GETTER=6, MAPPING_OFFSET=0x2d13cbc, BINDING_OFFSET=0x2d5f0c0 };
static pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
static dm_tg_snapshot latest;
static dm_tg_group group;
static dm_tg_hud_cohort hud_cohort;
static int scene_game;
static uint64_t scene_epoch,world_epoch;
static const module_t *main_module;
void dm_touch_game_set_scene(int game,uint64_t scene,uint64_t world) {
    pthread_mutex_lock(&lock);
    if(world_epoch!=world)dm_tg_hud_reset(&hud_cohort,world);
    if(scene_game!=game || scene_epoch!=scene || world_epoch!=world) {
        scene_game=game;scene_epoch=scene;world_epoch=world;memset(&group,0,sizeof group);
        int supported=latest.supported;memset(&latest,0,sizeof latest);latest.supported=supported;
        latest.scene_epoch=scene;latest.world_epoch=world;latest.portal.slot=-1;
    }
    pthread_mutex_unlock(&lock);
}
int dm_touch_game_snapshot(dm_tg_snapshot *out) {
    if(!out)return 0;
    pthread_mutex_lock(&lock);*out=latest;pthread_mutex_unlock(&lock);return out->valid;
}

int dm_touch_game_hide_own_glyph(uint64_t widget) {
    int match=0;pthread_mutex_lock(&lock);
    const module_t *m=main_module;uint64_t t=now_ns();
    if(m && scene_game && scene_epoch && world_epoch && hud_cohort.confirmed && hud_cohort.seen==63 &&
       hud_cohort.world_epoch==world_epoch && latest.supported && latest.valid && latest.ready &&
       latest.identity_scope==DM_TG_IDENTITY_NATIVE && latest.source==DM_TG_SOURCE_NATIVE_GETTER &&
       latest.scene_epoch==scene_epoch && latest.world_epoch==world_epoch &&
       latest.hud_identity==hud_cohort.hud_identity && latest.group_seen==63 &&
       latest.captured_ns && t>=latest.captured_ns && t-latest.captured_ns<=DM_TG_MAX_AGE_NS) {
        for(unsigned i=0;i<6;i++)if(widget && widget==hud_cohort.widget[i]) {

            static unsigned char object[0xc91];uint64_t vt=0;uint32_t slot=UINT32_MAX;
            if(widget<=UINT64_MAX-0xc90 && dm_tg_vm_read(widget,object,sizeof object)) {
                memcpy(&vt,object,8);memcpy(&slot,object+0xb18,4);
                if(vt==(uint64_t)m->base+0x208a5f8 && slot==i && !object[0xc90])match=1;
            }
            break;
        }
    }
    pthread_mutex_unlock(&lock);return match;
}

static unsigned dg[8], dg_id[8]; static uint64_t dg_last;
static void diag_tick(void) {
    static int on=-1; if(on<0) on=getenv("DM_BINDING_DIAG")!=NULL; if(!on) return;
    uint64_t t=now_ns(); if(t-dg_last<3000000000ull) return; dg_last=t;
    LOG("绑定诊断：调用 %u｜寄存器不符 %u｜槽越界 %u｜组不符 %u｜不在局内 %u｜身份未确认 %u（原因 3:%u 4:%u 5:%u 6:%u 7:%u，LR 偏移 %#llx 槽 %u）｜成功 %u｜scene_game %d world %llu",
        dg[0],dg[1],dg[2],dg[3],dg[4],dg[5],dg_id[3],dg_id[4],dg_id[5],dg_id[6],dg_id[7],(unsigned long long)dm_tg_identity_lr,dm_tg_identity_slot,dg[6],scene_game,(unsigned long long)world_epoch);
}
static int live_check;
void dm_touch_game_live_check(int on) { live_check=on; }






static int skill_live(const unsigned char *b,uint32_t id,int32_t item) {

    switch(id) {
    case 357: case 358: case 360: case 361: case 362: case 363: case 364: case 370:
        return 1;
    default: break;
    }
    if(id==UINT32_MAX || id==359 || id==219 || id==220)return 1;
    uint64_t pl=((uint64_t(*)(uint32_t))(b+0x45f9d0))(*(const uint32_t*)(b+0x2d20e4c)); if(!pl)return 1;
    uint8_t ver=*(uint8_t*)(pl+0x1bd); if(ver<1 || ver>3)return 1;
    uint64_t l=*(uint64_t*)(pl+0x100),n=l?*(uint64_t*)l:0;
    for(int k=0;n && k<1024;k++,n=*(uint64_t*)(n+8)) { uint64_t r=*(uint64_t*)n;
        if(!r || *(int16_t*)r!=(int16_t)id || *(int32_t*)(n+0x44)!=item)continue;
        int lv=*(int32_t*)(n+0x38); if(item==-1)lv+=((int(*)(uint64_t,uint64_t,int))(b+0x5b1710))(pl,n,0);
        int cap=**(int32_t**)(*(uint64_t*)(b+0x2d64a18+(uint64_t)ver*16)+0x1500); if(lv<0)lv=0; if(lv>cap)lv=cap;
        if(lv>0 || item!=-1)return lv>0;
        break; }

    { int dm_swapskill_equipped(int32_t),dm_swapskill_offhand_gives(int); return item!=-1 ? dm_swapskill_equipped(item) : dm_swapskill_offhand_gives((int)id); }
}
int dm_touch_game_skills(int *out,int capacity) {
    pthread_mutex_lock(&lock);
    const module_t *m=world_epoch?main_module:NULL;
    pthread_mutex_unlock(&lock);
    if(!m || !out || capacity<1)return 0;
    int n=0;
    for(int i=0;i<32;i++) {
        const unsigned char *e=(const unsigned char *)m->base+BINDING_OFFSET+i*DM_TG_STRIDE;
        uint32_t id;int32_t item;memcpy(&id,e,4);memcpy(&item,e+4,4);
        if(id>=512 || (live_check && !skill_live((const unsigned char *)m->base,id,item)))continue;
        int seen=0;for(int j=0;j<n;j++)if(out[j]==(int)id)seen=1;
        if(!seen && n<capacity)out[n++]=(int)id;
    }
    return n;
}
static void binding_getter(const uint64_t *regs) {
    const module_t *m=main_module;
    dg[0]++; diag_tick();
    if(!m || !regs || regs[10]!=(uint64_t)(m->base+BINDING_OFFSET) || regs[8]%DM_TG_STRIDE){dg[1]++;return;}
    uint64_t effective=regs[8]/DM_TG_STRIDE;
    uint32_t input_slot=(uint32_t)regs[19];
    if(input_slot>=DM_TG_SLOTS || effective>=28 || effective<input_slot){dg[2]++;return;}
    uint64_t selected_bank=effective-input_slot;
    if(selected_bank!=0 && selected_bank!=16){dg[3]++;return;}
#ifndef DM_TG_TEST_SYNTHETIC_IDENTITY
    if(regs[23]!=regs[19]) {dg[5]++;dg_id[4]++;return;}
#endif

    static unsigned dead_c[2]; static uint64_t dead_at[2]; uint64_t tn=live_check?now_ns():0; int bk=selected_bank!=0;
    unsigned dead=dead_c[bk]; int recheck=live_check && tn-dead_at[bk]>=200000000ull; if(recheck) { dead=0; dead_at[bk]=tn; }
    for(int i=0;recheck && i<DM_TG_SLOTS;i++) { uint32_t id;int32_t item;const unsigned char *e=(const unsigned char*)m->base+BINDING_OFFSET+(selected_bank+i)*DM_TG_STRIDE;memcpy(&id,e,4);memcpy(&item,e+4,4);
        static uint32_t gone[32]; int live=skill_live((const unsigned char*)m->base,id,item); if(!live)dead|=1u<<i;
        if(gone[selected_bank+i]!=(live?UINT32_MAX:id)) { gone[selected_bank+i]=live?UINT32_MAX:id; if(!live)LOG("技能钮：槽 %d 绑的技能 %u（物品 %d）玩家现在没有，按空位显示",(int)selected_bank+i,id,item); else LOG("技能钮：槽 %d 的技能回来了，照常显示",(int)selected_bank+i); } }
    if(recheck) dead_c[bk]=dead;
    pthread_mutex_lock(&lock);
    if(!scene_game || !world_epoch) {dg[4]++;pthread_mutex_unlock(&lock);return;}
    uint64_t caller=0,hud=0;
#ifndef DM_TG_TEST_SYNTHETIC_IDENTITY




    if(regs[23]!=regs[19]) {dg[5]++;dg_id[4]++;pthread_mutex_unlock(&lock);return;}
#endif
#ifdef DM_TG_TEST_SYNTHETIC_IDENTITY
    int scope=dm_touch_game_test_identity(regs,&caller,&hud);
#else
    int scope=dm_tg_hud_identity(&hud_cohort,regs,(uint64_t)m->base,scene_game,world_epoch,dm_tg_vm_read,&caller,&hud);
#endif
    if(scope==DM_TG_IDENTITY_UNCONFIRMED) {dg[5]++;if(dm_tg_identity_reason>=0&&dm_tg_identity_reason<8)dg_id[dm_tg_identity_reason]++;pthread_mutex_unlock(&lock);return;}
    unsigned char mapping[72],bindings[DM_TG_SLOTS*DM_TG_STRIDE]={0};
    memcpy(mapping,m->base+MAPPING_OFFSET,sizeof mapping);
    for(int i=0;i<DM_TG_SLOTS;i++)memcpy(bindings+i*DM_TG_STRIDE,m->base+BINDING_OFFSET+(selected_bank+i)*DM_TG_STRIDE,4);
    uint32_t observed_id;memcpy(&observed_id,m->base+BINDING_OFFSET+regs[8],4);
    dm_tg_snapshot s;
    dm_tg_parse(&s,mapping,sizeof mapping,bindings,sizeof bindings,(int)selected_bank,(int)input_slot,(int)effective,observed_id,now_ns());
    for(int i=0;i<DM_TG_SLOTS;i++)if(s.valid && (dead>>i&1) && !(s.portal.valid && s.portal.slot==i))s.raw_skill_id[i]=UINT32_MAX;
    s.scene_epoch=scene_epoch;s.world_epoch=world_epoch;
    dm_tg_group_observe(&group,&s,scene_game,scene_epoch,caller,hud,scope);latest=group.completed;dg[6]++;
    pthread_mutex_unlock(&lock);
}
int dm_touch_game_attach(void) {
    main_module=NULL;
    pthread_mutex_lock(&lock);int supported=gate_obs_present(OBS_BINDING_GETTER);memset(&latest,0,sizeof latest);
    latest.supported=supported;latest.scene_epoch=scene_epoch;latest.world_epoch=world_epoch;latest.portal.slot=-1;memset(&group,0,sizeof group);
    pthread_mutex_unlock(&lock);
    if(!supported)return 0;
    if(g_nmod<1 || strcmp(g_mod[0].name,"main") || !g_mod[0].base ||
       g_mod[0].size<BINDING_OFFSET+32*DM_TG_STRIDE || g_mod[0].size<MAPPING_OFFSET+72)return 0;
    main_module=&g_mod[0];gate_obs_set(OBS_BINDING_GETTER,binding_getter);return 1;
}
