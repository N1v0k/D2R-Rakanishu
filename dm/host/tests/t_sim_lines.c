


#define _POSIX_C_SOURCE 200809L
#include "sim_test_io.h"
#include "../dm_sim_core.h"
#include "../dm_sim_text.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static void show(const char *line){if(*line)puts(line+4);}
int main(int argc,char **argv) {
    char line[240];
    if(argc==2 && !strcmp(argv[1],"ids")) {
        const char *path=getenv("DM_SIM_DATA");assert(path);size_t n;char *input=sim_read(path,&n);assert(input);
        dm_sim_data d={0};assert(!dm_sim_load(&d,input,n,"rotw"));free(input);
        for(int id=1;id<DM_SIM_MAXSKILL;id++)if(d.skill[id] && !(d.skill[id]->fam&(DM_FAM_MODIFIER|DM_FAM_U)))printf("%d\n",id);
        dm_sim_free(&d);return 0;
    }
    assert(argc>=3 && !strcmp(argv[1],"lines"));
    int german=atoi(argv[2]);
    const double wide=19999.9,wider=29999.9;
    for(int act=0;act<=5;act++){dm_sim_heading_text(line,sizeof line,act,german);show(line);}
    dm_sim_skill_delta_text(line,sizeof line,wide,wider,german);show(line);
    dm_sim_skill_first_text(line,sizeof line,wider,german);show(line);
    return 0;
}
