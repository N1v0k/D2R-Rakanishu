#include "dm_touch_controls.h"
#include "../../host/dm_input.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static int checks, cases;
#define CHECK(expr) do { ++checks; if (!(expr)) { fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #expr); exit(1); } } while (0)
#define NEAR(a,b,e) CHECK(fabs((double)(a)-(double)(b)) <= (e))
#define RUN(f) do { int before = checks; f(); ++cases; printf("PASS %s (%d checks)\n", #f, checks-before); } while (0)

static const uint64_t keys[DM_TC_COUNT] = {
    DM_BTN_A, DM_BTN_B, DM_BTN_X, DM_BTN_Y, DM_BTN_R, 0,
    DM_BTN_LEFT, DM_BTN_UP, DM_BTN_DOWN, DM_BTN_RIGHT, DM_BTN_PLUS, DM_BTN_MINUS
};
static void press(dm_tc_state *s, int slot, int target) {
    dm_tc_target t = s->layout.target[target];
    dm_tc_begin(s, slot, t.x, t.y);
    if (target >= DM_TC_MAIN && target <= DM_TC_SKILL4) dm_tc_tick(s, s->last_time_s + 0.1);
}
static int g_tall;
static void layout_at(float w, float h) {
    dm_tc_layout l; dm_tc_layout_make(&l, w, h);
    NEAR(l.width,w,0); NEAR(l.height,h,0);
    for (int i=0; i<DM_TC_COUNT; ++i) {
        dm_tc_target a=l.target[i];
        CHECK(a.r>0 && isfinite(a.r));
        CHECK(a.x-a.r>=0 && a.x+a.r<=w);
        CHECK(a.y-a.r>=0 && a.y+a.r<=h);
        CHECK(a.button==keys[i]);
        CHECK(dm_tc_hit(&l,a.x,a.y)==i);
        for (int j=i+1; j<DM_TC_COUNT; ++j) {
            dm_tc_target b=l.target[j];
            CHECK(hypotf(a.x-b.x,a.y-b.y)>a.r+b.r);
        }
    }
    float k = g_tall ? 1.15f : 1;
    NEAR(l.target[DM_TC_MAIN].r,40*k,0.001);
    for (int i=DM_TC_SKILL1; i<=DM_TC_SKILL4; ++i) {
        NEAR(l.target[i].r,26*k,0.001);
        CHECK(hypotf(l.target[i].x-l.target[0].x,l.target[i].y-l.target[0].y)>=96*k);
        CHECK(hypotf(l.target[i].x-l.target[0].x,l.target[i].y-l.target[0].y)<=100*k);
    }




    const int order[5] = {DM_TC_BELT1, DM_TC_BELT2, DM_TC_BELT3, DM_TC_BELT4, DM_TC_MENU};
    float u = h/1320;
    for (int c=0; c<5; ++c) {
        dm_tc_target t=l.target[order[c]];
        NEAR(t.r,57*u,0.001);
        NEAR(t.y,1241.5*u,0.001);
        NEAR(t.x,w/2+(1090+120*(c+1)-1434)*u,0.01);
    }

    { dm_tc_target p=l.target[DM_TC_PORTAL], s4=l.target[DM_TC_SKILL4];
      NEAR(p.r,26*k,0.001); NEAR(p.x,s4.x,0.001); NEAR(p.y,s4.y-s4.r-12*k-p.r,0.001); }
}

static void difficulty_picker(void) {
    dm_tc_state s; dm_tc_init(&s,956,440);
    dm_tc_layout *l=&s.layout; float pitch=440.0f*120/1320;
    NEAR(l->diff.x,l->target[DM_TC_BELT1].x-pitch,0.01); NEAR(l->diff.y,l->target[DM_TC_BELT1].y,0.01);
    NEAR(l->pick[7].x-l->pick[0].x,7*pitch,0.01); CHECK(l->pick[0].y+l->pick[0].r<l->diff.y-l->diff.r);
    NEAR((l->pick[3].x+l->pick[4].x)/2,l->width/2+(1390-1434)*440.0f/1320,0.01);
    dm_tc_begin(&s,0,l->diff.x,l->diff.y); CHECK(s.diff_open==1 && s.buttons==0); dm_tc_end(&s,0); CHECK(s.buttons==0);
    dm_tc_begin(&s,0,l->pick[7].x,l->pick[7].y); CHECK(s.diff_request==8 && s.diff_open==0 && s.buttons==0); dm_tc_end(&s,0);
    s.diff_request=0;
    dm_tc_begin(&s,0,l->diff.x,l->diff.y); dm_tc_end(&s,0); CHECK(s.diff_open==1);
    press(&s,0,DM_TC_BELT2); CHECK(s.diff_open==0 && s.diff_request==0 && s.buttons==0); dm_tc_end(&s,0);
    dm_tc_begin(&s,0,l->diff.x,l->diff.y); dm_tc_end(&s,0); dm_tc_set_navigation_mode(&s,1); CHECK(s.diff_open==0);
    dm_tc_begin(&s,0,l->diff.x,l->diff.y); CHECK(s.diff_open==0);
}
static void layout_667(void) { layout_at(667,375); }
static void layout_812(void) { layout_at(812,375); }
static void layout_956(void) { layout_at(956,440); }
static void layout_1024(void) { layout_at(1024,768); }
static void layout_ipad169(void) { dm_tc_set_tall_scale(g_tall = 1); layout_at(1366,768); dm_tc_set_tall_scale(g_tall = 0); }
static void layout_pixels(void) { layout_at(2868,1320); }
static void small_layout(void) {
    dm_tc_layout l; dm_tc_layout_make(&l,333.5f,187.5f);
    NEAR(l.target[0].r,20,0.001); NEAR(l.joy_radius,30,0.001);
    for(int i=0;i<DM_TC_COUNT;i++) {
        CHECK(l.target[i].x>=l.target[i].r && l.target[i].x+l.target[i].r<=l.width);
        CHECK(l.target[i].y>=l.target[i].r && l.target[i].y+l.target[i].r<=l.height);
        for(int j=i+1;j<DM_TC_COUNT;j++) CHECK(hypotf(l.target[i].x-l.target[j].x,l.target[i].y-l.target[j].y)>l.target[i].r+l.target[j].r);
    }
}
static void hit_edges(void) {
    dm_tc_layout l; dm_tc_layout_make(&l,667,375);
    for(int i=0;i<DM_TC_COUNT;i++) {
        dm_tc_target t=l.target[i];
        CHECK(dm_tc_hit(&l,t.x+t.r,t.y)==i);
        CHECK(dm_tc_hit(&l,t.x+t.r+0.01f,t.y)==DM_TC_NONE);
    }
    CHECK(dm_tc_hit(&l,-1,50)==DM_TC_NONE);
    CHECK(dm_tc_hit(&l,800,50)==DM_TC_NONE);
    CHECK(dm_tc_hit(&l,NAN,50)==DM_TC_NONE);
}
static void key_hold_and_ownership(void) {
    dm_tc_state s; dm_tc_init(&s,667,375);
    for(int i=0;i<DM_TC_COUNT;i++) {
        if(i>=DM_TC_BELT1&&i<=DM_TC_BELT4) continue;
        int release=0;
        press(&s,0,i);
        CHECK(s.buttons==(release?0:keys[i]));
        dm_tc_move(&s,0,350,100); CHECK(s.buttons==(release?0:keys[i]));
        dm_tc_move(&s,0,s.layout.target[0].x,s.layout.target[0].y); CHECK(s.buttons==(release?0:keys[i]));
        dm_tc_tick(&s,s.last_time_s+0.1); dm_tc_end(&s,0); CHECK(s.buttons==(release?keys[i]:0));
        if(release) { CHECK(dm_tc_pulse_end(&s,s.pulse_epoch)); CHECK(s.buttons==0); }
    }
}
static void two_fingers_same_key(void) {
    dm_tc_state s; dm_tc_init(&s,667,375);
    press(&s,0,DM_TC_MAIN); press(&s,1,DM_TC_MAIN);
    CHECK(s.buttons==DM_BTN_A); dm_tc_end(&s,0); CHECK(s.buttons==DM_BTN_A);
    dm_tc_end(&s,1); CHECK(s.buttons==0);
}
static void joystick_deadzone(void) {




    dm_tc_state s; dm_tc_init(&s,667,375); NEAR(s.layout.joy_x,120,0.001); NEAR(s.layout.joy_y,260,0.001);
    NEAR(s.layout.joy_zone_r,266.8,0.001); NEAR(s.layout.joy_zone_t,168.75,0.001); NEAR(s.layout.joy_min_x,68,0.001); NEAR(s.layout.joy_max_y,307,0.001);
    dm_tc_begin(&s,0,90,275);
    CHECK(s.joy_slot==0); NEAR(s.joy_origin_x,90,0); NEAR(s.joy_origin_y,275,0);
    CHECK(s.lx==0 && s.ly==0);
    dm_tc_move(&s,0,96,275); CHECK(s.lx==0 && s.ly==0);
    dm_tc_move(&s,0,96.1f,275); CHECK(s.lx>0 && s.lx<100 && s.ly==0);
    dm_tc_move(&s,0,123,275); CHECK(abs(s.lx-16384)<=1 && s.ly==0);
    dm_tc_end(&s,0);

    dm_tc_begin(&s,0,200,200); CHECK(s.joy_slot==0); NEAR(s.joy_origin_x,200,0); NEAR(s.joy_origin_y,200,0); CHECK(s.lx==0 && s.ly==0);
    dm_tc_move(&s,0,600,200); CHECK(s.lx==DM_STICK_MAX && s.ly==0); NEAR(s.joy_origin_x,200,0);
    dm_tc_move(&s,0,200,25); CHECK(s.lx==0 && s.ly==DM_STICK_MAX); NEAR(s.joy_origin_y,200,0);
    dm_tc_end(&s,0); CHECK(s.joy_slot==-1 && s.lx==0 && s.ly==0); NEAR(s.layout.joy_x,120,0.001);




    dm_tc_begin(&s,0,10,370); CHECK(s.joy_slot==0); NEAR(s.joy_origin_x,68,0); NEAR(s.joy_origin_y,307,0); CHECK(s.lx==0 && s.ly==0);
    dm_tc_move(&s,0,16,370); CHECK(s.lx==0 && s.ly==0);
    dm_tc_move(&s,0,17,370); CHECK(abs(s.lx-607)<=1 && s.ly==0);
    dm_tc_move(&s,0,70,370); CHECK(s.lx==DM_STICK_MAX && s.ly==0); NEAR(s.joy_origin_x,68,0); NEAR(s.joy_origin_y,307,0);
    dm_tc_end(&s,0);
    dm_tc_begin(&s,0,266,169); CHECK(s.joy_slot==0); NEAR(s.joy_origin_x,266,0); NEAR(s.joy_origin_y,169,0); dm_tc_end(&s,0);

    dm_tc_begin(&s,0,267,275); CHECK(s.joy_slot==-1 && s.lx==0); dm_tc_end(&s,0);
    dm_tc_begin(&s,0,100,168); CHECK(s.joy_slot==-1 && s.lx==0); dm_tc_end(&s,0);
    dm_tc_begin(&s,0,300,150); CHECK(s.joy_slot==-1 && s.lx==0 && s.ly==0); dm_tc_move(&s,0,200,250); CHECK(s.joy_slot==-1 && s.lx==0); dm_tc_end(&s,0);

    { unsigned q = s.world_tap_seq; dm_tc_begin_at(&s,0,150,250,40.0); dm_tc_end_at(&s,0,40.1); CHECK(s.world_tap_seq==q+1); NEAR(s.world_tap_x,150,0); NEAR(s.world_tap_y,250,0);
      dm_tc_begin_at(&s,0,150,250,41.0); dm_tc_move_at(&s,0,200,250,41.05); dm_tc_end_at(&s,0,41.1); CHECK(s.world_tap_seq==q+1); }

    dm_tc_begin_at(&s,0,90,275,50.0); dm_tc_move_at(&s,0,260,160,50.2); CHECK(s.lx>0 && s.ly>0); dm_tc_cancel_finger(&s,0); CHECK(s.joy_slot==-1 && s.lx==0);
    dm_tc_begin_at(&s,1,236,128,50.4); CHECK(s.joy_slot==1 && s.lx>0 && s.ly>0); NEAR(s.joy_origin_x,90,0); NEAR(s.joy_origin_y,275,0); dm_tc_cancel_finger(&s,1);
    dm_tc_begin_at(&s,2,236,128,50.4+0.36); CHECK(s.joy_slot==-1); dm_tc_end_at(&s,2,51.5);

    dm_tc_begin_at(&s,0,90,275,52.0); dm_tc_move_at(&s,0,260,160,52.2); dm_tc_cancel_finger(&s,0);
    dm_tc_begin_at(&s,1,265,300,52.3); CHECK(s.joy_slot==1); NEAR(s.joy_origin_x,265,0); NEAR(s.joy_origin_y,300,0); CHECK(s.lx==0 && s.ly==0); dm_tc_end_at(&s,1,53.5);
}








static void joystick_clamp_956(void) {
    static const float at[3][4] = {{6, 434, 68, 372}, {60, 400, 68, 372}, {150, 330, 150, 330}};
    dm_tc_state s; dm_tc_init(&s,956,440); NEAR(s.layout.joy_min_x,68,0.001); NEAR(s.layout.joy_max_y,372,0.001);
    for (int i = 0; i < 3; i++) { float x = at[i][0], y = at[i][1];
        dm_tc_begin(&s,0,x,y); NEAR(s.joy_origin_x,at[i][2],0); NEAR(s.joy_origin_y,at[i][3],0); CHECK(s.lx==0 && s.ly==0);
        dm_tc_move(&s,0,x+60,y); CHECK(s.lx==DM_STICK_MAX && s.ly==0);
        dm_tc_move(&s,0,x,y-60); CHECK(s.lx==0 && s.ly==DM_STICK_MAX);
        dm_tc_move(&s,0,x-42.4264f,y-42.4264f); CHECK(abs(s.lx+14654)<=2 && abs(s.ly-29308)<=2);
        NEAR(s.joy_origin_x,at[i][2],0); NEAR(s.joy_origin_y,at[i][3],0); dm_tc_end(&s,0); }
}
static void joystick_axes(void) {
    dm_tc_state s; dm_tc_init(&s,667,375); dm_tc_begin(&s,0,90,275);
    dm_tc_move(&s,0,150,275); CHECK(s.lx==DM_STICK_MAX && s.ly==0);
    dm_tc_move(&s,0,30,275); CHECK(s.lx==-DM_STICK_MAX && s.ly==0);
    dm_tc_move(&s,0,90,215); CHECK(s.lx==0 && s.ly==DM_STICK_MAX);
    dm_tc_move(&s,0,90,335); CHECK(s.lx==0 && s.ly==-DM_STICK_MAX);
    dm_tc_move(&s,0,690,275); CHECK(s.lx==DM_STICK_MAX && s.ly==0);
    dm_tc_move(&s,0,150,215); CHECK(abs(s.lx-14654)<=1 && abs(s.ly-29308)<=1);
    dm_tc_move(&s,0,90+60*0.8660254f,275-30); CHECK(abs(s.lx-21451)<=2 && abs(s.ly-24769)<=2);
    dm_tc_set_iso_fix(0); dm_tc_move(&s,0,150,215); CHECK(abs(s.lx-23170)<=1 && abs(s.ly-23170)<=1); dm_tc_set_iso_fix(1);
    dm_tc_end(&s,0); CHECK(s.joy_slot==-1 && s.lx==0 && s.ly==0);
}
static void joystick_plus_skills(void) {
    dm_tc_state s; dm_tc_init(&s,667,375); dm_tc_begin(&s,0,90,275); dm_tc_move(&s,0,150,275);
    for(int i=0;i<5;i++) press(&s,i+1,i);
    uint64_t all=keys[0]|keys[1]|keys[2]|keys[3]|keys[4];
    CHECK(s.buttons==all); CHECK(s.lx==DM_STICK_MAX);
    dm_tc_end(&s,3); CHECK(s.buttons==(all&~keys[2]) && s.lx==DM_STICK_MAX);
    dm_tc_end(&s,0); CHECK(s.lx==0 && s.ly==0); CHECK(s.buttons==(all&~keys[2]));
}
static void no_auto_handoff(void) {
    dm_tc_state s; dm_tc_init(&s,667,375); dm_tc_begin(&s,0,90,275); dm_tc_begin(&s,1,150,295);
    dm_tc_end(&s,0); dm_tc_move(&s,1,210,295);
    CHECK(s.joy_slot==-1 && s.lx==0 && s.ly==0 && s.buttons==0);
    dm_tc_begin(&s,2,90,275); CHECK(s.joy_slot==2);
}
static void background_no_key(void) {
    dm_tc_state s; dm_tc_init(&s,667,375); dm_tc_begin(&s,0,400,100);
    CHECK(s.buttons==0 && s.joy_slot==-1);
    dm_tc_move(&s,0,s.layout.target[0].x,s.layout.target[0].y); CHECK(s.buttons==0);
    dm_tc_begin(&s,1,10,10); dm_tc_move(&s,1,100,200); CHECK(s.joy_slot==-1 && s.buttons==0);
    dm_tc_begin(&s,2,667*0.43f,200); CHECK(s.joy_slot==-1);
    dm_tc_begin(&s,3,100,375*0.25f); CHECK(s.joy_slot==-1);
}
static void circles_before_joystick(void) {
    dm_tc_state s; dm_tc_init(&s,667,375); press(&s,0,DM_TC_PORTAL);
    CHECK(s.buttons==0 && s.joy_slot==-1); CHECK(s.finger[0].target==DM_TC_PORTAL);
}
static void cancel_and_resize(void) {
    dm_tc_state s; dm_tc_init(&s,667,375); dm_tc_begin(&s,0,100,200); dm_tc_move(&s,0,160,200); press(&s,1,DM_TC_MAIN);
    dm_tc_cancel_all(&s); CHECK(s.buttons==0 && s.lx==0 && s.ly==0 && s.joy_slot==-1);
    for(int i=0;i<DM_TC_MAX_TOUCH;i++) CHECK(!s.finger[i].active);
    dm_tc_begin(&s,0,100,200); press(&s,1,DM_TC_MAIN); dm_tc_resize(&s,812,375);
    CHECK(s.buttons==0 && s.lx==0 && s.ly==0 && s.joy_slot==-1); NEAR(s.layout.width,812,0);
    for(int i=0;i<DM_TC_MAX_TOUCH;i++) CHECK(!s.finger[i].active);
    dm_tc_move(&s,1,s.layout.target[0].x,s.layout.target[0].y); CHECK(s.buttons==0);
}
static void invalid_slots_and_repeated_begin(void) {
    dm_tc_state s; dm_tc_init(&s,667,375);
    press(&s,-1,0); press(&s,DM_TC_MAX_TOUCH,0); dm_tc_move(&s,-1,1,1); dm_tc_end(&s,DM_TC_MAX_TOUCH);
    CHECK(s.buttons==0); dm_tc_begin(&s,0,NAN,10); CHECK(!s.finger[0].active);
    press(&s,0,DM_TC_MAIN); press(&s,0,DM_TC_SKILL1); CHECK(s.buttons==DM_BTN_A);
    dm_tc_move(&s,0,NAN,INFINITY); CHECK(s.buttons==DM_BTN_A);
    dm_tc_end(&s,0); CHECK(s.buttons==0);
    dm_tc_begin(&s,0,-1,200); dm_tc_begin(&s,1,100,500); CHECK(s.joy_slot==-1);
}
static void all_slots_hold(void) {
    dm_tc_state s; dm_tc_init(&s,667,375);
    for(int i=0;i<DM_TC_MAX_TOUCH;i++) press(&s,i,DM_TC_MAIN);
    for(int i=0;i<DM_TC_MAX_TOUCH-1;i++) { dm_tc_end(&s,i); CHECK(s.buttons==DM_BTN_A); }
    dm_tc_end(&s,DM_TC_MAX_TOUCH-1); CHECK(s.buttons==0);
}
static void enabled_targets(void) {
    dm_tc_state s; dm_tc_init(&s,667,375);
    CHECK(s.enabled_targets == ((1u<<DM_TC_COUNT)-1));
    dm_tc_begin(&s,0,100,200); dm_tc_move(&s,0,160,200); press(&s,1,DM_TC_MAIN);
    uint32_t ui = (1u<<DM_TC_MAIN)|(1u<<DM_TC_SKILL1)|(1u<<DM_TC_SKILL2)|
        (1u<<DM_TC_SKILL3)|(1u<<DM_TC_SKILL4)|(1u<<DM_TC_MENU);
    dm_tc_enable_targets(&s,ui);
    CHECK(s.enabled_targets==ui); CHECK(s.buttons==0 && s.lx==0 && s.ly==0 && s.joy_slot==-1);
    for(int i=0;i<DM_TC_COUNT;i++) {
        press(&s,0,i);
        CHECK(s.buttons==((ui&(1u<<i))?keys[i]:0));
        CHECK(s.joy_slot==-1); dm_tc_end(&s,0); dm_tc_pulse_end(&s,s.pulse_epoch);
    }
    press(&s,0,DM_TC_MAIN); dm_tc_enable_targets(&s,ui); CHECK(s.buttons==DM_BTN_A);
    dm_tc_enable_targets(&s,0); CHECK(s.buttons==0);
    press(&s,0,DM_TC_SKILL1); CHECK(s.buttons==0 && s.joy_slot==-1);
    dm_tc_end(&s,0); dm_tc_enable_targets(&s,(1u<<DM_TC_COUNT)-1); press(&s,0,DM_TC_SKILL1); CHECK(s.buttons==DM_BTN_B);
}
static void ui_scene_messages(void) {
    dm_tc_scene scene={0};
    CHECK(dm_tc_navigation(&scene,"FrontEndNavigation","ToFrontEnd","")==1);
    CHECK(dm_tc_navigation(&scene,"PanelManager","OpenPanel","LoadScreenPanel")==1);
    CHECK(dm_tc_navigation(&scene,"PanelManager","UnloadPanel","LoadScreenPanel")==0);
    CHECK(dm_tc_navigation(&scene,"PanelManager","OpenPanel","PauseLayoutGarden")==1);
    CHECK(dm_tc_navigation(&scene,"PanelManager","ClosePanel","PauseLayoutGarden")==0);
    CHECK(dm_tc_navigation(&scene,"PanelManager","OpenPanel","AutoMap")==0);
    CHECK(dm_tc_navigation(&scene,"PanelManager","OpenExclusivePanel","BankExpansionLayout")==1);
    CHECK(dm_tc_navigation(&scene,"PanelManager","OpenPanel","PlayerInventoryExpansionLayout")==1);
    CHECK(dm_tc_navigation(&scene,"PanelManager","ClosePanel","BankExpansionLayout")==1);
    CHECK(dm_tc_navigation(&scene,"PanelManager","ClosePanel","PlayerInventoryExpansionLayout")==0);
    CHECK(dm_tc_navigation(&scene,"PanelManager","OpenPanel","HUDPanel")==0);
    CHECK(dm_tc_navigation(&scene,"PanelManager","OpenPanel","SkillsTreePanel")==1);
    CHECK(dm_tc_navigation(&scene,"PanelManager","UnloadAll","")==0);
    CHECK(dm_tc_navigation(&scene,"FrontEndNavigation","ToFrontEnd","")==1);
}
static void pointer_tap(void) {
    dm_tc_state s; dm_tc_init(&s,667,375); dm_tc_set_navigation_mode(&s,1);
    dm_tc_begin_at(&s,0,300,180,10.0); dm_tc_end_at(&s,0,10.2);
    CHECK(s.pointer_valid); NEAR(s.pointer_x,300,0.001); NEAR(s.pointer_y,180,0.001);
    CHECK(s.pulse_button==DM_BTN_A); CHECK(s.buttons==DM_BTN_A);
}


static void pointer_stack_tap(void) {
    dm_tc_scene scene={0}; dm_tc_navigation(&scene,"PanelManager","UnloadAll",""); dm_tc_navigation(&scene,"PanelManager","OpenPanel","BankExpansionLayout"); dm_tc_set_panel_state(1);
    dm_tc_state s; dm_tc_init(&s,667,375); dm_tc_set_navigation_mode(&s,1); s.mouse_click_mode=1;
    dm_tc_begin_at(&s,0,104,91,10.0); dm_tc_pointer_item(&s,8); dm_tc_end_at(&s,0,10.1);
    CHECK(s.pointer_valid); NEAR(s.pointer_x,104,0.001); CHECK(s.stack_take_seq==0); CHECK(s.pulse_button==0);
    dm_tc_begin_at(&s,0,104,91,10.3); dm_tc_pointer_item(&s,8); dm_tc_end_at(&s,0,10.4);
    CHECK(s.stack_take_seq==1); CHECK(s.pulse_button==0); CHECK(s.use_seq==0); CHECK(s.buttons==0);
    dm_tc_begin_at(&s,0,104,91,12.0); dm_tc_pointer_item(&s,8); dm_tc_tick(&s,12.9); dm_tc_end_at(&s,0,12.9);
    CHECK(s.long_press_seq==1); CHECK(s.stack_take_seq==1);
    dm_tc_begin_at(&s,0,104,91,14.0); dm_tc_pointer_item(&s,8); dm_tc_move_at(&s,0,200,150,14.1); CHECK(s.finger[0].shop_drag); CHECK(s.pulse_button==0); CHECK(s.buttons==0);
    dm_tc_end_at(&s,0,14.5); CHECK(s.pulse_button==0); CHECK(s.stack_take_seq==1);
    dm_tc_navigation(&scene,"PanelManager","UnloadAll","");
}
static void pointer_item_tap(void) {
    dm_tc_scene scene={0}; dm_tc_navigation(&scene,"PanelManager","UnloadAll","");
    dm_tc_state s; dm_tc_init(&s,667,375); dm_tc_set_navigation_mode(&s,1);
    dm_tc_begin_at(&s,0,300,180,10.0); dm_tc_pointer_item(&s,1); dm_tc_end_at(&s,0,10.1);
    CHECK(s.pointer_valid); NEAR(s.pointer_x,300,0.001); NEAR(s.pointer_y,180,0.001); CHECK(s.pulse_button==0); CHECK(s.buttons==0);
    dm_tc_begin_at(&s,0,302,181,10.3); dm_tc_pointer_item(&s,1); dm_tc_end_at(&s,0,10.4);
    CHECK(s.pulse_button==0); CHECK(s.use_seq==1);
    dm_tc_begin_at(&s,0,302,181,10.6); dm_tc_pointer_item(&s,1); dm_tc_end_at(&s,0,10.7); CHECK(s.pulse_button==0);
    dm_tc_begin_at(&s,0,302,181,12.0); dm_tc_pointer_item(&s,1); dm_tc_end_at(&s,0,12.1); CHECK(s.pulse_button==DM_BTN_ZL); CHECK(dm_tc_pulse_end(&s,s.pulse_epoch));
    dm_tc_begin_at(&s,0,400,181,12.2); dm_tc_pointer_item(&s,1); dm_tc_end_at(&s,0,12.3); CHECK(s.pulse_button==0);
    dm_tc_navigation(&scene,"PanelManager","OpenPanel","BankExpansionLayout"); dm_tc_set_panel_state(1);
    dm_tc_begin_at(&s,0,400,181,12.5); dm_tc_pointer_item(&s,1); dm_tc_end_at(&s,0,12.6);
    CHECK(s.pulse_button==DM_BTN_Y); NEAR(s.pulse_hold_s,0.6,0.001); dm_tc_pulse_end(&s,s.pulse_epoch);
    dm_tc_navigation(&scene,"PanelManager","ClosePanel","BankExpansionLayout"); dm_tc_set_panel_state(0);
    dm_tc_begin_at(&s,0,100,100,13.5); dm_tc_end_at(&s,0,13.6); CHECK(s.pulse_button==DM_BTN_A); NEAR(s.pulse_hold_s,0,0.001);
    dm_tc_pulse_end(&s,s.pulse_epoch); s.mouse_click_mode=1;
    dm_tc_begin_at(&s,0,120,90,14.5); dm_tc_end_at(&s,0,14.6); CHECK(s.pulse_button==0); CHECK(s.mouse_click_seq==1); NEAR(s.mouse_click_x,120,0.001); NEAR(s.mouse_click_y,90,0.001);
    dm_tc_begin_at(&s,0,300,180,15.5); dm_tc_pointer_item(&s,1); dm_tc_end_at(&s,0,15.6); CHECK(s.mouse_click_seq==1); CHECK(s.pulse_button==0);
}




static void ui_drag_typing(void) {
    dm_tc_state s; dm_tc_init(&s,667,375); dm_tc_set_navigation_mode(&s,1); s.mouse_click_mode=1;
    dm_tc_begin_at(&s,0,300,200,1.0); dm_tc_move_at(&s,0,302,209,1.1); CHECK(s.finger[0].ui_drag==0);
    dm_tc_move_at(&s,0,303,212,1.2); CHECK(s.finger[0].ui_drag==1);
    dm_tc_move_at(&s,0,400,214,1.3); CHECK(s.finger[0].ui_drag==1); CHECK(s.buttons==0); CHECK(s.pulse_button==0);
    dm_tc_move_at(&s,0,300,200,1.4); dm_tc_end_at(&s,0,1.45); CHECK(s.mouse_click_seq==0); CHECK(s.pulse_button==0);
    dm_tc_begin_at(&s,0,300,200,2.0); dm_tc_move_at(&s,0,312,204,2.1); CHECK(s.finger[0].ui_drag==2);
    dm_tc_end_at(&s,0,2.2); CHECK(s.mouse_click_seq==0); CHECK(s.pulse_button==0);
    dm_tc_begin_at(&s,0,300,200,3.0); dm_tc_pointer_item(&s,1); dm_tc_move_at(&s,0,300,230,3.1); CHECK(s.finger[0].ui_drag==0); CHECK(s.finger[0].pointer_dragged);
    dm_tc_pointer_release(&s,1); dm_tc_end_at(&s,0,3.2); if (s.pulse_button) dm_tc_pulse_end(&s,s.pulse_epoch);
    dm_tc_begin_at(&s,0,300,200,4.0); dm_tc_pointer_item(&s,3); dm_tc_move_at(&s,0,300,240,4.1); CHECK(s.finger[0].ui_drag==0);
    dm_tc_end_at(&s,0,4.2); if (s.pulse_button) dm_tc_pulse_end(&s,s.pulse_epoch);
    s.mouse_click_mode=0;
    dm_tc_begin_at(&s,0,300,200,5.0); dm_tc_move_at(&s,0,300,240,5.1); CHECK(s.finger[0].ui_drag==0);
    dm_tc_end_at(&s,0,5.2);
}


static void pointer_over_disabled_target(void) {
    dm_tc_state s; dm_tc_init(&s,667,375); s.mouse_click_mode=1; dm_tc_set_navigation_mode(&s,1); dm_tc_enable_targets(&s,dm_tc_nav_mask(1));
    const dm_tc_target *b=&s.layout.target[DM_TC_BELT1];
    dm_tc_begin_at(&s,0,b->x,b->y,10.0); CHECK(s.pointer_slot==0); dm_tc_end_at(&s,0,10.1); CHECK(s.mouse_click_seq==1); CHECK(s.buttons==0);
}

static void world_tap(void) {
    dm_tc_state s; dm_tc_init(&s,667,375); dm_tc_enable_targets(&s,(1u<<DM_TC_COUNT)-1);
    dm_tc_begin_at(&s,0,400,120,10.0); dm_tc_end_at(&s,0,10.1); CHECK(s.world_tap_seq==1); NEAR(s.world_tap_x,400,0.001); NEAR(s.world_tap_y,120,0.001);
    dm_tc_begin_at(&s,0,250,150,11.0); CHECK(s.joy_slot==-1); dm_tc_end_at(&s,0,11.1); CHECK(s.world_tap_seq==2); NEAR(s.world_tap_x,250,0.001);
    dm_tc_begin_at(&s,0,120,250,11.5); CHECK(s.joy_slot==0); dm_tc_end_at(&s,0,11.6); CHECK(s.world_tap_seq==3); NEAR(s.world_tap_x,120,0.001); NEAR(s.world_tap_y,250,0.001);
    dm_tc_begin_at(&s,0,120,250,12.0); dm_tc_move_at(&s,0,170,250,12.1); dm_tc_end_at(&s,0,12.2); CHECK(s.world_tap_seq==3);
    dm_tc_begin_at(&s,0,400,120,13.0); dm_tc_end_at(&s,0,13.6); CHECK(s.world_tap_seq==3);
    const dm_tc_target *m=&s.layout.target[DM_TC_MAIN];
    dm_tc_begin_at(&s,0,m->x,m->y,14.0); CHECK(s.buttons==m->button); dm_tc_end_at(&s,0,14.1); CHECK(s.world_tap_seq==3);
    dm_tc_set_joy_block(&s,1); dm_tc_begin_at(&s,0,120,250,20.5); CHECK(s.joy_slot==-1 && s.lx==0 && s.ly==0); dm_tc_end_at(&s,0,20.6); CHECK(s.world_tap_seq==4); NEAR(s.world_tap_x,120,0.001);
    dm_tc_set_joy_block(&s,0); dm_tc_begin_at(&s,0,120,250,21.5); CHECK(s.joy_slot==0); dm_tc_end_at(&s,0,21.6); CHECK(s.world_tap_seq==5);
    dm_tc_set_navigation_mode(&s,1); dm_tc_begin_at(&s,0,400,120,16.0); dm_tc_end_at(&s,0,16.1); CHECK(s.world_tap_seq==5);
}

static void pointer_outside_close(void) {
    dm_tc_state s; dm_tc_init(&s,667,375); s.mouse_click_mode=1; dm_tc_set_navigation_mode(&s,1);
    dm_tc_begin_at(&s,0,330,60,10.0); dm_tc_pointer_item(&s,3); dm_tc_end_at(&s,0,10.1); CHECK(s.pulse_button==0);
    dm_tc_tick(&s,10.12); CHECK(s.pulse_button==DM_BTN_B); CHECK(dm_tc_pulse_end(&s,s.pulse_epoch));
    dm_tc_tick(&s,10.2); CHECK(s.pulse_button==0);
    dm_tc_tick(&s,10.45); CHECK(s.pulse_button==DM_BTN_B); CHECK(dm_tc_pulse_end(&s,s.pulse_epoch));
    dm_tc_set_navigation_mode(&s,0); dm_tc_tick(&s,10.8); CHECK(s.pulse_button==0); CHECK(s.close_left==0);                                                    dm_tc_set_navigation_mode(&s,1);
    dm_tc_begin_at(&s,0,330,60,20.0); dm_tc_pointer_item(&s,3); dm_tc_end_at(&s,0,20.1); dm_tc_tick(&s,20.12); CHECK(s.pulse_button==DM_BTN_B); CHECK(dm_tc_pulse_end(&s,s.pulse_epoch));
    { dm_tc_scene sc={0}; dm_tc_navigation(&sc,"PanelManager","OpenPanel","ConfirmationModal"); }
    dm_tc_tick(&s,20.5); CHECK(s.pulse_button==0); CHECK(s.close_left==0);
}
static void scene_dialog_only(void) {
    dm_tc_scene sc={0};
    dm_tc_navigation(&sc,"PanelManager","UnloadPanel","LoadScreenPanel"); CHECK(!dm_tc_scene_dialog_only(&sc));
    CHECK(dm_tc_navigation(&sc,"PanelManager","OpenPanel","NpcDialogPanel")); CHECK(dm_tc_scene_dialog_only(&sc));
    dm_tc_navigation(&sc,"PanelManager","OpenPanel","NpcTradePanel"); CHECK(!dm_tc_scene_dialog_only(&sc));
    dm_tc_navigation(&sc,"PanelManager","ClosePanel","NpcTradePanel"); dm_tc_navigation(&sc,"PanelManager","ClosePanel","NpcDialogPanel"); CHECK(!dm_tc_scene_dialog_only(&sc));
}
static void pointer_drag_out(void) {
    dm_tc_state s; dm_tc_init(&s,667,375); dm_tc_set_navigation_mode(&s,1);
    dm_tc_begin_at(&s,0,300,180,10.0); dm_tc_move_at(&s,0,320,180,10.1); CHECK(s.pulse_button==DM_BTN_A); CHECK(dm_tc_pulse_end(&s,s.pulse_epoch));
    dm_tc_move_at(&s,0,20,300,10.5); dm_tc_pointer_release(&s,3); dm_tc_end_at(&s,0,10.6);
    CHECK(s.pulse_button==DM_BTN_Y); NEAR(s.pulse_hold_s,0.15,0.001); NEAR(s.pointer_x,300,0.001); NEAR(s.pointer_y,180,0.001);
    CHECK(dm_tc_pulse_end(&s,s.pulse_epoch));
    dm_tc_begin_at(&s,0,300,180,11.0); dm_tc_move_at(&s,0,320,180,11.1); CHECK(dm_tc_pulse_end(&s,s.pulse_epoch));
    dm_tc_move_at(&s,0,340,200,11.5); dm_tc_end_at(&s,0,11.6); CHECK(s.pulse_button==DM_BTN_A);
    CHECK(dm_tc_pulse_end(&s,s.pulse_epoch));
    dm_tc_begin_at(&s,0,300,180,12.0); dm_tc_move_at(&s,0,320,180,12.1); CHECK(dm_tc_pulse_end(&s,s.pulse_epoch));
    dm_tc_move_at(&s,0,400,120,12.5); dm_tc_pointer_release(&s,4); dm_tc_end_at(&s,0,12.6);
    CHECK(s.pulse_button==DM_BTN_A); NEAR(s.pointer_x,400,0.001); NEAR(s.pointer_y,120,0.001); CHECK(dm_tc_pulse_end(&s,s.pulse_epoch));
    dm_tc_begin_at(&s,0,300,180,13.0); dm_tc_move_at(&s,0,320,180,13.1); CHECK(dm_tc_pulse_end(&s,s.pulse_epoch));
    dm_tc_move_at(&s,0,400,120,13.5); dm_tc_pointer_release(&s,0); dm_tc_end_at(&s,0,13.6);
    CHECK(s.pulse_button==DM_BTN_A); NEAR(s.pointer_x,300,0.001); NEAR(s.pointer_y,180,0.001);
}
static void pointer_drag(void) {
    dm_tc_state s; dm_tc_init(&s,667,375); dm_tc_set_navigation_mode(&s,1);
    dm_tc_begin_at(&s,0,300,180,10.0); dm_tc_move_at(&s,0,320,180,10.1);
    CHECK(s.pointer_valid); NEAR(s.pointer_x,300,0.001); NEAR(s.pointer_y,180,0.001);
    CHECK(s.pulse_button==DM_BTN_A); uint64_t first=s.pulse_epoch;
    CHECK(dm_tc_pulse_end(&s,first));
    dm_tc_move_at(&s,0,340,200,10.2);
    NEAR(s.pointer_x,340,0.001); NEAR(s.pointer_y,160,0.001);
    dm_tc_end_at(&s,0,10.3); NEAR(s.pointer_x,340,0.001); NEAR(s.pointer_y,160,0.001);
    CHECK(s.pulse_button==DM_BTN_A); CHECK(s.pulse_epoch!=first);
}
static void pointer_long(void) {
    dm_tc_state s; dm_tc_init(&s,667,375); dm_tc_set_navigation_mode(&s,1);
    dm_tc_begin_at(&s,0,300,180,10.0); dm_tc_tick(&s,10.5);
    CHECK(s.pointer_valid); NEAR(s.pointer_x,300,0.001); NEAR(s.pointer_y,180,0.001);
    CHECK(s.pulse_button==0); dm_tc_end_at(&s,0,10.6); CHECK(s.pulse_button==0);
}
static void pointer_shop_drag(void) {
    dm_tc_state s; dm_tc_init(&s,667,375); s.mouse_click_mode=1; dm_tc_set_navigation_mode(&s,1);
    dm_tc_begin_at(&s,0,150,200,10.0); dm_tc_pointer_item(&s,7); CHECK(s.pointer_item==1); CHECK(s.pointer_shop==1);
    dm_tc_move_at(&s,0,300,220,10.3); CHECK(s.pulse_button==0); CHECK(s.finger[0].shop_drag); NEAR(s.pointer_x,2,0.001); NEAR(s.pointer_y,2,0.001);
    dm_tc_move_at(&s,0,480,280,10.8); NEAR(s.pointer_x,2,0.001); NEAR(s.finger[0].aim_origin_x,150,0.001);
    dm_tc_end_at(&s,0,11.0); CHECK(s.pulse_button==0); CHECK(s.shop_drop_seq==1); CHECK(s.pointer_shop==0);
    dm_tc_begin_at(&s,0,150,200,12.0); dm_tc_pointer_item(&s,7); dm_tc_end_at(&s,0,12.1); CHECK(s.shop_drop_seq==1);
}
static void pointer_long_seq(void) {
    dm_tc_state s; dm_tc_init(&s,667,375); s.mouse_click_mode=1; dm_tc_set_navigation_mode(&s,1);
    dm_tc_begin_at(&s,0,300,180,10.0); dm_tc_pointer_item(&s,1); dm_tc_tick(&s,10.6); CHECK(s.long_press_seq==0);
    dm_tc_end_at(&s,0,11.6); CHECK(s.long_press_seq==1); CHECK(s.pulse_button==0);
    dm_tc_begin_at(&s,0,300,180,12.0); dm_tc_pointer_item(&s,1); dm_tc_tick(&s,12.6); dm_tc_move_at(&s,0,360,220,12.8); dm_tc_end_at(&s,0,13.2); CHECK(s.long_press_seq==1);
    dm_tc_pulse_end(&s,s.pulse_epoch);
    dm_tc_begin_at(&s,0,300,180,14.0); dm_tc_pointer_item(&s,0); dm_tc_tick(&s,15.0); dm_tc_end_at(&s,0,15.1); CHECK(s.long_press_seq==1);
}
static void pointer_long_then_drag(void) {
    dm_tc_state s; dm_tc_init(&s,667,375); dm_tc_set_navigation_mode(&s,1);
    dm_tc_begin_at(&s,0,300,180,10.0); dm_tc_tick(&s,10.5); CHECK(s.pulse_button==0);
    dm_tc_move_at(&s,0,340,200,10.6); CHECK(s.pulse_button==DM_BTN_A); CHECK(dm_tc_pulse_end(&s,s.pulse_epoch));
    dm_tc_move_at(&s,0,400,240,11.0); dm_tc_end_at(&s,0,11.1); CHECK(s.pulse_button==DM_BTN_A);
}
static void pointer_control_hit(void) {
    dm_tc_state s; dm_tc_init(&s,667,375); dm_tc_set_navigation_mode(&s,1);
    press(&s,0,DM_TC_MAIN); CHECK(!s.pointer_valid); CHECK(s.buttons==DM_BTN_A);
}
static void pointer_game_mode(void) {
    dm_tc_state s; dm_tc_init(&s,667,375);
    dm_tc_begin_at(&s,0,300,180,10.0); dm_tc_move_at(&s,0,340,200,10.1);
    CHECK(!s.pointer_valid); CHECK(s.joy_slot==-1); dm_tc_end_at(&s,0,10.2); CHECK(!s.pointer_valid);
}
static void pointer_pulse_queue(void) {
    dm_tc_state s; dm_tc_init(&s,667,375); dm_tc_set_navigation_mode(&s,1);
    dm_tc_begin_at(&s,0,300,180,10.0); dm_tc_move_at(&s,0,320,180,10.1);


    dm_tc_end_at(&s,0,10.2); CHECK(s.pulse_button==DM_BTN_A); NEAR(s.pointer_x,300,0.001); NEAR(s.pointer_y,180,0.001); CHECK(!dm_tc_pulse_pending(&s));
    uint64_t first=s.pulse_epoch; CHECK(dm_tc_pulse_end(&s,first));


    CHECK(s.pulse_button==0); CHECK(s.buttons==0); CHECK(dm_tc_pulse_pending(&s)); NEAR(s.pointer_y,180,0.001);
    CHECK(dm_tc_pulse_resume(&s)); CHECK(s.pulse_button==DM_BTN_A); CHECK(s.pulse_epoch!=first); NEAR(s.pointer_x,320,0.001); NEAR(s.pointer_y,140,0.001);
    CHECK(!dm_tc_pulse_resume(&s)); CHECK(dm_tc_pulse_end(&s,s.pulse_epoch)); CHECK(!dm_tc_pulse_pending(&s));
}


static void safe_insets(void) {
    dm_tc_layout a,b;dm_tc_layout_make(&a,956,440);
    dm_tc_set_safe_insets(62,62);dm_tc_layout_make(&b,956,440);dm_tc_set_safe_insets(0,0);
    for(int i=DM_TC_MAIN;i<=DM_TC_PORTAL;i++) {
        NEAR(a.target[i].x-b.target[i].x,62,.001);NEAR(a.target[i].y,b.target[i].y,.001);
        CHECK(b.target[i].x+b.target[i].r<=956-62-8+.001f);
    }
    NEAR(a.swap.x-b.swap.x,62,.001);NEAR(a.swap.y,b.swap.y,.001);
    NEAR(a.joy_x,120,.001);NEAR(b.joy_x,152,.001);NEAR(a.joy_y,325,.001);
    dm_tc_layout c;dm_tc_layout_make(&c,956,440);NEAR(c.target[DM_TC_MAIN].x,a.target[DM_TC_MAIN].x,.001);
}
static void swap_key(void) {
    dm_tc_layout l;dm_tc_layout_make(&l,956,440);
    NEAR(l.swap.r,22,.001);CHECK(l.swap.x<l.target[DM_TC_PORTAL].x);
    float travel=hypotf(l.swap.x-l.target[DM_TC_MAIN].x,l.swap.y-l.target[DM_TC_MAIN].y);
    NEAR(travel,hypotf(l.target[DM_TC_PORTAL].x-l.target[DM_TC_MAIN].x,l.target[DM_TC_PORTAL].y-l.target[DM_TC_MAIN].y),.001);
    CHECK(dm_tc_hit(&l,l.swap.x,l.swap.y)==DM_TC_SWAP);


    dm_tc_target main=l.target[DM_TC_MAIN];
    float d=main.r+12;
    CHECK(dm_tc_hit(&l,main.x+(l.swap.x-main.x)*d/travel,main.y+(l.swap.y-main.y)*d/travel)==DM_TC_NONE);
    dm_tc_state s;dm_tc_init(&s,956,440);press(&s,0,DM_TC_MAIN);
    dm_tc_move(&s,0,l.swap.x,l.swap.y);CHECK(s.skill_set==0);CHECK(s.finger[0].target==DM_TC_MAIN);
    dm_tc_end(&s,0);CHECK(s.skill_set==0);
    dm_tc_begin(&s,0,l.swap.x,l.swap.y);CHECK(s.skill_set==0);dm_tc_end(&s,0);CHECK(s.skill_set==1);
    dm_tc_begin(&s,0,l.swap.x,l.swap.y);dm_tc_move(&s,0,l.swap.x+l.swap.r+1,l.swap.y);
    dm_tc_move(&s,0,l.swap.x,l.swap.y);dm_tc_end(&s,0);CHECK(s.skill_set==1);
    dm_tc_begin(&s,0,l.swap.x,l.swap.y);dm_tc_cancel_finger(&s,0);CHECK(s.skill_set==1);
    dm_tc_begin(&s,0,l.swap.x,l.swap.y);dm_tc_resize(&s,812,375);CHECK(s.skill_set==1);
    dm_tc_end(&s,0);CHECK(s.skill_set==1);
}



static void skill_cluster_clearance(void) {
    const float sizes[][6]={{333.5f,187.5f,0,0,0,0},{667,375,0,0,0,0},{812,375,44,44,21,0},
        {874,402,62,62,21,0},{956,440,62,62,34,0},{1024,768,0,0,20,1},{1194,834,0,0,24,1},
        {1366,1024,0,0,24,1},{1366,768,32,32,24,1},{744,450,0,0,20,1},{507,285,0,0,20,1},{600,337,0,0,24,1},{874,402,62,0,21,0}};
    for(int above=0;above<2;above++) for(int n=0;n<(int)(sizeof(sizes)/sizeof(sizes[0]));n++) {
        float w=sizes[n][0],h=sizes[n][1],bottom=sizes[n][4];dm_tc_set_tall_scale((int)sizes[n][5]);
        dm_tc_set_swap_above(above);dm_tc_set_safe_bottom(bottom);
        dm_tc_set_safe_insets(sizes[n][2],sizes[n][3]);dm_tc_layout l;dm_tc_layout_make(&l,w,h);
        float k=dm_tc_scale_for_size(w,h);dm_tc_target group[7];
        CHECK(k<=1.15f);CHECK(l.tablet==(int)sizes[n][5]);CHECK(l.swap_above==above);
        if (!above) {
            CHECK(l.swap.x<l.target[DM_TC_PORTAL].x);
            dm_tc_target main=l.target[0],portal=l.target[DM_TC_PORTAL];
            NEAR(hypotf(l.swap.x-main.x,l.swap.y-main.y),hypotf(portal.x-main.x,portal.y-main.y),.001);
        }
        for(int i=0;i<6;i++)group[i]=l.target[i];group[6]=l.swap;
        for(int i=0;i<7;i++) {
            dm_tc_target t=group[i];CHECK(t.x-t.r>=sizes[n][2]);CHECK(t.x+t.r<=w-sizes[n][3]);
            CHECK(t.y-t.r>=0);CHECK(t.y+t.r<=h-fmaxf(24*k,bottom+8*k)+.001f);
            if(sizes[n][5] || (w>=667 && h>=375)) CHECK(t.r*2>=44);
            for(int j=i+1;j<7;j++)CHECK(hypotf(t.x-group[j].x,t.y-group[j].y)>=t.r+group[j].r+12*k-.001f);
            for(int j=DM_TC_BELT1;j<=DM_TC_MENU;j++) {
                dm_tc_target bar=l.target[j];CHECK(hypotf(t.x-bar.x,t.y-bar.y)>t.r+bar.r+8*k);
            }
            dm_tc_target map=l.target[DM_TC_MAP];
            CHECK(hypotf(t.x-map.x,t.y-map.y)>=t.r+map.r+12*k-.001f);
            CHECK(dm_tc_hit(&l,t.x,t.y)==(i==6?DM_TC_SWAP:i));
        }
    }
    dm_tc_set_tall_scale(1);dm_tc_set_safe_insets(0,0);dm_tc_set_safe_bottom(24);dm_tc_set_swap_above(0);
    dm_tc_state s;dm_tc_init(&s,1194,672);dm_tc_layout before=s.layout;
    press(&s,0,DM_TC_MAIN);CHECK(s.buttons!=0);dm_tc_set_swap_above(1);dm_tc_resize(&s,1194,834);
    CHECK(s.buttons==0 && s.joy_slot==DM_TC_NONE);CHECK(s.layout.swap_above==1);


    NEAR(s.layout.target[0].r,before.target[0].r,.001);
    NEAR(s.layout.height-s.layout.target[0].y,before.height-before.target[0].y,.001);
    for(int i=0;i<5;i++)NEAR(s.layout.target[i].x,before.target[i].x,.001);
    dm_tc_set_tall_scale(0);dm_tc_set_safe_insets(0,0);dm_tc_set_safe_bottom(0);dm_tc_set_swap_above(0);
}






static void belt_expand(void) {
    dm_tc_state s; dm_tc_init(&s, 956, 440); float bx = s.layout.target[DM_TC_BELT1].x, by = s.layout.target[DM_TC_BELT1].y;
    NEAR(bx, 403.333f, 0.01); NEAR(by, 413.833f, 0.01);
    dm_tc_begin_at(&s, 0, bx, by, 1.0); CHECK(s.buttons == 0); dm_tc_end_at(&s, 0, 1.05); CHECK(!s.belt_request); dm_tc_tick(&s, 1.05 + 0.01); CHECK(s.pulse_button == DM_BTN_LEFT); dm_tc_pulse_end(&s, s.pulse_epoch);
    s.belt_rows = 4; s.pulse_button = 0;
    dm_tc_begin_at(&s, 0, bx, by, 2.0); CHECK(s.buttons == 0); dm_tc_end_at(&s, 0, 2.05); dm_tc_tick(&s, 2.05 + 0.01); CHECK(s.pulse_button == DM_BTN_LEFT); CHECK(!s.belt_request); s.pulse_button = 0;
    dm_tc_begin_at(&s, 0, bx, by, 3.0); dm_tc_tick(&s, 3.4); CHECK(s.buttons == 0); dm_tc_end_at(&s, 0, 3.5); dm_tc_tick(&s, 3.5 + 0.01); CHECK(s.pulse_button == DM_BTN_LEFT); s.pulse_button = 0;
    dm_tc_begin_at(&s, 0, bx, by, 4.0); dm_tc_move_at(&s, 0, bx, by - 30, 4.05); CHECK(s.belt_request == 0); CHECK(s.buttons == 0); dm_tc_end_at(&s, 0, 4.1); CHECK(s.belt_request == 1); CHECK(s.pulse_button == 0); s.belt_request = 0;
    dm_tc_begin_at(&s, 0, bx, by, 5.0); dm_tc_move_at(&s, 0, bx, by + 25, 5.05); dm_tc_end_at(&s, 0, 5.1); CHECK(s.belt_request == 2); CHECK(s.pulse_button == 0); s.belt_request = 0;
    dm_tc_begin_at(&s, 0, bx, by, 5.2); dm_tc_tick(&s, 5.4); dm_tc_move_at(&s, 0, bx, by - 30, 5.5); dm_tc_end_at(&s, 0, 5.55); CHECK(s.belt_request == 1); CHECK(s.pulse_button == 0); CHECK(s.buttons == 0); s.belt_request = 0;
    dm_tc_begin_at(&s, 0, bx, by, 5.6); dm_tc_tick(&s, 5.8); dm_tc_move_at(&s, 0, bx, by + 36, 5.95); dm_tc_end_at(&s, 0, 5.95); CHECK(s.belt_request == 2); CHECK(s.pulse_button == 0); s.belt_request = 0;
    dm_tc_begin_at(&s, 0, bx, by, 5.96); dm_tc_move_at(&s, 0, bx + 40, by, 5.97); dm_tc_end_at(&s, 0, 5.98); CHECK(!s.belt_request); CHECK(s.pulse_button == 0);
    CHECK(s.layout.target[DM_TC_BELT3].button == DM_BTN_DOWN && s.layout.target[DM_TC_BELT4].button == DM_BTN_RIGHT);
    s.belt_open = 1;
    dm_tc_begin_at(&s, 0, bx, by - 55.0f, 6.0); CHECK(s.buttons == 0); dm_tc_end_at(&s, 0, 6.05); dm_tc_tick(&s, 6.05 + 0.01); CHECK(s.belt_request == (0x100 | 1 << 4)); CHECK(s.pulse_button == 0); CHECK(s.world_tap_seq == 0); s.belt_request = 0;
    dm_tc_begin_at(&s, 0, bx + 40*3, by - (165.0f + 2*162.0f)/3, 7.0); dm_tc_end_at(&s, 0, 7.05); dm_tc_tick(&s, 7.05 + 0.01); CHECK(s.belt_request == (0x100 | 3 << 4 | 3)); s.belt_request = 0;
    dm_tc_begin_at(&s, 0, bx, by, 8.0); CHECK(!s.belt_request); dm_tc_end_at(&s, 0, 8.05); dm_tc_tick(&s, 8.05 + 0.01); CHECK(s.pulse_button == DM_BTN_LEFT); s.pulse_button = 0;
    CHECK(dm_tc_belt_outside(&s, 600, 150)); CHECK(!dm_tc_belt_outside(&s, bx, by)); CHECK(!dm_tc_belt_outside(&s, bx, by - 165.0f/3)); s.belt_open = 0; CHECK(!dm_tc_belt_outside(&s, 600, 150)); s.belt_open = 1;
    dm_tc_begin_at(&s, 0, 100, 250, 9.0); CHECK(s.belt_request == 2); CHECK(s.joy_slot == DM_TC_NONE); dm_tc_end_at(&s, 0, 9.05); CHECK(s.world_tap_seq == 0); CHECK(s.buttons == 0);
}




static void deferred_skill_vs_belt(void) {

    dm_tc_state s; dm_tc_init(&s, 667, 375); s.skill_set = 1; s.belt_rows = 4; s.last_time_s = 10.0;
    dm_tc_target k = s.layout.target[DM_TC_SKILL1], b = s.layout.target[DM_TC_BELT1];
    dm_tc_begin_at(&s, 0, k.x, k.y, 10.000); dm_tc_end_at(&s, 0, 10.050); CHECK(s.defer_button == k.button);
    dm_tc_begin_at(&s, 1, b.x, b.y, 10.080); dm_tc_end_at(&s, 1, 10.120);
    double skill = -1, belt = -1; int belt_zl = 0, zl_gap = 0;
    for (double t = 10.13; t < 11.0; t += 0.01) { dm_tc_tick(&s, t);
        if (skill < 0 && !(s.buttons & DM_BTN_ZL)) zl_gap = 1;
        if (s.pulse_button == k.button && skill < 0) { skill = t; dm_tc_tick(&s, t + 0.03); s.last_time_s = t + 0.03; CHECK(dm_tc_pulse_end(&s, s.pulse_epoch)); }
        else if (s.pulse_button == b.button && belt < 0) { belt = t; belt_zl = (s.buttons & DM_BTN_ZL) != 0; CHECK(dm_tc_pulse_end(&s, s.pulse_epoch)); } }
    CHECK(skill >= 10.000 + DM_TC_ZL_LEAD_S - 1e-9 && !zl_gap);
    CHECK(belt > skill && belt >= 10.120 && !belt_zl);
    dm_tc_state c; dm_tc_init(&c, 667, 375); c.skill_set = 1; c.last_time_s = 20.0;
    dm_tc_begin_at(&c, 0, k.x, k.y, 20.000); dm_tc_end_at(&c, 0, 20.050); CHECK(c.defer_button);
    dm_tc_cancel_all(&c); dm_tc_tick(&c, 20.30); CHECK(c.defer_button == 0 && c.pulse_button == 0 && c.buttons == 0);
}




static void held_skill2_vs_belt(void) {
    dm_tc_state s; dm_tc_init(&s, 667, 375); s.skill_set = 1; s.last_time_s = 30.0;
    dm_tc_target k = s.layout.target[DM_TC_SKILL1], b = s.layout.target[DM_TC_BELT1];
    dm_tc_begin_at(&s, 0, k.x, k.y, 30.000); for (double t = 30.01; t < 30.60; t += 0.01) dm_tc_tick(&s, t);
    CHECK((s.buttons & DM_BTN_ZL) && (s.buttons & k.button));
    dm_tc_begin_at(&s, 1, b.x, b.y, 30.600); dm_tc_end_at(&s, 1, 30.640);
    double belt = -1, back = -1, zl_back = -1, zl_off = -1; int bad_zl = 0, bad_key = 0;
    for (double t = 30.65; t < 31.60; t += 0.01) { dm_tc_tick(&s, t);
        if (belt < 0 && zl_off < 0 && !(s.buttons & DM_BTN_ZL)) { zl_off = t; CHECK(!(s.buttons & k.button)); }
        if (s.pulse_button == b.button) { if (belt < 0) belt = t; if (s.buttons & DM_BTN_ZL) bad_zl = 1; if (s.buttons & k.button) bad_key = 1;
            if (t - belt > 0.025) { CHECK(dm_tc_pulse_end(&s, s.pulse_epoch)); dm_tc_tick(&s, t); } }
        else if (belt >= 0) { if (zl_back < 0 && (s.buttons & DM_BTN_ZL)) zl_back = t; if (back < 0 && (s.buttons & k.button)) { back = t; CHECK(s.buttons & DM_BTN_ZL); } } }
    CHECK(belt >= 30.640 && !bad_zl && !bad_key);
    CHECK(zl_off > 0 && belt - zl_off >= DM_TC_ZL_LEAD_S - 0.011);
    CHECK(zl_back > belt && back - zl_back >= DM_TC_ZL_LEAD_S - 0.011);
    dm_tc_end_at(&s, 0, 31.60);
}

static void no_ui_drag_outside_modal(void) {
    for (int kind = 0; kind <= 6; kind += 6) {
        dm_tc_state s; dm_tc_init(&s, 667, 375); dm_tc_set_navigation_mode(&s, 1); s.mouse_click_mode = 1; s.last_time_s = 1.0;
        dm_tc_begin_at(&s, 0, 300, 100, 1.0); dm_tc_pointer_item(&s, kind);
        dm_tc_move_at(&s, 0, 300, 140, 1.05); dm_tc_move_at(&s, 0, 300, 200, 1.10);
        CHECK(s.finger[0].ui_drag == (kind == 6 ? 0 : 1));
    }
}




static void tap_belt(dm_tc_state *s, int slot, int target, double t0) {
    dm_tc_target t = s->layout.target[target]; dm_tc_begin_at(s, slot, t.x, t.y, t0); dm_tc_end_at(s, slot, t0 + 0.08);
}
static void belt_single_double(void) {
    for (int set = 0; set < 2; ++set) {
        dm_tc_state s; dm_tc_init(&s, 956, 440); s.skill_set = set; uint64_t k = s.layout.target[DM_TC_BELT2].button;

        double t = 10; dm_tc_tick(&s, t); dm_tc_begin_at(&s, 0, s.layout.target[DM_TC_BELT2].x, s.layout.target[DM_TC_BELT2].y, t); CHECK(s.buttons == 0);
        dm_tc_end_at(&s, 0, t + 0.08); CHECK(s.buttons == k);
        CHECK(dm_tc_pulse_end(&s, s.pulse_epoch)); CHECK(s.buttons == 0);

        t = 20; dm_tc_tick(&s, t); tap_belt(&s, 0, DM_TC_BELT2, t); CHECK(s.buttons == k); unsigned e1 = s.pulse_epoch; CHECK(dm_tc_pulse_end(&s, e1));
        tap_belt(&s, 1, DM_TC_BELT2, t + 0.12); CHECK(s.buttons == k && s.pulse_epoch != e1); CHECK(dm_tc_pulse_end(&s, s.pulse_epoch)); CHECK(s.buttons == 0);

        t = 30; dm_tc_tick(&s, t); tap_belt(&s, 0, DM_TC_BELT2, t); e1 = s.pulse_epoch; tap_belt(&s, 1, DM_TC_BELT2, t + 0.10); CHECK(s.pulse_epoch == e1 && s.buttons == k);
        CHECK(dm_tc_pulse_end(&s, e1)); CHECK(s.buttons == 0); dm_tc_tick(&s, t + 0.20); CHECK(s.buttons == k && s.pulse_epoch != e1); CHECK(dm_tc_pulse_end(&s, s.pulse_epoch));
        dm_tc_tick(&s, t + 1.0); CHECK(s.buttons == 0 && !s.pulse_button);

        t = 40; dm_tc_tick(&s, t); tap_belt(&s, 0, DM_TC_BELT3, t); CHECK(s.buttons == s.layout.target[DM_TC_BELT3].button); dm_tc_pulse_end(&s, s.pulse_epoch);

        s.belt_rows = 3; s.belt_open = 1; float u = s.layout.height / 1320.0f, x = s.layout.target[DM_TC_BELT2].x, y = s.layout.target[DM_TC_BELT2].y - 165 * u;
        t = 60; dm_tc_tick(&s, t); dm_tc_begin_at(&s, 0, x, y, t); CHECK(s.belt_request == (0x100 | 1 << 4 | 1)); CHECK(s.buttons == 0); dm_tc_end_at(&s, 0, t + 0.08); s.belt_request = 0;
        dm_tc_begin_at(&s, 0, x, y, t + 0.2); CHECK(s.belt_request == (0x100 | 1 << 4 | 1)); CHECK(s.buttons == 0); dm_tc_end_at(&s, 0, t + 0.3); s.belt_request = 0;
        dm_tc_tick(&s, t + 1); CHECK(s.belt_request == 0);
    }
}





static void stick_from_effect(void) {
    dm_tc_state s; dm_tc_init(&s,667,375);
    unsigned q = s.world_tap_seq;
    dm_tc_begin_stick_at(&s,0,20,120,10.0);
    CHECK(s.joy_slot==0 && s.finger[0].target==DM_TC_JOYSTICK); NEAR(s.joy_origin_x,68,0); NEAR(s.joy_origin_y,120,0); CHECK(s.lx==0 && s.ly==0);
    dm_tc_move_at(&s,0,80,120,10.05); CHECK(s.lx==DM_STICK_MAX && s.ly==0);
    dm_tc_move_at(&s,0,22,120,10.08); dm_tc_end_at(&s,0,10.1); CHECK(s.joy_slot==-1 && s.lx==0 && s.world_tap_seq==q);
    dm_tc_begin_at(&s,0,90,275,11.0); CHECK(s.joy_slot==0);
    dm_tc_begin_stick_at(&s,1,100,250,11.1); CHECK(s.joy_slot==0 && s.finger[1].active && s.finger[1].target==DM_TC_NONE && s.finger[1].no_tap);
    dm_tc_end_at(&s,1,11.2); CHECK(s.world_tap_seq==q); dm_tc_end_at(&s,0,12.0);
    dm_tc_begin_stick_at(&s,0,20,120,13.0); dm_tc_begin_stick_at(&s,0,40,140,13.1); CHECK(s.joy_slot==0); NEAR(s.joy_origin_y,120,0);
    dm_tc_end_at(&s,0,14.0);
    s.navigation_mode = 1; { unsigned b = s.buttons; dm_tc_begin_stick_at(&s,0,300,200,14.5); CHECK(s.finger[0].active && s.finger[0].target==DM_TC_DIFF && s.pointer_slot!=0 && s.joy_slot==-1);
      dm_tc_move_at(&s,0,400,200,14.6); dm_tc_end_at(&s,0,14.7); CHECK(s.buttons==b && !s.finger[0].active && s.world_tap_seq==q); } s.navigation_mode = 0;
    dm_tc_begin_stick_at(&s,-1,20,120,15.0); dm_tc_begin_stick_at(&s,DM_TC_MAX_TOUCH,20,120,15.0); dm_tc_begin_stick_at(&s,0,NAN,120,15.0); CHECK(s.joy_slot==-1 && !s.finger[0].active);
}
int main(void) {
    RUN(layout_667); RUN(layout_812); RUN(layout_956); RUN(layout_1024); RUN(layout_ipad169); RUN(layout_pixels); RUN(small_layout); RUN(difficulty_picker);
    RUN(hit_edges); RUN(key_hold_and_ownership); RUN(two_fingers_same_key); RUN(joystick_deadzone); RUN(joystick_clamp_956); RUN(stick_from_effect);
    RUN(joystick_axes); RUN(joystick_plus_skills); RUN(no_auto_handoff); RUN(background_no_key);
    RUN(circles_before_joystick); RUN(cancel_and_resize); RUN(deferred_skill_vs_belt); RUN(held_skill2_vs_belt); RUN(belt_single_double); RUN(no_ui_drag_outside_modal); RUN(invalid_slots_and_repeated_begin); RUN(all_slots_hold); RUN(enabled_targets); RUN(ui_scene_messages);
    RUN(pointer_tap); RUN(world_tap); RUN(pointer_outside_close); RUN(scene_dialog_only); RUN(pointer_item_tap); RUN(pointer_stack_tap); RUN(pointer_over_disabled_target); RUN(ui_drag_typing); RUN(pointer_drag_out); RUN(pointer_drag); RUN(pointer_long); RUN(pointer_long_then_drag); RUN(pointer_long_seq); RUN(pointer_shop_drag); RUN(pointer_control_hit); RUN(pointer_game_mode); RUN(pointer_pulse_queue); RUN(safe_insets); RUN(swap_key); RUN(skill_cluster_clearance);
    RUN(belt_expand);
    printf("PASS %d/%d cases; %d/%d checks\n",cases,cases,checks,checks); return 0;
}
