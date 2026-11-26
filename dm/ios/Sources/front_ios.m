#include <math.h>
#include "../../host/game_addrs.h"






#import <UIKit/UIKit.h>
static double g_dm_brightness = -1;
double dm_brightness_now(void) { return g_dm_brightness; }
static volatile int g_dm_thermal = -1;
int dm_thermal_now(void) { return g_dm_thermal; }
#import <QuartzCore/CAMetalLayer.h>
#import <Metal/Metal.h>
#include <signal.h>
#include "front.h"
#include "dm_input.h"
void dm_gamepad_start(void (*on_mode)(int pad)); void dm_gamepad_touched(void);
#include "hle.h"
#import "DmTouchOverlay.h"
#include "dm_touch_native_art.h"
#include "dm_actions.h"
#include <stdatomic.h>
#include "dm_touch_game_state.h"
#include "../../host/dm_cursor.h"
#include "../../host/dm_hud.h"
#include "../../host/dm_world.h"
#include "../../host/dm_uiscreen.h"
#include "../../host/dm_actions.h"
#include "../../host/dm_i18n.h"
static NSString *SWT(const char *k) { return @(dm_ui(k)); }
static _Atomic int g_touch_frontend=1;
static void hud_stack_watch(void);


__attribute__((weak)) int dm_touch_game_attach(void) { return 0; }
__attribute__((weak)) int dm_touch_game_snapshot(dm_tg_snapshot *out) {
    if(out) { memset(out,0,sizeof(*out)); out->portal.slot=-1; }
    return 0;
}

__attribute__((weak)) void dm_touch_game_set_scene(int game,uint64_t epoch,uint64_t world) { (void)game; (void)epoch; (void)world; }
__attribute__((weak)) uint64_t dm_tg_portal_mask(const dm_tg_snapshot *s,uint64_t n) {
    (void)s; (void)n; return 0;
}

static CAMetalLayer *g_layer;



static BOOL touch_control_log_enabled(void) {
    static int enabled=-1;
    if(enabled<0) enabled=getenv("DM_TOUCH_CONTROL_LOG")!=NULL;
    return enabled;
}










@interface DmMetalView : UIView
- (void)cancelControls;
- (void)tickTouchControls:(double)now;
- (void)navigationMode:(int)navigation;
- (void)hudBlocked:(int)n;
- (void)frontendMode:(BOOL)fe;
- (void)padMode:(BOOL)pad;
- (void)restartCover:(int)mode;
- (void)captureTouchLayer:(NSString *)path;
- (void)refreshPortalBinding;
@end
static __weak DmMetalView *g_touch_view;




static int g_full;
static _Atomic uint64_t g_res_now;
static uint64_t g_res_cap;
static CGRect world_rect(UIView *v) { UIView *b = v.superview; int x, y, w, h, rw = 0, rh = 0; if (!g_full || !b) return v.bounds; dm_ui_rect(&x, &y, &w, &h);
    return dm_screen_real(&rw, &rh) && h > 0 && h < rh ? [v convertRect:b.bounds fromView:b] : v.bounds; }
static void world_norm(UIView *v, CGFloat x, CGFloat y, float *nx, float *ny) { CGRect r = world_rect(v); *nx = (float)((x - r.origin.x) / r.size.width); *ny = (float)((y - r.origin.y) / r.size.height); }


@interface DmScriptTouch : UITouch
@property (nonatomic) CGPoint scriptPoint;
@property (nonatomic) UITouchPhase scriptPhase;
@end
@interface DmVolumeWatch : NSObject
@end
@implementation DmVolumeWatch
- (void)observeValueForKeyPath:(NSString *)k ofObject:(id)o change:(NSDictionary *)c context:(void *)x {
    (void)k; (void)o; (void)x; double mtl_link_tick_age_ms(void);
    LOG("前端：系统音量 %.2f → %.2f（显示节拍上次回调 %.0f ms 前）", [c[NSKeyValueChangeOldKey] doubleValue], [c[NSKeyValueChangeNewKey] doubleValue], mtl_link_tick_age_ms());
}
@end
@implementation DmScriptTouch
- (CGPoint)locationInView:(UIView *)v { (void)v; return _scriptPoint; }
- (CGPoint)previousLocationInView:(UIView *)v { (void)v; return _scriptPoint; }
- (NSTimeInterval)timestamp { return CACurrentMediaTime(); }
- (UITouchPhase)phase { return _scriptPhase; }
@end
static void touch_script_start(void);
static _Atomic int g_touch_navigation = 1;

static _Atomic int g_nav_pub; int dm_front_navigation(void) { return atomic_load(&g_nav_pub); }
static void pad_mode_changed(int pad) { dispatch_async(dispatch_get_main_queue(),^{ [g_touch_view padMode:pad]; }); }
static void restart_cover_hook(int on) { dispatch_async(dispatch_get_main_queue(),^{ [g_touch_view restartCover:on]; }); }




@interface DmSwallowView : UIView
@end
@implementation DmSwallowView
- (void)touchesBegan:(NSSet<UITouch *> *)t withEvent:(UIEvent *)e {}
- (void)touchesMoved:(NSSet<UITouch *> *)t withEvent:(UIEvent *)e {}
- (void)touchesEnded:(NSSet<UITouch *> *)t withEvent:(UIEvent *)e {}
- (void)touchesCancelled:(NSSet<UITouch *> *)t withEvent:(UIEvent *)e {}
@end
@implementation DmMetalView {
    UITouch *_slot[16];
    dm_tc_state _controls;
    DmTouchOverlay *_overlay;
    UIView *_info;
    NSHashTable<UITouch *> *_swallow;
    BOOL _padMode, _feMode;
    UIView *_rsCover;
    int _navigation;
    CALayer *_mhPtr, *_thr[8];

    CALayer *_mgLabel[DM_MAP_GOAL_MAX], *_mgMark[DM_MAP_GOAL_MAX];
    NSMutableDictionary<NSString *,id> *_mgImg;
    uint64_t _modifierPublishEndNS,_modifierPublishEpoch;
    uint64_t _touchEventSeq,_touchReceiptSeq,_touchPublishedEventSeq;
    UITouch *_pendEnd; double _pendEndTime,_pendWall; BOOL _pendFire,_finalAsked;
    int32_t _minLeft,_maxLeft; uint64_t _lastButtons; unsigned _touchAPresses,_touchShots; UITouch *_uiFinger; CGPoint _uiFeO; BOOL _uiFeDrag; BOOL _uiDragOn; UITouch *_mapTouch, *_bindTouch; UIView *_ghostDim; BOOL _ghostOn; uint64_t _bindKey; uint32_t _selId; double _statNext; double _tipUntil, _addAt; unsigned _mapWorldSeq; BOOL _mapWorldAsked; CGPoint _mapWorldPt; CGPoint _mapP0, _bindP0; double _mapT0; BOOL _pulseTicking; uint64_t _pulseEpoch,_pulseS0; double _pulseT0,_pulseSeen,_pulseFree; unsigned _itemSeq,_relSeq; BOOL _relAsked,_itemAsked; int _itemKind;
}
+ (Class)layerClass { return CAMetalLayer.class; }
- (instancetype)initWithFrame:(CGRect)f {
    if (!(self = [super initWithFrame:f])) return nil;
    self.multipleTouchEnabled = YES;
    self.isAccessibilityElement=YES; self.accessibilityIdentifier=@"dm_touch_surface"; self.accessibilityLabel=SWT("dmA11ySurface");
    dm_tc_init(&_controls,f.size.width,f.size.height); _controls.mouse_click_mode=1;
    _overlay=[[DmTouchOverlay alloc] initWithFrame:self.bounds]; _overlay.controls=&_controls; _overlay.hidden=YES;
    [self addSubview:_overlay]; _info=[[UIView alloc] initWithFrame:self.bounds]; _info.userInteractionEnabled=NO; _info.hidden=YES; [self addSubview:_info]; g_touch_view=self; dm_gamepad_start(pad_mode_changed);
    _rsCover=[[DmSwallowView alloc] initWithFrame:self.bounds]; _rsCover.backgroundColor=UIColor.blackColor; _rsCover.hidden=YES; _rsCover.userInteractionEnabled=YES;
    { extern void (*g_dm_restart_cover_hook)(int); g_dm_restart_cover_hook=restart_cover_hook; }
    _navigation=0; [self navigationMode:atomic_load(&g_touch_navigation)];
    return self;
}
- (void)safeAreaInsetsDidChange { [super safeAreaInsetsDidChange]; [self setNeedsLayout]; }
- (void)layoutSubviews {
    [super layoutSubviews];
    CAMetalLayer *l=(CAMetalLayer *)self.layer;
    if(l==g_layer) l.drawableSize=CGSizeMake(self.bounds.size.width*l.contentsScale,self.bounds.size.height*l.contentsScale);
    UIEdgeInsets si=self.safeAreaInsets; static CGFloat sl=-1,sr=-1,sb=-1; BOOL safeChanged=(si.left!=sl || si.right!=sr || si.bottom!=sb);
    if (safeChanged) { sl=si.left; sr=si.right; sb=si.bottom; dm_tc_set_safe_bottom((float)si.bottom); dm_tc_set_safe_insets((float)si.left,(float)si.right); }


    { const char *tb=getenv("DM_TEST_EDGE_B"); dm_tc_set_edge_bottom(tb ? (float)atof(tb) : UIDevice.currentDevice.userInterfaceIdiom==UIUserInterfaceIdiomPad ? (float)si.bottom : 0); }
    { const char *tl=getenv("DM_TEST_SAFE_L"); CGFloat l=tl ? atof(tl) : si.left, hh=self.bounds.size.height; if(hh>0) dm_ui_safe_left((int)lround(l/hh*2160)); }
    { const char *tj=getenv("DM_TEST_JOY_ISO"); if(tj) dm_tc_set_iso_fix(atoi(tj)); }
    { const char *ts=getenv("DM_TEST_SAFE_R"); CGFloat r=ts ? atof(ts) : si.right, hh=self.bounds.size.height; if(hh>0) dm_ui_safe_right((int)lround(r/hh*2160)); }

    static int tall=-1; int t=UIDevice.currentDevice.userInterfaceIdiom==UIUserInterfaceIdiomPad || fmin(self.bounds.size.width,self.bounds.size.height)>=600; BOOL tallChanged=t!=tall;
    if (tallChanged) { tall=t; dm_tc_set_tall_scale(t); }
    if (safeChanged || tallChanged || _controls.layout.width!=self.bounds.size.width || _controls.layout.height!=self.bounds.size.height) {
        [self cancelControlsFrom:"layout"]; dm_tc_resize(&_controls,self.bounds.size.width,self.bounds.size.height);
    }
    _overlay.frame=self.bounds; _info.frame=self.bounds; [_overlay setNeedsDisplay]; if(!_rsCover.hidden && self.window) _rsCover.frame=self.window.bounds;
}
- (void)restartCover:(int)mode {
    UIView *host=self.window ?: self; if(mode){ if(_rsCover.superview!=host) [host addSubview:_rsCover]; _rsCover.frame=host.bounds; [host bringSubviewToFront:_rsCover]; }
    _rsCover.backgroundColor = mode==1 ? UIColor.blackColor : UIColor.clearColor; _rsCover.hidden=!mode;
    LOG("前端：一键重开遮罩%s（挂在%s，%.0fx%.0f）", mode==1 ? "黑屏盖上" : mode==2 ? "改透明、仍吞触点" : "撤掉", host==self ? "游戏视图" : "窗口", host.bounds.size.width, host.bounds.size.height); }
- (void)hudBlocked:(int)n { _overlay.hudBlocked=n>0; _overlay.dialogOnly=n<0; dm_tc_set_joy_block(&_controls,n<0); [_overlay setNeedsDisplay]; }


- (void)padMode:(BOOL)pad { _padMode=pad; _overlay.hidden=_feMode || pad; _info.hidden=_feMode; if(pad) { _tipUntil=-2; dm_tc_cancel_all(&_controls); [self pushFrom:"padmode" phase:"none"]; } }
- (void)frontendMode:(BOOL)fe { _feMode=fe; _overlay.hidden=fe || _padMode; _info.hidden=fe; if(fe) { _pendEnd=nil; _finalAsked=NO; for(int i_=0;i_<16;i_++) _slot[i_]=nil; dm_tc_cancel_all(&_controls); [self pushFrom:"frontend" phase:"none"]; } }
- (void)navigationMode:(int)n {
    if (_navigation==n) return;
    dm_tc_set_navigation_mode(&_controls,n); _navigation=n; atomic_store(&g_nav_pub,n);
    static int pad=-1; if(pad<0) { const char *e=getenv("DM_TOUCH_NAV_PAD"); pad=e && e[0]=='1'; }
    uint32_t mask=n ? (dm_tc_nav_mask(pad && n!=2) | (n==3 ? 1u<<DM_TC_MENU : 0)) :
        (((1u<<DM_TC_COUNT)-1) & ~(1u<<DM_TC_MAP));
    dm_tc_enable_targets(&_controls,mask); _overlay.navigationMode=n!=0; [self pushFrom:"navigation" phase:"none"];
    if(!n && !getenv("DM_TOUCH_SCRIPT_START")) touch_script_start();
    if(!n) [self itemLabelsOn];
    dm_ui_map_hold(n!=0);
}








- (void)itemLabelsOn {
    static int done, armed; const char *e=getenv("DM_TOUCH_DEFAULTS");
    if(done || armed || (e && !strcmp(e,"0"))) return;
    armed=1;
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW,(int64_t)(1.5*NSEC_PER_SEC)),dispatch_get_main_queue(),^{
        armed=0; if(self->_navigation || done) return;


        const char *tc=getenv("DM_TEST_LKEY_CLASH"); void dm_ui_key_pulse(uint64_t,int); void dm_ui_key_pulse_alone(uint64_t,int); done=1;
        if(tc && !strcmp(tc,"old")) { dm_ui_key_pulse(DM_BTN_PLUS,6); LOG("测试 DM_TEST_LKEY_CLASH=old：先按菜单键，33 毫秒后 L 不经裁决直接按出去（改前的做法）");
            dispatch_after(dispatch_time(DISPATCH_TIME_NOW,(int64_t)(0.033*NSEC_PER_SEC)),dispatch_get_main_queue(),^{ dm_ui_key_pulse(DM_BTN_L,9); }); return; }
        dm_ui_key_pulse_alone(DM_BTN_L,9);
        if(tc && !strcmp(tc,"pad")) { void dm_gamepad_inject(uint64_t,int32_t,int32_t,int32_t,int32_t); dm_gamepad_inject(DM_BTN_PLUS,0,0,0,0); LOG("测试 DM_TEST_LKEY_CLASH=pad：同一刻由虚拟手柄按菜单键（开物品栏）");
            dispatch_after(dispatch_time(DISPATCH_TIME_NOW,(int64_t)(0.12*NSEC_PER_SEC)),dispatch_get_main_queue(),^{ dm_gamepad_inject(0,0,0,0,0); }); }
        else if(tc) { dm_ui_key_pulse(DM_BTN_PLUS,6); LOG("测试 DM_TEST_LKEY_CLASH：同一刻替玩家按菜单键（开物品栏）"); }
    });
}
- (void)recordTouch:(UITouch *)touch slot:(int)slot phase:(const char *)phase {
    if(!touch_control_log_enabled()) return;
    CGPoint p=[touch locationInView:self];
    fprintf(stderr,"[touch-receipt] stage=event eventseq=%llu mono_ns=%llu source=uitouch phase=%s slot=%d touch=%p touch_timestamp_s=%.9f x=%.6f y=%.6f buttons=%#llx lx=%d ly=%d rx=%d ry=%d mode=%s\n",
        (unsigned long long)++_touchEventSeq,(unsigned long long)now_ns(),phase,slot,(__bridge void *)touch,touch.timestamp,p.x,p.y,
        (unsigned long long)_controls.buttons,_controls.lx,_controls.ly,_controls.rx,_controls.ry,_navigation?"navigation":"game");
}
- (void)pushFrom:(const char *)source phase:(const char *)phase {
    dm_touch ts[16]; int n=0; CGSize b=self.bounds.size;
    if (b.width>0 && b.height>0) for(int i=0;i<16;i++) if(_slot[i]) {
        CGPoint p=[_slot[i] locationInView:self];
        ts[n++]=(dm_touch){(uint32_t)i,(float)fmin(fmax(p.x/b.width,0),.99999),(float)fmin(fmax(p.y/b.height,0),.99999)};
    }
    dm_touch_set(ts,n);
    BOOL diagnostic=touch_control_log_enabled();
    uint64_t publishStart=diagnostic?now_ns():0;


    if (_controls.pointer_valid) dm_cursor_set_screen(_controls.pointer_x,_controls.pointer_y,b.width,b.height); else dm_cursor_clear();
    dm_input_set(_controls.buttons,_controls.lx,_controls.ly,_controls.rx,_controls.ry);
    if (_controls.belt_request) { int br=_controls.belt_request; _controls.belt_request=0;
        if (br==1 || br==2) dm_belt_expand(br==1); else dm_belt_use(br&0xf, br>>4&0xf); }
    if (_controls.diff_request) { dm_hud_request_players(_controls.diff_request); _controls.diff_request=0; }
    uint64_t publishEnd=(diagnostic || _controls.portal_pending)?now_ns():0;
    if(_controls.portal_pending && _modifierPublishEpoch!=_controls.gesture_epoch) {
        _modifierPublishEpoch=_controls.gesture_epoch;_modifierPublishEndNS=publishEnd;
    }
    if(!_controls.portal_phase) {_modifierPublishEpoch=0;_modifierPublishEndNS=0;}
    _minLeft=MIN(_minLeft,_controls.lx); _maxLeft=MAX(_maxLeft,_controls.lx);
    if((_controls.buttons & DM_BTN_A) && !(_lastButtons & DM_BTN_A))_touchAPresses++;
    _lastButtons=_controls.buttons;
    self.accessibilityValue=[NSString stringWithFormat:@"%@|buttons=%llu|lx=%d|ly=%d|min=%d|max=%d|a=%u|shot=%u",_navigation?@"navigation":@"game",_controls.buttons,_controls.lx,_controls.ly,_minLeft,_maxLeft,_touchAPresses,_touchShots];
    if(diagnostic) {
        uint64_t first=_touchEventSeq>_touchPublishedEventSeq?_touchPublishedEventSeq+1:0;
        fprintf(stderr,"[touch-receipt] stage=output receiptseq=%llu eventseq=%llu event_first_seq=%llu event_last_seq=%llu mono_ns=%llu publish_end_ns=%llu source=%s phase=%s buttons=%#llx lx=%d ly=%d rx=%d ry=%d mode=%s portal_request=%llu portal_binding=%llu portal_scene=%llu portal_phase=%d portal_pending=%d modifier_publish_end_ns=%llu feedback_group_start_ns=%llu feedback_captured_ns=%llu feedback_secondary=%d feedback_seen=%u feedback_identity_scope=%d\n",
        (unsigned long long)++_touchReceiptSeq,(unsigned long long)_touchEventSeq,(unsigned long long)first,(unsigned long long)_touchEventSeq,(unsigned long long)publishStart,
        (unsigned long long)publishEnd,source,phase,(unsigned long long)_controls.buttons,_controls.lx,_controls.ly,_controls.rx,_controls.ry,_navigation?"navigation":"game",
        (unsigned long long)_controls.gesture_epoch,(unsigned long long)_controls.portal_binding_identity,(unsigned long long)_controls.portal_scene_epoch,
        _controls.portal_phase,_controls.portal_pending,(unsigned long long)_modifierPublishEndNS,
        (unsigned long long)_overlay.nativeMapping.group_started_ns,(unsigned long long)_overlay.nativeMapping.captured_ns,
        _overlay.nativeMapping.secondary_group,_overlay.nativeMapping.group_seen,_overlay.nativeMapping.identity_scope);
        _touchPublishedEventSeq=_touchEventSeq;
    }
    [_overlay controlsChanged];
}
- (void)cancelControlsFrom:(const char *)source {
    _pendEnd=nil; _finalAsked=NO;
    for(int i_=0;i_<16;i_++) _slot[i_]=nil; dm_tc_cancel_all(&_controls); [self pushFrom:source phase:"none"];
    if(_uiDragOn) { _uiDragOn=NO; dm_ui_drag_end(); }
    _mapTouch=_bindTouch=nil;
}












- (uint64_t)skillKeyAt:(CGPoint)p empty:(BOOL *)empty {
    int i=dm_tc_hit(&_controls.layout,(float)p.x,(float)p.y);
    if((i==DM_TC_MAIN || (i>=DM_TC_SKILL1 && i<=DM_TC_SKILL4)) && (_controls.enabled_targets&(1u<<i))) { const dm_tc_target *t=&_controls.layout.target[i];
        dm_tg_snapshot n=_overlay.nativeMapping; uint64_t now_ns(void);
        dm_ta_glyph g=dm_ta_current_set(&n,t->button,now_ns(),_controls.skill_set); *empty=g.status==DM_TA_EMPTY;
        return t->button|(_controls.skill_set?DM_BTN_ZL:0); }
    return 0;
}












- (BOOL)minimapRect:(CGRect *)whole tap:(CGRect *)tap {
    float r[4]; CGSize z=self.bounds.size; if(z.width<=0 || !dm_ui_minimap_rect(r)) return NO;
    CGFloat x0=r[0]*z.width, y0=r[1]*z.height, w=(r[2]-r[0])*z.width, h=(r[3]-r[1])*z.height, band=h/4; if(band<30) band=MIN(30,h*0.4);
    if(whole) *whole=CGRectMake(x0,y0,w,h); if(tap) *tap=CGRectMake(x0,y0,w,band); return YES;
}
- (BOOL)inMinimap:(CGPoint)p {
    CGRect tap; if(_navigation || ![self minimapRect:NULL tap:&tap]) return NO;
    int control=dm_tc_hit(&_controls.layout,(float)p.x,(float)p.y);
    if(control>=0 || control==DM_TC_SWAP) return NO;
    return CGRectContainsPoint(tap,p);
}
- (void)cancelControls { [self cancelControlsFrom:"focus_cancel"]; }
- (void)refreshPortalBinding {
    int dm_custom_get(const char *key);int above=dm_custom_get("touch_swap_above")!=0;
    if (_controls.layout.swap_above!=above) {
        [self cancelControlsFrom:"layout"];dm_tc_set_swap_above(above);
        dm_tc_resize(&_controls,self.bounds.size.width,self.bounds.size.height);[_overlay setNeedsDisplay];
    }
    hud_stack_watch();
    { dm_hud_info h, o=_overlay.hud; dm_hud_get(&h); h.x=h.y=0; if (memcmp(&h,&o,sizeof h)) _overlay.hud=h; }
    dm_tg_snapshot snapshot={0};
    dm_touch_game_snapshot(&snapshot);
    _overlay.nativeMapping=snapshot;
    if(_navigation) return;
    if(!snapshot.supported) return;
    uint64_t mask=0; (void)dm_tg_portal_mask;
    if(_controls.layout.target[DM_TC_PORTAL].button!=mask || _controls.portal_binding_identity!=snapshot.binding_identity || _controls.portal_scene_epoch!=snapshot.scene_epoch) {
        dm_tc_set_portal_binding(&_controls,mask,snapshot.binding_identity,snapshot.scene_epoch);
        _modifierPublishEpoch=0;_modifierPublishEndNS=0;


        [self pushFrom:"bindings" phase:"none"];
    }
    if(_controls.portal_pending && mask && snapshot.ready && snapshot.secondary_group && snapshot.group_seen==63 &&
       _modifierPublishEpoch==_controls.gesture_epoch && _modifierPublishEndNS &&
       snapshot.group_started_ns>_modifierPublishEndNS && snapshot.captured_ns>_modifierPublishEndNS &&
       dm_tc_portal_ack(&_controls,_modifierPublishEpoch,snapshot.binding_identity,snapshot.scene_epoch))
        [self pushFrom:"portal_sequence" phase:"none"];

}


- (void)bindTapSync { float nx=0,ny=0; CGSize z=self.bounds.size;




    { static int was; int st=dm_ui_bind_active(); if(st==2 && was!=2 && z.width>0) { _controls.pointer_valid=1; _controls.pointer_x=0.155f*(float)z.width; _controls.pointer_y=0.5f*(float)z.height; } was=st; }
    int hk=dm_ui_bind_skill_hit(&nx,&ny); _controls.tap_button=0; _controls.tap_move=0;
    if(!hk) _selId=0;
    if(hk) { _controls.tap_x=nx*(float)z.width; _controls.tap_y=ny*(float)z.height;
        if(hk==1 && dm_ui_bind_active()==2) _controls.tap_button=DM_BTN_Y;
        else if(!dm_ui_bind_active()) _controls.tap_move=1; } }






- (void)shopGhostSync {
    void mtl_ui_ghost(int on, float px, float py, const float grid[4], float cx, float cy); int mtl_ui_ghost_rect(float r[4]);
    const dm_tc_finger *f = (_controls.navigation_mode && _controls.pointer_slot>=0) ? &_controls.finger[_controls.pointer_slot] : NULL;
    BOOL drag = f && f->active && f->shop_drag;
    if(!drag) { if(_ghostOn) { mtl_ui_ghost(0,0,0,NULL,0,0); [_ghostDim removeFromSuperview]; _ghostDim=nil; _ghostOn=NO; } return; }
    CGSize z=self.bounds.size; if(z.width<=0) return;






    float g[4]; if(!dm_ui_shelf_grid(g)) return;
    float r[4]; BOOL have=mtl_ui_ghost_rect(r);
    mtl_ui_ghost(1,(float)(f->aim_origin_x/z.width),(float)(f->aim_origin_y/z.height),g,(float)(f->x/z.width),(float)((f->y-DM_TC_POINTER_LIFT_PT*dm_tc_scale_for_size(z.width,z.height))/z.height));
    if(!_ghostOn) { _ghostOn=YES; LOG("前端：货架拖货跟手图 开（按下点 %.0f,%.0f）",f->aim_origin_x,f->aim_origin_y); }
    if(have && !_ghostDim && (_itemKind!=8 || dm_ui_stack_left()==1)) { _ghostDim=[[UIView alloc] initWithFrame:CGRectMake(r[0]*z.width,r[1]*z.height,(r[2]-r[0])*z.width,(r[3]-r[1])*z.height)];
        _ghostDim.backgroundColor=[UIColor colorWithWhite:0 alpha:.82]; _ghostDim.userInteractionEnabled=NO; [self addSubview:_ghostDim]; }
}










- (UIImage *)mgImage:(NSString *)name {
    id hit=_mgImg[name]; if(hit) return hit==NSNull.null ? nil : hit; if(!_mgImg) _mgImg=[NSMutableDictionary dictionary];
    UIImage *im=[UIImage imageWithContentsOfFile:[NSBundle.mainBundle.resourcePath stringByAppendingPathComponent:[@"mapgoal/" stringByAppendingString:name]]];
    _mgImg[name]=im ?: NSNull.null; return im;
}
- (void)mapGoalSync {
    static int old=-1; if(old<0) { int dm_custom_get(const char *key); old=dm_custom_get("item_notes")!=0; }
    dm_map_goal_view v={0}; float cell=0; int mode=dm_ui_map_geom(&cell), n=dm_map_goals(&v); int rw=0,rh=0; dm_screen_real(&rw,&rh);
    CGRect wr=world_rect(self); float mm[4]; BOOL haveMini=dm_ui_minimap_rect(mm);
    BOOL show=mode && n>0 && !_navigation && !_info.hidden && rw>0 && cell>0 && (mode==2 || haveMini);

    { static struct { dm_map_goal_view v; float cell, mm[4]; int mode, rw, rh, show, lg; CGRect wr; CGSize z; } last, cur; memset(&cur,0,sizeof cur);
      cur.v=v; cur.cell=cell; if(haveMini) memcpy(cur.mm,mm,sizeof mm); cur.mode=mode; cur.rw=rw; cur.rh=rh; cur.show=show; cur.lg=dm_lang_group(); cur.wr=wr; cur.z=self.bounds.size;
      if(_mgLabel[0] && !memcmp(&cur,&last,sizeof cur)) return; last=cur; }
    [CATransaction begin]; [CATransaction setDisableActions:YES];
    CGSize z=self.bounds.size; CGFloat k=wr.size.width/rw, c=cell*k, H=wr.size.height, lh=H*0.0236, sh=H*0.085*(mode==1?0.6:1);
    CGPoint o; CGRect area;
    if(mode==2) { o=CGPointMake(wr.origin.x+v.sx*wr.size.width, wr.origin.y+v.sy*wr.size.height); area=CGRectMake(wr.origin.x+wr.size.width*0.04, wr.origin.y+H*0.06, wr.size.width*0.92, H*0.72); }
    else { area=CGRectMake(mm[0]*z.width, mm[1]*z.height, (mm[2]-mm[0])*z.width, (mm[3]-mm[1])*z.height); o=CGPointMake(CGRectGetMidX(area),CGRectGetMidY(area)); area=CGRectInset(area,area.size.width*0.06,area.size.height*0.08); }
    CGRect placed[DM_MAP_GOAL_MAX]; int np=0;
    for(int i=0;i<DM_MAP_GOAL_MAX;i++) {
        if(!_mgLabel[i]) { _mgLabel[i]=[CALayer layer]; _mgMark[i]=[CALayer layer]; for(CALayer *l in @[_mgLabel[i],_mgMark[i]]) { l.contentsGravity=kCAGravityResizeAspect; l.hidden=YES; [_info.layer addSublayer:l]; } }
        if(!show || i>=n) { _mgLabel[i].hidden=YES; _mgMark[i].hidden=YES; continue; }
        int dx=v.x[i]-v.px, dy=v.y[i]-v.py; CGPoint p=CGPointMake(o.x+(dx-dy)*1.6*c, o.y+(dx+dy)*0.8*c);
        if(v.art[i]>=DM_MG_WP && dx*dx+dy*dy<144) { _mgLabel[i].hidden=YES; _mgMark[i].hidden=YES; continue; }
        BOOL inside=CGRectContainsPoint(area,p) && !(mode==2 && p.x>wr.origin.x+wr.size.width*0.72 && p.y>wr.origin.y+H*0.45);
        int art=v.art[i], sym=art>=64 && art<DM_MG_WP; UIImage *lab=nil, *mark=nil;
        NSString *base=art>=DM_MG_LVL ? [NSString stringWithFormat:@"lvl_%d",art-DM_MG_LVL] : art==DM_MG_WP ? @"lvl_wp" : [NSString stringWithFormat:@"label_%d",sym ? 0 : art];
        if(inside && sym) mark=[self mgImage:[NSString stringWithFormat:@"sym_%d.png",art-64]];
        else { int lg=dm_lang_group(); lab=lg!=DM_LG_ZH ? [self mgImage:[NSString stringWithFormat:@"%@_%s.png",base,lg==DM_LG_ES?"es":lg==DM_LG_MX?"mx":lg==DM_LG_RU?"ru":lg==DM_LG_KO?"ko":lg==DM_LG_DE?"de":lg==DM_LG_PT?"pt":"en"]] : (old && dm_lang_index()==8 ? [self mgImage:[base stringByAppendingString:@"_old.png"]] : nil);
            lab=lab ?: [self mgImage:[base stringByAppendingString:@".png"]]; if(inside) mark=[self mgImage:@"cross.png"]; }
        if(!inside) {
            CGFloat tx=p.x>o.x ? (CGRectGetMaxX(area)-o.x)/(p.x-o.x) : p.x<o.x ? (CGRectGetMinX(area)-o.x)/(p.x-o.x) : 1e9, ty=p.y>o.y ? (CGRectGetMaxY(area)-o.y)/(p.y-o.y) : p.y<o.y ? (CGRectGetMinY(area)-o.y)/(p.y-o.y) : 1e9, t=MIN(tx,ty);
            p=CGPointMake(o.x+(p.x-o.x)*t, o.y+(p.y-o.y)*t);
            if(mode==2 && p.x>wr.origin.x+wr.size.width*0.72 && p.y>wr.origin.y+H*0.45) p.y=wr.origin.y+H*0.45; }
        CGFloat lw=lab ? lh*lab.size.width/lab.size.height : 0, cw=lh*1.4, ch=cw*18/34;
        if(lab) { CGRect lr=CGRectMake(p.x-lw/2, (mark ? p.y-ch/2-2-lh : p.y-lh/2), lw, lh), occ=mark ? CGRectMake(lr.origin.x, lr.origin.y, lw, lh+2+ch) : lr; CGFloat sh0=0;
            for(int k=0;k<np;k++) if(CGRectIntersectsRect(CGRectOffset(occ,0,sh0),placed[k])) { sh0=CGRectGetMaxY(placed[k])+1-occ.origin.y; k=-1; }
            lr=CGRectOffset(lr,0,sh0); p.y+=sh0; if(np<DM_MAP_GOAL_MAX) placed[np++]=CGRectOffset(occ,0,sh0);
            _mgLabel[i].contents=(id)lab.CGImage; _mgLabel[i].frame=lr; _mgLabel[i].hidden=NO; } else _mgLabel[i].hidden=YES;
        if(mark) { CGFloat mw=sym ? sh*mark.size.width/mark.size.height : cw, mh=sym ? sh : ch; _mgMark[i].contents=(id)mark.CGImage; _mgMark[i].frame=CGRectMake(p.x-mw/2, p.y-mh/2, mw, mh); _mgMark[i].hidden=NO; } else _mgMark[i].hidden=YES;
    }
    [CATransaction commit];
    static int lm=-1, ln=-1, lc; int m2=show ? mode : 0; if((m2!=lm || n!=ln) && lc<40) { lm=m2; ln=n; lc++; LOG("小地图终点：%s %d 个（地图 %d，每格 %.2f 像素 = %.2f 点）", show ? "画" : "不画", n, mode, cell, c); }
}










- (void)mapHeadingSync {
    int dm_cam_heading(float *, float *, float *, float *); float fx=0, fz=0, sx=0, sy=0, cell=0, mm[4];
    int mode=dm_ui_map_geom(&cell); UIImage *im=[self mgImage:@"heading.png"];
    BOOL on=im && mode && !_navigation && !_info.hidden && dm_cam_heading(&fx,&fz,&sx,&sy) && (mode==2 || dm_ui_minimap_rect(mm));
    if(!_mhPtr) { _mhPtr=[CALayer layer]; _mhPtr.contentsGravity=kCAGravityResizeAspect; _mhPtr.hidden=YES;

        _mhPtr.shadowColor=[UIColor colorWithRed:212/255.0 green:196/255.0 blue:145/255.0 alpha:1].CGColor; _mhPtr.shadowOffset=CGSizeZero; _mhPtr.shadowRadius=4; _mhPtr.shadowOpacity=0.95;
        [_info.layer addSublayer:_mhPtr]; }
    [CATransaction begin]; [CATransaction setDisableActions:YES];
    if(!on) _mhPtr.hidden=YES;
    else { CGSize z=self.bounds.size; CGRect wr=world_rect(self); CGPoint o; CGFloat size;
        if(mode==2) { o=CGPointMake(wr.origin.x+sx*wr.size.width, wr.origin.y+sy*wr.size.height); size=wr.size.height*0.05; }
        else { CGRect a=CGRectMake(mm[0]*z.width, mm[1]*z.height, (mm[2]-mm[0])*z.width, (mm[3]-mm[1])*z.height); o=CGPointMake(CGRectGetMidX(a),CGRectGetMidY(a)); size=a.size.height*0.14; }
        CGFloat dx=(fx-fz)*1.6, dy=(fx+fz)*0.8, dl=hypot(dx,dy); if(dl<1e-6) { dx=0; dy=-1; } else { dx/=dl; dy/=dl; }
        CGFloat r=size*0.75;
        CGFloat iw=size*im.size.width/im.size.height, ang=atan2(dx,-dy);
        _mhPtr.contents=(id)im.CGImage; _mhPtr.bounds=CGRectMake(0,0,iw,size); _mhPtr.position=CGPointMake(o.x+dx*r, o.y+dy*r);
        _mhPtr.affineTransform=CGAffineTransformMakeRotation(ang);
        { static double lt; double t=CACurrentMediaTime(); if(t-lt>=2) { lt=t; LOG("小地图朝向指针：镜头 %.2f,%.2f → 地图方向 %.2f,%.2f 转角 %.0f° 圆心 %.0f,%.0f 指针 %.0f,%.0f 大小 %.0fx%.0f（地图 %d）", fx, fz, dx, dy, ang*180/M_PI, o.x, o.y, o.x+dx*r, o.y+dy*r, iw, size, mode); } }
        _mhPtr.hidden=NO; }
    [CATransaction commit];
}







- (void)threatsSync {
    int dm_cam_threats(float *, int); float t[8 * 5]; float cell=0; int n=(_navigation || _info.hidden || dm_ui_map_geom(&cell)==2) ? 0 : dm_cam_threats(t, 8);
    CGSize z=self.bounds.size; double now=CACurrentMediaTime();
    [CATransaction begin]; [CATransaction setDisableActions:YES];
    for(int i=0;i<8;i++) {
        if(!_thr[i]) { _thr[i]=[CALayer layer]; _thr[i].contentsGravity=kCAGravityResizeAspect; _thr[i].hidden=YES; _thr[i].shadowColor=UIColor.blackColor.CGColor; _thr[i].shadowOffset=CGSizeZero; _thr[i].shadowRadius=3; _thr[i].shadowOpacity=0.9; [_info.layer addSublayer:_thr[i]]; }
        UIImage *im=i<n ? [self mgImage:[NSString stringWithFormat:@"threat_%d.png",(int)fminf(fmaxf(t[i*5+4],0),3)]] : nil;
        if(!im) { _thr[i].hidden=YES; continue; }
        float u=fminf(fmaxf(t[i*5+3],0),1); CGFloat h=z.height*(0.035+0.03*u), w=h*im.size.width/im.size.height;
        _thr[i].contents=(id)im.CGImage; _thr[i].bounds=CGRectMake(0,0,w,h); _thr[i].position=CGPointMake(t[i*5]*z.width, t[i*5+1]*z.height);
        _thr[i].affineTransform=CGAffineTransformMakeRotation(t[i*5+2]);
        _thr[i].opacity=u>0.75f ? (float)(0.55+0.45*fabs(sin(now*5))) : 1; _thr[i].hidden=NO; }
    [CATransaction commit];
    { static int ln=-1, lc; if(n!=ln && lc<40) { ln=n; lc++; LOG("画面外怪物指针：%d 个", n); } }
}
- (void)tickTouchControls:(double)now {
    [self mapGoalSync];
    [self mapHeadingSync];
    [self threatsSync];
    { int dm_padmode_busy(void); BOOL hide=_feMode || _padMode || dm_padmode_busy(); if(_overlay.hidden!=hide) _overlay.hidden=hide; }
    { void dm_rift_hud_draw(UIView *, CGRect, BOOL); dm_rift_hud_draw(_info, world_rect(self), _navigation || _info.hidden); }
    {
        int dm_drop_haptic_take(void); int t = dm_drop_haptic_take();
        if (t >= 1 && t <= 3) { static UIImpactFeedbackGenerator *g[4]; if (!g[t]) g[t] = [[UIImpactFeedbackGenerator alloc] initWithStyle:t == 1 ? UIImpactFeedbackStyleHeavy : t == 2 ? UIImpactFeedbackStyleMedium : UIImpactFeedbackStyleLight];
            [g[t] impactOccurred]; LOG("掉落震动：前端震第 %d 档", t); } }
    dm_tc_set_tap_box(dm_ui_tap_box()); dm_tc_set_cur_stack(dm_ui_cur_stack_id()); dm_tc_set_hover_unid(dm_ui_hover_unid());
    { dm_hud_info hi; dm_hud_get(&hi); dm_tc_set_holding(&_controls,hi.valid && hi.holding && !dm_ui_targeting()); }
    if(_itemAsked && _controls.navigation_mode && _controls.pointer_slot>=0) { int ir=dm_ui_item_result(_itemSeq); if(ir>=0) { _itemAsked=NO; _itemKind=ir; dm_tc_pointer_item(&_controls,ir); } }
    if(_pendEnd) {
        BOOL ready=!(_itemAsked && dm_ui_item_result(_itemSeq)<0) && !(_finalAsked && dm_ui_item_result(_relSeq)<0);
        if(ready || CACurrentMediaTime()-_pendWall>0.25) { UITouch *pt=_pendEnd;


            LOG("前端：抬手挂起 %.0f ms 后结束（%s）：判类 %d%s", (CACurrentMediaTime()-_pendWall)*1000, ready ? "答案已回" : "超时，按没答上处理", _itemAsked ? dm_ui_item_result(_itemSeq) : _itemKind, _finalAsked ? [NSString stringWithFormat:@"，松手落点 %d", dm_ui_item_result(_relSeq)].UTF8String : "");
            _pendFire=YES; [self finishTouches:[NSSet setWithObject:pt] phase:"ended"]; _pendFire=NO; _pendEnd=nil; _finalAsked=NO; }
    }
    [self bindTapSync];
    [self shopGhostSync];




    if(_controls.navigation_mode && !dm_ui_bind_active() && !_itemAsked && _controls.pointer_slot>=0 && _controls.pointer_slot<DM_TC_MAX_TOUCH) { dm_tc_finger *f=&_controls.finger[_controls.pointer_slot]; float nx=0,ny=0; CGSize z=self.bounds.size; uint32_t hid=dm_ui_hit_id();
        if(f->active && !f->pointer_dragged && !f->ui_drag && now-f->start_time_s>=0.5 && hypotf(f->x-f->aim_origin_x,f->y-f->aim_origin_y)<=10*dm_tc_scale_for_size(z.width,z.height) && dm_ui_bind_skill_hit(&nx,&ny)==2 && hid>=1 && hid<=4) {
            if(!f->pointer_long) { f->pointer_long=1; _statNext=now; _selId=hid; _controls.pointer_valid=1; _controls.pointer_x=nx*(float)z.width; _controls.pointer_y=ny*(float)z.height; LOG("前端：按住「+」（属性 %u）→ 连续加点",hid-1); }
            if(now>=_statNext) { _statNext=now+0.15; dm_stat_add((int)hid-1,1); } } }


    if(_controls.navigation_mode && !dm_ui_bind_active() && !_itemAsked && _controls.pointer_slot>=0 && _controls.pointer_slot<DM_TC_MAX_TOUCH) { dm_tc_finger *f=&_controls.finger[_controls.pointer_slot]; float nx=0,ny=0; CGSize z=self.bounds.size;
        if(!_pendEnd && f->active && !f->pointer_dragged && !f->pointer_long && now-f->start_time_s>=0.5 && hypotf(f->x-f->aim_origin_x,f->y-f->aim_origin_y)<=10*dm_tc_scale_for_size(z.width,z.height) && dm_ui_bind_skill_hit(&nx,&ny)==1) {
            dm_tc_pointer_press(&_controls,nx*(float)z.width,ny*(float)z.height,DM_BTN_Y); LOG("前端：技能树里长按技能 (%.0f,%.0f) → 开绑定框",nx*z.width,ny*z.height); } }
    if(_mapWorldAsked) { int wr=dm_world_tap_result(_mapWorldSeq);
        if(wr>=0) { _mapWorldAsked=NO; if(wr==0 && !_navigation) { dm_ui_map_full(1); LOG("前端：点小地图那一块 (%.0f,%.0f) → 全屏地图",_mapWorldPt.x,_mapWorldPt.y); }
            else LOG("前端：点小地图那一块 (%.0f,%.0f) → 底下有世界目标（类 %d），交互、不切地图",_mapWorldPt.x,_mapWorldPt.y,wr); } }
    if(_addAt>0 && now>=_addAt) { dm_points_info pi; dm_points_get(&pi);
        if(pi.tree_open && pi.sel_skill>=0 && !dm_cursor_pending()) { _addAt=0; if(pi.sel_block==0) { dm_skill_add_selected(); LOG("前端：再点同一个技能 → 加 1 点"); } else if(pi.sel_block==5) { dm_skill_add_selected(); LOG("前端：再点同一个技能 → 已到 20、这个角色还没打通过秘境，交宿主出提示"); } else LOG("前端：再点同一个技能 → 不能加（原因 %d），不发",pi.sel_block); }
        else if(now>_addAt+1.0 || !pi.tree_open) { _addAt=0; LOG("前端：再点同一个技能 → 1 秒内游戏没选中它，不发"); } }






    if(!_controls.navigation_mode || _padMode) _tipUntil=-2;
    else if(_controls.pointer_slot>=0) _tipUntil=-1;
    else if(_tipUntil==-2) { if(dm_ui_free_cursor()) _tipUntil=now+0.3; }
    else if(_tipUntil>0 && now>=_tipUntil && !_controls.holding && !dm_ui_bind_active() && !dm_ui_touch_active() && !_controls.pulse_button && !dm_tc_pulse_pending(&_controls) && !dm_cursor_pending()) {
        CGSize z=self.bounds.size; _controls.pointer_valid=1; _controls.pointer_x=0.02f*(float)z.width; _controls.pointer_y=0.5f*(float)z.height; _tipUntil=0; }
    uint64_t tb=_controls.buttons;
    dm_tc_tick(&_controls, now);
    { int fg=0; for(int i=0;i<DM_TC_MAX_TOUCH;i++) fg|=_controls.finger[i].active; dm_ui_hand_idle(!fg && !_controls.pulse_button && !dm_tc_pulse_pending(&_controls) && !dm_cursor_pending() && !dm_ui_touch_active()); }
    if(_controls.buttons!=tb && !_controls.pulse_button) [self pushFrom:"tick" phase:"none"];
    else if(_controls.belt_request && !_controls.pulse_button) [self pushFrom:"tick" phase:"none"];
    if(_controls.pulse_button && !_pulseTicking) { [self pushFrom:"tick" phase:"none"]; [self armPulseEnd]; }
    if (_controls.pointer_valid)
        dm_cursor_set_screen(_controls.pointer_x, _controls.pointer_y, self.bounds.size.width, self.bounds.size.height);
    else if (_controls.navigation_mode) dm_cursor_clear();
}




static BOOL touch_done(UITouch *t) { UITouchPhase ph=t.phase; return ph==UITouchPhaseEnded || ph==UITouchPhaseCancelled; }
- (void)sweepEndedSlots {
    for(int i=0;i<16;i++) if(_slot[i] && _slot[i]!=_pendEnd && touch_done(_slot[i])) {
        LOG("前端：【红】清理残留触摸槽 %d（%s，阶段 %d，控件层 %s）", i, [_slot[i] isKindOfClass:DmScriptTouch.class] ? "脚本触摸" : "真触摸", (int)_slot[i].phase, _navigation ? "面板态" : "游戏态");
        if(_uiDragOn && _controls.pointer_slot==i) { _uiDragOn=NO; dm_ui_drag_end(); }
        dm_tc_cancel_finger(&_controls,i); _slot[i]=nil; }
    if(_mapTouch && touch_done(_mapTouch)) { LOG("前端：【红】清理残留触摸（小地图那根）"); _mapTouch=nil; }
    if(_bindTouch && touch_done(_bindTouch)) { LOG("前端：【红】清理残留触摸（空技能位那根）"); _bindTouch=nil; }
    if(_uiFinger && touch_done(_uiFinger)) { LOG("前端：【红】清理残留触摸（界面手指）"); _uiFinger=nil;
        if(_uiFeDrag||_uiDragOn) { _uiFeDrag=NO; _uiDragOn=NO; dm_ui_drag_end(); } else dm_ui_touch(3,0,0); }
}
- (void)touchesBegan:(NSSet<UITouch *> *)ts withEvent:(UIEvent *)e {
    (void)e;
    [self sweepEndedSlots];






    { BOOL wasPad=_padMode; dm_gamepad_touched();
      int dm_padmode_busy(void);
      if(wasPad || dm_padmode_busy()) { if(!_swallow) _swallow=[NSHashTable weakObjectsHashTable]; for(UITouch *t in ts) [_swallow addObject:t]; if(wasPad) LOG("前端：手柄模式下碰屏 → 只切回触屏模式，这一下不执行"); return; } }
    if(atomic_load(&g_touch_frontend)) { [self uiTouch:ts phase:0]; return; }
    [self refreshPortalBinding];
    if(_pendEnd) { UITouch *pt=_pendEnd; _pendFire=YES; [self finishTouches:[NSSet setWithObject:pt] phase:"ended"]; _pendFire=NO; _pendEnd=nil; _finalAsked=NO; }
    for(UITouch *t in ts) {
        int slot=-1;
        { int ex=0; _controls.belt_rows=dm_belt_rows(&ex); _controls.belt_open=ex; CGPoint q=[t locationInView:self];
          if(dm_tc_belt_outside(&_controls,q.x,q.y)) { dm_belt_expand(0); LOG("前端：腰带展开着点别处 (%.0f,%.0f) → 只收腰带",q.x,q.y); continue; } }
        if(!_navigation && !_bindTouch && !dm_ui_bind_active()) { BOOL em=NO; CGPoint bp=[t locationInView:self]; uint64_t bk=[self skillKeyAt:bp empty:&em]; if(bk && em) { _bindTouch=t; _bindKey=bk; _bindP0=bp; _mapT0=CACurrentMediaTime(); continue; } }
        if(!_mapTouch && [self inMinimap:[t locationInView:self]]) { _mapTouch=t; _mapP0=[t locationInView:self]; _mapT0=CACurrentMediaTime(); continue; }
        for(int i=0;i<16;i++) if(!_slot[i]) {
            _slot[i]=t; CGPoint p=[t locationInView:self]; { int ex=0; _controls.belt_rows=dm_belt_rows(&ex); _controls.belt_open=ex; } { int dm_cam_mode(void); _controls.cam_on=dm_cam_mode(); } dm_tc_begin_at(&_controls,i,p.x,p.y,CACurrentMediaTime()); slot=i;
            if(!_navigation && _controls.finger[i].target==DM_TC_PORTAL) { void dm_tp_request(void); dm_tp_request(); LOG("前端：点回城钮 → 免费回城"); }


            if(_controls.navigation_mode && _controls.pointer_slot==i && self.bounds.size.width>0) _itemSeq=dm_ui_item_query((float)(p.x/self.bounds.size.width),(float)(p.y/self.bounds.size.height)); _itemAsked=YES; _itemKind=-1;
            break;
        }
        [self recordTouch:t slot:slot phase:"began"];
    }
    [self pushFrom:"uitouch" phase:"began"];
}
- (void)touchesMoved:(NSSet<UITouch *> *)ts withEvent:(UIEvent *)e {
    (void)e;
    if(_swallow.count) { NSMutableSet *r=[ts mutableCopy]; for(UITouch *t in ts) if([_swallow containsObject:t]) [r removeObject:t]; if(!r.count) return; ts=r; }
    if(atomic_load(&g_touch_frontend)) { [self uiTouch:ts phase:1]; return; }
    UITouch *uf=_uiFinger;
    if(uf && [ts containsObject:uf]) { [self uiTouch:[NSSet setWithObject:uf] phase:1]; NSMutableSet *r=[ts mutableCopy]; [r removeObject:uf]; ts=r; if(!ts.count) return; }
    for(UITouch *t in ts) {
        int slot=-1;
        for(int i=0;i<16;i++) if(_slot[i]==t) { CGPoint p=[t locationInView:self]; dm_tc_move_at(&_controls,i,p.x,p.y,CACurrentMediaTime()); slot=i;
            if(_controls.cam_pitch!=0 || _controls.cam_yaw!=0) { void dm_cam_tilt(float); void dm_cam_turn(float); if(_controls.cam_pitch!=0) dm_cam_tilt(_controls.cam_pitch); if(_controls.cam_yaw!=0) dm_cam_turn(_controls.cam_yaw); _controls.cam_pitch=_controls.cam_yaw=0; }


            if(_controls.navigation_mode && _controls.pointer_slot==i && _controls.finger[i].ui_drag && self.bounds.size.width>0) { CGSize z=self.bounds.size; dm_tc_finger *f=&_controls.finger[i];
                if(!_uiDragOn) { _uiDragOn=YES; dm_ui_drag_lift((float)(DM_TC_POINTER_LIFT_PT*dm_tc_scale_for_size(z.width,z.height)/z.height)); dm_ui_drag_begin(f->ui_drag,(float)(f->aim_origin_x/z.width),(float)(f->aim_origin_y/z.height),(float)(44.0/z.height)); }
                dm_ui_drag_move((float)(p.x/z.width),(float)(p.y/z.height)); }


            if(_controls.navigation_mode && _controls.pointer_slot==i && _controls.finger[i].pointer_dragged && self.bounds.size.width>0) { _relSeq=dm_ui_drop_query((float)(p.x/self.bounds.size.width),(float)((p.y-DM_TC_POINTER_LIFT_PT*dm_tc_scale_for_size(self.bounds.size.width,self.bounds.size.height))/self.bounds.size.height),(float)(p.y/self.bounds.size.height)); _relAsked=YES; }
            break; }
        [self recordTouch:t slot:slot phase:"moved"];
    }
    [self pushFrom:"uitouch" phase:"moved"];
    [self armPulseEnd];
}






- (void)uiTouch:(NSSet<UITouch *> *)ts phase:(int)ph {
    CGSize b=self.bounds.size; if(b.width<=0 || b.height<=0) return;
    for(UITouch *t in ts) {
        if(ph==0 && !_uiFinger) { _uiFinger=t; _uiFeO=[t locationInView:self]; _uiFeDrag=NO; }
        if(t!=_uiFinger) continue;
        CGPoint p=[t locationInView:self]; float dy=fabs(p.y-_uiFeO.y);
        if(ph==1 && !_uiFeDrag && !_uiDragOn && dy>10 && dy>fabs(p.x-_uiFeO.x) && dm_ui_fe_drag_ok()) {
            _uiFeDrag=YES; _uiDragOn=YES; dm_ui_touch(3,0,0); dm_ui_drag_begin(1,(float)(_uiFeO.x/b.width),(float)(_uiFeO.y/b.height),0);
            LOG("前端：纵向拖 → 滚动（起手 %.0f,%.0f），这次不点", _uiFeO.x, _uiFeO.y); }
        if(_uiFeDrag) { if(ph==1) dm_ui_drag_move((float)(p.x/b.width),(float)(p.y/b.height)); else if(ph==2) { _uiFeDrag=NO; _uiDragOn=NO; dm_ui_drag_end(); _uiFinger=nil; } continue; }
        dm_ui_touch(ph,(float)(p.x/b.width),(float)(p.y/b.height));
        if(ph==2) _uiFinger=nil;
    }
}
- (void)finishTouches:(NSSet<UITouch *> *)ts phase:(const char *)phase {
    if(_swallow.count) { NSMutableSet *r=[ts mutableCopy]; for(UITouch *t in ts) if([_swallow containsObject:t]) { [r removeObject:t]; [_swallow removeObject:t]; } if(!r.count) return; ts=r; }


    UITouch *uf=_uiFinger;
    if(uf && [ts containsObject:uf]) { [self uiTouch:[NSSet setWithObject:uf] phase:2]; NSMutableSet *r=[ts mutableCopy]; [r removeObject:uf]; ts=r; }
    if(!ts.count) return;
    for(UITouch *t in ts) {
        int slot=-1;
        if(t==_mapTouch) { CGPoint p=[t locationInView:self]; _mapTouch=nil;
            if(!strcmp(phase,"ended") && !_navigation && hypot(p.x-_mapP0.x,p.y-_mapP0.y)<=14 && CACurrentMediaTime()-_mapT0<0.6) { CGSize z=self.bounds.size;




                if(dm_ui_map_is_full()) { dm_ui_map_full(0); LOG("前端：点小地图那一块 (%.0f,%.0f) → 回小地图",p.x,p.y); }
                else if(z.width>0) { { float wx,wy; world_norm(self,p.x,p.y,&wx,&wy); _mapWorldSeq=dm_world_tap(wx,wy); } _mapWorldAsked=YES; _mapWorldPt=p; } }
            continue; }
        if(t==_bindTouch) { CGPoint p=[t locationInView:self]; _bindTouch=nil;
            if(!strcmp(phase,"ended") && !_navigation && hypot(p.x-_bindP0.x,p.y-_bindP0.y)<=14) { dm_ui_bind_begin(_bindKey); LOG("前端：点空技能位 (%.0f,%.0f) → 给键 %#llx 选技能",p.x,p.y,(unsigned long long)_bindKey); }
            continue; }








        if(!_pendFire && !_pendEnd && !strcmp(phase,"ended") && _controls.navigation_mode && _controls.pointer_slot>=0 && _slot[_controls.pointer_slot]==t && self.bounds.size.width>0) {
            dm_tc_finger *pf=&_controls.finger[_controls.pointer_slot]; BOOL wait=NO;
            if(_itemAsked && dm_ui_item_result(_itemSeq)<0) wait=YES;
            else if(pf->pointer_dragged) { CGPoint ep=[t locationInView:self]; CGSize z=self.bounds.size;
                dm_tc_move_at(&_controls,_controls.pointer_slot,ep.x,ep.y,CACurrentMediaTime());
                _relSeq=dm_ui_drop_query((float)(ep.x/z.width),(float)((ep.y-DM_TC_POINTER_LIFT_PT*dm_tc_scale_for_size(z.width,z.height))/z.height),(float)(ep.y/z.height)); _relAsked=YES; _finalAsked=YES; wait=YES; }
            if(wait) { _pendEnd=t; _pendEndTime=CACurrentMediaTime(); _pendWall=CACurrentMediaTime(); continue; }
        }
        for(int i=0;i<16;i++) if(_slot[i]==t) {


            if(!strcmp(phase,"cancelled")) { dm_tc_finger *cf=&_controls.finger[i]; BOOL drag=_controls.navigation_mode && _controls.pointer_slot==i && (cf->pointer_dragged || cf->shop_drag);
                if(_uiDragOn && _controls.pointer_slot==i) { _uiDragOn=NO; dm_ui_drag_end(); }
                if(drag) { BOOL shop=cf->shop_drag; if(!shop) dm_tc_pointer_release(&_controls,0); dm_tc_end_at(&_controls,i,CACurrentMediaTime()); LOG("前端：拖动中被系统取消触摸 → %s",shop?"不买不取":"放回原处"); }
                else { dm_tc_cancel_finger(&_controls,i); LOG("前端：系统取消触摸（槽 %d）→ 不当松手",i); }
                if(_controls.pointer_slot<0) { _relAsked=NO; _itemAsked=NO; }
                _slot[i]=nil; slot=i; break; }
            if(_controls.navigation_mode && _controls.pointer_slot==i && _controls.finger[i].shop_drag) { int rk=_relAsked ? dm_ui_item_result(_relSeq) : 0; if(rk<0) rk=dm_ui_drop_last(); if(rk==5 && !dm_ui_drop_bag()) LOG("前端：%s拖出后松在背包格以外的物品区 → 不%s",_itemKind==8?"一叠":"货架上的货",_itemKind==8?"取":"买");                                                                                                                                              else if((rk==5 || rk==11) && self.bounds.size.width>0) { dm_tc_finger *sf=&_controls.finger[i]; if(rk==11) { dm_ui_stack_cube_at((float)(sf->aim_origin_x/self.bounds.size.width),(float)(sf->aim_origin_y/self.bounds.size.height)); LOG("前端：高级仓库里的一叠拖到魔盒格上松手 → 1 件进魔盒"); } else if(_itemKind==8) { dm_ui_stack_take_at((float)(sf->aim_origin_x/self.bounds.size.width),(float)(sf->aim_origin_y/self.bounds.size.height)); LOG("前端：高级仓库里的一叠拖到背包松手 → 取 1 件"); } else { LOG("前端：货架上的货拖到背包松手 → 买"); dm_ui_shop_buy_at((float)(sf->aim_origin_x/self.bounds.size.width),(float)(sf->aim_origin_y/self.bounds.size.height)); } _controls.pointer_x=sf->aim_origin_x; _controls.pointer_y=sf->aim_origin_y;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 } }
            else if(_controls.navigation_mode && _controls.pointer_slot==i && _controls.finger[i].pointer_dragged && _relAsked) { int rr=dm_ui_item_result(_relSeq); if(rr<0) rr=dm_ui_drop_last(); if(rr<0) rr=0; float dx,dy;                                                                                                                                                                                                                                                                                                                                         if((rr==5 && dm_ui_drop_point(&dx,&dy)) || (rr==0 && dm_ui_origin_point(&dx,&dy))) dm_tc_pointer_release_at(&_controls,dx*self.bounds.size.width,dy*self.bounds.size.height);                                                                                                                                                                                                                                                                                         else { if(rr==7) { dm_shop_sell_cursor(); rr=0; } dm_tc_pointer_release(&_controls,rr<0?1:rr); } }
            _relAsked=NO;
            if(_controls.navigation_mode && _controls.pointer_slot==i && _itemAsked) { _itemAsked=NO; dm_tc_pointer_item(&_controls,dm_ui_item_result(_itemSeq)); [self bindTapSync]; }
            unsigned wt=_controls.world_tap_seq;
            unsigned mc=_controls.mouse_click_seq; unsigned tt=_controls.tab_tap_seq; unsigned us=_controls.use_seq; unsigned ids=_controls.ident_seq; unsigned sts=_controls.stack_take_seq; unsigned lps=_controls.long_press_seq; unsigned its=_controls.item_tap_seq;
            BOOL dragged=_uiDragOn && _controls.pointer_slot==i;
            if(_uiDragOn && _controls.pointer_slot==i) { _uiDragOn=NO; dm_ui_drag_end(); }
            BOOL wasPtr=_controls.pointer_slot==i; unsigned tm=_controls.tap_move_seq;



            if(_controls.finger[i].target==DM_TC_SWAP) { CGPoint ep=[t locationInView:self];dm_tc_move_at(&_controls,i,ep.x,ep.y,CACurrentMediaTime()); }
            if(_pendFire) dm_tc_end_at(&_controls,i,_pendEndTime); else dm_tc_end_at(&_controls,i,CACurrentMediaTime()); _slot[i]=nil; slot=i;


            if(_controls.tap_move_seq!=tm) { uint32_t hid=dm_ui_hit_id();
                if(hid && hid==_selId) { if(hid<=4) { dm_stat_add((int)hid-1,1); LOG("前端：再点同一个「+」（属性 %u）→ 加 1 点",hid-1); }
                    else _addAt=_controls.last_time_s+0.3; }
                else { _selId=hid; LOG("前端：点加点类按钮 → 选中（标识 %#x）",hid); } }
            if(_controls.tab_tap_seq!=tt) dm_ui_tab_go();
            if(_controls.long_press_seq!=lps) dm_ui_long_press();
            if(_controls.item_tap_seq!=its) dm_ui_item_tap();
            if(_controls.stack_take_seq!=sts) { dm_stash_take(1); LOG("前端：双击高级仓库里的一叠 → 取 1 件"); }
            if(_controls.ident_seq!=ids) dm_ui_identify_hovered();
            if(_controls.use_seq!=us) dm_ui_use_item();
            if(_controls.world_tap_seq!=wt && self.bounds.size.width>0) { float wx,wy; world_norm(self,_controls.world_tap_x,_controls.world_tap_y,&wx,&wy); dm_ui_world_tap((float)(_controls.world_tap_x/self.bounds.size.width),(float)(_controls.world_tap_y/self.bounds.size.height),wx,wy); }




            BOOL itemTap=_controls.item_tap_seq!=its && !dragged && _controls.use_seq==us && _controls.ident_seq==ids && _controls.stack_take_seq==sts && _controls.long_press_seq==lps && _controls.mouse_click_seq==mc;
            if(wasPtr && _controls.navigation_mode) _tipUntil=(_controls.tap_move_seq!=tm || itemTap) ? -1 : _controls.last_time_s+(_controls.mouse_click_seq!=mc ? 0.05 : 2.8);
            if(_controls.mouse_click_seq!=mc && self.bounds.size.width>0) { float nx=_controls.mouse_click_x/self.bounds.size.width, ny=_controls.mouse_click_y/self.bounds.size.height; dm_ui_touch(0,nx,ny); dm_ui_touch(2,nx,ny); }
            break; }
        [self recordTouch:t slot:slot phase:phase];
    }
    [self pushFrom:"uitouch" phase:phase];
    [self armPulseEnd];
}










- (void)armPulseEnd {
    if(_pulseTicking || (!_controls.pulse_button && !dm_tc_pulse_pending(&_controls))) return;
    _pulseTicking=YES; _pulseEpoch=0; [self pulseTick];
}
- (void)pulseTick {
    uint64_t n=dm_input_samples(); double now=CACurrentMediaTime();
    if(_controls.pulse_button) {
        if(_pulseEpoch!=_controls.pulse_epoch) { _pulseEpoch=_controls.pulse_epoch; _pulseS0=n; _pulseT0=now; _pulseSeen=0; _pulseFree=0; }
        if(!_pulseSeen && n>_pulseS0) _pulseSeen=now;
        if(_pulseSeen && !_pulseFree && !dm_cursor_pending()) _pulseFree=now;
        double hold=_controls.pulse_hold_s;
        BOOL taken=_pulseSeen && now-_pulseSeen>=0.12 && n-_pulseS0>=3 && !dm_cursor_pending() && (hold<=0 || (_pulseFree && now-_pulseFree>=hold));
        if(taken || now-_pulseT0>2.0+hold) {
            if(!taken) LOG("[touch] 【红】脉冲 2 秒内游戏没取走（取了 %llu 次，扣着 %d 下），强制收回",(unsigned long long)(n-_pulseS0),dm_cursor_pending());
            if(dm_tc_pulse_end(&_controls,_pulseEpoch)) [self pushFrom:"release_cast" phase:"none"];
            _pulseS0=n; _pulseT0=now;
        }
    } else if(dm_tc_pulse_pending(&_controls)) {
        if(n-_pulseS0>=2 || now-_pulseT0>0.5) { if(dm_tc_pulse_resume(&_controls)) [self pushFrom:"release_cast" phase:"none"]; }
    } else { _pulseTicking=NO; return; }
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW,(int64_t)(NSEC_PER_SEC/120)),dispatch_get_main_queue(),^{ [self pulseTick]; });
}
- (void)touchesEnded:(NSSet<UITouch *> *)ts withEvent:(UIEvent *)e { (void)e; [self finishTouches:ts phase:"ended"]; }
- (void)touchesCancelled:(NSSet<UITouch *> *)ts withEvent:(UIEvent *)e { (void)e; [self finishTouches:ts phase:"cancelled"]; }
- (CGPoint)scriptPoint:(NSString *)spec move:(BOOL)move {


    CGPoint base=CGPointZero; NSString *rest=spec; BOOL absPt=[spec hasPrefix:@"="]; if(absPt) rest=[spec substringFromIndex:1];
    if([spec hasPrefix:@"@"]) {
        NSRange r=[spec rangeOfCharacterFromSet:[NSCharacterSet characterSetWithCharactersInString:@"+"]];
        NSString *name=[spec substringWithRange:NSMakeRange(1,(r.location==NSNotFound?spec.length:r.location)-1)];
        rest=r.location==NSNotFound?@"0,0":[spec substringFromIndex:r.location+1];
        NSArray *names=@[@"main",@"skill1",@"skill2",@"skill3",@"skill4",@"portal",@"belt1",@"belt2",@"belt3",@"belt4",@"menu",@"map"];
        NSUInteger i=[names indexOfObject:name];
        if([name isEqualToString:@"joy"]) base=CGPointMake(_controls.layout.joy_x,_controls.layout.joy_y);
        else if([name isEqualToString:@"swap"]) base=CGPointMake(_controls.layout.swap.x,_controls.layout.swap.y);
        else if(i!=NSNotFound) base=CGPointMake(_controls.layout.target[i].x,_controls.layout.target[i].y);
        else if([name isEqualToString:@"mmname"] || [name isEqualToString:@"mmbody"]) { CGRect wh,tp;
            if([self minimapRect:&wh tap:&tp]) { CGRect q=[name isEqualToString:@"mmname"]?tp:wh; base=CGPointMake(CGRectGetMidX(q),CGRectGetMidY(q));
                LOG("[script] 小地图：矩形 x %.0f–%.0f y %.0f–%.0f 点，点击区 y %.0f–%.0f（高 %.0f 点）；%s = (%.0f,%.0f)",wh.origin.x,CGRectGetMaxX(wh),wh.origin.y,CGRectGetMaxY(wh),tp.origin.y,CGRectGetMaxY(tp),tp.size.height,name.UTF8String,base.x,base.y); }
            else LOG("[script] 【红】%s：还没有小地图矩形",name.UTF8String); }
        else LOG("[script] 【红】未知目标 %s",name.UTF8String);
    }
    NSArray *xy=[rest componentsSeparatedByString:@","];
    CGFloat x=[xy[0] doubleValue], y=xy.count>1?[xy[1] doubleValue]:0;






    static double rw, rh; static int ref=-1;
    if(ref<0) { const char *e=getenv("DM_TOUCH_REF"); ref=e && sscanf(e,"%lfx%lf",&rw,&rh)==2 && rw>0 && rh>0; }
    if(ref && !absPt && ![spec hasPrefix:@"@"]) {
        CGFloat W=self.bounds.size.width, H=self.bounds.size.height, k=H/rh;
        if(move) y-=DM_TC_POINTER_LIFT_PT*dm_tc_scale_for_size(rw,rh);
        x=W/2+(x-rw/2)*k; y=y*k;
        if(move) y+=DM_TC_POINTER_LIFT_PT*dm_tc_scale_for_size(W,H);
    }
    return CGPointMake(base.x+x,base.y+y);
}
- (void)scriptPhase:(NSString *)phase slot:(int)slot at:(NSString *)spec {
    static DmScriptTouch *fingers[16];
    if(slot<0||slot>=16) return;
    if([phase isEqualToString:@"down"]) {




        if(fingers[slot]) { LOG("[script] 【红】手指 %d 还按着又按下 → 先抬起旧的（脚本里两处在用同一根手指）",slot); fingers[slot].scriptPhase=UITouchPhaseEnded; [self touchesEnded:[NSSet setWithObject:fingers[slot]] withEvent:nil]; fingers[slot]=nil; }
        fingers[slot]=[DmScriptTouch new]; fingers[slot].scriptPoint=[self scriptPoint:spec move:NO]; fingers[slot].scriptPhase=UITouchPhaseBegan;
        [self touchesBegan:[NSSet setWithObject:fingers[slot]] withEvent:nil];
    } else if(fingers[slot] && [phase isEqualToString:@"move"]) {
        fingers[slot].scriptPoint=[self scriptPoint:spec move:YES]; fingers[slot].scriptPhase=UITouchPhaseMoved;
        [self touchesMoved:[NSSet setWithObject:fingers[slot]] withEvent:nil];
    } else if(fingers[slot] && [phase isEqualToString:@"up"]) {
        fingers[slot].scriptPhase=UITouchPhaseEnded; [self touchesEnded:[NSSet setWithObject:fingers[slot]] withEvent:nil]; fingers[slot]=nil;
    } else if(fingers[slot] && [phase isEqualToString:@"cancel"]) {
        fingers[slot].scriptPhase=UITouchPhaseCancelled; [self touchesCancelled:[NSSet setWithObject:fingers[slot]] withEvent:nil]; fingers[slot]=nil;
    } else { LOG("[script] 【红】无效动作 %s slot %d",phase.UTF8String,slot); return; }
    LOG("[script] %s slot=%d spec=%s buttons=%#llx lx=%d ly=%d rx=%d ry=%d",phase.UTF8String,slot,spec.UTF8String?:"",
        (unsigned long long)_controls.buttons,_controls.lx,_controls.ly,_controls.rx,_controls.ry);
}
- (void)captureTouchLayer:(NSString *)path {
    [self refreshPortalBinding];
    UIGraphicsImageRendererFormat *f=[UIGraphicsImageRendererFormat preferredFormat]; f.opaque=NO; f.scale=self.window.screen.scale;
    UIImage *im=[[[UIGraphicsImageRenderer alloc] initWithBounds:_overlay.bounds format:f] imageWithActions:^(UIGraphicsImageRendererContext *ctx){ [self->_overlay.layer renderInContext:ctx.CGContext]; if(!self->_info.hidden) [self->_info.layer renderInContext:ctx.CGContext]; }];
    NSData *png=UIImagePNGRepresentation(im); BOOL imageOK=[png writeToFile:path atomically:YES];
    NSMutableDictionary *m=[[_overlay controlMetadata] mutableCopy]; m[@"scale"]=@(f.scale); m[@"pixel_size"]=@[@(CGImageGetWidth(im.CGImage)),@(CGImageGetHeight(im.CGImage))];
    NSError *error=nil; NSData *json=[NSJSONSerialization dataWithJSONObject:m options:NSJSONWritingPrettyPrinted error:&error];
    BOOL metadataOK=[json writeToFile:[path stringByAppendingString:@".json"] options:NSDataWritingAtomic error:&error];
    if(imageOK && metadataOK) { _touchShots++; [self pushFrom:"screenshot" phase:"none"]; fprintf(stderr,"[touch] overlay screenshot %s written\n",path.UTF8String); }
    else fprintf(stderr,"[touch] 【红】操作层截图写盘失败 PNG=%d JSON=%d %s\n",imageOK,metadataOK,error.localizedDescription.UTF8String?:"PNG write failed");
}
@end









int dm_hud_stack_msg(const char *act, const char *arg); void dm_hud_stack_dump(void);
static _Atomic int g_hud_blocked;
static uint64_t g_hud_blocked_since;
static void hud_stack_message(const char *act,const char *arg) {
    int n=dm_hud_stack_msg(act,arg);
    uint64_t t=now_ns();
    if(n>0 && !g_hud_blocked_since) g_hud_blocked_since=t;
    if(n<=0) g_hud_blocked_since=0;
    if(atomic_exchange(&g_hud_blocked,n)!=n) dispatch_async(dispatch_get_main_queue(),^{ [g_touch_view hudBlocked:n]; });
}






static void hud_stack_watch(void) {
    if(g_hud_blocked_since && now_ns()-g_hud_blocked_since>60000000000ull) {
        fprintf(stderr,"[hud] 面板栈已连续 60 秒非零（线索，非缺陷）：");
        dm_hud_stack_dump();
        fprintf(stderr,"\n"); g_hud_blocked_since=now_ns();
    }
}


static void touch_ui_message(const char *cat,const char *act,const char *arg) {
    if(!strcmp(cat,"PanelManager")) hud_stack_message(act,arg);
    static dm_tc_scene scene; static uint64_t sceneEpoch=1,worldEpoch=1;
    dm_tc_scene before=scene;
    dm_tc_set_panel_state(dm_ui_panel_state());
    int mode=dm_tc_navigation(&scene,cat,act,arg);






    if(scene.game && atomic_load(&g_hud_blocked)>0) mode=1;
    static int prevMode=1;
    if(before.game!=scene.game)++worldEpoch;
    { uint32_t dm_padmode_rebuilds(void); static uint32_t rb0; uint32_t rb=dm_padmode_rebuilds(); if(rb!=rb0) { rb0=rb; ++worldEpoch; } }
    if(mode!=prevMode)++sceneEpoch;
    prevMode=mode;


    dm_touch_game_set_scene(!mode,sceneEpoch,worldEpoch);
    int fe=!scene.game;
    if(atomic_exchange(&g_touch_frontend,fe)!=fe) dispatch_async(dispatch_get_main_queue(),^{ [g_touch_view frontendMode:fe]; });


    if(getenv("DM_TOUCH_SCRIPT_START") && !strcmp(cat,"PanelManager") && !strcmp(act,"OpenPanel") && !strcmp(arg,"MainMenuButtonRibbon")) dispatch_async(dispatch_get_main_queue(),^{ touch_script_start(); });
    int nmode=dm_tc_panel_mode(mode,&scene,atomic_load(&g_hud_blocked));
    if(atomic_exchange(&g_touch_navigation,nmode)!=nmode) dispatch_async(dispatch_get_main_queue(),^{ [g_touch_view navigationMode:nmode]; });
    dm_uiscreen_set_panel(nmode==1);
}







@interface DmRateHint : NSObject
- (void)tick:(CADisplayLink *)l;
@end
@implementation DmRateHint
- (void)tick:(CADisplayLink *)l {
    extern int mtl_fps_target(void);
    { static int last = -1; int f = mtl_fps_target(); if (f != last) { last = f; l.preferredFrameRateRange = f == 30 ? CAFrameRateRangeMake(30, 30, 30) : CAFrameRateRangeMake(60, 60, 60); } }
    DmMetalView *v = g_touch_view;
    if (v) {
        if (g_full) { static int lx = -1, ly, lw, lh, lrh, lrw; int x, y, w, h, rw = 0, rh = 0; dm_ui_rect(&x, &y, &w, &h);
            if (dm_screen_real(&rw, &rh) && (x != lx || y != ly || w != lw || h != lh || rh != lrh || rw != lrw)) { lx = x; ly = y; lw = w; lh = h; lrh = rh; lrw = rw; [v.superview setNeedsLayout]; [v.superview layoutIfNeeded]; } }
        [v tickTouchControls:CACurrentMediaTime()];
        [g_touch_view refreshPortalBinding];
    }
}
@end
static CADisplayLink *g_rate_link;












uint64_t front_game_res(int docked); void mtl_set_ui_rect_fn(void (*fn)(double *y, double *h));
static void ui_rect_frac(double *fy, double *fh) { int x, y, w, h, rw = 0, rh = 0; dm_ui_rect(&x, &y, &w, &h); if (dm_screen_real(&rw, &rh) && rh > 0) { *fy = (double)y / rh; *fh = (double)h / rh; } }
static CGRect letterbox_rect(CGRect f) {
    if (g_full && f.size.width > 0 && f.size.height > 0) {


        int x, y, w, h, rw = 0, rh = 0; dm_ui_rect(&x, &y, &w, &h); CGFloat k = UIScreen.mainScreen.nativeScale, vh = floor(f.size.width * 9 / 16), vy = floor((f.size.height - vh) / 2);
        if (dm_screen_real(&rw, &rh) && h > 0 && h < rh) { vy = round(f.size.height * y / rh * k) / k; vh = round(f.size.height * h / rh * k) / k; }
        else if (rw > 0 && rh > 0 && (double)rw * 9 > (double)rh * 16) { vh = round(f.size.width * rh / rw * k) / k; vy = round((f.size.height - vh) / 2 * k) / k; }
        return CGRectMake(f.origin.x, f.origin.y + vy, f.size.width, vh); }
    uint64_t r = front_game_res(1); double gw = (double)(uint32_t)r, gh = (double)(r >> 32);
    if (f.size.width <= 0 || f.size.height <= 0 || gw <= 0 || gh <= 0) return f;
    double ga = gw / gh, wa = f.size.width / f.size.height; if (fabs(wa / ga - 1) < 0.01) return f;
    CGFloat w = f.size.width, h = f.size.height;
    if (wa > ga) w = floor(h * ga); else h = floor(w / ga);
    return CGRectMake(f.origin.x + floor((f.size.width - w) / 2), f.origin.y + floor((f.size.height - h) / 2), w, h);
}
@interface DmLetterboxView : UIView
@property (nonatomic, strong) UIView *game;
@end

void dm_oe_res_changed(void);

static void window_px(CGSize s, CGFloat k) {
    if (!g_full || getenv("DM_GAME_RES") || s.width <= 0 || s.height <= 0 || k <= 0) return;
    if (UIDevice.currentDevice.userInterfaceIdiom != UIUserInterfaceIdiomPhone && !getenv("DM_TEST_WINDOW_PX")) return;
    double w = round(fmax(s.width, s.height) * k), h = round(fmin(s.width, s.height) * k), cw = (uint32_t)g_res_cap, ch = (double)(g_res_cap >> 32), f = fmin(1, fmin(cw / w, ch / h));
    uint64_t v = (uint64_t)((unsigned)(h * f) & ~1u) << 32 | ((unsigned)(w * f) & ~1u), old = atomic_exchange(&g_res_now, v);
    if (old == v || !old) return;
    LOG("画面：窗口变成 %.0fx%.0f 点 → 出图 %ux%u（原 %ux%u，启动预留 %.0fx%.0f）%s", s.width, s.height, (unsigned)v, (unsigned)(v >> 32), (unsigned)old, (unsigned)(old >> 32), cw, ch,
        dm_uiscreen_res_live() ? "，通知游戏重读" : "，游戏窗口还没建好，建好后按新值");
    if (dm_uiscreen_res_live()) dm_oe_res_changed();
}
@implementation DmLetterboxView
- (void)layoutSubviews { [super layoutSubviews]; window_px(self.bounds.size, UIScreen.mainScreen.nativeScale); if (self.bounds.size.width > 0 && self.bounds.size.height > 0) self.game.frame = letterbox_rect(self.bounds);
    { static CGRect last; CGRect g = self.game.frame; if (g_full && !CGRectEqualToRect(g, last)) { last = g; int x, y, w, h, rw = 0, rh = 0; dm_ui_rect(&x, &y, &w, &h); dm_screen_real(&rw, &rh); LOG("画面：游戏视图摆到 y %.0f 高 %.0f（窗口 %.0fx%.0f；真实画面 %dx%d，界面矩形 y %d 高 %d）", g.origin.y, g.size.height, self.bounds.size.width, self.bounds.size.height, rw, rh, y, h); } }
    if (g_full && g_layer.superlayer == self.layer) { [CATransaction begin]; [CATransaction setDisableActions:YES]; g_layer.frame = self.bounds;
        g_layer.drawableSize = CGSizeMake(self.bounds.size.width * g_layer.contentsScale, self.bounds.size.height * g_layer.contentsScale); [CATransaction commit]; } }


- (UIView *)hitTest:(CGPoint)p withEvent:(UIEvent *)e { UIView *h = [super hitTest:p withEvent:e]; return g_full && h == self ? self.game : h; }
@end


static UIView *test_fold(UIView *top) {
    const char *e = getenv("DM_TEST_FOLD"); if (!e) return top;
    for (const char *q = e; q && *q; q = strchr(q, ',') ? strchr(q, ',') + 1 : NULL) {
        double t = 0, w = 0, h = 0; if (sscanf(q, "%lf:%lfx%lf", &t, &w, &h) != 3 || w <= 0 || h <= 0) continue;
        __weak UIView *wt = top; dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(t * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
            UIView *u = wt; if (!u) return; LOG("测试：视图改成 %.0fx%.0f 点（DM_TEST_FOLD）", w, h); u.frame = CGRectMake(0, 0, w, h); [u setNeedsLayout]; [u layoutIfNeeded]; });
    }
    return top;
}
UIView *front_ios_view(CGRect frame) {
    front_game_res(1);
    CGRect full = frame; frame = letterbox_rect(frame);
    dm_tc_set_tall_scale(UIDevice.currentDevice.userInterfaceIdiom==UIUserInterfaceIdiomPad);
    DmMetalView *v = [[DmMetalView alloc] initWithFrame:frame];
    g_rate_link = [CADisplayLink displayLinkWithTarget:[DmRateHint new] selector:@selector(tick:)];
    g_rate_link.preferredFrameRateRange = CAFrameRateRangeMake(60, 60, 60);
    [g_rate_link addToRunLoop:NSRunLoop.mainRunLoop forMode:NSRunLoopCommonModes];
    g_layer = (CAMetalLayer *)v.layer;
    g_layer.device = MTLCreateSystemDefaultDevice();
    g_layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    g_layer.contentsScale = UIScreen.mainScreen.nativeScale;
    g_layer.drawableSize = CGSizeMake(frame.size.width * g_layer.contentsScale, frame.size.height * g_layer.contentsScale);

    if (g_full || !CGRectEqualToRect(full, frame)) { DmLetterboxView *box = [[DmLetterboxView alloc] initWithFrame:full]; box.backgroundColor = UIColor.blackColor; box.game = v; [box addSubview:v];
        if (g_full) {
            CAMetalLayer *ml = [CAMetalLayer layer]; ml.device = g_layer.device; ml.pixelFormat = MTLPixelFormatBGRA8Unorm; ml.contentsScale = g_layer.contentsScale; ml.frame = box.bounds;
            ml.drawableSize = CGSizeMake(full.size.width * ml.contentsScale, full.size.height * ml.contentsScale); [box.layer insertSublayer:ml atIndex:0];
            v.opaque = NO; v.layer.opaque = NO; v.backgroundColor = UIColor.clearColor; g_layer = ml; mtl_set_ui_rect_fn(ui_rect_frac);
            LOG("画面：窗口 %.0fx%.0f 比 16:9 窄 → 真全屏已开：出图层铺满整窗；游戏视图跟宿主的界面矩形走（局内贴底、局外居中），启动先按居中的 %.0fx%.0f 摆", full.size.width, full.size.height, frame.size.width, frame.size.height); return test_fold(box); }
        LOG("画面：窗口 %.0fx%.0f 与游戏画面比例不同 → 游戏视图 %.0fx%.0f 居中（左右各留 %.0f 点、上下各留 %.0f 点）", full.size.width, full.size.height, frame.size.width, frame.size.height, frame.origin.x - full.origin.x, frame.origin.y - full.origin.y); return test_fold(box); }
    return test_fold(v);
}

void *front_metal_layer(void) { return (__bridge void *)g_layer; }






uint64_t front_game_res(int docked) {
    static double aspect; static unsigned fw, fh;
    if (!aspect) {
        CGRect b = UIScreen.mainScreen.nativeBounds;

        const char *ts = getenv("DM_TEST_SCREEN"); unsigned tw = 0, th = 0; int phone = UIDevice.currentDevice.userInterfaceIdiom == UIUserInterfaceIdiomPhone;
        if (ts && sscanf(ts, "%ux%u", &tw, &th) == 2 && tw && th) { b = CGRectMake(0, 0, tw, th); phone = 1; }
        aspect = fmax(b.size.width, b.size.height) / fmin(b.size.width, b.size.height);
        const char *e = getenv("DM_GAME_RES"); if (e && sscanf(e, "%ux%u", &fw, &fh) == 2) aspect = (double)fw / fh;
        else { fw = (unsigned)fmax(b.size.width, b.size.height); fh = (unsigned)fmin(b.size.width, b.size.height);






            if (aspect < 16.0 / 9 - 0.01) { const char *fu = getenv("DM_IPAD_FULL");
                if (!fu || atoi(fu)) { g_full = 1; dm_uiscreen_enable(DM_UISCREEN_BOTTOM); } else { aspect = 16.0 / 9; fh = (unsigned)llround(fw * 9.0 / 16 / 2) * 2; } } }
        g_res_cap = (uint64_t)fh << 32 | fw;
        if (g_full && !e && phone && fh < 2160) {
            unsigned cw = (unsigned)llround(2160.0 * fw / fh / 2) * 2; g_res_cap = (uint64_t)2160 << 32 | cw;
            LOG("画面：比 16:9 窄的手机屏 %ux%u（折叠屏）→ 窗口按 %ux2160 预留，内外屏切换时出图尺寸在这以内跟着窗口变", fw, fh, cw); }
        atomic_store(&g_res_now, (uint64_t)fh << 32 | fw);
    }
    if (fw && docked) return dm_uiscreen_res_live() ? atomic_load(&g_res_now) : g_res_cap;
    uint64_t h = docked ? 900 : 720, w = (uint64_t)llround(h * aspect / 8) * 8;
    return h << 32 | w;
}

void front_run(void (*guest)(void)) {
    static char alt[65536];
    stack_t ss = {.ss_sp = alt, .ss_size = sizeof alt}; sigaltstack(&ss, 0);




    sigset_t all; sigfillset(&all); pthread_sigmask(SIG_UNBLOCK, &all, NULL);
    void uimsg_subscribe(void (*)(const char *,const char *,const char *));
    uimsg_subscribe(touch_ui_message);
    dm_touch_game_live_check(1);
    if(!dm_touch_game_attach()) LOG("【红】技能绑定读取没挂上（产物没有观测门 OBS_BINDING_GETTER，或 main 模块不符）：技能键图标 / 回城键绑定不工作");
    guest();
}





static UIAlertController *g_text_alert; static void (^g_text_alert_finish)(int);
int front_text_input(const char *title, const char *sub, const char *guide, const char *initial, int max_chars, char *out, int out_cap) {
    if (!g_layer) return -1;
    NSString *t = title && *title ? @(title) : SWT("dmInputTitle");
    NSString *msg = [NSString stringWithFormat:@"%s%s%s", sub ? sub : "", sub && *sub && guide && *guide ? "\n" : "", guide ? guide : ""];
    extern int g_swkbd_name_panel; int name_rule = g_swkbd_name_panel;
    if (name_rule) { NSString *rule = SWT("dmNameRule"); t = title && *title ? @(title) : SWT("dmNameTitle"); msg = msg.length ? [msg stringByAppendingFormat:@"\n%@", rule] : rule; }
    extern int g_swkbd_numeric_max; int num_max = g_swkbd_numeric_max;
    if (num_max >= 0) { if (!(title && *title)) t = SWT("dmQtyTitle"); msg = [NSString stringWithFormat:@"0 – %d", num_max]; }
    NSString *init = initial ? @(initial) : @"", *ph = name_rule ? SWT("dmNameBlankRandom") : guide && *guide ? @(guide) : nil;
    dispatch_semaphore_t done = dispatch_semaphore_create(0);
    __block int r = 0; __block NSString *text = nil;
    dispatch_async(dispatch_get_main_queue(), ^{
        UIAlertController *a = [UIAlertController alertControllerWithTitle:t message:msg preferredStyle:UIAlertControllerStyleAlert];


        a.overrideUserInterfaceStyle = UIUserInterfaceStyleDark;
        a.view.tintColor = [UIColor colorWithRed:199 / 255.0 green:179 / 255.0 blue:119 / 255.0 alpha:1];






        __block UIAlertAction *okAct = nil; __weak UIAlertController *wa = a;
        void (^name_check)(UITextField *) = ^(UITextField *tf) {
            if (!name_rule || !okAct) return;
            NSString *x = tf.text ?: @""; NSUInteger nb = [x lengthOfBytesUsingEncoding:NSUTF8StringEncoding]; NSString *why = nil;
            unichar c0 = x.length ? [x characterAtIndex:0] : 0, c1 = x.length ? [x characterAtIndex:x.length - 1] : 0;
            if (tf.markedTextRange) why = SWT("dmNamePickIme");
            else if (nb == 0) why = nil;
            else if (nb < 2) why = SWT("dmNameTooShort");
            else if (c0 == '-' || c0 == '_' || c1 == '-' || c1 == '_') why = SWT("dmNameEdge");
            okAct.enabled = why == nil;
            wa.message = why ? [msg stringByAppendingFormat:@"\n▸ %@", why] : msg;
        };
        [a addTextFieldWithConfigurationHandler:^(UITextField *f) {
            f.text = init; f.placeholder = ph; f.clearButtonMode = UITextFieldViewModeWhileEditing; f.keyboardAppearance = UIKeyboardAppearanceDark;
            if (name_rule) { f.autocorrectionType = UITextAutocorrectionTypeNo; f.autocapitalizationType = UITextAutocapitalizationTypeNone; f.spellCheckingType = UITextSpellCheckingTypeNo;
                [f addAction:[UIAction actionWithHandler:^(UIAction *act) {
                    UITextField *tf = (UITextField *)act.sender; if (tf.markedTextRange) return;
                    NSMutableString *o = [NSMutableString string]; int dash = 0; NSUInteger bytes = 0;
                    for (NSUInteger i = 0; i < tf.text.length; i++) { unichar c = [tf.text characterAtIndex:i]; NSUInteger nb = 0;
                        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) nb = 1;
                        else if ((c == '-' || c == '_') && !dash) { nb = 1; dash = 1; }
                        else if ((c >= 0x4E00 && c <= 0x9FFF) || (c >= 0x3400 && c <= 0x4DBF)) nb = 3;
                        if (!nb || bytes + nb > 15) continue;
                        [o appendFormat:@"%C", c]; bytes += nb; }
                    if (![o isEqualToString:tf.text]) tf.text = o;
                }] forControlEvents:UIControlEventEditingChanged]; }
            if (name_rule) [f addAction:[UIAction actionWithHandler:^(UIAction *act) { name_check((UITextField *)act.sender); }] forControlEvents:UIControlEventEditingChanged];
            if (num_max >= 0) { f.keyboardType = UIKeyboardTypeNumberPad;
                [f addAction:[UIAction actionWithHandler:^(UIAction *act) {
                    UITextField *tf = (UITextField *)act.sender; NSMutableString *o = [NSMutableString string]; long long v = 0;
                    for (NSUInteger i = 0; i < tf.text.length && o.length < 10; i++) { unichar c = [tf.text characterAtIndex:i]; if (c >= '0' && c <= '9') { [o appendFormat:@"%C", c]; v = v * 10 + (c - '0'); } }
                    if (v > num_max) [o setString:[NSString stringWithFormat:@"%d", num_max]];
                    if (![o isEqualToString:tf.text]) tf.text = o;
                }] forControlEvents:UIControlEventEditingChanged]; }


            if (max_chars > 0) [f addAction:[UIAction actionWithHandler:^(UIAction *act) {
                UITextField *tf = (UITextField *)act.sender;
                if (tf.markedTextRange || (int)tf.text.length <= max_chars) return;
                NSRange keep = [tf.text rangeOfComposedCharacterSequencesForRange:NSMakeRange(0, (NSUInteger)max_chars)];
                if ((int)NSMaxRange(keep) > max_chars) keep = [tf.text rangeOfComposedCharacterSequencesForRange:NSMakeRange(0, (NSUInteger)max_chars - 1)];
                tf.text = [tf.text substringWithRange:keep];
            }] forControlEvents:UIControlEventEditingChanged];
        }];
        void (^finish)(int) = ^(int ok) { UIAlertController *sa = wa; g_text_alert = nil; g_text_alert_finish = nil; if (ok) text = sa.textFields.firstObject.text;




            if (ok && name_rule && !text.length) { char nm[32]; if (dm_name_gen(nm, sizeof nm, dm_name_taken)) { text = @(nm); extern _Atomic int g_fe_name_no_accept; atomic_store(&g_fe_name_no_accept, 1); LOG("起名：随机名 → 名字框「%s」（名字留空点确定）", nm); } else { ok = 0; LOG("起名：随机名 → 名字框「」【红】（已有同名角色，试满 64 次仍重名），名字框不动"); } }
            r = ok; dispatch_semaphore_signal(done); };
        g_text_alert = a; g_text_alert_finish = [finish copy];
        [a addAction:[UIAlertAction actionWithTitle:SWT("dmCancel") style:UIAlertActionStyleCancel handler:^(UIAlertAction *x) { (void)x; finish(0); }]];
        okAct = [UIAlertAction actionWithTitle:SWT("dmConfirm") style:UIAlertActionStyleDefault handler:^(UIAlertAction *x) { (void)x; finish(1); }];
        [a addAction:okAct]; name_check(a.textFields.firstObject);






        UIViewController *vc = nil, *alt = nil, *any = nil;
        for (UIScene *s in UIApplication.sharedApplication.connectedScenes)
            if ([s isKindOfClass:UIWindowScene.class]) for (UIWindow *w in ((UIWindowScene *)s).windows) {
                if (!w.rootViewController) continue;
                if (w.isKeyWindow) vc = w.rootViewController;
                else if (s.activationState == UISceneActivationStateForegroundActive && !alt) alt = w.rootViewController;
                else if (!any) any = w.rootViewController; }
        if (!vc) { vc = alt ?: any; LOG("前端：系统输入框——此刻没有主窗口，改用%s", alt ? "前台活跃场景的窗口" : any ? "任一窗口" : "（也没有）"); }
        while (vc.presentedViewController) vc = vc.presentedViewController;
        if (!vc) { LOG("前端：【红】系统输入框没找到可弹的界面，按取消返回（游戏那边的输入框保持原值）"); r = 0; dispatch_semaphore_signal(done); return; }
        [vc presentViewController:a animated:YES completion:^{




            const char *tk = getenv("DM_TEST_KBD_TYPE"); if (!tk) return;
            NSString *typed = @(tk);
            dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 1500000000), dispatch_get_main_queue(), ^{
                int cancel = [typed isEqualToString:@":cancel"];
                UITextField *tf = a.textFields.firstObject;
                if (!cancel) { tf.text = typed; [tf sendActionsForControlEvents:UIControlEventEditingChanged]; }
                LOG("前端：测试键盘输入（DM_TEST_KBD_TYPE）%s「%s」→ 文本框现为「%s」", cancel ? "取消" : "填入", cancel ? "" : typed.UTF8String, tf.text.UTF8String ?: "");
                if (!cancel && okAct && !okAct.enabled) { LOG("前端：测试键盘输入——「确定」是灰的（%s），不点；1.5 秒后改按取消", a.message.UTF8String ?: ""); cancel = 1; }
                [a dismissViewControllerAnimated:NO completion:^{ finish(!cancel); }];
            });
        }];
    });
    dispatch_semaphore_wait(done, DISPATCH_TIME_FOREVER);
    if (!r) return 0;
    snprintf(out, out_cap, "%s", text.UTF8String ? text.UTF8String : "");
    return 1;
}





















#import <AVFoundation/AVFoundation.h>





void front_save_failed(int err) {
    LOG("前端：存档写失败 errno=%d，异步提示玩家", err);
    dispatch_async(dispatch_get_main_queue(), ^{
        UIViewController *vc = nil;
        for (UIScene *s in UIApplication.sharedApplication.connectedScenes)
            if ([s isKindOfClass:UIWindowScene.class]) for (UIWindow *w in ((UIWindowScene *)s).windows) if (w.isKeyWindow) vc = w.rootViewController;
        while (vc.presentedViewController) vc = vc.presentedViewController;
        if (!vc) { LOG("【红】前端：存档失败提示没有窗口 errno=%d", err); return; }
        UIAlertController *a = [UIAlertController alertControllerWithTitle:SWT("dmSaveNoSpace") message:nil preferredStyle:UIAlertControllerStyleAlert];
        [a addAction:[UIAlertAction actionWithTitle:SWT("dmConfirm") style:UIAlertActionStyleDefault handler:nil]];
        [vc presentViewController:a animated:YES completion:^{ LOG("前端：存档失败提示已显示 errno=%d", err); }];
    });
}

#define THERMAL_RESTORE_SEC 60

static void thermal_apply(void) {
    static unsigned gen;
    NSProcessInfoThermalState s = NSProcessInfo.processInfo.thermalState;
    unsigned my = ++gen;
    if (s >= NSProcessInfoThermalStateSerious) {
        if (getenv("DM_THERMAL_NOFALLBACK")) { LOG("前端：过热 %ld，DM_THERMAL_NOFALLBACK 不降档（只供功耗测量）", (long)s); return; }
        LOG("前端：过热 %ld（3D 比例改由实际掉帧驱动，这里只记一行）", (long)s); return; }
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)THERMAL_RESTORE_SEC * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
        if (my == gen && NSProcessInfo.processInfo.thermalState < NSProcessInfoThermalStateSerious) { LOG("前端：温度回落持续 %d 秒", THERMAL_RESTORE_SEC); }
    });
}





static int g_audio_bg, g_audio_intr;
void guest_clock_freeze_for(int reason, int on);
static void audio_apply(void) { dm_audio_suspend(g_audio_bg || g_audio_intr); }




static void background_wait_save(void) {
    int dm_ingame_now(void); void mtl_present_discard(int on); int dm_save_wait(double max_s, uint64_t *commits, int *froze, double *ms);
    if (!dm_ingame_now()) return;
    uint64_t n = 0; int froze = 0; double ms = 0;
    mtl_present_discard(1);
    dm_save_wait(2.5, &n, &froze, &ms);
    mtl_present_discard(0);
    LOG("前端：进后台等游戏存盘 %.0f ms，期间存档提交 %llu 次%s%s", ms, (unsigned long long)n, froze ? "（原先暂停着：冻结世界存盘、已恢复暂停）" : "", n ? "" : "【红】（2.5 秒内没等到）");
}






static void resign_save(void) {
    int dm_ingame_now(void); int dm_request_save(void); uint64_t now_ns(void);
    static uint64_t last; uint64_t now = now_ns();
    if (!dm_ingame_now() || (last && now - last < 30000000000ull)) return;
    if (dm_request_save()) { last = now; LOG("前端：失活，请游戏下一拍存一次盘（不等）"); }
}


void front_ios_focus_event(int ev) {
    static const char *k[] = {"sceneWillResignActive", "sceneDidEnterBackground", "sceneWillEnterForeground", "sceneDidBecomeActive"};
    { uint64_t guest_ns(void); LOG("前端：%s（客体时钟 %.3f s，声卡回调累计 %llu）", k[ev], guest_ns() / 1e9, (unsigned long long)dm_audio_render_calls()); }
    switch (ev) {
        case 0: [g_touch_view cancelControls]; { void dm_gamepad_reset(void); dm_gamepad_reset(); } dm_focus_set(2); resign_save(); break;


        case 1: dm_focus_set(3); dm_ui_lifecycle(1); g_audio_bg = 1; audio_apply(); { void dm_cloud_flush(int); dm_cloud_flush(1); }
            background_wait_save();
            guest_clock_freeze_for(1, 1); dm_gfx_suspend(1); break;
        case 2: dm_ui_lifecycle(0); { void dm_update_foreground(void); dm_update_foreground(); void dm_cloud_foreground(void); dm_cloud_foreground(); }
            { void dm_3d_scale_resumed(void); dm_3d_scale_resumed(); } dm_gfx_suspend(0); guest_clock_freeze_for(1, 0); g_audio_bg = g_audio_intr = 0; audio_apply(); dm_focus_set(2); break;
        case 3: dm_focus_set(1); audio_apply(); break;
    }
}











void mtl_grab_next_present(void (^cb)(CGImageRef img));
static void window_shot(NSString *name) {
    UIWindowScene *ws = nil; UIWindow *win = nil;
    for (UIScene *s in UIApplication.sharedApplication.connectedScenes) if ([s isKindOfClass:UIWindowScene.class]) ws = (UIWindowScene *)s;
    for (UIWindow *w in ws.windows) if (w.isKeyWindow) win = w;
    CALayer *gl = (__bridge CALayer *)front_metal_layer();
    if (!win || !gl) { LOG("【红】DM_SCREEN_SHOT：没有窗口 / 出图层"); return; }
    static uint64_t seq; uint64_t my = ++seq; __block int got = 0;
    mtl_grab_next_present(^(CGImageRef game) {
        if (game) CGImageRetain(game);
        dispatch_async(dispatch_get_main_queue(), ^{
            if (my != seq || got) { if (game) CGImageRelease(game); return; } got = 1;
            if (!game) { LOG("【红】DM_SCREEN_SHOT：%s 没拿到游戏画面", name.UTF8String); return; }
            NSMutableArray<CALayer *> *above = [NSMutableArray arrayWithArray:gl.sublayers ?: @[]];
            for (CALayer *ch = gl, *p = gl.superlayer; p; ch = p, p = p.superlayer) {
                NSUInteger i = [p.sublayers indexOfObject:ch]; if (i != NSNotFound) for (NSUInteger j = i + 1; j < p.sublayers.count; j++) [above addObject:p.sublayers[j]];
                if (p == win.layer) break; }
            UIGraphicsImageRendererFormat *fm = [UIGraphicsImageRendererFormat defaultFormat]; fm.scale = ws.screen.scale; fm.opaque = YES;
            UIImage *img = [[[UIGraphicsImageRenderer alloc] initWithSize:win.bounds.size format:fm] imageWithActions:^(UIGraphicsImageRendererContext *c) {
                CGContextRef cg = c.CGContext; [UIColor.blackColor setFill]; UIRectFill(win.bounds);
                [[UIImage imageWithCGImage:game] drawInRect:[gl convertRect:gl.bounds toLayer:win.layer]];
                for (CALayer *l in above) { if (l.hidden || l.opacity <= 0) continue; CGRect r = [l convertRect:l.bounds toLayer:win.layer];
                    CGContextSaveGState(cg); CGContextTranslateCTM(cg, r.origin.x, r.origin.y); [l renderInContext:cg]; CGContextRestoreGState(cg); } }];
            CGImageRelease(game);
            NSString *doc = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES)[0];
            [UIImagePNGRepresentation(img) writeToFile:[doc stringByAppendingPathComponent:name] atomically:YES];
            UIEdgeInsets si = win.safeAreaInsets;
            NSString *meta = [NSString stringWithFormat:@"window_pt\t%.0f\t%.0f\nscale\t%.0f\norientation\t%ld\nsafe_pt\t%.1f\t%.1f\t%.1f\t%.1f\n", win.bounds.size.width, win.bounds.size.height,
                              ws.screen.scale, (long)ws.interfaceOrientation, si.top, si.left, si.bottom, si.right];
            [meta writeToFile:[doc stringByAppendingPathComponent:[name stringByAppendingString:@".txt"]] atomically:YES encoding:NSUTF8StringEncoding error:nil];
            LOG("DM_SCREEN_SHOT：已截 %s（%.0fx%.0f 点 ×%.0f，方向 %ld；游戏画面取自上屏帧）", name.UTF8String, win.bounds.size.width, win.bounds.size.height, ws.screen.scale, (long)ws.interfaceOrientation);
        });
    });
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC), dispatch_get_main_queue(), ^{
        if (my != seq || got) return; got = 1; mtl_grab_next_present(nil);
        LOG("【红】DM_SCREEN_SHOT：%s 1 秒内没有上屏帧（显示节拍停了 / 在后台？），没截", name.UTF8String); });
}







static void touch_script_start(void) {
    static int started; const char *e=getenv("DM_TOUCH_SCRIPT"); if(started || !e) return; started=1;
    NSString *doc=NSSearchPathForDirectoriesInDomains(NSDocumentDirectory,NSUserDomainMask,YES)[0];
    NSError *rerr=nil; NSString *spath=[doc stringByAppendingPathComponent:@(e)];
    NSString *text=[NSString stringWithContentsOfFile:spath encoding:NSUTF8StringEncoding error:&rerr];
    if(!text) { NSDictionary *fa=[NSFileManager.defaultManager attributesOfFileSystemForPath:doc error:nil];
        LOG("[script] 【红】读不到脚本 %s：%s（路径 %s，存在 %d；主目录 %s；所在卷剩余 %.2f GB）",e,rerr.description.UTF8String ?: "无错误信息",spath.UTF8String,(int)[NSFileManager.defaultManager fileExistsAtPath:spath],NSHomeDirectory().UTF8String,[fa[NSFileSystemFreeSize] doubleValue]/1e9); return; }


    NSMutableArray<NSArray *> *steps=[NSMutableArray array];
    for(NSString *raw in [text componentsSeparatedByString:@"\n"]) {
        NSString *line=[raw stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceCharacterSet];
        if(!line.length || [line hasPrefix:@"#"]) continue;
        [steps addObject:[line componentsSeparatedByCharactersInSet:NSCharacterSet.whitespaceCharacterSet]];
    }
    [steps sortWithOptions:NSSortStable usingComparator:^NSComparisonResult(NSArray *a, NSArray *b) { return [@([a[0] doubleValue]) compare:@([b[0] doubleValue])]; }];
    int n=(int)steps.count; __block NSUInteger next=0; __block double t0=CACurrentMediaTime();




    __block int wk_on=0, wk_tx=0, wk_ty=0, wk_r=6, wk_tick=0; __block double wk_dead=0;






    __block int tu_on=0, tu_tick=0, tu_rect=0; __block double tu_dead=0, tu_up=0, tu_dx=0, tu_dy=0; static double tu_r[4];
    __block int ui_on=0, ui_tick=0; __block double ui_dead=0; __block NSString *ui_name=nil;




    __block int aw_on=0, aw_x=0, aw_y=0, aw_r=0, aw_tick=0, wk_skip=0;




    __block int awr_on=0, awr_base=0, awr_tick=0; __block double awr_dead=0;




    __block int aui_on=0, aui_tick=0; __block double aui_dead=0; __block NSString *aui_name=nil;
    void (^skip_wait)(const char *)=^(const char *why) { if(next>=steps.count) return; double T=[steps[next][0] doubleValue], nw=CACurrentMediaTime()-t0;
        if(T>nw+0.3) { t0-=T-nw-0.3; LOG("[script] %s：跳过剩余等待 %.1f 秒",why,T-nw-0.3); } };
    NSTimer *timer=[NSTimer timerWithTimeInterval:1.0/120 repeats:YES block:^(NSTimer *tm) {
        double now=CACurrentMediaTime()-t0;
        if(wk_on && (++wk_tick%12)==0) { dm_hud_info h; dm_hud_get(&h); double dx=wk_tx-h.x, dy=wk_ty-h.y, d=hypot(dx,dy);
            if(!h.valid || d<=wk_r || now>=wk_dead) { [g_touch_view scriptPhase:@"up" slot:15 at:nil]; wk_on=0;
                LOG("[script] walk_to (%d,%d) %s：现在 区域 %d (%d,%d)，差 %.0f",wk_tx,wk_ty,!h.valid?"玩家状态读不到":d<=wk_r?"到了":"超时",h.area,h.x,h.y,d); if(wk_skip && h.valid && d<=wk_r) skip_wait("walk_to 到了"); }
            else { double sx=dx-dy, sy=(dx+dy)/2, m=hypot(sx,sy); if(m<1e-6) m=1; [g_touch_view scriptPhase:@"move" slot:15 at:[NSString stringWithFormat:@"@joy+%d,%d",(int)lround(sx/m*40),(int)lround(sy/m*40)]]; } }
        if(aw_on && (++aw_tick%12)==0) { dm_hud_info h; dm_hud_get(&h);
            if(h.valid && h.area>0 && h.hp_max>0 && (!aw_r || hypot(aw_x-h.x,aw_y-h.y)<=aw_r)) { aw_on=0; LOG("[script] await pos：区域 %d (%d,%d)",h.area,h.x,h.y); skip_wait("await pos 等到了"); } }
        if(awr_on && (++awr_tick%12)==0) { int dm_runeword_done_count(void); int c=dm_runeword_done_count();
            if(c>awr_base) { awr_on=0; LOG("[script] await rw：一键合成结束（第 %d 次）",c); skip_wait("await rw 等到了"); }
            else if(now>=awr_dead) { awr_on=0; LOG("[script] 【红】await rw 超时：一键合成没结束"); } }
        if(aui_on && (++aui_tick%12)==0) { float nx=0, ny=0;
            if(dm_ui_watch_pos(&nx,&ny)) { aui_on=0; dm_ui_watch(NULL); LOG("[script] await ui %s：控件出现了",aui_name.UTF8String); skip_wait("await ui 等到了"); }
            else if(now>=aui_dead) { aui_on=0; dm_ui_watch(NULL); LOG("[script] 【红】await ui %s 超时：控件没出现",aui_name.UTF8String); } }
        if(tu_up && now>=tu_up) { [g_touch_view scriptPhase:@"up" slot:0 at:nil]; tu_up=0; }
        if(tu_on && (++tu_tick%12)==0) { float nx=0, ny=0; CGSize vs=g_touch_view.bounds.size; int have=dm_world_watch_pos(&nx,&ny); CGRect wr=world_rect(g_touch_view); double px=wr.origin.x+nx*wr.size.width+tu_dx, py=wr.origin.y+ny*wr.size.height+tu_dy; (void)vs;
            if(have && (!tu_rect || (px>=tu_r[0] && py>=tu_r[1] && px<=tu_r[2] && py<=tu_r[3]))) { tu_on=0; dm_world_watch(-1,0);
                LOG("[script] tap_unit：单位在窗口点 (%.0f,%.0f)，落触点",px,py); [g_touch_view scriptPhase:@"down" slot:0 at:[NSString stringWithFormat:@"=%.1f,%.1f",px,py]]; tu_up=now+0.12; }
            else if(now>=tu_dead) { tu_on=0; dm_world_watch(-1,0); if(have) LOG("[script] tap_unit 超时：单位在窗口点 (%.0f,%.0f)，没进指定矩形，没点",px,py); else LOG("[script] tap_unit 超时：画面里没有这类单位，没点"); } }




        if(ui_on && (++ui_tick%12)==0) { float nx=0, ny=0; CGSize vs=g_touch_view.bounds.size;
            if(dm_ui_watch_pos(&nx,&ny)) { ui_on=0; dm_ui_watch(NULL); double px=nx*vs.width, py=ny*vs.height;
                LOG("[script] tap_ui %s：控件在视图点 (%.0f,%.0f)，落触点",ui_name.UTF8String,px,py); [g_touch_view scriptPhase:@"down" slot:0 at:[NSString stringWithFormat:@"=%.1f,%.1f",px,py]]; tu_up=now+0.12; }
            else if(now>=ui_dead) { ui_on=0; dm_ui_watch(NULL); LOG("[script] 【红】tap_ui %s 超时：没有这个可见控件，没点",ui_name.UTF8String); } }




        if(next<steps.count && [steps[next][0] doubleValue]<=now) { double late=now-[steps[next][0] doubleValue];
            if(late>0.02) { t0+=late; now-=late; static int lg; if(late>0.1 && lg++<40) LOG("[script] 落后 %.2f 秒：后面各步顺延",late); } }
        while(next<steps.count && [steps[next][0] doubleValue]<=now) {
            NSArray *f=steps[next++]; NSString *op=f.count>1?f[1]:@""; aw_on=0;
            if([op isEqualToString:@"shot"]) window_shot(f[2]);
            else if([op isEqualToString:@"burst"] && f.count>=4) { void dm_gfx_burst(int, const char *); dm_gfx_burst([f[2] intValue], [doc stringByAppendingPathComponent:f[3]].UTF8String); LOG("[script] burst %d 帧 → %s", [f[2] intValue], [f[3] UTF8String]); }
            else if([op isEqualToString:@"tshot"]) [g_touch_view captureTouchLayer:[doc stringByAppendingPathComponent:f[2]]];




            else if([op isEqualToString:@"walk_to"] && f.count>=4) { wk_tx=[f[2] intValue]; wk_ty=[f[3] intValue]; wk_r=f.count>4?[f[4] intValue]:6; wk_dead=now+(f.count>5?[f[5] doubleValue]:15); wk_tick=0; wk_skip=f.count>6 && [f[6] isEqualToString:@"skip"];
                if(wk_on) [g_touch_view scriptPhase:@"up" slot:15 at:nil];
                [g_touch_view scriptPhase:@"down" slot:15 at:@"@joy"]; wk_on=1; }
            else if([op isEqualToString:@"tap_unit"] && f.count>=4) { dm_world_watch([f[2] intValue],(uint32_t)[f[3] intValue]); tu_on=1; tu_tick=0; tu_dead=now+(f.count>4?[f[4] doubleValue]:10);
                NSString *last=f.lastObject; tu_dx=tu_dy=0; if([last hasPrefix:@"+"]) { NSArray *o=[[last substringFromIndex:1] componentsSeparatedByString:@","]; if(o.count==2) { tu_dx=[o[0] doubleValue]; tu_dy=[o[1] doubleValue]; } }
                tu_rect=f.count>=9 && ![f[5] hasPrefix:@"+"]; if(tu_rect) for(int i=0;i<4;i++) tu_r[i]=[f[5+i] doubleValue]; }




            else if([op isEqualToString:@"tap_ui"] && f.count>=3) { ui_name=[f[2] stringByReplacingOccurrencesOfString:@"+" withString:@" "]; dm_ui_watch(ui_name.UTF8String); ui_on=1; ui_tick=0; ui_dead=now+(f.count>3?[f[3] doubleValue]:10); }
            else if([op isEqualToString:@"await"] && f.count>=4 && [f[2] isEqualToString:@"ui"]) { aui_name=[f[3] stringByReplacingOccurrencesOfString:@"+" withString:@" "]; dm_ui_watch(aui_name.UTF8String); aui_on=1; aui_tick=0; aui_dead=now+(f.count>4?[f[4] doubleValue]:30); }
            else if([op isEqualToString:@"await"] && f.count>=3 && [f[2] isEqualToString:@"rw"]) { int dm_runeword_done_count(void); awr_on=1; awr_tick=0; awr_base=dm_runeword_done_count(); awr_dead=now+(f.count>3?[f[3] doubleValue]:30); }
            else if([op isEqualToString:@"await"] && f.count>=3 && [f[2] isEqualToString:@"pos"]) { aw_on=1; aw_tick=0; aw_r=f.count>=6?[f[5] intValue]:0; aw_x=f.count>=6?[f[3] intValue]:0; aw_y=f.count>=6?[f[4] intValue]:0; }
            else if([op isEqualToString:@"pad"] && f.count>=3) { void dm_gamepad_inject(uint64_t,int32_t,int32_t,int32_t,int32_t); uint64_t m=strtoull([f[2] UTF8String],NULL,0);
                dm_gamepad_inject(m,f.count>3?[f[3] intValue]:0,f.count>4?[f[4] intValue]:0,f.count>5?[f[5] intValue]:0,f.count>6?[f[6] intValue]:0); LOG("[script] pad %#llx",(unsigned long long)m); }
            else if([op isEqualToString:@"pad2"] && f.count>=3) { void dm_gamepad_inject_n(int,uint64_t,int32_t,int32_t,int32_t,int32_t); uint64_t m=strtoull([f[2] UTF8String],NULL,0);
                dm_gamepad_inject_n(2,m,f.count>3?[f[3] intValue]:0,f.count>4?[f[4] intValue]:0,f.count>5?[f[5] intValue]:0,f.count>6?[f[6] intValue]:0); LOG("[script] pad2 %#llx",(unsigned long long)m); }
            else if([op isEqualToString:@"padoff"] || [op isEqualToString:@"padoff2"]) { void dm_gamepad_drop_n(int); dm_gamepad_drop_n([op isEqualToString:@"padoff2"]?2:1); LOG("[script] %s",op.UTF8String); }
            else if([op isEqualToString:@"focus"] && f.count>=3) { front_ios_focus_event([f[2] intValue]); LOG("[script] focus %s",[f[2] UTF8String]); }
            else if([op isEqualToString:@"padcur"] && f.count>=4) { void dm_padmode_test_cursor(float,float); dm_padmode_test_cursor([f[2] floatValue],[f[3] floatValue]); LOG("[script] padcur %s %s",[f[2] UTF8String],[f[3] UTF8String]); }
            else if([op isEqualToString:@"pos"]) { dm_hud_info h; dm_hud_get(&h); LOG("[script] pos 区域 %d (%d,%d) 生命 %d/%d",h.area,h.x,h.y,h.hp,h.hp_max); }
            else if([op isEqualToString:@"type"]) { UITextField *tf=g_text_alert.textFields.firstObject; NSString *tx=f.count>2?[[f subarrayWithRange:NSMakeRange(2,f.count-2)] componentsJoinedByString:@" "]:@"";
                if(tf) { for(NSUInteger i=0;i<tx.length;) { NSRange r=[tx rangeOfComposedCharacterSequenceAtIndex:i]; [tf insertText:[tx substringWithRange:r]]; i=NSMaxRange(r); } LOG("[script] type「%s」→ 框内「%s」",tx.UTF8String,tf.text.UTF8String); }
                else LOG("[script] type：没有弹着的输入对话框"); }
            else if([op isEqualToString:@"clear"]) { UITextField *tf=g_text_alert.textFields.firstObject; if(tf) { while(tf.text.length) [tf deleteBackward]; LOG("[script] clear → 框内空"); } else LOG("[script] clear：没有弹着的输入对话框"); }
            else if([op isEqualToString:@"alert"]) { int ok=f.count>2 && [f[2] isEqualToString:@"ok"]; UIAlertController *al=g_text_alert; void (^fin)(int)=g_text_alert_finish;
                UIAlertAction *okAct=nil; for(UIAlertAction *x in al.actions) if(x.style==UIAlertActionStyleDefault) okAct=x;
                if(al && fin && ok && okAct && !okAct.enabled) LOG("[script] alert 确定：按钮是灰的（框内「%s」），没点",al.textFields.firstObject.text.UTF8String);
                else if(al && fin) { LOG("[script] alert %s（框内「%s」）",ok?"确定":"取消",al.textFields.firstObject.text.UTF8String); [al dismissViewControllerAnimated:NO completion:^{ fin(ok); }]; }
                else LOG("[script] alert：没有弹着的输入对话框"); }
            else if([op isEqualToString:@"note"]) LOG("[script] note %s",[[f subarrayWithRange:NSMakeRange(2,f.count-2)] componentsJoinedByString:@" "].UTF8String);
            else [g_touch_view scriptPhase:op slot:[f[2] intValue] at:f.count>3?f[3]:nil];
        }




        if(next>=steps.count && !ui_on && !tu_on && !wk_on && !aw_on && !awr_on && !aui_on && !tu_up) { LOG("[script] 全部 %d 步执行完（%.2fs）",(int)steps.count,now); [tm invalidate]; }
    }];
    timer.tolerance=0; [NSRunLoop.mainRunLoop addTimer:timer forMode:NSRunLoopCommonModes];
    LOG("[script] 已排 %d 步（%s）",n,e);
}

void front_ios_lifecycle_install(void) {
    front_game_res(1);
    UIApplication.sharedApplication.idleTimerDisabled = YES;
    { uint64_t mtl_present_count(void); void front_ios_heat_start(uint64_t (*)(void), int); front_ios_heat_start(mtl_present_count, 1); }


    { const char *br = getenv("DM_BRIGHTNESS"); double want = br ? atof(br) : -1;
      [NSTimer scheduledTimerWithTimeInterval:2 repeats:YES block:^(NSTimer *tm) { (void)tm; double cur = UIScreen.mainScreen.brightness;
          if (want >= 0 && fabs(cur - want) > 0.01) { UIScreen.mainScreen.brightness = want; LOG("亮度 %.2f → 重设 %.2f（DM_BRIGHTNESS）", cur, want); }
          g_dm_brightness = UIScreen.mainScreen.brightness; }]; }


    { g_dm_thermal = (int)NSProcessInfo.processInfo.thermalState; LOG("温控档位 %d（启动）", g_dm_thermal); LOG("持续执行档：%d（1 = 系统已按稳态功率限制本进程）", (int)[NSProcessInfo.processInfo hasPerformanceProfile:NSProcessPerformanceProfileSustained]);
      [NSNotificationCenter.defaultCenter addObserverForName:NSProcessInfoThermalStateDidChangeNotification object:nil queue:nil usingBlock:^(NSNotification *n) { (void)n;
          int t = (int)NSProcessInfo.processInfo.thermalState; if (t != g_dm_thermal) { LOG("温控档位 %d → %d", g_dm_thermal, t); g_dm_thermal = t; } }]; }


    if (getenv("DM_SOAK")) {
        uint64_t t0 = now_ns();
        [NSTimer scheduledTimerWithTimeInterval:60 repeats:YES block:^(NSTimer *tm) { (void)tm;
            LOG("[浸泡] 第 %.0f 秒 温控档位 %ld", (now_ns() - t0) / 1e9, (long)NSProcessInfo.processInfo.thermalState); }];
    }




    const char *touchShot=getenv("DM_TOUCH_SHOT"); double touchAt; char touchName[256];
    if(touchShot && sscanf(touchShot,"%lf:%255s",&touchAt,touchName)==2) {
        NSString *file=@(touchName);
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW,(int64_t)(touchAt*1e9)),dispatch_get_main_queue(),^{
            NSString *docs=NSSearchPathForDirectoriesInDomains(NSDocumentDirectory,NSUserDomainMask,YES).firstObject;
            [g_touch_view captureTouchLayer:[docs stringByAppendingPathComponent:file]];
        });
    }
    const char *ss = getenv("DM_SCREEN_SHOT"); char sn[256]; double st;
    if (ss && sscanf(ss, "%lf:%255s", &st, sn) == 2) {
        NSString *name = @(sn);
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(st * 1e9)), dispatch_get_main_queue(), ^{ window_shot(name); });
    }    NSNotificationCenter *nc = NSNotificationCenter.defaultCenter; NSOperationQueue *mq = NSOperationQueue.mainQueue;
    [nc addObserverForName:NSProcessInfoThermalStateDidChangeNotification object:nil queue:mq usingBlock:^(NSNotification *n) { (void)n; thermal_apply(); }];
    if (NSProcessInfo.processInfo.thermalState >= NSProcessInfoThermalStateSerious) thermal_apply();

    AVAudioSession *as = AVAudioSession.sharedInstance; NSError *e = nil;




    if (![as setCategory:AVAudioSessionCategoryPlayback withOptions:AVAudioSessionCategoryOptionMixWithOthers error:&e] || ![as setActive:YES error:&e]) LOG("前端：音频会话设置失败 %s", e.localizedDescription.UTF8String);
    LOG("前端：音频会话 类别 %s 选项 %#lx（MixWithOthers %s）、别的 App 正在出声 %d", as.category.UTF8String, (unsigned long)as.categoryOptions, (as.categoryOptions & AVAudioSessionCategoryOptionMixWithOthers) ? "开" : "关", as.isOtherAudioPlaying);
    [nc addObserverForName:AVAudioSessionInterruptionNotification object:as queue:mq usingBlock:^(NSNotification *n) {
        AVAudioSessionInterruptionType t = [n.userInfo[AVAudioSessionInterruptionTypeKey] unsignedIntegerValue];
        if (t == AVAudioSessionInterruptionTypeBegan) { LOG("前端：音频被打断"); g_audio_intr = 1; audio_apply(); return; }


        g_audio_intr = 0; [AVAudioSession.sharedInstance setActive:YES error:nil]; audio_apply(); LOG("前端：打断结束%s", g_audio_bg ? "（仍在后台，回前台再出声）" : "，恢复音频");
    }];


    static DmVolumeWatch *vw; vw = [DmVolumeWatch new]; [as addObserver:vw forKeyPath:@"outputVolume" options:NSKeyValueObservingOptionOld | NSKeyValueObservingOptionNew context:NULL];
    [nc addObserverForName:AVAudioSessionRouteChangeNotification object:as queue:mq usingBlock:^(NSNotification *n) {
        double mtl_link_tick_age_ms(void);
        LOG("前端：音频路由变化 原因 %lu（显示节拍上次回调 %.0f ms 前）", (unsigned long)[n.userInfo[AVAudioSessionRouteChangeReasonKey] unsignedIntegerValue], mtl_link_tick_age_ms()); }];
    LOG("前端：生命周期 / 过热 / 音频会话已接（类别 Playback，与别的 App 混音）");








    const char *tb = getenv("DM_TEST_BG"); double tb_at, tb_dur;
    for (const char *tp = tb; tp && sscanf(tp, "%lf,%lf", &tb_at, &tb_dur) == 2; tp = strchr(tp, ';') ? strchr(tp, ';') + 1 : NULL) {
        void mtl_link_pause(int on);
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(tb_at * 1e9)), dispatch_get_main_queue(), ^{




            LOG("前端：测试进后台（DM_TEST_BG）"); mtl_link_pause(1);
            dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 150000000), dispatch_get_main_queue(), ^{ front_ios_focus_event(0); front_ios_focus_event(1); }); });
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)((tb_at + tb_dur) * 1e9)), dispatch_get_main_queue(), ^{
            LOG("前端：测试回前台（DM_TEST_BG）"); mtl_link_pause(0); front_ios_focus_event(2); front_ios_focus_event(3);
            uint64_t c0 = dm_audio_render_calls();
            dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 2000000000), dispatch_get_main_queue(), ^{
                int dm_game_paused(void); LOG("前端：测试回前台后 2 s 声卡回调 +%llu（客体时钟 %.3f s；原版暂停 %d）", (unsigned long long)(dm_audio_render_calls() - c0), guest_ns() / 1e9, dm_game_paused()); }); });
    }


    { const char *tr = getenv("DM_TEST_RESIGN"); double tr_at = tr ? atof(tr) : 0;
      if (tr_at > 0) dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(tr_at * 1e9)), dispatch_get_main_queue(), ^{ LOG("前端：测试失活（DM_TEST_RESIGN）"); front_ios_focus_event(0); }); }
    const char *ai = getenv("DM_TEST_AUDIO_INTERRUPT"); double ai_at, ai_dur;
    if (ai && sscanf(ai, "%lf,%lf", &ai_at, &ai_dur) == 2) {
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(ai_at * 1e9)), dispatch_get_main_queue(), ^{
            LOG("前端：测试打断开始（DM_TEST_AUDIO_INTERRUPT）声卡回调累计 %llu", (unsigned long long)dm_audio_render_calls());
            [nc postNotificationName:AVAudioSessionInterruptionNotification object:as userInfo:@{AVAudioSessionInterruptionTypeKey: @(AVAudioSessionInterruptionTypeBegan)}];
        });
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)((ai_at + ai_dur) * 1e9)), dispatch_get_main_queue(), ^{
            uint64_t c0 = dm_audio_render_calls();
            LOG("前端：测试打断结束（DM_TEST_AUDIO_INTERRUPT）声卡回调累计 %llu", (unsigned long long)c0);
            [nc postNotificationName:AVAudioSessionInterruptionNotification object:as userInfo:@{AVAudioSessionInterruptionTypeKey: @(AVAudioSessionInterruptionTypeEnded),
                                                                                                AVAudioSessionInterruptionOptionKey: @(AVAudioSessionInterruptionOptionShouldResume)}];
            dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 2000000000), dispatch_get_main_queue(), ^{
                LOG("前端：测试打断恢复后 2 s 声卡回调 +%llu", (unsigned long long)(dm_audio_render_calls() - c0));
            });
        });
    }




    static const double probe_at[] = {1, 3, 10};
    for (int i = 0; i < 3; i++) dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(probe_at[i] * 1e9)), dispatch_get_main_queue(), ^{
        UIApplication *app = UIApplication.sharedApplication; NSMutableString *sc = [NSMutableString string];
        for (UIScene *x in app.connectedScenes) {
            UIWindow *kw = nil; if ([x isKindOfClass:UIWindowScene.class]) kw = ((UIWindowScene *)x).keyWindow;
            [sc appendFormat:@" 场景 activationState=%ld keyWindow=%d", (long)x.activationState, kw != nil];
        }
        LOG("前端：激活诊断 t=%.0fs applicationState=%ld（0 激活 / 1 未激活 / 2 后台）受保护数据=%d%s", probe_at[i], (long)app.applicationState,
            app.isProtectedDataAvailable, sc.UTF8String);
    });
}







#include <sys/resource.h>
void nvn_gpu_recent(double *avg, double *p95, unsigned *n);




void front_ios_heat_start(uint64_t (*presents)(void), int has_gpu) {
    const char *e = getenv("DM_HEAT");
    if (!e) {
        if (!has_gpu) return;
        UIDevice.currentDevice.batteryMonitoringEnabled = YES;
        __block uint64_t t0 = now_ns(), tl = t0, pl = presents(); __block double cl = 0;
        { struct rusage ru; getrusage(RUSAGE_SELF, &ru); cl = ru.ru_utime.tv_sec + ru.ru_utime.tv_usec / 1e6 + ru.ru_stime.tv_sec + ru.ru_stime.tv_usec / 1e6; }
        [NSTimer scheduledTimerWithTimeInterval:10 repeats:YES block:^(NSTimer *tm) { (void)tm;
            uint64_t t = now_ns(), p = presents(); struct rusage ru; getrusage(RUSAGE_SELF, &ru);
            double cpu = ru.ru_utime.tv_sec + ru.ru_utime.tv_usec / 1e6 + ru.ru_stime.tv_sec + ru.ru_stime.tv_usec / 1e6, dt = (t - tl) / 1e9;
            double ga = -1, gp = -1; unsigned gn = 0; if (has_gpu) nvn_gpu_recent(&ga, &gp, &gn);
            if (dt < 15) {
                float bl = UIDevice.currentDevice.batteryLevel; UIDeviceBatteryState bs = UIDevice.currentDevice.batteryState; char gpu[64] = "GPU 无样本", bat[32] = "电量 未知";
                if (gn) snprintf(gpu, sizeof gpu, "GPU %.2f / p95 %.2f ms（%u 帧）", ga, gp, gn);
                if (bl >= 0) snprintf(bat, sizeof bat, "电量 %.0f%%", bl * 100);
                LOG("[诊断] 第 %.0f 秒 CPU %.0f%% 帧率 %.1f %s 温控 %ld 亮度 %.2f %s %s 低电量模式 %s 取样 %.2f ms", (t - t0) / 1e9, (cpu - cl) / dt * 100, (p - pl) / dt, gpu,
                    (long)NSProcessInfo.processInfo.thermalState, UIScreen.mainScreen.brightness, bat,
                    bs == UIDeviceBatteryStateCharging ? "充电中" : bs == UIDeviceBatteryStateFull ? "已充满" : bs == UIDeviceBatteryStateUnplugged ? "未充电" : "充电状态未知",
                    NSProcessInfo.processInfo.lowPowerModeEnabled ? "开" : "关", (now_ns() - t) / 1e6);
            }
            tl = t; pl = p; cl = cpu; }];
        return;
    }
    char rel[512]; snprintf(rel, sizeof rel, "%.*s", (int)strcspn(e, ","), e); const char *c = strchr(e, ','); int iv = c ? atoi(c + 1) : 10;
    NSString *doc = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES)[0];
    FILE *f = fopen([doc stringByAppendingPathComponent:@(rel)].fileSystemRepresentation, "w"); if (!f) return;
    fprintf(f, "秒\t温控\tCPU%%\t出图帧率\tGPU均值ms\tGPU_p95ms\tGPU样本\t亮度\t最高刷新\n"); fflush(f);
    __block uint64_t t0 = now_ns(), tl = t0, pl = presents(); __block double cl = 0;
    { struct rusage ru; getrusage(RUSAGE_SELF, &ru); cl = ru.ru_utime.tv_sec + ru.ru_utime.tv_usec / 1e6 + ru.ru_stime.tv_sec + ru.ru_stime.tv_usec / 1e6; }
    [NSTimer scheduledTimerWithTimeInterval:iv repeats:YES block:^(NSTimer *tm) { (void)tm;
        uint64_t t = now_ns(), p = presents(); struct rusage ru; getrusage(RUSAGE_SELF, &ru);
        double cpu = ru.ru_utime.tv_sec + ru.ru_utime.tv_usec / 1e6 + ru.ru_stime.tv_sec + ru.ru_stime.tv_usec / 1e6, dt = (t - tl) / 1e9;
        double ga = -1, gp = -1; unsigned gn = 0; if (has_gpu) nvn_gpu_recent(&ga, &gp, &gn);
        fprintf(f, "%.0f\t%ld\t%.1f\t%.1f\t%.2f\t%.2f\t%u\t%.2f\t%ld\n", (t - t0) / 1e9, (long)NSProcessInfo.processInfo.thermalState, (cpu - cl) / dt * 100,
                (p - pl) / dt, ga, gp, gn, UIScreen.mainScreen.brightness, (long)UIScreen.mainScreen.maximumFramesPerSecond);
        fflush(f); tl = t; pl = p; cl = cpu; }];
}





static uint64_t g_blank_n;
uint64_t front_blank_presents(void) { return g_blank_n; }
@interface DmBlankView : UIView
@end
@implementation DmBlankView { id<MTLCommandQueue> _q; CADisplayLink *_dl; }
+ (Class)layerClass { return CAMetalLayer.class; }
- (instancetype)initWithFrame:(CGRect)fr {
    if ((self = [super initWithFrame:fr])) {
        CAMetalLayer *l = (CAMetalLayer *)self.layer; l.device = MTLCreateSystemDefaultDevice(); l.pixelFormat = MTLPixelFormatBGRA8Unorm; l.contentsScale = UIScreen.mainScreen.nativeScale;
        _q = [l.device newCommandQueue];
        _dl = [CADisplayLink displayLinkWithTarget:self selector:@selector(tick)]; _dl.preferredFrameRateRange = CAFrameRateRangeMake(60, 60, 60);
        [_dl addToRunLoop:NSRunLoop.mainRunLoop forMode:NSRunLoopCommonModes];
    }
    return self;
}
- (void)layoutSubviews { [super layoutSubviews]; CAMetalLayer *l = (CAMetalLayer *)self.layer; l.drawableSize = CGSizeMake(self.bounds.size.width * l.contentsScale, self.bounds.size.height * l.contentsScale); }
- (void)tick {
    @autoreleasepool {
        id<CAMetalDrawable> dr = [(CAMetalLayer *)self.layer nextDrawable]; if (!dr) return;
        MTLRenderPassDescriptor *rp = [MTLRenderPassDescriptor renderPassDescriptor];
        rp.colorAttachments[0].texture = dr.texture; rp.colorAttachments[0].loadAction = MTLLoadActionClear; rp.colorAttachments[0].storeAction = MTLStoreActionStore;
        rp.colorAttachments[0].clearColor = MTLClearColorMake(50 / 255.0, 50 / 255.0, 50 / 255.0, 1);
        id<MTLCommandBuffer> cb = [_q commandBuffer]; [[cb renderCommandEncoderWithDescriptor:rp] endEncoding]; [cb presentDrawable:dr]; [cb commit]; g_blank_n++;
    }
}
@end
UIView *front_ios_blank_view(CGRect fr) { return [[DmBlankView alloc] initWithFrame:fr]; }
