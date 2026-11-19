#include "dm_touch_controls.h"
#include "../../host/dm_input.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
static int checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr,"FAIL line=%d %s\n",__LINE__,#x); exit(1); } } while (0)
static void begin(dm_tc_state *s,int n,int target) { dm_tc_begin(s,n,s->layout.target[target].x,s->layout.target[target].y); if(target>=DM_TC_MAIN && target<=DM_TC_SKILL4) dm_tc_tick(s,s->last_time_s+0.1);                                                                                                                                                                                                                                                                  }
static void drag(dm_tc_state *s,int n,int target,float dx,float dy) { dm_tc_move(s,n,s->layout.target[target].x+dx,s->layout.target[target].y+dy); }
int main(void) {
    dm_tc_state s; dm_tc_init(&s,667,375);
    CHECK(s.rx==0 && s.ry==0 && s.aim_slot==DM_TC_NONE && !s.navigation_mode && !s.map_cancel_active);
    begin(&s,0,DM_TC_MAIN); CHECK(s.buttons==DM_BTN_A && s.rx==0 && s.ry==0);

    drag(&s,0,DM_TC_MAIN,6,4); CHECK(s.buttons==DM_BTN_A && s.rx==0 && s.ry==0);
    drag(&s,0,DM_TC_MAIN,-8,-5); CHECK(s.buttons==DM_BTN_A && s.rx==0 && s.ry==0);
    drag(&s,0,DM_TC_MAIN,12,0); CHECK(s.rx>0 && s.ry==0);
    drag(&s,0,DM_TC_MAIN,60,0); CHECK(s.rx==32767 && s.ry==0 && s.aim_slot==0);
    drag(&s,0,DM_TC_MAIN,0,60); CHECK(s.rx==0 && s.ry==-32767);
    drag(&s,0,DM_TC_MAIN,30,-30); CHECK(s.rx==16384 && s.ry==16384);
    drag(&s,0,DM_TC_MAIN,120,-120); CHECK(s.rx==32767 && s.ry==32767);
    drag(&s,0,DM_TC_MAIN,0,0); CHECK(s.rx==0 && s.ry==0 && s.aim_slot==0);
    dm_tc_begin(&s,1,90,275); dm_tc_move(&s,1,150,275);
    drag(&s,0,DM_TC_MAIN,60,0); CHECK(s.lx==32767 && s.ly==0 && s.rx==32767 && s.ry==0);

    begin(&s,2,DM_TC_SKILL1); CHECK(s.buttons==(DM_BTN_A|DM_BTN_B));
    drag(&s,2,DM_TC_SKILL1,0,60); CHECK(s.lx==32767 && s.rx==0 && s.ry==-32767 && s.aim_slot==2 && s.buttons==(DM_BTN_A|DM_BTN_B));
    dm_tc_tick(&s,s.last_time_s+0.1); dm_tc_end(&s,2); CHECK(s.buttons==DM_BTN_A && s.rx==32767 && s.ry==0 && s.aim_slot==0 && !s.pulse_button);
    begin(&s,2,DM_TC_MAIN);
    dm_tc_target map=s.layout.target[DM_TC_MAP];
    dm_tc_move(&s,2,map.x+map.r,map.y-map.r);
    CHECK(s.map_cancel_active && s.buttons==DM_BTN_A && s.rx==32767 && s.lx==32767);
    dm_tc_move(&s,2,map.x-map.r-1,map.y); CHECK(!s.map_cancel_active && s.buttons==DM_BTN_A && s.aim_slot==0);
    dm_tc_end(&s,0); CHECK(s.buttons==0 && s.rx==0 && s.ry==0 && s.aim_slot==DM_TC_NONE && s.lx==32767);
    drag(&s,2,DM_TC_MAIN,60,0); CHECK(s.buttons==0 && s.rx==0 && s.ry==0);
    dm_tc_end(&s,2); CHECK(!s.map_cancel_active);
    begin(&s,0,DM_TC_SKILL2); drag(&s,0,DM_TC_SKILL2,60,0);
    dm_tc_move(&s,0,map.x,map.y); CHECK(s.map_cancel_active && s.buttons==0 && s.rx==0 && s.ry==0 && s.lx==32767);
    dm_tc_end(&s,0); CHECK(!s.map_cancel_active && s.lx==32767 && s.buttons==0 && !s.pulse_button);
    begin(&s,0,DM_TC_MAIN); drag(&s,0,DM_TC_MAIN,60,0);
    dm_tc_set_navigation_mode(&s,1); CHECK(s.navigation_mode && s.buttons==0 && s.lx==0 && s.rx==0 && s.ry==0);
    begin(&s,0,DM_TC_MAIN); drag(&s,0,DM_TC_MAIN,60,0); CHECK(s.buttons==DM_BTN_A && s.rx==0 && s.ry==0);
    dm_tc_move(&s,0,map.x,map.y); CHECK(!s.map_cancel_active && s.buttons==DM_BTN_A);
    dm_tc_set_navigation_mode(&s,1); CHECK(s.buttons==DM_BTN_A);
    dm_tc_set_navigation_mode(&s,0); CHECK(!s.navigation_mode && s.buttons==0 && s.rx==0 && s.ry==0);
    begin(&s,0,DM_TC_SKILL3); drag(&s,0,DM_TC_SKILL3,-60,0); CHECK(s.buttons==DM_BTN_Y && s.rx==-32767 && s.ry==0);
    dm_tc_move(&s,0,NAN,0); CHECK(s.buttons==DM_BTN_Y && s.rx==-32767); dm_tc_tick(&s,s.last_time_s+0.1); dm_tc_end(&s,0); CHECK(s.buttons==0 && s.rx==0 && s.ry==0 && !s.pulse_button);
    dm_tc_cancel_all(&s); CHECK(s.buttons==0 && s.rx==0 && s.ry==0 && s.aim_slot==DM_TC_NONE && !s.map_cancel_active && !s.pulse_button);
    begin(&s,0,DM_TC_SKILL4); drag(&s,0,DM_TC_SKILL4,5,-5);
    CHECK(s.buttons==DM_BTN_R && s.rx==0 && s.ry==0);
    dm_tc_tick(&s,s.last_time_s+0.1); dm_tc_end(&s,0); CHECK(s.buttons==0);
    begin(&s,0,DM_TC_SKILL4); drag(&s,0,DM_TC_SKILL4,0,-60);
    dm_tc_resize(&s,333.5f,187.5f); CHECK(s.rx==0 && s.ry==0 && s.buttons==0 && s.aim_slot==DM_TC_NONE && !s.pulse_button);
    begin(&s,0,DM_TC_MAIN); drag(&s,0,DM_TC_MAIN,15,-15); CHECK(s.rx==16384 && s.ry==16384);
    dm_tc_enable_targets(&s,0); CHECK(s.rx==0 && s.ry==0 && s.buttons==0);
    dm_tc_enable_targets(&s,(1u<<DM_TC_COUNT)-1);
    const int non_skill[]={DM_TC_PORTAL,DM_TC_BELT1,DM_TC_MENU,DM_TC_MAP};
    for(unsigned i=0;i<sizeof(non_skill)/sizeof(*non_skill);i++) {
        begin(&s,0,non_skill[i]); drag(&s,0,non_skill[i],30,-30); CHECK(s.rx==0 && s.ry==0 && s.aim_slot==DM_TC_NONE); dm_tc_end(&s,0);
    }
    dm_tc_resize(&s,667,375); dm_tc_set_portal_binding(&s,DM_BTN_ZL|DM_BTN_B,77,3); begin(&s,3,DM_TC_PORTAL);
    begin(&s,0,DM_TC_MAIN); drag(&s,0,DM_TC_MAIN,60,0);
    CHECK(s.buttons==(DM_BTN_ZL|DM_BTN_A) && s.rx==32767);
    CHECK(dm_tc_portal_ack(&s,s.gesture_epoch,77,3)); CHECK(s.buttons==(DM_BTN_ZL|DM_BTN_B|DM_BTN_A) && s.rx==32767);
    dm_tc_end(&s,0); CHECK(s.buttons==(DM_BTN_ZL|DM_BTN_B) && s.rx==0); dm_tc_end(&s,3); CHECK(s.buttons==0);
    dm_tc_target main=s.layout.target[DM_TC_MAIN];
    dm_tc_begin(&s,0,main.x+10,main.y+10);
    dm_tc_move(&s,0,main.x+40,main.y-20); CHECK(s.rx==16384 && s.ry==16384);
    dm_tc_move(&s,0,map.x,map.y); CHECK(s.map_cancel_active);
    dm_tc_cancel_all(&s); CHECK(!s.map_cancel_active && s.rx==0 && s.ry==0 && s.aim_slot==DM_TC_NONE);
    {
        dm_tc_state t; dm_tc_init(&t,667,375); dm_tc_target k=t.layout.target[DM_TC_SKILL2];
        dm_tc_tick(&t,1.00); dm_tc_begin_at(&t,0,k.x,k.y,1.00); CHECK(t.buttons==0);
        dm_tc_move_at(&t,0,k.x+60,k.y,1.03); CHECK(t.buttons==DM_BTN_X && t.rx==32767 && t.ry==0);
        dm_tc_tick(&t,1.40); dm_tc_move_at(&t,0,k.x,k.y-60,1.40); CHECK(t.buttons==DM_BTN_X && t.rx==0 && t.ry==32767);
        dm_tc_end_at(&t,0,2.00); CHECK(t.buttons==0 && !t.pulse_button);
        dm_tc_tick(&t,3.00); dm_tc_begin_at(&t,0,k.x,k.y,3.00); dm_tc_end_at(&t,0,3.05); CHECK(t.pulse_button==DM_BTN_X && t.buttons==DM_BTN_X && t.rx==0);
        CHECK(dm_tc_pulse_end(&t,t.pulse_epoch) && t.buttons==0);
        dm_tc_tick(&t,4.00); dm_tc_begin_at(&t,0,k.x,k.y,4.00); dm_tc_tick(&t,4.10); CHECK(t.buttons==DM_BTN_X && t.rx==0);
        dm_tc_end_at(&t,0,4.30); CHECK(t.buttons==0 && !t.pulse_button);
        dm_tc_target mn=t.layout.target[DM_TC_MAIN];
        dm_tc_tick(&t,5.00); dm_tc_begin_at(&t,0,mn.x,mn.y,5.00); CHECK(t.buttons==DM_BTN_A); dm_tc_end_at(&t,0,5.03); CHECK(t.buttons==0 && !t.pulse_button);
        dm_tc_tick(&t,6.00); dm_tc_begin_at(&t,0,k.x,k.y,6.00); dm_tc_tick(&t,6.09); CHECK(t.buttons==DM_BTN_X); dm_tc_end_at(&t,0,6.10); CHECK(t.pulse_button==DM_BTN_X && t.buttons==DM_BTN_X);
        CHECK(dm_tc_pulse_end(&t,t.pulse_epoch) && t.buttons==0);
    }
    {
        dm_tc_state t; dm_tc_init(&t,667,375); dm_tc_target w=t.layout.swap;
        CHECK(dm_tc_hit(&t.layout,w.x,w.y)==DM_TC_SWAP && t.skill_set==0);
        dm_tc_begin(&t,0,w.x,w.y); CHECK(t.skill_set==0 && t.buttons==0); dm_tc_end(&t,0); CHECK(t.skill_set==1 && t.buttons==0);
        dm_tc_target k=t.layout.target[DM_TC_SKILL3], mn=t.layout.target[DM_TC_MAIN], mu=t.layout.target[DM_TC_MENU];
        dm_tc_tick(&t,10.0); dm_tc_begin_at(&t,1,k.x,k.y,10.0); CHECK(t.buttons==DM_BTN_ZL);
        dm_tc_tick(&t,10.3); CHECK(t.buttons==(DM_BTN_ZL|DM_BTN_Y));
        dm_tc_end_at(&t,1,10.4); CHECK(t.buttons==0 && t.pulse_button==0);
        dm_tc_tick(&t,11.0); dm_tc_begin_at(&t,1,k.x,k.y,11.0); dm_tc_end_at(&t,1,11.05); CHECK(t.buttons==DM_BTN_ZL && t.pulse_button==0);
        dm_tc_tick(&t,11.1); CHECK(t.buttons==DM_BTN_ZL); dm_tc_tick(&t,11.18); CHECK(t.buttons==(DM_BTN_ZL|DM_BTN_Y));
        dm_tc_pulse_end(&t,t.pulse_epoch); CHECK(t.buttons==0);
        dm_tc_tick(&t,12.0); dm_tc_begin_at(&t,1,mn.x,mn.y,12.0); CHECK(t.buttons==DM_BTN_ZL);
        dm_tc_tick(&t,12.1); CHECK(t.buttons==DM_BTN_ZL); dm_tc_tick(&t,12.2); CHECK(t.buttons==(DM_BTN_ZL|DM_BTN_A));
        dm_tc_tick(&t,13.0); CHECK(t.buttons==(DM_BTN_ZL|DM_BTN_A)); dm_tc_end_at(&t,1,13.1); CHECK(t.buttons==0);
        dm_tc_begin_at(&t,1,mu.x,mu.y,14.0); CHECK(t.buttons==DM_BTN_PLUS); dm_tc_end_at(&t,1,14.1); CHECK(t.buttons==0);
        { dm_tc_target bl=t.layout.target[DM_TC_BELT1]; dm_tc_tick(&t,15.0); dm_tc_begin_at(&t,1,bl.x,bl.y,15.0); CHECK(t.buttons==0); dm_tc_end_at(&t,1,15.05);
          dm_tc_tick(&t,15.05+0.01); CHECK(t.buttons==DM_BTN_LEFT); dm_tc_pulse_end(&t,t.pulse_epoch); CHECK(t.buttons==0); }
        dm_tc_cancel_all(&t); CHECK(t.skill_set==1 && t.buttons==0);
        dm_tc_resize(&t,812,375); CHECK(t.skill_set==1);
        dm_tc_set_navigation_mode(&t,1); begin(&t,0,DM_TC_MAIN); CHECK(t.buttons==DM_BTN_A); dm_tc_end(&t,0);
        dm_tc_begin(&t,0,t.layout.swap.x,t.layout.swap.y); CHECK(t.skill_set==1 && t.buttons==0); dm_tc_end(&t,0);
        dm_tc_set_navigation_mode(&t,0); w=t.layout.swap;
        dm_tc_begin(&t,0,w.x,w.y); dm_tc_end(&t,0); CHECK(t.skill_set==0 && t.buttons==0);
    }
    printf("PASS skill drag axes/ownership/map-lock/navigation/resize/Portal coexistence/skill-set swap %d/%d checks\n",checks,checks);
    return 0;
}
