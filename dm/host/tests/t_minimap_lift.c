


#include "../dm_cursor.c"
#include "../game_addrs.h"
module_t g_mod[MAX_MOD]; int g_nmod;
static int busy, ingame=1, uy=314; static uint64_t frame=100;
uint64_t dm_guest_frame(void) { return frame; }
uint64_t now_ns(void) { return frame*16666667; }
int dm_padmode_busy(void) { return busy; }
int dm_uiscreen_ingame(void) { return ingame; }
void dm_ui_rect(int *x,int *y,int *w,int *h) { *x=0; *y=uy; *w=2360; *h=1326; }
void dm_uiscreen_set_ingame(int on) { ingame=on; }
void dm_custom_on_uimsg(const char *cat,const char *act,const char *arg) { (void)cat; (void)act; (void)arg; }
void dm_elysium_arm(int on) { (void)on; }
static int checks,fails;
#define CHECK(c) do { checks++; if(!(c)) { fails++; printf("FAIL line %d: %s\n",__LINE__,#c); } } while(0)
static _Alignas(8) unsigned char root[160], maps[2][160], extra[160], hud[160], old[96][160], incoming[160];
static uint64_t children[97];
static uint64_t ptr(void *p) { return (uint64_t)(uintptr_t)p; }
static int32_t *y(void *p) { return (int32_t *)((unsigned char *)p+0x74); }
static void widget(void *v,const char *name,int y0) {
 unsigned char *p=v; memset(p,0,160); *(const char **)(p+8)=name;
 *(int32_t *)(p+0x74)=y0; *(int32_t *)(p+0x78)=1280; *(int32_t *)(p+0x7c)=720;
 *(int32_t *)(p+0x84)=-110; p[0x50]=1;
}
static void tick(void) { frame+=6; ui_lift_pump(ptr(root),frame); }
int main(void) {
 unsigned char *base=calloc(1, GA_UI_ROOT + 8); if(!base)return 2;
 g_nmod=1; strcpy(g_mod[0].name,"main"); g_mod[0].base=base;
 *(uint64_t *)(base+GA_UI_ROOT)=ptr(root); *(uint64_t *)(root+0x58)=ptr(children); *(uint64_t *)(root+0x60)=1;
 widget(maps[0],"AutoMap",78); children[0]=ptr(maps[0]); tick(); CHECK(*y(maps[0])==-433);




 for(int k=0;k<20;k++) {
  unsigned char *old=maps[k%2], *next=maps[(k+1)%2];
  mv_restore(ptr(root),MV_LIFT); CHECK(*y(old)==78);
  busy=1; tick(); CHECK(*y(old)==78);
  memcpy(next,old,160); children[0]=ptr(next); tick(); CHECK(*y(next)==78);
  busy=0; tick(); CHECK(*y(next)==-433);
  for(int j=0;j<3;j++)tick(); CHECK(*y(next)==-433);
 }
 mv_restore(ptr(root),MV_LIFT); CHECK(*y(maps[0])==78);



 tick(); CHECK(*y(maps[0])==-433 && g_nmv==1);
 *(uint64_t *)(root+0x60)=0; tick(); CHECK(g_nmv==1 && dm_wref_get(&g_mv[0].r)==0);
 CHECK(g_mv[0].r.w==ptr(maps[0]) && *y(maps[0])==-433);
 *(uint64_t *)(root+0x60)=1; tick(); tick(); CHECK(*y(maps[0])==-433 && g_nmv==1);
 mv_restore(ptr(root),MV_LIFT); CHECK(*y(maps[0])==78 && g_nmv==0);



 g_nmv=0; widget(maps[0],"AutoMap",78); children[0]=ptr(maps[0]); tick();
 widget(extra,"HUDWarnings",12); children[0]=ptr(extra); tick(); CHECK(*y(extra)==12); tick(); CHECK(*y(extra)==-499);
 tick(); CHECK(*y(extra)==-499); CHECK(g_nmv==1 && g_mv[0].val==-499);
 mv_restore(ptr(root),MV_LIFT); CHECK(*y(extra)==12);



 children[0]=ptr(extra); *y(extra)=24; tick(); CHECK(*y(extra)==-487);
 uy=157; tick(); CHECK(*y(extra)==-231); tick(); CHECK(*y(extra)==-231);
 uy=0; tick(); CHECK(*y(extra)==24 && mv_count(MV_LIFT)==0);
 uy=314; tick(); CHECK(*y(extra)==-487);
 ingame=0; tick(); CHECK(*y(extra)==24 && mv_count(MV_LIFT)==0);



 g_nmv=0; ingame=0; uy=314; busy=0;
 widget(maps[0],"AutoMap",78); widget(hud,"HUDPanel",0); *(int32_t *)(hud+0x84)=0;
 children[0]=ptr(maps[0]); children[1]=ptr(hud); *(uint64_t *)(root+0x60)=2;
 frame+=6; (void)dm_ui_gen(); dm_ui_gen_bump(DM_UIGEN_PERFMODE);
 ui_lift_pump(ptr(root),frame); CHECK(*y(maps[0])==78);
 dm_ui_on_uimsg("PanelManager","UnloadPanel","LoadScreenPanel");
 CHECK(ingame && atomic_load(&g_load_up)==0); CHECK(*y(maps[0])==-433 && mv_count(MV_LIFT)==1);
 tick(); dm_ui_gen_bump(DM_UIGEN_PANEL_GONE);
 dm_ui_on_uimsg("PanelManager","OpenPanel","LoadScreenPanel");
 CHECK(!ingame && atomic_load(&g_load_up)==1); CHECK(*y(maps[0])==78 && mv_count(MV_LIFT)==0);
 busy=1; dm_ui_on_uimsg("PanelManager","UnloadPanel","LoadScreenPanel"); CHECK(*y(maps[0])==78 && g_nmv==0);
 busy=0; dm_ui_gen_bump(DM_UIGEN_PERFMODE); ui_lift_pump(ptr(root),frame); CHECK(*y(maps[0])==78);
 tick(); CHECK(*y(maps[0])==-433);
 busy=1; dm_ui_on_uimsg("PanelManager","OpenPanel","LoadScreenPanel"); CHECK(*y(maps[0])==-433);
 busy=0; dm_ui_on_uimsg("PanelManager","OpenPanel","LoadScreenPanel"); CHECK(*y(maps[0])==78 && g_nmv==0);




 ingame=1; g_nmv=0; *(uint64_t *)(root+0x60)=1;
 widget(old[0],"AutoMap",78); children[0]=ptr(old[0]); mv_offset(MV_LIFT,ptr(old[0]),1,-511,0);
 widget(incoming,"AutoMap",12); children[0]=ptr(incoming); mv_offset(MV_LIFT,ptr(incoming),1,-511,0);
 CHECK(g_nmv==1 && g_mv[0].orig==12 && g_mv[0].val==-499);
 mv_offset(MV_LIFT,ptr(incoming),1,-511,0); CHECK(*y(incoming)==-499);
 mv_restore(ptr(root),MV_LIFT); CHECK(*y(incoming)==12 && g_nmv==0);

 *(uint64_t *)(root+0x60)=3;
 for(int i=0;i<3;i++) { widget(old[i],"AutoMap",100+i); children[i]=ptr(old[i]); }
 for(int i=0;i<3;i++) mv_offset(MV_LIFT,ptr(old[i]),1,-511,0);
 widget(incoming,"HUDWarnings",12); children[1]=ptr(incoming); mv_offset(MV_LIFT,ptr(incoming),1,-511,0);
 CHECK(g_nmv==3 && g_mv[2].r.w==ptr(incoming) && g_mv[2].val==-499);
 CHECK(*y(old[0])==-411 && *y(old[2])==-409 && *y(incoming)==-499);
 mv_offset(MV_LIFT,ptr(incoming),1,-511,0); CHECK(*y(incoming)==-499);
 mv_restore(ptr(root),MV_LIFT); CHECK(*y(old[0])==100 && *y(old[2])==102 && *y(incoming)==12 && g_nmv==0);

 *(uint64_t *)(root+0x60)=96;
 for(int i=0;i<96;i++) { widget(old[i],"AutoMap",100+i); children[i]=ptr(old[i]); }
 for(int i=0;i<96;i++) mv_offset(MV_LIFT,ptr(old[i]),1,-511,0);
 CHECK(g_nmv==96);



 widget(incoming,"HUDWarnings",12); children[96]=ptr(incoming); *(uint64_t *)(root+0x60)=97;
 mv_offset(MV_LIFT,ptr(incoming),1,-511,0); CHECK(*y(incoming)==12 && g_nmv==96);



 children[0]=ptr(incoming); *(uint64_t *)(root+0x60)=96;
 mv_offset(MV_LIFT,ptr(incoming),1,-511,0);
 CHECK(g_nmv==96 && g_mv[95].r.w==ptr(incoming) && g_mv[95].orig==12 && g_mv[95].val==-499);
 for(int i=1;i<96;i++) CHECK(*y(old[i])==100+i-511);
 mv_offset(MV_LIFT,ptr(incoming),1,-511,0); CHECK(*y(incoming)==-499);
 mv_restore(ptr(root),MV_LIFT); CHECK(*y(incoming)==12 && g_nmv==0);
 for(int i=1;i<96;i++) CHECK(*y(old[i])==100+i);
 free(base); printf("t_minimap_lift: %d checks, %d failures\n",checks,fails); return fails!=0;
}
