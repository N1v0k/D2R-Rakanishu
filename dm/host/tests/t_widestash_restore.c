



#include "../dm_cursor.c"
module_t g_mod[MAX_MOD]; int g_nmod;
uint64_t dm_guest_frame(void) { return 100; }
uint64_t now_ns(void) { return 0; }
int dm_padmode_busy(void) { return 0; }
int dm_uiscreen_ingame(void) { return 1; }
void dm_ui_rect(int *x,int *y,int *w,int *h) { *x=0; *y=0; *w=2360; *h=1326; }
void dm_uiscreen_set_ingame(int on) { (void)on; }
void dm_custom_on_uimsg(const char *cat,const char *act,const char *arg) { (void)cat; (void)act; (void)arg; }
void dm_elysium_arm(int on) { (void)on; }
static int checks,fails;
#define CHECK(c) do { checks++; if(!(c)) { fails++; printf("FAIL line %d: %s\n",__LINE__,#c); } } while(0)
static _Alignas(8) unsigned char root[160], inv[160];
static uint64_t children[1];
static uint64_t ptr(void *p) { return (uint64_t)(uintptr_t)p; }
static int32_t *x(void) { return (int32_t *)(inv+0x70); }
enum { HOME = -8, SHIFT = 227, ALONE = -816 };
int main(void) {
 unsigned char *base=calloc(1,0x2f05460); if(!base)return 2;
 g_nmod=1; strcpy(g_mod[0].name,"main"); g_mod[0].base=base;
 *(uint64_t *)(base+0x2f05458)=ptr(root); *(uint64_t *)(root+0x58)=ptr(children); *(uint64_t *)(root+0x60)=1;
 *(const char **)(inv+8)="PlayerInventoryExpansionLayout"; *x()=HOME; inv[0x50]=1; children[0]=ptr(inv);

 mv_offset(MV_WIDE,ptr(inv),0,SHIFT,0); CHECK(*x()==HOME+SHIFT);
 mv_restore(ptr(root),MV_WIDE); CHECK(*x()==HOME && mv_count(MV_WIDE)==0);
 for(int k=0;k<4;k++) {

  mv_offset(MV_WIDE,ptr(inv),0,SHIFT,0); CHECK(*x()==HOME+SHIFT);
  *x()=ALONE;
  mv_restore(ptr(root),MV_WIDE); CHECK(*x()==ALONE);
  mv_restore(ptr(root),MV_WIDE); CHECK(*x()==ALONE);

  *x()=HOME+SHIFT;
  mv_restore(ptr(root),MV_WIDE); CHECK(*x()==HOME && mv_count(MV_WIDE)==0);
 }

 mv_offset(MV_WIDE,ptr(inv),0,SHIFT,0); *x()=ALONE; mv_restore(ptr(root),MV_WIDE);
 *x()=40; mv_offset(MV_WIDE,ptr(inv),0,SHIFT,0); CHECK(*x()==40+SHIFT);
 mv_restore(ptr(root),MV_WIDE); CHECK(*x()==40 && mv_count(MV_WIDE)==0);
 free(base); printf("t_widestash_restore: %d checks, %d failures\n",checks,fails); return fails!=0;
}
