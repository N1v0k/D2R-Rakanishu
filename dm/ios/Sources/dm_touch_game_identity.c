#include "dm_touch_game_identity.h"
#include "../../host/game_addrs.h"
#include <string.h>
#if defined(__APPLE__) || defined(__ANDROID__) || defined(__OHOS__)
#include <mach/mach.h>
#ifdef __APPLE__
#include <mach/vm_map.h>
#endif
#endif
int dm_tg_vm_read(uint64_t address,void *out,size_t bytes) {
    if(!out || !bytes || !address || address>UINT64_MAX-bytes)return 0;
#if defined(__APPLE__) || defined(__ANDROID__) || defined(__OHOS__)
    vm_size_t copied=0;
    kern_return_t rc=vm_read_overwrite(mach_task_self(),(vm_address_t)address,(vm_size_t)bytes,(vm_address_t)out,&copied);
    return rc==KERN_SUCCESS && copied==bytes;
#else
    (void)address;return 0;
#endif
}

int dm_tg_identity_reason; uint64_t dm_tg_identity_lr; uint32_t dm_tg_identity_slot;
void dm_tg_hud_reset(dm_tg_hud_cohort *c,uint64_t world) {memset(c,0,sizeof(*c));c->world_epoch=world;}
static uint64_t add(uint64_t h,uint64_t v) {
    for(int i=0;i<8;i++){h^=(unsigned char)(v>>(i*8));h*=1099511628211ull;}return h;
}
int dm_tg_hud_identity(dm_tg_hud_cohort *c,const uint64_t *regs,uint64_t base,int game,uint64_t world,
                       dm_tg_read_fn read_fn,uint64_t *caller,uint64_t *hud) {
    dm_tg_identity_reason=1;
    if(!caller || !hud)return DM_TG_IDENTITY_UNCONFIRMED;
    *caller=*hud=0;
    if(!c || !regs || !read_fn)return DM_TG_IDENTITY_UNCONFIRMED;
    if(c->world_epoch!=world)dm_tg_hud_reset(c,world);
    uint32_t arg=(uint32_t)regs[19];
    dm_tg_identity_reason=3;
    if(!game || !world || arg>=12 || base>UINT64_MAX-GA_AT_HUD_BIND_GET_RET)return DM_TG_IDENTITY_UNCONFIRMED;
    dm_tg_identity_reason=4;

    uint64_t fp=regs[29],frame[3]={0},lr=0,widget=0;
    if(fp<8 || fp>UINT64_MAX-16 || !read_fn(fp-8,frame,sizeof frame))return DM_TG_IDENTITY_UNCONFIRMED;
    widget=frame[0];lr=frame[2];dm_tg_identity_lr=lr-base;
    if(lr!=base+GA_AT_HUD_BIND_GET_RET || !widget || widget>UINT64_MAX-0xc90)return DM_TG_IDENTITY_UNCONFIRMED;

    uint32_t slot=UINT32_MAX;unsigned char always_alt=255;static unsigned char span[0xc91-0xb18];                                                                                                                                                                       dm_tg_identity_reason=5;
    if(read_fn(widget+0xb18,span,sizeof span)){memcpy(&slot,span,4);always_alt=span[0xc90-0xb18];}
    if(slot>=6 || always_alt ||
       (arg!=slot && arg!=slot+6)) { dm_tg_identity_slot=slot; return DM_TG_IDENTITY_UNCONFIRMED; }
    dm_tg_identity_reason=6; dm_tg_identity_slot=slot;
    if(c->widget[slot] && c->widget[slot]!=widget)return DM_TG_IDENTITY_UNCONFIRMED;
    for(unsigned i=0;i<6;i++)if(i!=slot && c->widget[i]==widget)return DM_TG_IDENTITY_UNCONFIRMED;
    if(c->confirmed){*caller=lr;*hud=c->hud_identity;dm_tg_identity_reason=0;return DM_TG_IDENTITY_NATIVE;}
    dm_tg_identity_reason=7;
    c->widget[slot]=widget;c->seen|=1u<<slot;
    if(c->seen==63) {
        uint64_t h=add(1469598103934665603ull,world);for(int i=0;i<6;i++)h=add(h,c->widget[i]);
        c->hud_identity=h?h:1;c->confirmed=1;
    }

    return DM_TG_IDENTITY_UNCONFIRMED;
}
