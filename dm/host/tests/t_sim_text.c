#include "../dm_sim_text.h"
#include <assert.h>

int main(void) {
    char text[160],number[32];
    dm_sim_number(number,13.3,1);assert(!strcmp(number,"13,3"));
    dm_sim_number(number,13.3,0);assert(!strcmp(number,"13.3"));
    assert(!dm_sim_delta_text(text,sizeof text,"Rotation",13.31,13.34,1," DPS"));
    assert(!dm_sim_delta_text(text,sizeof text,"Passive",0,0,1," DPS"));
    assert(dm_sim_delta_text(text,sizeof text,"Normaler Angriff",999.9,1234.5,1," DPS")>0);assert(text[3]=='3');puts(text+4);
    assert(dm_sim_skill_delta_text(text,sizeof text,10.8,12.5,1)>0);puts(text+4);
    assert(dm_sim_skill_delta_text(text,sizeof text,12.5,10.8,0)>0);assert(text[3]=='1');puts(text+4);
    assert(dm_sim_delta_text(text,sizeof text,"Überleben",599.8,599.9,1," s")>0);puts(text+4);
    assert(dm_sim_delta_text(text,sizeof text,"Treffer bis Tod",100,101,1,"")>0);
    assert(dm_sim_delta_text(text,sizeof text,"Mana reicht für",2.2,3.4,1," s")>0);puts(text+4);
    assert(dm_sim_skill_delta_text(text,sizeof text,99999.9,100000.0,1)>0);puts(text+4);
    assert(dm_sim_skill_delta_text(text,sizeof text,99999.9,100000.0,0)>0);puts(text+4);
    assert(dm_sim_skill_first_text(text,sizeof text,2.84,1)>0);assert(!strcmp(text+4,"Erster Punkt: 2,8 Schaden/s") && text[3]=='3');
    assert(!dm_sim_skill_first_text(text,sizeof text,.04,1) && !dm_sim_skill_first_text(text,sizeof text,0,0));
    assert(dm_sim_heading_text(text,sizeof text,2,1)>0 && !strcmp(text+4,"Kampfsimulation · Standardgegner Akt 2") && text[3]=='0');
    assert(dm_sim_heading_text(text,sizeof text,5,0)>0 && !strcmp(text+4,"Combat Sim · standard enemy Act 5"));
    assert(dm_sim_heading_text(text,sizeof text,0,1)>0 && !strcmp(text+4,"Kampfsimulation · Standardgegner"));
    return 0;
}
